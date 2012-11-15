// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SIMULATIONCRAFTQT_H
#define SIMULATIONCRAFTQT_H

#include "simulationcraft.hpp"
#include <QtGui/QtGui>
#include <QtWebKit>
#include <QTranslator>

#define TAB_WELCOME   0
#define TAB_OPTIONS   1
#define TAB_IMPORT    2
#define TAB_SIMULATE  3
#define TAB_OVERRIDES 4
#define TAB_HELP      5
#define TAB_LOG       6
#define TAB_RESULTS   7
#define TAB_SITE      8
class SimulationCraftWindow;
#ifdef SC_PAPERDOLL
#define TAB_PAPERDOLL 9
#include "simcpaperdoll.hpp"
class Paperdoll;
class PaperdollProfile;
#endif

#define TAB_BATTLE_NET 0
#define TAB_CHAR_DEV   1
#define TAB_RAWR       2
#define TAB_BIS        2
#define TAB_HISTORY    3
#define TAB_CUSTOM     4

#define HISTORY_VERSION "5.00"

class SimulationCraftTextEdit;
class SimulationCraftWebView;
class SimulationCraftCommandLine;
class SimulateThread;
#ifdef SC_PAPERDOLL
class PaperdollThread;
#endif
class ImportThread;

class StringHistory : public QStringList
{
public:
  int current_index;
  StringHistory() : current_index( -1 ) {}
  QString current()
  {
    if ( current_index < 0 ) return "";
    if ( current_index >= size() ) return "";
    return at( current_index );
  }
  QString backwards()
  {
    if ( current_index < size() ) current_index++;
    return current();
  }
  QString forwards()
  {
    if ( current_index >= 0 ) current_index--;
    return current();
  }
  QString next( int k )
  {
    return ( k == Qt::Key_Up ) ? backwards() : forwards();
  }
  void add( QString s )
  {
    current_index = -1;
    if ( size() > 0 ) if ( at( 0 ) == s ) return;
    prepend( s );
  }
  QString start()
  {
    current_index = 0;
    return current();
  }
};

class PersistentCookieJar : public QNetworkCookieJar
{
public:
  QString fileName;
  PersistentCookieJar( const QString& fn ) : fileName( fn ) {}
  virtual ~PersistentCookieJar() {}
  void load();
  void save();
};

class SC_PlainTextEdit : public QPlainTextEdit
{
private:
  QTextCharFormat textformat_default;
  QTextCharFormat textformat_error;
public:
  SC_PlainTextEdit() :
    QPlainTextEdit()
  {
    textformat_error.setFontPointSize( 20 );

    setAcceptDrops( false );
    setLineWrapMode( QPlainTextEdit::NoWrap );
  }

  void setformat_error()
  { setCurrentCharFormat( textformat_error );}

  void resetformat()
  { setCurrentCharFormat( textformat_default ); }

  /*
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
  */
};

class ReforgeButtonGroup : public QButtonGroup
{
  Q_OBJECT
public:
  ReforgeButtonGroup( QObject* parent = 0 );

private:
  int selected;

public slots:
  void setSelected( int state );
};

class SimulationCraftWindow : public QWidget
{
  Q_OBJECT

public:
  qint32 historyWidth, historyHeight;
  qint32 historyMaximized;
  QWidget *customGearTab;
  QWidget *customTalentsTab;
  QWidget *customGlyphsTab;
  QTabWidget* mainTab;
  QTabWidget* optionsTab;
  QTabWidget* importTab;
  QTabWidget* resultsTab;
  QTabWidget *createCustomProfileDock;
#ifdef SC_PAPERDOLL
  QTabWidget* paperdollTab;
  Paperdoll* paperdoll;
  PaperdollProfile* paperdollProfile;
#endif
  struct choices_t
  {
    // options
    QComboBox* version;
    QComboBox* world_lag;
    QComboBox* aura_delay;
    QComboBox* iterations;
    QComboBox* fight_length;
    QComboBox* fight_variance;
    QComboBox* fight_style;
    QComboBox* target_level;
    QComboBox* target_race;
    QComboBox* num_target;
    QComboBox* player_skill;
    QComboBox* threads;
    QComboBox* armory_region;
    QComboBox* armory_spec;
    QComboBox* default_role;
    QComboBox* debug;
    QComboBox* report_pets;
    QComboBox* print_style;
    QComboBox* statistics_level;
    QComboBox* deterministic_rng;
    QComboBox* center_scale_delta;
    // scaling
    QComboBox* scale_over;
    QComboBox* plots_points;
    QComboBox* plots_step;
    QComboBox* reforgeplot_amount;
    QComboBox* reforgeplot_step;
  } choice;

  QListWidget* itemDbOrder;
  QButtonGroup* buffsButtonGroup;
  QButtonGroup* debuffsButtonGroup;
  QButtonGroup* scalingButtonGroup;
  QButtonGroup* plotsButtonGroup;
  ReforgeButtonGroup* reforgeplotsButtonGroup;
  SimulationCraftWebView* battleNetView;
  SimulationCraftWebView* charDevView;
  SimulationCraftWebView* siteView;
  SimulationCraftWebView* helpView;
  SimulationCraftWebView* visibleWebView;
  PersistentCookieJar* charDevCookies;
  QPushButton* rawrButton;
  QByteArray rawrDialogState;
  //SimulationCraftTextEdit* rawrText;
  SC_PlainTextEdit* rawrText;
  QListWidget* historyList;
  //SimulationCraftTextEdit* simulateText;
  //SimulationCraftTextEdit* overridesText;
  SC_PlainTextEdit* simulateText;
  SC_PlainTextEdit* overridesText;
  SC_PlainTextEdit* logText;
  QPushButton* backButton;
  QPushButton* forwardButton;
  SimulationCraftCommandLine* cmdLine;
  QProgressBar* progressBar;
  QPushButton* mainButton;
  QGroupBox* cmdLineGroupBox;
  QGroupBox* createCustomCharData;


  QTimer* timer;
  ImportThread* importThread;
  SimulateThread* simulateThread;
#ifdef SC_PAPERDOLL
  PaperdollThread* paperdollThread;
#endif

  sim_t* sim;
  std::string simPhase;
  int simProgress;
  int simResults;
  QStringList resultsHtml;

  QString cmdLineText;
  QString logFileText;
  QString resultsFileText;

  StringHistory simulateCmdLineHistory;
  StringHistory logCmdLineHistory;
  StringHistory resultsCmdLineHistory;
  StringHistory optionsHistory;
  StringHistory simulateTextHistory;
  StringHistory overridesTextHistory;

  void    startImport( int tab, const QString& url );
  void    startSim();
  sim_t*  initSim();
  void    deleteSim();
  QString get_globalSettings();
  QString get_db_order() const;
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
  void createBuffsDebuffsTab();
  void createScalingTab();
  void createPlotsTab();
  void createReforgePlotsTab();
  void createImportTab();
  void createRawrTab();
  void createBestInSlotTab();
  void createCustomTab();
  void createSimulateTab();
  void createOverridesTab();
  void createHelpTab();
  void createLogTab();
  void createResultsTab();
  void createSiteTab();
  void createToolTips();
#ifdef SC_PAPERDOLL
  void createPaperdoll();
#endif
  void createItemDataSourceSelector( QFormLayout* );
  void updateVisibleWebView( SimulationCraftWebView* );

protected:
  virtual void closeEvent( QCloseEvent* );

private slots:
  void importFinished();
  void simulateFinished();
#ifdef SC_PAPERDOLL
  void paperdollFinished();
  void start_paperdoll_sim();
#endif
  void updateSimProgress();
  void cmdLineReturnPressed();
  void cmdLineTextEdited( const QString& );
  void backButtonClicked( bool checked=false );
  void forwardButtonClicked( bool checked=false );
  void mainButtonClicked( bool checked=false );
  void mainTabChanged( int index );
  void importTabChanged( int index );
  void resultsTabChanged( int index );
  void resultsTabCloseRequest( int index );
  void rawrButtonClicked( bool checked=false );
  void historyDoubleClicked( QListWidgetItem* item );
  void bisDoubleClicked( QTreeWidgetItem* item, int col );
  void allBuffsChanged( bool checked );
  void allDebuffsChanged( bool checked );
  void allScalingChanged( bool checked );
  void armoryRegionChanged( const QString& region );

public:
  SimulationCraftWindow( QWidget *parent = 0 );
};

class SimulationCraftCommandLine : public QLineEdit
{
  SimulationCraftWindow* mainWindow;

protected:
  virtual void keyPressEvent( QKeyEvent* e )
  {
    int k = e->key();
    if ( k != Qt::Key_Up && k != Qt::Key_Down )
    {
      QLineEdit::keyPressEvent( e );
      return;
    }
    switch ( mainWindow->mainTab->currentIndex() )
    {
    case TAB_WELCOME:
    case TAB_OPTIONS:
    case TAB_SIMULATE:
    case TAB_OVERRIDES:
      mainWindow->cmdLineText = mainWindow->simulateCmdLineHistory.next( k );
      setText( mainWindow->cmdLineText );
      break;
    case TAB_IMPORT:
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
  SimulationCraftCommandLine( SimulationCraftWindow* mw ) : mainWindow( mw ) {}
};

class SimulationCraftWebPage : public QWebPage
{
  Q_OBJECT
public:
  QString userAgentForUrl( const QUrl& /* url */ ) const
  { return QString( "simulationcraft_gui" ); }
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
    if ( mainWindow->visibleWebView == this )
    {
      mainWindow->progressBar->setValue( progress );
    }
  }
  void loadFinishedSlot( bool /* ok */ )
  {
    progress = 100;
    if ( mainWindow->visibleWebView == this )
    {
      mainWindow->progressBar->setValue( 100 );
    }
  }
  void urlChangedSlot( const QUrl& url )
  {
    if ( mainWindow->visibleWebView == this )
    {
      QString s = url.toString();
      if ( s == "about:blank" ) s = "results.html";
      mainWindow->cmdLine->setText( s );
    }
  }
  void linkClickedSlot( const QUrl& url )
  {
    setUrl( url );
  }

public:
  SimulationCraftWebView( SimulationCraftWindow* mw ) :
    mainWindow( mw ), progress( 0 )
  {
    connect( this, SIGNAL( loadProgress( int ) ),       this, SLOT( loadProgressSlot( int ) ) );
    connect( this, SIGNAL( loadFinished( bool ) ),      this, SLOT( loadFinishedSlot( bool ) ) );
    connect( this, SIGNAL( urlChanged( const QUrl& ) ), this, SLOT( urlChangedSlot( const QUrl& ) ) );

    connect( page(), SIGNAL( linkClicked( const QUrl& ) ), this, SLOT( linkClickedSlot( const QUrl& ) ) );
    page() -> setLinkDelegationPolicy( QWebPage::DelegateExternalLinks );
    SimulationCraftWebPage* page = new SimulationCraftWebPage();
    setPage( ( SimulationCraftWebPage* ) page );
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
  SimulateThread( SimulationCraftWindow* mw ) : mainWindow( mw ), sim( 0 ) {}
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
  QString item_db_sources;
  player_t* player;

  void importBattleNet();
  void importCharDev();
  void importRawr();

  void start( sim_t* s, int t, const QString& u, const QString& sources )
  { sim=s; tab=t; url=u; profile=""; item_db_sources = sources; player=0; QThread::start(); }
  virtual void run();
  ImportThread( SimulationCraftWindow* mw ) : mainWindow( mw ), sim( 0 ), player( 0 ) {}
};

#ifdef SC_PAPERDOLL

class PaperdollThread : public QThread
{
  Q_OBJECT
  SimulationCraftWindow* mainWindow;
  sim_t* sim;
  QString options;
  bool success;

public:
  player_t* player;

  void start( sim_t* s, player_t* p, const QString& o ) { sim=s; player=p; options=o; success=false; QThread::start(); }
  virtual void run();
  PaperdollThread( SimulationCraftWindow* mw ) : mainWindow( mw ), sim( 0 ), success( false ), player( 0 ) {}
};
#endif

#endif // SIMULATIONCRAFTQT_H
