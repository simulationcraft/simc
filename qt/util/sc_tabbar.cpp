// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_tabbar.hpp"


// ============================================================================
// SC_TabBar
// ============================================================================


SC_TabBar::SC_TabBar( QWidget* parent,
                      bool enableDraggedHover,
                      bool enableMouseHover,
                      bool enableContextMenu ) :
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

void SC_TabBar::enableDraggedTextHover( bool enable )
{
  enableDraggedTextHoverSignal = enable;
  setMouseTracking( enableMouseHoverTimeoutSignal || enable );
  setAcceptDrops( enable );
}

void SC_TabBar::initContextMenu()
{
  delete contextMenu;
  contextMenu = nullptr;

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

void SC_TabBar::updateContextMenu()
{
  if ( enableContextMenu )
  {
    renameTabAction   -> setVisible( enableContextMenuRenameTab );
    closeTabAction    -> setVisible( enableContextMenuCloseTab );
    closeOthersAction -> setVisible( enableContextMenuCloseOthers );
    closeAllAction    -> setVisible( enableContextMenuCloseAll );
  }
}

void SC_TabBar::dragEnterEvent( QDragEnterEvent* e )
{
  if ( enableDraggedTextHoverSignal )
    e -> acceptProposedAction();

  QTabBar::dragEnterEvent( e );
}

void SC_TabBar::dragLeaveEvent( QDragLeaveEvent* e )
{
  draggedTextOnSingleTabTimer.stop();
  mouseHoverTimeoutTimer.stop();
  hoveringOverTab = -1;

  QTabBar::dragLeaveEvent( e );
}

void SC_TabBar::dragMoveEvent( QDragMoveEvent* e )
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

void SC_TabBar::dropEvent( QDropEvent* e )
{
  // dropEvent is protected so we cannot forward this to the widget.. just dont accept it

  QTabBar::dropEvent( e );
}

void SC_TabBar::mouseMoveEvent( QMouseEvent* e )
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

void SC_TabBar::leaveEvent( QEvent* e )
{
  draggedTextOnSingleTabTimer.stop();
  mouseHoverTimeoutTimer.stop();
  hoveringOverTab = -1;

  QTabBar::leaveEvent( e );
}

void SC_TabBar::enterEvent( QEvent* e )
{
  startHoverTimer( tabAt( QCursor::pos() ) );

  QTabBar::enterEvent( e );
}

void SC_TabBar::startDraggedTextTimer( int tab )
{
  if ( enableDraggedTextHoverSignal && tab >= 0 )
  {
    hoveringOverTab = tab;
    draggedTextOnSingleTabTimer.start( draggedTextTimeout );
  }
}

void SC_TabBar::startHoverTimer( int tab )
{
  if ( enableMouseHoverTimeoutSignal && tab >= 0 )
  {
    hoveringOverTab = tab;
    mouseHoverTimeoutTimer.start( mouseHoverTimeout );
  }
}

bool SC_TabBar::event( QEvent* e )
{
  if ( e -> type() == QEvent::LayoutRequest )
  {
    // Issued when drag is completed
    // Best way I can find to enforce the SC_SimulateTab addTabWidget's location
    emit( layoutRequestEvent() );
  }

  return QTabBar::event( e );
}

void SC_TabBar::mousePressEvent(QMouseEvent *e)
{
	int tabUnderMouse = tabAt(e -> pos());

	emit(layoutRequestEvent());

	switch (e -> button()) 
	{
		case Qt::MiddleButton: 
		{
			tabCloseRequested(tabUnderMouse);
		}
		break;
		case Qt::LeftButton: 
		{
			setCurrentIndex(tabUnderMouse);
		}
		break;
    default:
    break;
	}
}

void SC_TabBar::mouseHoverTimedout()
{
  mouseHoverTimeoutTimer.stop();
  emit( mouseHoveredOverTab( hoveringOverTab ) );
}

void SC_TabBar::draggedTextTimedout()
{
  draggedTextOnSingleTabTimer.stop();
  emit( mouseDragHoveredOverTab( hoveringOverTab ) );
}

void SC_TabBar::renameTab()
{
  bool ok;
  QString text = QInputDialog::getText( this, tr( "Modify Tab Title" ),
                                        tr("New Tab Title:"), QLineEdit::Normal,
                                        tabText( currentIndex() ), &ok );
  if ( ok && !text.isEmpty() )
    setTabText( currentIndex(), text );
}

void SC_TabBar::showContextMenu( const QPoint& pos )
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
