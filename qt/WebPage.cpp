// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "WebPage.hpp"
#include <QDesktopServices>

SC_WebPage::SC_WebPage( QObject* parent ) : SC_WebEnginePage( parent )
{
}

bool SC_WebPage::acceptNavigationRequest( const QUrl& url, NavigationType, bool isMainFrame )
{
  if ( !isMainFrame )
  {
    return false;
  }

  QString url_to_show = url.toString();
  if ( url.isLocalFile() || url_to_show.contains( "battle.net" ) || url_to_show.contains( "battlenet" ) ||
       url_to_show.contains( "github.com" ) || url_to_show.contains( "worldofwarcraft.com" ) )
    return true;
  else
    QDesktopServices::openUrl( url_to_show );
  return false;
}
