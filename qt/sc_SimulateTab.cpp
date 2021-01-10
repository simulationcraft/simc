// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#include "sc_SimulateTab.hpp"

#include "util/sc_textedit.hpp"

#include <cassert>

SC_SimulateTab::SC_SimulateTab( QWidget* parent, SC_RecentlyClosedTabItemModel* modelToUse )
  : SC_RecentlyClosedTab( parent, modelToUse ), addTabWidget( new QWidget( this ) ), lastSimulateTabIndexOffset( -2 )
{
  setTabsClosable( true );

  setCloseAllTabsTitleText( tr( "Close ALL Simulate Tabs?" ) );
  setCloseAllTabsBodyText( tr( "Do you really want to close ALL simulation profiles?" ) );
  QIcon addTabIcon( ":/icon/addtab.png" );
  int i = addTab( addTabWidget, addTabIcon, addTabIcon.pixmap( QSize( 64, 64 ) ).isNull() ? "+" : "" );
  tabBar()->setTabButton( i, QTabBar::LeftSide, nullptr );
  tabBar()->setTabButton( i, QTabBar::RightSide, nullptr );
  addCloseAllExclude( addTabWidget );

  enableDragHoveredOverTabSignal( true );
  connect( this, SIGNAL( mouseDragHoveredOverTab( int ) ), this, SLOT( mouseDragSwitchTab( int ) ) );
  connect( this, SIGNAL( currentChanged( int ) ), this, SLOT( addNewTab( int ) ) );
  connect( this, SIGNAL( tabCloseRequested( int ) ), this, SLOT( TabCloseRequest( int ) ) );
  // Using QTabBar::tabMoved(int,int) signal turns out to be very wonky
  // It is emitted WHILE tabs are still being dragged and looks very funny
  // This is the best way I could find to enable this
  setMovable(
      true );  // Would need to disallow moving the + tab, or to the right of it. That would require subclassing tabbar
  connect( this, SIGNAL( tabBarLayoutRequestEvent() ), this, SLOT( enforceAddTabWidgetLocationInvariant() ) );
}

bool SC_SimulateTab::is_Default_New_Tab( int index )
{
  bool retval = false;
  if ( index >= 0 && index < count() && !isACloseAllExclude( widget( index ) ) )
    retval = ( tabText( index ) == defaultSimulateTabTitle &&
               static_cast<SC_TextEdit*>( widget( index ) )->toPlainText() == defaultSimulateText );
  return retval;
}

bool SC_SimulateTab::contains_Only_Default_Profiles()
{
  for ( int i = 0; i < count(); ++i )
  {
    QWidget* widget_ = widget( i );
    if ( !isACloseAllExclude( widget_ ) )
    {
      if ( !is_Default_New_Tab( i ) )
        return false;
    }
  }
  return true;
}

int SC_SimulateTab::add_Text( const QString& text, const QString& tab_name )
{
  SC_TextEdit* s = new SC_TextEdit();
  s->setPlainText( text );

  int indextoInsert = indexOf( addTabWidget );
  int i             = insertTab( indextoInsert, s, tab_name );
  setCurrentIndex( i );
  return i;
}

SC_TextEdit* SC_SimulateTab::current_Text()
{
  return static_cast<SC_TextEdit*>( currentWidget() );
}

void SC_SimulateTab::set_Text( const QString& text )
{
  SC_TextEdit* current_s = static_cast<SC_TextEdit*>( currentWidget() );
  current_s->setPlainText( text );
}

void SC_SimulateTab::append_Text( const QString& text )
{
  SC_TextEdit* current_s = static_cast<SC_TextEdit*>( currentWidget() );
  current_s->appendPlainText( text );
}

void SC_SimulateTab::close_All_Tabs()
{
  QList<QWidget*> specialTabsList;
  specialTabsList.append( addTabWidget );
  // add more special widgets to never delete here
  int totalTabsToDelete = count();
  for ( int i = 0; i < totalTabsToDelete; ++i )
  {
    bool deleteIndexOk = false;
    int indexToDelete  = 0;
    // look for a tab not in the specialTabsList to delete
    while ( !deleteIndexOk && indexToDelete < count() )
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

int SC_SimulateTab::insertAt()
{
  return indexOf( addTabWidget );
}

bool SC_SimulateTab::wrapToEnd()
{
  if ( currentIndex() == 0 )
  {
    setCurrentIndex( count() + lastSimulateTabIndexOffset );
    return true;
  }
  return false;
}

bool SC_SimulateTab::wrapToBeginning()
{
  if ( currentIndex() == count() + lastSimulateTabIndexOffset )
  {
    setCurrentIndex( 0 );
    return true;
  }
  return false;
}

void SC_SimulateTab::insertNewTabAt( int index, const QString& text, const QString& title )
{
  if ( index < 0 || index > count() )
    index = currentIndex();
  SC_TextEdit* s = new SC_TextEdit();
  s->setPlainText( text );
  insertTab( index, s, title );
  setCurrentIndex( index );
}

void SC_SimulateTab::keyPressEvent( QKeyEvent* e )
{
  int k                   = e->key();
  Qt::KeyboardModifiers m = e->modifiers();

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

void SC_SimulateTab::keyReleaseEvent( QKeyEvent* e )
{
  int k                   = e->key();
  Qt::KeyboardModifiers m = e->modifiers();
  switch ( k )
  {
    case Qt::Key_W:
    {
      if ( m.testFlag( Qt::ControlModifier ) )
      {
        TabCloseRequest( currentIndex() );
        return;
      }
    }
    break;
    case Qt::Key_T:
    {
      if ( m.testFlag( Qt::ControlModifier ) )
      {
        insertNewTabAt( currentIndex() + 1 );
        return;
      }
    }
    break;
    default:
      break;
  }
  SC_RecentlyClosedTab::keyReleaseEvent( e );
}

void SC_SimulateTab::showEvent( QShowEvent* e )
{
  QWidget* currentWidget = widget( currentIndex() );
  if ( currentWidget != nullptr )
  {
    if ( !currentWidget->hasFocus() )
    {
      currentWidget->setFocus();
    }
  }

  SC_RecentlyClosedTab::showEvent( e );
}

void SC_SimulateTab::TabCloseRequest( int index )
{
  if ( count() > 2 && index != ( count() - 1 ) )
  {
    int confirm;

    if ( is_Default_New_Tab( index ) )
    {
      confirm = QMessageBox::Yes;
    }
    else
    {
      confirm = QMessageBox::question( this, tr( "Close Simulate Tab" ),
                                       tr( "Do you really want to close this simulation profile?" ),
                                       QMessageBox::Yes | QMessageBox::No, QMessageBox::Yes );
    }
    if ( confirm == QMessageBox::Yes )
    {
      if ( index == currentIndex() && widget( currentIndex() + 1 ) == addTabWidget )
      {
        // Change index only if removing causes a new tab to be created
        setCurrentIndex( qMax<int>( 0, currentIndex() - 1 ) );
      }
      removeTab( index );
      widget( currentIndex() )->setFocus();
    }
  }
}

void SC_SimulateTab::addNewTab( int index )
{
  if ( index == indexOf( addTabWidget ) )
  {
    insertNewTabAt( index );
  }
}

void SC_SimulateTab::mouseDragSwitchTab( int tab )
{
  if ( tab != indexOf( addTabWidget ) )
  {
    setCurrentIndex( tab );
  }
}

void SC_SimulateTab::enforceAddTabWidgetLocationInvariant()
{
  int addTabWidgetIndex = indexOf( addTabWidget );
  if ( count() >= 1 )
  {
    if ( addTabWidgetIndex != count() - 1 )
    {
      tabBar()->moveTab( addTabWidgetIndex, count() - 1 );
    }
  }
}
