// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace   // UNNAMED NAMESPACE ==========================================
{

std::size_t encode_item_enchant_stats( const item_enchantment_data_t& enchantment, std::vector<stat_pair_t>& stats )
{
  assert( enchantment.id );

  for ( int i = 0; i < 3; i++ )
  {
    stat_pair_t s = item_database::item_enchantment_effect_stats( enchantment, i );
    if ( s.stat != STAT_NONE )
      stats.push_back( s );
  }

  return stats.size();
}

}  // UNNAMED NAMESPACE ====================================================

stat_pair_t item_database::item_enchantment_effect_stats( const item_enchantment_data_t& enchantment, int index )
{
  assert( enchantment.id );
  assert( index >= 0 && index < 3 );

  if ( enchantment.ench_type[ index ] == ITEM_ENCHANTMENT_STAT )
    return stat_pair_t( util::translate_item_mod( enchantment.ench_prop[ index ] ), enchantment.ench_amount[ index ] );

  return stat_pair_t();
}


std::string item_database::stat_to_str( int stat, int stat_amount )
{
  std::string stat_str;

  if ( stat_amount )
  {
    stat_e s = util::translate_item_mod( stat );
    if ( s != STAT_NONE )
    {
      char stat_buf[64];
      snprintf( stat_buf, sizeof( stat_buf ), "%d%s", stat_amount, util::stat_type_abbrev( s ) );
      stat_str = stat_buf;

      util::tokenize( stat_str );
    }
  }

  return stat_str;
}

// item_database_t::approx_scale_coefficient ==================================

double item_database::approx_scale_coefficient( unsigned current_ilevel, unsigned new_ilevel )
{
  if ( current_ilevel == 0 || new_ilevel == 0 )
    return 1.0;

  int diff = current_ilevel - new_ilevel;
  return 1.0 / pow( 1.15, diff / 15.0 );
}

// item_database_t::scaled_stat ===============================================

int item_database::scaled_stat( const item_data_t& item, const dbc_t& dbc, size_t idx, unsigned new_ilevel )
{
  // Safeguard against array overflow, should never happen in any case
  if ( idx >= sizeof_array( item.stat_val ) - 1 )
    return -1;

  if ( item.level == 0 || new_ilevel == 0 || item.stat_val[ idx ] <= 0 )
    return item.stat_val[ idx ];

  if ( item.level == ( int ) new_ilevel )
    return item.stat_val[ idx ];

  int slot_type = random_suffix_type( &item );
  double item_budget = 0, orig_budget = 0;

  if ( slot_type != -1 && item.quality > 0 )
  {
    const random_prop_data_t& ilevel_data = dbc.random_property( new_ilevel );
    const random_prop_data_t& orig_data = dbc.random_property( item.level );

    if ( item.quality == 4 )
    {
      item_budget = ilevel_data.p_epic[ slot_type ];
      orig_budget = orig_data.p_epic[ slot_type ];
    }
    else if ( item.quality == 3 )
    {
      item_budget = ilevel_data.p_rare[ slot_type ];
      orig_budget = orig_data.p_rare[ slot_type ];
    }
    else
    {
      item_budget = ilevel_data.p_uncommon[ slot_type ];
      orig_budget = orig_data.p_uncommon[ slot_type ];
    }
  }

  // Precise stat scaling formula for ilevel increase, stats should be
  // spot on.
  if ( item.stat_alloc[ idx ] > 0 && orig_budget > 0 && item_budget > 0 )
    return static_cast<int>( util::round( ( item.stat_alloc[ idx ] / 10000.0 ) * item_budget - item.stat_socket_mul[ idx ] * dbc.item_socket_cost( new_ilevel ) * ( item_budget / orig_budget ) ) );
  // TODO(?): Should we warn the user that we are using an approximation of
  // the upgraded stats, and that certain stats may be off by one?
  else
    return static_cast<int>( floor( item.stat_val[ idx ] * approx_scale_coefficient( item.level, new_ilevel ) ) );
}

// item_database_t::initialize_item_sources =================================

bool item_database::initialize_item_sources( item_t& item, std::vector<std::string>& source_list )
{
  source_list = item.sim -> item_db_sources;

  if ( ! item.option_data_source_str.empty() )
  {
    std::vector<std::string> item_sources_split = util::string_split( item.option_data_source_str, ":/|" );

    source_list.clear();

    for ( unsigned i = 0; i < item_sources_split.size(); i++ )
    {
      if ( ! util::str_compare_ci( item_sources_split[ i ], "local" ) &&
           ! util::str_compare_ci( item_sources_split[ i ], "wowhead" ) &&
           ! util::str_compare_ci( item_sources_split[ i ], "ptrhead" ) &&
           ! util::str_compare_ci( item_sources_split[ i ], "bcpapi" ) )
      {
        continue;
      }
      util::tokenize( item_sources_split[ i ] );
      source_list.push_back( item_sources_split[ i ] );
    }

    if ( source_list.empty() )
    {
      return false;
    }
  }

  return true;
}

// item_database_t::random_suffix_type ======================================

int item_database::random_suffix_type( const item_data_t* item )
{
  switch ( item -> item_class )
  {
  case ITEM_CLASS_WEAPON:
    switch ( item -> item_subclass )
    {
    case ITEM_SUBCLASS_WEAPON_AXE2:
    case ITEM_SUBCLASS_WEAPON_MACE2:
    case ITEM_SUBCLASS_WEAPON_POLEARM:
    case ITEM_SUBCLASS_WEAPON_SWORD2:
    case ITEM_SUBCLASS_WEAPON_STAFF:
    case ITEM_SUBCLASS_WEAPON_GUN:
    case ITEM_SUBCLASS_WEAPON_BOW:
    case ITEM_SUBCLASS_WEAPON_CROSSBOW:
      return 0;

    case ITEM_SUBCLASS_WEAPON_THROWN:
      return 4;
    default:
      return 3;
    }
    break;
  case ITEM_CLASS_ARMOR:
    switch ( item -> inventory_type )
    {
    case INVTYPE_HEAD:
    case INVTYPE_CHEST:
    case INVTYPE_LEGS:
    case INVTYPE_ROBE:
      return 0;

    case INVTYPE_SHOULDERS:
    case INVTYPE_WAIST:
    case INVTYPE_FEET:
    case INVTYPE_HANDS:
    case INVTYPE_TRINKET:
      return 1;

    case INVTYPE_NECK:
    case INVTYPE_WEAPONOFFHAND:
    case INVTYPE_HOLDABLE:
    case INVTYPE_FINGER:
    case INVTYPE_CLOAK:
    case INVTYPE_WRISTS:
    case INVTYPE_SHIELD:
      return 2;

    default:
      return -1;
    }
    break;
  default:
    return -1;
  }

  return -1;
}

int item_database::random_suffix_type( item_t& item )
{
  if ( weapon_t* w = item.weapon() )
  {
    switch ( w -> type )
    {
      // Two-hand weapons use the first point allocation budget
    case WEAPON_AXE_2H:
    case WEAPON_MACE_2H:
    case WEAPON_POLEARM:
    case WEAPON_SWORD_2H:
    case WEAPON_STAFF:
    case WEAPON_BOW:
    case WEAPON_CROSSBOW:
    case WEAPON_GUN:
      return 0;

      // Various ranged types use the fifth point allocation budget
    case WEAPON_THROWN:
      return 4;

      // One-hand/Off-hand/Main-hand weapons use the fourth point allocation budget
    default:
      return 3;
    }
  }

  // Armor handling goes by slot
  switch ( item.slot )
  {
  case SLOT_HEAD:
  case SLOT_CHEST:
  case SLOT_LEGS:
    return 0;

  case SLOT_SHOULDERS:
  case SLOT_WAIST:
  case SLOT_FEET:
  case SLOT_HANDS:
  case SLOT_TRINKET_1:
  case SLOT_TRINKET_2:
    return 1;

  case SLOT_NECK:
  case SLOT_WRISTS:
  case SLOT_FINGER_1:
  case SLOT_FINGER_2:
  case SLOT_OFF_HAND: // Shields, off hand items
  case SLOT_BACK:
    return 2;

    // Ranged non-weapons are relics, which do not have a point allocation
  case SLOT_TABARD:
  default:
    return -1;
  }
}

uint32_t item_database::armor_value( const item_data_t* item, const dbc_t& dbc, unsigned item_level )
{
  if ( ! item || item -> quality > 5 )
    return 0;

  unsigned ilevel = item_level ? item_level : item -> level;

  // Shield have separate armor table, bypass normal calculation
  if ( item -> item_class == ITEM_CLASS_ARMOR && item -> item_subclass == ITEM_SUBCLASS_ARMOR_SHIELD )
    return ( uint32_t ) floor( dbc.item_armor_shield( ilevel ).values[ item -> quality ] + 0.5 );

  // Only Cloth, Leather, Mail and Plate armor has innate armor values
  if ( item -> item_subclass == ITEM_SUBCLASS_ARMOR_MISC || item -> item_subclass > ITEM_SUBCLASS_ARMOR_PLATE )
    return 0;

  double m_invtype = 0, m_quality = 0, total_armor = 0;

  switch ( item -> inventory_type )
  {
  case INVTYPE_HEAD:
  case INVTYPE_SHOULDERS:
  case INVTYPE_CHEST:
  case INVTYPE_WAIST:
  case INVTYPE_LEGS:
  case INVTYPE_FEET:
  case INVTYPE_WRISTS:
  case INVTYPE_HANDS:
  case INVTYPE_CLOAK:
  case INVTYPE_ROBE:
  {
    total_armor = dbc.item_armor_total( ilevel ).armor_type[ item -> item_subclass - 1 ];
    m_quality   = dbc.item_armor_quality( ilevel ).values[ item -> quality ];
    unsigned invtype = item -> inventory_type;
    if ( invtype == INVTYPE_ROBE ) invtype = INVTYPE_CHEST;
    m_invtype = dbc.item_armor_inv_type( invtype ).armor_type[ item -> item_subclass - 1 ];
    break;
  }
  default: return 0;
  }

  return ( uint32_t ) floor( total_armor * m_quality * m_invtype + 0.5 );
}

// item_database_t::armor_value =============================================

uint32_t item_database::armor_value( item_t& item )
{
  return armor_value( &item.parsed.data, item.player -> dbc, item.item_level() );
}

// item_database_t::weapon_dmg_min/max ======================================

uint32_t item_database::weapon_dmg_min( item_t& item )
{
  return ( uint32_t ) floor( item.player -> dbc.weapon_dps( item.parsed.data.id, item.item_level() ) *
                             item.parsed.data.delay / 1000.0 * ( 1 - item.parsed.data.dmg_range / 2 ) );
}

uint32_t item_database::weapon_dmg_min( const item_data_t* item, const dbc_t& dbc )
{
  return ( uint32_t ) floor( dbc.weapon_dps( item ) * item -> delay / 1000.0 *
                             ( 1 - item -> dmg_range / 2 ) );
}

uint32_t item_database::weapon_dmg_max( const item_data_t* item, const dbc_t& dbc )
{
  return ( uint32_t ) floor( dbc.weapon_dps( item ) * item -> delay / 1000.0 *
                             ( 1 + item -> dmg_range / 2 ) + 0.5 );
}

uint32_t item_database::weapon_dmg_max( item_t& item )
{
  return ( uint32_t ) floor( item.player -> dbc.weapon_dps( item.parsed.data.id, item.item_level() ) *
                             item.parsed.data.delay / 1000.0 * ( 1 + item.parsed.data.dmg_range / 2 ) + 0.5 );
}

bool item_database::parse_gems( item_t& item )
{
  bool match = true;
  std::vector<std::string> stats;

  for ( size_t i = 0; i < sizeof_array( item.parsed.gem_id ); i++ )
  {
    if ( item.parsed.gem_id[ i ] == 0 )
    {
      /// Check if there's a gem slot, if so, this is ungemmed item.
      if ( item.parsed.data.socket_color[ i ] )
        match = false;
      continue;
    }

    if ( item.parsed.data.socket_color[ i ] )
    {
      if ( ! ( item_t::parse_gem( item, item.parsed.gem_id[ i ] ) & item.parsed.data.socket_color[ i ] ) )
        match = false;
    }
    else
    {
      // Naively accept gems to wrist/hands/waist past the "official" sockets, but only a
      // single extra one. Wrist/hands should be checked against player professions at
      // least ..
      // Also accept it on main/offhands for the new 5.1 legendary questline stuff
      if ( item.slot == SLOT_WRISTS || item.slot == SLOT_HANDS ||
           item.slot == SLOT_WAIST || item.slot == SLOT_MAIN_HAND ||
           item.slot == SLOT_OFF_HAND )
      {
        item_t::parse_gem( item, item.parsed.gem_id[ i ] );
        break;
      }
    }
  }

  // Socket bonus
  const item_enchantment_data_t& socket_bonus = item.player -> dbc.item_enchantment( item.parsed.data.id_socket_bonus );
  if ( match && socket_bonus.id )
  {
    std::vector<stat_pair_t> s;
    if ( encode_item_enchant_stats( socket_bonus, s ) )
      item.parsed.gem_stats.insert( item.parsed.gem_stats.end(), s.begin(), s.end() );
  }

  return true;
}

bool item_database::parse_item_spell_enchant( item_t& item,
                                              std::vector<stat_pair_t>& stats,
                                              special_effect_t& effect,
                                              unsigned enchant_id )
{
  if ( enchant_id == 0 ) return true;

  const item_enchantment_data_t& item_enchant = item.player -> dbc.item_enchantment( enchant_id );
  if ( ! item_enchant.id )
  {
    item.player -> sim -> errorf( "Unable to find enchant id %u from item enchantment database", enchant_id );
    return true;
  }

  effect.clear();
  stats.clear();

  for ( unsigned i = 0, k = 0; i < 3; ++i ) // loop through the 3 enchant effects and append them to result string
  {
    if ( item_enchant.ench_type[ i ] == ITEM_ENCHANTMENT_NONE )
      continue;

    // debug
    //std::cout << "id=" << item_enchant.id << " effectnr=" << i << " ";

    std::string enchant_effect; // tmp storage of the enchant effect

    if ( item_enchant.ench_type[ i ] != ITEM_ENCHANTMENT_STAT ) // "enchant spell"
    {
      // Intention: If enchant name contains a linked number ( $ ) or the spell contains a rank_str, use the spell name
      // otherwise use enchant name
      std::string dbc_name = item_enchant.name;
      const spell_data_t* es = item.player -> dbc.spell( item_enchant.ench_prop[ i ] );

      // Handle All stats enchants
      for ( size_t i = 0; i < es -> _effects -> size(); i++ )
      {
        // All stats is indicated by a misc value of -1
        if ( es -> effectN( i + 1 ).type() == E_APPLY_AURA &&
             es -> effectN( i + 1 ).subtype() == A_MOD_STAT &&
             es -> effectN( i + 1 ).misc_value1() == -1 )
        {
          stats.push_back( stat_pair_t( STAT_ALL, es -> effectN( i + 1 ).base_value() ) );
          break;
        }
      }

      // Kludge the rest
      if ( dbc_name.find( "$" ) != dbc_name.npos || ( es && es -> id() > 0 && es -> rank_str() != 0 ) )
      {
        if ( es && es -> id() > 0 )
        {
          enchant_effect = es -> name_cstr(); // Use Spell Name

          if ( es -> rank_str() != 0 ) // If rank str is available, append its number to the enchant name
          {
            std::string rank = std::string( es -> rank_str() );
            if (  rank.find( "Rank " ) != rank.npos )
              rank.erase( rank.find( "Rank " ), std::string( "Rank " ).length() );
            enchant_effect += "_" + rank;
          }
        }
      }
      else
      {
        if ( k++ == 0 ) enchant_effect = item_enchant.name;
      }

      util::tokenize( enchant_effect );

      // Special modifications
      util::erase_all( enchant_effect, "+" ); // remove plus signs

      // debug
      //std::cout << "enchant spell id=" << es -> id() << " enchant spell name=" << es -> name_cstr() << " ";
    }
    else // stat-enchant
    {
      stat_pair_t s = item_enchantment_effect_stats( item_enchant, i );
      if ( s.stat != STAT_NONE )
        stats.push_back( s );
    }

    if ( ! enchant_effect.empty() )
      effect.name_str = enchant_effect;

    // debug
    //std::cout << "name= " << enchant_effect << " enchant_name= " << item_enchant.name << "\n";
  }

  return true;
}

// item_database_t::download_slot ===========================================

bool item_database::download_slot( item_t& item )
{
  bool ret = load_item_from_data( item );

  if ( ret )
  {
    parse_gems( item );

    item.source_str = "Local";
  }

  return ret;
}

// item_database_t::load_item_from_data =====================================

bool item_database::load_item_from_data( item_t& item )
{
  // Simple copying of basic stats from item database (dbc) to the item object
  const item_data_t* data = item.player -> dbc.item( item.parsed.data.id );
  if ( ! data || ! data -> id ) return false;

  item.parsed.data = *data;
  item.name_str = data -> name;
  item.icon_str = data -> icon;
  item.parsed.data.name = item.name_str.c_str();
  item.parsed.data.icon = item.icon_str.c_str();

  util::tokenize( item.name_str );

  return true;
}

// item_database_t::download_item ===========================================

bool item_database::download_item( item_t& item )
{
  bool ret = load_item_from_data( item );

  if ( ret )
    item.source_str = "Local";

  return ret;
}

// item_database_t::download_glyph ==========================================

bool item_database::download_glyph( player_t* player, std::string& glyph_name,
                                    const std::string& glyph_id )
{
  long gid                 = strtol( glyph_id.c_str(), 0, 10 );
  const item_data_t* glyph = player -> dbc.item( gid );

  if ( gid <= 0 || ! glyph ) return false;

  glyph_name = glyph -> name;

  return true;
}

// item_database_t::parse_gem ===============================================

unsigned item_database::parse_gem( item_t& item, unsigned gem_id )
{
  if ( gem_id == 0 )
    return GEM_NONE;

  const item_data_t* gem = item.player -> dbc.item( gem_id );
  if ( ! gem )
    return GEM_NONE;

  const gem_property_data_t& gem_prop = item.player -> dbc.gem_property( gem -> gem_properties );
  if ( ! gem_prop.id )
    return GEM_NONE;

  if ( gem_prop.color == SOCKET_COLOR_META )
  {
    // For now, make meta gems simply parse the name out
    std::string gem_name = gem -> name;
    std::size_t cut_pt = gem_name.rfind( " Diamond" );
    if ( cut_pt != gem_name.npos )
    {
      gem_name.erase( cut_pt );
      util::tokenize( gem_name );
      meta_gem_e meta_type = util::parse_meta_gem_type( gem_name );
      if ( meta_type != META_GEM_NONE )
        item.player -> meta_gem = meta_type;
    }
  }
  else
  {
    // Fetch gem stats
    const item_enchantment_data_t& item_ench = item.player -> dbc.item_enchantment( gem_prop.enchant_id );
    if ( item_ench.id )
    {
      std::vector<stat_pair_t> stats;
      if ( encode_item_enchant_stats( item_ench, stats ) )
        item.parsed.gem_stats.insert( item.parsed.gem_stats.end(), stats.begin(), stats.end() );
    }
  }

  return gem_prop.color;
}

// item_database::upgrade_ilevel ==============================================

// TODO: DBC Based upgrading system would be safer, this works for now, probably
unsigned item_database::upgrade_ilevel( const item_data_t& item, unsigned upgrade_level )
{
  if ( item.quality == 3 ) return upgrade_level * 8;
  if ( item.quality == 4 ) return upgrade_level * 4;
  return 0;
}

