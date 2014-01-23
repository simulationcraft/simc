// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

// TODO: Phase Fingers, Frag Belt autodetection needs to work
// TODO: Automatic suffixing of main- and off-hand weapon enchants
// TODO: Check if Power Torrent, Jade Spirit share buffs, or allow separate

#include "simulationcraft.hpp"

using namespace enchant;

namespace /* ANONYMOUS NAMESPACE */
{
/*
  else if ( name == "darkglow_1"                          ) e = "OnSpellCast_250Spi_35%_15Dur_60Cd";
  else if ( name == "darkglow_embroidery_1"               ) e = "OnSpellCast_250Spi_35%_15Dur_60Cd";
  else if ( name == "darkglow_2"                          ) e = "OnSpellCast_580Spi_30%_15Dur_45Cd";
  else if ( name == "darkglow_embroidery_2"               ) e = "OnSpellCast_580Spi_30%_15Dur_45Cd";
  else if ( name == "darkglow_3"                          ) e = "OnSpellCast_3000Spi_25%_15Dur_57Cd";
  else if ( name == "darkglow_embroidery_3"               ) e = "OnSpellCast_3000Spi_25%_15Dur_57Cd";
  else if ( name == "swordguard_1"                        ) e = "OnAttackHit_400AP_20%_15Dur_60Cd";
  else if ( name == "swordguard_embroidery_1"             ) e = "OnAttackHit_400AP_20%_15Dur_60Cd";
  else if ( name == "swordguard_2"                        ) e = "OnAttackHit_1000AP_15%_15Dur_55Cd";
  else if ( name == "swordguard_embroidery_2"             ) e = "OnAttackHit_1000AP_15%_15Dur_55Cd";
  else if ( name == "swordguard_3"                        ) e = "OnAttackHit_4000AP_15%_15Dur_57Cd";
  else if ( name == "swordguard_embroidery_3"             ) e = "OnAttackHit_4000AP_15%_15Dur_57Cd";
  else if ( name == "flintlockes_woodchucker"             ) e = "OnAttackHit_1100Physical_300Agi_10%_10Dur_40Cd_nocrit"; // TO-DO: Confirm ICD.
*/

static const enchant_db_item_t __enchant_db[] = {
  /* Enchants */
  /* Mists of Pandaria */
  { "dancing_steel",           4444 },
  { "bloody_dancing_steel",    5125 },
  { "jade_spirit",             4442 },
  { "windsong",                4441 },
  { "elemental_force",         4443 },

  /* Engineering tinkers */
  { "synapse_springs",         4179 },
  { "synapse_springs_mark_ii", 4898 },
  { "synapse_springs_2",       4898 },

  /* Tailoring cloak enchants */
  { "lightweave_1",            3722 },
  { "lightweave_embroidery_1", 3722 },
  { "lightweave_2",            4115 },
  { "lightweave_embroidery_2", 4115 },
  { "lightweave_3",            4892 },
  { "lightweave_embroidery_3", 4892 },

  /* Gems */

  { 0,                  0             }
};

} /* ANONYMOUS NAMESPACE */

unsigned enchant::find_enchant_id( const std::string& name )
{
  for ( size_t i = 0; i < sizeof_array( __enchant_db ); i++ )
  {
    if ( util::str_compare_ci( __enchant_db[ i ].enchant_name, name ) )
      return __enchant_db[ i ].enchant_id;
  }

  return 0;
}

std::string enchant::find_enchant_name( unsigned enchant_id )
{
  for ( size_t i = 0; i < sizeof_array( __enchant_db ); i++ )
  {
    if ( __enchant_db[ i ].enchant_id == enchant_id )
      return __enchant_db[ i ].enchant_name;
  }

  return std::string();
}

