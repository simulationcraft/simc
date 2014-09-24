// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include <QtWidgets/QtWidgets>
#include <QtGui/QtGui>
#include "sc_relativepopup.hpp"
class SC_TabBar;

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
                       QString closeOthersWarningText = "Close other tabs?" );

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
  void closeAllTabs();
  void closeOtherTabs();
  void removeTab( int index );
  void enableMouseHoveredOverTabSignal( bool enable );
  void enableDragHoveredOverTabSignal( bool enable );
private:
  virtual QWidget* createCloseAllTabsWidget();
  void initTabBar();
public slots:
  void closeAllTabsRequest();
  void closeOtherTabsRequest();
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
  SC_RecentlyClosedTabItemModel( QObject* parent = nullptr );
signals:
  void removePreview( QWidget* );
  void newTabAdded( QWidget* widget, const QString& title, const QString& tooltip, const QIcon& icon );
  void restoreTab( QWidget* widget, const QString& title, const QString& tooltip, const QIcon& icon );
public slots:
  void restoreTab( const QModelIndex& index );
  void restoreAllTabs();
  void addTab( QWidget* widget, const QString& title, const QString& tooltip, const QIcon& icon);
private slots:
  void rowsAboutToBeRemovedSlot( const QModelIndex& parent, int start, int end );
  void rowsInsertedSlot( const QModelIndex& parent, int start, int end );
  void modelAboutToBeResetSlot();
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
                              SC_RecentlyClosedTabItemModel* modelToUse = nullptr );
  SC_RecentlyClosedTabItemModel* getModel()
  {
    return model;
  }
signals:
  void hideRequest();
  void showRequest();
  void restoreTab( QWidget* widget, const QString& title, const QString& tooltip, const QIcon& icon );
protected:
  void init();
  QWidget* initListView();
  QModelIndex getSelectionAndMakeSureIsValidIfElementsAvailable();
protected:
  void showEvent( QShowEvent* e )
  {
    Q_UNUSED( e );
    if ( previewAnItemOnShow )
      previewNext();
  }
public slots:
  void removePreview( QWidget* widget = nullptr );
  bool addIndexToPreview( const QModelIndex& index );
  bool previewNext( QWidget* exclude = nullptr );
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
  void addTab( QWidget* widget, const QString& title, const QString& tooltip, const QIcon& icon)
  {
    model -> addTab( widget, title, tooltip, icon );
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

class SC_HoverArea;

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
                       Qt::Corner corner = Qt::TopRightCorner );
  virtual int insertAt()
  {
    return count();
  }
  SC_RecentlyClosedTabItemModel* getModel()
  {
    return recentlyClosedTab -> getModel();
  }
public slots:
  void restoreTab( QWidget* widget, const QString& title, const QString& tooltip, const QIcon& icon );
};
