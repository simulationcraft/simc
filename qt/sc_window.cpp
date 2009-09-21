// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraftqt.h"
#include <QtWebKit>

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

void SimcraftWindow::createCmdLine()
{
  QHBoxLayout* cmdLineLayout = new QHBoxLayout();
  cmdLineLayout->addWidget( backButton = new QPushButton( "<" ) );
  cmdLineLayout->addWidget( forwardButton = new QPushButton( ">" ) );
  cmdLineLayout->addWidget( cmdLine = new QLineEdit() );
  cmdLineLayout->addWidget( progressBar = new QProgressBar() );
  cmdLineLayout->addWidget( mainButton = new QPushButton( "Go!" ) );
  backButton->setMaximumWidth( 30 );
  forwardButton->setMaximumWidth( 30 );
  progressBar->setMaximum( 100 );
  progressBar->setMaximumWidth( 100 );
  connect( backButton,    SIGNAL(clicked(bool)), this, SLOT(   backButtonClicked()) );
  connect( forwardButton, SIGNAL(clicked(bool)), this, SLOT(forwardButtonClicked()) );
  connect( mainButton,    SIGNAL(clicked(bool)), this, SLOT(   mainButtonClicked()) );
  cmdLineGroupBox = new QGroupBox();
  cmdLineGroupBox->setLayout( cmdLineLayout );
}

void SimcraftWindow::createWelcomeTab()
{
  QLabel* welcomeLabel = new QLabel( "<br><h1>Welcome to SimulationCraft!</h1><br><br>Help text will be here." );
  welcomeLabel->setAlignment( Qt::AlignLeft|Qt::AlignTop );
  mainTab->addTab( welcomeLabel, "Welcome" );
}
 
void SimcraftWindow::createGlobalsTab()
{
  QFormLayout* globalsLayout = new QFormLayout();
  globalsLayout->addRow( "Patch", patchChoice = createChoice( 2, "3.2.0", "3.2.2" ) );
  globalsLayout->addRow( "Latency", latencyChoice = createChoice( 2, "Low", "High" ) );
  globalsLayout->addRow( "Iterations", iterationsChoice = createChoice( 3, "100", "1000", "10000" ) );
  globalsLayout->addRow( "Length (sec)", fightLengthChoice = createChoice( 3, "100", "300", "500" ) );
  globalsLayout->addRow( "Fight Style", fightStyleChoice = createChoice( 2, "Patchwerk", "Helter Skelter" ) );
  globalsLayout->addRow( "Scale Factors", scaleFactorsChoice = createChoice( 2, "No", "Yes" ) );
  globalsLayout->addRow( "Threads", threadsChoice = createChoice( 4, "1", "2", "4", "8" ) );
  iterationsChoice->setCurrentIndex( 1 );
  fightLengthChoice->setCurrentIndex( 1 );
  threadsChoice->setCurrentIndex( 1 );
  QGroupBox* globalsGroupBox = new QGroupBox();
  globalsGroupBox->setLayout( globalsLayout );
  mainTab->addTab( globalsGroupBox, "Globals" );
}

void SimcraftWindow::createImportTab()
{
  importTab = new QTabWidget();
  mainTab->addTab( importTab, "Import" );

  armoryUsView = new QWebView();
  armoryUsView->setUrl( QUrl( "http://us.wowarmory.com" ) );
  importTab->addTab( armoryUsView, "Armory-US" );
  
  armoryEuView = new QWebView();
  armoryEuView->setUrl( QUrl( "http://eu.wowarmory.com" ) );
  importTab->addTab( armoryEuView, "Armory-EU" );
  
  wowheadView = new QWebView();
  wowheadView->setUrl( QUrl( "http://www.wowhead.com/?profiles" ) );
  importTab->addTab( wowheadView, "Wowhead" );
  
  chardevView = new QWebView();
  chardevView->setUrl( QUrl( "http://www.chardev.org" ) );
  importTab->addTab( chardevView, "CharDev" );
  
  warcrafterView = new QWebView();
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

  historyList = new QListWidget();
  historyList->setSortingEnabled( true );
  importTab->addTab( historyList, "History" );

  connect( historyList, SIGNAL(itemDoubleClicked(QListWidgetItem*)), this, SLOT(historyDoubleClicked(QListWidgetItem*)) );
}

void SimcraftWindow::createSimulateTab()
{
  simulateText = new QPlainTextEdit();
  simulateText->document()->setPlainText( "# Profiles will be downloaded into here.  Right-Click menu will Open/Save scripts." );
  mainTab->addTab( simulateText, "Simulate" );
}

void SimcraftWindow::createOverridesTab()
{
  overridesText = new QPlainTextEdit();
  overridesText->document()->setPlainText( "# Examples of all available global and player parms will shown here." );
  mainTab->addTab( overridesText, "Overrides" );
}

void SimcraftWindow::createLogTab()
{
  logText = new QPlainTextEdit();
  logText->setReadOnly(true);
  logText->document()->setPlainText( "Standard-Out will end up here....." );
  mainTab->addTab( logText, "Log" );
}

void SimcraftWindow::createResultsTab()
{
  resultsTab = new QTabWidget();
  resultsTab->setTabsClosable( true );
  mainTab->addTab( resultsTab, "Results" );
}

void SimcraftWindow::addHistoryItem( const QString& name, const QString& origin )
{
  QString label = name;
  while( label.size() < 20 ) label += " ";
  label += origin;

  for( int i=0; i < historyList->count(); i++ )
    if( historyList->item( i )->text() == label )
      return;

  historyList->addItem( label );
  historyList->sortItems();
}

bool SimcraftWindow::armoryUsImport( QString& profile )
{
  profile = armoryUsView->url().toString();
  return false;
}

bool SimcraftWindow::armoryEuImport( QString& profile )
{
  profile = armoryEuView->url().toString();
  return false;
}

bool SimcraftWindow::wowheadImport( QString& profile )
{
  QString url = wowheadView->url().toString();
  if ( url.contains( "?profile=" ) )
  {
    player_t* player = 0;
    sim = new sim_t();
    QString id = url.section( "=", -1 );
    if ( id.contains( "." ) )
    {
      std::string region = id.section( ".", 1, 1 ).toStdString();
      std::string server = id.section( ".", 2, 2 ).toStdString();
      std::string name   = id.section( ".", 3, 3 ).toStdString();
      player = wowhead_t::download_player( sim, region, server, name );
    }
    else
    {
      player = wowhead_t::download_player( sim, id.toStdString() );
    }
    if ( player )
    {
      std::string buffer = "";
      sim->init();
      player->create_profile( buffer );
      profile = buffer.c_str();
      addHistoryItem( QString( player->name_str.c_str() ), QString( player->origin_str.c_str() ) );
    }
    else
    {
      profile = "# Unabled to download profile: " + id;
    }
    delete sim;
    sim = 0;
  }
  else
  {
    profile = "# Navigate the wowhead import web view to a profile.";
  }
  return false;
}

bool SimcraftWindow::chardevImport( QString& profile )
{
  profile = chardevView->url().toString();
  return false;
}

bool SimcraftWindow::warcrafterImport( QString& profile )
{
  profile = warcrafterView->url().toString();
  return false;
}

bool SimcraftWindow::rawrImport( QString& profile )
{
  profile = "# Rawr Character Import Here\n"; 
  return false;
}

void SimcraftWindow::closeEvent( QCloseEvent* e ) 
{ 
  printf( "close event\n" ); fflush( stdout );
}

void SimcraftWindow::updateProgress()
{
}

void SimcraftWindow::backButtonClicked( bool checked )
{
  if( mainTab->currentIndex() == TAB_IMPORT )
  {
    switch( importTab->currentIndex() )
    {
    case TAB_ARMORY_US:  armoryUsView  ->back(); break;
    case TAB_ARMORY_EU:  armoryEuView  ->back(); break;
    case TAB_WOWHEAD:    wowheadView   ->back(); break;
    case TAB_CHARDEV:    chardevView   ->back(); break;
    case TAB_WARCRAFTER: warcrafterView->back(); break;
    }
  }
}

void SimcraftWindow::forwardButtonClicked( bool checked )
{
  if( mainTab->currentIndex() == TAB_IMPORT )
  {
    switch( importTab->currentIndex() )
    {
    case TAB_ARMORY_US:  armoryUsView  ->forward(); break;
    case TAB_ARMORY_EU:  armoryEuView  ->forward(); break;
    case TAB_WOWHEAD:    wowheadView   ->forward(); break;
    case TAB_CHARDEV:    chardevView   ->forward(); break;
    case TAB_WARCRAFTER: warcrafterView->forward(); break;
    }
  }
}

void SimcraftWindow::mainButtonClicked( bool checked )
{
  switch( mainTab->currentIndex() )
  {
  case TAB_WELCOME: printf( "Launch http://code.google.com/p/simulationcraft/\n" ); break;
  case TAB_GLOBALS: printf( "Save state of all options\n" ); break;
  case TAB_IMPORT:
    {
      QString profile = "";
      bool success = false;
      switch( importTab->currentIndex() )
      {
      case TAB_ARMORY_US:  success =   armoryUsImport( profile ); break;
      case TAB_ARMORY_EU:  success =   armoryEuImport( profile ); break;
      case TAB_WOWHEAD:    success =    wowheadImport( profile ); break;
      case TAB_CHARDEV:    success =    chardevImport( profile ); break;
      case TAB_WARCRAFTER: success = warcrafterImport( profile ); break;
      case TAB_RAWR:       success =       rawrImport( profile ); break;
      default: assert(0);
      }
      if ( success )
      {
	// Change button to "Cancel!"
      }
      simulateText->document()->setPlainText( profile );
      mainTab->setCurrentIndex( TAB_SIMULATE );
      break;
    }
  case TAB_SIMULATE:
  case TAB_OVERRIDES:
  case TAB_LOG:
    {
      QString resultsName = QString( "Results %1" ).arg( ++numResults );
      QTextBrowser* resultsBrowser = new QTextBrowser();
      resultsBrowser->document()->setPlainText( "Results Here.\n" );
      resultsTab->addTab( resultsBrowser, resultsName );
      resultsTab->setCurrentWidget( resultsBrowser );
      mainTab->setCurrentIndex( TAB_RESULTS );
      break;
    }
  case TAB_RESULTS: printf( "Save Results: %d\n", resultsTab->currentIndex() ); fflush(stdout); break;
  case TAB_HELP: break;
  default: assert(0);
  }
  
}

void SimcraftWindow::mainTabChanged( int index )
{
  switch( index )
  {
  case TAB_WELCOME:   mainButton->setText( "Go!"       ); break;
  case TAB_GLOBALS:   mainButton->setText( "Save!"     ); break;
  case TAB_IMPORT:    mainButton->setText( "Import!"   ); break;
  case TAB_SIMULATE:  mainButton->setText( "Simulate!" ); break;
  case TAB_OVERRIDES: mainButton->setText( "Simulate!" ); break;
  case TAB_LOG:       mainButton->setText( "Simulate!" ); break;
  case TAB_RESULTS:   mainButton->setText( "Save!"     ); break;
  default: assert(0);
  }
}

void SimcraftWindow::historyDoubleClicked( QListWidgetItem* item )
{
  QString text = item->text();
  QString url = text.section( ' ', 1, 1, QString::SectionSkipEmpty );

  if( text.count( "us.wowarmory." ) )
  {
    importTab->setCurrentIndex( TAB_ARMORY_US );
    armoryUsView->setUrl( url );
  }
  else if( url.count( "eu.wowarmory." ) )
  {
    importTab->setCurrentIndex( TAB_ARMORY_EU );
    armoryEuView->setUrl( url );
  }
  else if( url.count( ".wowhead." ) )
  {
    importTab->setCurrentIndex( TAB_WOWHEAD );
    wowheadView->setUrl( url );
  }
  else if( url.count( ".chardev." ) )
  {
    importTab->setCurrentIndex( TAB_CHARDEV );
    chardevView->setUrl( url );
  }
  else if( url.count( ".warcrafter." ) )
  {
    importTab->setCurrentIndex( TAB_WARCRAFTER );
    warcrafterView->setUrl( url );
  }
  else
  {
    importTab->setCurrentIndex( TAB_RAWR );
    rawrFile->setText( url );
  }
}

SimcraftWindow::SimcraftWindow(QWidget *parent)
  : QWidget(parent), sim(0), numResults(0)
{
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
  connect( timer, SIGNAL(timeout()), this, SLOT(updateProgress()) );

}
