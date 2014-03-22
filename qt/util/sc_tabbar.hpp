// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#ifdef QT_VERSION_5
#include <QtWidgets/QtWidgets>
#endif
#include <QtGui/QtGui>



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
