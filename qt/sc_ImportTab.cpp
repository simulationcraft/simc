// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_AddonImportTab.hpp"
#include "sc_AutomationTab.hpp"
#include "simulationcraftqt.hpp"

SC_ImportTab::SC_ImportTab( QWidget* parent )
  : SC_enumeratedTab<import_tabs_e>( parent ),
    automationTab( new SC_AutomationTab( this ) ),
    addonTab( new SC_AddonImportTab( this ) )
{
}
