// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SIMULATIONCRAFTQT_H
#define SIMULATIONCRAFTQT_H

#include <QtGui/QtGui>
#include <QtWebKit>
#include <simulationcraft.h>

#define TAB_WELCOME   0
#define TAB_OPTIONS   1
#define TAB_IMPORT    2
#define TAB_SIMULATE  3
#define TAB_OVERRIDES 4
#define TAB_EXAMPLES  5
#define TAB_LOG       6
#define TAB_RESULTS   7

#define TAB_ARMORY     0
#define TAB_WOWHEAD    1
#define TAB_CHARDEV    2
#define TAB_WARCRAFTER 3
#define TAB_RAWR       4
#define TAB_BIS        5
#define TAB_HISTORY    6

#define HISTORY_VERSION "1.4"

class SimulationCraftTextEdit;
class SimulationCraftWebView;
class SimulationCraftCommandLine;
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
    current_index = -1;
    if( size() > 0 ) if( at( 0 ) == s ) return;
    prepend( s );
  }
  QString start()
  {
    current_index = 0;
    return current();
  }
};

class SimulationCraftWindow : public QWidget
{
    Q_OBJECT

public:
    QTabWidget* mainTab;
    QTabWidget* optionsTab;
    QComboBox* patchChoice;
    QComboBox* latencyChoice;
    QComboBox* iterationsChoice;
    QComboBox* fightLengthChoice;
    QComboBox* fightStyleChoice;
    QComboBox* threadsChoice;
    QComboBox* smoothRNGChoice;
    QComboBox* armoryRegionChoice;
    QComboBox* armorySpecChoice;
    QButtonGroup* buffsButtonGroup;
    QButtonGroup* debuffsButtonGroup;
    QButtonGroup* scalingButtonGroup;
    QButtonGroup* plotsButtonGroup;
    QTabWidget* importTab;
    SimulationCraftWebView* armoryView;
    SimulationCraftWebView* wowheadView;
    SimulationCraftWebView* chardevView;
    SimulationCraftWebView* warcrafterView;
    SimulationCraftWebView* visibleWebView;
    QPushButton* rawrButton;
    QLabel* rawrDir;
    QByteArray rawrDialogState;
    QListWidget* rawrList;
    QListWidget* historyList;
    QTreeWidget* bisTree;
    SimulationCraftTextEdit* simulateText;
    SimulationCraftTextEdit* overridesText;
    QPlainTextEdit* logText;
    QTabWidget* resultsTab;
    QPushButton* backButton;
    QPushButton* forwardButton;
    SimulationCraftCommandLine* cmdLine;
    QProgressBar* progressBar;
    QPushButton* mainButton;
    QGroupBox* cmdLineGroupBox;

    QTimer* timer;
    ImportThread* importThread;
    SimulateThread* simulateThread;

    sim_t* sim;
    std::string simPhase;
    int simProgress;
    int simResults;
    QStringList resultsHtml;

    QString cmdLineText;
    QString rawrFileText;
    QString logFileText;
    QString resultsFileText;

    StringHistory simulateCmdLineHistory;
    StringHistory logCmdLineHistory;
    StringHistory resultsCmdLineHistory;
    StringHistory rawrCmdLineHistory;
    StringHistory optionsHistory;
    StringHistory simulateTextHistory;
    StringHistory overridesTextHistory;

    void    startImport( int tab, const QString& url );
    void    startSim();
    sim_t*  initSim();
    void    deleteSim();
    QString mergeOptions();

    void saveLog();
    void saveResults();

    void    decodeOptions( QString );
    QString encodeOptions();

    void loadHistory();
    void saveHistory();
    
    void createCmdLine();
    void createWelcomeTab();
    void createOptionsTab();
    void createGlobalsTab();
    void createBuffsTab();
    void createDebuffsTab();
    void createScalingTab();
    void createPlotsTab();
    void createImportTab();
    void createRawrTab();
    void createBestInSlotTab();
    void createSimulateTab();
    void createOverridesTab();
    void createExamplesTab();
    void createLogTab();
    void createResultsTab();
    void createToolTips();

protected:
    virtual void closeEvent( QCloseEvent* );

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
    void rawrButtonClicked( bool checked=false );
    void rawrDoubleClicked( QListWidgetItem* item );
    void historyDoubleClicked( QListWidgetItem* item );
    void bisDoubleClicked( QTreeWidgetItem* item, int col );
    void allBuffsChanged( bool checked );
    void allDebuffsChanged( bool checked );
    void allScalingChanged( bool checked );
    void armoryRegionChanged( const QString& region );

public:
    SimulationCraftWindow(QWidget *parent = 0);
};

class SimulationCraftTextEdit : public QPlainTextEdit
{
protected:
  virtual void dragEnterEvent( QDragEnterEvent* e )
  {
    e->acceptProposedAction();
  }
  virtual void dropEvent( QDropEvent* e )
  {
    appendPlainText( e->mimeData()->text() );
    e->acceptProposedAction();
  }
};

class SimulationCraftCommandLine : public QLineEdit
{
  SimulationCraftWindow* mainWindow;

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
    case TAB_OPTIONS:   
    case TAB_SIMULATE:  
    case TAB_OVERRIDES: 
      mainWindow->cmdLineText = mainWindow->simulateCmdLineHistory.next( k ); 
      setText( mainWindow->cmdLineText ); 
      break;
    case TAB_IMPORT:
      if( mainWindow->importTab->currentIndex() == TAB_RAWR )
      {
	mainWindow->rawrFileText = mainWindow->rawrCmdLineHistory.next( k ); 
	setText( mainWindow->rawrFileText ); 
      }
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
  SimulationCraftCommandLine( SimulationCraftWindow* mw ) : mainWindow(mw) {}
};

class SimulationCraftWebView : public QWebView
{
  Q_OBJECT

public:
  SimulationCraftWindow* mainWindow;
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
    ok=true;
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
  SimulationCraftWebView( SimulationCraftWindow* mw ) : 
    mainWindow(mw), progress(0) 
  {
    connect( this, SIGNAL(loadProgress(int)),       this, SLOT(loadProgressSlot(int)) );
    connect( this, SIGNAL(loadFinished(bool)),      this, SLOT(loadFinishedSlot(bool)) );
    connect( this, SIGNAL(urlChanged(const QUrl&)), this, SLOT(urlChangedSlot(const QUrl&)) );
  }
  virtual ~SimulationCraftWebView() {}
};

class SimulateThread : public QThread
{
    Q_OBJECT
    SimulationCraftWindow* mainWindow;
    sim_t* sim;

public:
    QString options;
    bool success;

    void start( sim_t* s, const QString& o ) { sim=s; options=o; success=false; QThread::start(); }
    virtual void run();
    SimulateThread( SimulationCraftWindow* mw ) : mainWindow(mw), sim(0) {}
};

class ImportThread : public QThread
{
    Q_OBJECT
    SimulationCraftWindow* mainWindow;
    sim_t* sim;

public:
    int tab;
    QString url;
    QString profile;
    player_t* player;

    void importArmory();
    void importWowhead();
    void importChardev();
    void importWarcrafter();
    void importRawr();

    void start( sim_t* s, int t, const QString& u ) { sim=s; tab=t; url=u; profile=""; player=0; QThread::start(); }
    virtual void run();
    ImportThread( SimulationCraftWindow* mw ) : mainWindow(mw), sim(0), player(0) {}
};

#endif // SIMULATIONCRAFTQT_H
