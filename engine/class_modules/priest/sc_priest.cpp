// ==========================================================================
// Priest Sim File
// Contact: https://github.com/orgs/simulationcraft/teams/priest/members
// Wiki: https://github.com/simulationcraft/simc/wiki/Priests
// ==========================================================================

#include "sc_priest.hpp"

#include "class_modules/apl/apl_priest.hpp"
#include "sc_enums.hpp"
#include "sim/sc_option.hpp"
#include "tcb/span.hpp"

#include "simulationcraft.hpp"

namespace priestspace
{
namespace actions
{
namespace spells
{
// ==========================================================================
// Levitate
// ==========================================================================
struct levitate_t final : public priest_spell_t
{
  levitate_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "levitate", p, p.find_class_spell( "Levitate" ) )
  {
    parse_options( options_str );
    ignore_false_positive = true;
  }
};

// ==========================================================================
// Power Word: Fortitude
// ==========================================================================
struct power_word_fortitude_t final : public priest_spell_t
{
  power_word_fortitude_t( priest_t& p, util::string_view options_str )
    : priest_spell_t( "power_word_fortitude", p, p.find_class_spell( "Power Word: Fortitude" ) )
  {
    parse_options( options_str );
    harmful               = false;
    ignore_false_positive = true;

    background = sim->overrides.power_word_fortitude != 0;
  }

  void execute() override
  {
    priest_spell_t::execute();

    if ( !sim->overrides.power_word_fortitude )
      sim->auras.power_word_fortitude->trigger();
  }
};

// ==========================================================================
// Smite
// ==========================================================================
struct smite_t final : public priest_spell_t
{
  smite_t( priest_t& p, util::string_view options_str ) : priest_spell_t( "smite", p )
  {
    parse_options( options_str );

    base_execute_time  = timespan_t::from_seconds( 2.5 );
    base_dd_multiplier = 2.5 / 3.5;
    may_crit           = true;

    // if ( priest().talents.divine_fury->ok() )
    // {
    //   base_execute_time -= timespan_t::from_seconds( 0.1 );
    // }

    // if ( priest().talents.searing_light->ok() )
    // {
    //   base_multiplier *= 1.05;
    // }

    // if ( priest().talents.force_of_will->ok() )
    // {
    //   base_multiplier *= 1.01;
    // }

    // if ( priest().talents.holy_specialization->ok() )
    // {
    //   base_crit += 0.01;
    // }

    // if ( priest().talents.force_of_will->ok() )
    // {
    //   base_crit += 0.01;
    // }

    // if ( priest().talents.focused_power->ok() )
    // {
    //   base_hit += 0.02;
    // }

    // TODO: fix spec?
    if ( priest().sets->has_set_bonus( 258, T4, B2 ) )
    {
      base_multiplier *= 1.05;
    }
  }

  timespan_t execute_time() const override
  {
    if ( priest().buffs.surge_of_light->check() )
    {
      return timespan_t::zero();
    }

    return priest_spell_t::execute_time();
  }

  virtual double cost()
  {
    if ( priest().buffs.surge_of_light->check() )
    {
      return 0;
    }

    return priest_spell_t::cost();
  }
};

// ==========================================================================
// Power Infusion
// ==========================================================================
struct power_infusion_t final : public priest_spell_t
{
  power_infusion_t( priest_t& p, util::string_view options_str, util::string_view name )
    : priest_spell_t( name, p, p.find_class_spell( "Power Infusion" ) )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    priest_spell_t::execute();

    // Trigger PI on the actor only if casting on itself or having the legendary
    if ( priest().options.self_power_infusion )
      player->buffs.power_infusion->trigger();
  }
};

// ==========================================================================
// Summon Pet
// ==========================================================================
/// Priest Pet Summon Base Spell
struct summon_pet_t : public priest_spell_t
{
  timespan_t summoning_duration;
  std::string pet_name;
  propagate_const<pet_t*> pet;

public:
  summon_pet_t( util::string_view n, priest_t& p )
    : priest_spell_t( n, p ), summoning_duration( timespan_t::zero() ), pet_name( n ), pet( nullptr )
  {
    harmful = false;
  }

  void init_finished() override
  {
    pet = player->find_pet( pet_name );

    priest_spell_t::init_finished();
  }

  void execute() override
  {
    if ( pet->is_sleeping() )
    {
      pet->summon( summoning_duration );
    }
    else
    {
      auto new_duration = pet->expiration->remains();
      new_duration += summoning_duration;
      pet->expiration->reschedule( new_duration );
    }

    priest_spell_t::execute();
  }

  bool ready() override
  {
    if ( !pet )
    {
      return false;
    }

    return priest_spell_t::ready();
  }
};

// ==========================================================================
// Summon Shadowfiend
// ==========================================================================
struct summon_shadowfiend_t final : public summon_pet_t
{
  summon_shadowfiend_t( priest_t& p, util::string_view options_str )
    : summon_pet_t( "shadowfiend", p )
  {
    parse_options( options_str );
    harmful            = false;
    summoning_duration = timespan_t::from_seconds( 15 );
  }
};

}  // namespace spells

namespace heals
{
// ==========================================================================
// Power Word: Shield
// ==========================================================================
struct power_word_shield_t final : public priest_absorb_t
{
  bool ignore_debuff;

  power_word_shield_t( priest_t& p, util::string_view options_str )
    : priest_absorb_t( "power_word_shield", p, p.find_class_spell( "Power Word: Shield" ) ), ignore_debuff( false )
  {
    add_option( opt_bool( "ignore_debuff", ignore_debuff ) );
    parse_options( options_str );

    spell_power_mod.direct = 5.5;  // hardcoded into tooltip, last checked 2017-03-18
  }
};

}  // namespace heals

}  // namespace actions

namespace buffs
{
}  // namespace buffs

namespace items
{
}  // namespace items

// ==========================================================================
// Priest Targetdata Definitions
// ==========================================================================

priest_td_t::priest_td_t( player_t* target, priest_t& p ) : actor_target_data_t( target, &p ), dots(), buffs()
{
  dots.shadow_word_pain = target->get_dot( "shadow_word_pain", &p );
  dots.vampiric_touch   = target->get_dot( "vampiric_touch", &p );
  dots.devouring_plague = target->get_dot( "devouring_plague", &p );
  dots.vampiric_embrace = target->get_dot( "vampiric_embrace", &p );

  target->register_on_demise_callback( &p, [ this ]( player_t* ) { target_demise(); } );
}

void priest_td_t::reset()
{
}

void priest_td_t::target_demise()
{
  priest().sim->print_debug( "{} demised. Priest {} resets targetdata for him.", *target, priest() );

  reset();
}

// ==========================================================================
// Priest Definitions
// ==========================================================================

priest_t::priest_t( sim_t* sim, util::string_view name, race_e r )
  : player_t( sim, PRIEST, name, r ),
    buffs(),
    talents(),
    specs(),
    dot_spells(),
    cooldowns(),
    gains(),
    benefits(),
    procs(),
    background_actions(),
    active_items(),
    pets( *this ),
    options()
{
  create_cooldowns();
  create_gains();
  create_procs();
  create_benefits();

  resource_regeneration = regen_type::DYNAMIC;
}

/** Construct priest cooldowns */
void priest_t::create_cooldowns()
{
  cooldowns.holy_fire  = get_cooldown( "holy_fire" );
  cooldowns.mind_blast = get_cooldown( "mind_blast" );
}

/** Construct priest gains */
void priest_t::create_gains()
{
  gains.mindbender                    = get_gain( "Mana Gained from Mindbender" );
  gains.shadow_word_death_self_damage = get_gain( "Shadow Word: Death self inflicted damage" );
}

/** Construct priest procs */
void priest_t::create_procs()
{
}

/** Construct priest benefits */
void priest_t::create_benefits()
{
}

/**
 * Define the acting role of the priest().
 *
 * If base_t::primary_role() has a valid role defined, use it, otherwise select spec-based default.
 */
role_e priest_t::primary_role() const
{
  switch ( base_t::primary_role() )
  {
    case ROLE_HEAL:
      return ROLE_HEAL;
    case ROLE_DPS:
    case ROLE_SPELL:
      return ROLE_SPELL;
    case ROLE_ATTACK:
      return ROLE_SPELL;
    default:
      // if ( specialization() == PRIEST_HOLY )
      // {
      //   return ROLE_HEAL;
      // }
      break;
  }

  return ROLE_SPELL;
}

/**
 * @brief Convert hybrid stats
 *
 *  Converts hybrid stats that either morph based on spec or only work
 *  for certain specs into the appropriate "basic" stats
 */
stat_e priest_t::convert_hybrid_stat( stat_e s ) const
{
  switch ( s )
  {
    case STAT_STR_AGI_INT:
    case STAT_STR_INT:
    case STAT_AGI_INT:
      return STAT_INTELLECT;
    case STAT_STR_AGI:
      return STAT_NONE;
    case STAT_SPIRIT:
      return STAT_SPIRIT;
    case STAT_BONUS_ARMOR:
      return STAT_NONE;
    default:
      return s;
  }
}

std::unique_ptr<expr_t> priest_t::create_expression( util::string_view expression_str )
{
  auto splits = util::string_split<util::string_view>( expression_str, "." );

  if ( auto pet_expr = create_pet_expression( expression_str, splits ) )
  {
    return pet_expr;
  }

  if ( splits.size() >= 2 )
  {
    if ( util::str_compare_ci( splits[ 0 ], "priest" ) )
    {
      if ( util::str_compare_ci( splits[ 1 ], "self_power_infusion" ) )
      {
        return expr_t::create_constant( "self_power_infusion", options.self_power_infusion );
      }
      throw std::invalid_argument( fmt::format( "Unsupported priest expression '{}'.", splits[ 1 ] ) );
    }
  }

  return player_t::create_expression( expression_str );
}  // namespace priestspace

void priest_t::assess_damage( school_e school, result_amount_type dtype, action_state_t* s )
{
  player_t::assess_damage( school, dtype, s );
}

double priest_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  return h;
}

double priest_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  return h;
}

double priest_t::composite_spell_crit_chance() const
{
  double sc = player_t::composite_spell_crit_chance();

  return sc;
}

double priest_t::composite_player_pet_damage_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s );

  return m;
}

double priest_t::composite_player_heal_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_heal_multiplier( s );

  return m;
}

double priest_t::composite_player_absorb_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_absorb_multiplier( s );

  return m;
}

double priest_t::composite_player_target_multiplier( player_t* t, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( t, school );

  return m;
}

double priest_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_INTELLECT )
  {
    return 0.05;
  }

  return 0.0;
}

action_t* priest_t::create_action( util::string_view name, const std::string& options_str )
{
  using namespace actions::spells;
  using namespace actions::heals;

  if ( name == "smite" )
  {
    return new smite_t( *this, options_str );
  }
  if ( name == "levitate" )
  {
    return new levitate_t( *this, options_str );
  }
  if ( name == "power_word_shield" )
  {
    return new power_word_shield_t( *this, options_str );
  }
  if ( name == "power_word_fortitude" )
  {
    return new power_word_fortitude_t( *this, options_str );
  }
  if ( name == "shadowfiend" )
  {
    return new summon_shadowfiend_t( *this, options_str );
  }
  if ( name == "power_infusion" )
  {
    return new power_infusion_t( *this, options_str, "power_infusion" );
  }

  return base_t::create_action( name, options_str );
}

void priest_t::create_pets()
{
  base_t::create_pets();

  if ( find_action( "shadowfiend" ) )
  {
    pets.shadowfiend = create_pet( "shadowfiend" );
  }
}

void priest_t::init_base_stats()
{
  base_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 0.0;
  base.spell_power_per_intellect = 1.0;

  // resources.base_regen_per_second[ RESOURCE_MANA ] *= 1.0 + talents.enlightenment->effectN( 1 ).percent();
}

void priest_t::init_resources( bool force )
{
  base_t::init_resources( force );
}

void priest_t::init_scaling()
{
  base_t::init_scaling();
}

void priest_t::init_finished()
{
  base_t::init_finished();
}

void priest_t::init_spells()
{
  base_t::init_spells();

  // Generic Spells
  specs.mind_blast        = find_class_spell( "Mind Blast" );
  specs.shadow_word_death = find_class_spell( "Shadow Word: Death" );

  // DoT Spells
  dot_spells.devouring_plague = find_class_spell( "Devouring Plague" );
  dot_spells.shadow_word_pain = find_class_spell( "Shadow Word: Pain" );
  dot_spells.vampiric_touch   = find_class_spell( "Vampiric Touch" );
  dot_spells.vampiric_embrace = find_class_spell( "Vampiric Embrace" );
}

void priest_t::create_buffs()
{
  base_t::create_buffs();
}

void priest_t::init_rng()
{
  player_t::init_rng();
}

void priest_t::init_background_actions()
{
}

void priest_t::do_dynamic_regen( bool forced )
{
  player_t::do_dynamic_regen( forced );
}

void priest_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras( action );
}

void priest_t::invalidate_cache( cache_e cache )
{
  player_t::invalidate_cache( cache );
}

std::string priest_t::default_potion() const
{
  return priest_apl::potion( this );
}

std::string priest_t::default_flask() const
{
  return priest_apl::flask( this );
}

std::string priest_t::default_food() const
{
  return priest_apl::food( this );
}

std::string priest_t::default_rune() const
{
  return priest_apl::rune( this );
}

std::string priest_t::default_temporary_enchant() const
{
  return priest_apl::temporary_enchant( this );
}

const priest_td_t* priest_t::find_target_data( const player_t* target ) const
{
  return _target_data[ target ];
}

/**
 * Obtain target_data for given target.
 * Always returns non-null targetdata pointer
 */
priest_td_t* priest_t::get_target_data( player_t* target ) const
{
  priest_td_t*& td = _target_data[ target ];
  if ( !td )
  {
    td = new priest_td_t( target, const_cast<priest_t&>( *this ) );
  }
  return td;
}

void priest_t::init_action_list()
{
  // 2020-12-31: Healing is outdated and not supported (both discipline and holy)
  if ( !sim->allow_experimental_specializations && primary_role() == ROLE_HEAL )
  {
    if ( !quiet )
      sim->error( "Role heal for priest '{}' is currently not supported.", name() );

    quiet = true;
    return;
  }

  if ( !action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }
  clear_action_priority_lists();

  priest_apl::shadow( this );

  use_default_action_list = true;

  base_t::init_action_list();
}

void priest_t::combat_begin()
{
  player_t::combat_begin();
}

// priest_t::reset ==========================================================

void priest_t::reset()
{
  base_t::reset();

  // Reset Target Data
  for ( player_t* target : sim->target_list )
  {
    if ( auto td = _target_data[ target ] )
    {
      td->reset();
    }
  }
}

void priest_t::target_mitigation( school_e school, result_amount_type dt, action_state_t* s )
{
  base_t::target_mitigation( school, dt, s );
}

void priest_t::create_options()
{
  base_t::create_options();

  add_option( opt_bool( "priest.autounshift", options.autoUnshift ) );
  add_option( opt_bool( "priest.fixed_time", options.fixed_time ) );
  add_option( opt_bool( "priest.self_power_infusion", options.self_power_infusion ) );
}

std::string priest_t::create_profile( save_e type )
{
  std::string profile_str = base_t::create_profile( type );

  if ( type & SAVE_PLAYER )
  {
    if ( !options.autoUnshift )
    {
      profile_str += fmt::format( "priest.autounshift={}\n", options.autoUnshift );
    }

    if ( !options.fixed_time )
    {
      profile_str += fmt::format( "priest.fixed_time={}\n", options.fixed_time );
    }
  }

  return profile_str;
}

void priest_t::copy_from( player_t* source )
{
  base_t::copy_from( source );

  priest_t* source_p = debug_cast<priest_t*>( source );

  options = source_p->options;
}

void priest_t::arise()
{
  base_t::arise();
}

}  // namespace priestspace

const module_t* module_t::priest()
{
  static priestspace::priest_module_t m;
  return &m;
}
