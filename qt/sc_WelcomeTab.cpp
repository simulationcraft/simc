// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_WelcomeTab.hpp"

#include "MainWindow.hpp"
#include "simulationcraftqt.hpp"

#include <QTimer>

SC_WelcomeTabWidget_WebEngine::SC_WelcomeTabWidget_WebEngine( SC_MainWindow* parent )
  : SC_WebEngineView( parent ), welcome_uri(), welcome_timer( new QTimer( this ) )
{
  QString welcomeFile( "" );
  for ( const auto& path : SC_PATHS::getDataPaths() )
  {
    QFile welcome_path( path + "/Welcome.html" );
    if ( welcome_path.exists() )
    {
      welcomeFile = welcome_path.fileName();
      break;
    }
  }
  welcome_uri = "file:///" + welcomeFile;
  qDebug() << "welcome_uri: " << welcome_uri << "\n";

  welcome_timer->setSingleShot( true );
  welcome_timer->setInterval( 500 );

  connect( welcome_timer, SIGNAL( timeout() ), this, SLOT( welcomeLoadSlot() ) );
  welcome_timer->start();

  connect( this, SIGNAL( urlChanged( const QUrl& ) ), this, SLOT( urlChangedSlot( const QUrl& ) ) );
}

void SC_WelcomeTabWidget::welcomeLoadSlot()
{
  setUrl( welcome_uri );
  welcome_timer->deleteLater();
}

void SC_WelcomeTabWidget::urlChangedSlot( const QUrl& url )
{
  if ( url.isLocalFile() )
  {
    return;
  }

  page()->triggerAction( QWebEnginePage::Stop );
  QDesktopServices::openUrl( url );
  page()->triggerAction( QWebEnginePage::Back );
}