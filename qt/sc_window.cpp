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
    simProgress = (int) ( 100.0 * sim->progress() );
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
    QDataStream in( &file );
    QString historyVersion;
    in >> historyVersion;
    if( historyVersion != HISTORY_VERSION ) return;
    QStringList importHistory;
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

    QString s = overridesTextHistory.backwards();
    if( ! s.isEmpty() ) overridesText->setPlainText( s );
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
    out << QString( HISTORY_VERSION );
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
  : QWidget(parent), visibleWebView(0), sim(0), simProgress(100), simResults(0)
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
  createExamplesTab();
  createLogTab();
  createResultsTab();
  createCmdLine();
  createToolTips();

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
  QString s = "<div align=center><h1>Welcome to SimulationCraft!</h1>If you are seeing this text, then Welcome.html was unable to load.</div>";

  QFile file( "Welcome.html" );
  if( file.open( QIODevice::ReadOnly ) )
  {
    s = file.readAll();
    file.close();
  }

  QTextBrowser* welcomeBanner = new QTextBrowser();
  welcomeBanner->setHtml( s );
  welcomeBanner->moveCursor( QTextCursor::Start );
  mainTab->addTab( welcomeBanner, "Welcome" );
}
 
void SimcraftWindow::createGlobalsTab()
{
  QFormLayout* globalsLayout = new QFormLayout();
  globalsLayout->setFieldGrowthPolicy( QFormLayout::FieldsStayAtSizeHint );
  globalsLayout->addRow(         "Patch",        patchChoice = createChoice( 2, "3.2.2", "3.3.0" ) );
  globalsLayout->addRow(       "Latency",      latencyChoice = createChoice( 2, "Low", "High" ) );
  globalsLayout->addRow(    "Iterations",   iterationsChoice = createChoice( 3, "100", "1000", "10000" ) );
  globalsLayout->addRow(  "Length (sec)",  fightLengthChoice = createChoice( 3, "100", "300", "500" ) );
  globalsLayout->addRow(   "Fight Style",   fightStyleChoice = createChoice( 2, "Patchwerk", "Helter Skelter" ) );
  globalsLayout->addRow( "Scale Factors", scaleFactorsChoice = createChoice( 2, "No", "Yes" ) );
  globalsLayout->addRow(       "Threads",      threadsChoice = createChoice( 4, "1", "2", "4", "8" ) );
  globalsLayout->addRow(   "Armory Spec",   armorySpecChoice = createChoice( 2, "active", "inactive" ) );
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
				  " To aid comparison, SimulationCraft can import the character xml file written by Rawr." );
  rawrLabel->setWordWrap( true );
  rawrLayout->addWidget( rawrLabel );
  rawrLayout->addWidget( rawrButton = new QPushButton( "Change Directory" ) );
  rawrLayout->addWidget( rawrDir = new QLabel( "" ) );
  rawrLayout->addWidget( rawrList = new QListWidget(), 1 );
  QGroupBox* rawrGroupBox = new QGroupBox();
  rawrGroupBox->setLayout( rawrLayout );
  importTab->addTab( rawrGroupBox, "Rawr" );

  createBestInSlotTab();

  historyList = new QListWidget();
  historyList->setSortingEnabled( true );
  importTab->addTab( historyList, "History" );

  connect( rawrButton,  SIGNAL(clicked(bool)),                       this, SLOT(rawrButtonClicked()) );
  connect( rawrList,    SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(   rawrDoubleClicked(QListWidgetItem*)) );
  connect( historyList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(historyDoubleClicked(QListWidgetItem*)) );
  connect( importTab,   SIGNAL(currentChanged(int)),                 this, SLOT(importTabChanged(int)) );
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
      { DRUID,      T8, "Druid_T8_00_55_16",                TAB_WOWHEAD, "13000407" },
      { DRUID,      T8, "Druid_T8_00_55_16_M",              TAB_WOWHEAD, "14726797" },
      { DRUID,      T8, "Druid_T8_58_00_13",                TAB_WOWHEAD, "13000433" },
      { DRUID,      T9, "Druid_T9_00_55_16",                TAB_WOWHEAD, "14717965" },
      { DRUID,      T9, "Druid_T9_00_55_16_M",              TAB_WOWHEAD, "14726081" },
      { DRUID,      T9, "Druid_T9_58_00_13",                TAB_WOWHEAD, "14187592" },
      { DRUID,      T9, "Druid_T9_58_00_13_2T8_2T9",        TAB_WOWHEAD, "14187598" },

      { HUNTER,     T8, "Hunter_T8_01_15_55",               TAB_WOWHEAD, "13001196" },
      { HUNTER,     T8, "Hunter_T8_07_57_07",               TAB_WOWHEAD, "13001144" },
      { HUNTER,     T8, "Hunter_T8_07_57_07_HM",            TAB_WOWHEAD, "13630091" },
      { HUNTER,     T8, "Hunter_T8_54_12_05",               TAB_WOWHEAD, "13000974" },
      { HUNTER,     T9, "Hunter_T8_02_18_51",               TAB_WOWHEAD, "14649679" },
      { HUNTER,     T9, "Hunter_T8_07_57_07",               TAB_WOWHEAD, "14189960" },
      { HUNTER,     T9, "Hunter_T8_07_57_07_HM",            TAB_WOWHEAD, "14651857" },
      { HUNTER,     T9, "Hunter_T8_53_14_04",               TAB_WOWHEAD, "14663160" },
      { HUNTER,     T9, "Hunter_T8_54_12_05",               TAB_WOWHEAD, "14662679" },

      { MAGE,       T8, "Mage_T8_00_53_18",                 TAB_WOWHEAD, "13000266" },
      { MAGE,       T8, "Mage_T8_18_00_53",                 TAB_WOWHEAD, "13000343" },
      { MAGE,       T8, "Mage_T8_20_51_00",                 TAB_WOWHEAD, "13001492" },
      { MAGE,       T8, "Mage_T8_53_18_00",                 TAB_WOWHEAD, "13000901" },
      { MAGE,       T8, "Mage_T8_57_03_11",                 TAB_WOWHEAD, "13001124" },
      { MAGE,       T9, "Mage_T9_00_53_18",                 TAB_WOWHEAD, "14306487" },
      { MAGE,       T9, "Mage_T9_18_00_53",                 TAB_WOWHEAD, "14306485" },
      { MAGE,       T9, "Mage_T9_20_51_00",                 TAB_WOWHEAD, "14306481" },
      { MAGE,       T9, "Mage_T9_53_18_00",                 TAB_WOWHEAD, "14306474" },
      { MAGE,       T9, "Mage_T9_57_03_11",                 TAB_WOWHEAD, "14306468" },

      { PALADIN,    T8, "Paladin_T8_05_11_55",              TAB_WOWHEAD, "14319567" },
      { PALADIN,    T9, "Paladin_T9_05_11_55",              TAB_WOWHEAD, "14637386" },

      { PRIEST,     T8, "Priest_T8_13_00_58",               TAB_WOWHEAD, "13000014" },
      { PRIEST,     T8, "Priest_T8_34_28_09",               TAB_WOWHEAD, "13000020" },
      { PRIEST,     T8, "Priest_T8_51_20_00",               TAB_WOWHEAD, "13000023" },
      { PRIEST,     T9, "Priest_T9_13_00_58_232",           TAB_WOWHEAD, "13043846" },
      { PRIEST,     T9, "Priest_T9_13_00_58_245",           TAB_WOWHEAD, "13043847" },
      { PRIEST,     T9, "Priest_T9_13_00_58_258",           TAB_WOWHEAD, "13043848" },

      { ROGUE,      T8, "Rogue_T8_07_51_13_FD",             TAB_WOWHEAD, "13006118" },
      { ROGUE,      T8, "Rogue_T8_08_20_43",                TAB_WOWHEAD, "13006155" },
      { ROGUE,      T8, "Rogue_T8_10_18_43",                TAB_WOWHEAD, "13006220" },
      { ROGUE,      T8, "Rogue_T8_15_51_05_FD",             TAB_WOWHEAD, "13006241" },
      { ROGUE,      T8, "Rogue_T8_15_51_05_M",              TAB_WOWHEAD, "13006278" },
      { ROGUE,      T8, "Rogue_T8_15_51_05_S",              TAB_WOWHEAD, "13006306" },
      { ROGUE,      T8, "Rogue_T8_18_51_02_BS",             TAB_WOWHEAD, "13006354" },
      { ROGUE,      T8, "Rogue_T8_23_05_43",                TAB_WOWHEAD, "13006482" },
      { ROGUE,      T8, "Rogue_T8_51_07_13",                TAB_WOWHEAD, "13007512" },
      { ROGUE,      T8, "Rogue_T8_51_13_07",                TAB_WOWHEAD, "13007548" },
      { ROGUE,      T9, "Rogue_T9_08_20_43",                TAB_WOWHEAD, "14562456" },
      { ROGUE,      T9, "Rogue_T9_15_51_05_AA",             TAB_WOWHEAD, "14562190" },
      { ROGUE,      T9, "Rogue_T9_15_51_05_FD",             TAB_WOWHEAD, "14419498" },
      { ROGUE,      T9, "Rogue_T9_15_51_05_MD",             TAB_WOWHEAD, "14562031" },
      { ROGUE,      T9, "Rogue_T9_51_13_07",                TAB_WOWHEAD, "14562941" },

      { SHAMAN,     T8, "Shaman_T8_16_55_00",               TAB_WOWHEAD, "13000114" },
      { SHAMAN,     T8, "Shaman_T8_57_14_00",               TAB_WOWHEAD, "13000116" },
      { SHAMAN,     T8, "Shaman_T8_57_14_00_ToW",           TAB_WOWHEAD, "14730877" },
      { SHAMAN,     T9, "Shaman_T9_16_55_00_258",           TAB_WOWHEAD, "14635667" },
      { SHAMAN,     T9, "Shaman_T9_20_51_00_258",           TAB_WOWHEAD, "14635698" },
      { SHAMAN,     T9, "Shaman_T9_57_14_00_245",           TAB_WOWHEAD, "14636463" },
      { SHAMAN,     T9, "Shaman_T9_57_14_00_258",           TAB_WOWHEAD, "14636542" },
      { SHAMAN,     T9, "Shaman_T9_57_14_00_258_ToW",       TAB_WOWHEAD, "14731192" },

      { WARLOCK,    T8, "Warlock_T8_00_13_58",              TAB_WOWHEAD, "13002350" },
      { WARLOCK,    T8, "Warlock_T8_00_40_31",              TAB_WOWHEAD, "13003653" },
      { WARLOCK,    T8, "Warlock_T8_00_41_30",              TAB_WOWHEAD, "13003812" },
      { WARLOCK,    T8, "Warlock_T8_00_56_15",              TAB_WOWHEAD, "13003861" },
      { WARLOCK,    T8, "Warlock_T8_03_13_55",              TAB_WOWHEAD, "13003527" },
      { WARLOCK,    T8, "Warlock_T8_03_52_16",              TAB_WOWHEAD, "13003837" },
      { WARLOCK,    T8, "Warlock_T8_53_00_18",              TAB_WOWHEAD, "13003573" },
      { WARLOCK,    T9, "Warlock_T9_00_13_58",              TAB_WOWHEAD, "13808359" },
      { WARLOCK,    T9, "Warlock_T9_00_40_31",              TAB_WOWHEAD, "14603726" },
      { WARLOCK,    T9, "Warlock_T9_00_41_30",              TAB_WOWHEAD, "14603857" },
      { WARLOCK,    T9, "Warlock_T9_00_56_15",              TAB_WOWHEAD, "14603959" },
      { WARLOCK,    T9, "Warlock_T9_03_13_55",              TAB_WOWHEAD, "14604165" },
      { WARLOCK,    T9, "Warlock_T9_03_52_16",              TAB_WOWHEAD, "14604604" },
      { WARLOCK,    T9, "Warlock_T9_53_00_18",              TAB_WOWHEAD, "14604901" },

      { WARRIOR,    T8, "Warrior_T8_19_52_00",              TAB_WOWHEAD, "13001761" },
      { WARRIOR,    T8, "Warrior_T8_54_17_00_A",            TAB_WOWHEAD, "13001790" },
      { WARRIOR,    T8, "Warrior_T8_54_17_00_M",            TAB_WOWHEAD, "13001883" },
      { WARRIOR,    T8, "Warrior_T8_54_17_00_S",            TAB_WOWHEAD, "13001872" },
      { WARRIOR,    T9, "Warrior_T9_19_52_00",              TAB_WOWHEAD, "13110966" },
      { WARRIOR,    T9, "Warrior_T9_54_17_00_A",            TAB_WOWHEAD, "13111680" },
      { WARRIOR,    T9, "Warrior_T9_54_17_00_M",            TAB_WOWHEAD, "13112110" },
      { WARRIOR,    T9, "Warrior_T9_54_17_00_S",            TAB_WOWHEAD, "13112036" },

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

void SimcraftWindow::createExamplesTab()
{
  QString s = "# If you are seeing this text, then Examples.simcraft was unable to load.";

  QFile file( "Examples.simcraft" );
  if( file.open( QIODevice::ReadOnly ) )
  {
    s = file.readAll();
    file.close();
  }

  QTextBrowser* examplesText = new QTextBrowser();
  examplesText->document()->setPlainText( s );
  mainTab->addTab( examplesText, "Examples" );
}
 
void SimcraftWindow::createResultsTab()
{
  QString s = "<div align=center><h1>Understanding SimulationCraft Output!</h1>If you are seeing this text, then Legend.html was unable to load.</div>";

  QFile file( "Legend.html" );
  if( file.open( QIODevice::ReadOnly ) )
  {
    s = file.readAll();
    file.close();
  }

  QTextBrowser* legendBanner = new QTextBrowser();
  legendBanner->setHtml( s );
  legendBanner->moveCursor( QTextCursor::Start );

  resultsTab = new QTabWidget();
  resultsTab->addTab( legendBanner, "Legend" );
  connect( resultsTab, SIGNAL(currentChanged(int)), this, SLOT(resultsTabChanged(int)) );
  mainTab->addTab( resultsTab, "Results" );
}

void SimcraftWindow::createToolTips()
{
  patchChoice->setToolTip( "Live: 3.2.2\n"
			   "PTR: 3.3.0" );

  latencyChoice->setToolTip( "Low:  queue=0.075  gcd=0.150  channel=0.250\n"
			     "High: queue=0.150  gcd=0.300  channel=0.500" );

  iterationsChoice->setToolTip( "100:   Fast and Rough\n"
				"1000:  Sufficient for DPS Analysis\n"
				"10000: Recommended for Scale Factor Generation" );

  fightLengthChoice->setToolTip( "For custom fight lengths use max_time=seconds." );

  fightStyleChoice->setToolTip( "Patchwerk: Tank-n-Spank\n"
				"Helter Skelter: Movement, Stuns, Interrupts" );

  scaleFactorsChoice->setToolTip( "Scale factors are necessary for gear ranking.\n"
				  "They require an additional simulation for every relevant stat." );

  threadsChoice->setToolTip( "Match the number of CPUs for optimal performance.\n"
			     "Most modern desktops have two at least two CPU cores." );

  armorySpecChoice->setToolTip( "Controls which Talent/Glyph specification is used when importing profiles from the Armory." );

  backButton->setToolTip( "Backwards" );
  forwardButton->setToolTip( "Forwards" );
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
  switch( tab )
  {
  case TAB_ARMORY_US:  importArmoryUs();   break;
  case TAB_ARMORY_EU:  importArmoryEu();   break;
  case TAB_WOWHEAD:    importWowhead();    break;
  case TAB_CHARDEV:    importChardev();    break;
  case TAB_WARCRAFTER: importWarcrafter(); break;
  case TAB_RAWR:       importRawr();       break;
  default: assert(0);
  }

  if( player )
  {
    sim->init();
    std::string buffer="";
    player->create_profile( buffer );
    profile = buffer.c_str();
  }
}

void SimcraftWindow::startImport( int tab, const QString& url )
{
  if( sim )
  {
    sim -> cancel();
    return;
  }
  if( tab == TAB_RAWR ) 
  {
    rawrCmdLineHistory.add( url );
    rawrFileText = "";
    cmdLine->setText( rawrFileText );
  }
  simProgress = 0;
  mainButton->setText( "Cancel!" );
  importThread->start( initSim(), tab, url );
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

  if( sim -> parse_options( argc, argv ) )
  {
    if( sim -> execute() )
    {
      sim -> scaling -> analyze();
      report_t::print_suite( sim );
      success = true;
    }
  }
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
  if( ! simulateThread->success )
  {
    logText->appendPlainText( "Simulation failed!\n" );
    mainTab->setCurrentIndex( TAB_LOG );
  }
  else if( file.open( QIODevice::ReadOnly ) )
  {
    QString resultsName = QString( "Results %1" ).arg( ++simResults );
    SimcraftWebView* resultsView = new SimcraftWebView( this );
    resultsHtml.append( file.readAll() );
    resultsView->setHtml( resultsHtml.last() );
    resultsTab->addTab( resultsView, resultsName );
    resultsTab->setCurrentWidget( resultsView );
    mainTab->setCurrentIndex( TAB_RESULTS );
  }
  else
  {
    logText->appendPlainText( "Unable to open html report!\n" );
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
  if( index <= 0 ) return;

  if( visibleWebView->url().toString() != "about:blank" ) return;

  resultsCmdLineHistory.add( cmdLine->text() );

  QFile file( cmdLine->text() );

  if( file.open( QIODevice::WriteOnly ) )
  {
    file.write( resultsHtml[ index-1 ].toAscii() );
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
  e->accept();
}

void SimcraftWindow::cmdLineTextEdited( const QString& s )
{
  switch( mainTab->currentIndex() )
  {
  case TAB_WELCOME:   cmdLineText = s; break;
  case TAB_GLOBALS:   cmdLineText = s; break;
  case TAB_SIMULATE:  cmdLineText = s; break;
  case TAB_OVERRIDES: cmdLineText = s; break;
  case TAB_EXAMPLES:  cmdLineText = s; break;
  case TAB_LOG:       logFileText = s; break;
  case TAB_RESULTS:   resultsFileText = s; break;
  case TAB_IMPORT:    if( importTab->currentIndex() == TAB_RAWR ) rawrFileText = s; break;
  }
}

void SimcraftWindow::cmdLineReturnPressed()
{
  if( mainTab->currentIndex() == TAB_IMPORT )
  {
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
    else
    {
      if( ! sim ) mainButtonClicked( true );
    }
  }
  else
  {
    if( ! sim ) mainButtonClicked( true );
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
  case TAB_EXAMPLES:  startSim(); break;
  case TAB_IMPORT:
    switch( importTab->currentIndex() )
    {
    case TAB_ARMORY_US:  startImport( TAB_ARMORY_US,  cmdLine->text() ); break;
    case TAB_ARMORY_EU:  startImport( TAB_ARMORY_EU,  cmdLine->text() ); break;
    case TAB_WOWHEAD:    startImport( TAB_WOWHEAD,    cmdLine->text() ); break;
    case TAB_CHARDEV:    startImport( TAB_CHARDEV,    cmdLine->text() ); break;
    case TAB_WARCRAFTER: startImport( TAB_WARCRAFTER, cmdLine->text() ); break;
    case TAB_RAWR:       startImport( TAB_RAWR,       cmdLine->text() ); break;
    }
    break;
  case TAB_LOG: saveLog(); break;
  case TAB_RESULTS: saveResults(); break;
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
    case TAB_EXAMPLES:  break;
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
    case TAB_EXAMPLES:  break;
    }
  }
}

void SimcraftWindow::rawrButtonClicked( bool checked )
{
  QFileDialog dialog( this );
  dialog.setFileMode( QFileDialog::Directory );
  dialog.setNameFilter( "Rawr Profiles (*.xml)" );
  dialog.restoreState( rawrDialogState );
  if( dialog.exec() )
  {
    rawrDialogState = dialog.saveState();
    QDir dir = dialog.directory();
    dir.setSorting( QDir::Name );
    dir.setFilter( QDir::Files );
    dir.setNameFilters( QStringList( "*.xml" ) );
    rawrDir->setText( dir.absolutePath() + DIRECTORY_DELIMITER );
    rawrList->clear();
    rawrList->addItems( dir.entryList() );
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
  case TAB_EXAMPLES:  cmdLine->setText(     cmdLineText ); mainButton->setText( sim ? "Cancel!" : "Simulate!" ); break;
  case TAB_LOG:       cmdLine->setText(     logFileText ); mainButton->setText( "Save!" ); break;
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
    if( index == TAB_RAWR )
    {
      cmdLine->setText( rawrFileText );
    }
    else
    {
      cmdLine->setText( "" );
    }
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
  if( index <= 0 )
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

void SimcraftWindow::rawrDoubleClicked( QListWidgetItem* item )
{
  rawrFileText = rawrDir->text();
  rawrFileText += item->text();
  cmdLine->setText( rawrFileText );
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

