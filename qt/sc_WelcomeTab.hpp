// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#pragma once

#include "WebEngineConfig.hpp"

class SC_MainWindow;

#if defined( SC_USE_WEBKIT )
/// Webkit Webview for Welcome.html welcome page
class SC_WelcomeTabWidget_WebKit : public SC_WebEngineView
{
  Q_OBJECT
public:
  SC_WelcomeTabWidget_WebKit( SC_MainWindow* parent );
private slots:
  void linkClickedSlot( const QUrl& url );
};
#endif  // SC_USE_WEBKIT

#if defined( SC_USE_WEBENGINE )
/// Webengine Webview for Welcome.html welcome page
class SC_WelcomeTabWidget_WebEngine : public SC_WebEngineView
{
  Q_OBJECT
public:
  SC_WelcomeTabWidget_WebEngine( SC_MainWindow* parent );
public slots:
  void welcomeLoadSlot();
private slots:
  void urlChangedSlot( const QUrl& url );

private:
  QString welcome_uri;
  QTimer* welcome_timer;
};
#endif  // SC_USE_WEBENGINE

#if defined( SC_USE_WEBKIT )
typedef SC_WelcomeTabWidget_WebKit SC_WelcomeTabWidget;
#elif defined( SC_USE_WEBENGINE )
typedef SC_WelcomeTabWidget_WebEngine SC_WelcomeTabWidget;
#else
#error "No Webkit selected"
#endif
