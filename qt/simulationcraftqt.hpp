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

#if defined( Q_WS_MAC ) || defined( Q_OS_MAC )
#include <CoreFoundation/CoreFoundation.h>
#endif
#ifdef QT_VERSION_5
#include <QtWidgets/QtWidgets>
#include <QtWebKitWidgets/QtWebKitWidgets>
#endif

class SC_MainWindow;
class SC_SearchBox;
class SC_TextEdit;
#ifdef SC_PAPERDOLL
#include "simcpaperdoll.hpp"
class Paperdoll;
class PaperdollProfile;
#endif

#include "util/sc_searchbox.hpp" // remove once implementations are moved to source files
#include "util/sc_textedit.hpp" // remove once implementations are moved to source files

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
  , TAB_COUNT
};

enum import_tabs_e
{
  TAB_BATTLE_NET = 0,
#if USE_CHARDEV
  TAB_CHAR_DEV,
#endif
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
  SC_RelativePopup(QWidget* parent,
      Qt::Corner parentCornerToAnchor = Qt::BottomRightCorner,
      Qt::Corner widgetCornerToAnchor = Qt::TopRightCorner) :
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
    QWidget* widget = qApp->widgetAt(QCursor::pos());
    QObject* parentObject = parent();
    QWidget* parentWidget = qobject_cast< QWidget* >( parentObject );

    while( widget != nullptr )
    {
      if ( widget == this )
      {
        return true;
      }
      else if ( parentWidget != nullptr ) // could be one if, but gcc complains about nullptr cast
      {
        if ( widget == parentWidget )
        {
          return true;
        }
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

  bool enableContextMenu;
  bool enableContextMenuRenameTab;
  bool enableContextMenuCloseTab;
  bool enableContextMenuCloseAll;
  bool enableContextMenuCloseOthers;
  QMenu* contextMenu;

  QAction* renameTabAction;
  QAction* closeTabAction;
  QAction* closeOthersAction;
  QAction* closeAllAction;
public:
  SC_TabBar( QWidget* parent = nullptr,
      bool enableDraggedHover = false,
      bool enableMouseHover = false,
      bool enableContextMenu = true ) :
    QTabBar( parent ),
    hoveringOverTab( -1 ),
    enableDraggedTextHoverSignal( enableDraggedHover ),
    draggedTextTimeout( 0 ),
    enableMouseHoverTimeoutSignal( enableMouseHover ),
    mouseHoverTimeout( 1500 ),
    enableContextMenu( enableContextMenu ),
    enableContextMenuRenameTab( true ),
    enableContextMenuCloseTab( true ),
    enableContextMenuCloseAll( false ),
    enableContextMenuCloseOthers( false ),
    contextMenu( nullptr ),
    renameTabAction( nullptr ),
    closeTabAction( nullptr ),
    closeOthersAction( nullptr ),
    closeAllAction( nullptr )
  {
    enableDraggedTextHover( enableDraggedTextHoverSignal );
    enableMouseHoverTimeout( enableMouseHoverTimeoutSignal );

    initContextMenu();

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
  void showContextMenuItemRenameTab( bool show )
  {
    enableContextMenuRenameTab = show;
    updateContextMenu();
  }
  void showContextMenuItemCloseTab( bool show )
  {
    enableContextMenuCloseTab = show;
    updateContextMenu();
  }
  void showContextMenuItemCloseOthers( bool show )
  {
    enableContextMenuCloseOthers = show;
    updateContextMenu();
  }
  void showContextMenuItemCloseAll( bool show )
  {
    enableContextMenuCloseAll = show;
    updateContextMenu();
  }
private:
  virtual void initContextMenu()
  {
    if ( contextMenu != nullptr )
    {
      delete contextMenu;
      contextMenu = nullptr;
    }
    if ( enableContextMenu )
    {
      renameTabAction =    new QAction( tr( "Rename Tab" ),   this );
      closeTabAction =     new QAction( tr( "Close Tab" ),    this );
      closeOthersAction =  new QAction( tr( "Close Others" ), this );
      closeAllAction =     new QAction( tr( "Close All" ),    this );

      connect( renameTabAction,    SIGNAL( triggered( bool ) ), this, SLOT( renameTab() ) );
      connect( closeTabAction,     SIGNAL( triggered( bool ) ), this, SLOT( closeTab() ) );
      connect( closeOthersAction,  SIGNAL( triggered( bool ) ), this, SLOT( closeOthersSlot() ) );
      connect( closeAllAction,     SIGNAL( triggered( bool ) ), this, SLOT( closeAllSlot() ) );

      setContextMenuPolicy( Qt::CustomContextMenu );
      connect( this, SIGNAL( customContextMenuRequested( const QPoint& ) ),
               this, SLOT(   showContextMenu( const QPoint& ) ) );

      contextMenu = new QMenu( this );
      contextMenu -> addAction( renameTabAction );
      contextMenu -> addAction( closeTabAction );
      contextMenu -> addAction( closeOthersAction );
      contextMenu -> addAction( closeAllAction );

      updateContextMenu();
    }
    else
    {
      setContextMenuPolicy( Qt::NoContextMenu );
    }
  }
  void updateContextMenu()
  {
    if ( enableContextMenu )
    {
      renameTabAction   -> setVisible( enableContextMenuRenameTab );
      closeTabAction    -> setVisible( enableContextMenuCloseTab );
      closeOthersAction -> setVisible( enableContextMenuCloseOthers );
      closeAllAction    -> setVisible( enableContextMenuCloseAll );
    }
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
  void renameTab()
  {
    bool ok;
    QString text = QInputDialog::getText( this, tr( "Modify Tab Title" ),
                                          tr("New Tab Title:"), QLineEdit::Normal,
                                          tabText( currentIndex() ), &ok );
    if ( ok && !text.isEmpty() )
       setTabText( currentIndex(), text );
  }
  void closeTab()
  {
    emit( tabCloseRequested( currentIndex() ) );
  }
  void closeOthersSlot()
  {
    emit( closeOthers() );
  }
  void closeAllSlot()
  {
    emit( closeAll() );
  }
  void showContextMenu( const QPoint& pos )
  {
    int tabUnderMouse = tabAt( pos );
    // Right clicking the tab bar while moving a tab causes this request to not be sent
    // Which can cause a segfault if user does ctrl+tab in SC_SimulateTab
    emit( layoutRequestEvent() );
    if ( tabUnderMouse >= 0 )
    {
      setCurrentIndex( tabUnderMouse );
      if ( contextMenu != nullptr )
      {
        contextMenu -> exec( mapToGlobal( pos ) );
      }
    }
  }
signals:
  void mouseHoveredOverTab( int tab );
  void mouseDragHoveredOverTab( int tab );
  void closeOthers();
  void closeAll();
  void layoutRequestEvent();
};

// ============================================================================
// SC_TabWidgetCloseAll
// ============================================================================

class SC_TabWidgetCloseAll : public QTabWidget
{
Q_OBJECT
  QSet<QWidget*> specialTabsListToNotDelete;
  QString closeAllTabsTitleText;
  QString closeAllTabsBodyText;
  QString closeOtherTabsTitleText;
  QString closeOtherTabsBodyText;
  SC_TabBar* scTabBar;
public:
  SC_TabWidgetCloseAll(QWidget* parent = nullptr,
      Qt::Corner corner = Qt::TopRightCorner,
      QString closeAllWarningTitle = "Close all tabs?",
      QString closeAllWarningText = "Close all tabs?",
      QString closeOthersWarningTitle = "Close other tabs?",
      QString closeOthersWarningText = "Close other tabs?" ) :
  QTabWidget( parent ),
  closeAllTabsTitleText( closeAllWarningTitle ),
  closeAllTabsBodyText( closeAllWarningText ),
  closeOtherTabsTitleText( closeOthersWarningTitle ),
  closeOtherTabsBodyText( closeOthersWarningText ),
  scTabBar( new SC_TabBar( this ) )
  {
    initTabBar();
    setCornerWidget( createCloseAllTabsWidget(), corner );
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
  void closeOtherTabs()
  {
    QWidget* currentVisibleTab = widget( currentIndex() );
    bool insertedTabSuccessfully = false;
    if ( currentVisibleTab != nullptr )
    {
      if ( ! specialTabsListToNotDelete.contains( currentVisibleTab ) )
      {
        // if the current tab is one that we should not delete
        // make sure that we do not remove that functionality by removing it from the set
        insertedTabSuccessfully = true;
        specialTabsListToNotDelete.insert( currentVisibleTab );
      }
    }
    closeAllTabs();
    if ( insertedTabSuccessfully )
    {
      specialTabsListToNotDelete.remove( currentVisibleTab );
    }
  }
  void removeTab( int index )
  {
    QWidget* widgetAtIndex = widget( index );
    if ( widgetAtIndex != nullptr )
    {
      // index could be out of bounds, only emit signal if there is a legit widget at the index
      emit( tabAboutToBeRemoved( widgetAtIndex, tabText( index ), tabToolTip( index ), tabIcon( index ) ) );
    }
  }
  void enableMouseHoveredOverTabSignal( bool enable )
  {
    scTabBar -> enableMouseHoverTimeout( enable );
  }
  void enableDragHoveredOverTabSignal( bool enable )
  {
    scTabBar -> enableDraggedTextHover( enable );
  }
private:
  virtual QWidget* createCloseAllTabsWidget()
  {
    // default will be a close all tabs button
    QToolButton* closeAllTabs = new QToolButton(this);

    QIcon closeAllTabsIcon( ":/icon/closealltabs.png" );
    if ( closeAllTabsIcon.pixmap( QSize( 64, 64 ) ).isNull() ) // icon failed to load
    {
      closeAllTabs -> setText( tr( "Close All Tabs" ) );
    }
    else
    {
      closeAllTabs -> setIcon( closeAllTabsIcon );
    }
    closeAllTabs -> setAutoRaise( true );

    connect( closeAllTabs, SIGNAL( clicked( bool ) ), this, SLOT( closeAllTabsRequest() ));

    return closeAllTabs;
  }
  void initTabBar()
  {
    setTabBar( scTabBar );
    setTabsClosable( true );

    scTabBar -> showContextMenuItemRenameTab( true );
    scTabBar -> showContextMenuItemCloseTab( true );
    scTabBar -> showContextMenuItemCloseOthers( true );
    scTabBar -> showContextMenuItemCloseAll( true );

    connect( scTabBar, SIGNAL( layoutRequestEvent() ),           this, SIGNAL( tabBarLayoutRequestEvent() ) );
    connect( scTabBar, SIGNAL( mouseHoveredOverTab( int ) ),     this, SIGNAL( mouseHoveredOverTab( int ) ) );
    connect( scTabBar, SIGNAL( mouseDragHoveredOverTab( int ) ), this, SIGNAL( mouseDragHoveredOverTab( int ) ) );
    connect( scTabBar, SIGNAL( closeOthers() ),                  this,   SLOT( closeOtherTabsRequest() ) );
    connect( scTabBar, SIGNAL( closeAll() ),                     this,   SLOT( closeAllTabsRequest() ) );
  }
public slots:
  void closeAllTabsRequest()
  {
    int confirm = QMessageBox::warning( this, closeAllTabsTitleText, closeAllTabsBodyText, QMessageBox::Yes | QMessageBox::No, QMessageBox::No );
    if ( confirm == QMessageBox::Yes )
      closeAllTabs();
  }
  void closeOtherTabsRequest()
  {
    int confirm = QMessageBox::warning( this, closeOtherTabsTitleText, closeOtherTabsBodyText, QMessageBox::Yes | QMessageBox::No, QMessageBox::No );
    if ( confirm == QMessageBox::Yes )
      closeOtherTabs();
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
  void restoreAllTabs()
  {
    int totalRowsToRestore = rowCount();
    for( int i = 0; i < totalRowsToRestore; i++ )
    {
      restoreTab( index( 0, 0 ) );
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
  bool enableRestoreAllContextMenu;
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
    enableClearHistoryContextMenu( true ),
    enableRestoreAllContextMenu( true )
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

      if ( enableRestoreAllContextMenu )
      {
        QAction* restoreAllAction = new QAction( tr( "&Restore All" ), listView );
        connect( restoreAllAction, SIGNAL( triggered( bool ) ), this, SLOT( restoreAllTabs() ) );
        listView -> addAction( restoreAllAction );
      }
      QAction* removeFromHistoryAction = new QAction( tr( "&Delete" ), listView );
      connect( removeFromHistoryAction, SIGNAL( triggered( bool ) ), this, SLOT( removeCurrentItem() ) );
      listView -> addAction( removeFromHistoryAction );
      if ( enableClearHistoryContextMenu )
      {
        QAction* clearHistoryAction = new QAction( tr( "&Clear History" ), listView );
        connect( clearHistoryAction, SIGNAL( triggered( bool ) ), this, SLOT( clearHistoryPrompt() ) );
        listView -> addAction( clearHistoryAction );
      }
    }

    connect( listView, SIGNAL( activated( const QModelIndex& ) ), model, SLOT( restoreTab( const QModelIndex& ) ) );
    connect( listView -> selectionModel(), SIGNAL( currentChanged( const QModelIndex&, const QModelIndex& ) ), this, SLOT( currentChanged( const QModelIndex&, const QModelIndex& ) ) );

    return listViewParent;
  }
  QModelIndex getSelectionAndMakeSureIsValidIfElementsAvailable()
  {
    QModelIndex index;

    if ( model -> rowCount() > 0 )
    {
      QItemSelectionModel* selection = listView -> selectionModel();
      index = selection -> currentIndex();
      if ( ! index.isValid() && selection -> hasSelection() )
      {
        QModelIndexList selectedIndicies = selection -> selectedIndexes();
        if ( ! selectedIndicies.isEmpty() )
        {
          index = selectedIndicies.first();
        }
      }
    }

    return index;
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
    model -> restoreTab( getSelectionAndMakeSureIsValidIfElementsAvailable() );
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
  void removeCurrentItem()
  {
    QModelIndex index = getSelectionAndMakeSureIsValidIfElementsAvailable();
    removePreviousTabFromPreview();
    int selectedRow = index.row();
    model -> removeRow( index.row() );
    index = model -> index( selectedRow, 0 );
    previewNextOrHide();
  }
  void clearHistoryPrompt()
  {
    if ( model -> rowCount() > 0 )
    {
      int confirm = QMessageBox::warning( nullptr, tr( "Clear history" ), tr( "Clear entire history?" ), QMessageBox::Yes | QMessageBox::No, QMessageBox::No );
      if ( confirm == QMessageBox::Yes )
        clearHistory();
    }
    else
    {
      emit( hideRequest() );
    }
  }
  void restoreAllTabs()
  {
    if ( model -> rowCount() > 0 )
    {
      model -> restoreAllTabs();
    }
    emit( hideRequest() );
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

class SC_MainWindowCommandLine : public QWidget
{
  Q_OBJECT
  Q_PROPERTY( state_e state READ currentState WRITE setState ) // Which page of the stacked layout are we on
public:
  enum state_e
  {
    IDLE = 0,
    SIMULATING,          // Simulating only one, nothing queued
    SIMULATING_MULTIPLE, // Simulating but others are queued
    SIMULATING_PAUSED,
    SIMULATING_MULTIPLE_PAUSED,
    STATE_COUNT
  };
  enum tabs_e // contains main_tabs_e then import_tabs_e
  {
    CMDLINE_TAB_WELCOME = 0,
    CMDLINE_TAB_OPTIONS,
    CMDLINE_TAB_IMPORT,
    CMDLINE_TAB_SIMULATE,
    CMDLINE_TAB_OVERRIDES,
    CMDLINE_TAB_HELP,
    CMDLINE_TAB_LOG,
    CMDLINE_TAB_RESULTS,
    CMDLINE_TAB_SITE,
#ifdef SC_PAPERDOLL
    CMDLINE_TAB_PAPERDOLL,
#endif
    CMDLINE_TAB_BATTLE_NET,
#if USE_CHARDEV
    CMDLINE_TAB_CHAR_DEV,
#endif
    CMDLINE_TAB_RAWR,
    CMDLINE_TAB_BIS,
    CMDLINE_TAB_HISTORY,
    CMDLINE_TAB_RECENT,
    CMDLINE_TAB_CUSTOM,
    CMDLINE_TAB_COUNT
  };
protected:
  QString text_simulate;
  QString text_queue;
  QString text_pause;
  QString text_resume;
  QString text_queue_tooltip;
  QString text_cancel;
  QString text_cancel_all;
  QString text_cancel_all_tooltip;
  QString text_save;
  QString text_import;
  QString text_prev;
  QString text_next;
  QString text_prev_tooltip;
  QString text_next_tooltip;
  QString text_custom;
  QString text_hide_widget; // call hide() on the button, else call show()
  enum widgets_e
  {
    BUTTON_MAIN = 0,
    BUTTON_QUEUE,
    BUTTON_PREV,
    BUTTON_NEXT,
    TEXTEDIT_CMDLINE,
    PROGRESSBAR_WIDGET,
    BUTTON_PAUSE,
    WIDGET_COUNT
  };
  enum progressbar_states_e // Different states for progressbars
  {
    PROGRESSBAR_IGNORE = 0,
    PROGRESSBAR_IDLE,
    PROGRESSBAR_SIMULATING,
    PROGRESSBAR_IMPORTING,
    PROGRESSBAR_BATTLE_NET,
#if USE_CHARDEV
    PROGRESSBAR_CHAR_DEV,
#endif
    PROGRESSBAR_HELP,
    PROGRESSBAR_SITE,
    PROGRESSBAR_STATE_COUNT
  };
  struct _widget_state
  {
    QString* text;
    QString* tool_tip;
    progressbar_states_e progressbar_state;
  };
  _widget_state states[STATE_COUNT][CMDLINE_TAB_COUNT][WIDGET_COUNT]; // all the states
  // ProgressBar state progress/format
  struct _progressbar_state
  {
    QString text;
    QString tool_tip;
    int progress;
  };
  _progressbar_state progressBarFormat[PROGRESSBAR_STATE_COUNT];
  // CommandLine buffers
  QString commandLineBuffer_DEFAULT; // different buffers for different tabs
  QString commandLineBuffer_TAB_BATTLE_NET;
  QString commandLineBuffer_TAB_RESULTS;
  QString commandLineBuffer_TAB_SITE;
  QString commandLineBuffer_TAB_HELP;
  QString commandLineBuffer_TAB_LOG;

  QWidget* widgets[STATE_COUNT][WIDGET_COUNT]; // holds all widgets in all states

  QStackedLayout* statesStackedLayout; // Contains all states
  tabs_e current_tab;
  state_e current_state;
public:
  SC_MainWindowCommandLine( QWidget* parent = nullptr ) :
    QWidget( parent ),
    statesStackedLayout( nullptr ),
    current_tab( CMDLINE_TAB_WELCOME ),
    current_state( IDLE )
  {
    init();
  }
  state_e currentState() const
  {
    return current_state;
  }
  void setSimulatingProgress( int value, QString format, QString toolTip )
  {
    updateProgress( PROGRESSBAR_SIMULATING, value, format, toolTip );
  }
  int getSimulatingProgress()
  {
    return getProgressBarProgressForState( PROGRESSBAR_SIMULATING );
  }
  void setImportingProgress( int value, QString format, QString toolTip )
  {
    updateProgress( PROGRESSBAR_IMPORTING, value, format, toolTip );
  }
  int getImportingProgress()
  {
    return getProgressBarProgressForState( PROGRESSBAR_IMPORTING );
  }
  void setBattleNetLoadProgress( int value, QString format, QString toolTip )
  {
    updateProgress( PROGRESSBAR_BATTLE_NET, value, format, toolTip );
  }
  int getBattleNetProgress()
  {
    return getProgressBarProgressForState( PROGRESSBAR_BATTLE_NET );
  }
#if USE_CHARDEV
  void setCharDevProgress( int value, QString format, QString tool_tip )
  {
    updateProgress( PROGRESSBAR_CHAR_DEV, value, tool_tip );
  }
  int getCharDevProgress()
  {
    return getProgressBarProgressForState( PROGRESSBAR_CHAR_DEV );
  }
#endif
  void setHelpViewProgress( int value, QString format, QString toolTip )
  {
    updateProgress( PROGRESSBAR_HELP, value, format, toolTip );
  }
  int getHelpViewProgress()
  {
    return getProgressBarProgressForState( PROGRESSBAR_HELP );
  }
  void setSiteLoadProgress( int value, QString format, QString toolTip )
  {
    updateProgress( PROGRESSBAR_SITE, value, format, toolTip );
  }
  int getSiteProgress()
  {
    return getProgressBarProgressForState( PROGRESSBAR_SITE );
  }
  QString commandLineText()
  {
    // gets commandline text for the current tab
    return commandLineText( current_tab );
  }
  QString commandLineText( main_tabs_e tab )
  {
    return commandLineText( convertTabsEnum( tab ) );
  }
  QString commandLineText( import_tabs_e tab )
  {
    return commandLineText( convertTabsEnum( tab ) );
  }
  QString commandLineText( tabs_e tab )
  {
    // returns current commandline text for the specified tab in the current state
    QString* text = getText( current_state, tab, TEXTEDIT_CMDLINE );
    QString retval;

    if ( text != nullptr )
    {
      retval = *text;
    }

    return retval;
  }
  void setCommandLineText( QString text )
  {
    // sets commandline text for the current tab
    setCommandLineText( current_tab, text );
  }
  void setCommandLineText( main_tabs_e tab, QString text )
  {
    setCommandLineText( convertTabsEnum( tab ), text );
  }
  void setCommandLineText( import_tabs_e tab, QString text )
  {
    setCommandLineText( convertTabsEnum( tab ), text );
  }
  void setCommandLineText( tabs_e tab, QString text )
  {
    adjustText( current_state, tab, TEXTEDIT_CMDLINE, text );
    updateWidget( current_state, tab, TEXTEDIT_CMDLINE );
  }
  void togglePaused()
  {
    setPaused( !isPaused() );
  }
  void setPaused( bool pause )
  {
    switch( current_state )
    {
    case SIMULATING:
      if ( pause )
      {
        setState( SIMULATING_PAUSED );
      }
      break;
    case SIMULATING_MULTIPLE:
      if ( pause )
      {
        setState( SIMULATING_MULTIPLE_PAUSED );
      }
      break;
    case SIMULATING_PAUSED:
      if ( ! pause )
      {
        setState( SIMULATING );
      }
      break;
    case SIMULATING_MULTIPLE_PAUSED:
      if ( ! pause )
      {
        setState( SIMULATING_MULTIPLE );
      }
      break;
    default:
      break;
    }
  }
  bool isPaused()
  {
    bool retval = false;

    switch( current_state )
    {
    case SIMULATING_PAUSED:
      retval = true;
      break;
    case SIMULATING_MULTIPLE_PAUSED:
      retval = true;
      break;
    default:
      break;
    }

    return retval;
  }
protected:
  void init()
  {
    initStateInfo();
    initStackedLayout();

    setState( current_state );
  }
  void initStateInfo()
  {
    // Initializes all state info
    // use setText() to set text for widgets in states/tabs
    // use setProgressBarState() to set progressbar's state in states/tabs
    // using text_hide_widget as the text, will hide that widget ONLY in that state/tab, and show it in others

    initWidgetsToNull();
    initTextStrings();
    initStatesStructToNull();
    initDefaultStates();

    // states are defaulted: special cases

    initImportStates();
    initLogResultsStates();
    initProgressBarStates();
    initCommandLineBuffers();
    initPauseStates();

  }
  void initWidgetsToNull()
  {
    for ( state_e state = IDLE; state < STATE_COUNT; state++ )
    {
      for ( widgets_e widget = BUTTON_MAIN; widget < WIDGET_COUNT; widget++ )
      {
        widgets[state][widget] = nullptr;
      }
    }
  }
  void initTextStrings()
  {
    // strings shared by widgets
    text_simulate           = tr( "Simulate!"   );
    text_pause              = tr( "Pause!"      );
    text_resume             = tr( "Resume!"     );
    text_queue              = tr( "Queue!"      );
    text_queue_tooltip      = tr( "Click to queue a simulation to run after the current one" );
    text_cancel             = tr( "Cancel! "    );
    text_cancel_all         = tr( "Cancel All!" );
    text_cancel_all_tooltip = tr( "Cancel ALL simulations, including what is queued" );
    text_save               = tr( "Save!"       );
    text_import             = tr( "Import!"     );
    text_prev               = tr( "<"           );
    text_next               = tr( ">"           );
    text_prev_tooltip       = tr( "Backwards"   );
    text_next_tooltip       = tr( "Forwards"    );
    text_hide_widget        = "hide_widget";
    // actually manually typing in hide_widget into the command line will actually HIDE IT
    // so append some garbage to it
    text_hide_widget.append( QString::number( rand() ) );
  }
  void initStatesStructToNull()
  {
    for ( state_e state = IDLE; state < STATE_COUNT; state++ )
    {
      for ( tabs_e tab = CMDLINE_TAB_WELCOME; tab < CMDLINE_TAB_COUNT; tab++ )
      {
        for ( widgets_e widget = BUTTON_MAIN; widget < WIDGET_COUNT; widget++ )
        {
          states[state][tab][widget].text = nullptr;
          states[state][tab][widget].tool_tip = nullptr;
          states[state][tab][widget].progressbar_state = PROGRESSBAR_IGNORE;
        }
      }
    }
  }
  void initDefaultStates()
  {
    // default states for widgets
    for ( tabs_e tab = CMDLINE_TAB_WELCOME; tab < CMDLINE_TAB_COUNT; tab++ )
    {
      // set the same defaults for every state
      for ( state_e state = IDLE; state < STATE_COUNT; state++ )
      {
        // buttons
        setText( state, tab, BUTTON_MAIN , &text_simulate );
        setText( state, tab, BUTTON_QUEUE, &text_queue, &text_queue_tooltip );
        setText( state, tab, BUTTON_PAUSE, &text_pause );
        setText( state, tab, BUTTON_PREV , &text_prev, &text_prev_tooltip );
        setText( state, tab, BUTTON_NEXT , &text_next, &text_next_tooltip );
        // progressbar
        setText( state, tab, PROGRESSBAR_WIDGET, &progressBarFormat[PROGRESSBAR_SIMULATING].text, &progressBarFormat[PROGRESSBAR_SIMULATING].tool_tip );
        setProgressBarState( state, tab, PROGRESSBAR_SIMULATING );
        // commandline buffer
        setText( state, tab, TEXTEDIT_CMDLINE, &commandLineBuffer_DEFAULT );
      }

      // simulating defaults:
      // mainbutton: simulate => cancel
      setText( SIMULATING, tab, BUTTON_MAIN , &text_cancel ); // instead of text_simulate
      setText( SIMULATING_PAUSED, tab, BUTTON_MAIN, &text_cancel );
      setText( SIMULATING_MULTIPLE, tab, BUTTON_MAIN , &text_cancel ); // instead of text_simulate
      setText( SIMULATING_MULTIPLE_PAUSED, tab, BUTTON_MAIN , &text_cancel ); // instead of text_simulate
      setText( SIMULATING, tab, BUTTON_PAUSE, &text_pause );

      // simulating_multiple defaults:
      // cancel => text_cancel_all
      for ( widgets_e widget = BUTTON_MAIN; widget < WIDGET_COUNT; widget++ )
      {
        if ( getText( SIMULATING_MULTIPLE, tab, widget ) == text_cancel )
        {
          setText( SIMULATING_MULTIPLE, tab, widget, &text_cancel_all, &text_cancel_all_tooltip );
          setText( SIMULATING_MULTIPLE_PAUSED, tab, widget, &text_cancel_all, &text_cancel_all_tooltip );
        }
      }
    }
  }
  void initImportStates()
  {
    // TAB_IMPORT + Subtabs
    setText( IDLE, CMDLINE_TAB_IMPORT, BUTTON_MAIN , &text_import );
    // special cases for all TAB_IMPORT subtabs
    // button_main: text_import
    // button_queue: text_import
    for ( import_tabs_e tab = TAB_BATTLE_NET; tab <= TAB_CUSTOM; tab++ )
    {
      tabs_e subtab = convertTabsEnum( tab );
      setText( IDLE      , subtab, BUTTON_MAIN , &text_import );
      setText( SIMULATING, subtab, BUTTON_QUEUE, &text_import );
      setText( SIMULATING_MULTIPLE, subtab, BUTTON_QUEUE, &text_import );
    }
  }
  void initStackedLayout()
  {
    // initializes the stackedlayout with all states
    statesStackedLayout = new QStackedLayout;

    for ( state_e state = IDLE; state < STATE_COUNT; state++ )
    {
      statesStackedLayout -> addWidget( createState( state ) );
    }

    setLayout( statesStackedLayout );
  }
  void initLogResultsStates()
  {
    // log/results: change button text so save is somewhere
    // IDLE: main button => save
    setText( IDLE      , CMDLINE_TAB_LOG    , BUTTON_MAIN , &text_save );
    setText( IDLE      , CMDLINE_TAB_RESULTS, BUTTON_MAIN , &text_save );
    // SIMULATING + SIMULATING_MULTIPLE: queue button => save
    for ( state_e state = SIMULATING; state <= SIMULATING_MULTIPLE_PAUSED; state++ )
    {
      setText( state, CMDLINE_TAB_LOG    , BUTTON_QUEUE, &text_save );
      setText( state, CMDLINE_TAB_RESULTS, BUTTON_QUEUE, &text_save );
    }
  }
  void initProgressBarStates()
  {
    // progressbar: site/chardev/help
    // certain tabs have their own progress bar states
    for ( state_e state = IDLE; state < STATE_COUNT; state++ )
    {
      // import: TAB_IMPORT and all subtabs have the importing state
      setProgressBarState( state, CMDLINE_TAB_IMPORT, PROGRESSBAR_IMPORTING );
      for ( import_tabs_e tab = TAB_BATTLE_NET; tab <= TAB_CUSTOM; tab++ )
      {
        tabs_e importSubtabs = convertTabsEnum( tab );
        setProgressBarState( state, importSubtabs, PROGRESSBAR_IMPORTING );
      }
      // battlenet: battlenet has its own state
      setProgressBarState( state, CMDLINE_TAB_BATTLE_NET, PROGRESSBAR_BATTLE_NET );
      // site: has its own state
      setProgressBarState( state, CMDLINE_TAB_SITE, PROGRESSBAR_SITE );
#if USE_CHARDEV
      // chardev: has its own state
      setProgressBarState( state, CMDLINE_TAB_CHAR_DEV, PROGRESSBAR_CHAR_DEV );
#endif
      // help: has its own state
      setProgressBarState( state, CMDLINE_TAB_HELP, PROGRESSBAR_HELP );
    }

    // set default format for every progressbar state to %p%
    QString defaultProgressBarFormat = "%p%";
    for ( progressbar_states_e state = PROGRESSBAR_IDLE; state < PROGRESSBAR_STATE_COUNT; state++ )
    {
      // QProgressBar -> setFormat()
      setProgressBarFormat( state, defaultProgressBarFormat, false );
      // set all progress to 100
      setProgressBarProgress( state, 100 );
    }
  }
  void initCommandLineBuffers()
  {
    // commandline: different tabs have different buffers
    for ( state_e state = IDLE; state < STATE_COUNT; state++ )
    {
      // log has its own buffer (for save file name)
      setText( state, CMDLINE_TAB_LOG,        TEXTEDIT_CMDLINE, &commandLineBuffer_TAB_LOG );
      // battlenet has its own buffer (for url)
      setText( state, CMDLINE_TAB_BATTLE_NET, TEXTEDIT_CMDLINE, &commandLineBuffer_TAB_BATTLE_NET );
      // site has its own buffer (for url)
      setText( state, CMDLINE_TAB_SITE,       TEXTEDIT_CMDLINE, &commandLineBuffer_TAB_SITE );
      // help has its own buffer (for url)
      setText( state, CMDLINE_TAB_HELP,       TEXTEDIT_CMDLINE, &commandLineBuffer_TAB_HELP );
      // results has its own buffer (for save file name)
      setText( state, CMDLINE_TAB_RESULTS,    TEXTEDIT_CMDLINE, &commandLineBuffer_TAB_RESULTS );
      // everything else shares the default buffer
    }
  }
  void initPauseStates()
  {
    for ( tabs_e tab = CMDLINE_TAB_WELCOME; tab < CMDLINE_TAB_COUNT; tab++ )
    {
      for ( state_e state = SIMULATING_PAUSED; state <= SIMULATING_MULTIPLE_PAUSED; state++ )
      {
        setText( state, tab, BUTTON_PAUSE, &text_resume );
      }
    }
  }
  virtual QWidget* createState( state_e state )
  {
    // Create the widget for the state
    QWidget* stateWidget = new QGroupBox;
    QLayout* stateWidgetLayout = new QHBoxLayout;

    stateWidget -> setLayout( stateWidgetLayout );

    switch ( state )
    {
    case STATE_COUNT:
      break;
    default:
      createCommandLine( state, stateWidget );
      break;
    }

    switch ( state )
    {
    case IDLE:
      createState_IDLE( stateWidget );
      break;
    case SIMULATING:
      createState_SIMULATING( stateWidget );
      break;
    case SIMULATING_MULTIPLE:
      createState_SIMULATING_MULTIPLE( stateWidget );
      break;
    case SIMULATING_PAUSED:
      createState_SIMULATING_PAUSED( stateWidget );
      break;
    case SIMULATING_MULTIPLE_PAUSED:
      createState_SIMULATING_MULTIPLE_PAUSED( stateWidget );
      break;
    default:
      break;
    }

    stateWidget -> setLayout( stateWidgetLayout );

    return stateWidget;
  }
  virtual void createState_IDLE( QWidget* parent )
  {
    // creates the IDLE state
    QLayout* parentLayout = parent -> layout();

    QPushButton* buttonMain = new QPushButton( *getText( IDLE, CMDLINE_TAB_WELCOME, BUTTON_MAIN ), parent );
    setWidget( IDLE, BUTTON_MAIN, buttonMain );
    parentLayout -> addWidget( buttonMain );

    connect( buttonMain, SIGNAL( clicked() ), this, SLOT( mainButtonClicked() ) );
  }
  virtual void createState_SIMULATING( QWidget* parent )
  {
    // creates the SIMULATING state
    _createState_SIMULATING( SIMULATING, parent );
  }
  virtual void createState_SIMULATING_PAUSED( QWidget* parent )
  {
    _createState_SIMULATING( SIMULATING_PAUSED, parent );
  }
  virtual void createState_SIMULATING_MULTIPLE( QWidget* parent )
  {
    // creates the SIMULATING_MULTIPLE state
    _createState_SIMULATING( SIMULATING_MULTIPLE, parent );
  }
  virtual void createState_SIMULATING_MULTIPLE_PAUSED( QWidget* parent )
  {
    _createState_SIMULATING( SIMULATING_MULTIPLE_PAUSED, parent );
  }
  virtual void _createState_SIMULATING( state_e state, QWidget* parent )
  {
    // creates either the SIMULATING or SIMULATING_MULTIPLE states
    QLayout* parentLayout = parent -> layout();

    QPushButton* buttonQueue = new QPushButton( *getText( IDLE, CMDLINE_TAB_WELCOME, BUTTON_QUEUE ), parent );
    QPushButton* buttonMain  = new QPushButton( *getText( IDLE, CMDLINE_TAB_WELCOME, BUTTON_MAIN  ), parent );
    QPushButton* buttonPause = new QPushButton( *getText( IDLE, CMDLINE_TAB_WELCOME, BUTTON_PAUSE ), parent );
    setWidget( state, BUTTON_QUEUE, buttonQueue );
    setWidget( state, BUTTON_MAIN , buttonMain );
    setWidget( state, BUTTON_PAUSE, buttonPause );
    parentLayout -> addWidget( buttonQueue );
    parentLayout -> addWidget( buttonMain );
    parentLayout -> addWidget( buttonPause );

    connect( buttonMain,  SIGNAL( clicked() ), this, SLOT( mainButtonClicked()  ) );
    connect( buttonQueue, SIGNAL( clicked() ), this, SLOT( queueButtonClicked() ) );
    connect( buttonPause, SIGNAL( clicked() ), this, SLOT( pauseButtonClicked() ) );
  }
  virtual void createCommandLine( state_e state, QWidget* parent )
  {
    // creates the commandline
    QLayout* parentLayout = parent -> layout();
    // Navigation buttons + Command Line
    QPushButton* buttonPrev = new QPushButton( tr( "<" ), parent );
    QPushButton* buttonNext = new QPushButton( tr( ">" ), parent );
    SC_CommandLine* commandLineEdit = new SC_CommandLine( parent );

    setWidget( state, BUTTON_PREV,      buttonPrev );
    setWidget( state, BUTTON_NEXT,      buttonNext );
    setWidget( state, TEXTEDIT_CMDLINE, commandLineEdit );

    buttonPrev -> setMaximumWidth( 30 );
    buttonNext -> setMaximumWidth( 30 );

    parentLayout -> addWidget( buttonPrev );
    parentLayout -> addWidget( buttonNext );
    parentLayout -> addWidget( commandLineEdit );

    connect( buttonPrev,      SIGNAL( clicked( bool ) ), this, SIGNAL(    backButtonClicked() ) );
    connect( buttonNext,      SIGNAL( clicked( bool ) ), this, SIGNAL( forwardButtonClicked() ) );
    connect( commandLineEdit, SIGNAL( switchToLeftSubTab() ), this, SIGNAL( switchToLeftSubTab() ) );
    connect( commandLineEdit, SIGNAL( switchToRightSubTab() ), this, SIGNAL( switchToRightSubTab() ) );
    connect( commandLineEdit, SIGNAL( currentlyViewedTabCloseRequest() ), this, SIGNAL( currentlyViewedTabCloseRequest() ) );
    connect( commandLineEdit, SIGNAL( returnPressed() ), this, SIGNAL( commandLineReturnPressed() ) );
    connect( commandLineEdit, SIGNAL( textEdited( const QString& ) ), this, SLOT( commandLineTextEditedSlot( const QString& ) ) );
    // Progress bar
    QProgressBar* progressBar = new QProgressBar( parent );

    setWidget( state, PROGRESSBAR_WIDGET, progressBar );

    progressBar -> setMaximum( 100 );
    progressBar -> setMaximumWidth( 200 );
    progressBar -> setMinimumWidth( 150 );

    QFont progressBarFont( progressBar -> font() );
    progressBarFont.setPointSize( 11 );
    progressBar -> setFont( progressBarFont );

    parentLayout -> addWidget( progressBar );
  }
  bool tryToHideWidget( QString* text, QWidget* widget )
  {
    // if the widget has the special text to hide it, then hide it; else show
    if ( text != nullptr )
    {
      if ( text == text_hide_widget )
      {
        widget -> hide();
        return true;
      }
      else
      {
        widget -> show();
      }
    }
    return false;
  }
  void emitSignal( QString* text )
  {
    // emit the proper signal for the given button text
    if ( text != nullptr )
    {
      if ( text == text_pause )
        emit( pauseClicked() );
      else if ( text == text_resume )
        emit( resumeClicked() );
      else if ( text == text_simulate )
      {
  #ifdef SC_PAPERDOLL
        if ( current_tab == TAB_PAPERDOLL )
        {
          emit( simulatePaperdollClicked() );
        }
        else
  #endif
        {
          emit( simulateClicked() );
        }
      }
      else if ( text == text_cancel )
      {
        switch ( current_tab )
        {
        case CMDLINE_TAB_IMPORT:
          emit( cancelImportClicked() );
          break;
        default:
          emit( cancelSimulationClicked() );
          break;
        }
      }
      else if ( text == text_cancel_all )
      {
        emit( cancelImportClicked() );
        emit( cancelAllSimulationClicked() );
      }
      else if ( text == text_import )
      {
        emit( importClicked() );
      }
      else if ( text == text_queue )
      {
        emit( queueClicked() );
      }
      else if ( text == text_save )
      {
        switch ( current_tab )
        {
        case CMDLINE_TAB_LOG:
          emit( saveLogClicked() );
          break;
        case CMDLINE_TAB_RESULTS:
          emit( saveResultsClicked() );
          break;
        default:
          assert( 0 );
        }
      }
    }
  }
  // Accessors for widgets/displayable text; hide underlying implementation of states even from init()
  void setProgressBarProgress( progressbar_states_e state, int value )
  {
    // update progress for the given progressbar state
    progressBarFormat[state].progress = value;
    updateWidget( current_state, current_tab, PROGRESSBAR_WIDGET );
  }
  int getProgressBarProgressForState( progressbar_states_e state )
  {
    return progressBarFormat[state].progress;
  }
  void updateProgressBars()
  {
    // actually updates currently visible progressbar's progress
    QProgressBar* progressBar = qobject_cast< QProgressBar* >( getWidget( current_state, PROGRESSBAR_WIDGET ) );
    if ( progressBar != nullptr )
    {
      progressBar -> setValue( getProgressBarProgressForState( getProgressBarStateForState( current_state, current_tab ) ) );
    }
  }
  void adjustText( state_e state, tabs_e tab, widgets_e widget, QString text )
  {
    // only change the text's value, not where it points to, only if its not null
    if ( states[state][tab][widget].text != nullptr )
    {
      (*states[state][tab][widget].text) = text;
    }
  }
  void setText( state_e state, tabs_e tab, widgets_e widget, QString* text, QString* tooltip = nullptr )
  {
    // change the text and tooltip's pointer values
    states[state][tab][widget].text = text;
    states[state][tab][widget].tool_tip = tooltip;
  }
  void setProgressBarState( state_e state, tabs_e tab, progressbar_states_e progressbar_state )
  {
    // set the given state->tab->progressbar's state
    states[state][tab][PROGRESSBAR_WIDGET].text = &( progressBarFormat[progressbar_state].text );
    states[state][tab][PROGRESSBAR_WIDGET].tool_tip = &( progressBarFormat[progressbar_state].tool_tip );
    states[state][tab][PROGRESSBAR_WIDGET].progressbar_state  = progressbar_state;
  }
  progressbar_states_e getProgressBarStateForState( state_e state, tabs_e tab )
  {
    // returns state->tab->progressbar's state
    return states[state][tab][PROGRESSBAR_WIDGET].progressbar_state;
  }
  void updateWidget( state_e state, tabs_e tab, widgets_e widget )
  {
    // update a given widget
    QString* text = getText( state, tab, widget );
    QString* toolTip = getToolTip( state, tab, widget );
    if ( text != nullptr )
    {
      if ( tab == current_tab )
      {
        switch ( widget )
        {
        case TEXTEDIT_CMDLINE:
        {
          SC_CommandLine* commandLine = qobject_cast< SC_CommandLine* >( getWidget( state, TEXTEDIT_CMDLINE ) );
          if ( commandLine != nullptr )
          {
            commandLine -> setText( *text );
            if ( toolTip != nullptr )
            {
              commandLine -> setToolTip( *toolTip );
            }
          }
          break;
        }
        case PROGRESSBAR_WIDGET:
        {
          QProgressBar* progressBar = qobject_cast< QProgressBar* >( getWidget( state, PROGRESSBAR_WIDGET ) );
          if ( progressBar != nullptr )
          {
            progressBar -> setFormat( *text );
            progressBar -> setValue( getProgressBarProgressForState( getProgressBarStateForState( current_state, current_tab ) ) );
            if ( toolTip != nullptr )
            {
              progressBar -> setToolTip( *toolTip );
            }
          }
          break;
        }
        default:
          QPushButton* button = qobject_cast< QPushButton* >( getWidget( state, widget ) );
          if ( button != nullptr )
          {
            button -> setText( *text );
            if ( toolTip != nullptr )
            {
              button -> setToolTip( *toolTip );
            }
            break;
          }
        }
      }
    }
  }
  QString* getText( state_e state, tabs_e tab, widgets_e widget )
  {
    // returns text to set the specified widget to
    return states[state][tab][widget].text;
  }
  QString* getToolTip( state_e state, tabs_e tab, widgets_e widget )
  {
    return states[state][tab][widget].tool_tip;
  }
  QWidget* getWidget( state_e state, widgets_e widget )
  {
    // returns the widget for the specified state
    return widgets[state][widget];
  }
  void setWidget( state_e state, widgets_e widget, QWidget* widgetPointer )
  {
    // sets the widget for the specified state
    widgets[state][widget] = widgetPointer;
  }
  void setProgressBarFormat( progressbar_states_e state, QString format, bool update = false )
  {
    // sets the QProgressBar->setFormat(format) text value for the specified state's progress bar
    progressBarFormat[state].text = format;
    if ( update &&
         getProgressBarStateForState( current_state, current_tab ) == state )
    {
      updateWidget( current_state, current_tab, PROGRESSBAR_WIDGET );
    }
  }
  void setProgressBarToolTip( progressbar_states_e state, QString toolTip, bool update = false )
  {
    progressBarFormat[state].tool_tip = toolTip;
    if ( update &&
         getProgressBarStateForState( current_state, current_tab ) == state )
    {
      updateWidget( current_state, current_tab, PROGRESSBAR_WIDGET );
    }
  }
  void updateProgress( progressbar_states_e state, int value, QString format, QString toolTip )
  {
    setProgressBarFormat( state, format );
    setProgressBarToolTip( state, toolTip );
    setProgressBarProgress( state, value );
  }
  tabs_e convertTabsEnum( main_tabs_e tab )
  {
    return static_cast< tabs_e >( tab );
  }
  tabs_e convertTabsEnum( import_tabs_e tab )
  {
    return static_cast< tabs_e >( tab + TAB_COUNT );
  }
  // End accessors
public slots:
  void mainButtonClicked()
  {
    emitSignal( getText( current_state, current_tab, BUTTON_MAIN  ) );
  }
  void pauseButtonClicked()
  {
    emitSignal( getText( current_state, current_tab, BUTTON_PAUSE ) );
  }
  void queueButtonClicked()
  {
    emitSignal( getText( current_state, current_tab, BUTTON_QUEUE ) );
  }
  void setTab( main_tabs_e tab )
  {
    setTab( convertTabsEnum( tab ) );
  }
  void setTab( import_tabs_e tab )
  {
    setTab( convertTabsEnum( tab ) );
  }
  void setTab( tabs_e tab )
  {
    assert( static_cast< int >( tab ) >= 0 &&
            static_cast< int >( tab ) < CMDLINE_TAB_COUNT &&
            "setTab argument out of bounds" );
    current_tab = tab;
    // sets correct text for the current_state for the given tab on all widgets
    for ( widgets_e widget = BUTTON_MAIN; widget < WIDGET_COUNT; widget++ )
    {
      QWidget* wdgt = getWidget( current_state, widget );
      if ( wdgt != nullptr )
      {
        if ( ! tryToHideWidget( getText( current_state, current_tab, widget ), wdgt ) )
        {
          // only change text if it will not be hidden
          updateWidget( current_state, current_tab, widget );
        }
      }
    }
  }
  void setState( state_e state )
  {
    // change to the specified state
    current_state = state;
    statesStackedLayout -> setCurrentIndex( static_cast< int >( state ) );
    setTab( current_tab );
  }
  void commandLineTextEditedSlot( const QString& text )
  {
    QString* current_text = getText( current_state, current_tab, TEXTEDIT_CMDLINE );
    if ( current_text != nullptr )
    {
      (*current_text) = text;
    }
    else
    {
      //TODO: something if current_text is null
    }

    emit( commandLineTextEdited( text ) );
  }
signals:
  void pauseClicked();
  void resumeClicked();
  void simulateClicked();
  void queueClicked();
  void importClicked();
  void saveLogClicked();
  void saveResultsClicked();
  void cancelSimulationClicked();
  void cancelImportClicked();
  void cancelAllSimulationClicked();
  void backButtonClicked();
  void forwardButtonClicked();
// SC_CommandLine signals
  void switchToLeftSubTab();
  void switchToRightSubTab();
  void currentlyViewedTabCloseRequest();
  void commandLineReturnPressed();
  void commandLineTextEdited( const QString& );
#ifdef SC_PAPERDOLL
  void simulatePaperdollClicked();
#endif
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
#if USE_CHARDEV
  PersistentCookieJar* charDevCookies;
#endif // USE_CHARDEV
  QPushButton* rawrButton;
  QByteArray rawrDialogState;
  SC_TextEdit* rawrText;
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
  bool    importRunning();
  void    startSim();
  bool    simRunning();

  sim_t*  initSim();
  void    deleteSim( sim_t* sim, SC_TextEdit* append_error_message = 0 );

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
  void mainTabChanged( int index );
  void importTabChanged( int index );
  void resultsTabChanged( int index );
  void resultsTabCloseRequest( int index );
  void rawrButtonClicked( bool checked = false );
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
#if defined( Q_WS_MAC ) || defined( Q_OS_MAC )
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
#if USE_CHARDEV
  void importCharDev();
#endif
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
