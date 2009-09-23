// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SIMCRAFTQT_H
#define SIMCRAFTQT_H

#include <QtGui/QtGui>
#include <QtWebKit>
#include <simcraft.h>

#define TAB_WELCOME   0
#define TAB_GLOBALS   1
#define TAB_IMPORT    2
#define TAB_SIMULATE  3
#define TAB_OVERRIDES 4
#define TAB_LOG       5
#define TAB_RESULTS   6
#define TAB_HELP      7

#define TAB_ARMORY_US  0
#define TAB_ARMORY_EU  1
#define TAB_WOWHEAD    2
#define TAB_CHARDEV    3
#define TAB_WARCRAFTER 4
#define TAB_RAWR       5
#define TAB_HISTORY    6

class SimcraftWebView;

class SimulateThread : public QThread
{
    Q_OBJECT
    sim_t* sim;
    QString options;

public:
    void start( sim_t* s, const QString& o ) { sim=s; options=o; QThread::start(); }
    virtual void run();
    SimulateThread() : sim(0) {}
};

class ImportThread : public QThread
{
    Q_OBJECT

public:
    sim_t* sim;
    QString url;
    QString profile;
    player_t* player;

    void importArmoryUs();
    void importArmoryEu();
    void importWowhead();
    void importChardev();
    void importWarcrafter();
    void importRawr();

    void start( sim_t* s, const QString& u ) { sim=s; url=u; profile=""; player=0; QThread::start(); }
    virtual void run();
    ImportThread() : sim(0), player(0) {}
};

class SimcraftWindow : public QWidget
{
    Q_OBJECT

public:
    QTabWidget* mainTab;
    QComboBox* patchChoice;
    QComboBox* latencyChoice;
    QComboBox* iterationsChoice;
    QComboBox* fightLengthChoice;
    QComboBox* fightStyleChoice;
    QComboBox* scaleFactorsChoice;
    QComboBox* threadsChoice;
    QTabWidget* importTab;
    SimcraftWebView* armoryUsView;
    SimcraftWebView* armoryEuView;
    SimcraftWebView* wowheadView;
    SimcraftWebView* chardevView;
    SimcraftWebView* warcrafterView;
    SimcraftWebView* visibleWebView;
    QLineEdit* rawrFile;
    QListWidget* historyList;
    QPlainTextEdit* simulateText;
    QPlainTextEdit* overridesText;
    QPlainTextEdit* logText;
    QTabWidget* resultsTab;
    QPushButton* backButton;
    QPushButton* forwardButton;
    QLineEdit* cmdLine;
    QString cmdLineText;
    QString logFileText;
    QString resultsFileText;
    QProgressBar* progressBar;
    QPushButton* mainButton;
    QGroupBox* cmdLineGroupBox;

    QTimer* timer;
    ImportThread* importThread;
    SimulateThread* simulateThread;

    int armoryUsProgress;
    int armoryEuProgress;
    int wowheadProgress;
    int chardevProgress;
    int warcrafterProgress;

    sim_t* sim;
    int simProgress;
    int simResults;
    QStringList resultsHtml;

    void    startImport( const QString& url );
    void    startSim();
    sim_t*  initSim();
    void    deleteSim();
    QString mergeOptions();

    void saveLog();
    void saveResults();
    
    void createCmdLine();
    void createWelcomeTab();
    void createGlobalsTab();
    void createImportTab();
    void createSimulateTab();
    void createOverridesTab();
    void createLogTab();
    void createResultsTab();

    void addHistoryItem( const QString& name, const QString& origin );

protected:
    virtual void closeEvent( QCloseEvent* e );

private slots:
    void importFinished();
    void simulateFinished();
    void updateSimProgress();
    void cmdLineReturnPressed();
    void cmdLineTextEdited( const QString& );
    void backButtonClicked( bool checked=false );
    void forwardButtonClicked( bool checked=false );
    void mainButtonClicked( bool checked=false );
    void mainTabChanged( int index );
    void importTabChanged( int index );
    void resultsTabChanged( int index );
    void historyDoubleClicked( QListWidgetItem* item );

public:
    SimcraftWindow(QWidget *parent = 0);
};

class SimcraftWebView : public QWebView
{
  Q_OBJECT

public:
  SimcraftWindow* mainWindow;
  int progress;

private slots:
  void loadProgressSlot( int p )
  {
    progress = p;
    if( mainWindow->visibleWebView == this )
    {
      mainWindow->progressBar->setValue( progress );
    }
  }
  void loadFinishedSlot( bool ok )
  {
    progress = 100;
    if( mainWindow->visibleWebView == this )
    {
      mainWindow->progressBar->setValue( 100 );
    }
  }
  void urlChangedSlot( const QUrl& url )
  {
    if( mainWindow->visibleWebView == this )
    {
      QString s = url.toString();
      if( s == "about:blank" ) s = "results.html";
      mainWindow->cmdLine->setText( s );
    }
  }

public:
  SimcraftWebView( SimcraftWindow* mw ) : 
    mainWindow(mw), progress(0) 
  {
    connect( this, SIGNAL(loadProgress(int)),       this, SLOT(loadProgressSlot(int)) );
    connect( this, SIGNAL(loadFinished(bool)),      this, SLOT(loadFinishedSlot(bool)) );
    connect( this, SIGNAL(urlChanged(const QUrl&)), this, SLOT(urlChangedSlot(const QUrl&)) );
  }
  virtual ~SimcraftWebView() {}
};

#endif // SIMCRAFTQT_H
