
// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "WebEngineConfig.hpp"

// ============================================================================
// SC_WebPage
// ============================================================================

class SC_WebPage : public SC_WebEnginePage
{
  Q_OBJECT
public:
  explicit SC_WebPage( QObject* parent = nullptr );

#if ( QT_VERSION >= QT_VERSION_CHECK( 5, 5, 0 ) )  // Functionality added to webengine in qt 5.5
  bool acceptNavigationRequest( const QUrl& url, NavigationType, bool isMainFrame ) override;
#endif // QT 5.5.0+
};
