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

#include <string>
#include <cctype>

#include <boost/regex.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>

#include "InterSpec/PhysicalUnits.h"
#include "SpecUtils/UtilityFunctions.h"

using namespace std;

namespace PhysicalUnits
{
#define POS_DECIMAL_REGEX "\\s*\\+?\\s*((\\d+(\\.\\d*)?)|(\\.\\d*))\\s*(?:[Ee][+\\-]?\\d+)?\\s*"
#define DIST_UNITS_REGEX "(meter|cm|km|mm|um|nm|m|ft|feet|'|inches|inch|in|\")"
#define PLUS_MINUS_REGEX "(\\xC2?\\xB1|\\+\\-\\s*|\\-\\+\\s*)"
#define TIME_UNIT_REGEX "(year|yr|y|day|d|hrs|hour|h|minutes|min|m|second|s|ms|microseconds|us|nanoseconds|ns)"
#define HALF_LIFE_REGEX "(hl|halflife|halflives|half-life|half-lives|half lives|half life)"
#define ACTIVITY_UNIT_REGEX "(bq|becquerel|ci|cu|curie|c)"
#define DURATION_REGEX "(\\+?\\d+:\\d\\d:\\d+(\\.\\d+)?)"
#define ISO_8601_DURATION_REGEX "[\\-+]?[Pp](?!$)(\\d+(?:\\.\\d+)?[Yy])?(\\d+(?:\\.\\d+)?[Mm])?(\\d+(?:\\.\\d+)?[Ww])?(\\d+(?:\\.\\d+)?[Dd])?([Tt](?=\\d)(\\d+(?:\\.\\d+)?[Hh])?(\\d+(?:\\.\\d+)?[Mm])?(\\d+(?:\\.\\d+)?[Ss])?)?"
  
const char * const sm_distanceRegex
    = "^(((\\s*[+\\-]?\\s*0\\s*)|((" POS_DECIMAL_REGEX ")\\s*" DIST_UNITS_REGEX ")+\\s*))+$";
const char * const sm_distanceUnitOptionalRegex
    = "^(((" POS_DECIMAL_REGEX ")\\s*(" DIST_UNITS_REGEX ")?)+\\s*)+$";
  
const char * const sm_distanceUncertaintyRegex
   = "^\\s*((" POS_DECIMAL_REGEX "(\\(\\s*" PLUS_MINUS_REGEX POS_DECIMAL_REGEX "\\s*\\s*\\)\\s*)?" DIST_UNITS_REGEX ")"
     "|(" POS_DECIMAL_REGEX "(\\s*" PLUS_MINUS_REGEX POS_DECIMAL_REGEX "\\s*)?" DIST_UNITS_REGEX ")"
     "|(" POS_DECIMAL_REGEX "\\s*" DIST_UNITS_REGEX "(\\s*" PLUS_MINUS_REGEX POS_DECIMAL_REGEX "\\s*" DIST_UNITS_REGEX ")?)"
     "|(" POS_DECIMAL_REGEX "\\s*" DIST_UNITS_REGEX "(\\(\\s*" PLUS_MINUS_REGEX POS_DECIMAL_REGEX "\\s*" DIST_UNITS_REGEX "\\s*\\))?)"
     "|(" POS_DECIMAL_REGEX DIST_UNITS_REGEX "\\s*(\\(" PLUS_MINUS_REGEX POS_DECIMAL_REGEX "\\s*" DIST_UNITS_REGEX "+\\s*\\)\\s*)?)"
     "|(" POS_DECIMAL_REGEX DIST_UNITS_REGEX "\\s*(" PLUS_MINUS_REGEX POS_DECIMAL_REGEX "\\s*" DIST_UNITS_REGEX "+\\s*)?)"
      ")+\\s*$";
  
  
const char * const sm_distanceUncertaintyUnitsOptionalRegex
  = "^\\s*((" POS_DECIMAL_REGEX "(\\(\\s*" PLUS_MINUS_REGEX POS_DECIMAL_REGEX "\\s*\\s*\\)\\s*)?" DIST_UNITS_REGEX "?)"
  "|(" POS_DECIMAL_REGEX "(\\s*" PLUS_MINUS_REGEX POS_DECIMAL_REGEX "\\s*)?" DIST_UNITS_REGEX ")"
  "|(" POS_DECIMAL_REGEX "\\s*" DIST_UNITS_REGEX "?(\\s*" PLUS_MINUS_REGEX POS_DECIMAL_REGEX "\\s*" DIST_UNITS_REGEX "?)?)"
  "|(" POS_DECIMAL_REGEX "\\s*" DIST_UNITS_REGEX "?(\\(\\s*" PLUS_MINUS_REGEX POS_DECIMAL_REGEX "\\s*" DIST_UNITS_REGEX "?\\s*\\))?)"
  "|(" POS_DECIMAL_REGEX DIST_UNITS_REGEX "?\\s*(" PLUS_MINUS_REGEX POS_DECIMAL_REGEX "\\s*" DIST_UNITS_REGEX "?\\s*)?)"
  ")+\\s*$";
  
  //Non escaped version of regex is: "^(\s*\+?\d+(\.\d*)?(?:[Ee][+-]?\d+)?\s*(?:m|M|k|g|G|t|T|u|p|n|milli|micro|pico|nano|kilo|mega|terra)?[\s-]*([Bb][Qq]|[Bb][Ee][Cc][Qq][Uu][Ee][Rr][Ee][Ll]|[Cc][Ii]|[Cc][Uu]|[Cc][Uu][Rr][Ii][Ee]))?\s*$"
const char * const sm_activityRegex = "^(\\s*" POS_DECIMAL_REGEX "\\s*(?:(?:m|k|g|t|u|p|n|milli|micro|pico|nano|kilo|mega|terra)?[\\s-]*" ACTIVITY_UNIT_REGEX "))?\\s*$";
                                       

const char * const sm_activityUnitOptionalRegex
            = "^(\\s*" POS_DECIMAL_REGEX "\\s*"
              "(?:(?:m|M|k|g|G|t|T|u|p|n|milli|micro|pico|nano|kilo|mega|terra)?"
              "[\\s-]*"
              "([Bb][Qq]|[Bb][Ee][Cc][Qq][Uu][Ee][Rr][Ee][Ll]|[Cc][Ii]|[Cc][Uu]|[Cc][Uu][Rr][Ii][Ee]))?)?"
              "\\s*$";
  
const char * const sm_timeDurationRegex
  = "\\s*((" POS_DECIMAL_REGEX "\\s*" TIME_UNIT_REGEX "\\s*)|(" DURATION_REGEX "\\s*)\\s*)|(" ISO_8601_DURATION_REGEX "\\s*)+";
  
const char * const sm_timeDurationHalfLiveOptionalRegex
   = "\\s*((" POS_DECIMAL_REGEX "\\s*" TIME_UNIT_REGEX "\\s*)"
     "|(" DURATION_REGEX "\\s*)"
     "|(" POS_DECIMAL_REGEX "\\s*" HALF_LIFE_REGEX "\\s*))+";

  
const UnitNameValuePairV sm_activityUnitNameValues{
  {"bq", bq},
  {"kBq", kBq},
  {"MBq", MBq},
  {"GBq", GBq},
  {"TBq", TBq},
  {"nCi", nCi},
  {"microCi", microCi},
  {"mCi", mCi},
  {"ci", ci}
};

const UnitNameValuePairV sm_activityUnitHtmlNameValues{
  {"bq", bq},
  {"kBq", kBq},
  {"MBq", MBq},
  {"GBq", GBq},
  {"TBq", TBq},
  {"nCi", nCi},
  {"&mu;Ci", microCi},
  {"mCi", mCi},
  {"ci", ci}
};
  
const UnitNameValuePairV sm_timeUnitNameValues{
  {"ps", picosecond},
  {"ns", nanosecond},
  {"microS", microsecond},
  {"ms", millisecond},
  {"seconds", second},
  {"hour", hour},
  {"day", day},
  {"month", month},
  {"year", year}
};

const UnitNameValuePairV sm_timeUnitHtmlNameValues{
  {"ps", picosecond},
  {"ns", nanosecond},
  {"micro-s", microsecond},
  {"ms", millisecond},
  {"seconds", second},
  {"hours", hour},
  {"days", day},
  {"months", month},
  {"years", year}
};

const UnitNameValuePairV sm_doseRateUnitHtmlNameValues{
  {"&mu;R/hr", (rem*1.0E-6)/hour},
  {"mR/hr", (rem*1.0E-3)/hour},
  {"R/hr", rem/hour},
  {"&mu;Sv/hr", (sievert*1.0E-6)/hour},
  {"mSv/hr", (sievert*1.0E-3)/hour},
  {"Sv/hr", sievert/hour}
};




std::string printToBestLengthUnits( double length, int maxNpostDecimal,
                                    const double cm_definition )
{
  using namespace std;
  length *= cm / cm_definition;

  char formatflag[32], buffer[64];
  snprintf(formatflag, sizeof(formatflag), "%%.%if %%s", maxNpostDecimal );
  
  if( length < (1000.0 * nm) )
    snprintf(buffer, sizeof(buffer), formatflag, (length/nm), "nm" );
  else if( length < (1000.0 * um) )
    snprintf(buffer, sizeof(buffer), formatflag, (length/um), "um" );
  else if( length < (10.0 * mm) )
    snprintf(buffer, sizeof(buffer), formatflag, (length/mm), "mm" );
  else if( length < (100.0 * cm) )
    snprintf(buffer, sizeof(buffer), formatflag, (length/cm), "cm" );
  else if( length < (1000.0 * m) )
    snprintf(buffer, sizeof(buffer), formatflag, (length/m), "m" );
  else
    snprintf(buffer, sizeof(buffer), formatflag, (length/(1000.0*m)), "km" );

  return buffer;
}//std::string printToBestLengthUnits( ... )

std::wstring printToBestLengthUnits( double length, double uncert,
                                     int maxNpostDecimal,
                                     const double cm_definition )
{
  using namespace std;
  length *= cm / cm_definition;
  uncert *= cm / cm_definition;
  
  double unit = 1000.0*m;
  wstring unitLabel = L" km";
  
  if( length < (1000.0 * nm) )
  {
    unit = nm;
    unitLabel = L" nm";
  }else if( length < (1000.0 * um) )
  {
    unit = um;
    unitLabel = L" um";
  }else if( length < (10.0 * mm) )
  {
    unit = mm;
    unitLabel = L" mm";
  }else if( length < (100.0 * cm) )
  {
    unit = cm;
    unitLabel = L" cm";
  }else if( length < (1000.0 * m) )
  {
    unit = m;
    unitLabel = L" m";
  }
  
  wstringstream txt;
  txt << setprecision(maxNpostDecimal) << fixed << (length/unit) << L" (\x00B1";
  if( ((uncert > 100.0*length) || (uncert < length/100.0)) && uncert!=0.0 )
    txt << setprecision(maxNpostDecimal) << scientific << (uncert/unit) << ")";
  else
    txt << setprecision(maxNpostDecimal) << fixed << (uncert/unit) << ")";
  txt << unitLabel;

  return txt.str();
}//std::string printToBestLengthUnits( ... )
  
  
std::string printToBestActivityUnits( double activity,
                                      int maxNpostDecimal,
                                      bool useCurries,
                                      double bq_definition )
{
  using namespace std;
  activity *= becquerel / bq_definition;

  char formatflag[32], buffer[64];
  const char *unitstr = useCurries ? "Ci" : "Bq";
  
  snprintf(formatflag, sizeof(formatflag), "%%.%if %%s%s", maxNpostDecimal, unitstr );
  

  if( useCurries )
    activity /= curie;
  else
    activity /= becquerel;

  if( activity < 1.0E-6 )
    snprintf(buffer, sizeof(buffer), formatflag, (activity*1.0E9), "n" );
  else if( activity < 1.0E-3 )
    snprintf(buffer, sizeof(buffer), formatflag, (activity*1.0E6), "u" );
  else if( activity < 1.0 )
    snprintf(buffer, sizeof(buffer), formatflag, (activity*1.0E3), "m" );
  else if( activity < 1.0E3 )
    snprintf(buffer, sizeof(buffer), formatflag, activity, "" );
  else if( activity < 1.0E6 )
    snprintf(buffer, sizeof(buffer), formatflag, (activity*1.0E-3), "k" );
  else
    snprintf(buffer, sizeof(buffer), formatflag, (activity*1.0E-6), "M" );
  
  return buffer;
}//std::string printToBestActivityUnits(...)


std::string printToBestTimeUnits( double time,
                                  int maxDecimal,
                                  double second_definition )
{
  using namespace std;
  time *= second / second_definition;

  char formatflag[32], buffer[64];
  snprintf(formatflag, sizeof(formatflag), "%%.%if %%s", maxDecimal );
  
  if( fabs(time) < second*1.0E-3 )
    snprintf(buffer, sizeof(buffer), formatflag, (time*1.0E6/second), "us" );
  else if( fabs(time) < second*0.1 )
    snprintf(buffer, sizeof(buffer), formatflag, (time*1.0E3/second), "ms");
  else if( fabs(time) < minute )
    snprintf(buffer, sizeof(buffer), formatflag, (time/second), "s");
  else if( fabs(time) > 1.0E3*year )
  {
    snprintf(formatflag, sizeof(formatflag), "%%.%ig y", maxDecimal );
    snprintf(buffer, sizeof(buffer), formatflag, (time/year) );
  }else if( fabs(time) > 0.99*year )
    snprintf(buffer, sizeof(buffer), formatflag, (time/year), "y");
  else if( fabs(time) > 3.0*day )
    snprintf(buffer, sizeof(buffer), formatflag, (time/day), "d");
  else if( fabs(time) > 3.0*hour )
    snprintf(buffer, sizeof(buffer), formatflag, (time/hour), "h");
  else if( fabs(time) > 3.0*minute )
    snprintf(buffer, sizeof(buffer), formatflag, (time/minute), "m");
  else if( fabs(time) > 1.0*second )
    snprintf(buffer, sizeof(buffer), formatflag, (time/second), "s");
  else if( fabs(time) > 1.0*millisecond )
    snprintf(buffer, sizeof(buffer), formatflag, (time/millisecond), "ms");
  else if( fabs(time) > 1.0*microsecond )
  {
//#if ( defined(WIN32) || defined(UNDER_CE) || defined(_WIN32) || defined(WIN64) )
    const unsigned char utf8mus[] = { 0xCE, 0xBC, 0x73, 0 };
    snprintf(buffer, sizeof(buffer), formatflag, (time/microsecond), (char *)utf8mus );
//#else
//    snprintf(buffer, sizeof(buffer), formatflag, (time/microsecond), "&mu;s" );
//#endif
  }else if( fabs(time) > 1.0*picosecond )
    snprintf(buffer, sizeof(buffer), formatflag, (time/picosecond), "ps");
  else
    snprintf(buffer, sizeof(buffer), formatflag, (time/second), "s");
  /*
  else
  {
    //Less than 4 days we'll return time in the format 'hh:mm:ss.ss'
    const int nhour = static_cast<int>( floor( time / hour ) );
    time -= nhour*hour;
    const int nminute = static_cast<int>( floor( time / minute ) );
    time -= nminute*minute;
    const int nsec = static_cast<int>( floor( time / second ) );
    time -= nsec*second;

    //Need to round the fractional seconds to nearest number of ticks
    typedef boost::posix_time::time_duration::tick_type tick_type;
    const double mult = std::pow(10.0,static_cast<double>(maxDecimal));
    tick_type ticks = boost::posix_time::time_duration::ticks_per_second();
    const int count = static_cast<int>( floor(time*mult*ticks+0.5) / mult );
  
    boost::posix_time::time_duration duration( nhour, nminute, nsec, count );
    string durstr = boost::posix_time::to_simple_string( duration );
  
    //Get rid of unwanted decimals (they should be all zeros from rounding above)
    const size_t pos = durstr.find_last_of( '.' );
    if( pos != string::npos )
      durstr = durstr.substr( 0, min(size_t(pos+maxDecimal+1), durstr.size()) );
    
    snprintf(buffer, sizeof(buffer), "%s", durstr.c_str() );
  }
   */
  
  return buffer;
}//printToBestTimeUnits(...)

  
std::string printToBestMassUnits( const double mass,
                                   const int maxDecimal,
                                   const double gram_definition )
{
  const double mass_grams = mass / gram_definition;
  
  if( IsInf(mass_grams) || IsNan(mass_grams) )
    return "Nan";
  
  char formatflag[32], massstr[64];
  snprintf(formatflag, sizeof(formatflag), "%%.%if %%s", maxDecimal );
  
  if( mass_grams < 1.0E-9 )
    snprintf( massstr, sizeof(massstr), formatflag, (1.0E12 * mass_grams), "pg" );
  else if( mass_grams < 1.0E-6 )
    snprintf( massstr, sizeof(massstr), formatflag, (1.0E9 * mass_grams), "ng" );
  else if( mass_grams < 1.0E-3 )
    snprintf( massstr, sizeof(massstr), formatflag, (1.0E6 * mass_grams), "ug" );
  else if( mass_grams < 1.0E0 )
    snprintf( massstr, sizeof(massstr), formatflag, (1.0E3 * mass_grams), "mg" );
  else if( mass_grams < 1.0E3 )
    snprintf( massstr, sizeof(massstr), formatflag, (1.0E0 * mass_grams), "g" );
  else// if( mass_grams < 1.0E6 )
    snprintf( massstr, sizeof(massstr), formatflag, (1.0E-3 * mass_grams), "kg" );
  //      else
  //      snprintf( massstr, sizeof(massstr), "%.4g g", mass_grams );

  return massstr;
}//std::string printToBestMassUnits(...)
  
  
  
string printToBestDoseUnits( double dose, const int maxNpostDecimal,
                             const bool useRemPerHr, const double remPerHrDef )
{
  using namespace std;
  dose *= (rem/hour) / remPerHrDef;
  
  char formatflag[32], buffer[64];
  const char *unitstr = useRemPerHr ? "rem/hr" : "sv/hr";
  
  snprintf(formatflag, sizeof(formatflag), "%%.%if %%s%s", maxNpostDecimal, unitstr );
  
  
  if( useRemPerHr )
    dose /= (rem/hour);
  else
    dose /= (sievert/hour);
  
  if( dose < 1.0E-6 )
    snprintf(buffer, sizeof(buffer), formatflag, (dose*1.0E9), "n" );
  else if( dose < 1.0E-3 )
    snprintf(buffer, sizeof(buffer), formatflag, (dose*1.0E6), "u" );
  else if( dose < 1.0 )
    snprintf(buffer, sizeof(buffer), formatflag, (dose*1.0E3), "m" );
  else if( dose < 1.0E3 )
    snprintf(buffer, sizeof(buffer), formatflag, dose, "" );
  else if( dose < 1.0E6 )
    snprintf(buffer, sizeof(buffer), formatflag, (dose*1.0E-3), "k" );
  else
    snprintf(buffer, sizeof(buffer), formatflag, (dose*1.0E-6), "M" );
  
  return buffer;
}//string printToBestDoseUnits(...)


double stringToTimeDuration( std::string str, double second_def )
{
  return stringToTimeDurationPossibleHalfLife( str, -1.0, second_def );
}//double stringToTimeDuration( std::string str, double second_def )
  

double stringToTimeDurationPossibleHalfLife( std::string str,
                                             const double hl,
                                             double second_def )
{
  double time_dur = 0.0;

  if( second_def <= 0.0 )
    throw runtime_error( "Second definition must be larger than zero" );
  
  UtilityFunctions::trim( str );
  UtilityFunctions::to_lower( str );
  
  while( str.find("+ ") != string::npos )
    UtilityFunctions::ireplace_all(str, "+ ", "+" );
  while( str.find("- ") != string::npos )
    UtilityFunctions::ireplace_all(str, "- ", "-" );

  const size_t colonpos = str.find(":");
  if( colonpos != std::string::npos )
  {
    size_t startpos, endpos;
    for( startpos = colonpos; startpos > 0; --startpos )
      if( isspace(str[startpos-1]) )
        break;
    for( endpos = colonpos; endpos < str.size(); ++endpos )
      if( isspace(str[endpos]) )
        break;
    
    string durstr = str.substr( startpos, endpos-startpos );
    str = str.substr(0,startpos) + str.substr(endpos);
    UtilityFunctions::trim( str );
    
    boost::posix_time::time_duration dur;
    dur = boost::posix_time::duration_from_string( durstr );
    if( dur.is_special() )
      throw runtime_error( "stringToTimeDuration(...): couldnt translate '"
                            + durstr + "' to a format like '23:59:59.000'" );
    time_dur = (1.0E-6 * dur.total_microseconds() * second_def);
    if( str.length() )
      time_dur += stringToTimeDurationPossibleHalfLife( str, hl, second_def );
    
    return time_dur;
  }//if( str.find(":") != std::string::npos )

  
  //if( (hl<=0.0) && (str.size()>2)
  //    && (str[0]=='p' || ((str[0]=='+' || str[0]=='-') && str[0]=='p') ) )
  if( str.find('p') != string::npos )
  {
    //extended format of ISO 8601, i.e., PnYnMnDTnHnMnS,
    //  ex 'P1Y2M2W3DT10H30M10.1S', '-P1347M', 'PT1M'
    
    boost::smatch matches;
    boost::regex expression( ISO_8601_DURATION_REGEX );
    if( boost::regex_search( str, matches, expression ) )
    {
      //for( size_t i = 0; i < matches.size(); ++i )
      //  cout << "Match[" << i << "] = '" << matches[i] << "'" << endl;
      //For: 'P1Y2M2W3DT10H30M10.1S'
      //matches[0] = 'P1Y2M2W3DT10H30M10.1S'
      //matches[1] = '1y'
      //matches[2] = '2m'
      //matches[3] = '2w'
      //matches[4] = '3d'
      //matches[5] = 'T10H30M10.1S'
      //matches[6] = '10h'
      //matches[7] = '30m'
      //matches[8] = '10.1s'
      double iso_dur = 0.0;
      for( size_t i = 1; i < 9; ++i )
      {
        string valstr = matches[i].str();
        if( i==5 || valstr.size()<=1 )
          continue;
        
        valstr = valstr.substr(0,valstr.size()-1);
        double val = std::stod( valstr );  //could throw, if our regex is messed up
        
        switch( i )
        {
          case 1: val *= PhysicalUnits::year;    break;
          case 2: val *= PhysicalUnits::month;   break;
          case 3: val *= 7.0*PhysicalUnits::day; break;
          case 4: val *= PhysicalUnits::day;     break;
          case 6: val *= PhysicalUnits::hour;    break;
          case 7: val *= PhysicalUnits::minute;  break;
          case 8: val *= PhysicalUnits::second;  break;
        }
        
        iso_dur += val;
      }//for( size_t i = 1; i < 10; ++i )
      
      const string fullmatch = matches[0].str();
      assert( fullmatch.size() > 0 );
      
      if( fullmatch[0] == '-' )
        iso_dur *= -1.0;
      
      if( fullmatch.size() > 1 ) //protect against regex just matching "p"
      {
        time_dur += iso_dur;
      
        if( str.size() == fullmatch.size() )
        return time_dur;
        
        const size_t startpos = str.find( fullmatch );
        if( startpos != string::npos )  //should always be true, but jic
        {
          str.erase( begin(str)+startpos, begin(str)+startpos+fullmatch.size() );
          UtilityFunctions::trim( str );
          if( !str.empty() )
            time_dur += stringToTimeDurationPossibleHalfLife( str, hl, second_def );
        }
        
        return time_dur;
      }//if( fullmatch.size() > 1 )
    }//if( boost::regex_match( str, matches, expression ) )
    //If not a required format, keep trying.
  }//if( str.size()>2 && (str[0]=='p' || ((str[0]=='+' || str[0]=='-') && str[0]=='p') )  )
  
  

  const string time_regex ="\\s*((\\+|\\-)?((\\d+(\\.\\d*)?)|(\\.\\d*))(?:[Ee][+\\-]?\\d+)?)"
                            "\\s*("
                            "ps|picosecond|pico\\-*sec|pico\\-*second"
                            "|ns|nano\\-*second|nano\\-*sec"
                            "|us|micro\\-*sec|micro\\-*second"
                            "|ms|milli\\-*sec|milli\\-*second"
                            "|second|s|minute|minutes|min|m"
                            "|hours|hour|hrs|h|days|day|d|year|yr|y"
                            "|hl|halflife|halflives|half\\-life|half\\-lives|half\\slives|half\\slife"
                            ")"
                            "\\s*((\\+|\\-)?(((\\d+(\\.\\d*)?)|(\\.\\d*)).*))?";
  
  boost::smatch matches;
  boost::regex expression( time_regex );
  if( boost::regex_match( str, matches, expression ) )
  {
    //for( size_t i = 0; i < matches.size(); ++i )
      //cerr << "stringToTimeDuration Match " << i << " is " << string( matches[i].first, matches[i].second ) << endl;
    
    string number = string( matches[1].first, matches[1].second );
    string letters = string( matches[7].first, matches[7].second );
    UtilityFunctions::trim( number );
    UtilityFunctions::trim( letters );
    UtilityFunctions::to_lower( letters );
    
    double value;
    if( !(stringstream(number) >> value) )
      throw std::runtime_error( "Unable to convert '" + number + "' to a number" );
    
    double unit = 0.0;
    if( UtilityFunctions::starts_with(letters, "ps" )
       || UtilityFunctions::starts_with(letters, "pico" ) )
      unit = second * 1.0E-12;
    else if( UtilityFunctions::starts_with(letters, "ns" )
        || UtilityFunctions::starts_with(letters, "nano" ) )
      unit = second * 1.0E-9;
    else if( UtilityFunctions::starts_with(letters, "us" )
        || UtilityFunctions::starts_with(letters, "micro" )
        || UtilityFunctions::starts_with(letters, "\xc2\xb5" ) )
      unit = second * 1.0E-6;
    else if( UtilityFunctions::starts_with(letters, "ms" )
        || UtilityFunctions::starts_with(letters, "mS" )
        || UtilityFunctions::starts_with(letters, "milli" ) )
      unit = second * 1.0E-3;
    else if( UtilityFunctions::starts_with(letters, "s" ) )
      unit = second;
    else if( letters=="m" || UtilityFunctions::contains(letters, "min" )  )
      unit = minute;
    else if( letters=="h"
             || UtilityFunctions::contains(letters, "hour" )
             || UtilityFunctions::contains(letters, "hrs" ) )
      unit = hour;
    else if( letters=="d" || UtilityFunctions::contains(letters, "day" ) )
      unit = day;
    else if( letters=="y"
             || UtilityFunctions::contains(letters, "yr" )
             || UtilityFunctions::contains(letters, "year" ) )
      unit = year;
    else if( UtilityFunctions::starts_with(letters, "h" ) && (hl > 0.0) )  //the regex hopefully makes sure
      unit = hl;
    else
    {
      std::string msg = "cant identify label '";
      msg += letters + "' as a unit of time";
      //std::cerr << msg << endl;
      throw runtime_error( msg );
    }

    time_dur += value*unit*(second_def/second);

    if( matches[8].length() > 0 )
      time_dur += stringToTimeDurationPossibleHalfLife( matches[8], hl, second_def );
  }else
  {
    //If we are here, it may be the case the user didnt enter any units
    //  which is okay only if they mean for zero time, so well check
    //  for that case, and if not throw an exception.
    const string zero_regex ="(?|\\+|\\-)?0+(?|\\.0*)*";
    boost::smatch zeromatches;
    boost::regex xeroexpression( zero_regex );
    if( boost::regex_match( str, zeromatches, xeroexpression ) )
      time_dur = 0.0;
    else
      throw runtime_error( "stringToTimeDuration(...): could not translate '"
                                + str + "' to a time");
  }//if( match regex ) / else

  return time_dur;
}//double stringToTimeDuration( std::string str, double second_def )

//stringToActivity(...): Takes a user string, and attempts to interpret
//  if as an activity.  Ex. '52.3 bq', '88 MCi', etc
//  throws std::runtime_error on failure
double stringToActivity( std::string str, double bq_def )
{
  UtilityFunctions::trim( str );
  boost::smatch mtch;
  boost::regex expr( "(" POS_DECIMAL_REGEX ")" "\\s*([a-zA-Z \\-]+)" );

  if( boost::regex_match( str, mtch, expr ) )
  {
#if( PERFORM_DEVELOPER_CHECKS )
    if( mtch.size() != 7 )
    {
      char buffer[512];
      snprintf( buffer, sizeof(buffer),
               "stringToActivity(..) regex yeilds %i matches instead of 6",
                static_cast<int>(mtch.size()) );
      log_developer_error( BOOST_CURRENT_FUNCTION, buffer );
    }
#endif
    
    assert( mtch.size() == 7 );
    
    //cout << "mtch.size()=" << mtch.size() << endl;
    //for( size_t i = 0; i < mtch.size(); ++i )
    //  cout << "\t" << string( mtch[i].first, mtch[i].second ) << endl;
    //cout << endl;
    
    string number = string( mtch[1].first, mtch[1].second );
    string letters = string( mtch[6].first, mtch[6].second );
    
    const double value = std::stod( number );
    double unit = 0.0;

    if( UtilityFunctions::istarts_with(letters, "n" ) )
      unit = 1.0E-9;
    else if( UtilityFunctions::istarts_with(letters, "u" )
             || UtilityFunctions::istarts_with(letters, "micro" )
             || UtilityFunctions::starts_with(letters, "\xc2\xb5" ) )
      unit = 1.0E-6;
    else if( UtilityFunctions::starts_with(letters, "m" )
             || UtilityFunctions::istarts_with(letters, "milli" ) )
      unit = 1.0E-3;
    else if( UtilityFunctions::istarts_with(letters, "b" )
             || UtilityFunctions::istarts_with(letters, "c" ) )
      unit = 1.0;
    else if( UtilityFunctions::istarts_with(letters, "k" ) )
      unit = 1.0E+3;
    else if( UtilityFunctions::starts_with(letters, "M" )
             || UtilityFunctions::istarts_with(letters, "mega" ) )
      unit = 1.0E+6;
    else if( UtilityFunctions::starts_with(letters, "G" ) )
      unit = 1.0E+9;
    else if( UtilityFunctions::starts_with(letters, "T" ) )
      unit = 1.0E+12;

    //There is no need for a prefix
    //else
    //{
      //std::string msg = "didnt contain valid prefix specification in '";
      //msg += letters + "'";
      //std::cerr << msg << endl;
      //throw runtime_error( msg );
    //}

    const bool hasb = UtilityFunctions::icontains(letters, "b" );
    const bool hasc = UtilityFunctions::icontains(letters, "c" );

    if( hasb && !hasc )
     unit *= becquerel;
    else if( hasc && !hasb )
     unit *= curie;
    else
    {
      std::string msg = "didnt contain unit specification (bq or ci) in '";
      msg += letters + "'";
      std::cerr << msg << endl;
      throw runtime_error( msg );
    }

    return value * unit * bq_def / becquerel;
  }else
  {
    throw std::runtime_error( std::string("'") + str
                              + "' is not appropriate activity string" );
  } //if( boost::regex_match( substr, mtch, expr ) ) / else

  return 0.0;
}//double stringToActivity( std::string str, double bq_def )



double stringToDistance( std::string str, double cm_definition )
{
  double distance = 0.0;

  const string float_regex ="\\s*\\+?(\\d+(\\.\\d*)?(?:[Ee][+\\-]?\\d+)?)"
                            "\\s*(meter|cm|km|mm|um|nm|m|ft|feet|'|inches|inch|in|\")"
                            "\\s*(\\d.+)?";
  
  string::size_type openParan = str.find( '(' );
  while( openParan != string::npos )
  {
    const string::size_type closeParan = str.find( ')', openParan );
    if( closeParan == string::npos )
      break;
    str = str.substr( 0, openParan ) + str.substr( closeParan + 1 );
    openParan = str.find( '(', openParan );
  }//while( openParan != string::npos )
  
  boost::smatch matches;
  boost::regex expression( float_regex, boost::regex::ECMAScript|boost::regex::icase );

  if( !boost::regex_match( str, matches, expression ) )
  {
    char msg[128];
    snprintf(msg, sizeof(msg), "'%s' is an invalid distance", str.c_str() );
    cerr << endl << msg << endl;
    throw std::runtime_error( msg );
  }//if( we dont have a match )

  
//  for( size_t i = 0; i < matches.size(); ++i )
//    cerr << "stringToDistance Match " << i << " is " << string( matches[i].first, matches[i].second ) << endl;

  
  string floatstr( matches[1].first, matches[1].second );
  string unitstr( matches[3].first, matches[3].second );
  UtilityFunctions::trim( floatstr );
  UtilityFunctions::trim( unitstr );
  UtilityFunctions::to_lower( unitstr );

  double unitval = 0.0;
  if( unitstr == "meter" )       unitval = m;
  else if( unitstr == "cm" )     unitval = cm;
  else if( unitstr == "km" )     unitval = 1000.0*m;
  else if( unitstr == "mm" )     unitval = mm;
  else if( unitstr == "um" )     unitval = 0.001*mm;
  else if( unitstr == "nm" )     unitval = 0.000001*mm;
  else if( unitstr == "m" )      unitval = m;
  else if( unitstr == "ft" )     unitval = 12.0*2.54*cm;
  else if( unitstr == "'" )      unitval = 12.0*2.54*cm;
  else if( unitstr == "feet" )   unitval = 12.0*2.54*cm;
  else if( unitstr == "in" )     unitval = 2.54*cm;
  else if( unitstr == "\"" )     unitval = 2.54*cm;
  else if( unitstr == "i" )      unitval = 2.54*cm;
  else if( unitstr == "in" )     unitval = 2.54*cm;
  else if( unitstr == "inch" )   unitval = 2.54*cm;
  else if( unitstr == "inchs" )  unitval = 2.54*cm;
  else if( unitstr == "inches" ) unitval = 2.54*cm;
  else if( unitstr == "attoparsec" )  unitval = 3.086*cm;
  else if( unitstr == "attoparsecs" ) unitval = 3.086*cm;
  
//  else assert(0);

  unitval *= (cm_definition / cm);

  distance = std::stod( floatstr );  //shouldnt ever throw, right?
  distance *= unitval;

  //if there are characters past what we needed for a match, maybe they are
  //  another distance string, lets try to add them on
  if( string(matches[4]).length() > 1 )
  {
    try
    {
      distance += stringToDistance( matches[4], cm_definition );
    }catch(...)
    {}
  }//if( string(matches[4]).length() )

//  cerr << str << " = " << distance/cm_definition << " cm, or "
//       << distance/cm_definition/2.54 << " inches" << endl;

  return distance;
}//double stringToDistance( std::string str )


const UnitNameValuePair &bestActivityUnit( const double activity,
                                           bool useCurries )
{
  UnitNameValuePairV::const_iterator begin, end, iter;

  if( useCurries )
  {
    begin = sm_activityUnitNameValues.begin() + 5;
    end = sm_activityUnitNameValues.end();
  }else
  {
    begin = sm_activityUnitNameValues.begin();
    end = begin + 5;
  }//if( useCurries ) / else


  for( iter = begin; iter != end; ++iter )
  {
    if( activity <= 100.0*iter->second )
      return *iter;
  }//for( iter = begin; begin != end; ++begin )

  return *(end-1);
}//const UnitNameValuePair &bestActivityUnit( const double activity )

  
const UnitNameValuePair &bestDoseUnitHtml( const double activity,
                                            bool useRem )
{
  UnitNameValuePairV::const_iterator begin, end, iter;
  
  if( useRem )
  {
    begin = sm_activityUnitNameValues.begin();
    end = begin + 3;
  }else
  {
    begin = sm_activityUnitNameValues.begin() +3;
    end = sm_activityUnitNameValues.end();
  }//if( useRem ) / else
  
  
  for( iter = begin; iter != end; ++iter )
  {
    if( activity <= 100.0*iter->second )
      return *iter;
  }//for( iter = begin; begin != end; ++begin )
  
  return *(end-1);
}//const UnitNameValuePair &bestDoseUnitHtml(...)
  
  
const UnitNameValuePair &bestActivityUnitHtml( const double activity,
                                                bool useCurries )
{
  const UnitNameValuePair &a = bestActivityUnit( activity, useCurries );
  
  UnitNameValuePairV::const_iterator begin, end, iter;
  begin = sm_activityUnitNameValues.begin();
  end = sm_activityUnitNameValues.end();
  
  for( iter = begin; iter != end; ++iter )
  {
    if( a.first == iter->first )
      return sm_activityUnitHtmlNameValues[iter-begin];
  }
  
  return sm_activityUnitHtmlNameValues[0];  //shouldnt ever happen
}//const UnitNameValuePair &bestActivityUnitHtml(...)

  
UnitNameValuePair bestTimeUnit( const double t )
{
  if( t <= 100.0*nanosecond )
    return UnitNameValuePair("nano-seconds", nanosecond );
  if( t <= 100.0*picosecond )
    return UnitNameValuePair("pico-seconds", picosecond );
  if( t <= 100.0*microsecond )
    return UnitNameValuePair("micro-seconds", microsecond );
  if( t <= 100.0*millisecond )
    return UnitNameValuePair("milli-seconds", millisecond );
  if( t <= 5.0*hour )
    return UnitNameValuePair("seconds", second );
  if( t <= 6.0*month )
    return UnitNameValuePair("days", day );
  if( t <= 5.0*year )
    return UnitNameValuePair("months", month );

  return UnitNameValuePair("Years", year );
}//UnitNameValuePair bestTimeUnit( const double time )


const UnitNameValuePair &bestTimeUnitLongHtml( const double t )
{
  if( t <= 100.0*nanosecond )
    return sm_timeUnitHtmlNameValues[1];
  if( t <= 100.0*picosecond )
    return sm_timeUnitHtmlNameValues[0];
  if( t <= 100.0*microsecond )
    return sm_timeUnitHtmlNameValues[2];
  if( t <= 100.0*millisecond )
    return sm_timeUnitHtmlNameValues[3];
  if( t <= 5.0*hour )
    return sm_timeUnitHtmlNameValues[4];
  if( t <= 6.0*month )
    return sm_timeUnitHtmlNameValues[6];
  if( t <= 5.0*year )
    return sm_timeUnitHtmlNameValues[7];

  return sm_timeUnitHtmlNameValues[8];
}//UnitNameValuePair bestTimeUnitLongHtml( const double time )



UnitNameValuePair bestTimeUnitShortHtml( const double t )
{
  if( t <= 100.0*nanosecond )
    return UnitNameValuePair("ns", nanosecond );
  if( t <= 100.0*picosecond )
    return UnitNameValuePair("ps", picosecond );
  if( t <= 100.0*microsecond )
    return UnitNameValuePair("&mu;s", microsecond );
  if( t <= 100.0*millisecond )
    return UnitNameValuePair("ms", millisecond );
  if( t <= 5.0*hour )
    return UnitNameValuePair("s", second );
  if( t <= 6.0*month )
    return UnitNameValuePair("d", day );
  if( t <= 5.0*year )
    return UnitNameValuePair("m", month );

  return UnitNameValuePair("y", year );
}//UnitNameValuePair bestTimeUnitShortHtml( const double time )

}//namespace PhysicalUnits
