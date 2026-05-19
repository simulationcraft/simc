// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "enchants.hpp"

#include "dbc/dbc.hpp"
#include "dbc/gem_data.hpp"
#include "dbc/item_database.hpp"
#include "dbc/permanent_enchant.hpp"
#include "dbc/spell_item_enchantment.hpp"
#include "item/item.hpp"
#include "player/player.hpp"
#include "player/unique_gear.hpp"
#include "sim/sim.hpp"
#include "special_effect.hpp"

//using namespace enchant;

// TODO: Check if Power Torrent, Jade Spirit share buffs, or allow separate

namespace /* ANONYMOUS NAMESPACE */
{
struct enchant_db_item_t
{
  const char* enchant_name;
  unsigned enchant_id;
};

/**
 * Additional translation mappings for enchant names. We use shorthands for
 * certain enchants (listed here), that cannot be deduced from the item
 * enchantment name in DBC. We can add additional shortcuts here, where
 * applicable. The enchant ID (number) maps to the array of
 * item_enchantment_data_t structs in sc_item_data.inc.
 */
static const enchant_db_item_t __enchant_db[] = {
  { "incandescent_essence", 7052 }
};

unsigned find_override_enchant_id( util::string_view name )
{
  for ( const auto& enchant_entry : __enchant_db )
  {
    if ( !enchant_entry.enchant_name )
    {
      continue;
    }

    if ( util::str_compare_ci( enchant_entry.enchant_name, name ) )
    {
      return enchant_entry.enchant_id;
    }
  }

  return 0;
}

std::string find_override_enchant_name( unsigned enchant_id )
{
  for ( const auto& enchant_entry : __enchant_db )
  {
    if ( !enchant_entry.enchant_name )
    {
      continue;
    }

    if ( enchant_entry.enchant_id == enchant_id )
    {
      return enchant_entry.enchant_name;
    }
  }

  return {};
}
} /* ANONYMOUS NAMESPACE */

/**
 * Initialize an item enchant from DBC data.
 *
 * This function initializes a Blizzard SpellItemEnchantment.dbc entry so that
 * simc understands it. Blizzard uses item enchantmnts to represent Enchants,
 * Gems, and Tinkers, and this function handles all of the initialization.
 *
 * The function supports three types of information per item enchantment entry
 * 1) Stat enchants (will be translated into simc-understandable stats) that
 *    will be inserted into the given vector reference. Typically called by
 *    various decode_xx() functions in item_t.
 * 2) Passive aura enchants. Certain passive aura enchants are supported by the
 *    system out of the box. Currently, any primary stat, or rating granting
 *    passive aura will be translated directly into simc-understandable stat.
 *    TODO-WOD: Once we have "passive buff" support, we can offload the stat
 *    deduction to stat_buff_t, reducing code replication.
 * 3) Procs. Item enchantments use three relevant proc types for simc
 *    - Spell procs when an item is worn (EQUIP_SPELL)
 *    - Spell procs when a weapon lands a hit (COMBAT_SPELL)
 *    - On-use abilities (USE_SPELL)
 *    This function translates the relevant proc type information so that
 *    special_effect_t is initialized correctly, and then passes the rest of
 *    the proc initialization to the generalized initialize_special_effect()
 *    function in sc_unique_gear.cpp.
 */
void enchant::initialize_item_enchant( item_t& item, std::vector<stat_pair_t>& stats, special_effect_source_e source,
                                       const item_enchantment_data_t& enchant )
{
  // Profession check
  profession_e profession = util::translate_profession_id( enchant.req_skill );
  if ( profession != PROFESSION_NONE )
  {
    if ( item.player->profession[ profession ] < static_cast<int>( enchant.req_skill_value ) )
    {
      item.sim->error( "{} attempting to use {} '{}' with {} skill level of {}, requires {}.",
                       *item.player, util::special_effect_source_string( source ), enchant.name,
                       util::profession_type_string( profession ), item.player->profession[ profession ],
                       enchant.req_skill_value );
      // Don't initialize the special effects, but do "succeed" the
      // initialization process.
      return;
    }
  }

  if ( enchant.min_ilevel > 0 && item.item_level() < enchant.min_ilevel )
  {
    item.sim->error( "Player {} enchant '{}' (id={}) requires a minimum ilevel of {}, item '{}' has {}",
      item.player->name(), enchant.name, enchant.id, enchant.min_ilevel, item.name(), item.item_level() );
  }

  if ( enchant.max_ilevel > 0 && item.item_level() > enchant.max_ilevel )
  {
    item.sim->error( "Player {} enchant '{}' (id={}) requires a maximum ilevel of {}, item '{}' has {}",
      item.player->name(), enchant.name, enchant.id, enchant.max_ilevel, item.name(), item.item_level() );
  }

  for ( size_t i = 0; i < std::size( enchant.ench_prop ); i++ )
  {
    special_effect_t effect( &item );
    effect.source       = source;
    effect.enchant_data = &( enchant );
    switch ( enchant.ench_type[ i ] )
    {
      // "Chance on Hit", we need to help simc a bit with proc flags
      case ITEM_ENCHANTMENT_COMBAT_SPELL:
        // Require that "chance on hit" enchant effects are hit with the
        // correct weapon
        effect.weapon_proc = true;
        effect.type        = SPECIAL_EFFECT_EQUIP;
        effect.proc_flags_ = PF_MELEE | PF_MELEE_ABILITY | PF_RANGED | PF_RANGED_ABILITY;
        break;
      case ITEM_ENCHANTMENT_DAMAGE:
      {
        double value = 0;
        if ( enchant.id_scaling == 0 )
          value = enchant.ench_amount[ i ];
        else
        {
          unsigned level = item.player->level();

          if ( enchant.max_scaling_level != 0 && static_cast<unsigned>( level ) > enchant.max_scaling_level )
            level = enchant.max_scaling_level;

          double budget = item.player->dbc->spell_scaling( static_cast<player_e>( enchant.id_scaling ), level );
          value         = util::round( budget * enchant.ench_coeff[ i ] );
        }

        if ( value != 0 )
        {
          item.weapon()->min_dmg += value;
          item.weapon()->max_dmg += value;
          item.weapon()->dps += value / item.weapon()->swing_time.total_seconds();
          item.weapon()->damage += value;
        }
        break;
      }
      case ITEM_ENCHANTMENT_EQUIP_SPELL:
        // Passive enchants get a special treatment. In essence, we only support
        // a couple, and they are handled without special effect initialization
        // for now. Unfortunately, they do need to be added to the special
        // effect list, so we can output them in saved profiles as the enchant
        // name, instead of a bunch of stats.
        if ( passive_enchant( item, enchant.ench_prop[ i ] ) )
        {
          continue;
        }
        else
          effect.type = SPECIAL_EFFECT_EQUIP;
        break;
      case ITEM_ENCHANTMENT_USE_SPELL:
        // Use spell is also 0, which is the "default value" it seems .. sigh
        if ( enchant.ench_prop[ i ] == 0 )
          continue;
        effect.type = SPECIAL_EFFECT_USE;
        break;
      case ITEM_ENCHANTMENT_RESISTANCE:
      case ITEM_ENCHANTMENT_STAT:
      {
        stat_pair_t stat = item_database::item_enchantment_effect_stats( item.player, enchant, as<int>( i ) );
        if ( stat.stat != STAT_NONE && stat.value != 0 )
          stats.push_back( stat );
        break;
      }
      case ITEM_ENCHANTMENT_APPLY_BONUS:
      {
        // If the bonus is already there, assume that it does not need to be applied because
        // item strings may already contain the bonuses if the item was exported from the game.
        if ( range::find( item.parsed.bonus_id, enchant.ench_prop[ i ] ) == item.parsed.bonus_id.end() )
        {
          auto bonuses = item.player->dbc->item_bonus( enchant.ench_prop[ i ] );
          for ( auto& bonus : bonuses )
          {
            item_database::apply_item_bonus( item, bonus );
          }
        }
        break;
      }
      default:
        break;
    }

    // Prefer to use the actual tokenized permanent enchant name if we can find it from our local
    // permanent enchant client data export
    auto tokenized_name = encoded_enchant_name( *item.player->dbc, enchant );
    if ( !tokenized_name.empty() &&
         ( source == SPECIAL_EFFECT_SOURCE_ENCHANT || source == SPECIAL_EFFECT_SOURCE_ADDON ) )
    {
      item.parsed.encoded_enchant = tokenized_name;
    }

    // First phase initialize the spell effect
    if ( effect.type != SPECIAL_EFFECT_NONE )
    {
      unique_gear::initialize_special_effect( effect, enchant.ench_prop[ i ] );
    }

    if ( effect.type != SPECIAL_EFFECT_NONE )
    {
      item.parsed.special_effects.push_back( new special_effect_t( effect ) );
    }
  }
}

/**
 * Figure out if the ITEM_ENCHANT_EQUIP_SPELL is in fact a passive enchant.
 * Such enchants include any passive spells that apply a stat aura, for
 * example.
 *
 * TODO-WOD: For now, this needs to sync with stat_buff_t stats.
 */
bool enchant::passive_enchant( item_t& item, unsigned spell_id )
{
  bool ret                  = false;
  const spell_data_t* spell = item.player->find_spell( spell_id );
  if ( !spell->ok() )
    return ret;

  // Don't support ilevel-based calculations
  if ( spell->flags( spell_attribute::SX_SCALE_ILEVEL ) )
  {
    return ret;
  }

  if ( !spell->flags( spell_attribute::SX_PASSIVE ) && spell->duration() >= timespan_t::zero() )
    return ret;

  for ( size_t i = 1, end = spell->effect_count(); i <= end; i++ )
  {
    const spelleffect_data_t& effect = spell->effectN( i );

    if ( effect.type() != E_APPLY_AURA )
      continue;

    double value = util::round( effect.average( item.player ) );
    stat_e stat  = STAT_NONE;

    switch ( effect.subtype() )
    {
      case A_MOD_STAT:
        if ( effect.misc_value1() == -1 )
          stat = STAT_ALL;
        else if ( effect.misc_value1() == -2 )
          stat = item.player->convert_hybrid_stat( STAT_STR_AGI_INT );
        else
          stat = static_cast<stat_e>( effect.misc_value1() + 1 );
        break;
      case A_MOD_RATING:
        stat = util::translate_rating_mod( effect.misc_value1() );
        break;
      // Clump all movement speed enchants additively into "passive_movement_speed"
      case A_MOD_SPEED_ALWAYS:
        item.player->base.stacking_movement_speed_modifier += effect.percent();
        ret = true;
        break;
      default:
        break;
    }

    if ( stat != STAT_NONE && value != 0 )
    {
      item.parsed.enchant_stats.emplace_back( stat, (int)value );
      ret = true;
    }
  }
  return ret;
}

/**
 * Intialize gem, based on game client data.
 *
 * Gem stats will be set to item_t::parsed.gem_stats, and the gem color
 * returned. Actual initialization is performed through the unified
 * initialize_item_enchant() function.
 */
item_socket_color enchant::initialize_gem( item_t& item, size_t gem_idx )
{
  auto gem_id = item.parsed.gem_id[ gem_idx ];
  if ( gem_id == 0 )
  {
    return SOCKET_COLOR_NONE;
  }

  const auto& gem = item.player->dbc->item( gem_id );
  if ( gem.id == 0 )
  {
    throw sc_invalid_item_string( fmt::format( "No gem data for id '{}'.", gem_id ) );
  }

  const gem_property_data_t& gem_prop = item.player->dbc->gem_property( gem.gem_properties );
  if ( !gem_prop.id )
    return SOCKET_COLOR_NONE;

  // Relics get handled elsewhere, because they are .. complicated
  if ( gem_prop.color & SOCKET_COLOR_RELIC )
  {
    return initialize_relic( item, gem_idx, gem_prop );
  }

  if ( gem_prop.color == SOCKET_COLOR_RESHII_FIBER )
    remove_other_reshii_bonuses( item );

  const item_enchantment_data_t& data = item.player->dbc->item_enchantment( gem_prop.enchant_id );

  enchant::initialize_item_enchant( item, item.parsed.gem_stats, SPECIAL_EFFECT_SOURCE_GEM, data );

  if ( !dbc::valid_gem_color( gem_prop.color ) )
  {
    throw sc_invalid_item_string( fmt::format( "Invalid gem color {} from id '{}'.", gem_prop.color, gem_id ) );
  }

  return static_cast<item_socket_color>( gem_prop.color );
}

item_socket_color enchant::initialize_relic( item_t& item, size_t relic_idx, const gem_property_data_t& gem_property )
{
  if ( !item.parsed.data.id_artifact )
  {
    return SOCKET_COLOR_NONE;
  }

  const item_enchantment_data_t& data = item.player->dbc->item_enchantment( gem_property.enchant_id );
  if ( data.id != gem_property.enchant_id )
  {
    return SOCKET_COLOR_NONE;
  }

  auto relic_id          = item.parsed.gem_id[ relic_idx ];
  const auto& relic_data = item.player->dbc->item( relic_id );
  if ( relic_data.id == 0 )
  {
    return SOCKET_COLOR_NONE;
  }

  // Make a fake relic item and apply bonuses to it
  item_t relic( item.player, "" );

  // Apply base stats to relic
  relic.parsed.data.init( relic_data, *item.player->dbc );
  relic.name_str    = relic_data.name;
  util::tokenize( relic.name_str );

  // Apply evil relic data as relic bonus ids, so we can scale the relic ilevel correctly
  range::for_each( item.parsed.gem_bonus_id[ relic_idx ],
                   [ &relic ]( unsigned id ) { relic.parsed.bonus_id.push_back( as<int>( id ) ); } );

  for ( size_t i = 0, end = std::size( data.ench_type ); i < end; ++i )
  {
    switch ( data.ench_type[ i ] )
    {
      case ITEM_ENCHANTMENT_RELIC_EVIL:
      {
        for ( auto bonus_id : relic.parsed.bonus_id )
        {
          auto bonuses = item.player->dbc->item_bonus( bonus_id );
          range::for_each( bonuses, [ &relic ]( const item_bonus_entry_t& entry ) {
            item_database::apply_item_bonus( relic, entry );
          } );
        }
        break;
      }
      case ITEM_ENCHANTMENT_APPLY_BONUS:
      {
        auto bonuses = item.player->dbc->item_bonus( data.ench_prop[ i ] );
        range::for_each( bonuses, [ &relic ]( const item_bonus_entry_t& entry ) {
          item_database::apply_item_bonus( relic, entry );
        } );
        break;
      }
      case ITEM_ENCHANTMENT_RELIC_RANK:
      default:
        break;
    }
  }

  // The relic ilevel (after applying item bonuses) is stored in the parsed base item data.
  // User has overriden relic item level with the relic_ilevel option on the item.
  if ( item.parsed.gem_ilevel[ relic_idx ] > 0 )
  {
    relic.parsed.data.level = item.parsed.gem_ilevel[ relic_idx ];
  }

  // Then, use a (seemingly) hard-coded curve point to figure out a scaled value for the +item level
  // increase
  double ilevel_value =
      item_database::curve_point_value( *item.player->dbc, RELIC_ILEVEL_BONUS_CURVE, relic.parsed.data.level );

  item.player->sim->print_debug( "relic: {} ilevel_increase=+{}", relic, util::floor( ilevel_value ) );

  item.parsed.gem_actual_ilevel[ relic_idx ] = as<unsigned>( util::floor( ilevel_value ) );

  // 2018-06-20: Blizzard has added the relic ilevel increases as bonus ids to the actual weapon in
  // BfA
  // item.parsed.data.level += as<int>(util::floor( ilevel_value ));

  return SOCKET_COLOR_RELIC;
}

const item_enchantment_data_t& enchant::find_item_enchant(
    const item_t&     item,
    util::string_view name )
{
  util::string_view enchant_str = name;
  unsigned rank = 0;
  int item_class = -1;
  int inventory_type = -1;
  int item_subclass = -1;

  auto enchant_id = find_override_enchant_id( name );
  // Check additional mapping table first
  if ( enchant_id > 0 )
  {
    return item.player->dbc->item_enchantment( enchant_id );
  }

  auto it = name.rfind( '_' );
  if ( it != std::string_view::npos )
  {
    auto rank_str = name.substr( it + 1 );
    auto parsed_rank = util::to_unsigned_ignore_error( rank_str, std::numeric_limits<unsigned>::max() );

    if ( parsed_rank != std::numeric_limits<unsigned>::max() )
    {
      enchant_str = name.substr( 0, it );
      rank = parsed_rank;
    }
  }

  item_class = item.dbc_item_class();
  inventory_type = item.dbc_inventory_type();
  item_subclass = item.dbc_item_subclass();

  const auto& entry = permanent_enchant_entry_t::find( enchant_str, rank, item_class,
      inventory_type, item_subclass, item.player->dbc->ptr );
  if ( entry.enchant_id == 0 )
  {
    return item_enchantment_data_t::nil();
  }

  return item_enchantment_data_t::find( entry.enchant_id, item.player->dbc->ptr );
}

std::string enchant::encoded_enchant_name(
    const dbc_t&                   dbc,
    const item_enchantment_data_t& enchant )
{
  if ( enchant.id == 0 )
  {
    return {};
  }

  auto enchant_name_override = find_override_enchant_name( enchant.id );
  // Check additional mapping table first
  if ( !enchant_name_override.empty() )
  {
    return enchant_name_override;
  }

  const auto& entry = permanent_enchant_entry_t::find( enchant.id, dbc.ptr );
  if ( entry.enchant_id == 0 )
  {
    return {};
  }

  if ( entry.rank != 0 )
  {
    return fmt::format( "{}_{}", entry.tokenized_name, entry.rank );
  }
  else
  {
    return entry.tokenized_name;
  }
}

void enchant::remove_other_reshii_bonuses( item_t& item )
{
  const std::array<int, 8> reshii_bonus_ids = { 12255, 12256, 12257, 12258, 12259, 12260, 12261, 12262 };
  // Remove all Reshii bonus ids, as they can be additive with the gem parsing
  auto& item_bonus_ids = item.parsed.bonus_id;
  bool found           = false;

  for ( auto bonus_id : item_bonus_ids )
  {
    if ( range::find( reshii_bonus_ids, bonus_id ) != reshii_bonus_ids.end() )
    {
      item.sim->print_debug( "Removing Reshii Wraps Stat bonus id {} from item {}", bonus_id, item.name() );
      found = true;
    }
  }

  if ( found )
  {
    item_bonus_ids.erase( std::remove_if( item_bonus_ids.begin(), item_bonus_ids.end(),
                                          [ &reshii_bonus_ids ]( int bonus_id ) {
                                            return range::find( reshii_bonus_ids, bonus_id ) != reshii_bonus_ids.end();
                                          } ),
                          item_bonus_ids.end() );

    // Remove all Reshii stats, as they are additive with the gem parsing
    for ( size_t i = 0; i < item.parsed.data.stat_type_e.size(); i++ )
    {
      stat_e s = item.stat( i );
      switch ( s )
      {
        case STAT_STRENGTH:
        case STAT_AGILITY:
        case STAT_INTELLECT:
        case STAT_STAMINA:
        case STAT_STR_AGI_INT:
        case STAT_STR_AGI:
        case STAT_STR_INT:
        case STAT_AGI_INT:
          break;  // Do not remove these stats
        default:
          // Remove all other stats
          item.base_stats.add_stat( s, -item.parsed.stat_val[ i ] );
          item.stats.add_stat( s, -item.parsed.stat_val[ i ] );
          item.parsed.stat_val[ i ]         = 0;
          item.parsed.data.stat_type_e[ i ] = -1;
          item.parsed.data.stat_alloc[ i ]  = 0;
          break;
      }
    }
  }
}