// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "EnumeratedTab.hpp"

class SC_AddonImportTab;

enum import_tabs_e : int
{
  TAB_IMPORT_NEW = 0,
  TAB_ADDON,
  TAB_BIS,
  TAB_RECENT,
  TAB_CUSTOM
};

class SC_ImportTab : public SC_enumeratedTab<import_tabs_e>
{
  Q_OBJECT
public:
  SC_ImportTab( QWidget* parent = nullptr );
  SC_AddonImportTab* addonTab;
};
