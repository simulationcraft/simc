// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

namespace   // ANONYMOUS NAMESPACE ==========================================
{

std::string stat_to_str( int stat, int stat_amount )
{
  std::string stat_str;

  if ( stat_amount )
  {
    stat_type s = util_t::translate_item_mod( stat );
    if ( s != STAT_NONE )
    {
      char stat_buf[64];
      snprintf( stat_buf, sizeof( stat_buf ), "%d%s", stat_amount, util_t::stat_type_abbrev( s ) );
      stat_str = stat_buf;

      armory_t::format( stat_str );
    }
  }

  return stat_str;
}

std::string encode_stats( const std::vector<std::string>& stats )
{
  std::string s;

  for ( unsigned i = 0; i < stats.size(); i++ )
  {
    if ( i ) s += '_';
    s += stats[ i ];
  }

  return s;
}

std::size_t encode_item_enchant_stats( const item_enchantment_data_t& enchantment, std::vector<std::string>& stats )
{
  assert( enchantment.id );

  for ( int i = 0; i < 3; i++ )
  {
    if ( enchantment.ench_type[ i ] != ITEM_ENCHANTMENT_STAT )
      continue;

    std::string stat_str = stat_to_str( enchantment.ench_prop[ i ], enchantment.ench_amount[ i ] );
    if ( ! stat_str.empty() ) stats.push_back( stat_str );
  }

  return stats.size();
}

std::size_t encode_item_stats( const item_data_t* item, std::vector<std::string>& stats )
{
  assert( item );

  for ( int i = 0; i < 10; i++ )
  {
    if ( item -> stat_type[ i ] < 0 )
      continue;

    std::string stat_str = stat_to_str( item -> stat_type[ i ], item -> stat_val[ i ] );
    if ( ! stat_str.empty() ) stats.push_back( stat_str );
  }

  return stats.size();
}

bool parse_item_quality( item_t& item, const item_data_t* item_data )
{
  assert( item_data );

  item.armory_quality_str.clear();

  if ( item_data -> quality > 1 )
    item.armory_quality_str = util_t::item_quality_string( item_data -> quality );

  return true;
}

bool parse_item_level( item_t& item, const item_data_t* item_data )
{
  assert( item_data );

  item.armory_ilevel_str = util_t::to_string( item_data -> level );

  return true;
}

bool parse_item_name( item_t& item, const item_data_t* item_data )
{
  assert( item_data );

  item.armory_name_str = item_data -> name;

  armory_t::format( item.armory_name_str );

  return true;
}

bool parse_item_heroic( item_t& item, const item_data_t* item_data )
{
  assert( item_data );

  item.armory_heroic_str.clear();

  if ( item_data -> flags_1 & ITEM_FLAG_HEROIC )
    item.armory_heroic_str = "1";

  return true;
}

bool parse_item_lfr( item_t& item, const item_data_t* item_data )
{
  assert( item_data );

  item.armory_lfr_str.clear();

  // HACK: Horrible hack to determine LFR loot.
  if ( ( ( item_data -> level == 384 ) || ( item_data -> level == 390 ) ) && ( item_data -> id >= 77969 ) && ! strstr( item.armory_name_str.c_str(), "gladiators" ) )
  {
    item.armory_lfr_str = "1";
  }

  //if ( item_data -> flags_1 & ITEM_FLAG_LFR )
  //  item.armory_lfr_str = "1";

  return true;
}

bool parse_item_armor_type( item_t& item, const item_data_t* item_data )
{
  assert( item_data );

  item.armory_armor_type_str.clear();

  if ( item_data -> item_class == ITEM_CLASS_ARMOR )
  {
    if ( item_data -> item_subclass == ITEM_SUBCLASS_ARMOR_CLOTH )
      item.armory_armor_type_str = "cloth";
    else if ( item_data -> item_subclass == ITEM_SUBCLASS_ARMOR_LEATHER )
      item.armory_armor_type_str = "leather";
    else if ( item_data -> item_subclass == ITEM_SUBCLASS_ARMOR_MAIL )
      item.armory_armor_type_str = "mail";
    else if ( item_data -> item_subclass == ITEM_SUBCLASS_ARMOR_PLATE )
      item.armory_armor_type_str = "plate";
  }

  return true;
}

bool parse_item_stats( item_t&            item,
                       const item_data_t* item_data )
{
  std::vector<std::string> stats;
  uint32_t armor;
  assert( item_data );

  item.armory_stats_str.clear();

  if ( ( armor = item_database_t::armor_value( item, item_data -> id ) ) > 0 )
  {
    char b[64];
    snprintf( b, sizeof( b ), "%darmor", armor );
    stats.push_back( b );
  }

  if ( encode_item_stats( item_data, stats ) > 0 )
    item.armory_stats_str = encode_stats( stats );

  return true;
}

bool parse_weapon_type( item_t&            item,
                        const item_data_t* item_data )
{
  double   speed;
  unsigned min_dam, max_dam;
  char     stat_buf[64];

  item.armory_weapon_str.clear();

  if ( item_data -> item_class != ITEM_CLASS_WEAPON )
    return true;

  speed   = item_data -> delay / 1000.0;
  min_dam = item_database_t::weapon_dmg_min( item, item_data -> id );
  max_dam = item_database_t::weapon_dmg_max( item, item_data -> id );

  if ( ! speed || ! min_dam || ! max_dam )
    return true;

  weapon_type w = util_t::translate_weapon_subclass( ( item_subclass_weapon ) item_data -> item_subclass );
  if ( w == WEAPON_NONE || w == WEAPON_WAND )
    return true;

  snprintf( stat_buf, sizeof( stat_buf ), "%s_%4.2fspeed_%umin_%umax",
            util_t::weapon_type_string( w ), speed, min_dam, max_dam );
  item.armory_weapon_str = stat_buf;

  return true;
}

// download_common ==========================================================

const item_data_t* download_common( item_t& item, const std::string& item_id )
{
  long iid = strtol( item_id.c_str(), 0, 10 );
  const item_data_t* item_data = item.player -> dbc.item( iid );
  if ( iid <= 0 || ! item_data )
  {
    item.sim -> errorf( "Player %s unable to find item id %s at slot %s.\n",
                        item.player -> name(), item_id.c_str(), item.slot_name() );
    return 0;
  }

  if ( ! item_database_t::load_item_from_data( item, item_data ) )
    return 0;

  return item_data;
}

// log_item =================================================================

void log_item( const item_t& item )
{
  if ( item.sim -> debug )
  {
    log_t::output( item.sim, "item_db: n=[%s] q=[%s] l=[%s] h=[%s] rf=[%s] a=[%s] s=[%s] w=[%s] g=[%s] e=[%s] r=[%s] rs=[%s] a=[%s]",
                   item.armory_name_str.c_str(),
                   item.armory_quality_str.c_str(),
                   item.armory_ilevel_str.c_str(),
                   item.armory_heroic_str.c_str(),
                   item.armory_lfr_str.c_str(),
                   item.armory_armor_type_str.c_str(),
                   item.armory_stats_str.c_str(),
                   item.armory_weapon_str.c_str(),
                   item.armory_gems_str.c_str(),
                   item.armory_enchant_str.c_str(),
                   item.armory_reforge_str.c_str(),
                   item.armory_random_suffix_str.c_str(),
                   item.armory_addon_str.c_str() );
  }
}

}  // ANONYMOUS NAMESPACE ====================================================

// item_database_t::initialize_item_sources =================================

bool item_database_t::initialize_item_sources( const item_t& item, std::vector<std::string>& source_list )
{
  source_list = item.sim -> item_db_sources;

  if ( ! item.option_data_source_str.empty() )
  {
    std::vector<std::string> item_sources_split;
    util_t::string_split( item_sources_split, item.option_data_source_str, ":/|", false );

    source_list.clear();

    for ( unsigned i = 0; i < item_sources_split.size(); i++ )
    {
      if ( ! util_t::str_compare_ci( item_sources_split[ i ], "local" ) &&
           ! util_t::str_compare_ci( item_sources_split[ i ], "mmoc" ) &&
           ! util_t::str_compare_ci( item_sources_split[ i ], "wowhead" ) &&
           ! util_t::str_compare_ci( item_sources_split[ i ], "ptrhead" ) &&
           ! util_t::str_compare_ci( item_sources_split[ i ], "armory" ) &&
           ! util_t::str_compare_ci( item_sources_split[ i ], "bcpapi" ) )
      {
        continue;
      }

      source_list.push_back( armory_t::format( item_sources_split[ i ] ) );
    }

    if ( source_list.empty() )
    {
      return false;
    }
  }

  return true;
}

// item_database_t::random_suffix_type ======================================

int item_database_t::random_suffix_type( const item_data_t* item )
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
      return 0;

    case ITEM_SUBCLASS_WEAPON_BOW:
    case ITEM_SUBCLASS_WEAPON_GUN:
    case ITEM_SUBCLASS_WEAPON_THROWN:
    case ITEM_SUBCLASS_WEAPON_CROSSBOW:
    case ITEM_SUBCLASS_WEAPON_WAND:
      return 4;
    default:
      return 3;
    }
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
      return 2;

    default:
      return -1;
    }
  default:
    return -1;
  }

  return -1;
}

int item_database_t::random_suffix_type( const item_t& item )
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
      return 0;

    // Various ranged types use the fifth point allocation budget
    case WEAPON_BOW:
    case WEAPON_CROSSBOW:
    case WEAPON_GUN:
    case WEAPON_THROWN:
    case WEAPON_WAND:
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
  case SLOT_RANGED:
  case SLOT_TABARD:
  default:
    return -1;
  }
}

uint32_t item_database_t::armor_value( const item_data_t* item, const dbc_t& dbc )
{
  if ( ! item || item -> quality > 5 )
    return 0;

  // Shield have separate armor table, bypass normal calculation
  if ( item -> item_class == ITEM_CLASS_ARMOR && item -> item_subclass == ITEM_SUBCLASS_ARMOR_SHIELD )
    return ( uint32_t ) floor( dbc.item_armor_shield( item -> level ).values[ item -> quality ] + 0.5 );

  // Only Cloth, Leather, Mail and Plate armor has innate armor values
  if ( item -> item_subclass != ITEM_SUBCLASS_ARMOR_MISC && item -> item_subclass > ITEM_SUBCLASS_ARMOR_PLATE )
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
    total_armor = dbc.item_armor_total( item -> level ).armor_type[ item -> item_subclass - 1 ];
    m_quality   = dbc.item_armor_quality( item -> level ).values[ item -> quality ];
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

uint32_t item_database_t::armor_value( const item_t& item_struct, unsigned item_id )
{
  const dbc_t& dbc = item_struct.player -> dbc;
  return armor_value( dbc.item( item_id ), dbc );
}

// item_database_t::weapon_dmg_min/max ======================================

uint32_t item_database_t::weapon_dmg_min( const item_t& item, unsigned item_id )
{
  return ( uint32_t ) floor( item.player -> dbc.weapon_dps( item_id ) *
                             item.player -> dbc.item( item_id ) -> delay / 1000.0 *
                             ( 1 - item.player -> dbc.item( item_id ) -> dmg_range / 2 ) );
}

uint32_t item_database_t::weapon_dmg_min( const item_data_t* item, const dbc_t& dbc )
{
  return ( uint32_t ) floor( dbc.weapon_dps( item -> id ) *
                             dbc.item( item -> id ) -> delay / 1000.0 *
                             ( 1 - dbc.item( item -> id ) -> dmg_range / 2 ) );
}

uint32_t item_database_t::weapon_dmg_max( const item_data_t* item, const dbc_t& dbc )
{
  return ( uint32_t ) floor( dbc.weapon_dps( item -> id ) *
                             dbc.item( item -> id ) -> delay / 1000.0 *
                             ( 1 + dbc.item( item -> id ) -> dmg_range / 2 ) + 0.5 );
}

uint32_t item_database_t::weapon_dmg_max( const item_t& item, unsigned item_id )
{
  return ( uint32_t ) floor( item.player -> dbc.weapon_dps( item_id ) *
                             item.player -> dbc.item( item_id ) -> delay / 1000.0 *
                             ( 1 + item.player -> dbc.item( item_id ) -> dmg_range / 2 ) + 0.5 );
}

bool item_database_t::parse_gems( item_t&            item,
                                  const item_data_t* item_data,
                                  const std::string  gem_ids[ 3 ] )
{
  bool match = true;
  std::vector<std::string> stats;

  item.armory_gems_str.clear();

  for ( unsigned i = 0; i < 3; i++ )
  {
    if ( gem_ids[ i ].empty() )
    {
      /// Check if there's a gem slot, if so, this is ungemmed item.
      if ( item_data -> socket_color[ i ] )
        match = false;
      continue;
    }

    if ( item_data -> socket_color[ i ] )
    {
      if ( ! ( item_t::parse_gem( item, gem_ids[ i ] ) & item_data -> socket_color[ i ] ) )
        match = false;
    }
    else
    {
      // Naively accept gems to wrist/hands/waist past the "official" sockets, but only a
      // single extra one. Wrist/hands should be checked against player professions at
      // least ..
      if ( item.slot == SLOT_WRISTS || item.slot == SLOT_HANDS || item.slot == SLOT_WAIST )
      {
        item_t::parse_gem( item, gem_ids[ i ] );
        break;
      }
    }
  }

  // Socket bonus
  const item_enchantment_data_t& socket_bonus = item.player -> dbc.item_enchantment( item_data -> id_socket_bonus );
  if ( match && socket_bonus.id )
  {
    if ( encode_item_enchant_stats( socket_bonus, stats ) > 0 && ! item.armory_gems_str.empty() )
      item.armory_gems_str += "_";

    item.armory_gems_str += encode_stats( stats );
  }

  return true;
}

bool item_database_t::parse_enchant( item_t&            item,
                                     const std::string& enchant_id )
{
  if ( enchant_id.empty() || enchant_id == "none" || enchant_id == "0" ) return true;

  long                                    eid = strtol( enchant_id.c_str(), 0, 10 );
  bool                              has_spell = false;
  std::vector<std::string> stats;

  const item_enchantment_data_t& item_enchant = item.player -> dbc.item_enchantment( eid );
  if ( ! item_enchant.id )
  {
    item.player -> sim -> errorf( "Unable to find enchant id %lu from item enchantment database", eid );
    return true;
  }

  item.armory_enchant_str.clear();

  for ( unsigned i = 0; i < 3; i++ )
  {
    if ( item_enchant.ench_type[ i ] == ITEM_ENCHANTMENT_NONE ) continue;
    if ( item_enchant.ench_type[ i ] != ITEM_ENCHANTMENT_STAT )
    {
      has_spell = true;
      break;
    }
  }

  // For now, if there's a spell in the enchant, defer back to old ways
  if ( has_spell )
  {
    return enchant_t::download( item, enchant_id );
  }
  else
  {
    if ( encode_item_enchant_stats( item_enchant, stats ) > 0 )
      item.armory_enchant_str = encode_stats( stats );
  }

  return true;
}

// item_database_t::download_slot ===========================================

bool item_database_t::download_slot( item_t&            item,
                                     const std::string& item_id,
                                     const std::string& enchant_id,
                                     const std::string& addon_id,
                                     const std::string& reforge_id,
                                     const std::string& rsuffix_id,
                                     const std::string  gem_ids[ 3 ] )
{
  const item_data_t* item_data = download_common( item, item_id );
  if ( ! item_data )
    return false;

  parse_gems( item, item_data, gem_ids );

  if ( ! parse_enchant( item, enchant_id ) )
  {
    item.sim -> errorf( "Player %s unable to parse enchant id %s for item \"%s\" at slot %s.\n",
                        item.player -> name(), enchant_id.c_str(), item.name(), item.slot_name() );
  }

  if ( ! enchant_t::download_addon( item, addon_id ) )
  {
    item.sim -> errorf( "Player %s unable to parse addon id %s for item \"%s\" at slot %s.\n",
                        item.player -> name(), addon_id.c_str(), item.name(), item.slot_name() );
  }

  if ( ! enchant_t::download_reforge( item, reforge_id ) )
  {
    item.sim -> errorf( "Player %s unable to parse reforge id %s for item \"%s\" at slot %s.\n",
                        item.player -> name(), reforge_id.c_str(), item.name(), item.slot_name() );
  }

  if ( ! enchant_t::download_rsuffix( item, rsuffix_id ) )
  {
    item.sim -> errorf( "Player %s unable to determine random suffix '%s' for item '%s' at slot %s.\n",
                        item.player -> name(), rsuffix_id.c_str(), item.name(), item.slot_name() );
  }

  log_item( item );

  return true;
}

// item_database_t::load_item_from_data =====================================

bool item_database_t::load_item_from_data( item_t& item, const item_data_t* item_data )
{
  if ( ! item_data ) return false;

  parse_item_name( item, item_data );
  parse_item_quality( item, item_data );
  parse_item_level( item, item_data );
  parse_item_heroic( item, item_data );
  parse_item_lfr( item, item_data );
  parse_item_armor_type( item, item_data );
  parse_item_stats( item, item_data );
  parse_weapon_type( item, item_data );

  return true;
}

// item_database_t::download_item ===========================================

bool item_database_t::download_item( item_t& item, const std::string& item_id )
{
  if ( ! download_common( item, item_id ) )
    return false;
  log_item( item );
  return true;
}

// item_database_t::download_glyph ==========================================

bool item_database_t::download_glyph( player_t* player, std::string& glyph_name, const std::string& glyph_id )
{
  long gid                 = strtol( glyph_id.c_str(), 0, 10 );
  const item_data_t* glyph = player -> dbc.item( gid );

  if ( gid <= 0 || ! glyph ) return false;

  glyph_name = glyph -> name;

  if (      glyph_name.substr( 0, 9 ) == "Glyph of " ) glyph_name.erase( 0, 9 );
  else if ( glyph_name.substr( 0, 8 ) == "Glyph - "  ) glyph_name.erase( 0, 8 );
  armory_t::format( glyph_name );

  return true;
}

// item_database_t::parse_gem ===============================================

int item_database_t::parse_gem( item_t& item, const std::string& gem_id )
{
  long gid = strtol( gem_id.c_str(), 0, 10 );
  if ( gid <= 0 )
    return SOCKET_COLOR_NONE;

  const item_data_t* gem = item.player -> dbc.item( gid );
  if ( ! gem )
    return SOCKET_COLOR_NONE;

  const gem_property_data_t& gem_prop = item.player -> dbc.gem_property( gem -> gem_properties );
  if ( ! gem_prop.id )
    return SOCKET_COLOR_NONE;

  if ( gem_prop.color == SOCKET_COLOR_META )
  {
    // For now, make meta gems simply parse the name out
    std::string gem_name = gem -> name;
    std::size_t cut_pt = gem_name.rfind( " Diamond" );
    if ( cut_pt != gem_name.npos )
    {
      gem_name.erase( cut_pt );
      armory_t::format( gem_name );
      item.armory_gems_str += gem_name;
    }
  }
  else
  {
    // Fetch gem stats
    const item_enchantment_data_t& item_ench = item.player -> dbc.item_enchantment( gem_prop.enchant_id );
    if ( item_ench.id )
    {
      std::vector<std::string> stats;
      if ( encode_item_enchant_stats( item_ench, stats ) )
      {
        if ( ! item.armory_gems_str.empty() )
          item.armory_gems_str += '_';
        item.armory_gems_str += encode_stats( stats );
      }
    }
  }

  return gem_prop.color;
}
