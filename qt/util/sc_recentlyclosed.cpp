// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_recentlyclosed.hpp"
#include "sc_tabbar.hpp"
#include <cassert>
#include "sc_hoverarea.hpp"

// ============================================================================
// SC_TabWidgetCloseAll
// ============================================================================

SC_TabWidgetCloseAll::SC_TabWidgetCloseAll(QWidget* parent,
                                           Qt::Corner corner,
                                           QString closeAllWarningTitle,
                                           QString closeAllWarningText,
                                           QString closeOthersWarningTitle,
                                           QString closeOthersWarningText ) :
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

void SC_TabWidgetCloseAll::enableMouseHoveredOverTabSignal( bool enable )
{
  scTabBar -> enableMouseHoverTimeout( enable );
}

void SC_TabWidgetCloseAll::enableDragHoveredOverTabSignal( bool enable )
{
  scTabBar -> enableDraggedTextHover( enable );
}

QWidget* SC_TabWidgetCloseAll::createCloseAllTabsWidget()
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

void SC_TabWidgetCloseAll::initTabBar()
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

void SC_TabWidgetCloseAll::closeAllTabs()
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

void SC_TabWidgetCloseAll::closeOtherTabs()
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

void SC_TabWidgetCloseAll::removeTab( int index )
{
  QWidget* widgetAtIndex = widget( index );
  if ( widgetAtIndex != nullptr )
  {
    // index could be out of bounds, only emit signal if there is a legit widget at the index
    emit( tabAboutToBeRemoved( widgetAtIndex, tabText( index ), tabToolTip( index ), tabIcon( index ) ) );
  }
}

void SC_TabWidgetCloseAll::closeAllTabsRequest()
{
  int confirm = QMessageBox::warning( this, closeAllTabsTitleText, closeAllTabsBodyText, QMessageBox::Yes | QMessageBox::No, QMessageBox::No );
  if ( confirm == QMessageBox::Yes )
    closeAllTabs();
}

void SC_TabWidgetCloseAll::closeOtherTabsRequest()
{
  int confirm = QMessageBox::warning( this, closeOtherTabsTitleText, closeOtherTabsBodyText, QMessageBox::Yes | QMessageBox::No, QMessageBox::No );
  if ( confirm == QMessageBox::Yes )
    closeOtherTabs();
}



// ============================================================================
// SC_RecentlyClosedTabItemModel
// ============================================================================

SC_RecentlyClosedTabItemModel::SC_RecentlyClosedTabItemModel( QObject* parent ) :
  QStandardItemModel( parent )
{
  connect( this, SIGNAL( rowsAboutToBeRemoved( const QModelIndex&, int, int ) ), this, SLOT( rowsAboutToBeRemovedSlot( const QModelIndex&, int, int ) ) );
  connect( this, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT( rowsInsertedSlot( const QModelIndex&, int, int ) ) );
  connect( this, SIGNAL( modelAboutToBeReset() ), this, SLOT( modelAboutToBeResetSlot() ) );
}

void SC_RecentlyClosedTabItemModel::restoreTab( const QModelIndex& index )
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

void SC_RecentlyClosedTabItemModel::restoreAllTabs()
{
  int totalRowsToRestore = rowCount();
  for( int i = 0; i < totalRowsToRestore; i++ )
  {
    restoreTab( index( 0, 0 ) );
  }
}

void SC_RecentlyClosedTabItemModel::addTab( QWidget* widget, const QString& title, const QString& tooltip, const QIcon& icon)
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

void SC_RecentlyClosedTabItemModel::rowsAboutToBeRemovedSlot( const QModelIndex& parent, int start, int end )
{
  Q_UNUSED( parent );
  for ( int row = start; row < end; row++ )
  {
    QModelIndex sibling = index( row, 0 );
    QWidget* widget = sibling.data( Qt::UserRole ).value< QWidget* >();
    QWidget* par = sibling.data( Qt::UserRole + 1 ).value< QWidget* >();
    emit( removePreview( widget ) );
    Q_ASSERT( par != nullptr );
    delete par;

    setData( sibling, QVariant::fromValue< QWidget* >( nullptr ), Qt::UserRole );
    setData( sibling, QVariant::fromValue< QWidget* >( nullptr ), Qt::UserRole + 1 );
  }
}

void SC_RecentlyClosedTabItemModel::rowsInsertedSlot( const QModelIndex& parent, int start, int end )
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

void SC_RecentlyClosedTabItemModel::modelAboutToBeResetSlot()
{
  rowsAboutToBeRemovedSlot( QModelIndex(), 0, rowCount() );
}



// ============================================================================
// SC_RecentlyClosedTabWidget
// ============================================================================

SC_RecentlyClosedTabWidget::SC_RecentlyClosedTabWidget( QWidget* parent,
                                                        QBoxLayout::Direction grow,
                                                        SC_RecentlyClosedTabItemModel* modelToUse ) :
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

void SC_RecentlyClosedTabWidget::init()
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

QWidget* SC_RecentlyClosedTabWidget::initListView()
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

QModelIndex SC_RecentlyClosedTabWidget::getSelectionAndMakeSureIsValidIfElementsAvailable()
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

void SC_RecentlyClosedTabWidget::removePreview( QWidget* widget )
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

bool SC_RecentlyClosedTabWidget::addIndexToPreview( const QModelIndex& index )
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

bool SC_RecentlyClosedTabWidget::previewNext( QWidget* exclude )
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


// ============================================================================
// SC_RecentlyClosedTab
// ============================================================================

SC_RecentlyClosedTab::SC_RecentlyClosedTab(QWidget* parent,
                                           SC_RecentlyClosedTabItemModel* modelToUse,
                                           bool enableRecentlyClosedTabs,
                                           Qt::Corner corner ) :
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

void SC_RecentlyClosedTab::restoreTab( QWidget* widget, const QString& title, const QString& tooltip, const QIcon& icon )
{
  int indexToInsert = insertAt();
  widget -> setParent( this );
  indexToInsert = insertTab( indexToInsert, widget, title );
  setTabToolTip( indexToInsert, tooltip );
  setTabIcon( indexToInsert, icon );
  setCurrentIndex( indexToInsert );
}
