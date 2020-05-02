// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sim/unique_gear.hpp"
#include "simulationcraft.hpp"

using namespace unique_gear;

bool class_scoped_callback_t::valid( const special_effect_t& effect ) const
{
  assert( effect.player );

  if ( class_.size() > 0 && range::find( class_, effect.player -> type ) == class_.end() )
  {
    return false;
  }

  if ( spec_.size() > 0 && range::find( spec_, effect.player -> specialization() ) == spec_.end() )
  {
    return false;
  }

  return true;
}

void proc_attack_t::override_data( const special_effect_t& e )
{
  super::override_data(e);

  if ( ( e.override_result_es_mask & RESULT_DODGE_MASK ) )
  {
    this -> may_dodge = e.result_es_mask & RESULT_DODGE_MASK;
  }

  if ( ( e.override_result_es_mask & RESULT_PARRY_MASK ) )
  {
    this -> may_parry = e.result_es_mask & RESULT_PARRY_MASK;
  }
}

/**
 * Initialize the proc callback. This method is called by each actor through
 * player_t::register_callbacks(), which is invoked as the last thing in the
 * actor initialization process.
 */
void dbc_proc_callback_t::initialize()
{
  listener -> sim -> print_debug( "Initializing proc: {}", effect );

  // Initialize proc chance triggers. Note that this code only chooses one, and
  // prioritizes RPPM > PPM > proc chance.
  if ( effect.rppm() > 0 && effect.rppm_scale() != RPPM_DISABLE )
  {
    rppm = listener -> get_rppm( effect.name(), effect.rppm(), effect.rppm_modifier(), effect.rppm_scale() );
    rppm->set_blp_state( static_cast<real_ppm_t::blp>( effect.rppm_blp_ ) );
  }
  else if ( effect.ppm() > 0 )
    ppm = effect.ppm();
  else if ( effect.proc_chance() != 0 )
    proc_chance = effect.proc_chance();

  // Initialize cooldown, if applicable
  if ( effect.cooldown() > timespan_t::zero() )
  {
    cooldown = listener -> get_cooldown( effect.cooldown_name() );
    cooldown -> duration = effect.cooldown();
  }

  // Initialize proc action
  proc_action = effect.create_action();

  // Initialize the potential proc buff through special_effect_t. Can return 0,
  // in which case the proc does not trigger a buff.
  proc_buff = effect.create_buff();

  if ( effect.weapon_proc && effect.item )
  {
    weapon = effect.item -> weapon();
  }

  // Register callback to new proc system
  listener -> callbacks.register_callback( effect.proc_flags(), effect.proc_flags2(), this );
}

/**
 * Cooldown name needs some special handling. Cooldowns in WoW are global, regardless of the
 * number of procs (i.e., weapon enchants). Thus, we straight up use the driver's name, or
 * override it with the special_effect_t name_str if it's defined. We can also fall back to using
 * the item name and the slot of the item, if ids are not defined.
 */
std::string special_effect_t::cooldown_name() const
{
  if ( ! name_str.empty() )
  {
    assert( name_str.size() );
    return name_str;
  }

  std::string n;
  if ( driver() -> id() > 0 )
  {
    n = driver() -> name_cstr();
    // Append the spell ID of the driver to the cooldown name. In some cases, the
    // drivers of different trinket procs are actually named identically, causing
    // issues when the trinkets are worn.
    n += "_" + util::to_string( driver() -> id() );
  }
  else if ( item )
  {
    n = item -> name();
    n += "_";
    n += item -> slot_name();
  }

  util::tokenize( n );

  assert( ! n.empty() );

  return n;
}

/**
 * Item-based special effects may have a cooldown group (in the client data). Cooldown groups are a
 * shared cooldown between all item effects (in essence, on-use effects) that use the same group
 */
std::string special_effect_t::cooldown_group_name() const
{
  if ( ! item )
  {
    return std::string();
  }

  unsigned cdgroup = cooldown_group();
  if ( cdgroup > 0 )
  {
    return "item_cd_" + util::to_string( cdgroup );
  }

  return std::string();
}

int special_effect_t::cooldown_group() const
{
  if ( ! item )
  {
    return 0;
  }

  // New-style On-Use item spells may use a special cooldown category to signal the shared cooldown
  if ( driver() -> category() == ITEM_TRINKET_BURST_CATEGORY )
  {
    return driver() -> category();
  }

  // For everything else, look at the item effects for a cooldown group
  for ( size_t i = 0; i < MAX_ITEM_EFFECT; ++i )
  {
    if ( item -> parsed.data.cooldown_group[ i ] > 0 )
    {
      return item -> parsed.data.cooldown_group[ i ];
    }
  }

  return 0;
}

timespan_t special_effect_t::cooldown_group_duration() const
{
  if ( ! item )
  {
    return timespan_t::zero();
  }

  // New-style On-Use items when using a special cooldown category signal the shared cooldown
  // duration in the spell itself
  if ( driver() -> category() == ITEM_TRINKET_BURST_CATEGORY )
  {
    return driver() -> category_cooldown();
  }

  // For everything else, look at the item effects with a cooldown group
  for ( size_t i = 0; i < MAX_ITEM_EFFECT; ++i )
  {
    if ( item -> parsed.data.cooldown_group[ i ] > 0 )
    {
      return timespan_t::from_millis( item -> parsed.data.cooldown_group_duration[ i ] );
    }
  }

  return 0_ms;
}

const item_t dbc_proc_callback_t::default_item_ = item_t();

