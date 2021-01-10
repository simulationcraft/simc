// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "WebView.hpp"

#include "MainWindow.hpp"
#include "WebPage.hpp"
#include "util/sc_searchbox.hpp"

#include <QtWidgets/QShortcut>

SC_WebView::SC_WebView( SC_MainWindow* mw, QWidget* parent, const QString& h )
  : SC_WebEngineView( parent ),
    searchBox( new SC_SearchBox() ),
    previousSearch( "" ),
    allow_mouse_navigation( false ),
    allow_keyboard_navigation( false ),
    allow_searching( false ),
    mainWindow( mw ),
    progress( 0 ),
    html_str( h )
{
  searchBox->hide();
  QShortcut* ctrlF  = new QShortcut( QKeySequence( Qt::CTRL + Qt::Key_F ), this );
  QShortcut* escape = new QShortcut( QKeySequence( Qt::Key_Escape ), this );
  connect( ctrlF, SIGNAL( activated() ), searchBox, SLOT( show() ) );
  connect( escape, SIGNAL( activated() ), this, SLOT( hideSearchBox() ) );
  connect( searchBox, SIGNAL( textChanged( const QString& ) ), this, SLOT( findSomeText( const QString& ) ) );
  connect( searchBox, SIGNAL( findPrev() ), this, SLOT( findPrev() ) );
  connect( searchBox, SIGNAL( findNext() ), this, SLOT( findNext() ) );
  connect( this, SIGNAL( loadProgress( int ) ), this, SLOT( loadProgressSlot( int ) ) );
  connect( this, SIGNAL( loadFinished( bool ) ), this, SLOT( loadFinishedSlot( bool ) ) );
  connect( this, SIGNAL( urlChanged( const QUrl& ) ), this, SLOT( urlChangedSlot( const QUrl& ) ) );

  SC_WebPage* page = new SC_WebPage( this );
  setPage( page );

  // Add QT Major Version to avoid "mysterious" problems resulting in qBadAlloc.
  QDir dir( mainWindow->TmpDir + QDir::separator() + "simc_webcache_qt" +
            std::string( QT_VERSION_STR ).substr( 0, 3 ).c_str() );
  if ( !dir.exists() )
    dir.mkpath( "." );

  QFileInfo fi( dir.absolutePath() );

  if ( fi.isDir() && fi.isWritable() )
  {
    QNetworkDiskCache* diskCache = new QNetworkDiskCache( this );
    diskCache->setCacheDirectory( dir.absolutePath() );
    QString test = diskCache->cacheDirectory();
  }
  else
  {
    qWarning() << "Can't create/write webcache.";
  }
}

void SC_WebView::store_html( const QString& s )
{
  html_str = s;
}

void SC_WebView::loadHtml()
{
  setHtml( html_str );
}

void SC_WebView::enableMouseNavigation()
{
  allow_mouse_navigation = true;
}

void SC_WebView::disableMouseNavigation()
{
  allow_mouse_navigation = false;
}

void SC_WebView::enableKeyboardNavigation()
{
  allow_keyboard_navigation = true;
}

void SC_WebView::disableKeyboardNavigation()
{
  allow_keyboard_navigation = false;
}

void SC_WebView::update_progress( int p )
{
  progress = p;
  mainWindow->updateWebView( this );
}

void SC_WebView::mouseReleaseEvent( QMouseEvent* e )
{
  if ( allow_mouse_navigation )
  {
    switch ( e->button() )
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
  SC_WebEngineView::mouseReleaseEvent( e );
}

void SC_WebView::keyReleaseEvent( QKeyEvent* e )
{
  if ( allow_keyboard_navigation )
  {
    int k                   = e->key();
    Qt::KeyboardModifiers m = e->modifiers();
    switch ( k )
    {
      case Qt::Key_R:
      {
        if ( m.testFlag( Qt::ControlModifier ) )
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
        if ( m.testFlag( Qt::AltModifier ) )
          back();
      }
      break;
      case Qt::Key_Right:
      {
        if ( m.testFlag( Qt::AltModifier ) )
          forward();
      }
      break;
      default:
        break;
    }
  }
  SC_WebEngineView::keyReleaseEvent( e );
}

void SC_WebView::resizeEvent( QResizeEvent* e )
{
  searchBox->updateGeometry();
  SC_WebEngineView::resizeEvent( e );
}

void SC_WebView::focusInEvent( QFocusEvent* e )
{
  hideSearchBox();
  SC_WebEngineView::focusInEvent( e );
}

void SC_WebView::loadProgressSlot( int p )
{
  update_progress( p );
}

void SC_WebView::loadFinishedSlot( bool /* ok */ )
{
  update_progress( 100 );
}

void SC_WebView::urlChangedSlot( const QUrl& url )
{
  url_to_show = url.toString();
  if ( url_to_show == "about:blank" )
  {
    url_to_show = "results.html";
  }
  mainWindow->updateWebView( this );
}

void SC_WebView::hideSearchBox()
{
  previousSearch = "";  // disable clearing of highlighted text on next search
  searchBox->hide();
}

void SC_WebView::findSomeText( const QString& text, SC_WebEnginePage::FindFlags options )
{
  findText( text, options );

  // If the QWebView scrolls due to searching with the SC_SearchBox visible
  // The SC_SearchBox could still be visible, as the area the searchbox is covering
  // is not redrawn, just moved
  // *HACK*
  // This just repaints the area directly above the search box to the top, in a huge rectangle
  QRect searchBoxRect = searchBox->rect();
  searchBoxRect.moveTopLeft( searchBox->geometry().topLeft() );
  switch ( searchBox->getCorner() )
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

void SC_WebView::findSomeText( const QString& text )
{
  SC_WebEnginePage::FindFlags flags = 0;
  if ( searchBox->reverseSearch() )
  {
    flags |= SC_WebEnginePage::FindBackward;
  }
  findSomeText( text, flags );
}

void SC_WebView::findPrev()
{
  findSomeText( searchBox->text(), SC_WebEnginePage::FindBackward );
}

void SC_WebView::findNext()
{
  findSomeText( searchBox->text(), 0 );
}