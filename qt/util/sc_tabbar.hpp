// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"

#include <QtWidgets/QtWidgets>
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
             bool enableContextMenu = true );
  void enableDraggedTextHover( bool enable );
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
  virtual void initContextMenu();
  void updateContextMenu();
protected:
  virtual void dragEnterEvent( QDragEnterEvent* e ) override;
  virtual void dragLeaveEvent( QDragLeaveEvent* e ) override;
  virtual void dragMoveEvent( QDragMoveEvent* e ) override;
  virtual void dropEvent( QDropEvent* e ) override;
  virtual void mouseMoveEvent( QMouseEvent* e ) override;
  virtual void leaveEvent( QEvent* e ) override;
  virtual void enterEvent( QEvent* e ) override;
  virtual void startDraggedTextTimer( int tab );
  virtual void startHoverTimer( int tab );
  virtual bool event( QEvent* e ) override;
public slots:
  void mouseHoverTimedout();
  void draggedTextTimedout();
  void renameTab();
  void showContextMenu( const QPoint& pos );
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

signals:
  void mouseHoveredOverTab( int tab );
  void mouseDragHoveredOverTab( int tab );
  void closeOthers();
  void closeAll();
  void layoutRequestEvent();
};
