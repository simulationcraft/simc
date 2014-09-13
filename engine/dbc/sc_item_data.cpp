// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

bool item_database::apply_item_bonus( item_t& item, const item_bonus_entry_t& entry )
{
  switch ( entry.type )
  {
    // Adjust ilevel, value is in 'value_1' field
    case ITEM_BONUS_ILEVEL:
      if ( item.sim -> debug )
        item.player -> sim -> out_debug.printf( "Player %s item '%s' adjusting ilevel by %d (old=%d new=%d)",
            item.player -> name(), item.name(), entry.value_1, item.parsed.data.level, item.parsed.data.level + entry.value_1 );
      item.parsed.data.level += entry.value_1;
      break;
    // Add new item stats. Value_1 has the item modifier, value_2 has the
    // allocation percent. This bonus type is the reason we don't really
    // support the bonus id stuff outside of our local item database.
    case ITEM_BONUS_MOD:
    {
      // First, check if the item already has that stat
      int found = -1;
      int offset = -1;
      for ( size_t i = 0, end = sizeof_array( item.parsed.data.stat_type_e ); i < end; i++ )
      {
        // Put the new stat in first available slot
        if ( offset == -1 && item.parsed.data.stat_type_e[ i ] == ITEM_MOD_NONE )
          offset = static_cast< int >( i );

        // Stat already found
        if ( found == -1 && item.parsed.data.stat_type_e[ i ] == entry.value_1 )
          found = static_cast< int >( i );
      }

      // New stat, and there's room.
      if ( found == -1 && offset != -1 )
      {
        if ( item.sim -> debug )
          item.player -> sim -> out_debug.printf( "Player %s item '%s' adding new stat %s offset=%d (allocation %u)", 
              item.player -> name(), item.name(), util::stat_type_string( util::translate_item_mod( entry.value_1 ) ), offset, entry.value_2 );
        item.parsed.data.stat_type_e[ offset ] = entry.value_1;
        item.parsed.data.stat_alloc[ offset ] = entry.value_2;
      }
      // Existing stat, set (?) new allocation percent
      else if ( found != -1 )
        item.parsed.data.stat_alloc[ offset ] = entry.value_2;
      // New stat but no room, this should never happen.
      else
      {
        item.player -> sim -> errorf( "Player %s item '%s' unable to add item modifier, stats full", item.player -> name(), item.name() );
        return false;
      }
      break;
    }
    // Item name description. We should do flagging here, I suppose. We don't
    // have the name descriptions exported at the moment though ..
    case ITEM_BONUS_DESC:
      break;
    // WoD random suffix name string; stats are in other options for that item
    // bonus id. Again, we don't export item name descriptions so we cannot
    // apply the name
    case ITEM_BONUS_SUFFIX:
      break;
    // Adjust required level of the item. Value in 'value_1'
    case ITEM_BONUS_REQ_LEVEL:
      if ( item.sim -> debug )
        item.player -> sim -> out_debug.printf( "Player %s item '%s' required level by %d (old=%d new=%d)",
            item.player -> name(), item.name(), entry.value_1, item.parsed.data.req_level, item.parsed.data.req_level + entry.value_1 );
      item.parsed.data.req_level += entry.value_1;
      break;
    // Number of sockets is in value 1, type (color) of sockets is in value 2
    case ITEM_BONUS_SOCKET:
    {
      if ( item.sim -> debug )
        item.player -> sim -> out_debug.printf( "Player %s item '%s' adding %d socket(s) (type=%d)",
            item.player -> name(), item.name(), entry.value_1, entry.value_2 );
      int n_added = 0;
      for ( size_t i = 0, end = sizeof_array( item.parsed.data.socket_color ); i < end && n_added < entry.value_1; i++ )
      {
        if ( item.parsed.data.socket_color[ i ] != SOCKET_COLOR_NONE )
          continue;

        item.parsed.data.socket_color[ i ] = entry.value_2;
        n_added++;
      }

      if ( n_added < entry.value_1 )
      {
        item.player -> sim -> errorf( "Player %s item '%s' unable to fit %d new sockets into the item (could only fit %d)",
            item.player -> name(), item.name(), entry.value_1, n_added );
        return false;
      }
      break;
    }
    default:
      break;
  }
  return true;
}

stat_pair_t item_database::item_enchantment_effect_stats( const item_enchantment_data_t& enchantment, int index )
{
  assert( enchantment.id );
  assert( index >= 0 && index < 3 );

  if ( enchantment.ench_type[ index ] == ITEM_ENCHANTMENT_STAT )
    return stat_pair_t( util::translate_item_mod( enchantment.ench_prop[ index ] ), enchantment.ench_amount[ index ] );

  return stat_pair_t();
}

stat_pair_t item_database::item_enchantment_effect_stats( player_t* player,
                                                          const item_enchantment_data_t& enchantment,
                                                          int index )
{
  assert( enchantment.id );
  assert( index >= 0 && index < 3 );

  if ( enchantment.ench_type[ index ] != ITEM_ENCHANTMENT_STAT )
    return stat_pair_t();
  stat_e stat = util::translate_item_mod( enchantment.ench_prop[ index ] );
  double value = 0;

  if ( enchantment.id_scaling == 0 )
    value = enchantment.ench_amount[ index ];
  else
  {
    unsigned level = player -> level;

    if ( static_cast< unsigned >( player -> level ) > enchantment.max_scaling_level )
      level = enchantment.max_scaling_level;

    double budget = player -> dbc.spell_scaling( static_cast< player_e >( enchantment.id_scaling ), level );
    value = util::round( budget * enchantment.ench_coeff[ index ] );
  }

  if ( stat != STAT_NONE && value != 0 )
    return stat_pair_t( stat, (int)value );

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

// item_database_t::approx_scale_coefficient ================================

double item_database::approx_scale_coefficient( unsigned current_ilevel, unsigned new_ilevel )
{
  if ( current_ilevel == 0 || new_ilevel == 0 )
    return 1.0;

  int diff = current_ilevel - new_ilevel;
  return 1.0 / pow( 1.15, diff / 15.0 );
}

// item_database_t::scaled_stat =============================================

int item_database::scaled_stat( const item_data_t& item, const dbc_t& dbc, size_t idx, unsigned new_ilevel )
{
  // Safeguard against array overflow, should never happen in any case
  if ( idx >= sizeof_array( item.stat_val ) - 1 )
    return -1;

  if ( item.level == 0 )
    return item.stat_val[ idx ];

  //if ( item.level == ( int ) new_ilevel )
  //  return item.stat_val[ idx ];

  int slot_type = random_suffix_type( &item );
  double item_budget = 0/*, orig_budget = 0*/;

  if ( slot_type != -1 && item.quality > 0 )
  {
    const random_prop_data_t& ilevel_data = dbc.random_property( new_ilevel );
    //const random_prop_data_t& orig_data = dbc.random_property( item.level );

    if ( item.quality == 4 )
    {
      item_budget = ilevel_data.p_epic[ slot_type ];
      //orig_budget = orig_data.p_epic[ slot_type ];
    }
    else if ( item.quality == 3 )
    {
      item_budget = ilevel_data.p_rare[ slot_type ];
      //orig_budget = orig_data.p_rare[ slot_type ];
    }
    else
    {
      item_budget = ilevel_data.p_uncommon[ slot_type ];
      //orig_budget = orig_data.p_uncommon[ slot_type ];
    }
  }

  // Precise stat scaling formula for ilevel increase, stats should be
  // spot on.
  if ( item.stat_alloc[ idx ] > 0 /* && orig_budget > 0 */ && item_budget > 0 )
  {
    double v_raw = util::round( item.stat_alloc[ idx ] * item_budget / 10000.0 );
    // Socket penalty is supposedly gone in Warlords of Draenor, but it really does not seem so in the latest alpha.
    // NOTENOTENOTENOTE: Item socket cost penalty multiplier _seems_ to be based on _BASE_ itemlevel, not the upgraded one
    double v_socket_penalty = util::round( item.stat_socket_mul[ idx ] * dbc.item_socket_cost( item.level ) );
    return static_cast<int>( v_raw - v_socket_penalty );
  }
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
  weapon_t* w = item.weapon();
  if ( w && w -> type != WEAPON_NONE )
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
  return ( uint32_t ) floor( item.player -> dbc.weapon_dps( &item.parsed.data, item.item_level() ) *
                             item.parsed.data.delay / 1000.0 * ( 1 - item.parsed.data.dmg_range / 2 ) );
}

uint32_t item_database::weapon_dmg_min( const item_data_t* item, const dbc_t& dbc, unsigned item_level )
{
  return ( uint32_t ) floor( dbc.weapon_dps( item, item_level ) * item -> delay / 1000.0 *
                             ( 1 - item -> dmg_range / 2 ) );
}

uint32_t item_database::weapon_dmg_max( const item_data_t* item, const dbc_t& dbc, unsigned item_level )
{
  return ( uint32_t ) ceil( dbc.weapon_dps( item, item_level ) * item -> delay / 1000.0 *
                            ( 1 + item -> dmg_range / 2 ) + 0.5 );
}

uint32_t item_database::weapon_dmg_max( item_t& item )
{
  return ( uint32_t ) ceil( item.player -> dbc.weapon_dps( &item.parsed.data, item.item_level() ) *
                            item.parsed.data.delay / 1000.0 * ( 1 + item.parsed.data.dmg_range / 2 ) + 0.5 );
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

  effect.reset();
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
      if ( es )
      {
        for ( size_t j = 0; j < es -> _effects -> size(); j++ )
        {
          // All stats is indicated by a misc value of -1
          if ( es -> effectN( j + 1 ).type() == E_APPLY_AURA &&
               es -> effectN( j + 1 ).subtype() == A_MOD_STAT &&
               es -> effectN( j + 1 ).misc_value1() == -1 )
          {
            stats.push_back( stat_pair_t( STAT_ALL, es -> effectN( j + 1 ).base_value() ) );
            break;
          }
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

// item_database_t::// item_database_t::load_item_from_data =====================================

bool item_database::load_item_from_data( item_t& item )
{
  // Simple copying of basic stats from item database (dbc) to the item object
  const item_data_t* data = item.player -> dbc.item( item.parsed.data.id );
  if ( ! data || ! data -> id ) return false;

  item.parsed.data = *data;
  item.name_str = data -> name;
  item.parsed.data.name = item.name_str.c_str();

  util::tokenize( item.name_str );

  // Item bonus for local source only. TODO: BCP API and Wowhead will need ..
  // something similar
  for ( size_t i = 0, end = item.parsed.bonus_id.size(); i < end; i++ )
  {
    std::vector<const item_bonus_entry_t*> item_bonuses = item.player -> dbc.item_bonus( item.parsed.bonus_id[ i ] );
    // Apply bonuses
    for ( size_t bonus_idx = 0, end = item_bonuses.size(); bonus_idx < end; bonus_idx++ )
    {
      if ( ! apply_item_bonus( item, *item_bonuses[ bonus_idx ] ) )
        return false;
    }
  }

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

// item_database::upgrade_ilevel ============================================

// TODO: DBC Based upgrading system would be safer, this works for now, probably
unsigned item_database::upgrade_ilevel( const item_data_t& item, unsigned upgrade_level )
{
  if ( item.quality == 3 ) return upgrade_level * 8;
  if ( item.quality == 4 ) return upgrade_level * 4;
  if ( item.quality == 5 ) return upgrade_level * 4;
  return 0;
}

double item_database::item_budget( const item_t* item, unsigned max_ilevel )
{
  double m_scale = 0;

  unsigned scaling_level = item -> item_level();
  if ( max_ilevel > 0 )
    scaling_level = std::min( scaling_level, max_ilevel );

  const random_prop_data_t& budget = item -> player -> dbc.random_property( scaling_level );

  if ( item -> parsed.data.quality >= 4 )
    m_scale = budget.p_epic[ 0 ];
  else if ( item -> parsed.data.quality == 3 )
    m_scale = budget.p_rare[ 0 ];
  else
    m_scale = budget.p_uncommon[ 0 ];

  return m_scale;
}

// item_database::parse_tokens ==============================================

size_t item_database::parse_tokens( std::vector<token_t>& tokens,
                                    const std::string&    encoded_str )
{
  std::vector<std::string> splits = util::string_split( encoded_str, "_" );

  tokens.resize( splits.size() );
  for ( size_t i = 0; i < splits.size(); i++ )
  {
    token_t& t = tokens[ i ];
    t.full = splits[ i ];
    int index = 0;
    while ( t.full[ index ] != '\0' &&
            t.full[ index ] != '%'  &&
            ! isalpha( static_cast<unsigned char>( t.full[ index ] ) ) ) index++;
    if ( index == 0 )
    {
      t.name = t.full;
      t.value = 0;
    }
    else
    {
      t.name = t.full.substr( index );
      t.value_str = t.full.substr( 0, index );
      t.value = atof( t.value_str.c_str() );
    }
  }

  return splits.size();
}

