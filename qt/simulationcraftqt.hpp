// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SIMULATIONCRAFTQT_H
#define SIMULATIONCRAFTQT_H

#include "simulationcraft.hpp"
#include <QtGui/QtGui>

#if defined( SC_USE_WEBENGINE )
#include <QtWebEngineWidgets/QtWebEngineWidgets>
#include <QtWebEngine/QtWebEngine>
#else
#include <QtWebKitWidgets/QtWebKitWidgets>
#include <QtWebKit/QtWebKit>
#endif
#include <QtCore/QTranslator>
#include <QtNetwork/QtNetwork>

#if defined( Q_OS_MAC )
#include <CoreFoundation/CoreFoundation.h>
#endif
#include <QtWidgets/QtWidgets>

class SC_MainWindow;
class SC_SearchBox;
class SC_TextEdit;
class SC_RelativePopup;
class SC_MainWindowCommandLine;

#include "util/sc_recentlyclosed.hpp" // remove once implementations are moved to source files
#include "util/sc_searchbox.hpp" // remove once implementations are moved to source files
#include "util/sc_textedit.hpp" // remove once implementations are moved to source files

#include "sc_importWindow.hpp"

#if defined( Q_OS_MAC ) || defined( VS_NEW_BUILD_SYSTEM )
#include "sc_importWindow.hpp"
#endif /* Q_OS_MAC || VS_NEW_BUILD_SYSTEM */

enum main_tabs_e
{
  TAB_WELCOME = 0,
  TAB_IMPORT,
  TAB_OPTIONS,
  TAB_SIMULATE,
  TAB_RESULTS,
  TAB_OVERRIDES,
  TAB_HELP,
  TAB_LOG,
  TAB_SPELLQUERY, 
  TAB_COUNT
};

enum import_tabs_e
{
  TAB_IMPORT_NEW = 0,
  TAB_ADDON,
  TAB_BIS,
  TAB_RECENT,
  TAB_CUSTOM
};

class SC_WebView;
//class SC_SpellQueryTab;
class SC_CommandLine;
class SC_SimulateThread;
class SC_AddonImportTab;
class SC_ImportThread;

#if defined( SC_USE_WEBENGINE )
typedef QWebEngineView SC_WebEngineView;
typedef QWebEnginePage SC_WebEnginePage;
#else
typedef QWebView SC_WebEngineView;

typedef QWebPage SC_WebEnginePage;
#endif

inline QString webEngineName()
{
#if defined(SC_USE_WEBKIT)
  return "WebKit";
#else
  return "WebEngine";
#endif
}

struct SC_PATHS
{
    static QStringList getDataPaths();
};

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
// SC_ReforgeButtonGroup
// ============================================================================

class SC_ReforgeButtonGroup : public QButtonGroup
{
  Q_OBJECT
public:
  SC_ReforgeButtonGroup( QObject* parent = nullptr  );

private:
  int selected;

private slots:
  void setSelected(int id, bool checked );
};


// ============================================================================
// SC_enumeratedTabWidget template
// ============================================================================

template <typename E>
class SC_enumeratedTab : public QTabWidget
{
  QList< QPair< Qt::Key, QList< Qt::KeyboardModifier > > > ignoreKeys;
public:
  SC_enumeratedTab( QWidget* parent = nullptr  ) :
    QTabWidget( parent )
  {

  }

  E currentTab()
  { return static_cast<E>( currentIndex() ); }

  void setCurrentTab( E t )
  { return setCurrentIndex( static_cast<int>( t ) ); }

  void addIgnoreKeyPressEvent( Qt::Key k, QList< Qt::KeyboardModifier > s)
  {
    QPair< Qt::Key, QList<Qt::KeyboardModifier > > p(k, s);
    if ( ! ignoreKeys.contains( p ) )
      ignoreKeys.push_back(p);
  }

  bool removeIgnoreKeyPressEvent( Qt::Key k, QList< Qt::KeyboardModifier > s)
  {
    QPair< Qt::Key, QList<Qt::KeyboardModifier > > p(k, s);
    return ignoreKeys.removeAll( p );
  }

  void removeAllIgnoreKeyPressEvent ( )
  {
    QList < QPair< Qt::Key, QList< Qt::KeyboardModifier > > > emptyList;
    ignoreKeys = emptyList;
  }

protected:
  void keyPressEvent( QKeyEvent* e ) override
  {
    int k = e -> key();
    Qt::KeyboardModifiers m = e -> modifiers();

    QList< QPair< Qt::Key, QList<Qt::KeyboardModifier > > >::iterator i = ignoreKeys.begin();
    for (; i != ignoreKeys.end(); ++i)
    {
      if ( (*i).first == k )
      {
        bool passModifiers = true;
        QList< Qt::KeyboardModifier >::iterator j = (*i).second.begin();

        for (; j != (*i).second.end(); ++j)
        {
          if ( m.testFlag( (*j) ) == false )
          {
            passModifiers = false;
            break;
          }
        }

        if ( passModifiers )
        {
          // key combination matches, send key to base classe's base
          QWidget::keyPressEvent( e );
          return;
        }
      }
    }
    // no key match
    QTabWidget::keyPressEvent( e );
  }
};

// ============================================================================
// SC_QueueItemModel
// ============================================================================

class SC_QueueItemModel : public QStandardItemModel
{
  Q_OBJECT
public:
  SC_QueueItemModel( QObject* parent = nullptr  ) :
    QStandardItemModel( parent )
  {
    connect( this, SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ),  this, SLOT( rowsRemovedSlot( const QModelIndex&, int, int ) ) );
    connect( this, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT( rowsInsertedSlot( const QModelIndex&, int, int ) ) );
  }
  void enqueue( const QString& title, const QString& options, const QString& fullOptions )
  {
    QStandardItem* item = new QStandardItem;
    item -> setData( QVariant::fromValue< QString >( title ), Qt::DisplayRole );
    item -> setData( QVariant::fromValue< QString >( options ), Qt::UserRole );
    item -> setData( QVariant::fromValue< QString >( fullOptions ), Qt::UserRole + 1 );

    appendRow( item );
  }
  std::tuple<QString, QString> dequeue()
  {
    QModelIndex indx = index( 0, 0 );
    std::tuple<QString, QString> retval;

    if ( indx.isValid() )
    {
      retval = std::make_tuple( indx.data( Qt::DisplayRole ).toString(),
                                indx.data( Qt::UserRole + 1 ).toString() );
      removeRow( 0 );
    }
    return retval;
  }

  bool isEmpty() const
  {
    return rowCount() <= 0;
  }
private slots:
  void rowsRemovedSlot( const QModelIndex& parent, int start, int end )
  {
    Q_UNUSED( parent );
    Q_UNUSED( start );
    Q_UNUSED( end );
  }
  void rowsInsertedSlot( const QModelIndex& parent, int start, int end )
  {
    Q_UNUSED( parent );
    int rowsAdded = ( end - start ) + 1; // if only one is added, end - start = 0
    if ( rowsAdded == rowCount() )
    {
      emit( firstItemWasAdded() );
    }
  }
signals:
  void firstItemWasAdded();
};

// ============================================================================
// SC_QueueListView
// ============================================================================

class SC_QueueListView : public QWidget
{
  Q_OBJECT
  QListView* listView;
  SC_QueueItemModel* model;
  int minimumItemsToShow;
public:
  SC_QueueListView( SC_QueueItemModel* model,
      int minimumItemsToShowWidget = 1,
      QWidget* parent = nullptr  ) :
    QWidget( parent ),
    listView( 0 ),
    model( model ),
    minimumItemsToShow( minimumItemsToShowWidget )
  {
    init();
  }
  SC_QueueItemModel* getModel()
  {
    return model;
  }
  void setMinimumNumberOfItemsToShowWidget( int number )
  {
    minimumItemsToShow = number;
    tryShow();
    tryHide();
  }
protected:
  void init()
  {
    QVBoxLayout* widgetLayout = new QVBoxLayout;
    widgetLayout -> addWidget( initListView() );

    QWidget* buttons = new QWidget( this );
    QPushButton* buttonOne = new QPushButton( tr( "test" ) );

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout -> addWidget( buttonOne );

    buttons -> setLayout( buttonsLayout );

    widgetLayout -> addWidget( buttons );
    setLayout( widgetLayout );

    tryHide();
  }
  QListView* initListView()
  {
    delete listView;
    listView = new QListView( this );

    listView -> setEditTriggers( QAbstractItemView::NoEditTriggers );
    listView -> setSelectionMode( QListView::SingleSelection );
    listView -> setModel( model );

    if ( model != 0 )
    {
      connect( model, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT( rowsInserted() ) );
      connect( model, SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ),  this, SLOT( rowsRemoved() ) );
    }

    QAction* remove = new QAction( tr( "Remove" ), listView );
    listView -> addAction( remove );
    listView -> setContextMenuPolicy( Qt::ActionsContextMenu );

    connect( remove, SIGNAL( triggered( bool ) ), this, SLOT( removeCurrentItem() ) );

    return listView;
  }
  void tryHide()
  {
    if ( model != 0 ) // gcc gave a weird error when making this one if
    {
      if ( model -> rowCount() < minimumItemsToShow &&
           minimumItemsToShow >= 0 )
      {
        if ( isVisible() )
        {
          hide();
        }
      }
    }
  }
  void tryShow()
  {
    if ( model != 0 )
    {
      if ( model -> rowCount() >= minimumItemsToShow )
      {
        if ( ! isVisible() )
        {
          show();
        }
      }
    }
  }
private slots:
  void removeCurrentItem()
  {
    QItemSelectionModel* selection = listView -> selectionModel();
    QModelIndex index = selection -> currentIndex();
    model -> removeRow( index.row(), selection -> currentIndex().parent() );
  }
  void rowsInserted()
  {
    tryShow();
  }
  void rowsRemoved()
  {
    tryHide();
  }
};

// ============================================================================
// SC_CommandLine
// ============================================================================

class SC_CommandLine : public QLineEdit
{
  Q_OBJECT
public:
  SC_CommandLine( QWidget* parent = nullptr  );
signals:
  void switchToLeftSubTab();
  void switchToRightSubTab();
  void currentlyViewedTabCloseRequest();
};

// ============================================================================
// SC_MainNavigationWidget
// ============================================================================

// ============================================================================
// SC_MainTabWidget
// ============================================================================

class SC_MainTab : public SC_enumeratedTab<main_tabs_e>
{
  Q_OBJECT
public:
  SC_MainTab( QWidget* parent = nullptr  ) :
    SC_enumeratedTab<main_tabs_e>( parent )
  {

  }
};

// ============================================================================
// SC_ResultTabWidget
// ============================================================================

enum result_tabs_e
{
  TAB_HTML = 0,
  TAB_TEXT,
  TAB_JSON,
  TAB_PLOTDATA
};

class SC_ResultTab : public SC_RecentlyClosedTab
{
  Q_OBJECT
  SC_MainWindow* mainWindow;
public:
  SC_ResultTab( SC_MainWindow* );
public slots:
  void TabCloseRequest( int index )
  {
    assert( index < count() );
    if ( count() > 0 )
    {
      int confirm = QMessageBox::question( this, tr( "Close Results Tab" ), tr( "Do you really want to close these results?" ), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );
      if ( confirm == QMessageBox::Yes )
      {
        setCurrentIndex( index - 1 );
        removeTab( index );
      }
    }
  }
};

class SC_SingleResultTab : public SC_enumeratedTab<result_tabs_e>
{
  Q_OBJECT
  SC_MainWindow* mainWindow;
public:
  SC_SingleResultTab( SC_MainWindow* mw, QWidget* parent = nullptr  ) :
    SC_enumeratedTab<result_tabs_e>( parent ),
    mainWindow( mw )
  {
    connect( this, SIGNAL( currentChanged( int ) ), this, SLOT( TabChanged( int ) ) );
  }

  void save_result();

public slots:
  void TabChanged( int index );
};

// ============================================================================
// SC_ImportTabWidget
// ============================================================================

class SC_ImportTab: public SC_enumeratedTab < import_tabs_e >
{
  Q_OBJECT
  public:
      SC_ImportTab( QWidget* parent = nullptr );
      SC_AddonImportTab* addonTab;
};

// ==========================================================================
// Utilities
// ==========================================================================


class SC_OptionsTab;
class SC_SpellQueryTab;

// ============================================================================
// SC_ComboBoxIntegerValidator
// ============================================================================

class SC_ComboBoxIntegerValidator: public QValidator
{
  Q_OBJECT
    int lowerBoundInclusive;
  int upperBoundInclusive;
  int lowerBoundDigitCount;
  int upperBoundDigitCount;
  QRegExp nonIntegerRegExp;
  QComboBox* comboBox;
  public:
  SC_ComboBoxIntegerValidator( int lowerBoundIntegerInclusive,
    int upperBoundIntegerInclusive,
    QComboBox* parent ):
    QValidator( parent ),
    lowerBoundInclusive( lowerBoundIntegerInclusive ),
    upperBoundInclusive( upperBoundIntegerInclusive ),
    lowerBoundDigitCount( 0 ),
    upperBoundDigitCount( 0 ),
    nonIntegerRegExp( "\\D*" ),
    comboBox( parent )
  {
    Q_ASSERT( lowerBoundInclusive <= upperBoundInclusive && "Invalid Arguments" );

    lowerBoundDigitCount = util::numDigits( lowerBoundInclusive );
    upperBoundDigitCount = util::numDigits( upperBoundInclusive );
  }

  static SC_ComboBoxIntegerValidator* CreateBoundlessValidator( QComboBox* parent = nullptr  )
  {
    return new SC_ComboBoxIntegerValidator( std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), parent );
  }

  static SC_ComboBoxIntegerValidator* CreateUpperBoundValidator( int upperBound, QComboBox* parent = nullptr  )
  {
    return new SC_ComboBoxIntegerValidator( std::numeric_limits<int>::min(), upperBound, parent );
  }

  static SC_ComboBoxIntegerValidator* CreateLowerBoundValidator( int lowerBound, QComboBox* parent = nullptr  )
  {
    return new SC_ComboBoxIntegerValidator( lowerBound, std::numeric_limits<int>::max(), parent );
  }

  static QComboBox* ApplyValidatorToComboBox( QValidator* validator, QComboBox* comboBox )
  {
    if ( comboBox != 0 )
    {
      if ( validator != 0 )
      {
        comboBox -> setEditable( true );
        comboBox -> setValidator( validator );
      }
    }
    return comboBox;
  }

  State isNumberValid( int number ) const
  {
    if ( isNumberInRange( number ) )
    {
      return QValidator::Acceptable;
    }
    else
    {
      // number is not in range... maybe it COULD be in range if the user types more
      if ( util::numDigits( number ) < lowerBoundDigitCount )
      {
        return QValidator::Intermediate;
      }
      // has enough digits... not valid
    }

    return QValidator::Invalid;
  }

  bool isNumberInRange( int number ) const
  {
    return (number >= lowerBoundInclusive &&
      number <= upperBoundInclusive);
  }

  void stripNonNumbersAndAdjustCursorPos( QString& input, int& cursorPos ) const
  {
    // remove erroneous characters
    QString modifiedInput = input.remove( nonIntegerRegExp );
    if ( cursorPos > 0 )
    {
      // move the cursor to the left by how many characters to the left gets removed
      QString charactersLeftOfCursor = input.leftRef( cursorPos ).toString();
      int characterCountLeftOfCursor = charactersLeftOfCursor.length();
      // count how many characters are removed left of cursor
      charactersLeftOfCursor = charactersLeftOfCursor.remove( nonIntegerRegExp );
      int removedCharacterCountLeftOfCursor = characterCountLeftOfCursor - charactersLeftOfCursor.length();
      int newCursorPos = qAbs( cursorPos - removedCharacterCountLeftOfCursor );
      // just double check for sanity that it is in bounds
      Q_ASSERT( qBound( 0, newCursorPos, modifiedInput.length() ) == newCursorPos );
      cursorPos = newCursorPos;
    }
    input = modifiedInput;
  }

  void fixup( QString& input ) const override
  {
    int cursorPos = 0;
    stripNonNumbersAndAdjustCursorPos( input, cursorPos );
  }

  State validate( QString& input, int& cursorPos ) const override
  {
    State retval = QValidator::Invalid;

    if ( input.length() != 0 )
    {
      stripNonNumbersAndAdjustCursorPos( input, cursorPos );

      bool conversionToIntWentOk;
      int number = input.toInt( &conversionToIntWentOk );

      if ( conversionToIntWentOk )
      {
        retval = isNumberValid( number );
      }
    }
    else
    {
      // zero length
      retval = QValidator::Intermediate;
    }

    return retval;
  }
};

class SC_SimulateTab;

// ============================================================================
// SC_MainWindow
// ============================================================================

class SC_MainWindow : public QWidget
{
  Q_OBJECT
public:
  QWidget* customGearTab;
  QWidget* customTalentsTab;
  SC_MainTab* mainTab;
  SC_OptionsTab* optionsTab;
  SC_ImportTab* importTab;
  SC_SimulateTab* simulateTab;
  SC_ResultTab* resultsTab;
  SC_SpellQueryTab* spellQueryTab;
  QTabWidget* createCustomProfileDock;

  BattleNetImportWindow* newBattleNetView;
  SC_WebView* helpView;
  SC_WebView* visibleWebView;
  SC_TextEdit* overridesText;
  SC_TextEdit* logText;
  QPushButton* backButton;
  QPushButton* forwardButton;
  SC_MainWindowCommandLine* cmdLine;
  QLineEdit* apikey;
  QGroupBox* cmdLineGroupBox;
  QGroupBox* createCustomCharData;
  SC_RecentlyClosedTabItemModel* recentlyClosedTabModel;
  SC_RecentlyClosedTabWidget* recentlyClosedTabImport;
  QDesktopWidget desktopWidget;

  QTimer* timer;
  QTimer* soloChar;
  SC_ImportThread* importThread;
  SC_SimulateThread* simulateThread;

  QSignalMapper mainTabSignalMapper;
  QList< QShortcut* > shortcuts;

  std::shared_ptr<sim_t> sim;
  std::shared_ptr<sim_t> import_sim;
  std::string simPhase;
  std::string importSimPhase;
  int simProgress;
  int importSimProgress;
  int soloimport;
  int simResults;

  QString AppDataDir; // output goes here
  QString ResultsDestDir; // user documents dir, default location offered to persitently save reports
  QString TmpDir; // application specific temporary dir

  QString cmdLineText;
  QString logFileText;
  QString resultsFileText;

  SC_QueueItemModel simulationQueue;
  int consecutiveSimulationsRun;

  void    startImport( int tab, const QString& url );
  bool    importRunning();
  void    startSim();
  bool    simRunning();

  std::shared_ptr<sim_t>  initSim();
  void    deleteSim( std::shared_ptr<sim_t>& sim, SC_TextEdit* append_error_message = 0 );

  void loadHistory();
  void saveHistory();

  void createCmdLine();
  void createWelcomeTab();
  void createImportTab();
  void createOptionsTab();
  void createSimulateTab();
  void createResultsTab();
  void createOverridesTab();
  void createHelpTab();
  void createLogTab();
  void createSpellQueryTab();
  void createTabShortcuts();
  void createCustomTab();
  void updateWebView( SC_WebView* );

  void toggle_pause()
  {
    if ( ! sim )
      return;

    if ( ! sim -> paused )
    {
      sim -> pause_mutex -> lock();
    }
    else
    {
      sim -> pause_mutex -> unlock();
    }

    sim -> paused = ! sim -> paused;
  }

protected:
  void closeEvent( QCloseEvent* ) override;

private slots:
  void updatetimer();
  void itemWasEnqueuedTryToSim();
  void importFinished();
  void simulateFinished( std::shared_ptr<sim_t> );
  void updateSimProgress();
  void cmdLineReturnPressed();
  void cmdLineTextEdited( const QString& );
  void backButtonClicked( bool checked = false );
  void forwardButtonClicked( bool checked = false );
  void reloadButtonClicked( bool checked = false );
  void mainButtonClicked( bool checked = false );
  void pauseButtonClicked( bool checked = false );
  void cancelButtonClicked();
  void queueButtonClicked();
  void importButtonClicked();
  void queryButtonClicked();
  void mainTabChanged( int index );
  void importTabChanged( int index );
  void resultsTabChanged( int index );
  void resultsTabCloseRequest( int index );
  void bisDoubleClicked( QTreeWidgetItem* item, int col );
  void armoryRegionChanged( const QString& region );
  void simulateTabRestored( QWidget* tab, const QString& title, const QString& tooltip, const QIcon& icon );
  void switchToASubTab( int direction );
  void switchToLeftSubTab();
  void switchToRightSubTab();
  void currentlyViewedTabCloseRequest();

public slots:
  void enqueueSim();
  void saveLog();
  void saveResults();
  void stopImport();
  void stopSim();
  void stopAllSim();
  void startNewImport( const QString&, const QString&, const QString&, const QString& );

public:
  SC_MainWindow( QWidget *parent = nullptr );
};

// ============================================================================
// SC_WebPage
// ============================================================================

class SC_WebPage : public SC_WebEnginePage
{
  Q_OBJECT
public:
  explicit SC_WebPage( QObject* parent = nullptr ):
    SC_WebEnginePage( parent )
  {}

#if defined( SC_USE_WEBKIT )
  QString userAgentForUrl( const QUrl& /* url */ ) const
  { return QString( "simulationcraft_gui" ); }
protected:
  virtual bool supportsExtension( Extension extension ) const
  {
    return extension == QWebPage::ErrorPageExtension;
  }
  virtual bool extension( Extension extension, const ExtensionOption* option = 0, ExtensionReturn* output = 0 )
  {
    if ( extension != SC_WebPage::ErrorPageExtension )
    {
      return false;
    }

    const ErrorPageExtensionOption* errorOption = static_cast< const ErrorPageExtensionOption* >( option );

    QString domain;
    switch( errorOption -> domain )
    {
    case SC_WebEnginePage::QtNetwork:
      domain = tr( "Network Error" );
      break;
    case SC_WebEnginePage::WebKit:
      domain = tr( "WebKit Error" );
      break;
    case SC_WebEnginePage::Http:
      domain = tr( "HTTP Error" );
      break;
    default:
      domain = tr( "Unknown Error" );
      break;
    }

    QString html;
    QString errorHtmlFile = QDir::currentPath() + "/Error.html";
#if defined( Q_OS_MAC )
    CFURLRef fileRef    = CFBundleCopyResourceURL( CFBundleGetMainBundle(), CFSTR( "Error" ), CFSTR( "html" ), 0 );
    if ( fileRef )
    {
      CFStringRef macPath = CFURLCopyFileSystemPath( fileRef, kCFURLPOSIXPathStyle );
      errorHtmlFile       = CFStringGetCStringPtr( macPath, CFStringGetSystemEncoding() );

      CFRelease( fileRef );
      CFRelease( macPath );
    }
#endif
    QFile errorHtml( errorHtmlFile );
    if ( errorHtml.open( QIODevice::ReadOnly | QIODevice::Text ) )
    {
      html = QString::fromUtf8( errorHtml.readAll() );
      errorHtml.close();
    }
    else
    {
      // VERY simple error page if we fail to load detailed error page
      html = "<html><head><title>Error</title></head><body><p>Failed to load <a href=\"SITE_URL\">SITE_URL</a></p><p>ERROR_DOMAIN ERROR_NUMBER: ERROR_STRING</p></body></html>";
    }

    html = html.replace( "ERROR_NUMBER", QString::number( errorOption -> error ) );
    html = html.replace( "ERROR_DOMAIN", domain );
    html = html.replace( "ERROR_STRING", errorOption -> errorString );
    html = html.replace( "SITE_URL", errorOption -> url.toString() );

    ErrorPageExtensionReturn* errorReturn = static_cast< ErrorPageExtensionReturn* >( output );
    errorReturn -> content = html.toUtf8();
    errorReturn -> contentType = "text/html";
    errorReturn -> baseUrl = errorOption -> url;
    return true;
  }
#elif defined( SC_USE_WEBENGINE ) && ( QT_VERSION >= QT_VERSION_CHECK( 5, 5, 0 ) ) // Functionality added to webengine in qt 5.5
  bool acceptNavigationRequest( const QUrl &url, NavigationType, bool isMainFrame ) override
  {
    if ( ! isMainFrame )
    {
      return false;
    }

    QString url_to_show = url.toString();
    if ( url.isLocalFile() || url_to_show.contains( "battle.net" ) || 
         url_to_show.contains( "battlenet" ) || url_to_show.contains( "github.com" ) ||
         url_to_show.contains ( "worldofwarcraft.com" ) )
      return true;
    else
      QDesktopServices::openUrl( url_to_show );
    return false;
  }
#endif /* SC_USE_WEBKIT */
};

// ============================================================================
// SC_WebView
// ============================================================================

class SC_WebView : public SC_WebEngineView
{
  Q_OBJECT
  SC_SearchBox* searchBox;
  QString previousSearch;
  bool allow_mouse_navigation;
  bool allow_keyboard_navigation;
  bool allow_searching;
public:
  SC_MainWindow* mainWindow;
  int progress;
  QString html_str;
  QString url_to_show;
  QByteArray out_html;

  SC_WebView( SC_MainWindow* mw, QWidget* parent = nullptr , const QString& h = QString() ) :
    SC_WebEngineView( parent ),
    searchBox( 0 ),
    previousSearch( "" ),
    allow_mouse_navigation( false ),
    allow_keyboard_navigation( false ),
    allow_searching( false ),
    mainWindow( mw ),
    progress( 0 ),
    html_str( h )
  {
    searchBox = new SC_SearchBox();
    searchBox -> hide();
    QShortcut* ctrlF = new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_F ), this );
    QShortcut* escape = new QShortcut( QKeySequence( Qt::Key_Escape ), this );
    connect( ctrlF,     SIGNAL( activated() ),                   searchBox, SLOT( show() ) );
    connect( escape,    SIGNAL( activated() ),                   this,      SLOT( hideSearchBox() ) );
    connect( searchBox, SIGNAL( textChanged( const QString& ) ), this,      SLOT( findSomeText( const QString& ) ) );
    connect( searchBox, SIGNAL( findPrev() ),                    this,      SLOT( findPrev() ) );
    connect( searchBox, SIGNAL( findNext() ),                    this,      SLOT( findNext() ) );
    connect( this,      SIGNAL( loadProgress( int ) ),           this,      SLOT( loadProgressSlot( int ) ) );
    connect( this,      SIGNAL( loadFinished( bool ) ),          this,      SLOT( loadFinishedSlot( bool ) ) );
    connect( this,      SIGNAL( urlChanged( const QUrl& ) ),     this,      SLOT( urlChangedSlot( const QUrl& ) ) );
#if defined ( SC_USE_WEBKIT )
    connect( this,      SIGNAL( linkClicked( const QUrl& ) ),    this,      SLOT( linkClickedSlot( const QUrl& ) ) );
#endif

    SC_WebPage* page = new SC_WebPage( this );
    setPage( page );
#if defined( SC_USE_WEBKIT )
    page -> setLinkDelegationPolicy( QWebPage::DelegateExternalLinks );
#endif

    // Add QT Major Version to avoid "mysterious" problems resulting in qBadAlloc.
    QDir dir( mainWindow -> TmpDir + QDir::separator() + "simc_webcache_qt" + std::string( QT_VERSION_STR ).substr( 0, 3 ).c_str() );
    if ( ! dir.exists() ) dir.mkpath( "." );

    QFileInfo fi( dir.absolutePath() );

    if ( fi.isDir() && fi.isWritable() )
    {
      QNetworkDiskCache* diskCache = new QNetworkDiskCache( this );
      diskCache -> setCacheDirectory( dir.absolutePath() );
      QString test = diskCache -> cacheDirectory();
#if defined( SC_USE_WEBKIT )
      page -> networkAccessManager()->setCache( diskCache );
#endif
    }
    else
    {
      qWarning() << "Can't create/write webcache.";
    }
  }

  void store_html( const QString& s )
  {
    html_str = s;
  }

  virtual ~SC_WebView() {}

  void loadHtml()
  {
    setHtml( html_str );
  }
#if defined( SC_USE_WEBKIT )
  QString toHtml()
  {
    return page() -> currentFrame() -> toHtml();
  }
#endif

  void enableMouseNavigation()
  {
    allow_mouse_navigation = true;
  }

  void disableMouseNavigation()
  {
    allow_mouse_navigation = false;
  }

  void enableKeyboardNavigation()
  {
    allow_keyboard_navigation = true;
  }

  void disableKeyboardNavigation()
  {
    allow_keyboard_navigation = false;
  }

private:
  void update_progress( int p )
  {
    progress = p;
    mainWindow -> updateWebView( this );
  }

protected:
  void mouseReleaseEvent( QMouseEvent* e ) override
  {
    if ( allow_mouse_navigation )
    {
      switch( e -> button() )
      {
      case Qt::XButton1:
        back();
        break;
      case Qt::XButton2:
        forward();
        break;
      default:
        break;
      }
    }
    SC_WebEngineView::mouseReleaseEvent( e );
  }

  void keyReleaseEvent( QKeyEvent* e ) override
  {
    if ( allow_keyboard_navigation )
    {
      int k = e -> key();
      Qt::KeyboardModifiers m = e -> modifiers();
      switch ( k )
      {
      case Qt::Key_R:
      {
        if ( m.testFlag( Qt::ControlModifier ) == true )
          reload();
      }
      break;
      case Qt::Key_F5:
      {
        reload();
      }
      break;
      case Qt::Key_Left:
      {
        if ( m.testFlag( Qt::AltModifier ) == true )
          back();
      }
      break;
      case Qt::Key_Right:
      {
        if ( m.testFlag( Qt::AltModifier ) == true )
          forward();
      }
      break;
      default: break;
      }
    }
    SC_WebEngineView::keyReleaseEvent( e );
  }

  void resizeEvent( QResizeEvent* e ) override
  {
    searchBox -> updateGeometry();
    SC_WebEngineView::resizeEvent( e );
  }

  void focusInEvent( QFocusEvent* e ) override
  {
    hideSearchBox();
    SC_WebEngineView::focusInEvent( e );
  }

private slots:
  void loadProgressSlot( int p )
  { update_progress( p ); }

  void loadFinishedSlot( bool /* ok */ )
  { update_progress( 100 ); }

  void urlChangedSlot( const QUrl& url )
  {
    url_to_show = url.toString();
    if ( url_to_show == "about:blank" )
    {
      url_to_show = "results.html";
    }
    mainWindow -> updateWebView( this );
  }

#if defined ( SC_USE_WEBKIT )
  void linkClickedSlot( const QUrl& url )
  {
    QString clickedurl = url.toString();

    // Wowhead links tend to crash the gui, so we'll send them to an external browser.
    // AMR and Lootrank links are nice to load externally too so we don't lose sim results
    // In general, we err towards opening things externally because we are not Mozilla
    // github.com is needed for our help tab; battle.net (us/eu) and battlenet (china) cover armory
    if ( url.isLocalFile() || clickedurl.contains( "battle.net" ) || clickedurl.contains( "battlenet" ) || clickedurl.contains( "github.com" ) )
      load( url );
    else
      QDesktopServices::openUrl( url );
  }
#endif

public slots:
  void hideSearchBox()
  {
    previousSearch = ""; // disable clearing of highlighted text on next search
    searchBox -> hide();
  }

  void findSomeText( const QString& text, SC_WebEnginePage::FindFlags options )
  {
#if defined( SC_USE_WEBKIT )
    if ( ! previousSearch.isEmpty() && previousSearch != text )
    {
      findText( "", SC_WebEnginePage::HighlightAllOccurrences ); // clears previous highlighting
    }
    previousSearch = text;

    if ( searchBox -> wrapSearch() )
    {
      options |= SC_WebEnginePage::FindWrapsAroundDocument;
    }
#endif
    findText( text, options );

    // If the QWebView scrolls due to searching with the SC_SearchBox visible
    // The SC_SearchBox could still be visible, as the area the searchbox is covering
    // is not redrawn, just moved
    // *HACK*
    // This just repaints the area directly above the search box to the top, in a huge rectangle
    QRect searchBoxRect = searchBox -> rect();
    searchBoxRect.moveTopLeft( searchBox -> geometry().topLeft() );
    switch( searchBox -> getCorner() )
    {
    case Qt::TopLeftCorner:
    case Qt::BottomLeftCorner:
      searchBoxRect.setTopLeft( QPoint( 0, 0 ) );
      searchBoxRect.setBottomLeft( rect().bottomLeft() );
      break;
    case Qt::TopRightCorner:
    case Qt::BottomRightCorner:
      searchBoxRect.setTopRight( rect().topRight() );
      searchBoxRect.setBottomRight( rect().bottomRight() );
      break;
    }
    repaint( searchBoxRect );
  }

  void findSomeText( const QString& text )
  {
    SC_WebEnginePage::FindFlags flags = 0;
    if ( searchBox -> reverseSearch() )
    {
      flags |= SC_WebEnginePage::FindBackward;
    }
    findSomeText( text, flags );
  }

  void findPrev()
  {
    findSomeText( searchBox -> text(), SC_WebEnginePage::FindBackward );
  }

  void findNext()
  {
    findSomeText( searchBox -> text(), 0 );
  }
};

class SC_ImportThread : public QThread
{
  Q_OBJECT
  SC_MainWindow* mainWindow;
  std::shared_ptr<sim_t> sim;

public:
  int tab;
  QString url;
  QString profile;
  QString item_db_sources;
  QString active_spec;
  QString m_role;
  QString api;
  QString error;
  player_t* player;
  QString region, realm, character, spec; // New import uses these

  void importWidget();

  void start( std::shared_ptr<sim_t> s, int t, const QString& u, const QString& sources, const QString& spec, const QString& role, const QString& apikey )
  { sim = s; tab = t; url = u; profile = ""; item_db_sources = sources; player = 0; active_spec = spec; m_role = role, api = apikey; error = ""; QThread::start(); }
  void start( std::shared_ptr<sim_t> s, const QString&, const QString&, const QString&, const QString& );
  void run() override;
  SC_ImportThread( SC_MainWindow* mw ) : mainWindow( mw ), sim( 0 ), tab(0), player( 0 ) {}
};

class SC_OverridesTab : public SC_TextEdit
{
public:
    SC_OverridesTab( QWidget* parent ) :
        SC_TextEdit( parent )
    {

        setPlainText( tr("# User-specified persistent global and player parameters will be set here.\n") );
    }
};

#endif // SIMULATIONCRAFTQT_H
