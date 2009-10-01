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
class SimcraftCommandLine;
class SimulateThread;
class ImportThread;

class StringHistory : public QStringList
{
public:
  int current_index;
  StringHistory() : current_index(-1) {}
  QString current() 
  { 
    if( current_index < 0 ) return "";
    if( current_index >= size() ) return "";
    return at( current_index );
  }
  QString backwards()
  {
    if( current_index < size() ) current_index++;
    return current();
  }
  QString forwards()
  {
    if( current_index >= 0 ) current_index--;
    return current();
  }
  QString next( int k )
  {
    return ( k == Qt::Key_Up ) ? backwards() : forwards();
  }
  void add( QString s )
  {
    if( size() > 0 ) if( at( 0 ) == s ) return;
    prepend( s );
    current_index = -1;
  }
  QString start()
  {
    current_index = 0;
    return current();
  }
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
    QComboBox* armorySpecChoice;
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
    SimcraftCommandLine* cmdLine;
    QString cmdLineText;
    QString logFileText;
    QString resultsFileText;
    QProgressBar* progressBar;
    QPushButton* mainButton;
    QGroupBox* cmdLineGroupBox;

    QTimer* timer;
    ImportThread* importThread;
    SimulateThread* simulateThread;

    sim_t* sim;
    int simProgress;
    int simResults;
    QStringList resultsHtml;

    StringHistory simulateCmdLineHistory;
    StringHistory logCmdLineHistory;
    StringHistory resultsCmdLineHistory;
    StringHistory globalsHistory;
    StringHistory simulateTextHistory;
    StringHistory overridesTextHistory;

    void    startImport( const QString& url );
    void    startSim();
    sim_t*  initSim();
    void    deleteSim();
    QString mergeOptions();

    void saveLog();
    void saveResults();

    void    decodeGlobals( QString );
    QString encodeGlobals();

    void loadHistory();
    void saveHistory();
    
    void createCmdLine();
    void createWelcomeTab();
    void createGlobalsTab();
    void createImportTab();
    void createSimulateTab();
    void createOverridesTab();
    void createLogTab();
    void createResultsTab();

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

class SimcraftCommandLine : public QLineEdit
{
  SimcraftWindow* mainWindow;

protected:
  virtual void keyPressEvent( QKeyEvent* e )
  {
    int k = e->key();
    if( k != Qt::Key_Up && k != Qt::Key_Down )
    {
      QLineEdit::keyPressEvent( e );
      return;
    }
    switch( mainWindow->mainTab->currentIndex() )
    {
    case TAB_WELCOME:   
    case TAB_GLOBALS:   
    case TAB_SIMULATE:  
    case TAB_OVERRIDES: 
      mainWindow->cmdLineText = mainWindow->simulateCmdLineHistory.next( k ); 
      setText( mainWindow->cmdLineText ); 
      break;
    case TAB_LOG:
      mainWindow->logFileText = mainWindow->logCmdLineHistory.next( k );
      setText( mainWindow->logFileText ); 
      break;
    case TAB_RESULTS:
      mainWindow->resultsFileText = mainWindow-> resultsCmdLineHistory.next( k );
      setText( mainWindow->resultsFileText ); 
      break;
    }
  }

public:
  SimcraftCommandLine( SimcraftWindow* mw ) : mainWindow(mw) {}
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

class SimulateThread : public QThread
{
    Q_OBJECT
    SimcraftWindow* mainWindow;
    sim_t* sim;
    QString options;

public:
    void start( sim_t* s, const QString& o ) { sim=s; options=o; QThread::start(); }
    virtual void run();
    SimulateThread( SimcraftWindow* mw ) : mainWindow(mw), sim(0) {}
};

class ImportThread : public QThread
{
    Q_OBJECT
    SimcraftWindow* mainWindow;
    sim_t* sim;

public:
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
    ImportThread( SimcraftWindow* mw ) : mainWindow(mw), sim(0), player(0) {}
};

#endif // SIMCRAFTQT_H
