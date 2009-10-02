// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraftqt.h"
#include <QtWebKit>

// ==========================================================================
// Utilities
// ==========================================================================

static QComboBox* createChoice( int count, ... )
{
  QComboBox* choice = new QComboBox();
  va_list vap;
  va_start( vap, count );
  for ( int i=0; i < count; i++ )
  {
    const char* s = ( char* ) va_arg( vap, char* );
    choice->addItem( s );
  }
  va_end( vap );
  return choice;
}

void SimcraftWindow::decodeGlobals( QString encoding )
{
  QStringList tokens = encoding.split( ' ' );
  if( tokens.count() == 8 )
  {
           patchChoice->setCurrentIndex( tokens[ 0 ].toInt() );
         latencyChoice->setCurrentIndex( tokens[ 1 ].toInt() );
      iterationsChoice->setCurrentIndex( tokens[ 2 ].toInt() );
     fightLengthChoice->setCurrentIndex( tokens[ 3 ].toInt() );
      fightStyleChoice->setCurrentIndex( tokens[ 4 ].toInt() );
    scaleFactorsChoice->setCurrentIndex( tokens[ 5 ].toInt() );
         threadsChoice->setCurrentIndex( tokens[ 6 ].toInt() );
      armorySpecChoice->setCurrentIndex( tokens[ 7 ].toInt() );
  }
}

QString SimcraftWindow::encodeGlobals()
{
  return QString( "%1 %2 %3 %4 %5 %6 %7 %8" )
    .arg(        patchChoice->currentIndex() )
    .arg(      latencyChoice->currentIndex() )
    .arg(   iterationsChoice->currentIndex() )
    .arg(  fightLengthChoice->currentIndex() )
    .arg(   fightStyleChoice->currentIndex() )
    .arg( scaleFactorsChoice->currentIndex() )
    .arg(      threadsChoice->currentIndex() )
    .arg(   armorySpecChoice->currentIndex() );
}

void SimcraftWindow::updateSimProgress()
{
  if( sim )
  {
    simProgress = (int) 100 * sim->progress();
  }
  else
  {
    simProgress = 100;
  }
  if( mainTab->currentIndex() != TAB_IMPORT &&
      mainTab->currentIndex() != TAB_RESULTS )
  {
    progressBar->setValue( simProgress );
  }
}

void SimcraftWindow::loadHistory()
{
  http_t::cache_load();
  QFile file( "simcraft_history.dat" );
  if( file.open( QIODevice::ReadOnly ) )
  {
    QStringList importHistory;
    QDataStream in( &file );
    in >> simulateCmdLineHistory;
    in >> logCmdLineHistory;
    in >> resultsCmdLineHistory;
    in >> globalsHistory;
    in >> simulateTextHistory;
    in >> overridesTextHistory;
    in >> importHistory;
    file.close();

    int count = importHistory.count();
    for( int i=0; i < count; i++ )
    {
      QListWidgetItem* item = new QListWidgetItem( importHistory.at( i ) );
      item->setFont( QFont( "fixed" ) );
      historyList->addItem( item );
    }

    decodeGlobals( globalsHistory.backwards() );
  }
}

void SimcraftWindow::saveHistory()
{
  http_t::cache_save();
  QFile file( "simcraft_history.dat" );
  if( file.open( QIODevice::WriteOnly ) )
  {
    globalsHistory.add( encodeGlobals() );

    QStringList importHistory;
    int count = historyList->count();
    for( int i=0; i < count; i++ ) 
    {
      importHistory.append( historyList->item( i )->text() );
    }

    QDataStream out( &file );
    out << simulateCmdLineHistory;
    out << logCmdLineHistory;
    out << resultsCmdLineHistory;
    out << globalsHistory;
    out << simulateTextHistory;
    out << overridesTextHistory;
    out << importHistory;
    file.close();
  }
}

// ==========================================================================
// Widget Creation
// ==========================================================================

SimcraftWindow::SimcraftWindow(QWidget *parent)
  : QWidget(parent), sim(0), simProgress(100), simResults(0)
{
  cmdLineText = "";
  logFileText = "log.txt";
  resultsFileText = "results.html";

  mainTab = new QTabWidget();
  createWelcomeTab();
  createGlobalsTab();
  createImportTab();
  createSimulateTab();
  createOverridesTab();
  createLogTab();
  createResultsTab();
  createCmdLine();
  connect( mainTab, SIGNAL(currentChanged(int)), this, SLOT(mainTabChanged(int)) );
  
  QVBoxLayout* vLayout = new QVBoxLayout();
  vLayout->addWidget( mainTab );
  vLayout->addWidget( cmdLineGroupBox );
  setLayout( vLayout );
  
  timer = new QTimer( this );
  connect( timer, SIGNAL(timeout()), this, SLOT(updateSimProgress()) );

  importThread = new ImportThread( this );
  simulateThread = new SimulateThread( this );

  connect(   importThread, SIGNAL(finished()), this, SLOT(  importFinished()) );
  connect( simulateThread, SIGNAL(finished()), this, SLOT(simulateFinished()) );

  loadHistory();
}

void SimcraftWindow::createCmdLine()
{
  QHBoxLayout* cmdLineLayout = new QHBoxLayout();
  cmdLineLayout->addWidget( backButton = new QPushButton( "<" ) );
  cmdLineLayout->addWidget( forwardButton = new QPushButton( ">" ) );
  cmdLineLayout->addWidget( cmdLine = new SimcraftCommandLine( this ) );
  cmdLineLayout->addWidget( progressBar = new QProgressBar() );
  cmdLineLayout->addWidget( mainButton = new QPushButton( "Simulate!" ) );
  backButton->setMaximumWidth( 30 );
  forwardButton->setMaximumWidth( 30 );
  progressBar->setMaximum( 100 );
  progressBar->setMaximumWidth( 100 );
  connect( backButton,    SIGNAL(clicked(bool)),   this, SLOT(    backButtonClicked()) );
  connect( forwardButton, SIGNAL(clicked(bool)),   this, SLOT( forwardButtonClicked()) );
  connect( mainButton,    SIGNAL(clicked(bool)),   this, SLOT(    mainButtonClicked()) );
  connect( cmdLine,       SIGNAL(returnPressed()),            this, SLOT( cmdLineReturnPressed()) );
  connect( cmdLine,       SIGNAL(textEdited(const QString&)), this, SLOT( cmdLineTextEdited(const QString&)) );
  cmdLineGroupBox = new QGroupBox();
  cmdLineGroupBox->setLayout( cmdLineLayout );
}

void SimcraftWindow::createWelcomeTab()
{
  const char* s =
    "<div align=center><h1>Welcome to SimulationCraft!</h1></div>"
    "<dl>"
    "<dt><h4>Overview</h4></dt>"
    "<dd>Some description.</dd>"
    "<dt><h4>Importing profiles</h4></dt>"
    "<dd>Some description.</dd>"
    "</dl>";

  QTextBrowser* welcomeBanner = new QTextBrowser();
  welcomeBanner->document()->setHtml( s );
  mainTab->addTab( welcomeBanner, "Welcome" );
}
 
void SimcraftWindow::createGlobalsTab()
{
  QFormLayout* globalsLayout = new QFormLayout();
  globalsLayout->setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );
  globalsLayout->addRow(         "Patch",        patchChoice = createChoice( 2, "3.2.0", "3.2.2" ) );
  globalsLayout->addRow(       "Latency",      latencyChoice = createChoice( 2, "Low", "High" ) );
  globalsLayout->addRow(    "Iterations",   iterationsChoice = createChoice( 3, "100", "1000", "10000" ) );
  globalsLayout->addRow(  "Length (sec)",  fightLengthChoice = createChoice( 3, "100", "300", "500" ) );
  globalsLayout->addRow(   "Fight Style",   fightStyleChoice = createChoice( 2, "Patchwerk", "Helter Skelter" ) );
  globalsLayout->addRow( "Scale Factors", scaleFactorsChoice = createChoice( 2, "No", "Yes" ) );
  globalsLayout->addRow(       "Threads",      threadsChoice = createChoice( 4, "1", "2", "4", "8" ) );
  globalsLayout->addRow(   "Armory Spec",   armorySpecChoice = createChoice( 2, "active", "inactive" ) );
  patchChoice->setCurrentIndex( 1 );
  iterationsChoice->setCurrentIndex( 1 );
  fightLengthChoice->setCurrentIndex( 1 );
  QGroupBox* globalsGroupBox = new QGroupBox();
  globalsGroupBox->setLayout( globalsLayout );
  mainTab->addTab( globalsGroupBox, "Globals" );
}

void SimcraftWindow::createImportTab()
{
  importTab = new QTabWidget();
  mainTab->addTab( importTab, "Import" );

  armoryUsView = new SimcraftWebView( this );
  armoryUsView->setUrl( QUrl( "http://us.wowarmory.com" ) );
  importTab->addTab( armoryUsView, "Armory-US" );
  
  armoryEuView = new SimcraftWebView( this );
  armoryEuView->setUrl( QUrl( "http://eu.wowarmory.com" ) );
  importTab->addTab( armoryEuView, "Armory-EU" );
  
  wowheadView = new SimcraftWebView( this );
  wowheadView->setUrl( QUrl( "http://www.wowhead.com/?profiles" ) );
  importTab->addTab( wowheadView, "Wowhead" );
  
  chardevView = new SimcraftWebView( this );
  chardevView->setUrl( QUrl( "http://www.chardev.org" ) );
  importTab->addTab( chardevView, "CharDev" );
  
  warcrafterView = new SimcraftWebView( this );
  warcrafterView->setUrl( QUrl( "http://www.warcrafter.net" ) );
  importTab->addTab( warcrafterView, "Warcrafter" );

  QVBoxLayout* rawrLayout = new QVBoxLayout();
  QLabel* rawrLabel = new QLabel( " http://rawr.codeplex.com\n\n"
				  "Rawr is an exceptional theorycrafting tool that excels at gear optimization."
				  " The key architectural difference between Rawr and SimulationCraft is one of"
				  " formulation vs simulation.  There are strengths and weaknesses to each"
				  " approach.  Since they come from different directions, one can be confident"
				  " in the result when they arrive at the same destination.\n\n"
				  " SimulationCraft can import the character xml file written by Rawr:" );
  rawrLabel->setWordWrap( true );
  rawrLayout->addWidget( rawrLabel );
  rawrLayout->addWidget( rawrFile = new QLineEdit() );
  rawrLayout->addWidget( new QLabel( "" ), 1 );
  QGroupBox* rawrGroupBox = new QGroupBox();
  rawrGroupBox->setLayout( rawrLayout );
  importTab->addTab( rawrGroupBox, "Rawr" );

  createBestInSlotTab();

  historyList = new QListWidget();
  historyList->setSortingEnabled( true );
  importTab->addTab( historyList, "History" );

  connect( historyList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(historyDoubleClicked(QListWidgetItem*)) );
  connect( importTab, SIGNAL(currentChanged(int)), this, SLOT(importTabChanged(int)) );
}

void SimcraftWindow::createBestInSlotTab()
{
  QStringList headerLabels( "Player Class" ); headerLabels += QString( "URL" );

  bisTree = new QTreeWidget();
  bisTree->setColumnCount( 2 );
  bisTree->setHeaderLabels( headerLabels );
  importTab->addTab( bisTree, "BiS" );

  int T8=0, T9=1, TMAX=2;
  const char* TNAMES[] = { "T8", "T9" };
  QTreeWidgetItem* rootItems[ PLAYER_MAX ][ TMAX ];
  for( int i=DEATH_KNIGHT; i <= WARRIOR; i++ )
  {
    QTreeWidgetItem* top = new QTreeWidgetItem( QStringList( util_t::player_type_string( i ) ) );
    top->setFont( 0, QFont( "fixed" ) );
    bisTree->addTopLevelItem( top );
    for( int j=0; j < TMAX; j++ )
    {
      top->addChild( rootItems[ i ][ j ] = new QTreeWidgetItem( QStringList( TNAMES[ j ] ) ) );
      rootItems[ i ][ j ]->setFont( 0, QFont( "fixed" ) );
    }
  }

  struct bis_entry_t { int type; int tier; const char* name; int site; const char* id; };

  bis_entry_t bisList[] = 
    {
      { PRIEST, T8, "Priest_T8_13_00_58",   TAB_WOWHEAD, "13000014" },
      { ROGUE,  T8, "Rogue_T8_15_51_05_FD", TAB_WOWHEAD, "13006241" },
      { SHAMAN, T9, "Shaman_T9_57_14_00",   TAB_WOWHEAD, "14165873" },
      { PLAYER_NONE, TMAX, NULL, -1, NULL }
    };

  for( int i=0; bisList[ i ].type != PLAYER_NONE; i++ )
  {
    bis_entry_t& b = bisList[ i ];
    QString url = "";
    switch( b.site )
    {
    case TAB_ARMORY_US: url += "http://us.wowarmory.com/character-sheet.xml?"; break;
    case TAB_ARMORY_EU: url += "http://eu.wowarmory.com/character-sheet.xml?"; break;
    case TAB_WOWHEAD:   url += "http://www.wowhead.com/?profile=";             break;
    }
    url += b.id;
    QStringList labels = QStringList( b.name ); labels += url;
    QTreeWidgetItem* item = new QTreeWidgetItem( labels );
    item->setFont( 0, QFont( "fixed" ) );
    item->setFont( 1, QFont( "fixed" ) );
    rootItems[ b.type ][ b.tier ]->addChild( item );
  }

  connect( bisTree, SIGNAL(itemDoubleClicked(QTreeWidgetItem*,int)), this, SLOT(bisDoubleClicked(QTreeWidgetItem*,int)) );
}

void SimcraftWindow::createSimulateTab()
{
  simulateText = new QPlainTextEdit();
  simulateText->setLineWrapMode( QPlainTextEdit::NoWrap );
  simulateText->document()->setDefaultFont( QFont( "fixed" ) );
  simulateText->document()->setPlainText( "# Profile will be downloaded into here." );
  mainTab->addTab( simulateText, "Simulate" );
}

void SimcraftWindow::createOverridesTab()
{
  overridesText = new QPlainTextEdit();
  overridesText->setLineWrapMode( QPlainTextEdit::NoWrap );
  overridesText->document()->setDefaultFont( QFont( "fixed" ) );
  overridesText->appendPlainText( "# User-specified persistent global and player parms will set here.." );
  mainTab->addTab( overridesText, "Overrides" );
}

void SimcraftWindow::createLogTab()
{
  logText = new QPlainTextEdit();
  logText->setLineWrapMode( QPlainTextEdit::NoWrap );
  logText->document()->setDefaultFont( QFont( "fixed" ) );
  logText->setReadOnly(true);
  mainTab->addTab( logText, "Log" );
}

void SimcraftWindow::createResultsTab()
{
  resultsTab = new QTabWidget();
  //resultsTab->setTabsClosable( true );
  connect( resultsTab, SIGNAL(currentChanged(int)), this, SLOT(resultsTabChanged(int)) );
  mainTab->addTab( resultsTab, "Results" );
}

// ==========================================================================
// Sim Initialization
// ==========================================================================

sim_t* SimcraftWindow::initSim()
{
  if( ! sim ) 
  {
    sim = new sim_t();
    sim -> output_file = fopen( "simcraft_log.txt", "w" );
    sim -> seed = (int) time( NULL );
    sim -> report_progress = 0;
  }
  return sim;
}

void SimcraftWindow::deleteSim()
{
  if( sim ) 
  {
    fclose( sim -> output_file );
    delete sim;
    sim = 0;
    QFile logFile( "simcraft_log.txt" );
    logFile.open( QIODevice::ReadOnly );
    logText->appendPlainText( logFile.readAll() );
    logFile.close();
  }
}

// ==========================================================================
// Import 
// ==========================================================================

void ImportThread::importArmoryUs()
{
  QString server, character;
  QStringList tokens = url.split( QRegExp( "[?&=]" ), QString::SkipEmptyParts );
  int count = tokens.count();
  for( int i=0; i < count-1; i++ ) 
  {
    QString& t = tokens[ i ];
    if( t == "r" )
    {
      server = tokens[ i+1 ];
    }
    else if( t == "n" || t == "cn" )
    {
      character = tokens[ i+1 ];
    }
  }
  if( server.isEmpty() || character.isEmpty() )
  {
    fprintf( sim->output_file, "Unable to determine Server and Character information!\n" );
  }
  else
  {
    std::string talents = mainWindow->armorySpecChoice->currentText().toStdString();
    player = armory_t::download_player( sim, "us", server.toStdString(), character.toStdString(), talents );
  }
}

void ImportThread::importArmoryEu()
{
  QString server, character;
  QStringList tokens = url.split( QRegExp( "[?&=#]" ), QString::SkipEmptyParts );
  int count = tokens.count();
  for( int i=0; i < count-1; i++ ) 
  {
    QString& t = tokens[ i ];
    if( t == "r" )
    {
      server = tokens[ i+1 ];
    }
    else if( t == "n" || t == "cn" )
    {
      character = tokens[ i+1 ];
    }
  }
  if( server.isEmpty() || character.isEmpty() )
  {
    fprintf( sim->output_file, "Unable to determine Server and Character information!\n" );
  }
  else
  {
    std::string talents = mainWindow->armorySpecChoice->currentText().toStdString();
    player = armory_t::download_player( sim, "eu", server.toStdString(), character.toStdString(), talents );
  }
}

void ImportThread::importWowhead()
{
  QString id;
  QStringList tokens = url.split( QRegExp( "[?&=]" ), QString::SkipEmptyParts );
  int count = tokens.count();
  for( int i=0; i < count-1; i++ ) 
  {
    if( tokens[ i ] == "profile" )
    {
      id = tokens[ i+1 ];
    }
  }
  if( id.isEmpty() )
  {
    fprintf( sim->output_file, "Import from Wowhead requires use of Profiler!\n" );
  }
  else
  {
    tokens = id.split( '.', QString::SkipEmptyParts );
    count = tokens.count();
    if( count == 1 )
    {
      player = wowhead_t::download_player( sim, id.toStdString() );
    }
    else if( count == 3 )
    {
      player = wowhead_t::download_player( sim, tokens[ 0 ].toStdString(), tokens[ 1 ].toStdString(), tokens[ 2 ].toStdString() ); 
    }
  }
}

void ImportThread::importChardev()
{
  fprintf( sim->output_file, "Support for Chardev profiles does not yet exist!\n" );
}

void ImportThread::importWarcrafter()
{
  fprintf( sim->output_file, "Support for Warcrafter profiles does not yet exist!\n" );
}

void ImportThread::importRawr()
{
  player = rawr_t::load_player( sim, url.toStdString() );
}

void ImportThread::run()
{
  if     ( url.count( "us.wowarmory" ) ) importArmoryUs();
  else if( url.count( "eu.wowarmory" ) ) importArmoryEu();
  else if( url.count( "wowhead"      ) ) importWowhead();
  else if( url.count( "chardev"      ) ) importChardev();
  else if( url.count( "warcrafter"   ) ) importWarcrafter();
  else importRawr();

  if( player )
  {
    sim->init();
    std::string buffer="";
    player->create_profile( buffer );
    profile = buffer.c_str();
  }
}

void SimcraftWindow::startImport( const QString& url )
{
  if( sim )
  {
    sim -> cancel();
    return;
  }
  simProgress = 0;
  mainButton->setText( "Cancel!" );
  importThread->start( initSim(), url );
  simulateText->document()->setPlainText( "# Profile will be downloaded into here." );
  mainTab->setCurrentIndex( TAB_SIMULATE );
  timer->start( 500 );
}

void SimcraftWindow::importFinished()
{
  timer->stop();
  simProgress = 100;
  progressBar->setValue( 100 );
  if ( importThread->player )
  {
    simulateText->setPlainText( importThread->profile );
    simulateTextHistory.add( importThread->profile );

    QString label = importThread->player->name_str.c_str();
    while( label.size() < 20 ) label += " ";
    label += importThread->player->origin_str.c_str();

    bool found = false;
    for( int i=0; i < historyList->count() && ! found; i++ )
      if( historyList->item( i )->text() == label )
	found = true;

    if( ! found )
    {
      QListWidgetItem* item = new QListWidgetItem( label );
      item->setFont( QFont( "fixed" ) );

      historyList->addItem( item );
      historyList->sortItems();
    }
  }
  else
  {
    simulateText->setPlainText( QString( "# Unable to generate profile from: " ) + importThread->url );
  }
  deleteSim();
  mainButton->setText( "Simulate!" );
  mainTab->setCurrentIndex( TAB_SIMULATE );
}

// ==========================================================================
// Simulate
// ==========================================================================

void SimulateThread::run()
{
  sim -> html_file_str = "simcraft_report.html";

  QStringList stringList = options.split( '\n', QString::SkipEmptyParts );

  int argc = stringList.count() + 1;
  char** argv = new char*[ argc ];

  QList<QByteArray> lines;
  lines.append( "simcraft" );
  for( int i=1; i < argc; i++ ) lines.append( stringList[ i-1 ].toAscii() );
  for( int i=0; i < argc; i++ ) argv[ i ] = lines[ i ].data();

  sim -> parse_options( argc, argv );
  sim -> execute();
  sim -> scaling -> analyze();

  report_t::print_suite( sim );
}

void SimcraftWindow::startSim()
{
  if( sim )
  {
    sim -> cancel();
    return;
  }
  globalsHistory.add( encodeGlobals() );
  globalsHistory.current_index = 0;
  simulateTextHistory.add(  simulateText->document()->toPlainText() );
  overridesTextHistory.add( overridesText->document()->toPlainText() );
  simulateCmdLineHistory.add( cmdLine->text() );
  simProgress = 0;
  mainButton->setText( "Cancel!" );
  simulateThread->start( initSim(), mergeOptions() ); 
  cmdLineText = "";
  cmdLine->setText( cmdLineText );
  timer->start( 500 );
}

QString SimcraftWindow::mergeOptions()
{
  QString options = "";
  options += "patch=" + patchChoice->currentText() + "\n";
  if( latencyChoice->currentText() == "Low" )
  {
    options += "queue_lag=0.075  gcd_lag=0.150  channel_lag=0.250\n";
  }
  else
  {
    options += "queue_lag=0.150  gcd_lag=0.300  channel_lag=0.500\n";
  }
  options += "iterations=" + iterationsChoice->currentText() + "\n";
  options += "max_time=" + fightLengthChoice->currentText() + "\n";
  if( fightStyleChoice->currentText() == "Helter Skelter" )
  {
    options += "raid_events=casting,cooldown=30,first=15/movement,cooldown=30,duration=6/stun,cooldown=60,duration=3\n";
  }
  if( scaleFactorsChoice->currentText() == "Yes" )
  { 
    options += "calculate_scale_factors=1\n";
  }
  options += "threads=" + threadsChoice->currentText() + "\n";
  options += simulateText->document()->toPlainText();
  options += "\n";
  options += overridesText->document()->toPlainText();
  options += "\n";
  options += cmdLine->text();
  options += "\n";
  return options;
}

void SimcraftWindow::simulateFinished()
{
  timer->stop();
  simProgress = 100;
  progressBar->setValue( 100 );
  QFile file( sim->html_file_str.c_str() );
  deleteSim();
  if( file.open( QIODevice::ReadOnly ) )
  {
    QString resultsName = QString( "Results %1" ).arg( ++simResults );
    SimcraftWebView* resultsView = new SimcraftWebView( this );
    resultsHtml.append( file.readAll() );
    resultsView->setHtml( resultsHtml.last() );
    resultsTab->addTab( resultsView, resultsName );
    resultsTab->setCurrentWidget( resultsView );
    mainButton->setText( "Save!" );
    mainTab->setCurrentIndex( TAB_RESULTS );
  }
  else
  {
    logText->appendPlainText( "Unable to open html report!\n" );
    mainButton->setText( "Save!" );
    mainTab->setCurrentIndex( TAB_LOG );
  }
}

// ==========================================================================
// Save Results
// ==========================================================================

void SimcraftWindow::saveLog()
{
  logCmdLineHistory.add( cmdLine->text() );

  QFile file( cmdLine->text() );

  if( file.open( QIODevice::WriteOnly ) )
  {
    file.write( logText->document()->toPlainText().toAscii() );
    file.close();
  }

  logText->appendPlainText( QString( "Log saved to: %1\n" ).arg( cmdLine->text() ) );
}

void SimcraftWindow::saveResults()
{
  int index = resultsTab->currentIndex();
  if( index < 0 ) return;

  if( visibleWebView->url().toString() != "about:blank" ) return;

  resultsCmdLineHistory.add( cmdLine->text() );

  QFile file( cmdLine->text() );

  if( file.open( QIODevice::WriteOnly ) )
  {
    file.write( resultsHtml[ index ].toAscii() );
    file.close();
  }

  logText->appendPlainText( QString( "Results saved to: %1\n" ).arg( cmdLine->text() ) );
}

// ==========================================================================
// Window Events
// ==========================================================================

void SimcraftWindow::closeEvent( QCloseEvent* e ) 
{ 
  saveHistory();
  armoryUsView->stop();
  armoryEuView->stop();
  wowheadView->stop();
  chardevView->stop();
  warcrafterView->stop();
  QCoreApplication::quit();
}

void SimcraftWindow::cmdLineTextEdited( const QString& s )
{
  switch( mainTab->currentIndex() )
  {
  case TAB_WELCOME:   cmdLineText = s; break;
  case TAB_GLOBALS:   cmdLineText = s; break;
  case TAB_SIMULATE:  cmdLineText = s; break;
  case TAB_OVERRIDES: cmdLineText = s; break;
  case TAB_HELP:      cmdLineText = s; break;
  case TAB_LOG:       logFileText = s; break;
  case TAB_RESULTS:   resultsFileText = s; break;
  case TAB_IMPORT:    break;
  }
}

void SimcraftWindow::cmdLineReturnPressed()
{
  switch( mainTab->currentIndex() )
  {
  case TAB_WELCOME:   startSim(); break;
  case TAB_GLOBALS:   startSim(); break;
  case TAB_SIMULATE:  startSim(); break;
  case TAB_OVERRIDES: startSim(); break;
  case TAB_IMPORT:
    if( cmdLine->text().count( "us.wowarmory" ) )
    {
      armoryUsView->setUrl( QUrl( cmdLine->text() ) ); 
      importTab->setCurrentIndex( TAB_ARMORY_US );
    }
    else if( cmdLine->text().count( "eu.wowarmory" ) )
    {
      armoryEuView->setUrl( QUrl( cmdLine->text() ) );
      importTab->setCurrentIndex( TAB_ARMORY_EU );
    }
    else if( cmdLine->text().count( "wowhead" ) )
    {
      wowheadView->setUrl( QUrl( cmdLine->text() ) );
      importTab->setCurrentIndex( TAB_WOWHEAD );
    }
    else if( cmdLine->text().count( "chardev" ) )
    {
      chardevView->setUrl( QUrl( cmdLine->text() ) );
      importTab->setCurrentIndex( TAB_CHARDEV );
    }
    else if( cmdLine->text().count( "warcrafter" ) ) 
    {
      warcrafterView->setUrl( QUrl( cmdLine->text() ) );
      importTab->setCurrentIndex( TAB_WARCRAFTER );
    }
    break;
  case TAB_LOG:     saveLog();      break;
  case TAB_RESULTS: saveResults();  break;
  case TAB_HELP: break;
  }
}

void SimcraftWindow::mainButtonClicked( bool checked )
{
  switch( mainTab->currentIndex() )
  {
  case TAB_WELCOME:   startSim(); break;
  case TAB_GLOBALS:   startSim(); break;
  case TAB_SIMULATE:  startSim(); break;
  case TAB_OVERRIDES: startSim(); break;
  case TAB_IMPORT:
    switch( importTab->currentIndex() )
    {
    case TAB_ARMORY_US:  startImport(   armoryUsView->url().toString() ); break;
    case TAB_ARMORY_EU:  startImport(   armoryEuView->url().toString() ); break;
    case TAB_WOWHEAD:    startImport(    wowheadView->url().toString() ); break;
    case TAB_CHARDEV:    startImport(    chardevView->url().toString() ); break;
    case TAB_WARCRAFTER: startImport( warcrafterView->url().toString() ); break;
    case TAB_RAWR:       startImport( rawrFile->text() ); break;
    }
    break;
  case TAB_LOG: saveLog(); break;
  case TAB_RESULTS: saveResults(); break;
  case TAB_HELP: break;
  }
}

void SimcraftWindow::backButtonClicked( bool checked )
{
  if( visibleWebView ) 
  {
    if( mainTab->currentIndex() == TAB_RESULTS && ! visibleWebView->history()->canGoBack() )
    {
      visibleWebView->setHtml( resultsHtml[ resultsTab->indexOf( visibleWebView ) ] );
      visibleWebView->history()->clear();
    }
    else
    {
      visibleWebView->back();
    }
  }
  else
  {
    switch( mainTab->currentIndex() )
    {
    case TAB_WELCOME:   break;
    case TAB_GLOBALS:   decodeGlobals( globalsHistory.backwards() ); break;
    case TAB_IMPORT:    break;
    case TAB_SIMULATE:   simulateText->setPlainText(  simulateTextHistory.backwards() ); break;
    case TAB_OVERRIDES: overridesText->setPlainText( overridesTextHistory.backwards() ); break;
    case TAB_LOG:       break;
    case TAB_RESULTS:   break;
    case TAB_HELP:      break;
    }
  }
}

void SimcraftWindow::forwardButtonClicked( bool checked )
{
  if( visibleWebView ) 
  {
    visibleWebView->forward();
  }
  else
  {
    switch( mainTab->currentIndex() )
    {
    case TAB_WELCOME:   break;
    case TAB_GLOBALS:   decodeGlobals( globalsHistory.forwards() ); break;
    case TAB_IMPORT:    break;
    case TAB_SIMULATE:   simulateText->setPlainText(  simulateTextHistory.forwards() ); break;
    case TAB_OVERRIDES: overridesText->setPlainText( overridesTextHistory.forwards() ); break;
    case TAB_LOG:       break;
    case TAB_RESULTS:   break;
    case TAB_HELP:      break;
    }
  }
}

void SimcraftWindow::mainTabChanged( int index )
{
  visibleWebView = 0;
  switch( index )
  {
  case TAB_WELCOME:   cmdLine->setText(     cmdLineText ); mainButton->setText( sim ? "Cancel!" : "Simulate!" ); break;
  case TAB_GLOBALS:   cmdLine->setText(     cmdLineText ); mainButton->setText( sim ? "Cancel!" : "Simulate!" ); break;
  case TAB_SIMULATE:  cmdLine->setText(     cmdLineText ); mainButton->setText( sim ? "Cancel!" : "Simulate!" ); break;
  case TAB_OVERRIDES: cmdLine->setText(     cmdLineText ); mainButton->setText( sim ? "Cancel!" : "Simulate!" ); break;
  case TAB_LOG:       cmdLine->setText(     logFileText ); mainButton->setText( "Save!" ); break;
  case TAB_HELP:      cmdLine->setText( resultsFileText ); mainButton->setText( "Help!" ); break;
  case TAB_IMPORT:    
    mainButton->setText( sim ? "Cancel!" : "Import!" ); 
    importTabChanged( importTab->currentIndex() ); 
    break;
  case TAB_RESULTS:   
    mainButton->setText( "Save!" ); 
    resultsTabChanged( resultsTab->currentIndex() ); 
    break;
  default: assert(0);
  }
  if( ! visibleWebView ) progressBar->setValue( simProgress );
}

void SimcraftWindow::importTabChanged( int index )
{
  if( index == TAB_RAWR ||
      index == TAB_BIS  ||
      index == TAB_HISTORY )
  {
    visibleWebView = 0;
    progressBar->setValue( simProgress );
    cmdLine->setText( "" );
  }
  else
  {
    visibleWebView = (SimcraftWebView*) importTab->widget( index );
    progressBar->setValue( visibleWebView->progress );
    cmdLine->setText( visibleWebView->url().toString() );
  }
}

void SimcraftWindow::resultsTabChanged( int index )
{
  if( index < 0 )
  {
    cmdLine->setText( "" );
  }
  else
  {
    visibleWebView = (SimcraftWebView*) resultsTab->widget( index );
    progressBar->setValue( visibleWebView->progress );
    QString s = visibleWebView->url().toString();
    if( s == "about:blank" ) s = resultsFileText;
    cmdLine->setText( s );
  }
}

void SimcraftWindow::historyDoubleClicked( QListWidgetItem* item )
{
  QString text = item->text();
  QString url = text.section( ' ', 1, 1, QString::SectionSkipEmpty );

  if( url.count( "us.wowarmory." ) )
  {
    armoryUsView->setUrl( url );
    importTab->setCurrentIndex( TAB_ARMORY_US );
  }
  else if( url.count( "eu.wowarmory." ) )
  {
    armoryEuView->setUrl( url );
    importTab->setCurrentIndex( TAB_ARMORY_EU );
  }
  else if( url.count( ".wowhead." ) )
  {
    wowheadView->setUrl( url );
    importTab->setCurrentIndex( TAB_WOWHEAD );
  }
  else if( url.count( ".chardev." ) )
  {
    chardevView->setUrl( url );
    importTab->setCurrentIndex( TAB_CHARDEV );
  }
  else if( url.count( ".warcrafter." ) )
  {
    warcrafterView->setUrl( url );
    importTab->setCurrentIndex( TAB_WARCRAFTER );
  }
  else
  {
    rawrFile->setText( url );
    importTab->setCurrentIndex( TAB_RAWR );
  }
}

void SimcraftWindow::bisDoubleClicked( QTreeWidgetItem* item, int col )
{
  if( item->columnCount() == 1 ) return;

  QString url = item->text( 1 );

  if( url.count( "us.wowarmory." ) )
  {
    armoryUsView->setUrl( url );
    importTab->setCurrentIndex( TAB_ARMORY_US );
  }
  else if( url.count( "eu.wowarmory." ) )
  {
    armoryEuView->setUrl( url );
    importTab->setCurrentIndex( TAB_ARMORY_EU );
  }
  else if( url.count( ".wowhead." ) )
  {
    wowheadView->setUrl( url );
    importTab->setCurrentIndex( TAB_WOWHEAD );
  }
  else if( url.count( ".chardev." ) )
  {
    chardevView->setUrl( url );
    importTab->setCurrentIndex( TAB_CHARDEV );
  }
  else if( url.count( ".warcrafter." ) )
  {
    warcrafterView->setUrl( url );
    importTab->setCurrentIndex( TAB_WARCRAFTER );
  }
}

