// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#pragma once

#include "WebEngineConfig.hpp"

class SC_MainWindow;

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


using SC_WelcomeTabWidget = SC_WelcomeTabWidget_WebEngine;
