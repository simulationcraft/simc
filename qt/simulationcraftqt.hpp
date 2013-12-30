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
  TAB_RECENT,
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
// SC_SearchBox
// ============================================================================

class SC_SearchBox : public QWidget
{
  Q_OBJECT
  QLineEdit* searchBox;
  QMenu* searchBoxContextMenu;
  QToolButton* searchBoxPrev;
  QToolButton* searchBoxNext;
  Qt::Corner corner;
  QAction* reverseAction;
  QAction* wrapAction;
  QAction* hideArrowsAction;
  bool reverse;
  bool wrap;
  bool enableReverseContextMenuItem;
  bool enableWrapAroundContextMenuItem;
  bool enableFindPrevNextContextMenuItem;
  bool enableHideArrowsContextMenuItem;
  bool emitFindOnTextChange;
  bool grabFocusOnShow;
  bool highlightTextOnShow;
public:
  SC_SearchBox( QWidget* parent = nullptr,
      Qt::Corner corner = Qt::BottomLeftCorner,
      bool show_arrows = true,
      QBoxLayout::Direction direction = QBoxLayout::LeftToRight ) :
    QWidget( parent ),
    searchBox( new QLineEdit ),
    searchBoxContextMenu( nullptr ),
    searchBoxPrev( new QToolButton( this ) ),
    searchBoxNext( new QToolButton( this ) ),
    corner( corner ),
    reverseAction( nullptr ),
    wrapAction( nullptr ),
    hideArrowsAction( nullptr ),
    reverse( false ),
    wrap( true ),
    enableReverseContextMenuItem( true ),
    enableWrapAroundContextMenuItem( true ),
    enableFindPrevNextContextMenuItem( true ),
    enableHideArrowsContextMenuItem( true ),
    emitFindOnTextChange( false ),
    grabFocusOnShow( true ),
    highlightTextOnShow( true )
  {
    QSettings settings;
    bool hide_arrows_setting = settings.value( "gui/search_hide_arrows", ! show_arrows ).toBool();

    QAction* altLeftAction =  new QAction( tr( "Find Previous" ), searchBox );
    altLeftAction -> setShortcut( QKeySequence( Qt::ALT + Qt::Key_Left  ) );
    QAction* altRightAction =  new QAction( tr( "Find Next" ), searchBox );
    altRightAction -> setShortcut( QKeySequence( Qt::ALT + Qt::Key_Right ) );
    QShortcut* altLeft =  new QShortcut( altLeftAction -> shortcut(), searchBox );
    QShortcut* altUp =    new QShortcut( QKeySequence( Qt::ALT + Qt::Key_Up    ), searchBox );
    QShortcut* altRight = new QShortcut( altRightAction -> shortcut(), searchBox );
    QShortcut* altDown =  new QShortcut( QKeySequence( Qt::ALT + Qt::Key_Down  ), searchBox );

    if ( enableReverseContextMenuItem ||
         enableWrapAroundContextMenuItem ||
         enableFindPrevNextContextMenuItem )
    {
      searchBoxContextMenu = searchBox -> createStandardContextMenu();
      searchBoxContextMenu -> addSeparator();
      searchBox -> setContextMenuPolicy( Qt::CustomContextMenu );
      connect( searchBox,     SIGNAL( customContextMenuRequested( const QPoint& ) ), this, SLOT( searchBoxContextMenuRequested( const QPoint& ) ) );
    }
    if ( enableFindPrevNextContextMenuItem )
    {
      searchBoxContextMenu -> addAction( altRightAction );
      searchBoxContextMenu -> addAction( altLeftAction );
    }
    if ( enableReverseContextMenuItem )
    {
      reverseAction = searchBoxContextMenu -> addAction( tr( "Reverse" ) );
      reverseAction -> setCheckable( true );
      reverseAction -> setChecked( reverse );
      connect( reverseAction, SIGNAL( triggered( bool ) ), this, SLOT( setReverseSearch( bool ) ) );
    }
    if ( enableWrapAroundContextMenuItem )
    {
      wrapAction = searchBoxContextMenu -> addAction( tr( "&Wrap search" ) );
      wrapAction -> setCheckable( true );
      wrapAction -> setChecked( wrap );
      connect( wrapAction,    SIGNAL( triggered( bool ) ), this, SLOT( setWrapSearch( bool ) ) );
    }
    if ( enableHideArrowsContextMenuItem )
    {
      hideArrowsAction = searchBoxContextMenu -> addAction( tr( "Hide Arrows" ) );
      hideArrowsAction -> setCheckable( true );
      hideArrowsAction -> setChecked( hide_arrows_setting ); // TODO connect to QSettings && sync all other SC_SearchBoxes on change
      connect( hideArrowsAction, SIGNAL( triggered( bool ) ), this, SLOT( setHideArrows( bool ) ) );
    }

    connect( searchBox,     SIGNAL( textChanged( const QString& ) ), this, SIGNAL( textChanged( const QString& ) ) );
    connect( searchBox,     SIGNAL( textChanged( const QString& ) ), this,   SLOT( searchBoxTextChanged( const QString& ) ) );
    connect( altLeftAction, SIGNAL( triggered( bool ) ),             this, SIGNAL( findPrev() ) );
    connect( altLeft,       SIGNAL( activated() ),                   this, SIGNAL( findPrev() ) );
    connect( altUp,         SIGNAL( activated() ),                   this, SIGNAL( findPrev() ) );
    connect( altRightAction,SIGNAL( triggered( bool ) ),             this, SIGNAL( findNext() ) );
    connect( altRight,      SIGNAL( activated() ),                   this, SIGNAL( findNext() ) );
    connect( altDown,       SIGNAL( activated() ),                   this, SIGNAL( findNext() ) );
    connect( searchBoxPrev, SIGNAL( pressed() ),                     this, SIGNAL( findPrev() ) );
    connect( searchBoxNext, SIGNAL( pressed() ),                     this, SIGNAL( findNext() ) );

    QIcon searchPrevIcon(":/icon/leftarrow.png");
    QIcon searchNextIcon(":/icon/rightarrow.png");
    if ( ! searchPrevIcon.pixmap( QSize( 64,64 ) ).isNull() )
    {
      searchBoxPrev -> setIcon( searchPrevIcon );
    }
    else
    {
      searchBoxPrev -> setText( tr( "<" ) );
    }
    if ( ! searchNextIcon.pixmap( QSize( 64,64 ) ).isNull() )
    {
      searchBoxNext -> setIcon( searchNextIcon );
    }
    else
    {
      searchBoxNext -> setText( tr( ">" ) );
    }

    QWidget* first = searchBoxPrev;
    QWidget* second = searchBoxNext;
    if ( direction == QBoxLayout::RightToLeft )
    {
      QWidget* tmp = first;
      first = second;
      second = tmp;
    }
    if ( direction != QBoxLayout::LeftToRight ||
         direction != QBoxLayout::RightToLeft )
      direction = QBoxLayout::LeftToRight;

    QBoxLayout* searchBoxLayout = new QBoxLayout( direction );
    searchBoxLayout -> addWidget( searchBox );
    searchBoxLayout -> addWidget( first );
    searchBoxLayout -> addWidget( second );
    if ( hide_arrows_setting )
    {
      searchBoxPrev -> hide();
      searchBoxNext -> hide();
    }
    setLayout( searchBoxLayout );

    // If this is not set, focus will be sent to parent when user tries to click
    // Which will usually be a SC_TextEdit which will call hide() upon grabbing focus
    searchBoxPrev -> setFocusPolicy( Qt::ClickFocus );
    searchBoxNext -> setFocusPolicy( Qt::ClickFocus );
  }
  void setFocus()
  {
    searchBox -> setFocus();
  }
  QString text() const
  {
    return searchBox -> text();
  }
  void setText( const QString& text )
  {
    searchBox -> setText( text );
  }
  void clear()
  {
    searchBox -> clear();
  }
  void setEmitFindOnTextChange( bool emitOnChange )
  {
    emitFindOnTextChange = emitOnChange;
  }
  void updateGeometry()
  {
    QLayout* widgetLayout = layout();
    QSize widgetSizeHint = widgetLayout -> sizeHint();
    QRect geo( QPoint( 0, 0 ), widgetSizeHint );

    QWidget* parentWidget = qobject_cast< QWidget* >( parent() );
    if ( parentWidget != nullptr )
    {
      QRect parentGeo = parentWidget -> geometry();

      switch( corner )
      {
      case Qt::BottomLeftCorner:
        geo.moveBottomLeft( parentGeo.bottomLeft() );
        break;
      case Qt::TopRightCorner:
        geo.moveTopRight( parentGeo.topRight() );
        break;
      case Qt::BottomRightCorner:
        geo.moveBottomRight( parentGeo.bottomRight() );
        break;
      case Qt::TopLeftCorner:
      default:
        break;
      }

      setGeometry( geo );
    }
  }
  Qt::Corner getCorner()
  {
    return corner;
  }
  bool reverseSearch() const
  {
    return reverse;
  }
  bool wrapSearch() const
  {
    return wrap;
  }
protected:
  virtual void keyPressEvent( QKeyEvent* e )
  {
    // Sending other keys to a QTextEdit (which is probably the parent)
    // Such as return, causes the textedit to not only insert a newline
    // while we are searching... but messes up the input of the textedit pretty badly

    // Only propagate Ctrl+Tab & Backtab
    bool propagate = false;
    switch( e -> key() )
    {
    case Qt::Key_Tab:
      if ( e -> modifiers().testFlag( Qt::ControlModifier ) )
        propagate = true;
      break;
    case Qt::Key_Backtab:
      propagate = true;
      break;
    case Qt::Key_Return:
      // We can use searchBox's returnPressed() signal, but then Shift+Return won't work
      if ( e -> modifiers().testFlag( Qt::ShiftModifier ) )
      {
        findPrev();
      }
      else
      {
        find();
      }
      break;
    default:
      break;
    }
    if ( propagate )
      QWidget::keyPressEvent( e );
  }
  virtual void showEvent( QShowEvent* e )
  {
    Q_UNUSED( e );
    updateGeometry();
    if ( grabFocusOnShow )
    {
      setFocus();
    }
    if ( highlightTextOnShow )
    {
      searchBox -> selectAll();
    }
  }
signals:
  void textChanged( const QString& );
  void findNext();
  void findPrev();
public slots:
  void find()
  {
    if ( ! reverse )
    {
      emit( findNext() );
    }
    else
    {
      emit( findPrev() );
    }
  }
  void setReverseSearch( bool reversed )
  {
    reverse = reversed;
    if ( reverseAction && reverseAction -> isChecked() != reverse )
    {
      reverseAction -> setChecked( reverse );
    }
  }
  void setWrapSearch( bool wrapped )
  {
    wrap = wrapped;
    if ( wrapAction && wrapAction -> isChecked() != wrap )
    {
      wrapAction -> setChecked( wrap );
    }
  }
  void setHideArrows( bool hideArrows )
  {
    searchBoxPrev -> setVisible( ! hideArrows );
    searchBoxNext -> setVisible( ! hideArrows );
    updateGeometry();
    if ( hideArrowsAction && hideArrowsAction -> isChecked() != hideArrows )
    {
      hideArrowsAction -> setChecked( hideArrows );
    }
  }
private slots:
  void searchBoxTextChanged( const QString& text )
  {
    Q_UNUSED( text );
    if ( emitFindOnTextChange )
      find();
  }
  void searchBoxContextMenuRequested( const QPoint& p )
  {
    searchBoxContextMenu -> exec( searchBox -> mapToGlobal( p ) );
  }
};

// ============================================================================
// SC_PlainTextEdit
// ============================================================================

class SC_TextEdit : public QTextEdit
{
  Q_OBJECT
private:
  QTextCharFormat textformat_default;
  QTextCharFormat textformat_error;
  QList< QPair< Qt::Key, QList< Qt::KeyboardModifier > > > ignoreKeys;
  SC_SearchBox* searchBox;
  Qt::Corner searchBoxCorner;
  bool enable_search;
public:
  bool edited_by_user;

  SC_TextEdit( QWidget* parent = 0, bool accept_drops = true, bool enable_search = true ) :
    QTextEdit( parent ),
    searchBox( nullptr ),
    searchBoxCorner( Qt::BottomLeftCorner ),
    enable_search( enable_search ),
    edited_by_user( false )
  {
    textformat_error.setFontPointSize( 20 );

    setAcceptDrops( accept_drops );
    setLineWrapMode( QTextEdit::NoWrap );

    QList< Qt::KeyboardModifier > ctrl;
    ctrl.push_back( Qt::ControlModifier );
    addIgnoreKeyPressEvent( Qt::Key_Tab, ctrl );
    addIgnoreKeyPressEvent( Qt::Key_W, ctrl ); // Close tab
    addIgnoreKeyPressEvent( Qt::Key_T, ctrl ); // New tab
    QList< Qt::KeyboardModifier > nothing;
    addIgnoreKeyPressEvent( Qt::Key_Backtab, nothing );

    connect( this, SIGNAL( textChanged() ), this, SLOT( text_edited() ) );

    if ( enable_search )
    {
      initSearchBox();
    }
  }

  void setformat_error()
  { //setCurrentCharFormat( textformat_error );
  }

  void resetformat()
  { //setCurrentCharFormat( textformat_default );
  }

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
          QAbstractScrollArea::keyPressEvent( e );
          return;
        }
      }
    }
    // no key match
    QTextEdit::keyPressEvent( e );
  }
  void initSearchBox()
  {
    if ( ! searchBox )
    {
      searchBox = new SC_SearchBox( this -> viewport(), searchBoxCorner );

      QShortcut* find = new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_F), this );
      connect( find, SIGNAL( activated() ), this, SLOT( find() ) );
      QShortcut* escape = new QShortcut( QKeySequence( Qt::Key_Escape ), this );
      connect( escape, SIGNAL( activated() ), this, SLOT( hideSearchBox() ) );
      connect( searchBox, SIGNAL( textChanged( const QString&) ), this, SLOT( findText( const QString& ) ) );

      connect( searchBox, SIGNAL( findPrev() ), this, SLOT( findPrev() ) );
      connect( searchBox, SIGNAL( findNext() ), this, SLOT( findNext() ) );

      searchBox -> hide();
    }
  }
  virtual void focusInEvent( QFocusEvent* e )
  {
    if ( enable_search )
    {
      hideSearchBox();
    }
    QTextEdit::focusInEvent( e );
  }
  virtual void resizeEvent( QResizeEvent* e )
  {
    Q_UNUSED( e );
    searchBox -> updateGeometry();
  }
  void findSomeText( const QString& text, QTextDocument::FindFlags options = 0, int position = -1 )
  {
    if ( ! text.isEmpty() )
    {
      QTextDocument* doc = document();
      if ( position < 0 )
        position = textCursor().selectionStart();
      QTextCursor found = doc -> find( text, position, options );
      if ( ! found.isNull() )
      {
        setTextCursor( found );
      }
      else if ( searchBox -> wrapSearch() )
      {
        if ( options & QTextDocument::FindBackward )
        {
          position = doc -> characterCount();
        }
        else
        {
          position = 0;
        }
        found = doc -> find( text, position, options );
        if ( ! found.isNull() )
        {
          setTextCursor( found );
        }
      }

    }
  }
private slots:
  void text_edited()
  {
    edited_by_user = true;
  }
public slots:
  void find()
  {
    if ( enable_search )
    {
      searchBox -> show();
    }
  }
  void findText( const QString& text )
  {
    QTextDocument::FindFlags flags;
    if ( searchBox -> reverseSearch() )
      flags |= QTextDocument::FindBackward;
    findSomeText( text, flags );
  }
  void findNext()
  {
    if ( enable_search )
      findSomeText( searchBox -> text(), 0, textCursor().selectionStart() + 1 );
  }
  void findPrev()
  {
    if ( enable_search )
      findSomeText( searchBox -> text(), QTextDocument::FindBackward );
  }
  void hideSearchBox()
  {
    if ( searchBox -> isVisible() )
    {
      searchBox -> hide();
      setFocus();
    }
  }
};

// ============================================================================
// SC_HoverAreaWidget
// ============================================================================

class SC_HoverArea : public QWidget
{
Q_OBJECT
Q_PROPERTY( int timeout READ timeout WRITE setTimeout )
  int timeout_;
  QTimer timeSinceMouseEntered;
public:
  SC_HoverArea( QWidget* parent = 0, int timeout = 1000 ) :
      QWidget( parent ), timeout_( timeout )
  {
    setMouseTracking(true);
    connect( &timeSinceMouseEntered, SIGNAL( timeout() ), this,
        SLOT( TimerTimeout() ) );
    timeSinceMouseEntered.setSingleShot( true );
  }

  int timeout() const
  {
    return timeout_;
  }

  void setTimeout( int timeout )
  {
    timeout_ = timeout;
  }

protected:
  virtual void enterEvent( QEvent* /* event */ )
  {
    if ( underMouse() && timeSinceMouseEntered.isActive() == false )
      timeSinceMouseEntered.start( timeout_ );
  }

  virtual void leaveEvent( QEvent* /* event */ )
  {
    timeSinceMouseEntered.stop();
  }

public slots:
  virtual void TimerTimeout()
  {
    emit( mouseHoverTimeout() );
  }

signals:
  void mouseHoverTimeout();
};

// ============================================================================
// SC_RelativePopupWidget
// ============================================================================

class SC_RelativePopup : public QWidget
{
Q_OBJECT
Q_PROPERTY( Qt::Corner parentCornerToAnchor READ parentCornerToAnchor WRITE setParentCornerToAnchor )
Q_PROPERTY( Qt::Corner widgetCornerToAnchor READ widgetCornerToAnchor WRITE setWidgetCornerToAnchor )
Q_PROPERTY( int timeTillHide READ timeTillHide WRITE setTimeTillHide )
Q_PROPERTY( int timeTillFastHide READ timeTillFastHide WRITE setTimeTillFastHide )
Q_PROPERTY( int timeFastHide READ timeFastHide WRITE setTimeFastHide )
public:
  SC_RelativePopup(QWidget* parent, Qt::Corner parentCornerToAnchor =
      Qt::BottomRightCorner, Qt::Corner widgetCornerToAnchor =
      Qt::TopRightCorner) :
      QWidget(parent), parentCornerToAnchor_(parentCornerToAnchor),
      widgetCornerToAnchor_( widgetCornerToAnchor), timeTillHide_(1000),
      timeTillFastHide_(800), timeFastHide_(200), hideChildren(true)
  {
    setMouseTracking(true);
    setWindowFlags(Qt::Popup | Qt::FramelessWindowHint);

    timerTillHide_.setSingleShot(true);
    timerTillFastHide_.setSingleShot(true);

    timeToWait__ = timeTillHide_;

    connect(&timerTillHide_, SIGNAL( timeout() ), this, SLOT( hideRequest() ));
    connect(&timerTillFastHide_, SIGNAL( timeout() ), this, SLOT( fastHideRequest() ));
  }
  // Q_PROPERTIES
  Qt::Corner parentCornerToAnchor() const
  {
    return parentCornerToAnchor_;
  }
  void setParentCornerToAnchor(Qt::Corner corner)
  {
    parentCornerToAnchor_ = corner;
  }
  Qt::Corner widgetCornerToAnchor() const
  {
    return parentCornerToAnchor_;
  }
  void setWidgetCornerToAnchor(Qt::Corner corner)
  {
    parentCornerToAnchor_ = corner;
  }
  int timeTillHide() const
  {
    return timeTillHide_;
  }
  void setTimeTillHide(int msec)
  {
    timeTillHide_ = msec;
  }
  int timeTillFastHide() const
  {
    return timeTillFastHide_;
  }
  void setTimeTillFastHide(int msec)
  {
    timeTillHide_ = msec;
  }
  int timeFastHide() const
  {
    return timeFastHide_;
  }
  void setTimeFastHide(int msec)
  {
    timeFastHide_ = msec;
  }
private:
  Qt::Corner parentCornerToAnchor_;
  Qt::Corner widgetCornerToAnchor_;
  // Initial time to hide
  int timeTillHide_;
  QTimer timerTillHide_;
  // After being visible/having focus for this time,
  // the mouse leaving should hide quickly
  int timeTillFastHide_;
  QTimer timerTillFastHide_;
  int timeFastHide_;
  int timeToWait__;

  int xbuffer;
  int ybuffer;
  bool hideChildren;
  void disableChildrenHiding()
  {
    hideChildren = false;
  }
  void enableChildrenHiding()
  {
    hideChildren = true;
  }
  bool isWidgetUnderCursorAChild()
  {
    QWidget *widget = qApp->widgetAt(QCursor::pos());

    while( widget != nullptr )
    {
      if ( widget == this )
      {
        return true;
      }
      widget = widget -> parentWidget();
    }

    return false;
  }
  void calculateGeometry()
  {
    // Convert to global points
    QObject* parent__ = parent();
    if ( parent__ == NULL )
      return;
    QWidget* parent_ = static_cast< QWidget* >( parent__ );
    QRect parentRect = parent_ -> geometry();
    parentRect.moveTopLeft( parent_ -> mapToGlobal(QPoint(0,0)) );

    QRect widgetRect( parentRect );
    QLayout* layout_ = layout();
    if ( layout_ != NULL )
    {
      QApplication::instance() -> sendPostedEvents();
      layout_ -> activate();
      layout_ -> update();
    }
    QRect normalGeometry_ = normalGeometry();
    // Use normal geometry if there is any
    if ( normalGeometry_.width() != 0 && normalGeometry_.height() != 0 )
    {
      widgetRect.setWidth( normalGeometry_.width() );
      widgetRect.setHeight( normalGeometry_.height() );
    }
    if ( layout_ != nullptr )
    {
      QSize sizeHint = layout_ -> sizeHint();
      if ( widgetRect.height() < sizeHint.height() )
        widgetRect.setHeight( sizeHint.height() );
      if ( widgetRect.width() < sizeHint.width() )
        widgetRect.setWidth( sizeHint.width() );
      widgetRect.setSize( sizeHint );
    }

    QPoint bindTo;

    switch(parentCornerToAnchor_)
    {
      default:
      case Qt::TopLeftCorner:
      bindTo = parentRect.topLeft(); break;
      case Qt::TopRightCorner:
      bindTo = parentRect.topRight(); break;
      case Qt::BottomLeftCorner:
      bindTo = parentRect.bottomLeft(); break;
      case Qt::BottomRightCorner:
      bindTo = parentRect.bottomRight(); break;
    }

    switch(widgetCornerToAnchor_)
    {
      default:
      case Qt::TopLeftCorner:
      widgetRect.moveTopLeft( bindTo ); break;
      case Qt::TopRightCorner:
      widgetRect.moveTopRight( bindTo ); break;
      case Qt::BottomLeftCorner:
      widgetRect.moveBottomLeft( bindTo ); break;
      case Qt::BottomRightCorner:
      widgetRect.moveBottomRight( bindTo ); break;
    }

    QDesktopWidget desktopWidget;
    // If user only has one screen, ensure the popup doesn't go off screen
    // If multiple screens, this could be annoying as the popup can be viewed on a 2nd screen
    if ( desktopWidget.screenCount() == 1)
      widgetRect = desktopWidget.screenGeometry( parent_ ).intersected( widgetRect );
    setGeometry( widgetRect );
  }
public slots:
  void hideRequest()
  {
    // Set time to hide to default length
    timeToWait__ = timeTillHide_;
    if ( isVisible() )
    {
      if ( ! isWidgetUnderCursorAChild() )
      {
        hide();
        if ( hideChildren )
        {
          // Hide children that are Popups
          // If we hide all children, floating widgets will be empty when they are shown again
          QRegExp matchAll(".*");
          QList< QWidget* > children = findChildren< QWidget* >( matchAll );

          for ( QList< QWidget*>::iterator it = children.begin();
              it != children.end(); ++it )
          {
            if ( ( *it ) -> windowType() == Qt::Popup )
            ( *it ) -> hide();
          }
          QList< SC_RelativePopup* > popupChildren = findChildren< SC_RelativePopup* >( matchAll );
          for ( QList< SC_RelativePopup* >::iterator it = popupChildren.begin();
              it != popupChildren.end(); ++it )
          {
            ( *it ) -> disableChildrenHiding();
          }
        }
      }
      else
      {
        timerTillHide_.start( timeToWait__ );
      }
    }
  }
  void fastHideRequest()
  {
    // set time to hide to the fast version
    timeToWait__ = timeFastHide_;
  }
protected:
  virtual void showEvent( QShowEvent* /* event */)
  {
    // Start waiting for the mouse
    if ( ! underMouse() )
    {
      if ( timerTillHide_.isActive() )
      timerTillHide_.stop();
      if ( timerTillFastHide_.isActive() )
      timerTillFastHide_.stop();
      timerTillHide_.start( timeTillHide_ );
    }
    calculateGeometry();
  }
  virtual void enterEvent( QEvent* /* event */)
  {
    if ( underMouse() )
    {
      // Have mouse so stop waiting to hide
      timerTillHide_.stop();
      // If we have the mouse for so long,
      // make the next hide quicker
      // most likely the user is done with the widget
      if ( timeToWait__ != timeFastHide_ )
      timerTillFastHide_.start( timeTillFastHide_ );
    }
  }
  virtual void leaveEvent( QEvent* /* event */)
  {
    timerTillHide_.start( timeToWait__ );
  }
  virtual void resizeEvent( QResizeEvent* event )
  {
    if ( event -> oldSize() != size() )
    {
      calculateGeometry();
    }
  }
};

// ============================================================================
// SC_TabBar
// ============================================================================

class SC_TabBar : public QTabBar
{
Q_OBJECT
  int hoveringOverTab;
Q_PROPERTY( int draggedTextHoverTimeout READ getDraggedTextHoverTimeout WRITE setDraggedTextHoverTimeout )
  QTimer draggedTextOnSingleTabTimer;
  bool enableDraggedTextHoverSignal;
  int draggedTextTimeout;
Q_PROPERTY( int nonDraggedHoverTimeout READ getMouseHoverTimeout WRITE setMouseHoverTimeout )
  QTimer mouseHoverTimeoutTimer;
  bool enableMouseHoverTimeoutSignal;
  int mouseHoverTimeout;
public:
  SC_TabBar( QWidget* parent = nullptr,
      bool enableDraggedHover = false,
      bool enableMouseHover = false ) :
    QTabBar( parent ),
    hoveringOverTab( -1 ),
    enableDraggedTextHoverSignal( enableDraggedHover ),
    draggedTextTimeout( 0 ),
    enableMouseHoverTimeoutSignal( enableMouseHover ),
    mouseHoverTimeout( 1500 )
  {
    enableDraggedTextHover( enableDraggedTextHoverSignal );
    enableMouseHoverTimeout( enableMouseHoverTimeoutSignal );

    connect( &draggedTextOnSingleTabTimer, SIGNAL( timeout() ), this, SLOT( draggedTextTimedout() ) );
    connect( &mouseHoverTimeoutTimer, SIGNAL( timeout() ), this, SLOT( mouseHoverTimedout() ) );
  }
  void enableDraggedTextHover( bool enable )
  {
    enableDraggedTextHoverSignal = enable;
    setMouseTracking( enableMouseHoverTimeoutSignal || enable );
    setAcceptDrops( enable );
  }
  void setDraggedTextHoverTimeout( int milisec )
  {
    draggedTextTimeout = milisec;
  }
  int getDraggedTextHoverTimeout() const
  {
    return draggedTextTimeout;
  }
  void enableMouseHoverTimeout( bool enable )
  {
    enableMouseHoverTimeoutSignal = enable;
    setMouseTracking( enableDraggedTextHoverSignal || enable );
  }
  void setMouseHoverTimeout( int milisec )
  {
    mouseHoverTimeout = milisec;
  }
  int getMouseHoverTimeout() const
  {
    return mouseHoverTimeout;
  }
protected:
  virtual void dragEnterEvent( QDragEnterEvent* e )
  {
    if ( enableDraggedTextHoverSignal )
      e -> acceptProposedAction();

    QTabBar::dragEnterEvent( e );
  }
  virtual void dragLeaveEvent( QDragLeaveEvent* e )
  {
    draggedTextOnSingleTabTimer.stop();
    mouseHoverTimeoutTimer.stop();
    hoveringOverTab = -1;

    QTabBar::dragLeaveEvent( e );
  }
  virtual void dragMoveEvent( QDragMoveEvent* e )
  {
    if ( enableDraggedTextHoverSignal )
    {
      // Mouse has moved, check if the mouse has moved to a different tab and if so restart timer
      int tabUnderMouse = tabAt( e -> pos() );
      // Check if the tab has been changed
      if ( hoveringOverTab != tabUnderMouse )
      {
        startDraggedTextTimer( tabUnderMouse );
      }
      e -> acceptProposedAction();
    }

    QTabBar::dragMoveEvent( e );
  }
  virtual void dropEvent( QDropEvent* e )
  {
    // dropEvent is protected so we cannot forward this to the widget.. just dont accept it

    QTabBar::dropEvent( e );
  }
  virtual void mouseMoveEvent( QMouseEvent* e )
  {
    if ( enableMouseHoverTimeoutSignal )
    {
      // Mouse has moved, check if the mouse has moved to a different tab and if so restart timer
      int tabUnderMouse = tabAt( e -> pos() );
      // Check if the tab has been changed
      if ( hoveringOverTab != tabUnderMouse )
      {
        startHoverTimer( tabUnderMouse );
      }
    }

    QTabBar::mouseMoveEvent( e );
  }
  virtual void leaveEvent( QEvent* e )
  {
    draggedTextOnSingleTabTimer.stop();
    mouseHoverTimeoutTimer.stop();
    hoveringOverTab = -1;

    QTabBar::leaveEvent( e );
  }
  virtual void enterEvent( QEvent* e )
  {
    startHoverTimer( tabAt( QCursor::pos() ) );

    QTabBar::enterEvent( e );
  }
  virtual void startDraggedTextTimer( int tab )
  {
    if ( enableDraggedTextHoverSignal && tab >= 0 )
    {
      hoveringOverTab = tab;
      draggedTextOnSingleTabTimer.start( draggedTextTimeout );
    }
  }
  virtual void startHoverTimer( int tab )
  {
    if ( enableMouseHoverTimeoutSignal && tab >= 0 )
    {
      hoveringOverTab = tab;
      mouseHoverTimeoutTimer.start( mouseHoverTimeout );
    }
  }
  virtual bool event( QEvent* e )
  {
    if ( e -> type() == QEvent::LayoutRequest )
    {
      // Issued when drag is completed
      // Best way I can find to enforce the SC_SimulateTab addTabWidget's location
      emit( layoutRequestEvent() );
    }

    return QTabBar::event( e );
  }
public slots:
  void mouseHoverTimedout()
  {
    mouseHoverTimeoutTimer.stop();
    emit( mouseHoveredOverTab( hoveringOverTab ) );
  }
  void draggedTextTimedout()
  {
    draggedTextOnSingleTabTimer.stop();
    emit( mouseDragHoveredOverTab( hoveringOverTab ) );
  }
signals:
  void mouseHoveredOverTab( int tab );
  void mouseDragHoveredOverTab( int tab );
  void layoutRequestEvent();
};

// ============================================================================
// SC_TabWidgetCloseAll
// ============================================================================

class SC_TabWidgetCloseAll : public QTabWidget
{
Q_OBJECT
  QSet<QWidget*> specialTabsListToNotDelete;

  virtual QWidget* createCloseAllTabsWidget()
  {
    // default will be a close all tabs button
    QToolButton* closeAllTabs = new QToolButton(this);

    QIcon closeAllTabsIcon(":/icon/closealltabs.png");
    if (closeAllTabsIcon.pixmap(QSize(64, 64)).isNull()) // icon failed to load
    {
      closeAllTabs->setText(tr("Close All Tabs"));
    }
    else
    {
      closeAllTabs->setIcon(closeAllTabsIcon);
    }
    closeAllTabs->setAutoRaise(true);

    connect( closeAllTabs, SIGNAL( clicked( bool ) ), this, SLOT( closeAllTabsRequest( bool ) ));

    return closeAllTabs;
  }
  QString closeAllTabsTitleText;
  QString closeAllTabsBodyText;
  SC_TabBar* scTabBar;
public:
  SC_TabWidgetCloseAll(QWidget* parent = nullptr,
      Qt::Corner corner = Qt::TopRightCorner,
      QString warningTitle = "Close all tabs?",
      QString warningText = "Close all tabs?" ) :
  QTabWidget( parent ),
  closeAllTabsTitleText( warningTitle ),
  closeAllTabsBodyText( warningText ),
  scTabBar( new SC_TabBar( this ) )
  {
    setTabBar( scTabBar );
    setTabsClosable( true );
    setCornerWidget( createCloseAllTabsWidget(), corner );
    connect( scTabBar, SIGNAL( layoutRequestEvent() ), this, SIGNAL( tabBarLayoutRequestEvent() ) );
    connect( scTabBar, SIGNAL( mouseHoveredOverTab( int ) ), this, SIGNAL( mouseHoveredOverTab( int ) ) );
    connect( scTabBar, SIGNAL( mouseDragHoveredOverTab( int ) ), this, SIGNAL( mouseDragHoveredOverTab( int ) ) );
  }
  void setCloseAllTabsTitleText( QString text )
  {
    closeAllTabsTitleText = text;
  }
  void setCloseAllTabsBodyText( QString text )
  {
    closeAllTabsBodyText = text;
  }
  void addCloseAllExclude( QWidget* widget )
  {
    specialTabsListToNotDelete.insert( widget );
  }
  bool removeCloseAllExclude( QWidget* widget )
  {
    return specialTabsListToNotDelete.remove( widget );
  }
  void removeAllCloseAllExcludes()
  {
    specialTabsListToNotDelete.clear();
  }
  bool isACloseAllExclude( QWidget* widget )
  {
    return specialTabsListToNotDelete.contains( widget );
  }
  void closeAllTabs()
  {
    // add more special widgets to never delete here
    int totalTabsToDelete = count();
    for ( int i = 0; i < totalTabsToDelete; ++i )
    {
      bool deleteIndexOk = false;
      int indexToDelete = 0;
      // look for a tab not in the specialTabsList to delete
      while ( ! deleteIndexOk && indexToDelete <= count() )
      {
        if ( specialTabsListToNotDelete.contains( widget( indexToDelete ) ) )
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
        assert( specialTabsListToNotDelete.contains( widget( indexToDelete ) ) == false );
        removeTab( indexToDelete );
      }
    }
  }
  void removeTab( int index )
  {
    emit( tabAboutToBeRemoved( widget( index ), tabText( index ), tabToolTip( index ), tabIcon( index ) ) );
  }
  void enableMouseHoveredOverTabSignal( bool enable )
  {
    scTabBar -> enableMouseHoverTimeout( enable );
  }
  void enableDragHoveredOverTabSignal( bool enable )
  {
    scTabBar -> enableDraggedTextHover( enable );
  }
public slots:
  void closeAllTabsRequest( bool /* clicked */)
  {
    int confirm = QMessageBox::warning( this, closeAllTabsTitleText, closeAllTabsBodyText, QMessageBox::Yes | QMessageBox::No, QMessageBox::No );
    if ( confirm == QMessageBox::Yes )
      closeAllTabs();
  }
signals:
  void tabAboutToBeRemoved( QWidget*, const QString& tabTitle, const QString& tabToolTip, const QIcon& );
  void mouseHoveredOverTab( int tab );
  void mouseDragHoveredOverTab( int tab );
  void tabBarLayoutRequestEvent();
};

// ============================================================================
// SC_RecentlyClosedTabItemModel
// ============================================================================

class SC_RecentlyClosedTabItemModel : public QStandardItemModel
{
Q_OBJECT
public:
  SC_RecentlyClosedTabItemModel( QObject* parent = nullptr ) :
    QStandardItemModel( parent )
  {
    connect( this, SIGNAL( rowsAboutToBeRemoved( const QModelIndex&, int, int ) ), this, SLOT( rowsAboutToBeRemovedSlot( const QModelIndex&, int, int ) ) );
    connect( this, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT( rowsInsertedSlot( const QModelIndex&, int, int ) ) );
    connect( this, SIGNAL( modelAboutToBeReset() ), this, SLOT( modelAboutToBeResetSlot() ) );
  }
signals:
  void removePreview( QWidget* );
  void newTabAdded( QWidget* widget, const QString& title, const QString& tooltip, const QIcon& icon );
  void restoreTab( QWidget* widget, const QString& title, const QString& tooltip, const QIcon& icon );
public slots:
  void restoreTab( const QModelIndex& index )
  {
    if ( index.isValid() )
    {
      QWidget* tab = index.data( Qt::UserRole ).value< QWidget* >();
      QString title = index.data( Qt::DisplayRole ).toString();
      QString tooltip = index.data( Qt::ToolTipRole ).toString();
      QIcon icon = index.data( Qt::DecorationRole ).value< QIcon >();

      emit( removePreview( tab ) );
      emit( restoreTab( tab, title, tooltip, icon ) );
      // removeRow will delete the parent
      removeRow( index.row(), index.parent() );
    }
  }
  void addTab( QWidget* widget, const QString& title, const QString& tooltip, const QIcon& icon)
  {
    QStandardItem* closedTab = new QStandardItem;
    QWidget* newparent = new QWidget;
    // Removing a tab from a QTabWidget does not free the tab until the parent is deleted
    widget -> setParent( newparent );
    closedTab -> setData( QVariant::fromValue< QString >( title ), Qt::DisplayRole );
    closedTab -> setData( QVariant::fromValue< QWidget* >( widget ), Qt::UserRole );
    closedTab -> setData( QVariant::fromValue< QWidget* >( newparent ), Qt::UserRole + 1);
    closedTab -> setData( QVariant::fromValue< QString >( tooltip ), Qt::ToolTipRole );
    closedTab -> setData( QVariant::fromValue< QIcon >( icon ), Qt::DecorationRole );

    appendRow( closedTab );
  }
private slots:
  void rowsAboutToBeRemovedSlot( const QModelIndex& parent, int start, int end )
  {
    Q_UNUSED( parent );
    for ( int row = start; row < end; row++ )
    {
      QModelIndex sibling = index( row, 0 );
      QWidget* widget = sibling.data( Qt::UserRole ).value< QWidget* >();
      QWidget* parent = sibling.data( Qt::UserRole + 1 ).value< QWidget* >();
      emit( removePreview( widget ) );
      Q_ASSERT( parent != nullptr );
      delete parent;

      setData( sibling, QVariant::fromValue< QWidget* >( nullptr ), Qt::UserRole );
      setData( sibling, QVariant::fromValue< QWidget* >( nullptr ), Qt::UserRole + 1 );
    }
  }
  void rowsInsertedSlot( const QModelIndex& parent, int start, int end )
  {
    Q_UNUSED( parent );
    for ( int row = start; row < end; row++ )
    {
      QModelIndex sibling = index( row, 0 );
      QWidget* tab = sibling.data( Qt::UserRole ).value< QWidget* >();
      QString title = sibling.data( Qt::DisplayRole ).toString();
      QString tooltip = sibling.data( Qt::ToolTipRole ).toString();
      QIcon icon = sibling.data( Qt::DecorationRole ).value< QIcon >();
      emit( newTabAdded( tab, title, tooltip, icon ) );
    }
  }
  void modelAboutToBeResetSlot()
  {
    rowsAboutToBeRemovedSlot( QModelIndex(), 0, rowCount() );
  }
};

// ============================================================================
// SC_RecentlyClosedTabWidget
// ============================================================================

class SC_RecentlyClosedTabWidget : public QWidget
{
Q_OBJECT
  QWidget* previewPaneCurrentWidget;
  SC_RecentlyClosedTabItemModel* model;
  QBoxLayout::Direction grow;
  QListView* listView;
  QPushButton* clearHistoryButton;
  QWidget* currentlyPreviewedWidget;
  QWidget* currentlyPreviewedWidgetsParent;
  QMenu* contextMenu;
  QBoxLayout* boxLayout;
  int stretchIndex;
  bool previewAnItemOnShow;
  bool enableContextMenu;
  bool enableClearHistoryButton;
  bool enableClearHistoryContextMenu;
public:
  SC_RecentlyClosedTabWidget( QWidget* parent = nullptr,
      QBoxLayout::Direction grow = QBoxLayout::LeftToRight,
      SC_RecentlyClosedTabItemModel* modelToUse = nullptr ) :
    QWidget( parent ),
    model( modelToUse ),
    grow( grow ),
    listView( nullptr ),
    clearHistoryButton( nullptr ),
    currentlyPreviewedWidget( nullptr ),
    currentlyPreviewedWidgetsParent( nullptr ),
    contextMenu( nullptr ),
    boxLayout( nullptr ),
    stretchIndex( 0 ),
    previewAnItemOnShow( true ),
    enableContextMenu( true ),
    enableClearHistoryButton( true ),
    enableClearHistoryContextMenu( false )
  {
    if ( model == nullptr )
    {
      model = new SC_RecentlyClosedTabItemModel( this );
    }

    init();
  }
  SC_RecentlyClosedTabItemModel* getModel()
  {
    return model;
  }
signals:
  void hideRequest();
  void showRequest();
  void restoreTab( QWidget* widget, const QString& title, const QString& tooltip, const QIcon& icon );
protected:
  void init()
  {
    stretchIndex = 1;
    boxLayout = new QBoxLayout( grow );
    boxLayout -> addWidget( initListView() );
    boxLayout -> insertStretch( stretchIndex, 0 );
    setLayout( boxLayout );

    connect( model, SIGNAL( removePreview( QWidget* ) ), this,  SLOT( removePreviewAndAddNextOrHide( QWidget* ) ) );
    connect( model, SIGNAL( restoreTab( QWidget*, const QString&, const QString&, const QIcon& ) ),
             this,  SIGNAL( restoreTab( QWidget*, const QString&, const QString&, const QIcon& ) ) );
  }
  QWidget* initListView()
  {
    QWidget* listViewParent = new QWidget( this );
    listViewParent -> setMinimumHeight( 350 );
    listViewParent -> setMinimumWidth( 100 );
    listViewParent -> setMaximumWidth( 150 );
    QLayout* listViewParentLayout = new QBoxLayout( QBoxLayout::TopToBottom );

    listView = new QListView( listViewParent );
    listView -> setEditTriggers( QAbstractItemView::NoEditTriggers );
    listView -> setMouseTracking( true );
    listView -> setSelectionMode( QListView::SingleSelection );
    listView -> setModel( model );

    listViewParentLayout -> addWidget( listView );
    if ( enableClearHistoryButton )
    {
      clearHistoryButton = new QPushButton( tr( "&Clear History" ), this );
      listViewParentLayout -> addWidget( clearHistoryButton );
      connect( clearHistoryButton, SIGNAL( clicked() ), this, SLOT( clearHistory() ) );
    }

    listViewParent -> setLayout( listViewParentLayout );

    if ( enableContextMenu )
    {
      listView -> setContextMenuPolicy( Qt::ActionsContextMenu );

      QAction* removeFromHistoryAction = new QAction( tr( "&Delete" ), listView );
      connect( removeFromHistoryAction, SIGNAL( triggered( bool ) ), this,  SLOT( removeCurrentItem( bool ) ) );
      listView -> addAction( removeFromHistoryAction );
      if ( enableClearHistoryContextMenu )
      {
        QAction* clearHistoryAction = new QAction( tr( "&Clear History" ), listView );
        connect( clearHistoryAction,      SIGNAL( triggered( bool ) ), this,  SLOT( clearHistoryPrompt() ) );
        listView -> addAction( clearHistoryAction );
      }
    }

    connect( listView,             SIGNAL( activated( const QModelIndex& ) ),             model, SLOT( restoreTab( const QModelIndex& ) ) );
    connect( listView -> selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( currentChanged( const QModelIndex&, const QModelIndex& ) ) );

    return listViewParent;
  }
protected:
  void showEvent( QShowEvent* e )
  {
    Q_UNUSED( e );
    if ( previewAnItemOnShow )
      previewNext();
  }
public slots:
  void addTab( QWidget* widget, const QString& title, const QString& tooltip, const QIcon& icon)
  {
    model -> addTab( widget, title, tooltip, icon );
  }
  void removePreview( QWidget* widget = nullptr )
  {
    if ( widget == nullptr )
      widget = currentlyPreviewedWidget;
    if ( currentlyPreviewedWidget == nullptr )
      return;
    if ( currentlyPreviewedWidget == widget )
    {
      boxLayout -> setStretch( stretchIndex, 1 );
      boxLayout -> removeWidget( currentlyPreviewedWidget );

      if ( currentlyPreviewedWidgetsParent != nullptr )
        currentlyPreviewedWidget -> setParent( currentlyPreviewedWidgetsParent );

      currentlyPreviewedWidget = nullptr;
      currentlyPreviewedWidgetsParent = nullptr;
    }
  }
  void removePreviewAndAddNextOrHide( QWidget* widget )
  {
    removePreview( widget );
    previewNextOrHide( widget );
  }
  void removePreviousTabFromPreview()
  {
    removePreview( currentlyPreviewedWidget );
  }
  void restoreCurrentlySelected()
  {
    model -> restoreTab( listView -> selectionModel() -> currentIndex() );
  }
  bool addIndexToPreview( const QModelIndex& index )
  {
    if ( index.isValid() )
    {
      QWidget* widget = index.data( Qt::UserRole ).value< QWidget* >();
      if ( widget != nullptr )
      {
        if ( currentlyPreviewedWidget != nullptr )
          removePreview( currentlyPreviewedWidget );
        widget -> show();
        boxLayout -> addWidget( widget, 1 );
        boxLayout -> setStretch( stretchIndex, 0 );
        currentlyPreviewedWidget = widget;
        currentlyPreviewedWidgetsParent = index.data( Qt::UserRole + 1 ).value< QWidget* >();
        listView -> setFocus();
        return true;
      }
    }
    return false;
  }
  bool previewNext( QWidget* exclude = nullptr )
  {
    bool retval = false;
    QModelIndex index = listView -> currentIndex();
    // If no selection, then go with first index
    if ( ! index.isValid() )
      index = model -> index( 0, 0 );

    QWidget* widget = index.data( Qt::UserRole ).value< QWidget* >();
    int triedRows = 1;

    // Search up if we start at the last element;
    int increment = index.row() == ( model -> rowCount() ) ? -1:1;
    // Try each row looking for a widget that is not excluded
    while ( triedRows <= model -> rowCount() &&
            widget == exclude )
    {
      int newIndex = ( index.row() + increment ) % model -> rowCount();
      index = model -> index( newIndex, 0 );
      widget = index.data( Qt::UserRole ).value< QWidget* >();
      triedRows++;
    }
    // index is ok and we didnt try all rows
    if ( index.isValid() &&
         triedRows <= model -> rowCount() )
    {
      QItemSelectionModel* selection = listView -> selectionModel();
      addIndexToPreview( index );
      selection -> clear();
      selection -> select( index, QItemSelectionModel::Select );
      widget = index.data( Qt::UserRole ).value< QWidget* >();
      if ( currentlyPreviewedWidget != widget )
        addIndexToPreview( index );
      retval = true;
    }
    return retval;
  }
  void previewNextOrHide( QWidget* exclude = nullptr )
  {
    if ( ! previewNext( exclude ) )
      emit( hideRequest() );
  }
  void showContextMenu( const QPoint& p )
  {
    if ( enableContextMenu )
    {
      if ( model -> rowCount() > 0 )
        contextMenu -> exec( listView -> mapToGlobal( p ) );
    }
  }
  void restoreTabSlot( QWidget* widget, const QString& title, const QString& tooltip, const QIcon& icon )
  {
    emit( restoreTab( widget, title, tooltip, icon ) );
  }
  void clearHistory()
  {
    model -> clear();
    emit( hideRequest() );
  }
  void removeCurrentItem( bool /* checked */ )
  {
    QItemSelectionModel* selection = listView -> selectionModel();
    QModelIndex index = selection -> currentIndex();
    int selectedRow = index.row();
    model -> removeRow( index.row(), selection -> currentIndex().parent() );
    index = model -> index( selectedRow, 0 );
    previewNextOrHide();
  }
  void currentChanged( const QModelIndex& current, const QModelIndex& previous )
  {
    Q_UNUSED( previous );
    removePreview();
    addIndexToPreview( current );
  }
};

// ============================================================================
// SC_RecentlyClosedTab
// ============================================================================

class SC_RecentlyClosedTab : public SC_TabWidgetCloseAll
{
Q_OBJECT
  bool enabled;
  SC_HoverArea* hoverArea;
  SC_RelativePopup* recentlyClosedPopup;
  SC_RecentlyClosedTabWidget* recentlyClosedTab;
public:
  SC_RecentlyClosedTab(QWidget* parent = nullptr,
      SC_RecentlyClosedTabItemModel* modelToUse = nullptr,
      bool enableRecentlyClosedTabs = true,
      Qt::Corner corner = Qt::TopRightCorner ) :
  SC_TabWidgetCloseAll( parent, corner ),
  enabled( enableRecentlyClosedTabs ),
  hoverArea( nullptr ),
  recentlyClosedPopup( nullptr ),
  recentlyClosedTab( nullptr )
  {
    if ( enabled )
    {
      hoverArea = new SC_HoverArea( this, 600 );
      QWidget* cornerWidgetOld = cornerWidget( corner );
      assert( cornerWidgetOld != nullptr );

      QBoxLayout* hoverAreaLayout = new QBoxLayout( QBoxLayout::LeftToRight );
      hoverAreaLayout -> setContentsMargins( 0, 0, 0, 0 );
      hoverAreaLayout -> addWidget( cornerWidgetOld );

      hoverArea -> setLayout( hoverAreaLayout );
      cornerWidgetOld -> setParent( hoverArea );

      Qt::Corner parentCorner;
      Qt::Corner widgetCorner;
      QBoxLayout::Direction grow;
      switch ( corner )
      {
      default:
      case Qt::TopRightCorner:
        parentCorner = Qt::BottomRightCorner;
        widgetCorner = Qt::TopRightCorner;
        grow = QBoxLayout::RightToLeft;
        break;
      case Qt::TopLeftCorner:
        parentCorner = Qt::BottomLeftCorner;
        widgetCorner = Qt::TopLeftCorner;
        grow = QBoxLayout::LeftToRight;
        break;
      case Qt::BottomRightCorner: // Bottom corners don't work well
        parentCorner = Qt::TopRightCorner;
        widgetCorner = Qt::BottomRightCorner;
        grow = QBoxLayout::RightToLeft;
        break;
      case Qt::BottomLeftCorner:
        parentCorner = Qt::TopLeftCorner;
        widgetCorner = Qt::BottomLeftCorner;
        grow = QBoxLayout::LeftToRight;
        break;
      }
      recentlyClosedPopup = new SC_RelativePopup( cornerWidgetOld, parentCorner, widgetCorner );
      recentlyClosedTab   = new SC_RecentlyClosedTabWidget( recentlyClosedPopup, grow, modelToUse );
      QBoxLayout* recentlyClosedPopupLayout = new QBoxLayout( grow );
      recentlyClosedPopupLayout -> addWidget( recentlyClosedTab );
      recentlyClosedPopup -> setLayout( recentlyClosedPopupLayout );

      QShortcut* escape = new QShortcut( QKeySequence( Qt::Key_Escape ), recentlyClosedPopup );

      connect( escape,            SIGNAL( activated() ),         recentlyClosedPopup, SLOT( hide() ) );
      connect( recentlyClosedTab, SIGNAL( hideRequest() ),       recentlyClosedPopup, SLOT( hide() ) );
      connect( recentlyClosedTab, SIGNAL( showRequest() ),       recentlyClosedPopup, SLOT( show() ) );
      connect( hoverArea,         SIGNAL( mouseHoverTimeout() ), recentlyClosedPopup, SLOT( show() ) );
      connect( recentlyClosedTab, SIGNAL( restoreTab( QWidget*, const QString&, const QString&, const QIcon& ) ),
               this,              SLOT  ( restoreTab( QWidget*, const QString&, const QString&, const QIcon& ) ) );
      setCornerWidget( hoverArea, corner );
      cornerWidgetOld -> show();
    }
    connect(   this,              SIGNAL( tabAboutToBeRemoved( QWidget*, const QString&, const QString&, const QIcon& ) ),
               recentlyClosedTab, SLOT  ( addTab( QWidget*, const QString&, const QString&, const QIcon&) ) );
  }
  virtual int insertAt()
  {
    return count();
  }
  SC_RecentlyClosedTabItemModel* getModel()
  {
    return recentlyClosedTab -> getModel();
  }
public slots:
  void restoreTab( QWidget* widget, const QString& title, const QString& tooltip, const QIcon& icon )
  {
    int indexToInsert = insertAt();
    widget -> setParent( this );
    indexToInsert = insertTab( indexToInsert, widget, title );
    setTabToolTip( indexToInsert, tooltip );
    setTabIcon( indexToInsert, icon );
    setCurrentIndex( indexToInsert );
  }
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
    if ( addTabWidgetIndex >= 1 )
    {
      if ( addTabWidgetIndex != count() - 1 )
      {
        tabBar() -> moveTab( addTabWidgetIndex, count() - 1 );
      }
    }
  }
};

class SC_OptionsTab;

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
  SC_TextEdit* rawrText;
  QListWidget* historyList;
  SC_TextEdit* overridesText;
  SC_TextEdit* logText;
  QPushButton* backButton;
  QPushButton* forwardButton;
  SC_CommandLine* cmdLine;
  QProgressBar* progressBar;
  QPushButton* mainButton;
  QGroupBox* cmdLineGroupBox;
  QGroupBox* createCustomCharData;
  SC_RecentlyClosedTabItemModel* recentlyClosedTabModel;
  SC_RecentlyClosedTabWidget* recentlyClosedTabImport;

  QTimer* timer;
  ImportThread* importThread;
  SimulateThread* simulateThread;

  QSignalMapper mainTabSignalMapper;
  QList< QShortcut* > shortcuts;
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
  QString ResultsDestDir;
  QString TmpDir;

  QString cmdLineText;
  QString logFileText;
  QString resultsFileText;

  void    startImport( int tab, const QString& url );
  void    startSim();
  sim_t*  initSim();
  void    deleteSim( sim_t* sim, SC_TextEdit* append_error_message = 0 );

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
  void createTabShortcuts();
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
  void reloadButtonClicked( bool checked = false );
  void mainButtonClicked( bool checked = false );
  void mainTabChanged( int index );
  void importTabChanged( int index );
  void resultsTabChanged( int index );
  void resultsTabCloseRequest( int index );
  void rawrButtonClicked( bool checked = false );
  void historyDoubleClicked( QListWidgetItem* item );
  void bisDoubleClicked( QTreeWidgetItem* item, int col );
  void armoryRegionChanged( const QString& region );
  void simulateTabRestored( QWidget* tab, const QString& title, const QString& tooltip, const QIcon& icon );

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
  SC_SearchBox* searchBox;
  QString previousSearch;
  bool allow_mouse_navigation;
  bool allow_keyboard_navigation;
  bool allow_searching;
public:
  SC_MainWindow* mainWindow;
  int progress;
  QString html_str;

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
  void store_html( const QString& s )
  {
    html_str = s;
  }
  virtual ~SC_WebView() {}
  void loadHtml()
  {
    setHtml( html_str );
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
    if ( mainWindow -> visibleWebView == this )
    {
      mainWindow -> progressBar -> setValue( progress );
    }
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
    if ( mainWindow->visibleWebView == this )
    {
      QString s = url.toString();
      if ( s == "about:blank" ) s = "results.html";
      mainWindow->cmdLine->setText( s );
    }
  }
  void linkClickedSlot( const QUrl& url )
  { load( url ); }
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
      options |= QWebPage::FindWrapsAroundDocument;
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
      flags |= QWebPage::FindBackward;
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
