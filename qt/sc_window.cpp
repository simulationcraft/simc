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
    cmdLineLayout->addWidget( cmdLine = new QLineEdit() );
    cmdLineLayout->addWidget( progressBar = new QProgressBar() );
    cmdLineLayout->addWidget( mainButton = new QPushButton( "Go!" ) );
    progressBar->setMaximum( 100 );
    progressBar->setMaximumWidth( 100 );
    connect( mainButton, SIGNAL(clicked(bool)), this, SLOT(mainButtonClicked()) );
    cmdLineGroupBox = new QGroupBox();
    cmdLineGroupBox->setLayout( cmdLineLayout );
}

void SimcraftWindow::createWelcomeTab()
{
    QLabel* welcomeLabel = new QLabel( "\n  Welcome to SimulationCraft!\n\n  Help text will be here.\n" );
    welcomeLabel->setAlignment( Qt::AlignLeft|Qt::AlignTop );
    mainTab->addTab( welcomeLabel, "Welcome" );
}
 
void SimcraftWindow::createGlobalsTab()
{
    QFormLayout* globalsLayout = new QFormLayout();
    globalsLayout->addRow( "Region", regionChoice = createChoice( 2, "US", "EU" ) );
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
    QGroupBox* globalsGroupBox = new QGroupBox( "Global Options" );
    globalsGroupBox->setLayout( globalsLayout );
    mainTab->addTab( globalsGroupBox, "Globals" );
}

void SimcraftWindow::createImportTab()
{
    importTab = new QTabWidget();
    connect( importTab, SIGNAL(currentChanged(int)), this, SLOT(importTabChanged(int)) );
    mainTab->addTab( importTab, "Import" );

    armoryView = new QWebView();
    armoryView->setUrl( QUrl( "http://www.wowarmory.com" ) );
    importTab->addTab( armoryView, "Armory" );

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
    QGroupBox* rawrGroupBox = new QGroupBox( "Rawr Character XML File" );
    rawrGroupBox->setLayout( rawrLayout );
    importTab->addTab( rawrGroupBox, "Rawr" );
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

void SimcraftWindow::closeEvent( QCloseEvent* e ) 
{ 
  printf( "close event\n" ); fflush( stdout );
  // Seems necessary to handle these WebViews with extra-special care.
  armoryView->deleteLater();
  wowheadView->deleteLater();
  chardevView->deleteLater();
  warcrafterView->deleteLater();
}

void SimcraftWindow::updateProgress()
{
}

void SimcraftWindow::mainButtonClicked( bool checked )
{
    switch( mainTab->currentIndex() )
    {
        case 0: printf( "Launch http://code.google.com/p/simulationcraft/\n" ); break;
        case 1: printf( "Save state of all options\n" ); break;
        case 2:
        {
            QString profile = "";
            switch( importTab->currentIndex() )
            {
                case 0: profile = "# Armory Profile Here\n"; break;
                case 1: profile = "# Wowhead Profile Here\n"; break;
                case 2: profile = "# CharDev not yet supported.\n"; break;
                case 3: profile = "# Warcrafter not yet supported.\n"; break;
                case 4: profile = "# Rawr Character Import Here\n"; break;
                default: assert(0);
            }
            simulateText->document()->setPlainText( profile );
            mainTab->setCurrentIndex( 3 );
            break;
        }
        case 3:
        case 4:
        case 5:
        {
            QString resultsName = QString( "Results %1" ).arg( resultsTab->count()+1 );
            QTextBrowser* resultsBrowser = new QTextBrowser();
            resultsBrowser->document()->setPlainText( "Results Here.\n" );
            resultsTab->addTab( resultsBrowser, resultsName );
            resultsTab->setCurrentWidget( resultsBrowser );
            mainTab->setCurrentIndex( 6 );
            break;
        }
        case 6: printf( "Save Results: %d\n", resultsTab->currentIndex() ); break;
        default: assert(0);
    }
    fflush(stdout);
}

void SimcraftWindow::mainTabChanged( int index )
{
    switch( index )
    {
        case 0: mainButton->setText( "Go!"       ); break;
        case 1: mainButton->setText( "Save!"     ); break;
        case 2: mainButton->setText( "Import!"   ); break;
        case 3: mainButton->setText( "Simulate!" ); break;
        case 4: mainButton->setText( "Simulate!" ); break;
        case 5: mainButton->setText( "Simulate!" ); break;
        case 6: mainButton->setText( "Save!"     ); break;
        default: assert(0);
    }
}

void SimcraftWindow::importTabChanged( int index )
{
}

SimcraftWindow::SimcraftWindow(QWidget *parent)
    : QWidget(parent)
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

    timer = new QTimer();
    connect( timer, SIGNAL(timeout()), this, SLOT(updateProgress()) );

}
