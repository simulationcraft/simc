
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

  bool acceptNavigationRequest( const QUrl& url, NavigationType, bool isMainFrame ) override;
};
