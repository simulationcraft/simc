// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "dbc/client_data.hpp"
#include "dbc/item_effect.hpp"
#include "dbc/spell_item_enchantment.hpp"
#include "dbc/item_bonus.hpp"
#include "dbc/item_data.hpp"
#include "dbc/item_scaling.hpp"
#include "dbc/item_database.hpp"
#include "dbc/dbc.hpp"
#include "item/special_effect.hpp"
#include "item/item.hpp"
#include "player/sc_player.hpp"
#include "sim/sc_sim.hpp"
#include <cctype>

namespace {
  template <item_subclass_consumable CLASS>
  struct consumable_filter_t
  {
    bool operator()(const dbc_item_data_t& obj) const
    { return obj.item_class == ITEM_CLASS_CONSUMABLE && obj.item_subclass == CLASS; }
  };

  struct gem_filter_t
  {
    // No "other" gems, nor relicn
    bool operator()( const dbc_item_data_t& obj ) const
    { return obj.item_class == ITEM_CLASS_GEM && obj.item_subclass != 9 && obj.item_subclass != 11; }
  };

  // Potions need their own filter unfortunately, because some potions are of sub class 8 (other)
  struct potion_filter_t
  {
    bool operator()( const dbc_item_data_t& obj ) const
    {
      return obj.item_class == ITEM_CLASS_CONSUMABLE &&
             ( obj.item_subclass == ITEM_SUBCLASS_POTION ||
               obj.item_subclass == ITEM_SUBCLASS_CONSUMABLE_OTHER );
    }
  };

  dbc::filtered_dbc_index_t<dbc_item_data_t, potion_filter_t> potion_data_index;
  dbc::filtered_dbc_index_t<dbc_item_data_t, consumable_filter_t<ITEM_SUBCLASS_FLASK>> flask_data_index;
  dbc::filtered_dbc_index_t<dbc_item_data_t, consumable_filter_t<ITEM_SUBCLASS_FOOD>> food_data_index;
  dbc::filtered_dbc_index_t<dbc_item_data_t, gem_filter_t> gem_index;
}

/* Initialize item database
 */
void dbc::init_item_data()
{
  // Create id-indexes
  potion_data_index.init( dbc_item_data_t::data( false ), false );
  flask_data_index.init( dbc_item_data_t::data( false ), false );
  food_data_index.init( dbc_item_data_t::data( false ), false );
  gem_index.init( dbc_item_data_t::data( false ), false );
#if SC_USE_PTR
  potion_data_index.init( dbc_item_data_t::data( true ), true );
  flask_data_index.init( dbc_item_data_t::data( true ), true );
  food_data_index.init( dbc_item_data_t::data( true ), true );
  gem_index.init( dbc_item_data_t::data( true ), true );
#endif
}

std::pair<const curve_point_t*, const curve_point_t*> dbc_t::curve_point( unsigned curve_id, double value ) const
{
  auto data = curve_point_t::find( curve_id, ptr );

  const curve_point_t * lower_bound = nullptr;
  const curve_point_t * upper_bound = nullptr;
  for ( const auto& point : data )
  {
    assert( point.curve_id == curve_id );
    if ( point.primary1 <= value )
    {
      lower_bound = &( point );
    }

    if ( point.primary1 >= value )
    {
      upper_bound = &( point );
      break;
    }
  }

  if ( lower_bound == nullptr )
  {
    lower_bound = upper_bound;
  }

  if ( upper_bound == nullptr )
  {
    upper_bound = lower_bound;
  }

  return std::pair<const curve_point_t*, const curve_point_t*>(
      lower_bound, upper_bound );
}

double item_database::curve_point_value( dbc_t& dbc, unsigned curve_id, double point_value )
{
  auto curve_data = dbc.curve_point( curve_id, point_value );

  double scaled_result = 0;
  // Lands on a value, use data
  if ( curve_data.first->primary1 == point_value )
  {
    scaled_result = curve_data.first->primary2;
  }
  // Below lower bound, use lower bound value
  else if ( curve_data.first->primary1 > point_value )
  {
    scaled_result = curve_data.first->primary2;
  }
  // Above upper bound, use upper bound value
  else if ( curve_data.second->primary1 < point_value )
  {
    scaled_result = curve_data.second->primary2;
  }
  else if ( curve_data.second->primary1 == point_value )
  {
    // Should never happen
    assert( 0 );
  }
  else
  {
    // Linear interpolation
    scaled_result = curve_data.first->primary2 + ( curve_data.second->primary2 - curve_data.first->primary2 ) *
      ( point_value - curve_data.first->primary1 ) / ( curve_data.second->primary1 - curve_data.first->primary1 );
  }

  return scaled_result;
}

// TODO: Needs some way to figure what value to pass, for now presume min of player level, max
// level. Also presumes we are only scaling itemlevel for now, this is almost certainly not 100%
// true in all cases for the use of curve data.
void item_database::apply_item_scaling( item_t& item, unsigned curve_id, unsigned player_level )
{
  // No scaling needed
  if ( curve_id == 0 )
  {
    return;
  }

  auto curve_data = curve_point_t::find( curve_id, item.player->dbc->ptr );
  if ( curve_data.empty() )
  {
    return;
  }

  unsigned base_value = std::min( player_level, as<unsigned>( curve_data.back().primary1 ) );

  double scaled_result = curve_point_value( *item.player->dbc, curve_id, base_value );

  if ( item.sim -> debug )
  {
    item.sim -> print_debug( "{}: Scaling {} from ilevel {} to {} ({:.3f}) using player level {}",
        item.player -> name(), item.name(), item.parsed.data.level,
        static_cast<unsigned>( util::round( scaled_result, 0 ) ),
        scaled_result, base_value );
  }

  item.parsed.data.level = static_cast<unsigned>( util::round( scaled_result, 0 ) );
}

bool item_database::apply_item_bonus( item_t& item, const item_bonus_entry_t& entry )
{
  switch ( entry.type )
  {
    // Adjust ilevel, value is in 'value_1' field
    case ITEM_BONUS_ILEVEL:
      // Blizzard has currently unknown means to disable adjust ilevel item bonus on
      // items. Currently, disable them on when bonus IDs 7215 or 7250 are present.
      // TODO: This appears to be happening because of some kind of priority system for
      // item bonuses where these are applied last, overriding any other ajustments.
      if ( range::find( item.parsed.bonus_id, 7215 ) != item.parsed.bonus_id.end() ||
           range::find( item.parsed.bonus_id, 7250 ) != item.parsed.bonus_id.end() )
      {
        break;
      }

      if ( item.sim -> debug )
      {
        item.player -> sim -> print_debug( "Player {} item '{}' adjusting ilevel by {} (old={} new={})",
            item.player -> name(), item.name(), entry.value_1, item.parsed.data.level, item.parsed.data.level + entry.value_1 );
      }
      item.parsed.data.level += entry.value_1;
      break;
    // Add new item stats. Value_1 has the item modifier, value_2 has the
    // allocation percent. This bonus type is the reason we don't really
    // support the bonus id stuff outside of our local item database.
    case ITEM_BONUS_MOD:
    {
      // Check that Simulationcraft understands the stat the Blizzard's item bonus is proposing to
      // add to the item
      auto simc_stat_type = util::translate_item_mod( entry.value_1 );
      if ( simc_stat_type == STAT_NONE )
      {
        break;
      }

      // First, check if the item already has that stat
      int found = -1;
      int offset = -1;
      for ( size_t i = 0, end = range::size( item.parsed.data.stat_type_e ); i < end; i++ )
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
          item.player -> sim -> print_debug( "Player {} item '{}' adding new stat {} offset={} (allocation {})",
              item.player -> name(), item.name(), util::translate_item_mod( entry.value_1 ), offset, entry.value_2 );
        item.parsed.data.stat_type_e[ offset ] = entry.value_1;
        item.parsed.data.stat_alloc[ offset ] = entry.value_2;
      }
      // Existing stat, add more allocation percent
      else if ( found != -1 )
      {
        if ( item.sim -> debug )
          item.player -> sim -> print_debug( "Player {} item '{}' adding existing stat {} offset={} (allocation {})",
              item.player -> name(), item.name(), util::translate_item_mod( entry.value_1 ), found , entry.value_2 );
        item.parsed.data.stat_alloc[ found ] += entry.value_2;
      }
      // New stat but no room, this should never happen.
      else
      {
        item.player -> sim -> error( "Player {} item '{}' unable to add item modifier, stats full", item.player -> name(), item.name() );
        return false;
      }
      break;
    }
    case ITEM_BONUS_QUALITY:
      if ( item.sim -> debug )
      {
        item.player -> sim -> print_debug( "Player {} item '{}' adjusting quality (old={} new={})",
            item.player -> name(), item.name(), util::item_quality_string( item.parsed.data.quality ),
            util::item_quality_string( entry.value_1 ) );
      }
      item.parsed.data.quality = entry.value_1;
      break;
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
        item.player -> sim -> print_debug( "Player {} item '{}' adjusting required level by {} (old={} new={})",
            item.player -> name(), item.name(), entry.value_1, item.parsed.data.req_level, item.parsed.data.req_level + entry.value_1 );
      item.parsed.data.req_level += entry.value_1;
      break;
    // Number of sockets is in value 1, type (color) of sockets is in value 2
    case ITEM_BONUS_SOCKET:
    {
      if ( item.sim -> debug )
        item.player -> sim -> print_debug( "Player {} item '{}' adding {} socket(s) (type={})",
            item.player -> name(), item.name(), entry.value_1, entry.value_2 );
      int n_added = 0;
      for ( size_t i = 0, end = range::size( item.parsed.data.socket_color ); i < end && n_added < entry.value_1; i++ )
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
    // This is backed up by some unknown (to us) client data at the moment. Just hardcode the values
    // based on the given bonus IDs, and hope for the best. Also note that this bonus id type
    // statically uses the current character's level to do the ilevel scaling (differs from
    // ITEM_BONUS_SCALING_2).
    case ITEM_BONUS_SCALING:
    {
      item_database::apply_item_scaling( item, entry.value_4, item.player -> level() );
      break;
    }
    // This bonus type uses a curve point to scale the base item level, however it seems to also
    // require that there's a drop level associated with the item. Drop levels are included in
    // itemstrings nowadays for items where they matter.
    //
    // Note that relics do seem to also include item bonuses that use this bonus type, but item
    // string information associated with a relic does not give out drop level for the relic. Thus,
    // Simulationcraft will scale relics purely by using the (seemingly often) present item level
    // adjustment bonus type.
    case ITEM_BONUS_SCALING_2:
      if ( item.parsed.drop_level > 0 )
      {
        item_database::apply_item_scaling( item, entry.value_4, item.parsed.drop_level );
      }
      break;
    case ITEM_BONUS_ADD_ITEM_EFFECT:
    {
      auto effect = item_effect_t::find( entry.value_1, item.player->dbc->ptr );
      if ( effect.id == 0 )
      {
        item.player->sim->error( "Player {} item '{}' unknown item effect id {}, ignoring",
            item.player->name(), item.name(), entry.value_1 );
        return true;
      }

      auto spell = item.player->find_spell( effect.spell_id );
      if ( spell->id() == 0 )
      {
        item.player->sim->error( "Player {} item '{}' unknown spell id {} for item effect id {}, ignoring",
            item.player->name(), item.name(), effect.spell_id, entry.value_1 );
        return true;
      }

      const size_t index = item.parsed.data.add_effect( effect );
      item.player->sim->print_debug( "Player {} item '{}' adding effect {} (type={}, index={})",
          item.player->name(), item.name(), effect.spell_id,
          util::item_spell_trigger_string( static_cast<item_spell_trigger_type>( effect.type ) ),
          index );
      break;
    }
    case ITEM_BONUS_MOD_ITEM_STAT:
    {
      if ( entry.value_1 == 0 )
      {
        return true;
      }

      auto stat_type = util::translate_item_mod( entry.value_1 );
      if ( stat_type == STAT_NONE )
      {
        item.player->sim->error( "Player {} item '{}' unknown modified stat type {}, ignoring",
            item.player->name(), item.name(), entry.value_1 );
        return true;
      }

      for ( size_t i = 0, end = range::size( item.parsed.data.stat_type_e ); i < end; i++ )
      {
        if ( item_database::is_crafted_item_mod( item.parsed.data.stat_type_e[ i ] ) )
        {
          item.player->sim->print_debug( "Player {} item '{}' modifying stat type to '{}' (index={})",
              item.player->name(), item.name(), util::stat_type_abbrev( stat_type ), i );
          item.parsed.data.stat_type_e[ i ] = entry.value_1;
          break;
        }
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
  else if ( enchantment.ench_type[ index ] == ITEM_ENCHANTMENT_RESISTANCE )
    return stat_pair_t( STAT_BONUS_ARMOR, enchantment.ench_amount[ index ] );

  return stat_pair_t();
}

stat_pair_t item_database::item_enchantment_effect_stats( player_t* player,
                                                          const item_enchantment_data_t& enchantment,
                                                          int index )
{
  assert( enchantment.id );
  assert( index >= 0 && index < 3 );

  if ( enchantment.ench_type[ index ] != ITEM_ENCHANTMENT_STAT &&
       enchantment.ench_type[ index ] != ITEM_ENCHANTMENT_RESISTANCE )
    return stat_pair_t();

  stat_e stat;
  if ( enchantment.ench_type[ index ] == ITEM_ENCHANTMENT_RESISTANCE )
    stat = STAT_BONUS_ARMOR;
  else
    stat = util::translate_item_mod( enchantment.ench_prop[ index ] );

  double value = 0;
  // First try to figure out the stat enchant through the spell that is associated with it (e.g.,
  // the enchant spell)
  if ( enchantment.id_spell > 0 )
  {
    auto spell = player -> find_spell( enchantment.id_spell );
    for ( size_t effect_idx = 1, end = spell -> effect_count(); effect_idx <= end; ++effect_idx )
    {
      const auto& effect = spell -> effectN( effect_idx );
      // Use the first Enchant Item effect found in the spell
      if ( effect.type() == E_ENCHANT_ITEM )
      {
        value = util::round( effect.average( player ) );
        break;
      }
    }
  }

  // If we cannot find any value through spell data, revert back to using item enchantment data
  if ( value == 0 )
  {
    if ( enchantment.id_scaling == 0 )
      value = enchantment.ench_amount[ index ];
    else
    {
      unsigned level = player -> level();

      if ( static_cast< unsigned >( level ) > enchantment.max_scaling_level )
        level = enchantment.max_scaling_level;

      double budget = player -> dbc->spell_scaling( static_cast< player_e >( enchantment.id_scaling ), level );
      value = util::round( budget * enchantment.ench_coeff[ index ] );
    }
  }

  if ( stat != STAT_NONE && value != 0 )
  {
    return stat_pair_t( stat, (int)value );
  }

  return stat_pair_t();
}

std::string item_database::stat_to_str( int stat, int stat_amount )
{
  stat_e s = util::translate_item_mod( stat );
  if ( stat_amount && s != STAT_NONE )
  {
    return util::tokenize_fn( fmt::format( "{}{}", stat_amount, util::stat_type_abbrev( s ) ) );
  }

  return {};
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

int item_database::scaled_stat( const item_t& item, const dbc_t& dbc, size_t idx, unsigned new_ilevel )
{
  // Safeguard against array overflow, should never happen in any case
  if ( idx >= range::size( item.parsed.data.stat_type_e ) - 1 )
    return -1;

  if ( item.parsed.data.level == 0 )
    return item.parsed.stat_val[ idx ];

  // 8.3 introduces "corruption" related stats that are not scaled by ilevel or anything.
  // They are applied (for now) through item bonuses, and do not use the normal "stat
  // alocation" scheme.
  if ( item.parsed.data.stat_type_e[ idx ] == ITEM_MOD_CORRUPTION ||
       item.parsed.data.stat_type_e[ idx ] == ITEM_MOD_CORRUPTION_RESISTANCE )
  {
    return item.parsed.data.stat_alloc[ idx ];
  }

  //if ( item.level == ( int ) new_ilevel )
  //  return item.stat_val[ idx ];

  int slot_type = random_suffix_type( item.parsed.data );
  double item_budget = 0/*, orig_budget = 0*/;

  if ( slot_type != -1 && item.parsed.data.quality > 0 )
  {
    const random_prop_data_t& ilevel_data = dbc.random_property( new_ilevel );
    //const random_prop_data_t& orig_data = dbc.random_property( item.level );

    // Epic/Legendary
    if ( item.parsed.data.quality == 4 || item.parsed.data.quality == 5 )
    {
      item_budget = ilevel_data.p_epic[ slot_type ];
      //orig_budget = orig_data.p_epic[ slot_type ];
    }
    // Rare/Heirloom
    else if ( item.parsed.data.quality == 3 || item.parsed.data.quality == 7 )
    {
      item_budget = ilevel_data.p_rare[ slot_type ];
      //orig_budget = orig_data.p_rare[ slot_type ];
    }
    // Rest
    else
    {
      item_budget = ilevel_data.p_uncommon[ slot_type ];
      //orig_budget = orig_data.p_uncommon[ slot_type ];
    }
  }

  // Precise stat scaling formula for ilevel increase, stats should be
  // spot on.
  if ( item.parsed.data.stat_alloc[ idx ] > 0 /* && orig_budget > 0 */ && item_budget > 0 )
  {
    // Socket penalty is supposedly gone in Warlords of Draenor, but it really does not seem so in
    // the latest alpha.  NOTENOTENOTENOTE: Item socket cost penalty multiplier _seems_ to be based
    // on _BASE_ itemlevel, not the upgraded one
    auto socket_mul = []( const dbc_item_data_t& item, size_t idx ) {
      if ( idx < item._dbc_stats_count )
        return item._dbc_stats[ idx ].socket_mul;
      return 0.0f;
    };
    double v_socket_penalty = socket_mul( item.parsed.data, idx ) * dbc.item_socket_cost( item.base_item_level() );
    double v_raw = item.parsed.data.stat_alloc[ idx ] * item_budget * 0.0001 - v_socket_penalty;
    auto stat_type = static_cast<item_mod_type>( item.parsed.data.stat_type_e[ idx ] );

    if ( util::is_combat_rating( stat_type ) )
    {
      v_raw = apply_combat_rating_multiplier( item, v_raw );
    }
    else if ( stat_type == ITEM_MOD_STAMINA )
    {
      v_raw = apply_stamina_multiplier( item, v_raw );
    }

    return static_cast<int>( util::round( v_raw ) );
  }
  // TODO(?): Should we warn the user that we are using an approximation of
  // the upgraded stats, and that certain stats may be off by one?
  else
    return static_cast<int>( floor( item.parsed.stat_val[ idx ] *
                                    approx_scale_coefficient( item.parsed.data.level, new_ilevel ) ) );
}

// item_database_t::initialize_item_sources =================================

bool item_database::initialize_item_sources( item_t& item, std::vector<std::string>& source_list )
{
  source_list = item.sim -> item_db_sources;

  if ( ! item.option_data_source_str.empty() )
  {
    auto item_sources_split = util::string_split<util::string_view>( item.option_data_source_str, ":/|" );

    source_list.clear();

    for ( auto split : item_sources_split )
    {
      if ( ! util::str_compare_ci( split, "local" ) &&
           ! util::str_compare_ci( split, "wowhead" ) &&
           ! util::str_compare_ci( split, "ptrhead" ) &&
           ! util::str_compare_ci( split, "bcpapi" ) )
      {
        continue;
      }
      source_list.emplace_back( util::tokenize_fn( split ) );
    }

    if ( source_list.empty() )
    {
      return false;
    }
  }

  return true;
}

// item_database_t::random_suffix_type ======================================

int item_database::random_suffix_type( const dbc_item_data_t& item )
{
  switch ( item.item_class )
  {
    case ITEM_CLASS_WEAPON:
      switch ( item.item_subclass )
      {
        case ITEM_SUBCLASS_WEAPON_AXE2:
        case ITEM_SUBCLASS_WEAPON_MACE2:
        case ITEM_SUBCLASS_WEAPON_POLEARM:
        case ITEM_SUBCLASS_WEAPON_SWORD2:
        case ITEM_SUBCLASS_WEAPON_STAFF:
        case ITEM_SUBCLASS_WEAPON_GUN:
        case ITEM_SUBCLASS_WEAPON_BOW:
        case ITEM_SUBCLASS_WEAPON_CROSSBOW:
        case ITEM_SUBCLASS_WEAPON_THROWN:
          return 0;
        default:
          return 3;
      }
    case ITEM_CLASS_ARMOR:
      switch ( item.inventory_type )
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
        case INVTYPE_FINGER:
        case INVTYPE_CLOAK:
        case INVTYPE_WRISTS:
          return 2;

        case INVTYPE_WEAPONOFFHAND:
        case INVTYPE_HOLDABLE:
        case INVTYPE_SHIELD:
          return 3;

        default:
          return -1;
      }
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
      case WEAPON_THROWN:
        return 0;

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
    case SLOT_BACK:
      return 2;

    case SLOT_OFF_HAND: // Shields, off hand items
      return 3;

    default:
      return -1;
  }
}

uint32_t item_database::armor_value( const dbc_item_data_t& item, const dbc_t& dbc, unsigned item_level )
{
  if ( item.id == 0 || item.quality > 5 )
    return 0;

  unsigned ilevel = item_level ? item_level : item.level;

  // Shield have separate armor table, bypass normal calculation
  if ( item.item_class == ITEM_CLASS_ARMOR && item.item_subclass == ITEM_SUBCLASS_ARMOR_SHIELD )
    return ( uint32_t ) floor( dbc.item_armor_shield( ilevel ).value( item.quality ) + 0.5 );

  // Only Cloth, Leather, Mail and Plate armor has innate armor values
  if ( item.item_subclass == ITEM_SUBCLASS_ARMOR_MISC || item.item_subclass > ITEM_SUBCLASS_ARMOR_PLATE )
    return 0;

  double m_invtype = 0;
  double m_quality = 0;
  double total_armor = 0;

  switch ( item.inventory_type )
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
      total_armor = dbc.item_armor_total( ilevel ).value( item.item_subclass - 1 );
      m_quality   = dbc.item_armor_quality( ilevel ).value( item.quality );
      unsigned invtype = item.inventory_type;
      if ( invtype == INVTYPE_ROBE ) invtype = INVTYPE_CHEST;
      m_invtype = dbc.item_armor_inv_type( invtype ).value( item.item_subclass - 1 );
      break;
    }
    default: return 0;
  }

  return ( uint32_t ) floor( total_armor * m_quality * m_invtype + 0.5 );
}

// item_database_t::armor_value =============================================

uint32_t item_database::armor_value( const item_t& item )
{
  return armor_value( item.parsed.data, *item.player->dbc, item.item_level() );
}

// item_database_t::weapon_dmg_min/max ======================================

uint32_t item_database::weapon_dmg_min( item_t& item )
{
  return ( uint32_t ) floor( item.player->dbc->weapon_dps( item.parsed.data, item.item_level() ) *
                             item.parsed.data.delay / 1000.0 * ( 1 - item.parsed.data.dmg_range / 2 ) );
}

uint32_t item_database::weapon_dmg_min( const dbc_item_data_t& item, const dbc_t& dbc, unsigned item_level )
{
  return ( uint32_t ) floor( dbc.weapon_dps( item, item_level ) * item.delay / 1000.0 *
                             ( 1 - item.dmg_range / 2 ) );
}

uint32_t item_database::weapon_dmg_max( const dbc_item_data_t& item, const dbc_t& dbc, unsigned item_level )
{
  return ( uint32_t ) ceil( dbc.weapon_dps( item, item_level ) * item.delay / 1000.0 *
                            ( 1 + item.dmg_range / 2 ) + 0.5 );
}

uint32_t item_database::weapon_dmg_max( item_t& item )
{
  return ( uint32_t ) ceil( item.player->dbc->weapon_dps( item.parsed.data, item.item_level() ) *
                            item.parsed.data.delay / 1000.0 * ( 1 + item.parsed.data.dmg_range / 2 ) + 0.5 );
}

bool item_database::parse_item_spell_enchant( item_t& item,
                                              std::vector<stat_pair_t>& stats,
                                              special_effect_t& effect,
                                              unsigned enchant_id )
{
  if ( enchant_id == 0 ) return true;

  const item_enchantment_data_t& item_enchant = item.player -> dbc->item_enchantment( enchant_id );
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
      const spell_data_t* es = item.player -> find_spell( item_enchant.ench_prop[ i ] );

      // Handle All stats enchants
      if ( es )
      {
        for ( const spelleffect_data_t& ed : es -> effects() )
        {
          // All stats is indicated by a misc value of -1
          if ( ed.type() == E_APPLY_AURA &&
               ed.subtype() == A_MOD_STAT &&
               ed.misc_value1() == -1 )
          {
            stats.emplace_back( STAT_ALL, static_cast<int>( ed.average( item.player ) ) );
            break;
          }
        }
      }

      // Kludge the rest
      if ( dbc_name.find( '$' ) != dbc_name.npos || ( es && es -> id() > 0 ) )
      {
        if ( es && es -> id() > 0 )
        {
          enchant_effect = es -> name_cstr(); // Use Spell Name

          const auto& spell_text = item.player->dbc->spell_text( es -> id() );
          if ( spell_text.rank() != nullptr ) // If rank str is available, append its number to the enchant name
          {
            std::string rank = std::string( spell_text.rank() );
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
  const auto& data = item.player->dbc->item( item.parsed.data.id );
  if ( data.id == 0 ) return false;

  item.parsed.data.init( data, *item.player->dbc );
  item.name_str = data.name;
  item.parsed.data.name = item.name_str.c_str();

  util::tokenize( item.name_str );

  // Apply azerite level to item level conversion first, but only for Blizzard API sourced profiles
  if ( item.player->profile_source_ == profile_source::BLIZZARD_API )
  {
    if ( item.parsed.azerite_level > 0 )
    {
      item.parsed.data.level = item.player->dbc->azerite_item_level( item.parsed.azerite_level );
    }
  }

  // Apply any azerite powers that apply bonus ids
  for ( auto power_id : item.parsed.azerite_ids )
  {
    const auto& power = item.player->dbc->azerite_power( power_id );
    if ( power.id == 0 || power.bonus_id == 0 )
    {
      continue;
    }

    auto it = range::find( item.parsed.bonus_id, power.bonus_id );
    if ( it == item.parsed.bonus_id.end() )
    {
      item.parsed.bonus_id.push_back( power.bonus_id );
    }
  }

  // Item bonus for local source only. TODO: BCP API and Wowhead will need ..
  // something similar
  for ( size_t i = 0, end = item.parsed.bonus_id.size(); i < end; i++ )
  {
    auto item_bonuses = item.player->dbc->item_bonus( item.parsed.bonus_id[ i ] );
    // Apply bonuses
    for ( const auto& bonus : item_bonuses )
    {
      if ( !apply_item_bonus( item, bonus ) )
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

double item_database::item_budget( const player_t* player, unsigned ilevel )
{
  if ( player == nullptr )
  {
    return 0.0;
  }

  const random_prop_data_t& budget = player -> dbc->random_property( ilevel );

  // Since we don't know the quality of the "item", just return the uncommon item budget. The
  // budgets have been the same for all qualities for a long while, but this function will break if
  // this ever changes.
  return budget.p_uncommon[ 0 ];
}

double item_database::item_budget( const item_t* item, unsigned max_ilevel )
{
  double m_scale = 0;

  unsigned scaling_level = item -> item_level();
  if ( max_ilevel > 0 )
    scaling_level = std::min( scaling_level, max_ilevel );

  const random_prop_data_t& budget = item -> player -> dbc->random_property( scaling_level );

  if ( item -> parsed.data.quality >= 4 )
    m_scale = budget.p_epic[ 0 ];
  else if ( item -> parsed.data.quality == 3 )
    m_scale = budget.p_rare[ 0 ];
  else
    m_scale = budget.p_uncommon[ 0 ];

  return m_scale;
}

// item_database::parse_tokens ==============================================

std::vector<item_database::token_t> item_database::parse_tokens( util::string_view encoded_str )
{
  std::vector<token_t> tokens;
  auto splits = util::string_split<util::string_view>( encoded_str, "_" );

  tokens.reserve( splits.size() );
  for ( auto& split : splits )
  {
    token_t t ;
    t.full = std::string(split);
    int index = 0;
    while ( t.full[ index ] != '\0' &&
            t.full[ index ] != '%'  &&
            ! std::isalpha( static_cast<unsigned char>( t.full[ index ] ) ) ) index++;
    if ( index == 0 )
    {
      t.name = t.full;
      t.value = 0;
    }
    else
    {
      t.name = t.full.substr( index );
      t.value_str = t.full.substr( 0, index );
      t.value = std::stod( t.value_str );
    }
    tokens.push_back( std::move(t) );
  }

  return tokens;
}

const dbc_item_data_t& dbc::find_gem( util::string_view gem, bool ptr, bool tokenized )
{
  return gem_index.get( ptr, [&gem, tokenized]( const dbc_item_data_t* obj ) {
      if ( tokenized )
      {
        return util::tokenize_fn( obj->name ) == gem;
      }
      else
      {
        return gem == obj->name;
      }
  } );
}

const dbc_item_data_t& dbc::find_consumable( item_subclass_consumable type, bool ptr, const std::function<bool(const dbc_item_data_t*)>& f )
{
  switch ( type )
  {
    case ITEM_SUBCLASS_POTION:
      return potion_data_index.get( ptr, f );
    case ITEM_SUBCLASS_FLASK:
      return flask_data_index.get( ptr, f );
    case ITEM_SUBCLASS_FOOD:
      return food_data_index.get( ptr, f );
    default:
      return dbc_item_data_t::nil();
  }
}

static std::string get_bonus_id_desc( const dbc_t& dbc, util::span<const item_bonus_entry_t> entries )
{
  for ( size_t i = 0; i < entries.size(); ++i )
  {
    if ( entries[ i ].type == ITEM_BONUS_DESC )
    {
      return dbc.item_description( entries[ i ].value_1 ).description;
    }
  }

  return std::string();
}

static std::string get_bonus_id_suffix( const dbc_t& dbc, util::span<const item_bonus_entry_t> entries )
{
  for ( size_t i = 0; i < entries.size(); ++i )
  {
    if ( entries[ i ].type == ITEM_BONUS_SUFFIX )
    {
      return dbc.item_description( entries[ i ].value_1 ).description;
    }
  }

  return std::string();
}

static std::pair<std::pair<int, double>, std::pair<int, double> > get_bonus_id_scaling( const dbc_t& dbc, util::span<const item_bonus_entry_t> entries )
{
  for ( size_t i = 0; i < entries.size(); ++i )
  {
    if ( entries[ i ].value_4 == 0 )
    {
      continue;
    }

    if ( entries[ i ].type == ITEM_BONUS_SCALING || entries[ i ].type == ITEM_BONUS_SCALING_2 )
    {
      auto curve_data = curve_point_t::find( entries[ i ].value_4, dbc.ptr );
      if ( curve_data.empty() )
      {
        return std::pair<std::pair<int, double>, std::pair<int, double> >(
            std::pair<int, double>( -1, 0 ),
            std::pair<int, double>( -1, 0 ) );
      }

      auto curve_data_min = dbc.curve_point( entries[ i ].value_4, curve_data.front().primary1 );
      auto curve_data_max = dbc.curve_point( entries[ i ].value_4, curve_data.back().primary2 );
      assert(curve_data_min.first);
      assert(curve_data_max.first);

      return std::pair<std::pair<int, double>, std::pair<int, double>>(
          std::pair<int, double>( as<int>( curve_data.front().primary1 ), curve_data_min.first->primary2 ),
          std::pair<int, double>( as<int>( curve_data.back().primary1 ), curve_data_max.first->primary2 ) );
    }
  }

  return std::pair<std::pair<int, double>, std::pair<int, double> >(
      std::pair<int, double>( -1, 0 ),
      std::pair<int, double>( -1, 0 ) );
}

static int get_bonus_id_ilevel( util::span<const item_bonus_entry_t> entries )
{
  for ( size_t i = 0; i < entries.size(); ++i )
  {
    if ( entries[ i ].type == ITEM_BONUS_ILEVEL )
    {
      return entries[ i ].value_1;
    }
  }

  return 0;
}

static int get_bonus_id_base_ilevel( util::span<const item_bonus_entry_t> entries )
{
  for ( size_t i = 0; i < entries.size(); ++i )
  {
    if ( entries[ i ].type == ITEM_BONUS_SET_ILEVEL )
    {
      return entries[ i ].value_1;
    }
  }

  return 0;
}

static std::string get_bonus_id_quality( util::span<const item_bonus_entry_t> entries )
{
  for ( auto& entry : entries )
  {
    if ( entry.type == ITEM_BONUS_QUALITY )
    {
      return util::item_quality_string( entry.value_1 );
    }
  }

  return "";
}

static int get_bonus_id_sockets( util::span<const item_bonus_entry_t> entries )
{
  for ( size_t i = 0; i < entries.size(); ++i )
  {
    if ( entries[ i ].type == ITEM_BONUS_SOCKET )
    {
      return entries[ i ].value_1;
    }
  }

  return 0;
}

static int get_bonus_power_index( util::span<const item_bonus_entry_t> entries )
{
  auto it = range::find_if( entries, []( const item_bonus_entry_t& entry ) {
    return entry.type == ITEM_BONUS_ADD_RANK;
  } );

  if ( it != entries.end() )
  {
    return ( *it ).value_1;
  }

  return -1;
}

static std::string get_bonus_item_effect( util::span<const item_bonus_entry_t> entries, const dbc_t& dbc )
{
  std::vector<std::string> entries_str;

  range::for_each( entries, [&entries_str, &dbc]( const item_bonus_entry_t& entry ) {
    if ( entry.type != ITEM_BONUS_ADD_ITEM_EFFECT )
    {
      return;
    }
    auto effect = item_effect_t::find( entry.value_1, dbc.ptr );
    if ( effect.id == 0 )
    {
      entries_str.emplace_back( fmt::format( "Unknown Effect (id={})", entry.value_1 ) );
    }
    else
    {
      auto spell = dbc.spell( effect.spell_id );
      if ( !spell )
      {
        entries_str.emplace_back( fmt::format( "Unknown Spell (id={}, index={})",
          effect.spell_id, effect.index ) );
      }
      else
      {
        std::vector<std::string> fields;
        fields.emplace_back( fmt::format( "id={}", spell->id() ) );
        fields.emplace_back( fmt::format( "index={}", effect.index ) );
        fields.emplace_back( fmt::format( "type={}",
          util::item_spell_trigger_string( static_cast<item_spell_trigger_type>( effect.type ) ) ) );
        if ( effect.cooldown_duration > 1 )
        {
          fields.emplace_back( fmt::format( "cooldown={}ms", effect.cooldown_duration ) );
        }

        if ( effect.cooldown_group_duration > 1 )
        {
          fields.emplace_back( fmt::format( "group_cooldown={}ms", effect.cooldown_group_duration ) );
        }

        entries_str.emplace_back( fmt::format( "{} ({})", spell->name_cstr(),
          fmt::join( fields, ", " ) ) );
      }
    }
  } );

  return util::string_join( entries_str, ", " );
}

static std::vector< std::tuple< item_mod_type, double, double > > get_bonus_id_stats(
    util::span<const item_bonus_entry_t> entries )
{
  double total = 0;

  for ( size_t i = 0; i < entries.size(); ++i )
  {
    if ( entries[ i ].type == ITEM_BONUS_MOD )
    {
      total += static_cast<double>( entries[ i ].value_2 );
    }
  }

  std::vector<std::tuple<item_mod_type, double, double> > data;
  for ( size_t i = 0; i < entries.size(); ++i )
  {
    if ( entries[ i ].type == ITEM_BONUS_MOD )
    {
      if ( entries[ i ].value_1 != ITEM_MOD_CORRUPTION &&
           entries[ i ].value_1 != ITEM_MOD_CORRUPTION_RESISTANCE )
      {
        data.emplace_back(
              static_cast<item_mod_type>( entries[ i ].value_1 ),
              entries[ i ].value_2 / total,
              entries[ i ].value_2 / 10000.0
        );
      }
      else
      {
        data.emplace_back(
              static_cast<item_mod_type>( entries[ i ].value_1 ),
              entries[ i ].value_2 / total,
              entries[ i ].value_2
        );
      }
    }
  }

  return data;
}

static std::string get_bonus_mod_stat( util::span<const item_bonus_entry_t> entries )
{
  fmt::memory_buffer b;

  for ( size_t i = 0; i < entries.size(); ++i )
  {
    if ( entries[ i ].type != ITEM_MOD_BONUS_STAT_1 &&
         entries[ i ].type != ITEM_MOD_BONUS_STAT_2 )
    {
      continue;
    }

    if ( entries[ i ].value_1 == 0 )
    {
      continue;
    }

    if ( b.size() > 0 )
    {
      fmt::format_to( b, ", " );
    }

    fmt::format_to( b, "{} ({})",
        util::stat_type_abbrev( util::translate_item_mod( entries[ i ].value_1 ) ),
        entries[ i ].value_2 );

  }

  return fmt::to_string( b );
}

std::string dbc::bonus_ids_str( const dbc_t& dbc )
{
  std::vector<unsigned> bonus_ids;
  fmt::memory_buffer s;

  for ( const auto& e : item_bonus_entry_t::data( dbc.ptr ) )
  {
    if ( std::find( bonus_ids.begin(), bonus_ids.end(), e.bonus_id ) != bonus_ids.end() )
    {
      continue;
    }

    // Need at least one "relevant" type for us
    if ( e.type != ITEM_BONUS_ILEVEL && e.type != ITEM_BONUS_MOD &&
         e.type != ITEM_BONUS_SOCKET && e.type != ITEM_BONUS_SCALING &&
         e.type != ITEM_BONUS_SCALING_2 && e.type != ITEM_BONUS_SET_ILEVEL &&
         e.type != ITEM_BONUS_ADD_RANK && e.type != ITEM_BONUS_QUALITY &&
         e.type != ITEM_BONUS_ADD_ITEM_EFFECT && e.type != ITEM_BONUS_MOD_ITEM_STAT )
    {
      continue;
    }

    if ( e.type == ITEM_BONUS_ILEVEL && e.value_1 == 0 )
    {
      continue;
    }

    bonus_ids.push_back( e.bonus_id );
  }

  range::sort( bonus_ids );

  for ( size_t i = 0; i < bonus_ids.size(); ++i )
  {
    const auto entries = dbc.item_bonus( bonus_ids[ i ] );
    std::string desc = get_bonus_id_desc( dbc, entries );
    std::string suffix = get_bonus_id_suffix( dbc, entries );
    std::string quality = get_bonus_id_quality( entries );
    int ilevel = get_bonus_id_ilevel( entries );
    int sockets = get_bonus_id_sockets( entries );
    int base_ilevel = get_bonus_id_base_ilevel( entries );
    auto stats = get_bonus_id_stats( entries );
    std::pair< std::pair<int, double>, std::pair<int, double> > scaling = get_bonus_id_scaling( dbc, entries );
    auto power_index = get_bonus_power_index( entries );
    std::string item_effects = get_bonus_item_effect( entries, dbc );
    auto item_mod_stat = get_bonus_mod_stat( entries );

    std::vector<std::string> fields;

    fields.push_back( "bonus_id={ " + util::to_string( bonus_ids[ i ] ) + " }" );
    if ( base_ilevel != 0 )
    {
      fields.push_back( "base_ilevel={ " + util::to_string( base_ilevel ) + " }" );
    }

    if ( ! desc.empty() )
    {
      fields.push_back( "desc={ " + desc + " }" );
    }

    if ( ! suffix.empty() )
    {
      fields.push_back( "suffix={ " + suffix + " }" );
    }

    if ( ! quality.empty() )
    {
      fields.push_back( "quality={ " + quality + " }" );
    }

    if ( ilevel != 0 )
    {
      fields.push_back( "ilevel_adjust={ " + util::to_string( ilevel ) + " }" );
    }

    if ( sockets > 0 )
    {
      fields.push_back( "socket={ " + util::to_string( sockets ) + " }" );
    }

    if ( power_index > -1 )
    {
      fields.push_back( "add_rank={ power_index=" + util::to_string( power_index ) + " }" );
    }

    if ( !stats.empty() )
    {
      std::string stats_str = "stats={ ";
      for ( size_t j = 0; j < stats.size(); ++j )
      {
        stats_str += util::to_string( util::round( std::get<1>( stats[ j ] ) * 100.0, 0 ) ) + "% ";
        stats_str += util::stat_type_abbrev( util::translate_item_mod( std::get<0>( stats[ j ] ) ) );
        stats_str += " [" + util::to_string( util::round( std::get<2>( stats[ j ] ), 4 ), 4 ) + "]";
        if ( j < stats.size() - 1 )
        {
          stats_str += ", ";
        }
      }
      fields.push_back( stats_str + " }" );
    }

    if ( scaling.first.first >= 0 )
    {
      std::string str = "ilevel={ ";

      str += util::to_string( scaling.first.second ) + " @plvl " + util::to_string( scaling.first.first );
      str += " - ";
      str += util::to_string( scaling.second.second ) + " @plvl " + util::to_string( scaling.second.first );
      str += " }";
      fields.push_back( str );
    }

    if ( !item_effects.empty() )
    {
      fields.emplace_back( fmt::format( "effects={{ {} }}", item_effects ) );
    }

    if ( !item_mod_stat.empty() )
    {
      fields.emplace_back( fmt::format( "mod_to_stat={{ {} }}", item_mod_stat ) );
    }

    fmt::format_to( s, "{}\n", fmt::join( fields, ", " ) );
  }

  return to_string( s );
}

unsigned dbc_t::child_item( unsigned id ) const
{
  if ( id == 0 )
  {
    return 0;
  }

  for ( const auto& entry : item_child_equipment_t::data( ptr ) )
  {
    if ( id == entry.id_item )
    {
      return entry.id_child;
    }
  }

  return 0;
}

unsigned dbc_t::parent_item( unsigned id ) const
{
  if ( id == 0 )
  {
    return 0;
  }

  for ( const auto& entry : item_child_equipment_t::data( ptr ) )
  {
    if ( id == entry.id_child )
    {
      return entry.id_item;
    }
  }

  return 0;
}

bool item_database::has_item_bonus_type( const item_t& item, item_bonus_type bonus )
{
  // For all parsed bonus ids ..
  //
  return range::any_of( item.parsed.bonus_id, [ &item, bonus ]( int bonus_id ) {
    auto bonuses = item.player -> dbc->item_bonus( bonus_id );
    // If there's a bonus id of type bonus, return true
    return range::find_if( bonuses, [ bonus ]( const item_bonus_entry_t& entry ) {
      return entry.type == bonus;
    } ) != bonuses.end();
  } );
}

double item_database::apply_combat_rating_multiplier( const player_t*               player,
                                                      combat_rating_multiplier_type type,
                                                      unsigned                      ilevel,
                                                      double                        raw_amount )
{
  auto combat_rating_multiplier = player -> dbc->combat_rating_multiplier( ilevel, type );
  if ( combat_rating_multiplier != 0 )
  {
    return raw_amount * combat_rating_multiplier;
  }

  return raw_amount;
}

double item_database::apply_stamina_multiplier( const player_t*               player,
                                                combat_rating_multiplier_type type,
                                                unsigned                      ilevel,
                                                double                        raw_amount )
{
  auto multiplier = player -> dbc->stamina_multiplier( ilevel, type );
  if ( multiplier != 0 )
  {
    return raw_amount * multiplier;
  }

  return raw_amount;
}

double item_database::apply_combat_rating_multiplier( const item_t& item, double amount )
{
  auto type = item_combat_rating_type( item.parsed.data );
  if ( type == CR_MULTIPLIER_INVALID )
  {
    return amount;
  }

  return apply_combat_rating_multiplier( item.player, type, item.item_level(), amount );
}

double item_database::apply_stamina_multiplier( const item_t& item, double amount )
{
  auto type = item_combat_rating_type( item.parsed.data );
  if ( type == CR_MULTIPLIER_INVALID )
  {
    return amount;
  }

  return apply_stamina_multiplier( item.player, type, item.item_level(), amount );
}

combat_rating_multiplier_type item_database::item_combat_rating_type( const dbc_item_data_t& data )
{
  switch ( data.inventory_type )
  {
    case INVTYPE_NECK:
    case INVTYPE_FINGER:
      return CR_MULTIPLIER_JEWLERY;
    case INVTYPE_TRINKET:
      return CR_MULTIPLIER_TRINKET;
    case INVTYPE_WEAPON:
    case INVTYPE_2HWEAPON:
    case INVTYPE_WEAPONMAINHAND:
    case INVTYPE_WEAPONOFFHAND:
    case INVTYPE_RANGED:
    case INVTYPE_RANGEDRIGHT:
    case INVTYPE_THROWN:
      return CR_MULTIPLIER_WEAPON;
    case INVTYPE_ROBE:
    case INVTYPE_HEAD:
    case INVTYPE_SHOULDERS:
    case INVTYPE_CHEST:
    case INVTYPE_CLOAK:
    case INVTYPE_BODY:
    case INVTYPE_WRISTS:
    case INVTYPE_WAIST:
    case INVTYPE_LEGS:
    case INVTYPE_FEET:
    case INVTYPE_SHIELD:
    case INVTYPE_HOLDABLE:
    case INVTYPE_HANDS:
      return CR_MULTIPLIER_ARMOR;
    default:
      return CR_MULTIPLIER_INVALID;
  }
}

void item_database::convert_stat_values( item_t& item )
{
  if ( item.parsed.data.quality < 2 || item.parsed.data.quality > 7 )
  {
    return;
  }

  auto slot_type = item_database::random_suffix_type( item.parsed.data );
  if ( slot_type == -1 )
  {
    return;
  }

  const auto& ilevel_data = item.player -> dbc->random_property( item.parsed.data.level );
  double budget = 0.0;
  if ( item.parsed.data.quality == 4 || item.parsed.data.quality == 5 )
  {
    budget = ilevel_data.p_epic[ slot_type ];
  }
  // Rare/Heirloom
  else if ( item.parsed.data.quality == 3 || item.parsed.data.quality == 7 )
  {
    budget = ilevel_data.p_rare[ slot_type ];
  }
  // Rest
  else
  {
    budget = ilevel_data.p_uncommon[ slot_type ];
  }

  for ( size_t i = 0; i < item.parsed.stat_val.size() && item.parsed.stat_val[ i ] > 0; ++i )
  {
    double cr_coeff = 1.0;
    if ( util::is_combat_rating( static_cast<item_mod_type>( item.parsed.data.stat_type_e[ i ] ) ) )
    {
      auto item_cr_type = item_database::item_combat_rating_type( item.parsed.data );
      if ( item_cr_type != CR_MULTIPLIER_INVALID )
      {
        cr_coeff = item.player -> dbc->combat_rating_multiplier( item.parsed.data.level,
            item_cr_type );
      }
    }

    double alloc = ( item.parsed.stat_val[ i ] / cr_coeff ) * 10000.0 / budget;
    item.parsed.data.stat_alloc[ i ] = static_cast<int>( alloc );
    item.parsed.stat_val[ i ] = 0;
  }
}
