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

/**
 * Additional translation mappings for enchant names. We use shorthands for
 * certain enchants (listed here), that cannot be deduced from the item
 * enchantment name in DBC. We can add additional shortcuts here, where
 * applicable.
 */
static const enchant_db_item_t __enchant_db[] = {
  /* Engineering tinkers */
  { "synapse_springs_2",       4898 },

  /* Tailoring cloak enchants */
  { "lightweave_1",            3722 },
  { "lightweave_2",            4115 },
  { "lightweave_3",            4892 },
  { 0,                         0    }
};

} /* ANONYMOUS NAMESPACE */

unsigned enchant::find_enchant_id( const std::string& name )
{
  for ( size_t i = 0; i < sizeof_array( __enchant_db ) - 1; i++ )
  {
    if ( util::str_compare_ci( __enchant_db[ i ].enchant_name, name ) )
      return __enchant_db[ i ].enchant_id;
  }

  return 0;
}

std::string enchant::find_enchant_name( unsigned enchant_id )
{
  for ( size_t i = 0; i < sizeof_array( __enchant_db ) - 1; i++ )
  {
    if ( __enchant_db[ i ].enchant_id == enchant_id )
      return __enchant_db[ i ].enchant_name;
  }

  return std::string();
}

/**
 * Deduce enchant name from DBC enchant data.
 *
 * The function loops through all item enchantments in the exported game client
 * data and checks if the item enchant name (and possibly rank) match the user
 * input enchant name. The function matches un-ranked enchants (for example
 * Dancing Steel), and ranked enchants in two ways. First, the full name
 * including the full rank string is checked (i.e.,
 * lightweave_embroidery_rank_2). If this is not a match, the function
 * additionally checks against the full name, where "rank_" is removed. Returns
 * an invalid item enchantment (with id of 0) if nothing is found.
 */
const item_enchantment_data_t& enchant::find_item_enchant( const dbc_t& dbc, const std::string& name )
{
  // Check additional mapping table first
  if ( find_enchant_id( name ) > 0 )
    return dbc.item_enchantment( find_enchant_id( name ) );

  for ( const item_enchantment_data_t* item_enchant = dbc.item_enchantments();
        item_enchant -> id != 0;
        item_enchant++ )
  {
    if ( item_enchant -> name == 0 )
      continue;

    std::string enchant_name = item_enchant -> name;
    util::tokenize( enchant_name );
    // Check for partial match in the name, enchant name must be fully
    // contained in the given parameter
    if ( ! util::str_in_str_ci( name, enchant_name ) )
      continue;

    // Partial match found, see if the spell has a rank string, and massage
    // name to account for it.
    for ( size_t i = 0; i < sizeof_array( item_enchant -> ench_prop ); i++ )
    {
      if ( item_enchant -> ench_prop[ i ] == 0 ||
           item_enchant -> ench_type[ i ] == 0 )
        continue;

      std::string rank_str;
      if ( dbc.spell( item_enchant -> ench_prop[ i ] ) -> rank_str() )
        rank_str = dbc.spell( item_enchant -> ench_prop[ i ] ) -> rank_str();
      util::tokenize( rank_str );

      // Compare to exact enchant name match, if rank string is missing
      if ( util::str_compare_ci( name, enchant_name ) )
        return *item_enchant;

      // Compare directly to enchant name + rank
      if ( util::str_compare_ci( name, enchant_name + "_" + rank_str ) )
        return *item_enchant;

      std::string::size_type offset = rank_str.find( "rank_" );
      if ( offset != std::string::npos )
      {
        // Erase "rank_", and try again
        rank_str.erase( offset, 5 );

        // Match <enchant name>_<enchant rank>
        if ( util::str_compare_ci( name, enchant_name + "_" + rank_str ) )
          return *item_enchant;
      }
    }
  }

  return dbc.item_enchantment( 0 );
}

