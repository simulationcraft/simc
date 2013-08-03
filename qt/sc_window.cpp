// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "simulationcraftqt.hpp"
#ifdef SC_PAPERDOLL
#include "simcpaperdoll.hpp"
#endif
#include <QtWebKit/QtWebKit>
#if defined( Q_WS_MAC ) || defined( Q_OS_MAC )
#include <CoreFoundation/CoreFoundation.h>
#endif
#if QT_VERSION_5
#include <QStandardPaths>
#endif

namespace { // UNNAMED NAMESPACE

const char* SIMC_HISTORY_FILE = "simc_history.dat";
const char* SIMC_LOG_FILE = "simc_log.txt";

// ==========================================================================
// Utilities
// ==========================================================================


const QString defaultSimulateText( "# Profile will be downloaded into a new tab.\n"
                                   "#\n"
                                   "# Clicking Simulate will create a simc_gui.simc profile for review.\n" );

} // UNNAMED NAMESPACE

// ==========================================================================
// SC_MainWindow
// ==========================================================================

void SC_MainWindow::updateSimProgress()
{
  sim_t* sim = 0;

#ifdef SC_PAPERDOLL
  // If we're in the paperdoll tab, check progress on the current sim running
  if ( mainTab -> currentTab() == TAB_PAPERDOLL )
    sim = paperdoll_sim ? paperdoll_sim : SC_MainWindow::sim;
  else
#endif
    sim = SC_MainWindow::sim;

  if ( sim )
  {
    simProgress = static_cast<int>( 100.0 * sim -> progress( simPhase ) );
  }
  else
  {
    simPhase = "%p%";
    simProgress = 100;
  }
  if ( mainTab -> currentTab() != TAB_IMPORT &&
       mainTab -> currentTab() != TAB_RESULTS )
  {
    progressBar -> setFormat( QString::fromUtf8( simPhase.c_str() ) );
    progressBar -> setValue( simProgress );
  }
}

void SC_MainWindow::loadHistory()
{
  http::cache_load( ( TmpDir.toStdString() + "/" + "simc_cache.dat" ).c_str() );

  QFile file( AppDataDir + "/" + SIMC_HISTORY_FILE );
  if ( file.open( QIODevice::ReadOnly ) )
  {
    QDataStream in( &file );

    QString historyVersion;
    in >> historyVersion;
    if ( historyVersion != QString( SC_VERSION ) ) return;
    in >> historyWidth;
    in >> historyHeight;
    in >> historyMaximized;
    in >> simulateCmdLineHistory;
    in >> logCmdLineHistory;
    in >> resultsCmdLineHistory;
    in >> optionsHistory;
    SC_StringHistory SimulateTextHistory_Title;
    SC_StringHistory SimulateTextHistory_Content;
    in >> SimulateTextHistory_Title;
    in >> SimulateTextHistory_Content;
    in >> overridesTextHistory;
    QStringList importHistory;
    in >> importHistory;
    file.close();

    for ( int i = 0, count = importHistory.count(); i < count; i++ )
      historyList -> addItem( new QListWidgetItem( importHistory.at( i ) ) );

    assert( SimulateTextHistory_Title.count() == SimulateTextHistory_Content.count() );
    for ( int i = 0, count = SimulateTextHistory_Title.count(); i < count; i++ )
      simulateTab -> add_Text( SimulateTextHistory_Content.at( i ), SimulateTextHistory_Title.at( i ) );

    optionsTab -> decodeOptions( optionsHistory.backwards() );

    QString s = overridesTextHistory.backwards();
    if ( ! s.isEmpty() )
      overridesText -> setPlainText( s );
  }
}

void SC_MainWindow::saveHistory()
{
  charDevCookies -> save();
  http::cache_save( ( TmpDir.toStdString() + "/" + "simc_cache.dat" ).c_str() );

  QFile file( AppDataDir + "/" + SIMC_HISTORY_FILE );
  if ( file.open( QIODevice::WriteOnly ) )
  {
    optionsHistory.add( optionsTab -> encodeOptions() );

    QStringList importHistory;
    int count = historyList -> count();
    for ( int i = 0; i < count; i++ )
      importHistory.append( historyList -> item( i ) -> text() );

    QDataStream out( &file );
    out << QString( SC_VERSION );
    out << ( qint32 ) width();
    out << ( qint32 ) height();
    out << ( qint32 ) ( ( windowState() & Qt::WindowMaximized ) ? 1 : 0 );
    out << simulateCmdLineHistory;
    out << logCmdLineHistory;
    out << resultsCmdLineHistory;
    out << optionsHistory;

    SC_StringHistory SimulateTextHistory_Title;
    SC_StringHistory SimulateTextHistory_Content;
    for ( int i = 0; i < simulateTab -> count(); ++i )
    {
      SC_PlainTextEdit* tab = static_cast<SC_PlainTextEdit*>( simulateTab -> widget( i ) );

      if ( simulateTab -> tabText( i ) == "Simulate!" )
      {
        if ( tab -> edited_by_user )
          simulateTab -> setTabText( i, QDateTime::currentDateTime().toString() );
        else
          continue; // skip legend
      }

      SimulateTextHistory_Title.add( simulateTab -> tabText( i ) );
      SimulateTextHistory_Content.add( tab -> toPlainText() );
    }
    out << SimulateTextHistory_Title;;
    out << SimulateTextHistory_Content;
    out << overridesTextHistory;
    out << importHistory;
    file.close();
  }
}

// ==========================================================================
// Widget Creation
// ==========================================================================

SC_MainWindow::SC_MainWindow( QWidget *parent )
  : QWidget( parent ),
    historyWidth( 0 ), historyHeight( 0 ), historyMaximized( 1 ),
    visibleWebView( 0 ), sim( 0 ), paperdoll_sim( 0 ), simPhase( "%p%" ), simProgress( 100 ), simResults( 0 ),
    AppDataDir( "." ), TmpDir( "." )
{
  setAttribute( Qt::WA_AlwaysShowToolTips );
#if defined( Q_WS_MAC ) || defined( Q_OS_MAC )
  QDir::home().mkpath( "Library/Application Support/SimulationCraft" );
  AppDataDir = TmpDir = QDir::home().absoluteFilePath( "Library/Application Support/SimulationCraft" );
#endif
#ifdef SC_TO_INSTALL // GUI will be installed, use default AppData & Temp location for files created
#ifdef Q_OS_WIN32
  QDir::home().mkpath( "SimulationCraft" );
  AppDataDir = TmpDir = QDir::home().absoluteFilePath( "SimulationCraft" );
#endif
#endif

#if QT_VERSION_5
  QStringList s = QStandardPaths::standardLocations( QStandardPaths::CacheLocation );
  if ( ! s.empty() )
  {
    QDir a( s.first() );
    if ( a.isReadable() )
      TmpDir = s.first();
  }
  else
  {
    s = QStandardPaths::standardLocations( QStandardPaths::TempLocation );
    if ( ! s.empty() )
    {
      QDir a( s.first() );
      if ( a.isReadable() )
        TmpDir = s.first();
    }
  }
#endif

#ifdef SC_TO_INSTALL // GUI will be installed, use default AppData location for files created
  #if QT_VERSION_5
    QStringList z = QStandardPaths::standardLocations( QStandardPaths::DataLocation );
    if ( ! z.empty() )
    {
      QDir a( z.first() );
      if ( a.isReadable() )
        AppDataDir = z.first();
    }
  #endif
#endif

  logFileText =  AppDataDir + "/" + "log.txt";
  resultsFileText =  AppDataDir + "/" + "results.html";

  mainTab = new SC_MainTab( this );
  createWelcomeTab();
  createOptionsTab();
  createImportTab();
  createSimulateTab();
  createOverridesTab();
  createHelpTab();
  createLogTab();
  createResultsTab();
  createSiteTab();
  createCmdLine();
  createToolTips();
#ifdef SC_PAPERDOLL
  createPaperdoll();
#endif

  connect( mainTab, SIGNAL( currentChanged( int ) ), this, SLOT( mainTabChanged( int ) ) );

  QVBoxLayout* vLayout = new QVBoxLayout();
  vLayout -> addWidget( mainTab );
  vLayout -> addWidget( cmdLineGroupBox );
  setLayout( vLayout );

  timer = new QTimer( this );
  connect( timer, SIGNAL( timeout() ), this, SLOT( updateSimProgress() ) );

  importThread = new ImportThread( this );
  connect(   importThread, SIGNAL( finished() ), this, SLOT(  importFinished() ) );

  simulateThread = new SimulateThread( this );
  connect( simulateThread, SIGNAL( simulationFinished( sim_t* ) ), this, SLOT( simulateFinished( sim_t* ) ) );

#ifdef SC_PAPERDOLL
  paperdollThread = new PaperdollThread( this );
  connect( paperdollThread, SIGNAL( finished() ), this, SLOT( paperdollFinished() ) );

  connect( paperdollProfile,    SIGNAL( profileChanged() ), this, SLOT( start_intermediate_paperdoll_sim() ) );
  connect( optionsTab,          SIGNAL( optionsChanged() ), this, SLOT( start_intermediate_paperdoll_sim() ) );
#endif

  setAcceptDrops( true );

  loadHistory();
}

void SC_MainWindow::createCmdLine()
{
  QHBoxLayout* cmdLineLayout = new QHBoxLayout();
  cmdLineLayout -> addWidget( backButton = new QPushButton( "<" ) );
  cmdLineLayout -> addWidget( forwardButton = new QPushButton( ">" ) );
  cmdLineLayout -> addWidget( cmdLine = new SC_CommandLine( this ) );
  cmdLineLayout -> addWidget( progressBar = new QProgressBar() );
  cmdLineLayout -> addWidget( mainButton = new QPushButton( "Simulate!" ) );
  backButton -> setMaximumWidth( 30 );
  forwardButton -> setMaximumWidth( 30 );
  progressBar -> setMaximum( 100 );
  progressBar -> setMaximumWidth( 200 );
  progressBar -> setMinimumWidth( 150 );
  QFont progfont( progressBar -> font() );
  progfont.setPointSize( 11 );
  progressBar -> setFont( progfont );
  connect( backButton,    SIGNAL( clicked( bool ) ),   this, SLOT(    backButtonClicked() ) );
  connect( forwardButton, SIGNAL( clicked( bool ) ),   this, SLOT( forwardButtonClicked() ) );
  connect( mainButton,    SIGNAL( clicked( bool ) ),   this, SLOT(    mainButtonClicked() ) );
  connect( cmdLine,       SIGNAL( returnPressed() ),            this, SLOT( cmdLineReturnPressed() ) );
  connect( cmdLine,       SIGNAL( textEdited( const QString& ) ), this, SLOT( cmdLineTextEdited( const QString& ) ) );
  cmdLineGroupBox = new QGroupBox();
  cmdLineGroupBox -> setLayout( cmdLineLayout );
}

void SC_MainWindow::createWelcomeTab()
{
  mainTab -> addTab( new SC_WelcomeTabWidget( this ), tr( "Welcome" ) );
}

void SC_MainWindow::createOptionsTab()
{
  optionsTab = new SC_OptionsTab( this );
  mainTab -> addTab( optionsTab, tr( "Options" ) );

  connect( optionsTab, SIGNAL( armory_region_changed( const QString& ) ), this, SLOT( armoryRegionChanged( const QString& ) ) );
}

SC_WelcomeTabWidget::SC_WelcomeTabWidget( SC_MainWindow* parent ) :
  QWebView( parent )
{
  QString welcomeFile = QDir::currentPath() + "/Welcome.html";

#if defined( Q_WS_MAC ) || defined( Q_OS_MAC )
  CFURLRef fileRef    = CFBundleCopyResourceURL( CFBundleGetMainBundle(), CFSTR( "Welcome" ), CFSTR( "html" ), 0 );
  if ( fileRef )
  {
    CFStringRef macPath = CFURLCopyFileSystemPath( fileRef, kCFURLPOSIXPathStyle );
    welcomeFile         = CFStringGetCStringPtr( macPath, CFStringGetSystemEncoding() );

    CFRelease( fileRef );
    CFRelease( macPath );
  }
#endif
  setUrl( "file:///" + welcomeFile );
}

void SC_MainWindow::createImportTab()
{
  importTab = new SC_ImportTab( this );
  mainTab -> addTab( importTab, tr( "Import" ) );

  battleNetView = new SC_WebView( this );
  battleNetView -> setUrl( QUrl( "http://us.battle.net/wow/en" ) );
  importTab -> addTab( battleNetView, tr( "Battle.Net" ) );

  charDevCookies = new PersistentCookieJar( ( AppDataDir + QDir::separator() + "chardev.cookies" ).toStdString().c_str() );
  charDevCookies -> load();
  charDevView = new SC_WebView( this );
  charDevView -> page() -> networkAccessManager() -> setCookieJar( charDevCookies );
  charDevView -> setUrl( QUrl( "http://chardev.org/?planner" ) );
  importTab -> addTab( charDevView, tr( "CharDev" ) );

  createRawrTab();
  createBestInSlotTab();

  historyList = new QListWidget();
  historyList -> setSortingEnabled( true );
  importTab -> addTab( historyList, tr( "History" ) );

  connect( rawrButton,  SIGNAL( clicked( bool ) ),                       this, SLOT( rawrButtonClicked() ) );
  connect( historyList, SIGNAL( itemDoubleClicked( QListWidgetItem* ) ), this, SLOT( historyDoubleClicked( QListWidgetItem* ) ) );
  connect( importTab,   SIGNAL( currentChanged( int ) ),                 this, SLOT( importTabChanged( int ) ) );

  // Commenting out until it is more fleshed out.
  // createCustomTab();
}

void SC_MainWindow::createRawrTab()
{
  QVBoxLayout* rawrLayout = new QVBoxLayout();
  QLabel* rawrLabel = new QLabel( QString( " http://rawr.codeplex.com\n\n" ) +
                                  tr(  "Rawr is an exceptional theorycrafting tool that excels at gear optimization."
                                       " The key architectural difference between Rawr and SimulationCraft is one of"
                                       " formulation vs simulation.\nThere are strengths and weaknesses to each"
                                       " approach.  Since they come from different directions, one can be confident"
                                       " in the result when they arrive at the same destination.\n\n"
                                       " To aid comparison, SimulationCraft can import the character xml file written by Rawr.\n\n"
                                       " Alternatively, paste xml from the Rawr in-game addon into the space below." ) );
  rawrLabel -> setWordWrap( true );
  rawrLayout -> addWidget( rawrLabel );
  rawrLayout -> addWidget( rawrButton = new QPushButton( tr( "Load Rawr XML" ) ) );
  rawrLayout -> addWidget( rawrText = new SC_PlainTextEdit( this ), 1 );
  QGroupBox* rawrGroupBox = new QGroupBox();
  rawrGroupBox -> setLayout( rawrLayout );
  importTab -> addTab( rawrGroupBox, "Rawr" );
}

void SC_MainWindow::createBestInSlotTab()
{
  // Create BiS Tree ( table with profiles )
  QStringList headerLabels( tr( "Player Class" ) ); headerLabels += QString( tr( "Location" ) );

  QTreeWidget* bisTree = new QTreeWidget();
  bisTree -> setColumnCount( 1 );
  bisTree -> setHeaderLabels( headerLabels );

  const int TIER_MAX = 3;
#if SC_BETA == 1
  const char* tierNames[] = { "" }; // For the beta include ALL profiles
#else
  const char* tierNames[] = { "T14", "T15", "T16" };
#endif
  QTreeWidgetItem* playerItems[ PLAYER_MAX ];
  range::fill( playerItems, 0 );
  QTreeWidgetItem* rootItems[ PLAYER_MAX ][ TIER_MAX ];
  for ( player_e i = DEATH_KNIGHT; i <= WARRIOR; i++ )
  {
    range::fill( rootItems[ i ], 0 );
  }

// Scan all subfolders in /profiles/ and create a list
#if ! defined( Q_WS_MAC ) && ! defined( Q_OS_MAC )
  QDir tdir = QString( "profiles" );
#else
  CFURLRef fileRef    = CFBundleCopyResourceURL( CFBundleGetMainBundle(), CFSTR( "profiles" ), 0, 0 );
  QDir tdir;
  if ( fileRef )
  {
    CFStringRef macPath = CFURLCopyFileSystemPath( fileRef, kCFURLPOSIXPathStyle );
    tdir            = QString( CFStringGetCStringPtr( macPath, CFStringGetSystemEncoding() ) );

    CFRelease( fileRef );
    CFRelease( macPath );
  }
#endif
  tdir.setFilter( QDir::Dirs );


  QStringList tprofileList = tdir.entryList();
  int tnumProfiles = tprofileList.count();
  // Main loop through all subfolders of ./profiles/
  for ( int i = 0; i < tnumProfiles; i++ )
  {
#if ! defined( Q_WS_MAC ) && ! defined( Q_OS_MAC )
    QDir dir = QString( "profiles/" + tprofileList[ i ] );
#else
    CFURLRef fileRef = CFBundleCopyResourceURL( CFBundleGetMainBundle(),
                       CFStringCreateWithCString( NULL,
                           tprofileList[ i ].toUtf8().constData(),
                           kCFStringEncodingUTF8 ),
                       0,
                       CFSTR( "profiles" ) );
    QDir dir;
    if ( fileRef )
    {
      CFStringRef macPath = CFURLCopyFileSystemPath( fileRef, kCFURLPOSIXPathStyle );
      dir            = QString( CFStringGetCStringPtr( macPath, CFStringGetSystemEncoding() ) );

      CFRelease( fileRef );
      CFRelease( macPath );
    }
#endif
    dir.setSorting( QDir::Name );
    dir.setFilter( QDir::Files );
    dir.setNameFilters( QStringList( "*.simc" ) );

    QStringList profileList = dir.entryList();
    int numProfiles = profileList.count();
    for ( int i = 0; i < numProfiles; i++ )
    {
      QString profile = dir.absolutePath() + "/";
      profile = QDir::toNativeSeparators( profile );
      profile += profileList[ i ];

      player_e player = PLAYER_MAX;

      // Hack! For now...  Need to decide sim-wide just how the heck we want to refer to DKs.
      if ( profile.contains( "Death_Knight" ) )
        player = DEATH_KNIGHT;
      else
      {
        for ( player_e j = PLAYER_NONE; j < PLAYER_MAX; j++ )
        {
          if ( profile.contains( util::player_type_string( j ), Qt::CaseInsensitive ) )
          {
            player = j;
            break;
          }
        }
      }

      // exclude generate profiles
      if ( profile.contains( "generate" ) )
        continue;

      int tier = TIER_MAX;
      for ( int j = 0; j < TIER_MAX && tier == TIER_MAX; j++ )
        if ( profile.contains( tierNames[ j ] ) )
          tier = j;

      if ( player != PLAYER_MAX && tier != TIER_MAX )
      {
        if ( !rootItems[ player ][ tier ] )
        {
          if ( !playerItems[ player ] )
          {
            QTreeWidgetItem* top = new QTreeWidgetItem( QStringList( util::inverse_tokenize( util::player_type_string( player ) ).c_str() ) );
            playerItems[ player ] = top;
          }

          if ( !rootItems[ player ][ tier ] )
          {
            QTreeWidgetItem* tieritem = new QTreeWidgetItem( QStringList( tierNames[ tier ] ) );
            playerItems[ player ] -> addChild( rootItems[ player ][ tier ] =  tieritem );
          }
        }

        QTreeWidgetItem* item = new QTreeWidgetItem( QStringList() << profileList[ i ] << profile );
        rootItems[ player ][ tier ] -> addChild( item );
      }
    }
  }

  // Register all the added profiles ( done here so they show up alphabetically )
  for ( player_e i = DEATH_KNIGHT; i <= WARRIOR; i++ )
  {
    if ( playerItems[ i ] )
    {
      bisTree -> addTopLevelItem( playerItems[ i ] );
      for ( int j = 0; j < TIER_MAX; j++ )
      {
        if ( rootItems[ i ][ j ] )
        {
          rootItems[ i ][ j ] -> setExpanded( true ); // Expand the subclass Tier bullets by default
          rootItems[ i ][ j ] -> sortChildren( 0, Qt::AscendingOrder );
        }
      }
    }
  }

  bisTree -> setColumnWidth( 0, 300 );

  connect( bisTree, SIGNAL( itemDoubleClicked( QTreeWidgetItem*, int ) ), this, SLOT( bisDoubleClicked( QTreeWidgetItem*, int ) ) );

  // Create BiS Introduction

  QFormLayout* bisIntroductionFormLayout = new QFormLayout();
  QLabel* bisText = new QLabel( tr( "These sample profiles are attempts at creating the best possible gear, talent, glyph and action priority list setups to achieve the highest possible average damage per second.\n"
                                    "The profiles are created with a lot of help from the theorycrafting community.\n"
                                    "They are only as good as the thorough testing done on them, and the feedback and critic we receive from the community, including yourself.\n"
                                    "If you have ideas for improvements, try to simulate them. If they result in increased dps, please open a ticket on our Issue tracker.\n"
                                    "The more people help improve BiS profiles, the better will they reach their goal of representing the highest possible dps." ) );
  bisIntroductionFormLayout -> addRow( bisText );

  QWidget* bisIntroduction = new QWidget();
  bisIntroduction -> setLayout( bisIntroductionFormLayout );

  // Create BiS Tab ( Introduction + BiS Tree )

  QVBoxLayout* bisTabLayout = new QVBoxLayout();
  bisTabLayout -> addWidget( bisIntroduction, 1 );
  bisTabLayout -> addWidget( bisTree, 9 );

  QGroupBox* bisTab = new QGroupBox();
  bisTab -> setLayout( bisTabLayout );
  importTab -> addTab( bisTab, tr( "Sample Profiles" ) );

}

void SC_MainWindow::createCustomTab()
{
  //In Dev - Character Retrieval Boxes & Buttons
  //In Dev - Load & Save Profile Buttons
  //In Dev - Profiler Slots, Talent & Glyph Layout
  QHBoxLayout* customLayout = new QHBoxLayout();
  QGroupBox* customGroupBox = new QGroupBox();
  customGroupBox -> setLayout( customLayout );
  importTab -> addTab( customGroupBox, "Custom Profile" );
  customLayout -> addWidget( createCustomCharData = new QGroupBox( tr( "Character Data" ) ), 1 );
  createCustomCharData -> setObjectName( QString::fromUtf8( "createCustomCharData" ) );
  customLayout -> addWidget( createCustomProfileDock = new QTabWidget(), 1 );
  createCustomProfileDock -> setObjectName( QString::fromUtf8( "createCustomProfileDock" ) );
  createCustomProfileDock -> setAcceptDrops( true );
  customGearTab = new QWidget();
  customGearTab -> setObjectName( QString::fromUtf8( "customGearTab" ) );
  createCustomProfileDock -> addTab( customGearTab, QString() );
  customTalentsTab = new QWidget();
  customTalentsTab -> setObjectName( QString::fromUtf8( "customTalentsTab" ) );
  createCustomProfileDock -> addTab( customTalentsTab, QString() );
  customGlyphsTab = new QWidget();
  customGlyphsTab -> setObjectName( QString::fromUtf8( "customGlyphsTab" ) );
  createCustomProfileDock -> addTab( customGlyphsTab, QString() );
  createCustomProfileDock -> setTabText( createCustomProfileDock -> indexOf( customGearTab ), tr( "Gear", "createCustomTab" ) );
  createCustomProfileDock -> setTabToolTip( createCustomProfileDock -> indexOf( customGearTab ), tr( "Customise Gear Setup", "createCustomTab" ) );
  createCustomProfileDock -> setTabText( createCustomProfileDock -> indexOf( customTalentsTab ), tr( "Talents", "createCustomTab" ) );
  createCustomProfileDock -> setTabToolTip( createCustomProfileDock -> indexOf( customTalentsTab ), tr( "Customise Talents", "createCustomTab" ) );
  createCustomProfileDock -> setTabText( createCustomProfileDock -> indexOf( customGlyphsTab ), tr( "Glyphs", "createCustomTab" ) );
  createCustomProfileDock -> setTabToolTip( createCustomProfileDock -> indexOf( customGlyphsTab ), tr( "Customise Glyphs", "createCustomTab" ) );
}

void SC_MainWindow::createSimulateTab()
{
  simulateTab = new SC_SimulateTab( mainTab );
  simulateTab -> setTabsClosable( true );
  simulateTab -> setMovable( true );
  simulateTab -> add_Text( defaultSimulateText, "Simulate!" );
  simulateTab -> current_Text() -> edited_by_user = false;

  connect( simulateTab, SIGNAL( tabCloseRequested( int ) ), this, SLOT( simulateTabCloseRequest( int ) ) );

  mainTab -> addTab( simulateTab, tr( "Simulate" ) );

}

void SC_MainWindow::createOverridesTab()
{
  overridesText = new SC_PlainTextEdit( this );
  overridesText -> setPlainText( "# User-specified persistent global and player parms will set here.\n" );

  // Set a bigger font size, it's not like people put much into the override tab
  QFont override_font = QFont();
  override_font.setPixelSize( 24 );
  overridesText -> setFont ( override_font );

  mainTab -> addTab( overridesText, tr( "Overrides" ) );
}

void SC_MainWindow::createLogTab()
{
  logText = new SC_PlainTextEdit(  this, false );
  //logText -> document() -> setDefaultFont( QFont( "fixed" ) );
  logText -> setReadOnly( true );
  logText -> setPlainText( "Look here for error messages and simple text-only reporting.\n" );
  mainTab -> addTab( logText, tr( "Log" ) );
}

void SC_MainWindow::createHelpTab()
{
  helpView = new SC_WebView( this );
  helpView -> setUrl( QUrl( "http://code.google.com/p/simulationcraft/wiki/StartersGuide" ) );
  mainTab -> addTab( helpView, tr( "Help" ) );
}

void SC_MainWindow::createResultsTab()
{

  resultsTab = new QTabWidget();
  resultsTab -> setTabsClosable( true );
  connect( resultsTab, SIGNAL( currentChanged( int ) ),    this, SLOT( resultsTabChanged( int ) )      );
  connect( resultsTab, SIGNAL( tabCloseRequested( int ) ), this, SLOT( resultsTabCloseRequest( int ) ) );
  mainTab -> addTab( resultsTab, tr( "Results" ) );
}

void SC_MainWindow::createSiteTab()
{
  siteView = new SC_WebView( this );
  siteView -> setUrl( QUrl( "http://code.google.com/p/simulationcraft/" ) );
  mainTab -> addTab( siteView, tr( "Site" ) );
}

void SC_MainWindow::createToolTips()
{

  backButton -> setToolTip( tr( "Backwards" ) );
  forwardButton -> setToolTip( tr( "Forwards" ) );

  optionsTab -> createToolTips();
}

#ifdef SC_PAPERDOLL
void SC_MainWindow::createPaperdoll()
{
  QWidget* paperdollTab = new QWidget( this );
  QHBoxLayout* paperdollMainLayout = new QHBoxLayout();
  paperdollMainLayout -> setAlignment( Qt::AlignLeft | Qt::AlignTop );
  paperdollTab -> setLayout( paperdollMainLayout );

  paperdollProfile = new PaperdollProfile();
  paperdoll = new Paperdoll( this, paperdollProfile, paperdollTab );
  ItemSelectionWidget* items = new ItemSelectionWidget( paperdollProfile, paperdollTab );

  paperdollMainLayout -> addWidget( items );
  paperdollMainLayout -> addWidget( paperdoll );


  mainTab -> addTab( paperdollTab, "Paperdoll" );
}
#endif

void SC_MainWindow::updateVisibleWebView( SC_WebView* wv )
{
  assert( wv );
  visibleWebView = wv;
  progressBar -> setFormat( "%p%" );
  progressBar -> setValue( visibleWebView -> progress );
  cmdLine -> setText( visibleWebView -> url().toString() );
}

// ==========================================================================
// Sim Initialization
// ==========================================================================

sim_t* SC_MainWindow::initSim()
{
  sim_t* sim = new sim_t();
  sim -> output_file = fopen( ( AppDataDir.toStdString() + "/" + SIMC_LOG_FILE ).c_str(), "w" );
  sim -> report_progress = 0;
#if SC_USE_PTR
  sim -> parse_option( "ptr", ( ( optionsTab -> choice.version -> currentIndex() == 1 ) ? "1" : "0" ) );
#endif
  sim -> parse_option( "debug", ( ( optionsTab -> choice.debug -> currentIndex() == 2 ) ? "1" : "0" ) );
  return sim;
}

void SC_MainWindow::deleteSim( sim_t* sim, SC_PlainTextEdit* append_error_message )
{
  if ( sim )
  {
    fclose( sim -> output_file );
    delete sim;

    QString contents;
    QFile logFile( AppDataDir + "/" + SIMC_LOG_FILE );
    if ( logFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
      contents = QString::fromUtf8( logFile.readAll() );
      logFile.close();
    }

    if ( ! simulateThread -> success )
      logText -> setformat_error();

    // If requested, append the error message to the given Text Widget as well.
    if ( append_error_message )
      append_error_message -> appendPlainText( contents );

    logText -> appendPlainText( contents );
    logText -> moveCursor( QTextCursor::End );
    logText -> resetformat();
  }
}

void SC_MainWindow::startImport( int tab, const QString& url )
{
  if ( sim )
  {
    sim  ->  cancel();
    return;
  }
  simProgress = 0;
  mainButton -> setText( "Cancel!" );
  sim = initSim();
  importThread -> start( sim, tab, url, optionsTab -> get_db_order(), optionsTab -> get_active_spec(), optionsTab -> get_player_role() );
  simulateTab -> add_Text( defaultSimulateText, QString() );
  mainTab -> setCurrentTab( TAB_SIMULATE );
  timer -> start( 500 );
}

void SC_MainWindow::importFinished()
{
  timer -> stop();
  simPhase = "%p%";
  simProgress = 100;
  progressBar -> setFormat( simPhase.c_str() );
  progressBar -> setValue( simProgress );
  if ( importThread -> player )
  {
    simulateTab -> set_Text( importThread -> profile );
    simulateTab->setTabText( simulateTab -> currentIndex(), QString::fromUtf8( importThread -> player -> name_str.c_str() ) );

    QString label = QString::fromUtf8( importThread -> player -> name_str.c_str() );
    while ( label.size() < 20 ) label += ' ';
    label += QString::fromUtf8( importThread -> player -> origin_str.c_str() );

    bool found = false;
    for ( int i = 0; i < historyList -> count() && ! found; i++ )
      if ( historyList -> item( i ) -> text() == label )
        found = true;

    if ( ! found )
    {
      QListWidgetItem* item = new QListWidgetItem( label );
      //item -> setFont( QFont( "fixed" ) );

      historyList -> addItem( item );
      historyList -> sortItems();
    }

    deleteSim( sim ); sim = 0;
  }
  else
  {
    simulateTab -> current_Text() -> setformat_error(); // Print error message in big letters

    simulateTab -> append_Text( "# Unable to generate profile from: " + importThread -> url + "\n" );

    deleteSim( sim, simulateTab -> current_Text() ); sim = 0;

    simulateTab -> current_Text() -> resetformat(); // Reset font
  }

  mainButton -> setText( "Simulate!" );
  mainTab -> setCurrentTab( TAB_SIMULATE );
}

void SC_MainWindow::startSim()
{
  if ( sim )
  {
    sim -> cancel();
    return;
  }
  optionsHistory.add( optionsTab -> encodeOptions() );
  optionsHistory.current_index = 0;
  if ( simulateTab -> current_Text() -> toPlainText() != defaultSimulateText )
  {
    //simulateTextHistory.add( simulateText -> toPlainText() );
  }
  overridesTextHistory.add( overridesText -> toPlainText() );
  simulateCmdLineHistory.add( cmdLine -> text() );
  simProgress = 0;
  mainButton -> setText( "Cancel!" );
  sim = initSim();
  simulateThread -> start( sim, optionsTab -> mergeOptions() );
  // simulateText -> setPlainText( defaultSimulateText() );
  cmdLineText = "";
  cmdLine -> setText( cmdLineText );
  timer -> start( 100 );
}

#ifdef SC_PAPERDOLL

player_t* SC_MainWindow::init_paperdoll_sim( sim_t*& sim )
{

  sim = initSim();

  PaperdollProfile* profile = paperdollProfile;
  const module_t* module = module_t::get( profile -> currentClass() );
  player_t* player = module ? module -> create_player( sim, "Paperdoll Player", profile -> currentRace() ) : NULL;

  if ( player )
  {
    player -> role = util::parse_role_type( optionsTab -> choice.default_role -> currentText().toUtf8().constData() );

    player -> _spec = profile -> currentSpec();

    profession_e p1 = profile -> currentProfession( 0 );
    profession_e p2 = profile -> currentProfession( 1 );
    player -> professions_str = std::string();
    if ( p1 != PROFESSION_NONE )
      player -> professions_str += std::string( util::profession_type_string( p1 ) );
    if ( p1 != PROFESSION_NONE && p2 != PROFESSION_NONE )
      player -> professions_str += "/";
    if ( p2 != PROFESSION_NONE )
      player -> professions_str += util::profession_type_string( p2 );

    for ( slot_e i = SLOT_MIN; i < SLOT_MAX; i++ )
    {
      const item_data_t* profile_item = profile -> slotItem( i );

      if ( profile_item )
      {
        player -> items.push_back( item_t( player, std::string() ) );
        item_t& item = player -> items.back();
        item.options_str += "id=" + util::to_string( profile_item -> id );
      }
    }
  }
  return player;
}

void SC_MainWindow::start_intermediate_paperdoll_sim()
{
  if ( paperdoll_sim )
  {
    paperdoll_sim -> cancel();
    paperdollThread -> wait( 100 );
    deleteSim( paperdoll_sim );
  }

  player_t* player = init_paperdoll_sim( paperdoll_sim );

  if ( player )
  {
    paperdoll -> setCurrentDPS( "", 0, 0 );

    paperdollThread -> start( paperdoll_sim, player, optionsTab -> get_globalSettings() );

    timer -> start( 100 );
    simProgress = 0;
  }
}

void SC_MainWindow::start_paperdoll_sim()
{
  if ( sim )
  {
    return;
  }
  if ( paperdoll_sim )
  {
    paperdoll_sim -> cancel();
    paperdollThread -> wait( 1000 );
    deleteSim( paperdoll_sim ); paperdoll_sim = 0;
  }

  player_t* player = init_paperdoll_sim( sim );

  if ( player )
  {


    optionsHistory.add( optionsTab -> encodeOptions() );
    optionsHistory.current_index = 0;
    if ( simulateTab -> current_Text() -> toPlainText() != defaultSimulateText )
    {
      //simulateTextHistory.add( simulateText -> toPlainText() );
    }
    overridesTextHistory.add( overridesText -> toPlainText() );
    simulateCmdLineHistory.add( cmdLine -> text() );
    simProgress = 0;
    mainButton -> setText( "Cancel!" );
    simulateThread -> start( sim, optionsTab -> get_globalSettings() );
    // simulateText -> setPlainText( defaultSimulateText() );
    cmdLineText = "";
    cmdLine -> setText( cmdLineText );
    timer -> start( 100 );
    simProgress = 0;
  }
}
#endif

void SC_MainWindow::simulateFinished( sim_t* sim )
{
  timer -> stop();
  simPhase = "%p%";
  simProgress = 100;
  progressBar -> setFormat( simPhase.c_str() );
  progressBar -> setValue( simProgress );
  QFile file( sim -> html_file_str.c_str() );
  bool sim_was_debug = sim -> debug;
  deleteSim( sim ); SC_MainWindow::sim = 0;
  if ( ! simulateThread -> success )
  {
    logText -> setformat_error();
    logText -> appendPlainText( "Simulation failed!\n" );
    logText -> moveCursor( QTextCursor::End );
    logText -> resetformat();
    mainTab -> setCurrentTab( TAB_LOG );
  }
  else if ( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
  {
    QString resultsName = QString( "Results %1" ).arg( ++simResults );
    SC_WebView* resultsView = new SC_WebView( this, resultsTab, QString::fromUtf8( file.readAll() ) );
    resultsView -> loadHtml();
    resultsTab -> addTab( resultsView, resultsName );
    resultsTab -> setCurrentWidget( resultsView );
    resultsView->setFocus();
    mainTab -> setCurrentTab( sim_was_debug ? TAB_LOG : TAB_RESULTS );
  }
  else
  {
    logText -> setformat_error();
    logText -> appendPlainText( "Unable to open html report!\n" );
    logText -> resetformat();
    mainTab -> setCurrentTab( TAB_LOG );
  }
}

#ifdef SC_PAPERDOLL
void SC_MainWindow::paperdollFinished()
{
  timer -> stop();
  simPhase = "%p%";
  simProgress = 100;
  progressBar -> setFormat( simPhase.c_str() );
  progressBar -> setValue( simProgress );

  simProgress = 100;

  timer -> start( 100 );
}
#endif

// ==========================================================================
// Save Results
// ==========================================================================

void SC_MainWindow::saveLog()
{
  logCmdLineHistory.add( cmdLine->text() );

  QFile file( cmdLine->text() );

  if ( file.open( QIODevice::WriteOnly | QIODevice::Text ) )
  {
    file.write( logText -> toPlainText().toUtf8() );
    file.close();
  }

  logText->appendPlainText( QString( "Log saved to: %1\n" ).arg( cmdLine->text() ) );
}

void SC_MainWindow::saveResults()
{
  int index = resultsTab -> currentIndex();
  if ( index < 0 ) return;

  resultsCmdLineHistory.add( cmdLine -> text() );

  QFile file( cmdLine -> text() );

  if ( file.open( QIODevice::WriteOnly | QIODevice::Text ) )
  {
    file.write( visibleWebView -> html_str.toUtf8() );
    file.close();
  }

  logText -> appendPlainText( QString( "Results saved to: %1\n" ).arg( cmdLine->text() ) );
}

// ==========================================================================
// Window Events
// ==========================================================================

void SC_MainWindow::closeEvent( QCloseEvent* e )
{
  saveHistory();
  battleNetView -> stop();
  QCoreApplication::quit();
  e -> accept();
}

void SC_MainWindow::cmdLineTextEdited( const QString& s )
{
  switch ( mainTab -> currentTab() )
  {
    case TAB_WELCOME:   cmdLineText = s; break;
    case TAB_OPTIONS:   cmdLineText = s; break;
    case TAB_SIMULATE:  cmdLineText = s; break;
    case TAB_OVERRIDES: cmdLineText = s; break;
    case TAB_HELP:      cmdLineText = s; break;
    case TAB_SITE:      cmdLineText = s; break;
    case TAB_LOG:       logFileText = s; break;
    case TAB_RESULTS:   resultsFileText = s; break;
    default:  break;
  }
}

void SC_MainWindow::cmdLineReturnPressed()
{
  if ( mainTab -> currentTab() == TAB_IMPORT )
  {
    if ( cmdLine->text().count( "battle.net" ) ||
         cmdLine->text().count( "battlenet.com" ) )
    {
      battleNetView -> setUrl( QUrl::fromUserInput( cmdLine -> text() ) );
      importTab -> setCurrentTab( TAB_BATTLE_NET );
    }
    else if ( cmdLine->text().count( "chardev.org" ) )
    {
      charDevView -> setUrl( QUrl::fromUserInput( cmdLine -> text() ) );
      importTab -> setCurrentTab( TAB_CHAR_DEV );
    }
    else
    {
      if ( ! sim ) mainButtonClicked( true );
    }
  }
  else
  {
    if ( ! sim ) mainButtonClicked( true );
  }
}

void SC_MainWindow::mainButtonClicked( bool /* checked */ )
{
  switch ( mainTab -> currentTab() )
  {
    case TAB_WELCOME:   startSim(); break;
    case TAB_OPTIONS:   startSim(); break;
    case TAB_SIMULATE:  startSim(); break;
    case TAB_OVERRIDES: startSim(); break;
    case TAB_HELP:      startSim(); break;
    case TAB_SITE:      startSim(); break;
    case TAB_IMPORT:
      switch ( importTab -> currentTab() )
      {
        case TAB_BATTLE_NET: startImport( TAB_BATTLE_NET, cmdLine->text() ); break;
        case TAB_CHAR_DEV:   startImport( TAB_CHAR_DEV,   cmdLine->text() ); break;
        case TAB_RAWR:       startImport( TAB_RAWR,       "Rawr XML"      ); break;
        default: break;
      }
      break;
    case TAB_LOG: saveLog(); break;
    case TAB_RESULTS: saveResults(); break;
#ifdef SC_PAPERDOLL
    case TAB_PAPERDOLL: start_paperdoll_sim(); break;
#endif
  }
}

void SC_MainWindow::backButtonClicked( bool /* checked */ )
{
  if ( visibleWebView )
  {
    if ( mainTab -> currentTab() == TAB_RESULTS && ! visibleWebView->history()->canGoBack() )
    {
//        visibleWebView->setHtml( resultsHtml[ resultsTab->indexOf( visibleWebView ) ] );
      visibleWebView -> loadHtml();

      QWebHistory* h = visibleWebView->history();
      visibleWebView->history()->clear(); // This is not appearing to work.
      h->setMaximumItemCount( 0 );
      h->setMaximumItemCount( 100 );
    }
    else
    {
      visibleWebView->back();
    }
    visibleWebView->setFocus();
  }
  else
  {
    switch ( mainTab -> currentTab() )
    {
      case TAB_OPTIONS:   optionsTab -> decodeOptions( optionsHistory.backwards() ); break;
      case TAB_OVERRIDES: overridesText->setPlainText( overridesTextHistory.backwards() ); overridesText->setFocus(); break;
      default:            break;
    }
  }
}

void SC_MainWindow::forwardButtonClicked( bool /* checked */ )
{
  if ( visibleWebView )
  {
    visibleWebView->forward();
    visibleWebView->setFocus();
  }
  else
  {
    switch ( mainTab->currentIndex() )
    {
      case TAB_WELCOME:   break;
      case TAB_OPTIONS:   optionsTab -> decodeOptions( optionsHistory.forwards() ); break;
      case TAB_IMPORT:    break;
      case TAB_OVERRIDES: overridesText->setPlainText( overridesTextHistory.forwards() ); overridesText->setFocus(); break;
      case TAB_LOG:       break;
      case TAB_RESULTS:   break;
    }
  }
}

void SC_MainWindow::rawrButtonClicked( bool /* checked */ )
{
  QFileDialog dialog( this );
  dialog.setFileMode( QFileDialog::ExistingFile );
  dialog.setNameFilter( "Rawr Profiles (*.xml)" );
  dialog.restoreState( rawrDialogState );
  if ( dialog.exec() )
  {
    rawrDialogState = dialog.saveState();
    QStringList fileList = dialog.selectedFiles();
    if ( ! fileList.empty() )
    {
      QFile rawrFile( fileList.at( 0 ) );
      if ( rawrFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
      {
        QTextStream in( &rawrFile );
        in.setCodec( "UTF-8" );
        in.setAutoDetectUnicode( true );
        rawrText->setPlainText( in.readAll() );
        rawrText->moveCursor( QTextCursor::Start );
      }
    }
  }
}

void SC_MainWindow::mainTabChanged( int index )
{
  visibleWebView = 0;
  switch ( index )
  {
    case TAB_WELCOME:
    case TAB_OPTIONS:
    case TAB_SIMULATE:
    case TAB_OVERRIDES:
#ifdef SC_PAPERDOLL
    case TAB_PAPERDOLL:
#endif
    case TAB_HELP:      cmdLine->setText( cmdLineText ); mainButton->setText( sim ? "Cancel!" : "Simulate!" ); break;
    case TAB_LOG:       cmdLine->setText( logFileText ); mainButton->setText( "Save!" ); break;
    case TAB_IMPORT:
      mainButton->setText( sim ? "Cancel!" : "Import!" );
      importTabChanged( importTab->currentIndex() );
      break;
    case TAB_RESULTS:
      cmdLine -> setText( resultsFileText ); mainButton -> setText( "Save!" );
      resultsTabChanged( resultsTab -> currentIndex() );
      break;
    case TAB_SITE:
      cmdLine->setText( cmdLineText );
      mainButton->setText( sim ? "Cancel!" : "Simulate!" );
      updateVisibleWebView( siteView );
      break;
    default: assert( 0 );
  }
  if ( visibleWebView )
  {
    progressBar->setFormat( "%p%" );
  }
  else
  {
    progressBar->setFormat( simPhase.c_str() );
    progressBar->setValue( simProgress );
  }
}

void SC_MainWindow::importTabChanged( int index )
{
  if ( index == TAB_RAWR    ||
       index == TAB_BIS     ||
       index == TAB_CUSTOM  ||
       index == TAB_HISTORY )
  {
    visibleWebView = 0;
    progressBar->setFormat( simPhase.c_str() );
    progressBar->setValue( simProgress );
    cmdLine->setText( "" );
  }
  else
  {
    updateVisibleWebView( debug_cast<SC_WebView*>( importTab -> widget( index ) ) );
  }
}

void SC_MainWindow::resultsTabChanged( int index )
{
  if ( resultsTab -> count() > 0 )
  {
    updateVisibleWebView( debug_cast<SC_WebView*>( resultsTab -> widget( index ) ) );
  }
  cmdLine -> setText( resultsFileText );
}

void SC_MainWindow::resultsTabCloseRequest( int index )
{
  resultsTab -> removeTab( index );
}

void SC_MainWindow::simulateTabCloseRequest( int index )
{
  if ( simulateTab -> tabText( index ) == "Simulate!" )
  {
    // Ignore attempts to close Legend
  }
  else
  {
    simulateTab -> removeTab( index );
  }
}

void SC_MainWindow::historyDoubleClicked( QListWidgetItem* item )
{
  QString text = item -> text();
  QString url = text.section( ' ', 1, 1, QString::SectionSkipEmpty );

  if ( url.count( "battle.net" ) || url.count( "battlenet.com" ) )
  {
    battleNetView -> setUrl( url );
    importTab -> setCurrentIndex( TAB_BATTLE_NET );
  }
  else if ( url.count( "chardev.org" ) )
  {
    charDevView -> setUrl( url );
    importTab -> setCurrentIndex( TAB_CHAR_DEV );
  }
  else
  {
    //importTab->setCurrentIndex( TAB_RAWR );
  }
}

void SC_MainWindow::bisDoubleClicked( QTreeWidgetItem* item, int /* col */ )
{
  QString profile = item -> text( 1 );
  QString s = "Unable to import profile " + profile;

  QFile file( profile );
  if ( file.open( QIODevice::ReadOnly | QIODevice::Text ) )
  {
    s = QString::fromUtf8( file.readAll() );
    file.close();
  }
  simulateTab -> add_Text( s, item -> text( 0 ) );
  mainTab -> setCurrentTab( TAB_SIMULATE );
  simulateTab -> current_Text() -> setFocus();
}

void SC_MainWindow::armoryRegionChanged( const QString& region )
{
  battleNetView -> stop();
  battleNetView -> setUrl( "http://" + region + ".battle.net/wow/en" );
}

// ==========================================================================
// SimulateThread
// ==========================================================================

void SimulateThread::run()
{
  QByteArray utf8_profile = options.toUtf8();
  QFile file( mainWindow -> AppDataDir + QDir::separator () + "simc_gui.simc" );
  if ( file.open( QIODevice::WriteOnly | QIODevice::Text ) )
  {
    file.write( utf8_profile );
    file.close();
  }

  sim -> html_file_str = ( mainWindow -> AppDataDir + QDir::separator() + "simc_report.html" ).toStdString();
  sim -> xml_file_str  = ( mainWindow -> AppDataDir + QDir::separator() + "simc_report.xml" ).toStdString();
  sim -> reforge_plot_output_file_str = ( mainWindow -> AppDataDir + QDir::separator() + "reforge_plot.csv" ).toStdString();

  sim_control_t description;

  success = description.options.parse_text( utf8_profile.constData() );

  if ( success )
  {
    success = sim -> setup( &description );
  }
  if ( sim -> challenge_mode ) sim -> scale_to_itemlevel = 463;

  if ( success )
  {
    success = sim -> execute();
  }
  if ( success )
  {
    sim -> scaling -> analyze();
    sim -> plot -> analyze();
    sim -> reforge_plot -> analyze();
    report::print_suite( sim );
  }
}

// ============================================================================
// SC_CommandLine
// ============================================================================

void SC_CommandLine::keyPressEvent( QKeyEvent* e )
{
  int k = e -> key();
  if ( k != Qt::Key_Up && k != Qt::Key_Down )
  {
    QLineEdit::keyPressEvent( e );
    return;
  }
  switch ( mainWindow -> mainTab -> currentTab() )
  {
    case TAB_OVERRIDES:
      mainWindow->cmdLineText = mainWindow->simulateCmdLineHistory.next( k );
      setText( mainWindow->cmdLineText );
      break;
    case TAB_LOG:
      mainWindow->logFileText = mainWindow->logCmdLineHistory.next( k );
      setText( mainWindow->logFileText );
      break;
    case TAB_RESULTS:
      mainWindow -> resultsFileText = mainWindow-> resultsCmdLineHistory.next( k );
      setText( mainWindow -> resultsFileText );
      break;
    default: break;
  }
}

// ==========================================================================
// SC_ReforgeButtonGroup
// ==========================================================================

SC_ReforgeButtonGroup::SC_ReforgeButtonGroup( QObject* parent ) :
  QButtonGroup( parent ), selected( 0 )
{}

void SC_ReforgeButtonGroup::setSelected( int state )
{
  if ( state )
    selected++;
  else
    selected--;

  // Three selected, disallow selection of any more
  if ( selected >= 3 )
  {
    QList< QAbstractButton* > b = buttons();
    for ( QList< QAbstractButton* >::iterator i = b.begin(); i != b.end(); ++i )
      if ( ! ( *i ) -> isChecked() )
        ( *i ) -> setEnabled( false );
  }
  // Less than three selected, allow selection of all/any
  else
  {
    QList< QAbstractButton* > b = buttons();
    for ( QList< QAbstractButton* >::iterator i = b.begin(); i != b.end(); ++i )
      ( *i ) -> setEnabled( true );
  }
}

// ==========================================================================
// PersistentCookieJar
// ==========================================================================

void PersistentCookieJar::save()
{
  QFile file( fileName );
  if ( file.open( QIODevice::WriteOnly ) )
  {
    QDataStream out( &file );
    QList<QNetworkCookie> cookies = allCookies();
    qint32 count = ( qint32 ) cookies.count();
    out << count;
    for ( int i = 0; i < count; i++ )
    {
      const QNetworkCookie& c = cookies.at( i );
      out << c.name();
      out << c.value();
    }
    file.close();
  }
}

void PersistentCookieJar::load()
{
  QFile file( fileName );
  if ( file.open( QIODevice::ReadOnly ) )
  {
    QDataStream in( &file );
    QList<QNetworkCookie> cookies;
    qint32 count;
    in >> count;
    for ( int i = 0; i < count; i++ )
    {
      QByteArray name, value;
      in >> name;
      in >> value;
      cookies.append( QNetworkCookie( name, value ) );
    }
    setAllCookies( cookies );
    file.close();
  }
}

#ifdef SC_PAPERDOLL

void PaperdollThread::run()
{
  cache::advance_era();

  sim -> iterations = 100;

  QStringList stringList = options.split( '\n', QString::SkipEmptyParts );

  std::vector<std::string> args;
  for ( int i = 0; i < stringList.count(); ++i )
    args.push_back( stringList[ i ].toUtf8().constData() );
  sim_control_t description;

  success = description.options.parse_args( args );

  if ( success )
  {
    success = sim -> setup( &description );
  }
  if ( success )
  {
    for ( unsigned int i = 0; i < 10; ++i )
    {
      sim -> current_iteration = 0;
      sim -> execute();
      player_t::scales_over_t s = player -> scales_over();
      mainWindow -> paperdoll -> setCurrentDPS( s.name, s.value, s.stddev * sim -> confidence_estimator );
    }
  }
}
#endif
