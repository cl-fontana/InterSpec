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
#include <sstream>
#include <string>
#include <vector>

#include <boost/regex.hpp>

#include <Wt/WText>
#include <Wt/WImage>
#include <Wt/WLabel>
#include <Wt/WSlider>
#include <Wt/WLineEdit>
#include <Wt/WDoubleSpinBox>
#include <Wt/WComboBox>
#include <Wt/WTabWidget>
#include <Wt/WGridLayout>
#include <Wt/WPushButton>
#include <Wt/WApplication>
#include <Wt/WStringStream>
#include <Wt/WDoubleValidator>
#include <Wt/WRegExpValidator>
#include <Wt/WContainerWidget>

// Disable streamsize <=> size_t warnings in boost
#pragma warning(disable:4244)

#include "InterSpec/SpecMeas.h"
#include "InterSpec/HelpSystem.h"
#include "InterSpec/WarningWidget.h"
#include "InterSpec/InterSpec.h"
#include "InterSpec/SpecMeasManager.h"
#include "InterSpec/SpectraFileModel.h"
#include "SpecUtils/UtilityFunctions.h"
#include "InterSpec/InterSpecApp.h"
#include "InterSpec/CanvasForDragging.h"
#include "InterSpec/CompactFileManager.h"
#include "SpecUtils/SpectrumDataStructs.h"
#if( !ANDROID && !IOS )
#include "InterSpec/FileDragUploadResource.h"
#endif

using namespace Wt;
using namespace std;


CompactFileManager::CompactFileManager( SpecMeasManager *fileManager,
                                        InterSpec *hostViewer,
                                        CompactFileManager::DisplayMode mode,
                                        WContainerWidget *parent )
  : WContainerWidget( parent ),
    m_foregroundTitle( 0 ),
    m_files( fileManager->model() ),
    m_hostViewer( hostViewer ),
    m_fullFileManager( fileManager ),
    m_displayMode( mode )
{
  addStyleClass( "CompactFileManager" );

#if (USE_DB_TO_STORE_SPECTRA)
  std::shared_ptr<DataBaseUtils::DbSession> sql = hostViewer->sql();
#endif
  
  const bool showToolTipInstantly
            = InterSpecUser::preferenceValue<bool>( "ShowTooltips", hostViewer );
  
  WGridLayout *layout = 0;
  WTabWidget  *tabWidget = 0;
  switch( m_displayMode )
  {
    case TopToBottom:
      break;
    
    case LeftToRight:
      layout = new WGridLayout();
      setLayout( layout );
      layout->setContentsMargins( 0, 0, 0, 0 );
      layout->setHorizontalSpacing( 0 );
      layout->setVerticalSpacing( 0 );
    break;
      
    case Tabbed:
      tabWidget = new WTabWidget( this );
    break;
  }//switch( m_displayMode )
  
  const char *prevArrow = "InterSpec_resources/images/previous_arrow.png";
  const char *nextArrow = "InterSpec_resources/images/next_arrow.png";
  const char *val_regex = "(\\-?\\d+\\s*((\\-|to|through)\\s*\\d+)?,*\\s*)+";
  
//  WPopupWidget *sampleToolTip = 0, *scaleToolTip = 0;
  
  WDoubleValidator *dblval = new WDoubleValidator( this );
  dblval->setRange( 0.0, 10000.0 );
//  dblval->setInvalidBlankText( "1.0" );
  
  WRegExpValidator *validator = new WRegExpValidator( val_regex, this );
  validator->setFlags( Wt::MatchCaseInsensitive );
  
  for( int j = 0; j < 3; ++j )
  {
    SpectrumType type;
    WLabel *label = NULL;

    switch( j )
    {
      case 0:
        type = kForeground;
        label = new WLabel( "Foreground:" );
      break;
      
      case 1:
        type = kBackground;
        label = new WLabel( "Background:" );
      break;
      
      case 2:
        type = kSecondForeground;
        label = new WLabel( "Second Foreground:" );
      break;
    }//switch( i )
    
    WContainerWidget *wrapper = new WContainerWidget();
    WGridLayout *wrapperLayout = new WGridLayout(wrapper);
    wrapperLayout->setVerticalSpacing( 0 );
    wrapperLayout->setHorizontalSpacing( 0 );
    wrapperLayout->setContentsMargins(0, 0, 0, 0);
    wrapper->addStyleClass( "CompactSampleWrapper" );
    wrapper->setOverflow(Wt::WContainerWidget::OverflowHidden);
    
    label->setInline( false );
    wrapperLayout->addWidget( label,0,0 );

    m_selects[type] = new WComboBox(  );
    wrapperLayout->addWidget( m_selects[type],1,0 );
    m_selects[type]->setInline( false );
    m_selects[type]->addStyleClass( "displaySampleSelect" );
    m_sampleDivs[type] = new WContainerWidget( );
    wrapperLayout->addWidget( m_sampleDivs[type],2,0 );
    wrapperLayout->setRowStretch(2, 1);
    m_sampleDivs[type]->setStyleClass( "displaySampleDiv" );

    m_prevSampleNumButtons[type] = new WImage( prevArrow, m_sampleDivs[type] );
    m_prevSampleNumButtons[type]->setHiddenKeepsGeometry( true );
    m_prevSampleNumButtons[type]->setStyleClass( "displayPrevSample" );
    
    
    m_prevSampleNumButtons[type]->clicked().connect(
                 boost::bind( &CompactFileManager::handleUserIncrementSampleNum,
                              this, type, false) );
    
    if( type==kForeground && (mode==LeftToRight || mode==Tabbed) )
    {
      m_foregroundTitle = new WText( "" );
      m_foregroundTitle->addStyleClass( "CompactSpecTitle" );
      m_foregroundTitle->setInline( false );
      m_foregroundTitle->hide();
      wrapperLayout->addWidget( m_foregroundTitle,3,0 );
    }//( type==kForeground && mode==LeftToRight )
    
    if( mode==TopToBottom || type == kForeground )
    {
      m_scaleValueTxt[type] = nullptr;
      m_rescaleByLiveTime[type] = nullptr;
    }else
    {
      WContainerWidget *rowdiv = new WContainerWidget( );
      wrapperLayout->addWidget( rowdiv,3,0, AlignBottom);
      WGridLayout *slidelayout = new WGridLayout( rowdiv );

      slidelayout->setHorizontalSpacing(10);
      slidelayout->setVerticalSpacing(10);
      m_scaleValueTxt[type] = new WDoubleSpinBox();
#if (IOS || ANDROID)
      //can't use m_hostViewer->isMobile() here, so use #IOS
      //20150123: on android at least, calling setNativeControl() causes
      //  a javascript exception (having to do with the validate) when first
      //  loading the app.
//      m_scaleValueTxt[type]->setNativeControl(true); //mobile should not show spinner
#endif
      m_scaleValueTxt[type]->setSingleStep(0.1);
      m_scaleValueTxt[type]->setRange( 0.0, 1000000.0 );
      m_scaleValueTxt[type]->setValidator( dblval );
      m_scaleValueTxt[type]->addStyleClass( "numberValidator"); //used to detect mobile keyboard
      m_scaleValueTxt[type]->addStyleClass( "SpecNormTxt" );
      m_scaleValueTxt[type]->changed().connect( boost::bind( &CompactFileManager::handleUserEnterdScaleFactor, this, type) );
      m_scaleValueTxt[type]->mouseWheel().connect( boost::bind( &CompactFileManager::handleUserEnterdScaleFactorWheel, this, type, _1) );
        
    
      HelpSystem::attachToolTipOn( m_scaleValueTxt[type],
                                  "Factor to scale the spectrum by. Entering an"
                                  " empty string will cause spectrum to be live"
                                  " time normalized to foreground spectrum.",
                                  showToolTipInstantly );
      
      WLabel *label = new WLabel( "Scale Factor:" );
      slidelayout->addWidget( label, 0, 0, AlignLeft | AlignMiddle );
      slidelayout->addWidget( m_scaleValueTxt[type], 0, 1, AlignLeft | AlignMiddle );
      
      m_rescaleByLiveTime[type] = new WPushButton( "Normalize" );
      m_rescaleByLiveTime[type]->hide();
      m_rescaleByLiveTime[type]->clicked().connect( boost::bind( &CompactFileManager::handleRenormalizeByLIveTime, this, type) );
      //20190707: making this button visible adds some jitter to the layout, but
      //  since this is a rare-ish action, we'll live with it for now.
      slidelayout->addWidget( m_rescaleByLiveTime[type], 0, 2, AlignCenter | AlignMiddle );
      
      slidelayout->setColumnStretch( 1, 1 );
//      slidelayout->setRowStretch( 1, 1 );
    
      m_scaleValueTxt[type]->disable();
    }//if( type == kForeground )
    
    
    m_displayedPreTexts[type] = new WText( m_sampleDivs[type] );
    WLineEdit *edit = new WLineEdit( m_sampleDivs[type] );
    edit->addStyleClass( "displaySampleInput" );
    edit->setValidator( validator );
    m_displaySampleNumEdits[type] = edit;
  
    const char *tooltip = "Enter the sample number you'de like displayed here"
                           ". You may enter a range of sample numbers similar"
                           " to '33-39' or '33 to 39'; or CSV sample numbers"
                           " like '34,39,84'";
    HelpSystem::attachToolTipOn( edit, tooltip, showToolTipInstantly,
                                   HelpSystem::Bottom );
    

    m_displaySampleNumEdits[type]->setTextSize( 6 );
    m_displayedPostTexts[type] = new WText( m_sampleDivs[type] );
    m_nextSampleNumButtons[type] = new WImage( nextArrow, m_sampleDivs[type] );
    m_nextSampleNumButtons[type]->addStyleClass( "displayNextSample" );
    
    
    m_nextSampleNumButtons[type]->clicked().connect(
                 boost::bind( &CompactFileManager::handleUserIncrementSampleNum,
                              this, type, true) );
    
    m_displaySampleNumEdits[type]->enterPressed().connect(
                 boost::bind( &CompactFileManager::handleUserChangeSampleNum,
                              this, type ) );
    
    m_displaySampleNumEdits[type]->blurred().connect(
                 boost::bind( &CompactFileManager::handleSampleNumEditBlur,
                              this, type ) );
    
    m_sampleDivs[type]->setHiddenKeepsGeometry( true );

    m_selects[type]->setCurrentIndex( -1 );
    m_selects[type]->activated().connect(
                 boost::bind( &CompactFileManager::handleFileChangeRequest,
                              this, _1, type ) );


    
    switch( m_displayMode )
    {
      case TopToBottom:
        addWidget( wrapper );
        break;
        
      case LeftToRight:
            
        if (j!=0)
            layout->addWidget( wrapper, 0, j, 2,1 );
        else
            layout->addWidget( wrapper, 0, j, 1,1);
        layout->setRowStretch(0, 1);
        layout->setColumnStretch(j, 1);
            
        if (j==0)
        {
          //Foreground, add in Manager and Library buttons for quick access
          WContainerWidget* temp = new WContainerWidget();
          temp->addStyleClass( "CompactManagerButtons" );

            WGridLayout* layout2 = new WGridLayout();
            layout2->setContentsMargins(0,0,0,0);
            temp->setLayout(layout2);
            WPushButton *button = new WPushButton(  "Spectrum Manager...");
            //button->setIcon(Wt::WLink("InterSpec_resources/images/computer.png" ));
            button->clicked().connect( m_hostViewer->fileManager(), &SpecMeasManager::startSpectrumManager );
            layout2->addWidget(button,0,0);
#if( USE_DB_TO_STORE_SPECTRA )
            WPushButton *button2 = new WPushButton( "Previous...");
            button2->clicked().connect(  boost::bind(
                                      &SpecMeasManager::browseDatabaseSpectrumFiles, m_hostViewer->fileManager(), "", (SpectrumType)0, std::shared_ptr<SpectraFileHeader>() ) );
            layout2->addWidget(button2,0,1);
#endif
            layout2->setColumnStretch(0, 1);
            layout2->setColumnStretch(1, 1);
            layout->addWidget( temp, 1, j, AlignMiddle );
        }
        break;
        
      case Tabbed:
      {
        const char *tablabel = 0;
        switch( j )
        {
          case 0: tablabel = "Foreground"; break;
          case 1: tablabel = "Background"; break;
          case 2: tablabel = "Second";     break;
        }//switch( i )

        tabWidget->addTab( wrapper, tablabel, WTabWidget::PreLoading );
        break;
      }//case Tabbed:
    }//switch( m_displayMode )
  }//for( int j = 0; j < 3; ++j )
  
  //Lets add in a few more customizations based on the display type
  switch( m_displayMode )
  {
    case TopToBottom:
    {
      //Add a tool tip note, as well as an explicit text not if widget is large
      //  enough
      const char *msg = "Drag files onto app to open";
      const char *tooltip = "";
      
      WText *note = new WText( msg );
      HelpSystem::attachToolTipOn( this, tooltip, showToolTipInstantly );
      //setToolTip( tooltip );

      
      note->setAttributeValue( "style", "position:absolute;bottom:5px;"
                                + note->attributeValue("style"));
      note->addStyleClass( "CompactInfoTxt" );
      note->setInline( false );
      addWidget( note );
    
      //should hide note if total widget is less than ~210px.
      const string jsref = "$('#" + note->id() + "')";
      const string js = "function(self, w, h, layout){"
                        "if(h<320) " + jsref + ".hide();"
                        "else " +  jsref + ".show();}";
      setJavaScriptMember( "wtResize", js );

      break;
    }//case TopToBottom:
      
    case LeftToRight:
      //Nothing to do here
    break;
      
    case Tabbed:
    {
      //Add in a link to open files in the database, as
#if( USE_DB_TO_STORE_SPECTRA )
      WContainerWidget *buttons = new WContainerWidget( this );
      WPushButton *button = new WPushButton( "Prev. Saved Spectra", buttons );
        button->clicked().connect( boost::bind(
                                &SpecMeasManager::browseDatabaseSpectrumFiles,fileManager,"", (SpectrumType)0, std::shared_ptr<SpectraFileHeader>()) );
#endif
      break;
    }//case Tabbed:
  }//switch( m_displayMode )
    
  // Then actually pull in the available files. If there are none, say so.
  refreshContents();

  // Lastly, hook up the update handler so it'll keep it posted.
  m_files->rowsInserted().connect( this, &CompactFileManager::refreshContents );
  m_files->rowsRemoved().connect( this, &CompactFileManager::refreshContents );
  hostViewer->displayedSpectrumChanged().connect(
                          boost::bind( &CompactFileManager::handleDisplayChange,
                                       this, _1, _2, _3 ) );
}//CompactFileManager constructor


CompactFileManager::~CompactFileManager()
{
  // no-op
}

void CompactFileManager::handleFileChangeRequest( int row, SpectrumType type )
{
  if( row < 0 || row >= m_files->rowCount() )
  {
    m_displayedPreTexts[type]->setText( "" );
    m_displayedPostTexts[type]->setText( "" );
    m_displaySampleNumEdits[type]->setText( "" );
    std::shared_ptr<SpecMeas> blankptr;
    m_fullFileManager->displayFile( -1, blankptr, type, false, true, false );
    if( m_foregroundTitle && (type==kForeground) )
    {
      m_foregroundTitle->setText("");
      m_foregroundTitle->hide();
    }//if( type == kForeground )
    return;
  }//if( we should unload current file )

  std::shared_ptr<const SpectraFileHeader> header = m_files->fileHeader(row);
  std::shared_ptr<SpecMeas> meas = header->parseFile();

  m_fullFileManager->displayFile( row, meas, type, false, true, false );
}//void handleFileChangeRequest( SpectrumType type );

/**
 Refactored static method so can be called outside of CompactFileManager
 
 cfm - optional, if provided, will call handleDisplayChange.  Set to NULL normally if called statically.  Otherwise, CompactFileManager calls will provide it.
 */
void CompactFileManager::changeToSampleNum( int sampleNum,
                                            SpectrumType type, InterSpec *hostViewer, CompactFileManager* cfm )
{
  std::shared_ptr<SpecMeas> meas = hostViewer->measurment( type );

  if( meas )
  {
    const set<int> total_sample_nums = meas->sample_numbers();
    if( total_sample_nums.find(sampleNum) == total_sample_nums.end() )
    {
      if (cfm)
          cfm->handleDisplayChange( type, meas, hostViewer->displayedSamples(type) );
      
      passMessage( "Sample number requested doesnt exist.",
                   "", WarningWidget::WarningMsgHigh );
      return;
    }//if( total_sample_nums.find(sampleNum) == total_sample_nums.end() )

    
    set<int> sampleNumToLoad;
    sampleNumToLoad.insert( sampleNum );
    hostViewer->changeDisplayedSampleNums( sampleNumToLoad, type );
//    m_hostViewer->setSpectrum( meas, sampleNumToLoad, type, false );
  }else
  {
    passMessage( "Cant change sample numbers, there is no current measurment",
                 "", WarningWidget::WarningMsgHigh );
  }//if( meas ) / else
}//void changeToSampleNum(...)


void CompactFileManager::changeToSampleRange( int first, int last,
                                              SpectrumType type )
{
  std::shared_ptr<SpecMeas> meas = m_hostViewer->measurment( type );

  if( meas )
  {
    const set<int> total_sample_nums = meas->sample_numbers();
    const set<int>::const_iterator firstPos = total_sample_nums.find(first);
    set<int>::const_iterator lastPos = total_sample_nums.find(last);

    if( firstPos == total_sample_nums.end()
        || lastPos == total_sample_nums.end() )
    {
      handleDisplayChange( type, meas, m_hostViewer->displayedSamples(type) );
      passMessage( "One of the sample number requested doesnt exist.",
                   "", WarningWidget::WarningMsgHigh );
      return;
    }//if( total_sample_nums.find(sampleNum) == total_sample_nums.end() )

    set<int> sampleNumToLoad;
    if( lastPos != total_sample_nums.end() )
      ++lastPos;
    sampleNumToLoad.insert( firstPos, lastPos );
    m_hostViewer->changeDisplayedSampleNums( sampleNumToLoad, type );
//    m_hostViewer->setSpectrum( meas, sampleNumToLoad, type, false );
  }else
  {
    passMessage( "Can't change sample numbers, there is no current measurment",
                 "",WarningWidget::WarningMsgHigh );
  }//if( meas ) / else
}//void changeToSampleRange(...)


void CompactFileManager::handleSampleNumEditBlur( SpectrumType type )
{
  if( m_previousSampleTxt[type] == m_displaySampleNumEdits[type]->text() )
    return;

  handleUserChangeSampleNum( type );
}//void handleSampleNumEditBlur( SpectrumType spectrum_type )


void CompactFileManager::handleSpectrumScale( const double scale, SpectrumType type )
{
  switch( type )
  {
    case SpectrumType::kForeground:
      cerr << "CompactFileManager::handleSpectrumScale: cannot handle scaling of foreground - ignoring" << endl;
      return;
      
    case SpectrumType::kBackground:
    case SpectrumType::kSecondForeground:
      m_hostViewer->setDisplayScaleFactor( scale, type );
      updateDisplayedScaleFactorNumbers( scale, type );
      if( m_rescaleByLiveTime[type] )
        m_rescaleByLiveTime[type]->show();
      break;
  }//switch( type )
}//void handleSpectrumScale( const double scale, SpectrumType spectrum_type );


void CompactFileManager::handleUserChangeSampleNum( SpectrumType type )
{
  const WString text = m_displaySampleNumEdits[type]->text();

  string fulltxt = text.toUTF8();

  //Lets replace numbers seperated by spaces, to be seperated by commas.
  //  We cant do a simple replace of spaces to commas, and using a regex would
  //  require using a lookahead or behind, and I dont think boost supports that
  //  always.  Note that below while loop is a little ineficient, but whatever
  boost::smatch mtch;
  boost::regex expr( ".*(\\d\\s+\\d).*" );
  while( boost::regex_match( fulltxt, mtch, expr ) )
    fulltxt[mtch[1].first - fulltxt.begin() + 1] = ',';
  
//Using a sregex_token_iterator doesnt work for single digit numbers, ex
//  '1 2 3 45 983 193'  will go to '1,2 3,45,983,193'
//  boost::regex expr( "\\d\\s+\\d" );
//  for( boost::sregex_token_iterator iter(fulltxt.begin(),fulltxt.end(),expr,0);
//      iter != boost::sregex_token_iterator(); ++iter )
//    fulltxt[iter->first - fulltxt.begin()+1] = ',';
  
  vector<string> sampleranges;
  UtilityFunctions::split( sampleranges, fulltxt, "," );

  std::shared_ptr<SpecMeas> meas = m_hostViewer->measurment( type );

  if( !meas )
  {
    passMessage( "Can't change sample numbers, there is no current measurment",
                 "", WarningWidget::WarningMsgHigh );
    m_displaySampleNumEdits[type]->setText( "" );
    return;
  }//if( !meas )

  const set<int> samples = meas->sample_numbers();
  set<int> sampleNumToLoad;

  try
  {
    for( string textStr : sampleranges )
    {
      UtilityFunctions::trim( textStr );
      if( textStr.empty() )
        continue;

      boost::smatch matches;
      boost::regex range_expression( "(\\d+)\\s*(\\-|to|through)\\s*(\\d+)",
                                     boost::regex::perl|boost::regex::icase );
      if( boost::regex_match( textStr, matches, range_expression ) )
      {
        string firstStr = string( matches[1].first, matches[1].second );
        string lastStr = string( matches[3].first, matches[3].second );

        int first = std::stoi( firstStr );
        int last = std::stoi( lastStr );
        if( last < first )
          std::swap( last, first );

        const set<int>::const_iterator firstPos = samples.find(first);
        set<int>::const_iterator lastPos = samples.find(last);

        if( firstPos==samples.end() || lastPos==samples.end() )
        {
          stringstream msg;
          if( firstPos == samples.end() )
            msg << "Sample number " << firstStr << " doesnt exist.";
          else
            msg << "Sample number " << lastStr << " doesnt exist.";
          passMessage( msg.str(), "", WarningWidget::WarningMsgHigh );
          const set<int> samples = m_hostViewer->displayedSamples(type);
          handleDisplayChange( type, meas, samples );
          return;
        }//if( samples.find(sampleNum) == samples.end() )

        if( lastPos != samples.end() )
          ++lastPos;
        sampleNumToLoad.insert( firstPos, lastPos );
      }else
      {
        const int sample = std::stoi( textStr );
        
        if( !samples.count(sample) )
        {
          stringstream msg;
          msg << "Sample number " << sample << " does not exist in the measurement.";
          passMessage( msg.str(), "", WarningWidget::WarningMsgHigh );
          const set<int> samples = m_hostViewer->displayedSamples(type);
          handleDisplayChange( type, meas, samples );
          return;
        }//if( !samples.count(sample) )
          
//        changeToSampleNum( sample, type );
        sampleNumToLoad.insert( sample );
      }//if( is sample range ) / else
    }//for( string textStr : sampleranges )

//    m_hostViewer->setSpectrum( meas, sampleNumToLoad, type, false );
    m_hostViewer->changeDisplayedSampleNums( sampleNumToLoad, type );
    updateDisplayedScaleFactorNumbers( m_hostViewer->displayScaleFactor(type), type );
  }catch( exception &e )
  { 
    cerr << SRC_LOCATION << "\n\t" << e.what() << endl;
    passMessage( "Error changing to requested sample number(s)",
                 "", WarningWidget::WarningMsgHigh );

    if( meas )
      handleDisplayChange( type, meas, m_hostViewer->displayedSamples(type) );
//    m_displaySampleNumEdits[type]->setText( "" );
  }//  try / catch
}//void handleUserChangeSampleNum(...)

/**
 Backward compatibility for previous CompactFileManager calls to call the refactored method
 */
void CompactFileManager::handleUserIncrementSampleNum( SpectrumType type,
                                                      bool increment )
{
    handleUserIncrementSampleNum(type, increment, m_hostViewer, m_files, this);
}

/**
 Refactored code so can be called statically from outside of CompactFileManager
 */
void CompactFileManager::handleUserIncrementSampleNum( SpectrumType type,
                                                       bool increment , InterSpec *hostViewer, SpectraFileModel *files, CompactFileManager* cfm)
{
  try
  {
    std::shared_ptr<SpecMeas> meas = hostViewer->measurment( type );

    if( !meas )
      throw std::runtime_error( "Unable to get measurment" );

    const set<int> total_sample_nums = meas->sample_numbers();
    const set<int> &currentSamples = hostViewer->displayedSamples( type );
    int currentSample = -1;

    if( currentSamples.empty() && total_sample_nums.size() )
      currentSample = *(total_sample_nums.begin());
    else if( currentSamples.size() > 1 )
      currentSample = *(currentSamples.rbegin());
    else if( currentSamples.size() )
      currentSample = *(currentSamples.begin());

    if( total_sample_nums.size() == 1 && currentSamples.size() )
      return;

    const WModelIndex index = files->index( meas );

    if( !index.isValid() )
      throw std::runtime_error( "Unable to get index" );

    set<int>::iterator pos = total_sample_nums.find( currentSample );
    if( pos == total_sample_nums.end() )
      throw std::runtime_error( "Unable to find current index" );

    if( increment )
    {
      ++pos;
    }else
    {
      if( pos == total_sample_nums.begin() )
        pos = total_sample_nums.end();
      --pos;
    }//if( increment ) / else

    if( pos == total_sample_nums.end() )
      pos = total_sample_nums.begin();

    changeToSampleNum( *pos, type , hostViewer, cfm);
  }catch( std::runtime_error e )
  {
    cerr << "CompactFileManager::handleUserIncrementSampleNum(...): caught "
         << e.what() << endl;
    passMessage( "Error advancing spectrum", "",WarningWidget::WarningMsgHigh );
  }catch(...)
  {
    passMessage( "Error advancing spectrum", "",WarningWidget::WarningMsgHigh );
  }//try / catch
}//void handleUserWantsNextSampleNum( SpectrumType spectrum_type )


void CompactFileManager::handleDisplayChange( SpectrumType spectrum_type,
                                  const std::shared_ptr<SpecMeas> meas,
                                  const set<int> &sample_numbers )
{
  if( spectrum_type < 0 || spectrum_type >= 3 )
    throw runtime_error( SRC_LOCATION + " - totally unexpected error" );

  if( m_foregroundTitle && (spectrum_type==kForeground) )
    m_foregroundTitle->hide();
  
  WComboBox *select = m_selects[spectrum_type];
  if( !meas )
  {
    select->setCurrentIndex( select->count()-1 );
    m_previousSampleTxt[spectrum_type] = "";
    m_displayedPreTexts[spectrum_type]->setText( "" );
    m_displayedPostTexts[spectrum_type]->setText( "" );
    m_displaySampleNumEdits[spectrum_type]->setText( "" );
    updateDisplayedScaleFactorNumbers( 1.0, spectrum_type );
    if( m_scaleValueTxt[spectrum_type] )
      m_scaleValueTxt[spectrum_type]->disable();
    if( m_rescaleByLiveTime[spectrum_type] )
      m_rescaleByLiveTime[spectrum_type]->hide();
    
    return;
  }//if( !meas )

  if( m_scaleValueTxt[spectrum_type] )
  {
    m_scaleValueTxt[spectrum_type]->enable();
  }
  
  if( m_rescaleByLiveTime[spectrum_type] )
    m_rescaleByLiveTime[spectrum_type]->hide();

  WModelIndex index = m_files->index( meas );
  if( !index.isValid() )
  {
    select->setCurrentIndex( select->count()-1 );
    m_previousSampleTxt[spectrum_type] = "";
    return;
  }//if( !index.isValid() )

  //if( meas )
  //  cout << "There are " << meas->sample_numbers().size() << " sample numbers, " << meas->measurements().size() << " meas" << endl;
  
  select->setCurrentIndex( index.row() );

  const set<int> total_sample_nums = meas->sample_numbers();

  if( sample_numbers.size() == 0 || total_sample_nums.size() == 0 )
  {
//    m_sampleDivs[spectrum_type]->hide();
  }else if( sample_numbers.size() == 1 )
  {
    const int lastNumber = *(total_sample_nums.rbegin());
    const int displayedNumber = *(sample_numbers.begin());

    m_displayedPostTexts[spectrum_type]->show();
    m_displaySampleNumEdits[spectrum_type]->show();
    
    const bool hideArrows = (total_sample_nums.size() < 2);
    m_nextSampleNumButtons[spectrum_type]->setHidden( hideArrows );
    m_prevSampleNumButtons[spectrum_type]->setHidden( hideArrows );
    
    switch( m_displayMode )
    {
      case TopToBottom:
        if( m_displayedPreTexts[spectrum_type]->text() != "Sample " )
          m_displayedPreTexts[spectrum_type]->setText( "Sample " );
      break;
      case LeftToRight: case Tabbed: break;
    }//switch( m_displayMode )
    
    
    WStringStream postMsg, editVal;
    postMsg << "/" << lastNumber; //(int)total_sample_nums.size();
    m_displayedPostTexts[spectrum_type]->setText( postMsg.str() );
    editVal << displayedNumber;
    m_displaySampleNumEdits[spectrum_type]->setText( editVal.str() );
    
    
    if( spectrum_type == kForeground )
    {
      WString title;
      const std::vector<int> &dets = meas->detector_numbers();
      if( dets.size() == 1 )
      {
        MeasurementConstShrdPtr m = meas->measurement(displayedNumber, dets[0] );
        if( m )
        {
          title = WString::fromUTF8(m->title());
#ifndef WT_NO_STD_WSTRING
          std::wstring str = title;
          if( str.length() > 80 )
            str = str.substr( 0, 80 );
          title = str;
#else
          std::string str = title.toUTF8();
          UtilityFunctions::utf8_limit_str_size( str, 79 );
          title = str;
#endif
        }//if( m )
      }//if( dets.size() == 1 )
      
      if( m_foregroundTitle )
      {
        m_foregroundTitle->setText( title );
        if( !title.empty() )
          m_foregroundTitle->show();
      }//if( m_foregroundTitle )
    }//if( spectrum_type == kForeground )
  }else if( total_sample_nums.size() == sample_numbers.size() )
  {
    switch( m_displayMode )
    {
      case TopToBottom:
        if( m_displayedPreTexts[spectrum_type]->text() != "Sample " )
          m_displayedPreTexts[spectrum_type]->setText( "Sample " );
        break;
      case LeftToRight: case Tabbed: break;
    }//switch( m_displayMode )
    
    char buffer[64];
    const int totalNumber = *(total_sample_nums.rbegin());
    const int lastNumber = *(sample_numbers.rbegin());
    const int firstNumber = *(sample_numbers.begin());
    snprintf( buffer, sizeof(buffer), "/%i", totalNumber+1 );
    m_displayedPostTexts[spectrum_type]->setText( buffer );
    if( lastNumber == firstNumber )
      snprintf( buffer, sizeof(buffer), "%i", lastNumber );
    else if( sample_numbers.size() == 2 )
      snprintf( buffer, sizeof(buffer), "%i,%i", firstNumber, lastNumber );
    else
      snprintf( buffer, sizeof(buffer), "%i-%i", firstNumber, lastNumber );
    m_displaySampleNumEdits[spectrum_type]->setText( buffer );
    m_nextSampleNumButtons[spectrum_type]->hide();
    m_prevSampleNumButtons[spectrum_type]->hide();
  }else
  {
    const int lastNumber = *(total_sample_nums.rbegin());
    
    switch( m_displayMode )
    {
      case TopToBottom:
        if( m_displayedPreTexts[spectrum_type]->text() != "Sample " )
          m_displayedPreTexts[spectrum_type]->setText( "Sample " );
      break;
      case LeftToRight: case Tabbed:
      break;
    }//switch( m_displayMode )
      
    WStringStream postMsg;
    postMsg << "/" << lastNumber;
    m_displayedPostTexts[spectrum_type]->setText( postMsg.str() );
   
    const string displaySequence = UtilityFunctions::sequencesToBriefString( sample_numbers );
    const bool added = (displaySequence.find(',') != string::npos);
    m_displaySampleNumEdits[spectrum_type]->setText( displaySequence );
    
    m_nextSampleNumButtons[spectrum_type]->setHidden( !added );
    m_prevSampleNumButtons[spectrum_type]->setHidden( !added );
  }//else if( meas->passthrough() ){...}
  
  m_previousSampleTxt[spectrum_type]
                               = m_displaySampleNumEdits[spectrum_type]->text();
  
  const double livetimeSF = m_hostViewer->displayScaleFactor(spectrum_type);
  
  updateDisplayedScaleFactorNumbers( livetimeSF, spectrum_type );
  
  if( spectrum_type == kForeground )
  {
    if( !!m_hostViewer->measurment(kBackground) )
      updateDisplayedScaleFactorNumbers( m_hostViewer->displayScaleFactor(kBackground), kBackground );
    if( !!m_hostViewer->measurment(kSecondForeground) )
      updateDisplayedScaleFactorNumbers( m_hostViewer->displayScaleFactor(kSecondForeground), kSecondForeground );
  }//if( spectrum_type == kForeground )
}//void handleDisplayChange(...)


void CompactFileManager::refreshContents()
{
  // Clear out the combo boxes
  for( int i = 0; i < 3; ++i )
    m_selects[i]->clear();

  if( m_files->rowCount() < 1 )
  {
    for( int i = 0; i < 3; ++i )
      m_selects[i]->addItem( "No uploaded/available spectra." );
  }else
  {
    for( int i = 0; i < m_files->rowCount(); ++i )
    {
      const WString name = m_files->fileHeader( i )->displayName();
      const WString time = m_files->fileHeader(i)->spectrumTime().toString("MMM d, yyyy");
      for( int i = 0; i < 3; ++i )
        m_selects[i]->addItem( name + " (" + time + ")" );
    }//for( loop over available files )

    m_selects[kForeground]->addItem( "No foreground" );
    m_selects[kBackground]->addItem( "No background" );
    m_selects[kSecondForeground]->addItem( "No second foreground" );

    // Lastly, select the proper file. If there is no active file in that
    // category, just select the default "No _______" option.
    for( int i = 0; i < 3; ++i )
    {
      const SpectrumType type = SpectrumType(i);
      std::shared_ptr<SpecMeas> meas = m_hostViewer->measurment( type );
      handleDisplayChange( type, meas, m_hostViewer->displayedSamples(type) );
    }//for( int i = 0; i < 3; ++i )
  }//if( m_files->rowCount() < 1 ) / else
}//void refreshContents()


void CompactFileManager::updateDisplayedScaleFactorNumbers( const double sf,
                                               const SpectrumType type )
{
  if( !m_scaleValueTxt[type] )
    return;
  
  char buffer[32];
  snprintf( buffer, sizeof(buffer), "%.2f", sf );
  m_scaleValueTxt[type]->setText( buffer );
}//void updateDisplayedScaleFactorNumbers(...)


//void CompactFileManager::handleSliderChanged( const int slidervalue,
//                                              const SpectrumType type )
//{
//  bool update = true;
//  const double multiple = (slidervalue-50.0) / 50.0;
//  
//  const double oldsf = m_hostViewer->displayScaleFactor(type);
//  
//  double sf = oldsf + (0.25 * multiple * oldsf);
//  
//  if( !m_hostViewer->measurment(type) )
//  {
//    sf = 1.0;
//    update = false;
//  }
//
//  updateDisplayedScaleFactorNumbers( sf, type );
//  
//  if( update )
//    m_hostViewer->setDisplayScaleFactor( sf, type );
//}//void handleSliderChanged(...);


void CompactFileManager::handleUserEnterdScaleFactor( const SpectrumType type )
{
  if( !m_scaleValueTxt[type] )
    return;
  
  float sf;
  bool update = true;
  
  const string val = m_scaleValueTxt[type]->text().toUTF8();
  
  if( val.empty() )
  {
    const float lt = m_hostViewer->liveTime(type);
    const float datalt = m_hostViewer->liveTime(kForeground);
    sf = ((lt>0.0f) ? (datalt/lt) : 1.0f);
//    char buffer[16];
//    snprintf(buffer, sizeof(buffer), "%.2f", sf );
  }else if( !(stringstream(val) >> sf) )
  {
    update = false;
    sf = m_hostViewer->displayScaleFactor(type);
  }//
  
  updateDisplayedScaleFactorNumbers( sf, type );
  
  
  if( update )
  {
    if( m_rescaleByLiveTime[type] )
      m_rescaleByLiveTime[type]->show();
    m_hostViewer->setDisplayScaleFactor( sf, type );
  }
}//void handleUserEnterdScaleFactor( const SpectrumType type )



void CompactFileManager::handleUserEnterdScaleFactorWheel( const SpectrumType type,  WMouseEvent e )
{
  int i = e.wheelDelta();
  m_scaleValueTxt[type]->setValue(m_scaleValueTxt[type]->value() + m_scaleValueTxt[type]->singleStep()*i);
  handleUserEnterdScaleFactor(type);
}//void handleUserEnterdScaleFactor( const SpectrumType type )


void CompactFileManager::handleRenormalizeByLIveTime( const SpectrumType type )
{
  const float lt = m_hostViewer->liveTime(type);
  const float datalt = m_hostViewer->liveTime(kForeground);
  const double sf = ((lt>0.0f && datalt>0.0f) ? (datalt/lt) : 1.0f);
  updateDisplayedScaleFactorNumbers( sf, type );
  if( m_rescaleByLiveTime[type] )
    m_rescaleByLiveTime[type]->hide();
  m_hostViewer->setDisplayScaleFactor( sf, type );
}//void handleRenormalizeByLIveTime( const SpectrumType type )

