// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SIMULATIONCRAFTQT_H
#define SIMULATIONCRAFTQT_H

#include "simulationcraft.hpp"
#include <QtGui/QtGui>
#include <QtWebKit/QtWebKit>
#include <QtCore/QTranslator>
#ifdef QT_VERSION_5
#include <QtWidgets/QtWidgets>
#include <QtWebKitWidgets/QtWebKitWidgets>
#endif

class SC_MainWindow;
#ifdef SC_PAPERDOLL
#include "simcpaperdoll.hpp"
class Paperdoll;
class PaperdollProfile;
#endif

enum main_tabs_e
{
  TAB_WELCOME = 0,
  TAB_OPTIONS,
  TAB_IMPORT,
  TAB_SIMULATE,
  TAB_OVERRIDES,
  TAB_HELP,
  TAB_LOG,
  TAB_RESULTS,
  TAB_SITE
#ifdef SC_PAPERDOLL
  , TAB_PAPERDOLL
#endif
};

enum import_tabs_e
{
  TAB_BATTLE_NET = 0,
  TAB_CHAR_DEV,
  TAB_RAWR,
  TAB_BIS,
  TAB_HISTORY,
  TAB_CUSTOM
};

class SC_WebView;
class SC_CommandLine;
class SimulateThread;
#ifdef SC_PAPERDOLL
class PaperdollThread;
#endif
class ImportThread;

// ============================================================================
// SC_StringHistory
// ============================================================================

class SC_StringHistory : public QStringList
{
public:
  int current_index;
  SC_StringHistory() : current_index( -1 ) {}
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

// ============================================================================
// PersistentCookieJar
// ============================================================================

class PersistentCookieJar : public QNetworkCookieJar
{
public:
  QString fileName;
  PersistentCookieJar( const QString& fn ) : fileName( fn ) {}
  virtual ~PersistentCookieJar() {}
  void load();
  void save();
};

// ============================================================================
// SC_PlainTextEdit
// ============================================================================

class SC_PlainTextEdit : public QPlainTextEdit
{
  Q_OBJECT
private:
  QTextCharFormat textformat_default;
  QTextCharFormat textformat_error;
public:
  bool edited_by_user;

  SC_PlainTextEdit( QWidget* parent = 0, bool accept_drops = true ) :
    QPlainTextEdit( parent ),
    edited_by_user( false )
  {
    textformat_error.setFontPointSize( 20 );

    setAcceptDrops( accept_drops );
    setLineWrapMode( QPlainTextEdit::NoWrap );

    connect( this, SIGNAL( textChanged() ), this, SLOT( text_edited() ) );
  }

  void setformat_error()
  { setCurrentCharFormat( textformat_error );}

  void resetformat()
  { setCurrentCharFormat( textformat_default ); }

  /*
  protected:
  virtual void dragEnterEvent( QDragEnterEvent* e )
  {
    e -> acceptProposedAction();
  }
  virtual void dropEvent( QDropEvent* e )
  {
    appendPlainText( e -> mimeData()-> text() );
    e -> acceptProposedAction();
  }
  */

private slots:

  void text_edited()
  { edited_by_user = true; }
};

// ============================================================================
// SC_ReforgeButtonGroup
// ============================================================================

class SC_ReforgeButtonGroup : public QButtonGroup
{
  Q_OBJECT
public:
  SC_ReforgeButtonGroup( QObject* parent = 0 );

private:
  int selected;

public slots:
  void setSelected( int state );
};


// ============================================================================
// SC_enumeratedTabWidget template
// ============================================================================

template <typename E>
class SC_enumeratedTab : public QTabWidget
{
public:
  SC_enumeratedTab( QWidget* parent = 0 ) :
    QTabWidget( parent )
  {

  }

  E currentTab()
  { return static_cast<E>( currentIndex() ); }

  void setCurrentTab( E t )
  { return setCurrentIndex( static_cast<int>( t ) ); }
};

// ============================================================================
// SC_MainTabWidget
// ============================================================================

class SC_MainTab : public SC_enumeratedTab<main_tabs_e>
{
  Q_OBJECT
public:
  SC_MainTab( QWidget* parent = 0 ) :
    SC_enumeratedTab<main_tabs_e>( parent )
  {

  }
};

// ============================================================================
// SC_WelcomeTabWidget
// ============================================================================

class SC_WelcomeTabWidget : public QWebView
{
  Q_OBJECT
public:
  SC_WelcomeTabWidget( SC_MainWindow* parent = nullptr );
};

// ============================================================================
// SC_OptionsTabWidget
// ============================================================================
class SC_OptionsTab : public QTabWidget
{
  Q_OBJECT
public:
  SC_OptionsTab( SC_MainWindow* parent );

  void    decodeOptions( QString );
  QString encodeOptions();
  QString get_db_order() const;
  QString get_globalSettings();
  QString mergeOptions();
  QString get_active_spec();
  QString get_player_role();

  void createToolTips();
  QListWidget* itemDbOrder;
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
    QComboBox* tmi_boss;
    QComboBox* tmi_actor_only;
    QComboBox* debug;
    QComboBox* report_pets;
    QComboBox* print_style;
    QComboBox* statistics_level;
    QComboBox* deterministic_rng;
    QComboBox* center_scale_delta;
    QComboBox* challenge_mode;
    // scaling
    QComboBox* scale_over;
    QComboBox* plots_points;
    QComboBox* plots_step;
    QComboBox* reforgeplot_amount;
    QComboBox* reforgeplot_step;
  } choice;

  QButtonGroup* buffsButtonGroup;
  QButtonGroup* debuffsButtonGroup;
  QButtonGroup* scalingButtonGroup;
  QButtonGroup* plotsButtonGroup;
  SC_ReforgeButtonGroup* reforgeplotsButtonGroup;
protected:
  SC_MainWindow* mainWindow;
  void createGlobalsTab();
  void createBuffsDebuffsTab();
  void createScalingTab();
  void createPlotsTab();
  void createReforgePlotsTab();
  void createItemDataSourceSelector( QFormLayout* );

private slots:
  void allBuffsChanged( bool checked );
  void allDebuffsChanged( bool checked );
  void allScalingChanged( bool checked );
  void _optionsChanged();
signals:
  void armory_region_changed( const QString& );
  void optionsChanged(); // FIXME: hookup to everything

};
// ============================================================================
// SC_ImportTabWidget
// ============================================================================

class SC_ImportTab : public SC_enumeratedTab<import_tabs_e>
{
  Q_OBJECT
public:
  SC_ImportTab( QWidget* parent = 0 ) :
    SC_enumeratedTab<import_tabs_e>( parent )
  {
  }
};

// ============================================================================
// SC_SimulateTabWidget
// ============================================================================

class SC_SimulateTab : public QTabWidget
{
  Q_OBJECT
public:
  SC_SimulateTab( QWidget* parent = nullptr ) :
    QTabWidget( parent )
  {
  }

  int add_Text( const QString& text, const QString& tab_name )
  {
    SC_PlainTextEdit* s = new SC_PlainTextEdit( this );
    s -> setPlainText( text );
    int i = addTab( s, tab_name );
    setCurrentIndex( count() - 1 );
    return i;
  }

  SC_PlainTextEdit* current_Text()
  {
    return static_cast<SC_PlainTextEdit*>( currentWidget() );
  }

  void set_Text( const QString& text )
  {
    SC_PlainTextEdit* current_s = static_cast<SC_PlainTextEdit*>( currentWidget() );
    current_s -> setPlainText( text );
  }

  void append_Text( const QString& text )
  {
    SC_PlainTextEdit* current_s = static_cast<SC_PlainTextEdit*>( currentWidget() );
    current_s -> appendPlainText( text );
  }
};

// ============================================================================
// SC_MainWindow
// ============================================================================

class SC_MainWindow : public QWidget
{
  Q_OBJECT
public:
  qint32 historyWidth, historyHeight;
  qint32 historyMaximized;
  QWidget* customGearTab;
  QWidget* customTalentsTab;
  QWidget* customGlyphsTab;
  SC_MainTab* mainTab;
  SC_OptionsTab* optionsTab;
  SC_ImportTab* importTab;
  SC_SimulateTab* simulateTab;
  QTabWidget* resultsTab;
  QTabWidget* createCustomProfileDock;
#ifdef SC_PAPERDOLL
  QTabWidget* paperdollTab;
  Paperdoll* paperdoll;
  PaperdollProfile* paperdollProfile;
#endif


  SC_WebView* battleNetView;
  SC_WebView* charDevView;
  SC_WebView* siteView;
  SC_WebView* helpView;
  SC_WebView* visibleWebView;
  PersistentCookieJar* charDevCookies;
  QPushButton* rawrButton;
  QByteArray rawrDialogState;
  SC_PlainTextEdit* rawrText;
  QListWidget* historyList;
  SC_PlainTextEdit* overridesText;
  SC_PlainTextEdit* logText;
  QPushButton* backButton;
  QPushButton* forwardButton;
  SC_CommandLine* cmdLine;
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
  sim_t* paperdoll_sim;
  std::string simPhase;
  int simProgress;
  int simResults;
//  QStringList resultsHtml;

  QString AppDataDir;
  QString TmpDir;

  QString cmdLineText;
  QString logFileText;
  QString resultsFileText;

  SC_StringHistory simulateCmdLineHistory;
  SC_StringHistory logCmdLineHistory;
  SC_StringHistory resultsCmdLineHistory;
  SC_StringHistory optionsHistory;
  SC_StringHistory overridesTextHistory;

  void    startImport( int tab, const QString& url );
  void    startSim();
  sim_t*  initSim();
  void    deleteSim( sim_t* sim, SC_PlainTextEdit* append_error_message = 0 );

  void saveLog();
  void saveResults();

  void loadHistory();
  void saveHistory();

  void createCmdLine();
  void createWelcomeTab();
  void createOptionsTab();
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
  void updateVisibleWebView( SC_WebView* );

protected:
  virtual void closeEvent( QCloseEvent* );

private slots:
  void importFinished();
  void simulateFinished( sim_t* );
#ifdef SC_PAPERDOLL
  void paperdollFinished();
  player_t* init_paperdoll_sim( sim_t*& );
  void start_intermediate_paperdoll_sim();
  void start_paperdoll_sim();
#endif
  void updateSimProgress();
  void cmdLineReturnPressed();
  void cmdLineTextEdited( const QString& );
  void backButtonClicked( bool checked = false );
  void forwardButtonClicked( bool checked = false );
  void mainButtonClicked( bool checked = false );
  void mainTabChanged( int index );
  void importTabChanged( int index );
  void resultsTabChanged( int index );
  void resultsTabCloseRequest( int index );
  void simulateTabCloseRequest( int index );
  void rawrButtonClicked( bool checked = false );
  void historyDoubleClicked( QListWidgetItem* item );
  void bisDoubleClicked( QTreeWidgetItem* item, int col );
  void armoryRegionChanged( const QString& region );

public:
  SC_MainWindow( QWidget *parent = 0 );
};

// ============================================================================
// SC_CommandLine
// ============================================================================

class SC_CommandLine : public QLineEdit
{
private:
  SC_MainWindow* mainWindow;

  virtual void keyPressEvent( QKeyEvent* e );
public:
  SC_CommandLine( SC_MainWindow* mw ) : mainWindow( mw ) {}
};

// ============================================================================
// SC_WebPage
// ============================================================================

class SC_WebPage : public QWebPage
{
  Q_OBJECT
public:
  explicit SC_WebPage( QObject* parent = 0 ) :
    QWebPage( parent )
  {}

  QString userAgentForUrl( const QUrl& /* url */ ) const
  { return QString( "simulationcraft_gui" ); }
};

// ============================================================================
// SC_WebView
// ============================================================================

class SC_WebView : public QWebView
{
  Q_OBJECT
public:
  SC_MainWindow* mainWindow;
  int progress;
  QString html_str;

  SC_WebView( SC_MainWindow* mw, QWidget* parent = 0, const QString& h = QString() ) :
    QWebView( parent ),
    mainWindow( mw ), progress( 0 ), html_str( h )
  {
    connect( this, SIGNAL( loadProgress( int ) ),        this, SLOT( loadProgressSlot( int ) ) );
    connect( this, SIGNAL( loadFinished( bool ) ),       this, SLOT( loadFinishedSlot( bool ) ) );
    connect( this, SIGNAL( urlChanged( const QUrl& ) ),  this, SLOT( urlChangedSlot( const QUrl& ) ) );
    connect( this, SIGNAL( linkClicked( const QUrl& ) ), this, SLOT( linkClickedSlot( const QUrl& ) ) );

    SC_WebPage* page = new SC_WebPage( this );
    setPage( page );
    page -> setLinkDelegationPolicy( QWebPage::DelegateExternalLinks );

    // Add QT Major Version to avoid "mysterious" problems resulting in qBadAlloc. Qt4 and Qt5 webcache do not like each other
    QDir dir( mainWindow -> TmpDir + QDir::separator() + "simc_webcache_qt" + std::string( QT_VERSION_STR ).substr( 0, 3 ).c_str() );
    if ( ! dir.exists() ) dir.mkpath( "." );

    QFileInfo fi( dir.absolutePath() );

    if ( fi.isDir() && fi.isWritable() )
    {
      QNetworkDiskCache* diskCache = new QNetworkDiskCache( this );
      diskCache -> setCacheDirectory( dir.absolutePath() );
      QString test = diskCache -> cacheDirectory();
      page -> networkAccessManager()->setCache( diskCache );
    }
    else
    {
      qDebug() << "Can't write webcache! sucks";
    }


  }
  virtual ~SC_WebView() {}

  void loadHtml()
  { setHtml( html_str ); }

private:
  void update_progress( int p )
  {
    progress = p;
    if ( mainWindow -> visibleWebView == this )
    {
      mainWindow -> progressBar -> setValue( progress );
    }
  }

private slots:
  void loadProgressSlot( int p )
  { update_progress( p ); }
  void loadFinishedSlot( bool /* ok */ )
  { update_progress( 100 ); }
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
  { load( url ); }
};

// ============================================================================
// SimulateThread
// ============================================================================

class SimulateThread : public QThread
{
  Q_OBJECT
  SC_MainWindow* mainWindow;
  sim_t* sim;

public:
  QString options;
  bool success;

  void start( sim_t* s, const QString& o ) { sim = s; options = o; success = false; QThread::start(); }
  virtual void run();
  SimulateThread( SC_MainWindow* mw ) : mainWindow( mw ), sim( 0 )
  {
    connect( this, SIGNAL( finished() ), this, SLOT( sim_finished() ) );
  }
private slots:
  void sim_finished()
  { emit simulationFinished( sim ); }

signals:
  void simulationFinished( sim_t* s );
};

class ImportThread : public QThread
{
  Q_OBJECT
  SC_MainWindow* mainWindow;
  sim_t* sim;

public:
  int tab;
  QString url;
  QString profile;
  QString item_db_sources;
  QString active_spec;
  QString m_role;
  player_t* player;

  void importBattleNet();
  void importCharDev();
  void importRawr();

  void start( sim_t* s, int t, const QString& u, const QString& sources, const QString& spec, const QString& role )
  { sim = s; tab = t; url = u; profile = ""; item_db_sources = sources; player = 0; active_spec = spec; m_role = role; QThread::start(); }
  virtual void run();
  ImportThread( SC_MainWindow* mw ) : mainWindow( mw ), sim( 0 ), player( 0 ) {}
};

#ifdef SC_PAPERDOLL

class PaperdollThread : public QThread
{
  Q_OBJECT
  SC_MainWindow* mainWindow;
  sim_t* sim;
  QString options;
  bool success;

public:
  player_t* player;

  void start( sim_t* s, player_t* p, const QString& o ) { sim = s; player = p; options = o; success = false; QThread::start(); }
  virtual void run();
  PaperdollThread( SC_MainWindow* mw ) : mainWindow( mw ), sim( 0 ), success( false ), player( 0 ) {}
};
#endif

#endif // SIMULATIONCRAFTQT_H
