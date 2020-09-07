// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "WebPage.hpp"

SC_WebPage::SC_WebPage( QObject* parent ) : SC_WebEnginePage( parent )
{
}

#if defined( SC_USE_WEBKIT )
QString SC_WebPage::userAgentForUrl( const QUrl& /* url */ ) const
{
  return QString( "simulationcraft_gui" );
}

virtual bool SC_WebPage::supportsExtension( Extension extension ) const
{
  return extension == QWebPage::ErrorPageExtension;
}
virtual bool SC_WebPage::extension( Extension extension, const ExtensionOption* option = 0,
                                    ExtensionReturn* output = 0 )
{
  if ( extension != SC_WebPage::ErrorPageExtension )
  {
    return false;
  }

  const ErrorPageExtensionOption* errorOption = static_cast<const ErrorPageExtensionOption*>( option );

  QString domain;
  switch ( errorOption->domain )
  {
    case SC_WebEnginePage::QtNetwork:
      domain = tr( "Network Error" );
      break;
    case SC_WebEnginePage::WebKit:
      domain = tr( "WebKit Error" );
      break;
    case SC_WebEnginePage::Http:
      domain = tr( "HTTP Error" );
      break;
    default:
      domain = tr( "Unknown Error" );
      break;
  }

  QString html;
  QString errorHtmlFile = QDir::currentPath() + "/Error.html";
#if defined( Q_OS_MAC )
  CFURLRef fileRef = CFBundleCopyResourceURL( CFBundleGetMainBundle(), CFSTR( "Error" ), CFSTR( "html" ), 0 );
  if ( fileRef )
  {
    CFStringRef macPath = CFURLCopyFileSystemPath( fileRef, kCFURLPOSIXPathStyle );
    errorHtmlFile       = CFStringGetCStringPtr( macPath, CFStringGetSystemEncoding() );

    CFRelease( fileRef );
    CFRelease( macPath );
  }
#endif
  QFile errorHtml( errorHtmlFile );
  if ( errorHtml.open( QIODevice::ReadOnly | QIODevice::Text ) )
  {
    html = QString::fromUtf8( errorHtml.readAll() );
    errorHtml.close();
  }
  else
  {
    // VERY simple error page if we fail to load detailed error page
    html =
        "<html><head><title>Error</title></head><body><p>Failed to load <a "
        "href=\"SITE_URL\">SITE_URL</a></p><p>ERROR_DOMAIN ERROR_NUMBER: ERROR_STRING</p></body></html>";
  }

  html = html.replace( "ERROR_NUMBER", QString::number( errorOption->error ) );
  html = html.replace( "ERROR_DOMAIN", domain );
  html = html.replace( "ERROR_STRING", errorOption->errorString );
  html = html.replace( "SITE_URL", errorOption->url.toString() );

  ErrorPageExtensionReturn* errorReturn = static_cast<ErrorPageExtensionReturn*>( output );
  errorReturn->content                  = html.toUtf8();
  errorReturn->contentType              = "text/html";
  errorReturn->baseUrl                  = errorOption->url;
  return true;
}
#elif defined( SC_USE_WEBENGINE ) && \
    ( QT_VERSION >= QT_VERSION_CHECK( 5, 5, 0 ) )  // Functionality added to webengine in qt 5.5
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
#endif                                             /* SC_USE_WEBKIT */