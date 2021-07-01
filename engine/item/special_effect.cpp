// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "special_effect.hpp"

#include "action/attack.hpp"
#include "action/spell.hpp"
#include "action/heal.hpp"
#include "buff/sc_buff.hpp"
#include "dbc/item_database.hpp"
#include "item.hpp"
#include "player/sc_player.hpp"
#include "player/unique_gear_helper.hpp"
#include "sim/real_ppm.hpp"

namespace {

struct proc_parse_opt_t
{
  util::string_view opt;
  unsigned flags;
};

constexpr proc_parse_opt_t __proc_opts[] =
{
  { "genericspell", PF_NONE_SPELL                                               },
  { "spell",        PF_MAGIC_SPELL | PF_PERIODIC                                },
  { "directspell",  PF_MAGIC_SPELL                                              },
  { "periodic",     PF_PERIODIC                                                 },
  { "genericheal",  PF_NONE_HEAL                                                },
  { "heal",         PF_MAGIC_HEAL | PF_PERIODIC                                 },
  { "directheal",   PF_MAGIC_HEAL                                               },
  { "attack",       PF_MELEE | PF_MELEE_ABILITY | PF_RANGED | PF_RANGED_ABILITY },
  { "wattack",      PF_MELEE | PF_RANGED                                        },
  { "sattack",      PF_MELEE_ABILITY | PF_RANGED_ABILITY                        },
  { "melee",        PF_MELEE | PF_MELEE_ABILITY                                 },
  { "wmelee",       PF_MELEE                                                    },
  { "smelee",       PF_MELEE_ABILITY                                            },
  { "wranged",      PF_RANGED                                                   },
  { "sranged",      PF_RANGED_ABILITY                                           },
};

constexpr proc_parse_opt_t __proc2_opts[] =
{
  { "hit",         PF2_ALL_HIT          },
  { "crit",        PF2_CRIT             },
  { "glance",      PF2_GLANCE           },
  { "dodge",       PF2_DODGE            },
  { "parry",       PF2_PARRY            },
  { "miss",        PF2_MISS             },
  { "cast",        PF2_CAST             },
  { "impact",      PF2_LANDED           },
  { "tickheal",    PF2_PERIODIC_HEAL    },
  { "tickdamage",  PF2_PERIODIC_DAMAGE  },
};

bool has_proc( util::span<const util::string_view> opts, util::string_view proc )
{
  for ( auto & opt : opts )
  {
    if ( util::str_compare_ci( opt, proc ) )
      return true;
  }
  return false;
}

void parse_proc_flags( util::string_view flags,
                       util::span<const proc_parse_opt_t> opts,
                       unsigned& proc_flags )
{
  auto splits = util::string_split<util::string_view>( flags, "/" );

  for( auto& proc_opt : opts )
  {
    if ( has_proc( splits, proc_opt.opt ) )
      proc_flags |= proc_opt.flags;
  }
}

} // unnamed namespace

special_effect_t::special_effect_t( const item_t* item ) :
    item( item ),
    player( item -> player )
{
  reset();
}

void special_effect_t::reset()
{
  type = SPECIAL_EFFECT_NONE;
  source = SPECIAL_EFFECT_SOURCE_NONE;

  name_str.clear();
  trigger_str.clear();
  encoding_str.clear();

  proc_flags_ = 0;
  proc_flags2_ = 0;

  stat = STAT_NONE;
  stat_amount = 0;

  school = SCHOOL_NONE;
  discharge_amount = 0;
  discharge_scaling = 0;
  aoe = 0;
  reverse_stack_reduction = 0;

  // Must match buff creator defaults for now
  max_stacks = -1;
  proc_chance_ = -1;
  expire_on_max_stack = -1;

  // ppm < 0 = real ppm, ppm > 0 = normal "ppm", min_double off
  ppm_ = std::numeric_limits<double>::min();
  rppm_scale_ = RPPM_NONE;
  rppm_modifier_ = -1.0;
  rppm_blp_ = real_ppm_t::BLP_ENABLED;

  // Must match buff creator defaults for now
  duration_ = timespan_t::min();

  // Currently covers both cooldown and internal cooldown, as typically
  // cooldown is only used for on-use effects. Must match buff creator default
  // for now.
  cooldown_ = timespan_t::min();

  tick = timespan_t::zero();

  cost_reduction = false;
  refresh = -1;
  chance_to_discharge = false;
  reverse = false;
  proc_delay = false;
  unique = false;
  weapon_proc = false;

  override_result_es_mask = 0;
  result_es_mask = 0;

  spell_id = 0;
  trigger_spell_id = 0;

  execute_action = nullptr;
  custom_buff = nullptr;
  custom_init = nullptr;
  custom_init_object.clear();

  action_disabled = false;
  buff_disabled = false;

  enchant_data = nullptr;
}

const spell_data_t* special_effect_t::driver() const
{
  if ( !player || spell_id == 0 )
    return spell_data_t::nil();

  return player -> find_spell( spell_id );
}

const spell_data_t* special_effect_t::trigger() const
{
  if ( ! player )
    return spell_data_t::nil();

  // Note, bypasses level checks and such
  if ( trigger_spell_id > 0 )
  {
    return dbc::find_spell( player, trigger_spell_id );
  }

  for ( size_t i = 1, end = driver() -> effect_count(); i <= end; i++ )
  {
    if ( driver() -> effectN( i ).trigger() -> ok() )
      return driver() -> effectN( i ).trigger();
  }

  // As a fallback, return the driver spell as the "trigger" as well. This
  // allows on-use side of special effects use the same code paths, and
  // shouldnt break procs.
  return driver();
}


/**
 * Detect stat buff type from spelleffect data.
 *
 * Helper function to detect stat buff type from spell effect data, more specifically by mapping Blizzard's
 * effect sub types to actual stats in simc.
 */
stat_e special_effect_t::stat_buff_type( const spelleffect_data_t& effect ) const
{
  stat_e stat = STAT_NONE;

  // All stat buffs in game client data are "auras"
  if ( effect.type() != E_APPLY_AURA )
    return stat;

  switch ( effect.subtype() )
  {
    case A_MOD_STAT:
      if ( effect.misc_value1() == -1 )
        stat = STAT_ALL;  // Primary stats only, ratings are A_MOD_RATING
      else if ( effect.misc_value1() == -2 )
        stat = STAT_STR_AGI_INT;
      else
        stat = static_cast<stat_e>( effect.misc_value1() + 1 );
      break;
    case A_MOD_RATING:
      stat = util::translate_rating_mod( effect.misc_value1() );
      break;
    case A_MOD_INCREASE_HEALTH_2:
    case A_MOD_INCREASE_HEALTH:
      stat = STAT_MAX_HEALTH;
      break;
    case A_MOD_RESISTANCE:
      stat = STAT_BONUS_ARMOR;
      break;
    case A_MOD_ATTACK_POWER:
    case A_MOD_RANGED_ATTACK_POWER:
      stat = STAT_ATTACK_POWER;
      break;
    case A_MOD_DAMAGE_DONE:
      if ( effect.misc_value1() & 0x7E )
        stat = STAT_SPELL_POWER;
      break;
    case A_465:
      stat = STAT_BONUS_ARMOR;
      break;
    default:
      break;
  }

  return stat;
}

bool special_effect_t::is_stat_buff() const
{
  if ( stat != STAT_NONE )
    return true;

  // Prioritize trigger scanning before driver scanning; in reality if both
  // the trigger and driver spells contain "stat buffs" (as per simc
  // definition), the system will not support it. In this case, custom buffs
  // and/or callbacks should be used.
  for ( size_t i = 1, end = trigger() -> effect_count(); i <= end; i++ )
  {
    const spelleffect_data_t& effect = trigger() -> effectN( i );
    if ( effect.id() == 0 )
      continue;

    if ( stat_buff_type( effect ) != STAT_NONE )
      return true;
  }

  return false;
}

stat_e special_effect_t::stat_type() const
{
  if ( stat != STAT_NONE )
    return stat;

  for ( size_t i = 1, end = trigger() -> effect_count(); i <= end; i++ )
  {
    const spelleffect_data_t& effect = trigger() -> effectN( i );
    if ( effect.id() == 0 )
      continue;

    stat_e type = stat_buff_type( effect );
    if ( type != STAT_NONE )
      return type;
  }

  return STAT_NONE;
}

int special_effect_t::max_stack() const
{
  if ( max_stacks != -1 )
    return max_stacks;

  if ( trigger() -> max_stacks() > 0 )
    return trigger() -> max_stacks();

  if ( driver() -> max_stacks() > 0 )
    return driver() -> max_stacks();

  return -1;
}

stat_buff_t* special_effect_t::initialize_stat_buff() const
{
  if ( buff_t* b = buff_t::find( player, name() ) )
  {
    return debug_cast<stat_buff_t*>( b );
  }

  const spell_data_t* spell_data = spell_data_t::nil();
  // Setup the spell for the stat buff
  if ( trigger() -> id() > 0 )
    spell_data = trigger();
  else if ( driver() -> id() > 0 )
    spell_data = driver();

  stat_buff_t* buff = make_buff<stat_buff_t>( player, name(), spell_data,
                               source == SPECIAL_EFFECT_SOURCE_ITEM ? item : nullptr );

  // Setup user option overrides. Note that if there are no user set overrides,
  // the buff will automagically deduce correct options from the spell data,
  // the special_effect_t object should not issue overrides to the creator
  // object.
  if ( max_stacks > 0 )
    buff->set_max_stack( max_stacks );

  if ( duration_ > timespan_t::zero() )
    buff->set_duration( duration_ );

  if ( reverse )
    buff->set_reverse( true );

  if ( tick > timespan_t::zero() )
  {
    buff->set_period( tick );
    buff->set_tick_behavior( buff_tick_behavior::CLIP );
  }

  // Make the buff always proc. The proc chance is handled by the proc callback, so the buff should
  // always trigger.
  buff->set_chance( 1 );

  buff->set_refresh_behavior( buff_refresh_behavior::DURATION );

  // If user given stat is defined, override whatever the spell would contain
  if ( stat != STAT_NONE )
    buff->add_stat( stat, stat_amount );

  if ( reverse_stack_reduction > 0 )
  {
    buff->set_reverse_stack_count( reverse_stack_reduction );
  }

  return buff;
}

bool special_effect_t::is_absorb_buff() const
{
  for ( size_t i = 1, end = trigger() -> effect_count(); i <= end; i++ )
  {
    const spelleffect_data_t& effect = trigger() -> effectN( i );
    if ( effect.id() == 0 )
      continue;

    if ( effect.subtype() == A_SCHOOL_ABSORB )
      return true;
  }

  return false;
}

absorb_buff_t* special_effect_t::initialize_absorb_buff() const
{
  if ( buff_t* b = buff_t::find( player, name() ) )
  {
    return debug_cast<absorb_buff_t*>( b );
  }

  // Setup the spell for the buff
  const spell_data_t* buff_spell = spell_data_t::nil();
  if ( trigger() -> id() > 0 )
  {
    buff_spell = trigger();
  }
  else if ( driver() -> id() > 0 )
  {
    buff_spell = driver();
  }

  auto buff = make_buff<absorb_buff_t>( player, name(), buff_spell,
                               source == SPECIAL_EFFECT_SOURCE_ITEM ? item : nullptr );

  // Setup user option overrides. Note that if there are no user set overrides,
  // the buff will automagically deduce correct options from the spell data,
  // the special_effect_t object should not issue overrides to the creator
  // object.
  if ( max_stacks > 0 )
    buff->set_max_stack( max_stacks );

  if ( duration_ > timespan_t::zero() )
    buff->set_duration( duration_ );

  if ( reverse )
    buff->set_reverse( true );

  if ( tick > timespan_t::zero() )
  {
    buff->set_period( tick );
    buff->set_tick_behavior( buff_tick_behavior::CLIP );
  }

  // Make the buff always proc. The proc chance is handled by the proc callback, so the buff should
  // always trigger.
  buff->set_chance( 1 );

  buff->set_refresh_behavior( buff_refresh_behavior::DURATION );

  if ( reverse_stack_reduction > 0 )
  {
    buff->set_reverse_stack_count( reverse_stack_reduction );
  }

  return buff;
}

special_effect_buff_e special_effect_t::buff_type() const
{
  if ( buff_disabled )
    return SPECIAL_EFFECT_BUFF_NONE;
  else if ( custom_buff != nullptr )
    return SPECIAL_EFFECT_BUFF_CUSTOM;
  else if ( is_stat_buff() )
    return SPECIAL_EFFECT_BUFF_STAT;
  else if ( is_absorb_buff() )
    return SPECIAL_EFFECT_BUFF_ABSORB;

  return SPECIAL_EFFECT_BUFF_NONE;
}

buff_t* special_effect_t::create_buff() const
{

  if ( buff_type() != SPECIAL_EFFECT_BUFF_CUSTOM &&
       buff_type() != SPECIAL_EFFECT_BUFF_NONE &&
       buff_type() != SPECIAL_EFFECT_BUFF_DISABLED )
  {
    buff_t* b = buff_t::find( player, name() );
    if ( b )
    {
      return b;
    }
  }

  switch ( buff_type() )
  {
    case SPECIAL_EFFECT_BUFF_CUSTOM:
      return custom_buff;
    case SPECIAL_EFFECT_BUFF_STAT:
      return initialize_stat_buff();
    case SPECIAL_EFFECT_BUFF_ABSORB:
      return initialize_absorb_buff();
    default:
      return nullptr;
  }
}

action_t* special_effect_t::create_action() const
{
  // Custom actions have done their create_proc_action() call in the second phase init of the
  // special effect
  if ( action_type() != SPECIAL_EFFECT_ACTION_CUSTOM &&
       action_type() != SPECIAL_EFFECT_ACTION_NONE &&
       action_type() != SPECIAL_EFFECT_ACTION_DISABLED )
  {
    action_t* a = player -> find_action( name() );
    if ( a )
    {
      return a;
    }

    a = player -> create_proc_action( name(), *this );
    if ( a )
    {
      return a;
    }
  }

  switch ( action_type() )
  {
  case SPECIAL_EFFECT_ACTION_CUSTOM:
    return execute_action;
  case SPECIAL_EFFECT_ACTION_SPELL:
    return initialize_offensive_spell_action();
  case SPECIAL_EFFECT_ACTION_HEAL:
    return initialize_heal_action();
  case SPECIAL_EFFECT_ACTION_ATTACK:
    return initialize_attack_action();
  case SPECIAL_EFFECT_ACTION_RESOURCE:
    return initialize_resource_action();
  default:
    return nullptr;
  }
}

special_effect_action_e special_effect_t::action_type() const
{
  if ( action_disabled )
    return SPECIAL_EFFECT_ACTION_NONE;
  else if ( execute_action != nullptr )
    return SPECIAL_EFFECT_ACTION_CUSTOM;
  else if ( is_offensive_spell_action() )
    return SPECIAL_EFFECT_ACTION_SPELL;
  else if ( is_heal_action() )
    return SPECIAL_EFFECT_ACTION_HEAL;
  else if ( is_attack_action() )
    return SPECIAL_EFFECT_ACTION_ATTACK;
  else if ( is_resource_action() )
    return SPECIAL_EFFECT_ACTION_RESOURCE;

  return SPECIAL_EFFECT_ACTION_NONE;
}

bool special_effect_t::is_resource_action() const
{
  for ( size_t i = 1, end = trigger() -> effect_count(); i <= end; i++ )
  {
    const spelleffect_data_t& effect = trigger() -> effectN( i );
    if ( effect.id() == 0 )
      continue;

    if ( effect.type() == E_ENERGIZE )
      return true;

    if ( effect.type() == E_APPLY_AURA )
    {
      if ( effect.subtype() == A_PERIODIC_ENERGIZE )
        return true;
    }
  }

  return false;
}

spell_t* special_effect_t::initialize_resource_action() const
{
  const spell_data_t* s = spell_data_t::nil();

  // Setup the spell data
  if ( trigger() -> id() > 0 )
    s = trigger();
  else if ( driver() -> id() > 0 )
    s = driver();

  auto spell = new unique_gear::proc_resource_t( name(), player, s, source == SPECIAL_EFFECT_SOURCE_ITEM ? item : nullptr );
  spell -> init();
  return spell;
}

bool special_effect_t::is_offensive_spell_action() const
{
  if ( school != SCHOOL_NONE && ( discharge_amount > 0 || discharge_scaling > 0 ) )
  {
    return true;
  }

  for ( size_t i = 1, end = trigger() -> effect_count(); i <= end; i++ )
  {
    const spelleffect_data_t& effect = trigger() -> effectN( i );
    if ( effect.id() == 0 )
      continue;

    if ( effect.type() == E_SCHOOL_DAMAGE || effect.type() == E_HEALTH_LEECH )
      return true;

    if ( effect.type() == E_APPLY_AURA )
    {
      if ( effect.subtype() == A_PERIODIC_DAMAGE || effect.subtype() == A_PERIODIC_LEECH )
        return true;
    }
  }

  return false;
}

spell_t* special_effect_t::initialize_offensive_spell_action() const
{
  auto spell = new unique_gear::proc_spell_t( *this );
  spell -> init();
  return spell;
}

bool special_effect_t::is_heal_action() const
{
  for ( size_t i = 1, end = trigger() -> effect_count(); i <= end; i++ )
  {
    const spelleffect_data_t& effect = trigger() -> effectN( i );
    if ( effect.id() == 0 )
      continue;

    if ( effect.type() == E_HEAL )
      return true;

    if ( effect.type() == E_APPLY_AURA )
    {
      if ( effect.subtype() == A_PERIODIC_HEAL )
        return true;
    }
  }

  return false;
}

heal_t* special_effect_t::initialize_heal_action() const
{
  const spell_data_t* s = spell_data_t::nil();

  // Setup the spell data
  if ( trigger() -> id() > 0 )
    s = trigger();
  else if ( driver() -> id() > 0 )
    s = driver();

  auto heal = new unique_gear::proc_heal_t( name(), player, s, source == SPECIAL_EFFECT_SOURCE_ITEM ? item : nullptr );
  heal -> init();
  return heal;
}

bool special_effect_t::is_attack_action() const
{
  if ( school != SCHOOL_NONE && ( discharge_amount != 0 || discharge_scaling < 0 ) )
  {
    return true;
  }

  for ( size_t i = 1, end = trigger() -> effect_count(); i <= end; i++ )
  {
    const spelleffect_data_t& effect = trigger() -> effectN( i );
    if ( effect.id() == 0 )
      continue;

    if ( effect.type() == E_WEAPON_DAMAGE || effect.type() == E_WEAPON_PERCENT_DAMAGE )
      return true;
  }

  return false;
}

attack_t* special_effect_t::initialize_attack_action() const
{
  const spell_data_t* s = spell_data_t::nil();

  // Setup the spell data
  if ( trigger() -> id() > 0 )
    s = trigger();
  else if ( driver() -> id() > 0 )
    s = driver();

  auto attack = new unique_gear::proc_attack_t( name(), player, s, source == SPECIAL_EFFECT_SOURCE_ITEM ? item : nullptr );
  attack -> init();
  return attack;
}

unsigned special_effect_t::proc_flags() const
{
  if ( proc_flags_ != 0 )
    return proc_flags_;

  if ( driver() -> proc_flags() != 0 )
    return driver() -> proc_flags();

  return 0;
}

unsigned special_effect_t::proc_flags2() const
{
  // These do not have a spell-data equivalent, so they are going to always be
  // set manually, or (most commonly) default to 0. The new proc callback
  // system will automatically use "default values" appropriate for the proc,
  // if it is passed a zero proc_flags2. The default value is "spell landing",
  // as in a positive result of the spell type. For periodic effects and heals,
  // the system will require "an amount value", i.e., a healing or damaging
  // result.
  return proc_flags2_;
}

// DBC data has no concept of "old style" procs per minute, they will be
// manually set by simc.
double special_effect_t::ppm() const
{
  if ( ppm_ != std::numeric_limits<double>::min() )
    return ppm_;

  return 0;
}

double special_effect_t::rppm() const
{
  if ( ppm_ <= 0 && ppm_ != std::numeric_limits<double>::min() )
    return std::fabs( ppm_ );

  return driver() -> real_ppm();
}

unsigned special_effect_t::rppm_scale() const
{
  if ( rppm_scale_ != RPPM_NONE )
  {
    return rppm_scale_;
  }

  if ( !player )
  {
    return 0;
  }

  return player -> dbc->real_ppm_scale( driver() -> id() );
}

double special_effect_t::rppm_modifier() const
{
  if ( rppm_modifier_ != -1.0 )
  {
    return rppm_modifier_;
  }

  if ( !player )
  {
    return 0.0;
  }

  return player -> dbc->real_ppm_modifier( driver() -> id(), player, item ? item -> item_level() : 0 );
}

/**
 * Get cooldown duration.
 *
 * Cooldowns are automatically extracted from the driver spell. However, the "Cooldown" value is prioritized over
 * "Internal Cooldown" in DBC data. In reality, there should not be any (single) driver, that includes both.
 */
timespan_t special_effect_t::cooldown() const
{
  if ( cooldown_ >= timespan_t::zero() )
    return cooldown_;

  // First, check item cooldowns, since they override (in simc) whatever the
  // spell cooldown may be
  if ( source == SPECIAL_EFFECT_SOURCE_ITEM && item )
  {
    for ( const item_effect_t& effect : item -> parsed.data.effects )
    {
      if ( effect.spell_id == spell_id && effect.cooldown_duration > 0 )
        return timespan_t::from_millis( effect.cooldown_duration );
    }
  }

  if ( driver() -> cooldown() > timespan_t::zero() )
    return driver() -> cooldown();
  else if ( driver() -> internal_cooldown() > timespan_t::zero() )
    return driver() -> internal_cooldown();

  return timespan_t::zero();
}

timespan_t special_effect_t::duration() const
{
  if ( duration_ > timespan_t::zero() )
    return duration_;
  else if ( trigger() -> duration() > timespan_t::zero() )
    return trigger() -> duration();
  else if ( driver() -> duration() > timespan_t::zero() )
    return driver() -> duration();

  return timespan_t::zero();
}


/**
 * Get tick time.
 *
 * The tick time is selected from the first periodically executed effect of the _triggered_ spell.
 * Periodically executing drivers are currently not supported. Note that ticking behavior has now moved to buff_t,
 *  instead of being kludged in the callback itself (as it was done in the "proc_callback_t" days).
 */
timespan_t special_effect_t::tick_time() const
{
  if ( tick > timespan_t::zero() )
    return tick;

  // Search trigger for now, it's not likely that the driver is ticking
  for ( size_t i = 1; i <= trigger() -> effect_count(); i++ )
  {
    if ( trigger() -> effectN( i ).period() > timespan_t::zero() )
      return trigger() -> effectN( i ).period();
  }

  return timespan_t::zero();
}

double special_effect_t::proc_chance() const
{
  if ( proc_chance_ > 0 )
    return proc_chance_;

  return driver() -> proc_chance();
}

std::string special_effect_t::name() const
{
  if ( ! name_str.empty() )
    return name_str;

  // Guess proc name based on spells.
  std::string n;
  std::string base_name;
  // Use driver name, if it's not hidden or passive, or there's no trigger spell to use
  if ( ( ! driver() -> flags( spell_attribute::SX_HIDDEN ) && ! driver() -> flags( spell_attribute::SX_PASSIVE ) ) ||
       ! trigger() -> ok() )
    n = driver() -> name_cstr();
  // Driver is hidden, try to use the spell that the driver procs
  else if ( trigger() -> ok() )
    n = trigger() -> name_cstr();

  util::tokenize( n );

  base_name = n;

  // As a last resort, try to make the special effect name out of spell_id
  if ( n.empty() && spell_id > 0 )
  {
    n = "unknown_special_effect_" + util::to_string( spell_id );
  }

  // Assert on empty names; we do need a reasonable name for the proc.
  assert( ! n.empty() );

  // Append weapon suffix automatically to the name.
  // TODO: We need a "shared" mechanism here
  if ( item && item -> slot == SLOT_OFF_HAND )
    n += "_oh";

  if ( action_type() != SPECIAL_EFFECT_ACTION_CUSTOM &&
       action_type() != SPECIAL_EFFECT_ACTION_NONE &&
       action_type() != SPECIAL_EFFECT_ACTION_DISABLED )
  {
    action_t* a = player -> find_action( base_name );
    // If for some reason the trigger spell would be named identical to another spell that the actor
    // already has, rename this new one with the slot of the item, if there is an item
    if ( a && a->id != trigger()->id() )
    {
      n += " (";
      if ( item )
      {
        n += item->slot_name();
      }
      else
      {
        n += util::special_effect_source_string( source );
      }
      n += ")";
    }
  }

  return n;
}

void format_to( const special_effect_t& se, fmt::format_context::iterator out )
{
  fmt::format_to( out, "{}", se.name() );
  fmt::format_to( out, " type={}", se.type );
  fmt::format_to( out, " source={}", se.source );

  if ( ! se.trigger_str.empty() )
    fmt::format_to( out, " proc={}", se.trigger_str );

  if ( se.spell_id > 0 )
    fmt::format_to( out, " driver={}", se.spell_id );

  if ( se.trigger() -> ok() )
    fmt::format_to( out, " trigger={}", se.trigger() -> id() );

  if ( se.is_stat_buff() )
  {
    fmt::format_to( out, " stat={}", util::stat_type_abbrev( se.stat == STAT_NONE ? se.stat_type() : se.stat ) );
    fmt::format_to( out, " amount={}", se.stat_amount );
    fmt::format_to( out, " duration={}", se.duration() );
    if ( se.tick_time() != timespan_t::zero() )
      fmt::format_to( out, " tick={}", se.tick_time() );
    if ( se.reverse )
    {
      fmt::format_to( out, " Reverse" );
      if ( se.reverse_stack_reduction > 0 )
        fmt::format_to( out, "({})", se.reverse_stack_reduction );
    }
  }

  if ( se.school != SCHOOL_NONE )
  {
    fmt::format_to( out, " school={}", se.school );
    fmt::format_to( out, " amount={}", se.discharge_amount );
    if ( se.discharge_scaling > 0 )
      fmt::format_to( out, " coeff={}", se.discharge_scaling );
  }

  if ( se.max_stacks > 0 )
    fmt::format_to( out, " max_stack={}", se.max_stacks );

  if ( se.proc_chance() > 0 )
    fmt::format_to( out, " proc_chance={}%", se.proc_chance() * 100.0 );

  if ( se.ppm() > 0 )
    fmt::format_to( out, " ppm={}", se.ppm() );

  if ( se.rppm() > 0 && se.rppm_scale() != RPPM_DISABLE )
  {
    fmt::format_to( out, " rppm={}", se.rppm() * se.rppm_modifier() );
    if ( se.rppm_scale() & RPPM_HASTE )
      fmt::format_to( out, " (Haste)" );
    if ( se.rppm_scale() & RPPM_CRIT )
      fmt::format_to( out, " (Crit)" );
    if ( se.rppm_scale() & RPPM_ATTACK_SPEED )
      fmt::format_to( out, " (ASpeed)" );
  }

  if ( se.cooldown() > timespan_t::zero() )
  {
    util::string_view name;
    if ( se.type == SPECIAL_EFFECT_EQUIP )
      name = "icd";
    else if ( se.type == SPECIAL_EFFECT_USE )
      name = "cd";
    else
      name = "(i)cd";
    fmt::format_to( out, " {}={}", name, se.cooldown() );

    if ( se.cooldown_group() > 0 )
    {
      fmt::format_to( out, " cdgrp={} cdgrp_duration={}",
                      se.cooldown_group(), se.cooldown_group_duration() );
    }
  }

  if ( se.weapon_proc )
    fmt::format_to( out, " weaponproc" );

  if ( se.expire_on_max_stack != -1 )
  {
    fmt::format_to( out, " expire_buff_on_max_stack={}", as<bool>( se.expire_on_max_stack ) );
  }
}

void special_effect::parse_special_effect_encoding( special_effect_t& effect,
                                          const std::string& encoding )
{
  if ( encoding.empty() || encoding == "custom" || encoding == "none" )
    return;

   auto tokens = item_database::parse_tokens( encoding );

  effect.encoding_str = encoding;

  for ( auto& t : tokens )
  {
    stat_e s;
    school_e sc;

    if ( ( s = util::parse_stat_type( t.name ) ) != STAT_NONE )
    {
      effect.stat = s;
      effect.stat_amount = t.value;
    }
    else if ( ( sc = util::parse_school_type( t.name ) ) != SCHOOL_NONE )
    {
      effect.school = sc;
      effect.discharge_amount = t.value;

      auto splits = util::string_split<util::string_view>( t.value_str, "+" );
      if ( splits.size() == 2 )
      {
        effect.discharge_amount  = util::to_double( splits[ 0 ] );
        effect.discharge_scaling = util::to_double( splits[ 1 ] ) / 100.0;
      }
    }
    else if ( t.name == "stacks" || t.name == "stack" )
    {
      effect.max_stacks = ( int ) t.value;
    }
    else if ( t.name == "%" )
    {
      effect.proc_chance_ = t.value / 100.0;
    }
    else if ( t.name == "ppm" )
    {
      effect.ppm_ = t.value;
    }
    else if ( util::str_prefix_ci( t.name, "rppm" ) )
    {
      if ( t.value != 0 )
        effect.ppm_ = -t.value;

      if ( util::str_in_str_ci( t.name, "spellcrit" ) )
        effect.rppm_scale_ |= RPPM_CRIT;
      else if ( util::str_in_str_ci( t.name, "attackcrit" ) )
        effect.rppm_scale_ |= RPPM_CRIT;
      else if ( util::str_in_str_ci( t.name, "haste" ) )
        effect.rppm_scale_ |= RPPM_HASTE;
      else
        effect.rppm_scale_ = RPPM_NONE;
    }
    else if ( util::str_compare_ci( t.name, "noblp" ) )
    {
      effect.rppm_blp_ = real_ppm_t::BLP_DISABLED;
    }
    else if ( t.name == "duration" || t.name == "dur" )
    {
      effect.duration_ = timespan_t::from_seconds( t.value );
    }
    else if ( t.name == "cooldown" || t.name == "cd" )
    {
      effect.cooldown_ = timespan_t::from_seconds( t.value );
    }
    else if ( t.name == "tick" )
    {
      effect.tick = timespan_t::from_seconds( t.value );
    }
    else if ( t.full == "stackreduction" )
    {
      effect.reverse_stack_reduction = as<int>( t.value );
    }
    else if ( t.full == "reverse" )
    {
      effect.reverse = true;
    }
    else if ( t.full == "chance" )
    {
      effect.chance_to_discharge = true;
    }
    else if ( t.name == "costrd" )
    {
      effect.cost_reduction = true;
      effect.refresh = 0;
    }
    else if ( t.name == "nocrit" )
    {
      effect.override_result_es_mask |= RESULT_CRIT_MASK;
      effect.result_es_mask &= ~RESULT_CRIT_MASK;
    }
    else if ( t.name == "maycrit" )
    {
      effect.override_result_es_mask |= RESULT_CRIT_MASK;
      effect.result_es_mask |= RESULT_CRIT_MASK;
    }
    else if ( t.name == "nomiss" )
    {
      effect.override_result_es_mask |= RESULT_MISS_MASK;
      effect.result_es_mask &= ~RESULT_MISS_MASK;
    }
    else if ( t.name == "maymiss" )
    {
      effect.override_result_es_mask |= RESULT_MISS_MASK;
      effect.result_es_mask |= RESULT_MISS_MASK;
    }
    else if ( t.name == "nododge" )
    {
      effect.override_result_es_mask |= RESULT_DODGE_MASK;
      effect.result_es_mask &= ~RESULT_DODGE_MASK;
    }
    else if ( t.name == "maydodge" )
    {
      effect.override_result_es_mask |= RESULT_DODGE_MASK;
      effect.result_es_mask |= RESULT_DODGE_MASK;
    }
    else if ( t.name == "noparry" )
    {
      effect.override_result_es_mask |= RESULT_PARRY_MASK;
      effect.result_es_mask &= ~RESULT_PARRY_MASK;
    }
    else if ( t.name == "mayparry" )
    {
      effect.override_result_es_mask |= RESULT_PARRY_MASK;
      effect.result_es_mask |= RESULT_PARRY_MASK;
    }
    else if ( t.name == "norefresh" )
    {
      effect.refresh = 0;
    }
    else if ( t.name == "refresh" )
    {
      effect.refresh = 1;
    }
    else if ( t.full == "aoe" )
    {
      effect.aoe = -1;
    }
    else if ( t.name == "aoe" )
    {
      effect.aoe = ( int ) t.value;
      if ( effect.aoe < -1 )
        effect.aoe = -1;
    }
    else if ( t.name == "driver" )
    {
      effect.spell_id = static_cast<int>( t.value );
    }
    else if ( t.name == "trigger" )
    {
      effect.trigger_spell_id = static_cast<int>( t.value );
    }
    else if ( util::str_in_str_ci( t.full, "procby" ) )
      parse_proc_flags( t.full, __proc_opts, effect.proc_flags_ );
    else if ( util::str_in_str_ci( t.full, "procon" ) )
      parse_proc_flags( t.full, __proc2_opts, effect.proc_flags2_ );
    else
    {

      throw std::invalid_argument(
                fmt::format("Unknown 'use/equip=' token '{}'.",
                    t.full));
    }
  }
}

/**
 * Ensure that the proc has enough information to actually proc in simc. This
 * essentially requires a non-zero proc_chance or PPM field in effect, or the
 * driver spell to contain a non-zero RPPM or proc chance. Additionally, we
 * need to have some sort of flags indicating what events trigger the proc.
 *
 * TODO: Action support
 */
bool special_effect::usable_proc( const special_effect_t& effect )
{
  // Valid proc flags (either old- or new style), we can proc this effect.
  if ( effect.proc_flags() == 0 )
  {
    if (effect.item)
    {
      effect.item->sim->print_debug("Effect '{}' no proc flags / trigger type", effect.name() );
    }
    return false;
  }

  // A non-zero chance to proc it through one of the proc chance triggers
  if ( effect.ppm() == 0 && effect.rppm() == 0 && effect.proc_chance() == 0 )
  {
    if (effect.item)
    {
      effect.item->sim->print_debug("Effect '{}' No RPPM / PPM / Proc chance", effect.name() );
    }
    return false;
  }

  // Require a valid buff or an action
  if ( effect.buff_type() == SPECIAL_EFFECT_BUFF_NONE && effect.action_type() == SPECIAL_EFFECT_ACTION_NONE )
  {
    if (effect.item)
    {
      effect.item->sim->print_debug("Effect '{}' No constructible buff or action", effect.name() );
    }
    return false;
  }

  return true;
}

/**
 * Cooldown name needs some special handling. Cooldowns in WoW are global, regardless of the
 * number of procs (i.e., weapon enchants). Thus, we straight up use the driver's name, or
 * override it with the special_effect_t name_str if it's defined. We can also fall back to using
 * the item name and the slot of the item, if ids are not defined.
 */
std::string special_effect_t::cooldown_name() const
{
  if (!name_str.empty())
  {
    assert(!name_str.empty());
    return name_str;
  }

  std::string n;
  if (driver()->id() > 0)
  {
    n = driver()->name_cstr();
    // Append the spell ID of the driver to the cooldown name. In some cases, the
    // drivers of different trinket procs are actually named identically, causing
    // issues when the trinkets are worn.
    n += "_" + util::to_string(driver()->id());
  }
  else if (item)
  {
    n = item->name();
    n += "_";
    n += item->slot_name();
  }

  util::tokenize(n);

  assert(!n.empty());

  return n;
}

/**
 * Item-based special effects may have a cooldown group (in the client data). Cooldown groups are a
 * shared cooldown between all item effects (in essence, on-use effects) that use the same group
 */
std::string special_effect_t::cooldown_group_name() const
{
  if (!item)
  {
    return std::string();
  }

  unsigned cdgroup = cooldown_group();
  if (cdgroup > 0)
  {
    return "item_cd_" + util::to_string(cdgroup);
  }

  return std::string();
}

int special_effect_t::cooldown_group() const
{
  if (!item)
  {
    return 0;
  }

  // New-style On-Use item spells may use a special cooldown category to signal the shared cooldown
  if (driver()->category() == ITEM_TRINKET_BURST_CATEGORY)
  {
    return driver()->category();
  }

  // For everything else, look at the item effects for a cooldown group
  for ( const item_effect_t& effect : item -> parsed.data.effects )
  {
    if ( effect.cooldown_group > 0 )
    {
      return effect.cooldown_group;
    }
  }

  return 0;
}

timespan_t special_effect_t::cooldown_group_duration() const
{
  if (!item)
  {
    return timespan_t::zero();
  }

  // New-style On-Use items when using a special cooldown category signal the shared cooldown
  // duration in the spell itself
  if (driver()->category() == ITEM_TRINKET_BURST_CATEGORY)
  {
    return driver()->category_cooldown();
  }

  // For everything else, look at the item effects with a cooldown group
  for ( const item_effect_t& effect : item -> parsed.data.effects )
  {
    if ( effect.cooldown_group > 0 )
    {
      return timespan_t::from_millis( effect.cooldown_group_duration );
    }
  }

  return 0_ms;
}
