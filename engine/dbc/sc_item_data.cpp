// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

#include "generated/sc_item_data2.inc"
#if SC_USE_PTR
#include "generated/sc_item_data_ptr2.inc"
#endif

namespace {
  template<typename T, item_subclass_consumable CLASS>
  struct consumable_filter_t
  {
    bool operator()(const T* obj) const
    { return obj -> item_class == ITEM_CLASS_CONSUMABLE && obj -> item_subclass == CLASS; }
  };

  struct gem_filter_t
  {
    // No "other" gems, nor relicn
    bool operator()( const item_data_t* obj ) const
    { return obj->item_class == ITEM_CLASS_GEM && obj->item_subclass != 9 && obj->item_subclass != 11; }
  };

  // Potions need their own filter unfortunately, because some potions are of sub class 8 (other)
  struct potion_filter_t
  {
    bool operator()( const item_data_t* obj ) const
    {
      return obj->item_class == ITEM_CLASS_CONSUMABLE &&
             ( obj->item_subclass == ITEM_SUBCLASS_POTION ||
               obj->item_subclass == ITEM_SUBCLASS_CONSUMABLE_OTHER );
    }
  };

  item_data_t nil_item_data;
  item_enchantment_data_t nil_ied;
  gem_property_data_t nil_gpd;
  dbc_index_t<item_enchantment_data_t, id_member_policy> item_enchantment_data_index;
  dbc_index_t<item_data_t, id_member_policy> item_data_index;

  typedef filtered_dbc_index_t<item_data_t, potion_filter_t, id_member_policy> potion_data_t;
  typedef filtered_dbc_index_t<item_data_t, consumable_filter_t<item_data_t, ITEM_SUBCLASS_FLASK>, id_member_policy> flask_data_t;
  typedef filtered_dbc_index_t<item_data_t, consumable_filter_t<item_data_t, ITEM_SUBCLASS_FOOD>, id_member_policy> food_data_t;

  potion_data_t potion_data_index;
  flask_data_t flask_data_index;
  food_data_t food_data_index;

  filtered_dbc_index_t<item_data_t, gem_filter_t, id_member_policy> gem_index;
}

const item_name_description_t* dbc::item_name_descriptions( bool ptr )
{
  const item_name_description_t* p = __item_name_description_data;
#if SC_USE_PTR
  if ( ptr )
    p = __ptr_item_name_description_data;
#else
  (void)ptr;
#endif

  return p;
}

std::size_t dbc::n_item_name_descriptions( bool ptr )
{
#if SC_USE_PTR
  if ( ptr )
  {
    return PTR_ITEM_NAME_DESCRIPTION_SIZE;
  }
  else
  {
    return ITEM_NAME_DESCRIPTION_SIZE;
  }
#else
  (void)ptr;
  return ITEM_NAME_DESCRIPTION_SIZE;
#endif
}

const item_bonus_entry_t* dbc::item_bonus_entries( bool ptr )
{
  const item_bonus_entry_t* p = __item_bonus_data;
#if SC_USE_PTR
  if ( ptr )
    p = __ptr_item_bonus_data;
#else
  (void)ptr;
#endif

  return p;
}

std::size_t dbc::n_item_bonuses( bool ptr )
{
#if SC_USE_PTR
  if ( ptr )
  {
    return PTR_ITEM_BONUS_SIZE;
  }
  else
  {
    return ITEM_BONUS_SIZE;
  }
#else
  (void)ptr;
  return ITEM_BONUS_SIZE;
#endif
}

const char* dbc::item_name_description( unsigned id, bool ptr )
{
  const item_name_description_t* p = __item_name_description_data;
#if SC_USE_PTR
  if ( ptr )
    p = __ptr_item_name_description_data;
#else
  ( void ) ptr;
#endif

  while ( p -> id != 0 )
  {
    if ( p -> id == id )
    {
      return p -> description;
    }

    p++;
  }

  return nullptr;
}
unsigned dbc_t::random_property_max_level() const
{
  int n = RAND_PROP_POINTS_SIZE;
#if SC_USE_PTR
  if ( ptr )
    n = PTR_RAND_PROP_POINTS_SIZE;
#endif
  assert( n > 0 );
  return as<unsigned>( n );
}

const random_prop_data_t& dbc_t::random_property( unsigned ilevel ) const
{
  static random_prop_data_t __default {};
  if ( ilevel < 1 || ilevel > MAX_ILEVEL )
  {
    return __default;
  }
#if SC_USE_PTR
  return ptr ? __ptr_rand_prop_points_data[ ilevel - 1 ] : __rand_prop_points_data[ ilevel - 1 ];
#else
  return __rand_prop_points_data[ ilevel - 1 ];
#endif
}

const item_scale_data_t& dbc_t::item_damage_1h( unsigned ilevel ) const
{
  static item_scale_data_t __default {};
  if ( ilevel < 1 || ilevel > MAX_ILEVEL )
  {
    return __default;
  }
#if SC_USE_PTR
  return ptr ? __ptr_itemdamageonehand_data[ ilevel - 1 ] : __itemdamageonehand_data[ ilevel - 1 ];
#else
  return __itemdamageonehand_data[ ilevel - 1 ];
#endif
}

const item_scale_data_t& dbc_t::item_damage_2h( unsigned ilevel ) const
{
  static item_scale_data_t __default {};
  if ( ilevel < 1 || ilevel > MAX_ILEVEL )
  {
    return __default;
  }
#if SC_USE_PTR
  return ptr ? __ptr_itemdamagetwohand_data[ ilevel - 1 ] : __itemdamagetwohand_data[ ilevel - 1 ];
#else
  return __itemdamagetwohand_data[ ilevel - 1 ];
#endif
}

const item_scale_data_t& dbc_t::item_damage_caster_1h( unsigned ilevel ) const
{
  static item_scale_data_t __default {};
  if ( ilevel < 1 || ilevel > MAX_ILEVEL )
  {
    return __default;
  }
#if SC_USE_PTR
  return ptr ? __ptr_itemdamageonehandcaster_data[ ilevel - 1 ] : __itemdamageonehandcaster_data[ ilevel - 1 ];
#else
  return __itemdamageonehandcaster_data[ ilevel - 1 ];
#endif
}

const item_scale_data_t& dbc_t::item_damage_caster_2h( unsigned ilevel ) const
{
  static item_scale_data_t __default {};
  if ( ilevel < 1 || ilevel > MAX_ILEVEL )
  {
    return __default;
  }
#if SC_USE_PTR
  return ptr ? __ptr_itemdamagetwohandcaster_data[ ilevel - 1 ] : __itemdamagetwohandcaster_data[ ilevel - 1 ];
#else
  return __itemdamagetwohandcaster_data[ ilevel - 1 ];
#endif
}

const item_scale_data_t& dbc_t::item_armor_quality( unsigned ilevel ) const
{
  static item_scale_data_t __default {};
  if ( ilevel < 1 || ilevel > MAX_ILEVEL )
  {
    return __default;
  }
#if SC_USE_PTR
  return ptr ? __ptr_itemarmorquality_data[ ilevel - 1 ] : __itemarmorquality_data[ ilevel - 1 ];
#else
  return __itemarmorquality_data[ ilevel - 1 ];
#endif
}

const item_scale_data_t& dbc_t::item_armor_shield( unsigned ilevel ) const
{
  static item_scale_data_t __default {};
  if ( ilevel < 1 || ilevel > MAX_ILEVEL )
  {
    return __default;
  }
#if SC_USE_PTR
  return ptr ? __ptr_itemarmorshield_data[ ilevel - 1 ] : __itemarmorshield_data[ ilevel - 1 ];
#else
  return __itemarmorshield_data[ ilevel - 1 ];
#endif
}

const item_armor_type_data_t& dbc_t::item_armor_total( unsigned ilevel ) const
{
  static item_armor_type_data_t __default {};
  if ( ilevel < 1 || ilevel > MAX_ILEVEL )
  {
    return __default;
  }
#if SC_USE_PTR
  return ptr ? __ptr_itemarmortotal_data[ ilevel - 1 ] : __itemarmortotal_data[ ilevel - 1 ];
#else
  return __itemarmortotal_data[ ilevel - 1 ];
#endif
}

std::vector<const item_bonus_entry_t*> dbc_t::item_bonus( unsigned bonus_id ) const
{
#if SC_USE_PTR
  const item_bonus_entry_t* p = ptr ? __ptr_item_bonus_data : __item_bonus_data;
#else
  const item_bonus_entry_t* p = __item_bonus_data;
#endif

  std::vector<const item_bonus_entry_t*> entries;

  while ( p -> id != 0 )
  {
    if ( p -> bonus_id == bonus_id )
      entries.push_back(p);
    p++;
  }

  return entries;
}

std::vector<const item_upgrade_t*> dbc_t::item_upgrades( unsigned item_id ) const
{
#if SC_USE_PTR
  const item_upgrade_rule_t* upgrade_data = ptr ? __ptr_item_upgrade_rule_data : __item_upgrade_rule_data;
  const item_upgrade_t* upgrade_data2 = ptr ? __ptr_upgrade_rule_data : __upgrade_rule_data,
         * upgrade_data3 = upgrade_data2;
#else
  const item_upgrade_rule_t* upgrade_data = __item_upgrade_rule_data;
  const item_upgrade_t* upgrade_data2 = __upgrade_rule_data, * upgrade_data3 = upgrade_data2;
#endif

  std::vector<const item_upgrade_t*> data;
  unsigned upgrade_group = 0;

  // Find the correct upgrade rule, so we can figure out the base upgrade id
  while ( upgrade_data -> id != 0 )
  {
    if ( upgrade_data -> item_id == item_id )
    {
      break;
    }

    upgrade_data++;
  }

  // Find the upgrade group from the upgrade id
  while ( upgrade_data2 -> id != 0 )
  {
    if ( upgrade_data2 -> id == upgrade_data -> upgrade_id )
    {
      upgrade_group = upgrade_data2 -> upgrade_group;
      break;
    }

    upgrade_data2++;
  }

  // Collect all upgrade levels into the vector in the upgrade group
  while ( upgrade_data3 -> id != 0 )
  {
    if ( upgrade_data3 -> upgrade_group == upgrade_group )
    {
      data.push_back( upgrade_data3 );
    }

    upgrade_data3++;
  }

  // Sort based on the previous upgrade id. The base will have 0, and subsequent levels the previous
  // level's id. Results in a 0->N ilevel upgrades in however many steps in the vector.
  range::sort( data, []( const item_upgrade_t* l, const item_upgrade_t* r )
      { return l -> previous_upgrade_id < r -> previous_upgrade_id; } );

  return data;
}

const item_enchantment_data_t& dbc_t::item_enchantment( unsigned enchant_id ) const
{
  if ( const item_enchantment_data_t* p = item_enchantment_data_index.get( maybe_ptr( ptr ), enchant_id ) )
    return *p;
  else
    return nil_ied;
}

const item_data_t* dbc_t::item( unsigned item_id ) const
{ return item_data_index.get( ptr, item_id ); }

const gem_property_data_t& dbc_t::gem_property( unsigned gem_id ) const
{
  ( void )ptr;

#if SC_USE_PTR
  const gem_property_data_t* p = ptr ? __ptr_gem_property_data : __gem_property_data;
#else
  const gem_property_data_t* p = __gem_property_data;
#endif

  do
  {
    if ( p -> id == gem_id )
      return *p;
  }
  while ( ( p++ ) -> id );

  return nil_gpd;
}

const item_data_t* dbc::items( bool ptr )
{
  ( void )ptr;

  const item_data_t* p = __items_noptr();
#if SC_USE_PTR
  if ( ptr )
    p = __items_ptr();
#endif
  return p;
}

size_t dbc::n_items( bool ptr )
{
  ( void )ptr;

  size_t n = n_items_noptr();
#if SC_USE_PTR
  if ( ptr )
    n = n_items_ptr();
#endif

  return n;
}

const item_enchantment_data_t* dbc::item_enchantments( bool ptr )
{
  ( void )ptr;

  const item_enchantment_data_t* p = __spell_item_ench_data;
#if SC_USE_PTR
  if ( ptr )
    p = __ptr_spell_item_ench_data;
#endif
  return p;
}

size_t dbc::n_item_enchantments( bool ptr )
{
  ( void )ptr;

  size_t n = SPELL_ITEM_ENCH_SIZE;
#if SC_USE_PTR
  if ( ptr )
    n = PTR_SPELL_ITEM_ENCH_SIZE;
#endif

  return n;
}

const gem_property_data_t* dbc::gem_properties( bool ptr )
{
  ( void )ptr;

  const gem_property_data_t* p = __gem_property_data;
#if SC_USE_PTR
  if ( ptr )
    p = __ptr_gem_property_data;
#endif
  return p;
}

item_bonus_tree_entry_t& dbc_t::resolve_item_bonus_tree_data( unsigned level ) const
{
    assert(level > 0 && level <= MAX_LEVEL);
#if SC_USE_PTR
    return ptr ? __ptr_item_bonus_tree_data[level - 1]
               : __item_bonus_tree_data[level - 1];
#else
    return __item_bonus_tree_data[level - 1];
#endif
}

item_bonus_node_entry_t& dbc_t::resolve_item_bonus_map_data( unsigned level ) const
{
    assert(level > 0 && level <= MAX_LEVEL);
#if SC_USE_PTR
    return ptr ? __ptr_item_bonus_map_data[level - 1] :
                 __item_bonus_map_data[level - 1];
#else
    return __item_bonus_map_data[level - 1];
#endif
}

const item_armor_type_data_t& dbc_t::item_armor_inv_type( unsigned inv_type ) const
{
  assert( inv_type > 0 && inv_type <= 23 );
#if SC_USE_PTR
  return ptr ? __ptr_armor_slot_data[ inv_type - 1 ] : __armor_slot_data[ inv_type - 1 ];
#else
  return __armor_slot_data[ inv_type - 1 ];
#endif
}

/* Initialize item database
 */
void dbc::init_item_data()
{
  // Create id-indexes
  item_data_index.init( __items_noptr(), false );
  item_enchantment_data_index.init( __spell_item_ench_data, false );
  potion_data_index.init( __items_noptr(), false );
  flask_data_index.init( __items_noptr(), false );
  food_data_index.init( __items_noptr(), false );
  gem_index.init( __items_noptr(), false );
#if SC_USE_PTR
  item_data_index.init( __items_ptr(), true );
  item_enchantment_data_index.init( __ptr_spell_item_ench_data, true );
  potion_data_index.init( __items_ptr(), true );
  flask_data_index.init( __items_ptr(), true );
  food_data_index.init( __items_ptr(), true );
  gem_index.init( __items_ptr(), true );
#endif
}

const scaling_stat_distribution_t* dbc_t::scaling_stat_distribution( unsigned id )
{
#if SC_USE_PTR
  const scaling_stat_distribution_t* table = ptr ? &__ptr_scaling_stat_distribution_data[ 0 ]
                                                 : &__scaling_stat_distribution_data[ 0 ];
#else
  const scaling_stat_distribution_t* table = &__scaling_stat_distribution_data[ 0 ];
#endif

  while ( table -> id != 0 )
  {
    if ( table -> id == id )
      return table;
    table++;
  }

  return nullptr;
}

std::pair<const curve_point_t*, const curve_point_t*> dbc_t::curve_point( unsigned curve_id, double value )
{
#if SC_USE_PTR
  const curve_point_t* table = ptr ? &__ptr_curve_point_data[ 0 ]
                                   : &__curve_point_data[ 0 ];
#else
  const curve_point_t* table = &__curve_point_data[ 0 ];
#endif

  const curve_point_t* lower_bound = nullptr, * upper_bound = nullptr;
  while ( table -> curve_id != 0 )
  {
    if ( table -> curve_id != curve_id )
    {
      table++;
      continue;
    }

    if ( table -> val1 <= value )
    {
      lower_bound = table;
    }

    if ( table -> val1 >= value )
    {
      upper_bound = table;
      break;
    }

    table++;
  }

  if ( lower_bound == nullptr )
  {
    lower_bound = upper_bound;
  }

  if ( upper_bound == nullptr )
  {
    upper_bound = lower_bound;
  }

  return std::pair<const curve_point_t*, const curve_point_t*>( lower_bound, upper_bound );
}

item_data_t* item_data_t::find( unsigned id, bool ptr )
{
  item_data_t* i = item_data_index.get( ptr, id );
  if ( ! i )
    return &nil_item_data;
  return i;
}

double item_database::curve_point_value( dbc_t& dbc, unsigned curve_id, double point_value )
{
  auto curve_data = dbc.curve_point( curve_id, point_value );

  double scaled_result = 0;
  // Lands on a value, use data
  if ( curve_data.first -> val1 == point_value )
  {
    scaled_result = curve_data.first -> val2;
  }
  // Below lower bound, use lower bound value
  else if ( curve_data.first -> val1 > point_value )
  {
    scaled_result = curve_data.first -> val2;
  }
  // Above upper bound, use upper bound value
  else if ( curve_data.second -> val1 < point_value )
  {
    scaled_result = curve_data.second -> val2;
  }
  else if ( curve_data.second -> val1 == point_value )
  {
    // Should never happen
    assert( 0 );
  }
  else
  {
    // Linear interpolation
    scaled_result = curve_data.first -> val2 + ( curve_data.second -> val2 - curve_data.first -> val2 ) *
      ( point_value - curve_data.first -> val1 ) / ( curve_data.second -> val1 - curve_data.first -> val1 );
  }

  return scaled_result;
}

// TODO: Needs some way to figure what value to pass, for now presume min of player level, max
// level. Also presumes we are only scaling itemlevel for now, this is almost certainly not 100%
// true in all cases for the use of curve data.
void item_database::apply_item_scaling( item_t& item, unsigned scaling_id, unsigned player_level )
{
  // No scaling needed
  if ( scaling_id == 0 )
  {
    return;
  }

  const scaling_stat_distribution_t* data = item.player -> dbc.scaling_stat_distribution( scaling_id );
  // Unable to find the scaling stat distribution
  if ( data == nullptr )
  {
    throw std::invalid_argument(fmt::format("Unable to find scaling information for {} scaling id {}.",
        item.name(), item.parsed.data.id_scaling_distribution));
  }

  // Player level lower than item minimum scaling level, shouldnt happen but let item init go
  // through
  if ( player_level < data -> min_level )
  {
    return;
  }

  unsigned base_value = std::min( player_level, data -> max_level );

  double scaled_result = curve_point_value( item.player -> dbc, data -> curve_id, base_value );

  if ( item.sim -> debug )
  {
    item.sim -> out_debug.printf( "%s: Scaling %s from ilevel %d to %u (%.3f) using player level %u",
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
      // Existing stat, add more allocation percent
      else if ( found != -1 )
      {
        if ( item.sim -> debug )
          item.player -> sim -> out_debug.printf( "Player %s item '%s' adding existing stat %s offset=%d (allocation %u)",
              item.player -> name(), item.name(), util::stat_type_string( util::translate_item_mod( entry.value_1 ) ), found , entry.value_2 );
        item.parsed.data.stat_alloc[ found ] += entry.value_2;
      }
      // New stat but no room, this should never happen.
      else
      {
        item.player -> sim -> errorf( "Player %s item '%s' unable to add item modifier, stats full", item.player -> name(), item.name() );
        return false;
      }
      break;
    }
    case ITEM_BONUS_QUALITY:
      if ( item.sim -> debug )
      {
        item.player -> sim -> out_debug.printf( "Player %s item '%s' adjusting quality (old=%s new=%s)",
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
        item.player -> sim -> out_debug.printf( "Player %s item '%s' adjusting required level by %d (old=%d new=%d)",
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
    // This is backed up by some unknown (to us) client data at the moment. Just hardcode the values
    // based on the given bonus IDs, and hope for the best. Also note that this bonus id type
    // statically uses the current character's level to do the ilevel scaling (differs from
    // ITEM_BONUS_SCALING_2).
    case ITEM_BONUS_SCALING:
    {
      item_database::apply_item_scaling( item, entry.value_1, item.player -> level() );
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
        item_database::apply_item_scaling( item, entry.value_1, item.parsed.drop_level );
      }
      break;
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

      double budget = player -> dbc.spell_scaling( static_cast< player_e >( enchantment.id_scaling ), level );
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

int item_database::scaled_stat( const item_t& item, const dbc_t& dbc, size_t idx, unsigned new_ilevel )
{
  // Safeguard against array overflow, should never happen in any case
  if ( idx >= sizeof_array( item.parsed.data.stat_type_e ) - 1 )
    return -1;

  if ( item.parsed.data.level == 0 )
    return item.parsed.stat_val[ idx ];

  //if ( item.level == ( int ) new_ilevel )
  //  return item.stat_val[ idx ];

  int slot_type = random_suffix_type( &item.parsed.data );
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
    double v_socket_penalty = item.parsed.data.stat_socket_mul[ idx ] *
                              dbc.item_socket_cost( item.base_item_level() );

    int v_raw = item.parsed.data.stat_alloc[ idx ] * item_budget * 0.0001 - v_socket_penalty + 0.5;
    auto stat_type = static_cast<item_mod_type>( item.parsed.data.stat_type_e[ idx ] );

    if ( util::is_combat_rating( stat_type ) )
    {
      v_raw = apply_combat_rating_multiplier( item, as<double>( v_raw ) );
    }
    else if ( stat_type == ITEM_MOD_STAMINA )
    {
      v_raw = apply_stamina_multiplier( item, as<double>( v_raw ) );
    }

    return v_raw;
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
        case ITEM_SUBCLASS_WEAPON_THROWN:
          return 0;
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

uint32_t item_database::armor_value( const item_t& item )
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
      const spell_data_t* es = item.player -> find_spell( item_enchant.ench_prop[ i ] );

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
            stats.push_back( stat_pair_t( STAT_ALL, static_cast<int>(es -> effectN( j + 1 ).average( item.player ) )) );
            break;
          }
        }
      }

      // Kludge the rest
      if ( dbc_name.find( "$" ) != dbc_name.npos || ( es && es -> id() > 0 && es -> rank_str() != nullptr ) )
      {
        if ( es && es -> id() > 0 )
        {
          enchant_effect = es -> name_cstr(); // Use Spell Name

          if ( es -> rank_str() != nullptr ) // If rank str is available, append its number to the enchant name
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

  // Apply azerite level to item level conversion first, but only for Blizzard API sourced profiles
  if ( item.player->profile_source_ == profile_source::BLIZZARD_API )
  {
    if ( item.parsed.azerite_level > 0 )
    {
      item.parsed.data.level = item.player->dbc.azerite_item_level( item.parsed.azerite_level );
    }
  }

  // Apply any azerite powers that apply bonus ids
  for ( auto power_id : item.parsed.azerite_ids )
  {
    const auto& power = item.player->dbc.azerite_power( power_id );
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
    auto item_bonuses = item.player -> dbc.item_bonus( item.parsed.bonus_id[ i ] );
    // Apply bonuses
    for ( const auto bonus : item_bonuses )
    {
      if ( ! apply_item_bonus( item, *bonus ) )
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

// item_database::upgrade_ilevel ============================================

// TODO: DBC Based upgrading system would be safer, this works for now, probably
unsigned item_database::upgrade_ilevel( const item_t& item, unsigned upgrade_level )
{
  if ( upgrade_level == 0 )
  {
    return 0;
  }

  std::vector<const item_upgrade_t*> upgrades = item.player->dbc.item_upgrades( item.parsed.data.id );
  if ( upgrades.size() < upgrade_level )
  {
    if ( item.player -> sim -> debug )
    {
      item.player -> sim -> out_debug.printf( "%s %s too high upgrade level specified, %u given, max is %u",
          item.player -> name(), item.name(), upgrade_level, upgrades.size() );
    }
    return 0;
  }

  return upgrades[ upgrade_level ] -> ilevel_delta;
}

double item_database::item_budget( const player_t* player, unsigned ilevel )
{
  if ( player == nullptr )
  {
    return 0.0;
  }

  const random_prop_data_t& budget = player -> dbc.random_property( ilevel );

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

std::vector<item_database::token_t> item_database::parse_tokens( const std::string& encoded_str )
{
  std::vector<token_t> tokens;
  std::vector<std::string> splits = util::string_split( encoded_str, "_" );

  tokens.reserve( splits.size() );
  for ( auto& split : splits )
  {
    token_t t ;
    t.full = split;
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
      t.value = std::stod( t.value_str.c_str() );
    }
    tokens.push_back( std::move(t) );
  }

  return tokens;
}

const item_data_t* dbc::find_gem( const std::string& gem, bool ptr, bool tokenized )
{
  const item_data_t* i = gem_index.get( ptr, [&gem, tokenized]( const item_data_t* obj ) {
      if ( tokenized )
      {
        std::string n = obj->name;
        util::tokenize( n );
        return n == gem;
      }
      else
      {
        return gem == obj->name;
      }
  } );

  return i ? i : &( nil_item_data );
}

const item_data_t* dbc::find_consumable( item_subclass_consumable type, bool ptr, const std::function<bool(const item_data_t*)>& f )
{
  const item_data_t* i = nullptr;
  switch ( type )
  {
    case ITEM_SUBCLASS_POTION:
      i = potion_data_index.get( ptr, f );
      break;
    case ITEM_SUBCLASS_FLASK:
      i = flask_data_index.get( ptr, f );
      break;
    case ITEM_SUBCLASS_FOOD:
      i = food_data_index.get( ptr, f );
      break;
    default:
      break;
  }

  return i ? i : &( nil_item_data );
}

static std::string get_bonus_id_desc( bool ptr, const std::vector<const item_bonus_entry_t*>& entries )
{
  for ( size_t i = 0; i < entries.size(); ++i )
  {
    if ( entries[ i ] -> type == ITEM_BONUS_DESC )
    {
      return dbc::item_name_description( entries[ i ] -> value_1, ptr );
    }
  }

  return std::string();
}

static std::string get_bonus_id_suffix( bool ptr, const std::vector<const item_bonus_entry_t*>& entries )
{
  for ( size_t i = 0; i < entries.size(); ++i )
  {
    if ( entries[ i ] -> type == ITEM_BONUS_SUFFIX )
    {
      return dbc::item_name_description( entries[ i ] -> value_1, ptr );
    }
  }

  return std::string();
}

static std::pair<std::pair<int, double>, std::pair<int, double> > get_bonus_id_scaling( dbc_t& dbc, const std::vector<const item_bonus_entry_t*>& entries )
{
  for ( size_t i = 0; i < entries.size(); ++i )
  {
    if ( entries[ i ] -> value_1 == 0 )
    {
      continue;
    }

    if ( entries[ i ] -> type == ITEM_BONUS_SCALING || entries[ i ] -> type == ITEM_BONUS_SCALING_2 )
    {
      const scaling_stat_distribution_t* data = dbc.scaling_stat_distribution( entries[ i ] -> value_1 );
      assert( data );
      std::pair<const curve_point_t*, const curve_point_t*> curve_data_min = dbc.curve_point( data -> curve_id, data -> min_level );
      std::pair<const curve_point_t*, const curve_point_t*> curve_data_max = dbc.curve_point( data -> curve_id, data -> max_level );
      assert(curve_data_min.first);
      assert(curve_data_max.first);

      return std::pair<std::pair<int, double>, std::pair<int, double> >(
          std::pair<int, double>( data -> min_level, curve_data_min.first -> val2 ),
          std::pair<int, double>( data -> max_level, curve_data_max.first -> val2 ) );
    }
  }

  return std::pair<std::pair<int, double>, std::pair<int, double> >( std::pair<int, double>( -1, 0 ), std::pair<int, double>( -1, 0 ) );
}

static int get_bonus_id_ilevel( const std::vector<const item_bonus_entry_t*>& entries )
{
  for ( size_t i = 0; i < entries.size(); ++i )
  {
    if ( entries[ i ] -> type == ITEM_BONUS_ILEVEL )
    {
      return entries[ i ] -> value_1;
    }
  }

  return 0;
}

static int get_bonus_id_base_ilevel( const std::vector<const item_bonus_entry_t*>& entries )
{
  for ( size_t i = 0; i < entries.size(); ++i )
  {
    if ( entries[ i ] -> type == ITEM_BONUS_SET_ILEVEL )
    {
      return entries[ i ] -> value_1;
    }
  }

  return 0;
}

static std::string get_bonus_id_quality( const std::vector<const item_bonus_entry_t*>& entries )
{
  for ( auto& entry : entries )
  {
    if ( entry -> type == ITEM_BONUS_QUALITY )
    {
      return util::item_quality_string( entry -> value_1 );
    }
  }

  return "";
}

static int get_bonus_id_sockets( const std::vector<const item_bonus_entry_t*>& entries )
{
  for ( size_t i = 0; i < entries.size(); ++i )
  {
    if ( entries[ i ] -> type == ITEM_BONUS_SOCKET )
    {
      return entries[ i ] -> value_1;
    }
  }

  return 0;
}

static int get_bonus_power_index( const std::vector<const item_bonus_entry_t*>& entries )
{
  auto it = range::find_if( entries, []( const item_bonus_entry_t* entry ) {
    return entry -> type == ITEM_BONUS_ADD_RANK;
  } );

  if ( it != entries.end() )
  {
    return ( *it ) -> value_1;
  }

  return -1;
}

static std::vector< std::tuple< item_mod_type, double, double > > get_bonus_id_stats(
    const std::vector<const item_bonus_entry_t*>& entries )
{
  double total = 0;

  for ( size_t i = 0; i < entries.size(); ++i )
  {
    if ( entries[ i ] -> type == ITEM_BONUS_MOD )
    {
      total += static_cast<double>( entries[ i ] -> value_2 );
    }
  }

  std::vector<std::tuple<item_mod_type, double, double> > data;
  for ( size_t i = 0; i < entries.size(); ++i )
  {
    if ( entries[ i ] -> type == ITEM_BONUS_MOD )
    {
      data.push_back( std::make_tuple(
            static_cast<item_mod_type>( entries[ i ] -> value_1 ),
            entries[ i ] -> value_2 / total,
            entries[ i ] -> value_2 / 10000.0
      ) );
    }
  }

  return data;
}

std::string dbc::bonus_ids_str( dbc_t& dbc)
{
  std::vector<unsigned> bonus_ids;
  std::stringstream s;

  const item_bonus_entry_t* e = dbc::item_bonus_entries( dbc.ptr );
  while ( e -> id != 0 )
  {
    if ( std::find( bonus_ids.begin(), bonus_ids.end(), e -> bonus_id ) != bonus_ids.end() )
    {
      e++;
      continue;
    }

    // Need at least one "relevant" type for us
    if ( e -> type != ITEM_BONUS_ILEVEL && e -> type != ITEM_BONUS_MOD &&
         e -> type != ITEM_BONUS_SOCKET && e -> type != ITEM_BONUS_SCALING &&
         e -> type != ITEM_BONUS_SCALING_2 && e -> type != ITEM_BONUS_SET_ILEVEL &&
         e -> type != ITEM_BONUS_ADD_RANK && e -> type != ITEM_BONUS_QUALITY )
    {
      e++;
      continue;
    }

    if ( e -> type == ITEM_BONUS_ILEVEL && e -> value_1 == 0 )
    {
      e++;
      continue;
    }

    bonus_ids.push_back( e -> bonus_id );

    e++;
  }

  range::sort( bonus_ids );

  for ( size_t i = 0; i < bonus_ids.size(); ++i )
  {
    std::vector<const item_bonus_entry_t*> entries = dbc.item_bonus( bonus_ids[ i ] );
    std::string desc = get_bonus_id_desc( dbc.ptr, entries );
    std::string suffix = get_bonus_id_suffix( dbc.ptr, entries );
    std::string quality = get_bonus_id_quality( entries );
    int ilevel = get_bonus_id_ilevel( entries );
    int sockets = get_bonus_id_sockets( entries );
    int base_ilevel = get_bonus_id_base_ilevel( entries );
    auto stats = get_bonus_id_stats( entries );
    std::pair< std::pair<int, double>, std::pair<int, double> > scaling = get_bonus_id_scaling( dbc, entries );
    auto power_index = get_bonus_power_index( entries );

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

    if ( stats.size() > 0 )
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

    for ( size_t j = 0; j < fields.size(); ++j )
    {
      s << fields[ j ];
      if ( j < fields.size() - 1 )
      {
        s << ", ";
      }
    }

    s << std::endl;
  }

  return s.str();
}

const item_child_equipment_t* dbc::child_equipments( bool ptr )
{
#if SC_USE_PTR
  const item_child_equipment_t* p = ptr ? __ptr_item_child_equipment_data : __item_child_equipment_data;
#else
  ( void ) ptr;
  const item_child_equipment_t* p = __item_child_equipment_data;
#endif

  return p;
}

unsigned dbc_t::child_item( unsigned id ) const
{
  const item_child_equipment_t* p = dbc::child_equipments( ptr );
  if ( id == 0 )
  {
    return 0;
  }

  while ( p -> id != 0 )
  {
    if ( id == p -> id_item )
    {
      return p -> id_child;
    }

    p++;
  }

  return 0;
}

unsigned dbc_t::parent_item( unsigned id ) const
{
  const item_child_equipment_t* p = dbc::child_equipments( ptr );
  if ( id == 0 )
  {
    return 0;
  }

  while ( p -> id != 0 )
  {
    if ( id == p -> id_child )
    {
      return p -> id_item;
    }

    p++;
  }

  return 0;
}

bool item_database::has_item_bonus_type( const item_t& item, item_bonus_type bonus )
{
  // For all parsed bonus ids ..
  //
  auto it = range::find_if( item.parsed.bonus_id, [ &item, bonus ]( int bonus_id ) {
    auto bonuses = item.player -> dbc.item_bonus( bonus_id );
    // If there's a bonus id of type bonus, return true
    return range::find_if( bonuses, [ bonus ]( const item_bonus_entry_t* entry ) {
      return entry -> type == bonus;
    } ) != bonuses.end();
  } );

  return it != item.parsed.bonus_id.end();
}

double item_database::apply_combat_rating_multiplier( const player_t*               player,
                                                      combat_rating_multiplier_type type,
                                                      unsigned                      ilevel,
                                                      double                        raw_amount )
{
  auto combat_rating_multiplier = player -> dbc.combat_rating_multiplier( ilevel, type );
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
  auto multiplier = player -> dbc.stamina_multiplier( ilevel, type );
  if ( multiplier != 0 )
  {
    return raw_amount * multiplier;
  }

  return raw_amount;
}

double item_database::apply_combat_rating_multiplier( const item_t& item, double amount )
{
  auto type = item_combat_rating_type( &item.parsed.data );
  if ( type == CR_MULTIPLIER_INVALID )
  {
    return amount;
  }

  return apply_combat_rating_multiplier( item.player, type, item.item_level(), amount );
}

double item_database::apply_stamina_multiplier( const item_t& item, double amount )
{
  auto type = item_combat_rating_type( &item.parsed.data );
  if ( type == CR_MULTIPLIER_INVALID )
  {
    return amount;
  }

  return apply_stamina_multiplier( item.player, type, item.item_level(), amount );
}

combat_rating_multiplier_type item_database::item_combat_rating_type( const item_data_t* data )
{
  switch ( data -> inventory_type )
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

  auto slot_type = item_database::random_suffix_type( &item.parsed.data );
  if ( slot_type == -1 )
  {
    return;
  }

  const auto& ilevel_data = item.player -> dbc.random_property( item.parsed.data.level );
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
      auto item_cr_type = item_database::item_combat_rating_type( &item.parsed.data );
      if ( item_cr_type != CR_MULTIPLIER_INVALID )
      {
        cr_coeff = item.player -> dbc.combat_rating_multiplier( item.parsed.data.level,
            item_cr_type );
      }
    }

    double alloc = ( item.parsed.stat_val[ i ] / cr_coeff ) * 10000.0 / budget;
    item.parsed.data.stat_alloc[ i ] = static_cast<int>( alloc );
    item.parsed.stat_val[ i ] = 0;
  }
}
