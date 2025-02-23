/* InterSpec: an application to analyze spectral gamma radiation data.
 
 Copyright 2018 National Technology & Engineering Solutions of Sandia, LLC
 (NTESS). Under the terms of Contract DE-NA0003525 with NTESS, the U.S.
 Government retains certain rights in this software.
 For questions contact William Johnson via email at wcjohns@sandia.gov, or
 alternative emails of interspec@sandia.gov.
 
 This library is free software; you can redistribute it and/or
 modify it under the terms of the GNU Lesser General Public
 License as published by the Free Software Foundation; either
 version 2.1 of the License, or (at your option) any later version.
 
 This library is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 Lesser General Public License for more details.
 
 You should have received a copy of the GNU Lesser General Public
 License along with this library; if not, write to the Free Software
 Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include "InterSpec_config.h"

#include <memory>
#include <iostream>

#include <boost/math/constants/constants.hpp>
#include <boost/math/special_functions/erf.hpp>
#include <boost/math/distributions/poisson.hpp>

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_utils.hpp"
#include "rapidxml/rapidxml_print.hpp"

#include "InterSpec/PeakDef.h"
#include "InterSpec/PeakFit.h"
#include "InterSpec/InterSpecApp.h"
#include "InterSpec/WarningWidget.h"
#include "InterSpec/PhysicalUnits.h"
#include "SandiaDecay/SandiaDecay.h"
#include "SpecUtils/UtilityFunctions.h"
#include "SpecUtils/SpectrumDataStructs.h"

using namespace std;

const int PeakDef::sm_xmlSerializationVersion = 0;
const int PeakContinuum::sm_xmlSerializationVersion = 0;

namespace
{
  //clones 'source' into the document that 'result' is a part of.
  //  'result' is cleared and set lexically equal to 'source'.
  void clone_node_deep( const ::rapidxml::xml_node<char> *source,
                        ::rapidxml::xml_node<char> *result )
  {
    using namespace ::rapidxml;
    
    xml_document<char> *doc = result->document();
    if( !doc )
      throw runtime_error( "clone_node_deep: insert result into document before calling" );
    
    result->remove_all_attributes();
    result->remove_all_nodes();
    result->type(source->type());
    
    
    // Clone name and value
    char *str = doc->allocate_string( source->name(), source->name_size() );
    result->name( str, source->name_size());
    
    if( source->value() )
    {
      str = doc->allocate_string( source->value(), source->value_size() );
      result->value( str, source->value_size() );
    }
    
    // Clone child nodes and attributes
    for( xml_node<char> *child = source->first_node(); child; child = child->next_sibling() )
    {
      xml_node<char> *clone = doc->allocate_node( child->type() );
      result->append_node( clone );
      clone_node_deep( child, clone );
    }
    
    for( xml_attribute<char> *attr = source->first_attribute(); attr; attr = attr->next_attribute())
    {
      const char *name = doc->allocate_string( attr->name(), attr->name_size() );
      const char *value = doc->allocate_string( attr->value(), attr->value_size() );
      xml_attribute<char> *clone = doc->allocate_attribute(name, value, attr->name_size(), attr->value_size());
      result->append_attribute( clone );
    }
  }//void clone_node_deep(...)
  
  
  bool gives_off_gammas( const SandiaDecay::Nuclide *nuc )
  {
    if( !nuc )
      return false;
      
    for( const SandiaDecay::Transition *t : nuc->decaysToChildren )
    {
      for( const SandiaDecay::RadParticle &p : t->products )
      {
        if( p.type == SandiaDecay::GammaParticle )
          return true;
      }
        
      if( gives_off_gammas( t->child ) )
        return true;
    }//for( size_t decN = 0; decN < decaysToChildren.size(); ++decN )
      
    return false;
  }//bool gives_off_gammas( const SandiaDecay::Nuclide *nuc )
  
  
  double landau_cdf(double x, double xi, double x0) {
    // implementation of landau distribution (from DISLAN)
    //The algorithm was taken from the Cernlib function dislan(G110)
    //Reference: K.S.Kolbig and B.Schorr, "A program package for the Landau
    //distribution", Computer Phys.Comm., 31(1984), 97-111
    //
    //Lifted from the root/math/mathcore/src/ProbFuncMathCore.cxx file
    //  by wcjohns 20120216
    
    static double p1[5] = {0.2514091491e+0,-0.6250580444e-1, 0.1458381230e-1, -0.2108817737e-2, 0.7411247290e-3};
    static double q1[5] = {1.0            ,-0.5571175625e-2, 0.6225310236e-1, -0.3137378427e-2, 0.1931496439e-2};
    
    static double p2[4] = {0.2868328584e+0, 0.3564363231e+0, 0.1523518695e+0, 0.2251304883e-1};
    static double q2[4] = {1.0            , 0.6191136137e+0, 0.1720721448e+0, 0.2278594771e-1};
    
    static double p3[4] = {0.2868329066e+0, 0.3003828436e+0, 0.9950951941e-1, 0.8733827185e-2};
    static double q3[4] = {1.0            , 0.4237190502e+0, 0.1095631512e+0, 0.8693851567e-2};
    
    static double p4[4] = {0.1000351630e+1, 0.4503592498e+1, 0.1085883880e+2, 0.7536052269e+1};
    static double q4[4] = {1.0            , 0.5539969678e+1, 0.1933581111e+2, 0.2721321508e+2};
    
    static double p5[4] = {0.1000006517e+1, 0.4909414111e+2, 0.8505544753e+2, 0.1532153455e+3};
    static double q5[4] = {1.0            , 0.5009928881e+2, 0.1399819104e+3, 0.4200002909e+3};
    
    static double p6[4] = {0.1000000983e+1, 0.1329868456e+3, 0.9162149244e+3, -0.9605054274e+3};
    static double q6[4] = {1.0            , 0.1339887843e+3, 0.1055990413e+4, 0.5532224619e+3};
    
    static double a1[4] = {0, -0.4583333333e+0, 0.6675347222e+0,-0.1641741416e+1};
    
    static double a2[4] = {0,  1.0            ,-0.4227843351e+0,-0.2043403138e+1};
    
    double v = (x - x0)/xi;
    double u;
    double lan;
    
    if (v < -5.5) {
      u = std::exp(v+1);
      lan = 0.3989422803*std::exp(-1./u)*std::sqrt(u)*(1+(a1[1]+(a1[2]+a1[3]*u)*u)*u);
    }
    else if (v < -1 ) {
      u = std::exp(-v-1);
      lan = (std::exp(-u)/std::sqrt(u))*(p1[0]+(p1[1]+(p1[2]+(p1[3]+p1[4]*v)*v)*v)*v)/
      (q1[0]+(q1[1]+(q1[2]+(q1[3]+q1[4]*v)*v)*v)*v);
    }
    else if (v < 1)
      lan = (p2[0]+(p2[1]+(p2[2]+p2[3]*v)*v)*v)/(q2[0]+(q2[1]+(q2[2]+q2[3]*v)*v)*v);
    else if (v < 4)
      lan = (p3[0]+(p3[1]+(p3[2]+p3[3]*v)*v)*v)/(q3[0]+(q3[1]+(q3[2]+q3[3]*v)*v)*v);
    else if (v < 12) {
      u = 1./v;
      lan = (p4[0]+(p4[1]+(p4[2]+p4[3]*u)*u)*u)/(q4[0]+(q4[1]+(q4[2]+q4[3]*u)*u)*u);
    }
    else if (v < 50) {
      u = 1./v;
      lan = (p5[0]+(p5[1]+(p5[2]+p5[3]*u)*u)*u)/(q5[0]+(q5[1]+(q5[2]+q5[3]*u)*u)*u);
    }
    else if (v < 300) {
      u = 1./v;
      lan = (p6[0]+(p6[1]+(p6[2]+p6[3]*u)*u)*u)/(q6[0]+(q6[1]+(q6[2]+q6[3]*u)*u)*u);
    }
    else {
      u = 1./(v-v*std::log(v)/(v+1));
      lan = 1-(a2[1]+(a2[2]+a2[3]*u)*u)*u;
    }
    
    return lan;
  }//double landau_cdf(double x, double xi, double x0)

}//namespace


double skewedGaussianIntegral( double x0, double x1,
                               double mu, double s,
                               double L )
{
  using boost::math::erf;
  using boost::math::erfc;
  static const double sqrt2 = boost::math::constants::root_two<double>();
  
  return -0.5*erf((x0 - mu)/(sqrt2*s)) + erf((x1 - mu)/(sqrt2*s))
   + exp((L*(L*s*s - 2*x0 + 2*mu))/2)*erfc((L*s*s - x0 + mu)/(sqrt2*s))
  - exp((L*(L*s*s - 2*x1 + 2*mu))/2) *erfc((L*s*s - x1 + mu)/(sqrt2*s));
}//double skewedGaussianIntegral(...)

double skewedGaussianIndefinitIntegral( double x,
                              double A, double c,
                              double w, double t )
{
  using boost::math::erf;
  using boost::math::erfc;
  static const double sqrt2 = boost::math::constants::root_two<double>();
  
//  integral (A e^(1/2 (w/t)^2-(x-c)/t) (1/2+1/2 erf(((x-c)/w-w/t)/sqrt(2))))/t dx =
  return -0.5*A*exp(-x/t)*(exp((2*c*t+w*w)/(2*t*t))*(erf(((x-c)/w-w/t)/sqrt2)+1)-exp(x/t)*erf((x-c)/(sqrt2*w)));
}

double skewedGaussianIntegral( double x0, double x1,
                              double A, double c,
                              double w, double t )
{
  return skewedGaussianIndefinitIntegral( x1, A, c, w, t ) - skewedGaussianIndefinitIntegral( x0, A, c, w, t );
}


void findROIEnergyLimits( double &lowerEnengy, double &upperEnergy,
                         const PeakDef &peak, const std::shared_ptr<const Measurement> &data )
{
  std::shared_ptr<const PeakContinuum> continuum = peak.continuum();
  if( continuum->energyRangeDefined() )
  {
    lowerEnengy  = continuum->lowerEnergy();
    upperEnergy = continuum->upperEnergy();
    return;
  }//if( continuum->energyRangeDefined() )
  
  if( !data )
  {
    lowerEnengy = peak.lowerX();
    upperEnergy = peak.upperX();
    return;
  }//if( !data )
  
  const size_t lowbin = findROILimit( peak, data, false );
  const size_t upbin  = findROILimit( peak, data, true );
  lowerEnengy = data->gamma_channel_lower( lowbin );
  upperEnergy = data->gamma_channel_upper( upbin );
}//void findROIEnergyLimits(...)


#define PRINT_ROI_DEBUG_INFO 0

#if( PRINT_ROI_DEBUG_INFO )
namespace
{
  class DebugLog
  {
    //Make it so cout/cerr statments always end up non-interleaved when multiple
    // threads are calling cout/cerr
  public:
    explicit DebugLog( std::ostream &os ) : os(os) {}
    ~DebugLog() { os << ss.rdbuf() << std::flush; }
    template <typename T>
    DebugLog& operator<<(T const &t){ ss << t; return *this;}
  private:
    std::ostream &os;
    std::stringstream ss;
  };
}//namespace
#endif


size_t findROILimit( const PeakDef &peak, const std::shared_ptr<const Measurement> &dataH, bool high )
{
  if( !peak.gausPeak() )
    return dataH->find_gamma_channel( (high ? peak.upperX() : peak.lowerX()) );
  

  
  //This implemntation is an adaptation of how PCGAP defines a region of
  //  interest.
  //  The basic idea is to include a maximum of 11.75 sigma away from the mean
  //  of the peak, but then start at ~1.5 sigma from mean, and try to detect if
  //  a new feature is occuring, and if so, stop the region of interest there.
  //  A feature is "detected" if the value of the bin contents exceeds 2.5 sigma
  //  from the "expected" value, where the expected value starts off being the
  //  smallest bin value so far (well, this bin averaged with the bins on either
  //  side of it), and then each preceeding bin is added to this background
  //  value.
  //
  //References for PCGAP are at:
  //  http://www.inl.gov/technicalpublications/Documents/3318133.pdf
  //  http://www.osti.gov/bridge/servlets/purl/800710-Zc4iYJ/native/800710.pdf
  
  //My _guess_ on how peak easy identifies the region of interest, _not_
  //  how this function does it.
  //  1) Default to +- between 7.0 and 7.5 sigma
  //  2) Starting at ~2.0 sigma from mean, go through and find if there is
  //     a 'new' feature, and if so, stop the background there.
  //     A new feature may be indicated by a statistically significant
  //     increase in counts (relative to background).
  //  3) Find out if the lower energy tail area should be increased.
  //     not sure on this, up to about 11.5 sigma from the mean
  
  
  //20150108: wcjohns converted this function from using ROOT style accessors
  //  (GetBinContent(bin)) to the SpectrumDataStructs style accessors
  //  (gamma_channel_content(channel)), and using unsigned integers seems to
  //  work okay, but he didnt carefully check for wrap-around or other issues,
  //  so is leaving the indexing type as int for now
  typedef int indexing_t;
  //  typedef size_t indexing_t;
  
  const indexing_t nchannel = (!dataH ? size_t(0): dataH->num_gamma_channels());
  
  if( nchannel<128 )
    throw runtime_error( "findROILimit(...): Invalid input" );
  
  const bool highres = (nchannel > 3000);
  
  const vector<float> &contents = *dataH->gamma_channel_contents();
  
  
  std::shared_ptr<const PeakContinuum> continuum = peak.continuum();
  double lowxrange  = continuum->lowerEnergy();
  double highxrange = continuum->upperEnergy();
  
  const bool definedRange = (lowxrange!=highxrange);
  
  if( definedRange )
  {
    if( high )
      return dataH->find_gamma_channel( highxrange-0.00001 );
    else
      return dataH->find_gamma_channel( lowxrange );
  }//if( definedRange )
  
  const double mean = peak.mean();
  const double sigma = peak.sigma();
  
  const int direction = high ? 1 : -1;
  lowxrange = mean + direction*7.5*sigma;//11.75*sigma;
  
  //Need to add a mechanism to try and shrink the xrange to only ~7.5 sigma from mean
  //  when there isnt a lot of ambiguity to the continuum.
  //(FWHM=2.35*sigma)
  //May extend to 18.75 sigma (8 FWHM)
  
  const indexing_t nSideChannel = 1;
  
  indexing_t startchannel = dataH->find_gamma_channel( mean + direction*1.5*sigma );
  if( !high && startchannel < nSideChannel )
    startchannel = nSideChannel;
  
  indexing_t minChannel = startchannel;
  double minVal = contents[startchannel];
  indexing_t nbackbin = 1 + 2*nSideChannel;
  float backval = dataH->gamma_channels_sum( startchannel - nSideChannel,
                                             startchannel + nSideChannel );
  backval = std::max( backval, static_cast<float>(nbackbin) );
  
  const indexing_t meanchannel = dataH->find_gamma_channel( mean );
  indexing_t lastchannel  = dataH->find_gamma_channel( lowxrange + direction*0.0001 );
  
  //Make sure were not looking to far and that loop will terminate
  if( high ) lastchannel = std::min( lastchannel, nchannel-2 );
  else       lastchannel = std::max( lastchannel, indexing_t(1) );
  if( (high && (startchannel > lastchannel)) || (!high && (startchannel < lastchannel)) )
    startchannel = lastchannel;
  if( high || lastchannel>=direction )
    lastchannel += direction;
  
  
#if( PRINT_ROI_DEBUG_INFO )
  const bool debug_this_peak = false; //(fabs(peak.mean() - 1155.33) < 1.0) && direction>0;
  const vector<float> &energies = *dataH->gamma_channel_energies();
  
  if( debug_this_peak )
    DebugLog(cerr) << "\n\n\n\nTo start with, lowxrange=" << lowxrange << " lastbin="
    << lastchannel << ", x(lastbin)=" << energies[lastchannel]
    << " for peak at mean=" << mean << ", sigma=" << sigma << "\n";
#endif
  
  //Lets find the bin with the smallest contents, and
  for( indexing_t channel = startchannel + direction*nSideChannel;
       channel != lastchannel && channel>nSideChannel; channel += direction )
  {
    assert( channel < (dataH->num_gamma_channels() + 100) );
    const float val = contents[channel];
    
    if( val <= minVal && (!highres || (contents[channel+direction] <= minVal)) )
    {
      minVal = val;
      minChannel = channel;
      nbackbin = 1 + 2*nSideChannel;
      backval = dataH->gamma_channels_sum( channel-nSideChannel,
                                           channel+nSideChannel );
      backval = std::max( backval, static_cast<float>(nbackbin) );
      channel += direction;
      if( channel == lastchannel )
        break;
#if( PRINT_ROI_DEBUG_INFO )
      if( debug_this_peak )
        DebugLog(cerr) << "FindMin: New background val at " << energies[channel]
        << ", backval/nbackbin=" << backval/nbackbin << "\n";
#endif
    }else
    {
      //If val is greater than backval, we _may_ be hitting a new feature, like
      //  a new peak, if we are, lets stop going any further away from the mean.
      //  Not detecting this may cause us to extend _past_ the new feature and
      //  find a global minimum we clearly dount want
      
      float background = backval / nbackbin;
      float background_sigma = sqrt(backval) / nbackbin;
      
      //For low resolution spectra, lets estimate the slope of the continuum, and
      //  use this to correct maximum_allowable number of counts; without doing this
      //  the lower ROI range for peaks on a falling continuum can be much to short.
      //  (not tested for HPGe)

      if( (nbackbin > 2) && !highres && channel > nbackbin )
      {
        try
        {
          const float *x = &(*dataH->channel_energies())[0];
          const float *y = &(*dataH->gamma_counts())[0];
          x = x + channel - direction - ((direction < 0) ? 0 : nbackbin);
          y = y + channel - direction - ((direction < 0) ? 0 : nbackbin);
          
          vector<double> coeffs, uncerts;
          fit_to_polynomial( x, y, nbackbin, 1, coeffs, uncerts );
          
          const float thisx = ((direction < 0) ? x[direction] : x[nbackbin+1]);
          background = std::max( coeffs[0] + thisx*coeffs[1], 1.0*nbackbin );
        }catch(...)
        {
        }
      }//if( nbackbin > 2 )

      const float sigma = sqrt( background_sigma*background_sigma + background );
      //      double max_allowable = std::ceil(background + 2.575829*sigma) + 0.001;
      float max_allowable = std::ceil(background + 3.0f*sigma) + 0.001f;
      
      if( background < 20.0f )
      {
        //boost::math::quantile shows up pretty significantly on the profiler,
        //  so we'll reduce the accuracy a bit.  I've arbitrarily selected a
        //  precision of 6 decimal places, even though this is probably more
        //  than we need (numerically weird stuff happens in places I dont
        //  understand, so erroring on the side of caution).
        using boost::math::policies::digits10;
        using boost::math::poisson_distribution;
        typedef boost::math::policies::policy<digits10<6> > my_pol_6;
        
        const poisson_distribution<float, my_pol_6 > pois( background );
        max_allowable = boost::math::quantile( pois, 0.99f );
      }//if( background < 20 )
      
      
      
      if( val > max_allowable && (!highres || contents[channel+direction] > max_allowable) )
      {
        //XXX - the below 3 is purely empircal, and meant to help avoid
        //      contamination due to the new feature
        if( channel >= 3*direction )
          lastchannel = channel - 3*direction;
        
#if( PRINT_ROI_DEBUG_INFO )
        if( debug_this_peak )
          DebugLog(cerr) << "FindMin: Found last bin at " << dataH->gamma_channel_center(lastchannel)
          << ", val=" << val << ", max_allowable=" << max_allowable << "\n";
#endif
        break;
      }else
      {
        ++nbackbin;
        backval += val;
#if( PRINT_ROI_DEBUG_INFO )
        if( debug_this_peak )
          DebugLog(cerr) << "FindMin: bin " << channel << ", x(bin)=" << energies[channel]
          << ", val=" << val << ", max_allowable=" << max_allowable
          << ", background=" << background << ", sigma=" << sigma << "\n";
#endif
      }
    }//if( val <= minVal ) / else
  }//for( ; bin > lastbin; --bin )
  
  nbackbin = 1 + 2*nSideChannel;
  backval = dataH->gamma_channels_sum( minChannel - nSideChannel,
                                       minChannel + nSideChannel );
  backval = std::max( backval, static_cast<float>(nbackbin) );
  
#if( PRINT_ROI_DEBUG_INFO )
  if( debug_this_peak )
    DebugLog(cerr) << "1) Bin with smallest contends at "
    << dataH->gamma_channel_center(minChannel)
    << ", backval=" << (backval/3.0) << ", lastbin=" << lastchannel
    << " x(lastbin)=" << dataH->gamma_channel_center(lastchannel) << "\n";
#endif
  
  //Make sure were not looking to far and that loop will terminate
  if( high ) lastchannel = std::min( lastchannel, nchannel-2 );
  else       lastchannel = std::max( lastchannel, indexing_t(1) );
  if( (high && (minChannel > lastchannel)) || (!high && (minChannel < lastchannel)) )
    minChannel = lastchannel;
  
  if( lastchannel || direction>0 )
    lastchannel += direction;
  
#if( PRINT_ROI_DEBUG_INFO )
  if( debug_this_peak )
    DebugLog(cerr) << "2) Bin with smallest contends at " << energies[minChannel]
    << ", backval=" << (backval/3.0) << ", lastbin=" << lastchannel
    << " x(lastbin)=" << energies[lastchannel] << "\n";
#endif
  
  
  if( direction < 0 && ((float(lastchannel)/float(nchannel)) < 0.04) )
  {
    size_t lower_channel = 0, upper_channel = 0;
    ExperimentalPeakSearch::find_spectroscopic_extent( dataH, lower_channel, upper_channel );
    if( static_cast<int>(lower_channel) >= lastchannel )
    {
      lastchannel = lower_channel ? lower_channel - 1 : 0;
      minChannel = std::max( minChannel, lastchannel );
    }
  }//if( direction < 0 && dataH->GetBinCenter(lastbin) < 100.0 )
  
  
  for( indexing_t channel = minChannel + direction*nSideChannel;
      channel != lastchannel && channel != meanchannel; channel += direction )
  {
    const float val = contents[channel];
    const float nextval = (channel>1 && (nchannel-channel)>0)  //probably is fine, but we'll check JIC
                          ? contents[channel+direction]
                          : contents[channel];
    
    float back = backval / nbackbin;
    float back_uncert = std::sqrt( backval ) / nbackbin;
    const float sigma = std::sqrt( back + back_uncert*back_uncert );
    //    float max_allowable = std::ceil(back + 2.575829*sigma) +  0.001;
    //    float min_allowable = std::floor(back - 2.575829*sigma) - 0.001;
    float max_allowable = std::ceil(back + 2.8f*sigma) +  0.001f;
    float min_allowable = std::floor(back - 2.8f*sigma) - 0.001f;
    
    if( back < 20.0f )
    {
      //Will use a reduced precision poisson_distribution to speed things up,
      //  since we really dont care past about 3 decimal places (but erring on
      //  the side of caution since I dont fully understand implications of
      //  reducing the accuracy).
      using boost::math::policies::digits10;
      using boost::math::poisson_distribution;
      typedef boost::math::policies::policy<digits10<6> > my_pol_6;
      
      const poisson_distribution<float, my_pol_6 > pois( back );
      max_allowable = boost::math::quantile( pois, 0.99f );
      min_allowable = boost::math::quantile( pois, 0.01f );
    }//if( val < 20.0 )
    
    //For high resolution spectra we'll require two bins to be outside of
    //  tollerance, since this will preserve the intent, but allow single bin
    //  spikes (which I swear are more common than poisson!) to not mess up the
    //  ROI.  It could probably be done for low resolution spectra, but I havent
    //  tested if (since single lowres peaks ROIs typically get calculated by
    //  find_roi_for_2nd_deriv_candidate(...) anyway
    if( (val>max_allowable && (!highres ||nextval>max_allowable))
        || (val<min_allowable && (!highres || nextval<min_allowable)) )
    {
#if( PRINT_ROI_DEBUG_INFO )
      if( debug_this_peak )
        DebugLog(cerr) << "Setting bin to " << (channel - direction) << " from expected "
        << lastchannel << " at energy=" << energies[channel-direction] << " kev"
        << ", this bring limit to " << (mean-energies[channel-direction])/sigma
        << " sigma from mean, val=" << val << ", min_allowable="
        << min_allowable << ", max_allowable=" << max_allowable << "\n";
#endif
      lastchannel = channel - (channel>0 ? direction : 0);
      break;
    }//if( val>max_allowable || val<min_allowable )
#if( PRINT_ROI_DEBUG_INFO )
    else
    {
      if( debug_this_peak )
        DebugLog(cerr) << "FindLimit: bin " << channel << ", x(bin)=" << energies[channel]
        << ", val=" << val
        << ", min_allowable=" << min_allowable
        << ", max_allowable=" << max_allowable
        << ", back=" << back << ", sigma=" << sigma << "\n";
    }
#endif
    
    nbackbin++;
    backval += val;
  }//for( int bin = minBin; bin > lastbin; --bin )
  
  
  //In principle, lastbin is the furthest from the mean we can end up, with
  //  the maximum being 11.75*sigma, or whereever a new feature was detected
  
#if( PRINT_ROI_DEBUG_INFO )
  if( debug_this_peak )
    DebugLog(cerr) << "minBin was " << minChannel << ", lastbin=" << lastchannel
    << " x(lastbin)=" << energies[lastchannel] << "\n";
#endif
  
  
  //Try to detect if there is a signficant skew on the peak by comparing
  //  4 to 7 sigma, to 7 to 11.75 sigma (or wherever is lastbin) to see if they
  //  are statistically compatible; if they are, just have ROI go to 7 sigma
  const int mean_channel = dataH->find_gamma_channel( mean );
  const int good_cont_channel = dataH->find_gamma_channel( mean + direction*7.05*sigma );
  if( (std::abs(int(lastchannel)-mean_channel) > std::abs(good_cont_channel-mean_channel)) )
  {
    const indexing_t nearest_channel = dataH->find_gamma_channel( mean + direction*3.5*sigma );
    bool isNotDecreasing
    = isStatisticallyGreaterOrEqual( nearest_channel, good_cont_channel,
                                    good_cont_channel, lastchannel, dataH, 3.0 );
    
    const bool do_slope_check = false;
    if( do_slope_check )
    {
      int near_start = static_cast<int>(min(good_cont_channel,(int)nearest_channel));
      int near_end = static_cast<int>(max(good_cont_channel,(int)nearest_channel));
      const int num_near_channel = near_end - near_start;
      
      //      indexing_t far_start = min(lastbin,good_cont_bin);
      //      indexing_t far_end = max(lastbin,good_cont_bin);
      int far_start = min(lastchannel,nearest_channel);
      int far_end = max(lastchannel,nearest_channel);
      const int num_far_channel = far_end - far_start;
      
      vector<float> near_data( num_near_channel, 0.0f ), near_x( num_near_channel, 0.0f );
      vector<float> far_data( num_far_channel, 0.0f ), far_x( num_far_channel, 0.0f );
      
      for( int channel = near_start; channel < near_end; ++channel )
      {
        near_x[channel-near_start] = dataH->gamma_channel_center(channel);
        near_x[channel-near_start] = static_cast<float>(channel-near_start+1);
        near_data[channel-near_start] = contents[channel];
      }
      
      for( int channel = far_start; channel < far_end; ++channel )
      {
        far_x[channel-far_start] = dataH->gamma_channel_center( channel );
        far_x[channel-far_start] = static_cast<float>(channel-far_start+1);
        far_data[channel-far_start] = contents[channel];
      }
      
      std::vector<double> near_coeffs, near_uncerts;
      std::vector<double> far_coeffs, far_uncerts;
      double chi1 = fit_to_polynomial( &(near_x[0]), &(near_data[0]), 
		                               static_cast<int>(near_x.size()), 1, near_coeffs, near_uncerts );
      double chi2 = fit_to_polynomial( &(far_x[0]), &(far_data[0]), 
		                               static_cast<int>(far_data.size()), 1, far_coeffs, far_uncerts );
      
      chi1 /= (near_x.size()-2);
      chi2 /= (far_data.size()-2);
      
      const double mult = (high ? 1.0 : -1.0);
      double uncert = sqrt( near_uncerts[1]*near_uncerts[1] + far_uncerts[1]*far_uncerts[1] );
      const double lowerx = mult * near_coeffs[1];
      const double upperx = mult * far_coeffs[1];
      
      cerr << "\n\nIsDecreasing=" << !isNotDecreasing
      << "  nbin=" << near_x.size() << ", " << far_x.size()
      << ", high=" << high << endl;
      cerr << "chi1=" << chi1 << ", chi2=" << chi2 << endl;
      cerr << "lowerx=" << lowerx << ", upperx=" << upperx << ", uncert=" << uncert << ", nsigma=" << (upperx-lowerx)/uncert << endl;
      cerr << "near_coeffs[1]=" << near_coeffs[1] << " +- " << near_uncerts[1] << endl;
      cerr << "far_coeffs[1]=" << far_coeffs[1] << " +- " << far_uncerts[1] << endl;
    }//if( do_slope_check )
    
    
    if( high || isNotDecreasing )
      lastchannel = good_cont_channel;
    //    else
    //    {
    //      //now check from 11.75 to to 16.5 sigma, to see if we should include down
    //      //  to there.
    //      const int start = good_cont_bin;
    //      const int end = dataH->FindFixBin( mean + direction*16.5*sigma );
    //      isNotDecreasing = isStatisticallyGreaterOrEqual( good_cont_bin, lastbin,
    //                                                      start, end, dataH, 2.0 );
    //      if( !isNotDecreasing )
    //        lastbin = end;
    //    }
  }//if( abs(lastbin-mean_bin) > abs(good_cont_bin-mean_bin) )
  
  
  float val = dataH->gamma_channel_center(lastchannel);
  if( direction < 0 )
  {
    if( ((mean-val)/sigma) < 1.75 )
      lastchannel = dataH->find_gamma_channel( mean - 1.75*sigma );
  }else
  {
    if( ((val-mean)/sigma) < 1.75 )
      lastchannel = dataH->find_gamma_channel( mean + 1.75*sigma );
  }
  
#if( PRINT_ROI_DEBUG_INFO )
  if( debug_this_peak )
    DebugLog(cerr) << "Returning bin " << lastchannel
    << ", x=" << dataH->gamma_channel_center(lastchannel) << "\n";
#endif
  
  return lastchannel;
}//int findROILimit(...)




bool isStatisticallyGreaterOrEqual( const size_t start1, const size_t end1,
                                    const size_t start2, const size_t end2,
                                    const std::shared_ptr<const Measurement> &dataH,
                                    const double nsigma )
{
  size_t lowerbin = std::min( start1, end1 );
  size_t upperbin = std::max( start1, end1 );
  const double lower_area = dataH->gamma_channels_sum( lowerbin, upperbin );
  const int num_near_mean_bins = upperbin - lowerbin + 1;
  const double avrg_near_mean_area = lower_area / num_near_mean_bins;
  const double avrg_near_mean_uncert = sqrt(lower_area) / num_near_mean_bins;
  
  lowerbin = std::min( start2, end2 );
  upperbin = std::max( start2, end2 );
  const double upper_area = dataH->gamma_channels_sum( lowerbin, upperbin );
  const int num_tail_bins = upperbin - lowerbin + 1;
  const double tail_area = upper_area / num_tail_bins;
  const double avrg_tail_uncert = sqrt(upper_area) / num_tail_bins;
  
  const double uncert = sqrt(avrg_near_mean_uncert*avrg_near_mean_uncert
                             + avrg_tail_uncert*avrg_tail_uncert);
  
/*
#if( PRINT_ROI_DEBUG_INFO )
    DebugLog(cerr) << "Found avrg_near_mean_area=" << avrg_near_mean_area << "+-" << avrg_near_mean_uncert
    << ", tail_area=" << tail_area << "+-" << avrg_near_mean_uncert
    << ", uncert=" << uncert
    << " ---> (avrg_near_mean_area-2.0*uncert)=" << (avrg_near_mean_area-nsigma*uncert)
    << ", nsigma=" << ((avrg_near_mean_area-tail_area)/uncert)
    << "\n";
#endif
*/
  
  return (tail_area > (avrg_near_mean_area-nsigma*uncert));
}//isStatisticallyGreaterOrEqual( ... )


void estimatePeakFitRange( const PeakDef &peak, const std::shared_ptr<const Measurement> &dataH,
                           int &xlowbin, int &xhighbin )
{
  if( !dataH  )
    return;
  
  std::shared_ptr<const PeakContinuum> continuum = peak.continuum();
  double lowxrange  = continuum->lowerEnergy();
  double highxrange = continuum->upperEnergy();
  
  const bool definedRange = (lowxrange!=highxrange);
  if( definedRange )
  {
    xlowbin  = max( dataH->FindFixBin( lowxrange ), 1 );
    xhighbin = min(dataH->FindFixBin( highxrange-0.00001 ), dataH->GetNbinsX());
    return;
  }//if( definedRange )
  
  const double mean = peak.mean();
  const double sigma = peak.gausPeak() ? peak.sigma() : 0.5*0.25*peak.roiWidth();
  
  
  if( continuum->type() == PeakContinuum::External )
  {
    lowxrange  = mean - 4.0*sigma;
    highxrange = mean + 4.0*sigma;
    
    xlowbin  = max( dataH->FindFixBin( lowxrange ), 1 );
    xhighbin = min(dataH->FindFixBin( highxrange ), dataH->GetNbinsX());
    return;
  }//if( peak.m_offsetType == PeakDef::External )
  
  
  const bool polyContinuum = continuum->isPolynomial();
  if( polyContinuum )
  {
    const size_t lowerchannel = findROILimit( peak, dataH, false );
    const size_t upperchannel = findROILimit( peak, dataH, true );
    
    xlowbin  = (int)lowerchannel + 1;
    xhighbin = (int)upperchannel + 1;
  }else
  {
    xlowbin = max( dataH->FindFixBin( mean - 4.0*sigma ), 1 );
    xhighbin = min( dataH->FindFixBin( mean + 4.0*sigma ), dataH->GetNbinsX() );
  }
  
  if( peak.skewType() == PeakDef::LandauSkew )
  {
    double landau_mode = peak.coefficient(PeakDef::LandauMode);
    double landau_sigma = peak.coefficient(PeakDef::LandauSigma);
    cout << "xlow was " << lowxrange << " now is ";
    lowxrange = min( lowxrange, mean-landau_mode+(0.22278*landau_sigma) - 10.0*landau_sigma );
    cout << lowxrange << endl;
    xlowbin = max( dataH->FindFixBin( lowxrange ), 1 );
    xhighbin = min( dataH->FindFixBin( highxrange ), dataH->GetNbinsX() );
  }//if( we have a landau tail )
  
  //Lets avoid some wierd going to too small of peak widths
  const int numfitbin = xhighbin - xlowbin;
  if( numfitbin <= 9 )  //9 chosen arbitrarily
  {
    xlowbin -= (10-numfitbin)/2;
    xhighbin += (10-numfitbin)/2;
  }//if( numfitbin <= 9 )
  
}//void setPeakXLimitsFromData( PeakDef &peak, const std::shared_ptr<const Measurement> &dataH )


ostream &operator<<( std::ostream &stream, const PeakContinuum &cont )
{
  switch( cont.type() )
  {
    case PeakContinuum::NoOffset:
      stream << "Underfined continuum";
    break;
    case PeakContinuum::External:
      stream << "Globally defined continuum";
    break;
    case PeakContinuum::Constant:   case PeakContinuum::Linear:
    case PeakContinuum::Quardratic: case PeakContinuum::Cubic:
      stream << "Polynomial continuum with values {";
      for( size_t i = 0; i < cont.m_values.size(); ++i )
        stream << (i?", ":"") << cont.m_values[i];
      stream << "} relative to " << cont.m_refernceEnergy << " keV";
    break;
  }//switch( m_type )

  stream << ", valid from " << cont.lowerEnergy()
         << " to " << cont.upperEnergy() << " keV";
  
  return stream;
}//operator<<( std::ostream &stream, const PeakContinuum &cont )


std::ostream &operator<<( std::ostream &stream, const PeakDef &peak )
{
  stream << "mean=" << peak.m_coefficients[PeakDef::Mean];
  if( peak.m_uncertainties[PeakDef::Mean] > 0.0 )
    stream << "+-" << peak.m_uncertainties[PeakDef::Mean];
  stream << ", sigma=" << peak.m_coefficients[PeakDef::Sigma];
  if( peak.m_uncertainties[PeakDef::Sigma] > 0.0 )
    stream << "+-" << peak.m_uncertainties[PeakDef::Sigma];
  stream << ", amplitude=" << peak.m_coefficients[PeakDef::GaussAmplitude];
  if( peak.m_uncertainties[PeakDef::GaussAmplitude] > 0.0 )
    stream << "+-" << peak.m_uncertainties[PeakDef::GaussAmplitude];

  if( peak.m_transition )
  {
    const SandiaDecay::RadParticle *particle = NULL;
    const SandiaDecay::Transition *transition = peak.m_transition;
    const int index = peak.m_radparticleIndex;
    const int nproducts = static_cast<int>( transition->products.size() );
    if( (index>=0)  && (index<nproducts) )
      particle = &(transition->products[index]);
    stream << ", decay="
           << (transition->parent ? transition->parent->symbol : string("N/A") )
           << "->"
           << (transition->child ? transition->child->symbol : string("N/A") )
           << " " << (particle ? particle->energy : -1.0) << " keV";
  }else if( peak.m_sourceGammaType == PeakDef::AnnihilationGamma )
  {
    stream << ", Annihilation Gamma";
  }
  
  stream << ", " << *peak.m_continuum
         << ", chi2=" << peak.m_coefficients[PeakDef::Chi2DOF]
         << ", landau_amplitude=" << peak.m_coefficients[PeakDef::LandauAmplitude]
         << ", landau_mode=" << peak.m_coefficients[PeakDef::LandauMode]
         << ", landau_sigma=" << peak.m_coefficients[PeakDef::LandauSigma]
         << std::flush;
  return stream;
}//std::ostream &operator<<( std::ostream &stream, const PeakDef &peak )



#if( PERFORM_DEVELOPER_CHECKS )
void PeakDef::equalEnough( const PeakDef &lhs, const PeakDef &rhs )
{
  char buffer[512];
  
  if( lhs.m_userLabel != rhs.m_userLabel )
    throw runtime_error( "PeakDef user label for LHS ('"
                        + lhs.m_userLabel + "') doesnt match RHS ('"
                        + rhs.m_userLabel + "')" );
  
  if( lhs.m_type != rhs.m_type )
  {
    snprintf(buffer, sizeof(buffer),
             "PeakDef peak type doesnt match, %i vs %i",
             int(lhs.m_type), int(rhs.m_type) );
    throw runtime_error( buffer );
  }
  
  if( lhs.m_skewType != rhs.m_skewType )
  {
    snprintf(buffer, sizeof(buffer), "PeakDef skew type doesnt match, %i vs %i",
             int(lhs.m_skewType), int(rhs.m_skewType) );
    throw runtime_error( buffer );
  }
  
  
  for( CoefficientType t = CoefficientType(0); t < NumCoefficientTypes; t = CoefficientType(t+1) )
  {
    const double a = lhs.m_coefficients[t];
    const double b = rhs.m_coefficients[t];
    const double diff = fabs( a - b );
    
    if( diff > 1.0E-6*max(fabs(a),fabs(b)) )
    {
      snprintf(buffer, sizeof(buffer),
               "PeakDef coeficient %s of LHS (%1.8E) vs RHS (%1.8E) is out of tolerance.",
               to_string(t), lhs.m_coefficients[t], rhs.m_coefficients[t] );
      throw runtime_error( buffer );
    }
  }
  
  for( CoefficientType t = CoefficientType(0); t < NumCoefficientTypes; t = CoefficientType(t+1) )
  {
    const double a = lhs.m_uncertainties[t];
    const double b = rhs.m_uncertainties[t];
    const double diff = fabs( a - b );

    if( diff > 1.0E-6*max(fabs(a),fabs(b)) )
    {
      snprintf(buffer, sizeof(buffer),
               "PeakDef uncertanity %s of LHS (%1.8E) vs RHS (%1.8E) is out of tolerance.",
               to_string(t), lhs.m_uncertainties[t], rhs.m_uncertainties[t] );
      throw runtime_error( buffer );
    }
  }
  
  for( CoefficientType t = CoefficientType(0); t < NumCoefficientTypes; t = CoefficientType(t+1) )
  {
    if( lhs.m_fitFor[t] != rhs.m_fitFor[t] )
    {
      snprintf(buffer, sizeof(buffer),
               "PeakDef fit for %s of LHS (%i) vs RHS (%i) doesnt match.",
               to_string(t), int(lhs.m_fitFor[t]), int(rhs.m_fitFor[t]) );
      throw runtime_error( buffer );
    }
  }
  
  
  if( !!lhs.m_continuum != !!rhs.m_continuum )
  {
    snprintf(buffer, sizeof(buffer),
             "PeakDef availablity of continuum of LHS (%i) vs RHS (%i) continuums doent match.",
             int(!!lhs.m_continuum), int(!!rhs.m_continuum) );
    throw runtime_error( buffer );
  }

  if( !!lhs.m_continuum )
    PeakContinuum::equalEnough( *lhs.m_continuum, *rhs.m_continuum );
 
  if( lhs.m_parentNuclide != rhs.m_parentNuclide )
  {
    snprintf(buffer, sizeof(buffer),
             "PeakDef parent nuclide of LHS (%s) vs RHS (%s) doent match.",
        (lhs.m_parentNuclide ? lhs.m_parentNuclide->symbol.c_str() : "none"),
        (rhs.m_parentNuclide ? rhs.m_parentNuclide->symbol.c_str() : "none") );
    throw runtime_error( buffer );
  }
  
  if( lhs.m_transition != rhs.m_transition )
  {
    snprintf(buffer, sizeof(buffer),
             "PeakDef nuclide transition of LHS (%s -> %s) vs RHS (%s -> %s) doent match.",
             (lhs.m_transition->parent ? lhs.m_transition->parent->symbol.c_str() : "none"),
             (lhs.m_transition->child ? lhs.m_transition->child->symbol.c_str() : "none"),
             (rhs.m_transition->parent ? rhs.m_transition->parent->symbol.c_str() : "none"),
             (rhs.m_transition->child ? rhs.m_transition->child->symbol.c_str() : "none") );
    throw runtime_error( buffer );
  }
  
  if( lhs.m_radparticleIndex != rhs.m_radparticleIndex )
  {
    snprintf(buffer, sizeof(buffer),
             "PeakDef particle index of LHS (%i) vs RHS (%i) doent match.",
             lhs.m_radparticleIndex, rhs.m_radparticleIndex );
    throw runtime_error( buffer );
  }
  
  if( lhs.m_sourceGammaType != rhs.m_sourceGammaType )
  {
    snprintf(buffer, sizeof(buffer),
             "PeakDef is annihilation of LHS (%i) vs RHS (%i) doent match.",
             int(lhs.m_sourceGammaType), int(rhs.m_sourceGammaType) );
    throw runtime_error( buffer );
  }
  
//  std::vector< CandidateNuclide > m_candidateNuclides;
  
  if( lhs.m_xrayElement != rhs.m_xrayElement )
  {
    snprintf(buffer, sizeof(buffer),
             "PeakDef xray of LHS (%s) vs RHS (%s) doent match.",
             (lhs.m_xrayElement ? lhs.m_xrayElement->symbol.c_str() : "none"),
             (rhs.m_xrayElement ? rhs.m_xrayElement->symbol.c_str() : "none") );
    throw runtime_error( buffer );
  }
  
  if( fabs(lhs.m_xrayEnergy - rhs.m_xrayEnergy) > 0.001 )
  {
    snprintf(buffer, sizeof(buffer),
             "PeakDef xray energy of LHS (%1.8E keV) vs RHS (%1.8E keV) doent match.",
             lhs.m_xrayEnergy, rhs.m_xrayEnergy );
    throw runtime_error( buffer );
  }

  if( lhs.m_reaction != rhs.m_reaction )
  {
    snprintf(buffer, sizeof(buffer),
             "PeakDef reaction of LHS (%s) vs RHS (%s) doent match.",
             (lhs.m_reaction ? lhs.m_reaction->name().c_str() : "none"),
             (rhs.m_reaction ? rhs.m_reaction->name().c_str() : "none") );
    throw runtime_error( buffer );
  }

  if( fabs(lhs.m_reactionEnergy - rhs.m_reactionEnergy) > 0.001 )
  {
    snprintf(buffer, sizeof(buffer),
             "PeakDef reaction energy energy of LHS (%1.8E keV) vs RHS (%1.8E keV) doent match.",
             lhs.m_reactionEnergy, rhs.m_reactionEnergy );
    throw runtime_error( buffer );
  }
  
  if( lhs.m_useForCalibration != rhs.m_useForCalibration )
  {
    snprintf(buffer, sizeof(buffer),
             "PeakDef use for calibration of LHS (%i) vs RHS (%i) doent match.",
             int(lhs.m_useForCalibration), int(rhs.m_useForCalibration) );
    throw runtime_error( buffer );
  }
  
  if( lhs.m_useForShieldingSourceFit != rhs.m_useForShieldingSourceFit )
  {
    snprintf(buffer, sizeof(buffer),
             "PeakDef use for shielding source fit of LHS (%i) vs RHS (%i) doent match.",
             int(lhs.m_useForShieldingSourceFit), int(rhs.m_useForShieldingSourceFit) );
    throw runtime_error( buffer );
  }
  
  if( lhs.m_useForDetectorResponseFit != rhs.m_useForDetectorResponseFit )
  {
    snprintf(buffer, sizeof(buffer),
             "PeakDef use for DRF fit of LHS (%i) vs RHS (%i) doent match.",
             int(lhs.m_useForDetectorResponseFit), int(rhs.m_useForDetectorResponseFit) );
    throw runtime_error( buffer );
  }
}//void equalEnough( const PeakDef &lhs, const PeakDef &rhs )


void PeakContinuum::equalEnough( const PeakContinuum &lhs, const PeakContinuum &rhs )
{
  
  char buffer[512];
  
  if( lhs.m_type != rhs.m_type )
  {
    snprintf(buffer, sizeof(buffer),
             "PeakContinuum type of LHS (%i) vs RHS (%i) doesnt match.",
             int(lhs.m_type), int(rhs.m_type) );
    throw runtime_error( buffer );
  }
  
  if( fabs(lhs.m_lowerEnergy - rhs.m_lowerEnergy) > 0.0001 )
  {
    snprintf(buffer, sizeof(buffer),
             "PeakContinuum ROI lower energy of LHS (%1.8E keV) vs RHS (%1.8E keV) doesnt match.",
             lhs.m_lowerEnergy, rhs.m_lowerEnergy );
    throw runtime_error( buffer );
  }
  
  if( fabs(lhs.m_upperEnergy - rhs.m_upperEnergy) > 0.0001 )
  {
    snprintf(buffer, sizeof(buffer),
             "PeakContinuum ROI upper energy of LHS (%1.8E keV) vs RHS (%1.8E keV) doesnt match.",
             lhs.m_upperEnergy, rhs.m_upperEnergy );
    throw runtime_error( buffer );
  }

  if( fabs(lhs.m_refernceEnergy - rhs.m_refernceEnergy) > 0.0001 )
  {
    snprintf(buffer, sizeof(buffer),
             "PeakContinuum reference energy of LHS (%1.8E keV) vs RHS (%1.8E keV) doesnt match.",
             lhs.m_refernceEnergy, rhs.m_refernceEnergy );
    throw runtime_error( buffer );
  }
  
  if( lhs.m_values.size() != rhs.m_values.size() )
  {
    snprintf(buffer, sizeof(buffer),
             "PeakContinuum number of coefficients of LHS (%i) vs RHS (%i) doesnt match.",
             int(lhs.m_values.size()), int(rhs.m_values.size()) );
    throw runtime_error( buffer );
  }

  if( lhs.m_uncertainties.size() != rhs.m_uncertainties.size() )
  {
    snprintf(buffer, sizeof(buffer),
             "PeakContinuum number of uncertainties of LHS (%i) vs RHS (%i) doesnt match.",
             int(lhs.m_uncertainties.size()), int(rhs.m_uncertainties.size()) );
    throw runtime_error( buffer );
  }


  if( lhs.m_fitForValue.size() != rhs.m_fitForValue.size() )
  {
    snprintf(buffer, sizeof(buffer),
             "PeakContinuum number of fit for variables of LHS (%i) vs RHS (%i) doesnt match.",
             int(lhs.m_fitForValue.size()), int(rhs.m_fitForValue.size()) );
    throw runtime_error( buffer );
  }
  
  if( lhs.m_values.size() != lhs.m_uncertainties.size()
     || lhs.m_values.size() != lhs.m_fitForValue.size() )
    throw runtime_error( "PeakContinuum something totally whack with number of coefficents somewhere!" );
  
  
  for( size_t i = 0; i < lhs.m_values.size(); ++i )
  {
    if( fabs(lhs.m_values[i]-rhs.m_values[i]) > (1.0E-5 * std::max(fabs(lhs.m_values[i]),fabs(rhs.m_values[i])))  )
    {
      snprintf(buffer, sizeof(buffer),
               "PeakContinuum value of %ith variables of LHS (%1.8E) vs RHS (%1.8E) doesnt match within tolerance.",
               int(i), lhs.m_values[i], rhs.m_values[i] );
      throw runtime_error( buffer );
    }

    if( fabs(lhs.m_uncertainties[i]-rhs.m_uncertainties[i]) > (1.0E-4 * std::max(fabs(lhs.m_uncertainties[i]),fabs(rhs.m_uncertainties[i])))  )
    {
      snprintf(buffer, sizeof(buffer),
               "PeakContinuum value of %ith uncertainty of LHS (%1.8E) vs RHS (%1.8E) doesnt match within tolerance.",
               int(i), lhs.m_uncertainties[i], rhs.m_uncertainties[i] );
      throw runtime_error( buffer );
    }

    if( lhs.m_fitForValue[i] != rhs.m_fitForValue[i] )
    {
      snprintf(buffer, sizeof(buffer),
               "PeakContinuum value of %ith fit for of LHS (%i) vs RHS (%i) doesnt match.",
               int(i), int(lhs.m_fitForValue[i]), int(rhs.m_fitForValue[i]) );
      throw runtime_error( buffer );
    }
  }
  
  if( !!lhs.m_externalContinuum != !!rhs.m_externalContinuum )
  {
    snprintf(buffer, sizeof(buffer),
             "PeakContinuum availablity of external continuum LHS (%i) vs RHS (%i) continuums doent match.",
             int(!!lhs.m_externalContinuum), int(!!rhs.m_externalContinuum) );
    throw runtime_error( buffer );
  }
  
  if( !!lhs.m_externalContinuum )
  {
    try
    {
      Measurement::equalEnough( *lhs.m_externalContinuum, *rhs.m_externalContinuum );
    }catch( std::exception &e )
    {
      snprintf( buffer, sizeof(buffer), "PeakContinuum caught testing external continuum: %s", e.what() );
      throw runtime_error( buffer );
    }//try / catch
  }
}//void equalEnough( const PeakContinuum &lhs, const PeakContinuum &rhs )
#endif //PERFORM_DEVELOPER_CHECKS


PeakDef::PeakDef()
{
  reset();
}


void PeakDef::reset()
{
  m_userLabel                = "";
  m_type                     = GaussianDefined;
  m_skewType                 = PeakDef::NoSkew;

  m_parentNuclide            = NULL;
  m_transition               = NULL;
  m_radparticleIndex         = -1;
  m_sourceGammaType          = NormalGamma;
  m_useForCalibration        = true;
  m_useForShieldingSourceFit = false;
  m_useForDetectorResponseFit = -1;
  
  m_xrayElement              = NULL;
  m_xrayEnergy               = 0.0;
  m_reaction                 = NULL;
  m_reactionEnergy           = 0.0;

  m_lineColor                = Wt::WColor();
  
  std::shared_ptr<PeakContinuum> newcont = std::make_shared<PeakContinuum>();
  m_continuum = newcont;
  
  for( CoefficientType t = CoefficientType(0);
       t < NumCoefficientTypes; t = CoefficientType(t+1) )
  {
    m_coefficients[t] = 0.0;
    m_uncertainties[t] = -1.0;
    
    switch( t )
    {
      case PeakDef::Mean:
      case PeakDef::Sigma:
      case PeakDef::GaussAmplitude:
        m_fitFor[t] = true;
      break;
        
      case PeakDef::LandauAmplitude:
      case PeakDef::LandauMode:
      case PeakDef::LandauSigma:
      case PeakDef::Chi2DOF:
      case PeakDef::NumCoefficientTypes:
        m_fitFor[t] = false;
      break;
    }//switch( type )
  }//for( loop over coefficients )
}//void PeakDef::reset()


PeakDef::PeakDef( double m, double s, double a )
{
  reset();
  m_coefficients[PeakDef::Mean] = m;
  m_coefficients[PeakDef::Sigma] = s;
  m_coefficients[PeakDef::GaussAmplitude] = a;
}



PeakDef::PeakDef( double xlow, double xhigh, double mean,
                    std::shared_ptr<const Measurement> data, std::shared_ptr<const Measurement> background )
{
  reset();
  m_type = PeakDef::DataDefined;
  m_coefficients[PeakDef::Mean] = mean;
  m_continuum->setRange( xlow, xhigh );
  
  if( !data )
    return;

  m_continuum->setType( PeakContinuum::External );
  m_continuum->setExternalContinuum( background );
  m_continuum->setRange( xlow, xhigh );
  
  m_coefficients[PeakDef::GaussAmplitude] = gamma_integral( data, xlow, xhigh );
  if( background )
    m_coefficients[PeakDef::GaussAmplitude] -= gamma_integral( background, xlow, xhigh );
}//PeakDef( constructor )



const char *PeakDef::to_string( const CoefficientType type )
{
  switch( type )
  {
    case PeakDef::Mean:                return "Centroid";
    case PeakDef::Sigma:               return "Width";
    case PeakDef::GaussAmplitude:      return "Amplitude";
    case PeakDef::LandauAmplitude:     return "LandauAmplitude";
    case PeakDef::LandauMode:          return "LandauMode";
    case PeakDef::LandauSigma:         return "LandauSigma";
    case PeakDef::Chi2DOF:             return "Chi2";
    case PeakDef::NumCoefficientTypes: return "";
  }//switch( type )

  return "";
}//const char *PeakDef::to_string( const CoefficientType type )


void PeakDef::gammaTypeFromUserInput( std::string &txt,
                                      PeakDef::SourceGammaType &type )
{
  
  type = PeakDef::NormalGamma;
  
  if( UtilityFunctions::icontains( txt, "s.e." ) )
  {
    type = PeakDef::SingleEscapeGamma;
    UtilityFunctions::ireplace_all( txt, "s.e.", "" );
  }
  
  if( UtilityFunctions::icontains( txt, "single escape" ) )
  {
    type = PeakDef::SingleEscapeGamma;
    UtilityFunctions::ireplace_all( txt, "single escape", "" );
  }
  
  if( UtilityFunctions::icontains( txt, "se " ) && txt.size() > 5 )
  {
    type = PeakDef::SingleEscapeGamma;
    UtilityFunctions::ireplace_all( txt, "se ", "" );
  }
  
  if( UtilityFunctions::icontains( txt, "d.e." ) )
  {
    type = PeakDef::DoubleEscapeGamma;
    UtilityFunctions::ireplace_all( txt, "d.e.", "" );
  }
  
  if( UtilityFunctions::icontains( txt, "double escape" ) )
  {
    type = PeakDef::DoubleEscapeGamma;
    UtilityFunctions::ireplace_all( txt, "double escape", "" );
  }
  
  if( UtilityFunctions::icontains( txt, "de " ) && txt.size() > 5 )
  {
    type = PeakDef::DoubleEscapeGamma;
    UtilityFunctions::ireplace_all( txt, "de ", "" );
  }
  
  if( UtilityFunctions::icontains( txt, "x-ray" )
      || UtilityFunctions::icontains( txt, "xray" )
     || UtilityFunctions::icontains( txt, "x ray" ) )
  {
    
    type = PeakDef::XrayGamma;
    UtilityFunctions::ireplace_all( txt, "xray", "" );
    UtilityFunctions::ireplace_all( txt, "x-ray", "" );
    UtilityFunctions::ireplace_all( txt, "x ray", "" );
  }
  
  
}//PeakDef::SourceGammaType gammaType( std::string txt )


const Wt::WColor &PeakDef::lineColor() const
{
  return m_lineColor;
}

void PeakDef::setLineColor( const Wt::WColor &color )
{
  m_lineColor = color;
}


void PeakContinuum::toXml( rapidxml::xml_node<char> *parent, const int contId ) const
{
  using namespace rapidxml;
  
  xml_document<char> *doc = parent ? parent->document() : (xml_document<char> *)0;
  
  if( !doc )
    throw runtime_error( "PeakContinuum::toXml(...): invalid input" );
  
  char buffer[128];
  xml_node<char> *node = 0;
  xml_node<char> *cont_node = doc->allocate_node( node_element, "PeakContinuum" );

  snprintf( buffer, sizeof(buffer), "%i", sm_xmlSerializationVersion );
  const char *val = doc->allocate_string( buffer );
  xml_attribute<char> *att = doc->allocate_attribute( "version", val );
  cont_node->append_attribute( att );

  snprintf( buffer, sizeof(buffer), "%i", contId );
  val = doc->allocate_string( buffer );
  att = doc->allocate_attribute( "id", val );
  cont_node->append_attribute( att );
    
  parent->append_node( cont_node );
  
  const char *type = 0;
  switch( m_type )
  {
    case NoOffset:   type = "NoOffset";   break;
    case Constant:   type = "Constant";   break;
    case Linear:     type = "Linear";     break;
    case Quardratic: type = "Quardratic"; break;
    case Cubic:      type = "Cubic";      break;
    case External:   type = "External";   break;
  }//switch( m_type )
  
  node = doc->allocate_node( node_element, "Type", type );
  cont_node->append_node( node );
  
  snprintf( buffer, sizeof(buffer), "%1.8e", m_lowerEnergy );
  val = doc->allocate_string( buffer );
  node = doc->allocate_node( node_element, "LowerEnergy", val );
  cont_node->append_node( node );
  
  snprintf( buffer, sizeof(buffer), "%1.8e", m_upperEnergy );
  val = doc->allocate_string( buffer );
  node = doc->allocate_node( node_element, "UpperEnergy", val );
  cont_node->append_node( node );
  
  snprintf( buffer, sizeof(buffer), "%1.8e", m_refernceEnergy );
  val = doc->allocate_string( buffer );
  node = doc->allocate_node( node_element, "ReferenceEnergy", val );
  cont_node->append_node( node );
  
  if( m_type != NoOffset && m_type != External )
  {
    stringstream valsstrm, uncertstrm, fitstrm;
    for( size_t i = 0; i < m_values.size(); ++i )
    {
      const char *spacer = (i ? " " : "");
      snprintf( buffer, sizeof(buffer), "%1.8e", m_values[i] );  
      valsstrm << spacer << buffer;
    
      snprintf( buffer, sizeof(buffer), "%1.8e", m_uncertainties[i] );  
      uncertstrm << spacer << buffer;
    
      fitstrm << spacer << (m_fitForValue[i] ? '1': '0');
    }//for( size_t i = 0; i < m_values.size(); ++i )
    
    xml_node<char> *coeffs_node = doc->allocate_node( node_element, "Coefficients" );
    cont_node->append_node( coeffs_node );
        
    val = doc->allocate_string( valsstrm.str().c_str() );
    node = doc->allocate_node( node_element, "Values", val );
    coeffs_node->append_node( node );
    
    val = doc->allocate_string( uncertstrm.str().c_str() );
    node = doc->allocate_node( node_element, "Uncertainties", val );
    coeffs_node->append_node( node );
    
    val = doc->allocate_string( fitstrm.str().c_str() );
    node = doc->allocate_node( node_element, "Fittable", val );
    coeffs_node->append_node( node );
  }//if( m_type != NoOffset && m_type != External )
  
  if( !!m_externalContinuum )
  {
    stringstream contXml;
    m_externalContinuum->write_2006_N42_xml( contXml );
    //We actually need to parse the XML here, and then insert it into the hierarchy
    
    const string datastr = contXml.str();
    std::unique_ptr<char []> data( new char [datastr.size()+1] );
    memcpy( data.get(), datastr.c_str(), datastr.size()+1 );
    
    xml_document<char> contdoc;
    const int flags = rapidxml::parse_normalize_whitespace
                     | rapidxml::parse_trim_whitespace;
    contdoc.parse<flags>( data.get() );
    
    node = doc->allocate_node( node_element, "ExternalContinuum", val );
    cont_node->append_node( node );
    
    xml_node<char> *spec_node = contdoc.first_node( "Measurement", 11 );
    if( !spec_node )
      throw runtime_error( "Didnt get expected Measurement node" );
    spec_node = spec_node->first_node( "Spectrum", 8 );
    if( !spec_node )
      throw runtime_error( "Didnt get expected Spectrum node" );
    
    xml_node<char> *new_spec_node = doc->allocate_node( node_element );
    node->append_node( new_spec_node );
    
    clone_node_deep( spec_node, new_spec_node );
  }//if( !!m_externalContinuum )
}//void PeakContinuum::toXml(...)





void PeakContinuum::fromXml( const rapidxml::xml_node<char> *cont_node, int &contId )
{
  using namespace rapidxml;
  using ::rapidxml::internal::compare;
  
  if( !cont_node )
    throw runtime_error( "PeakContinuum::fromXml(...): invalid input" );
  
  if( !compare( cont_node->name(), cont_node->name_size(), "PeakContinuum", 13, false ) )
    throw std::logic_error( "PeakContinuum::fromXml(...): invalid input node name" );
  
  xml_attribute<char> *att = cont_node->first_attribute( "version", 7 );
  
  int version;
  if( !att || !att->value() || (sscanf(att->value(), "%i", &version)!=1) )
    throw runtime_error( "PeakContinuum invalid version" );
  
  if( version != sm_xmlSerializationVersion )
    throw runtime_error( "Invalid PeakContinuum version" );
  
  att = cont_node->first_attribute( "id", 2 );
  if( !att || !att->value() || (sscanf(att->value(), "%i", &contId)!=1) )
    throw runtime_error( "PeakContinuum invalid ID" );

  xml_node<char> *node = cont_node->first_node( "Type", 4 );

  if( !node || !node->value() )
    throw runtime_error( "PeakContinuum not Type node" );
  
  if( compare(node->value(),node->value_size(),"NoOffset",8,false) )
    m_type = NoOffset;
  else if( compare(node->value(),node->value_size(),"Constant",8,false) )
    m_type = Constant;
  else if( compare(node->value(),node->value_size(),"Linear",6,false) )
    m_type = Linear;
  else if( compare(node->value(),node->value_size(),"Quardratic",10,false) )
    m_type = Quardratic;
  else if( compare(node->value(),node->value_size(),"Cubic",5,false) )
    m_type = Cubic;
  else if( compare(node->value(),node->value_size(),"External",8,false) )
    m_type = External;
  else
    throw runtime_error( "Invalid continuum type" );
  
  float dummyval;
  node = cont_node->first_node( "LowerEnergy", 11 );
  if( !node || !node->value() || (sscanf(node->value(),"%e",&dummyval) != 1) )
    throw runtime_error( "Continuum didnt have LowerEnergy" );
  m_lowerEnergy = dummyval;
    
  node = cont_node->first_node( "UpperEnergy", 11 );
  if( !node || !node->value() || (sscanf(node->value(),"%e",&dummyval) != 1) )
    throw runtime_error( "Continuum didnt have UpperEnergy" );
  m_upperEnergy = dummyval;
  
  node = cont_node->first_node( "ReferenceEnergy", 15 );
  if( !node || !node->value() || (sscanf(node->value(),"%e",&dummyval) != 1) )
    throw runtime_error( "Continuum didnt have ReferenceEnergy" );
  m_refernceEnergy = dummyval;
  
  if( m_type != NoOffset && m_type != External )
  {
    xml_node<char> *coeffs_node = cont_node->first_node("Coefficients",12);
    if( !coeffs_node )
      throw runtime_error( "Contoinuum didt have Coefficients node" );
    
    std::vector<float> contents;
    node = coeffs_node->first_node( "Values", 6 );
    if( !node || !node->value() )
      throw runtime_error( "Continuum didnt have Coefficient Values" );
    
    UtilityFunctions::split_to_floats( node->value(), node->value_size(), contents );
    m_values.resize( contents.size() );
    for( size_t i = 0; i < contents.size(); ++i )
      m_values[i] = contents[i]; 
    
    
    node = coeffs_node->first_node( "Uncertainties", 13 );
    if( !node || !node->value() )
      throw runtime_error( "Continuum didnt have Coefficient Uncertainties" );  
    
    UtilityFunctions::split_to_floats( node->value(), node->value_size(), contents );
    m_uncertainties.resize( contents.size() );
    for( size_t i = 0; i < contents.size(); ++i )
      m_uncertainties[i] = contents[i]; 
    
    
    node = coeffs_node->first_node( "Fittable", 8 );
    if( !node || !node->value() )
      throw runtime_error( "Continuum didnt have Coefficient Fittable" );  
    
    UtilityFunctions::split_to_floats( node->value(), node->value_size(), contents );
    m_fitForValue.resize( contents.size() );
    for( size_t i = 0; i < contents.size(); ++i )
      m_fitForValue[i] = (contents[i] > 0.5f); 
    
    if( m_values.size() != m_uncertainties.size() 
        || m_fitForValue.size() != m_values.size() )
      throw runtime_error( "Continuum coefficients not consistent" );
  }else
  {
    m_values.clear();
    m_uncertainties.clear();
    m_fitForValue.clear();
  }//if( m_type != NoOffset && m_type != External ) / else
  
  
  node = cont_node->first_node( "ExternalContinuum", 17 );
  if( node )
  {
    node = node->first_node( "Spectrum", 8 );
    if( !node )
      throw runtime_error( "Spectrum node expected under ExternalContinuum" );
    std::shared_ptr<Measurement> meas = std::make_shared<Measurement>();
    m_externalContinuum = meas;
    meas->set_2006_N42_spectrum_node_info( node );
    
    if( !meas->channel_energies() || meas->channel_energies()->empty() )
      meas->popuplate_channel_energies_from_coeffs();    
  }//if( node )
}//void PeakContinuum::fromXml(...)



rapidxml::xml_node<char> *PeakDef::toXml( rapidxml::xml_node<char> *parent,
                     rapidxml::xml_node<char> *continuum_parent,
           std::map<std::shared_ptr<PeakContinuum>,int> &continuums ) const
{
  using namespace rapidxml;
  
  xml_document<char> *doc = parent ? parent->document() : (xml_document<char> *)0;
  
  if( !doc )
    throw runtime_error( "PeakDef::toXml(...): invalid input" );
  
  if( !m_continuum )
    throw logic_error( "PeakDef::toXml(...): continuum should be valid" );
  
  if( !continuums.count(m_continuum) )
  {
    const int index = static_cast<int>( continuums.size() + 1 );
    m_continuum->toXml( continuum_parent, index );
    continuums[m_continuum] = index;
  }//if( !continuums.count(m_continuum) )
  
  char buffer[128];
  const int contID = continuums[m_continuum];
  
  xml_node<char> *node = 0;
  xml_node<char> *peak_node = doc->allocate_node( node_element, "Peak" );
  
  snprintf( buffer, sizeof(buffer), "%i", sm_xmlSerializationVersion );
  const char *val = doc->allocate_string( buffer );
  xml_attribute<char> *att = doc->allocate_attribute( "version", val );
  peak_node->append_attribute( att );
  
  snprintf( buffer, sizeof(buffer), "%i", contID );
  val = doc->allocate_string( buffer );
  att = doc->allocate_attribute( "continuumID", val );
  peak_node->append_attribute( att );
  
  parent->append_node( peak_node );
  
  if( m_userLabel.size() )
  {
    val = doc->allocate_string( m_userLabel.c_str() );
    node = doc->allocate_node( node_element, "UserLabel", val );
    peak_node->append_node( node );
  }//if( m_userLabel.size() )
  
  
  switch( m_type )
  {
    case GaussianDefined: val = "GaussianDefined"; break;
    case DataDefined:     val = "DataDefined";     break;
  }//switch( m_type )
  
  node = doc->allocate_node( node_element, "Type", val );
  peak_node->append_node( node );
  
  switch( m_skewType )
  {
    case NoSkew:     val = "NoSkew";     break;
    case LandauSkew: val = "LandauSkew"; break;
  }//switch( m_skewType )
  
  node = doc->allocate_node( node_element, "Skew", val );
  peak_node->append_node( node );
  
  
  for( CoefficientType t = CoefficientType(0); 
       t < NumCoefficientTypes; t = CoefficientType(t+1) )
  {
    const char *label = to_string( t );
    
    snprintf( buffer, sizeof(buffer), "%1.8e %1.8e", m_coefficients[t], m_uncertainties[t] );
    val = doc->allocate_string( buffer );
    node = doc->allocate_node( node_element, label, val );
    
    att = doc->allocate_attribute( "fit", (m_fitFor[t] ? "true" : "false") );
    node->append_attribute( att );
    
    peak_node->append_node( node );
  }//for(...)
  
  att = doc->allocate_attribute( "forCalibration", (m_useForCalibration ? "true" : "false") );
  peak_node->append_attribute( att );
  
  att = doc->allocate_attribute( "source", (m_useForShieldingSourceFit ? "true" : "false") );
  peak_node->append_attribute( att );
  
  if( m_useForDetectorResponseFit==0 || m_useForDetectorResponseFit==1 )
  {
    att = doc->allocate_attribute( "useForDrfFit", (m_useForDetectorResponseFit ? "true" : "false") );
    peak_node->append_attribute( att );
  }
  
  if( !m_lineColor.isDefault() )
  {
    //Added 20181027 without incremeneting XML version since we're making it
    //  optional
    val = doc->allocate_string( m_lineColor.cssText(false).c_str() );  //Note: not including alpha because of Wt bug
    node = doc->allocate_node( node_element, "LineColor", val );
    peak_node->append_node( node );
  }//
  
  
  const char *gammaTypeVal = 0;
  switch( m_sourceGammaType )
  {
    case PeakDef::NormalGamma:       gammaTypeVal = "NormalGamma";       break;
    case PeakDef::AnnihilationGamma: gammaTypeVal = "AnnihilationGamma"; break;
    case PeakDef::SingleEscapeGamma: gammaTypeVal = "SingleEscapeGamma"; break;
    case PeakDef::DoubleEscapeGamma: gammaTypeVal = "DoubleEscapeGamma"; break;
    case PeakDef::XrayGamma:         gammaTypeVal = "XrayGamma";         break;
  }//switch( m_sourceGammaType )

  
  if( m_parentNuclide )
  {    
    xml_node<char> *nuc_node = doc->allocate_node( node_element, "Nuclide" );
    peak_node->append_node( nuc_node );
    
    val = doc->allocate_string( m_parentNuclide->symbol.c_str() );
    node = doc->allocate_node( node_element, "Name", val );
    nuc_node->append_node( node );
    
    if( m_transition )
    {
      string transistion_parent, decay_child;

      const SandiaDecay::Nuclide *trans_parent = m_transition->parent;
      transistion_parent = trans_parent->symbol;
      if( m_transition->child )
        decay_child = m_transition->child->symbol;
      const double energy = m_transition->products[m_radparticleIndex].energy;
      
      val = doc->allocate_string( transistion_parent.c_str() );
      node = doc->allocate_node( node_element, "DecayParent", val );
      nuc_node->append_node( node );
      
      val = doc->allocate_string( decay_child.c_str() );
      node = doc->allocate_node( node_element, "DecayChild", val );
      nuc_node->append_node( node );
      
      snprintf( buffer, sizeof(buffer), "%1.8e", energy );
      val = doc->allocate_string( buffer );
      node = doc->allocate_node( node_element, "DecayGammaEnergy", val );
      nuc_node->append_node( node );
    }//if( m_transition )
    
    node = doc->allocate_node( node_element, "DecayGammaType", gammaTypeVal );
    nuc_node->append_node( node );
  }//if( m_parentNuclide )
  
  if( m_xrayElement )
  {
    xml_node<char> *xray_node = doc->allocate_node( node_element, "XRay" );
    peak_node->append_node( xray_node );
    
    val = doc->allocate_string( m_xrayElement->symbol.c_str() );
    node = doc->allocate_node( node_element, "Element", val );
    xray_node->append_node( node );
    
    snprintf( buffer, sizeof(buffer), "%1.8e", m_xrayEnergy );
    val = doc->allocate_string( buffer );
    node = doc->allocate_node( node_element, "Energy", val );
    xray_node->append_node( node );
  }//if( m_xrayElement )
  
  if( m_reaction )
  {
    xml_node<char> *rctn_node = doc->allocate_node( node_element, "Reaction" );
    peak_node->append_node( rctn_node );
    
    val = doc->allocate_string( m_reaction->name().c_str() );
    node = doc->allocate_node( node_element, "Name", val );
    rctn_node->append_node( node );
    
    snprintf( buffer, sizeof(buffer), "%1.8e", m_reactionEnergy );
    val = doc->allocate_string( buffer );
    node = doc->allocate_node( node_element, "Energy", val );
    rctn_node->append_node( node );
    
    node = doc->allocate_node( node_element, "Type", gammaTypeVal );
    rctn_node->append_node( node );
  }//if( m_reaction )
  
  return peak_node;
}//rapidxml::xml_node<char> *toXml(...)



void PeakDef::fromXml( const rapidxml::xml_node<char> *peak_node,
             const std::map<int,std::shared_ptr<PeakContinuum> > &continuums )
{
  using namespace rapidxml;
  using ::rapidxml::internal::compare;
  
  if( !peak_node )
    throw logic_error( "PeakDef::fromXml(...): invalid input node" );
  
  if( !compare( peak_node->name(), peak_node->name_size(), "Peak", 4, false ) )
    throw std::logic_error( "PeakDef::fromXml(...): invalid input node name" );
  
  reset();
  
  int contID;
  xml_attribute<char> *att = peak_node->first_attribute( "continuumID", 11 );
  if( !att )
    throw runtime_error( "No continuum ID" );
  if( sscanf( att->value(), "%i", &contID ) != 1 )
    throw runtime_error( "Non integer continuum ID" );
  
  std::map<int,std::shared_ptr<PeakContinuum> >::const_iterator contpos;
  contpos = continuums.find( contID );
  if( contpos == continuums.end() )
    throw runtime_error( "Couldnt find valud continuum for peak" );
  
  m_continuum = contpos->second;
  
  att = peak_node->first_attribute( "forCalibration", 14 );
  if( !att )
    throw runtime_error( "missing forCalibration attribute" );
  m_useForCalibration = compare(att->value(),att->value_size(),"true",4,false);
  if( !m_useForCalibration && !compare(att->value(),att->value_size(),"false",5,false) )
    throw runtime_error( "invalis forCalibration value" );
  
  att = peak_node->first_attribute( "source", 6 );
  if( !att )
    throw runtime_error( "missing source attribute" );
  m_useForShieldingSourceFit = compare(att->value(),att->value_size(),"true",4,false);
  if( !m_useForShieldingSourceFit && !compare(att->value(),att->value_size(),"false",5,false) )
    throw runtime_error( "invalid source value" );
  
  att = peak_node->first_attribute( "useForDrfFit", 12 );
  if( att )
    m_useForDetectorResponseFit = compare(att->value(),att->value_size(),"true",4,false);
  else
    m_useForDetectorResponseFit = -1;
  
  if( att && !m_useForDetectorResponseFit && !compare(att->value(),att->value_size(),"false",5,false) )
    throw runtime_error( "invalid use for DRF fit value" );
  
  att = peak_node->first_attribute( "version", 7 );
  if( !att )
    throw runtime_error( "missing version attribute" );
  
  int version;
  if( sscanf( att->value(), "%i", &version ) != 1 )
    throw runtime_error( "Non integer version number" );
  
  if( version != sm_xmlSerializationVersion )
    throw runtime_error( "Invalid peak version" );
  
  xml_node<char> *node = peak_node->first_node("UserLabel",9);
  if( node && node->value() )
    m_userLabel = node->value();
  
  node = peak_node->first_node("Type",4);
  if( !node || !node->value() )
    throw runtime_error( "No peak type" );
  
  if( compare(node->value(),node->value_size(),"GaussianDefined",15,false) )
    m_type = GaussianDefined;
  else if( compare(node->value(),node->value_size(),"DataDefined",11,false) )
    m_type = DataDefined;
  else
    throw runtime_error( "Invalid peak type" );
  
  node = peak_node->first_node("Skew",4);
  if( !node || !node->value() )
    throw runtime_error( "No peak skew type" );
  
  
  if( compare(node->value(),node->value_size(),"NoSkew",6,false) )
    m_skewType = NoSkew;
  else if( compare(node->value(),node->value_size(),"LandauSkew",10,false) )
    m_skewType = LandauSkew;
  else
    throw runtime_error( "Invalid peak skew type" );
    
  
  for( CoefficientType t = CoefficientType(0); 
      t < NumCoefficientTypes; t = CoefficientType(t+1) )
  {
    const char *label = to_string( t );
    
    node = peak_node->first_node(label);
    if( !node || !node->value() )
      throw runtime_error( "No coefficent " + string(label) );
    
    float dblval, dbluncrt;
    if( sscanf(node->value(), "%g %g", &dblval, &dbluncrt) != 2 )
      throw runtime_error( "unable to read value or uncert for " + string(label) );
    
    m_coefficients[t] = dblval;
    m_uncertainties[t] = dbluncrt;
    
    att = node->first_attribute("fit",3);
    if( !att || !att->value() )
      throw runtime_error( "No fit attribute for " + string(label) );
    
    m_fitFor[t] = compare(att->value(),att->value_size(),"true",4,false);
    if( !m_fitFor[t] && !compare(att->value(),att->value_size(),"false",5,false) )
      throw runtime_error( "invalid fit value" );
  }//for(...)

  
  xml_node<char> *line_color_node = peak_node->first_node("LineColor",9);
  if( line_color_node && line_color_node->value_size()==7 )
  {
    //Added 20181027 without incremeneting XML version since we're making it
    //  optional
    const string color = string( line_color_node->value(), line_color_node->value_size() );
    try
    {
      m_lineColor = Wt::WColor(color);
    }catch(...)
    {
      m_lineColor = Wt::WColor();
    }
  }else
  {
    m_lineColor = Wt::WColor();
  }
  
  xml_node<char> *nuc_node = peak_node->first_node("Nuclide",7);
  xml_node<char> *xray_node = peak_node->first_node("XRay",4);
  xml_node<char> *rctn_node = peak_node->first_node("Reaction",8);
  
  try
  {
  
    if( nuc_node )
    {
      const SandiaDecay::SandiaDecayDataBase *db = DecayDataBaseServer::database();
      xml_node<char> *name_node = nuc_node->first_node("Name",4);
      xml_node<char> *p_node = nuc_node->first_node("DecayParent",11);
      xml_node<char> *c_node = nuc_node->first_node("DecayChild",10);
      xml_node<char> *e_node = nuc_node->first_node("DecayGammaEnergy",16);
      xml_node<char> *type_node = nuc_node->first_node("DecayGammaType",14);
    
      const bool isNormalNucTrans = (p_node && c_node && e_node && name_node->value()
                                    && p_node->value() && c_node->value() && e_node->value());
    
      bool gotGammaType = false;
      const char *typeval = type_node->value();
      const size_t typelen = type_node->value_size();
    
      if( compare( typeval, typelen, "NormalGamma", 11, false ) )
      {
        gotGammaType = true;
        m_sourceGammaType = PeakDef::NormalGamma;
      }else if( compare( typeval, typelen, "AnnihilationGamma", 17, false ) )
      {
        gotGammaType = true;
        m_sourceGammaType = PeakDef::AnnihilationGamma;
      }else if( compare( typeval, typelen, "SingleEscapeGamma", 17, false ) )
      {
        gotGammaType = true;
        m_sourceGammaType = PeakDef::SingleEscapeGamma;
      }else if( compare( typeval, typelen, "DoubleEscapeGamma", 17, false ) )
      {
        gotGammaType = true;
        m_sourceGammaType = PeakDef::DoubleEscapeGamma;
      }else if( compare( typeval, typelen, "XrayGamma", 9, false ) )
      {
        gotGammaType = true;
        m_sourceGammaType = PeakDef::XrayGamma;
      }

      if( !name_node || !gotGammaType )
        throw runtime_error( "Invalidly specified nuclide" );
    
      m_parentNuclide = db->nuclide( name_node->value() );
    
      if( isNormalNucTrans )
      {
        const SandiaDecay::Nuclide *parent = db->nuclide( p_node->value() );
        const SandiaDecay::Nuclide *child = db->nuclide( c_node->value() );
    
        if( !m_parentNuclide )
          throw runtime_error( "Invalid nuclide name " + string(name_node->value()) );
    
        float decay_gamma_energy;
        if( sscanf( e_node->value(), "%g", &decay_gamma_energy ) != 1 )
          throw runtime_error( "Invalid nuclide gamma energy" );
    
        for( size_t i = 0; i < parent->decaysToChildren.size(); ++i )
        {
          const SandiaDecay::Transition *trans = parent->decaysToChildren[i];
          if( trans->parent==parent && trans->child==child )
          {
            size_t nearest = 0;
            double delta_eneregy = 999.9;
            for( size_t j = 0; j < trans->products.size(); ++j )
            {
              const SandiaDecay::RadParticle &particle = trans->products[j];
              if( particle.type == SandiaDecay::GammaParticle
                  || (m_sourceGammaType == PeakDef::AnnihilationGamma
                      && particle.type == SandiaDecay::PositronParticle)
                  || (m_sourceGammaType == PeakDef::XrayGamma
                     && particle.type == SandiaDecay::XrayParticle)
                 )
              {
                const double de = fabs( decay_gamma_energy - particle.energy );
                if( de < delta_eneregy )
                {
                  delta_eneregy = de;
                  nearest = j;
                }//if( de < delta_eneregy )
              }//if( particle.type == SandiaDecay::GammaParticle )
            }//for( size_tj = 0; j < trans->products.size(); ++j )
        
            if( delta_eneregy > 1.0 )
              throw std::runtime_error( "Couldnt find gamma near in energy to "
                                     + string(e_node->value()) + " keV" );
        
            m_radparticleIndex = static_cast<int>(nearest);
            m_transition = trans;
            i = parent->decaysToChildren.size();
          }//if( this transition matches )
        }//for( size_t i = 0; i < parent->decaysToChildren.size(); ++i )
      
        if( !m_transition && (m_sourceGammaType != PeakDef::AnnihilationGamma) )
        {
          if( parent && child )
            throw std::runtime_error( "Couldnt find specified transition for "
                                     + parent->symbol + " to " + child->symbol );
          else
            throw std::runtime_error( "Couldnt find specified transition" );
        }//if( !m_transition )
      }//if( isNormalNucTrans )
    }//if( nuc_node )
  
  
    if( xray_node )
    {
      const SandiaDecay::SandiaDecayDataBase *db = DecayDataBaseServer::database();
  
      xml_node<char> *el_node = xray_node->first_node("Element",7);
      xml_node<char> *energy_node = xray_node->first_node("Energy",6);
  
      if( !el_node || !el_node->value() || !energy_node || !energy_node->value() )
        throw runtime_error( "Ill specified xray" );
    
      m_xrayElement = db->element( el_node->value() );
      if( !m_xrayElement )
        throw std::runtime_error( "Couldnt retrieve x-ray element" );
    
      float dummyval;
      if( sscanf( energy_node->value(), "%g", &dummyval ) != 1 )
        throw runtime_error( "non numeric xray energy" );
      m_xrayEnergy = dummyval;
    }//if( xray_node )
  
    if( rctn_node )
    {
      const ReactionGamma *rctns = ReactionGammaServer::database();
    
      xml_node<char> *name_node   = rctn_node->first_node("Name",4);
      xml_node<char> *energy_node = rctn_node->first_node("Energy",6);
      xml_node<char> *type_node   = rctn_node->first_node("Type",4);
    
      if( !name_node || !name_node->value() || !energy_node || !energy_node->value() )
        throw runtime_error( "Ill specified reaction" );
    
      float dummyval;
      if( sscanf( energy_node->value(), "%g", &dummyval ) != 1 )
        throw runtime_error( "non numeric reaction energy" );
      m_reactionEnergy = dummyval;
  
      //We will default do NormalGamma since early versions of serializtion didnt
      //  write the type
      m_sourceGammaType = PeakDef::NormalGamma;
    
      if( type_node )
      {
        const char *typeval = type_node->value();
        const size_t typelen = type_node->value_size();
  
        if( compare( typeval, typelen, "NormalGamma", 11, false ) )
          m_sourceGammaType = PeakDef::NormalGamma;
        else if( compare( typeval, typelen, "AnnihilationGamma", 17, false ) )
          m_sourceGammaType = PeakDef::AnnihilationGamma;
        else if( compare( typeval, typelen, "SingleEscapeGamma", 17, false ) )
          m_sourceGammaType = PeakDef::SingleEscapeGamma;
        else if( compare( typeval, typelen, "DoubleEscapeGamma", 17, false ) )
          m_sourceGammaType = PeakDef::DoubleEscapeGamma;
        else if( compare( typeval, typelen, "XrayGamma", 9, false ) )
          m_sourceGammaType = PeakDef::XrayGamma;
      }//if( type_node )

    
      const string reaction = name_node->value();
      std::vector<const ReactionGamma::Reaction *> candidates;
      rctns->reactions( float(m_reactionEnergy-1.0),
                        float(m_reactionEnergy+1.0), candidates );
      for( size_t i = 0; i < candidates.size(); ++i )
        if( candidates[i]->name() == reaction )
          m_reaction = candidates[i];
      //Could have called ReactionGamma::gammas( const string &name,...) as well
      //  rather than doing the manual search above
    
      if( !m_reaction )
        throw std::runtime_error( "Couldnt find reaction '" + reaction + "'" );
    }//if( rctn_node )
  }catch( std::exception &e )
  {
    m_radparticleIndex = -1;
    m_transition = NULL;
    m_sourceGammaType = NormalGamma;
    m_parentNuclide = NULL;
    if( wApp )
    {
      stringstream msg;
      msg << "Failed to assign peak at " << mean() << " keV to nuclide/xray/reaction: " << e.what();
      passMessage( msg.str(), "", WarningWidget::WarningMsgHigh );
    }
  }
}//void fromXml(...)

#if( SpecUtils_ENABLE_D3_CHART )
std::string PeakDef::gaus_peaks_to_json(const std::vector<std::shared_ptr<const PeakDef> > &peaks)
{
	stringstream answer;
	if (peaks.empty())
		return answer.str();

	std::shared_ptr<const PeakContinuum> continuum = peaks[0]->continuum();
	if (!continuum)
		throw runtime_error("gaus_peaks_to_json: invalid continuum");
	const char *q = "\""; // for creating valid json format

	answer << "{" << q << "type" << q << ":" << q;
	switch (continuum->type())
	{
	  case PeakContinuum::NoOffset:   answer << "NoOffset";   break;
	  case PeakContinuum::Constant:   answer << "Constant";   break;
	  case PeakContinuum::Linear:     answer << "Linear";     break;
	  case PeakContinuum::Quardratic: answer << "Quardratic"; break;
	  case PeakContinuum::Cubic:      answer << "Cubic";      break;
	  case PeakContinuum::External:   answer << "External";   break;
	}//switch( continuum->type() )
	answer << q << "," << q << "lowerEnergy" << q << ":" << continuum->lowerEnergy()
		<< "," << q << "upperEnergy" << q << ":" << continuum->upperEnergy();

  switch( continuum->type() )
  {
    case PeakContinuum::NoOffset:
      break;
      
    case PeakContinuum::Constant:
    case PeakContinuum::Linear:
    case PeakContinuum::Quardratic:
    case PeakContinuum::Cubic:
    {
      answer << "," << q << "referenceEnergy" << q << ":" << continuum->referenceEnergy();
      const vector<double> &values = continuum->parameters();
      const vector<double> &uncerts = continuum->unertainties();
      answer << "," << q << "coeffs" << q << ":[";
      for (size_t i = 0; i < values.size(); ++i)
        answer << (i ? "," : "") << values[i];
      answer << "]," << q << "coeffUncerts" << q << ":[";
      for (size_t i = 0; i < uncerts.size(); ++i)
        answer << (i ? "," : "") << uncerts[i];
      answer << "]";
      
      answer << "," << q << "fitForCoeff" << q << ":[";
      for (size_t i = 0; i < continuum->fitForParameter().size(); ++i)
        answer << (i ? "," : "") << (continuum->fitForParameter()[i] ? "true" : "false");
      answer << "]";
      
      break;
    }//polynomial continuum
      
      
    case PeakContinuum::External:
    {
      if( continuum->externalContinuum()
          && continuum->externalContinuum()->num_gamma_channels() )
      {
        std::shared_ptr<const Measurement> hist = continuum->externalContinuum();
        size_t firstbin = hist->find_gamma_channel(continuum->lowerEnergy());
        size_t lastbin = hist->find_gamma_channel(continuum->upperEnergy());
        
        if (firstbin > 0)
          --firstbin;
        if (firstbin > 0)
          --firstbin;
        if (lastbin < (hist->num_gamma_channels() - 1))
          ++lastbin;
        if (lastbin < (hist->num_gamma_channels() - 1))
          ++lastbin;
        
        answer << "," << q << "continuumEnergies" << q << ":[";
        for (size_t i = 0; i <= lastbin; ++i)
          answer << (i ? "," : "") << hist->gamma_channel_lower(i);
        answer << "]," << q << "continuumCounts" << q << ":[";
        for (size_t i = 0; i <= lastbin; ++i)
          answer << (i ? "," : "") << hist->gamma_channel_content(i);
        answer << "]";
      }
    }//case PeakContinuum::External:
  }//switch( continuum->type() )


	answer << "," << q << "peaks" << q << ":[";
	for (size_t i = 0; i < peaks.size(); ++i)
	{
		const PeakDef &p = *peaks[i];
		if (continuum != p.continuum())
			throw runtime_error("gaus_peaks_to_json: peaks all must share same continuum");
		answer << (i ? "," : "") << "{";

		if (p.userLabel().size())
			answer << q << "userLabel" << q << ":" << q << Wt::WWebWidget::escapeText(p.userLabel()) << q << ",";

		if (!p.lineColor().isDefault())
			answer << q << "lineColor" << q << ":" << q << p.lineColor().cssText(false) << q << ",";

		answer << q << "type" << q << ":";
		switch (p.type())
		{
		  case PeakDef::GaussianDefined:
        answer << q << "GaussianDefined" << q << ",";
      break;
        
		  case PeakDef::DataDefined:
        answer << q << "DataDefined" << q << ",";
      break;
		}//switch( p.type() )

		answer << q << "skewType" << q << ":";
		switch( p.skewType() )
		{
		  case PeakDef::NoSkew:
        answer << q << "NoSkew" << q << ",";
        break;
        
		  case PeakDef::LandauSkew:
        answer << q << "LandauSkew" << q << ",";
        break;
		}//switch( p.type() )

		for (PeakDef::CoefficientType t = PeakDef::CoefficientType(0);
			t < PeakDef::NumCoefficientTypes; t = PeakDef::CoefficientType(t + 1))
		{
			answer << q << PeakDef::to_string(t) << q << ":[" << p.coefficient(t)
				<< "," << p.uncertainty(t) << "," << (p.fitFor(t) ? "true" : "false")
				<< "],";
		}//for(...)

		answer << q << "forCalibration" << q << ":" << (p.useForCalibration() ? "true" : "false")
			<< "," << q << "forSourceFit" << q << ":" << (p.useForShieldingSourceFit() ? "true" : "false");

		const char *gammaTypeVal = 0;
		switch (p.sourceGammaType())
		{
		case PeakDef::NormalGamma:       gammaTypeVal = "NormalGamma";       break;
		case PeakDef::AnnihilationGamma: gammaTypeVal = "AnnihilationGamma"; break;
		case PeakDef::SingleEscapeGamma: gammaTypeVal = "SingleEscapeGamma"; break;
		case PeakDef::DoubleEscapeGamma: gammaTypeVal = "DoubleEscapeGamma"; break;
		case PeakDef::XrayGamma:         gammaTypeVal = "XrayGamma";         break;
		}//switch( p.sourceGammaType() )

		if (p.parentNuclide() || p.xrayElement() || p.reaction())
			answer << "," << q << "sourceType" << q << ":" << q << gammaTypeVal << q;

		if (p.parentNuclide())
		{
			const SandiaDecay::Transition *trans = p.nuclearTransition();
			const SandiaDecay::RadParticle *decayPart = p.decayParticle();

			answer << "," << q << "nuclide" << q << ": { " << q << "name" << q << ": " << q << p.parentNuclide()->symbol << q;
			if (trans && decayPart)
			{
				string transistion_parent, decay_child;
				const SandiaDecay::Nuclide *trans_parent = trans->parent;
				transistion_parent = trans_parent->symbol;
				if (trans->child)
					decay_child = trans->child->symbol;

				answer << "," << q << "decayParent" << q << ":" << q << transistion_parent << q;
				answer << "," << q << "decayChild" << q << ":" << q << decay_child << q;
				answer << "," << q << "DecayGammaEnergy" << q << ":" << decayPart->energy << "";
			}//if( m_transition )

			answer << "}";
		}//if( m_parentNuclide )


		if (p.xrayElement())
		{
			answer << "," << q << "xray" << q << ": {" << q << "element" << q << ":" << q << p.xrayElement() << q
				<< "," << q << "energy" << q << ":" << p.xrayEnergy() << "}";
		}//if( m_xrayElement )


		if (p.reaction())
		{
			answer << "," << q << "reaction" << q << ":{" << q << "name" << q << ":" << q << p.reaction()->name() << q << "," << q << "energy" << q << ":"
				<< p.reactionEnergy() << "}";
		}//if( m_reaction )

		answer << "}";
	}//for( size_t i = 0; i < peaks.size(); ++i )

	answer << "]}";

	return answer.str();
}//std::string PeakDef::gaus_peaks_to_json(...)


string PeakDef::peak_json(const vector<std::shared_ptr<const PeakDef> > &inpeaks)
{
	if (inpeaks.empty())
		return "[]";

	typedef std::map< std::shared_ptr<const PeakContinuum>, vector<std::shared_ptr<const PeakDef> > > ContinuumToPeakMap_t;

	ContinuumToPeakMap_t continuumToPeaks;
	for (size_t i = 0; i < inpeaks.size(); ++i)
		continuumToPeaks[inpeaks[i]->continuum()].push_back(inpeaks[i]);

	string json = "[";
	for (const ContinuumToPeakMap_t::value_type &vt : continuumToPeaks)
		json += ((json.size()>2) ? "," : "") + gaus_peaks_to_json(vt.second);

	json += "]";
	return json;
}//string peak_json( inpeaks )
#endif //#if( SpecUtils_ENABLE_D3_CHART )

std::shared_ptr<PeakContinuum> PeakDef::continuum()
{
  return m_continuum;
}

std::shared_ptr<const PeakContinuum> PeakDef::continuum() const
{
  return m_continuum;
}

std::shared_ptr<PeakContinuum> PeakDef::getContinuum()
{
  return m_continuum;
}

void PeakDef::setContinuum( std::shared_ptr<PeakContinuum> continuum )
{
  if( !continuum )
    throw runtime_error( "PeakDef::setContinuum(...): invalid input" );
  m_continuum = continuum;
}//void setContinuum(...)


void PeakDef::makeUniqueNewContinuum()
{
  m_continuum = std::make_shared<PeakContinuum>(*m_continuum);
}


//The below should in principle take care of gaussian area and the skew area
double PeakDef::peakArea() const
{
  double amp = m_coefficients[PeakDef::GaussAmplitude];
  
  switch( m_skewType )
  {
    case PeakDef::NoSkew:
    break;
    
    case PeakDef::LandauSkew:
      amp += skew_integral( lowerX(), upperX() );
    break;
  }//switch( m_skewType )
  
  return amp;
}//double peakArea() const


double PeakDef::peakAreaUncert() const
{
  double uncert = m_uncertainties[PeakDef::GaussAmplitude];
  
  switch( m_skewType )
  {
    case PeakDef::NoSkew:
    break;
      
    case PeakDef::LandauSkew:
    {
      const double skew_area = skew_integral( lowerX(), upperX() );
      const double frac_uncert = m_uncertainties[PeakDef::LandauAmplitude]
                                 / m_coefficients[PeakDef::LandauAmplitude];
      const double skew_uncert = skew_area * frac_uncert;
      //XXX - below assumes ampltude and skew amplitude are uncorelated,
      //      which they are not.
      uncert = sqrt( uncert*uncert + skew_uncert*skew_uncert );
      break;
    }//case PeakDef::LandauSkew:
  }//switch( m_skewType )
  
  return uncert;
}//double peakAreaUncert() const


void PeakDef::setPeakArea( const double a )
{
  double area = a; //m_coefficients[PeakDef::GaussAmplitude];
  
  switch( m_skewType )
  {
    case PeakDef::NoSkew:
    break;
      
    case PeakDef::LandauSkew:
    {
      const double skew_area = skew_integral( lowerX(), upperX() );
      const double skew_frac = skew_area / (skew_area + area);
      area = (1.0 - skew_frac) * a;
      break;
    }//case PeakDef::LandauSkew:
  }//switch( m_skewType )
  
  m_coefficients[PeakDef::GaussAmplitude] = area;
}//void PeakDef::setPeakArea( const double a )


void PeakDef::setPeakAreaUncert( const double uncert )
{
  switch( m_skewType )
  {
    case PeakDef::NoSkew:
      m_uncertainties[PeakDef::GaussAmplitude] = uncert;
    break;
      
    case PeakDef::LandauSkew:
    {
      //XXX - the below only modifies the gaus uncertainty, and not the skew
      //  uncertainty - I was just to lazy to do it properly (should also look
      //  at how coorelated the skew amplitude error is to peak amplitude error
      //  ...)
      const double skew_area = skew_integral( lowerX(), upperX() );
      const double gauss_area = m_coefficients[PeakDef::GaussAmplitude];
      const double total_area = skew_area + gauss_area;
      const double frac_uncert = uncert / total_area;
      const double new_gauss_uncert = frac_uncert * gauss_area;
      m_uncertainties[PeakDef::GaussAmplitude] = new_gauss_uncert;
      
      /*
      const double gaussuncert = m_coefficients[PeakDef::GaussAmplitude];
      const double skewuncert = m_coefficients[PeakDef::LandauAmplitude];
      
      if( skewuncert > 0.0 && skew_area > 0.0 )
      {
        const double gauss_area = m_coefficients[PeakDef::GaussAmplitude];
        const double area = skew_area + gauss_area;
        const double skew_uncert = skew_area * skewuncert
                                   / m_coefficients[PeakDef::LandauAmplitude];
        const double skew_frac_uncert = skew_uncert / (skew_uncert+gaussuncert);
        
        const double skew_uncert = skew_area * skew_frac_uncert
        
        m_uncertainties[PeakDef::GaussAmplitude] = fracuncert*gauss_area;
        m_uncertainties[PeakDef::LandauAmplitude] = fracuncert*;
      }else
      {
        m_uncertainties[PeakDef::GaussAmplitude] = uncert;
      }//if( skewuncert > 0.0 ) / else
       */
      break;
    }//case PeakDef::LandauSkew:
  }//switch( m_skewType )
}//void setPeakAreaUncert( const double a )


double PeakDef::areaFromData( std::shared_ptr<const Measurement> data ) const
{
  double sumval = 0.0;
  if( !data || !m_continuum || !m_continuum->energyRangeDefined() )
    return sumval;
  
  try
  {
    const float energyStart = (float)m_continuum->lowerEnergy();
    const float energyEnd = (float)m_continuum->upperEnergy();
    
    const size_t lower_channel = data->find_gamma_channel( energyStart );
    const size_t upper_channel = data->find_gamma_channel(energyEnd);
    
    for( size_t i = lower_channel; i <= upper_channel; ++i )
    {
      const float e0 = std::max( energyStart, data->gamma_channel_lower(i) );
      const float e1 = std::min( energyEnd, data->gamma_channel_upper(i) );
      const double data_area_i = data->gamma_integral(e0, e1);
      const double cont_area_1 = m_continuum->offset_integral(e0, e1);
      if( data_area_i > cont_area_1 )
        sumval += (data_area_i - cont_area_1);
    }
  }catch( std::exception &e )
  {
    cerr << "Caught exception in PeakDef::areaFromData(): " << e.what() << endl;
    return 0.0;
  }
  
  return sumval;
}//double areaFromData( std::shared_ptr<const Measurement> data ) const;

bool PeakDef::lessThanByMean( const PeakDef &lhs, const PeakDef &rhs )
{
  return (lhs.m_coefficients[PeakDef::Mean] < rhs.m_coefficients[PeakDef::Mean]);
}//lessThanByMean(...)

bool PeakDef::lessThanByMeanShrdPtr( const std::shared_ptr<const PeakDef> &lhs,
                                  const std::shared_ptr<const PeakDef> &rhs )
{
  if( !lhs || !rhs )
    return (lhs < rhs);
  return lessThanByMean( *lhs, *rhs );
}


bool PeakDef::causilyDisconnected( const PeakDef &lower_peak,
                                    const PeakDef &upper_peak,
                                    const double ncausality,
                                   const bool useRoiAsWell )
{
  if( lower_peak.continuum() == upper_peak.continuum() )
    return false;
  
  double lower_upper( 0.0 ), upper_lower( 0.0 );

  if( lower_peak.mean() < upper_peak.mean() )
  {
    lower_upper = lower_peak.gausPeak() ? lower_peak.mean() + ncausality*lower_peak.sigma() : lower_peak.upperX();
    upper_lower = upper_peak.gausPeak() ? upper_peak.mean() - ncausality*upper_peak.sigma() : upper_peak.lowerX();
    if( useRoiAsWell )
    {
      lower_upper = std::max( lower_upper, lower_peak.upperX() );
      upper_lower = std::min( upper_lower, upper_peak.lowerX() );
    }
  }else
  {
    lower_upper = upper_peak.gausPeak() ? upper_peak.mean() + ncausality*upper_peak.sigma() : upper_peak.upperX();
    upper_lower = lower_peak.gausPeak() ? lower_peak.mean() - ncausality*lower_peak.sigma() : lower_peak.lowerX();
    if( useRoiAsWell )
    {
      lower_upper = std::max( lower_upper, upper_peak.upperX() );
      upper_lower = std::min( upper_lower, lower_peak.lowerX() );
    }
  }//if( lower_peak.mean() < upper_peak.mean() ) / else

  return (upper_lower > lower_upper);
}//bool PeakDef::causilyDisconnected


bool PeakDef::causilyConnected( const PeakDef &lower_peak,
                                   const PeakDef &upper_peak,
                                   const double ncausality,
                                   const bool useRoiAsWell )
{
  return !causilyDisconnected( lower_peak, upper_peak, ncausality, useRoiAsWell );
}


bool PeakDef::ageFitNotAllowed( const SandiaDecay::Nuclide *nuc )
{
  if( !nuc || nuc->decaysToStableChildren() )
    return true;
  
  //now check for cases like Cs137 where the isotope reqches prompt and
  //  secular equilibrium very quickly (half life for these less than a day)
  //  and these time spans are less than the parents half life
  const double hl = nuc->halfLife;
  const double prompt = nuc->promptEquilibriumHalfLife();
  const double secular = nuc->secularEquilibriumHalfLife();
  
  //simpleFast: will catch cases like Cs137 where the element decays to
  //  very short lived daughters
  const bool simpleFast = ( (prompt > DBL_MIN)    //can obtain prompt equilibrium
                           && (prompt < 0.01*hl) //prompt half life less than 1% parents half life
                           && (secular < hl)     //can obtain secular equilibrium
                           && (prompt < 86400.0*SandiaDecay::second) //half lives for both of these is less than a day (an arbitrary time period)
                           && (secular < 86400.0*SandiaDecay::second) );
  
  if( simpleFast )
    return true;
  
  
  //catch cases like W187 who none of its descendants give off gammas
  for( const SandiaDecay::Transition *t : nuc->decaysToChildren )
    if( gives_off_gammas( t->child ) )
      return false;
  
  //if( prompt && (decsendnats of promp nuclide dont give off gammas) )
  //  return true;
  //return false;
  
  return true;
  /*
   //Check for cases like W187, whose none of its prodigeny give off gammas
   const vector<const SandiaDecay::Transition *> &decays = nuc->decaysToChildren;
   
   
   for( size_t decN = 0; decN < decays.size(); ++decN )
   {
   const SandiaDecay::Transition *trans = decays[decN];
   if( trans->child )
   {
   }
   }//for( size_t decN = 0; decN < decays.size(); ++decN )
   
   
   return false;
   */
}//bool ageFitNotAllowed( const SandiaDecay::Nuclide *nuc )


double PeakDef::defaultDecayTime( const SandiaDecay::Nuclide *nuclide, string *stranswer )
{
  //Same logic as defaultDecayTime(...), just returns string. If you change
  // the logic in this funcion, you should also change defaultDecayTime(...).
  string decayTimeStr = "";
  double decaytime = 0;
  if( nuclide->canObtainSecularEquilibrium() )
  {
    decaytime = 10.0*nuclide->secularEquilibriumHalfLife();
  }else
  {
    decaytime = 7.0 * nuclide->promptEquilibriumHalfLife();
  }
  
  if( decaytime <= 0.0 || decaytime < 0.5*nuclide->halfLife )
  {
    decaytime = 2.5 * nuclide->halfLife;
    decayTimeStr = "2.5 HL";
  }
  
  if( nuclide->decaysToStableChildren() )
  {
    decaytime = 0.0;
    decayTimeStr = "0 s";
  }
  
  if( nuclide->halfLife > 100.0*SandiaDecay::year )
  {
    decaytime = 20.0 * SandiaDecay::year;
    decayTimeStr = "20 y";
  }
  
  //I *think* promptEquilibriumHalfLife() can maybe give a large value for an
  //  isotope (although I dont know of any examples of this) with a small half
  //  life, so we'll preotect against it.
  if( decaytime > 100.0*nuclide->halfLife )
  {
    decaytime = 7.0 * nuclide->halfLife;
    decayTimeStr = "7 HL";
  }
  
  if( decayTimeStr.empty() )
    decayTimeStr = PhysicalUnits::printToBestTimeUnits( decaytime, 2, SandiaDecay::second );
  
  if( stranswer )
    *stranswer = decayTimeStr;
  
  return decaytime;
}//string defaultDecayTime(..)


void PeakDef::findNearestPhotopeak( const SandiaDecay::Nuclide *nuclide,
                                     const double energy,
                                     const double windowHalfWidth,
                                     const SandiaDecay::Transition *&transition,
                                     size_t &transition_index,
                                     SourceGammaType &sourceGammaType )
{
  transition = NULL;
  transition_index = 0;
  sourceGammaType = PeakDef::NormalGamma;
  
  if( !nuclide )
    return;
  
  SandiaDecay::NuclideMixture mixture;
  mixture.addNuclide( SandiaDecay::NuclideActivityPair(nuclide,1.0) );
  
  const double decaytime = defaultDecayTime( nuclide );
  
  vector<SandiaDecay::EnergyRatePair> gammas
  = mixture.gammas( decaytime,
                   SandiaDecay::NuclideMixture::OrderByAbundance, true );
  
  if( gammas.empty() )
    return;
  
  map<const SandiaDecay::Transition *, vector<size_t> > ec_trans;
  
  
  double best_delta_e = 99999.9;
  SandiaDecay::EnergyRatePair nearest_gamma = gammas[0];
  for( const SandiaDecay::EnergyRatePair &gamma : gammas )
  {
    const double delta_e = fabs( gamma.energy - energy );
    if( delta_e < best_delta_e )
    {
      best_delta_e = delta_e;
      nearest_gamma = gamma;
    }//if( delta_e < best_delta_e )
  }//for( const SandiaDecay::EnergyRatePair &gamma : gammas )
  
  //loop over the decays and find the gamma nearest 'energy'
  best_delta_e = 99999.9;
  
  const vector<SandiaDecay::NuclideActivityPair> activities
                                                = mixture.activity( decaytime );
  double minEnergy = energy - windowHalfWidth;
  double maxEnergy = energy + windowHalfWidth;
  if( windowHalfWidth <= 0.0 )
  {
    minEnergy = -std::numeric_limits<double>::max();
    maxEnergy = std::numeric_limits<double>::max();
  }//if( windowHalfWidth <= 0.0 )
  
  
  double max_intensity = 0.0;
  for( SandiaDecay::NuclideActivityPair activity : activities )
  {
    for( const SandiaDecay::Transition *trans : activity.nuclide->decaysToChildren )
    {
      const vector<SandiaDecay::RadParticle> &products = trans->products;
      for( size_t index = 0; index < products.size(); ++index )
      {
        const SandiaDecay::RadParticle &product = products[index];
        
        float energy;
        if( product.type == SandiaDecay::GammaParticle || product.type == SandiaDecay::XrayParticle )
          energy = product.energy;
        else if( product.type == SandiaDecay::PositronParticle )
          energy = 510.998910f;
        else
          continue;
        
        if( energy > maxEnergy || energy < minEnergy )
          continue;
        
        double intensity = activity.activity * trans->branchRatio * product.intensity;
        max_intensity = max( max_intensity, intensity );
      }//for( size_t index = 0; index < products.size(); ++index )
    }//for( const SandiaDecay::Transition *trans : activity.nuclide->decaysToChildren )
  }//for( SandiaDecay::NuclideActivityPair activity : activities )
  
  double positronfrac = 0.0;
  
  for( SandiaDecay::NuclideActivityPair activity : activities )
  {
    for( const SandiaDecay::Transition *trans : activity.nuclide->decaysToChildren )
    {
      const vector<SandiaDecay::RadParticle> &products = trans->products;
      for( size_t index = 0; index < products.size(); ++index )
      {
        const SandiaDecay::RadParticle &product = products[index];
        
        if( product.type == SandiaDecay::PositronParticle )
        {
          const double energy = 510.998910;
          if( energy > maxEnergy || energy < minEnergy )
            continue;
          
          double intensity = activity.activity * trans->branchRatio * product.intensity;
          positronfrac += intensity / max_intensity;
          ec_trans[trans].push_back( index );
        }else if( product.type == SandiaDecay::GammaParticle || product.type == SandiaDecay::XrayParticle )
        {
          if( product.energy > maxEnergy || product.energy < minEnergy )
            continue;
          
          double intensity = activity.activity * trans->branchRatio * product.intensity;
          const double fracIntensity = intensity / max_intensity;
          const double minRelativeBr = 1.0E-10;
          
          if( fracIntensity < minRelativeBr )
            continue;
          
          if( fracIntensity <= 0.0 )
            continue;
          
//          const double delta_e = fabs( product.energy - nearest_gamma.energy ); //XXX - I think nearest_gamma.energy should just be energy (wcjohns 20140711)
          const double delta_e = fabs( product.energy - energy);
          double scaleDeltaE = (0.1*windowHalfWidth + delta_e) / fracIntensity;
          if( windowHalfWidth <= 0.0 )
            scaleDeltaE = delta_e;
          
          if( scaleDeltaE <= best_delta_e )
          {
            best_delta_e       = scaleDeltaE;
            transition         = trans;
            transition_index   = index;
            sourceGammaType = ((product.type==SandiaDecay::GammaParticle)
                                ? PeakDef::NormalGamma : PeakDef::XrayGamma);
          }//if( delta_e < best_delta_e )
        }//if( positron ) / else if( gamma )
      }//for( const SandiaDecay::RadParticle &particle : products )
    }//for( const SandiaDecay::Transition *trans : decays)
  }//for( SandiaDecay::NuclideActivityPair activity : activities )
  
  if( positronfrac > 0.0 )
  {
    const double delta_e = fabs( 510.998910 - energy );
    double scaleDeltaE = (0.1*windowHalfWidth + delta_e) / positronfrac;
    if( windowHalfWidth <= 0.0 )
      scaleDeltaE = delta_e;
    
    if( scaleDeltaE <= best_delta_e )
    {
      sourceGammaType = PeakDef::AnnihilationGamma;
      
      if( ec_trans.size()==1 && ec_trans.begin()->second.size()==1 )
      {
        transition         = ec_trans.begin()->first;
        transition_index   = ec_trans.begin()->second[0];
      }else
      {
        transition = NULL;
        transition_index = 0;
      }//if( there is only one EC decay ) / else
    }//if( delta_e < best_delta_e )
  }//if( positronfrac > 0.0 )
  
}//void findNearestPhotopeak(...)



const SandiaDecay::EnergyIntensityPair *PeakDef::findNearestXray(
                                const SandiaDecay::Element *el, double energy )
{
  if( !el )
    return NULL;
  
  const size_t nxray = el->xrays.size();
  size_t nearest = 0;
  double nearestDE = 999999.9;
  
  for( size_t i = 0; i < nxray; ++i )
  {
    const double thisDE = fabs( el->xrays[i].energy - energy );
    if( thisDE < nearestDE )
    {
      nearest = i;
      nearestDE = thisDE;
    }//if( thisDE < nearestE )
  }//for( size_t i = 0; i < nxray; ++i )
  
  if( nearestDE > 999999.0 )
    return NULL;
  
  return &(el->xrays[nearest]);
}//double findNearestXray( const SandiaDecay::Element *el, double energy )


/*
PeakDef::PeakDef( const PeakDef &rhs )
{
  *this = rhs;
}

const PeakDef &PeakDef::operator=( const PeakDef &rhs )
{
  if( (&rhs) == this )
    return *this;

  m_type       = rhs.m_type;
  m_offsetType = rhs.m_offsetType;

  m_parentNuclide            = rhs.m_parentNuclide;
  m_transition               = rhs.m_transition;
  m_radparticleIndex         = rhs.m_radparticleIndex;
  m_useForCalibration        = rhs.m_useForCalibration;
  m_useForShieldingSourceFit = rhs.m_useForShieldingSourceFit;
  m_useForDetectorResponseFit = rhs.m_useForDetectorResponseFit;

  const size_t nbytes = sizeof(m_coefficients[0])*NumCoefficientTypes;
  memcpy( &(m_coefficients[0]), &(rhs.m_coefficients[0]), nbytes );
  memcpy( &(m_uncertainties[0]), &(rhs.m_uncertainties[0]), nbytes );
}//const PeakDef &operator=( const PeakDef &rhs )
*/

bool PeakDef::operator==( const PeakDef &rhs ) const
{
  for( CoefficientType t = CoefficientType(0);
       t < NumCoefficientTypes; t = CoefficientType(t+1) )
  {
    if( m_coefficients[t] != rhs.m_coefficients[t] )
      return false;
  }

  return m_type==rhs.m_type
         && (*m_continuum == *rhs.m_continuum)
      && m_parentNuclide==rhs.m_parentNuclide
      && m_transition==rhs.m_transition
      && m_sourceGammaType==rhs.m_sourceGammaType
      && m_radparticleIndex==rhs.m_radparticleIndex
      && m_useForCalibration==rhs.m_useForCalibration
      && m_useForShieldingSourceFit==rhs.m_useForShieldingSourceFit
      && m_useForDetectorResponseFit==rhs.m_useForDetectorResponseFit
      && m_lineColor==rhs.m_lineColor
      ;
}//PeakDef::operator==


void PeakDef::clearSources()
{
  m_radparticleIndex = -1;
  m_parentNuclide = nullptr;
  m_transition = nullptr;
  m_xrayElement = nullptr;
  m_reaction = nullptr;
  m_sourceGammaType = PeakDef::NormalGamma;
  m_xrayEnergy = m_reactionEnergy = 0.0f;
}//void PeakDef::clearSources()


bool PeakDef::hasSourceGammaAssigned() const
{
  return ((m_parentNuclide && m_transition && m_radparticleIndex>=0) || m_xrayElement || m_reaction);
}

void PeakDef::setNuclearTransition( const SandiaDecay::Nuclide *parentNuclide,
                                    const SandiaDecay::Transition *transition,
                                    const int index,
                                    const SourceGammaType sourceType )
{
  const size_t ind = static_cast<size_t>( index );
  m_transition = transition;
  m_sourceGammaType = sourceType;
  
  if( m_transition && (ind<m_transition->products.size()) && (index>=0) )
    m_radparticleIndex = index;
  else
    m_radparticleIndex = -1;
  m_parentNuclide = parentNuclide;

  if( m_transition || sourceType==PeakDef::AnnihilationGamma )
  {
    m_xrayElement = NULL;
    m_xrayEnergy = 0.0;
    m_reaction = NULL;
    m_reactionEnergy = 0.0;
  }//if( m_transition )
}//void setNuclearTransition( SandiaDecay::Transition *transition, int radParticle )


void PeakDef::setXray( const SandiaDecay::Element *el, const float energy )
{
  m_xrayElement = el;
  m_xrayEnergy = energy;
  
  if( el )
  {
    m_radparticleIndex = -1;
    m_parentNuclide = NULL;
    m_transition = NULL;
    m_reaction = NULL;
    m_reactionEnergy = 0.0f;
    m_sourceGammaType = PeakDef::NormalGamma;
  }//if( el )
}//void setXray( const SandiaDecay::Element *el, const float energy )


void PeakDef::setReaction( const ReactionGamma::Reaction *rctn,
                           const float energy,
                           const SourceGammaType sourceType )
{
  m_reaction = rctn;
  m_reactionEnergy = energy;
  
  if( rctn )
  {
    m_radparticleIndex = -1;
    m_parentNuclide = NULL;
    m_transition = NULL;
    m_xrayElement = NULL;
    m_xrayEnergy = 0.0f;
    if( sourceType == PeakDef::AnnihilationGamma )
      throw runtime_error( "PeakDef::setReaction can not be called with source"
                           " type PeakDef::AnnihilationGamma" );
    m_sourceGammaType = sourceType;
  }//if( rctn )
}//void setReaction( const ReactionGamma::Reaction *rctn, double energy )



const SandiaDecay::Transition *PeakDef::nuclearTransition() const
{
  return m_transition;
}//const Transition *nuclearTransition() const


const SandiaDecay::Nuclide *PeakDef::parentNuclide() const
{
  return m_parentNuclide;
}//const SandiaDecay::Nuclide *parentNuclide() const;

int PeakDef::decayParticleIndex() const
{
  return m_radparticleIndex;
}

const SandiaDecay::RadParticle *PeakDef::decayParticle() const
{
  if( !m_transition || (m_radparticleIndex < 0)
      || (m_radparticleIndex>=static_cast<int>(m_transition->products.size())) )
    return NULL;

  return &(m_transition->products[m_radparticleIndex]);
}//const RadParticle *decayParticle() const


PeakDef::SourceGammaType PeakDef::sourceGammaType() const
{
  return m_sourceGammaType;
}

const SandiaDecay::Element *PeakDef::xrayElement() const
{
  return m_xrayElement;
}

float PeakDef::xrayEnergy() const
{
  return m_xrayEnergy;
}

const ReactionGamma::Reaction *PeakDef::reaction() const
{
  return m_reaction;
}

float PeakDef::reactionEnergy() const
{
  return m_reactionEnergy;
}


float PeakDef::gammaParticleEnergy() const
{
  if( m_parentNuclide )
  {
    switch( m_sourceGammaType )
    {
      case NormalGamma:
      case XrayGamma:
        if( m_transition && (m_radparticleIndex >= 0) )
          return m_transition->products.at(m_radparticleIndex).energy;
      break;
      case AnnihilationGamma:
        return static_cast<float>( 510.99891*SandiaDecay::keV );
      case SingleEscapeGamma:
        if( m_transition && (m_radparticleIndex >= 0) )
          return m_transition->products.at(m_radparticleIndex).energy - 510.9989f;
      break;
      case DoubleEscapeGamma:
        if( m_transition && (m_radparticleIndex >= 0) )
          return m_transition->products.at(m_radparticleIndex).energy - 2.0f*510.9989f;
      break;
    }
  }//if( m_parentNuclide )
  
  if( m_xrayElement )
    return m_xrayEnergy;
  
  if( m_reaction )
    return m_reactionEnergy;
  
  throw runtime_error( "Peak doesnt have a gamma associated with it" );
  
  return 0.0f;
}//float gammaParticleEnergy() const


const std::vector< PeakDef::CandidateNuclide > &PeakDef::candidateNuclides() const
{
  return m_candidateNuclides;
}//const std::vector< CandidateNuclide > &candidateNuclides() const


void PeakDef::addCandidateNuclide( const PeakDef::CandidateNuclide &candidate )
{
  m_candidateNuclides.push_back( candidate );
}//void addCandidateNuclide( const CandidateNuclide &candidate )


void PeakDef::addCandidateNuclide( const SandiaDecay::Nuclide * const nuclide,
                                   const SandiaDecay::Transition * const transition,
                                   const int radparticleIndex,
                                   const SourceGammaType sourceGammaType,
                                   const float weight )
{
  CandidateNuclide candidate;
  candidate.nuclide = nuclide;
  candidate.transition = transition;
  candidate.radparticleIndex = radparticleIndex;
  candidate.weight = weight;
  candidate.sourceGammaType = sourceGammaType;
  m_candidateNuclides.push_back( candidate );
}//void addCandidateNuclide(...)

void PeakDef::setCandidateNuclides( const std::vector<PeakDef::CandidateNuclide> &cand )
{
  m_candidateNuclides = cand;
}//void setCandidateNuclides( std::vector<const CandidateNuclide> &candidates )



void PeakDef::inheritUserSelectedOptions( const PeakDef &parent,
                                          const bool inheritNonFitForValues )
{
  if( parent.m_parentNuclide )
  {
    setNuclearTransition( parent.m_parentNuclide, parent.m_transition,
                          parent.m_radparticleIndex, parent.m_sourceGammaType );
  }else if( parent.m_xrayElement )
  {
    setXray( parent.m_xrayElement, parent.m_xrayEnergy );
  }else if( parent.m_reaction )
  {
    setReaction( parent.m_reaction, parent.m_reactionEnergy,
                 parent.m_sourceGammaType );
  }else
  {
    m_reaction = NULL;
    m_transition = NULL;
    m_xrayElement = NULL;
    m_parentNuclide = NULL;
    m_radparticleIndex = -1;
    m_sourceGammaType = NormalGamma;
    m_xrayEnergy = m_reactionEnergy = 0.0;
  }//if( parent nuclide ) / else / else
  
  m_lineColor = parent.lineColor();
  
  m_useForCalibration = parent.m_useForCalibration;
  m_useForShieldingSourceFit = parent.m_useForShieldingSourceFit;
  m_useForDetectorResponseFit = parent.m_useForDetectorResponseFit;
  
  for( CoefficientType t = CoefficientType(0);
      t < NumCoefficientTypes; t = CoefficientType(t+1) )
  {
    m_fitFor[t] = parent.m_fitFor[t];
    
    if( inheritNonFitForValues && !m_fitFor[t] )
    {
      switch( t )
      {
        case PeakDef::Mean:             case PeakDef::Sigma:
        case PeakDef::GaussAmplitude:   case PeakDef::LandauAmplitude:
        case PeakDef::LandauMode:       case PeakDef::LandauSigma:
          m_coefficients[t]  = parent.m_coefficients[t];
          m_uncertainties[t] = parent.m_uncertainties[t];
        break;
        
        case PeakDef::Chi2DOF: case PeakDef::NumCoefficientTypes:
          break;
      }//switch( t )
    }//if( inheritNonFitForValues )
  }//for( loop over PeakDef::CoefficientType )
}//void inheritUserSelectedOptions(...)


bool PeakContinuum::defined() const
{
  switch( m_type )
  {
    case NoOffset:
      return false;
      
    case Cubic:
      if( m_values[3] != 0.0)
        return true;
    case Quardratic:
      if( m_values[2] != 0.0)
        return true;
    case Linear:
      if( m_values[1] != 0.0)
        return true;
    case Constant:
      return (m_values[0] != 0.0);
    break;
    
    case External:
      return !!m_externalContinuum;
    break;
  }//switch( m_type )
  
  return false;
}//bool defined() const




double PeakDef::lowerX() const
{
  if( m_continuum->PeakContinuum::energyRangeDefined() )
    return m_continuum->lowerEnergy();

  if( m_skewType == PeakDef::LandauSkew )
    return min(m_coefficients[PeakDef::Mean] - m_coefficients[PeakDef::LandauSigma]+(0.22278*m_coefficients[PeakDef::LandauSigma])-25.0*m_coefficients[PeakDef::LandauSigma],
        m_coefficients[PeakDef::Mean] - 4.0*m_coefficients[PeakDef::Sigma]);

  return m_coefficients[PeakDef::Mean] - 4.0*m_coefficients[PeakDef::Sigma];
}//double lowerX() const


double PeakDef::upperX() const
{
  if( m_continuum->PeakContinuum::energyRangeDefined() )
    return m_continuum->upperEnergy();
  
  return m_coefficients[PeakDef::Mean] + 4.0*m_coefficients[PeakDef::Sigma];
}//double upperX() const


double PeakDef::gauss_integral( const double x0, const double x1 ) const
{
  double integral = gaus_integral( m_coefficients[PeakDef::Mean],
                                   m_coefficients[PeakDef::Sigma],
                                   m_coefficients[PeakDef::GaussAmplitude],
                                   x0, x1 );
  
  switch( m_skewType )
  {
    case PeakDef::NoSkew:
    break;
    
    case PeakDef::LandauSkew:
      integral += skew_integral( x0, x1 );
    break;
  };//enum SkewType

  return integral;
}//double gauss_integral( const double x0, const double x1 ) const;


double PeakDef::offset_integral( const double x0, const double x1 ) const
{
  return m_continuum->offset_integral( x0, x1 );
}//double offset_integral( const double x0, const double x1 ) const


double PeakDef::landau_potential_lowerX( const double peak_mean,
                                          const double peak_sigma )
{
  return peak_mean - 8.0*peak_sigma;
}//double landau_potential_lowerX() const


double PeakDef::landau_potential_upperX( const double peak_mean,
                                          const double /*peak_sigma*/)
{
  return peak_mean;
}//double landau_potential_upperX() const


double PeakDef::landau_integral( const double x0, const double x1,
                                  const double peak_mean,
                                  const double amplitude,
                                  const double mode,
                                  const double sigma )
{
  if( amplitude <= 0.0 )
    return 0.0;

  const double y0 = landau_cdf( peak_mean - x0, mode, sigma );
  const double y1 = landau_cdf( peak_mean - x1, mode, sigma );
  
  return amplitude*(y0-y1);
}//static double landau_integral( ... )

double PeakDef::skew_integral( const double xbinlow, const double xbinup,
                               const double peak_amplitude,
                               const double peak_mean,
                               const double t, const double s, const double b )
{
  //XXX - should make landaumode and landausigma relative to peak sigma
  return peak_amplitude*landau_integral( xbinlow, xbinup, peak_mean, t, s, b );

  /*
  //Below is an implementation of the skew definition TSpectrumFit uses.
  //  I cant seem to get it to work real well when using TMinuit2 to fit for
  //  the skew - additionally I dont know if the s*erfc( p )/2 term is well
  //  motoivated.
  const double amp = peak_amplitude /sqrt( 2.0*boost::math::constants::pi<double>()*sigma*sigma );

  const double x = (xbinup+xbinlow)/2.0;
  const double p = (x-peak_mean)/sigma;
  const double c = p + 1.0 / (2.0 * b);
  double e = p / b;
  if( e > 700.0 )
    e = 700.0;

  double skew = 0.0;
  if( b != 0.0 )
    skew += 0.5*t*exp( e ) * boost::math::erfc( c );
  skew += 0.5*s*boost::math::erfc( p );

  return amp * skew;
  */
}//double PeakDef::skew_integral(...)


double PeakDef::skew_integral( const double x0, const double x1 ) const
{
  double area = 0.0;
  
  switch( m_skewType )
  {
    case PeakDef::NoSkew:
      break;
      
    case PeakDef::LandauSkew:
      area = skew_integral( x0, x1, m_coefficients[PeakDef::GaussAmplitude],
                           m_coefficients[PeakDef::Mean],
                           m_coefficients[PeakDef::LandauAmplitude],
                           m_coefficients[PeakDef::LandauMode],
                           m_coefficients[PeakDef::LandauSigma] );
      break;
  };//switch( m_skewType )
  
  return area;
}//double skew_integral( const double x0, const double x1 ) const



double PeakDef::gaus_integral( const double peak_mean, const double peak_sigma,
                               const double peak_amplitude,
                               const double x0, const double x1 )
{
  if( peak_sigma==0.0 || peak_amplitude==0.0 )
    return 0.0;

  //Since the data is only float accuracy, there is no reason to calculate
  //  things past 9 digits (the erf function does hit the profiler pretty
  //  decently while fitting for peaks)
  using boost::math::policies::digits10;
  typedef boost::math::policies::policy<digits10<9> > my_pol_9;
  
  const double sqrt2 = boost::math::constants::root_two<double>();
  
  const double range = x1 - x0;
  const double erflowarg = (x0-peak_mean)/(sqrt2*peak_sigma);
  const double erfhigharg = (x0+range-peak_mean)/(sqrt2*peak_sigma);
  const double cdflow = 0.5*( 1.0 + boost::math::erf( erflowarg, my_pol_9() ) );
  const double cdhigh = 0.5*( 1.0 + boost::math::erf( erfhigharg, my_pol_9() ) );

  return peak_amplitude * (cdhigh - cdflow);
}//double gaus_integral(...)


////////////////////////////////////////////////////////////////////////////////

PeakContinuum::PeakContinuum()
: m_type( PeakContinuum::NoOffset ),
  m_lowerEnergy( 0.0 ),
  m_upperEnergy( 0.0 ),
  m_refernceEnergy( 0.0 )
{
}//PeakContinuum constructor

bool PeakContinuum::operator==( const PeakContinuum &rhs ) const
{
  return m_type==rhs.m_type
         && m_lowerEnergy == rhs.m_lowerEnergy
         && m_lowerEnergy == rhs.m_lowerEnergy
         && m_upperEnergy == rhs.m_upperEnergy
         && m_refernceEnergy == rhs.m_refernceEnergy
         && m_values == rhs.m_values
         && m_uncertainties == rhs.m_uncertainties
         && m_fitForValue == rhs.m_fitForValue
         && m_externalContinuum == rhs.m_externalContinuum;
}

void PeakContinuum::setParameters( double referenceEnergy,
                                   const std::vector<double> &x,
                                   const std::vector<double> &uncertainties )
{
  switch( m_type )
  {
    case NoOffset: case External:
      throw runtime_error( "PeakContinuum::setParameters invalid m_type" );
      
    case Constant:   case Linear:
    case Quardratic: case Cubic:
    break;
  };//switch( m_type )
  
  if( x.size() != static_cast<size_t>(m_type) )
    throw runtime_error( "PeakContinuum::setParameters invalid parameter size" );
  
  m_values = x;
  m_refernceEnergy = referenceEnergy;
  m_fitForValue.resize( m_values.size(), true );
  
  if( uncertainties.empty() )
  {
    m_uncertainties.clear();
    m_uncertainties.resize( x.size(), 0.0 );
  }else
  {
    if( uncertainties.size() != static_cast<size_t>(m_type) )
      throw runtime_error( "PeakContinuum::setParameters invalid uncert size" );
    m_uncertainties = uncertainties;
  }//if( uncertainties.empty() ) / else
}//void setParameters(...)



bool PeakContinuum::setPolynomialCoefFitFor( size_t polyCoefNum, bool fit )
{
  if( polyCoefNum >= m_fitForValue.size() )
    return false;
  
  m_fitForValue[polyCoefNum] = fit;
  return true;
}//bool setPolynomialCoefFitFor( size_t polyCoefNum, bool fit )


bool PeakContinuum::setPolynomialCoef( size_t polyCoef, double val )
{
  if( polyCoef >= m_values.size() )
    return false;
  
  m_values[polyCoef] = val;
  return true;
}


bool PeakContinuum::setPolynomialUncert( size_t polyCoef, double val )
{
  if( polyCoef >= m_uncertainties.size() )
    return false;
  
  m_uncertainties[polyCoef] = val;
  return true;
}

void PeakContinuum::setParameters( double referenceEnergy,
                                   const double *parameters,
                                   const double *uncertainties )
{
  switch( m_type )
  {
    case NoOffset: case External:
      throw runtime_error( "PeakContinuum::setParameters invalid m_type" );
      
    case Constant:   case Linear:
    case Quardratic: case Cubic:
      break;
  };//switch( m_type )
  
  if( !parameters )
    throw runtime_error( "PeakContinuum::setParameters invalid paramters" );
  
  vector<double> uncerts, values( parameters, parameters+m_type );
  if( uncertainties )
    uncerts.insert( uncerts.end(), uncertainties, uncertainties+m_type );
  
  setParameters( referenceEnergy, values, uncerts );
}//setParameters


void PeakContinuum::setExternalContinuum( const std::shared_ptr<const Measurement> &data )
{
  if( m_type != External )
    throw runtime_error( "PeakContinuum::setExternalContinuum invalid m_type" );
 
  m_externalContinuum = data;
}//setExternalContinuum(...)


void PeakContinuum::setRange( const double lower, const double upper )
{
  m_lowerEnergy = lower;
  m_upperEnergy = upper;
  if( m_lowerEnergy > m_upperEnergy )
    std::swap( m_lowerEnergy, m_upperEnergy );
}//void setRange( const double lowerenergy, const double upperenergy )


bool PeakContinuum::energyRangeDefined() const
{
  return (m_lowerEnergy != m_upperEnergy);
}//bool energyRangeDefined() const


bool PeakContinuum::isPolynomial() const
{
  switch( m_type )
  {
    case NoOffset:   case External:
      return false;
    case Constant:   case Linear:
    case Quardratic: case Cubic:
      return true;
    break;
  }//switch( m_type )
  
  return false;
}//bool isPolynomial() const


void PeakContinuum::setType( PeakContinuum::OffsetType type )
{
  m_type = type;
  
  switch( m_type )
  {
    case NoOffset:
      m_values.clear();
      m_uncertainties.clear();
      m_fitForValue.clear();
      m_externalContinuum.reset();
      m_lowerEnergy = m_upperEnergy = m_refernceEnergy = 0.0;
    break;
      
    case Constant:
      m_values.resize( 1, 0.0 );
      m_uncertainties.resize( 1, 0.0 );
      m_fitForValue.resize( 1, true );
      m_externalContinuum.reset();
    break;
    
    case Linear:
      m_values.resize( 2, 0.0 );
      m_uncertainties.resize( 2, 0.0 );
      m_fitForValue.resize( 2, true );
      m_externalContinuum.reset();
    break;
      
    case Quardratic:
      m_values.resize( 3, 0.0 );
      m_uncertainties.resize( 3, 0.0 );
      m_fitForValue.resize( 3, true );
      m_externalContinuum.reset();
    break;
    
    case Cubic:
      m_values.resize( 4, 0.0 );
      m_uncertainties.resize( 4, 0.0 );
      m_fitForValue.resize( 4, true );
      m_externalContinuum.reset();
    break;
      
    case External:
      m_values.clear();
      m_uncertainties.clear();
      m_fitForValue.clear();
      m_refernceEnergy = 0.0;
    break;
  };//switch( m_type )
  
}//void setType( PeakContinuum::OffsetType type )

void PeakContinuum::calc_linear_continuum_eqn( const std::shared_ptr<const Measurement> &data,
                                        const double x0, const double x1,
                                        const int nbin )
{
  m_refernceEnergy = x0;
  m_lowerEnergy = x0;
  m_upperEnergy = x1;
  m_type = PeakContinuum::Linear;
  m_values.resize( 2 );
  m_fitForValue.resize( 2, true );
  
  const size_t xlowchannel = data->find_gamma_channel( (float)x0 );
  const size_t xhighchannel = data->find_gamma_channel( (float)x1 );
  
  eqn_from_offsets( xlowchannel, xhighchannel, m_refernceEnergy, data, nbin, m_values[1], m_values[0] );
}//void calc_continuum_eqn(...)


double PeakContinuum::offset_integral( const double x0, const double x1 ) const
{
  switch( m_type )
  {
    case NoOffset:
      return 0.0;
    case Constant:   case Linear:
    case Quardratic: case Cubic:
      return offset_eqn_integral( &(m_values[0]),
                                 m_type, x0, x1, m_refernceEnergy );
    case External:
      if( !m_externalContinuum )
        return 0.0;
      return gamma_integral( m_externalContinuum, x0, x1 );
    break;
  };//enum OffsetType
  
  return 0.0;
}//double offset_integral( const double x0, const double x1 ) const


void PeakContinuum::eqn_from_offsets( size_t lowchannel,
                                      size_t highchannel,
                               const double peakMean,
                               const std::shared_ptr<const Measurement> &data,
                               const size_t nbin,
                               double &m, double &b )
{
  double x1  = data->gamma_channel_lower( lowchannel ) - peakMean;
  const double dx1 = data->gamma_channel_width( lowchannel );
  
  //XXX - hack! below 'c' will be inf if the following check would fail; I
  //      really should work out how to fix this properly with math, but for
  //      right now I'll fudge it, which will toss the answer off a bit, but
  //      whatever.
  if( fabs(2.0*x1*dx1 + dx1*dx1) < DBL_EPSILON )
  {
    static int ntimes = 0;
    if( ntimes++ < 4 )
      cerr << "Should fix math in PeakContinuum::eqn_from_offsets" << endl;
    x1 = data->gamma_channel_lower( lowchannel+1 ) - peakMean;
  }
  
  const double x2  = data->gamma_channel_lower( highchannel ) - peakMean;
  
  const double dx2 = data->gamma_channel_width( highchannel );
  
  if( lowchannel < nbin )
    lowchannel = nbin;
  highchannel = std::min( highchannel, data->num_gamma_channels()-1 );
  
  const double nbinInv = 1.0 / (1.0+2.0*nbin);
  const double y1 = nbinInv*data->gamma_channels_sum( lowchannel-nbin, lowchannel+nbin );
  const double y2 = nbinInv*data->gamma_channels_sum( highchannel-nbin, highchannel+nbin);

  const double c = (2.0*x2*dx2 + dx2*dx2)/(2.0*x1*dx1 + dx1*dx1);
  b = (y2-y1*c)/(dx2-dx1*c);
  m = 2.0*(y1-b*dx1)/(2.0*x1*dx1+dx1*dx1);
  
  if( IsNan(m) || IsInf(m) || IsNan(b) || IsInf(b) )
  {
    cerr << "PeakContinuum::eqn_from_offsets(...): Invalid results" << endl;
    m = b = 0.0;
  }
}//eqn_from_offsets(...)


double PeakContinuum::offset_eqn_integral( const double *coefs,
                                           PeakContinuum::OffsetType type,
                                           double x0, double x1,
                                           const double peak_mean )
{
  switch( type )
  {
    case NoOffset: case External:
      throw runtime_error( "PeakContinuum::offset_eqn_integral(...) may only be"
                           " called for polynomial backgrounds" );
      
    case Constant:   case Linear:
    case Quardratic: case Cubic:
    break;
  };//enum OffsetType
    
  x0 -= peak_mean;
  x1 -= peak_mean;
  
  double answer = 0.0;
  const int maxorder = static_cast<int>( type );
  for( int order = 0; order < maxorder; ++order )
  {
    const double exp = order + 1.0;
    answer += (coefs[order]/exp) * (std::pow(x1,exp) - std::pow(x0,exp));
  }//for( int order = 0; order < maxorder; ++order )
  
  return std::max( answer, 0.0 );
}//offset_eqn_integral(...)


void PeakContinuum::translate_offset_polynomial( double *new_coefs,
                                                 const double *old_coefs,
                                                 PeakContinuum::OffsetType type,
                                                 const double new_center,
                                                 const double old_center )
{
  switch( type )
  {
    case NoOffset:
    case External:
      throw runtime_error( "translate_offset_polynomial invalid offset type" );
      
    case Quardratic:
    case Cubic:
      throw runtime_error( "translate_offset_polynomial does not yet support "
                           "quadratic or cubic polynomials" );
      
    case Linear:
    {
      new_coefs[0] = old_coefs[0] + old_coefs[1] * (new_center - old_center);
      new_coefs[1] = old_coefs[1];
      break;
    }//case Linear:
      
    case Constant:
      new_coefs[0] = old_coefs[0];
      break;
  }//switch( type )
}//void translate_offset_polynomial(...)

