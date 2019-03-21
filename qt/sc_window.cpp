// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"
#include "simulationcraftqt.hpp"
#include "sc_OptionsTab.hpp"
#include "sc_SpellQueryTab.hpp"
#include "sc_SimulationThread.hpp"
#include "sc_SampleProfilesTab.hpp"
#include "sc_SimulateTab.hpp"
#include "sc_WelcomeTab.hpp"
#include "sc_AddonImportTab.hpp"
#include "sc_UpdateCheck.hpp"
#include "util/sc_mainwindowcommandline.hpp"
#include "util/git_info.hpp"
#if defined( Q_OS_MAC )
#include <CoreFoundation/CoreFoundation.h>
#endif
#include <QStandardPaths>
#include <QDateTime>


namespace { // UNNAMED NAMESPACE

constexpr int SC_GUI_HISTORY_VERSION = 810;

#if ! defined( SC_USE_WEBKIT )
struct HtmlOutputFunctor
{
  QString fname;

  HtmlOutputFunctor(const QString& fn) : fname( fn )
  { }

  void operator()( QString htmlOutput )
  {
    QFile file( fname );
    if (file.open(QIODevice::WriteOnly | QIODevice::Text))
    {
      QByteArray out_utf8 = htmlOutput.toUtf8();
      qint64 ret = file.write(out_utf8);
      file.close();
      assert(ret == htmlOutput.size());
      (void)ret;
    }
  }
};
#endif
} // UNNAMED NAMESPACE

// ==========================================================================
// SC_PATHS
// ==========================================================================

QStringList SC_PATHS::getDataPaths()
{
#if defined( Q_OS_WIN )
    return QStringList(QCoreApplication::applicationDirPath());
#elif defined( Q_OS_MAC )
    return QStringList(QCoreApplication::applicationDirPath() + "/../Resources");
#else
  #if !defined( SC_TO_INSTALL )
    return QStringList(QCoreApplication::applicationDirPath());
  #else
    QStringList shared_paths;
    QStringList appdatalocation =  QStandardPaths::standardLocations( QStandardPaths::DataLocation );
    for( int i = 0; i < appdatalocation.size(); ++i )
    {
      QDir dir( appdatalocation[ i ]);
        if ( dir.exists() )
        {
          shared_paths.append(dir.path());
        }
    }
    return shared_paths;
  #endif
#endif
}

// ==========================================================================
// SC_MainWindow
// ==========================================================================

void SC_MainWindow::updateSimProgress()
{
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

  cmdLine -> setSimulatingProgress( simProgress, QString::fromStdString(simPhase), QString::fromStdString(progressBarToolTip) );
  cmdLine -> setImportingProgress( importSimProgress, QString::fromStdString(importSimPhase), QString::fromStdString(progressBarToolTip) );
}

void SC_MainWindow::loadHistory()
{
  QSettings settings;
  QString apikey = settings.value( "options/apikey" ).toString();
  QString credentials_id = settings.value( "options/api_client_id" ).toString();
  QString credentials_secret = settings.value( "options/api_client_secret" ).toString();

  QVariant simulateHistory = settings.value( "user_data/simulateHistory" );
  if ( simulateHistory.isValid() )
  {
    for ( auto& entry : simulateHistory.toList() )
    {
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
  int gui_version_number = settings.value( "gui/gui_version_number", 0 ).toInt();
  if ( gui_version_number < SC_GUI_HISTORY_VERSION )
  {
    settings.clear();
    settings.setValue( "options/api_client_id", credentials_id );
    settings.setValue( "options/api_client_secret", credentials_secret );
    QMessageBox::information( this, tr("GUI settings reset"),
                              tr("We have reset your configuration settings due to major changes to the GUI") );
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

  QString cache_file = QDir::toNativeSeparators( TmpDir + "/simc_cache.dat" );
  http::cache_load( cache_file.toStdString() );

  optionsTab -> decodeOptions();
  spellQueryTab -> decodeSettings();

  if ( simulateTab -> count() <= 1 )
  { // If we haven't retrieved any simulate tabs from history, add a default one.
    simulateTab -> add_Text( defaultSimulateText, tr("Simulate!") );
  }

  // If we had an old apikey, display a popup explaining what's going on
  if ( apikey.count() )
  {
    auto deprecated_apikey = new QMessageBox( this );

    deprecated_apikey->addButton( tr( "I understand" ), QMessageBox::AcceptRole );
    deprecated_apikey->setText( tr( "Armory API key is now deprecated" ) );
    deprecated_apikey->setInformativeText( tr(
        "<p>"
          "On January 6th 2019, Blizzard is shutting down the old armory API endpoints "
          "(see <a target=\"_blank\" href=\"https://dev.battle.net/docs\">here</a> and "
          "<a target=\"_blank\" href=\"https://us.battle.net/forums/en/bnet/topic/20769317376\">here</a>). "
          "You are seeing this notification because you have a custom Armory API key defined in the "
          "Simulationcraft GUI options. To continue using the armory import feature, "
          "your Armory API key must be updated."
        "</p>"
        "<p>"
          "There are brief instructions below on how to create new credentials for Simulationcraft "
          "to continue using the armory import feature. Note that if you are also using "
          "<b>apikey.txt</b> or <b>simc_apikey</b>, you can choose to not update the GUI options, "
          "and rather add the client credentials to that file. More information can be found "
          "<a target=\"_blank\" href=\"https://github.com/simulationcraft/simc/wiki/BattleArmoryAPI\">here</a>."
        "</p>"
        "<ul>"
          "<li>Go to <a target=\"_blank\" href=\"https://develop.battle.net\">https://develop.battle.net</a></li>"
          "<li>Log in with your Battle.net account. Note that you need two-factor authentication enabled.</li>"
          "<li>Select \"API ACCESS\" (at top left), and then create a new client</li>"
          "<li>Select a unique name and click create, leave Redirect URIs empty</li>"
          "<li>Under Simulationcraft GUI options, add the client id and secret</li>"
        "</ul>" ) );

    deprecated_apikey->show();
  }

  if ( settings.value( "options/update_check" ).toString() == "Yes" ||
       !settings.contains( "options/update_check" ) )
  {
    auto updateCheck = new UpdateCheckWidget( this );
    updateCheck->start();
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

  QString cache_file = QDir::toNativeSeparators( TmpDir + "/simc_cache.dat" );
  http::cache_save( cache_file.toStdString() );

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

  settings.endGroup();// end user_data

  optionsTab -> encodeOptions();
  spellQueryTab -> encodeSettings();
}

// ==========================================================================
// Widget Creation
// ==========================================================================

SC_MainWindow::SC_MainWindow( QWidget *parent )
  : QWidget( parent ),
  visibleWebView(),
  cmdLine(),
  recentlyClosedTabModel(),
  sim(),
  import_sim(),
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
  setWindowTitle( QCoreApplication::applicationName() + " " + QCoreApplication::applicationVersion() + " " + webEngineName());
  setAttribute( Qt::WA_AlwaysShowToolTips );

#if defined( Q_OS_WIN )
  AppDataDir = QCoreApplication::applicationDirPath();
#else
  QStringList q = QStandardPaths::standardLocations( QStandardPaths::AppDataLocation );
  assert( !q.isEmpty() );
  AppDataDir = q.first();
  QDir::home().mkpath( AppDataDir );
#endif

  TmpDir = QStandardPaths::writableLocation( QStandardPaths::CacheLocation );
  assert( !TmpDir.isEmpty() );
  qDebug() << "TmpDir: " << TmpDir;

  // Set ResultsDestDir
  ResultsDestDir = QStandardPaths::writableLocation(QStandardPaths::DocumentsLocation );
  assert( !ResultsDestDir.isEmpty() );
  qDebug() << "ResultsDestDir: " << ResultsDestDir;

  logFileText = QDir::toNativeSeparators( AppDataDir + "/log.txt" );
  resultsFileText = QDir::toNativeSeparators( AppDataDir + "/results.html" );

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
#if defined( Q_OS_MAC )
  createTabShortcuts();
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
  connect( simulateThread, &SC_SimulateThread::simulationFinished, this, &SC_MainWindow::simulateFinished );

  setAcceptDrops( true );
  loadHistory();

#if defined( Q_OS_MAC ) || defined( VS_NEW_BUILD_SYSTEM )
  new BattleNetImportWindow( this );
#endif /* Q_OS_MAC || VS_NEW_BUILD_SYSTEM */
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

  connect( optionsTab, SIGNAL( armory_region_changed( const QString& ) ),
           newBattleNetView -> widget(), SLOT( armoryRegionChangedIn( const QString& ) ) );

  connect( newBattleNetView -> widget(), SIGNAL( armoryRegionChangedOut( const QString& ) ),
           optionsTab,                   SLOT( _armoryRegionChanged( const QString& ) ) );
}

void SC_MainWindow::createImportTab()
{
  importTab = new SC_ImportTab( this );
  mainTab -> addTab( importTab, tr( "Import" ) );

  newBattleNetView = new BattleNetImportWindow( this, true );
  importTab -> addTab( newBattleNetView, tr( "Battle.net" ) );

  importTab -> addTab( importTab -> addonTab, tr("Simc Addon") );

  SC_SampleProfilesTab* bisTab = new SC_SampleProfilesTab( this );
  connect( bisTab -> tree, SIGNAL( itemDoubleClicked( QTreeWidgetItem*, int ) ), this, SLOT( bisDoubleClicked( QTreeWidgetItem*, int ) ) );
  importTab -> addTab( bisTab, tr( "Sample Profiles" ) );

  recentlyClosedTabImport = new SC_RecentlyClosedTabWidget( this, QBoxLayout::LeftToRight );
  recentlyClosedTabModel = recentlyClosedTabImport -> getModel();
  importTab -> addTab( recentlyClosedTabImport, tr( "Recently Closed" ) );
  connect(recentlyClosedTabImport, SIGNAL(restoreTab(QWidget*, const QString&, const QString&, const QIcon&)),
	  this, SLOT(simulateTabRestored(QWidget*, const QString&, const QString&, const QIcon&)));

  connect(importTab, SIGNAL(currentChanged(int)), this, SLOT(importTabChanged(int)));

  // Commenting out until it is more fleshed out.
  // createCustomTab();
}


void SC_MainWindow::createCustomTab()
{
  //In Dev - Character Retrieval Boxes & Buttons
  //In Dev - Load & Save Profile Buttons
  //In Dev - Profiler Slots, Talent Layout
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
  createCustomProfileDock -> setTabText( createCustomProfileDock -> indexOf( customGearTab ), tr( "Gear", "createCustomTab" ) );
  createCustomProfileDock -> setTabToolTip( createCustomProfileDock -> indexOf( customGearTab ), tr( "Customize Gear Setup", "createCustomTab" ) );
  createCustomProfileDock -> setTabText( createCustomProfileDock -> indexOf( customTalentsTab ), tr( "Talents", "createCustomTab" ) );
  createCustomProfileDock -> setTabToolTip( createCustomProfileDock -> indexOf( customTalentsTab ), tr( "Customize Talents", "createCustomTab" ) );
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
  helpView -> setUrl( QUrl( "https://github.com/simulationcraft/simc/wiki/StartersGuide" ) );
  mainTab -> addTab( helpView, tr( "Help" ) );
}

void SC_MainWindow::createResultsTab()
{
  resultsTab = new SC_ResultTab( this );
  connect( resultsTab, SIGNAL( currentChanged( int ) ), this, SLOT( resultsTabChanged( int ) ) );
  connect( resultsTab, SIGNAL( tabCloseRequested( int ) ), this, SLOT( resultsTabCloseRequest( int ) ) );
  mainTab -> addTab( resultsTab, tr( "Results" ) );
}

void SC_MainWindow::createSpellQueryTab()
{
  spellQueryTab = new SC_SpellQueryTab( this );
  mainTab -> addTab( spellQueryTab, tr( "Spell Query" ) );
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

void SC_MainWindow::updateWebView( SC_WebView* wv )
{
  assert( wv );
  visibleWebView = wv;
  if ( cmdLine != 0 ) // can be called before widget is setup
  {
    if ( visibleWebView == helpView )
    {
      cmdLine -> setHelpViewProgress( visibleWebView -> progress, "%p%", "" );
      cmdLine -> setCommandLineText( TAB_HELP, visibleWebView -> url_to_show );
    }
  }
}

// ==========================================================================
// Sim Initialization
// ==========================================================================

std::shared_ptr<sim_t> SC_MainWindow::initSim()
{
  auto sim = std::make_shared<sim_t>();
  sim -> pause_mutex = std::unique_ptr<mutex_t>(new mutex_t());
  sim -> report_progress = 0;
#if SC_USE_PTR
  sim -> parse_option( "ptr", ( ( optionsTab -> choice.version -> currentIndex() == 1 ) ? "1" : "0" ) );
#endif
  sim -> parse_option( "debug", ( ( optionsTab -> choice.debug -> currentIndex() == 2 ) ? "1" : "0" ) );
  sim -> cleanup_threads = true;

  return sim;
}

void SC_MainWindow::deleteSim( std::shared_ptr<sim_t>& sim, SC_TextEdit* append_error_message )
{
  if ( sim )
  {
    std::list< std::string > files;
    std::vector< std::string > errorListCopy( sim -> error_list );
    files.push_back( sim -> output_file_str );
    files.push_back( sim -> html_file_str );
    files.push_back( sim -> json_file_str );
    files.push_back( sim -> reforge_plot_output_file_str );

    std::string output_file_str = sim -> output_file_str;
    bool sim_control_was_not_zero = sim -> control != 0;

    sim = nullptr;

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
        append_error_message -> appendPlainText( QString::fromStdString( suggestions ) );
        if ( windowsPermissionRecommendation.length() != 0 )
        {
          contents.append( QString::fromStdString( windowsPermissionRecommendation + "\n" ) );
        }
      }
      if ( contents.contains( "Internal server error" ) )
      {
        contents.append( "\nAn Internal server error means an error occurred with the server, trying again later should fix it\n" );
      }
      contents.append( "\nIf for some reason you cannot resolve this issue, check if it is a known issue at\n https://github.com/simulationcraft/simc/issues\n" );
      contents.append( "Or try an older version\n http://www.simulationcraft.org/download.html\n" );
      contents.append( "And if all else fails you can come talk to us on IRC at\n irc.stratics.com (#simulationcraft)\n" );
    }

    // If requested, append the error message to the given Text Widget as well.
    if ( append_error_message )
    {
      append_error_message -> appendPlainText( contents );
    }
    if ( logText != append_error_message )
    {
      logText -> appendPlainText( contents );
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

void SC_MainWindow::startNewImport( const QString& region, const QString& realm, const QString& character, const QString& specialization )
{
  if ( importRunning() )
  {
    stopImport();
    return;
  }
  simProgress = 0;
  import_sim = initSim();
  importThread -> start( import_sim, region, realm, character, specialization );
  simulateTab -> add_Text( defaultSimulateText, tr( "Importing" ) );
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

    QString label = QString::fromStdString( importThread -> player -> name_str );
    while ( label.size() < 20 ) label += ' ';
    label += QString::fromStdString( importThread -> player -> origin_str );
    if ( label.contains( ".api." ) )
    { // Strip the s out of https and the api. out of the string so that it is a usable link.
      label.replace( QString( ".api" ), QString( "" ) );
      label.replace( QString( "https"), QString( "http" ) );
    }
    deleteSim( import_sim );
  }
  else
  {
    simulateTab -> setTabText( simulateTab -> currentIndex(), tr( "Import Failed" ) );
    simulateTab -> append_Text( tr("# Unable to generate profile from: ") + importThread -> url + "\n" );
    if (!importThread->error.isEmpty())
    {
        simulateTab -> append_Text( QString( "# ") + importThread->error );
    }

    for( const std::string& error : import_sim -> error_list )
    {
        simulateTab -> append_Text( QString( "# ") + QString::fromStdString( error ) );
    }
    deleteSim( import_sim, simulateTab -> current_Text() );
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

  // Clear log text on success so users don't get confused if there is
  // an error from previous attempts
  if ( consecutiveSimulationsRun == 0 ) // Only clear on first queued sim
  {
    logText -> clear();
  }
  simProgress = 0;
  sim = initSim();

  auto value = simulationQueue.dequeue();

  QString tab_name = std::get<0>( value );

  // Build combined input profile
  auto simc_gui_profile = std::get<1>( value );
  QString simc_version;
  if ( !git_info::available())
  {
    simc_version = QString("### SimulationCraft %1 for World of Warcraft %2 %3 (wow build %4) ###\n").
        arg(SC_VERSION).arg(sim->dbc.wow_version()).arg(sim->dbc.wow_ptr_status()).arg(sim->dbc.build_level());
  }
  else
  {
    simc_version = QString("### SimulationCraft %1 for World of Warcraft %2 %3 (wow build %4, git build %5 %6) ###\n").
            arg(SC_VERSION).arg(sim->dbc.wow_version()).arg(sim->dbc.wow_ptr_status()).arg(sim->dbc.build_level()).
            arg(git_info::branch()).arg(git_info::revision());
  }
  QString gui_version = QString("### Using QT %1 with %2 ###\n\n").arg(QTCORE_VERSION_STR).arg(webEngineName());
  simc_gui_profile = simc_version + gui_version + simc_gui_profile;
  QByteArray utf8_profile = simc_gui_profile.toUtf8();

  // Save profile to simc_gui.simc
  QFile file( AppDataDir + QDir::separator() + "simc_gui.simc" );
  if ( file.open( QIODevice::WriteOnly | QIODevice::Text ) )
  {
    file.write( utf8_profile );
    file.close();
  }

  QString reportFileBase = QDir::toNativeSeparators( optionsTab -> getReportlDestination() );
  sim -> output_file_str = (reportFileBase + ".txt").toStdString();
  sim -> html_file_str = (reportFileBase + ".html").toStdString();
  sim -> json_file_str = (reportFileBase + ".json").toStdString();

  //sim -> xml_file_str = (reportFileBase + ".xml").toStdString();
  sim -> reforge_plot_output_file_str = (reportFileBase + "_plotdata.csv").toStdString();

  if ( optionsTab -> get_api_key().size() == 65 ) // api keys are 32 characters long, it's not worth parsing <32 character keys.
    sim -> parse_option( "apikey",  optionsTab -> get_api_key().toUtf8().constData() );

  if ( cmdLine -> currentState() != SC_MainWindowCommandLine::SIMULATING_MULTIPLE )
  {
    cmdLine -> setState( SC_MainWindowCommandLine::SIMULATING );
  }
  simulateThread -> start( sim, utf8_profile, tab_name );

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

void SC_MainWindow::simulateFinished( std::shared_ptr<sim_t> sim )
{
  simPhase = "%p%";
  simProgress = 100;
  cmdLine -> setSimulatingProgress( simProgress, QString::fromStdString(simPhase), tr( "Finished!" ) );
  bool sim_was_debug = sim -> debug || sim -> log;

  logText -> clear();
  if ( ! simulateThread -> success )
  {
      // Spell Query ( is no not an error )
      if ( mainTab -> currentTab() == TAB_SPELLQUERY )
      {
        QString result;
        std::stringstream ss;
        try
        {
          sim -> spell_query -> evaluate();
          report::print_spell_query( ss, *sim, *sim -> spell_query, sim -> spell_query_level );
        }
        catch ( const std::exception& e ){
          ss << "ERROR! Spell Query failure: " << e.what() << std::endl;
        }
        result = QString::fromStdString( ss.str() );
        if ( result.isEmpty() )
        {
          result = "No results found!";
        }
        spellQueryTab -> textbox.result -> setPlainText( result );
        spellQueryTab -> checkForSave();
      }
      else
      {
        logText -> moveCursor( QTextCursor::End );
        if ( mainTab -> currentTab() != TAB_SPELLQUERY )
          mainTab -> setCurrentTab( TAB_LOG );

        QString errorText;
        errorText += tr("SimulationCraft encountered an error!");
        errorText += "<br><br>";
        errorText += "<i>" + simulateThread -> getErrorCategory() + ":</i>";
        errorText += "<br>";
        errorText += "<b>" + simulateThread -> getError().replace("\n", "<br>") + "</b>";
        errorText += "<br><br>";
        errorText += tr("If you think this is a bug, please open a ticket, copying the detailed text below.<br>"
                        "It contains all the input options of the last simulation"
                        " and helps us reproduce the issue.<br>");

        // Build error details which users can copy-paste into a ticket
        // Use simulation options used, same as goes to simc_gui.simc
        QString errorDetails;
        errorDetails += "# SimulationCraft encountered an error!\n";
        errorDetails += QString("* Category: %1\n").arg(simulateThread -> getErrorCategory());
        errorDetails += QString("* Error: %1\n\n").arg(simulateThread -> getError());
        errorDetails += "## All options used for simulation:\n";
        errorDetails += "``` ini\n"; // start code section
        errorDetails += simulateThread -> getOptions();
        errorDetails += "\n```\n"; // end code section

        QMessageBox mBox;
        mBox.setWindowTitle(simulateThread -> getErrorCategory());
        mBox.setText( errorText );
        mBox.setDetailedText( errorDetails );
        mBox.exec();

        // print to log as well.
        logText -> appendHtml( errorText );
        logText -> appendPlainText( errorDetails );
}

  }
  else
  {
    QDateTime now = QDateTime::currentDateTime();
    now.setOffsetFromUtc(now.offsetFromUtc());
    QString datetime = now.toString(Qt::ISODate);
    QString resultsName = simulateThread -> getTabName();
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
    int index = resultsTab -> addTab( resultsEntry, resultsName );
    resultsTab -> setTabToolTip( index, datetime );
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
    if ( html_file.open( QIODevice::ReadOnly ) )
    {
      resultsHtmlView -> out_html = html_file.readAll();
      html_file.close();
    }
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

    // JSON
    SC_TextEdit* resultsJSONView = new SC_TextEdit( resultsEntry );
    resultsEntry -> addTab( resultsJSONView, "JSON" );

    QFile json_file( sim -> json_file_str.c_str() );
    if ( json_file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
      resultsJSONView -> appendPlainText( json_file.readAll() );
      json_file.close();
    }
    else
    {
      resultsJSONView -> setPlainText( tr( "Error opening %1. %2" ).arg( sim -> json_file_str.c_str(), json_file.errorString() ) );
    }

    // Plot Data
    SC_TextEdit* resultsPlotView = new SC_TextEdit( resultsEntry );
    resultsEntry -> addTab( resultsPlotView, tr("plot data") );
    QFile plot_file( sim -> reforge_plot_output_file_str.c_str() );
    if ( plot_file.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
      resultsPlotView -> appendPlainText( plot_file.readAll() );
      plot_file.close();
    }
    else
    {
        resultsPlotView -> setPlainText( tr( "Error opening %1. %2" ).arg( sim -> reforge_plot_output_file_str.c_str(), plot_file.errorString() ) );
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

  logText->appendPlainText( QString( tr("Log saved to: %1\n") ).arg( cmdLine -> commandLineText( TAB_LOG ) ) );
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
  default: break;
  }
}

void SC_MainWindow::cmdLineReturnPressed()
{
  if ( !sim ) cmdLine -> mainButtonClicked();
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
			case TAB_RECENT:     recentlyClosedTabImport -> restoreCurrentlySelected(); break;
                        default: break;
		}
		break;
	  case TAB_LOG: saveLog(); break;
	  case TAB_RESULTS: saveResults(); break;
	  case TAB_SPELLQUERY: spellQueryTab -> run_spell_query(); break;
	  case TAB_COUNT: break;
          default: break;
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
	  case TAB_RECENT:     recentlyClosedTabImport -> restoreCurrentlySelected(); break;
	  case TAB_ADDON:
	  {
		  QString profile = importTab -> addonTab -> toPlainText();
		  simulateTab -> add_Text( profile,  tr( "SimC Addon Import" ) );
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
    default: break;
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
  visibleWebView = nullptr;
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
  case TAB_WELCOME:		break;
  case TAB_OPTIONS:		break;
  case TAB_SIMULATE:	break;
  case TAB_OVERRIDES:	break;
  case TAB_HELP:		break;
  case TAB_LOG:       cmdLine -> setCommandLineText( TAB_LOG, logFileText ); break;
  case TAB_IMPORT:
    importTabChanged( importTab->currentIndex() );
    break;
  case TAB_RESULTS:
    resultsTabChanged( resultsTab -> currentIndex() );
    break;
  case TAB_SPELLQUERY:
    break;
  default: assert( 0 ); break;
  }
}

void SC_MainWindow::importTabChanged( int index )
{
  if ( index == TAB_BIS ||
       index == TAB_RECENT ||
       index == TAB_ADDON ||
       index == TAB_IMPORT_NEW )
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
  QMessageBox msgBox( QMessageBox::Question, tr( "Close Result Tab" ), tr( "Do you really want to close this result?" ), QMessageBox::Yes | QMessageBox::No );
  auto close_all_button = msgBox.addButton(tr("Close permanently"), QMessageBox::YesRole);
  int confirm = msgBox.exec();
  bool close_permanently = msgBox.clickedButton() == close_all_button;
  if ( confirm == QMessageBox::Yes || close_permanently )
  {
    auto tab = static_cast <SC_SingleResultTab*>(resultsTab -> widget( index ));
    resultsTab -> removeTab( index, close_permanently );
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
  if ( !file.open( QIODevice::WriteOnly ) )
  {
    return;
  }

  QDataStream out( &file );
  const QList<QNetworkCookie>& cookies = allCookies();
  out << static_cast<int>(cookies.count());
  for ( const auto& cookie : cookies )
  {
    out << cookie.name();
    out << cookie.value();
  }
}

void PersistentCookieJar::load()
{
  QFile file( fileName );
  if ( !file.open( QIODevice::ReadOnly ) )
  {
    return;
  }

  QDataStream in( &file );
  QList<QNetworkCookie> cookies;
  int count;
  in >> count;
  for ( int i = 0; i < count; i++ )
  {
    QByteArray name, value;
    in >> name;
    in >> value;
    cookies.append( QNetworkCookie( name, value ) );
  }
  setAllCookies( cookies );
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
  case TAB_JSON:
    defaultDestination = "results_json.json"; extension = "json"; break;
  case TAB_PLOTDATA:
    defaultDestination = "results_plotdata.csv"; extension = "csv"; break;
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
    switch ( currentTab() )
    {
    case TAB_HTML:
    {
      QFile file( f.selectedFiles().at( 0 ) );
      if ( file.open( QIODevice::WriteOnly | QIODevice::Text ) )
      {
        file.write( debug_cast<SC_WebView*>( currentWidget() )->out_html );
        file.close();
      }
      break;
    }
      case TAB_TEXT:	break;
      case TAB_JSON:	break;
      case TAB_PLOTDATA:break;
      case TAB_CSV:
      {
        QFile file(f.selectedFiles().at(0));
        if (file.open(QIODevice::WriteOnly | QIODevice::Text))
        {
          file.write(static_cast<SC_TextEdit*>(currentWidget())->toPlainText().toUtf8());
          file.close();
        }
        break;
      }
    }
    QMessageBox::information( this, tr( "Save Result" ), tr( "Result saved to %1" ).arg( f.selectedFiles().at( 0 )), QMessageBox::Ok, QMessageBox::Ok );
    mainWindow -> logText -> appendPlainText( QString( tr("Results saved to: %1\n") ).arg( f.selectedFiles().at( 0 )) );
  }
}

void SC_SingleResultTab::TabChanged( int /* index */ )
{

}

SC_ResultTab::SC_ResultTab( SC_MainWindow* mw ):
  SC_RecentlyClosedTab( mw ),
  mainWindow( mw )
{

  setTabsClosable( true );
  setMovable( true );
  enableDragHoveredOverTabSignal( true );
  connect( this, SIGNAL( mouseDragHoveredOverTab( int ) ), this, SLOT( setCurrentIndex( int ) ) );
}
