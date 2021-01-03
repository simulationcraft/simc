// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#ifndef SIMULATIONCRAFTQT_H
#define SIMULATIONCRAFTQT_H

#include <QtCore/QTranslator>
#include <QtGui/QtGui>
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
struct player_t;
struct sim_t;

#include "EnumeratedTab.hpp"
#include "sc_importWindow.hpp"
#include "util/generic.hpp"
#include "util/sc_recentlyclosed.hpp"  // remove once implementations are moved to source files
#include "util/sc_searchbox.hpp"       // remove once implementations are moved to source files
#include "util/sc_textedit.hpp"        // remove once implementations are moved to source files
#include "util/string_view.hpp"

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

class SC_WebView;
// class SC_SpellQueryTab;
class SC_CommandLine;
class SC_SimulateThread;
class SC_AddonImportTab;
class SC_ImportThread;

inline QString webEngineName()
{
  return "WebEngine";
}

struct SC_PATHS
{
  static QStringList getDataPaths();
};

// ============================================================================
// SC_ReforgeButtonGroup
// ============================================================================

class SC_ReforgeButtonGroup : public QButtonGroup
{
  Q_OBJECT
public:
  SC_ReforgeButtonGroup( QObject* parent = nullptr );

private:
  int selected;

private slots:
  void setSelected( int id, bool checked );
};

// ============================================================================
// SC_QueueItemModel
// ============================================================================

class SC_QueueItemModel : public QStandardItemModel
{
  Q_OBJECT
public:
  SC_QueueItemModel( QObject* parent = nullptr ) : QStandardItemModel( parent )
  {
    connect( this, SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ), this,
             SLOT( rowsRemovedSlot( const QModelIndex&, int, int ) ) );
    connect( this, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this,
             SLOT( rowsInsertedSlot( const QModelIndex&, int, int ) ) );
  }
  void enqueue( const QString& title, const QString& options, const QString& fullOptions )
  {
    QStandardItem* item = new QStandardItem;
    item->setData( QVariant::fromValue<QString>( title ), Qt::DisplayRole );
    item->setData( QVariant::fromValue<QString>( options ), Qt::UserRole );
    item->setData( QVariant::fromValue<QString>( fullOptions ), Qt::UserRole + 1 );

    appendRow( item );
  }
  std::tuple<QString, QString> dequeue()
  {
    QModelIndex indx = index( 0, 0 );
    std::tuple<QString, QString> retval;

    if ( indx.isValid() )
    {
      retval = std::make_tuple( indx.data( Qt::DisplayRole ).toString(), indx.data( Qt::UserRole + 1 ).toString() );
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
    int rowsAdded = ( end - start ) + 1;  // if only one is added, end - start = 0
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
  SC_QueueListView( SC_QueueItemModel* model, int minimumItemsToShowWidget = 1, QWidget* parent = nullptr )
    : QWidget( parent ), listView( 0 ), model( model ), minimumItemsToShow( minimumItemsToShowWidget )
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
    widgetLayout->addWidget( initListView() );

    QWidget* buttons       = new QWidget( this );
    QPushButton* buttonOne = new QPushButton( tr( "test" ) );

    QHBoxLayout* buttonsLayout = new QHBoxLayout;
    buttonsLayout->addWidget( buttonOne );

    buttons->setLayout( buttonsLayout );

    widgetLayout->addWidget( buttons );
    setLayout( widgetLayout );

    tryHide();
  }
  QListView* initListView()
  {
    delete listView;
    listView = new QListView( this );

    listView->setEditTriggers( QAbstractItemView::NoEditTriggers );
    listView->setSelectionMode( QListView::SingleSelection );
    listView->setModel( model );

    if ( model != 0 )
    {
      connect( model, SIGNAL( rowsInserted( const QModelIndex&, int, int ) ), this, SLOT( rowsInserted() ) );
      connect( model, SIGNAL( rowsRemoved( const QModelIndex&, int, int ) ), this, SLOT( rowsRemoved() ) );
    }

    QAction* remove = new QAction( tr( "Remove" ), listView );
    listView->addAction( remove );
    listView->setContextMenuPolicy( Qt::ActionsContextMenu );

    connect( remove, SIGNAL( triggered( bool ) ), this, SLOT( removeCurrentItem() ) );

    return listView;
  }
  void tryHide()
  {
    if ( model != 0 )  // gcc gave a weird error when making this one if
    {
      if ( model->rowCount() < minimumItemsToShow && minimumItemsToShow >= 0 )
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
      if ( model->rowCount() >= minimumItemsToShow )
      {
        if ( !isVisible() )
        {
          show();
        }
      }
    }
  }
private slots:
  void removeCurrentItem()
  {
    QItemSelectionModel* selection = listView->selectionModel();
    QModelIndex index              = selection->currentIndex();
    model->removeRow( index.row(), selection->currentIndex().parent() );
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
  SC_MainTab( QWidget* parent = nullptr ) : SC_enumeratedTab<main_tabs_e>( parent )
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
      int confirm =
          QMessageBox::question( this, tr( "Close Results Tab" ), tr( "Do you really want to close these results?" ),
                                 QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );
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
  SC_SingleResultTab( SC_MainWindow* mw, QWidget* parent = nullptr )
    : SC_enumeratedTab<result_tabs_e>( parent ), mainWindow( mw )
  {
    connect( this, SIGNAL( currentChanged( int ) ), this, SLOT( TabChanged( int ) ) );
  }

  void save_result();

public slots:
  void TabChanged( int index );
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
  QString region, realm, character, spec;  // New import uses these

  void importWidget();

  void start( std::shared_ptr<sim_t> s, int t, const QString& u, const QString& sources, const QString& spec,
              const QString& role, const QString& apikey )
  {
    sim             = s;
    tab             = t;
    url             = u;
    profile         = "";
    item_db_sources = sources;
    player          = 0;
    active_spec     = spec;
    m_role = role, api = apikey;
    error = "";
    QThread::start();
  }
  void start( std::shared_ptr<sim_t> s, const QString&, const QString&, const QString&, const QString& );
  void run() override;
  SC_ImportThread( SC_MainWindow* mw ) : mainWindow( mw ), sim( 0 ), tab( 0 ), player( 0 )
  {
  }
};

class SC_OverridesTab : public SC_TextEdit
{
public:
  SC_OverridesTab( QWidget* parent ) : SC_TextEdit( parent )
  {
    setPlainText( tr( "# User-specified persistent global and player parameters will be set here.\n" ) );
  }
};

namespace util
{
// explicit conversion of string_view to QString
inline QString to_QString( string_view sv )
{
  return QString::fromUtf8( sv.data(), as<int>( sv.size() ) );
}

}  // namespace util

#endif  // SIMULATIONCRAFTQT_H
