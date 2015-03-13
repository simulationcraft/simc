// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "simulationcraftqt.hpp"
#include "SC_OptionsTab.hpp"
#include "SC_SpellQueryTab.hpp"
#include "sc_SimulationThread.hpp"
#include "sc_SampleProfilesTab.hpp"
#include "sc_AutomationTab.hpp"
#include "util/sc_mainwindowcommandline.hpp"
#ifdef SC_PAPERDOLL
#include "simcpaperdoll.hpp"
#endif
#if defined( Q_OS_MAC )
#include <CoreFoundation/CoreFoundation.h>
#endif
#include <QStandardPaths>

static int SC_GUI_HISTORY_VERSION = 650;

namespace { // UNNAMED NAMESPACE

#if ! defined( SC_USE_WEBKIT )
struct HtmlOutputFunctor
{
  QFile* file_;

  HtmlOutputFunctor( QFile* out_file ) : file_( out_file )
  { }

  void operator()( QString htmlOutput )
  {
    QByteArray out_utf8 = htmlOutput.toUtf8();
    file_ -> write( out_utf8.constData(), out_utf8.size() );
    file_ -> close();
  }
};
#endif
} // UNNAMED NAMESPACE

// ==========================================================================
// SC_PATHS
// ==========================================================================

QString SC_PATHS::getDataPath()
{
#if defined( Q_OS_WIN )
    return QCoreApplication::applicationDirPath();
#elif defined( Q_OS_MAC )
    return QCoreApplication::applicationDirPath() + "/../Resources";
#else
    QString shared_path;
    QStringList appdatalocation =  QStandardPaths::standardLocations( QStandardPaths::DataLocation );
    for( int i = 0; i < appdatalocation.size(); ++i )
    {
      QDir dir( appdatalocation[ i ]);
        if ( dir.exists() )
        {
          shared_path = dir.path();
            break;
        }
    }
    return shared_path;
#endif
}

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

  std::string progressBarToolTip;

  if ( simRunning() )
  {
    simProgress = static_cast<int>( 100.0 * sim -> progress( simPhase, &progressBarToolTip ) );
  }
  if ( importRunning() )
  {
    importSimProgress = static_cast<int>( 100.0 * import_sim -> progress( importSimPhase, &progressBarToolTip ) );
    if ( soloChar -> isActive() && importSimProgress == 0 )
    {
      soloimport += 2;
      importSimProgress = static_cast<int>( std::min( soloimport, 100 ) );
    }
  }
#if !defined( SC_WINDOWS ) && !defined( SC_OSX )
  // Progress bar text in Linux is displayed inside the progress bar as opposed next to it in Windows
  // so it does not look as bad to include iteration details in it
  if ( simPhase.find( ": " ) == std::string::npos ) // can end up with Simulating: : : : : : : in rare circumstances
  {
    simPhase += ": ";
    simPhase += progressBarToolTip;
  }
#endif

  cmdLine -> setSimulatingProgress( simProgress, simPhase.c_str(), progressBarToolTip.c_str() );
  cmdLine -> setImportingProgress( importSimProgress, importSimPhase.c_str(), progressBarToolTip.c_str() );
}

void SC_MainWindow::loadHistory()
{
  QSettings settings;
  QString saveApiKey;
  saveApiKey = settings.value( "options/apikey" ).toString();
  
  QVariant simulateHistory = settings.value( "user_data/simulateHistory" );
  if ( simulateHistory.isValid() )
  {
    QList<QVariant> a = simulateHistory.toList();
    for ( int i = 0; i < a.size(); ++i )
    {
      const QVariant& entry = a.at( i );
      if ( entry.isValid() )
      {
        QStringList sl = entry.toStringList();
        if ( sl.size() == 2 )
        {
          simulateTab -> add_Text( sl.at( 1 ), sl.at( 0 ) );
        }
      }
    }
  }
  QVariant gui_version_number = settings.value( "gui/gui_version_number", 0 );
  if ( gui_version_number.toInt() < SC_GUI_HISTORY_VERSION )
  {
    settings.clear();
    settings.setValue( "options/apikey", saveApiKey );
    QMessageBox msgBox;
    msgBox.setText( tr("We have reset your configuration settings due to major changes to the GUI") );
    msgBox.exec();
  }

  QVariant size = settings.value( "gui/size" );
  QRect savedApplicationGeometry = geometry();
  if ( size.isValid() )
  {
    savedApplicationGeometry.setSize( size.toSize() );
  }
  QVariant pos = settings.value( "gui/position" );
  if ( pos.isValid() )
  {
    savedApplicationGeometry.moveTopLeft( pos.toPoint() );
  }
  QVariant maximized = settings.value( "gui/maximized" );
  if ( maximized.isValid() )
  {
    if ( maximized.toBool() )
      showMaximized();
    else
      showNormal();
  }
  else
    showMaximized();

  QString cache_file = TmpDir + "/simc_cache.dat";
  std::string cache_file_str = cache_file.toStdString();
  http::cache_load( cache_file_str.c_str() );

  QVariant history = settings.value( "user_data/historyList" );
  if ( history.isValid() )
  {
    QList<QVariant> a = history.toList();
    for ( int i = 0; i < a.size(); ++i )
    {
      const QVariant& entry = a.at( i );
      if ( entry.isValid() )
      {
        QString s = entry.toString();
        historyList -> addItem( new QListWidgetItem( s ) );
      }
    }
  }
  optionsTab -> decodeOptions();
  importTab -> automationTab -> decodeSettings();
  spellQueryTab -> decodeSettings();

  if ( simulateTab -> count() <= 1 )
  { // If we haven't retrieved any simulate tabs from history, add a default one.
    simulateTab -> add_Text( defaultSimulateText, tr("Simulate!") );
  }
}

void SC_MainWindow::saveHistory()
{
  QSettings settings;
  settings.beginGroup( "gui" );
  settings.setValue( "gui_version_number", SC_GUI_HISTORY_VERSION );
  settings.setValue( "size", normalGeometry().size() );
  settings.setValue( "position", normalGeometry().topLeft() );
  settings.setValue( "maximized", bool( windowState() & Qt::WindowMaximized ) );
  settings.endGroup();

  QString cache_file = TmpDir + "/simc_cache.dat";
  std::string cache_file_str = cache_file.toStdString();
  http::cache_save( cache_file_str.c_str() );

  settings.beginGroup( "user_data" );

  // simulate tab history
  QList< QVariant > simulateHist;
  for ( int i = 0; i < simulateTab -> count() - 1; ++i )
  {
    SC_TextEdit* tab = static_cast<SC_TextEdit*>( simulateTab -> widget( i ) );
    QStringList entry;
    entry << simulateTab -> tabText( i );
    entry << tab -> toPlainText();
    simulateHist.append( QVariant( entry ) );
  }
  settings.setValue( "simulateHistory", simulateHist );
  // end simulate tab history

  QList<QVariant> history;
  int count = historyList -> count();
  for ( int i = 0; i < count; i++ )
    history.append( QVariant( historyList -> item( i ) -> text() ) );
  settings.setValue( "historyList", history );

  settings.endGroup();

  optionsTab -> encodeOptions();
  importTab -> automationTab -> encodeSettings();
  spellQueryTab -> encodeSettings();
}

// ==========================================================================
// Widget Creation
// ==========================================================================

SC_MainWindow::SC_MainWindow( QWidget *parent )
  : QWidget( parent ),
  visibleWebView( 0 ),
  cmdLine( 0 ), // segfault in updateWebView() if not null
  recentlyClosedTabModel( 0 ),
  sim( 0 ),
  import_sim( 0 ),
  paperdoll_sim( 0 ),
  simPhase( "%p%" ),
  importSimPhase( "%p%" ),
  simProgress( 100 ),
  importSimProgress( 100 ),
  soloimport( 0 ),
  simResults( 0 ),
  AppDataDir( "." ),
  ResultsDestDir( "." ),
  TmpDir( "." ),
  consecutiveSimulationsRun( 0 )
{
  setWindowTitle( QCoreApplication::applicationName() + " " + QCoreApplication::applicationVersion() );
  setAttribute( Qt::WA_AlwaysShowToolTips );

#if defined( Q_OS_MAC )
  QDir::home().mkpath( "Library/Application Support/SimulationCraft" );
  AppDataDir = QDir::home().absoluteFilePath( "Library/Application Support/SimulationCraft" );
#elif defined( Q_OS_WIN )
  AppDataDir = QCoreApplication::applicationDirPath();
#else
  QStringList q = QStandardPaths::standardLocations( QStandardPaths::CacheLocation );
  assert( !q.isEmpty() );
  AppDataDir = q.first();
#endif

  QStringList s = QStandardPaths::standardLocations( QStandardPaths::CacheLocation );
  assert( !s.isEmpty() );
  TmpDir = s.first();

  // Set ResultsDestDir
  s = QStandardPaths::standardLocations( QStandardPaths::DocumentsLocation );
  assert( !s.isEmpty() );
  ResultsDestDir = s.first();

  logFileText = AppDataDir + "/" + "log.txt";
  resultsFileText = AppDataDir + "/" + "results.html";

  mainTab = new SC_MainTab( this );

  createWelcomeTab();
  createImportTab();
  createOptionsTab();
  createSimulateTab();
  createResultsTab();
  createOverridesTab();
  createHelpTab();
  createLogTab();

  createSpellQueryTab();
  createCmdLine();
  createToolTips();
#if defined( Q_OS_MAC )
  createTabShortcuts();
#endif
#ifdef SC_PAPERDOLL
  createPaperdoll();
#endif

  connect( mainTab, SIGNAL( currentChanged( int ) ), this, SLOT( mainTabChanged( int ) ) );

  QVBoxLayout* vLayout = new QVBoxLayout();
  vLayout -> addWidget( mainTab, 1 );
  vLayout -> addWidget( cmdLine, 0 );
  setLayout( vLayout );
  vLayout -> activate();

  soloChar = new QTimer( this );
  connect( soloChar, SIGNAL( timeout() ), this, SLOT( updatetimer() ) );

  timer = new QTimer( this );
  connect( timer, SIGNAL( timeout() ), this, SLOT( updateSimProgress() ) );

  importThread = new SC_ImportThread( this );
  connect( importThread, SIGNAL( finished() ), this, SLOT( importFinished() ) );

  simulateThread = new SC_SimulateThread( this );
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
  cmdLine = new SC_MainWindowCommandLine( this );

  connect( &simulationQueue, SIGNAL( firstItemWasAdded() ), this, SLOT( itemWasEnqueuedTryToSim() ) );
  connect( cmdLine, SIGNAL( pauseClicked() ), this, SLOT( pauseButtonClicked() ) );
  connect( cmdLine, SIGNAL( resumeClicked() ), this, SLOT( pauseButtonClicked() ) );
  connect( cmdLine, SIGNAL( backButtonClicked() ), this, SLOT( backButtonClicked() ) );
  connect( cmdLine, SIGNAL( forwardButtonClicked() ), this, SLOT( forwardButtonClicked() ) );
  connect( cmdLine, SIGNAL( simulateClicked() ), this, SLOT( enqueueSim() ) );
  connect( cmdLine, SIGNAL( queueClicked() ), this, SLOT( enqueueSim() ) );
  connect( cmdLine, SIGNAL( queryClicked() ), this, SLOT( queryButtonClicked() ) );
  connect( cmdLine, SIGNAL( importClicked() ), this, SLOT( importButtonClicked() ) );
  connect( cmdLine, SIGNAL( saveLogClicked() ), this, SLOT( saveLog() ) );
  connect( cmdLine, SIGNAL( saveResultsClicked() ), this, SLOT( saveResults() ) );
  connect( cmdLine, SIGNAL( cancelSimulationClicked() ), this, SLOT( stopSim() ) );
  connect( cmdLine, SIGNAL( cancelAllSimulationClicked() ), this, SLOT( stopAllSim() ) );
  connect( cmdLine, SIGNAL( cancelImportClicked() ), this, SLOT( stopImport() ) );
#ifdef SC_PAPERDOLL
  connect( cmdLine, SIGNAL( simulatePaperdollClicked() ), this, SLOT( start_paperdoll_sim() ) );
#endif
  connect( cmdLine, SIGNAL( commandLineReturnPressed() ), this, SLOT( cmdLineReturnPressed() ) );
  connect( cmdLine, SIGNAL( commandLineTextEdited( const QString& ) ), this, SLOT( cmdLineTextEdited( const QString& ) ) );
  connect( cmdLine, SIGNAL( switchToLeftSubTab() ), this, SLOT( switchToLeftSubTab() ) );
  connect( cmdLine, SIGNAL( switchToRightSubTab() ), this, SLOT( switchToRightSubTab() ) );
  connect( cmdLine, SIGNAL( currentlyViewedTabCloseRequest() ), this, SLOT( currentlyViewedTabCloseRequest() ) );
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

#if ! defined( SC_USE_WEBKIT )
void SC_WelcomeTabWidget::welcomeLoadSlot()
{
  setUrl( welcome_uri );
  welcome_timer -> deleteLater();
}
#endif

SC_WelcomeTabWidget::SC_WelcomeTabWidget( SC_MainWindow* parent ) :
  SC_WebEngineView( parent )
{
  QString welcomeFile = SC_PATHS::getDataPath() + "/Welcome.html";

#ifndef SC_USE_WEBKIT
  welcome_uri = "file:///" + welcomeFile;
  welcome_timer = new QTimer( this );
  welcome_timer -> setSingleShot( true );
  welcome_timer -> setInterval( 500 );

  connect( welcome_timer, SIGNAL( timeout() ), this, SLOT( welcomeLoadSlot() ) );
  welcome_timer -> start();

  connect( this, SIGNAL( urlChanged(const QUrl& ) ), this, SLOT( urlChangedSlot(const QUrl&) ) );
#else
  setUrl( "file:///" + welcomeFile );
  page() -> setLinkDelegationPolicy( QWebPage::DelegateAllLinks );
  connect( this, SIGNAL( linkClicked( const QUrl& ) ), this, SLOT( linkClickedSlot( const QUrl& ) ) );
#endif
}

void SC_MainWindow::createImportTab()
{
  importTab = new SC_ImportTab( this );
  mainTab -> addTab( importTab, tr( "Import" ) );

  battleNetView = new SC_WebView( this );
  battleNetView -> setUrl( QUrl( "http://us.battle.net/wow/en" ) );
  battleNetView -> enableMouseNavigation();
  battleNetView -> disableKeyboardNavigation();
  importTab -> addTab( battleNetView, tr( "Battle.Net" ) );

  createSampleProfilesTab();

  historyList = new QListWidget();
  historyList -> setSortingEnabled( true );
  importTab -> addTab( historyList, tr( "History" ) );

  recentlyClosedTabImport = new SC_RecentlyClosedTabWidget( this, QBoxLayout::LeftToRight );
  recentlyClosedTabModel = recentlyClosedTabImport -> getModel();
  importTab -> addTab( recentlyClosedTabImport, tr( "Recently Closed" ) );

  importTab -> addTab( importTab -> automationTab, tr( "Automation" ) );

  connect( historyList, SIGNAL( itemDoubleClicked( QListWidgetItem* ) ), this, SLOT( historyDoubleClicked( QListWidgetItem* ) ) );
  connect( importTab, SIGNAL( currentChanged( int ) ), this, SLOT( importTabChanged( int ) ) );
  connect( recentlyClosedTabImport, SIGNAL( restoreTab( QWidget*, const QString&, const QString&, const QIcon& ) ),
           this, SLOT( simulateTabRestored( QWidget*, const QString&, const QString&, const QIcon& ) ) );

  // Commenting out until it is more fleshed out.
  // createCustomTab();
}

void SC_MainWindow::createSampleProfilesTab()
{
  SC_SampleProfilesTab* bisTab = new SC_SampleProfilesTab( this );
  connect( bisTab -> tree, SIGNAL( itemDoubleClicked( QTreeWidgetItem*, int ) ), this, SLOT( bisDoubleClicked( QTreeWidgetItem*, int ) ) );
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
  importTab -> addTab( customGroupBox, tr("Custom Profile") );
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
  createCustomProfileDock -> setTabToolTip( createCustomProfileDock -> indexOf( customGearTab ), tr( "Customize Gear Setup", "createCustomTab" ) );
  createCustomProfileDock -> setTabText( createCustomProfileDock -> indexOf( customTalentsTab ), tr( "Talents", "createCustomTab" ) );
  createCustomProfileDock -> setTabToolTip( createCustomProfileDock -> indexOf( customTalentsTab ), tr( "Customize Talents", "createCustomTab" ) );
  createCustomProfileDock -> setTabText( createCustomProfileDock -> indexOf( customGlyphsTab ), tr( "Glyphs", "createCustomTab" ) );
  createCustomProfileDock -> setTabToolTip( createCustomProfileDock -> indexOf( customGlyphsTab ), tr( "Customize Glyphs", "createCustomTab" ) );
}

void SC_MainWindow::createSimulateTab()
{
  simulateTab = new SC_SimulateTab( mainTab, recentlyClosedTabModel );
  mainTab -> addTab( simulateTab, tr( "Simulate" ) );
}

void SC_MainWindow::createOverridesTab()
{
  overridesText = new SC_OverridesTab( this );

  mainTab -> addTab( overridesText, tr( "Overrides" ) );
}

void SC_MainWindow::createLogTab()
{
  logText = new SC_TextEdit( this, false );
  logText -> setReadOnly( true );
  logText -> setPlainText( tr("Look here for error messages and simple text-only reporting.\n") );
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
  resultsTab = new SC_ResultTab( this );
  resultsTab -> setTabsClosable( true );
  connect( resultsTab, SIGNAL( currentChanged( int ) ), this, SLOT( resultsTabChanged( int ) ) );
  connect( resultsTab, SIGNAL( tabCloseRequested( int ) ), this, SLOT( resultsTabCloseRequest( int ) ) );
  mainTab -> addTab( resultsTab, tr( "Results" ) );
}

void SC_MainWindow::createSpellQueryTab()
{
  spellQueryTab = new SC_SpellQueryTab( this );
  mainTab -> addTab( spellQueryTab, tr( "Spell Query" ) );
}

void SC_MainWindow::createToolTips()
{
  optionsTab -> createToolTips();
}

void SC_MainWindow::createTabShortcuts()
{
  Qt::Key keys[] = { Qt::Key_1, Qt::Key_2, Qt::Key_3, Qt::Key_4, Qt::Key_5, Qt::Key_6, Qt::Key_7, Qt::Key_8, Qt::Key_9, Qt::Key_unknown };
  for ( int i = 0; keys[i] != Qt::Key_unknown; i++ )
  {
    // OS X needs to set the sequence to Cmd-<number>, since Alt is used for normal keys in certain cases
#if ! defined( Q_OS_MAC )
    QShortcut* shortcut = new QShortcut( QKeySequence( Qt::ALT + keys[i] ), this );
#else
    QShortcut* shortcut = new QShortcut( QKeySequence( Qt::CTRL + keys[i] ), this );
#endif
    mainTabSignalMapper.setMapping( shortcut, i );
    connect( shortcut, SIGNAL( activated() ), &mainTabSignalMapper, SLOT( map() ) );
    shortcuts.push_back( shortcut );
  }
  connect( &mainTabSignalMapper, SIGNAL( mapped( int ) ), mainTab, SLOT( setCurrentIndex( int ) ) );
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

  mainTab -> addTab( paperdollTab, tr("Paperdoll") );
}
#endif

void SC_MainWindow::updateWebView( SC_WebView* wv )
{
  assert( wv );
  visibleWebView = wv;
  if ( cmdLine != 0 ) // can be called before widget is setup
  {
    if ( visibleWebView == battleNetView )
    {
      cmdLine -> setBattleNetLoadProgress( visibleWebView -> progress, "%p%", "" );
      cmdLine -> setCommandLineText( TAB_BATTLE_NET, visibleWebView -> url_to_show );
    }
    else if ( visibleWebView == helpView )
    {
      cmdLine -> setHelpViewProgress( visibleWebView -> progress, "%p%", "" );
      cmdLine -> setCommandLineText( TAB_HELP, visibleWebView -> url_to_show );
    }
  }
}

// ==========================================================================
// Sim Initialization
// ==========================================================================

sim_t* SC_MainWindow::initSim()
{
  sim_t* sim = new sim_t();
  sim -> pause_mutex = new mutex_t();
  sim -> report_progress = 0;
#if SC_USE_PTR
  sim -> parse_option( "ptr", ( ( optionsTab -> choice.version -> currentIndex() == 1 ) ? "1" : "0" ) );
#endif
  sim -> parse_option( "debug", ( ( optionsTab -> choice.debug -> currentIndex() == 2 ) ? "1" : "0" ) );

  return sim;
}

void SC_MainWindow::deleteSim( sim_t* sim, SC_TextEdit* append_error_message )
{
  if ( sim )
  {
    std::list< std::string > files;
    std::vector< std::string > errorListCopy( sim -> error_list );
    files.push_back( sim -> output_file_str );
    files.push_back( sim -> html_file_str );
    files.push_back( sim -> xml_file_str );
    files.push_back( sim -> reforge_plot_output_file_str );
    files.push_back( sim -> csv_output_file_str );

    std::string output_file_str = sim -> output_file_str;
    bool sim_control_was_not_zero = sim -> control != 0;

    delete sim -> pause_mutex;
    delete sim;
    sim = 0;

    QString contents;
    bool logFileOpenedSuccessfully = false;
    QFile logFile( QString::fromStdString( output_file_str ) );

    if ( sim_control_was_not_zero ) // sim -> setup() was not called, output file was not opened
    {
      // ReadWrite failure indicates permission issues
      if ( logFile.open( QIODevice::ReadWrite | QIODevice::Text ) )
      {
        contents = QString::fromUtf8( logFile.readAll() );
        logFile.close();
        logFileOpenedSuccessfully = true;
      }
    }
    // Nothing in the log file, no point wondering about "permission issues"
    else
      logFileOpenedSuccessfully = true;

    if ( !logFileOpenedSuccessfully )
    {
      for ( std::vector< std::string >::iterator it = errorListCopy.begin(); it != errorListCopy.end(); ++it )
      {
        contents.append( QString::fromStdString( *it + "\n" ) );
      }
      // If the failure is due to permissions issues, make this very clear to the user that the problem is on their end and what must be done to fix it
      std::list< QString > directoriesWithPermissionIssues;
      std::list< QString > filesWithPermissionIssues;
      std::list< QString > filesThatAreDirectories;
      std::string windowsPermissionRecommendation;
      std::string suggestions;
#ifdef SC_WINDOWS
      if ( QSysInfo::WindowsVersion >= QSysInfo::WV_VISTA )
      {
        windowsPermissionRecommendation = "Try running the program with administrative privileges by right clicking and selecting \"Run as administrator\"\n Or even installing the program to a different directory may help resolve these permission issues.";
      }
#endif
      for ( std::list< std::string >::iterator it = files.begin(); it != files.end(); ++it )
      {
        if ( !( *it ).empty() )
        {
          QFileInfo file( QString::fromStdString( *it ) );
          if ( file.isDir() )
          {
            filesThatAreDirectories.push_back( file.absoluteFilePath() );
          }
          else if ( !file.isWritable() )
          {
            QFileInfo filesDirectory( file.absolutePath() );
            if ( !filesDirectory.isWritable() )
              directoriesWithPermissionIssues.push_back( file.absolutePath() );
            filesWithPermissionIssues.push_back( file.absoluteFilePath() );
          }
        }
      }
      directoriesWithPermissionIssues.unique();
      if ( filesThatAreDirectories.size() != 0 )
      {
        suggestions.append( "The following files are directories, SimulationCraft uses these as files, please rename them\n" );
      }
      for ( std::list< QString >::iterator it = filesThatAreDirectories.begin(); it != filesThatAreDirectories.end(); ++it )
      {
        suggestions.append( "   " + ( *it ).toStdString() + "\n" );
      }
      if ( filesWithPermissionIssues.size() != 0 )
      {
        suggestions.append( "The following files have permission issues and are unwritable\n SimulationCraft needs to write to these files to simulate\n" );
      }
      for ( std::list< QString >::iterator it = filesWithPermissionIssues.begin(); it != filesWithPermissionIssues.end(); ++it )
      {
        suggestions.append( "   " + ( *it ).toStdString() + "\n" );
      }
      if ( directoriesWithPermissionIssues.size() != 0 )
      {
        suggestions.append( "The following directories have permission issues and are unwritable\n meaning SimulationCraft cannot create files in these directories\n" );
      }
      for ( std::list< QString >::iterator it = directoriesWithPermissionIssues.begin(); it != directoriesWithPermissionIssues.end(); ++it )
      {
        suggestions.append( "   " + ( *it ).toStdString() + "\n" );
      }
      if ( suggestions.length() != 0 )
      {
        suggestions = "\nSome possible suggestions on how to fix:\n" + suggestions;
        append_error_message -> append( QString::fromStdString( suggestions ) );
        if ( windowsPermissionRecommendation.length() != 0 )
        {
          contents.append( QString::fromStdString( windowsPermissionRecommendation + "\n" ) );
        }
      }
      if ( contents.contains( "Internal server error" ) )
      {
        contents.append( "\nAn Internal server error means an error occurred with the server, trying again later should fix it\n" );
      }
      contents.append( "\nIf for some reason you cannot resolve this issue, check if it is a known issue at\n https://code.google.com/p/simulationcraft/issues/list\n" );
      contents.append( "Or try an older version\n https://code.google.com/p/simulationcraft/wiki/Downloads\n" );
      contents.append( "And if all else fails you can come talk to us on IRC at\n irc.stratics.com (#simulationcraft)\n" );
    }

    // If requested, append the error message to the given Text Widget as well.
    if ( append_error_message )
    {
      append_error_message -> append( contents );
    }
    if ( logText != append_error_message )
    {
      logText -> append( contents );
      logText -> moveCursor( QTextCursor::End );
    }
  }
}

void SC_MainWindow::enqueueSim()
{
  QString title = simulateTab -> tabText( simulateTab -> currentIndex() );
  QString options = simulateTab -> current_Text() -> toPlainText().toUtf8();
  QString fullOptions = optionsTab -> mergeOptions();

  simulationQueue.enqueue( title, options, fullOptions );
}

void SC_MainWindow::startImport( int tab, const QString& url )
{
  if ( importRunning() )
  {
    stopImport();
    return;
  }
  simProgress = 0;
  import_sim = initSim();
  importThread -> start( import_sim, tab, url, optionsTab -> get_db_order(), optionsTab -> get_active_spec(), optionsTab -> get_player_role(), optionsTab -> get_api_key() );
  simulateTab -> add_Text( defaultSimulateText, tr( "Importing" ) );
}

void SC_MainWindow::stopImport()
{
  if ( importRunning() )
  {
    import_sim -> cancel();
  }
}

bool SC_MainWindow::importRunning()
{
  return ( import_sim != 0 );
}

void SC_MainWindow::itemWasEnqueuedTryToSim()
{
  if ( !simRunning() )
  {
    startSim();
  }
  else
  {
    cmdLine -> setState( SC_MainWindowCommandLine::SIMULATING_MULTIPLE );
  }
}

void SC_MainWindow::updatetimer()
{
  if ( importRunning() )
  {
    updateSimProgress();
    soloChar -> start();
  }
  else
  {
    soloimport = 0;
  }
}

void SC_MainWindow::importFinished()
{
  importSimPhase = "%p%";
  simProgress = 100;
  importSimProgress = 100;
  cmdLine -> setImportingProgress( importSimProgress, importSimPhase.c_str(), "" );

  logText -> clear();

  if ( importThread -> player )
  {
    simulateTab -> set_Text( importThread -> profile );
    simulateTab->setTabText( simulateTab -> currentIndex(), QString::fromUtf8( importThread -> player -> name_str.c_str() ) );

    QString label = QString::fromUtf8( importThread -> player -> name_str.c_str() );
    while ( label.size() < 20 ) label += ' ';
    label += QString::fromUtf8( importThread -> player -> origin_str.c_str() );
    if ( label.contains( ".api." ) )
    { // Strip the s out of https and the api. out of the string so that it is a usable link.
      label.replace( QString( ".api" ), QString( "" ) );
      label.replace( QString( "https"), QString( "http" ) );
    }
    bool found = false;
    for ( int i = 0; i < historyList -> count() && !found; i++ )
    {
      if ( historyList -> item( i ) -> text() == label )
        found = true;
    }

    if ( !found )
    {
      QListWidgetItem* item = new QListWidgetItem( label );
      historyList -> addItem( item );
      historyList -> sortItems();
    }

    deleteSim( import_sim ); import_sim = 0;
  }
  else
  {
    simulateTab -> setTabText( simulateTab -> currentIndex(), tr( "Import Failed" ) );
    simulateTab -> append_Text( tr("# Unable to generate profile from: ") + importThread -> url + "\n" );
    for( size_t i = 0; i < import_sim -> error_list.size(); ++i )
    {
        simulateTab -> append_Text( QString( "# ") + QString::fromStdString( import_sim -> error_list[ i ] ) );
    }
    deleteSim( import_sim, simulateTab -> current_Text() ); import_sim = 0;
  }

  if ( !simRunning() )
  {
    timer -> stop();
  }
  mainTab -> setCurrentTab( TAB_SIMULATE );
}

void SC_MainWindow::startSim()
{
  saveHistory(); // This is to save whatever work the person has done, just in case there's a crash.
  if ( simRunning() )
  {
    stopSim();
    return;
  }
  if ( simulationQueue.isEmpty() )
  {
    // enqueue will call itemWasEnqueuedTryToSim() since it is empty
    // which starts the sim
    enqueueSim();
    return;
  }
  optionsTab -> encodeOptions();
  importTab -> automationTab -> encodeSettings();

  // Clear log text on success so users don't get confused if there is
  // an error from previous attempts
  if ( consecutiveSimulationsRun == 0 ) // Only clear on first queued sim
  {
    logText -> clear();
  }
  simProgress = 0;
  sim = initSim();

  QString options = simulationQueue.dequeue();

  QByteArray utf8_profile = options.toUtf8();
  QFile file( AppDataDir + QDir::separator() + "simc_gui.simc" );
  if ( file.open( QIODevice::WriteOnly | QIODevice::Text ) )
  {
    file.write( utf8_profile );
    file.close();
  }

  QString reportFileBase = optionsTab -> getReportlDestination();
  sim -> output_file_str = (reportFileBase + ".txt").toStdString();
  sim -> html_file_str = (reportFileBase + ".html").toStdString();

  sim -> xml_file_str = (reportFileBase + ".xml").toStdString();
  sim -> reforge_plot_output_file_str = (reportFileBase + "_plotdata.csv").toStdString();
  sim -> csv_output_file_str = (reportFileBase + ".csv").toStdString();

  if ( optionsTab -> get_api_key().size() == 32 ) // api keys are 32 characters long, it's not worth parsing <32 character keys.
    sim -> parse_option( "apikey",  optionsTab -> get_api_key().toUtf8().constData() );

  if ( cmdLine -> currentState() != SC_MainWindowCommandLine::SIMULATING_MULTIPLE )
  {
    cmdLine -> setState( SC_MainWindowCommandLine::SIMULATING );
  }
  simulateThread -> start( sim, utf8_profile );

  cmdLineText = "";
  timer -> start( 100 );
}

void SC_MainWindow::stopSim()
{
  if ( simRunning() )
  {
    simulationQueue.clear();
    sim -> cancel();
    stopImport();

    if ( sim -> paused )
    {
      toggle_pause();
    }
  }
}
void SC_MainWindow::stopAllSim()
{
  stopSim();
}

bool SC_MainWindow::simRunning()
{
  return ( sim != 0 );
}

#ifdef SC_PAPERDOLL

player_t* SC_MainWindow::init_paperdoll_sim( sim_t*& sim )
{
  sim = initSim();

  PaperdollProfile* profile = paperdollProfile;
  const module_t* module = module_t::get( profile -> currentClass() );
  player_t* player = module ? module -> create_player( sim, tr("Paperdoll Player"), profile -> currentRace() ) : NULL;

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
  simPhase = "%p%";
  simProgress = 100;
  cmdLine -> setSimulatingProgress( simProgress, simPhase.c_str(), tr( "Finished!" ) );
  bool sim_was_debug = sim -> debug || sim -> log;

  logText -> clear();
  if ( ! simulateThread -> success )
  {
    logText -> moveCursor( QTextCursor::End );
    if ( mainTab -> currentTab() != TAB_SPELLQUERY )
      mainTab -> setCurrentTab( TAB_LOG );

    qDebug() << "sim failed!" << simulateThread -> getError();
    logText -> append( simulateThread -> getError() );
    logText -> append( tr("Simulation failed!") );

    // Spell Query
    if ( mainTab -> currentTab() == TAB_SPELLQUERY )
    {
      QString result;
      std::stringstream ss;
      try
      {
        sim -> spell_query -> evaluate();
        report::print_spell_query( ss, sim -> dbc, *sim -> spell_query, sim -> spell_query_level );
      }
      catch ( const std::exception& e ){
        ss << "ERROR! Spell Query failure: " << e.what() << std::endl;
      }
      result = QString::fromStdString( ss.str() );
      if ( result.isEmpty() )
      {
        result = "No results found!";
      }
      spellQueryTab -> textbox.result -> setText( result );
      spellQueryTab -> checkForSave();
    }
  }
  else
  {
    QString resultsName = tr( "Results %1" ).arg( ++simResults );
    SC_SingleResultTab* resultsEntry = new SC_SingleResultTab( this, resultsTab );
    if ( resultsTab -> count() != 0 )
    {
      QList< Qt::KeyboardModifier > s;
      s.push_back( Qt::ControlModifier );
      QList< Qt::KeyboardModifier > emptyList;
      if ( resultsTab -> count() == 1 )
      {
        SC_SingleResultTab* resultsEntry_one = static_cast <SC_SingleResultTab*>(resultsTab -> widget( 0 ));
        resultsEntry_one -> addIgnoreKeyPressEvent( Qt::Key_Tab, s );
        resultsEntry_one -> addIgnoreKeyPressEvent( Qt::Key_Backtab, emptyList );
      }
      resultsEntry -> addIgnoreKeyPressEvent( Qt::Key_Tab, s );
      resultsEntry -> addIgnoreKeyPressEvent( Qt::Key_Backtab, emptyList );
    }
    resultsTab -> addTab( resultsEntry, resultsName );
    if ( ++consecutiveSimulationsRun == 1 )
    {
      // only switch to the new tab if we are the first simulation in queue
      resultsTab -> setCurrentWidget( resultsEntry );
    }

    // HTML
    SC_WebView* resultsHtmlView = new SC_WebView( this, resultsEntry );
    resultsHtmlView -> enableKeyboardNavigation();
    resultsHtmlView -> enableMouseNavigation();
    resultsEntry -> addTab( resultsHtmlView, "html" );
    QFile html_file( sim -> html_file_str.c_str() );
    QString html_file_absolute_path = QFileInfo( html_file ).absoluteFilePath();
    // just load it, let the error page extension handle failure to open
    resultsHtmlView -> load( QUrl::fromLocalFile( html_file_absolute_path ) );

    // Text
    SC_TextEdit* resultsTextView = new SC_TextEdit( resultsEntry );
    resultsEntry -> addTab( resultsTextView, "text" );
    QFile logFile( sim -> output_file_str.c_str() );
    if ( logFile.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
      resultsTextView -> setPlainText( logFile.readAll() );
      logFile.close();
    }
    else
    {
        resultsTextView -> setPlainText( tr( "Error opening %1. %2" ).arg( sim -> output_file_str.c_str(), logFile.errorString() ) );
    }

    // XML
    SC_TextEdit* resultsXmlView = new SC_TextEdit( resultsEntry );
    resultsEntry -> addTab( resultsXmlView, "xml" );
    QFile xml_file( sim -> xml_file_str.c_str() );
    if ( xml_file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
      resultsXmlView -> append( xml_file.readAll() );
      xml_file.close();
    }
    else
    {
      resultsXmlView -> setPlainText( tr( "Error opening %1. %2" ).arg( sim -> xml_file_str.c_str(), xml_file.errorString() ) );
    }

    // Plot Data
    SC_TextEdit* resultsPlotView = new SC_TextEdit( resultsEntry );
    resultsEntry -> addTab( resultsPlotView, tr("plot data") );
    QFile plot_file( sim -> reforge_plot_output_file_str.c_str() );
    if ( plot_file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
      resultsPlotView -> append( plot_file.readAll() );
      plot_file.close();
    }
    else
    {
        resultsPlotView -> setPlainText( tr( "Error opening %1. %2" ).arg( sim -> reforge_plot_output_file_str.c_str(), plot_file.errorString() ) );
    }

    // CSV
    SC_TextEdit* resultsCsvView = new SC_TextEdit( resultsEntry );
    resultsEntry -> addTab( resultsCsvView, "csv" );
    QFile csv_file( sim -> csv_output_file_str.c_str() );
    if ( csv_file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
      resultsCsvView -> append( csv_file.readAll() );
      csv_file.close();
    }

    if ( simulationQueue.isEmpty() )
    {
      mainTab -> setCurrentTab( sim_was_debug ? TAB_LOG : TAB_RESULTS );
    }

  }
  if ( simulationQueue.isEmpty() && !importRunning() )
    timer -> stop();

  deleteSim( sim, simulateThread -> success == true ? 0 : logText ); SC_MainWindow::sim = 0;

  if ( !simulationQueue.isEmpty() )
  {
    startSim(); // Continue simming what is left in the queue
  }
  else
  {
    consecutiveSimulationsRun = 0;
    cmdLine -> setState( SC_MainWindowCommandLine::IDLE );
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
  QFile file( cmdLine -> commandLineText( TAB_LOG ) );

  if ( file.open( QIODevice::WriteOnly | QIODevice::Text ) )
  {
    file.write( logText -> toPlainText().toUtf8() );
    file.close();
  }

  logText->append( QString( tr("Log saved to: %1\n") ).arg( cmdLine -> commandLineText( TAB_LOG ) ) );
}

void SC_MainWindow::saveResults()
{
  int index = resultsTab -> currentIndex();
  if ( index < 0 ) return;

  SC_SingleResultTab* t = debug_cast<SC_SingleResultTab*>( resultsTab -> currentWidget() );
  t -> save_result();
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
  case TAB_LOG:       logFileText = s; break;
  case TAB_RESULTS:   resultsFileText = s; break;
  default:  break;
  }
}

void SC_MainWindow::cmdLineReturnPressed()
{
  if ( mainTab -> currentTab() == TAB_IMPORT )
  {
    if ( cmdLine -> commandLineText().count( "battle.net" ) ||
      cmdLine -> commandLineText().count( "battlenet.com" ) )
    {
      battleNetView -> setUrl( QUrl::fromUserInput( cmdLine -> commandLineText() ) );
      cmdLine -> setCommandLineText( TAB_BATTLE_NET, cmdLine -> commandLineText() );
      importTab -> setCurrentTab( TAB_BATTLE_NET );
    }
    else
    {
      if ( !sim ) cmdLine -> mainButtonClicked();
    }
  }
  else
  {
    if ( !sim ) cmdLine -> mainButtonClicked();
  }
}

void SC_MainWindow::mainButtonClicked( bool /* checked */ )
{
  switch ( mainTab -> currentTab() )
  {
  case TAB_WELCOME:   enqueueSim(); break;
  case TAB_OPTIONS:   enqueueSim(); break;
  case TAB_SIMULATE:  enqueueSim(); break;
  case TAB_OVERRIDES: enqueueSim(); break;
  case TAB_HELP:      enqueueSim(); break;
  case TAB_IMPORT:
    switch ( importTab -> currentTab() )
    {
    case TAB_BATTLE_NET: startImport( TAB_BATTLE_NET, cmdLine -> commandLineText() ); break;
    case TAB_RECENT:     recentlyClosedTabImport -> restoreCurrentlySelected(); break;
    default: break;
    }
    break;
  case TAB_LOG: saveLog(); break;
  case TAB_RESULTS: saveResults(); break;
  case TAB_SPELLQUERY: spellQueryTab -> run_spell_query();
#ifdef SC_PAPERDOLL
  case TAB_PAPERDOLL: start_paperdoll_sim(); break;
#endif
  case TAB_COUNT:
    break;
  }
}

void SC_MainWindow::cancelButtonClicked()
{
  switch ( mainTab -> currentTab() )
  {
  case TAB_IMPORT:
    if ( simRunning() )
    {
      stopSim();
    }
    break;
  default:
    break;
  }
}

void SC_MainWindow::queueButtonClicked()
{
  enqueueSim();
}

void SC_MainWindow::importButtonClicked()
{
  switch ( importTab -> currentTab() )
  {
  case TAB_BATTLE_NET:
  {
    soloChar -> start( 50 );
    startImport( TAB_BATTLE_NET, cmdLine -> commandLineText( TAB_BATTLE_NET ) );
    break;
  }
  case TAB_RECENT:     recentlyClosedTabImport -> restoreCurrentlySelected(); break;
  case TAB_AUTOMATION:
  {
      QString profile = importTab -> automationTab -> startImport();
      simulateTab -> add_Text( profile,  tr( "Automation Import" ) );
      mainTab -> setCurrentTab( TAB_SIMULATE );
  }
      break;
  default: break;
  }
}

void SC_MainWindow::queryButtonClicked()
{
  spellQueryTab -> run_spell_query();
}

void SC_MainWindow::pauseButtonClicked( bool )
{
  cmdLine -> togglePaused();
  toggle_pause();
}

void SC_MainWindow::backButtonClicked( bool /* checked */ )
{
  if ( visibleWebView )
  {
    if ( mainTab -> currentTab() == TAB_RESULTS && !visibleWebView->history()->canGoBack() )
    {
      visibleWebView -> loadHtml();
#if defined( SC_USE_WEBKIT )
      QWebHistory* h = visibleWebView->history();
      h->setMaximumItemCount( 0 );
      h->setMaximumItemCount( 100 );
#endif
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
    case TAB_OPTIONS:   break;
    case TAB_OVERRIDES: break;
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
}

void SC_MainWindow::reloadButtonClicked( bool )
{
  if ( visibleWebView )
  {
    visibleWebView->reload();
    visibleWebView->setFocus();
  }
}

void SC_MainWindow::mainTabChanged( int index )
{
  visibleWebView = 0;
  if ( index == TAB_IMPORT )
  {
    cmdLine -> setTab( static_cast<import_tabs_e>( importTab -> currentIndex() ) );
  }
  else
  {
    cmdLine -> setTab( static_cast<main_tabs_e>( index ) );
  }

  // clear spell_query entries when changing tabs
  if ( cmdLine -> commandLineText().startsWith( "spell_query" ) )
    cmdLine -> setCommandLineText( "" );

  switch ( index )
  {
  case TAB_WELCOME:
  case TAB_OPTIONS:
  case TAB_SIMULATE:
  case TAB_OVERRIDES:
#ifdef SC_PAPERDOLL
  case TAB_PAPERDOLL:
#endif
  case TAB_HELP:      break;
  case TAB_LOG:       cmdLine -> setCommandLineText( TAB_LOG, logFileText ); break;
  case TAB_IMPORT:
    importTabChanged( importTab->currentIndex() );
    break;
  case TAB_RESULTS:
    resultsTabChanged( resultsTab -> currentIndex() );
    break;
  case TAB_SPELLQUERY:
    break;
  default: assert( 0 );
  }
}

void SC_MainWindow::importTabChanged( int index )
{
  if ( index == TAB_BIS ||
       index == TAB_CUSTOM ||
       index == TAB_HISTORY ||
       index == TAB_AUTOMATION ||
       index == TAB_RECENT )
  {
    cmdLine -> setTab( static_cast<import_tabs_e>( index ) );
  }
  else
  {
    updateWebView( debug_cast<SC_WebView*>( importTab -> widget( index ) ) );
    cmdLine -> setTab( static_cast<import_tabs_e>( index ) );
  }
}

void SC_MainWindow::resultsTabChanged( int /* index */ )
{
  if ( resultsTab -> count() > 0 )
  {
    SC_SingleResultTab* s = static_cast<SC_SingleResultTab*>( resultsTab -> currentWidget() );
    s -> TabChanged( s -> currentIndex() );
  }
}

void SC_MainWindow::resultsTabCloseRequest( int index )
{
  int confirm = QMessageBox::question( this, tr( "Close Result Tab" ), tr( "Do you really want to close this result?" ), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );
  if ( confirm == QMessageBox::Yes )
  {
    resultsTab -> removeTab( index );
    if ( resultsTab -> count() == 1 )
    {
      SC_SingleResultTab* tab = static_cast <SC_SingleResultTab*>(resultsTab -> widget( 0 ));
      tab -> removeAllIgnoreKeyPressEvent();
    }
    else if ( resultsTab -> count() == 0 )
    {
      if ( simulateTab -> contains_Only_Default_Profiles() )
      {
        mainTab -> setCurrentIndex( mainTab -> indexOf( importTab ) );
      }
      else
      {
        mainTab -> setCurrentIndex( mainTab -> indexOf( simulateTab ) );
      }
    }
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
}

void SC_MainWindow::bisDoubleClicked( QTreeWidgetItem* item, int /* col */ )
{
  QString profile = item -> text( 1 );
  if ( profile.isEmpty() )
    return;
  QString s = tr("Unable to import profile ") + profile;

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

void SC_MainWindow::simulateTabRestored( QWidget*, const QString&, const QString&, const QIcon& )
{
  mainTab -> setCurrentTab( TAB_SIMULATE );
}

void SC_MainWindow::switchToASubTab( int direction )
{
  QTabWidget* tabWidget = 0;
  switch ( mainTab -> currentTab() )
  {
  case TAB_SIMULATE:
    tabWidget = simulateTab;
    break;
  case TAB_RESULTS:
    tabWidget = resultsTab;
    break;
  case TAB_IMPORT:
    tabWidget = importTab;
    break;
  default:
    return;
  }

  int new_index = tabWidget -> currentIndex();

  if ( direction > 0 )
  {
    if ( tabWidget -> count() - new_index > 1 )
      tabWidget -> setCurrentIndex( new_index + 1 );
  }
  else if ( direction < 0 )
  {
    if ( new_index > 0 )
      tabWidget -> setCurrentIndex( new_index - 1 );
  }
}

void SC_MainWindow::switchToLeftSubTab()
{
  switchToASubTab( -1 );
}

void SC_MainWindow::switchToRightSubTab()
{
  switchToASubTab( 1 );
}

void SC_MainWindow::currentlyViewedTabCloseRequest()
{
  switch ( mainTab -> currentTab() )
  {
  case TAB_SIMULATE:
  {
    simulateTab -> TabCloseRequest( simulateTab -> currentIndex() );
  }
  break;
  case TAB_RESULTS:
  {
    resultsTab -> TabCloseRequest( resultsTab -> currentIndex() );
  }
  break;
  default: break;
  }
}


// ============================================================================
// SC_CommandLine
// ============================================================================

SC_CommandLine::SC_CommandLine( QWidget* parent ):
QLineEdit( parent )
{
  QShortcut* altUp = new QShortcut( QKeySequence( Qt::ALT + Qt::Key_Up ), this );
  QShortcut* altLeft = new QShortcut( QKeySequence( Qt::ALT + Qt::Key_Left ), this );
  QShortcut* altDown = new QShortcut( QKeySequence( Qt::ALT + Qt::Key_Down ), this );
  QShortcut* altRight = new QShortcut( QKeySequence( Qt::ALT + Qt::Key_Right ), this );
  QShortcut* ctrlDel = new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_Delete ), this );

  connect( altUp, SIGNAL( activated() ), this, SIGNAL( switchToLeftSubTab() ) );
  connect( altLeft, SIGNAL( activated() ), this, SIGNAL( switchToLeftSubTab() ) );
  connect( altDown, SIGNAL( activated() ), this, SIGNAL( switchToRightSubTab() ) );
  connect( altRight, SIGNAL( activated() ), this, SIGNAL( switchToRightSubTab() ) );
  connect( ctrlDel, SIGNAL( activated() ), this, SIGNAL( currentlyViewedTabCloseRequest() ) );
}

// ==========================================================================
// SC_ReforgeButtonGroup
// ==========================================================================

SC_ReforgeButtonGroup::SC_ReforgeButtonGroup( QObject* parent ):
QButtonGroup( parent ), selected( 0 )
{
  connect( this, SIGNAL( buttonToggled( int, bool ) ), this, SLOT( setSelected( int, bool ) ) );
}

void SC_ReforgeButtonGroup::setSelected( int id, bool checked )
{
  Q_UNUSED( id );
  if ( checked )
    selected++;
  else
    selected--;

  // Three selected, disallow selection of any more
  if ( selected >= 3 )
  {
    foreach ( QAbstractButton* button, buttons() )
    {
      button -> setEnabled( button -> isChecked() );
    }
  }
  // Less than three selected, allow selection of all/any
  else
  {
    foreach ( QAbstractButton* button, buttons() )
    {
      button -> setEnabled( true );
    }
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
    qint32 count = (qint32)cookies.count();
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

void SC_SingleResultTab::save_result()
{
  QString destination;
  QString defaultDestination;
  QString extension;
  switch ( currentTab() )
  {
  case TAB_HTML:
    defaultDestination = "results_html.html"; extension = "html"; break;
  case TAB_TEXT:
    defaultDestination = "results_text.txt"; extension = "txt"; break;
  case TAB_XML:
    defaultDestination = "results_xml.xml"; extension = "xml"; break;
  case TAB_PLOTDATA:
    defaultDestination = "results_plotdata.csv"; extension = "csv"; break;
  case TAB_CSV:
    defaultDestination = "results_csv.csv"; extension = "csv"; break;
  default: break;
  }
  destination = defaultDestination;
  QString savePath = mainWindow -> ResultsDestDir;
  QString commandLinePath = mainWindow -> cmdLine -> commandLineText( TAB_RESULTS );
  int fname_offset = commandLinePath.lastIndexOf( QDir::separator() );

  if ( commandLinePath.size() > 0 )
  {
    if ( fname_offset == -1 )
      destination = commandLinePath;
    else
    {
      savePath = commandLinePath.left( fname_offset + 1 );
      destination = commandLinePath.right( commandLinePath.size() - ( fname_offset + 1 ) );
    }
  }
  if ( destination.size() == 0 )
  {
    destination = defaultDestination;
  }
  if ( destination.indexOf( "." + extension ) == -1 )
    destination += "." + extension;
  QFileInfo savePathInfo( savePath );
  QFileDialog f( this );
  f.setDirectory( savePathInfo.absoluteDir() );
  f.setAcceptMode( QFileDialog::AcceptSave );
  f.setDefaultSuffix( extension );
  f.selectFile( destination );
  f.setWindowTitle( tr("Save results") );

  if ( f.exec() )
  {
    QFile file( f.selectedFiles().at( 0 ) );
    if ( file.open( QIODevice::WriteOnly | QIODevice::Text ) )
    {
      switch ( currentTab() )
      {
      case TAB_HTML:
#if defined( SC_USE_WEBKIT )
        file.write(static_cast<SC_WebView*>(currentWidget())->toHtml().toUtf8());
#else
        static_cast<SC_WebView*>(currentWidget()) -> page() -> toHtml( HtmlOutputFunctor( &file ) );
#endif /* SC_USE_WEBKIT */
        break;
      case TAB_TEXT:
      case TAB_XML:
      case TAB_PLOTDATA:
      case TAB_CSV:
        file.write( static_cast<SC_TextEdit*>( currentWidget() ) -> toPlainText().toUtf8() );
        break;
      default: break;
      }
      file.close();
      QMessageBox::information( this, tr( "Save Result" ), tr( "Result saved to %1" ).arg( file.fileName() ), QMessageBox::Ok, QMessageBox::Ok );
      mainWindow -> logText -> append( QString( tr("Results saved to: %1\n") ).arg( file.fileName() ) );
    }
  }
}

void SC_SingleResultTab::TabChanged( int /* index */ )
{

}

SC_ResultTab::SC_ResultTab( SC_MainWindow* mw ):
SC_RecentlyClosedTab( mw ),
mainWindow( mw )
{
  setMovable( true );
  enableDragHoveredOverTabSignal( true );
  connect( this, SIGNAL( mouseDragHoveredOverTab( int ) ), this, SLOT( setCurrentIndex( int ) ) );
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
