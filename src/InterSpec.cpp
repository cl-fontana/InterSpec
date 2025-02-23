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

// Get rid of some issues lurking in the boost libraries.
#pragma warning(disable:4244)
#pragma warning(disable:4800)
// Block out some UUID warnings
#pragma warning(disable:4996)

#include <ctime>
#include <tuple>
#include <mutex>
#include <locale>
#include <vector>
#include <string>
#include <limits>
#include <sstream>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <sys/stat.h>

#include <boost/ref.hpp>
#include <boost/date_time/posix_time/posix_time.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_generators.hpp>
#include <boost/uuid/uuid_io.hpp>

#include <Wt/WText>
#include <Wt/WTree>
#include <Wt/Utils>
#include <Wt/WTable>
#include <Wt/WLabel>
#include <Wt/WImage>
#include <Wt/WBreak>
#include <Wt/WPoint>
#include <Wt/WServer>
#include <Wt/Dbo/Dbo>
#include <Wt/WAnchor>
#include <Wt/WSpinBox>
#include <Wt/WIconPair>
#include <Wt/WGroupBox>
#include <Wt/WTextArea>
#include <Wt/WCheckBox>
#include <Wt/WTreeView>
#include <Wt/WTreeNode>
#include <Wt/WTemplate>
#include <Wt/WIOService>
#include <Wt/WAnimation>
#include <Wt/WTabWidget>
#include <Wt/WPopupMenu>
#include <Wt/WFileUpload>
#include <Wt/WPushButton>
#include <Wt/WWidgetItem>
#include <Wt/WBorderLayout>

#include <Wt/Json/Array>
#include <Wt/Json/Parser>
#include <Wt/Json/Object>
#include <Wt/Json/Value>

#include <Wt/WGridLayout>
#include <Wt/WHBoxLayout>
#include <Wt/WJavaScript>
#include <Wt/WApplication>
#include <Wt/WEnvironment>
#include <Wt/WProgressBar>
#include <Wt/WSplitButton>
#include <Wt/WSelectionBox>
#include <Wt/WCssStyleSheet>
#include <Wt/Dbo/QueryModel>
#include <Wt/WDoubleSpinBox>
#include <Wt/WSuggestionPopup>
#include <Wt/WContainerWidget>
#include <Wt/WDefaultLoadingIndicator>

#if( USE_DB_TO_STORE_SPECTRA )
#include <Wt/Json/Array>
#include <Wt/Json/Value>
#include <Wt/Json/Object>
#include <Wt/Json/Parser>
#include <Wt/Json/Serializer>
#endif

#include "rapidxml/rapidxml.hpp"
#include "rapidxml/rapidxml_print.hpp"
#include "rapidxml/rapidxml_utils.hpp"

#include "InterSpec/MakeDrf.h"
#include "InterSpec/PeakFit.h"
#include "InterSpec/PopupDiv.h"
#include "InterSpec/PeakEdit.h"
#include "InterSpec/SpecMeas.h"
#include "InterSpec/InterSpec.h"
#include "InterSpec/IsotopeId.h"
#include "InterSpec/AuxWindow.h"
#include "InterSpec/PeakModel.h"
#include "InterSpec/ColorTheme.h"
#include "InterSpec/MaterialDB.h"
#include "InterSpec/GammaXsGui.h"
#include "InterSpec/HelpSystem.h"
#include "InterSpec/DecayWindow.h"
#include "InterSpec/ColorSelect.h"
#include "InterSpec/InterSpecApp.h"
#include "InterSpec/Recalibrator.h"
#include "InterSpec/DetectorEdit.h"
#include "InterSpec/DataBaseUtils.h"
#include "InterSpec/UseInfoWindow.h"
#include "InterSpec/OneOverR2Calc.h"
#include "InterSpec/WarningWidget.h"
#include "InterSpec/SpectrumChart.h"
#include "InterSpec/PhysicalUnits.h"
#include "InterSpec/InterSpecUser.h"
#include "InterSpec/DoseCalcWidget.h"
#include "SpecUtils/SpecUtilsAsync.h"
#include "InterSpec/PeakFitChi2Fcn.h"
#include "InterSpec/SpecMeasManager.h"
#include "InterSpec/PeakInfoDisplay.h"
#include "InterSpec/SpecFileSummary.h"
#include "InterSpec/ColorThemeWindow.h"
#include "InterSpec/GammaCountDialog.h"
#include "InterSpec/SpectraFileModel.h"
#include "SpecUtils/UtilityFunctions.h"
#include "InterSpec/ActivityConverter.h"
#include "InterSpec/LocalTimeDelegate.h"
#include "InterSpec/CanvasForDragging.h"
#include "InterSpec/PeakSearchGuiUtils.h"
#include "InterSpec/CompactFileManager.h"
#include "InterSpec/SpectrumDisplayDiv.h"
#include "InterSpec/MassAttenuationTool.h"
#include "SpecUtils/SpectrumDataStructs.h"
#include "InterSpec/DetectorPeakResponse.h"
#include "InterSpec/IsotopeSearchByEnergy.h"
#include "InterSpec/ShieldingSourceDisplay.h"
#include "InterSpec/ReferencePhotopeakDisplay.h"
#include "InterSpec/LicenseAndDisclaimersWindow.h"

#if( USE_SPECTRUM_CHART_D3 )
#include "InterSpec/D3SpectrumDisplayDiv.h"
#endif

#if( USE_DB_TO_STORE_SPECTRA )
#include "InterSpec/DbStateBrowser.h"
#endif

#if( !ANDROID && !IOS )
#include "InterSpec/FileDragUploadResource.h"
#endif

#if( IOS )
#include "target/ios/InterSpec/FileHandling.h"
#endif 

#if( ALLOW_URL_TO_FILESYSTEM_MAP )
#include "InterSpec/DbToFilesystemLink.h"
#endif

#if( INCLUDE_ANALYSIS_TEST_SUITE )
#include "InterSpec/SpectrumViewerTester.h"
#endif

#if( USE_GOOGLE_MAP )
#include "InterSpec/GoogleMap.h"
#endif

#if( USE_SEARCH_MODE_3D_CHART )
#include "InterSpec/SearchMode3DChart.h"
#endif

#if( USE_TERMINAL_WIDGET )
#include "InterSpec/TerminalWidget.h"
#endif

#if( USE_SIMPLE_NUCLIDE_ASSIST )
#include "InterSpec/SimpleNuclideAssist.h"
#endif

#if( USE_SPECRUM_FILE_QUERY_WIDGET )
#include "InterSpec/SpecFileQueryWidget.h"
#endif

#if( SpecUtils_ENABLE_D3_CHART )
#include "SpecUtils/D3SpectrumExport.h"
#endif

#include "js/InterSpec.js"

#define INLINE_JAVASCRIPT(...) #__VA_ARGS__

using namespace Wt;
using namespace std;

std::mutex InterSpec::sm_staticDataDirectoryMutex;
std::string InterSpec::sm_staticDataDirectory = "data";

#if( BUILD_AS_ELECTRON_APP || IOS || ANDROID || BUILD_AS_OSX_APP || (BUILD_AS_LOCAL_SERVER && (defined(WIN32) || defined(__APPLE__)) ) )
std::mutex InterSpec::sm_writableDataDirectoryMutex;
std::string InterSpec::sm_writableDataDirectory = "";
#endif  //if( not a webapp )



void log_error_message( const std::string &message, const std::string &source, const int priority )
{
  InterSpecApp *app = dynamic_cast<InterSpecApp*>( Wt::WApplication::instance() );
  if( app )
    app->svlog(message,source,priority);
  else
    std::cerr << source << ": " << message << std::endl;
}

namespace
{
  static const char * const PeakInfoTabTitle      = "Peak Manager";
  static const char * const GammaLinesTabTitle    = "Reference Photopeaks";
  static const char * const CalibrationTabTitle   = "Energy Calibration";
  static const char * const NuclideSearchTabTitle = "Nuclide Search";
  static const char * const FileTabTitle          = "Spectrum Files";
  
//#if( !BUILD_FOR_WEB_DEPLOYMENT )
//  const WTabWidget::LoadPolicy TabLoadPolicy = WTabWidget::LazyLoading;
//#else
  const WTabWidget::LoadPolicy TabLoadPolicy = WTabWidget::PreLoading;
//#endif

  void postSvlogHelper( const WString &msg, const int priority )
  {
    InterSpecApp *app = dynamic_cast<InterSpecApp *>(wApp);
    if( app )
      app->svlog( msg, WString(), priority );
  }
  
  //adapted from: http://stackoverflow.com/questions/1894886/parsing-a-comma-delimited-stdstring
  struct csv_reader: std::ctype<char>
  {
    csv_reader(): std::ctype<char>(get_table()) {}
    static std::ctype_base::mask const* get_table()
    {
      static std::vector<std::ctype_base::mask> rc(table_size, std::ctype_base::mask());
      rc[','] = std::ctype_base::space;
    	rc[' '] = std::ctype_base::space;
      rc['\n'] = std::ctype_base::space;
      return &rc[0];
    }
  };//struct csv_reader
  

  //DeleteOnClosePopupMenu is a PopupDivMenu that deletes itself on close.
  //  Necassary because (with Wt 3.3.4 at least) using the aboutToHide() signal
  //  to delete the menu causes a crash.
  class DeleteOnClosePopupMenu : public PopupDivMenu
  {
    bool m_deleteWhenHidden;
  public:
    DeleteOnClosePopupMenu( WPushButton *p, const PopupDivMenu::MenuType t )
    : PopupDivMenu( p, t ), m_deleteWhenHidden( false ) {}
    virtual ~DeleteOnClosePopupMenu(){}
    void markForDelete(){ m_deleteWhenHidden = true; }
    virtual void setHidden( bool hidden, const WAnimation &a = WAnimation() )
    {
      PopupDivMenu::setHidden( hidden, a );
      if( hidden && m_deleteWhenHidden )
        delete this;
    }
  };//class PeakRangePopupMenu

  
  //Returns -1 if you shouldnt add the peak to the hint peaks
  int add_hint_peak_pos( const std::shared_ptr<const PeakDef> &peak,
                        const std::deque< std::shared_ptr<const PeakDef> > &existing )
  {
    if( !peak )
      return -1;
    
    if( existing.empty() )
      return 0;
  
    const double sigma_frac = 0.7;
    const double mean = peak->mean();
    const double sigma = peak->sigma();
    
    deque< std::shared_ptr<const PeakDef> >::const_iterator pos;
    pos = lower_bound( existing.begin(), existing.end(),
                       peak, &PeakDef::lessThanByMeanShrdPtr );
    
    bool nearother = false;
    if( pos != existing.begin() )
      nearother |= ((fabs(((*(pos-1))->mean() - mean)/sigma) < sigma_frac));
    if( pos != existing.end() )
      nearother |= ( fabs(((*pos)->mean() - mean)/sigma) < sigma_frac );
    
    return (nearother ? -1 : static_cast<int>(pos - existing.begin()));
  }//int add_hint_peak_pos(...)
  
  
  bool try_update_hint_peak( const std::shared_ptr<const PeakDef> &newpeak,
                             std::shared_ptr<SpecMeas> &meas,
                             const set<int> &samples )
  {
    if( !meas || !newpeak )
      return false;
    
    std::shared_ptr< const SpecMeas::PeakDeque > hintPeaks
                                        = meas->automatedSearchPeaks( samples );
    
    if( !hintPeaks )
      return false;
    
    const int pos = add_hint_peak_pos( newpeak, *hintPeaks );
    if( pos >= 0 )
    {
      std::shared_ptr< SpecMeas::PeakDeque > newHintPeaks
        = std::make_shared<SpecMeas::PeakDeque>( hintPeaks->begin(), hintPeaks->end() );
      newHintPeaks->insert( newHintPeaks->begin() + pos, newpeak );
      meas->setAutomatedSearchPeaks( samples, newHintPeaks );
      return true;
    }//if( pos >= 0 )
    
    return false;
  }//try_update_hint_peak(...)
}//namespace


InterSpec::InterSpec( WContainerWidget *parent )
  : WContainerWidget( parent ),
    m_peakModel( 0 ),
    m_spectrum( 0 ),
    m_timeSeries( 0 ),
    m_detectorToShowMenu( 0 ),
    m_mobileMenuButton(0),
    m_mobileBackButton(0),
    m_mobileForwardButton(0),
    m_notificationDiv(0),
    m_warnings( 0 ),
    m_warningsWindow( 0 ),
    m_fileManager( 0 ),
    m_layout( 0 ),
    m_chartsLayout( 0 ),
    m_toolsLayout( 0 ),
    m_menuDiv( 0 ),
    m_peakInfoDisplay( 0 ),
    m_peakInfoWindow( 0 ),
    m_peakEditWindow( 0 ),
    m_currentToolsTab( 0 ),
    m_toolsTabs( 0 ),
    m_recalibrator( 0 ),
    m_calibrateContainer( 0 ),
    m_recalibratorWindow( 0 ),
    m_gammaCountDialog( 0 ),
    m_specFileQueryDialog( 0 ),
    m_shieldingSuggestion( 0 ),
    m_shieldingSourceFit( 0 ),
    m_shieldingSourceFitWindow( 0 ),
    m_materialDB( nullptr ),
    m_nuclideSearchWindow( 0 ),
    m_isotopeSearchContainer(0),
    m_isotopeSearch( 0 ),
    m_fileMenuPopup( 0 ),
    m_toolsMenuPopup( 0 ),
    m_helpMenuPopup( 0 ),
    m_displayOptionsPopupDiv( 0 ),
#if( USE_DB_TO_STORE_SPECTRA )
    m_saveState( 0 ),
    m_saveStateAs( 0 ),
    m_createTag( 0 ),
    m_currentStateID( -1 ),
#endif
    m_rightClickMenu( 0 ),
    m_rightClickEnergy( -DBL_MAX ),
    m_rightClickNuclideSuggestMenu( 0 ),
#if( USE_SIMPLE_NUCLIDE_ASSIST )
    m_leftClickMenu( 0 ),
#endif
#if( USE_SAVEAS_FROM_MENU )
    m_downloadMenu( 0 ),
  m_downloadMenus{0},
#endif
  m_logYItems{0},
  m_toolTabsVisibleItems{0},
  m_backgroundSubItems{0},
  m_verticalLinesItems{0},
  m_horizantalLinesItems{0},
  m_tabToolsMenuItems{0},
  m_featureMarkersShown{false},
#if( USE_GOOGLE_MAP )
    m_mapMenuItem( 0 ),
#endif
#if( USE_SEARCH_MODE_3D_CHART )
    m_searchMode3DChart( 0 ),
#endif
#if( USE_TERMINAL_WIDGET )
    m_terminalMenuItem( 0 ),
    m_terminal( 0 ),
    m_terminalWindow( 0 ),
#endif
    m_clientDeviceType( 0x0 ),
    m_referencePhotopeakLines( 0 ),
    m_referencePhotopeakLinesWindow( 0 ),
    m_licenseWindow( nullptr ),
    m_useInfoWindow( 0 ),
    m_preserveCalibWindow( 0 ),
    m_renderedWidth( 0 ),
    m_renderedHeight( 0 ),
    m_colorPeaksBasedOnReferenceLines( true ),
    m_findingHintPeaks( false )
{
  //Initialization of the app (this function) takes about 11ms on my 2.6 GHz
  //  Intel Core i7, as of (20150316).
  
  addStyleClass( "InterSpec" );
  
  //Setting setLayoutSizeAware doesnt seem to add any appreciable network
  //  overhead, at least according to the chrome inspectrion panel when running
  //  locally
  setLayoutSizeAware( true );

  
  //for notification div
  m_notificationDiv = new WContainerWidget();
  m_notificationDiv->setStyleClass("qtipDiv");
  m_notificationDiv->setId("qtip-growl-container");
  wApp->domRoot()->addWidget( m_notificationDiv );
  
  if( !isMobile() )
    initHotkeySignal();
  
  // Try to grab the username.
  string username = static_cast<InterSpecApp*>(wApp)->getUserNameFromEnvironment();

#if( !BUILD_FOR_WEB_DEPLOYMENT )
  if( username == "" )
    username = InterSpecApp::userNameFromOS();
#endif
  
  //If they don't have a username, grab their stored UUID.
  if( username == "" )
  {
    try
    {
      username = wApp->environment().getCookie( "SpectrumViewerUUID" );
    }catch(...)
    {
      stringstream usernamestrm;
      usernamestrm << boost::uuids::random_generator()();
      username = usernamestrm.str();
      wApp->setCookie( "SpectrumViewerUUID", username, 3600*24*365 );
    }//try / catch
  }//if( no username )
  
  detectClientDeviceType();

  
  if( isPhone() )
    username += "_phone";
  else if( isTablet() )
    username += "_tablet";

// Set up the session; open the database.
  m_sql.reset( new DataBaseUtils::DbSession() );
  
  
  // Try to find the user in the database, if not, make a new entry
  {//begin interacting with DB
    DataBaseUtils::DbTransaction transaction( *m_sql );
    m_user = m_sql->session()->find< InterSpecUser >().where( "userName = ?" )
                         .bind( username ).limit(1).resultValue();
    
    if( m_user )
    {
      InterSpecUser::initFromDbValues( m_user, m_sql );
    }else
    {
      InterSpecUser::DeviceType type = InterSpecUser::Desktop;
      if( isPhone() )
        type = InterSpecUser::PhoneDevice;
      else if( isTablet() )
        type = InterSpecUser::TabletDevice;
    
      InterSpecUser *newuser = new InterSpecUser( username, type );
      m_user = m_sql->session()->add( newuser );
    
      InterSpecUser::initFromDefaultValues( m_user, m_sql );
    }//if( m_user ) / else
  
    m_user.modify()->startingNewSession();
    
    transaction.commit();
  }//end interacting with DB
  

  m_peakModel  = new PeakModel( this );
#if( USE_SPECTRUM_CHART_D3 )
  m_spectrum   = new D3SpectrumDisplayDiv();
#else
  m_spectrum   = new SpectrumDisplayDiv();
  m_spectrum->setPlotAreaPadding( 80, 2, 10, 44 );
#endif
  m_timeSeries = new SpectrumDisplayDiv();
  m_calibrateContainer = new WContainerWidget();
  
  m_timeSeries->setPlotAreaPadding( 80, 0, 10, 44 );
  
  if( isPhone() )
  {
    //TODO: layoutSizeChanged(...) will trigger the compact axis anyway, but
    //      I should check if doing it here saves a roundtrip
    m_spectrum->setCompactAxis( true );
    m_timeSeries->setCompactAxis( true );
    
    //For iPhoneX we need to:
    //  X - adjust padding for safe area
    //  X - Add padding to mobile menu to cover the notch area.
    //  X - Change position of hamburger menu
    //  - set .Wt-domRoot background color to match the charts
    //  X - Detect orientation changes, and respond appropriately
    //  X - Get the Wt area to fill the entire width (and height)
    //  -Initial AuxWindow width/height correct
    
    LOAD_JAVASCRIPT(wApp, "js/InterSpec.js", "InterSpec", wtjsDoOrientationChange);
    
    const char *js = INLINE_JAVASCRIPT(
      window.addEventListener("orientationchange", Wt.WT.DoOrientationChange );
      setTimeout( Wt.WT.DoOrientationChange, 0 );
      setTimeout( Wt.WT.DoOrientationChange, 250 );  //JIC - doesnt look necassary
      setTimeout( Wt.WT.DoOrientationChange, 1000 );
    );
    
    doJavaScript( js );
  }//if( isPhone() )
  
  m_spectrum->setPeakModel( m_peakModel );
  
  m_isotopeSearch = new IsotopeSearchByEnergy( this, m_spectrum );
  m_isotopeSearch->setLoadLaterWhenInvisible(true);

  m_warnings = new WarningWidget( m_spectrum, this );

  // Set up the floating energy recalibrator.
  initRecalibrator();

#if( USE_SPECTRUM_CHART_D3 )
  const WEnvironment &env = wApp->environment();
  const bool isOldIE = (env.agentIsIE() && env.agent()<WEnvironment::IE9);
  
  if( isOldIE )
  {
    m_timeSeries->connectWtMouseConnections();
  }else
  {
    m_timeSeries->enableOverlayCanvas( false, true, false );
    m_timeSeries->setIsTimeDisplay();
  }//if( isOldIE ) / else
#else
  const WEnvironment &env = wApp->environment();
  const bool isOldIE = (env.agentIsIE() && env.agent()<WEnvironment::IE9);

  if( isOldIE )
  {
    m_spectrum->connectWtMouseConnections();
    m_timeSeries->connectWtMouseConnections();
  }else
  {
    m_spectrum->enableOverlayCanvas( true, false, true );
    m_timeSeries->enableOverlayCanvas( false, true, false );

    m_spectrum->setIsEnergyDisplay();
    m_timeSeries->setIsTimeDisplay();

    if( m_spectrum->overlayCanvasJsException() )
      m_spectrum->overlayCanvasJsException()->connect( boost::bind( &InterSpec::overlayCanvasJsExceptionCallback, this, _1 ) );
  }//if( isOldIE ) / else
#endif  //#if( !USE_SPECTRUM_CHART_D3 )
  
#if( BUILD_FOR_WEB_DEPLOYMENT && !USE_SPECTRUM_CHART_D3 )
  m_spectrum->setControlDragDebouncePeriod( 500 );
#endif 

#if( !USE_SPECTRUM_CHART_D3 )
  m_spectrum->controlMouseMoved().connect( boost::bind( &SpectrumDisplayDiv::setControlDragContinuumPreview,
                                                      m_spectrum, _1, _2 ) );
#endif

  m_fileManager = new SpecMeasManager( this );
  
  m_peakInfoDisplay = new PeakInfoDisplay( this, m_spectrum, m_peakModel );

  initMaterialDbAndSuggestions();
  
  WWidget *menuWidget = NULL;
  if( isMobile() )
  {
    m_mobileMenuButton = new WPushButton( "", wApp->domRoot() );

    m_mobileMenuButton->addStyleClass( "MobileMenuButton btn" );
    //Need to set z-index inline rather than in css so AuxWindows loaded before
    //  the CSS can be forced above the button
    m_mobileMenuButton->setZIndex( 8388635 );
   
    //hamburger
    PopupDivMenu *popup = new PopupDivMenu( m_mobileMenuButton, PopupDivMenu::AppLevelMenu );
    popup->addPhoneBackItem( nullptr );
    
    m_mobileMenuButton->removeStyleClass( "Wt-btn" );
    menuWidget = popup;
    
#if( !SpecUtils_ENABLE_D3_CHART )
    m_spectrum->setAvoidMobileMenu( true );
#endif
    
    //Add this transparent overlay when mobile left menu slides in, so that
    // we can capture the click to hide the menu.  If this is not there, the
    // canvas will take over and not propagate the event to close the menu down.
    doJavaScript( "$('body').append('<div class=\"mobilePopupMenuOverlay\" style=\"display: none;\"></div>');" );
    
    m_mobileBackButton = new WPushButton( "", wApp->domRoot() );
    m_mobileBackButton->addStyleClass( "MobilePrevSample btn" );
    m_mobileBackButton->setZIndex( 8388635 );
    m_mobileBackButton->clicked().connect( boost::bind(&InterSpec::handleUserIncrementSampleNum, this, kForeground, false) );
    m_mobileBackButton->setHidden(true);
      
    m_mobileForwardButton = new WPushButton( "", wApp->domRoot() );
    m_mobileForwardButton->addStyleClass( "MobileNextSample btn" );
    m_mobileForwardButton->setZIndex( 8388635 );
    m_mobileForwardButton->clicked().connect( boost::bind(&InterSpec::handleUserIncrementSampleNum, this, kForeground, true) );
    m_mobileForwardButton->setHidden(true);
  }else
  {
    m_menuDiv = new WContainerWidget();
    m_menuDiv->addStyleClass( "TopMenuDiv" );
    menuWidget = m_menuDiv;
  }
  
  addFileMenu( menuWidget, isMobile()  );
  addDisplayMenu( menuWidget );
  
  addToolsMenu( menuWidget );
  addAboutMenu( menuWidget );

  
  /* Set the loading indicator so that it's the highest z-index, so always visible  */
  Wt::WApplication *app = Wt::WApplication::instance();
  WDefaultLoadingIndicator *indicator = new Wt::WDefaultLoadingIndicator();
  indicator->addStyleClass( "LoadingIndicator" );
  app->setLoadingIndicator( indicator );
    
  if( m_menuDiv )
  {
    WImage *snlLogo = new WImage( "InterSpec_resources/images/SNL_Stacked_Black_Blue_tiny.png", m_menuDiv );  //ToDo: maybe put a link to sandia.gov when/if this is clicked
    snlLogo->addStyleClass("URLogo");
    m_menuDiv->setStyleClass( "UpperMenuDivWithLogos" );
    snlLogo->setFloatSide( Right );
  }//if( m_menuDiv )

  m_layout = new WGridLayout();
  m_layout->setContentsMargins( 0, 0, 0, 0 );
  m_layout->setHorizontalSpacing( 0 );

  setLayout( m_layout );
  
  if( m_menuDiv )
    m_layout->addWidget( m_menuDiv, m_layout->rowCount(), 0 );
  m_layout->addWidget( m_spectrum, m_layout->rowCount(), 0 );
  m_layout->addWidget( m_timeSeries, m_layout->rowCount(), 0 );
  
  m_timeSeries->enableLegend( false );
  m_timeSeries->showHistogramIntegralsInLegend( false );
  m_timeSeries->setMouseDragHighlights( true, true );

  if( m_timeSeries->overlayCanvasEnabled() )
    m_timeSeries->allowArrowToMoveSingleClickRegion( true );

  m_timeSeries->xRangeChanged().connect( boost::bind(
      &InterSpec::changeTimeRange, this, _1, _2, kForeground ) );
  
  m_timeSeries->shiftKeyDragged().connect(
      boost::bind( &InterSpec::sampleNumbersToDisplayAddded, this, _1, _2,
                 kForeground ) );
  
  m_timeSeries->altKeyDragged().connect( boost::bind(
      &InterSpec::changeTimeRange, this, _1, _2, kBackground ) );
  
  m_timeSeries->shiftAltKeyDragged().connect(
      boost::bind( &InterSpec::sampleNumbersToDisplayAddded, this, _1, _2,
                   kBackground ) );
  
  
  m_spectrum->setXAxisTitle( "Energy (keV)" );
  m_spectrum->setYAxisTitle( "Counts/Channel" );
  m_timeSeries->setXAxisTitle( "Real Time of Measurement (seconds)" );
  m_timeSeries->setYAxisTitle( "Gamma CPS" );
  m_timeSeries->setY2AxisTitle( "Neutron CPS" );
  m_timeSeries->setAutoAdjustDisplayRebinFactor( true );
  m_spectrum->setXAxisTitle( "Energy (keV)" );
  m_spectrum->setYAxisTitle( "Counts/Channel" );

  m_spectrum->enableLegend( false );
  m_spectrum->showHistogramIntegralsInLegend( true );
#if( USE_SPECTRUM_CHART_D3 )
#else
  m_spectrum->setAutoAdjustDisplayRebinFactor( true );
#endif
  m_spectrum->shiftAltKeyDragged().connect( this, &InterSpec::handleShiftAltDrag );

//  m_spectrum->rightClicked().connect( boost::bind( &InterSpec::createPeakEdit, this, _1) );
  m_rightClickMenu = new PopupDivMenu( m_mobileMenuButton, PopupDivMenu::TransientMenu );
  m_rightClickMenu->setPositionScheme( Wt::Absolute );
  m_rightClickMenu->addStyleClass( " Wt-popupmenu Wt-outset" );
  m_rightClickMenu->aboutToHide().connect( this, &InterSpec::rightClickMenuClosed );
  
  if( isMobile() )
  {
    PopupDivMenuItem *item = m_rightClickMenu->addPhoneBackItem( NULL );
    item->triggered().connect( boost::bind(&PopupDivMenu::setHidden, m_rightClickMenu, true, WAnimation()) );
  }//if( isPhone() || isTablet() )
  
  for( RightClickItems i = RightClickItems(0);
       i < kNumRightClickItems; i = RightClickItems(i+1) )
  {
    switch( i )
    {
      case kPeakEdit:
        m_rightClickMenutItems[i] = m_rightClickMenu->addMenuItem( "Peak Editor..." );
        m_rightClickMenutItems[i]->triggered().connect( this, &InterSpec::peakEditFromRightClick );
        break;
      case kRefitPeak:
        m_rightClickMenutItems[i] = m_rightClickMenu->addMenuItem( "Refit Peak" );
        m_rightClickMenutItems[i]->triggered().connect( this, &InterSpec::refitPeakFromRightClick );
        break;
      case kRefitROI:
        m_rightClickMenutItems[i] = m_rightClickMenu->addMenuItem( "Refit ROI" );
        m_rightClickMenutItems[i]->triggered().connect( this, &InterSpec::refitPeakFromRightClick );
        break;
      case kDeletePeak:
        m_rightClickMenutItems[i] = m_rightClickMenu->addMenuItem( "Delete Peak" );
        m_rightClickMenutItems[i]->triggered().connect( this, &InterSpec::deletePeakFromRightClick );
        break;
      case kChangeNuclide:
        m_rightClickNuclideSuggestMenu = m_rightClickMenu->addPopupMenuItem( "Change Nuclide" );
        m_rightClickMenutItems[i] = m_rightClickNuclideSuggestMenu->parentItem();
        break;
      case kAddPeak:
        m_rightClickMenutItems[i] = m_rightClickMenu->addMenuItem( "Add Peak" );
        m_rightClickMenutItems[i]->triggered().connect( this, &InterSpec::addPeakFromRightClick );
        break;
      case kShareContinuumWithLeftPeak:
        m_rightClickMenutItems[i] = m_rightClickMenu->addMenuItem( "Combine Cont. Left" );
        m_rightClickMenutItems[i]->triggered().connect( boost::bind( &InterSpec::shareContinuumWithNeighboringPeak, this, true ) );
        break;
      case kShareContinuumWithRightPeak:
        m_rightClickMenutItems[i] = m_rightClickMenu->addMenuItem( "Combine Cont. Right" );
        m_rightClickMenutItems[i]->triggered().connect( boost::bind( &InterSpec::shareContinuumWithNeighboringPeak, this, false ) );
        break;
        
      case kMakeOwnContinuum:
        m_rightClickMenutItems[i] = m_rightClickMenu->addMenuItem( "Own Continuum" );
        m_rightClickMenutItems[i]->triggered().connect( this, &InterSpec::makePeakFromRightClickHaveOwnContinuum );
        break;
        
      case kNumRightClickItems:
        break;
    }//switch( i )
  }//for( loop over right click menu items )
  
  m_spectrum->rightClicked().connect( boost::bind( &InterSpec::handleRightClick, this, _1, _2, _3, _4 ) );
  m_spectrum->chartClicked().connect( boost::bind( &InterSpec::handleLeftClick, this, _1, _2, _3, _4 ) );
  
//  m_spectrum->controlKeyDragged().connect( boost::bind( &InterSpec::findPeakFromUserRange, this, _1, _2 ) );
  m_spectrum->controlKeyDragged().connect( boost::bind( &InterSpec::userAskedToFitPeaksInRange, this, _1, _2, _3, _4 ) );
  
  m_spectrum->shiftKeyDragged().connect( boost::bind( &InterSpec::excludePeaksFromRange, this, _1, _2 ) );
  m_spectrum->doubleLeftClick().connect( boost::bind( &InterSpec::searchForSinglePeak, this, _1 ) );
  
  
  
  m_timeSeries->setHidden( true );
  m_layout->setVerticalSpacing( 0 );
  
  m_layout->setRowStretch( m_menuDiv ? 1 : 0, 5 );
  m_layout->setRowStretch( m_menuDiv ? 2 : 1, 3 );
  
#if( USE_OSX_NATIVE_MENU )
  m_menuDiv->hide();
#endif
  
#if( BUILD_AS_ELECTRON_APP && USE_ELECTRON_NATIVE_MENU )
  m_menuDiv->hide();
  PopupDivMenu::triggerElectronMenuUpdate();
#endif
  
  if( m_user->preferenceValue<bool>( "StartDocked" ) && !isPhone() )
  {
    setToolTabsVisible( true );
  }else
  {
    //Disallow showing tool tabs when on a phone
    if( !isPhone() )
      m_toolTabsVisibleItems[0]->setHidden(false);
    m_toolTabsVisibleItems[1]->setHidden(true);
    
    //Add Ref photopeaks, etc. to tool menu
    addToolsTabToMenuItems();
  }//If( start with tool tabs showing ) / else
 
#if( !ANDROID && !IOS )
  initDragNDrop();
#endif
  
  initWindowZoomWatcher();
  
#if( USE_DB_TO_STORE_SPECTRA )
  updateSaveWorkspaceMenu();
#endif
  
#if( APPLY_OS_COLOR_THEME_FROM_JS && !BUILD_AS_OSX_APP )
  initOsColorThemeChangeDetect();
#endif
  
  applyColorTheme( nullptr );
}//InterSpec( constructor )


void InterSpec::setStaticDataDirectory( const std::string &dir )
{
  std::lock_guard<std::mutex> lock( sm_staticDataDirectoryMutex );
  
  if( !UtilityFunctions::is_directory(dir) )
    throw runtime_error( "InterSpec::setStaticDataDirectory(): " + dir + " is not a directory." );
  
  sm_staticDataDirectory = dir;

#ifdef _WIN32
  MassAttenuation::set_data_directory( UtilityFunctions::convert_from_utf8_to_utf16(dir) );
#else
  MassAttenuation::set_data_directory( dir );
#endif
  const string rctn_xml_file = UtilityFunctions::append_path( dir, "sandia.reactiongamma.xml" );
  if( !UtilityFunctions::is_file(rctn_xml_file) )
    throw runtime_error( "InterSpec::setStaticDataDirectory(): " + dir + " does not contain a sandia.reactiongamma.xml file." );
  ReactionGammaServer::set_xml_file_location( rctn_xml_file );
  
  const string decay_xml_file = UtilityFunctions::append_path( dir, "sandia.decay.xml" );
  if( !UtilityFunctions::is_file(decay_xml_file) )
    throw runtime_error( "InterSpec::setStaticDataDirectory(): " + dir + " does not contain a sandia.decay.xml file." );
  DecayDataBaseServer::setDecayXmlFile( decay_xml_file );
}//void setStaticDataDirectory( const std::string &dir )


std::string InterSpec::staticDataDirectory()
{
  std::lock_guard<std::mutex> lock( sm_staticDataDirectoryMutex );
  return sm_staticDataDirectory;
}

#if( BUILD_AS_ELECTRON_APP || IOS || ANDROID || BUILD_AS_OSX_APP || (BUILD_AS_LOCAL_SERVER && (defined(WIN32) || defined(__APPLE__)) ) )
void InterSpec::setWritableDataDirectory( const std::string &dir )
{
  std::lock_guard<std::mutex> lock( sm_writableDataDirectoryMutex );
  
  if( !dir.empty() && !UtilityFunctions::is_directory(dir) )
    throw runtime_error( "InterSpec::setWritableDataDirectory(): " + dir + " is not a directory." );
  
  //Set the serial to module database file, if the user has one in thier
  //  application data directory.
  //Currently this is being done in the target specific code; it should all be
  //  moved here, or really in ResourceUpdate tool if that ever gets implemented.
  //const vector<string> serial_db = UtilityFunctions::ls_files_in_directory( dir, "serial_to_model.csv" );
  //if( !serial_db.empty() )
  //  SerialToDetectorModel::set_detector_model_input_csv( serial_db[0] );
  
  sm_writableDataDirectory = dir;
}//setWritableDataDirectory( const std::string &dir )

std::string InterSpec::writableDataDirectory()
{
  std::lock_guard<std::mutex> lock( sm_writableDataDirectoryMutex );
  
  if( sm_writableDataDirectory.empty() )
    throw runtime_error( "writableDataDirectory hasnt been set." );
  
  return sm_writableDataDirectory;
}//string writableDataDirectory()
#endif  //if( not a webapp )



InterSpec::~InterSpec()
{
  //The deletion of the DOM root node will destrow all the AuxWindows we
  //  have open, but I am manually taking care of them below due to a crash
  //  I have been getting in the WApplication destructor for Wt 3.3.1-rc1

  cerr << "Destructing InterSpec from session '" << (wApp ? wApp->sessionId() : string("")) << "'" << endl;

  if( m_licenseWindow )
  {
    delete m_licenseWindow;
    m_licenseWindow = nullptr;
  }
  
  closeShieldingSourceFitWindow();
  
  if( m_peakInfoDisplay )
  {
    if( m_toolsTabs && m_toolsTabs->indexOf(m_peakInfoDisplay)>=0 )
        m_toolsTabs->removeTab( m_peakInfoDisplay );
    if( m_peakInfoWindow )
      m_peakInfoWindow->contents()->removeWidget( m_peakInfoDisplay );
    delete m_peakInfoDisplay;
    m_peakInfoDisplay = nullptr;
  }//if( m_peakInfoDisplay )

  if( m_peakInfoWindow )
  {
    delete m_peakInfoWindow;
    m_peakInfoWindow = nullptr;
  }//if( m_peakInfoWindow )
  
  if( m_recalibrator )
  {
    if( m_toolsTabs && m_toolsTabs->indexOf(m_calibrateContainer)>=0 )
      m_toolsTabs->removeTab( m_calibrateContainer );
    
    if( m_recalibratorWindow )
      m_recalibratorWindow->stretcher()->removeWidget( m_recalibrator );
    
    delete m_recalibrator;
    m_recalibrator = nullptr;
    
    if( m_calibrateContainer )
    {
      delete m_calibrateContainer;
      m_calibrateContainer = nullptr;
    }//if( m_calibrateContainer )
  }//if( m_recalibrator )
  
  if( m_recalibratorWindow )
  {
    delete m_recalibratorWindow;
    m_recalibratorWindow = nullptr;
  }//if( m_recalibratorWindow )
    
  if( m_shieldingSourceFitWindow )
  {
    delete m_shieldingSourceFitWindow;
    m_shieldingSourceFitWindow = nullptr;
  }//if( m_shieldingSourceFitWindow )
  
  if( m_nuclideSearchWindow )
  {
    delete m_nuclideSearchWindow;
    m_nuclideSearchWindow = nullptr;
  }
  
  if( m_referencePhotopeakLines )
  {
    if( m_toolsTabs && m_toolsTabs->indexOf(m_referencePhotopeakLines)>=0 )
      m_toolsTabs->removeTab( m_referencePhotopeakLines );
    
    m_referencePhotopeakLines->clearAllLines();
    delete m_referencePhotopeakLines;
    m_referencePhotopeakLines = nullptr;
  }//if( m_referencePhotopeakLines )
  
  if( m_referencePhotopeakLinesWindow )
  {
    delete m_referencePhotopeakLinesWindow;
    m_referencePhotopeakLinesWindow = nullptr;
  }//if( m_referencePhotopeakLinesWindow )
  
  if( m_warnings )
  {
    if( m_warningsWindow )
      m_warningsWindow->stretcher()->removeWidget( m_warnings );
    delete m_warnings;
    m_warnings = nullptr;
  }//if( m_warnings )

  if( m_warningsWindow )
  {
    delete m_warningsWindow;
    m_warningsWindow = nullptr;
  }//if( m_warningsWindow )
  
  if( m_isotopeSearch )
  {
    delete m_isotopeSearch;
    m_isotopeSearch = nullptr;
  }//if( m_isotopeSearch )
  
  if( m_peakEditWindow )
  {
    delete m_peakEditWindow;
    m_peakEditWindow = nullptr;
  }//if( m_peakEditWindow )
  
  if( m_mobileMenuButton )
    delete m_mobileMenuButton;

  if (m_mobileBackButton )
    delete m_mobileBackButton;
  
  if (m_mobileForwardButton)
    delete m_mobileForwardButton;
  
  if( m_notificationDiv )
    delete m_notificationDiv;
  
  if( m_fileManager )
    delete m_fileManager;

  if( m_shieldingSuggestion )
    delete m_shieldingSuggestion;
  m_shieldingSuggestion = NULL;

  if( m_menuDiv )
  {
    delete m_menuDiv;
    m_menuDiv = nullptr;
  }//if( m_menuDiv )

}//InterSpec destructor



#if( SpecUtils_ENABLE_D3_CHART )

string InterSpec::print_d3_reference_gammas() const
{
  string result;
  
  if( m_referencePhotopeakLines )
  {
    result = m_referencePhotopeakLines->jsonReferenceLinesArray();
  }else
  {
    result = "[]";
  }//if( m_referencePhotopeakLines ) / else
  
  return result;
}//string InterSpec::print_d3_reference_gammas() const

//Temporary function (20160224) to aid in development of d3.js spectrum rendering
string InterSpec::print_d3_json() const
{
  std::ostringstream ostr;
  const char *q = "\"";  // for creating valid json format
    
  std::shared_ptr<const Measurement> foreground = m_spectrum->data();
  std::shared_ptr<const Measurement> background = m_spectrum->background();
  std::shared_ptr<const Measurement> secondary  = m_spectrum->secondData();
    
  std::shared_ptr<Measurement> data = m_spectrum->data();
  std::shared_ptr<Measurement> back = m_spectrum->background();
  std::shared_ptr<Measurement> second = m_spectrum->secondData();
    
  typedef deque< PeakModel::PeakShrdPtr > PeakDeque;
  string peakstring;
  
  // Update time
  ostr << "{\n\t" << q << "updateTime" << q << ":" << q << UtilityFunctions::to_iso_string(boost::posix_time::second_clock::local_time()) << q;
  
  // spectrum values
  ostr << ",\n\t" << q << "spectra" << q << ": [";
  

  if( data )
  {
    string title = Wt::WWebWidget::escapeText(data->title()).toUTF8();
    if( title != data->title() )
      data->set_title( title );  //JIC, proper escaping not implemented in SpecUtils yet.
    
    D3SpectrumExport::D3SpectrumOptions options;
    options.line_color = "black";
    options.display_scale_factor = displayScaleFactor(kForeground);
    
    std::shared_ptr<const PeakDeque > peaks = m_dataMeasurement->peaks(m_displayedSamples);
    if( peaks )
    {
      vector<PeakModel::PeakShrdPtr> inpeaks( peaks->begin(), peaks->end() );
      options.peaks_json = PeakDef::peak_json( inpeaks );
    }
    
    D3SpectrumExport::write_spectrum_data_js( ostr, *data, options, 0, 1 );
  }

  if( back )
  {
    if( data )
      ostr << ",";
    
    string title = Wt::WWebWidget::escapeText(back->title()).toUTF8();
    if( title != back->title() )
      back->set_title( title );  //JIC, proper escaping not implemented in SpecUtils yet.
    
    D3SpectrumExport::D3SpectrumOptions options;
    options.line_color = "steelblue";
    options.display_scale_factor = displayScaleFactor(kBackground);
    
    std::shared_ptr<const PeakDeque > peaks = m_backgroundMeasurement->peaks(m_backgroundSampleNumbers);
    if( peaks )
    {
      vector<PeakModel::PeakShrdPtr> inpeaks( peaks->begin(), peaks->end() );
      options.peaks_json = PeakDef::peak_json( inpeaks );
    }
    
    D3SpectrumExport::write_spectrum_data_js( ostr, *back, options, 1, -1 );
  }
  
  
  if( second )
  {
    if( data || back )
      ostr << ",";
    
    string title = Wt::WWebWidget::escapeText(second->title()).toUTF8();
    if( title != second->title() )
      second->set_title( title );  //JIC, proper escaping not implemented in SpecUtils yet.
    
    D3SpectrumExport::D3SpectrumOptions options;
    options.line_color = "green";
    options.display_scale_factor = displayScaleFactor(kSecondForeground);
    
    std::shared_ptr<const PeakDeque > peaks = m_backgroundMeasurement->peaks(m_sectondForgroundSampleNumbers);
    if( peaks )
    {
      vector<PeakModel::PeakShrdPtr> inpeaks( peaks->begin(), peaks->end() );
      options.peaks_json = PeakDef::peak_json( inpeaks );
    }
    
    D3SpectrumExport::write_spectrum_data_js( ostr, *second, options, 2, 1 );
  }
  
  // end of spectrum json
  ostr << "\n\t]";
  // end of json
  ostr << "\n}\n";
  return ostr.str();
}//std::string InterSpec::print_d3_json() const


D3SpectrumExport::D3SpectrumChartOptions InterSpec::getD3SpectrumOptions() const
{
  double xMin, xMax, yMin, yMax;
  displayedSpectrumRange(xMin, xMax, yMin, yMax);
  
  map<string,string> referc_line_json;
  
  if( m_referencePhotopeakLines )
    referc_line_json = m_referencePhotopeakLines->jsonReferenceLinesMap();
  
  D3SpectrumExport::D3SpectrumChartOptions options(
                                 /* title: */"Interactive Spectrum Development",
                                 /* xAxisTitle: */"Energy", /* yAxisTitle: */"Counts",
                                 /* dataTitle: */(m_spectrum->data() ? m_spectrum->data()->title() :
                                                  m_spectrum->background() ? m_spectrum->background()->title() :
                                                  m_spectrum->secondData() ? m_spectrum->secondData()->title() :
                                                  "Foreground"),
                                 /* useLogYAxis: */m_spectrum->yAxisIsLog(),
                                 /* showVerticalGridLines: */m_spectrum->verticalLinesShowing(),
                                 /* showHorizontalGridLines: */m_spectrum->horizontalLinesShowing(),
                                 /* legendEnabled: */m_spectrum->legendIsEnabled(),
                                 /* compactXAxis: */m_spectrum->isAxisCompacted(),
                                 /* showPeakUserLabels: */m_spectrum->showingPeakLabel( SpectrumChart::kShowPeakUserLabel ),
                                 /* showPeakEnergyLabels: */m_spectrum->showingPeakLabel( SpectrumChart::kShowPeakEnergyLabel ),
                                 /* showPeakNuclideLabels: */m_spectrum->showingPeakLabel( SpectrumChart::kShowPeakNuclideLabel ),
                                 /* showPeakNuclideEnergyLabels: */ m_spectrum->showingPeakLabel( SpectrumChart::kShowPeakNuclideEnergies ),
                                 /* showEscapePeakMarker: */m_featureMarkersShown[EscapePeakMarker],
                                 /* showComptonPeakMarker: */m_featureMarkersShown[ComptonPeakMarker],
                                 /* showComptonEdgeMarker: */m_featureMarkersShown[ComptonEdgeMarker],
                                 /* showSumPeakMarker: */m_featureMarkersShown[SumPeakMarker],
                                 /* backgroundSubtract: */m_spectrum->backgroundSubtract(),
                                 /* xMin: */xMin, /* xMax: */xMax,
                                 referc_line_json
  );

  return options;
}
#endif //#if( SpecUtils_ENABLE_D3_CHART )

/**
 Calls CompactFileManager's refactored and static method handleUserIncrementSampleNum
 */
void InterSpec::handleUserIncrementSampleNum( SpectrumType type,
                                                      bool increment)
{
    CompactFileManager::handleUserIncrementSampleNum(type, increment, this, m_fileManager->model(), NULL);
}

std::shared_ptr<DataBaseUtils::DbSession> InterSpec::sql()
{
  return m_sql;
};


void InterSpec::layoutSizeChanged( int w, int h )
{
  m_renderedWidth = w;
  m_renderedHeight = h;
  
  const bool comactX = (h <= 420); //Apple iPhone 6+, 6s+, 7+, 8+
  if( (h > 20) && (comactX != m_spectrum->isAxisCompacted()) )
  {
    //Only set axis compact if it isnt already; dont ever set non-compact
    //  ToDo: For case screen is made small, so axis goes compact, then user
    //        makes screen large, currently wont un-compactify axis, even if
    //        user wants this; should evaluate if its worth checking the user
    //        preference about this.
    //const bool makeCompact = InterSpecUser::preferenceValue<bool>( "CompactXAxis", this );
    
    if( comactX && !m_spectrum->isAxisCompacted() )
      m_spectrum->setCompactAxis( comactX );
    
    if( comactX && !m_timeSeries->isAxisCompacted() )
      m_timeSeries->setCompactAxis( comactX );
  }
  
#if( IOS || ANDROID )
  //When the soft-keyboard disapears (on Android at a minimum), the overlays
  //  dont resize properly (until you change tab below the chart, or something)
  //  so we will force it.
#if( !USE_SPECTRUM_CHART_D3 )
  m_spectrum->forceOverlayAlign();
#endif
  
  if( !m_timeSeries->isHidden() )
    m_timeSeries->forceOverlayAlign();
#endif  //#if( IOS || ANDROID )
}//void layoutSizeChanged( int w, int h )

//isSupportFile(): If the platform supports file transfer.  Use this method
//instead of #if(IOS) or isMobile() because those hold no context.
bool InterSpec::isSupportFile() const
{
#if( IOS )
    //return false;
  return true; //20180602: it looks like web version of InterSpec allows you to browse for files (from google drive, etc) - not tried from app yet though.
#else
    return true;
#endif
}

float InterSpec::kevPerPixel() const
{
  return m_spectrum->xUnitsPerPixel();
}


bool InterSpec::isMobile() const
{
  return (m_clientDeviceType & MobileClient);
}

bool InterSpec::isPhone() const
{
  return (m_clientDeviceType & PhoneClient);
}

bool InterSpec::isTablet() const
{
  return (m_clientDeviceType & TabletClient);
}

bool InterSpec::isDesktop() const
{
  return (m_clientDeviceType & DesktopBrowserClient);
}

bool InterSpec::isDedicatedApp() const
{
  return (m_clientDeviceType & DedicatedAppClient);
}

void InterSpec::detectClientDeviceType()
{
  m_clientDeviceType= 0x0;

  InterSpecApp *app= dynamic_cast<InterSpecApp *>( wApp );
  if( !app )
    return;


  const bool phone= app->isPhone();
  const bool tablet= app->isTablet();
  const bool mobile= app->isMobile();

  for( ClientDeviceType type= ClientDeviceType( 0x1 );
       type < NumClientDeviceType; type= ClientDeviceType( type << 1 ) )
  {
    switch( type )
    {
      case DesktopBrowserClient:
        if( !phone && !tablet && !mobile )
          m_clientDeviceType |= type;
        break;

      case PhoneClient:
        if( phone )
          m_clientDeviceType |= type;
        break;

      case TabletClient:
        if( tablet )
          m_clientDeviceType |= type;
        break;

      case MobileClient:
        if( mobile )
          m_clientDeviceType |= type;
        break;

      case HighBandwithClient:
#if( !BUILD_FOR_WEB_DEPLOYMENT )
        m_clientDeviceType|= type;
#else
        if( app->environment().clientAddress().find( "127.0.0" ) != string::npos )
          m_clientDeviceType|= type;
// should probably do some other testing here....
#endif
        //        std::cerr << "env.clientAddress()=" << env.clientAddress() <<
        //        std::endl;
        //        env.clientAddress() //134.252.17.78 (from anothother
        //        computer), 127.0.0.1 from same computer
        break;

      case DedicatedAppClient:
#if( !BUILD_FOR_WEB_DEPLOYMENT )
        m_clientDeviceType|= type;
#endif
        break;

      case NumClientDeviceType:
        break;
    } // switch( type )
  } // for( loop over ClientDeviceType enums )
} // void detectClientDeviceType()


#if( !ANDROID && !IOS )
void InterSpec::initDragNDrop()
{
  LOAD_JAVASCRIPT(wApp, "js/InterSpec.js", "InterSpec", wtjsFileUploadFcn);
  
  doJavaScript( "$('.Wt-domRoot').data('ForegroundUpUrl','" +
               m_fileManager->foregroundDragNDrop()->url() + "');" );
  
  doJavaScript( "$('.Wt-domRoot').data('BackgroundUpUrl','" +
               m_fileManager->backgroundDragNDrop()->url() + "');" );
  
  doJavaScript( "$('.Wt-domRoot').data('SecondUpUrl','" +
               m_fileManager->secondForegroundDragNDrop()->url() + "');" );
  
  doJavaScript( "Wt.WT.FileUploadFcn();" );
}//void InterSpec::initDragNDrop()
#endif //#if( !ANDROID && !IOS )


void InterSpec::initWindowZoomWatcher()
{
  //Look for window.onresize events, and force the JS canvases to re-align....
  string command;
  
#if( !USE_SPECTRUM_CHART_D3 )
  JSlot *specslot = alignSpectrumOverlayCanvas();
  if( specslot )
    command += specslot->execJs();
#endif
  
  JSlot *timeslot = alignTimeSeriesOverlayCanvas();
  if( timeslot )
    command += timeslot->execJs();  //Christian [04182018]: Changed "specslot->execJs()" to "timeslot->execJs()", causing issues with D3 charts
  
  if( !command.empty() )
  {
    //command += "console.log('got resize event');";
    //The onresize event gets called before Wt adjusts the layout of all the
    //  widgets, meaning the new height/width of the canvases arent available.
    //  I'm not sure why the JS member function wtResize doesnt get called when
    //  there are zoom changes...
    //Another option would be to call back to the c++ on resize, then have it
    //  execute the JS, which might be alittle more robust than a set delay of
    //  half a second
    wApp->doJavaScript("$(window).on('resize', function(){setTimeout(function(){" + command + "},500);});");
  }//if( !command.empty() )
}//void initWindowZoomWatcher()

void InterSpec::initHotkeySignal()
{
  //TODO: currenlty integers are used to represent the shortcuts; this should
  //      be changed to an enum.  The reason integers are used rather than just
  //      the key code 
  if( !!m_hotkeySignal )
    return;
  
  //We are specifying for the javascript to not be collected since the response
  //  will change if the tools tabs is shown or not.
  m_hotkeySignal.reset( new JSignal<unsigned int>( this, "hotkey", false ) );
  
  //sender.id was undefined in the following js, so had to work around this a bit
  const char *js = INLINE_JAVASCRIPT(
    function(id,e){
      if(!e||(typeof e.keyCode === 'undefined'))
        return;
      if(e.metaKey||e.altKey||!e.ctrlKey||e.shiftKey)
        return;
      var v = 0;
      switch( e.keyCode ){
        case 83: case 49: v=1; break; //s
        case 80: case 50: v=2; break; //p
        case 82: case 51: v=3; break; //r
        case 69: case 52: v=4; break; //e
        case 78: case 53: v=5; break; //n
        case 72:          v=6; break; //h
        case 73:          v=7; break; //i
        default:
          return;  //show
      }
      
      if(v){
        e.preventDefault();
        e.stopPropagation();
        Wt.emit(id,{name:'hotkey'},v);
      }
    }
  );
  
  const string jsfcn = string("function(s,e){var f=")+js+";f('"+id()+"',e);}";
  m_hotkeySlot.reset( new JSlot( jsfcn, this ) );

  wApp->domRoot()->keyWentDown().connect( *m_hotkeySlot );
  m_hotkeySignal->connect( boost::bind( &InterSpec::hotkeyPressed, this, _1 ) );
}//void initHotkeySignal()


void InterSpec::hotkeyPressed( const unsigned int value )
{
  if( m_toolsTabs )
  {
    string expectedTxt;
    switch( value )
    {
      case 1: expectedTxt = FileTabTitle;          break;
      case 2: expectedTxt = PeakInfoTabTitle;      break;
      case 3: expectedTxt = GammaLinesTabTitle;    break;
      case 4: expectedTxt = CalibrationTabTitle;   break;
      case 5: expectedTxt = NuclideSearchTabTitle; break;
      case 6: HelpSystem::createHelpWindow( "getting-started" ); break;
      case 7: showWelcomeDialog( true ); break;
    }//switch( value )
  
    if( expectedTxt.empty() )
      return;
  
    for( int i = 0; i < m_toolsTabs->count(); ++i )
    {
      if( m_toolsTabs->tabText(i).toUTF8() == expectedTxt )
      {
        m_toolsTabs->setCurrentIndex( i );
        handleToolTabChanged( i );
        break;
      }
    }//for( int i = 0; i < m_toolsTabs->count(); ++i )
  }else
  {
    switch( value )
    {
      case 1: showCompactFileManagerWindow(); break;
      case 2: showPeakInfoWindow();           break;
      case 3: showGammaLinesWindow();         break;
      case 4: showRecalibratorWindow();       break;
      case 5: showNuclideSearchWindow();      break;
    }//switch( value )
  }//if( tool tabs visible ) / else
}//void hotkeyPressed( const int value )



void InterSpec::rightClickMenuClosed()
{
  m_rightClickEnergy = -DBL_MAX;
}//void rightClickMenuClosed()

#if( USE_SIMPLE_NUCLIDE_ASSIST )
void InterSpec::leftClickMenuClosed()
{
  if( !m_leftClickMenu )
    return;
}
#endif

void InterSpec::peakEditFromRightClick()
{
  if( m_rightClickEnergy < -99999.9 )
    return;
  createPeakEdit( m_rightClickEnergy );
}//void peakEditFromRightClick()


std::shared_ptr<const PeakDef> InterSpec::nearestPeak( const double energy ) const
{
  std::shared_ptr<const PeakDef> nearPeak;
  double minDE = std::numeric_limits<double>::infinity();
  
  const int nrow = m_peakModel->rowCount();
  for( int row = 0; row < nrow; ++row )
  {
    WModelIndex index = m_peakModel->index( row, 0 );
    const PeakModel::PeakShrdPtr &peak = m_peakModel->peak( index );
    const double dE = fabs( peak->mean() - energy );
    if( (dE < minDE)
        && ((energy > peak->lowerX()) && (energy < peak->upperX())) )
    {
      minDE = dE;
      nearPeak = peak;
    }//if( dE < minDE )
  }//for( int row = 0; row < nrow; ++row )

  return nearPeak;
}//std::shared_ptr<const PeakDef> nearestPeak() const

void InterSpec::refitPeakFromRightClick()
{
  std::shared_ptr<const PeakDef> peak = nearestPeak( m_rightClickEnergy );
  if( !peak )
  {
    passMessage( "There was no peak to refit", "", WarningWidget::WarningMsgInfo );
    return;
  }
  
  PeakShrdVec inpeakOrigs;
  vector<PeakDef> inputPeak, fixedPeaks, outputPeak;
  
  WModelIndex peakIndex;
  
  const int npeak = static_cast<int>(m_peakModel->npeaks());
  for( int peakn = 0; peakn < npeak; ++peakn )
  {
    WModelIndex index = m_peakModel->index( peakn, 0 );
    std::shared_ptr<const PeakDef> thispeak = m_peakModel->peak(index);
    if( thispeak == peak || peak->continuum() == thispeak->continuum() )
    {
      peakIndex = index;
      inputPeak.push_back( *thispeak );
      inpeakOrigs.push_back( thispeak );
    }else
      fixedPeaks.push_back( *thispeak );
  }//for( int peak = 0; peak < npeak; ++peak )
  
  if( !peakIndex.isValid() || inputPeak.empty() )
  {
    passMessage( "Error finding peak to refit", "", WarningWidget::WarningMsgHigh );
    return;
  }
  
  std::sort( inputPeak.begin(), inputPeak.end(), &PeakDef::lessThanByMean );
  
  std::shared_ptr<const Measurement> data = m_spectrum->data();
  
  if( inputPeak.size() > 1 )
  {
    const std::shared_ptr<DetectorPeakResponse> &detector
                                                = m_dataMeasurement->detector();
    PeakShrdVec result = refitPeaksThatShareROI( data, detector, inpeakOrigs, 0.25 );
    
    if( result.size() == inputPeak.size() )
    {
      for( size_t i = 0; i < result.size(); ++i )
        fixedPeaks.push_back( *result[i] );
      std::sort( fixedPeaks.begin(), fixedPeaks.end(), &PeakDef::lessThanByMean );
      m_peakModel->setPeaks( fixedPeaks );
#if ( USE_SPECTRUM_CHART_D3 )
      m_spectrum->updateData();
#endif
      return;
    }else
    {
      cerr << "refitPeaksThatShareROI was not successful" << endl;
    }//if( result.size() == inputPeak.size() ) / else
  }//if( inputPeak.size() > 1 )
  
  
//  const double lowE = peak->mean() - 0.1;
//  const double upE = peak->mean() + 0.1;
  const double lowE = inputPeak.front().mean() - 0.1;
  const double upE = inputPeak.back().mean() + 0.1;
  const double ncausalitysigma = 0.0;
  const double stat_threshold  = 0.0;
  const double hypothesis_threshold = 0.0;
  
  const bool isRefit = true;
  
  outputPeak = fitPeaksInRange( lowE, upE, ncausalitysigma, stat_threshold,
                               hypothesis_threshold, inputPeak, data,
                               fixedPeaks, isRefit );
  if( outputPeak.size() != inputPeak.size() )
  {
    WStringStream msg;
    msg << "Failed to refit peak (became insignificant), from "
        << int(inputPeak.size()) << " to " << int(outputPeak.size()) << " peaks";
    passMessage( msg.str(), "", WarningWidget::WarningMsgInfo );
    return;
  }//if( outputPeak.size() != 1 )
  
  if( inputPeak.size() > 1 )
  {
    fixedPeaks.insert( fixedPeaks.end(), outputPeak.begin(), outputPeak.end() );
    std::sort( fixedPeaks.begin(), fixedPeaks.end(), &PeakDef::lessThanByMean );
    m_peakModel->setPeaks( fixedPeaks );
  }else
  {
    m_peakModel->removePeak( peakIndex );
    addPeak( outputPeak[0], false );
  }//if( inputPeak.size() > 1 )
#if ( USE_SPECTRUM_CHART_D3 )
  m_spectrum->updateData();
#endif
}//void refitPeakFromRightClick()


void InterSpec::addPeakFromRightClick()
{
  std::shared_ptr<const Measurement> dataH = m_spectrum->data();
  std::shared_ptr<const PeakDef> peak = nearestPeak( m_rightClickEnergy );
  if( !peak
      || m_rightClickEnergy < peak->lowerX()
      || m_rightClickEnergy > peak->upperX()
      || !m_dataMeasurement
      || !dataH )
  {
    passMessage( "There was no ROI to add peak to",
                 "", WarningWidget::WarningMsgInfo );
    return;
  }//if( !peak )
  
  //get all the peaks that belong to this ROI
  typedef std::shared_ptr<const PeakContinuum> ContPtr;
  typedef map<ContPtr, std::vector<PeakDef > > ContToPeakMap;
  
  
  const auto origContinumm = peak->continuum();
  const std::shared_ptr<const deque<PeakModel::PeakShrdPtr>> allOrigPeaks = m_peakModel->peaks();
  
  ContToPeakMap contToPeaks;
  for( const PeakModel::PeakShrdPtr &thispeak : *allOrigPeaks )
  {
    if( thispeak )
      contToPeaks[thispeak->continuum()].push_back( *thispeak );
  }
  
  std::vector<PeakDef> origRoiPeaks = contToPeaks[peak->continuum()];
  
  //Make sure we dont try to mix data defined and gaussian defined peaks
  for( const PeakDef &p : origRoiPeaks )
  {
    if( !p.gausPeak() )
    {
      passMessage( "Sorry, cant add peaks to a ROI with a data defined peak.",
                  "", WarningWidget::WarningMsgInfo );
      return;
    }
  }//for( const PeakDef &p : origRoiPeaks )
  
  if( origRoiPeaks.empty() )
    throw runtime_error( "Logic error in InterSpec::addPeakFromRightClick()" );
  
  const double x0 = peak->lowerX();
  const double x1 = peak->upperX();
  const int lowbin = dataH->FindFixBin(x0);
  const int highbin = dataH->FindFixBin(x1);
  const int nbin = highbin - lowbin;
  
  double startingChi2, fitChi2;
  
  {//begin codeblock to evaluate startingChi2
    MultiPeakFitChi2Fcn chi2fcn( origRoiPeaks.size(), dataH,
                                peak->continuum()->type(),
                                lowbin, highbin );
    startingChi2 = chi2fcn.evalRelBinRange( 0, chi2fcn.nbin(), origRoiPeaks );
  }//end codeblock to evaluate startingChi2
  
  
  bool inserted = false;
  vector< std::shared_ptr<PeakDef> > answer;
  for( PeakDef p : origRoiPeaks )
  {
    if( fabs(p.mean() - peak->mean()) < 0.01 )
    {
      inserted = true;
      PeakDef newpeak = p;
      newpeak.setMean( m_rightClickEnergy );
      newpeak.setSigma( newpeak.sigma() / sqrt(2.0) );
      newpeak.setAmplitude( 0.25*p.amplitude() );
      p.setAmplitude( 0.75*p.amplitude() );
      p.setSigma( p.sigma() / sqrt(2.0) );
      
      if( newpeak.mean() < p.mean() )
      {
        answer.push_back( std::make_shared<PeakDef>( newpeak ) );
        answer.push_back( std::make_shared<PeakDef>( p ) );
      }else
      {
        answer.push_back( std::shared_ptr<PeakDef>( new PeakDef(p) ) );
        answer.push_back( std::shared_ptr<PeakDef>( new PeakDef(newpeak) ) );
      }//if( newpeak.mean() < p.mean() ) / else
    }else
    {
//      p.setFitFor(PeakDef::Mean, false);
//      p.setFitFor(PeakDef::Sigma, false);
//      p.setFitFor(PeakDef::GaussAmplitude, false);
      answer.push_back( std::make_shared<PeakDef>( p ) );
    }
  }//for( PeakDef p : origRoiPeaks )
  
  if( !inserted )
    throw runtime_error( "Logic error 2 in InterSpec::addPeakFromRightClick()" );

  
  const MultiPeakInitialGuesMethod methods[] = { FromInputPeaks, UniformInitialGuess, FromDataInitialGuess };
  
  for( auto method : methods )
  {
    vector< std::shared_ptr<PeakDef> > orig_answer;
    for( auto p : answer )
      orig_answer.push_back( make_shared<PeakDef>( *p ) );
    
    findPeaksInUserRange( x0, x1, int(answer.size()), method, dataH,
                        m_dataMeasurement->detector(), answer, fitChi2 );
  
    std::vector<PeakDef> newRoiPeaks;
    for( size_t i = 0; i < answer.size(); ++i )
      newRoiPeaks.push_back( *answer[i] );
    
    MultiPeakFitChi2Fcn chi2fcn( newRoiPeaks.size(), dataH,
                                 peak->continuum()->type(),
                                 lowbin, highbin );
    fitChi2 = chi2fcn.evalRelBinRange( 0, chi2fcn.nbin(), newRoiPeaks );
    
    if( fitChi2 < startingChi2 )
    {
      //cout << "Method " << method << " gave fitChi2=" << fitChi2 << ", which is better than initial startingChi2=" << startingChi2 << endl;
      break;
    }
    
    answer = orig_answer;
  }//for( auto method : methods )
  
//  could try to fix all peaks other than the new one, and the nearest one, do the
//  fit, then replace the other peaks to original fitFor state, and refit all of
//  them.
  
  const double dof = (nbin + 3*origRoiPeaks.size() + peak->continuum()->type());
  const double chi2Dof = fitChi2 / dof;
  
  cerr << "m_rightClickEnergy=" << m_rightClickEnergy << endl;
  cerr << "PreChi2=" << startingChi2 << ", PostChi2=" << fitChi2 << endl;
  cerr << "Got chi2Dof=" << chi2Dof << " when fitting for new peak" << endl;
  
  for( size_t i = 0; i < answer.size(); ++i )
  {
    cerr << "Peak " << i << " at " << answer[i]->mean() << " w/ width="
         << answer[i]->sigma() << ", and amp=" << answer[i]->amplitude() << endl;
  }
  
  //Remove old ROI peaks
  if( fitChi2 > startingChi2 )
  {
    passMessage( "Adding a peak did not improve overall ROI fit - not adding one.",
                "", WarningWidget::WarningMsgInfo );
    return;
  }
  
  
  std::map<std::shared_ptr<PeakDef>,PeakModel::PeakShrdPtr> new_to_orig_peaks;
  for( const PeakModel::PeakShrdPtr &thispeak : *allOrigPeaks )
  {
    if( !thispeak || (thispeak->continuum() != origContinumm) )
      continue;
    
    m_peakModel->removePeak( thispeak );
    
    //Associate new peak 
    double dist = 99999.9;
    std::shared_ptr<PeakDef> nearest_new;
    for( std::shared_ptr<PeakDef> newpeak : answer )
    {
      const double d = fabs( newpeak->mean() - thispeak->mean() );
      if( d < dist && !new_to_orig_peaks.count(newpeak) )
      {
        dist = d;
        nearest_new = newpeak;
      }
    }//
    if( nearest_new )
    {
      nearest_new->inheritUserSelectedOptions( *thispeak, true );
      new_to_orig_peaks[nearest_new] = thispeak;
    }
  }//for( const PeakModel::PeakShrdPtr &thispeak : *allOrigPeaks )
  
  
  for( size_t i = 0; i < answer.size(); ++i )
  {
    const bool isNew = !new_to_orig_peaks.count(answer[i]);
    InterSpec::addPeak( *answer[i], isNew );
  }
  
#if ( USE_SPECTRUM_CHART_D3 )
  m_spectrum->updateData();
#endif
}//void addPeakFromRightClick()


void InterSpec::makePeakFromRightClickHaveOwnContinuum()
{
  const std::shared_ptr<const Measurement> data = m_spectrum->data();
  std::shared_ptr<const PeakDef> peak = nearestPeak( m_rightClickEnergy );
  if( !peak || !data
      || m_rightClickEnergy < peak->lowerX()
      || m_rightClickEnergy > peak->upperX() )
  {
    passMessage( "There was no ROI to add peak to",
                "", WarningWidget::WarningMsgInfo );
    return;
  }//if( !peak )
  
  std::shared_ptr<const PeakContinuum> oldcont = peak->continuum();
  
  PeakDef newpeak(*peak);
  std::shared_ptr<PeakContinuum> cont
                                = std::make_shared<PeakContinuum>( *oldcont );
  newpeak.setContinuum( cont );
  
  cont->setRange( -1.0, -1.0 );
  size_t lowbin = findROILimit( newpeak, data, false );
  size_t upbin  = findROILimit( newpeak, data, true );
  double minx = data->gamma_channel_lower( lowbin );
  double maxx = data->gamma_channel_upper( upbin );
  cont->setRange( minx, maxx );
  
  m_peakModel->removePeak( peak );
  addPeak( newpeak, true );
#if ( USE_SPECTRUM_CHART_D3 )
  m_spectrum->updateData();
#endif
  
  refitPeakFromRightClick();
  
  //Now we gotta refit the peaks that have the continuum we took this peak from
  std::shared_ptr<const std::deque< PeakModel::PeakShrdPtr > > peaks;
  peaks = m_peakModel->peaks();
  if( !peaks )
    return;
  
  //const cast hack!  (I *think* its okay since the peak is refit anyway)
  std::const_pointer_cast<PeakContinuum>(oldcont)->setRange( -1.0, -1.0 );
  
  minx = std::numeric_limits<double>::infinity();
  maxx = -std::numeric_limits<double>::infinity();
  vector<PeakModel::PeakShrdPtr > oldneighbors;
  for( deque< PeakModel::PeakShrdPtr >::const_iterator iter = peaks->begin();
       iter != peaks->end(); ++iter )
  {
    if( (*iter)->continuum() == oldcont )
    {
      oldneighbors.push_back( *iter );
      lowbin = findROILimit( *(*iter), data, false );
      upbin  = findROILimit( *(*iter), data, true );
      const double a = data->gamma_channel_lower( lowbin );
      const double z = data->gamma_channel_upper( upbin );
      minx = std::min( minx, a );
      maxx = std::max( maxx, z );
    }//if( (*iter)->continuum() == oldcont )
  }//for( iterate over peaks )
  
  if( oldneighbors.empty() )
    return;
  
  //const cast hack!  (I *think* its okay since the peak is refit anyway)
  std::const_pointer_cast<PeakContinuum>(oldcont)->setRange( minx, maxx );
  const double oldenergy = m_rightClickEnergy;
  m_rightClickEnergy = oldneighbors[0]->mean();
  refitPeakFromRightClick();
  m_rightClickEnergy = oldenergy;
}//void makePeakFromRightClickHaveOwnContinuum()


void InterSpec::shareContinuumWithNeighboringPeak( const bool shareWithLeft )
{
  std::shared_ptr<const PeakDef> peak = nearestPeak( m_rightClickEnergy );
  if( !peak
     || m_rightClickEnergy < peak->lowerX()
     || m_rightClickEnergy > peak->upperX() )
  {
    passMessage( "There was no ROI to add peak to",
                "", WarningWidget::WarningMsgInfo );
    return;
  }//if( !peak )
  
  std::shared_ptr<const std::deque< PeakModel::PeakShrdPtr > > peaks;
  peaks = m_peakModel->peaks();
  
  if( !peaks )
    return;
  
  std::shared_ptr<const PeakDef> peaktoshare;
  deque<PeakModel::PeakShrdPtr >::const_iterator iter;
  
//  boost::function<bool(const PeakModel::PeakShrdPtr &, const PeakModel::PeakShrdPtr &)> meansort;
//  meansort = boost::bind( &PeakModel::compare, _1, _2, PeakModel::kMean, Wt::AscendingOrder );
//  iter = lower_bound( peaks->begin(), peaks->end(), peak, meansort );
  iter = std::find( peaks->begin(), peaks->end(), peak );
  
  if( iter==peaks->end() || (*iter)!=peak )
    throw runtime_error( "InterSpec::shareContinuumWithNeighboringPeak: "
                         "error searching for peak I should have found" );
  
  if( shareWithLeft )
  {
    if( iter == peaks->begin() )
      return;
    peaktoshare = *(iter-1);
  }else
  {
    if( (iter+1) == peaks->end() )
      return;
    peaktoshare = *(iter+1);
  }//if( shareWithLeft ) / else
  
  if( peak->continuum() == peaktoshare->continuum() )
    return;
  
  if( !peaktoshare->continuum()->isPolynomial() || !peaktoshare->gausPeak() )
    return;
  
  vector<PeakModel::PeakShrdPtr> leftpeaks, rightpeaks;
  vector<PeakModel::PeakShrdPtr> &pp = (shareWithLeft ? rightpeaks : leftpeaks);
  vector<PeakModel::PeakShrdPtr> &op = (shareWithLeft ? leftpeaks : rightpeaks);
  
  for( deque< PeakModel::PeakShrdPtr >::const_iterator iter = peaks->begin();
       iter != peaks->end(); ++iter )
  {
    if( (*iter)->continuum() == peak->continuum() )
      pp.push_back( *iter );
    else if( (*iter)->continuum() == peaktoshare->continuum() )
      op.push_back( *iter );
  }//for( iterate over peaks )
  
  
  std::shared_ptr<const PeakContinuum> oldcontinuum
                           = peaktoshare->continuum();
  std::shared_ptr<PeakContinuum> continuum
                           = std::make_shared<PeakContinuum>( *oldcontinuum );
  
  const double minx = min( leftpeaks[0]->lowerX(), rightpeaks[0]->lowerX() );
  const double maxx = max( leftpeaks[0]->upperX(), rightpeaks[0]->upperX() );
  continuum->setRange( minx, maxx );
  
  //Make sure the order polynomial is at least the minimum of either side
  if( leftpeaks[0]->continuum()->isPolynomial()
      && continuum->type() < leftpeaks[0]->continuum()->type() )
    continuum->setType( leftpeaks[0]->continuum()->type() );
  
  if( rightpeaks[0]->continuum()->isPolynomial()
     && continuum->type() < rightpeaks[0]->continuum()->type() )
    continuum->setType( rightpeaks[0]->continuum()->type() );
  
  
  for( PeakModel::PeakShrdPtr &p : leftpeaks )
  {
    PeakDef newpeak( *p );
    newpeak.setContinuum( continuum );
    m_peakModel->removePeak( p );
    addPeak( newpeak, false );
  }//for( PeakModel::PeakShrdPtr &p : leftpeaks )
  
  for( PeakModel::PeakShrdPtr &p : rightpeaks )
  {
    PeakDef newpeak( *p );
    newpeak.setContinuum( continuum );
    m_peakModel->removePeak( p );
    addPeak( newpeak, false );
  }//for( PeakModel::PeakShrdPtr &p : leftpeaks )
  
#if ( USE_SPECTRUM_CHART_D3 )
  m_spectrum->updateData();
#endif

//Instead of the more computationally expensive stuff (lots of signals get
//  emmitted, tables re-rendered, and charts re-drawn - although hopefully
//  mostly lazili) above, you could do the following not very nice const casts
//  std::shared_ptr<PeakContinuum> continuum;
//  continuum = boost::const_pointer_cast<PeakContinuum>( peaktoshare->continuum() );
//  if( shareWithLeft )
//    continuum->setRange( peaktoshare->lowerX(), peak->upperX() );
//  else
//    continuum->setRange( peak->lowerX(), peaktoshare->upperX() );
//  boost::const_pointer_cast<PeakDef>(peak)->setContinuum( continuum );

  refitPeakFromRightClick();
}//void shareContinuumWithNeighboringPeak( const bool shareWithLeft )


void InterSpec::deletePeakFromRightClick()
{
  std::shared_ptr<const PeakDef> peak = nearestPeak( m_rightClickEnergy );
  if( !peak )
  {
    passMessage( "There was no peak to delete", "", WarningWidget::WarningMsgInfo );
    return;
  }

  const int npeak = static_cast<int>(m_peakModel->npeaks());
  for( int peakn = 0; peakn < npeak; ++peakn )
  {
    WModelIndex index = m_peakModel->index( peakn, 0 );
    std::shared_ptr<const PeakDef> thispeak = m_peakModel->peak(index);
    if( thispeak == peak )
    {
      m_peakModel->removePeak( index );
#if ( USE_SPECTRUM_CHART_D3 )
      m_spectrum->updateData();
#endif
      return;
    }
  }//for( int peak = 0; peak < npeak; ++peak )
}//void deletePeakFromRightClick()




void InterSpec::setPeakNuclide( const std::shared_ptr<const PeakDef> peak,
                                     std::string nuclide )
{
  //TODO: should probably add in some error logging or something
  const SandiaDecay::SandiaDecayDataBase *db = DecayDataBaseServer::database();
  
  if( !peak || nuclide.empty() || !db )
  {
    cerr << "InterSpec::setPeakNuclide(): invalid input" << endl;
    return;
  }//if( !peak || nuclide.empty() || !db )
  
  
//  enum SetGammaSource{ NoSourceChange, SourceChange, SourceAndUseChanged };
//  static SetGammaSource setNuclideXrayReaction( PeakDef &peak, std::string txt,
//                                               const double nsigma_window );
  
//  const SandiaDecay::Nuclide *nuc = db->nuclide( nuclide );
//  if( !nuc )
//    return;
  
  WModelIndex index = m_peakModel->indexOfPeak( peak );
  if( !index.isValid() )
  {
    cerr << "InterSpec::setPeakNuclide(): couldnt find index for peak"
         << endl;
    return;
  }//if( !index.isValid() )
  
  index = m_peakModel->index( index.row(), PeakModel::kIsotope );
  
  m_peakModel->setData( index, boost::any(WString(nuclide)) );
}//void setPeakNuclide(...)


void InterSpec::updateRightClickNuclidesMenu(
                                  const std::shared_ptr<const PeakDef> peak,
                                  std::shared_ptr< std::vector<std::string> > nuclides )
{
  //TODO: should probably add in some error logging or something
  PopupDivMenu *menu = m_rightClickNuclideSuggestMenu;
  
  if( !peak || !nuclides || m_rightClickEnergy < 0.0 ||!menu )
    return;
  
  for( WMenuItem *item : menu->items() )
  {
    if( !item->hasStyleClass("PhoneMenuBack")
        && !item->hasStyleClass("PhoneMenuClose") )
    delete item;
  }//for( WMenuItem *item : menu->items() )

  
  for( size_t i = 0; i < nuclides->size(); ++i )
  {
    const std::string &nuc = (*nuclides)[i];
    
    if( i==0 && nuc.size()
        && (peak->parentNuclide() || peak->reaction() || peak->xrayElement())  )
    {
      PopupDivMenuItem *item = menu->addMenuItem( nuc, "", true );
      item->setAttributeValue("style", "background: grey; color: white;"
                                       + item->attributeValue("style"));
    }else if( nuc.size() )
    {
      PopupDivMenuItem *item = menu->addMenuItem( nuc, "", true );
      
      if( nuc[0]=='(' && nuc[nuc.size()-1]==')' )
        item->disable();
      else
        item->triggered().connect( boost::bind( &InterSpec::setPeakNuclide, this, peak, nuc ) );
    }else if( i != (nuclides->size()-1) )
    {
      menu->addSeparator();
    }
  }//for( size_t i = 0; i < nuclides->size(); ++i )
  
  //Adjust the menu so its all visible, since it may be larger now, but first
  //  check to make sure the menu is still visible.  We have to do this client
  //  side since the server is unaware whats hidden/visible.
  WMenuItem *parentItem = menu->parentItem();
  WMenu *parentMenu = (parentItem ? parentItem->parentMenu() : (WMenu *)0);
  if( !isMobile() && parentMenu )
  {
    //The positioning margin is copied from WPopupMenu.js and could change in
    //  future versions of Wt (current 3.3.4).  The AdjustTopPos(...) function
    //  is from PopupDiv.cpp, and makes sure Wts positioning is reasonable.
    doJavaScript(  "setTimeout(function(){try{"
                     "var u=" + menu->jsRef() + ", p=" + parentItem->jsRef() + ";"
                     "if(u.style.display==='block'){"
                       "var m=Wt.WT.px(u,'paddingTop')+Wt.WT.px(u,'borderTopWidth');"
                       "Wt.WT.positionAtWidget(u.id,p.id, Wt.WT.Horizontal,-m);"
                       "Wt.WT.AdjustTopPos(u.id);"
                     "}"
                   "}catch(e){console.log('Failed re-positioning menu');}},0);" );
  }//if( !isMobile() && parentMenu )
  
  if( nuclides->empty() )
  {
    PopupDivMenuItem *item = menu->addMenuItem( "No Suggestions", "", true );
    item->setAttributeValue("style", "background: grey; color: white;"
                            + item->attributeValue("style"));
  }
  
  WApplication *app = wApp;
  if( app )
    app->triggerUpdate();
}//updateRightClickNuclidesMenu()


void InterSpec::handleLeftClick( double energy, double counts,
                                      int pageX, int pageY )
{
  if( (m_toolsTabs && m_currentToolsTab == m_toolsTabs->indexOf(m_isotopeSearchContainer))
      || m_nuclideSearchWindow )
  {
    setIsotopeSearchEnergy( energy );
    return;
  }
  
#if( !USE_SIMPLE_NUCLIDE_ASSIST && USE_TERMINAL_WIDGET )
  if( m_toolsTabs && m_terminal
      && (m_toolsTabs->currentIndex() == m_toolsTabs->indexOf(m_terminal)) )
  {
    m_terminal->chartClicked(energy,counts,pageX,pageY);
  }
#endif

  
#if( USE_SIMPLE_NUCLIDE_ASSIST )
  if( m_leftClickMenu )
    delete m_leftClickMenu;
  
  m_leftClickMenu = new SimpleNuclideAssistPopup( energy, this, pageX, pageY ); //PopupDivMenu( 0, PopupDivMenu::TransientMenu );
  if( m_leftClickMenu->isValid() )
  {
    m_leftClickMenu->aboutToHide().connect( this, &InterSpec::leftClickMenuClosed );
  }else
  {
    delete m_leftClickMenu;
    m_leftClickMenu = 0;
  }
#endif //#if( USE_SIMPLE_NUCLIDE_ASSIST )
  
}//void handleLeftClick(...)




void InterSpec::handleRightClick( double energy, double counts,
                                       int pageX, int pageY )
{
  if( !m_dataMeasurement )
    return;
  
  const std::shared_ptr<const PeakDef> peak = nearestPeak( energy );
  m_rightClickEnergy = energy;
  
  
  typedef std::shared_ptr<const std::deque< PeakModel::PeakShrdPtr > > PeakContainer_t;
  PeakContainer_t peaks = m_peakModel->peaks();
  
  if( !peaks || !peak )
    return;
  
  //We actually need to make a copy of peaks since we will post to outside the
  //  main thread where the deque is expected to remain constant, however, the
  //  PeakModel deque may get changed by like the user deleting a peak.
  peaks = make_shared< std::deque< PeakModel::PeakShrdPtr > >( begin(*peaks), end(*peaks) );
  
  
  //see how many other peaks share ROI
  size_t npeaksInRoi = 0;
  for( const PeakModel::PeakShrdPtr &p : *peaks )
    npeaksInRoi += (p->continuum() == peak->continuum());
  
  std::deque< PeakModel::PeakShrdPtr >::const_iterator iter;
  
  for( RightClickItems i = RightClickItems(0);
      i < kNumRightClickItems; i = RightClickItems(i+1) )
  {
    switch( i )
    {
      case kPeakEdit: case kDeletePeak: case kAddPeak:
//        m_rightClickMenutItems[i]->setHidden( !peak );
      break;
        
      case kChangeNuclide:
      {
        if( m_rightClickNuclideSuggestMenu )
        {
          for( WMenuItem *item : m_rightClickNuclideSuggestMenu->items() )
          {
            if( !item->hasStyleClass("PhoneMenuBack")
                && !item->hasStyleClass("PhoneMenuClose") )
              delete item;
          }//for( WMenuItem *item : m_rightClickNuclideSuggestMenu->items() )
          
          m_rightClickNuclideSuggestMenu->addMenuItem( "...calculating...",
                                                       "", false );
          
          Wt::WServer *server = Wt::WServer::instance();
          if( server )  //this should always be true
          {
            Wt::WIOService &io = server->ioService();
            std::shared_ptr< vector<string> > candidates
                                       = std::make_shared<vector<string> >();
            boost::function<void(void)> updater = wApp->bind(
                       boost::bind(&InterSpec::updateRightClickNuclidesMenu,
                                   this, peak, candidates ) );
            
            std::shared_ptr<const Measurement> hist = displayedHistogram( kForeground );
            std::shared_ptr<const SpecMeas> meas = measurment( kForeground );
            std::shared_ptr<const DetectorPeakResponse> detector;
            if( !!meas )
              detector = meas->detector();
        
            PeakContainer_t hintpeaks;
            if( m_dataMeasurement )
              hintpeaks = m_dataMeasurement->automatedSearchPeaks( m_displayedSamples );
//            if( !hintpeaks || (!!peaks && hintpeaks->size() <= peaks->size()) )
//              hintpeaks = peaks;
            
            //Make a copy of hintpeaks deque since we will be posting to outside
            //  the main thread, where the original deque could actually get
            //  modified by another call, but what we're posting to expected it
            //  to remain constant.
            if( hintpeaks )
              hintpeaks = make_shared< std::deque< PeakModel::PeakShrdPtr > >( begin(*hintpeaks), end(*hintpeaks) );
            
            vector<ReferenceLineInfo> refLines;
            if( m_referencePhotopeakLines )
              refLines = m_referencePhotopeakLines->showingNuclides();
            
            const string session_id = wApp->sessionId();
            
            boost::function<void(void)> worker = [=](){
              IsotopeId::populateCandidateNuclides( hist, peak, hintpeaks,
                      peaks, refLines, detector, session_id, candidates, updater );
            };
            
            io.post( worker );
          }//if( server )
        }//if( menu )
        else
          cerr << "Serious error getting WMenu" << endl;
        
        break;
      }//case kChangeNuclide:
        
      case kRefitPeak:
        m_rightClickMenutItems[i]->setHidden( !peak->gausPeak() || npeaksInRoi>1 );
      break;
        
      case kRefitROI:
        m_rightClickMenutItems[i]->setHidden( !peak->gausPeak() || npeaksInRoi<2 );
      break;
        
      case kShareContinuumWithLeftPeak:
      {
        iter = std::find( peaks->begin(), peaks->end(), peak );
        if( iter == peaks->begin() )
        {
          m_rightClickMenutItems[i]->setHidden( true );
          break;
        }//if( iter == peaks.begin() )
        
        if( iter==peaks->end() || (*iter)!=peak )
          throw runtime_error( "InterSpec::handleRightClick: "
                               "error searching for peak I should have found 1" );
        
        PeakModel::PeakShrdPtr leftpeak = *(iter - 1);
        if( leftpeak->continuum() == peak->continuum()
            || !leftpeak->continuum()->isPolynomial() )
        {
          m_rightClickMenutItems[i]->setHidden( true );
          break;
        }//if( it alread shares a continuum with the left peak )
        
        const double leftupper = leftpeak->upperX();
        const double rightlower = peak->lowerX();
        const double rightupper = peak->upperX();
        
        //The below 2.0 is arbitrary
        const bool show = ((rightlower-leftupper) < 2.0*(rightupper-rightlower));
        m_rightClickMenutItems[i]->setHidden( !show );
        break;
      }//case kShareContinuumWithLeftPeak:
        
        
      case kShareContinuumWithRightPeak:
      {
        iter = std::find( peaks->begin(), peaks->end(), peak );
        
        if( iter==peaks->end() || (*iter)!=peak )
          throw runtime_error( "InterSpec::handleRightClick: "
                               "error searching for peak I should have found 2" );

        if( (iter+1) == peaks->end() )
        {
          m_rightClickMenutItems[i]->setHidden( true );
          break;
        }//if( iter == peaks.begin() )
        
        
        PeakModel::PeakShrdPtr rightpeak = *(iter + 1);
        if( rightpeak->continuum() == peak->continuum()
            || !rightpeak->continuum()->isPolynomial() )
        {
          m_rightClickMenutItems[i]->setHidden( true );
          break;
        }//if( it alread shares a continuum with the left peak )
        
        const double rightlower = rightpeak->lowerX();
        const double leftlower = peak->lowerX();
        const double leftupper = peak->upperX();
        
        //The below 2.0 is arbitrary
        const bool show = ((rightlower-leftupper) < 2.0*(leftupper-leftlower));
        m_rightClickMenutItems[i]->setHidden( !show );
        break;
      }//case kShareContinuumWithRightPeak:
      
      
      case kMakeOwnContinuum:
      {
        bool shares = false;
        std::shared_ptr<const PeakContinuum> cont = peak->continuum();
        for( PeakModel::PeakShrdPtr p : *peaks )
          shares = (shares || (p!=peak && p->continuum()==cont) );
        m_rightClickMenutItems[i]->setHidden( !shares );
        break;
      }//kMakeOwnContinuum
        
      case kNumRightClickItems:
      break;
    }//switch( i )
  }//for( loop over right click menu items )
  
  if( isMobile() )
    m_rightClickMenu->showFromMouseOver();
  else
    m_rightClickMenu->popup( WPoint(pageX,pageY) );
}//void handleRightClick(...)


void InterSpec::createPeakEdit( double energy )
{
  if( m_peakEditWindow )
  {
    m_peakEditWindow->peakEditor()->changePeak( energy );
  }else
  {
    m_peakEditWindow = new PeakEditWindow( energy, m_peakModel, this );
    m_peakEditWindow->editingDone().connect( this, &InterSpec::deletePeakEdit );
    m_peakEditWindow->finished().connect( this, &InterSpec::deletePeakEdit );
    m_peakEditWindow->resizeToFitOnScreen();
    
    PeakEdit *editor = m_peakEditWindow->peakEditor();
    if( !editor->isEditingValidPeak() )
    {
      delete m_peakEditWindow;
      m_peakEditWindow = 0;
    }//if( !editor->isEditingValidPeak() )
  }//if( m_peakEditWindow ) / else
  

}//void createPeakEdit( Wt::WMouseEvent event )


void InterSpec::deletePeakEdit()
{
  if( m_peakEditWindow )
  {
    delete m_peakEditWindow;
    m_peakEditWindow = NULL;
  }
}//void deletePeakEdit()



void InterSpec::setIsotopeSearchEnergy( double energy )
{
  if( !m_isotopeSearch )
    return;
  
  double sigma = -1.0;

  if( m_toolsTabs )
  {
    if( m_currentToolsTab != m_toolsTabs->indexOf(m_isotopeSearchContainer) )
      return;
  }else if( !m_nuclideSearchWindow )
  {
    return;
  }
  
  //check to see if this is within 3.0 sigma of a peak, and if so, set the
  //  energy to the mean of that peak
  PeakModel::PeakShrdPtr peak = m_peakModel->nearestPeak( energy );
  
  if( !!peak )
  {
    const double width = peak->gausPeak() ? peak->fwhm() : 0.5*peak->roiWidth();
    if( (fabs(peak->mean()-energy) < (3.0/2.634)*width) )
    {
      energy = peak->mean();
      sigma = width;
      
      //For low res spectra make relatively less wide
      if( !!m_dataMeasurement && m_dataMeasurement->num_gamma_channels()<4100 )
        sigma *= 0.35;
    }//if( within 3 sigma of peak )
  }//if( !!peak )
  
  m_isotopeSearch->setNextSearchEnergy( energy, sigma );
}//void setIsotopeSearchEnergy( double energy );

void InterSpec::setFeatureMarkerOption( InterSpec::FeatureMarkerType option, bool show )
{
  m_featureMarkersShown[option] = show;
}

Wt::Signal<SpectrumType,std::shared_ptr<SpecMeas>, std::set<int> > &
                                      InterSpec::displayedSpectrumChanged()
{
  return m_displayedSpectrumChangedSignal;
}



WModelIndex InterSpec::addPeak( PeakDef peak,
                                    const bool associateShowingNuclideXrayRctn )
{
  if( fabs(peak.mean())<0.1 && fabs(peak.amplitude())<0.1 )
    return WModelIndex();
  
  if( !m_referencePhotopeakLines || !associateShowingNuclideXrayRctn ) {
#if ( USE_SPECTRUM_CHART_D3 )
    WModelIndex index = m_peakModel->addNewPeak( peak );
    m_spectrum->updateData();
    return index;
#else
    return m_peakModel->addNewPeak( peak );
#endif
  }
  
  if( peak.parentNuclide() || peak.xrayElement() || peak.reaction() ) {
#if ( USE_SPECTRUM_CHART_D3 )
    WModelIndex index = m_peakModel->addNewPeak( peak );
    m_spectrum->updateData();
    return index;
#else
    return m_peakModel->addNewPeak( peak );
#endif
  }
  
  auto foreground = displayedHistogram(kForeground);
  PeakSearchGuiUtils::assign_nuclide_from_reference_lines( peak, m_peakModel, foreground, m_referencePhotopeakLines, m_colorPeaksBasedOnReferenceLines );
  
  WModelIndex newpeakindex = m_peakModel->addNewPeak( peak );
#if ( USE_SPECTRUM_CHART_D3 )
  m_spectrum->updateData();
#endif
  
  PeakModel::PeakShrdPtr newpeak = m_peakModel->peak(newpeakindex);
  try_update_hint_peak( newpeak, m_dataMeasurement, m_displayedSamples );
  
  return newpeakindex;
}//WModelIndex addPeak( PeakDef peak )



#if( IOS )
void InterSpec::exportSpecFile()
{
  //Need to:
  //  -Build gui to allow selecting foreground/background/secondary as well
  //   selecting which samples/detectors/etc is wanted; this can be made general
  //   for all operating systems or deployments.
  //  -Enable/disable "Export Spectrum" menu item as a spectrum is avaialble or not

  if( !m_dataMeasurement )
    return;
  
  hand_spectrum_to_other_app( m_dataMeasurement );
}//void exportSpecFile()
#endif



#if( USE_DB_TO_STORE_SPECTRA )
void InterSpec::saveStateToDb( Wt::Dbo::ptr<UserState> entry )
{
  if( !entry || entry->user != m_user || !m_user.session() )
    throw runtime_error( "Invalid input to saveStateToDb()" );
  
  if( entry->isWriteProtected() )
    throw runtime_error( "UserState is write protected; "
                         "please clone state to re-save" );
  
//  if( !m_user->preferenceValue<bool>("SaveSpectraToDb") )
//    throw runtime_error( "You must enable saving spectra to database inorder to"
//                         " save the apps state." );

  try
  {
    saveShieldingSourceModelToForegroundSpecMeas();
    
    DataBaseUtils::DbTransaction transaction( *m_sql );
    entry.modify()->serializeTime = WDateTime::currentDateTime();
  
    if( !entry->creationTime.isValid() )
      entry.modify()->creationTime = entry->serializeTime;
    
    const bool forTesting = (entry->stateType == UserState::kForTest);
    
    std::shared_ptr<const SpecMeas> foreground = measurment( kForeground );
    std::shared_ptr<const SpecMeas> second = measurment( kSecondForeground );
    std::shared_ptr<const SpecMeas> background = measurment( kBackground );
    
    Dbo::ptr<UserFileInDb> dbforeground = measurmentFromDb( kForeground, true );
    Dbo::ptr<UserFileInDb> dbsecond     = measurmentFromDb( kSecondForeground, true );
    Dbo::ptr<UserFileInDb> dbbackground = measurmentFromDb( kBackground, true );
  
    //JIC, make sure indices have all been assigned to everything.
    m_sql->session()->flush();
    
    if( (entry->snapshotTagParent || forTesting)
        || (dbforeground && dbforeground->isWriteProtected()) )
    {
      dbforeground = UserFileInDb::makeDeepWriteProtectedCopyInDatabase( dbforeground, true );
      if( dbforeground && !(entry->snapshotTagParent || forTesting) )
        UserFileInDb::removeWriteProtection( dbforeground );
    }
    
    if( (entry->snapshotTagParent || forTesting)
         && dbsecond
         && !dbsecond->isWriteProtected()
         && (foreground != second) )
    {
      dbsecond = UserFileInDb::makeDeepWriteProtectedCopyInDatabase( dbsecond, true );
      if( dbsecond && !(entry->snapshotTagParent || forTesting) )
        UserFileInDb::removeWriteProtection( dbsecond );
    }else if( foreground == second )
      dbsecond = dbforeground;
    
    if( (entry->snapshotTagParent || forTesting)
        && dbbackground
        && !dbbackground->isWriteProtected()
        && (background != foreground)
        && (background != second))
    {
      dbbackground = UserFileInDb::makeDeepWriteProtectedCopyInDatabase( dbbackground, true );
      if( dbbackground && !(entry->snapshotTagParent || forTesting) )
        UserFileInDb::removeWriteProtection( dbbackground );
    }else if( background == foreground )
      dbbackground = dbforeground;
    else if( background == second )
      dbbackground = dbsecond;
    
    if( !dbforeground && foreground )
      throw runtime_error( "Error saving foreground to the database" );
    if( !dbsecond && second )
      throw runtime_error( "Error saving second foreground to the database" );
    if( !dbbackground && background )
      throw runtime_error( "Error saving background to the database" );
    
    entry.modify()->snapshotTagParent = entry->snapshotTagParent;
    entry.modify()->snapshotTags = entry->snapshotTags;
    
    //We need to make sure dbforeground, dbbackground, dbsecond will have been
    //  written to the database, so there id()'s will be not -1.
    m_sql->session()->flush();
    
    entry.modify()->foregroundId = dbforeground.id();
    entry.modify()->backgroundId = dbbackground.id();
    entry.modify()->secondForegroundId = dbsecond.id();
    
    /*
    entry.modify()->shieldSourceModelId = -1;
    if( m_shieldingSourceFit )
    {
      Wt::Dbo::ptr<ShieldingSourceModel> model = m_shieldingSourceFit->modelInDb();
      
      //make sure model has actually been written to the DB, so index wont be 0
      if( model )
        m_sql->session()->flush();
        
      entry.modify()->shieldSourceModelId = model.id();
    }
     */
    
//    entry.modify()->otherSpectraCsvIds;
    {
      string &str = (entry.modify()->foregroundSampleNumsCsvIds);
      str.clear();
      for( const int i : m_displayedSamples )
        str += (str.empty() ? "" : ",") + std::to_string( i );
    }
    
    {
      string &str = (entry.modify()->backgroundSampleNumsCsvIds);
      str.clear();
      for( const int i : m_backgroundSampleNumbers )
        str += (str.empty() ? "" : ",") + std::to_string( i );
    }
    
    {
      string &str = (entry.modify()->secondForegroundSampleNumsCsvIds);
      str.clear();
      for( const int i : m_sectondForgroundSampleNumbers )
        str += (str.empty() ? "" : ",") + std::to_string( i );
    }
    
    {
      const vector<bool> dispDet = detectors_to_display();
      const vector<int>  detNums = foreground ? foreground->detector_numbers() : vector<int>();
      
      string &str = (entry.modify()->showingDetectorNumbersCsv);
      str.clear();
      for( size_t i = 0; i < dispDet.size(); ++i )
        if( dispDet[i] )
          str += (str.empty() ? "" : ",") + std::to_string( detNums[i] );
    }
    
    entry.modify()->energyAxisMinimum = m_spectrum->xAxisMinimum();
    entry.modify()->energyAxisMaximum = m_spectrum->xAxisMaximum();
    entry.modify()->countsAxisMinimum = m_spectrum->yAxisMinimum();
    entry.modify()->countsAxisMaximum = m_spectrum->yAxisMaximum();
#if( USE_SPECTRUM_CHART_D3 )
    entry.modify()->displayBinFactor = 0;
#else
    entry.modify()->displayBinFactor = m_spectrum->displayRebinFactor();
#endif
    
    
    
    entry.modify()->shownDisplayFeatures = 0x0;
    if( toolTabsVisible() )
      entry.modify()->shownDisplayFeatures |= UserState::kDockedWindows;
    if( m_spectrum->yAxisIsLog() )
      entry.modify()->shownDisplayFeatures |= UserState::kLogSpectrumCounts;
    if( m_user->preferenceValue<bool>( "ShowVerticalGridlines" ) )
      entry.modify()->shownDisplayFeatures |= UserState::kVerticalGridLines;
    if( m_user->preferenceValue<bool>( "ShowHorizontalGridlines" ) )
      entry.modify()->shownDisplayFeatures |= UserState::kHorizontalGridLines;
    if( m_spectrum->legendIsEnabled() )
    {
      cerr << "Legend enabled" << endl;
      entry.modify()->shownDisplayFeatures |= UserState::kSpectrumLegend;
    }else cerr << "Legend NOT enabled" << endl;
    
    if( m_shieldingSourceFit )
      entry.modify()->shownDisplayFeatures |= UserState::kShowingShieldSourceFit;
    
    if( m_timeSeries->legendIsEnabled() )
      entry.modify()->shownDisplayFeatures |= UserState::kTimeSeriesLegend;
    
    entry.modify()->backgroundSubMode = UserState::kNoSpectrumSubtract;
    if( m_spectrum->backgroundSubtract() )
      entry.modify()->backgroundSubMode = UserState::kBackgorundSubtract;
    
    entry.modify()->currentTab = UserState::kNoTabs;
    if( m_toolsTabs )
    {
      const WString &txt = m_toolsTabs->tabText( m_toolsTabs->currentIndex() );
      if( txt == PeakInfoTabTitle )
        entry.modify()->currentTab = UserState::kPeakInfo;
      else if( txt == GammaLinesTabTitle )
        entry.modify()->currentTab = UserState::kGammaLines;
      else if( txt == CalibrationTabTitle )
        entry.modify()->currentTab = UserState::kCalibration;
      else if( txt == NuclideSearchTabTitle )
        entry.modify()->currentTab = UserState::kIsotopeSearch;
      else if( txt == FileTabTitle )
        entry.modify()->currentTab = UserState::kFileTab;
    }//if( m_toolsTabs )
    
    entry.modify()->showingMarkers = 0x0;
    
    entry.modify()->disabledNotifications = 0x0;

    for( WarningWidget::WarningMsgLevel level = WarningWidget::WarningMsgLevel(0);
        level <= WarningWidget::WarningMsgHigh;
        level = WarningWidget::WarningMsgLevel(level+1) )
    {
      if( !m_warnings->active( level ) )
        entry.modify()->disabledNotifications |= (0x1<<level);
    }
    
    entry.modify()->showingPeakLabels = 0x0;
    for( SpectrumChart::PeakLabels label = SpectrumChart::PeakLabels(0);
         label < SpectrumChart::kNumPeakLabels;
         label = SpectrumChart::PeakLabels(label+1) )
    {
      if( m_spectrum->showingPeakLabel( label ) )
        entry.modify()->showingPeakLabels |= (0x1<<label);
    }

    entry.modify()->showingWindows = 0x0;
    //m_warningsWindow
    //m_peakInfoWindow
    //m_recalibratorWindow
    //m_shieldingSourceFitWindow
    //m_referencePhotopeakLinesWindow
//    m_nuclideSearchWindow
//    enum ShowingWindows
//    {
//      kEnergyCalibration = 0x1, DetectorEditSelect = 0x2 //etc..
//    };
  
    entry.modify()->isotopeSearchEnergiesXml.clear();
    if( m_isotopeSearch )
      m_isotopeSearch->serialize( entry.modify()->isotopeSearchEnergiesXml );
   
    entry.modify()->gammaLinesXml.clear();
    if( m_referencePhotopeakLines )
      m_referencePhotopeakLines->serialize( entry.modify()->gammaLinesXml );
    
  /*
   enum FeatureMarkers
   {
   kEscapePeaks = 0x1, kCompPeak = 0x2, kComptonEdge = 0x4, kSumPeak = 0x8
   };
   */

    Json::Array userOptions;
    const Wt::Dbo::collection< Wt::Dbo::ptr<UserOption> > &prefs
                                                     = m_user->m_dbPreferences;
    
    vector< Dbo::ptr<UserOption> > options;
    std::copy( prefs.begin(), prefs.end(), std::back_inserter(options) );
    
    for( vector< Dbo::ptr<UserOption> >::const_iterator iter = options.begin();
        iter != options.end(); ++iter )
    {
      Dbo::ptr<UserOption> option = *iter;
      Json::Value val( Json::ObjectType );
      Json::Object &obj = val;
      obj["type"]  = int(option->m_type);
      obj["name"]  = WString(option->m_name);
      
      switch( option->m_type )
      {
        case UserOption::String:
          obj["value"] = WString( option->m_value );
        break;
        
        case UserOption::Decimal:
          obj["value"] = std::stod( option->m_value );
          
        break;
        
        case UserOption::Integer:
          obj["value"] = std::stoi( option->m_value );
        break;
        
        case UserOption::Boolean:
          obj["value"] = (option->m_value== "true" || option->m_value=="1");
        break;
      }//switch( m_type )
      
      userOptions.push_back( val );
    }//for( loop over DB entries )
  
    entry.modify()->userOptionsJson = Json::serialize( userOptions, 0 );
    
    //The color theme, if anything other than default is being used.
    //  (I guess we could serialize the default, but whateer for now)
    try
    {
      Wt::Dbo::ptr<ColorThemeInfo> theme;
      const int colorThemIndex = m_user->preferenceValue<int>("ColorThemeIndex", this);
      if( colorThemIndex > 0 )
        theme = m_user->m_colorThemes.find().where( "id = ?" ).bind( colorThemIndex ).resultValue();
      if( theme )
        entry.modify()->colorThemeJson = theme->json_data;
    }catch(...)
    {
      //Probably shouldnt ever happen
      cerr << "saveStateToDb(): Caught exception retrieving color theme from database" << endl;
    }//try / catch
    
    if( forTesting || entry->snapshotTagParent )
      UserState::makeWriteProtected( entry, m_sql->session() );
    
    transaction.commit();
  }catch( std::exception &e )
  {
    cerr << "saveStateToDb(...) caught: " << e.what() << endl;
    throw runtime_error( "I'm sorry there was a technical error saving your"
                         " user state to the database." );
    //to do: handle errors
  }//try / catch
}//void saveStateToDb( Wt::Dbo::ptr<UserState> entry )


Dbo::ptr<UserState> InterSpec::serializeStateToDb( const Wt::WString &name,
                                                        const Wt::WString &desc,
                                                        const bool forTesting,
                                                        Dbo::ptr<UserState> parent)
{
  Dbo::ptr<UserState> answer;
  
  UserState *state = new UserState();
  state->user = m_user;
  state->stateType = forTesting ? UserState::kForTest : UserState::kUserSaved;
  state->creationTime = WDateTime::currentDateTime();
  state->name = name;
  state->description = desc;

  
  {//Begin interaction with database
    DataBaseUtils::DbTransaction transaction( *m_sql );
    if( parent )
    {
      //Making sure we get the main parent
      while( parent->snapshotTagParent )
        parent = parent->snapshotTagParent;
      state->snapshotTagParent = parent;
    }//parent
    
    answer = m_user.session()->add( state );
    transaction.commit();
  }//End interaction with database
  
  try
  {
    saveStateToDb( answer );
  }catch( std::exception &e )
  {
    passMessage( e.what(), "", WarningWidget::WarningMsgHigh );
  }
      
  if (parent) {
    WString msg = "Created new tag '";
    msg += name.toUTF8();
    msg += "' under snapshot '";
    msg += parent.get()->name.toUTF8();
    msg += "'.";
    passMessage( msg, "", WarningWidget::WarningMsgInfo );
  }else
  {
    WString msg = "Created new snapshot '";
    msg += name.toUTF8();
    msg += "'";
    passMessage( msg, "", WarningWidget::WarningMsgSave );
  }
    
  return answer;
}//Dbo::ptr<UserState> serializeStateToDb(...)



void InterSpec::loadStateFromDb( Wt::Dbo::ptr<UserState> entry )
{
  //This function takes about 5 ms (to load the previous HPGe state, on new app
  //  construction) on my 2.6 GHz Intel Core i7, as of (20150316).
  if( !entry )
    throw runtime_error( "UserState was invalid" );

  Wt::Dbo::ptr<UserState> parent = entry->snapshotTagParent;
  
  try
  {
    closeShieldingSourceFitWindow();
    
    switch( entry->stateType )
    {
      case UserState::kEndOfSessionTemp:
      case UserState::kUserSaved:
      case UserState::kForTest:
      case UserState::kUndefinedStateType:
      case UserState::kEndOfSessionHEAD:
      break;
    }//switch( entry->stateType )

    DataBaseUtils::DbTransaction transaction( *m_sql );
    
    if( parent )
    {
      while( parent->snapshotTagParent )
        parent = parent->snapshotTagParent;
    }

    
    Dbo::ptr<UserFileInDb> dbforeground, dbsecond, dbbackground;
    if( entry->foregroundId >= 0 )
      dbforeground = m_sql->session()->find<UserFileInDb>().where( "id = ?" )
                                      .bind( entry->foregroundId );
    if( entry->backgroundId >= 0
        && entry->backgroundId != entry->foregroundId )
      dbbackground = m_sql->session()->find<UserFileInDb>().where( "id = ?" )
                                      .bind( entry->backgroundId );
    else if( entry->backgroundId == entry->foregroundId )
      dbbackground = dbforeground;
      
    if( entry->secondForegroundId >= 0
        && entry->secondForegroundId != entry->foregroundId
        && entry->secondForegroundId != entry->backgroundId )
      dbsecond = m_sql->session()->find<UserFileInDb>().where( "id = ?" )
                                  .bind( entry->secondForegroundId );
    else if( entry->secondForegroundId == entry->foregroundId )
      dbsecond = dbforeground;
    else if( entry->secondForegroundId == entry->backgroundId )
      dbsecond = dbbackground;

    if( entry->foregroundId >= 0
        && (!dbforeground || !dbforeground->filedata.size()) )
      throw runtime_error( "Unable to locate foreground in database" );
    if( entry->backgroundId >= 0
        && (!dbbackground || !dbbackground->filedata.size()) )
      throw runtime_error( "Unable to locate background in database" );
    if( entry->secondForegroundId >= 0
        && (!dbsecond || !dbsecond->filedata.size()) )
      throw runtime_error( "Unable to locate second foreground in database" );
    
    //Essentially reset the state of the app
    m_fileManager->removeAllFiles();
    if( m_referencePhotopeakLines )
      m_referencePhotopeakLines->clearAllLines();
    
    deletePeakEdit();
    deleteGammaCountDialog();
    
    if( m_shieldingSourceFitWindow && !m_shieldingSourceFitWindow->isHidden() )
      m_shieldingSourceFitWindow->hide();
    
    bool wasDocked = (entry->shownDisplayFeatures & UserState::kDockedWindows);
    if( toolTabsVisible() != wasDocked )
      setToolTabsVisible( wasDocked );

    //Now start reloading the state
    set<int> foregroundNums, backgroundNums, secondNums, otherSamples;
    std::shared_ptr<SpecMeas> foreground, second, background;
    std::shared_ptr<SpecMeas> snapforeground, snapsecond, snapbackground;
    std::shared_ptr<SpectraFileHeader> foregroundheader, backgroundheader,
                                          secondheader;
    
    
    if( parent )
    {
      //If we are loading a snapshot, we actually want to associate the
      //  foreground/second/backgorund entries in the database with the parent
      //  entries, but actually load the snapshots.  This is so when the user
      //  saves changes, the parents saved state recieves the changes, and the
      //  snapshot doesnt get changed.
      //All of this is really ugly and horrible, and should be improved!
      
//      const bool autosave
//        = InterSpecUser::preferenceValue<bool>( "AutoSaveSpectraToDb", this );
//      if( autosave )
//      {
//      Its actually to late, HEAD will be set to the tag version even if the
//      user immediate selects save as.
//        const char *txt = "Auto saving is currently enabled, meaning the most"
//                          " recent version of this snapshot will get"
//                          " over-written with the current state (plus any"
//                          " changes you make) unless you disable auto saving,"
//                          " or do a &quot;Save As&quot; asap.";
//        passMessage( txt, "", WarningWidget::WarningMsgMedium );
//      }
      
      if( dbforeground && dbforeground->filedata.size() )
        snapforeground = (*dbforeground->filedata.begin())->decodeSpectrum();
    
      if( dbsecond && dbsecond->filedata.size() && dbsecond != dbforeground )
        snapsecond = (*dbsecond->filedata.begin())->decodeSpectrum();
      else if( dbsecond == dbforeground )
        snapsecond = snapforeground;
    
      if( dbbackground && dbbackground->filedata.size()
          && dbbackground != dbforeground
          && dbbackground != dbsecond  )
        snapbackground = (*dbbackground->filedata.begin())->decodeSpectrum();
      else if( dbbackground == dbforeground )
        snapbackground = snapforeground;
      else if( dbbackground == dbsecond )
        snapbackground = snapsecond;
      
      
      if( parent->foregroundId >= 0 )
        dbforeground = m_sql->session()->find<UserFileInDb>().where( "id = ?" )
                                        .bind( parent->foregroundId );
      if( parent->backgroundId >= 0
          && parent->backgroundId != parent->foregroundId )
      {
        dbbackground = m_sql->session()->find<UserFileInDb>().where( "id = ?" )
                                        .bind( parent->backgroundId );
      }else if( parent->backgroundId == parent->foregroundId )
      {
        dbbackground = dbforeground;
      }else if( parent->backgroundId < 0 && entry->backgroundId >= 0 )
      {
        parent.modify()->backgroundId = entry->backgroundId;
      }else
      {
        dbbackground.reset();
      }
      
      if( parent->secondForegroundId >= 0
         && parent->secondForegroundId != parent->foregroundId
         && parent->secondForegroundId != parent->backgroundId )
      {
        dbsecond = m_sql->session()->find<UserFileInDb>().where( "id = ?" )
                                    .bind( parent->secondForegroundId );
      }else if( parent->secondForegroundId < 0
                && entry->secondForegroundId >= 0 )
      {
        parent.modify()->secondForegroundId = entry->secondForegroundId;
      }else if( parent->secondForegroundId == parent->foregroundId )
      {
        dbsecond = dbforeground;
      }else if( parent->secondForegroundId == parent->backgroundId )
      {
        dbsecond = dbbackground;
      }else
      {
        dbsecond.reset();
      }
    }//if( parent )
    
    if( dbforeground )
      m_fileManager->setDbEntry( dbforeground, foregroundheader, foreground, 0);
    
    if( !parent || snapbackground )
    {
      if( dbbackground && dbbackground != dbforeground )
        m_fileManager->setDbEntry( dbbackground, backgroundheader, background, 0);
      else if( dbbackground == dbforeground )
        background = foreground;
    }//if( !parent || snapbackground )
    
    if( !parent || snapsecond )
    {
      if( dbsecond && dbsecond != dbforeground && dbsecond != dbbackground )
        m_fileManager->setDbEntry( dbsecond, secondheader, second, 0);
      else if( dbsecond == dbforeground )
        second = foreground;
      else if( dbsecond == dbbackground )
        second = background;
    }//if( !parent || snapsecond )
    
    if( parent )
    {
      if( foregroundheader && snapforeground )
      {
        foregroundheader->setMeasurmentInfo( snapforeground );
        foreground = snapforeground;
      }
      
      if( snapbackground != snapforeground )
      {
        if( backgroundheader && snapbackground )
        {
          backgroundheader->setMeasurmentInfo( snapbackground );
          background = snapbackground;
        }else if( snapbackground && dbbackground )
        {
          dbbackground = UserFileInDb::makeDeepWriteProtectedCopyInDatabase( dbbackground, true );
          if( dbbackground )
            UserFileInDb::removeWriteProtection( dbbackground );
          m_fileManager->setDbEntry( dbbackground, backgroundheader, background, 0);
        }
      }//if( snapbackground != snapforeground )
      
      
      if( (snapsecond != snapforeground) && (snapsecond != snapbackground) )
      {
        if( secondheader && snapsecond )
        {
          secondheader->setMeasurmentInfo( snapsecond );
          second = snapsecond;
        }else if( snapsecond && dbsecond )
        {
          dbsecond = UserFileInDb::makeDeepWriteProtectedCopyInDatabase( dbsecond, true );
          if( dbsecond )
            UserFileInDb::removeWriteProtection( dbsecond );
          m_fileManager->setDbEntry( dbsecond, secondheader, second, 0);
        }
      }
    }//if( parent )
    
    
    
    stringstream strm( entry->foregroundSampleNumsCsvIds );
    std::copy( istream_iterator<int>( strm ), istream_iterator<int>(),
               std::inserter( foregroundNums, foregroundNums.end() ) );
    strm.str( entry->secondForegroundSampleNumsCsvIds );
    std::copy( istream_iterator<int>( strm ), istream_iterator<int>(),
              std::inserter( secondNums, secondNums.end() ) );
    strm.str( entry->backgroundSampleNumsCsvIds );
    std::copy( istream_iterator<int>( strm ), istream_iterator<int>(),
              std::inserter( backgroundNums, backgroundNums.end() ) );
    strm.str( entry->otherSpectraCsvIds );
    std::copy( istream_iterator<int>( strm ), istream_iterator<int>(),
              std::inserter( otherSamples, otherSamples.end() ) );
    
    setSpectrum( foreground, foregroundNums, kForeground, false );
    setSpectrum( background, backgroundNums, kBackground, false );
    setSpectrum( second, secondNums, kSecondForeground, false );
    
    //Load the other spectra the user had opened.  Note that they were not
    //  write protected so they may have been changed or removed
    //...should check to make sure that they are lazily loaded, so as to not
    //  waste to much CPU.
    for( const int id : otherSamples )
    {
      try
      {
        Dbo::ptr<UserFileInDb> dbfile;
        std::shared_ptr<SpecMeas> meas;
        std::shared_ptr<SpectraFileHeader> header;
        dbfile = m_sql->session()->find<UserFileInDb>().where( "id = ?" ).bind( id );
        if( dbfile )
          m_fileManager->setDbEntry( dbfile, header, meas, 0 );
      }catch(...){}
    }//for( const int id : otherSamples )
    
    //Load the ShieldingSourceModel
    Dbo::ptr<ShieldingSourceModel> fitmodel;
    if( entry->shieldSourceModelId >= 0 )
      fitmodel = m_sql->session()->find<ShieldingSourceModel>()
                         .where( "id = ?" ).bind( entry->shieldSourceModelId );
    if( fitmodel )
    {
      cerr << "When loading state from DB, not loading Shielding/Source Fit"
      << " model, due to lame GUI ish" << endl;
//      if( !m_shieldingSourceFit )
//        showShieldingSourceFitWindow();
      if( m_shieldingSourceFit )
        m_shieldingSourceFit->loadModelFromDb( fitmodel );
    }//if( fitmodel )
    
    if( m_shieldingSourceFitWindow )
      m_shieldingSourceFitWindow->hide();
    
    if( m_nuclideSearchWindow )
    {
      m_isotopeSearchContainer->layout()->removeWidget( m_isotopeSearch );
      delete m_isotopeSearchContainer;
      m_isotopeSearchContainer = 0;
    }
    
//  std::string showingDetectorNumbersCsv;
//  see:  std::vector<bool> detectors_to_display() const;
    
    float xmin = entry->energyAxisMinimum;
    float xmax = entry->energyAxisMaximum;
    std::shared_ptr<const Measurement> hist = m_spectrum->data();
    
    if( hist && (xmin != xmax) && (xmax > xmin)
        && !IsInf(xmin) && !IsInf(xmax) && !IsNan(xmin) && !IsNan(xmax) )
    {
      const size_t nbin = hist->num_gamma_channels();
      xmin = std::max( xmin, hist->gamma_channel_lower( 0 ) );
      xmax = std::min( xmax, hist->gamma_channel_upper( nbin - 1 ) );
      m_spectrum->setXAxisRange( xmin, xmax );
#if( !USE_SPECTRUM_CHART_D3 )
      m_spectrum->guessAndUpdateDisplayRebinFactor();
#endif
    }//if( xmin != xmax )
    
#if( !USE_SPECTRUM_CHART_D3 )
    if( (entry->countsAxisMinimum != entry->countsAxisMaximum)
        && (entry->countsAxisMinimum > 0.0 || entry->countsAxisMaximum > 0.0) )
    {
      double newFactor = static_cast<double>( m_spectrum->displayRebinFactor());
      double scale = (entry->displayBinFactor > 0)
                                  ? (newFactor / entry->displayBinFactor) : 1.0;
      
      m_spectrum->setYAxisRange( entry->countsAxisMinimum,
                                 scale * entry->countsAxisMaximum );
    }//if( entry->countsAxisMinimum != entry->countsAxisMaximum )
#endif
    
//    bool logY = (entry->shownDisplayFeatures & UserState::kLogSpectrumCounts);
//    bool grid = (entry->shownDisplayFeatures & UserState::kGridLines);
//    cerr << "logY=" << logY << endl;
//    m_spectrum->setYAxisLog( logY );
//    m_spectrum->showGridLines( grid );
    
    if( (entry->shownDisplayFeatures & UserState::kSpectrumLegend) )
      m_spectrum->enableLegend( false );
    else
      m_spectrum->disableLegend();
    
    if( (entry->shownDisplayFeatures & UserState::kTimeSeriesLegend) )
      m_timeSeries->enableLegend( false );
    else
      m_timeSeries->disableLegend();
    
    
//  SpectrumSubtractMode backgroundSubMode;
    
    if( wasDocked )
    {
      WString title;
      switch( entry->currentTab )
      {
        case UserState::kPeakInfo:      title = PeakInfoTabTitle;      break;
        case UserState::kGammaLines:    title = GammaLinesTabTitle;    break;
        case UserState::kCalibration:   title = CalibrationTabTitle;   break;
        case UserState::kIsotopeSearch: title = NuclideSearchTabTitle; break;
        case UserState::kFileTab:       title = FileTabTitle;          break;
        case UserState::kNoTabs:                                       break;
      };//switch( entry->currentTab )
      
      for( int tab = 0; tab < m_toolsTabs->count(); ++tab )
      {
        if( m_toolsTabs->tabText( tab ) == title )
        {
          m_toolsTabs->setCurrentIndex( tab );
          m_currentToolsTab = tab;
          break;
        }//if( m_toolsTabs->tabText( tab ) == title )
      }//for( int tab = 0; tab < m_toolsTabs->count(); ++tab )
    }//if( wasDocked )
    
    //    if( entry->displayBinFactor > 0 )
    //      m_spectrum->setDisplayRebinFactor( displayBinFactor );
#if( !USE_SPECTRUM_CHART_D3 )
    m_spectrum->guessAndUpdateDisplayRebinFactor();
#endif
    
    //Should take care of entry->showingWindows here, so we can relie on it below
    if( entry->gammaLinesXml.size() )
    {
      bool toolTabsHidden = false;
      if( !m_referencePhotopeakLines )    //tool tabs must be hidden, so lets show the window
      {
        showGammaLinesWindow();//if entry->showingWindows was implemented
        toolTabsHidden = true;
        assert( m_referencePhotopeakLines );
      }
      
      string data = entry->gammaLinesXml;
      m_referencePhotopeakLines->deSerialize( data );
      
      if( toolTabsHidden )
        closeGammaLinesWindow();
    }//if( entry->gammaLinesXml.size() )
    
    if( m_isotopeSearch && entry->isotopeSearchEnergiesXml.size() )
    {
      string data = entry->isotopeSearchEnergiesXml;
      const bool display = !wasDocked || entry->currentTab == UserState::kIsotopeSearch;
      m_isotopeSearch->deSerialize( data, display );
    }//if( m_isotopeSearch && entry->isotopeSearchEnergiesXml.size() )

    for( SpectrumChart::PeakLabels label = SpectrumChart::PeakLabels(0);
        label < SpectrumChart::kNumPeakLabels;
        label = SpectrumChart::PeakLabels(label+1) )
    {
      const bool showing = (entry->showingPeakLabels & (0x1<<label));
      m_spectrum->setShowPeakLabel( label, showing );
    }//for( loop over peak labels )

    
    if( entry->userOptionsJson.size() )
    {
      try
      {
      Json::Value userOptionsVal( Json::ArrayType );
      Json::parse( entry->userOptionsJson, userOptionsVal );
      cerr << "Parsed User Options, but not doing anything with them" << endl;
      const Json::Array &userOptions = userOptionsVal;
      for( size_t i = 0; i < userOptions.size(); ++i )
      {
          const Json::Object &obj = userOptions[i];
          WString name = obj.get("name");
          const int type = obj.get("type");
          const UserOption::DataType datatype = UserOption::DataType( type );
      
          switch( datatype )
          {
            case UserOption::String:
            case UserOption::Decimal:
            case UserOption::Integer:  
              break;
            case UserOption::Boolean:
            {
              bool value = obj.get("value");
              cerr << "Pushing value for " << name.toUTF8() << endl;
              InterSpecUser::pushPreferenceValue( m_user, name.toUTF8(),
                                                  value, this, wApp );
            }//case UserOption::Boolean:
          }//switch( datatype )
        }//for( size_t i = 0; i < userOptions.size(); ++i )
      }catch( std::exception &e )
      {
        cerr << "Failed to parse JSON user preference with error: "
             << e.what() << endl;
        //well let this error slide (happens on OSX, Wt::JSON claims boost isnt
        //  new enough)
      }//try / catch
    }//if( entry->userOptionsJson.size() )
    
    if( (entry->shownDisplayFeatures & UserState::kShowingShieldSourceFit) )
      showShieldingSourceFitWindow();
    
    if( entry->colorThemeJson.size() > 32 )  //32 is arbitrary
    {
      try
      {
        auto theme = make_shared<ColorTheme>();
        ColorTheme::fromJson( entry->colorThemeJson, *theme );
        applyColorTheme( theme );
      }catch(...)
      {
        cerr << "loadStateFromDb(): Caught exception to set hte color them" << endl;
      }//try / catch to load ColorTheme.
    }//if( its reasonable we might have color theme information )
    
    
//  int showingMarkers;        //bitwise or of FeatureMarkers
//  int showingWindows;
    
    m_currentStateID = entry.id();
    if( parent )
      m_currentStateID = parent.id();
    
    transaction.commit();
  }catch( std::exception &e )
  {
    m_currentStateID = -1;
    updateSaveWorkspaceMenu();
    cerr << "\n\n\n\nloadStateFromDb(...) caught: " << e.what() << endl;
    throw runtime_error( e.what() );
  }//try / catch
  
  m_currentStateID = entry.id();
  updateSaveWorkspaceMenu();
}//void loadStateFromDb( Wt::Dbo::ptr<UserState> entry )


//Whenever m_currentStateID changes value, we need to update the menu!
void InterSpec::updateSaveWorkspaceMenu()
{
  m_saveStateAs->setDisabled( !m_dataMeasurement );
  
  //check if Save menu needs to be updated
  if( m_currentStateID >= 0 )
  {
    try
    {
      DataBaseUtils::DbTransaction transaction( *m_sql );
      Dbo::ptr<UserState> state = m_sql->session()
                                       ->find<UserState>( "where id = ?" )
                                       .bind( m_currentStateID );
      transaction.commit();
      
      if( state && (state->stateType==UserState::kEndOfSessionHEAD
                    || state->stateType==UserState::kUserSaved) )
      {
        m_saveState->setDisabled(false);
        m_createTag->setDisabled(false);
      } //UserState exists
      else
      { //no UserState
        m_saveState->setDisabled(true);
        m_createTag->setDisabled(true);
      } //no UserState
      return;
    }catch( std::exception &e )
    {
      cerr << "\nstartStoreStateInDb() caught: " << e.what() << endl;
    }//try / catch
  }

  m_saveState->setDisabled(true);
  m_createTag->setDisabled(true);
}//void updateSaveWorkspaceMenu()

#endif //#if( USE_DB_TO_STORE_SPECTRA )


std::shared_ptr<const ColorTheme> InterSpec::getColorTheme()
{
  if( m_colorTheme )
    return m_colorTheme;
  
  auto theme = make_shared<ColorTheme>();
  
  try
  {
    const int colorThemIndex = m_user->preferenceValue<int>("ColorThemeIndex", this);
    if( colorThemIndex < 0 )
    {
      auto deftheme = ColorTheme::predefinedTheme( ColorTheme::PredefinedColorTheme(-colorThemIndex) );
      if( deftheme )
        theme.reset( deftheme.release() );
    }else
    {
      DataBaseUtils::DbTransaction transaction( *m_sql );
      auto themeFromDb = m_user->m_colorThemes.find().where( "id = ?" ).bind( colorThemIndex ).resultValue();
      transaction.commit();
   
      theme->setFromDataBase( *themeFromDb );
    }
  }catch(...)
  {
    //We'll get the default color theme here.
  }//try / catch
  
  m_colorTheme = theme;
  
  return theme;
}//Wt::Dbo<ColorTheme> getColorTheme()


void InterSpec::setReferenceLineColors( std::shared_ptr<const ColorTheme> theme )
{
  if( !m_referencePhotopeakLines )
    return;
  
  if( !theme )
    theme = getColorTheme();
  
  m_referencePhotopeakLines->setPeaksGetAssignedRefLineColor( theme->peaksTakeOnReferenceLineColor );
  m_referencePhotopeakLines->setColors( theme->referenceLineColor );
  m_referencePhotopeakLines->setColorsForSpecificSources( theme->referenceLineColorForSources );
}//void setReferenceLineColors( Wt::Dbo<ColorTheme> ptr )


void InterSpec::applyColorTheme( shared_ptr<const ColorTheme> theme )
{
  //20190324: This function, called with a nullptr, takes 30us on my 2018 mbp.
  if( !theme )
    theme = getColorTheme();
  m_colorTheme = theme;

  this->m_colorPeaksBasedOnReferenceLines = theme->peaksTakeOnReferenceLineColor;
  
  m_spectrum->setForegroundSpectrumColor( theme->foregroundLine );
  m_spectrum->setBackgroundSpectrumColor( theme->backgroundLine );
  m_spectrum->setSecondarySpectrumColor( theme->secondaryLine );
  m_spectrum->setDefaultPeakColor( theme->defaultPeakLine );
  
  m_spectrum->setAxisLineColor( theme->spectrumAxisLines );
  m_spectrum->setChartMarginColor( theme->spectrumChartMargins );
  m_spectrum->setChartBackgroundColor( theme->spectrumChartBackground );
  m_spectrum->setTextColor( theme->spectrumChartText );
  
  m_timeSeries->setForegroundSpectrumColor( theme->timeChartGammaLine );
  m_timeSeries->setSecondarySpectrumColor( theme->timeChartNeutronLine );
  
  m_timeSeries->setAxisLineColor( theme->timeAxisLines );
  m_timeSeries->setChartMarginColor( theme->timeChartMargins );
  m_timeSeries->setChartBackgroundColor( theme->timeChartBackground );
  m_timeSeries->setTextColor( theme->timeChartText );
  
  m_timeSeries->setForegroundHighlightColor( theme->timeHistoryForegroundHighlight );
  m_timeSeries->setBackgroundHighlightColor( theme->timeHistoryBackgroundHighlight );
  m_timeSeries->setSecondaryHighlightColor( theme->timeHistorySecondaryHighlight );
  
  m_timeSeries->setOccupiedTimeSamplesColor( theme->occupancyIndicatorLines );
  
  setReferenceLineColors( theme );

  
  string cssfile;
  if( !theme->nonChartAreaTheme.empty() )
    cssfile = "InterSpec_resources/themes/" + theme->nonChartAreaTheme + "/" + theme->nonChartAreaTheme + ".css";
  
  
  if( !m_currentColorThemeCssFile.empty() && (m_currentColorThemeCssFile != cssfile) )
  {
    wApp->removeStyleSheet( m_currentColorThemeCssFile );
    m_currentColorThemeCssFile.clear();
  }
  
  if( (m_currentColorThemeCssFile != cssfile)
      && !UtilityFunctions::iequals(theme->nonChartAreaTheme, "default") )
  {
    if( UtilityFunctions::is_file(cssfile) )
    {
      wApp->useStyleSheet( cssfile );
      m_currentColorThemeCssFile = cssfile;
    }else
    {
      passMessage( "Could not load the non-peak-area theme '" + theme->nonChartAreaTheme + "'",
                  "", WarningWidget::WarningMsgLow );
    }
  }//if( should load CSS file )
}//void InterSpec::applyColorTheme()


#if( BUILD_AS_OSX_APP || APPLY_OS_COLOR_THEME_FROM_JS )
void InterSpec::osThemeChange( std::string name )
{
  cout << "InterSpec::osThemeChange('" << name << "');" << endl;
  
  UtilityFunctions::to_lower(name);
  if( name != "light" && name != "dark" && name != "default" && name != "no-preference" && name != "no-support" )
  {
    cerr << "InterSpec::osThemeChange('" << name << "'): unknown OS theme." << endl;
    return;
  }
  
  try
  {
    const int colorThemIndex = m_user->preferenceValue<int>("ColorThemeIndex", this);
    if( colorThemIndex >= 0 )
      return;
    
    const auto oldTheme = ColorTheme::PredefinedColorTheme(-colorThemIndex);
    
    if( oldTheme != ColorTheme::PredefinedColorTheme::DefaultColorTheme )
      return;
    
    std::unique_ptr<ColorTheme> theme;
    
    if( name == "dark" )
    {
      //Check to see if we already have dark applied
      if( m_colorTheme && UtilityFunctions::icontains( m_colorTheme->theme_name.toUTF8(), "Dark") )
        return;
     
      cerr << "Will set to dark" << endl;
      theme = ColorTheme::predefinedTheme( ColorTheme::PredefinedColorTheme::DarkColorTheme );
    }else if( name == "light" || name == "default" || name == "no-preference" || name == "no-support" )
    {
      //Check to see if we already have light applied
      if( m_colorTheme && UtilityFunctions::icontains( m_colorTheme->theme_name.toUTF8(), "Default") )
        return;
      
      cerr << "Will set to default" << endl;
      theme = ColorTheme::predefinedTheme( ColorTheme::PredefinedColorTheme::DefaultColorTheme );
    }
    
    if( theme )
      applyColorTheme( make_shared<ColorTheme>(*theme) );
  }catch(...)
  {
    cerr << "InterSpec::osThemeChange() caught exception - not doing anything" << endl;
  }
  
}//void osThemeChange( std::string name )
#endif


void InterSpec::showColorThemeWindow()
{
  //Takes ~5ms (on the server) to create a ColorThemeWindow.
	new ColorThemeWindow(this);
}//void showColorThemeWindow()


AuxWindow *InterSpec::showIEWarningDialog()
{
  try
  {
    wApp->environment().getCookie( "IEWarningDialog" );
    return NULL;
  }catch(...){}


  AuxWindow *dialog = new AuxWindow( "Not Compatible with Internet Explorer", AuxWindowProperties::IsAlwaysModal );
  dialog->setAttributeValue( "style", "max-width: 80%;" );

  WContainerWidget *contents = dialog->contents();
  contents->setContentAlignment( Wt::AlignCenter );

  WText *instructions = new WText(
          "This webapp has not been tested with Microsoft Internet Explorer, and<br>"
          " as a result some things will either fail to render or may not even work.<br>"
          "<p>Please consider using a reasonably recent version of Firefox, Chrome, or Safari.</p>"
          , XHTMLUnsafeText, contents );
  instructions->setInline( false );

  WPushButton *ok = new WPushButton( "Close", contents );
  ok->clicked().connect( dialog, &AuxWindow::hide );

  WCheckBox *cb = new WCheckBox( "Do not show again", contents );
  cb->setFloatSide( Right );
  cb->checked().connect( boost::bind( &InterSpec::setShowIEWarningDialogCookie, this, false ) );
  cb->unChecked().connect( boost::bind( &InterSpec::setShowIEWarningDialogCookie, this, true ) );

  dialog->centerWindow();
  dialog->show();
  
  dialog->finished().connect( boost::bind( AuxWindow::deleteAuxWindow, dialog ) );
  
  return dialog;
} // AuxWindow *showIEWarningDialog()


void InterSpec::showLicenseAndDisclaimersWindow( const bool is_awk,
                                      std::function<void()> finished_callback )
{
  if( m_licenseWindow )
  {
    m_licenseWindow->show();
    return;
  }
  
  m_licenseWindow = new LicenseAndDisclaimersWindow( is_awk, renderedWidth(), renderedHeight() );
  
  m_licenseWindow->finished().connect( std::bind([this,finished_callback](){
    
    //#warning "Setting VersionAgreedToUseTerms to zero for debugging"
    //InterSpecUser::setPreferenceValue( m_user, "VersionAgreedToUseTerms", 0, this );
    InterSpecUser::setPreferenceValue( m_user, "VersionAgreedToUseTerms", COMPILE_DATE_AS_INT, this );
    
    deleteLicenseAndDisclaimersWindow();
    
    if( finished_callback )
      finished_callback();
  }) );
}//void showLicenseAndDisclaimersWindow()


void InterSpec::deleteLicenseAndDisclaimersWindow()
{ 
  if( !m_licenseWindow )
    return;
  
  AuxWindow::deleteAuxWindow( m_licenseWindow );
  m_licenseWindow = nullptr;
}//void deleteLicenseAndDisclaimersWindow()


void InterSpec::showWelcomeDialog( bool force )
{
  if( !force && !m_user->preferenceValue<bool>( "ShowSplashScreen" ) )
    return;
  
  if( m_useInfoWindow )
  {
    m_useInfoWindow->show();
    return;
  }
  
  WServer::instance()->post( wApp->sessionId(), std::bind( [this](){
    /*
     For Android, showing this useInfoWindow at startup causes some exceptions
     in the JavaScript whenever the loading indicator is shown.  I'm pretty
     confused by it.
     I tried setting a new loading indicator in deleteWelcomeCountDialog(), but it didnt work
     
     Havent checked if creating this window via "posting" helps
     */
    if( m_useInfoWindow )
      return;
    
    std::function<void(bool)> dontShowAgainCallback = [this](bool value){
      InterSpecUser::setPreferenceValue<bool>( m_user, "ShowSplashScreen", value, this );
    };
    m_useInfoWindow = new UseInfoWindow( dontShowAgainCallback , this );
  
    m_useInfoWindow->finished().connect( this, &InterSpec::deleteWelcomeCountDialog );
    
    wApp->triggerUpdate();
  } ) );
}//void showWelcomeDialog()


void InterSpec::deleteWelcomeCountDialog()
{
  if( !m_useInfoWindow )
    return;
  
  AuxWindow::deleteAuxWindow( m_useInfoWindow );
  m_useInfoWindow = nullptr;
}//void deleteWelcomeCountDialog()


void InterSpec::deletePreserveCalibWindow()
{
  if( m_preserveCalibWindow )
  {
    delete m_preserveCalibWindow;
    m_preserveCalibWindow = 0;
  }
}//void deletePreserveCalibWindow()


void InterSpec::setShowIEWarningDialogCookie( bool show )
{
  if( show )
    wApp->setCookie( "IEWarningDialog", "", 0 );
  else
    wApp->setCookie( "IEWarningDialog", "noshow", 3600*24*30 );
} // void InterSpec::setShowIEWarningDialogCookie( bool show )



void InterSpec::showGammaCountDialog()
{
  if( m_gammaCountDialog )
  {
//    m_gammaCountDialog->show();
//    m_gammaCountDialog->expand();
    return;
  }//if( m_gammaCountDialog )

  m_gammaCountDialog = new GammaCountDialog( this );
//  m_gammaCountDialog->rejectWhenEscapePressed();
  m_gammaCountDialog->finished().connect( this, &InterSpec::deleteGammaCountDialog );
//  m_gammaCountDialog->resizeToFitOnScreen();
}//void showGammaCountDialog()


void InterSpec::deleteGammaCountDialog()
{
  if( m_gammaCountDialog )
    delete m_gammaCountDialog;

  m_gammaCountDialog = 0;
}//void deleteGammaCountDialog()




#if( USE_SPECRUM_FILE_QUERY_WIDGET )
void InterSpec::showFileQueryDialog()
{
  if( m_specFileQueryDialog )
    return;
  
  m_specFileQueryDialog = new AuxWindow( "Spectrum File Query Tool", AuxWindowProperties::TabletModal );
  m_specFileQueryDialog->setResizable( true );
  //m_specFileQueryDialog->disableCollapse();
  
  SpecFileQueryWidget *qw = new SpecFileQueryWidget( this );
  WGridLayout *stretcher = m_specFileQueryDialog->stretcher();
  stretcher->addWidget( qw, 0, 0 );
  stretcher->setContentsMargins( 0, 0, 0, 0 );
  stretcher->setVerticalSpacing( 0 );
  stretcher->setHorizontalSpacing( 0 );
  
  WPushButton *closeButton = m_specFileQueryDialog->addCloseButtonToFooter( "Close", true );
  closeButton->clicked().connect( boost::bind( &AuxWindow::hide, m_specFileQueryDialog ) );
  
  m_specFileQueryDialog->finished().connect( this, &InterSpec::deleteFileQueryDialog );
  
  if( m_renderedWidth > 100 && m_renderedHeight > 100 && !isPhone() )
  {
    m_specFileQueryDialog->resize( 0.85*m_renderedWidth, 0.85*m_renderedHeight );
    m_specFileQueryDialog->centerWindow();
  }
  
  AuxWindow::addHelpInFooter( m_specFileQueryDialog->footer(), "spectrum-file-query" );
}//void showFileQueryDialog()


void InterSpec::deleteFileQueryDialog()
{
  if( !m_specFileQueryDialog )
    return;
  delete m_specFileQueryDialog;
  m_specFileQueryDialog = 0;
}//void deleteFileQueryDialog()
#endif




PopupDivMenu * InterSpec::displayOptionsPopupDiv()
{
  return m_displayOptionsPopupDiv;
}//WContainerWidget *displayOptionsPopupDiv()


void InterSpec::logMessage( const Wt::WString& message, const Wt::WString& source, int priority )
{
#if( PERFORM_DEVELOPER_CHECKS )
  static std::mutex s_message_mutex;
  
  {//begin codeblock to logg message
    std::lock_guard<std::mutex> file_gaurd( s_message_mutex );
    ofstream output( "interspec_messages_to_users.txt", ios::out | ios::app );
    const boost::posix_time::ptime now = WDateTime::currentDateTime().toPosixTime();
    output << "Message " << UtilityFunctions::to_iso_string( now ) << " ";
    if( !source.empty() )
      output << "[" << source.toUTF8() << ", " << priority << "]: ";
    else
      output << "[" << priority << "]: ";
    output << message.toUTF8() << endl << endl;
  }//end codeblock to logg message
#endif
  
  if( wApp )
  {
    m_messageLogged.emit( message, source, priority );
//    wApp->triggerUpdate();
  }else
  {
    const boost::posix_time::ptime now = WDateTime::currentDateTime().toPosixTime();
    cerr << "Message " << UtilityFunctions::to_iso_string( now ) << " ";
    if( !source.empty() )
      cerr << "[" << source.toUTF8() << ", " << priority << "]: ";
    else
      cerr << "[" << priority << "]: ";
    cerr << message.toUTF8() << endl << endl;
  }
}//void InterSpec::logMessage(...)



Wt::Signal< Wt::WString, Wt::WString, int > &InterSpec::messageLogged()
{
  return m_messageLogged;
}


Wt::WContainerWidget *InterSpec::menuDiv()
{
  return m_menuDiv;
}


void InterSpec::showWarningsWindow()
{
  m_warnings->createContent();
  
  if( !m_warningsWindow )
  {
    m_warningsWindow = new AuxWindow( "Notification/Logs",
                  (Wt::WFlags<AuxWindowProperties>(AuxWindowProperties::TabletModal) | AuxWindowProperties::IsAlwaysModal) );
    m_warningsWindow->contents()->setOffsets( WLength(0, WLength::Pixel), Wt::Left | Wt::Top );
    m_warningsWindow->rejectWhenEscapePressed();
    m_warningsWindow->stretcher()->addWidget( m_warnings, 0, 0, 1, 1 );
    m_warningsWindow->stretcher()->setContentsMargins(0,0,0,0);
    m_warningsWindow->setResizable(true);
    m_warningsWindow->resizeScaledWindow(0.75, 0.75);
    m_warningsWindow->centerWindow();
    m_warningsWindow->finished().connect( boost::bind( &InterSpec::handleWarningsWindowClose, this, false ) );
        
      
    WPushButton *clearButton = new WPushButton( "Delete Logs", m_warningsWindow->footer() );
    clearButton->clicked().connect( boost::bind( &WarningWidget::clearMessages, m_warnings ) );
    clearButton->addStyleClass( "BinIcon" );
      if (isMobile())
      {
          clearButton->setFloatSide( Right );
      }
      WPushButton *closeButton = m_warningsWindow->addCloseButtonToFooter();
      closeButton->clicked().connect( boost::bind( &AuxWindow::hide, m_warningsWindow ) );

  }//if( !m_warningsWindow )
  
  m_warningsWindow->show();
}//void showWarningsWindow()


void InterSpec::handleWarningsWindowClose( bool close )
{
    m_warningsWindow->stretcher()->removeWidget( m_warnings );
    AuxWindow::deleteAuxWindow( m_warningsWindow );
    m_warningsWindow = 0;
}//void handleWarningsWindowClose( bool )


void InterSpec::showPeakInfoWindow()
{
  if( m_toolsTabs )
  {
    cerr << SRC_LOCATION << "\n\tTemporary hack - we dont want to show the"
         << " peak info window when tool tabs are showing since things get funky"
         << endl;
    m_toolsTabs->setCurrentWidget( m_peakInfoDisplay );
    m_currentToolsTab = m_toolsTabs->currentIndex();
    return;
  }//if( m_toolsTabs )
 
  if( m_toolsTabs && m_peakInfoDisplay )
  {
    m_toolsTabs->removeTab( m_peakInfoDisplay );
    m_toolsTabs->setCurrentIndex( 0 );
    m_currentToolsTab = 0;
  }//if( m_toolsTabs && m_peakInfoDisplay )
  
  if( !m_peakInfoWindow )
  {
    m_peakInfoWindow = new AuxWindow( "Peak Manager" );
    m_peakInfoWindow->rejectWhenEscapePressed();
    WBorderLayout *layout = new WBorderLayout();
    layout->setContentsMargins(0, 0, 15, 0);
    m_peakInfoWindow->contents()->setLayout( layout );
    //m_peakInfoWindow->contents()->setPadding(0);
    //m_peakInfoWindow->contents()->setMargin(0);
    
    layout->addWidget( m_peakInfoDisplay, Wt::WBorderLayout::Center );
    WContainerWidget *buttons = new WContainerWidget();
    layout->addWidget( buttons, Wt::WBorderLayout::South );

    WContainerWidget* footer = m_peakInfoWindow->footer();
      
    WPushButton* closeButton = m_peakInfoWindow->addCloseButtonToFooter("Close",true);
    closeButton->clicked().connect( boost::bind( &AuxWindow::hide, m_peakInfoWindow ) );
      
    WPushButton *b = new WPushButton( CalibrationTabTitle, footer );
    // b->setIcon(WLink("InterSpec_resources/images/calibrate.png"));
    b->clicked().connect( this, &InterSpec::showRecalibratorWindow );
    b->setFloatSide(Wt::Right);
      
    b = new WPushButton( GammaLinesTabTitle, footer );
    // b->setIcon(WLink("InterSpec_resources/images/reflines.png"));
    b->clicked().connect( this, &InterSpec::showGammaLinesWindow );
    b->setFloatSide(Wt::Right);
      
      
    m_peakInfoWindow->resizeScaledWindow( 0.75, 0.5 );
    m_peakInfoWindow->centerWindow();
    m_peakInfoWindow->setResizable( true );
    m_peakInfoWindow->finished().connect( this, &InterSpec::handlePeakInfoClose );
  }//if( !m_peakInfoWindow )

  m_peakInfoWindow->show();
}//void showPeakInfoWindow()


void InterSpec::handlePeakInfoClose()
{
  if( !m_peakInfoWindow )
    return;

  if( m_toolsTabs )
  {
    m_peakInfoWindow->contents()->removeWidget( m_peakInfoDisplay );
    if( m_toolsTabs->indexOf(m_peakInfoDisplay) < 0 )
    {
      m_toolsTabs->addTab( m_peakInfoDisplay, PeakInfoTabTitle, TabLoadPolicy );
      m_toolsTabs->setCurrentIndex( m_toolsTabs->indexOf(m_peakInfoDisplay) );
    }//if( m_toolsTabs->indexOf(m_peakInfoDisplay) < 0 )
    m_currentToolsTab = m_toolsTabs->currentIndex();
    
    delete m_peakInfoWindow;
    m_peakInfoWindow = NULL;
  }//if( m_toolsTabs )
}//void handlePeakInfoClose()


#if( USE_DB_TO_STORE_SPECTRA )
void InterSpec::finishStoreStateInDb( WLineEdit *nameedit,
                                           WTextArea *descriptionarea,
                                           AuxWindow *window,
                                           const bool forTesting,
                                           Dbo::ptr<UserState> parent )
{
  const WString &name = nameedit->text();
  if( name.empty() )
  {
    passMessage( "You must specify a name", "", WarningWidget::WarningMsgHigh );
    return;
  }//if( name.empty() )
  
  const WString &desc = (!descriptionarea ? "" : descriptionarea->text());
  
  Dbo::ptr<UserState> state = serializeStateToDb( name, desc, forTesting, parent );
  
  if( parent )
    m_currentStateID = parent.id();
  else
    m_currentStateID = state.id();
    
  updateSaveWorkspaceMenu();
  
#if( INCLUDE_ANALYSIS_TEST_SUITE )
  if( forTesting )
  {
    string filepath = UtilityFunctions::append_path(TEST_SUITE_BASE_DIR, "analysis_tests");
    
    if( !UtilityFunctions::is_directory( filepath ) )
      throw runtime_error( "CWD didnt contain a 'analysis_tests' folder as expected" );
    
    const boost::posix_time::ptime localtime = boost::posix_time::second_clock::local_time();
    
    string timestr = UtilityFunctions::to_iso_string( localtime );
    string::size_type period_pos = timestr.find_last_of( '.' );
    timestr.substr( 0, period_pos );
    
    filepath = UtilityFunctions::append_path( filepath, (name.toUTF8() + "_" + timestr + ".n42") );
    
#ifdef _WIN32
    const std::wstring wfilepath = UtilityFunctions::convert_from_utf8_to_utf16(filepath);
    ofstream output( wfilepath.c_str(), ios::binary|ios::out );
#else
    ofstream output( filepath.c_str(), ios::binary|ios::out );
#endif
    if( !output.is_open() )
      throw runtime_error( "Couldnt open test file ouput '"
                           + filepath + "'" );
    
    storeTestStateToN42( output, name.toUTF8(), desc.toUTF8() );
  }//if( forTesting )
#endif
  if (window)
      delete window;
}//void finishStoreStateInDb()


#if( INCLUDE_ANALYSIS_TEST_SUITE )
void InterSpec::storeTestStateToN42( std::ostream &output,
                                          const std::string &name,
                                          const std::string &descr )
{
  try
  {
    if( !m_dataMeasurement )
      throw runtime_error( "No valid foreground file" );
    
    SpecMeas meas;
    meas.uniqueCopyContents( *m_dataMeasurement );
    
    
#if( PERFORM_DEVELOPER_CHECKS )
    try
    {
      SpecMeas::equalEnough( meas, *m_dataMeasurement );
    }catch( std::exception &e )
    {
      log_developer_error( BOOST_CURRENT_FUNCTION, e.what() );
    }
#endif
    
    const set<int> foregroundsamples = displayedSamples( kForeground );
    const set<int> backgroundsamples = displayedSamples( kBackground );
    set<int> newbacksamples;
    
    for( const MeasurementConstShrdPtr &p : meas.measurements() )
    {
      const int samplenum = p->sample_number();
      if( foregroundsamples.count(samplenum) )
        meas.set_source_type( Measurement::Foreground, p );
      else
        meas.remove_measurment( p, false );
    }//for( const MeasurementConstShrdPtr &p : meas.measurements() )
    
    
    std::shared_ptr< std::deque< std::shared_ptr<const PeakDef> > >
    foregroundpeaks, backgroundpeaks;
    foregroundpeaks = meas.peaks( foregroundsamples );
    
    if( !!m_backgroundMeasurement )
    {
      backgroundpeaks = m_backgroundMeasurement->peaks( backgroundsamples );
      
      std::vector< MeasurementConstShrdPtr > backgrounds = m_backgroundMeasurement->measurements();
      for( const MeasurementConstShrdPtr &p : backgrounds )
      {
        if( !backgroundsamples.count( p->sample_number()) )
          continue;
        
        MeasurementShrdPtr backmeas = std::make_shared<Measurement>( *p );
        meas.add_measurment( backmeas, false );
        meas.set_source_type( Measurement::Background, backmeas );
        newbacksamples.insert( backmeas->sample_number() );
      }//for( const MeasurementConstShrdPtr &p : m_backgroundMeasurement->measurements() )
    }//if( !!m_backgroundMeasurement )
 
    //Now remove all peaks not for the currently displayed samples.
    meas.removeAllPeaks();
    
    //But add back in peaks for just the displayed foreground and background
    if( !!foregroundpeaks && foregroundpeaks->size() )
      meas.setPeaks( *foregroundpeaks, foregroundsamples );
    if( !!backgroundpeaks && backgroundpeaks->size() )
      meas.setPeaks( *backgroundpeaks, newbacksamples );
    
    using namespace ::rapidxml;
    std::shared_ptr< xml_document<char> > n42doc;
    n42doc = meas.create_2012_N42_xml();
    
    if( !n42doc )
      throw runtime_error( "Failed to create 2012 N42 XML document" );
    
    xml_node<char> *RadInstrumentData = n42doc->first_node( "RadInstrumentData", 17 );
    
    if( !RadInstrumentData )
      throw runtime_error( "Logic error trying to retrieve RadInstrumentData node" );
    
    xml_node<char> *InterSpecNode = RadInstrumentData->first_node( "DHS:InterSpec", 13 );
    if( !InterSpecNode )
      throw runtime_error( "Logic error trying to retrieve DHS:InterSpec node" );
    
    //Make a note that this contains information for testing as well
    xml_attribute<char> *attr = n42doc->allocate_attribute( "ForTesting", "true" );
    InterSpecNode->append_attribute( attr );
    
    //Add user options into the XML
    m_user->userOptionsToXml( InterSpecNode, this );
    
    if( newbacksamples.size() )
    {
      vector<int> backsamples( newbacksamples.begin(), newbacksamples.end() );
      stringstream backsamplesstrm;
      for( size_t i = 0; i < backsamples.size(); ++i )
        backsamplesstrm << (i?" ":"") << backsamples[i];
      const char *val = n42doc->allocate_string( backsamplesstrm.str().c_str() );
      xml_node<char> *backgroundsamples_node
                    = n42doc->allocate_node( node_element,
                                      "DisplayedBackgroundSampleNumber", val );
      InterSpecNode->append_node( backgroundsamples_node );
    }//if( newbacksamples.size() )
    
    if( m_shieldingSourceFit
        && m_shieldingSourceFit->userChangedDuringCurrentForeground() )
    {
      m_shieldingSourceFit->serialize( InterSpecNode );
      cerr << "\n\nThe shielding/source fit model WAS changed for current foreground" << endl;
    }else
    {
      cerr << "\n\nThe shielding/source fit model was NOT changed for current foreground" << endl;
    }
    
    const boost::posix_time::ptime localtime = boost::posix_time::second_clock::local_time();
    const string timestr = UtilityFunctions::to_iso_string( localtime );
    
    const char *val = n42doc->allocate_string( timestr.c_str(), timestr.size()+1 );
    xml_node<char> *node = n42doc->allocate_node( node_element, "TestSaveDateTime", val );
    InterSpecNode->append_node( node );
    
    if( name.size() )
    {
      val = n42doc->allocate_string( name.c_str(), name.size()+1 );
      node = n42doc->allocate_node( node_element, "TestName", val );
      InterSpecNode->append_node( node );
    }//if( name.size() )
    
    if( descr.size() )
    {
      val = n42doc->allocate_string( descr.c_str(), descr.size()+1 );
      node = n42doc->allocate_node( node_element, "TestDescription", val );
      InterSpecNode->append_node( node );
    }//if( name.size() )
    
    string xml_data;
    rapidxml::print( std::back_inserter(xml_data), *n42doc, 0 );
    
    if( !output.write( xml_data.c_str(), xml_data.size() ) )
      throw runtime_error( "" );
  }catch( std::exception &e )
  {
    string msg = "Failed to save test state to N42 file: ";
    msg += e.what();
    passMessage( msg, "", WarningWidget::WarningMsgHigh );
  }//try / catch
}//void storeTestStateToN42( const std::string &name )



void InterSpec::loadTestStateFromN42( std::istream &input )
{
  using namespace rapidxml;
  try
  {
    rapidxml::file<char> input_file( input );
    
    xml_document<char> doc;
    doc.parse<rapidxml::parse_trim_whitespace>( input_file.data() );
    xml_node<char> *doc_node = doc.first_node();
    
    std::shared_ptr<SpecMeas> meas = std::make_shared<SpecMeas>();
    const bool loaded = meas->load_from_N42_document( doc_node );
    
    if( !loaded )
      throw runtime_error( "Failed to load SpecMeas from XML document" );
    
    const xml_node<char> *RadInstrumentData = doc.first_node( "RadInstrumentData", 17 );
    
    if( !RadInstrumentData )
      throw runtime_error( "Error trying to retrieve RadInstrumentData node" );
    
    const xml_node<char> *InterSpecNode = RadInstrumentData->first_node( "DHS:InterSpec", 13 );
    if( !InterSpecNode )
      throw runtime_error( "Error trying to retrieve DHS:InterSpec node" );

    meas->decodeSpecMeasStuffFromXml( InterSpecNode );
    
    const xml_attribute<char> *attr = InterSpecNode->first_attribute( "ForTesting" );
    if( !attr || !attr->value()
        || !rapidxml::internal::compare(attr->value(), attr->value_size(), "true", 4, true) )
      throw runtime_error( "Input N42 file doesnt look to be a test state" );
    
    set<int> backgroundsamplenums;
    const xml_node<char> *backsample_node
               = InterSpecNode->first_node( "DisplayedBackgroundSampleNumber" );
    
    if( backsample_node && backsample_node->value_size() )
    {
      std::vector<int> results;
      const bool success = UtilityFunctions::split_to_ints(
                                      backsample_node->value(),
                                      backsample_node->value_size(), results );
      if( !success )
        throw runtime_error( "Error parsing background sample numbers" );
      backgroundsamplenums.insert( results.begin(), results.end() );
    }//if( backsample_node && backsample_node->value_size() )
    
    
    const xml_node<char> *preferences = InterSpecNode->first_node( "preferences" );
    if( preferences )
      InterSpecUser::restoreUserPrefsFromXml( m_user, preferences, this );
    else
      cerr << "Warning, couldnt find preferences in the XML"
              " when restoring state from XML" << endl;
    
    
//    {
//      std::shared_ptr<const std::deque< std::shared_ptr<const PeakDef> > > peaks;
//      peaks = meas->peaks( meas->displayedSampleNumbers() );
//      if( !!peaks )
//        cerr << "There were NO peaks for displayed sample numbers" << endl;
//      else
//        cerr << "There were " << peaks->size() << " peaks for displayed sample numbers" << endl;
//    
//      cerr << "Displaying sample numbers: ";
//      for( int i : meas->displayedSampleNumbers() )
//        cerr << i << ", ";
//      cerr << endl;
      
//      std::set<std::set<int> > samps = meas->sampleNumsWithPeaks();
//      cerr << "There are samps.size()=" << samps.size() << endl;
//      for( auto a : samps )
//      {
//        cerr << "Set: ";
//        for( auto b : a )
//          cerr << b << " ";
//        cerr << endl;
//      }
//    }
    
    
    
    const xml_node<char> *name = InterSpecNode->first_node( "TestName" );
    const xml_node<char> *descrip = InterSpecNode->first_node( "TestDescription" );
    const xml_node<char> *savedate = InterSpecNode->first_node( "TestSaveDateTime" );
    
    
    std::shared_ptr<SpecMeas> dummy;
    setSpectrum( dummy, std::set<int>(), kBackground, false );
    setSpectrum( dummy, std::set<int>(), kSecondForeground, false );
    
    string filename = meas->filename();
    if( name && name->value_size() )
      filename = name->value();
    
    std::shared_ptr<SpectraFileHeader> header;
    header = m_fileManager->addFile( filename, meas );
    setSpectrum( meas, meas->displayedSampleNumbers(), kForeground, false );
    
    if( backgroundsamplenums.size() )
      setSpectrum( meas, backgroundsamplenums, kBackground, false );
    
    const xml_node<char> *sourcefit = InterSpecNode->first_node( "ShieldingSourceFit" );
    if( sourcefit )
    {
      if( !m_shieldingSourceFit )
      {
        showShieldingSourceFitWindow();
        if( m_shieldingSourceFitWindow )
          m_shieldingSourceFitWindow->hide();
      }//if( !m_shieldingSourceFit )
      
      m_shieldingSourceFit->deSerialize( sourcefit );
    }//if( sourcefit )
    
    stringstream msg;
    msg << "Loaded test state";
    if( name && name->value_size() )
      msg << " named '" << name->value() << "'";
    if( savedate && savedate->value_size() )
      msg << " saved " << savedate->value();
    if( descrip && descrip->value_size() )
      msg << " with description " << descrip->value();
    
    passMessage( msg.str(), "", WarningWidget::WarningMsgInfo );
  }catch( std::exception &e )
  {
    string msg = "InterSpec::loadTestStateFromN42: ";
    msg += e.what();
    passMessage( msg, "", WarningWidget::WarningMsgHigh );
  }//try / catch
}//void InterSpec::loadTestStateFromN42()

namespace
{
  void doTestStateLoad( WSelectionBox *filesbox,
                        AuxWindow *window,
                        InterSpec *viewer )
  {
    const string filename = string("analysis_tests/")
                            + filesbox->currentText().toUTF8();
    viewer->loadN42TestState( filename );
    delete window;
  }
}

void InterSpec::startN42TestStates()
{
  const std::string path_to_tests = UtilityFunctions::append_path( TEST_SUITE_BASE_DIR, "analysis_tests" );
  
  const vector<string> files
                  = UtilityFunctions::recursive_ls( path_to_tests, ".n42" );
  
  if( files.empty() )
  {
    passMessage( "There are no test files to load in the 'analysis_tests' "
                 "directory", "", WarningWidget::WarningMsgHigh );
    return;
  }//if( files.empty() )
  
  AuxWindow *window = new AuxWindow( "Test State N42 Files" );
  window->resizeWindow( 450, 400 );
  
  WGridLayout *layout = window->stretcher();
  WSelectionBox *filesbox = new WSelectionBox();
  layout->addWidget( filesbox, 0, 0, 1, 2 );
  
  for( const string &p : files )
  {
    string name = p;
    if( UtilityFunctions::starts_with(name, "analysis_tests/" ) )
      name = name.substr(15);
    filesbox->addItem( name );
  }
  
  WPushButton *button = new WPushButton( "Cancel" );
  layout->addWidget( button, 1, 0 );
  button->clicked().connect( boost::bind(&AuxWindow::deleteAuxWindow, window) );
  
  button = new WPushButton( "Load" );
  button->disable();
//  button->clicked().connect( boost::bind( [=](){
//    loadN42TestState( filesbox->currentText().toUTF8() );
//    delete window;
//  }) );
  button->clicked().connect( boost::bind( &doTestStateLoad, filesbox, window, this ) );
  
  layout->addWidget( button, 1, 1 );
  filesbox->activated().connect( button, &WPushButton::enable );
  
  layout->setRowStretch( 0, 1 );
  layout->setColumnStretch( 0, 1 );
  layout->setColumnStretch( 1, 1 );
  
  window->show();
  window->centerWindow();
}//void startN42TestStates()


void InterSpec::loadN42TestState( const std::string &filename )
{
  if( filename.empty() )
    return;
  
#ifdef _WIN32
  const std::wstring wfilename = UtilityFunctions::convert_from_utf8_to_utf16(filename);
  ifstream input( wfilename.c_str(), ios::in | ios::binary );
#else
  ifstream input( filename.c_str(), ios::in | ios::binary );
#endif
  if( !input.is_open() )
    throw runtime_error( "Couldnt open test state file: " + filename );
  
  loadTestStateFromN42( input );
}//void loadN42TestState( const std::string &filename )



void InterSpec::startStoreTestStateInDb()
{
  if( m_currentStateID >= 0 )
  {
    AuxWindow *window = new AuxWindow( "Warning", AuxWindowProperties::PhoneModal );
    WText *t = new WText( "Overwrite current test state?", window->contents() );
    t->setInline( false );
    
    window->setClosable( false );

    WPushButton *button = new WPushButton( "Yes", window->footer() );
    button->clicked().connect( boost::bind( &AuxWindow::deleteAuxWindow, window ) );
    button->clicked().connect( boost::bind( &InterSpec::startStoreStateInDb, this, true, false, true, false ) );
    
    button = new WPushButton( "No", window->footer() );
    button->clicked().connect( boost::bind( &AuxWindow::deleteAuxWindow, window ) );
    button->clicked().connect( boost::bind( &InterSpec::startStoreStateInDb, this, true, true, false, false ) );
    
    button = new WPushButton( "Cancel", window->footer() );
    button->clicked().connect( boost::bind( &AuxWindow::deleteAuxWindow, window ) );
    window->centerWindow();
    window->show();
    
    window->rejectWhenEscapePressed();
    window->finished().connect( boost::bind( &AuxWindow::deleteAuxWindow, window ) );
  }else
  {
    startStoreStateInDb( true, true, true, false );
  }//if( m_currentStateID >= 0 ) / else
}//void startStoreTestStateInDb()
#endif //#if( INCLUDE_ANALYSIS_TEST_SUITE )


//Save the snapshot and ALSO the spectra.
void InterSpec::stateSave()
{
  if( m_currentStateID > 0 )
  {
    //TODO: Should check if can save?
    saveShieldingSourceModelToForegroundSpecMeas();
    startStoreStateInDb( false, false, false, false ); //save snapshot
  }else
  {
    //No currentStateID, so just save as new snapshot
    stateSaveAs();
  }//if( m_currentStateID > 0 ) / else
} //void stateSave()


//Copied from startStoreStateInDb()
//This is the new combined Save As method (snapshot/spectra)
void InterSpec::stateSaveAs()
{
  const bool forTesting = false;
    
  AuxWindow *window = new AuxWindow( "Store State As", (Wt::WFlags<AuxWindowProperties>(AuxWindowProperties::IsAlwaysModal) | AuxWindowProperties::TabletModal) );
  window->rejectWhenEscapePressed();
  window->finished().connect( boost::bind( &AuxWindow::deleteAuxWindow, window ) );
  window->setClosable( false );
  WGridLayout *layout = window->stretcher();
    
  WLineEdit *edit = new WLineEdit();
  edit->setEmptyText( "(Name to store under)" );
  WText *label = new WText( "Name" );
  layout->addWidget( label, 2, 0 );
  layout->addWidget( edit,  2, 1 );
    
  if( !!m_dataMeasurement && m_dataMeasurement->filename().size() )
      edit->setText( m_dataMeasurement->filename() );
    
  WTextArea *summary = new WTextArea();
  label = new WText( "Desc." );
  summary->setEmptyText( "(Optional description)" );
  layout->addWidget( label, 3, 0 );
  layout->addWidget( summary, 3, 1 );
  
  layout->setColumnStretch( 1, 1 );
  layout->setRowStretch( 3, 1 );
  
  label = new WText( "This will save the current app state to the database - to export<br/> as a separate file, use the &quot;Export File&quot; menu option" );
  label->setInline( false );
  label->setTextAlignment( Wt::AlignCenter );
  layout->addWidget( label, 4, 1 );
  
  WPushButton *closeButton = window->addCloseButtonToFooter("Cancel", false);
  closeButton->clicked().connect( boost::bind( &AuxWindow::deleteAuxWindow, window ) );
    
  WPushButton *save = new WPushButton( "Save" , window->footer() );
  save->setIcon( "InterSpec_resources/images/disk2.png" );
  
  save->clicked().connect( boost::bind( &InterSpec::stateSaveAsAction,
                                        this, edit, summary, window, forTesting ) );
    
  window->setMinimumSize(WLength(450), WLength(250));
    
  window->centerWindow();
  window->show();
}//void stateSaveAs()


//New method to save tag for snapshot
void InterSpec::stateSaveTag()
{
  AuxWindow *window = new AuxWindow( "Tag current state",
                  (Wt::WFlags<AuxWindowProperties>(AuxWindowProperties::IsAlwaysModal) | AuxWindowProperties::TabletModal) );
  window->rejectWhenEscapePressed();
  window->finished().connect( boost::bind( &AuxWindow::deleteAuxWindow, window ) );
  window->setClosable( false );
  window->disableCollapse();
  WGridLayout *layout = window->stretcher();
    
  WLineEdit *edit = new WLineEdit();
  if( !isMobile() )
    edit->setFocus(true);
  WText *label = new WText( "Name" );
  layout->addWidget( label, 2, 0 );
  layout->addWidget( edit,  2, 1 );
    
  layout->setColumnStretch( 1, 1 );
  layout->setRowStretch( 2, 1 );
  
  WPushButton *closeButton = window->addCloseButtonToFooter("Cancel", false);
  closeButton->clicked().connect( boost::bind( &AuxWindow::deleteAuxWindow, window ) );
  
  WPushButton *save = new WPushButton( "Tag" , window->footer() );
  //save->setIcon( "InterSpec_resources/images/disk2.png" );
    
  save->clicked().connect( boost::bind( &InterSpec::stateSaveTagAction,
                                         this, edit, window ) );
    
  window->setMinimumSize(WLength(450), WLength::Auto);
    
  window->centerWindow();
  window->show();
}//void stateSaveTag()


void InterSpec::stateSaveTagAction( WLineEdit *nameedit, AuxWindow *window )
{
  Dbo::ptr<UserState> state;
    
  if( m_currentStateID >= 0 )
  {
    try
    {
      DataBaseUtils::DbTransaction transaction( *m_sql );
      state = m_sql->session()->find<UserState>( "where id = ?" )
                               .bind( m_currentStateID );
      transaction.commit();
    }catch( std::exception &e )
    {
        cerr << "\nstartStoreStateInDb() caught: " << e.what() << endl;
    }//try / catch
  }//if( m_currentStateID >= 0 )
    
  //Save Snapshot/enviornment
  if( state )
    finishStoreStateInDb( nameedit, 0, 0, false, state );
  else
    passMessage( "You must first save a state before creating a snapshot.",
                 "", WarningWidget::WarningMsgHigh );
      
  //close window
  AuxWindow::deleteAuxWindow( window );
}//stateSaveTagAction()


//Action to Save As Snapshot
void InterSpec::stateSaveAsAction( WLineEdit *nameedit,
                                      WTextArea *descriptionarea,
                                      AuxWindow *window,
                                      const bool forTesting )
{
  //Need to remove the current foregorund/background/second from the file
  //  manager, and then re-add back in the SpecMeas so they will be saved to
  //  new database entries.  Otherwise saving the current state as, will create
  //  a new UserState that points to the current database entries for the
  //  SpecMeas
  for( int i = 0; i < 3; i++ )
  {
    std::shared_ptr<SpecMeas> m = measurment( SpectrumType(i) );
    if( m )
    {
      m_fileManager->removeSpecMeas( m, false );
      m_fileManager->addFile( m->filename(), m );
      m->setModified();
    }
  }//for( int i = 0; i < 3; i++ )
  
  //Save Snapshot/enviornment
  finishStoreStateInDb( nameedit, descriptionarea, NULL, forTesting, Dbo::ptr<UserState>() );
    
  //close window
  AuxWindow::deleteAuxWindow(window);
}//stateSaveAsAction()



//Note: Used indirectly by new combined snapshot method stateSave()
void InterSpec::startStoreStateInDb( const bool forTesting,
                                          const bool asNewState,
                                          const bool allowOverWrite,
                                          const bool endOfSessionNoDelete )
{
  Dbo::ptr<UserState> state;
  
  if( m_currentStateID >= 0 && !asNewState )
  {
      //If saving a tag, make sure to save to the parent.  We never overwrite TAGS
      try
      {
          DataBaseUtils::DbTransaction transaction( *m_sql );
          Dbo::ptr<UserState> childState  = m_sql->session()->find<UserState>( "where id = ? AND SnapshotTagParent_id >= 0" )
          .bind( m_currentStateID );
          
          if (childState)
          {
              state = m_sql->session()->find<UserState>( "where id = ?" )
              .bind( childState.get()->snapshotTagParent.id() );
          }
          
          transaction.commit();
      }catch( std::exception &e )
      {
          cerr << "\nstartStoreStateInDb() caught: " << e.what() << endl;
      }//try / catch

    if (!state)
    {
        try
        {
            DataBaseUtils::DbTransaction transaction( *m_sql );
            state = m_sql->session()->find<UserState>( "where id = ?" )
                           .bind( m_currentStateID );
            transaction.commit();
        }catch( std::exception &e )
        {
            cerr << "\nstartStoreStateInDb() caught: " << e.what() << endl;
        }//try / catch
    } //!state
  }//if( m_currentStateID >= 0 )
  
  if( state )
  { //Save without dialog
      
    const bool writeProtected = state->isWriteProtected();
    
    if( allowOverWrite && writeProtected )
    {
      DataBaseUtils::DbTransaction transaction( *m_sql );
      UserState::removeWriteProtection( state, m_sql->session() );
      transaction.commit();
    }//if( allowOverWrite && writeProtected )
    
      if (endOfSessionNoDelete)
      {
          DataBaseUtils::DbTransaction transaction( *m_sql );
          state.modify()->stateType = UserState::kEndOfSessionHEAD;
          transaction.commit();
      }
      
    try
    {
      saveStateToDb( state );
    }catch( std::exception &e )
    {
      passMessage( e.what(), "", WarningWidget::WarningMsgHigh );
    }//try / catch
    
    if( writeProtected )
    {
      DataBaseUtils::DbTransaction transaction( *m_sql );
      UserState::makeWriteProtected( state, m_sql->session() );
      transaction.commit();
    }//if( writeProtected )
    
    
    passMessage( "Saved state '" + state->name + "'", "",
                 WarningWidget::WarningMsgSave );
    return;
  }//if( state )
  
  AuxWindow *window = new AuxWindow( "Create Snapshot", (Wt::WFlags<AuxWindowProperties>(AuxWindowProperties::IsAlwaysModal) | AuxWindowProperties::TabletModal) );
  window->rejectWhenEscapePressed();
  window->finished().connect( boost::bind( &AuxWindow::deleteAuxWindow, window ) );
  window->setClosable( false );
  WGridLayout *layout = window->stretcher();
  WLineEdit *edit = new WLineEdit();
  edit->setEmptyText( "(Name to store under)" );
  WText *label = new WText( "Name" );
  layout->addWidget( label, 0, 0 );
  layout->addWidget( edit,  0, 1 );
  
  if( !!m_dataMeasurement && m_dataMeasurement->filename().size() )
    edit->setText( m_dataMeasurement->filename() );
  
  WTextArea *summary = new WTextArea();
  label = new WText( "Desc." );
  summary->setEmptyText( "(Optional description)" );
  layout->addWidget( label, 1, 0 );
  layout->addWidget( summary, 1, 1 );
  layout->setColumnStretch( 1, 1 );
  layout->setRowStretch( 1, 1 );
  
  
  WPushButton *closeButton = window->addCloseButtonToFooter("Cancel", false);
  closeButton->clicked().connect( boost::bind( &AuxWindow::deleteAuxWindow, window ) );

  WPushButton *save = new WPushButton( "Create" , window->footer() );
  save->setIcon( "InterSpec_resources/images/disk2.png" );
  
  save->clicked().connect( boost::bind( &InterSpec::finishStoreStateInDb,
                                       this, edit, summary, window, forTesting, Wt::Dbo::ptr<UserState>() ) );

  window->setMinimumSize(WLength(450), WLength(250));
  
  window->centerWindow();
  window->show();
}//void InterSpec::startStoreStateInDb()


//Deprecated - no longer used
void InterSpec::browseDatabaseStates( bool allowTests )
{
  new DbStateBrowser( this, allowTests );
}//void InterSpec::browseDatabaseStates()
#endif //#if( USE_DB_TO_STORE_SPECTRA )


#if( USE_DB_TO_STORE_SPECTRA && INCLUDE_ANALYSIS_TEST_SUITE )
void InterSpec::startStateTester()
{
  new SpectrumViewerTesterWindow( this );
}//void startStateTester()
#endif  //#if( INCLUDE_ANALYSIS_TEST_SUITE )




void InterSpec::addFileMenu( WWidget *parent, bool isMobile )
{
  if( m_fileMenuPopup )
    return;

  const bool showToolTipInstantly
         = InterSpecUser::preferenceValue<bool>( "ShowTooltips", this );
  
  PopupDivMenu *parentMenu = dynamic_cast<PopupDivMenu *>( parent );
  WContainerWidget *menuDiv = dynamic_cast<WContainerWidget *>( parent );
  if( !parentMenu && !menuDiv )
    throw runtime_error( "InterSpec::addFileMenu(): parent passed in must"
                         " be a PopupDivMenu  or WContainerWidget" );

  
  string menuname = "InterSpec";
#if( !defined(__APPLE__) && BUILD_AS_ELECTRON_APP && USE_ELECTRON_NATIVE_MENU )
  menuname = "File";
#else
  if( isMobile )
    menuname = "Spectra";
#endif
  
  if( menuDiv )
  {
    WPushButton *button = new WPushButton( menuname, menuDiv );
    button->addStyleClass( "MenuLabel" );
    m_fileMenuPopup = new PopupDivMenu( button, PopupDivMenu::AppLevelMenu );
    
//    button = new WPushButton( "Spectrum", menuDiv );
//    button->addStyleClass( "MenuLabel" );
//    m_fileMenuPopup = new PopupDivMenu( button, PopupDivMenu::AppLevelMenu );
  }else
  {
    m_fileMenuPopup = parentMenu->addPopupMenuItem( menuname );
//    m_fileMenuPopup = parentMenu->addPopupMenuItem( "Spectrum" );
  }//if( menuDiv ) / else

  PopupDivMenuItem *item = (PopupDivMenuItem *)0;
  
#if( defined(__APPLE__) && BUILD_AS_ELECTRON_APP && USE_ELECTRON_NATIVE_MENU )
  PopupDivMenuItem *aboutitem = m_fileMenuPopup->createAboutThisAppItem();
  
  aboutitem->triggered().connect( boost::bind( &InterSpec::showLicenseAndDisclaimersWindow, this, false, std::function<void()>{} ) );
  
  m_fileMenuPopup->addSeparator();
  m_fileMenuPopup->addRoleMenuItem( PopupDivMenu::MenuRole::Hide );
  m_fileMenuPopup->addRoleMenuItem( PopupDivMenu::MenuRole::HideOthers );
  m_fileMenuPopup->addRoleMenuItem( PopupDivMenu::MenuRole::UnHide );
  m_fileMenuPopup->addRoleMenuItem( PopupDivMenu::MenuRole::Front );
  m_fileMenuPopup->addSeparator();
#endif
  
  
  
#if( USE_DB_TO_STORE_SPECTRA )
  //  item = m_fileMenuPopup->addMenuItem( "Test Serialization" );
  //  item->triggered().connect( this, &InterSpec::testLoadSaveState );
  
  //----temporary
  //    PopupDivMenu *backupmenu = m_fileMenuPopup->addPopupMenuItem( "Backup", "InterSpec_resources/images/database_go.png" );
  
  //    m_fileMenuPopup->addSeparator();
  
  if( m_fileManager )
  {
    
    item = m_fileMenuPopup->addMenuItem( "Manager...", "InterSpec_resources/images/file_manager_small.png" );
    HelpSystem::attachToolTipOn(item, "Manage loaded spectra", showToolTipInstantly );
    
    item->triggered().connect( m_fileManager, &SpecMeasManager::startSpectrumManager );
    
    m_fileMenuPopup->addSeparator();
    
    const char *save_txt = "Store";           //"Save"
    const char *save_as_txt = "Store As...";  //"Save As..."
    const char *tag_txt = "Tag...";
    
    // --- new save menu ---
    m_saveState = m_fileMenuPopup->addMenuItem( save_txt );
    m_saveState->triggered().connect( boost::bind( &InterSpec::stateSave, this ) );
    HelpSystem::attachToolTipOn(m_saveState, "Saves the current app state to InterSpecs database", showToolTipInstantly );
    
    
    // --- new save as menu ---
    m_saveStateAs = m_fileMenuPopup->addMenuItem( save_as_txt );
    m_saveStateAs->triggered().connect( boost::bind( &InterSpec::stateSaveAs, this ) );
    HelpSystem::attachToolTipOn(m_saveStateAs, "Saves the current app state to a new listing in InterSpecs database", showToolTipInstantly );
    m_saveStateAs->setDisabled( true );
    
    // --- new save tag menu ---
    m_createTag = m_fileMenuPopup->addMenuItem( tag_txt , "InterSpec_resources/images/stop_time_small.png");
    m_createTag->triggered().connect( boost::bind(&InterSpec::stateSaveTag, this ));
    
    HelpSystem::attachToolTipOn(m_createTag, "Tags the current Interspec state so you "
                                "can revert to at a later time.  When loading an app state, "
                                "you can choose either the most recent save, or select past tagged versions.", showToolTipInstantly );
    
    m_fileMenuPopup->addSeparator();
    
    item = m_fileMenuPopup->addMenuItem( "Previous..." , "InterSpec_resources/images/db_small.png");
    item->triggered().connect( boost::bind( &SpecMeasManager::browseDatabaseSpectrumFiles, m_fileManager, "", (SpectrumType)0, std::shared_ptr<SpectraFileHeader>()) );
    HelpSystem::attachToolTipOn(item, "Opens previously saved states", showToolTipInstantly );
    
    
    if( isMobile )
    {
      item = m_fileMenuPopup->addMenuItem( "Loaded Spectra..." );
      item->triggered().connect( this, &InterSpec::showCompactFileManagerWindow );
    }//if( isMobile )
    
    m_fileMenuPopup->addSeparator();
  }//if( m_fileManager )
#endif  //USE_DB_TO_STORE_SPECTRA

  PopupDivMenu *subPopup = 0;

  subPopup = m_fileMenuPopup->addPopupMenuItem( "Samples" );
  
  item = subPopup->addMenuItem( "Ba-133 (16k channel)" );
  item->triggered().connect( boost::bind( &SpecMeasManager::loadFromFileSystem, m_fileManager,
                                         "example_spectra/ba133_source_640s_20100317.n42",
                                         kForeground, k2006Icd1Parser ) );
  if( isMobile )
    item = subPopup->addMenuItem( "Passthrough (16k channel)" );
  else
    item = subPopup->addMenuItem( "Passthrough (16k bin ICD1, 8 det., 133 samples)" );
  item->triggered().connect( boost::bind( &SpecMeasManager::loadFromFileSystem, m_fileManager,
                                         "example_spectra/passthrough.n42",
                                         kForeground, k2006Icd1Parser ) );
  
  item = subPopup->addMenuItem( "Background (16k bin N42)" );
  item->triggered().connect( boost::bind( &SpecMeasManager::loadFromFileSystem, m_fileManager,
                                         "example_spectra/background_20100317.n42",
                                         kBackground, k2006Icd1Parser ) );
  //If its a mobile device, we'll give a few more spectra to play with
  if( isMobile )
  {
    item = subPopup->addMenuItem( "Ba-133 (Low Res, No Calib)" );
    item->triggered().connect( boost::bind( &SpecMeasManager::loadFromFileSystem, m_fileManager,
                                           "example_spectra/Ba133LowResNoCalib.spe",
                                           kForeground, kIaeaParser ) );
    
    item = subPopup->addMenuItem( "Co-60 (Low Res, No Calib)" );
    item->triggered().connect( boost::bind( &SpecMeasManager::loadFromFileSystem, m_fileManager,
                                           "example_spectra/Co60LowResNoCalib.spe",
                                           kForeground, kIaeaParser ) );
    
    item = subPopup->addMenuItem( "Cs-137 (Low Res, No Calib)" );
    item->triggered().connect( boost::bind( &SpecMeasManager::loadFromFileSystem, m_fileManager,
                                           "example_spectra/Cs137LowResNoCalib.spe",
                                           kForeground, kIaeaParser ) );
    item = subPopup->addMenuItem( "Th-232 (Low Res, No Calib)" );
    item->triggered().connect( boost::bind( &SpecMeasManager::loadFromFileSystem, m_fileManager,
                                           "example_spectra/Th232LowResNoCalib.spe",
                                           kForeground, kIaeaParser ) );
  }//if( isMobile )
  
  if( isSupportFile() )
  {
    string tip = "Creates a dialog to browse for a spectrum file on your file system.";
    if( !isMobile )
      tip += " Drag and drop the file directly into the app window as a quicker alternative.";
    
    item = m_fileMenuPopup->addMenuItem( "Open File..." );
    HelpSystem::attachToolTipOn( item, tip, showToolTipInstantly );
    item->triggered().connect( boost::bind ( &SpecMeasManager::startQuickUpload, m_fileManager ) );
  } //!isSupportFile
  

#if( USE_SAVEAS_FROM_MENU )
  if( isSupportFile() )
  {
    //only display when not on desktop.
    m_downloadMenu = m_fileMenuPopup->addPopupMenuItem( "Export File" );
    
    for( SpectrumType i = SpectrumType(0); i<=kBackground; i = SpectrumType(i+1) )
    {
      m_downloadMenus[i] = m_downloadMenu->addPopupMenuItem( descriptionText( i ) );
      
      for( SaveSpectrumAsType j = SaveSpectrumAsType(0);
          j < kNumSaveSpectrumAsType; j = SaveSpectrumAsType(j+1) )
      {
        const string desc = descriptionText( j );
        item = m_downloadMenus[i]->addMenuItem( desc + " File" );
        DownloadCurrentSpectrumResource *resource
                     = new DownloadCurrentSpectrumResource( i, j, this, item );
        
#if( USE_OSX_NATIVE_MENU || (BUILD_AS_ELECTRON_APP && USE_ELECTRON_NATIVE_MENU) )
        //If were using OS X native menus, we obviously cant relly on the
        //  browser responding to a click on an anchor; we also cant click the
        //  link of the PopupDivMenuItem itself in javascript, or we get a
        //  cycle of the server telling the browser to click the link again,
        //  each time it tells it to do it in javascript.  So we will introduce
        //  a false menu item that will actually have the URL of the download
        //  associated with its anchor, and not connect any server side
        //  triggered() events to it, thus breaking the infiinite cycle - blarg.
        PopupDivMenuItem *fakeitem = new PopupDivMenuItem( "", "" );
        m_downloadMenus[i]->WPopupMenu::addItem( fakeitem );
        fakeitem->setLink( WLink( resource ) );
        fakeitem->setLinkTarget( TargetNewWindow );

        if( fakeitem->anchor() )
        {
          const string jsclick = "try{document.getElementById('"
                                 + fakeitem->anchor()->id()
                                 + "').click();}catch(e){}";
          item->triggered().connect( boost::bind( &WApplication::doJavaScript, wApp, jsclick, true ) );
        }else
        {
          cerr << "Unexpected error accessing the anchor for file downloading" << endl;
        }
#else
        item->setLink( WLink( resource ) );
        item->setLinkTarget( TargetNewWindow );
#endif
        
        const char *tooltip = 0;
        switch( j )
        {
           case kTxtSpectrumFile:
            tooltip = "A space delimited file will be created with a header of"
                      " things like live time, followed by three columns"
                      " (channel number, lower channel energy, counts). Each"
                      " record in the file will be sequentially recorded with"
                      " three line breaks between records.";
            break;
            
           case kCsvSpectrumFile:
            tooltip = "A comma delimietd file will be created with a channel"
                      " lower energy and a counts column.  All records in the"
                      " current spectrum file will be written sequentially,"
                      " with an extra line break and &quot;<b>Energy, Data</b>"
                      "&quot; header between subsequent records.";
            break;
            
           case kPcfSpectrumFile:
            tooltip = "A binary PCF file will be created which contains all"
                      " records of the current file.";
            break;
            
           case kXmlSpectrumFile:
            tooltip = "A simple spectromiter style 2006 N42 XML file will be"
                      " produced which contains all records in the current file.";
            break;
            
           case k2012N42SpectrumFile:
            tooltip = "A 2012 N42 XML document will be produced which contains"
                      " all samples of the current spectrum file, as well as"
                      " some additional <code>InterSpec</code> information"
                      " such as the identified peaks and detector response.";
            break;
            
           case kChnSpectrumFile:
            tooltip = "A binary integer CHN file will be produced containing a"
                      " single spectrum matching what is currently shown.";
            break;
            
           case kBinaryIntSpcSpectrumFile:
            tooltip = "A binary floating point SPC file will be produced containing a"
                      " single spectrum matching what is currently shown.";
            break;
           case kBinaryFloatSpcSpectrumFile:
            tooltip = "A binary integer SPC file will be produced containing a"
                      " single spectrum matching what is currently shown.";
            break;
            
          case kAsciiSpcSpectrumFile:
            tooltip = "A ascii based SPC file will be produced containing a"
                      " single spectrum matching what is currently shown.";
            break;
          
          case kExploraniumGr130v0SpectrumFile:
            tooltip = "A binary Exploranium GR130 file will be produced with"
                      " each record (spectrum) containing 256 channels.";
            break;
            
          case kExploraniumGr135v2SpectrumFile:
            tooltip = "A binary Exploranium GR135v2 file (includes neutron info)"
                      " will be produced with each record (spectrum) containing"
                      " 1024 channels.";
            break;
          
          case kIaeaSpeSpectrumFile:
            tooltip = "A ASCII based standard file format that will contain a"
                      " single spectrum only.";
            break;
#if( SpecUtils_ENABLE_D3_CHART )
          case kD3HtmlSpectrumFile:
            tooltip = "An HTML file using D3.js to generate a spectrum chart"
                          " that you can optionally interact with and view offline.";
            break;
#endif //#if( SpecUtils_ENABLE_D3_CHART )
           case kNumSaveSpectrumAsType:
            break;
        }//switch( j )
        
        
        const bool showInstantly = true;
        if( tooltip )
          HelpSystem::attachToolTipOn( item, tooltip, showInstantly,
                              HelpSystem::Right, HelpSystem::CanDelayShowing );
      }//for( loop over file types )
      
      m_downloadMenus[i]->disable();
      if( m_downloadMenus[i]->parentItem() )
        m_downloadMenu->setItemHidden( m_downloadMenus[i]->parentItem(), true );
    }//for( loop over spectrums )
    
    m_fileMenuPopup->setItemHidden(m_downloadMenu->parentItem(),true); //set as hidden first, will be visible when spectrum is added
  } //!isMobile
#elif( IOS )
  PopupDivMenuItem *exportFile = m_fileMenuPopup->addMenuItem( "Export Spectrum..." );
  exportFile->triggered().connect( this, &InterSpec::exportSpecFile );
#else
if (isSupportFile())
{
  item = m_fileMenuPopup->addMenuItem( "Export" );
  item->triggered().connect( m_fileManager, &SpecMeasManager::displayQuickSaveAsDialog );
}//isSupportFile()
    
#endif //#if( USE_SAVEAS_FROM_MENU )
  

  
#if( USE_DB_TO_STORE_SPECTRA )
  assert( !m_fileManager || m_saveStateAs );
  assert( !m_fileManager || m_saveState );
#endif //#if( USE_DB_TO_STORE_SPECTRA )
  
  
#if( INCLUDE_ANALYSIS_TEST_SUITE )
  m_fileMenuPopup->addSeparator();
  PopupDivMenu* testing = m_fileMenuPopup->addPopupMenuItem( "Testing" , "InterSpec_resources/images/testing.png");
  item = testing->addMenuItem( "Store App Test State..." );
  item->triggered().connect( boost::bind( &InterSpec::startStoreTestStateInDb, this ) );
  HelpSystem::attachToolTipOn(item,"Stores app state as part of the automated test suite", showToolTipInstantly );
  
  item = testing->addMenuItem( "Restore App Test State..." );
  item->triggered().connect( boost::bind(&InterSpec::browseDatabaseStates, this, true) );
  HelpSystem::attachToolTipOn(item, "Restores InterSpec to a previously stored state.", showToolTipInstantly );
  
  item = testing->addMenuItem( "Load N42 Test State..." );
  item->triggered().connect( boost::bind(&InterSpec::startN42TestStates, this) );
  HelpSystem::attachToolTipOn(item, "Loads a InterSpec specific variant of a "
                                    "2012 N42 file that contains Foreground, "
                                    "Background, User Settings, and Shielding/"
                                    "Source model.", showToolTipInstantly );
  
  item = testing->addMenuItem( "Show Testing Widget..." );
  item->triggered().connect( boost::bind(&InterSpec::startStateTester, this ) );
#endif //#if( INCLUDE_ANALYSIS_TEST_SUITE )


#if(USE_OSX_NATIVE_MENU )
    //Add a separtor before Quit InterSpec
    m_fileMenuPopup->addSeparator();
#endif

#if( BUILD_AS_ELECTRON_APP && USE_ELECTRON_NATIVE_MENU )
  m_fileMenuPopup->addSeparator();
  m_fileMenuPopup->addRoleMenuItem( PopupDivMenu::MenuRole::Quit );
#endif
  
} // void InterSpec::addFileMenu( WContainerWidget *menuDiv, bool show_examples )


bool InterSpec::toolTabsVisible() const
{
  return m_toolsTabs;
}

void InterSpec::addToolsTabToMenuItems()
{
  if( !m_toolsMenuPopup )
    return;
  
  if( m_tabToolsMenuItems[static_cast<int>(ToolTabMenuItems::RefPhotopeaks)] )
    return;
  
  bool showToolTipInstantly = false;
  
  try
  {
    showToolTipInstantly = InterSpecUser::preferenceValue<bool>( "ShowTooltips", this );
  }catch(...)
  {
  }
  
  const int index_offest = isMobile() ? 1 : 0;
  
  Wt::WMenuItem *item = nullptr;
  const char *tooltip = nullptr, *icon = nullptr;
  
#if( IOS || ANDROID )
  icon = "InterSpec_resources/images/reflines.svg";
#else
  icon = "InterSpec_resources/images/reflines.png";
#endif
  item = m_toolsMenuPopup->insertMenuItem( index_offest + 0, GammaLinesTabTitle, icon , true );
  item->triggered().connect( boost::bind( &InterSpec::showGammaLinesWindow, this ) );
  tooltip = "Allows user to display x-rays and/or gammas from elements, isotopes, or nuclear reactions."
            " Also provides user with a shortcut to change detector and account for shielding.";
  HelpSystem::attachToolTipOn( item, tooltip, showToolTipInstantly );
  m_tabToolsMenuItems[static_cast<int>(ToolTabMenuItems::RefPhotopeaks)] = item;
  
#if( IOS || ANDROID )
  icon = "InterSpec_resources/images/peakmanager.svg";
#else
  icon = "InterSpec_resources/images/peakmanager.png";
#endif
  item = m_toolsMenuPopup->insertMenuItem( index_offest + 1, PeakInfoTabTitle, icon, true );
  item->triggered().connect( this, &InterSpec::showPeakInfoWindow );
  tooltip = "Provides shortcuts to search for and identify peaks. Displays parameters of all fit peaks in a sortable table.";
  HelpSystem::attachToolTipOn( item, tooltip, showToolTipInstantly );
  m_tabToolsMenuItems[static_cast<int>(ToolTabMenuItems::PeakManager)] = item;
  
#if( IOS || ANDROID )
  icon = "InterSpec_resources/images/calibrate.svg";
#else
  icon = "InterSpec_resources/images/calibrate.png";
#endif
  item = m_toolsMenuPopup->insertMenuItem( index_offest + 2, CalibrationTabTitle, icon, true );
  item->triggered().connect( this, &InterSpec::showRecalibratorWindow );
  tooltip = "Allows user to modify or fit for offset, linear, or quadratic energy calibration terms,"
            " as well as edit non-linear deviation pairs.<br />"
            "Energy calibration can also be done graphically by right-click dragging the spectrum from original to modified energy"
            " (e.g., drag the data peak to the appropriate reference line).";
  HelpSystem::attachToolTipOn( item, tooltip, showToolTipInstantly );
  m_tabToolsMenuItems[static_cast<int>(ToolTabMenuItems::EnergyCal)] = item;
  
#if( IOS || ANDROID )
  icon = "InterSpec_resources/images/magnifier_black.svg";
#else
  //macOS (and probably Electron) need PNGs.
  icon = "InterSpec_resources/images/magnifier_black.png";
#endif
  item = m_toolsMenuPopup->insertMenuItem( index_offest + 3, NuclideSearchTabTitle, icon, true );
  item->triggered().connect( this, &InterSpec::showNuclideSearchWindow);
  tooltip = "Search for nuclides with constraints on energy, branching ratio, and half life.";
  HelpSystem::attachToolTipOn( item, tooltip, showToolTipInstantly );
  m_tabToolsMenuItems[static_cast<int>(ToolTabMenuItems::NuclideSearch)] = item;

  
#if( IOS || ANDROID )
  icon = "InterSpec_resources/images/auto_peak_search.svg";
#else
  icon = "InterSpec_resources/images/auto_peak_search.png";
#endif
  item = m_toolsMenuPopup->insertMenuItem( index_offest + 4, "Auto Peak Search", icon, true );
  item->triggered().connect( boost::bind( &PeakSearchGuiUtils::automated_search_for_peaks, this, true ) );
  m_tabToolsMenuItems[static_cast<int>(ToolTabMenuItems::AutoPeakSearch)] = item;
  
  
  item = m_toolsMenuPopup->addSeparatorAt( index_offest + 5 );
  
  m_tabToolsMenuItems[static_cast<int>(ToolTabMenuItems::Seperator)] = item;
}//void addToolsTabToMenuItems()


void InterSpec::removeToolsTabToMenuItems()
{
  if( !m_toolsMenuPopup )
    return;
  
  for( int i = 0; i < static_cast<int>(ToolTabMenuItems::NumItems); ++i )
  {
    if( !m_tabToolsMenuItems[i] )
      continue;
 
    if( m_tabToolsMenuItems[i]->isSeparator() )
      m_toolsMenuPopup->removeSeperator( m_tabToolsMenuItems[i] );
    
    delete m_tabToolsMenuItems[i];
    m_tabToolsMenuItems[i] = nullptr;
  }
  
}//void removeToolsTabToMenuItems()


void InterSpec::setToolTabsVisible( bool showToolTabs )
{
  if( static_cast<bool>(m_toolsTabs) == showToolTabs )
    return;
  
  m_toolTabsVisibleItems[0]->setHidden( showToolTabs );
  m_toolTabsVisibleItems[1]->setHidden( !showToolTabs );
  
  if( showToolTabs )
    removeToolsTabToMenuItems();
  else
    addToolsTabToMenuItems();
  
  const int layoutVertSpacing = isMobile() ? 10 : 5;
  
  if( showToolTabs )
  { //We are showing the tool tabs
    if( m_menuDiv )
      m_layout->removeWidget( m_menuDiv );
    m_layout->removeWidget( m_spectrum );
    m_layout->removeWidget( m_timeSeries );
    m_layout->clear();
    
    //We have to completely replace m_layout or else the vertical spacing
    //  changes wont quite trigger.  Prob a Wt bug, so should investigate again
    //  if we ever upgrade Wt.
    delete m_layout;
    m_layout = new WGridLayout();
    m_layout->setContentsMargins( 0, 0, 0, 0 );
    m_layout->setHorizontalSpacing( 0 );
    setLayout( m_layout );
    
    if( m_peakInfoWindow )
    {
      m_peakInfoWindow->contents()->removeWidget( m_peakInfoDisplay );
      delete m_peakInfoWindow;
      m_peakInfoWindow = NULL;
    }//if( m_peakInfoWindow )
    
    string refNucXmlState;
    if( m_referencePhotopeakLines
        && (!m_referencePhotopeakLines->currentlyShowingNuclide().empty()
             || m_referencePhotopeakLines->persistedNuclides().size()) )
    {
      m_referencePhotopeakLines->serialize( refNucXmlState );
    }
    
    closeGammaLinesWindow();
    
    if( m_recalibratorWindow )
    {
      m_recalibratorWindow->stretcher()->removeWidget( m_recalibrator );
      delete m_recalibratorWindow;
      m_recalibratorWindow = 0;
      m_recalibrator->setHidden(true);
    }//if( m_recalibratorWindow )
    
    m_toolsTabs = new WTabWidget();
    //m_toolsTabs->addStyleClass( "ToolsTabs" );
    
    CompactFileManager *compact = new CompactFileManager( m_fileManager, this, CompactFileManager::LeftToRight );
    m_toolsTabs->addTab( compact, FileTabTitle, TabLoadPolicy );
    
#if( USE_SPECTRUM_CHART_D3 )
    m_spectrum->yAxisScaled().connect( boost::bind( &CompactFileManager::handleSpectrumScale, compact, _1, _2 ) );
#endif
    
    //WMenuItem * peakManTab =
    m_toolsTabs->addTab( m_peakInfoDisplay, PeakInfoTabTitle, TabLoadPolicy );
//    const char *tooltip = "Displays parameters of all identified peaks in a sortable table.";
//    HelpSystem::attachToolTipOn( peakManTab, tooltip, showToolTipInstantly, HelpSystem::Top );
//    peakManTab->setIcon("InterSpec_resources/images/peakmanager.png");
    
    if( m_referencePhotopeakLines )
    {
      m_referencePhotopeakLines->clearAllLines();
      delete m_referencePhotopeakLines;
    }
      
    if( m_referencePhotopeakLinesWindow )
      delete m_referencePhotopeakLinesWindow;
    m_referencePhotopeakLinesWindow = NULL;
      
    m_referencePhotopeakLines = new ReferencePhotopeakDisplay( m_spectrum,
                                              m_materialDB.get(),
                                              m_shieldingSuggestion,
                                              this );
    setReferenceLineColors( nullptr );
    
    //PreLoading is necessary on the m_referencePhotopeakLines widget, so that the
    //  "Isotope Search" widget will work properly when a nuclide is clicked
    //  on to display its photpeaks
    //XXX In Wt 3.3.4 at least, the contents of m_referencePhotopeakLines
    //  are not actually loaded to the client until the tab is clicked, and I
    //  cant seem to get this to actually happen.
    //WMenuItem *refPhotoTab =
    m_toolsTabs->addTab( m_referencePhotopeakLines, GammaLinesTabTitle, TabLoadPolicy );
      
//      const char *tooltip = "Allows user to display x-rays and/or gammas from "
//                            "elements, isotopes, or nuclear reactions. Also "
//                            "provides user with a shortcut to change detector "
//                            "and account for shielding.";
//      HelpSystem::attachToolTipOn( refPhotoTab, tooltip, showToolTipInstantly, HelpSystem::Top );
      //refPhotoTab->setIcon("InterSpec_resources/images/reflines.png");
      
    m_toolsTabs->currentChanged().connect( this, &InterSpec::handleToolTabChanged );
    
    WGridLayout *gridLayout = new WGridLayout();
    m_recalibrator->setRecalibratorLayout(gridLayout);
    m_recalibrator->setHidden(false);
    m_recalibrator->setWideLayout(); //do this after unhiding to trigger Recalibrator::refreshRecalibrator();


    m_calibrateContainer->clear();
    m_calibrateContainer->setLayout(gridLayout);
    m_calibrateContainer->addStyleClass( "RecalibratorWide" );
    /*WMenuItem * calibTab = */ m_toolsTabs->addTab( m_calibrateContainer, CalibrationTabTitle, TabLoadPolicy );
    //calibTab->setIcon("InterSpec_resources/images/calibrate.png");
//    const char *tooltip = "Allows user to fit for offset, linear, and/or "
//                          "quadratic terms. Can also be accessed graphically by "
//                          "right-click dragging from original to modified energy.";
//    HelpSystem::attachToolTipOn( calibTab, tooltip, showToolTipInstantly , HelpSystem::Top);
    
    m_chartsLayout = new WGridLayout();
    m_chartsLayout->setContentsMargins( 0, 0, 0, 0 );
    if( m_timeSeries->isHidden() )
    {
      m_chartsLayout->setVerticalSpacing( 0 );
    }else
    {
      m_chartsLayout->setRowStretch( 0, 3 );
      m_chartsLayout->setRowStretch( 1, 2 );
      m_chartsLayout->setRowResizable( 0 );
      m_chartsLayout->setVerticalSpacing( layoutVertSpacing );
    }
    
    m_chartsLayout->setHorizontalSpacing( 0 );
    
    m_chartsLayout->addWidget( m_spectrum, 0, 0 );
    m_chartsLayout->addWidget( m_timeSeries, 1, 0 );
    
    m_toolsLayout = new WGridLayout();
    m_toolsLayout->setContentsMargins( 0, 0, 0, 0 );
    m_toolsLayout->setVerticalSpacing( 0 );
    m_toolsLayout->setHorizontalSpacing( 0 );
    m_toolsLayout->addWidget( m_toolsTabs, 0, 0 );
    
    int row = 0;
    if( m_menuDiv )
      m_layout->addWidget( m_menuDiv,  row++, 0 );
    m_layout->addLayout( m_chartsLayout, row, 0 );
    m_layout->setRowResizable( row, true );
    m_layout->setRowStretch( row, 5 );
    
    m_layout->setVerticalSpacing( layoutVertSpacing );
    if( m_menuDiv && !m_menuDiv->isHidden() )  //get rid of a small amoutn of space between the menu bar and the chart
      m_spectrum->setMargin( -layoutVertSpacing, Wt::Top );
    
    //Without using the wrapper below, the tabs widget will change height, even
    //  if it is explicitly set, when different tabs are clicked (unless a
    //  manual resize is performed by the user first)
    //m_layout->addLayout( toolsLayout,   ++row, 0 );
    WContainerWidget *toolsWrapper = new WContainerWidget();
    toolsWrapper->setLayout( m_toolsLayout );
    toolsWrapper->setHeight( 245 );
    m_layout->addWidget( toolsWrapper, ++row, 0 );
    m_layout->setRowStretch( row, 0 );
    
    if( m_nuclideSearchWindow )
    {
      m_isotopeSearch->clearSearchEnergiesOnClient();
      m_nuclideSearchWindow->stretcher()->removeWidget( m_isotopeSearch );
      delete m_nuclideSearchWindow;
      m_nuclideSearchWindow = 0;
    }//if( m_recalibratorWindow )
    
    assert( !m_isotopeSearchContainer );
    
    m_isotopeSearchContainer = new WContainerWidget();
    WGridLayout *isoSearchLayout = new WGridLayout();
    m_isotopeSearchContainer->setLayout( isoSearchLayout );
    isoSearchLayout->setContentsMargins( 0, 0, 0, 0 );
    isoSearchLayout->addWidget( m_isotopeSearch, 0, 0 );
    m_isotopeSearchContainer->setMargin( 0 );
    m_isotopeSearchContainer->setPadding( 0 );
    isoSearchLayout->setRowStretch( 0, 1 );
    isoSearchLayout->setColumnStretch( 0, 1 );
    
    //WMenuItem *nuclideTab =
    m_toolsTabs->addTab( m_isotopeSearchContainer, NuclideSearchTabTitle, TabLoadPolicy );
//    const char *tooltip = "Search for nuclides with constraints on energy, "
//                          "branching ratio, and half life.";
//    HelpSystem::attachToolTipOn( nuclideTab, tooltip, showToolTipInstantly, HelpSystem::Top );
    
    //nuclideTab->setIcon("InterSpec_resources/images/magnifier.png");
//    if( m_isotopeSearch )
//      m_isotopeSearch->loadSearchEnergiesToClient();
    
    //Make sure the current tab is the peak info display
    m_toolsTabs->setCurrentWidget( m_peakInfoDisplay );
    
    if( m_referencePhotopeakLines && refNucXmlState.size() )
      m_referencePhotopeakLines->deSerialize( refNucXmlState );
  }else
  {
    //We are hidding the tool tabs
    m_chartsLayout->removeWidget( m_spectrum );
    m_chartsLayout->removeWidget( m_timeSeries );
    
    if( m_menuDiv )
      m_layout->removeWidget( m_menuDiv );
    m_toolsTabs->removeTab( m_peakInfoDisplay );
    m_toolsTabs->removeTab( m_calibrateContainer );
    
    m_isotopeSearch->clearSearchEnergiesOnClient();
    m_isotopeSearchContainer->layout()->removeWidget( m_isotopeSearch );
    m_toolsTabs->removeTab( m_isotopeSearchContainer );
    delete m_isotopeSearchContainer;
    m_isotopeSearchContainer = nullptr;
    
    if( m_referencePhotopeakLines )
      m_referencePhotopeakLines->clearAllLines();
    
    m_toolsTabs = nullptr;
    m_chartsLayout = nullptr;
    if( !m_referencePhotopeakLinesWindow )
      m_referencePhotopeakLines = nullptr;
    m_toolsLayout = nullptr;
    
    m_layout->clear();
    
    //We have to completely replace m_layout or else the vertical spacing
    //  changes wont quite trigger.  Prob a Wt bug, so should investigate again
    //  if we ever upgrade Wt.
    //delete m_layout;
    m_layout = new WGridLayout();
    m_layout->setContentsMargins( 0, 0, 0, 0 );
    m_layout->setHorizontalSpacing( 0 );
    setLayout( m_layout );
    
    int row = -1;
    if( m_menuDiv )
      m_layout->addWidget( m_menuDiv, ++row, 0 );
    m_layout->addWidget( m_spectrum, ++row, 0 );
    m_layout->setRowStretch( row, 5 );
    m_layout->setRowResizable( row );
    m_layout->addWidget( m_timeSeries, ++row, 0 );
    m_layout->setRowStretch( row, 3 );
    if( m_timeSeries->isHidden() )
    {
      m_layout->setVerticalSpacing( 0 );
      if( m_menuDiv && !m_menuDiv->isHidden() )
        m_spectrum->setMargin( 0, Wt::Top );
    }else
    {
      const int vertSpacing = isMobile() ? 10 : 5;
      m_layout->setVerticalSpacing( vertSpacing );
      if( m_menuDiv && !m_menuDiv->isHidden() )
        m_spectrum->setMargin( -vertSpacing, Wt::Top );
    }
  }//if( showToolTabs ) / else
  
  if( m_toolsTabs )
    m_currentToolsTab = m_toolsTabs->currentIndex();
  else
    m_currentToolsTab = -1;
    
    
    if (m_mobileBackButton && m_mobileForwardButton)
    {
        if (!toolTabsVisible() && m_dataMeasurement && !m_dataMeasurement->passthrough()
             && m_dataMeasurement->sample_numbers().size()> 1)
        {
            m_mobileBackButton->setHidden(false);
            m_mobileForwardButton->setHidden(false);
        }
        else
        {
            m_mobileBackButton->setHidden(true);
            m_mobileForwardButton->setHidden(true);
            
        }
    }
  
#if( USE_SPECTRUM_CHART_D3 )
  //Not sure _why_ this next statement is needed, but it is, or else the
  //  spectrum chart shows up with no data
  m_spectrum->updateData();
#endif
}//void setToolTabsVisible( bool showToolTabs )


void InterSpec::addDisplayMenu( WWidget *parent )
{
  PopupDivMenu *parentMenu = dynamic_cast<PopupDivMenu *>( parent );
  WContainerWidget *menuDiv = dynamic_cast<WContainerWidget *>( parent );
  if( !parentMenu && !menuDiv )
    throw runtime_error( "InterSpec::addDisplayMenu(): parent passed in"
                        " must be a PopupDivMenu  or WContainerWidget" );
 
  const bool showToolTipInstantly
         = InterSpecUser::preferenceValue<bool>( "ShowTooltips", this );
  
  if( menuDiv )
  {
    WPushButton *button = new WPushButton( "View", menuDiv );
    button->addStyleClass( "MenuLabel" );
    m_displayOptionsPopupDiv = new PopupDivMenu( button, PopupDivMenu::AppLevelMenu );
  }else
  {
    m_displayOptionsPopupDiv = parentMenu->addPopupMenuItem( "View" );
  }//if( menuDiv ) / else
  
  m_toolTabsVisibleItems[0] = m_displayOptionsPopupDiv->addMenuItem( "Show Tool Tabs" , "InterSpec_resources/images/dock_small.png" );
  m_toolTabsVisibleItems[1] = m_displayOptionsPopupDiv->addMenuItem( "Hide Tool Tabs" , "InterSpec_resources/images/undock_small.png" );
  m_toolTabsVisibleItems[0]->triggered().connect( boost::bind( &InterSpec::setToolTabsVisible, this, true ) );
  m_toolTabsVisibleItems[1]->triggered().connect( boost::bind( &InterSpec::setToolTabsVisible, this, false ) );

  
  if (isPhone())
  {
      //hide Dock mode on small phone screens
      m_toolTabsVisibleItems[0]->setHidden(true);
  } //isPhone
    
  m_toolTabsVisibleItems[1]->setHidden(true);
  
  m_displayOptionsPopupDiv->addSeparator();

  PopupDivMenu *chartmenu = m_displayOptionsPopupDiv->addPopupMenuItem( "Chart Options" , "InterSpec_resources/images/spec_settings_small.png");
  
  addPeakLabelSubMenu( chartmenu ); //add Peak menu
  
  chartmenu->addSeparator();
  
  
  const bool logypref = m_user->preferenceValue<bool>( "LogY" );
  m_logYItems[0] = chartmenu->addMenuItem( "Log Y Scale" );
  m_logYItems[1] = chartmenu->addMenuItem( "Linear Y Scale" );
  m_logYItems[0]->setHidden( logypref );
  m_logYItems[1]->setHidden( !logypref );
  m_logYItems[0]->triggered().connect( boost::bind( &InterSpec::setLogY, this, true  ) );
  m_logYItems[1]->triggered().connect( boost::bind( &InterSpec::setLogY, this, false ) );
  m_spectrum->setYAxisLog( logypref );

  
  const bool verticleLines = InterSpecUser::preferenceValue<bool>( "ShowVerticalGridlines", this );
  m_verticalLinesItems[0] = chartmenu->addMenuItem( "Show Vertical Lines" , "InterSpec_resources/images/sc_togglegridvertical.png");
  m_verticalLinesItems[1] = chartmenu->addMenuItem( "Hide Vertical Lines" , "InterSpec_resources/images/sc_togglegridvertical.png");
  m_verticalLinesItems[0]->triggered().connect( boost::bind( &InterSpec::setVerticalLines, this, true ) );
  m_verticalLinesItems[1]->triggered().connect( boost::bind( &InterSpec::setVerticalLines, this, false ) );
  m_verticalLinesItems[0]->setHidden( verticleLines );
  m_verticalLinesItems[1]->setHidden( !verticleLines );
  m_spectrum->showVerticalLines( verticleLines );
  m_timeSeries->showVerticalLines( verticleLines );
  
  const bool horizontalLines = InterSpecUser::preferenceValue<bool>( "ShowHorizontalGridlines", this );
  m_horizantalLinesItems[0] = chartmenu->addMenuItem( "Show Horizontal Lines" , "InterSpec_resources/images/sc_togglegridhorizontal.png");
  m_horizantalLinesItems[1] = chartmenu->addMenuItem( "Hide Horizontal Lines" , "InterSpec_resources/images/sc_togglegridhorizontal.png");
  m_horizantalLinesItems[0]->triggered().connect( boost::bind( &InterSpec::setHorizantalLines, this, true ) );
  m_horizantalLinesItems[1]->triggered().connect( boost::bind( &InterSpec::setHorizantalLines, this, false ) );
  m_horizantalLinesItems[0]->setHidden( horizontalLines );
  m_horizantalLinesItems[1]->setHidden( !horizontalLines );
  m_spectrum->showHorizontalLines( horizontalLines );
  m_timeSeries->showHorizontalLines( horizontalLines );
  
#if( USE_SPECTRUM_CHART_D3 )
  const bool showSlider = InterSpecUser::preferenceValue<bool>( "ShowXAxisSlider", this );
  m_spectrum->showXAxisSliderChart( showSlider );
  m_showXAxisSliderItems[0] = chartmenu->addMenuItem( "Show Energy Slider" , "");
  m_showXAxisSliderItems[1] = chartmenu->addMenuItem( "Hide Energy Slider" , "");
  m_showXAxisSliderItems[0]->triggered().connect( boost::bind( &InterSpec::setXAxisSlider, this, true ) );
  m_showXAxisSliderItems[1]->triggered().connect( boost::bind( &InterSpec::setXAxisSlider, this, false ) );
  m_showXAxisSliderItems[0]->setHidden( showSlider );
  m_showXAxisSliderItems[1]->setHidden( !showSlider );
  
  
  const bool showScalers = InterSpecUser::preferenceValue<bool>( "ShowYAxisScalers", this );
  m_spectrum->showYAxisScalers( showScalers );
  
  m_showYAxisScalerItems[0] = chartmenu->addMenuItem( "Show Y-Axis Scalers" , "");
  m_showYAxisScalerItems[1] = chartmenu->addMenuItem( "Hide Y-Axis Scalers" , "");
  m_showYAxisScalerItems[0]->triggered().connect( boost::bind( &InterSpec::setShowYAxisScalers, this, true ) );
  m_showYAxisScalerItems[1]->triggered().connect( boost::bind( &InterSpec::setShowYAxisScalers, this, false ) );
  m_showYAxisScalerItems[0]->setHidden( showScalers );
  m_showYAxisScalerItems[1]->setHidden( !showScalers );
  
  
  if( isPhone() )
  {
    m_compactXAxisItems[0] = m_compactXAxisItems[1] = nullptr;
  }else
  {
    const bool makeCompact = InterSpecUser::preferenceValue<bool>( "CompactXAxis", this );
    m_spectrum->setCompactAxis( makeCompact );
    m_compactXAxisItems[0] = chartmenu->addMenuItem( "Compact X-Axis" , "");
    m_compactXAxisItems[1] = chartmenu->addMenuItem( "Normal X-Axis" , "");
    m_compactXAxisItems[0]->triggered().connect( boost::bind( &InterSpec::setXAxisCompact, this, true ) );
    m_compactXAxisItems[1]->triggered().connect( boost::bind( &InterSpec::setXAxisCompact, this, false ) );
    m_compactXAxisItems[0]->setHidden( makeCompact );
    m_compactXAxisItems[1]->setHidden( !makeCompact );
  }
#endif
  
  //What we should do here is have a dialog that pops up that lets users  select
  //  colors for foreground, background, secondary, as well as the first number
  //  of reference lines.
  //  Probably also chart background, chart text, chart axis.  Should allow
  //  users to save these as "themes", as well as include a few by default...
  //Probably should also include an option to say whether fitted phtopeaks
  //  should adopt the color of the current refernce photopeak lines
  
  //With the below, it seems the color of the widget never actually gets
  //  changed... also, it wouldnt be supported on native menus.
  //chartmenu->addSeparator();
  //ColorSelect *color = new ColorSelect(ColorSelect::PrefferNative);
  //chartmenu->addWidget( color );
  //color->cssColorChanged().connect(<#const std::string &function#>)
  
  chartmenu->addSeparator();

  m_backgroundSubItems[0] = chartmenu->addMenuItem( "Background Subtract" );
  m_backgroundSubItems[1] = chartmenu->addMenuItem( "Un-Background Subtract" );
  m_backgroundSubItems[0]->triggered().connect( boost::bind( &InterSpec::setBackgroundSub, this, true ) );
  m_backgroundSubItems[1]->triggered().connect( boost::bind( &InterSpec::setBackgroundSub, this, false ) );
  m_backgroundSubItems[0]->disable();
  m_backgroundSubItems[1]->hide();
  
  PopupDivMenuItem *item = chartmenu->addMenuItem( "Show Spectrum Legend" );
  m_spectrum->legendDisabled().connect( item, &PopupDivMenuItem::show );
  m_spectrum->legendEnabled().connect( item,  &PopupDivMenuItem::hide );
  item->triggered().connect( boost::bind(
#if ( USE_SPECTRUM_CHART_D3 )
                                         &D3SpectrumDisplayDiv::enableLegend,
#else
                                         &SpectrumDisplayDiv::enableLegend,
#endif
                                         m_spectrum,
                                         false ) );
  item->hide(); //we are already showing the legend

  item = chartmenu->addMenuItem( "Show Time Legend" );
  m_timeSeries->legendDisabled().connect( item, &PopupDivMenuItem::show );
  m_timeSeries->legendEnabled().connect( item,  &PopupDivMenuItem::hide );
  item->triggered().connect( boost::bind( &SpectrumDisplayDiv::enableLegend, m_timeSeries, false ) );
  item->hide();
  
#if( USE_GOOGLE_MAP )
  m_mapMenuItem = m_displayOptionsPopupDiv->addMenuItem( "Map","InterSpec_resources/images/map_small.png" );
  m_mapMenuItem->triggered().connect( boost::bind( &InterSpec::createMapWindow, this, kForeground ) );
  m_mapMenuItem->disable();
  HelpSystem::attachToolTipOn( m_mapMenuItem,
                    "Show measurment(s) location on a Google Map. Only enabled"
                    " when GPS coordinates are available.", showToolTipInstantly );
#endif
  
#if( USE_SEARCH_MODE_3D_CHART )
  m_searchMode3DChart = m_displayOptionsPopupDiv->addMenuItem( "Show 3D View","" );
  m_searchMode3DChart->triggered().connect( boost::bind( &InterSpec::create3DSearchModeChart, this ) );
  m_searchMode3DChart->disable();
  HelpSystem::attachToolTipOn( m_searchMode3DChart,
                        "Shows Time vs. Energy vs. Counts view for search mode or RPM data.", showToolTipInstantly );
#endif
  
  addDetectorMenu( m_displayOptionsPopupDiv );
  
#if( !USE_SPECTRUM_CHART_D3 )
  CanvasForDragging *overlay = m_spectrum->overlayCanvas();
  if( overlay )
#endif
  {
    string js;
    PopupDivMenu *submenu = NULL;
    
    submenu = m_displayOptionsPopupDiv->addPopupMenuItem( "Feature Markers" );
    
    //We have to put all the below checkboxes into a div, inorder to have the
    //  tool-tip work with the mouse anywhere over the item, otherwise only the
    //  check-box itself will have the tool-tip, not the label
    
    
    WCheckBox* cb = new WCheckBox( "Escape Peaks" );
    cb->setChecked(false);
    PopupDivMenuItem *item = submenu->addWidget( cb );

    HelpSystem::attachToolTipOn(item, "Show energy of single and double escapes at"
                     " E-511 keV and E-1022 keV after a pair creation"
                     " event happened in the detector", showToolTipInstantly );
    
    cb->checked().connect( boost::bind( &InterSpec::setFeatureMarkerOption, this, EscapePeakMarker, true ) );
    cb->unChecked().connect( boost::bind( &InterSpec::setFeatureMarkerOption, this, EscapePeakMarker, false ) );
    
#if( USE_SPECTRUM_CHART_D3 )
    cb->checked().connect( boost::bind( &D3SpectrumDisplayDiv::setFeatureMarkerOption, m_spectrum, EscapePeakMarker, true ) );
    cb->unChecked().connect( boost::bind( &D3SpectrumDisplayDiv::setFeatureMarkerOption, m_spectrum, EscapePeakMarker, false ) );
#else
    const string can = "$('#c" + overlay->id() + "')";
    js = "function(s,e){try{"+can+".data('escpeaks',s.checked);}catch(err){}}";
    
    std::shared_ptr<JSlot> jsslot = std::make_shared<JSlot>( js, cb );
    m_unNamedJSlots.push_back( jsslot );
    cb->changed().connect( *jsslot );
    
#if( USE_OSX_NATIVE_MENU || (BUILD_AS_ELECTRON_APP && USE_ELECTRON_NATIVE_MENU) )
    js = "try{"+can+".data('escpeaks',true);}catch(err){}";
    cb->checked().connect( boost::bind(&WApplication::doJavaScript, wApp, js, false) );
    js = "try{"+can+".data('escpeaks',false);}catch(err){}";
    cb->unChecked().connect( boost::bind(&WApplication::doJavaScript, wApp, js, false) );
#endif
#endif
  
    
#if( USE_OSX_NATIVE_MENU || (BUILD_AS_ELECTRON_APP && USE_ELECTRON_NATIVE_MENU)  )
    cerr << "\n\n\nCompton angle not yet implemented for macOS or Electron Native Menus\n\n" << endl;
#else
    cb = new WCheckBox( "Comp. Peak" );
    cb->setChecked(false);
    item = submenu->addWidget( cb );

    HelpSystem::attachToolTipOn(item, "Show energy of photons which compton scattered"
                     " through the given angle before reaching the"
                     " detector", showToolTipInstantly );
    WContainerWidget *angleDiv = new WContainerWidget( item );
    angleDiv->clicked().preventPropagation();
    angleDiv->clicked().preventDefaultAction();
    WLabel *label = new WLabel( "Angle", angleDiv );
    label->setMargin( WLength(1.8,WLength::FontEm), Wt::Left );
    WSpinBox *spin = new WSpinBox( angleDiv );
    spin->setTextSize( 3 );
    label->setBuddy( spin );
    spin->setRange( 0, 180 );
    spin->setValue( 180 );
    
    cb->checked().connect( boost::bind( &InterSpec::setFeatureMarkerOption, this, ComptonPeakMarker, true ) );
    cb->unChecked().connect( boost::bind( &InterSpec::setFeatureMarkerOption, this, ComptonPeakMarker, false ) );
    
#if( USE_SPECTRUM_CHART_D3 )
    //For some reason the signal through c++ always gives a value of 180 - so as
    //  a hack, we'll just do it in JS.
    //spin->valueChanged().connect( boost::bind( &D3SpectrumDisplayDiv::setComptonPeakAngle, m_spectrum, _1 ) );
    js = "function(s,e){try{" + m_spectrum->jsRef() + ".chart.setComptonPeakAngle(s.value);}catch(err){}}";
    spin->changed().connect( js );
    m_spectrum->setComptonPeakAngle( spin->value() );
    
    cb->checked().connect( boost::bind( &D3SpectrumDisplayDiv::setFeatureMarkerOption, m_spectrum, ComptonPeakMarker, true ) );
    cb->unChecked().connect( boost::bind( &D3SpectrumDisplayDiv::setFeatureMarkerOption, m_spectrum, ComptonPeakMarker, false ) );
#else
    js = "function(s,e){try{" + can + ".data('compangle', s.value);}catch(err){}}";
    jsslot = std::make_shared<JSlot>( js, spin );
    m_unNamedJSlots.push_back( jsslot );
    spin->changed().connect( *jsslot );
    
    js = "function(s,e){try{"
    "var spin = Wt.WT.getElement('" + spin->id() + "');"
    "if(spin && s.checked)"
    + can + ".data('compangle', spin.value);"
    "else "
    + can + ".data('compangle', null);"
    "}catch(err){}}";
    jsslot = std::make_shared<JSlot>( js, spin );
    m_unNamedJSlots.push_back( jsslot );
    cb->checked().connect( *jsslot );
    cb->unChecked().connect( *jsslot );
#endif
    
    spin->disable();
    cb->unChecked().connect( spin, &WSpinBox::disable );
    cb->checked().connect( spin, &WSpinBox::enable );
#endif //USE_OSX_NATIVE_MENU / else
    
    cb = new WCheckBox( "Compton Edge" );
    cb->setChecked(false);
    item = submenu->addWidget( cb );


	HelpSystem::attachToolTipOn(item, "Maximum energy detected (&#952; = 180 &#176;) for photon which"
                     " interacted once in the detector via a compton"
                     " interaction", showToolTipInstantly );
    
    //    checkbox->setToolTip( "Maximum energy detected (&theta;=180&deg;) for photon which interacted once in the detector via a compton interaction", XHTMLText ); //\u03B8=180\u00B0

    cb->checked().connect( boost::bind( &InterSpec::setFeatureMarkerOption, this, ComptonEdgeMarker, true ) );
    cb->unChecked().connect( boost::bind( &InterSpec::setFeatureMarkerOption, this, ComptonEdgeMarker, false ) );
    
#if( USE_SPECTRUM_CHART_D3 )
    cb->checked().connect( boost::bind( &D3SpectrumDisplayDiv::setFeatureMarkerOption, m_spectrum, ComptonEdgeMarker, true ) );
    cb->unChecked().connect( boost::bind( &D3SpectrumDisplayDiv::setFeatureMarkerOption, m_spectrum, ComptonEdgeMarker, false ) );
#else
    js = "function(s,e){try{"+can+".data('compedge',s.checked);}catch(err){}}";
    jsslot = std::make_shared<JSlot>( js, cb );
    m_unNamedJSlots.push_back( jsslot );
    cb->changed().connect( *jsslot );
    
#if( USE_OSX_NATIVE_MENU  || (BUILD_AS_ELECTRON_APP && USE_ELECTRON_NATIVE_MENU) )
    js = "try{"+can+".data('compedge',true);}catch(err){}";
    cb->checked().connect( boost::bind(&WApplication::doJavaScript, wApp, js, false) );
    js = "try{"+can+".data('compedge',false);}catch(err){}";
    cb->unChecked().connect( boost::bind(&WApplication::doJavaScript, wApp, js, false) );
#endif
#endif
    
    
    cb = new WCheckBox( "Sum Peak" );
    cb->setChecked(false);
    item = submenu->addWidget( cb );
    
    HelpSystem::attachToolTipOn(item, "Energy of peak due to random summing of coincident"
                     " photopeak gammas.", showToolTipInstantly );
    
    cb->checked().connect( boost::bind( &InterSpec::setFeatureMarkerOption, this, SumPeakMarker, true ) );
    cb->unChecked().connect( boost::bind( &InterSpec::setFeatureMarkerOption, this, SumPeakMarker, false ) );
#if( USE_SPECTRUM_CHART_D3 )
    cb->checked().connect( boost::bind( &D3SpectrumDisplayDiv::setFeatureMarkerOption, m_spectrum, SumPeakMarker, true ) );
    cb->unChecked().connect( boost::bind( &D3SpectrumDisplayDiv::setFeatureMarkerOption, m_spectrum, SumPeakMarker, false ) );
#else
    js = "function(s,e){try{"+can+".data('sumpeak',s.checked);"
    +can+".data('sumpeakclick',null);}catch(err){}}";
    jsslot = std::make_shared<JSlot>( js, cb );
    m_unNamedJSlots.push_back( jsslot );
    cb->changed().connect( *jsslot );

#if( USE_OSX_NATIVE_MENU  || (BUILD_AS_ELECTRON_APP && USE_ELECTRON_NATIVE_MENU)  )
    js = "try{"+can+".data('sumpeak',true);"+can+".data('sumpeakclick',null);}catch(err){}";
    cb->checked().connect( boost::bind(&WApplication::doJavaScript, wApp, js, false) );
    js = "try{"+can+".data('sumpeak',false);}catch(err){}";
    cb->unChecked().connect( boost::bind(&WApplication::doJavaScript, wApp, js, false) );
#endif
#endif
    
    //If didnt want to use JSlot, could do...
    //    js = can + ".data('compangle',null);";
    //    checkbox->checked().connect( boost::bind( &WApplication::doJavaScript, wApp, js, true ) );
    
#if ( USE_SPECTRUM_CHART_D3 )
    //ToDo: add ability to save <svg> to PNG.
    //  https://stackoverflow.com/questions/5433806/convert-embedded-svg-to-png-in-place
    //  or https://stackoverflow.com/questions/11567668/svg-to-canvas-with-d3-js/23667012#23667012
    //  or http://dinbror.dk/blog/how-to-download-an-inline-svg-as-jpg-or-png/
#elif( !IOS )
    //Add a download link to convert the canvas into a PNG and download it.
    m_displayOptionsPopupDiv->addSeparator();
    auto saveitem = m_displayOptionsPopupDiv->addMenuItem( "Save Spectrum as PNG" );
    saveitem->triggered().connect( boost::bind(&InterSpec::saveChartToPng, this, true) );
#endif  //USE_SPECTRUM_CHART_D3 / else
    
    chartmenu->addSeparator();
  }//if( overlay )
  
#if( BUILD_AS_ELECTRON_APP && USE_ELECTRON_NATIVE_MENU )
  m_displayOptionsPopupDiv->addSeparator();
  m_displayOptionsPopupDiv->addRoleMenuItem( PopupDivMenu::MenuRole::ToggleFullscreen );
  m_displayOptionsPopupDiv->addSeparator();
  m_displayOptionsPopupDiv->addRoleMenuItem( PopupDivMenu::MenuRole::ResetZoom );
  m_displayOptionsPopupDiv->addRoleMenuItem( PopupDivMenu::MenuRole::ZoomIn );
  m_displayOptionsPopupDiv->addRoleMenuItem( PopupDivMenu::MenuRole::ZoomOut );
#if( PERFORM_DEVELOPER_CHECKS )
  m_displayOptionsPopupDiv->addSeparator();
  m_displayOptionsPopupDiv->addRoleMenuItem( PopupDivMenu::MenuRole::ToggleDevTools );
#endif
#endif
} // void addDisplayMenu( menuParentDiv )


void InterSpec::addDetectorMenu( WWidget *menuWidget )
{
  if( m_detectorToShowMenu )
    return;
  
  PopupDivMenu *parentPopup = dynamic_cast<PopupDivMenu *>( menuWidget );

  if( parentPopup )
  {
    m_detectorToShowMenu = parentPopup->addPopupMenuItem( "Detectors" /*, "InterSpec_resources/images/detector_small_white.png"*/ );
  }else
  {
    WContainerWidget *menuDiv = dynamic_cast<WContainerWidget *>(menuWidget);
    if( !menuDiv )
      throw runtime_error( "addDetectorMenu: serious error" );
    
    WPushButton *button = new WPushButton( "Detectors", menuDiv );
    button->addStyleClass( "MenuLabel" );
    m_detectorToShowMenu = new PopupDivMenu( button, PopupDivMenu::AppLevelMenu );
  }//if( parentPopup )
  
  if( m_detectorToShowMenu->parentItem() )
    m_detectorToShowMenu->parentItem()->disable();
//  PopupDivMenuItem *item = m_detectorToShowMenu->addMenuItem( "Energy Calibration" );
//  item->triggered().connect( boost::bind( &WDialog::setHidden, m_recalibratorWindow, false, WAnimation() ) );
//  item->triggered().connect( boost::bind( &InterSpec::showRecalibratorWindow, this ) );
}//void addDetectorMenu( WContainerWidget *menuDiv )


void InterSpec::handRecalibratorWindowClose()
{
  if( !m_recalibratorWindow || !m_recalibrator )
    return;
  
  m_recalibrator->setHidden(true);
  
  if( !m_toolsTabs )
  {
    m_recalibratorWindow->stretcher()->removeWidget( m_recalibrator );
    delete m_recalibratorWindow;
    m_recalibratorWindow = 0;
    return;
  }//if( !m_toolsTabs )
  
  
  WGridLayout *layout = m_recalibratorWindow->stretcher();
  layout->removeWidget( m_recalibrator );

  WGridLayout * gridLayout = new WGridLayout();
  m_recalibrator->setRecalibratorLayout(gridLayout);
  m_calibrateContainer->clear();
  m_calibrateContainer->setLayout(gridLayout);

  if( m_toolsTabs->indexOf( m_calibrateContainer ) < 0 )
    m_toolsTabs->addTab( m_calibrateContainer, CalibrationTabTitle, TabLoadPolicy );
  
  m_currentToolsTab = m_toolsTabs->currentIndex();
  
  m_recalibrator->setWideLayout();
  delete m_recalibratorWindow;
  m_recalibratorWindow = NULL;
}//void handRecalibratorWindowClose()


void InterSpec::showRecalibratorWindow()
{
  m_recalibrator->setHidden(false);
  
  if( m_recalibratorWindow && !m_toolsTabs )
  {
    m_recalibratorWindow->show();
    m_recalibrator->refreshRecalibrator();
    return;
  }

  const int index = (m_toolsTabs ? m_toolsTabs->indexOf( m_calibrateContainer ) : -1);
  
  if( !m_recalibratorWindow || index >= 0 )
  {
    if( index >= 0 )
    {
      m_toolsTabs->removeTab( m_calibrateContainer );
    }//if( index >= 0 )
    
    if( m_recalibratorWindow )
      delete m_recalibratorWindow;
    
    m_recalibratorWindow = new AuxWindow( "Energy Calibration" );
    m_recalibratorWindow->rejectWhenEscapePressed();
    WContainerWidget *container = new WContainerWidget( m_recalibratorWindow->contents() );
    container->addStyleClass( "RecalibratorTall" );
    WGridLayout *layout = new WGridLayout();
    container->setLayout( layout );
    container->setOverflow( WContainerWidget::OverflowHidden );
    container->setMinimumSize( WLength::Auto, 600 );
    
    m_recalibrator->setRecalibratorLayout( layout );
    m_recalibrator->setTallLayout(m_recalibratorWindow);
   
    m_recalibratorWindow->footer()->addWidget(m_recalibrator->getFooter());
    m_recalibratorWindow->setClosable(false);
    m_recalibratorWindow->finished().connect(boost::bind( &AuxWindow::deleteAuxWindow, m_recalibratorWindow ) );

    m_recalibratorWindow->setHeight( 700 );
  }//if( m_toolsTabs->removeTab( m_recalibrator ) )
  
  m_recalibratorWindow->show();
  m_recalibratorWindow->resizeToFitOnScreen();
  m_recalibratorWindow->centerWindow();

//  m_recalibratorWindow->rejectWhenEscapePressed();
  
  if( m_toolsTabs )
    m_currentToolsTab = m_toolsTabs->currentIndex();
  m_recalibrator->refreshRecalibrator();
}//void showRecalibratorWindow()



void InterSpec::setLogY( bool logy )
{
  InterSpecUser::setPreferenceValue<bool>( m_user, "LogY", logy, this );
  m_logYItems[0]->setHidden( logy );
  m_logYItems[1]->setHidden( !logy );
  m_spectrum->setYAxisLog( logy );
}//void setLogY( bool logy )


void InterSpec::setBackgroundSub( bool subtract )
{
  m_backgroundSubItems[0]->setHidden( subtract );
  m_backgroundSubItems[1]->setHidden( !subtract );
  m_spectrum->setBackgroundSubtract( subtract );
}//void setBackgroundSub( bool sub )


void InterSpec::setVerticalLines( bool show )
{
  InterSpecUser::setPreferenceValue<bool>( m_user, "ShowVerticalGridlines", show, this );
  m_verticalLinesItems[0]->setHidden( show );
  m_verticalLinesItems[1]->setHidden( !show );
  m_spectrum->showVerticalLines( show );
  m_timeSeries->showVerticalLines( show );
}//void setVerticalLines( bool show )


void InterSpec::setHorizantalLines( bool show )
{
  InterSpecUser::setPreferenceValue<bool>( m_user, "ShowHorizontalGridlines", show, this );
  m_horizantalLinesItems[0]->setHidden( show );
  m_horizantalLinesItems[1]->setHidden( !show );
  m_spectrum->showHorizontalLines( show );
  m_timeSeries->showHorizontalLines( show );
}//void setHorizantalLines( bool show )


#if( USE_SPECTRUM_CHART_D3 )
void InterSpec::setXAxisSlider( bool show )
{
  InterSpecUser::setPreferenceValue<bool>( m_user, "ShowXAxisSlider", show, this );
  m_showXAxisSliderItems[0]->setHidden( show );
  m_showXAxisSliderItems[1]->setHidden( !show );
  
  if( show )
  {
    //Default to compact x-axis.
    if( m_compactXAxisItems[0] )
      m_compactXAxisItems[0]->setHidden( true );
    if( m_compactXAxisItems[1] )
      m_compactXAxisItems[1]->setHidden( false );
    
     m_spectrum->setCompactAxis( true );
    m_timeSeries->setCompactAxis( true );
  }else
  {
    //Go back to whatever the user wants/selects
    const bool makeCompact = InterSpecUser::preferenceValue<bool>( "CompactXAxis", this );
    
    if( m_compactXAxisItems[0] )
      m_compactXAxisItems[0]->setHidden( makeCompact );
    if( m_compactXAxisItems[1] )
      m_compactXAxisItems[1]->setHidden( !makeCompact );
    
    m_spectrum->setCompactAxis( makeCompact );
    m_timeSeries->setCompactAxis( makeCompact );
  }//show /hide
  
  m_spectrum->showXAxisSliderChart( show );
}//void setXAxisSlider( bool show )


void InterSpec::setXAxisCompact( bool compact )
{
  InterSpecUser::setPreferenceValue<bool>( m_user, "CompactXAxis", compact, this );
  
  if( m_compactXAxisItems[0] )
    m_compactXAxisItems[0]->setHidden( compact );
  if( m_compactXAxisItems[1] )
    m_compactXAxisItems[1]->setHidden( !compact );
  
  m_spectrum->setCompactAxis( compact );
  m_timeSeries->setCompactAxis( compact );
}//void setXAxisCompact( bool compact )


void InterSpec::setShowYAxisScalers( bool show )
{
  InterSpecUser::setPreferenceValue<bool>( m_user, "ShowYAxisScalers", show, this );
  
  if( m_showYAxisScalerItems[0] )
    m_showYAxisScalerItems[0]->setHidden( show );
  if( m_showYAxisScalerItems[1] )
    m_showYAxisScalerItems[1]->setHidden( !show );
  
  m_spectrum->showYAxisScalers( show );
}//void setShowYAxisScalers( bool show )


#endif


ReferencePhotopeakDisplay *InterSpec::referenceLinesWidget()
{
  return m_referencePhotopeakLines;
}

#if( defined(WIN32) && BUILD_AS_ELECTRON_APP )
  //When users drag files from Outlook on windows into the app
  //  you can call the following functions
void InterSpec::dragEventWithFileContentsStarted()
{
  //Set JS variable to indicate that this isnt a normal file drop on the browser
  doJavaScript( "$('.Wt-domRoot').data('DropFileContents',true);" );
}


void InterSpec::dragEventWithFileContentsFinished()
{ 
  doJavaScript( "$('.Wt-domRoot').data('DropFileContents',false);"
	            "$('#Uploader').remove();"
				"$('.Wt-domRoot').data('IsDragging',false);"); 
}

#endif ///#if( defined(WIN32) && BUILD_AS_ELECTRON_APP )


Wt::JSlot *InterSpec::hotkeyJsSlot()
{
  return m_hotkeySlot.get();
}//const Wt::JSlot *hotkeyJsSlot() const;


void InterSpec::addPeakLabelSubMenu( PopupDivMenu *parentWidget )
{
  PopupDivMenu *menu = parentWidget->addPopupMenuItem( "Peak Labels",  "InterSpec_resources/images/tag_green.png" );
  
  
//  PopupDivMenuItem *item = menu->addMenuItem( "Show User Labels", "", false );
  //
  WCheckBox *cb = new WCheckBox( "Show User Labels" );
  cb->setChecked(false);
  PopupDivMenuItem *item = menu->addWidget( cb );
  
  cb->checked().connect(
                        boost::bind(
#if ( USE_SPECTRUM_CHART_D3 )
                                    &D3SpectrumDisplayDiv::setShowPeakLabel,
#else
                                    &SpectrumDisplayDiv::setShowPeakLabel,
#endif
                                    m_spectrum, SpectrumChart::kShowPeakUserLabel, true ) );
  cb->unChecked().connect(
                          boost::bind(
#if ( USE_SPECTRUM_CHART_D3 )
                                      &D3SpectrumDisplayDiv::setShowPeakLabel,
#else
                                      &SpectrumDisplayDiv::setShowPeakLabel,
#endif
                                      m_spectrum, SpectrumChart::kShowPeakUserLabel, false ) );
  
  cb = new WCheckBox( "Show Peak Energies" );
  cb->setChecked(false);
  item = menu->addWidget( cb );
  
  cb->checked().connect(
                        boost::bind(
#if ( USE_SPECTRUM_CHART_D3 )
                                    &D3SpectrumDisplayDiv::setShowPeakLabel,
#else
                                    &SpectrumDisplayDiv::setShowPeakLabel,
#endif
                                    m_spectrum, SpectrumChart::kShowPeakEnergyLabel, true ) );
  cb->unChecked().connect(
                          boost::bind(
#if ( USE_SPECTRUM_CHART_D3 )
                                      &D3SpectrumDisplayDiv::setShowPeakLabel,
#else
                                      &SpectrumDisplayDiv::setShowPeakLabel,
#endif
                                      m_spectrum, SpectrumChart::kShowPeakEnergyLabel, false ) );
  
  cb = new WCheckBox( "Show Nuclide Names" );
  cb->setChecked(false);
  item = menu->addWidget( cb );
  
  cb->checked().connect(
                        boost::bind(
#if ( USE_SPECTRUM_CHART_D3 )
                                    &D3SpectrumDisplayDiv::setShowPeakLabel,
#else
                                    &SpectrumDisplayDiv::setShowPeakLabel,
#endif
                                    m_spectrum, SpectrumChart::kShowPeakNuclideLabel, true ) );
  cb->unChecked().connect(
                          boost::bind(
#if ( USE_SPECTRUM_CHART_D3 )
                                      &D3SpectrumDisplayDiv::setShowPeakLabel,
#else
                                      &SpectrumDisplayDiv::setShowPeakLabel,
#endif
                                      m_spectrum, SpectrumChart::kShowPeakNuclideLabel, false ) );
  
  cb = new WCheckBox( "Show Nuclide Energies" );
  cb->setChecked(false);
  item = menu->addWidget( cb );
  
  cb->checked().connect(
                        boost::bind(
                                    
#if ( USE_SPECTRUM_CHART_D3 )
                                    &D3SpectrumDisplayDiv::setShowPeakLabel,
#else
                                    &SpectrumDisplayDiv::setShowPeakLabel,
#endif
                                    m_spectrum, SpectrumChart::kShowPeakNuclideEnergies, true ) );
  cb->unChecked().connect(
                          boost::bind(
                                      
#if ( USE_SPECTRUM_CHART_D3 )
                                      &D3SpectrumDisplayDiv::setShowPeakLabel,
#else
                                      &SpectrumDisplayDiv::setShowPeakLabel,
#endif
                                      m_spectrum, SpectrumChart::kShowPeakNuclideEnergies,  false ) );
}//void addPeakLabelMenu( Wt::WContainerWidget *menuDiv )



void InterSpec::addAboutMenu( Wt::WWidget *parent )
{
  if( m_helpMenuPopup )
    return;
  
  PopupDivMenu *parentMenu = dynamic_cast<PopupDivMenu *>( parent );
  WContainerWidget *menuDiv = dynamic_cast<WContainerWidget *>( parent );
  if( !parentMenu && !menuDiv )
    throw runtime_error( "InterSpec::addAboutMenu(): parent passed in"
                         " must be a PopupDivMenu  or WContainerWidget" );

  if( menuDiv )
  {
    WPushButton *button = new WPushButton( "Help", menuDiv );
    button->addStyleClass( "MenuLabel" );
    m_helpMenuPopup = new PopupDivMenu( button, PopupDivMenu::AppLevelMenu );
  }else
  {
    m_helpMenuPopup = parentMenu->addPopupMenuItem( "Help" );
  }//if( menuDiv ) / else

  PopupDivMenuItem *item = m_helpMenuPopup->addMenuItem( "Welcome..." );
  item->triggered().connect( boost::bind( &InterSpec::showWelcomeDialog, this, true ) );

  m_helpMenuPopup->addSeparator();
  
  //if( isMobile() )
  //  item = m_helpMenuPopup->addMenuItem( "Help Contents..." ,  "InterSpec_resources/images/help_mobile.svg");
  //else
    item = m_helpMenuPopup->addMenuItem( "Help Contents..." ,  "InterSpec_resources/images/help_small.png");
  
  item->triggered().connect( boost::bind( &HelpSystem::createHelpWindow, string("getting-started") ) );

  Wt::WMenuItem *notifications = m_helpMenuPopup->addMenuItem( "Notification Logs..." , "InterSpec_resources/images/log_file_small.png");
  notifications->triggered().connect( this, &InterSpec::showWarningsWindow );

  m_helpMenuPopup->addSeparator();
  PopupDivMenu *subPopup = m_helpMenuPopup->addPopupMenuItem( "Options", "InterSpec_resources/images/cog_small.png" );
    
  const bool showToolTipInstantly = InterSpecUser::preferenceValue<bool>( "ShowTooltips", this );
    
  WCheckBox *cb = new WCheckBox( " Automatically store session" );
  InterSpecUser::associateWidget( m_user, "AutoSaveSpectraToDb", cb, this, false );
  item = subPopup->addWidget( cb );
  HelpSystem::attachToolTipOn( item, "Automatically stores app state", showToolTipInstantly );

  if (!isMobile())
  {
      WCheckBox* checkbox = new WCheckBox( " Instant tooltips" );
    
      InterSpecUser::associateWidget( m_user, "ShowTooltips", checkbox, this, false );
      item = subPopup->addWidget( checkbox );
      HelpSystem::attachToolTipOn( item,
            "Instant tooltips show up immediately and is helpful for beginners "
            "to understand how to use InterSpec.  Advanced users are recommended"
            " to turn this off, causing tooltips to show only after a 1 second"
                              " delay." , true, HelpSystem::Right, HelpSystem::AlwaysShow );
      checkbox->checked().connect( boost::bind( &InterSpec::toggleToolTip, this, true ) );
      checkbox->unChecked().connect( boost::bind( &InterSpec::toggleToolTip, this, false ) );
  } //!isMobile()
  
    //High Bandwidth interactions
#if( USE_HIGH_BANDWIDTH_INTERACTIONS && !USE_SPECTRUM_CHART_D3 )
    WCheckBox *highBWCb = new WCheckBox( "Smooth Zoom/Pan" );
    item = subPopup->addWidget( highBWCb );
    
#if( BUILD_FOR_WEB_DEPLOYMENT )
    const char *smoothzoomtext = "Smooth zooming out and panning of spectrum - "
    " increases bandwidth used.";
#else
    const char *smoothzoomtext = "Smooth zooming out and panning of spectrum.";
#endif
    
    HelpSystem::attachToolTipOn( item, smoothzoomtext, showToolTipInstantly );
    InterSpecUser::associateWidget( m_user, "SmoothZoomPan", highBWCb, this, false );
    highBWCb->checked().connect( this, &InterSpec::enableSmoothChartOperations );
    highBWCb->unChecked().connect( this, &InterSpec::disableSmoothChartOperations );
    if( !highBWCb->isChecked() )
        disableSmoothChartOperations();
    
    subPopup->addSeparator();
#endif
  
	item = subPopup->addMenuItem("Color Themes...");
	item->triggered().connect(boost::bind(&InterSpec::showColorThemeWindow, this));
	subPopup->addSeparator();

#if( PROMPT_USER_BEFORE_LOADING_PREVIOUS_STATE )
  WCheckBox *promptOnLoad = new WCheckBox( "Prompt to load prev state" );
  item = subPopup->addWidget( promptOnLoad );
  const char *prompttext = "At application start, ask to load previous state.";
  HelpSystem::attachToolTipOn( item, prompttext, showToolTipInstantly );
  InterSpecUser::associateWidget( m_user, "PromptStateLoadOnStart", promptOnLoad, this, false );
  
  WCheckBox *doLoad = new WCheckBox( "Load prev state on start" );
  item = subPopup->addWidget( doLoad );
  const char *doloadtext = "At application start, automatically load previous state, if not set to be prompted";
  HelpSystem::attachToolTipOn( item, doloadtext, showToolTipInstantly );
  InterSpecUser::associateWidget( m_user, "LoadPrevStateOnStart", doLoad, this, false );
#endif
    
  m_helpMenuPopup->addSeparator();
  
  item = m_helpMenuPopup->addMenuItem( "About InterSpec..." );
  item->triggered().connect( boost::bind( &InterSpec::showLicenseAndDisclaimersWindow, this, false, std::function<void()>{} ) );
}//void addAboutMenu( Wt::WContainerWidget *menuDiv )


void InterSpec::toggleToolTip( const bool sticky )
{
  //update all existing qtips
  const char *delay = sticky ? "0" : "1000";
  const char *keyPressHide = sticky ? "" : " keypress";

  char buffer[248];  //only need about 149 characters
  snprintf( buffer, sizeof(buffer),
           "$('.qtip-rounded.delayable').qtip('option', 'show.delay', %s);"
           "$('.qtip-rounded.delayable').qtip('option', 'hide.event', 'mouseleave focusout%s' );",
           delay, keyPressHide );
  wApp->doJavaScript( buffer );
}//void toggleToolTip( const bool sticky )


int InterSpec::paintedWidth() const
{
  return max( m_spectrum->layoutWidth(), m_timeSeries->layoutWidth() );
}//int paintedWidth() const


int InterSpec::paintedHeight() const
{
  //XXX This isnt correct due to all padding and stuff
  return m_spectrum->layoutHeight() + m_timeSeries->layoutHeight();
}//int paintedHeight() const



const std::set<int> &InterSpec::displayedSamples( SpectrumType type ) const
{
  static const std::set<int> empty;
  switch( type )
  {
    case kForeground:
    {
      if( !m_dataMeasurement )
        return empty;
      return m_displayedSamples;
    }//case kForeground:

    case kSecondForeground:
    {
      if( !m_secondDataMeasurement )
        return empty;
      return m_sectondForgroundSampleNumbers;
    }//case kSecondForeground:

    case kBackground:
    {
      if( !m_backgroundMeasurement )
        return empty;
      return m_backgroundSampleNumbers;
    }//case kBackground:
  }//switch( type )

  throw runtime_error( SRC_LOCATION + " - Serious Badness" );

  return empty;  //keep compiler from complaining
}//const std::set<int> &displayedSamples( SpectrumType spectrum_type ) const


std::set<int> InterSpec::displayedDetectorNumbers() const
{
  set<int> answer;
  if( !m_dataMeasurement )
    return answer;
  
  const vector<bool> det_use = detectors_to_display();
  const std::vector<int> &det_nums = m_dataMeasurement->detector_numbers();
  
  if( det_use.size() != det_nums.size() )
    throw runtime_error( "Serious logic error in InterSpec::displayedDetectorNumbers()" );
  
  for( size_t i = 0; i < det_nums.size(); ++i )
    if( det_use[i] )
      answer.insert( det_nums[i] );
  
  return answer;
}//std::set<int> displayedDetectorNumbers() const


std::shared_ptr<const SpecMeas> InterSpec::measurment( SpectrumType type ) const
{
  switch( type )
  {
    case kForeground:
      return m_dataMeasurement;
    case kSecondForeground:
      return m_secondDataMeasurement;
    case kBackground:
      return m_backgroundMeasurement;
  }//switch( type )

  return std::shared_ptr<const SpecMeas>();
}//measurment(...)


std::shared_ptr<SpecMeas> InterSpec::measurment( SpectrumType type )
{
  switch( type )
  {
    case kForeground:
      return m_dataMeasurement;
    case kSecondForeground:
      return m_secondDataMeasurement;
    case kBackground:
      return m_backgroundMeasurement;
  }//switch( type )

  return std::shared_ptr<SpecMeas>();
}//std::shared_ptr<SpecMeas> measurment( SpectrumType spectrum_type )


#if( USE_DB_TO_STORE_SPECTRA )
Wt::Dbo::ptr<UserFileInDb> InterSpec::measurmentFromDb( SpectrumType type,
                                                             bool update )
{
  try
  {
    Wt::Dbo::ptr<UserFileInDb> answer;
    std::shared_ptr<SpectraFileHeader> header;
  
    SpectraFileModel *fileModel = m_fileManager->model();
    std::shared_ptr<SpecMeas> meas = measurment( type );
    if( !meas )
      return answer;
  
    WModelIndex index = fileModel->index( meas );
    if( !index.isValid() )
      return answer;
    
    header = fileModel->fileHeader( index.row() );
    if( !header )
      return answer;
  
    answer = header->dbEntry();
    if( answer && !meas->modified() )
      return answer;
  
    const bool savePref = m_user->preferenceValue<bool>( "AutoSaveSpectraToDb" /*"SaveSpectraToDb"*/);
    if( !savePref )
      return answer;
    
    Dbo::ptr<UserFileInDb> dbback;
    
    if( type == kForeground )
    {
      WModelIndex bindex;
      std::shared_ptr<SpectraFileHeader> bheader;
      std::shared_ptr<SpecMeas> background = measurment( kBackground );
      if( background )
         bindex = fileModel->index( background );
      if( bindex.isValid() )
        bheader = fileModel->fileHeader( bindex.row() );
      if( bheader )
      {
        dbback = bheader->dbEntry();
        if( !dbback )
        {
          try
          {
            bheader->saveToDatabase( background );
            dbback = bheader->dbEntry();
          }catch(...){}
        }
      }//if( bheader )
    }//if( type == kForeground )
    
    if( update && savePref )
      header->saveToDatabase( meas );
    
    return header->dbEntry();
  }catch( std::exception &e )
  {
    cerr << "\n\nSpectrumViewer::measurmentFromDb(...) caught: " << e.what()
         << endl;
  }//try / catch
  
  return Wt::Dbo::ptr<UserFileInDb>();
}//Wt::Dbo::ptr<UserFileInDb> measurmentFromDb( SpectrumType type, bool update );
#endif //#if( USE_DB_TO_STORE_SPECTRA )

std::shared_ptr<const Measurement> InterSpec::displayedHistogram( SpectrumType spectrum_type ) const
{
  switch( spectrum_type )
  {
    case kForeground:
      return m_spectrum->data();
    case kSecondForeground:
      return m_spectrum->secondData();
    case kBackground:
      return m_spectrum->background();
//  m_spectrum->continuum();
  }//switch( spectrum_type )

  throw runtime_error( "InterSpec::displayedHistogram(...): invalid input arg" );

  return std::shared_ptr<const Measurement>();
}//displayedHistogram(...)

#if ( USE_SPECTRUM_CHART_D3 )
#else
void InterSpec::saveChartToPng( const bool spectrum )
{
  std::shared_ptr<const SpecMeas> spec = measurment(kForeground);
  string filename = (spec ? spec->filename() : string("spectrum"));
  const string ext = UtilityFunctions::file_extension(filename);
  if( !ext.empty() && (ext.size() <= filename.size()) )
    filename = filename.substr(0,filename.size()-ext.size());
  if( filename.empty() )
    filename = "spectrum";
  if( !spectrum )
    filename += "_timechart";
  std::string timestr = UtilityFunctions::to_iso_string( boost::posix_time::second_clock::local_time() );
  auto ppos = timestr.find('.');
  if( ppos != string::npos )
    timestr = timestr.substr(0,ppos);
  filename += "_" + timestr + ".png";
  
  if( spectrum )
    m_spectrum->saveChartToPng( filename );
  else
    m_timeSeries->saveChartToPng( filename );
}//saveSpectrumToPng()
#endif //USE_SPECTRUM_CHART_D3 / else


double InterSpec::displayScaleFactor( SpectrumType spectrum_type ) const
{
  return m_spectrum->displayScaleFactor( spectrum_type );
}//double displayScaleFactor( SpectrumType spectrum_type ) const


void InterSpec::setDisplayScaleFactor( const double sf,
                                            const SpectrumType spectrum_type )
{
  m_spectrum->setDisplayScaleFactor( sf, spectrum_type );
}//void setDisplayScaleFactor( const double sf, SpectrumType spectrum_type );


float InterSpec::liveTime( SpectrumType type ) const
{
  if( !measurment(type) )
    return 0.0f;
  
  switch( type )
  {
    case kForeground:
      return m_spectrum->foregroundLiveTime();
    case kSecondForeground:
      return m_spectrum->secondForegroundLiveTime();
    case kBackground:
      return m_spectrum->backgroundLiveTime();
  }//switch( type )
  
  return 0.0f;
}//float liveTime( SpectrumType type ) const


int InterSpec::renderedWidth() const
{
  return m_renderedWidth;
}

int InterSpec::renderedHeight() const
{
  return m_renderedHeight;
}


void InterSpec::createOneOverR2Calculator()
{
//  OneOverR2Calc *calc =
  new OneOverR2Calc();
  
//  if( !toolTabsVisible() )
//  {
//    const int maxHeight = static_cast<int>(0.95*paintedHeight());
//    const int maxWidth = static_cast<int>(0.95*paintedWidth());
//    calc->setMaximumSize( maxWidth, maxHeight );
//    calc->contents()->setOverflow( WContainerWidget::OverflowAuto );
//  }//if( !toolTabsVisible() )
}//void createOneOverR2Calculator()


void InterSpec::createActivityConverter()
{
    new ActivityConverter();
}//void createActivityConverter()


void InterSpec::createDecay()
{
  Decay *decay = new Decay( this );
  
  if( m_referencePhotopeakLines )
  {
    const ReferenceLineInfo &nuc
                          = m_referencePhotopeakLines->currentlyShowingNuclide();
    if( nuc.nuclide )
    {
      decay->clearAllNuclides();
      
      //\todo We could do a little better and check the Shielding/Source Fit
      //  widget and grab those activities (and ages) if they match
      
      decay->addNuclide( nuc.nuclide->atomicNumber,
                         nuc.nuclide->massNumber,
                         nuc.nuclide->isomerNumber,
                         1.0*PhysicalUnits::microCi, true,
                         0.0, 5.0*nuc.age );
    }//if( nuc.nuclide )
  }//if( m_referencePhotopeakLines )
  
}//void createDecay()

void InterSpec::createFileParameterWindow()
{
  new SpecFileSummary( this );
}//void createFileParameterWindow()


#if( USE_GOOGLE_MAP )
void InterSpec::displayOnlySamplesWithinView( GoogleMap *map,
                                  const SpectrumType targetSamples,
                                  const SpectrumType fromSamples )
{
  float uplat, leftlng, lowerlat, rightlng;
  map->getMapExtent( uplat, leftlng, lowerlat, rightlng );
  
  if( lowerlat > uplat )
    std::swap( lowerlat, uplat );
  if( leftlng > rightlng )
    std::swap( leftlng, rightlng );
  
  std::set<int> sample_numbers;
  std::shared_ptr<SpecMeas> meass = measurment( fromSamples );
  
  if( !meass )
  {
    passMessage( "Could not load spectrum", "", WarningWidget::WarningMsgHigh );
    return;
  }//if( !meass )
  
  for( const int sample : meass->sample_numbers() )
  {
    bool samplewithin = false;
    for( const int detnum : meass->detector_numbers() )
    {
      MeasurementConstShrdPtr m = meass->measurement( sample, detnum );
      if( !!m && m->has_gps_info()
         && m->longitude()>=leftlng && m->longitude()<=rightlng
         && m->latitude()>=lowerlat && m->latitude()<=uplat )
      {
        samplewithin = true;
        break;
      }
    }//for( const int detnum : meass->detector_numbers() )
    
    if( samplewithin )
      sample_numbers.insert( sample );
  }//for( int sample : meass->sample_numbers() )
  
  if( sample_numbers.empty() )
  {
    passMessage( "There were no samples in the visible map area.",
                 "", WarningWidget::WarningMsgHigh );
    return;
  }//if( sample_numbers.empty() )
  
  
  if( (fromSamples!=targetSamples) || targetSamples!=kForeground )
    setSpectrum( meass, sample_numbers, targetSamples, true );
  else
    changeDisplayedSampleNums( sample_numbers, targetSamples );
}//displayOnlySamplesWithinView(...)


void InterSpec::createMapWindow( SpectrumType spectrum_type )
{
  std::shared_ptr<const SpecMeas> meas = measurment( spectrum_type );
  
  if( !meas )
    return;
  
  const set<int> &samples = displayedSamples( spectrum_type );
  
  AuxWindow *window = new AuxWindow( "Map" );
  
  int w = 0.66*renderedWidth();
  int h = 0.8*renderedHeight();
  
  const char *label = 0;
  switch( spectrum_type )
  {
    case kForeground:       label = "Foreground";        break;
    case kSecondForeground: label = "Second Foreground"; break;
    case kBackground:       label = "Background";        break;
  }//switch( spectrum_type )
  
  window->disableCollapse();
  window->setResizable( true );
  window->finished().connect( boost::bind( &AuxWindow::deleteAuxWindow, window ) );
  //window->footer()->setStyleClass( "modal-footer" );
  
  const bool enableLoadingVisible = (meas->sample_numbers().size() > 1);

  GoogleMap *googlemap = new GoogleMap( enableLoadingVisible );
  WGridLayout *layout = window->stretcher();
  googlemap->addMeasurment( meas, label, samples );
  layout->addWidget( googlemap, 0, 0 );
  layout->setContentsMargins( 0, 0, 0, 0 );
  layout->setVerticalSpacing( 0 );
  layout->setHorizontalSpacing( 0 );
  
  //We need to set the footer height explicitly, or else the window->resize()
  //  messes up.
//  window->footer()->resize( WLength::Auto, WLength(50.0) );
  
  if( enableLoadingVisible )
  {
    WPushButton *button = new WPushButton( "Load Visible Points...", window->footer() );
    WPopupMenu *menu = new WPopupMenu();
    menu->setAutoHide( true );
    button->setMenu( menu );
    WMenuItem *item = menu->addItem( "As Foreground" );
    item->triggered().connect( boost::bind( &InterSpec::displayOnlySamplesWithinView, this, googlemap, kForeground, spectrum_type ) );
    item = menu->addItem( "As Background" );
    item->triggered().connect( boost::bind( &InterSpec::displayOnlySamplesWithinView, this, googlemap, kBackground, spectrum_type ) );
    item = menu->addItem( "As Secondary" );
    item->triggered().connect( boost::bind( &InterSpec::displayOnlySamplesWithinView, this, googlemap, kSecondForeground, spectrum_type ) );
  }//if( meas->measurements().size() > 10 )
  
  
  WPushButton *closeButton = window->addCloseButtonToFooter();
  closeButton->clicked().connect( window, &AuxWindow::hide );
  
  window->resize( WLength(w), WLength(h) );
  window->show();
  window->centerWindow();
  window->rejectWhenEscapePressed();
  
//  window->resizeToFitOnScreen();
}//void createMapWindow()
#endif //#if( USE_GOOGLE_MAP )


#if( USE_SEARCH_MODE_3D_CHART )
void InterSpec::create3DSearchModeChart()
{
  if( !m_dataMeasurement || !m_dataMeasurement->passthrough() )
  {
    passMessage( "The 3D chart is only available for search mode or RPM passthrough data.",
                "", WarningWidget::WarningMsgInfo );
    return;
  }//if( we dont have the proper data to make a 3D chart )
  
  AuxWindow *dialog = new AuxWindow( "3D Data View" );
  dialog->disableCollapse();
  dialog->Wt::WDialog::rejectWhenEscapePressed();
  
  WGridLayout *layout = dialog->stretcher();
  layout->setHorizontalSpacing( 0 );
  layout->setVerticalSpacing( 0 );
  layout->setContentsMargins( 0, 0, 0, 0 );
  SearchMode3DChart *chart = new SearchMode3DChart( this );
  layout->addWidget( chart, 0, 0 );
  
  dialog->show();
  dialog->setClosable( true );
  dialog->resizeScaledWindow( 0.95, 0.95 );
  dialog->centerWindow();
  dialog->finished().connect( boost::bind( &AuxWindow::deleteAuxWindow, dialog ) );
}//void create3DSearchModeChart()
#endif


#if( USE_TERMINAL_WIDGET )
void InterSpec::createTerminalWidget()
{
  if( m_terminal )
    return;
  
  m_terminal = new TerminalWidget( this );
  m_terminal->focusText();
  
  if( m_toolsTabs )
  {
    WMenuItem *item = m_toolsTabs->addTab( m_terminal, "Terminal" );
    item->setCloseable( true );
    m_toolsTabs->setCurrentWidget( m_terminal );
    const int index = m_toolsTabs->currentIndex();
    m_toolsTabs->setTabToolTip( index, "Numeric, algebraic, and text-based spectrum interaction terminal." );
    m_toolsTabs->tabClosed().connect( this, &InterSpec::handleTerminalWindowClose );
  }else
  {
    m_terminalWindow = new AuxWindow( "Terminal" );
    m_terminalWindow->setClosable( true );
    
    m_terminalWindow->rejectWhenEscapePressed();
    m_terminalWindow->finished().connect( this, &InterSpec::handleTerminalWindowClose );

    m_terminalWindow->show();
    if( m_renderedWidth > 100 && m_renderedHeight > 100 && !isPhone() )
    {
      m_terminalWindow->resizeWindow( 0.95*m_renderedWidth, 0.25*m_renderedHeight );
      m_terminalWindow->centerWindow();
    }
    
    m_terminalWindow->stretcher()->addWidget( m_terminal, 0, 0 );
  }//if( toolTabsVisible() )
  
  m_terminalMenuItem->disable();
}//void createTerminalWidget()


void InterSpec::handleTerminalWindowClose()
{
  if( !m_terminal )
    return;
 
  m_terminalMenuItem->enable();
  
  if( m_terminalWindow )
  {
    delete m_terminalWindow;
  }else
  {
    delete m_terminal;
    if( m_toolsTabs )
      m_toolsTabs->setCurrentIndex( 2 );
  }
  
  m_terminal = 0;
  m_terminalWindow = 0;
}//void handleTerminalWindowClose()
#endif  //#if( USE_TERMINAL_WIDGET )



void InterSpec::addToolsMenu( Wt::WWidget *parent )
{
  
  const bool showToolTipInstantly = InterSpecUser::preferenceValue<bool>( "ShowTooltips", this );
  
  PopupDivMenu *parentMenu = dynamic_cast<PopupDivMenu *>( parent );
  WContainerWidget *menuDiv = dynamic_cast<WContainerWidget *>( parent );
  if( !parentMenu && !menuDiv )
    throw runtime_error( "InterSpec::addToolsMenu(): parent passed in"
                        " must be a PopupDivMenu  or WContainerWidget" );
  
  PopupDivMenu *popup = NULL;
  
  if( menuDiv )
  {
    WPushButton *button = new WPushButton( "Tools", menuDiv );
    button->addStyleClass( "MenuLabel" );
    popup = new PopupDivMenu( button, PopupDivMenu::AppLevelMenu );
  }else
  {
    popup = parentMenu->addPopupMenuItem( "Tools" );
  }

  m_toolsMenuPopup = popup;
  
  PopupDivMenuItem *item = NULL;

  item = popup->addMenuItem( "Activity/Shielding Fit" );
  HelpSystem::attachToolTipOn( item,"Allows advanced input of shielding material and activity around source isotopes to improve the fit." , showToolTipInstantly );
  item->triggered().connect( boost::bind( &InterSpec::showShieldingSourceFitWindow, this ) );
  
  item = popup->addMenuItem( "Gamma XS Calc", "" );
  HelpSystem::attachToolTipOn( item,"Allows user to determine the cross section for gammas of arbitrary energy though any material in <code>InterSpec</code>'s library. Efficiency estimates for detection of the gamma rays inside the full energy peak and the fraction of gamma rays that will make it through the material without interacting with it can be provided with the input of additional information.", showToolTipInstantly );
  item->triggered().connect( boost::bind( &InterSpec::showGammaXsTool, this ) );
    
    
  item = popup->addMenuItem( "Dose Calc", "" );
  HelpSystem::attachToolTipOn( item,
      "Allows you to compute dose, activity, shielding, or distance, given the"
      " other pieces of information.", showToolTipInstantly );
  item->triggered().connect( boost::bind( &InterSpec::showDoseTool, this ) );
  
//  item = popup->addMenuItem( Wt::WString::fromUTF8("1/r² Calculator") );  // is superscript 2
#if( USE_OSX_NATIVE_MENU  || (BUILD_AS_ELECTRON_APP && USE_ELECTRON_NATIVE_MENU) )
  item = popup->addMenuItem( Wt::WString::fromUTF8("1/r\x0032 Calculator") );  //works on OS X at least.
#else
  item = popup->addMenuItem( Wt::WString::fromUTF8("1/r<sup>2</sup> Calculator") );
  item->makeTextXHTML();
#endif
  
  HelpSystem::attachToolTipOn( item,"Allows user to use two dose measurements taken at different distances from a source to determine the absolute distance to the source from the nearer measurement.", showToolTipInstantly );
  
  item->triggered().connect( this, &InterSpec::createOneOverR2Calculator );

  item = popup->addMenuItem( "Activity Converter" );
  HelpSystem::attachToolTipOn( item,"Curie to Becquerel converter.", showToolTipInstantly );
  item->triggered().connect( this, &InterSpec::createActivityConverter );
  
  item = popup->addMenuItem( "Nuclide Decay Info" );
  HelpSystem::attachToolTipOn( item,"Allows user to obtain advanced information about activities, gamma/alpha/beta production rates, decay chain, and daughter nuclides." , showToolTipInstantly );
  item->triggered().connect( this, &InterSpec::createDecay );

  
  item = popup->addMenuItem( "Detector Response Select" );
  HelpSystem::attachToolTipOn( item,"Allows user to change the detector response function.", showToolTipInstantly );
  item->triggered().connect( boost::bind( &InterSpec::showDetectorEditWindow, this ) );
  
  item = popup->addMenuItem( "Make Detector Response" );
  HelpSystem::attachToolTipOn( item, "Create detector response function from characterization data.", showToolTipInstantly );
  item->triggered().connect( boost::bind( &InterSpec::showMakeDrfWindow, this ) );

  
  item = popup->addMenuItem( "File Parameters" );
  HelpSystem::attachToolTipOn( item,"Allows user to view/edit the file parameters. If ever the application is unable to render activity calculation, use this tool to provide parameters the original file did not provide; <code>InterSpec</code> needs all parameters for activity calculation.", showToolTipInstantly );
  item->triggered().connect( this, &InterSpec::createFileParameterWindow );

  item = popup->addMenuItem( "Energy Range Count" );
  HelpSystem::attachToolTipOn( item, "Sums the number of gammas in region of interest (ROI). Can also be accessed by left-click dragging over the ROI while holding both the <kbd><b>ALT</b></kbd> and <kbd><b>SHIFT</b></kbd> keys.", showToolTipInstantly );
  item->triggered().connect( this, &InterSpec::showGammaCountDialog );
  
#if( USE_SPECRUM_FILE_QUERY_WIDGET )
  item = popup->addMenuItem( "File Query Tool" );
  HelpSystem::attachToolTipOn( item, "Searches through a directory (recursively) for spectrum files that match specafiable conditions.", showToolTipInstantly );
  item->triggered().connect( this, &InterSpec::showFileQueryDialog );
#endif
  
#if( USE_TERMINAL_WIDGET )
  m_terminalMenuItem = popup->addMenuItem( "Math/Command Terminal" );
  HelpSystem::attachToolTipOn( m_terminalMenuItem, "Creates a terminal that provides numeric and algebraic computations, as well as allowing text based interactions with the spectra.", showToolTipInstantly );
  m_terminalMenuItem->triggered().connect( this, &InterSpec::createTerminalWidget );
#endif
}//void InterSpec::addToolsMenu( Wt::WContainerWidget *menuDiv )


void InterSpec::initRecalibrator()
{
  m_recalibrator = new Recalibrator( m_spectrum, this, m_peakModel );

    //right drag -> calibrate
    
    m_spectrum->rightMouseDragg().connect( m_recalibrator, &Recalibrator::handleGraphicalRecalRequest );
}//void initRecalibrator()


void InterSpec::fillMaterialDb( std::shared_ptr<MaterialDB> materialDB,
                                     const std::string sessionid,
                                     boost::function<void(void)> update )
{
  const SandiaDecay::SandiaDecayDataBase *db = DecayDataBaseServer::database();
  try
  {
    //materialDB can get destructed if the session ends immediately....
    materialDB->parseGadrasMaterialFile( "data/MaterialDataBase.txt", db, false );
    
    WServer::instance()->post( sessionid, update );
  }catch( std::exception &e )
  {
    WString msg = "Error initilizing the material database: " + string(e.what());
    
    WServer::instance()->post( sessionid, boost::bind( &postSvlogHelper, msg, int(WarningWidget::WarningMsgHigh) ) );
    
    return;
  }//try / catch
}//void fillMaterialDb(...)


void InterSpec::pushMaterialSuggestionsToUsers()
{
  if( !m_materialDB || !m_shieldingSuggestion )
    throw runtime_error( "pushMaterialSuggestionsToUsers(): you must"
                        " call initMaterialDbAndSuggestions() first." );
  
  for( const string &name : m_materialDB->names() )
    m_shieldingSuggestion->addSuggestion( name, name );
  
  wApp->triggerUpdate();
}//void pushMaterialSuggestionsToUsers()



void InterSpec::initMaterialDbAndSuggestions()
{
  if( !m_shieldingSuggestion )
  {
    WSuggestionPopup::Options popupOptions;
    popupOptions.highlightBeginTag  = "<b>";          //Open tag to highlight a match in a suggestion.
    popupOptions.highlightEndTag    = "</b>";         //Close tag to highlight a match in a suggestion.
    popupOptions.listSeparator      = ',';            //(char) When editing a list of values, the separator used for different items.
    popupOptions.whitespace         = " \\t()";       //When editing a value, the whitespace characters ignored before the current value.
    popupOptions.wordSeparators     = "-_., ;()";     //To show suggestions based on matches of the edited value with parts of the suggestion.
    popupOptions.appendReplacedText = "";             //When replacing the curr
    m_shieldingSuggestion = new WSuggestionPopup( popupOptions );
    m_shieldingSuggestion->addStyleClass("suggestion");
    m_shieldingSuggestion->setJavaScriptMember("wtNoReparent", "true");
    m_shieldingSuggestion->setFilterLength( 0 );
    m_shieldingSuggestion->setMaximumSize( WLength::Auto, WLength(15, WLength::FontEm) );
  }//if( !m_shieldingSuggestion )

  if( !m_materialDB )
  {
    m_materialDB = std::make_shared<MaterialDB>();
    
    boost::function<void(void)> success = wApp->bind( boost::bind(&InterSpec::pushMaterialSuggestionsToUsers, this) );
    
    boost::function<void(void)> worker = boost::bind( &fillMaterialDb,
                                    m_materialDB, wApp->sessionId(), success );
    WServer::instance()->ioService().post( worker );
  }//if( !m_materialDB )
}//void InterSpec::initMaterialDbAndSuggestions()


void InterSpec::showGammaXsTool()
{
  new GammaXsWindow( m_materialDB.get(), m_shieldingSuggestion, this );
} //showGammaXsTool()


void InterSpec::showDoseTool()
{
  new DoseCalcWindow( m_materialDB.get(), m_shieldingSuggestion, this );
}


void InterSpec::showMakeDrfWindow()
{
  MakeDrf::makeDrfWindow( this, m_materialDB.get(), m_shieldingSuggestion );
}//void showDetectorEditWindow()


void InterSpec::showDetectorEditWindow()
{
  std::shared_ptr<DetectorPeakResponse> currentDet;
  if( m_dataMeasurement )
    currentDet = m_dataMeasurement->detector();
  InterSpec *specViewer = this;
  SpectraFileModel *fileModel = m_fileManager->model();

  new DetectorEditWindow( currentDet, specViewer, fileModel );
}//void showDetectorEditWindow()


void InterSpec::showCompactFileManagerWindow()
{
  const CompactFileManager::DisplayMode cfmMode
                                 = isMobile() ? CompactFileManager::Tabbed
                                              : CompactFileManager::TopToBottom;
  CompactFileManager *compact
                       = new CompactFileManager( m_fileManager, this, cfmMode );

#if( USE_SPECTRUM_CHART_D3 )
  m_spectrum->yAxisScaled().connect( boost::bind( &CompactFileManager::handleSpectrumScale, compact, _1, _2 ) );
#endif
  
  AuxWindow *window = new AuxWindow( "Select Opened Spectra to Display", (AuxWindowProperties::TabletModal) );
  window->disableCollapse();
  window->finished().connect( boost::bind( &AuxWindow::deleteAuxWindow, window ) );
  
  
  WPushButton *closeButton = window->addCloseButtonToFooter();
  closeButton->clicked().connect(window, &AuxWindow::hide);
  
  WGridLayout *layout = window->stretcher();
  layout->addWidget( compact, 0, 0 );
  
  window->show();
//  window->resizeToFitOnScreen();
  window->centerWindow();
}//void showCompactFileManagerWindow()

void InterSpec::closeNuclideSearchWindow()
{
  if( !m_nuclideSearchWindow )
    return;
  
  m_isotopeSearch->clearSearchEnergiesOnClient();
  m_nuclideSearchWindow->stretcher()->removeWidget( m_isotopeSearch );
  
  delete m_nuclideSearchWindow;
  m_nuclideSearchWindow = 0;
  
  if( m_toolsTabs )
  {
    m_isotopeSearchContainer = new WContainerWidget();
    WGridLayout *isotopeSearchGridLayout = new WGridLayout();
    m_isotopeSearchContainer->setLayout( isotopeSearchGridLayout );

    isotopeSearchGridLayout->addWidget( m_isotopeSearch, 0, 0 );
    isotopeSearchGridLayout->setRowStretch( 0, 1 );
    isotopeSearchGridLayout->setColumnStretch( 0, 1 );

    m_toolsTabs->addTab( m_isotopeSearchContainer, NuclideSearchTabTitle, TabLoadPolicy );
    m_currentToolsTab = m_toolsTabs->currentIndex();
  }//if( m_toolsTabs )
}//void closeNuclideSearchWindow()

void InterSpec::showNuclideSearchWindow()
{
  if( m_nuclideSearchWindow )
  {
    m_nuclideSearchWindow->show();
    m_nuclideSearchWindow->resizeToFitOnScreen();
    m_nuclideSearchWindow->centerWindow();
    m_isotopeSearch->loadSearchEnergiesToClient();
    return;
  }//if( m_nuclideSearchWindow )
  
  if( m_toolsTabs && m_isotopeSearchContainer )
  {
    m_isotopeSearchContainer->layout()->removeWidget( m_isotopeSearch );
    m_toolsTabs->removeTab( m_isotopeSearchContainer );
    delete m_isotopeSearchContainer;
    m_isotopeSearchContainer = 0;
  }
  
  m_nuclideSearchWindow = new AuxWindow( NuclideSearchTabTitle, (AuxWindowProperties::TabletModal) );
  m_nuclideSearchWindow->contents()->setOverflow(Wt::WContainerWidget::OverflowHidden);
  m_nuclideSearchWindow->finished().connect( boost::bind( &InterSpec::closeNuclideSearchWindow, this ) );
  m_nuclideSearchWindow->rejectWhenEscapePressed();
  
  //if( isMobile() )
  //{
  //  m_nuclideSearchWindow->contents()->setPadding( 0 );
  //  m_nuclideSearchWindow->contents()->setMargin( 0 );
  //}//if( isPhone() )
  
  m_nuclideSearchWindow->stretcher()->setContentsMargins( 0, 0, 0, 0 );
  m_nuclideSearchWindow->stretcher()->addWidget( m_isotopeSearch, 0, 0 );
  
  //We need to set the footer height explicitly, or else the window->resize()
  //  messes up.
//  m_nuclideSearchWindow->footer()->resize( WLength::Auto, WLength(50.0) );
  
 
  Wt::WPushButton *closeButton = m_nuclideSearchWindow->addCloseButtonToFooter("Close",true);
  
  closeButton->clicked().connect( boost::bind( &InterSpec::closeNuclideSearchWindow, this ) );
  
  AuxWindow::addHelpInFooter( m_nuclideSearchWindow->footer(), "nuclide-search-dialog" );
  
  m_nuclideSearchWindow->resize( WLength(800,WLength::Pixel), WLength(510,WLength::Pixel));
  m_nuclideSearchWindow->resizeToFitOnScreen();
  m_nuclideSearchWindow->centerWindow();
  m_nuclideSearchWindow->setResizable(true);
  m_nuclideSearchWindow->show();
  
  m_isotopeSearch->loadSearchEnergiesToClient(); //clear the isotope search on the canvas
  
  if( m_toolsTabs )
    m_currentToolsTab = m_toolsTabs->currentIndex();
}//void showNuclideSearchWindow()


void InterSpec::showShieldingSourceFitWindow()
{
  if( !m_shieldingSourceFit )
  {
    assert( m_peakInfoDisplay );

    m_shieldingSourceFitWindow = new AuxWindow( "Activity/Shielding Fit" );
    m_shieldingSourceFit = new ShieldingSourceDisplay( m_peakModel, this,
                                          m_shieldingSuggestion, m_materialDB.get() );

    m_shieldingSourceFitWindow->setResizable( true );
    m_shieldingSourceFitWindow->contents()->setOffsets(WLength(0,WLength::Pixel));
    m_shieldingSourceFitWindow->stretcher()->addWidget( m_shieldingSourceFit, 0, 0 );
    m_shieldingSourceFitWindow->stretcher()->setContentsMargins(0,0,0,0);
   
//    m_shieldingSourceFitWindow->footer()->resize(WLength::Auto, WLength(50.0));
    
    WPushButton *closeButton = m_shieldingSourceFitWindow->addCloseButtonToFooter();
    closeButton->clicked().connect(m_shieldingSourceFitWindow, &AuxWindow::hide);
    
    AuxWindow::addHelpInFooter( m_shieldingSourceFitWindow->footer(), "activity-shielding-dialog" );
  
    m_shieldingSourceFitWindow->rejectWhenEscapePressed();
    
    //Should take lock on m_dataMeasurement->mutex_
    
    rapidxml::xml_document<char> *shield_source = nullptr;
    if( m_dataMeasurement )
      shield_source = m_dataMeasurement->shieldingSourceModel();
    
    if( shield_source && shield_source->first_node() )
    {
      //string msg = "Will try to deserailize: \n";
      //rapidxml::print( std::back_inserter(msg), *shield_source->first_node(), 0 );
      //cout << msg << endl << endl;
      try
      {
        m_shieldingSourceFit->deSerialize( shield_source->first_node() );
      }catch( std::exception &e )
      {
        string xmlstring;
        rapidxml::print(std::back_inserter(xmlstring), *shield_source, 0);
        stringstream debugmsg;
        debugmsg << "Error loading Shielding/Source model: "
                    "\n\tError Message: " << e.what()
                 << "\n\tModel XML: " << xmlstring;
#if( PERFORM_DEVELOPER_CHECKS )
        log_developer_error( BOOST_CURRENT_FUNCTION, debugmsg.str().c_str() );
#else
        cerr << debugmsg.str() << endl;
#endif
        passMessage( "There was an error loading the shielding/source model - model state is suspect!",
                    "", WarningWidget::WarningMsgHigh );
      }
    }//if( shield_source )
    
    
//    m_shieldingSourceFitWindow->resizeScaledWindow( 0.75, 0.75 );
    
    const double windowWidth = 0.95 * renderedWidth();
    const double windowHeight = 0.95 * renderedHeight();
    
//    double footerheight = m_shieldingSourceFitWindow->footer()->height().value();
//    m_shieldingSourceFitWindow->setMinimumSize( WLength(200), WLength(windowHeight) );
    
    
    if( (windowHeight > 100) && (windowWidth > 100) )
    {
      if( !isPhone() )
        m_shieldingSourceFitWindow->resizeWindow( windowWidth, windowHeight );

      //Give the m_shieldingSourceFitWindow a hint about what size it will be
      //  rendered at so it can decide what widgets should be rendered - acounting
      //  for borders and stuff (roughly)
      m_shieldingSourceFit->initialSizeHint( windowWidth - 12, windowHeight - 28 - 50 );
    }else if( !isPhone() )
    {
      //When loading an application state that is showing this window, we may
      //  not know the window size (e.g., windowWidth==windowHeight==0), so
      //  instead skip giving the initial size hint, and instead size things
      //  client side (maybe we should just do this always?)
      m_shieldingSourceFitWindow->resizeScaledWindow( 0.95, 0.95 );
    }
      
//    m_shieldingSourceFitWindow->contents()->  setHeight(WLength(windowHeight));

    m_shieldingSourceFitWindow->centerWindow();

    m_shieldingSourceFitWindow->finished().connect( this, &InterSpec::closeShieldingSourceFitWindow );
    
    m_shieldingSourceFitWindow->WDialog::setHidden(false);
    
    m_shieldingSourceFitWindow->show();
    m_shieldingSourceFitWindow->centerWindow();
  }else
  {
    const double windowWidth = 0.95 * renderedWidth();
    const double windowHeight = 0.95 * renderedHeight();
    m_shieldingSourceFitWindow->resizeWindow( windowWidth, windowHeight );
    
    m_shieldingSourceFitWindow->resizeToFitOnScreen();
    m_shieldingSourceFitWindow->show();
    m_shieldingSourceFitWindow->centerWindow();
  }//if( !m_shieldingSourceFit )
}//void showShieldingSourceFitWindow()


void InterSpec::saveShieldingSourceModelToForegroundSpecMeas()
{
  if( !m_shieldingSourceFitWindow || !m_shieldingSourceFit || !m_dataMeasurement )
    return;
  
  string xml_data;
  std::unique_ptr<rapidxml::xml_document<char>> doc( new rapidxml::xml_document<char>() );
  
  m_shieldingSourceFit->serialize( doc.get() );
  
  //rapidxml::print(std::back_inserter(xml_data), *doc, 0);
  //cout << "\n\nsaveShieldingSourceModelToForegroundSpecMeas Model: " << xml_data << endl << endl;
  
  m_dataMeasurement->setShieldingSourceModel( std::move(doc) );
}//void saveShieldingSourceModelToForegroundSpecMeas()

void InterSpec::closeShieldingSourceFitWindow()
{
  if( !m_shieldingSourceFitWindow || !m_shieldingSourceFit )
    return;
  
#if( USE_DB_TO_STORE_SPECTRA )
  m_shieldingSourceFit->saveModelIfAlreadyInDatabase();
#endif
  
  saveShieldingSourceModelToForegroundSpecMeas();
  
  delete m_shieldingSourceFitWindow;
  m_shieldingSourceFitWindow = nullptr;
  m_shieldingSourceFit = nullptr;
}//void closeShieldingSourceFitWindow()


void InterSpec::showGammaLinesWindow()
{
  if( m_referencePhotopeakLinesWindow )
  {
    m_referencePhotopeakLinesWindow->show();
    return;
  }

  if( m_toolsTabs && m_referencePhotopeakLines )
    m_toolsTabs->removeTab( m_referencePhotopeakLines );

  
  std::string xml_state;
  
  if( m_referencePhotopeakLines )
  {
    if( !m_referencePhotopeakLines->currentlyShowingNuclide().empty()
        || m_referencePhotopeakLines->persistedNuclides().size() )
    m_referencePhotopeakLines->serialize( xml_state );
    
    m_referencePhotopeakLines->clearAllLines();
    delete m_referencePhotopeakLines;
    m_referencePhotopeakLines = NULL;
  }//if( m_referencePhotopeakLines )

  m_referencePhotopeakLinesWindow = new AuxWindow( GammaLinesTabTitle, (AuxWindowProperties::TabletModal) );
  m_referencePhotopeakLinesWindow->contents()->setOverflow(WContainerWidget::OverflowHidden);
  m_referencePhotopeakLinesWindow->rejectWhenEscapePressed();

  m_referencePhotopeakLines = new ReferencePhotopeakDisplay( m_spectrum,
                                               m_materialDB.get(),
                                               m_shieldingSuggestion,
                                               this );
  setReferenceLineColors( nullptr );
  
  if( xml_state.size() )
    m_referencePhotopeakLines->deSerialize( xml_state );

  Wt::WGridLayout *layout = new Wt::WGridLayout();
  layout->setContentsMargins(5,5,5,5);
  m_referencePhotopeakLinesWindow->contents()->setLayout(layout);
  layout->addWidget( m_referencePhotopeakLines, 0, 0 );

  Wt::WPushButton *closeButton = m_referencePhotopeakLinesWindow->addCloseButtonToFooter("Close",true);
  
  if( isPhone() )
  {
    m_referencePhotopeakLines->displayingNuclide().connect( boost::bind( &WPushButton::setText, closeButton, WString("Show Lines")) );
    m_referencePhotopeakLines->nuclidesCleared().connect( boost::bind( &WPushButton::setText, closeButton, WString("Close")) );
  }
  
  closeButton->clicked().connect( m_referencePhotopeakLinesWindow, &AuxWindow::hide );
  m_referencePhotopeakLinesWindow->finished().connect( boost::bind( &InterSpec::closeGammaLinesWindow, this ) );
  
  AuxWindow::addHelpInFooter( m_referencePhotopeakLinesWindow->footer(),
                              "reference-gamma-lines-dialog" );
  
  double w = renderedWidth();
  if( w < 100.0 )
    w = 800.0;
  w = std::min( w, 800.0 );
  
  m_referencePhotopeakLinesWindow->resize( WLength(w,WLength::Pixel), WLength(310,WLength::Pixel));
  m_referencePhotopeakLinesWindow->setResizable(true);
  m_referencePhotopeakLinesWindow->resizeToFitOnScreen();
  m_referencePhotopeakLinesWindow->centerWindow();
  m_referencePhotopeakLinesWindow->show();
  
  if( m_toolsTabs )
    m_currentToolsTab = m_toolsTabs->currentIndex();
}//void showGammaLinesWindow()


void InterSpec::closeGammaLinesWindow()
{
  if( !m_referencePhotopeakLinesWindow )
    return;
  
  //When the "back" button is pressed on mobile phones
  if( isPhone() && m_referencePhotopeakLinesWindow->isHidden() )
    return;
  
  if( isPhone() )
  {
    m_referencePhotopeakLinesWindow->hide();
    return;
  }
  
  string xmlstate;
  if( m_referencePhotopeakLines )
  {
    if( !m_referencePhotopeakLines->currentlyShowingNuclide().empty()
         || m_referencePhotopeakLines->persistedNuclides().size() )
      m_referencePhotopeakLines->serialize( xmlstate );
    m_referencePhotopeakLines->clearAllLines();
    if( m_toolsTabs && m_toolsTabs->indexOf( m_referencePhotopeakLines ) >= 0 )
      m_toolsTabs->removeTab( m_referencePhotopeakLines );
    delete m_referencePhotopeakLines;
    m_referencePhotopeakLines = 0;
  }//if( m_referencePhotopeakLines )

  delete m_referencePhotopeakLinesWindow;
  m_referencePhotopeakLinesWindow = 0;

  if( m_toolsTabs )
  {
    m_referencePhotopeakLines = new ReferencePhotopeakDisplay( m_spectrum,
                                                   m_materialDB.get(),
                                                   m_shieldingSuggestion,
                                                   this );
    setReferenceLineColors( nullptr );
    
    m_toolsTabs->addTab( m_referencePhotopeakLines, GammaLinesTabTitle, TabLoadPolicy );
    
    if( xmlstate.size() )
      m_referencePhotopeakLines->deSerialize( xmlstate );
  }//if( m_toolsTabs )
  
  if( m_toolsTabs )
    m_currentToolsTab = m_toolsTabs->currentIndex();
}//void closeGammaLinesWindow()


void InterSpec::handleToolTabChanged( int tab )
{
  if( !m_toolsTabs )
    return;
  
  const int refTab = m_toolsTabs->indexOf(m_referencePhotopeakLines);
  const int calibtab = m_toolsTabs->indexOf(m_calibrateContainer);
  const int searchTab = m_toolsTabs->indexOf(m_isotopeSearchContainer);
  
  if( m_referencePhotopeakLines && (tab == refTab) && !isMobile() )
    m_referencePhotopeakLines->setFocusToIsotopeEdit();
    
  if( m_isotopeSearch && (m_currentToolsTab==searchTab) )
    m_isotopeSearch->clearSearchEnergiesOnClient();
  
  if( m_isotopeSearch && (tab==searchTab) )
    m_isotopeSearch->loadSearchEnergiesToClient();
  
  if( tab == calibtab )
  {
    m_recalibrator->refreshRecalibrator();
    if( InterSpecUser::preferenceValue<bool>( "ShowTooltips", this ) )
      passMessage( "You can also recalibrate graphically by right-clicking and "
                   "dragging the spectrum to where you want",
                   "", WarningWidget::WarningMsgInfo );
  }//if( tab == calibtab )
  
  m_currentToolsTab = tab;
}//void InterSpec::handleToolTabChanged( int tabSwitchedTo )


SpecMeasManager *InterSpec::fileManager()
{
  return m_fileManager;
}


PeakModel *InterSpec::peakModel()
{
  return m_peakModel;
};


Wt::Signal<std::shared_ptr<DetectorPeakResponse> > &InterSpec::detectorChanged()
{
  return m_detectorChanged;
}


Wt::Signal<std::shared_ptr<DetectorPeakResponse> > &InterSpec::detectorModified()
{
  return m_detectorModified;
}


float InterSpec::sample_real_time_increment(
                                   const std::shared_ptr<const SpecMeas> &meas,
                                   const int sample,
                                   const std::set<int> &detector_numbers )
{
  if( !meas )
    return 0.0f;
  
  const vector<MeasurementConstShrdPtr> &measurement
                                         = meas->sample_measurements( sample );
  float realtime = 0.0f;
  for( const auto &m : measurement )
  {
    if( detector_numbers.count(m->detector_number()) )
      realtime = std::max( realtime, m->real_time() );
  }
  return realtime;
  
  
/*
  int nback = 0, nnonback = 0;
  double backtime = 0.0, nonbacktime = 0.0;
  for( const MeasurementConstShrdPtr &m : measurement )
  {
    if( m->source_type() == Measurement::Background )
    {
      ++nback;
      backtime += m->real_time();
    }else
    {
      ++nnonback;
      nonbacktime += m->real_time();
    }
  }//for( const MeasurementConstShrdPtr &m : measurement )
  
  if( nnonback )
    return nonbacktime/nnonback;
  if( nback )
    return backtime/nback;
  return 0.0;
*/
}//double sample_real_time_increment()


std::set<int> InterSpec::timeRangeToSampleNumbers( double t0, double t1 )
{
  //t0 may be the exact lower edge, and t1 may be the exact upper edge, so we
  //  need to add/subtract a small epsilon so we dont extend into the
  //  neighboring bins
  
  if( t0 > t1 )
    std::swap( t0, t1 );
  
  t0 += 1.0E-6;
  t1 -= 1.0E-6;

  set<int> answer;
  
  if( !m_dataMeasurement )
    return answer;
  
  const vector<pair<float,int> > binning = passthroughTimeToSampleNumber();
  
  if( binning.empty() )
    return answer;
  
  size_t startind, endind;
  for( startind = 0; startind < (binning.size()-1); ++startind )
    if( binning[startind+1].first > t0 )
      break;
  for( endind = startind; endind < (binning.size()-1); ++endind )
    if( binning[endind+1].first > t1 )
      break;
  
  for( size_t i = startind; i <= endind; ++i )
    answer.insert( binning[i].second );
  
  return answer;
}//timeRangeToSampleNumbers(...)


/*
double InterSpec::liveTime( const std::set<int> &samplenums ) const
{
  double time = 0.0;
  
  if( !m_dataMeasurement )
    return 0.0;
  
  const vector<bool> det_use = detectors_to_display();
  const set<int> sample_numbers = validForegroundSamples();
  
  const vector<int> &detnums = m_dataMeasurement->detector_numbers();
  const vector<int>::const_iterator detnumbegin = detnums.begin();
  const vector<int>::const_iterator detnumend = detnums.end();
  
  for( int sample : samplenums )
  {
    const vector<MeasurementConstShrdPtr> measurement
    = m_dataMeasurement->sample_measurements( sample );
    
    for( const MeasurementConstShrdPtr &m : measurement )
    {
      const int detn = m->detector_number();
      const size_t detpos = std::find(detnumbegin,detnumend,detn) - detnumbegin;
      
      if( detpos < det_use.size() && det_use[detpos] )
        time += m->live_time();
    }//for( const MeasurementConstShrdPtr &m : measurement )
  }//for( int sample : prev_displayed_samples )

  return time;
}//double liveTime( const std::set<int> &samplenums );
*/

void InterSpec::changeDisplayedSampleNums( const std::set<int> &samples,
                                                const SpectrumType type )
{
  std::shared_ptr<SpecMeas> meas = measurment( type );
  
  if( !meas )
    return;
  
  std::shared_ptr<const Measurement> prevhist = displayedHistogram(type);
  
  std::set<int> *sampleset = 0;
  
  switch( type )
  {
    case kForeground:
      sampleset = &m_displayedSamples;
      deletePeakEdit();
    break;
      
    case kSecondForeground:
      sampleset = &m_sectondForgroundSampleNumbers;
    break;
      
    case kBackground:
      sampleset = &m_backgroundSampleNumbers;
    break;
  }//switch( type )
  
  if( (*sampleset) == samples )
    return;
  
  (*sampleset) = samples;
  if( sampleset->empty() && !!meas )
    (*sampleset) = meas->sample_numbers();
  
  //should update the highlighted regions right here
  vector< pair<double,double> > regions = timeRegionsToHighlight( type );
  m_timeSeries->setTimeHighLightRegions( regions, type );
  
  switch( type )
  {
    case kForeground:
      displayForegroundData( true );
    break;
      
    case kSecondForeground:
      displaySecondForegroundData();
    break;
      
    case kBackground:
      displayBackgroundData();
    break;
  }//switch( type )
  
  //Right now, we will only search for hint peaks for foreground
#if( !ANDROID && !IOS )
  switch( type )
  {
    case kForeground:
      if( !!m_dataMeasurement
         && !m_dataMeasurement->automatedSearchPeaks(samples) )
        searchForHintPeaks( m_dataMeasurement, samples );
      break;
      
    case kSecondForeground:
    case kBackground:
      break;
  }//switch( spec_type )
#endif
  
  m_displayedSpectrumChangedSignal.emit( type, meas, (*sampleset) );
}//void InterSpec::changeDisplayedSampleNums( const std::set<int> &samples )


void InterSpec::sampleNumbersToDisplayAddded( const double t0,
                                                   const double t1,
                                                   const SpectrumType type )
{
  std::shared_ptr<SpecMeas> meas = measurment( type );
  
  if( !m_dataMeasurement )
    return;
  
  const set<int> newSampleNums = timeRangeToSampleNumbers( t0, t1 );
  
  if( meas != m_dataMeasurement )
  {
    setSpectrum( m_dataMeasurement, newSampleNums, type, false );
  }else
  {
    //if newSampleNums is entirely in sampleNums, then we will remove them
    bool hasAll = true;
    set<int> sampleNums = displayedSamples( type );
  
    for( int i : newSampleNums )
    {
      if( !sampleNums.count(i) )
      {
        hasAll = false;
        break;
      }
    }//for( int i : newSampleNums )
  
    if( hasAll )
    {
      for( int i : newSampleNums )
        sampleNums.erase( i );
    }else
    {
      sampleNums.insert( newSampleNums.begin(), newSampleNums.end() );
    }//if( hasAll )
    
    changeDisplayedSampleNums( sampleNums, type );
  }//if( meas != m_dataMeasurement )
}//void sampleNumbersToDisplayAddded( const double t0, const double t1 )


void InterSpec::changeTimeRange( const double t0, const double t1,
                                      const SpectrumType type )
{
  if( !m_dataMeasurement )
    return;
  
  const bool updateothers = (type==kForeground && !displayedHistogram(kForeground) );
  
  std::shared_ptr<SpecMeas> meas = measurment( type );
  const set<int> sampleNums = timeRangeToSampleNumbers( t0, t1 );
  
  if( meas != m_dataMeasurement )
    setSpectrum( m_dataMeasurement, sampleNums, type, false );
  else
    changeDisplayedSampleNums( sampleNums, type );
  
  if( updateothers && (m_backgroundMeasurement == m_dataMeasurement) )
  {
    //cerr << "Here" << endl;
    //For passthrough spectra, if you go from no foreground to having a
    //  foreground, and you previously had a background from the same file,
    //  the background will be highlighted on the time series chart, but the
    //  spectrum wont show up, so we should catch this
  }
  
}//void changeTimeRange( const double t0, const double t1 )



void InterSpec::findAndSetExcludedSamples( std::set<int> definetly_keep_samples )
{
  m_excludedSamples.clear();

  if( !m_dataMeasurement || !m_dataMeasurement->passthrough() )
    return;

  const set<int> all_samples = m_dataMeasurement->sample_numbers();

  for( const int sample : all_samples )
  {
    vector< MeasurementConstShrdPtr > measurements = m_dataMeasurement->sample_measurements( sample );

    if( definetly_keep_samples.count(sample) > 0 )
      continue;

    if( measurements.empty() )
    {
      m_excludedSamples.insert( sample );
    }else
    {
      const MeasurementConstShrdPtr meas = measurements.front();
      //XXX - Assuming background and calibration statis is the same for all
      //      detectors
      //const bool back = (meas->source_type() == Measurement::Background);
      const bool calib = (meas->source_type() == Measurement::Calibration);

      if( /*back ||*/ calib )
        m_excludedSamples.insert( sample );
    }//if( measurements.empty() ) / else
  }//for( const int sample : all_samples )
}//void InterSpec::findAndSetExcludedSamples()


std::set<int> InterSpec::validForegroundSamples() const
{
  set<int> sample_nums;

  if( !m_dataMeasurement )
    return sample_nums;

  sample_nums = m_dataMeasurement->sample_numbers();
  for( const int s : m_excludedSamples )
    sample_nums.erase( s );
  
  set<int> torm;
  for( const int s : sample_nums )
  {
    const vector< MeasurementConstShrdPtr > meas
                               = m_dataMeasurement->sample_measurements(s);
    for( const MeasurementConstShrdPtr &m : meas )
    {
      if( m->source_type() == Measurement::Background )
        torm.insert( s );
    }
  }//for( const int s : sample_nums )
  
  for( const int s : torm )
    sample_nums.erase( s );

  return sample_nums;
}//std::set<int> validForegroundSamples() const


void InterSpec::emitDetectorChanged()
{
  WApplication *app = wApp;
  if( !app )
  {
    cerr << "InterSpec::emitDetectorChanged: no app available" << endl;
    return;
  }
  
  std::shared_ptr<DetectorPeakResponse> det;
  if( !!m_dataMeasurement )
  {
    std::lock_guard<std::recursive_mutex> scoped_lock( m_dataMeasurement->mutex() );
    det = m_dataMeasurement->detector();
  }
  
  m_detectorChanged.emit( det );
  app->triggerUpdate();
}//void emitDetectorChanged( std::shared_ptr<DetectorPeakResponse> det )


#if( APPLY_OS_COLOR_THEME_FROM_JS && !BUILD_AS_OSX_APP )
void InterSpec::initOsColorThemeChangeDetect()
{
  m_osColorThemeChange.reset( new JSignal<std::string>( this, "OsColorThemeChange", true ) );
  m_osColorThemeChange->connect( boost::bind( &InterSpec::osThemeChange, this, _1 ) );
  
  LOAD_JAVASCRIPT(wApp, "js/InterSpec.js", "InterSpec", wtjsSetupOsColorThemeChangeJs);
  
  doJavaScript( "Wt.WT.SetupOsColorThemeChangeJs('" + id() + "')" );
}//void initOsColorThemeChangeDetect()
#endif


void InterSpec::loadDetectorToPrimarySpectrum( DetectorType type,
                                                    std::shared_ptr<SpecMeas> meas,
                                                    const string sessionId,
                                                    bool keepModStatus,
                                                    boost::function<void(void)> modifiedcallback )
{
  if( !meas )
    return;
  
  std::shared_ptr<DetectorPeakResponse> det;

  //First see if the user has opted for a detector for this serial number of
  //  detector model
  det = DetectorEdit::getUserPrefferedDetector( m_sql, m_user, meas );
  
  const bool usingUserDefaultDet = !!det;
  
  if( !det )
  {
    if( type == kUnknownDetector )
      return;
    
    try
    {
      switch( type )
      {
        case kIdentiFinderDetector:
        case kIdentiFinderNGDetector:
        case kIdentiFinderLaBr3Detector:
          det = DetectorEdit::initARelEffDetector( type, this );
        break;
        
        default:
          break;
      }
    
      if( !det )
        det = DetectorEdit::initAGadrasDetector( type, this );
    }catch( std::exception &e )
    {
      cerr << "InterSpec::loadDetectorToPrimarySpectrum caught: "
           << e.what() << endl;
      return;
    }
  }//if( !det )

  if( !det )
    return;
  
  {//begin meas->mutex_ protected codeblock
    std::lock_guard<std::recursive_mutex> scoped_lock( meas->mutex() );
    if( meas->detector() /*|| meas->detector_type()!=kUnknownDetector*/ )
      return;
    
    const bool wasModified = meas->modified();
    const bool wasModifiedSinceDecode = meas->modified_since_decode();
    
    meas->setDetector( det );
    
    if( keepModStatus )
    {
      if( !wasModified )
        meas->reset_modified();
      if( !wasModifiedSinceDecode )
        meas->reset_modified_since_decode();
    }//if( keepModStatus )
  }//end meas->mutex_ protected codeblock

  if( !modifiedcallback.empty() )
    WServer::instance()->post( sessionId, modifiedcallback );

  if( usingUserDefaultDet )
  {
    WServer::instance()->post( sessionId, std::bind( [](){
      //ToDo: could add button to remove association with DRF in database,
      //      similar to the "Start Fresh Session" button.  Skeleton code to do this
      /*
       std::unique_ptr<Wt::JSignal<> > m_clearDrfAssociation;
       m_clearDrfAssociation.reset( new JSignal<>(this, "removeDrfAssociation", false) );
       m_clearDrfAssociation->connect( this, &InterSpec::... );
       
       WStringStream js;
       js << "<div onclick=\"Wt.emit('" << id() << "',{name:'removeDrfAssociation'});"
       "$('.qtip.jgrowl:visible:last').remove();return false;\" "
       "class=\"clearsession\"><span class=\"clearsessiontxt\">Remove association of detector with DRF.</span></div>";
       
       passMessage( "Using the detector response function you specified to use as default for this detector."
                    + js.str(), "", WarningWidget::WarningMsgInfo );
       */
      
      passMessage( "Using the detector response function you specified to use as default for this detector.",
                   "", WarningWidget::WarningMsgInfo );
    }) );
  }//if( usingUserDefaultDet )
  
}//void InterSpec::loadDetectorToPrimarySpectrum( WApplication *app )


void InterSpec::doFinishupSetSpectrumWork( std::shared_ptr<SpecMeas> meas,
                                  vector<boost::function<void(void)> > workers )
{
  cout << "In doFinishupSetSpectrumWork" << endl;
  if( !meas || workers.empty() )
    return;
  
  bool modified, modifiedSinceDecode;
  cout << "About to grab locl" << endl;
  {//begin codeblock to access meas
    std::lock_guard<std::recursive_mutex> scoped_lock( meas->mutex() );
    modified = meas->modified();
    modifiedSinceDecode = meas->modified_since_decode();
  }//end codeblock to access meas
  cout << "About to do Threadpool" << endl;
  {
    SpecUtilsAsync::ThreadPool pool;
    for( size_t i = 0; i < workers.size(); ++i )
      pool.post( workers[i] );
    pool.join();
  }
  cout << "About to reset flags" << endl;
  {//begin codeblock to access meas
    std::lock_guard<std::recursive_mutex> scoped_lock( meas->mutex() );
    if( !modified )
      meas->reset_modified();
    if( !modifiedSinceDecode )
      meas->reset_modified_since_decode();
  }//end codeblock to access meas
  cout << "Done In doFinishupSetSpectrumWork" << endl;
}//void InterSpec::doFinishupSetSpectrumWork( boost::function<void(void)> workers )


void InterSpec::setSpectrum( std::shared_ptr<SpecMeas> meas,
                                    std::set<int> sample_numbers,
                                  const SpectrumType spec_type,
                                  const bool checkForPrevioudEnergyCalib )
{
  vector< boost::function<void(void)> > furtherworkers;
  
  bool wasModified = false, wasModifiedSinceDecode = false;
  
  if( !!meas )
  {
    wasModified = meas->modified();
    wasModifiedSinceDecode = meas->modified_since_decode();
    
    if( m_useInfoWindow )
    {
      delete m_useInfoWindow;
      m_useInfoWindow = nullptr;
    }
    
/*
    const std::vector<std::string> &names = meas->detector_names();
    std::vector< std::shared_ptr<const Measurement> > meass = meas->measurements();
    
    vector<ofstream> outfiles( names.size() );
    for( size_t i = 0; i < names.size(); ++i )
      outfiles[i].open( ("/Users/wcjohns/Downloads/det_" + names[i] + ".txt").c_str() );
    vector<float> max_energies( names.size(), 0.0f );
    
    for( size_t i = 0; i < meass.size(); ++i )
    {
      const size_t nchannel = meass[i]->num_gamma_channels();
      if( !meass[i]->gamma_channel_contents() || nchannel < 10)
        continue;
      
      const size_t index = std::find( names.begin(), names.end(), meass[i]->detector_name() ) - names.begin();
      for( size_t j = 0; j < meass[i]->gamma_channel_contents()->size(); ++j )
        outfiles[index] << meass[i]->gamma_channel_contents()->at(j) << " ";
      outfiles[index] << endl;
      max_energies[index] = max( max_energies[index], meass[i]->gamma_channel_upper(nchannel-1));
    }
*/
    
/*
    MeasurementShrdPtr sumspec = meas->sum_measurements( meas->sample_numbers(), vector<bool>(meas->detector_numbers().size(),true) );
    const std::shared_ptr< const std::vector<float> > &counts = sumspec->gamma_channel_contents();
    
    ofstream output( ("/Users/wcjohns/sum_" + meas->filename() + ".txt").c_str() );
    output << "LiveTime: " << sumspec->live_time() << endl;
    output << "RealTime: " << sumspec->real_time() << endl;
    for( size_t i = 0; i < counts->size(); ++i )
      output << (*counts)[i] << " ";
    output << endl;
*/
  }//if( meas )
  
  std::shared_ptr<SpecMeas> previous = measurment(spec_type);
  const bool sameSpec = (meas==previous);
  std::shared_ptr<const Measurement> prev_display = m_spectrum->histUsedForXAxis();
  

  if( (spec_type == kForeground) && previous && (previous != meas) )
  {
    closeShieldingSourceFitWindow();
    
#if( USE_DB_TO_STORE_SPECTRA )
    if( m_user->preferenceValue<bool>( "AutoSaveSpectraToDb" ) )
    {
      //We also need to do this in the InterSpec destructor as well.
      //   Also maybe change size limitations to only apply to auto saving
      if( m_currentStateID >= 0 )
      {
        //Save to (HEAD) of current state
      }else
      {
        //Create a state
        //Handle case where file is to large to be saved
      }
    }
#endif //#if( USE_DB_TO_STORE_SPECTRA )
  }//if( (spec_type == kForeground) && !!previous && (previous != meas) )
  
  if( !!meas && isMobile() && !toolTabsVisible() /* && checkForPrevioudEnergyCalib */
      && m_referencePhotopeakLines  && (spec_type == kForeground) )
    m_referencePhotopeakLines->clearAllLines();
  
  string msg;
  switch( spec_type )
  {
    case kForeground:
#if( USE_DB_TO_STORE_SPECTRA )
      m_currentStateID = -1;
      updateSaveWorkspaceMenu();
#endif
      if( !sameSpec )
        deletePeakEdit();
    break;
    
    case kSecondForeground: case kBackground:
    break;
  }//switch( spec_type )

  if( meas
      /*&& sample_numbers.empty() */
      && !sameSpec
      && (spec_type==meas->displayType())
      && meas->displayedSampleNumbers().size() )
  {
    sample_numbers = meas->displayedSampleNumbers();
  }

#if( USE_SAVEAS_FROM_MENU )
  if( m_downloadMenu && m_downloadMenus[spec_type] )
  {
    m_downloadMenus[spec_type]->setDisabled( !meas );
    WMenuItem *item = m_downloadMenus[spec_type]->parentItem();
    if( item )
      m_downloadMenu->setItemHidden( item, !meas );
      
    bool allhidden=true;
    for( SpectrumType i = SpectrumType(0); i<=kBackground; i = SpectrumType(i+1) )
    {
      if (!m_downloadMenu->isItemHidden( m_downloadMenus[i]->parentItem()))
      {
        allhidden=false;
        break;
      }//if (!m_downloadMenus[i]->isHidden())
    }//    for( SpectrumType i = SpectrumType(0); i<=kBackground; i = SpectrumType(i+1) )
      
    m_fileMenuPopup->setItemHidden(m_downloadMenu->parentItem(),allhidden);
  }//if( m_downloadMenu && m_downloadMenus[spec_type] )
#endif
  
  
  switch( spec_type )
  {
    case kForeground:
      m_detectorChangedConnection.disconnect();
      m_detectorModifiedConnection.disconnect();
      m_displayedSpectrumChanged.disconnect();
      
      if( meas )
      {
        //Lets keep using the same detector if we are loading a new spectrum
        //  with the same number of bins, but doesnt have a detector of its own
        const bool sameNBins = ( m_dataMeasurement && meas
               && (m_dataMeasurement->num_gamma_channels()==meas->num_gamma_channels()) );

        std::shared_ptr<DetectorPeakResponse> old_det;
        if( m_dataMeasurement )
          old_det = m_dataMeasurement->detector();

        if( meas && (!meas->detector() || !meas->detector()->isValid() )
            && old_det && old_det->isValid()
            && sameNBins && meas->num_gamma_channels() )
        {
          meas->setDetector( old_det );
        }

        if( meas->detector() != old_det )
        {
          auto drf = meas->detector();
          m_detectorChanged.emit( meas->detector() );
          
          if( drf )
          {
            auto drfcopy = std::make_shared<DetectorPeakResponse>( *drf );
            boost::function<void(void)> worker = boost::bind( &DetectorEdit::updateLastUsedTimeOrAddToDb, drfcopy, m_user.id(), m_sql );
            WServer::instance()->ioService().post( worker );
          }
        }//if( meas->detector() != old_det )
        
        DetectorType detType = meas->detector_type();
        if( detType == kUnknownDetector )
          detType = SpecMeas::guessDetectorTypeFromFileName( meas->filename() );
      
        if( !meas->detector() /* && (detType != kUnknownDetector) */ )
        {
          boost::function<void()> updateemit
                  = wApp->bind( boost::bind( &InterSpec::emitDetectorChanged,
                                this ) );
          
          boost::function<void()> worker
                  = wApp->bind( boost::bind( &InterSpec::loadDetectorToPrimarySpectrum,
                          this, detType, meas, wApp->sessionId(), true, updateemit ) );
          furtherworkers.push_back( worker );
        }//if( we could try to load a detector type )
        
        m_detectorChangedConnection = m_detectorChanged.connect( boost::bind( &SpecMeas::detectorChangedCallback, meas.get(), _1 ) );
        m_detectorModifiedConnection = m_detectorModified.connect( boost::bind( &SpecMeas::detectorChangedCallback, meas.get(), _1 ) );
        m_displayedSpectrumChanged = m_displayedSpectrumChangedSignal.connect( boost::bind( &SpecMeas::displayedSpectrumChangedCallback, meas.get(), _1, _2, _3 ) );
      }//if( meas )

      m_dataMeasurement = meas;
      
      findAndSetExcludedSamples( sample_numbers );

      if( !sameSpec && m_shieldingSourceFit )
        m_shieldingSourceFit->newForegroundSet();
    break;

    case kSecondForeground:
      m_secondDataMeasurement = meas;
      m_sectondForgroundSampleNumbers = sample_numbers;
      if( meas && m_sectondForgroundSampleNumbers.empty() )
        m_sectondForgroundSampleNumbers = meas->sample_numbers();
    break;

    case kBackground:
      m_backgroundMeasurement = meas;
      m_backgroundSampleNumbers = sample_numbers;
      if( meas && m_backgroundSampleNumbers.empty() )
        m_backgroundSampleNumbers = meas->sample_numbers();
    break;
  };//switch( spec_type )

  if( msg.size() )
    passMessage( msg, "", 0 );

  
  //If loading a new foreground that has a different number of channels than
  //  the background/secondary, and differnet number of bins than previous
  //  foreground, get rid of the background/secondary since the user probably
  //  isnt interested in files from a completely different detector anymore.
  if( spec_type == kForeground && m_dataMeasurement && !sameSpec )
  {
    //Assume we will use all the detectors (just to determine binning)
    const vector<bool> use_gamma( m_dataMeasurement->detector_names().size(), true );
    
    ShrdConstFVecPtr binning = getBinning( sample_numbers,
                                           use_gamma, m_dataMeasurement );
    ShrdConstFVecPtr prev_binning = prev_display ? prev_display->channel_energies() : ShrdConstFVecPtr();
    
    const bool diff_fore_nchan = ((!prev_binning || !binning) || (prev_binning->size() != binning->size()));
    
    const size_t num_foreground_channels = binning ? binning->size() : 0;
    size_t num_sec_channel = 0, num_back_channel = 0;
    if( m_secondDataMeasurement )
      num_sec_channel = m_secondDataMeasurement->num_gamma_channels();
    if( m_backgroundMeasurement )
      num_back_channel = m_backgroundMeasurement->num_gamma_channels();
    
    if( diff_fore_nchan && num_sec_channel && (num_sec_channel != num_foreground_channels) )
    {
#if( USE_SAVEAS_FROM_MENU )
      m_downloadMenus[kSecondForeground]->setDisabled( true );
      WMenuItem *item = m_downloadMenus[kSecondForeground]->parentItem();
      if( item )
        m_downloadMenu->setItemHidden( item, true );
#endif
      
      m_secondDataMeasurement = std::shared_ptr<SpecMeas>();
      m_spectrum->setSecondData( std::shared_ptr<Measurement>(), -1.0, -1.0, -1.0, false );
      
      m_displayedSpectrumChangedSignal.emit( kSecondForeground,
                                            m_secondDataMeasurement,
                                            std::set<int>() );
    }//if( num_sec_channel )
    
    if( diff_fore_nchan && num_back_channel && num_foreground_channels && (num_back_channel != num_foreground_channels) )
    {
#if( USE_SAVEAS_FROM_MENU )
      m_downloadMenus[kBackground]->setDisabled( true );
      WMenuItem *item = m_downloadMenus[kBackground]->parentItem();
      if( item )
        m_downloadMenu->setItemHidden( item, true );
#endif
      
      m_backgroundMeasurement = std::shared_ptr<SpecMeas>();
      m_spectrum->setBackground( std::shared_ptr<Measurement>(), -1.0, -1.0, -1.0 );
      m_displayedSpectrumChangedSignal.emit( kBackground,
                                            m_backgroundMeasurement,
                                            std::set<int>() );
    }//if( nSecondBins )
  }//if( spec_type == kForeground )
  
  
  
  //Fall throughs intentional
  switch( spec_type )
  {
    case kForeground:
      updateGuiForPrimarySpecChange( sample_numbers );
#if( USE_DB_TO_STORE_SPECTRA )
      m_saveStateAs->setDisabled( !m_dataMeasurement );
#endif
    case kSecondForeground:
      displaySecondForegroundData();
    case kBackground:
      displayBackgroundData();
  };//switch( spec_type )
  
  
  deletePreserveCalibWindow();
  
  if( checkForPrevioudEnergyCalib && !sameSpec && m_recalibrator && !!meas && !!m_dataMeasurement )
  {
    switch( spec_type )
    {
      case kForeground:
        if( PreserveCalibWindow::candidate(meas,previous) )
          m_preserveCalibWindow = new PreserveCalibWindow( meas, spec_type,
                                         previous, spec_type, m_recalibrator );
      break;
    
      case kSecondForeground:
      case kBackground:
        if( PreserveCalibWindow::candidate(meas,m_dataMeasurement) )
          m_preserveCalibWindow = new PreserveCalibWindow( meas, spec_type,
                                                m_dataMeasurement, kForeground,
                                                m_recalibrator );
      break;
    };//switch( spec_type )
  
    if( m_preserveCalibWindow )
      m_preserveCalibWindow->finished().connect( this, &InterSpec::deletePreserveCalibWindow );
  }//if( !sameSpec && m_recalibrator && !!meas )
  
  
  switch( spec_type )
  {
    case kForeground:
      if( !!m_dataMeasurement && ((!m_dataMeasurement)!=(!previous)) )
        doJavaScript( "$('.Wt-domRoot').data('HasForeground',1);" );
      else if( !m_dataMeasurement )
        doJavaScript( "$('.Wt-domRoot').data('HasForeground',0);" );
    break;
    
    case kSecondForeground: case kBackground:
      if( !!meas && !m_dataMeasurement )
      {
        passMessage( "You must load a foreground spectrum before viewing a"
                     " background or second foreground spectrum.",
                    "", WarningWidget::WarningMsgHigh );
      }
    break;
  }//switch( spec_type )
  
  
  // Update the recalibrator, as there is new data.
  if( m_recalibrator )
    m_recalibrator->refreshRecalibrator();
  m_displayedSpectrumChangedSignal.emit( spec_type, meas, sample_numbers );
  
  if( meas )
  {
    if( !wasModified )
      meas->reset_modified();
    if( !wasModifiedSinceDecode )
      meas->reset_modified_since_decode();
  }//if( meas )
  
#if( USE_GOOGLE_MAP )
//  m_secondDataMeasurement && m_dataMeasurement m_backgroundMeasurement
  const bool hasGps = (m_dataMeasurement && m_dataMeasurement->has_gps_info());
  if( m_mapMenuItem )
    m_mapMenuItem->setDisabled( !hasGps );
#endif
  
#if( USE_SEARCH_MODE_3D_CHART )
  const bool isSearchData = (m_dataMeasurement && m_dataMeasurement->passthrough());
  if( m_searchMode3DChart )
    m_searchMode3DChart->setDisabled( !isSearchData );
#endif

  
  //Right now, we will only search for hint peaks for foreground
#if( !ANDROID && !IOS )
  switch( spec_type )
  {
    case kForeground:
    {
      if( m_dataMeasurement )
      {
        auto peaks = m_dataMeasurement->automatedSearchPeaks(sample_numbers);
        if( !peaks )
          searchForHintPeaks( m_dataMeasurement, sample_numbers );
      }
      break;
    }
      
    case kSecondForeground:
    case kBackground:
      break;
  }//switch( spec_type )
#endif
  
  if( meas && furtherworkers.size() )
  {
    boost::function<void(void)> worker = boost::bind(
                                  &InterSpec::doFinishupSetSpectrumWork,
                                  this, meas, furtherworkers );
    WServer::instance()->ioService().post( worker );
  }//if( meas && furtherworkers.size() )
  
  if( m_mobileBackButton && m_mobileForwardButton )
  {
    if( !toolTabsVisible() && meas && spec_type==kForeground
        && !meas->passthrough() && (meas->sample_numbers().size()>1) )
    {
      m_mobileBackButton->setHidden(false);
      m_mobileForwardButton->setHidden(false);
    }else
    {
      m_mobileBackButton->setHidden(true);
      m_mobileForwardButton->setHidden(true);
    }
  }//if( m_mobileBackButton && m_mobileForwardButton )
  
  
  //Check for warnings from parsing the file, and display to user - not
  //  implemented well yet.
  if( spec_type == kForeground && m_dataMeasurement && !sameSpec )
  {
    try
    {
      const vector<bool> use_gamma( m_dataMeasurement->detector_names().size(), true );
      const size_t index = m_dataMeasurement->suggested_gamma_binning_index( sample_numbers, use_gamma );
      auto meas = m_dataMeasurement->measurements().at(index);
      if( meas && meas->energy_calibration_model() == Measurement::EquationType::UnspecifiedUsingDefaultPolynomial )
      {
        passMessage( "Warning, could not find the energy calibration, so using a default one", "", WarningWidget::WarningMsgMedium );
      }
    }catch( std::exception & )
    {
    }
    
    
  }//if( spec_type == kForeground && m_dataMeasurement && !sameSpec )
  
  
  //Display a notice to the user about how they can select different portions of
  //  passthrough/search-mode data
  if( spec_type==kForeground && !!m_dataMeasurement
      && m_dataMeasurement->passthrough() )
  {
    const bool showToolTipInstantly
                 = InterSpecUser::preferenceValue<bool>( "ShowTooltips", this );
  
    if( showToolTipInstantly )
    {
      const char *tip = "Clicking and dragging on the time-series (bottom)"
      " chart, will change the time range the energy spectrum"
      " is summed over.  You can also shift-click or"
      " shift-drag to add additional time spans."
      " Shift-clicking or shift dragging entirely within a"
      " currently used (highlighted) time span will remove"
      " that portion of the time span. Holding the 'alt' key"
      " while doing the above will perform the same actions,"
      " but for the background if it is the same spectrum"
      " file as the foreground.";
      passMessage( tip, "", WarningWidget::WarningMsgInfo );
    }//if( showToolTipInstantly )
  }//if( passthrough foreground )
}//void setSpectrum(...)


void InterSpec::reloadCurrentSpectrum( SpectrumType spec_type )
{
  std::shared_ptr<SpecMeas> meas;
  std::set<int> sample_numbers;

  switch( spec_type )
  {
    case kForeground:
      meas = m_dataMeasurement;
      sample_numbers = m_displayedSamples;
    break;

    case kSecondForeground:
      meas = m_secondDataMeasurement;
      sample_numbers = m_sectondForgroundSampleNumbers;
    break;

    case kBackground:
      meas = m_backgroundMeasurement;
      sample_numbers = m_backgroundSampleNumbers;
    break;
  }//switch( spec_type )

  setSpectrum( meas, sample_numbers, spec_type, false );
}//void reloadCurrentSpectrum( SpectrumType spec_type )



void InterSpec::finishLoadUserFilesystemOpenedFile(
                                std::shared_ptr<SpecMeas> meas,
                                std::shared_ptr<SpectraFileHeader> header,
                                const SpectrumType type )
{
  SpectraFileModel *fileModel = m_fileManager->model();
  
  try
  {
    const int row = fileModel->addRow( header );
    m_fileManager->displayFile( row, meas, type, true, true, true );
  }catch( std::exception & )
  {
    passMessage( "There was an error loading "
                 + (!!meas ? meas->filename() : string("spectrum file")),
                "", WarningWidget::WarningMsgHigh );
  }//try / catch
  
}//finishLoadUserFilesystemOpenedFile(...)


void InterSpec::promptUserHowToOpenFile( std::shared_ptr<SpecMeas> meas,
                                             std::shared_ptr<SpectraFileHeader> header )
{
  //Dialog layout only tested on phone.
  AuxWindow *dialog = new AuxWindow( header->displayName().toUTF8(),
                  (Wt::WFlags<AuxWindowProperties>(AuxWindowProperties::IsAlwaysModal) | AuxWindowProperties::TabletModal) );
  dialog->disableCollapse();
  
  WGridLayout *layout = dialog->stretcher();
  const char *msg =
  "This file looks like its from the same "
  "detector as the current foreground."
  "<br />How would you like to open this spectrum file?";
  WText *text = new WText( msg );
  text->setAttributeValue( "style", "text-align: center;" );
  layout->addWidget( text, 0, 0, AlignMiddle );
  
  WContainerWidget *buttonDiv = new WContainerWidget();
  layout->addWidget( buttonDiv, 1, 0, AlignMiddle );
  
  WGridLayout *buttonlayout = new WGridLayout();
  buttonDiv->setLayout( buttonlayout );
  
  WPushButton *button = new WPushButton( "Foreground" );
  button->clicked().connect( dialog, &AuxWindow::hide );
  button->clicked().connect( boost::bind( &InterSpec::finishLoadUserFilesystemOpenedFile, this, meas, header, kForeground ) );
  buttonlayout->addWidget( button, 0, 0, AlignMiddle );
  button->setFocus( true );
  
  button = new WPushButton( "Background" );
  button->clicked().connect( dialog, &AuxWindow::hide );
  button->clicked().connect( boost::bind( &InterSpec::finishLoadUserFilesystemOpenedFile, this, meas, header, kBackground ) );
  buttonlayout->addWidget( button, 0, 1, AlignMiddle );
  
  button = new WPushButton( "Secondary" );
  button->clicked().connect( dialog, &AuxWindow::hide );
  button->clicked().connect( boost::bind( &InterSpec::finishLoadUserFilesystemOpenedFile, this, meas, header, kSecondForeground ) );
  buttonlayout->addWidget( button, 0, 2, AlignMiddle );
  
  dialog->finished().connect( boost::bind( &AuxWindow::deleteAuxWindow, dialog ) );
  
  dialog->show();
  dialog->centerWindow();
}//void promptUserHowToOpenFile(...)



bool InterSpec::userOpenFileFromFilesystem( const std::string path, std::string displayFileName  )
{
  try
  {
    if( displayFileName.empty() )
      displayFileName = UtilityFunctions::filename(path);
    
    if( !m_fileManager )  //shouldnt ever happen.
      throw runtime_error( "Internal logic error, no valid m_fileManager" );
    
    if( !UtilityFunctions::is_file(path) )
      throw runtime_error( "Could not access file '" + path + "'" );
  
    auto header = std::make_shared<SpectraFileHeader>( m_user, true, this );
    auto meas = header->setFile( displayFileName, path, kAutoParser );
  
    if( !meas )
      throw runtime_error( "Failed to decode file" );
    
    bool couldBeBackground = true;
    if( !m_dataMeasurement || !meas
        || meas->uuid() == m_dataMeasurement->uuid() )
    {
      couldBeBackground = false;
    }else
    {
      couldBeBackground &= (m_dataMeasurement->instrument_id() == meas->instrument_id());
      couldBeBackground &= (m_dataMeasurement->num_gamma_channels() == meas->num_gamma_channels());
    }//if( !m_dataMeasurement || !meas )

    //Should we check if this meas has the same UUID as the second of background?
    
    if( couldBeBackground )
    {
      promptUserHowToOpenFile( meas, header );
      return true;
    }else
    {
//      return m_fileManager->loadFromFileSystem( path, kForeground, kAutoParser );
      cout << "Will load file " << path << " requested to be loaded at "
      << WDateTime::currentDateTime().toString(DATE_TIME_FORMAT_STR)
      << endl;
      
      SpectraFileModel *fileModel = m_fileManager->model();
      const int row = fileModel->addRow( header );
      m_fileManager->displayFile( row, meas, kForeground, true, true, true );
      return true;
    }
  }catch( std::exception &e )
  {
    cerr << "Caught exception '" << e.what() << "' when trying to load '"
         << path << "'" << endl;
    SpecMeasManager::displayInvalidFileMsg( displayFileName, e.what() );
    return false;
  }//try / catch
  
  return true;
}//bool userOpenFileFromFilesystem( const std::string filepath )


#if( ALLOW_URL_TO_FILESYSTEM_MAP )
bool InterSpec::loadFileFromDbFilesystemLink( const int id, const bool askIfBackound )
{
  try
  {
    using namespace DbToFilesystemLink;
    FileIdToLocation fileinfo = getFileToOpenToInfo( id );
    //        const int nsec = fileinfo.m_utcRequestTime.secsTo( WDateTime::currentDateTime() );
    
    string username = fileinfo.m_userName.toUTF8();
    UtilityFunctions::ireplace_all( username, "_phone", "" );
    UtilityFunctions::ireplace_all( username, "_tablet", "" );
    UtilityFunctions::ireplace_all( username, "_mobile", "" );
    
    const bool sameUser = (username.empty() ? true
                           : (m_user->m_userName == username) );
    
    if( !fileinfo.m_foregroundFilePath.empty()
       && UtilityFunctions::is_file(fileinfo.m_foregroundFilePath.toUTF8())
       //            && fileinfo.m_utcRequestTime.isValid()
       //            && nsec < 300 && nsec >= 0
       && !fileinfo.m_fufilled
       && sameUser )
    {
      if( m_fileManager )
      {
        typedef map<SpectrumType,string> TypeToPathMap;
        TypeToPathMap specToLoadMap;
        
        specToLoadMap[kForeground] = fileinfo.m_foregroundFilePath.toUTF8();
        specToLoadMap[kSecondForeground] = fileinfo.m_secondForegroundFilePath.toUTF8();
        specToLoadMap[kBackground] = fileinfo.m_backgroundFilePath.toUTF8();
        
        for( TypeToPathMap::const_iterator iter = specToLoadMap.begin();
            iter != specToLoadMap.end(); ++iter )
        {
          SpectrumType type = iter->first;
          const string filepath = iter->second;
          
          if( filepath.empty() || !UtilityFunctions::is_file(filepath) )
            continue;
          
          const string displayName = UtilityFunctions::filename( filepath );
          
          try
          {
            std::shared_ptr<SpecMeas> meas;
            std::shared_ptr<SpectraFileHeader> header;
            
#if( SUPPORT_ZIPPED_SPECTRUM_FILES )
            {
              //This is a hack to allow opening zip files on iOS.
              //  Currently the selection GUI looks horrible, and the spectrum
              //  is always loaded as foreground.
              const string extension = UtilityFunctions::file_extension(filepath);
              bool iszip = UtilityFunctions::iequals(extension, ".zip");
              
              if( !iszip ) //check for zip files magic number to more-confirm
              {
#ifdef _WIN32
                const std::wstring wfilepath = UtilityFunctions::convert_from_utf8_to_utf16(filepath);
                ifstream test( wfilepath.c_str() );
#else
                ifstream test( filepath.c_str() );
#endif
                iszip = (test.get()==0x50 && test.get()==0x4B
                         && test.get()==0x03 && test.get()==0x04);
              }//if( !iszip )
              
              if( iszip )
              {
                if( fileManager()->handleZippedFile( displayName, filepath, -1 ) )
                  return true;
              }//if( iszip )
            }
#endif
            
            header = std::make_shared<SpectraFileHeader>( m_user, true, this );
            meas = header->setFile( displayName, filepath, kAutoParser );
            
            
            bool couldBeBackground = askIfBackound;
            if( !m_dataMeasurement || !meas
                || meas->uuid() == m_dataMeasurement->uuid() )
            {
              couldBeBackground = false;
            }else if( askIfBackound )
            {
              couldBeBackground &= (type == kForeground);
              couldBeBackground &= (m_dataMeasurement->instrument_id() == meas->instrument_id());
              couldBeBackground &= (m_dataMeasurement->num_gamma_channels() == meas->num_gamma_channels());
            }//if( !m_dataMeasurement || !meas )
            
            
            if( !couldBeBackground )
            {
              finishLoadUserFilesystemOpenedFile( meas, header, type );
              
              
              cout << "Will load file '" << filepath
                   << "' requested to be loaded at "
                   << fileinfo.m_utcRequestTime.toString(DATE_TIME_FORMAT_STR)
                   << " (currently "
                   << WDateTime::currentDateTime().toString(DATE_TIME_FORMAT_STR)
                   << ")" << endl;
            }else
            {
              promptUserHowToOpenFile( meas, header );
            }//
          }catch( std::exception &e )
          {
            cerr << "InterSpec::loadFileFromDbFilesystemLink(...) caught: "
                 << e.what() << endl;
            passMessage( "There was an error loading " + displayName,
                         "", WarningWidget::WarningMsgHigh );
          }//try / catch
          
//          m_fileManager->loadFromFileSystem( fileinfo.m_foregroundFilePath.toUTF8(), kForeground, kAutoParser );
        }//for( loop over files to potentially open )
      }//if( m_fileManager )
      
    }else
    {
      cerr << "Failed to find entry in FileIdToLocation with id=" << id
      << endl;
      return false;
    }//if( file_entry ) / else
  }catch( std::exception &e )
  {
    cerr << "\n\tFailed to load file indicated by 'specfile' argument."
    << "  Caught: " << e.what() << endl << endl;
    return false;
  }//try / catch
  
  return true;
}//bool loadFileFromDbFilesystemLink( int id )
#endif


void InterSpec::detectorsToDisplayChanged()
{
  displayBackgroundData();
  displaySecondForegroundData();
  displayForegroundData( true );
  displayTimeSeriesData( true ); //wcjohns change 20160602 to true, to force an update of highlighted regions, should consider removing the highlight option
}//void detectorsToDisplayChanged()


void InterSpec::updateGuiForPrimarySpecChange( std::set<int> display_sample_nums )
{
  m_displayedSamples = display_sample_nums;

  if( m_detectorToShowMenu )
  {
    const vector<WMenuItem *> items = m_detectorToShowMenu->items();
    for( WMenuItem *item : items )
    {
      if( !item->hasStyleClass("PhoneMenuBack") )
      {
//        m_detectorToShowMenu->removeItem( item );  //This seems unecassary, leave commented as test
        delete item;
      }
    }
    
#if( BUILD_AS_ELECTRON_APP && USE_ELECTRON_NATIVE_MENU )
#if( !defined(WIN32) )
    //20190125: hmm, looks like detectors menu is behaving okay - I guess I fixed it somewhere else?
    //#warning "Need to do something to get rid of previous detectors from Electrons menu"
#endif
//  https://github.com/electron/electron/issues/527
    m_detectorToShowMenu->clearElectronMenu();
#endif
  }//if( m_detectorToShowMenu )
  
  m_timeSeries->clearTimeHighlightRegions( kForeground );
  m_timeSeries->clearTimeHighlightRegions( kBackground );
  m_timeSeries->clearTimeHighlightRegions( kSecondForeground );
  m_timeSeries->clearOccupancyRegions();
  
  if( m_displayedSamples.empty() )
    m_displayedSamples = validForegroundSamples();

  const vector<int> det_nums = m_dataMeasurement
                               ? m_dataMeasurement->detector_numbers()
                               : vector<int>();
  const vector<string> &det_names = m_dataMeasurement
                                    ? m_dataMeasurement->detector_names()
                                    : vector<string>();
  const vector<string> &neut_det_names = m_dataMeasurement
                                 ? m_dataMeasurement->neutron_detector_names()
                                 : vector<string>();

  for( size_t index = 0; index < det_nums.size(); ++index )
  {
    const int detnum = det_nums[index];
    
    string name;
    try
    {
      name = det_names.at(index);
    }catch(...)
    {
      cerr << SRC_LOCATION << "\n\tSerious logic error = please fix" << endl;
      continue;
    }

    char title[512];
    if( name.size() )
      snprintf( title, sizeof(title), "Det. %i (%s)", detnum, name.c_str() );
    else
      snprintf( title, sizeof(title), "Det. %i", detnum );
    
    vector<string>::const_iterator neut_name_pos;
    neut_name_pos = std::find( neut_det_names.begin(), neut_det_names.end(), name );
    const int isneut = (neut_name_pos != neut_det_names.end());
    //if( isneut )
      //snprintf( title, sizeof(title), "Det. %i (%s)", i, name.c_str() );

    if( m_detectorToShowMenu )
    {
#if( WT_VERSION>=0x3030300 )
#if( USE_OSX_NATIVE_MENU  || (BUILD_AS_ELECTRON_APP && USE_ELECTRON_NATIVE_MENU) )
      WCheckBox *cb = new WCheckBox( title );
      cb->setChecked( true );
      PopupDivMenuItem *item = m_detectorToShowMenu->addWidget( cb, false );
#else
      PopupDivMenuItem *item = m_detectorToShowMenu->addMenuItem(title,"",false);
      item->setCheckable( true );
      item->setChecked( true );
      Wt::WCheckBox *cb = item->checkBox();
      if( !cb )
        throw runtime_error( "Serious error creating checkbox in menu item" );
#endif
      
#else
      WCheckBox *cb = new WCheckBox( title );
      cb->setChecked(true);
      
      //NOTE: this is necessary to prevent problems in menu state
      //cb->checked().connect( boost::bind(&WCheckBox::setChecked, cb, true) );
      //cb->unChecked().connect( boost::bind(&WCheckBox::setChecked, cb, false) );
      
      PopupDivMenuItem *item = m_detectorToShowMenu->addWidget( cb, false );
#endif
      
      if( isneut )
        item->addStyleClass( "NeutDetCbItem" );
      
      cb->checked().connect( boost::bind( &InterSpec::detectorsToDisplayChanged, this ) );
      cb->unChecked().connect( boost::bind( &InterSpec::detectorsToDisplayChanged, this ) );
      //item->triggered().connect( boost::bind( &InterSpec::detectorsToDisplayChanged, this, item ) );
      
      if( det_nums.size() == 1 )
        item->disable();
    }//if( m_detectorToShowMenu )
  }//for( gamma detector )
  
  if( m_detectorToShowMenu && m_detectorToShowMenu->parentItem() )
    m_detectorToShowMenu->parentItem()->setDisabled( det_nums.empty() );
  
  const bool keep_current_energy_range = false;
  displayForegroundData( keep_current_energy_range );

  displayTimeSeriesData( true );
}//bool updateGuiForPrimarySpecChange( const std::string &filename )


void InterSpec::setOverlayCanvasVisible( bool visible )
{
#if( !USE_SPECTRUM_CHART_D3 )
  m_spectrum->setOverlayCanvasVisible( visible );
#endif
  m_timeSeries->setOverlayCanvasVisible( visible );
}//void setOverlayCanvasVisible( bool visible )


#if( !USE_SPECTRUM_CHART_D3 )
Wt::JSlot *InterSpec::alignSpectrumOverlayCanvas()
{
  return m_spectrum->alignOverlayCanvas();
}
#endif

Wt::JSlot *InterSpec::alignTimeSeriesOverlayCanvas()
{
 return m_timeSeries->alignOverlayCanvas();
}

size_t InterSpec::addHighlightedEnergyRange( const float lowerEnergy,
                                            const float upperEnergy,
                                            const WColor &color )
{
  return m_spectrum->addDecorativeHighlightRegion( lowerEnergy, upperEnergy, color );
}//void setHighlightedEnergyRange( double lowerEnergy, double upperEnergy )


bool InterSpec::removeHighlightedEnergyRange( const size_t regionid )
{
  return m_spectrum->removeDecorativeHighlightRegion( regionid );
}//bool removeHighlightedEnergyRange( const size_t regionid );

#if( !USE_SPECTRUM_CHART_D3 )
void InterSpec::setSpectrumScrollingParent( WContainerWidget *parent )
{
  m_spectrum->setScrollingParent( parent );
}//void setSpectrumScrollingParent( Wt::WContainerWidget *parent )

void InterSpec::setScrollY( int scrollY )
{
  m_spectrum->setScrollY( scrollY );
}
#endif  //#if( !USE_SPECTRUM_CHART_D3 )

void InterSpec::setTimeSeriesScrollingParent( WContainerWidget *parent )
{
  m_timeSeries->setScrollingParent( parent );
}//void setTimeSeriesScrollingParent( Wt::WContainerWidget *parent );


void InterSpec::setDisplayedEnergyRange( float lowerEnergy, float upperEnergy )
{
  if( upperEnergy < lowerEnergy )
    std::swap( upperEnergy, lowerEnergy );
  
  m_spectrum->setXAxisRange( lowerEnergy, upperEnergy );
}//void setDisplayedEnergyRange()

void InterSpec::displayedSpectrumRange( double &xmin, double &xmax, double &ymin, double &ymax ) const
{
  m_spectrum->visibleRange( xmin, xmax, ymin, ymax );
}

void InterSpec::handleShiftAltDrag( double lowEnergy, double upperEnergy )
{
  if( !m_gammaCountDialog && upperEnergy<=lowEnergy )
    return;

  showGammaCountDialog();
  m_gammaCountDialog->setEnergyRange( lowEnergy, upperEnergy );
}//void InterSpec::handleShiftAltDrag( double lowEnergy, double upperEnergy )


#if( USE_HIGH_BANDWIDTH_INTERACTIONS && !USE_SPECTRUM_CHART_D3 )
void InterSpec::enableSmoothChartOperations()
{
  if( m_spectrum->overlayCanvas() )
    wApp->doJavaScript( "$('#c" + m_spectrum->overlayCanvas()->id()
                        + "').data('HighBandwidth',true);" );
}//void enableSmoothChartOperations()


void InterSpec::disableSmoothChartOperations()
{
  if( m_spectrum->overlayCanvas() )
    wApp->doJavaScript( "$('#c" + m_spectrum->overlayCanvas()->id()
                       + "').data('HighBandwidth',null);" );
}//void disableSmoothChartOperations()
#endif  //#if( USE_HIGH_BANDWIDTH_INTERACTIONS )



void InterSpec::overlayCanvasJsExceptionCallback( const std::string &message )
{
  if( UtilityFunctions::starts_with( message, "[initCanvasForDragging exception]" ) )
  {
#if( !USE_SPECTRUM_CHART_D3 )
    m_spectrum->disableOverlayCanvas();
#endif
    m_timeSeries->disableOverlayCanvas();
  }//if( starts_with( message, "[initCanvasForDragging exception]" ) )

  const string msg = "There was a problem with the clientside javascript.<br>"
                     "Some or all features may not function corectly.<br>"
                     "&nbsp;&nbsp;&nbsp;&nbsp;Message: '" + message + "'";
  passMessage( msg, "", WarningWidget::WarningMsgHigh );  
}//void overlayCanvasJsExceptionCallback( const std::string &message )



void InterSpec::searchForSinglePeak( const double x )
{
  if( !m_peakModel )
    throw runtime_error( "InterSpec::searchForSinglePeak(...): "
                        "shoudnt be called if peak model isnt set.");
  
  std::shared_ptr<Measurement> data = m_spectrum->data();
  
  if( !m_dataMeasurement || !data )
    return;
  
  const double xmin = m_spectrum->xAxisMinimum();
  const double xmax = m_spectrum->xAxisMaximum();
  
#if( USE_SPECTRUM_CHART_D3 )
  const double specWidthPx = m_spectrum->chartWidthInPixels();
  const double pixPerKeV = (xmax > xmin && xmax > 0.0 && specWidthPx > 10.0) ? std::max(0.001,(specWidthPx/(xmax - xmin))): 0.001;
#else
  const double xpixmin = m_spectrum->mapEnergyToXPixel( xmin );
  const double xpixmax = m_spectrum->mapEnergyToXPixel( xmax );
  const double pixPerKeV = std::max( 0.001, (xpixmax-xpixmin)/(xmax-xmin) );
#endif
  
  
  std::shared_ptr<const DetectorPeakResponse> det = m_dataMeasurement->detector();
  vector< PeakModel::PeakShrdPtr > origPeaks;
  if( !!m_peakModel->peaks() )
  {
    for( const PeakModel::PeakShrdPtr &p : *m_peakModel->peaks() )
    {
      origPeaks.push_back( p );
      
      //Avoid fitting a peak in the same area a data defined peak is.
      if( !p->gausPeak() && (x >= p->lowerX()) && (x <= p->upperX()) )
        return;
    }
  }//if( m_peakModel->peaks() )
  
  pair< PeakShrdVec, PeakShrdVec > foundPeaks;
  foundPeaks = searchForPeakFromUser( x, pixPerKeV, data, origPeaks );
  
  cerr << "Found " << foundPeaks.first.size() << " peaks to add, and "
       << foundPeaks.second.size() << " peaks to remove" << endl;
  
  if( foundPeaks.first.empty()
      || foundPeaks.second.size() >= foundPeaks.first.size() )
  {
    char msg[256];
    snprintf( msg, sizeof(msg), "Couldn't find peak a peak near %.1f keV", x );
    passMessage( msg, "", 0 );
    return;
  }//if( foundPeaks.first.empty() )
  
  
  for( const PeakModel::PeakShrdPtr &p : foundPeaks.second )
    m_peakModel->removePeak( p );

  
  //We want to add all of the previously found peaks back into the model, before
  //  adding the new peak.
  PeakShrdVec peakstoadd( foundPeaks.first.begin(), foundPeaks.first.end() );
  PeakShrdVec existingpeaks( foundPeaks.second.begin(), foundPeaks.second.end() );
  
  //First add all the new peaks that have a nuclide/reaction/xray associated
  //  with them, since we know they are existing peaks
  for( const PeakModel::PeakShrdPtr &p : foundPeaks.first )
  {
    if( p->parentNuclide() || p->reaction() || p->xrayElement() )
    {
      //find nearest previously existing peak, and add new peak, while removing
      //  old one from existingpeaks
      int nearest = -1;
      double smallesdist = DBL_MAX;
      for( size_t i = 0; i < existingpeaks.size(); ++i )
      {
        const PeakModel::PeakShrdPtr &prev = existingpeaks[i];
        if( prev->parentNuclide() != p->parentNuclide() )
          continue;
        if( prev->reaction() != p->reaction() )
          continue;
        if( prev->xrayElement() != p->xrayElement() )
          continue;
        
        const double thisdif = fabs(p->mean() - prev->mean());
        if( thisdif < smallesdist )
        {
          nearest = static_cast<int>( i );
          smallesdist = thisdif;
        }
      }
      
      if( nearest >= 0 )
      {
        addPeak( *p, false );
        existingpeaks.erase( existingpeaks.begin() + nearest );
        peakstoadd.erase( std::find(peakstoadd.begin(), peakstoadd.end(), p) );
      }//if( nearest >= 0 )
    }//if( p->parentNuclide() || p->reaction() || p->xrayElement() )
  }//for( const PeakModel::PeakShrdPtr &p : peakstoadd )
  
  
  //Now go through and add the new versions of the previously existing peaks,
  //  using energy to match the previous to current peak.
  for( const PeakModel::PeakShrdPtr &p : existingpeaks )
  {
    size_t nearest = 0;
    double smallesdist = DBL_MAX;
    for( size_t i = 0; i < peakstoadd.size(); ++i )
    {
      const double thisdif = fabs(p->mean() - peakstoadd[i]->mean());
      if( thisdif < smallesdist )
      {
        nearest = i;
        smallesdist = thisdif;
      }
    }
    
    std::shared_ptr<const PeakDef> peakToAdd = peakstoadd[nearest];
    peakstoadd.erase( peakstoadd.begin() + nearest );
    addPeak( *peakToAdd, false );
  }//for( const PeakModel::PeakShrdPtr &p : existingpeaks )
  
  //Just in case we messed up the associations between the existing peak an
  //  their respective new version, we'll add all peaks that have a
  //  nuclide/reaction/xray associated with them, since they must be previously
  //  existing
  for( const PeakModel::PeakShrdPtr &p : peakstoadd )
    if( p->parentNuclide() || p->reaction() || p->xrayElement() )
      addPeak( *p, false );
  
  //Finally, in principle we will add the new peak here
  for( const PeakModel::PeakShrdPtr &p : peakstoadd )
  {
    if( !p->parentNuclide() && !p->reaction() && !p->xrayElement() )
      addPeak( *p, true );
  }
  
  
#if( USE_SPECTRUM_CHART_D3 )
  m_spectrum->updateForegroundPeaksToClient();
  //m_spectrum->updateData();
#endif
}//void searchForSinglePeak( const double x )


void InterSpec::automatedPeakSearchStarted()
{
  if( m_peakInfoDisplay )
    m_peakInfoDisplay->enablePeakSearchButton( false );
}//void automatedPeakSearchStarted()


void InterSpec::automatedPeakSearchCompleted()
{
  if( m_peakInfoDisplay )
    m_peakInfoDisplay->enablePeakSearchButton( true );

#if( USE_SPECTRUM_CHART_D3 )
  m_spectrum->updateData();
#endif
}//void automatedPeakSearchCompleted()


bool InterSpec::colorPeaksBasedOnReferenceLines() const
{
  return m_colorPeaksBasedOnReferenceLines;
}



void InterSpec::searchForHintPeaks( const std::shared_ptr<SpecMeas> &data,
                                         const std::set<int> &samples )
{
  cerr << "Starting searchForHintPeaks()" << endl;
  
  std::shared_ptr<const deque< PeakModel::PeakShrdPtr > > origPeaks
                                                        = m_peakModel->peaks();
  if( !!origPeaks )
    origPeaks = std::make_shared<deque<PeakModel::PeakShrdPtr> >( *origPeaks );
  
  std::shared_ptr< vector<std::shared_ptr<const PeakDef> > > searchresults
            = std::make_shared< vector<std::shared_ptr<const PeakDef> > >();
  
  std::weak_ptr<const Measurement> weakdata = m_spectrum->data();
  std::weak_ptr<SpecMeas> spectrum = data;
  
  boost::function<void(void)> callback = wApp->bind(
                boost::bind(&InterSpec::setHintPeaks,
                this, spectrum, samples, origPeaks, searchresults) );
  
  boost::function<void(void)> worker = boost::bind( &PeakSearchGuiUtils::search_for_peaks_worker,
                                                   weakdata, origPeaks,
                                                   vector<ReferenceLineInfo>(), false,
                                                   searchresults,
                                                   callback, wApp->sessionId(), true );

  if( m_findingHintPeaks )
  {
    m_hintQueue.push_back( worker );
  }else
  {
    Wt::WServer *server = Wt::WServer::instance();
    if( server )  //this should always be true
    {
      m_findingHintPeaks = true;
      server->ioService().post( worker );
    }//if( server )
  }
}//void searchForHintPeaks(...)


void InterSpec::setHintPeaks( std::weak_ptr<SpecMeas> weak_spectrum,
                  std::set<int> samplenums,
                  std::shared_ptr<const std::deque< std::shared_ptr<const PeakDef> > > existing,
                  std::shared_ptr<std::vector<std::shared_ptr<const PeakDef> > > resultpeaks )
{
  cerr << "InterSpec::setHintPeaks(...) with "
       << (!!resultpeaks ? resultpeaks->size() : size_t(0)) << " peaks." << endl;
  
#if( PERFORM_DEVELOPER_CHECKS )
  if( !wApp )
    log_developer_error( BOOST_CURRENT_FUNCTION, "setHintPeaks() being called from not within the event loop!" );
#endif

  m_findingHintPeaks = false;
  
  if( m_hintQueue.size() )
  {
    Wt::WServer *server = Wt::WServer::instance();
    if( server )  //this should always be true
    {
      m_findingHintPeaks = true;
      cerr << "InterSpec::setHintPeaks(...): posting queued job" << endl;
      boost::function<void()> worker = m_hintQueue.back();
      m_hintQueue.pop_back();
      server->ioService().post( worker );
    }//if( server )
  }//if( m_hintQueue.size() )
  
  typedef std::shared_ptr<const PeakDef> PeakPtr;
  typedef deque< PeakPtr > PeakDeque;
  std::shared_ptr<SpecMeas> spectrum = weak_spectrum.lock();
  
  if( !spectrum || !resultpeaks )
  {
    cerr << "InterSpec::setHintPeaks(): invalid SpecMeas" << endl;
    return;
  }//if( !spectrum )
  
//  if( spectrum != m_dataMeasurement && spectrum != m_backgroundMeasurement
//      && spectrum != m_secondDataMeasurement )
//  {
//    cerr << "InterSpec::setHintPeaks(): SpecMeas not current spectrum"
//         << endl;
//    return;
//  }
  
  //we could check to see if the spectrum and sample numbers are still the
  //  current one.  If not the user probably doesnt care about this spectrum,
  //  so why bother storing ther results?
  
  std::shared_ptr< PeakDeque > newpeaks
    = std::make_shared<PeakDeque>( resultpeaks->begin(), resultpeaks->end() );
  
  //See if the user has added any peaks since we did the automated search
  std::shared_ptr<PeakDeque> current_user_peaks = spectrum->peaks( samplenums );
  
  vector< PeakPtr > addedpeaks;
  if( !!current_user_peaks && !existing )
  {
    addedpeaks.insert( addedpeaks.end(), current_user_peaks->begin(), current_user_peaks->end() );
  }else if( !!current_user_peaks && !!existing )
  {
    for( const PeakPtr &p : *current_user_peaks )
      if( std::find(existing->begin(),existing->end(),p) != existing->end() )
        addedpeaks.push_back( p );
  }
  
  if( addedpeaks.size() )
  {
    for( PeakPtr p : addedpeaks )
    {
      const int pos = add_hint_peak_pos( p, *newpeaks );
      if( pos >= 0 )
        newpeaks->insert( newpeaks->begin() + pos, p );
    }
  }//if( addedpeaks.size() )
  
  spectrum->setAutomatedSearchPeaks( samplenums, newpeaks );
//  existing
}//void setHintPeaks(...)



void InterSpec::findPeakFromControlDrag( double x0, double x1, int nPeaks )
{
  if( !m_dataMeasurement || nPeaks < 1 )
    return;
  
  int bestchi2 = -1;
  double chi2[FromInputPeaks];
  vector<std::shared_ptr<PeakDef> > answer[FromInputPeaks];
  
  std::shared_ptr<const Measurement> dataH = m_spectrum->data();
  if( !dataH )
    return;
  
  std::vector<std::thread> thread_grp;
  
  for( MultiPeakInitialGuesMethod method = MultiPeakInitialGuesMethod(0);
      method < FromInputPeaks;
      method = MultiPeakInitialGuesMethod(method+1) )
  {
    boost::function<void(void)> fctn
              = boost::bind( &findPeaksInUserRange, x0, x1, nPeaks, method,
                              dataH, m_dataMeasurement->detector(),
                              boost::ref(answer[method]),
                              boost::ref(chi2[method]) );
    thread_grp.emplace_back( fctn );
    //threads.create_thread( fctn );
  }//for( loop over methods )
  
  const int lowbin = dataH->FindFixBin(x0);
  const int highbin = dataH->FindFixBin(x1);
  const int nbin = highbin - lowbin;
  
  for( auto &thread : thread_grp )
    if( thread.joinable() )
      thread.join();
  
//  bestchi2 = *std::min_element( chi2, chi2+FromInputPeaks );
  for( MultiPeakInitialGuesMethod method = MultiPeakInitialGuesMethod(0);
      method < FromInputPeaks;
      method = MultiPeakInitialGuesMethod(method+1) )
  {
    cerr << "Method " << method << " yeilded chi2=" << chi2[method]
         << " (" << (chi2[method]/nbin) << " chi2/bin)" << endl;
    
    if( bestchi2 < 0 || chi2[method] < chi2[bestchi2] )
      bestchi2 = method;
  }//for(...)
  
  //Remove peaks from x0 to x1
  for( int peakn = 0; peakn < int(m_peakModel->npeaks()); ++peakn )
  {
    const double mean = m_peakModel->peak(peakn).mean();
    if( mean >= x0 && mean <= x1 )
    {
      m_peakModel->removePeak( peakn );
      peakn = -1;
    }//if( mean >= x0 && mean <= x1 )
  }//for( loop over peaks )
  
#if( USE_SPECTRUM_CHART_D3 )
  m_spectrum->updateData();
#endif

  
  const double dof = (nbin + 3*nPeaks + answer[bestchi2][0]->type());
  const double chi2Dof = chi2[bestchi2] / dof;
  
  for( size_t i = 0; i < answer[bestchi2].size(); ++i )
  {
    answer[bestchi2][i]->set_coefficient( chi2Dof, PeakDef::Chi2DOF );
    addPeak( *(answer[bestchi2][i]), true );
  }
}//void findPeakFromControlDrag( )




void InterSpec::userAskedToFitPeaksInRange( double x0, double x1,
                                int pageLeft, int pagetop )
{
  DeleteOnClosePopupMenu *menu = new DeleteOnClosePopupMenu( m_mobileMenuButton, PopupDivMenu::TransientMenu );
  menu->aboutToHide().connect( menu, &DeleteOnClosePopupMenu::markForDelete );
  menu->setPositionScheme( Wt::Absolute );
  
  PopupDivMenuItem *item = 0;
  if( isMobile() )
    item = menu->addPhoneBackItem( NULL );
//  item->triggered().connect( boost::bind( &deleteMenu, menu ) );
  
  item = menu->addMenuItem( "Single Peak" );
//  item->triggered().connect( boost::bind( &InterSpec::findPeakFromUserRange, this, x0, x1 ) );
  item->triggered().connect( boost::bind( &InterSpec::findPeakFromControlDrag, this, x0, x1, 1 ) );
  
  item = menu->addMenuItem(  "Two Peaks" );
  item->triggered().connect( boost::bind( &InterSpec::findPeakFromControlDrag, this, x0, x1, 2 ) );
  
  item = menu->addMenuItem(  "Three Peaks" );
  item->triggered().connect( boost::bind( &InterSpec::findPeakFromControlDrag, this, x0, x1, 3 ) );
  
  item = menu->addMenuItem(  "Four Peaks" );
  item->triggered().connect( boost::bind( &InterSpec::findPeakFromControlDrag, this, x0, x1, 4 ) );
  
  item = menu->addMenuItem(  "Five Peaks" );
  item->triggered().connect( boost::bind( &InterSpec::findPeakFromControlDrag, this, x0, x1, 5 ) );

//  WPointF roi_middle = m_spectrum->energyCountsToPixels( x0, 100 );
//  menu->popup( WPoint(roi_middle.x() + pageLeft,
//                      0.5*m_spectrum->layoutHeight() + pagetop) );

  if( isMobile() )
  {
    menu->addStyleClass( " Wt-popupmenu Wt-outset" );
    menu->showFromMouseOver();
  }else
  {
    menu->addStyleClass( " Wt-popupmenu Wt-outset NumPeakSelect" );
    menu->popup( WPoint(pageLeft-30,pagetop-30) );
  }
}//void userAskedToFitPeaksInRange(...)

/*
 //Depreciated 20150204 by wcjohns in favor of calling 
 //  InterSpec::findPeakFromControlDrag()
void InterSpec::findPeakFromUserRange( double x0, double x1 )
{
  if( !m_peakModel )
    throw runtime_error( "SpectrumDisplayDiv::findPeakFromUserRange(...): "
                        "shoudnt be called if peak model isnt set.");
  
  std::shared_ptr<Measurement> data = m_spectrum->data();
  
  if( !data )
  {
    passMessage( "No spectrum data", "", WarningWidget::WarningMsgMedium );
    return;
  }
    
  if( x0 > x1 )
    swap( x0, x1 );
  
  //round to the edges of the bins the drag action was from
  const size_t lowerchannel = data->find_gamma_channel( x0 );
  const size_t upperchannel = data->find_gamma_channel( x1 );
  x0 = data->gamma_channel_lower( lowerchannel );
  x1 = data->gamma_channel_upper( upperchannel );
  
  PeakDef peak( 0.0, 0.0, 0.0 );
  peak.setMean( 0.5*(x0+x1) );
  peak.setSigma( (x1-x0)/8.0 );
  std::shared_ptr<PeakContinuum> continuum = peak.continuum();
  
  continuum->calc_linear_continuum_eqn( data, x0, x1, 1 );
  
  const double data_area = data->gamma_channels_sum( lowerchannel, upperchannel );
  const double continuum_area = peak.offset_integral( x0, x1 );
  peak.setAmplitude( data_area - continuum_area );
  
  std::vector<PeakDef> peakV;
  peakV.push_back( peak );
  
  const double stat_threshold = 0.5;
  const double hypothesis_threshold = 0.5;
  
  //XXX - we promised the user to use a given continuum, however the below only
  //      uses this as a starting point
  peakV = fitPeaksInRange( x0+0.00001, x1-0.00001, 0,
                           stat_threshold, hypothesis_threshold,
                           peakV, data, m_peakModel->peakVec() );
  
  for( size_t i = 0; i < peakV.size(); ++i )
    addPeak( peakV[i], true );
    
  if( peakV.size()==0 )
    passMessage( "No peaks were found. You might try a slighlty changed ROI "
                 "region.", "", WarningWidget::WarningMsgLow );
}//void findPeakFromUserRange( const double x0, const double x1 )
*/

void InterSpec::excludePeaksFromRange( double x0, double x1 )
{
  if( !m_peakModel )
    throw runtime_error( "SpectrumDisplayDiv::excludePeaksFromRange(...): "
                        "shoudnt be called if peak model isnt set.");
  if( x0 > x1 )
    swap( x0, x1 );
  
  vector<PeakDef> all_peaks = m_peakModel->peakVec();
//  vector<PeakDef> peaks_in_range = peaksInRange( x0, x1, 4.0, all_peaks );
  vector<PeakDef> peaks_in_range = peaksTouchingRange( x0, x1, all_peaks );
  
  if( peaks_in_range.empty() )
    return;
  
  std::shared_ptr<const Measurement> data = m_spectrum->data();
  if( !data )
    return;
  
  for( const PeakDef &peak : peaks_in_range )
  {
    vector<PeakDef>::iterator iter = std::find( all_peaks.begin(),
                                                all_peaks.end(), peak );
    if( iter != all_peaks.end() )
      all_peaks.erase( iter );
  }//for( peak in range ...)
  
  
  vector<PeakDef> peaks_to_keep;
  double minEffectedPeak = DBL_MAX, maxEffectedPeak = -DBL_MAX;
  
  for( PeakDef peak : peaks_in_range )
  {
    if( peak.mean()>=x0 && peak.mean()<=x1 )
      continue;
    
    std::shared_ptr<PeakContinuum> continuum = peak.continuum();
    
    double lowx = 0.0, upperx = 0.0;
    findROIEnergyLimits( lowx, upperx, peak, data );
    
    minEffectedPeak = std::min( minEffectedPeak, peak.mean() );
    maxEffectedPeak = std::max( maxEffectedPeak, peak.mean() );

    if( x0>=upperx || x1<=lowx )
    {
      peaks_to_keep.push_back( peak );
      continue;
    }
    
    const bool x0InPeak = ((x0>=lowx) && (x0<=upperx));
    const bool x1InPeak = ((x1>=lowx) && (x1<=upperx));
    
    if( !x0InPeak && !x1InPeak )
    {
      peaks_to_keep.push_back( peak );
      continue;
    }else if( x0InPeak && x1InPeak )
    {
      if( x0>peak.mean() )
        upperx = x0;
      else
        lowx = x1;
    }else if( x0InPeak )
    {
      upperx = x0;
    }else //if( x1InPeak )
    {
      lowx = x1;
    }//if / else

    continuum->setRange( lowx, upperx );
    
    peaks_to_keep.push_back( peak );
  }//for( PeakDef peak : peaks_in_range )
  
  
  std::shared_ptr<const DetectorPeakResponse> detector
                                          = measurment(kForeground)->detector();
  map<std::shared_ptr<const PeakContinuum>,PeakShrdVec > peaksinroi;
  for( PeakDef peak : peaks_to_keep )
    peaksinroi[peak.continuum()].push_back( std::make_shared<PeakDef>(peak) );
  
  map<std::shared_ptr<const PeakContinuum>, PeakShrdVec >::const_iterator iter;
  for( iter = peaksinroi.begin(); iter != peaksinroi.end(); ++iter )
  {
    //Instead of doing the fitting here, we could just:
    //m_rightClickEnergy = iter->second[0].mean();
    //refitPeakFromRightClick();
    //This would make things more consistent between right clicking to fit, and
    //  erasing part of the ROI (which is what happened if we are here).
    
    PeakShrdVec newpeaks = refitPeaksThatShareROI( data, detector, iter->second, 0.25 );
    if( newpeaks.size() == iter->second.size() )
    {
      for( size_t j = 0; j < newpeaks.size(); ++j )
        all_peaks.push_back( PeakDef(*newpeaks[j]) );
      std::sort( all_peaks.begin(), all_peaks.end(), &PeakDef::lessThanByMean );
    }else
    {
      //if newpeaks.empty(), then the Chi2 of the fit probably did not improve,
      //  so we'll just use the existing peaks.
      //I dont actually know how I feel about this: on one hand I want to
      //  believe in the Chi2, but on the other, we should probably respond to
      //  the user chaning the ROI. refitPeakFromRightClick() will then attempt
      //  a second fitting methos
      if( newpeaks.size() )
        cerr << "Warning: failed to refit peaks for some reason; got "
             << newpeaks.size()  << " with an input of " << iter->second.size()
             << endl;
      for( size_t j = 0; j < iter->second.size(); ++j )
        all_peaks.push_back( PeakDef(*iter->second[j]) );
      
      std::sort( all_peaks.begin(), all_peaks.end(), &PeakDef::lessThanByMean );
      
      /*
      double stat_threshold = 0.5, hypothesis_threshold = 0.5;
      vector<PeakDef> newpeaks;
      newpeaks = fitPeaksInRange( minEffectedPeak,
                                 maxEffectedPeak,
                                 3.0, stat_threshold, hypothesis_threshold,
                                 all_peaks,
                                 m_spectrum->data(),
                                 vector<PeakDef>() );
      if( all_peaks.size() == newpeaks.size() )
        all_peaks = newpeaks;
       */
    }
  }//for( loop over peaksinroi elements )
  
  
  m_peakModel->setPeaks( all_peaks );
#if( USE_SPECTRUM_CHART_D3 )
  //m_spectrum->updateData();
  m_spectrum->updateForegroundPeaksToClient();
#endif
  
//  m_peakModel->setPeaks( newpeaks );
}//void excludePeaksFromRange( const double x0, const double x1 )


void InterSpec::guessIsotopesForPeaks( WApplication *app )
{
  InterSpec *viewer = this;
  if( !m_peakModel )
    throw runtime_error( "SpectrumDisplayDiv::refitPeakAmplitudes(...): "
                         "shoudnt be called if peak model isnt set.");
  
  std::shared_ptr<const Measurement> data;
  std::shared_ptr<const DetectorPeakResponse> detector;
  std::shared_ptr<const deque< PeakModel::PeakShrdPtr > > all_peaks;
  
  {//begin code-block to get input data
    std::unique_ptr<WApplication::UpdateLock> applock;
    if( app )
      applock.reset( new WApplication::UpdateLock(app) );
    
    if( app && m_peakModel->peaks() )
      all_peaks = std::make_shared<deque<PeakModel::PeakShrdPtr> >( *m_peakModel->peaks() );
    else
      all_peaks = m_peakModel->peaks();
    
    if( !all_peaks || all_peaks->empty() )
      return;
    
    data = m_spectrum->data();
  
    if( viewer && viewer->measurment(kForeground) )
    {
      detector = viewer->measurment(kForeground)->detector();
      
      if( detector )
      {
        DetectorPeakResponse *resp = new DetectorPeakResponse( *detector );
        detector.reset( resp );
      }//if( detector )
    }//if( viewer && viewer->measurment(kForeground) )
  
    if( detector && !detector->hasResolutionInfo() )
    {
      try
      {
        std::shared_ptr<DetectorPeakResponse> newdetector = std::make_shared<DetectorPeakResponse>( *detector );
        newdetector->fitResolution( all_peaks, data, DetectorPeakResponse::kGadrasResolutionFcn );
        detector = newdetector;
      }catch( std::exception & )
      {
        detector.reset();
      }
    }
    
    if( !detector || !detector->isValid() )
    {
      DetectorPeakResponse *detPtr = new DetectorPeakResponse();
      detector.reset( detPtr );
    
      const char *drf_dir = "data/GenericGadrasDetectors/HPGe 40%";
      if( data && (data->num_gamma_channels() <= 2049) )
        drf_dir = "data/GenericGadrasDetectors/NaI 1x1";

      detPtr->fromGadrasDirectory( "data/GenericGadrasDetectors/NaI 1x1" );
    }//if( !detector || !detector->isValid() )
  }//end code-block to get input data
  
  vector<IsotopeId::PeakToNuclideMatch> idd( all_peaks->size() );
  
  size_t peakn = 0, rownum = 0;
  vector<size_t> rownums;
  SpecUtilsAsync::ThreadPool threadpool;
//  vector< boost::function<void()> > workers;
  vector<PeakModel::PeakShrdPtr> inputpeaks;
  vector<PeakDef> modifiedPeaks;
  for( PeakModel::PeakShrdPtr peak : *all_peaks )
  {
    ++rownum;
    inputpeaks.push_back( peak );
    
    if( (peak->parentNuclide()
         && (peak->nuclearTransition()
             || (peak->sourceGammaType()==PeakDef::AnnihilationGamma) ))
        || peak->reaction() || peak->xrayElement() )
    {
      modifiedPeaks.push_back( *peak );
      continue;
    }
    
    threadpool.post( boost::bind( &IsotopeId::suggestNuclides,
                                  boost::ref(idd[peakn]), peak, all_peaks,
                                  data, detector ) );
//    workers.push_back( boost::bind( &suggestNuclides, boost::ref(idd[peakn]),
//                                   peak, all_peaks, data, detector ) ) );
    rownums.push_back( rownum-1 );
    ++peakn;
  }//for( PeakModel::PeakShrdPtr peak : *all_peaks )
  
  threadpool.join();
//  UtilityFunctions::do_asyncronous_work( workers, false );

    
  for( size_t resultnum = 0; resultnum < rownums.size(); ++resultnum )
  {
    const size_t row = rownums[resultnum];
    const PeakModel::PeakShrdPtr peak = m_peakModel->peakPtr( row ); 
    PeakDef newPeak = *inputpeaks[row];
      
    if( newPeak.parentNuclide() || newPeak.reaction() || newPeak.xrayElement() )
    {
      modifiedPeaks.push_back( newPeak );
      continue;
    }//if( !isotopeData.empty() )
      
    const IsotopeId::PeakToNuclideMatch match = idd[resultnum];
    vector<PeakDef::CandidateNuclide> candidates;
      
    for( size_t j = 0; j < match.nuclideWeightPairs.size(); ++j )
    {
      const IsotopeId::NuclideStatWeightPair &p = match.nuclideWeightPairs[j];
      
      PeakDef::SourceGammaType sourceGammaType = PeakDef::NormalGamma;
      size_t radparticleIndex;
      const SandiaDecay::Transition *transition = NULL;
      const double sigma = newPeak.gausPeak() ? newPeak.sigma() : 0.125*newPeak.roiWidth();
      PeakDef::findNearestPhotopeak( p.nuclide, match.energy,
                                       4.0*sigma, transition,
                                       radparticleIndex, sourceGammaType );
      if( j == 0 )
        newPeak.setNuclearTransition( p.nuclide, transition,
                                      int(radparticleIndex), sourceGammaType );
        
      if( transition || (sourceGammaType==PeakDef::AnnihilationGamma) )
      {
        PeakDef::CandidateNuclide candidate;
        candidate.nuclide          = p.nuclide;
        candidate.weight           = p.weight;
        candidate.transition       = transition;
        candidate.sourceGammaType  = sourceGammaType;
        candidate.radparticleIndex = static_cast<int>(radparticleIndex);
        candidates.push_back( candidate );
      }//if( transition )
    }//for( size_t j = 0; j < match.nuclideWeightPairs.size(); ++j )

    newPeak.setCandidateNuclides( candidates );
    modifiedPeaks.push_back( newPeak );
  }//for( size_t i = 0; i < idd.size(); ++i )

  {//begin codeblock to set modified peaks
    std::unique_ptr<WApplication::UpdateLock> applock;
    if( app )
      applock.reset( new WApplication::UpdateLock(app) );
    
    m_peakModel->setPeaks( modifiedPeaks );
#if( USE_SPECTRUM_CHART_D3 )
    m_spectrum->updateData();
#endif
    
    if( app )
      app->triggerUpdate();
  }//end codeblock to set modified peaks
}//void SpectrumDisplayDiv::guessIsotopesForPeaks()


vector<pair<float,int> > InterSpec::passthroughTimeToSampleNumber() const
{
  std::vector<std::pair<float,int> > answer;
  
  if( !m_dataMeasurement )
    return answer;
  
  const set<int> foregroundSamples = validForegroundSamples();
  const set<int> disp_det_nums = displayed_detector_numbers();
  
  
  float time = 0.0f;
  vector<pair<float,int> > foreground;
  for( const int sample : foregroundSamples )
  {
    const float thistime = sample_real_time_increment( m_dataMeasurement, sample, disp_det_nums );
    if( thistime <= 0.0 )
      continue;
    foreground.push_back( make_pair(time,sample) );
    time += thistime;
  }//for( const int sample : sample_nums )
  
  if( foreground.size() )
    foreground.push_back( make_pair(time,foreground.back().second + 1) );
  
  //
  float backtime = 0.0f;
  vector<pair<float,int> > background;
  const set<int> all_sample_nums = m_dataMeasurement->sample_numbers();
  for( const int s : all_sample_nums )
  {
    if( m_excludedSamples.count(s) )
      continue;
    
    const float thistime = sample_real_time_increment( m_dataMeasurement, s, disp_det_nums );
    if( thistime <= 0.0 )
      continue;
    
    bool isback = false;
    const vector< MeasurementConstShrdPtr > meas
                              = m_dataMeasurement->sample_measurements(s);
    for( const MeasurementConstShrdPtr &m : meas )
      isback |= (m->source_type() == Measurement::Background);
    
    if( isback )
    {
      backtime += thistime;
      background.push_back( make_pair(backtime,s) );
    }
  }//for( const int s : sample_nums )
  
  if( !background.empty() && foreground.empty() )
  {
    background.push_back( make_pair(backtime,background.back().second + 1) );
    return background;
  }
  if( background.empty() )
    return foreground;
  
  float backscale = 1.0f;
  if( backtime > 0.1f*time )
    backscale = ( std::ceil(0.1f*time) ) / backtime;
  
  answer.reserve( foreground.size() + background.size() + 1 );
  
  float lastt = -backscale*background.back().first;
  for( size_t i = 0; i < background.size(); ++i )
  {
    answer.push_back( make_pair(lastt, background[i].second));
    lastt = -backscale*background.back().first + backscale*background[i].first;
  }
  
  answer.insert( answer.end(), foreground.begin(), foreground.end() );
  
  return answer;
}//vector<std::pair<float,int> > passthroughTimeToSampleNumber() const


void InterSpec::setChartSpacing()
{
  const int vertSpacing = (m_timeSeries->isHidden() ? 0 : (isMobile() ? 10 : 5));

  //Changing the vertical space doesnt seem to reliably trigger in Wt 3.3.4
  //  so we have to re-create the layout here...
  if( m_chartsLayout && (m_chartsLayout->verticalSpacing() != vertSpacing) )
  {
    m_chartsLayout->removeWidget( m_spectrum );
    m_chartsLayout->removeWidget( m_timeSeries );
    
    for( int i = 0; i < m_layout->count(); ++i )
    {
      WLayoutItem *t = m_layout->itemAt(i);
      if( t->layout() == m_chartsLayout )
      {
        m_layout->removeItem( t );
        break;
      }
    }//for( int i = 0; i < m_layout->count(); ++i )
    
    //wait, did m_chartsLayout get deleted?
    
    m_chartsLayout = new WGridLayout();
    m_chartsLayout->setContentsMargins( 0, 0, 0, 0 );
    m_chartsLayout->setVerticalSpacing( vertSpacing );
    m_chartsLayout->addWidget( m_spectrum, 0, 0 );
    m_chartsLayout->addWidget( m_timeSeries, 1, 0 );
    m_chartsLayout->setRowStretch( 0, 3 );
    m_chartsLayout->setRowStretch( 1, 2 );
    m_chartsLayout->setRowResizable( 0 );
    
    m_layout->addLayout( m_chartsLayout, m_menuDiv ? 1 : 0, 0 );
  }else if( !m_chartsLayout && (m_layout->verticalSpacing() != vertSpacing) )
  {
    //The tool tabs are not showing
    if( m_menuDiv )
      m_layout->removeWidget( m_menuDiv );
    m_layout->removeWidget( m_spectrum );
    m_layout->removeWidget( m_timeSeries );
    
    delete m_layout;
    m_layout = new WGridLayout();
    m_layout->setContentsMargins( 0, 0, 0, 0 );
    m_layout->setHorizontalSpacing( 0 );
    
    if( m_menuDiv )
      m_layout->addWidget( m_menuDiv, 0, 0 );
    m_layout->addWidget( m_spectrum, m_layout->rowCount(), 0 );
    m_layout->setRowResizable( m_layout->rowCount() - 1 );
    m_layout->setRowStretch( m_layout->rowCount() - 1, 5 );
    m_layout->addWidget( m_timeSeries, m_layout->rowCount(), 0 );
    m_layout->setRowStretch( m_layout->rowCount() - 1, 3 );
    m_layout->setVerticalSpacing( vertSpacing );
    
    setLayout( m_layout );
  }
  
  if( m_menuDiv && !m_menuDiv->isHidden() )
  {
    if( m_chartsLayout || !m_timeSeries->isHidden() )
      m_spectrum->setMargin( -vertSpacing, Wt::Top );
    else
      m_spectrum->setMargin( 0, Wt::Top );
  }else
  {
    m_spectrum->setMargin( 0, Wt::Top );
  }
  
}//void setChartSpacing()


void InterSpec::displayTimeSeriesData( bool updateHighlightRegionsDisplay )
{
  std::shared_ptr<Measurement> gammaH, neutronH;

  bool useAllDetector = true;
  const vector<bool> det_to_use = detectors_to_display();

  const size_t num_det = det_to_use.size();
  if( !!m_dataMeasurement && num_det != m_dataMeasurement->detector_names().size() )
    throw runtime_error( "Serious logic error in InterSpec::displayTimeSeriesData" );

  const set<int> disp_det_nums = displayed_detector_numbers();

  for( const bool use : det_to_use )
    useAllDetector = (useAllDetector && use);
  
  const vector<pair<float,int> > binning = passthroughTimeToSampleNumber();  //last entry, if non-empty, is the right-most edge of the last bin, not its own bin
  const size_t nbins = (binning.size() < 3 ? 0 : (binning.size()-1)); //
  
  if( m_displayedSamples.empty() || !m_dataMeasurement )
    m_displayedSamples = validForegroundSamples();

  if( !!m_dataMeasurement && m_dataMeasurement->passthrough() && nbins >= 3 )
  {
    if( m_timeSeries->isHidden() )
    {
      m_timeSeries->setHidden( false );
      setChartSpacing();
    }//if( m_timeSeries->isHidden() )
    
    const vector<string> &det_names = m_dataMeasurement->detector_names();
    if( det_names.size() != det_to_use.size() )
      throw runtime_error( "Inconsistent number of detectors." );

    auto channel_energies = std::make_shared< vector<float> >( binning.size() );
    vector<float> &bin_edges = *channel_energies;
    
    for( size_t i = 0; i < binning.size(); ++i )
      bin_edges[i] = binning[i].first;
    
    gammaH      = std::make_shared<Measurement>();
    neutronH    = std::make_shared<Measurement>();
    
    gammaH->set_title( "Foreground Gamma" );
    neutronH->set_title( "Neutrons" );
    std::shared_ptr< vector<float> > gamma_counts, background_counts, nuetron_counts;
    gamma_counts      = std::make_shared< vector<float> >(nbins, 0.0f);
    nuetron_counts    = std::make_shared< vector<float> >(nbins, 0.0f);

    for( size_t timeslice = 0; timeslice < nbins; ++timeslice )
    {
      const auto time_to_sample = binning[timeslice];
      const int sample_number = time_to_sample.second;
      
      const float live_time = sample_real_time_increment( m_dataMeasurement, sample_number, disp_det_nums );
      float num_gamma = 0.0f, num_nuteron = 0.0f;
      
      if( useAllDetector )
      {
        const auto meas = m_dataMeasurement->sample_measurements( sample_number );
        
        for( size_t i = 0; i < meas.size(); ++i )
        {
          if( !meas[i] )
            continue;  //shouldnt ever happen
          if( meas[i]->num_gamma_channels() > 2 )  //weed out GMTubes
            num_gamma += meas[i]->gamma_count_sum();
          num_nuteron += meas[i]->neutron_counts_sum();
        }//for( size_t i = 0; i < meas.size(); ++i )
      }else
      {
        for( size_t detind = 0; detind < det_names.size(); ++detind )
        {
          const string &detname = det_names[detind];
          if( det_to_use[detind] )
          {
            const auto m = m_dataMeasurement->measurement( sample_number, detname );
            if( m )
            {
              if( m->num_gamma_channels() > 2 )  //weed out GMTubes
                num_gamma += m->gamma_count_sum();
              num_nuteron += m->neutron_counts_sum();
            }//if( m )
          }//if( det_to_use[det] )
        }//for( loop over gamma detectors )
      }//if( use all gamma ) / else
      
      
      if( live_time > 0.0f )
      {
        num_gamma /= live_time;
        num_nuteron /= live_time;
      }
      
      (*gamma_counts)[timeslice] += num_gamma;
      (*nuetron_counts)[timeslice] += num_nuteron;
    }//for( const int sample_number : sample_nums )
    
    gammaH->set_channel_energies( channel_energies );
    neutronH->set_channel_energies( channel_energies );
    
    gammaH->set_gamma_counts( gamma_counts, 0.0f, 0.0f );
    neutronH->set_gamma_counts( nuetron_counts, 0.0f, 0.0f );
    
    
    //Let
    if( neutronH->gamma_channels_sum(0,nbins) > 0.000001 )
      m_timeSeries->setPlotAreaPadding( 80, 10, 80, 42 ); //(left,top,right,bottom)
    else
      m_timeSeries->setPlotAreaPadding( 80, 10, 10, 42 );
    if( m_timeSeries->overlayCanvas() )
      m_timeSeries->overlayCanvas()->setChartPadding();
  
    
    //Lets not take up memorry if there was no neutron
    if( !!neutronH && neutronH->gamma_channels_sum(0,nbins) < 0.000001 )
      neutronH.reset();
    
    const bool keep_current_xrange = false;
    m_timeSeries->setData( gammaH, -1.0, -1.0, -1.0, keep_current_xrange );
    m_timeSeries->setBackground( std::shared_ptr<Measurement>(), -1.0, -1.0, -1.0 );  //we could use this for like the GMTubes or whatever
    m_timeSeries->setSecondData( neutronH, -1.0, -1.0, -1.0, true );
    
    if( updateHighlightRegionsDisplay )
    {
      for( SpectrumType t = SpectrumType(0); t <= kBackground; t = SpectrumType(t+1) )
      {
        const vector< pair<double,double> > regions = timeRegionsToHighlight( t );
        m_timeSeries->setTimeHighLightRegions( regions, t );
      }//for( SpectrumType t = SpectrumType(0); t <= kBackground; t = SpectrumType(t+1) )
      
      const auto foreFromfile = timeRegionsFromFile( Measurement::OccupancyStatus::Occupied );
      m_timeSeries->setOccupancyRegions( foreFromfile );
    }//if( updateHighlightRegionsDisplay )
  }else
  {
    m_timeSeries->setData( gammaH, -1.0, -1.0, -1.0, false );
    m_timeSeries->setBackground( std::shared_ptr<Measurement>(), -1.0, -1.0, -1.0 );
    m_timeSeries->setSecondData( neutronH, -1.0, -1.0, -1.0, true );
    
    if( !m_timeSeries->isHidden() )
    {
      m_timeSeries->setHidden( true );
      setChartSpacing();
    }//if( !m_timeSeries->isHidden() )

    if( updateHighlightRegionsDisplay )
    {
      m_timeSeries->clearTimeHighlightRegions(kForeground);
      m_timeSeries->clearTimeHighlightRegions(kSecondForeground);
      m_timeSeries->clearTimeHighlightRegions(kBackground);
      m_timeSeries->clearOccupancyRegions();
    }
  }//if( passthrough ) / else
  
  return;
}//void displayTimeSeriesData()



ShrdConstFVecPtr InterSpec::getBinning( std::set<int> sample_numbers,
                                              const vector<bool> det_to_use,
                                              std::shared_ptr<const SpecMeas> measurement )
{
  if( !measurement )
    return std::shared_ptr< const std::vector<float> >();
  
  size_t index;
  try
  {
    index = measurement->suggested_gamma_binning_index( sample_numbers, det_to_use );
  }catch( std::exception &e )
  {
    cerr << "InterSpec::getBinning() caught: " << e.what() << endl;
    return std::shared_ptr< const std::vector<float> >();
  }
  
  MeasurementConstShrdPtr meas = measurement->measurements()[index];
  if( meas )
    return meas->channel_energies();
  
  return std::shared_ptr< const std::vector<float> >();
}//getBinning(...)



std::set<int> InterSpec::sampleRangeToSet( int start_sample,  int end_sample,
                                std::shared_ptr<const SpecMeas> meas,
                                const std::set<int> &excluded_samples )
{
  std::set<int> display_samples;
  if( meas )
  {
    set<int> sample_numbers = meas->sample_numbers();
    for( int s : excluded_samples )
      sample_numbers.erase( s );

    const int first_sample = (*sample_numbers.begin());
    const int last_sample = (*sample_numbers.rbegin());
  //  const int num_samples = static_cast<int>( meas->sample_numbers().size() );

    if( start_sample < first_sample ) start_sample = first_sample;
    if( end_sample < 0 )              end_sample = last_sample;
    if( end_sample > last_sample )    end_sample = last_sample;

    if( sample_numbers.size() == 1 )
      start_sample = end_sample = first_sample;

    for( int sample : sample_numbers )
      if( sample >= start_sample && sample <= end_sample )
        display_samples.insert( sample );
  }//if( meas )

  return display_samples;
}//sampleRangeToSet


void InterSpec::displayForegroundData( int start_sample,  int end_sample )
{
  m_displayedSamples = sampleRangeToSet( start_sample, end_sample, m_dataMeasurement, m_excludedSamples );
  const bool keep_current_energy_range = true;
  displayForegroundData( keep_current_energy_range );
}//void InterSpec::displayForegroundData( int start_sample,  int end_sample )


std::vector<bool> InterSpec::detectors_to_display() const
{
  const size_t ndet = m_dataMeasurement ? m_dataMeasurement->detector_numbers().size() : size_t(0);
  vector<bool> use_gamma( ndet, true );
  
  if( !m_detectorToShowMenu || !ndet )
    return use_gamma;
  
  const vector<WMenuItem *> items = m_detectorToShowMenu->items();
  
  size_t detnum = 0;
  for( size_t i = 0; i < items.size(); ++i )
  {
    if( items[i]->hasStyleClass("PhoneMenuBack") )
      continue;
    
#if( WT_VERSION>=0x3030300 )
    PopupDivMenuItem *item = dynamic_cast<PopupDivMenuItem *>( items[i] );
    WCheckBox *cb = (item ? item->checkBox() : (WCheckBox *)0);
#else
    WCheckBox *cb = 0;
    for( int j = 0; !cb && (j < items[i]->count()); ++j )
      cb = dynamic_cast<WCheckBox *>(items[i]->widget(j));
#endif
    
    if( cb && detnum < use_gamma.size() )
      use_gamma[detnum] = cb->isChecked();
    else
      cerr << "XXX - Failed to get checkbox in menu!" << endl;
    ++detnum;
  }// for( size_t i = 0; i < items.size(); ++i )
  
  if( detnum != ndet )
    throw runtime_error( "InterSpec::detectors_to_display(): "
                         "serious logic error" );

  return use_gamma;
}//std::vector<bool> detectors_to_display() const


vector<string> InterSpec::detector_names() const
{
  if( !m_dataMeasurement )
    return vector<string>();
  
  return m_dataMeasurement->detector_names();
}//vector<string> detector_names() const

set<int> InterSpec::displayed_detector_numbers() const
{
  set<int> answer;
  
  if( !m_dataMeasurement )
    return answer;
  
  const vector<int> &numbers = m_dataMeasurement->detector_numbers();
  const vector<bool> use = detectors_to_display();
  
  if( numbers.size() != use.size() )
    throw runtime_error( "InterSpec::displayed_detector_numbers(): "
                        "Serious logic error.");
  
  for( size_t i = 0; i < numbers.size(); ++i )
    if( use[i] )
      answer.insert( numbers[i] );
  
  return answer;
}

vector<string> InterSpec::displayed_detector_names() const
{
  vector<string> answer;
  
  if( !m_dataMeasurement )
    return answer;
  
  const vector<string> &names = m_dataMeasurement->detector_names();
  const vector<bool> use = detectors_to_display();
  
  if( names.size() != use.size() )
    throw runtime_error( "InterSpec::displayed_detector_names(): "
                         "Serious logic error.");

  for( size_t i = 0; i < names.size(); ++i )
    if( use[i] )
      answer.push_back( names[i] );
  
  return answer;
}//vector<string> displayed_detector_names() const


void InterSpec::refreshDisplayedCharts()
{
  if( !!m_dataMeasurement )
    displayForegroundData( true );
  if( !!m_secondDataMeasurement )
    displaySecondForegroundData();
  if( !!m_backgroundMeasurement )
    displayBackgroundData();
}//void refreshDisplayedCharts()


vector< pair<double,double> > InterSpec::timeRegionsToHighlight(
                                                const SpectrumType type ) const
{
  vector< pair<double,double> > timeranges;
  
  const std::set<int> &wantedSamples = displayedSamples( type );
  std::shared_ptr<const SpecMeas> meas = measurment( type );
  
  if( wantedSamples.empty() || !meas || meas!=m_dataMeasurement
     || m_timeSeries->isHidden()
     || (type==kForeground && wantedSamples==validForegroundSamples()) )
    return timeranges;

  const vector<pair<float,int> > binning = passthroughTimeToSampleNumber();
  
  bool inregion = false;
  double startt;
  for( size_t i = 0; (i+1) < binning.size(); ++i )
  {
    const bool want = wantedSamples.count( binning[i].second );
    if( want && !inregion )
    {
      inregion = true;
      startt = binning[i].first;
    }else if( !want && inregion )
    {
      timeranges.push_back( make_pair(startt, binning[i].first) );
      inregion = false;
    }
  }//for( size_t i = 0; i < binning.size(); ++i )
  
  if( inregion && binning.size() )
    timeranges.push_back( make_pair(startt, binning.back().first) );
  
  return timeranges;
}//timeRegionsToHighlight(...)


vector< pair<double,double> > InterSpec::timeRegionsFromFile( const Measurement::OccupancyStatus type ) const
{
  vector< pair<double,double> > timeranges;
  
  std::shared_ptr<const SpecMeas> meas = measurment( kForeground );
  
  if( !meas || m_timeSeries->isHidden() )
    return timeranges;
  
  std::set<int> wantedSamples;

  for( const auto &m : meas->measurements() )
    if( m && (m->occupied() == type) )
      wantedSamples.insert( m->sample_number() );
  
  if( wantedSamples.empty() || (wantedSamples == meas->sample_numbers()) )
    return timeranges;
  
  const vector<pair<float,int> > binning = passthroughTimeToSampleNumber();
  
  bool inregion = false;
  double startt;
  for( size_t i = 0; (i+1) < binning.size(); ++i )
  {
    const bool want = wantedSamples.count( binning[i].second );
    if( want && !inregion )
    {
      inregion = true;
      startt = binning[i].first;
    }else if( !want && inregion )
    {
      timeranges.push_back( make_pair(startt, binning[i].first) );
      inregion = false;
    }
  }//for( size_t i = 0; i < binning.size(); ++i )
  
  if( inregion && binning.size() )
    timeranges.push_back( make_pair(startt, binning.back().first) );
  
  return timeranges;
}//timeRegionsFromFile(...)


void InterSpec::displayForegroundData( const bool current_energy_range )
{
  if( !m_dataMeasurement )
  {
    m_backgroundSubItems[0]->disable();
    m_backgroundSubItems[0]->show();
    m_backgroundSubItems[1]->hide();
    m_spectrum->setData( std::shared_ptr<Measurement>(), -1.0, -1.0, -1.0, false );
    m_peakModel->setPeakFromSpecMeas( m_dataMeasurement, m_displayedSamples );
    return;
  }//if( !m_dataMeasurement )

  if( m_displayedSamples.empty() )
    m_displayedSamples = validForegroundSamples();

  m_peakModel->setPeakFromSpecMeas( m_dataMeasurement, m_displayedSamples );
  
  vector<bool> use_gamma = detectors_to_display();

  ShrdConstFVecPtr binning = getBinning( m_displayedSamples,
                                         use_gamma, m_dataMeasurement );

  size_t num_sec_channel = 0, num_back_channel = 0;
  if( m_secondDataMeasurement )
    num_sec_channel = m_secondDataMeasurement->num_gamma_channels();
  if( m_backgroundMeasurement )
    num_back_channel = m_backgroundMeasurement->num_gamma_channels();

  if( !binning )
  {
    size_t num_det_use = 0;
    for( bool t : use_gamma )
      num_det_use += t;

    if( num_det_use )
    {
      const size_t nspectra = num_det_use*m_displayedSamples.size();
      string msg = "<p>I couldnt determine binning to display the spectrum.</p>";
      if( nspectra > 1 )
        msg += "<p>You might try selecting only a single spectra to display in "
               "the <b>File manager</b>.</p>";
      passMessage( msg, "", WarningWidget::WarningMsgHigh );
    }//if( num_det_use )
  }//if( !binning )


  const bool canSub = (m_dataMeasurement && m_backgroundMeasurement);
  const bool isSub = m_spectrum->backgroundSubtract();
  if( m_backgroundSubItems[0]->isEnabled() != canSub )
    m_backgroundSubItems[0]->setDisabled( !canSub );
  if( m_backgroundSubItems[0]->isHidden() != isSub )
  {
    m_backgroundSubItems[0]->setHidden( isSub );
    m_backgroundSubItems[1]->setHidden( !isSub );
  }//if( m_backgroundSubItems[0]->isHidden() != isSub )

  std::shared_ptr<Measurement> dataH = m_dataMeasurement->sum_measurements( m_displayedSamples, use_gamma );
  
  if( dataH )
    dataH->set_title( "Foreground" );

  const float lt = dataH ? dataH->live_time() : 0.0f;
  const float rt = dataH ? dataH->real_time() : 0.0f;
  const float sum_neut = dataH ? dataH->neutron_counts_sum() : 0.0f;
  
  m_spectrum->setData( dataH, lt, rt, sum_neut, current_energy_range );
}//void displayForegroundData()


void InterSpec::displaySecondForegroundData()
{
  if( !m_secondDataMeasurement )
  {
    if( !!m_spectrum->secondData() )
    {
      m_sectondForgroundSampleNumbers.clear();
      m_spectrum->setSecondData( std::shared_ptr<Measurement>(), -1.0, -1.0, -1.0, false );
      if( !m_timeSeries->isHidden() )
        m_timeSeries->hide();
    }//if( !!m_spectrum->secondData() )
    
    return;
  }//if( !m_secondDataMeasurement )
  
  
  if( m_secondDataMeasurement->num_gamma_channels() )
  {
    if( !m_secondDataMeasurement->num_measurements() )
      throw runtime_error( "Serious logic error in"
                           " InterSpec::displaySecondForegroundData()" );

    vector<bool> foreground_det_use = detectors_to_display();
  
    //Lets make sure the second measurment actually has the same detectors
    //  as the foreground.  If it does, assume we want to display same detectors
    //  , if not, assume we should display all background detectors
    const vector<string> foredet = m_dataMeasurement ? m_dataMeasurement->detector_names() : vector<string>();
    const vector<string> &secodet = m_secondDataMeasurement->detector_names();
    const set<string> forenameset(foredet.begin(),foredet.end());
    const set<string> seconameset(secodet.begin(),secodet.end());
    
    vector<bool> second_det_use = foreground_det_use;
    if( forenameset != seconameset )
      second_det_use = vector<bool>( secodet.size(), true );
    
/*
    ShrdConstFVecPtr data_binning = getBinning( m_displayedSamples, foreground_det_use, m_dataMeasurement );
    const int nDataBin = static_cast<int>( data_binning ? data_binning->size() : 0 );

    ShrdConstFVecPtr second_binning = getBinning( m_sectondForgroundSampleNumbers, second_det_use, m_secondDataMeasurement );
    const int nSecDataBin = static_cast<int>( second_binning ? second_binning->size() : 0 );

    if( nDataBin && (nDataBin != nSecDataBin) )
    {
      stringstream msg;
      msg << "Binning of second foreground data (" << nSecDataBin << " bins) "
           << "does not match that of the foreground (" << nDataBin << " bins)";
      passMessage( msg.str(), "", WarningWidget::WarningMsgHigh );

      m_secondDataMeasurement = std::shared_ptr<SpecMeas>();
      m_sectondForgroundSampleNumbers.clear();
      m_spectrum->setSecondData( std::shared_ptr<Measurement>(), -1.0, -1.0, -1.0, false );

      return;
    }//if( there was a binning mismatch )
*/
    
    std::shared_ptr<Measurement> histH;
    if( m_secondDataMeasurement )
      histH = m_secondDataMeasurement->sum_measurements(m_sectondForgroundSampleNumbers, second_det_use );
    if( histH )
      histH->set_title( "Second Foreground" );
    
    const float lt = histH ? histH->live_time() : 0.0f;
    const float rt = histH ? histH->real_time() : 0.0f;
    const float neutronCounts = histH ? histH->neutron_counts_sum() : -1.0f;
    
    m_spectrum->setSecondData( histH, lt, rt, neutronCounts, false );
  }else
  {
    m_sectondForgroundSampleNumbers.clear();
    m_spectrum->setSecondData( std::shared_ptr<Measurement>(), -1.0, -1.0, -1.0, false );
  }//if( m_backgroundMeasurement ) / else
  
  if( !m_timeSeries->isHidden() )
  {
    vector< pair<double,double> > regions = timeRegionsToHighlight(kSecondForeground);
    m_timeSeries->setTimeHighLightRegions( regions, kSecondForeground );
  }
}//void displaySecondForegroundData()




void InterSpec::displayBackgroundData()
{
  if( !m_backgroundMeasurement && !m_spectrum->background() )
  {
    m_backgroundSubItems[0]->disable();
    m_backgroundSubItems[0]->show();
    m_backgroundSubItems[1]->hide();
    m_backgroundSampleNumbers.clear();
    return;
  }

  if( m_backgroundMeasurement && m_dataMeasurement
      && m_backgroundMeasurement->num_gamma_channels() )
  {
    vector<bool> foreground_det_use = detectors_to_display();
    
    //Lets make sure the background measurment actually has the same detectors
    //  as the foreground.  If it does, assume we want to display same detectors
    //  , if not, assume we should display all background detectors
    const vector<string> &foredet = m_dataMeasurement->detector_names();
    const vector<string> &backdet = m_backgroundMeasurement->detector_names();
    const set<string> forenameset(foredet.begin(),foredet.end());
    const set<string> backnameset(backdet.begin(),backdet.end());
   
    vector<bool> background_det_use = foreground_det_use;
    if( forenameset != backnameset )
      background_det_use = vector<bool>( backdet.size(), true );
    
//    ShrdConstFVecPtr data_binning = getBinning( m_displayedSamples, foreground_det_use, m_dataMeasurement );
//    const int nDataBin = static_cast<int>( data_binning ? data_binning->size() : 0 );

//    ShrdConstFVecPtr background_binning = getBinning( m_backgroundSampleNumbers, background_det_use, m_backgroundMeasurement );
//    const int nBackBin = static_cast<int>( background_binning ? background_binning->size() : 0 );

/*
    if( nDataBin && (nDataBin != nBackBin) )
    {
      stringstream msg;
      msg << "Binning of background data (" << nBackBin << " bins) does "
          << "not match that of data (" << nDataBin << " bins)";
      passMessage( msg.str(), "", WarningWidget::WarningMsgHigh );
      m_backgroundMeasurement = std::shared_ptr<SpecMeas>();
      m_spectrum->setBackground( std::shared_ptr<Measurement>(), -1.0, -1.0, -1.0 );

      m_backgroundSampleNumbers.clear();

      return;
//      throw runtime_error( "m_dataMeasurement->channel_energies().size() != m_backgroundMeasurement.channel_energies().size()" );
    }//if( there was a binning mismatch )
*/
    
    if( m_backgroundSampleNumbers.empty() )
      m_backgroundSampleNumbers = m_backgroundMeasurement->sample_numbers();

    std::shared_ptr<Measurement> backgroundH;
    if( m_backgroundMeasurement )
      backgroundH = m_backgroundMeasurement->sum_measurements( m_backgroundSampleNumbers, background_det_use );
    if( backgroundH )
      backgroundH->set_title( "Background" );
    
    const float lt = backgroundH ? backgroundH->live_time() : 0.0f;
    const float rt = backgroundH ? backgroundH->real_time() : 0.0f;
    const float neutronCounts = backgroundH ? backgroundH->neutron_counts_sum() : -1.0f;
    m_spectrum->setBackground( backgroundH, lt, rt, neutronCounts );
  }else
  {
//    if( !m_dataMeasurement )
//      m_backgroundMeasurement = std::shared_ptr<SpecMeas>();
    m_backgroundSampleNumbers.clear();
    m_spectrum->setBackground( std::shared_ptr<Measurement>(), -1.0, -1.0, -1.0 );
  }//if( m_backgroundMeasurement ) / else
  
  if( !m_timeSeries->isHidden() )
  {
    vector< pair<double,double> > regions = timeRegionsToHighlight(kBackground);
    m_timeSeries->setTimeHighLightRegions( regions, kBackground );
  }
  
  const bool canSub = (m_dataMeasurement && m_backgroundMeasurement);
  const bool isSub = m_spectrum->backgroundSubtract();
  if( m_backgroundSubItems[0]->isEnabled() != canSub )
    m_backgroundSubItems[0]->setDisabled( !canSub );
  if( m_backgroundSubItems[0]->isHidden() != isSub )
  {
    m_backgroundSubItems[0]->setHidden( isSub );
    m_backgroundSubItems[1]->setHidden( !isSub );
  }//if( m_backgroundSubItems[0]->isHidden() != isSub )
}//void displayBackgroundData()




