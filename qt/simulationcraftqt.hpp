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
#include <QtNetwork/QtNetwork>

#if defined( Q_OS_MAC )
#include <CoreFoundation/CoreFoundation.h>
#endif
#include <QtWidgets/QtWidgets>
#include <QtWebKitWidgets/QtWebKitWidgets>

class SC_MainWindow;
class SC_SearchBox;
class SC_TextEdit;
class SC_RelativePopup;
class SC_MainWindowCommandLine;
#ifdef SC_PAPERDOLL
#include "simcpaperdoll.hpp"
class Paperdoll;
class PaperdollProfile;
#endif

#include "util/sc_recentlyclosed.hpp" // remove once implementations are moved to source files
#include "util/sc_searchbox.hpp" // remove once implementations are moved to source files
#include "util/sc_textedit.hpp" // remove once implementations are moved to source files


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
  TAB_SPELLQUERY
#ifdef SC_PAPERDOLL
  , TAB_PAPERDOLL
#endif
  , TAB_COUNT
};

enum import_tabs_e
{
  TAB_BATTLE_NET = 0,
  TAB_BIS,
  TAB_HISTORY,
  TAB_RECENT,
  TAB_AUTOMATION,
  TAB_CUSTOM
};

class SC_WebView;
//class SC_SpellQueryTab;
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
  QList< QPair< Qt::Key, QList< Qt::KeyboardModifier > > > ignoreKeys;
public:
  SC_enumeratedTab( QWidget* parent = 0 ) :
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
  virtual void keyPressEvent( QKeyEvent* e )
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
  SC_QueueItemModel( QObject* parent = nullptr ) :
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
  QString dequeue()
  {
    QModelIndex indx = index( 0, 0 );
    QString retval;
    if ( indx.isValid() )
    {
      retval = indx.data( Qt::UserRole + 1 ).toString();
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
      QWidget* parent = nullptr ) :
    QWidget( parent ),
    listView( nullptr ),
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
    if ( listView != nullptr )
    {
      delete listView;
    }
    listView = new QListView( this );

    listView -> setEditTriggers( QAbstractItemView::NoEditTriggers );
    listView -> setSelectionMode( QListView::SingleSelection );
    listView -> setModel( model );

    if ( model != nullptr )
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
    if ( model != nullptr ) // gcc gave a weird error when making this one if
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
    if ( model != nullptr )
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
  SC_CommandLine( QWidget* parent = nullptr );
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
// SC_ResultTabWidget
// ============================================================================

enum result_tabs_e
{
  TAB_HTML = 0,
  TAB_TEXT,
  TAB_XML,
  TAB_PLOTDATA,
  TAB_CSV
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
  SC_SingleResultTab( SC_MainWindow* mw, QWidget* parent = 0 ) :
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

class SC_ImportTab : public SC_enumeratedTab<import_tabs_e>
{
  Q_OBJECT
public:
  SC_ImportTab( QWidget* parent = 0 ) :
    SC_enumeratedTab<import_tabs_e>( parent )
  {
  }

  // these are options on the Automation tab
  struct choices_t
  {
    QComboBox* player_class;
    QComboBox* player_spec;
    QComboBox* player_race;
    QComboBox* player_level;
    QComboBox* comp_type;
  } choice;

  struct labels_t
  {
    QLabel* talents;
    QLabel* glyphs;
    QLabel* gear;
    QLabel* rotationHeader;
    QLabel* rotationFooter;
    QLabel* advanced;
    QLabel* sidebar;
    QLabel* footer;
  } label;

  struct textBoxes_t
  {
    QLineEdit* talents;
    QLineEdit* glyphs;
    SC_TextEdit* gear;
    SC_TextEdit* rotationHeader;
    SC_TextEdit* rotationFooter;
    SC_TextEdit* advanced;
    SC_TextEdit* sidebar;
    SC_TextEdit* helpbar;
    SC_TextEdit* footer;
  } textbox;

  QString advTalent;
  QString advGlyph;
  QString advGear;
  QString advRotation;

  void encodeSettings();
  void load_setting( QSettings& s, const QString& name, QComboBox* choice, const QString& default_value );
  void load_setting( QSettings& s, const QString& name, QString* text, const QString& default_value);
  void load_setting( QSettings& s, const QString& name, QLineEdit* textbox, const QString& default_value);
  void load_setting( QSettings& s, const QString& name, SC_TextEdit* textbox, const QString& default_value);
  void decodeSettings();
  void createAutomationTab();
  void createTooltips();

public slots:
  void setSpecDropDown( const int player_class );
  void setSidebarClassText();
  void compTypeChanged( const int comp );
};

// ==========================================================================
// Utilities
// ==========================================================================


const QString defaultSimulateText( "# Profile will be downloaded into a new tab.\n"
                                   "#\n"
                                   "# Clicking Simulate will create a simc_gui.simc profile for review.\n" );

const QString defaultSimulateTabTitle( "Simulate" );

// ============================================================================
// SC_SimulateTabWidget
// ============================================================================

class SC_SimulateTab : public SC_RecentlyClosedTab
{
  Q_OBJECT
  QWidget* addTabWidget;
  int lastSimulateTabIndexOffset;
public:
  SC_SimulateTab( QWidget* parent = nullptr, SC_RecentlyClosedTabItemModel* modelToUse = nullptr ) :
    SC_RecentlyClosedTab( parent, modelToUse ),
    addTabWidget( new QWidget( this ) ),
    lastSimulateTabIndexOffset( -2 )
  {
    setTabsClosable( true );

    setCloseAllTabsTitleText( tr( "Close ALL Simulate Tabs?" ) );
    setCloseAllTabsBodyText( tr( "Do you really want to close ALL simulation profiles?" ) );
    QIcon addTabIcon(":/icon/addtab.png");
    int i = addTab( addTabWidget, addTabIcon, addTabIcon.pixmap(QSize(64, 64)).isNull() ? "+":"" );
    tabBar() -> setTabButton( i, QTabBar::LeftSide, nullptr );
    tabBar() -> setTabButton( i, QTabBar::RightSide, nullptr );
    addCloseAllExclude( addTabWidget );

    enableDragHoveredOverTabSignal( true );
    connect( this, SIGNAL( mouseDragHoveredOverTab( int ) ), this, SLOT( mouseDragSwitchTab( int ) ) );
    connect( this, SIGNAL( currentChanged( int ) ), this, SLOT( addNewTab( int ) ) );
    connect( this, SIGNAL( tabCloseRequested( int ) ), this, SLOT( TabCloseRequest( int ) ) );
    // Using QTabBar::tabMoved(int,int) signal turns out to be very wonky
    // It is emitted WHILE tabs are still being dragged and looks very funny
    // This is the best way I could find to enable this
    setMovable( true ); // Would need to disallow moving the + tab, or to the right of it. That would require subclassing tabbar
    connect( this, SIGNAL( tabBarLayoutRequestEvent() ), this, SLOT( enforceAddTabWidgetLocationInvariant() ) );
  }

  static void format_document( SC_TextEdit* /*s*/ )
  {
    // Causes visual disappearance of text. Deactivated for now.
    /*QTextDocument* d = s -> document();
    QTextBlock b = d -> begin();
    QRegExp comment_rx( "^\\s*\\#" );
    QTextCharFormat* comment_format = new QTextCharFormat();
    comment_format -> setForeground( QColor( "forestgreen" ) );
    while ( b.isValid() )
    {
      if ( comment_rx.indexIn( b.text() ) != -1 )
      {
        QTextCursor c(b);
        c.select( QTextCursor::BlockUnderCursor );
        c.setCharFormat( *comment_format );
      }
      b = b.next();
    }*/
  }

  bool is_Default_New_Tab( int index )
  {
    bool retval = false;
    if ( index >= 0 && index < count() &&
         ! isACloseAllExclude( widget( index ) ) )
      retval = ( tabText( index ) == defaultSimulateTabTitle &&
                 static_cast<SC_TextEdit*>( widget( index ) ) -> toPlainText() == defaultSimulateText );
    return retval;
  }

  bool contains_Only_Default_Profiles()
  {
    for ( int i = 0; i < count(); ++i )
    {
      QWidget* widget_ = widget( i );
      if ( ! isACloseAllExclude( widget_ ) )
      {
        if ( ! is_Default_New_Tab( i ) )
          return false;
      }
    }
    return true;
  }

  int add_Text( const QString& text, const QString& tab_name )
  {
    SC_TextEdit* s = new SC_TextEdit();
    s -> setText( text );
    format_document( s );
    int indextoInsert = indexOf( addTabWidget );
    int i = insertTab( indextoInsert, s, tab_name );
    setCurrentIndex( i );
    return i;
  }

  SC_TextEdit* current_Text()
  {
    return static_cast<SC_TextEdit*>( currentWidget() );
  }

  void set_Text( const QString& text )
  {
    SC_TextEdit* current_s = static_cast<SC_TextEdit*>( currentWidget() );
    current_s -> setText( text );
    format_document( current_s );
  }

  void append_Text( const QString& text )
  {
    SC_TextEdit* current_s = static_cast<SC_TextEdit*>( currentWidget() );
    current_s -> append( text );
    format_document( current_s );
  }
  void close_All_Tabs()
  {
    QList < QWidget* > specialTabsList;
    specialTabsList.append( addTabWidget );
    // add more special widgets to never delete here
    int totalTabsToDelete = count();
    for ( int i = 0; i < totalTabsToDelete; ++i )
    {
      bool deleteIndexOk = false;
      int indexToDelete = 0;
      // look for a tab not in the specialTabsList to delete
      while ( ! deleteIndexOk && indexToDelete < count() )
      {
        if ( specialTabsList.contains( widget( indexToDelete ) ) )
        {
          deleteIndexOk = false;
          indexToDelete++;
        }
        else
        {
          deleteIndexOk = true;
        }
      }
      if ( deleteIndexOk )
      {
        assert( specialTabsList.contains( widget( indexToDelete ) ) == false );
        removeTab( indexToDelete );
      }
    }
  }
  virtual int insertAt()
  {
    return indexOf( addTabWidget );
  }
  bool wrapToEnd()
  {
    if ( currentIndex() == 0 )
    {
      setCurrentIndex( count() + lastSimulateTabIndexOffset );
      return true;
    }
    return false;
  }
  bool wrapToBeginning()
  {
    if ( currentIndex() == count() + lastSimulateTabIndexOffset )
    {
      setCurrentIndex( 0 );
      return true;
    }
    return false;
  }
  void insertNewTabAt( int index = -1, const QString text=defaultSimulateText, const QString title=defaultSimulateTabTitle )
  {
    if ( index < 0 || index > count() )
      index = currentIndex();
    SC_TextEdit* s = new SC_TextEdit();
    s -> setText( text );
    format_document( s );
    insertTab( index , s, title );
    setCurrentIndex( index);
  }
protected:
  virtual void keyPressEvent( QKeyEvent* e )
  {
    int k = e -> key();
    Qt::KeyboardModifiers m = e -> modifiers();

    if ( k == Qt::Key_Backtab ||
         ( k == Qt::Key_Tab && m.testFlag( Qt::ControlModifier ) && m.testFlag( Qt::ShiftModifier ) ) )
    {
      if ( wrapToEnd() )
        return;
    }
    else if ( k == Qt::Key_Tab )
    {
      if ( wrapToBeginning() )
        return;
    }
    else if ( k == Qt::Key_W && m.testFlag( Qt::ControlModifier ) )
    {
      // close the tab in keyReleaseEvent()
      return;
    }
    else if ( k == Qt::Key_T && m.testFlag( Qt::ControlModifier ) )
    {
      return;
    }
    QTabWidget::keyPressEvent( e );
  }
  virtual void keyReleaseEvent( QKeyEvent* e )
  {
    int k = e -> key();
    Qt::KeyboardModifiers m = e -> modifiers();
    switch ( k )
    {
    case Qt::Key_W:
    {
      if ( m.testFlag( Qt::ControlModifier ) == true )
      {
        TabCloseRequest( currentIndex() );
        return;
      }
    }
      break;
    case Qt::Key_T:
    {
      if ( m.testFlag( Qt::ControlModifier ) == true )
      {
        insertNewTabAt( currentIndex() + 1 );
        return;
      }
    }
      break;
    default: break;
    }
    SC_RecentlyClosedTab::keyReleaseEvent( e );
  }
  virtual void showEvent( QShowEvent* e )
  {
    QWidget* currentWidget = widget( currentIndex() );
    if ( currentWidget != nullptr )
    {
      if ( ! currentWidget -> hasFocus() )
      {
        currentWidget -> setFocus();
      }
    }

    SC_RecentlyClosedTab::showEvent( e );
  }
public slots:
  void TabCloseRequest( int index )
  {
    if ( count() > 2 && index != (count() - 1) )
    {
      int confirm;

      if ( is_Default_New_Tab( index ) )
      {
        confirm = QMessageBox::Yes;
      }
      else
      {
        confirm = QMessageBox::question( this, tr( "Close Simulate Tab" ), tr( "Do you really want to close this simulation profile?" ), QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );
      }
      if ( confirm == QMessageBox::Yes )
      {
        if ( index == currentIndex() &&
             widget( currentIndex() + 1 ) == addTabWidget )
        {
          // Change index only if removing causes a new tab to be created
          setCurrentIndex( qMax<int>( 0, currentIndex() - 1 ) );
        }
        removeTab( index );
        widget( currentIndex() ) -> setFocus();
      }
    }
  }
  void addNewTab( int index )
  {
    if ( index == indexOf( addTabWidget ) )
    {
      insertNewTabAt( index );
    }
  }
  void mouseDragSwitchTab( int tab )
  {
    if ( tab != indexOf( addTabWidget ) )
    {
      setCurrentIndex( tab );
    }
  }
  void enforceAddTabWidgetLocationInvariant()
  {
    int addTabWidgetIndex = indexOf( addTabWidget );
    if ( count() >= 1 )
    {
      if ( addTabWidgetIndex != count() - 1 )
      {
        tabBar() -> moveTab( addTabWidgetIndex, count() - 1 );
      }
    }
  }
};

class SC_OptionsTab;
class SC_SpellQueryTab;

// ============================================================================
// SC_ComboBoxIntegerValidator
// ============================================================================

class SC_ComboBoxIntegerValidator : public QValidator
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
      QComboBox* parent ) :
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
  static SC_ComboBoxIntegerValidator* CreateBoundlessValidator( QComboBox* parent = nullptr )
  {
    return new SC_ComboBoxIntegerValidator( std::numeric_limits<int>::min(), std::numeric_limits<int>::max(), parent );
  }
  static SC_ComboBoxIntegerValidator* CreateUpperBoundValidator( int upperBound, QComboBox* parent = nullptr )
  {
    return new SC_ComboBoxIntegerValidator( std::numeric_limits<int>::min(), upperBound, parent );
  }
  static SC_ComboBoxIntegerValidator* CreateLowerBoundValidator( int lowerBound, QComboBox* parent = nullptr )
  {
    return new SC_ComboBoxIntegerValidator( lowerBound, std::numeric_limits<int>::max(), parent );
  }
  static QComboBox* ApplyValidatorToComboBox( QValidator* validator, QComboBox* comboBox )
  {
    if ( comboBox != nullptr )
    {
      if ( validator != nullptr )
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
    return ( number >= lowerBoundInclusive &&
             number <= upperBoundInclusive );
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
  virtual void fixup( QString& input ) const
  {
    int cursorPos = 0;
    stripNonNumbersAndAdjustCursorPos( input, cursorPos );
  }
  virtual State validate( QString& input, int& cursorPos ) const
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

// ============================================================================
// SC_MainWindow
// ============================================================================

class SC_MainWindow : public QWidget
{
  Q_OBJECT
public:
  QWidget* customGearTab;
  QWidget* customTalentsTab;
  QWidget* customGlyphsTab;
  SC_MainTab* mainTab;
  SC_OptionsTab* optionsTab;
  SC_ImportTab* importTab;
  SC_SimulateTab* simulateTab;
  SC_ResultTab* resultsTab;
  SC_SpellQueryTab* spellQueryTab;
  QTabWidget* createCustomProfileDock;
#ifdef SC_PAPERDOLL
  QTabWidget* paperdollTab;
  Paperdoll* paperdoll;
  PaperdollProfile* paperdollProfile;
#endif

  SC_WebView* battleNetView;
  SC_WebView* helpView;
  SC_WebView* visibleWebView;
  QListWidget* historyList;
  SC_TextEdit* overridesText;
  SC_TextEdit* logText;
  QPushButton* backButton;
  QPushButton* forwardButton;
  SC_MainWindowCommandLine* cmdLine;
  QGroupBox* cmdLineGroupBox;
  QGroupBox* createCustomCharData;
  SC_RecentlyClosedTabItemModel* recentlyClosedTabModel;
  SC_RecentlyClosedTabWidget* recentlyClosedTabImport;
  QDesktopWidget desktopWidget;

  QTimer* timer;
  ImportThread* importThread;
  SimulateThread* simulateThread;

  QSignalMapper mainTabSignalMapper;
  QList< QShortcut* > shortcuts;
#ifdef SC_PAPERDOLL
  PaperdollThread* paperdollThread;
#endif

  sim_t* sim;
  sim_t* import_sim;
  sim_t* paperdoll_sim;
  std::string simPhase;
  std::string importSimPhase;
  int simProgress;
  int importSimProgress;
  int simResults;
//  QStringList resultsHtml;

  QString AppDataDir;
  QString ResultsDestDir;
  QString TmpDir;

  QString cmdLineText;
  QString logFileText;
  QString resultsFileText;

  SC_QueueItemModel simulationQueue;
  int consecutiveSimulationsRun;

  void    startImport( int tab, const QString& url );
  void    startAutomationImport( int tab );
  bool    importRunning();
  void    startSim();
  bool    simRunning();

  sim_t*  initSim();
  void    deleteSim( sim_t* sim, SC_TextEdit* append_error_message = 0 );

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
  void createToolTips();
  void createTabShortcuts();
  void createBestInSlotTab();
  void createCustomTab();

#ifdef SC_PAPERDOLL
  void createPaperdoll();
#endif
  void updateWebView( SC_WebView* );

private:
  QRect getSmallestScreenGeometry();
  QRect adjustGeometryToIncludeFrame( QRect );
  QPoint getMiddleOfScreen( int );
  int getScreenThatGeometryBelongsTo( QRect );
  void applyAdequateApplicationGeometry( QRect );
  void applyAdequateApplicationGeometry();

protected:
  virtual void closeEvent( QCloseEvent* );
  virtual void showEvent( QShowEvent* );

private slots:
  void itemWasEnqueuedTryToSim();
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
  void historyDoubleClicked( QListWidgetItem* item );
  void bisDoubleClicked( QTreeWidgetItem* item, int col );
  void armoryRegionChanged( const QString& region );
  void simulateTabRestored( QWidget* tab, const QString& title, const QString& tooltip, const QIcon& icon );
  void switchToASubTab( int direction );
  void switchToLeftSubTab();
  void switchToRightSubTab();
  void currentlyViewedTabCloseRequest();
  void screenResized( int );

public slots:
  void enqueueSim();
  void saveLog();
  void saveResults();

  void stopImport();
  void stopSim();
  void stopAllSim();
  
public:
  SC_MainWindow( QWidget *parent = 0 );
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
protected:
  virtual bool supportsExtension( Extension extension ) const
  {
    return extension == QWebPage::ErrorPageExtension;
  }
  virtual bool extension( Extension extension, const ExtensionOption* option = nullptr, ExtensionReturn* output = nullptr )
  {
    if ( extension != QWebPage::ErrorPageExtension )
    {
      return false;
    }

    const ErrorPageExtensionOption* errorOption = static_cast< const ErrorPageExtensionOption* >( option );

    QString domain;
    switch( errorOption -> domain )
    {
    case QWebPage::QtNetwork:
      domain = tr( "Network Error" );
      break;
    case QWebPage::WebKit:
      domain = tr( "WebKit Error" );
      break;
    case QWebPage::Http:
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
};

// ============================================================================
// SC_WebView
// ============================================================================

class SC_WebView : public QWebView
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

  SC_WebView( SC_MainWindow* mw, QWidget* parent = 0, const QString& h = QString() ) :
    QWebView( parent ),
    searchBox( nullptr ),
    previousSearch( "" ),
    allow_mouse_navigation( false ),
    allow_keyboard_navigation( false ),
    allow_searching( false ),
    mainWindow( mw ),
    progress( 0 ),
    html_str( h )
  {
    searchBox = new SC_SearchBox( this );
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
    connect( this,      SIGNAL( linkClicked( const QUrl& ) ),    this,      SLOT( linkClickedSlot( const QUrl& ) ) );

    SC_WebPage* page = new SC_WebPage( this );
    setPage( page );
    page -> setLinkDelegationPolicy( QWebPage::DelegateExternalLinks );

    // Add QT Major Version to avoid "mysterious" problems resulting in qBadAlloc.
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
  void store_html( const QString& s )
  {
    html_str = s;
  }
  virtual ~SC_WebView() {}
  void loadHtml()
  {
    setHtml( html_str );
  }
  QString toHtml()
  {
    return page() -> currentFrame() -> toHtml();
  }
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
  virtual void mouseReleaseEvent( QMouseEvent* e )
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
    QWebView::mouseReleaseEvent( e );
  }
  virtual void keyReleaseEvent( QKeyEvent* e )
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
    QWebView::keyReleaseEvent( e );
  }
  virtual void resizeEvent( QResizeEvent* e )
  {
    searchBox -> updateGeometry();
    QWebView::resizeEvent( e );
  }
  virtual void focusInEvent( QFocusEvent* e )
  {
    hideSearchBox();
    QWebView::focusInEvent( e );
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
  void linkClickedSlot( const QUrl& url )
  {
    QString clickedurl = url.toString();

    if ( url.isLocalFile() || clickedurl.contains( "battle.net" ) || clickedurl.contains( "google.com" ) || clickedurl.contains( "simulationcraft.org" ) ) 
      // Make sure that battle.net profiles and simulationcraft wiki/website are loaded in gui.
      load( url );
    else // Wowhead links tend to crash the gui, so we'll send them to an external browser.
      QDesktopServices::openUrl( url );
  }

public slots:
  void hideSearchBox()
  {
    previousSearch = ""; // disable clearing of highlighted text on next search
    searchBox -> hide();
  }
  void findSomeText( const QString& text, QWebPage::FindFlags options )
  {
    if ( ! previousSearch.isEmpty() && previousSearch != text )
    {
      findText( "", QWebPage::HighlightAllOccurrences ); // clears previous highlighting
    }
    previousSearch = text;

    if ( searchBox -> wrapSearch() )
    {
      options |= QWebPage::FindWrapsAroundDocument;
    }
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
    QWebPage::FindFlags flags = 0;
    if ( searchBox -> reverseSearch() )
    {
      flags |= QWebPage::FindBackward;
    }
    findSomeText( text, flags );
  }
  void findPrev()
  {
    findSomeText( searchBox -> text(), QWebPage::FindBackward );
  }
  void findNext()
  {
    findSomeText( searchBox -> text(), 0 );
  }
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

  void toggle_pause()
  { sim -> toggle_pause(); }

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

  void start( sim_t* s, int t, const QString& u, const QString& sources, const QString& spec, const QString& role )
  { sim = s; tab = t; url = u; profile = ""; item_db_sources = sources; player = 0; active_spec = spec; m_role = role; QThread::start(); }
  virtual void run();
  ImportThread( SC_MainWindow* mw ) : mainWindow( mw ), sim( 0 ), player( 0 ) {}
};


namespace automation {

  QString tokenize( QString qstr );
  QStringList splitPreservingComments( QString qstr );

  QString automation_main( int sim_type,
                        QString player_class,
                        QString player_spec,
                        QString player_race,
                        QString player_level,
                        QString player_talents,
                        QString player_glyphs,
                        QString player_gear,
                        QString player_rotationHeader,
                        QString player_rotationFooter,
                        QString advanced_text,
                        QString sidebar_text,
                        QString footer_text
                      );

  QString auto_talent_sim( QString player_class,
                           QString base_profile_info,
                           QStringList advanced_text,
                           QString player_glyphs,
                           QString player_gear,
                           QString player_rotation
                         );

  QString auto_glyph_sim( QString player_class,
                          QString base_profile_info,
                          QString player_talents,
                          QStringList advanced_text,
                          QString player_gear,
                          QString player_rotation
                        );

  QString auto_gear_sim( QString player_class,
                         QString base_profile_info, 
                         QString player_talents,
                         QString player_glyphs,
                         QStringList advanced_text,
                         QString player_rotation
                       );

  QString auto_rotation_sim( QString player_class,
                             QString player_spec,
                             QString base_profile_info,
                             QString player_talents,
                             QString player_glyphs,
                             QString player_gear,
                             QString player_rotationHeader,
                             QString player_rotationFooter,
                             QStringList advanced_text,
                             QString sidebar_text
                           );

  QStringList convert_shorthand( QStringList shorthandList, QString sidebar_text );
  QStringList splitOption( QString options_shorthand );
  QStringList splitOnFirst( QString str, const char* delimiter );

} // end automation namespace

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
