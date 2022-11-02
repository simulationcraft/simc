// ==========================================================================
// Priest Pets Sim File
// Contact: https://github.com/orgs/simulationcraft/teams/priest/members
// Wiki: https://github.com/simulationcraft/simc/wiki/Priests
// ==========================================================================

#include "sc_priest.hpp"
#include "util/util.hpp"

#include "simulationcraft.hpp"

using namespace priestspace;

namespace
{
// Merge pet stats with the same action from other pets, as well as with the owners action responsible for triggering
// this pet action.
void merge_pet_stats_to_owner_action( player_t& owner, pet_t& pet, action_t& action,
                                      util::string_view owner_action_name )
{
  auto first_pet = owner.find_pet( pet.name_str );
  if ( first_pet )
  {
    auto first_pet_action = first_pet->find_action( action.name_str );
    if ( first_pet_action )
    {
      if ( action.stats == first_pet_action->stats )
      {
        // This is the first pet created. Add its stat as a child to priest action associated with triggering this pet
        // spell
        auto owner_action = owner.find_action( owner_action_name );
        if ( owner_action )
        {
          owner_action->add_child( &action );
        }
      }
      if ( !owner.sim->report_pets_separately )
      {
        action.stats = first_pet_action->stats;
      }
    }
  }
}

// Merge pet stats with the same action from other pets
void merge_pet_stats( player_t& owner, pet_t& pet, action_t& action )
{
  if ( !owner.sim->report_pets_separately )
  {
    auto first_pet = owner.find_pet( pet.name_str );
    if ( first_pet )
    {
      auto first_pet_action = first_pet->find_action( action.name_str );
      if ( first_pet_action )
      {
        {
          action.stats = first_pet_action->stats;
        }
      }
    }
  }
}

namespace actions
{
struct shadow_spike_t;
struct shadow_spike_volley_t;
struct shadow_sear_t;
struct shadow_nova_t;
}  // namespace actions

/**
 * Pet base class
 *
 * Defines characteristics common to ALL priest pets.
 */
struct priest_pet_t : public pet_t
{
  priest_pet_t( sim_t* sim, priest_t& owner, util::string_view pet_name, bool guardian = false )
    : pet_t( sim, &owner, pet_name, PET_NONE, guardian )
  {
  }

  void init_base_stats() override
  {
    pet_t::init_base_stats();

    base.position = POSITION_BACK;
    base.distance = 3;

    owner_coeff.ap_from_sp = 1.0;
    owner_coeff.sp_from_sp = 1.0;
  }

  void schedule_ready( timespan_t delta_time, bool waiting ) override
  {
    if ( main_hand_attack && !main_hand_attack->execute_event )
    {
      main_hand_attack->schedule_execute();
    }

    pet_t::schedule_ready( delta_time, waiting );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = pet_t::composite_player_multiplier( school );

    return m;
  }

  resource_e primary_resource() const override
  {
    return RESOURCE_ENERGY;
  }

  void create_buffs() override
  {
    pet_t::create_buffs();

    buffs.power_infusion = make_buff( this, "power_infusion", find_spell( 10060 ) )
                               ->set_default_value_from_effect( 1 )
                               ->set_cooldown( 0_ms )
                               ->add_invalidate( CACHE_HASTE );
  }

  double composite_melee_haste() const override
  {
    double h = pet_t::composite_melee_haste();

    if ( buffs.power_infusion )
      h *= 1.0 / ( 1.0 + buffs.power_infusion->check_value() );

    return h;
  }

  double composite_spell_haste() const override
  {
    double h = pet_t::composite_spell_haste();

    if ( buffs.power_infusion )
      h *= 1.0 / ( 1.0 + buffs.power_infusion->check_value() );

    return h;
  }

  double composite_melee_speed() const override
  {
    double h = pet_t::composite_melee_speed();

    if ( buffs.power_infusion )
      h *= 1.0 / ( 1.0 + buffs.power_infusion->check_value() );

    return h;
  }

  double composite_spell_speed() const override
  {
    double h = pet_t::composite_spell_speed();

    if ( buffs.power_infusion )
      h *= 1.0 / ( 1.0 + buffs.power_infusion->check_value() );

    return h;
  }

  priest_t& o()
  {
    return static_cast<priest_t&>( *owner );
  }
  const priest_t& o() const
  {
    return static_cast<priest_t&>( *owner );
  }
};

struct priest_pet_melee_t : public melee_attack_t
{
  bool first_swing;

  priest_pet_melee_t( priest_pet_t& p, util::string_view name )
    : melee_attack_t( name, &p, spell_data_t::nil() ), first_swing( true )
  {
    school            = SCHOOL_SHADOW;
    weapon            = &( p.main_hand_weapon );
    weapon_multiplier = 1.0;
    base_execute_time = weapon->swing_time;
    may_crit          = true;
    background        = true;
    repeating         = true;
  }

  void reset() override
  {
    melee_attack_t::reset();
    first_swing = true;
  }

  timespan_t execute_time() const override
  {
    // First swing comes instantly after summoning the pet
    if ( first_swing )
      return timespan_t::zero();

    return melee_attack_t::execute_time();
  }

  void schedule_execute( action_state_t* state = nullptr ) override
  {
    melee_attack_t::schedule_execute( state );

    first_swing = false;
  }

  priest_pet_t& p()
  {
    return static_cast<priest_pet_t&>( *player );
  }
  const priest_pet_t& p() const
  {
    return static_cast<priest_pet_t&>( *player );
  }
};

struct priest_pet_spell_t : public spell_t, public parse_buff_effects_t<priest_td_t>
{
  bool affected_by_shadow_weaving;

  priest_pet_spell_t( util::string_view token, priest_pet_t& p, const spell_data_t* s )
    : spell_t( token, &p, s ), parse_buff_effects_t( this ), affected_by_shadow_weaving( false )
  {
    may_crit = true;

    if ( data().ok() )
    {
      apply_buff_effects();
    }
  }

  // Syntax: parse_buff_effects[<S[,S...]>]( buff[, ignore_mask|use_stacks[, use_default]][, spell1[,spell2...] )
  //  buff = buff to be checked for to see if effect applies
  //  ignore_mask = optional bitmask to skip effect# n corresponding to the n'th bit
  //  use_stacks = optional, default true, whether to multiply value by stacks
  //  use_default = optional, default false, whether to use buff's default value over effect's value
  //  S = optional list of template parameter(s) to indicate spell(s) with redirect effects
  //  spell = optional list of spell(s) with redirect effects that modify the effects on the buff
  void apply_buff_effects()
  {
    // using S = const spell_data_t*;

    parse_buff_effects( p().o().buffs.voidform );
    parse_buff_effects( p().o().buffs.shadowform );
    parse_buff_effects( p().o().buffs.twist_of_fate, p().o().talents.twist_of_fate );
    parse_buff_effects( p().o().buffs.devoured_pride );
    parse_buff_effects( p().o().buffs.dark_ascension, true );  // Buffs corresponding non-periodic spells
  }

  priest_pet_t& p()
  {
    return static_cast<priest_pet_t&>( *player );
  }
  const priest_pet_t& p() const
  {
    return static_cast<priest_pet_t&>( *player );
  }

  double cost() const override
  {
    double c = spell_t::cost() * std::max( 0.0, get_buff_effects_value( cost_buffeffects, false, false ) );
    return c;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double ta = spell_t::composite_ta_multiplier( s ) * get_buff_effects_value( ta_multiplier_buffeffects );
    return ta;
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double da = spell_t::composite_da_multiplier( s ) * get_buff_effects_value( da_multiplier_buffeffects );
    return da;
  }

  double composite_crit_chance() const override
  {
    double cc = spell_t::composite_crit_chance() + get_buff_effects_value( crit_chance_buffeffects, true );
    return cc;
  }

  timespan_t execute_time() const override
  {
    timespan_t et = spell_t::execute_time() * get_buff_effects_value( execute_time_buffeffects );
    return et;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t dd = spell_t::composite_dot_duration( s ) * get_buff_effects_value( dot_duration_buffeffects );
    return dd;
  }

  double recharge_multiplier( const cooldown_t& cd ) const override
  {
    double rm =
        action_t::recharge_multiplier( cd ) * get_buff_effects_value( recharge_multiplier_buffeffects, false, false );
    return rm;
  }

  double composite_target_da_multiplier( player_t* t ) const override
  {
    double tdm = action_t::composite_target_da_multiplier( t );

    if ( affected_by_shadow_weaving )
    {
      tdm *= p().o().shadow_weaving_multiplier( t, id );
    }

    return tdm;
  }

  double composite_target_ta_multiplier( player_t* t ) const override
  {
    double ttm = spell_t::composite_target_ta_multiplier( t );

    if ( affected_by_shadow_weaving )
    {
      ttm *= p().o().shadow_weaving_multiplier( t, id );
    }

    return ttm;
  }
};

namespace fiend
{
namespace actions
{
struct inescapable_torment_t;
struct shadowflame_prism_legendary_t;
}  // namespace actions

/**
 * Abstract base class for Shadowfiend and Mindbender
 */
struct base_fiend_pet_t : public priest_pet_t
{
  propagate_const<actions::inescapable_torment_t*> inescapable_torment;
  propagate_const<actions::shadowflame_prism_legendary_t*> shadowflame_prism_legendary;

  struct gains_t
  {
    propagate_const<gain_t*> fiend;
  } gains;

  enum class fiend_type
  {
    Shadowfiend,
    Mindbender
  } fiend_type;

  double direct_power_mod;

  base_fiend_pet_t( sim_t* sim, priest_t& owner, util::string_view name, enum fiend_type type )
    : priest_pet_t( sim, owner, name ),
      inescapable_torment( nullptr ),
      shadowflame_prism_legendary( nullptr ),
      gains(),
      fiend_type( type ),
      direct_power_mod( 0.0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.5 );

    owner_coeff.health = 0.3;
  }

  virtual double mana_return_percent() const = 0;
  virtual double insanity_gain() const       = 0;

  void init_action_list() override;

  void init_background_actions() override;

  void init_gains() override
  {
    priest_pet_t::init_gains();

    switch ( fiend_type )
    {
      case fiend_type::Mindbender:
      {
        gains.fiend = o().gains.mindbender;
      }
      break;
      default:
        gains.fiend = o().gains.shadowfiend;
        break;
    }
  }

  void init_resources( bool force ) override
  {
    priest_pet_t::init_resources( force );

    resources.initial[ RESOURCE_MANA ] = owner->resources.max[ RESOURCE_MANA ];
    resources.current = resources.max = resources.initial;
  }

  void arise() override
  {
    priest_pet_t::arise();
  }

  void demise() override
  {
    priest_pet_t::demise();
    o().buffs.devoured_pride->cancel();
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override;
};

struct shadowfiend_pet_t final : public base_fiend_pet_t
{
  double power_leech_insanity;

  shadowfiend_pet_t( sim_t* sim, priest_t& owner, util::string_view name = "shadowfiend" )
    : base_fiend_pet_t( sim, owner, name, fiend_type::Shadowfiend ),
      power_leech_insanity( o().find_spell( 262485 )->effectN( 1 ).resource( RESOURCE_INSANITY ) )
  {
    direct_power_mod = 0.408;  // New modifier after Spec Spell has been 0'd -- Anshlun 2020-10-06
    npc_id           = 19668;

    main_hand_weapon.min_dmg = owner.dbc->spell_scaling( owner.type, owner.level() ) * 2;
    main_hand_weapon.max_dmg = owner.dbc->spell_scaling( owner.type, owner.level() ) * 2;

    main_hand_weapon.damage = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
  }

  double mana_return_percent() const override
  {
    return 0.0;
  }
  double insanity_gain() const override
  {
    if ( o().talents.shadowfiend.enabled() )
    {
      return power_leech_insanity;
    }
    else
    {
      return 0;
    }
  }
};

struct mindbender_pet_t final : public base_fiend_pet_t
{
  const spell_data_t* mindbender_spell;
  double power_leech_insanity;

  mindbender_pet_t( sim_t* sim, priest_t& owner, util::string_view name = "mindbender" )
    : base_fiend_pet_t( sim, owner, name, fiend_type::Mindbender ),
      mindbender_spell( owner.find_spell( 123051 ) ),
      power_leech_insanity( o().find_spell( 200010 )->effectN( 1 ).resource( RESOURCE_INSANITY ) )
  {
    direct_power_mod = 0.442;  // New modifier after Spec Spell has been 0'd -- Anshlun 2020-10-06
    npc_id           = 62982;

    main_hand_weapon.min_dmg = owner.dbc->spell_scaling( owner.type, owner.level() ) * 2;
    main_hand_weapon.max_dmg = owner.dbc->spell_scaling( owner.type, owner.level() ) * 2;
    main_hand_weapon.damage  = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
  }

  double mana_return_percent() const override
  {
    double m = mindbender_spell->effectN( 1 ).percent();
    return m / 100;
  }

  double insanity_gain() const override
  {
    return power_leech_insanity;
  }

  void demise() override
  {
    base_fiend_pet_t::demise();

    if ( o().talents.shadow.inescapable_torment.enabled() )
    {
      if ( o().cooldowns.mind_blast->is_ready() )
      {
        o().procs.inescapable_torment_missed_mb->occur();
      }
      if ( o().cooldowns.shadow_word_death->is_ready() )
      {
        o().procs.inescapable_torment_missed_swd->occur();
      }
    }
  }
};

namespace actions
{
struct shadowcrawl_t final : public priest_pet_spell_t
{
  shadowcrawl_t( base_fiend_pet_t& p ) : priest_pet_spell_t( "shadowcrawl", p, p.find_pet_spell( "Shadowcrawl" ) )
  {
    may_miss = false;
    harmful  = false;
  }

  base_fiend_pet_t& p()
  {
    return static_cast<base_fiend_pet_t&>( *player );
  }
  const base_fiend_pet_t& p() const
  {
    return static_cast<base_fiend_pet_t&>( *player );
  }
};

struct fiend_melee_t : public priest_pet_melee_t
{
  fiend_melee_t( base_fiend_pet_t& p ) : priest_pet_melee_t( p, "melee" )
  {
    weapon                  = &( p.main_hand_weapon );
    weapon_multiplier       = 0.0;
    base_dd_min             = weapon->min_dmg;
    base_dd_max             = weapon->max_dmg;
    attack_power_mod.direct = p.direct_power_mod;
  }

  base_fiend_pet_t& p()
  {
    return static_cast<base_fiend_pet_t&>( *player );
  }
  const base_fiend_pet_t& p() const
  {
    return static_cast<base_fiend_pet_t&>( *player );
  }

  timespan_t execute_time() const override
  {
    if ( base_execute_time == timespan_t::zero() )
      return timespan_t::zero();

    // Mindbender inherits haste from the player
    timespan_t hasted_time = base_execute_time * player->cache.spell_speed();

    return hasted_time;
  }

  void impact( action_state_t* s ) override
  {
    priest_pet_melee_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      // Insanity generation hack for Shadow Weaving
      if ( p().o().specialization() == PRIEST_SHADOW )
      {
        p().o().trigger_shadow_weaving( s );
      }

      if ( p().o().talents.shadow.puppet_master.enabled() )
      {
        p().o().buffs.coalescing_shadows->trigger();
      }

      if ( p().o().talents.shadowfiend.enabled() || p().o().talents.shadow.mindbender.enabled() )
      {
        if ( p().o().specialization() == PRIEST_SHADOW )
        {
          double amount = p().insanity_gain();
          p().o().resource_gain( RESOURCE_INSANITY, amount, p().gains.fiend, nullptr );
        }
        else
        {
          double mana_reg_pct = p().mana_return_percent();
          if ( mana_reg_pct > 0.0 )
          {
            p().o().resource_gain( RESOURCE_MANA, p().o().resources.max[ RESOURCE_MANA ] * p().mana_return_percent(),
                                   p().gains.fiend );
          }
        }
      }
    }
  }
};

// ==========================================================================
// Shadowflame Rift (Shadowlands Legendary)
// ==========================================================================
struct shadowflame_rift_t final : public priest_pet_spell_t
{
  shadowflame_rift_t( base_fiend_pet_t& p ) : priest_pet_spell_t( "shadowflame_rift", p, p.o().find_spell( 344748 ) )
  {
    background                 = true;
    affected_by_shadow_weaving = true;

    // This is hard coded in the spell
    // Depending on Mindbender or Shadowfiend this hits differently
    switch ( p.fiend_type )
    {
      case base_fiend_pet_t::fiend_type::Mindbender:
      {
        spell_power_mod.direct *= 0.442;
      }
      break;
      default:
        spell_power_mod.direct *= 0.408;
        break;
    }
  }
};

// ==========================================================================
// Inescapable Torment Damage
// talent=373427
// ?     =373442
// ==========================================================================
struct inescapable_torment_damage_t final : public priest_pet_spell_t
{
  inescapable_torment_damage_t( base_fiend_pet_t& p )
    : priest_pet_spell_t( "inescapable_torment_damage", p, p.o().find_spell( 373442 ) )
  {
    background                 = true;
    affected_by_shadow_weaving = true;

    // This is hard coded in the spell
    // spcoeff * $?a137032[${0.326139}][${0.442}]
    if ( p.fiend_type == base_fiend_pet_t::fiend_type::Mindbender )
    {
      spell_power_mod.direct *= 0.442;
    }

    // Negative modifier used for point scaling
    // Effect#4 [op=set, values=(-50, 0)]
    spell_power_mod.direct *= ( 1 + p.o().talents.shadow.inescapable_torment->effectN( 4 ).percent() );
  }
};

// ==========================================================================
// Shadowflame Prism (Shadowlands Legendary)
// ==========================================================================
struct shadowflame_prism_legendary_t final : public priest_pet_spell_t
{
  timespan_t duration;

  shadowflame_prism_legendary_t( base_fiend_pet_t& p )
    : priest_pet_spell_t( "shadowflame_prism_legendary", p, p.o().find_spell( 336143 ) ),
      duration( timespan_t::from_seconds( data().effectN( 3 ).base_value() ) )
  {
    background = true;

    impact_action = new shadowflame_rift_t( p );
    add_child( impact_action );
  }

  void execute() override
  {
    priest_pet_spell_t::execute();

    auto& current_pet = p();

    if ( !current_pet.is_sleeping() )
    {
      auto remaining_duration = current_pet.expiration->remains();
      auto new_duration       = remaining_duration + duration;
      sim->print_debug( "Increasing {} duration by {}, new duration is {} up from {}.", current_pet.full_name_str,
                        duration, new_duration, remaining_duration );
      current_pet.expiration->reschedule( new_duration );
    }
  }
};

// ==========================================================================
// Inescapable Torment
// ==========================================================================
struct inescapable_torment_t final : public priest_pet_spell_t
{
  timespan_t duration;

  inescapable_torment_t( base_fiend_pet_t& p )
    : priest_pet_spell_t( "inescapable_torment", p, p.o().talents.shadow.inescapable_torment ),
      duration( timespan_t::from_seconds( data().effectN( 3 ).base_value() ) )
  {
    background = true;

    impact_action = new inescapable_torment_damage_t( p );
    add_child( impact_action );

    // Base spell also has damage values
    base_dd_min = base_dd_max = spell_power_mod.direct = 0.0;
  }

  void execute() override
  {
    priest_pet_spell_t::execute();

    auto& current_pet = p();

    if ( !current_pet.is_sleeping() )
    {
      auto remaining_duration = current_pet.expiration->remains();
      auto new_duration       = remaining_duration + duration;
      sim->print_debug( "Increasing {} duration by {}, new duration is {} up from {}.", current_pet.full_name_str,
                        duration, new_duration, remaining_duration );
      current_pet.expiration->reschedule( new_duration );
    }
  }
};
}  // namespace actions

void base_fiend_pet_t::init_action_list()
{
  main_hand_attack = new actions::fiend_melee_t( *this );

  if ( action_list_str.empty() )
  {
    action_priority_list_t* precombat = get_action_priority_list( "precombat" );
    precombat->add_action( "snapshot_stats",
                           "Snapshot raid buffed stats before combat begins and "
                           "pre-potting is done." );

    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "wait" );
  }

  priest_pet_t::init_action_list();
}

void base_fiend_pet_t::init_background_actions()
{
  priest_pet_t::init_background_actions();

  inescapable_torment         = new fiend::actions::inescapable_torment_t( *this );
  shadowflame_prism_legendary = new fiend::actions::shadowflame_prism_legendary_t( *this );
}

action_t* base_fiend_pet_t::create_action( util::string_view name, util::string_view options_str )
{
  return priest_pet_t::create_action( name, options_str );
}

}  // namespace fiend

struct priest_pallid_command_t : public priest_pet_t
{
  priest_pallid_command_t( priest_t* owner, util::string_view pet_name )
    : priest_pet_t( owner->sim, *owner, pet_name, true )
  {
  }

  void demise() override
  {
    priest_pet_t::demise();
    o().buffs.rigor_mortis->expire();
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override;
};

struct rattling_mage_t final : public priest_pallid_command_t
{
  rattling_mage_t( priest_t* owner ) : priest_pallid_command_t( owner, "rattling_mage" )
  {
  }

  void init_action_list() override
  {
    priest_pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "unholy_bolt" );
  }
};

struct cackling_chemist_t final : public priest_pallid_command_t
{
  cackling_chemist_t( priest_t* owner ) : priest_pallid_command_t( owner, "cackling_chemist" )
  {
  }

  void init_action_list() override
  {
    priest_pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "throw_viscous_concoction" );
  }
};

struct rattling_mage_unholy_bolt_t final : public priest_pet_spell_t
{
  propagate_const<buff_t*> rigor_mortis_buff;

  rattling_mage_unholy_bolt_t( priest_pallid_command_t& p, util::string_view options )
    : priest_pet_spell_t( "unholy_bolt", p, p.o().find_spell( 356431 ) ), rigor_mortis_buff( p.o().buffs.rigor_mortis )
  {
    // BUG: https://github.com/SimCMinMax/WoW-BugTracker/issues/854
    affected_by_shadow_weaving = false;
    parse_options( options );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = priest_pet_spell_t::composite_da_multiplier( s );

    m *= 1 + rigor_mortis_buff->check_stack_value();

    return m;
  }

  void init() override
  {
    priest_pet_spell_t::init();

    merge_pet_stats( p().o(), p(), *this );
  }
};

struct cackling_chemist_throw_viscous_concoction_t final : public priest_pet_spell_t
{
  propagate_const<buff_t*> rigor_mortis_buff;

  cackling_chemist_throw_viscous_concoction_t( priest_pallid_command_t& p, util::string_view options )
    : priest_pet_spell_t( "throw_viscous_concoction", p, p.o().find_spell( 356633 ) ),
      rigor_mortis_buff( p.o().buffs.rigor_mortis )
  {
    parse_options( options );
  }

  void init() override
  {
    priest_pet_spell_t::init();

    merge_pet_stats( p().o(), p(), *this );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = priest_pet_spell_t::composite_da_multiplier( s );

    m *= 1 + rigor_mortis_buff->check_stack_value();

    return m;
  }
};

action_t* priest_pallid_command_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "unholy_bolt" )
  {
    return new rattling_mage_unholy_bolt_t( *this, options_str );
  }

  if ( name == "throw_viscous_concoction" )
  {
    return new cackling_chemist_throw_viscous_concoction_t( *this, options_str );
  }

  return priest_pet_t::create_action( name, options_str );
}

// ==========================================================================
// Eternal Call to the Void and Idol of C'Thun
// ==========================================================================
struct void_tendril_t final : public priest_pet_t
{
  void_tendril_t( priest_t* owner ) : priest_pet_t( owner->sim, *owner, "void_tendril", true )
  {
  }

  void init_action_list() override
  {
    priest_pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "mind_flay" );
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override;
};

// Insanity gain (legendary=336214, cthun=377358)
struct void_tendril_mind_flay_t final : public priest_pet_spell_t
{
  double void_tendril_insanity_gain;

  void_tendril_mind_flay_t( void_tendril_t& p, util::string_view options )
    : priest_pet_spell_t( "mind_flay", p, p.o().find_spell( 193473 ) ),
      void_tendril_insanity_gain( p.o().find_spell( 377358 )->effectN( 1 ).resource( RESOURCE_INSANITY ) )
  {
    parse_options( options );
    channeled                  = true;
    hasted_ticks               = false;
    affected_by_shadow_weaving = true;

    // BUG: This talent is cursed
    // https://github.com/SimCMinMax/WoW-BugTracker/issues/1029
    if ( p.o().bugs )
    {
      if ( p.o().level() == 70 )
      {
        base_td += 1275;
      }
      else
      {
        base_td += 219;
      }

      spell_power_mod.tick *= 0.6;
    }
  }

  void init() override
  {
    priest_pet_spell_t::init();

    // TODO: remove after launch
    if ( p().o().talents.shadow.idol_of_cthun.enabled() )
    {
      merge_pet_stats_to_owner_action( p().o(), p(), *this, "idol_of_cthun" );
    }
    else
    {
      merge_pet_stats_to_owner_action( p().o(), p(), *this, "eternal_call_to_the_void" );
    }
  }

  timespan_t composite_dot_duration( const action_state_t* ) const override
  {
    // Not hasted
    return dot_duration;
  }

  timespan_t tick_time( const action_state_t* ) const override
  {
    // Not hasted
    return base_tick_time;
  }

  void tick( dot_t* d ) override
  {
    priest_pet_spell_t::tick( d );

    // TODO: remove after launch
    if ( p().o().talents.shadow.idol_of_cthun.enabled() )
    {
      p().o().generate_insanity( void_tendril_insanity_gain, p().o().gains.insanity_idol_of_cthun_mind_flay,
                                 d->state->action );
    }
    else
    {
      p().o().generate_insanity( void_tendril_insanity_gain, p().o().gains.insanity_eternal_call_to_the_void_mind_flay,
                                 d->state->action );
    }
  }
};

action_t* void_tendril_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "mind_flay" )
  {
    return new void_tendril_mind_flay_t( *this, options_str );
  }

  return priest_pet_t::create_action( name, options_str );
}

struct void_lasher_t final : public priest_pet_t
{
  void_lasher_t( priest_t* owner ) : priest_pet_t( owner->sim, *owner, "void_lasher", true )
  {
  }

  void init_action_list() override
  {
    priest_pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "mind_sear" );
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override;
};

// Insanity gain (legendary=208232, cthun=377358)
struct void_lasher_mind_sear_tick_t final : public priest_pet_spell_t
{
  void_lasher_mind_sear_tick_t( void_lasher_t& p, const spell_data_t* s ) : priest_pet_spell_t( "mind_sear_tick", p, s )
  {
    background = true;
    dual       = true;
    aoe        = -1;
    radius     = data().effectN( 2 ).radius_max();  // base radius is 100yd, actual is stored in effect 2

    // BUG: The damage this is dealing is not following spell data
    // https://github.com/SimCMinMax/WoW-BugTracker/issues/1029
    if ( p.o().bugs )
    {
      spell_power_mod.direct *= 0.6;
      da_multiplier_buffeffects.clear();  // This is in spelldata to scale with things but it does not in game
    }

    // BUG: This spell is not being affected by Mastery
    // https://github.com/SimCMinMax/WoW-BugTracker/issues/931
    if ( !p.o().bugs )
    {
      affected_by_shadow_weaving = true;
    }
  }

  void init() override
  {
    priest_pet_spell_t::init();

    // TODO: remove after launch
    if ( p().o().talents.shadow.idol_of_cthun.enabled() )
    {
      merge_pet_stats_to_owner_action( p().o(), p(), *this, "idol_of_cthun" );
    }
    else
    {
      merge_pet_stats_to_owner_action( p().o(), p(), *this, "eternal_call_to_the_void" );
    }
  }

  timespan_t composite_dot_duration( const action_state_t* ) const override
  {
    // Not hasted
    return dot_duration;
  }

  timespan_t tick_time( const action_state_t* ) const override
  {
    // Not hasted
    return base_tick_time;
  }
};

// Legendary: 344754 -> 344752
// Idol of C'thun: 394976 -> 394979
struct void_lasher_mind_sear_t final : public priest_pet_spell_t
{
  const double void_lasher_insanity_gain;

  void_lasher_mind_sear_t( void_lasher_t& p, util::string_view options )
    : priest_pet_spell_t( "mind_sear", p, p.o().find_spell( 394976 ) ),
      void_lasher_insanity_gain( p.o().find_spell( 377358 )->effectN( 1 ).resource( RESOURCE_INSANITY ) )
  {
    parse_options( options );
    channeled    = true;
    hasted_ticks = false;
    tick_action  = new void_lasher_mind_sear_tick_t( p, data().effectN( 1 ).trigger() );
  }

  // You only get the Insanity on your main target
  void tick( dot_t* d ) override
  {
    priest_pet_spell_t::tick( d );

    // TODO: remove after launch
    if ( p().o().talents.shadow.idol_of_cthun.enabled() )
    {
      p().o().generate_insanity( void_lasher_insanity_gain, p().o().gains.insanity_idol_of_cthun_mind_sear,
                                 d->state->action );
    }
    else
    {
      p().o().generate_insanity( void_lasher_insanity_gain, p().o().gains.insanity_eternal_call_to_the_void_mind_sear,
                                 d->state->action );
    }
  }
};

action_t* void_lasher_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "mind_sear" )
  {
    return new void_lasher_mind_sear_t( *this, options_str );
  }

  return priest_pet_t::create_action( name, options_str );
}

// ==========================================================================
// Thing From Beyond (Idol of Yoggsaron)
// ==========================================================================
struct thing_from_beyond_t final : public priest_pet_t
{
  thing_from_beyond_t( priest_t* owner ) : priest_pet_t( owner->sim, *owner, "thing_from_beyond", true )
  {
  }

  void init_action_list() override
  {
    priest_pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "void_spike" );
  }

  // Tracking buff to easily get pet uptime (especially in AoE this is easier)
  virtual void arise() override
  {
    pet_t::arise();

    o().buffs.thing_from_beyond->increment();
  }

  virtual void demise() override
  {
    pet_t::demise();

    o().buffs.thing_from_beyond->decrement();
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override;
};

struct void_spike_cleave_t final : public priest_pet_spell_t
{
  void_spike_cleave_t( thing_from_beyond_t& p )
    : priest_pet_spell_t( "void_spike_cleave", p, p.o().find_spell( 396895 ) )
  {
    aoe          = -1;
    travel_speed = 0;

    // Since we manually remove the main target add 1 to the scaling so it SQRT scales correctly
    reduced_aoe_targets = data().effectN( 3 ).base_value() + 1;
    radius              = data().effectN( 1 ).radius_max();

    background = dual = true;
    may_miss          = false;
    may_crit          = true;

    // BUG: Instead of triggering for 30% of the damage done it has its own spell power scaling
    // https://github.com/SimCMinMax/WoW-BugTracker/issues/1000
    if ( !p.o().bugs )
    {
      spell_power_mod.direct = 0.0;
      may_crit               = false;
    }
  }

  void init() override
  {
    priest_pet_spell_t::init();

    merge_pet_stats( p().o(), p(), *this );
  }

  // Does not hit your main target, but uses it to determine radius
  std::vector<player_t*>& target_list() const override
  {
    target_cache.is_valid = false;

    std::vector<player_t*>& tl = priest_pet_spell_t::target_list();

    tl.erase( std::remove( tl.begin(), tl.end(), target ), tl.end() );

    return tl;
  }

  void trigger( player_t* target, double original_amount )
  {
    // BUG: Instead of triggering for 30% of the damage done it has its own spell power scaling
    // https://github.com/SimCMinMax/WoW-BugTracker/issues/1000
    if ( !p().o().bugs )
    {
      base_dd_min = base_dd_max = original_amount * data().effectN( 2 ).percent();
    }

    set_target( target );
    execute();
  }
};

struct void_spike_t final : public priest_pet_spell_t
{
  propagate_const<void_spike_cleave_t*> child_void_spike_cleave;

  void_spike_t( thing_from_beyond_t& p, util::string_view options )
    : priest_pet_spell_t( "void_spike", p, p.o().find_spell( 373279 ) ), child_void_spike_cleave( nullptr )
  {
    parse_options( options );

    gcd_type = gcd_haste_type::SPELL_HASTE;

    child_void_spike_cleave = new void_spike_cleave_t( p );
    add_child( child_void_spike_cleave );

    // BUG: Does not scale with Mastery
    // https://github.com/SimCMinMax/WoW-BugTracker/issues/931
    // NOTE: a recent blue post said it should, but on beta it is not
    if ( !p.o().bugs )
    {
      affected_by_shadow_weaving = true;
    }
  }

  void init() override
  {
    priest_pet_spell_t::init();

    merge_pet_stats( p().o(), p(), *this );
  }

  void impact( action_state_t* s ) override
  {
    priest_pet_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      child_void_spike_cleave->trigger( s->target, s->result_amount );
    }
  }
};

action_t* thing_from_beyond_t::create_action( util::string_view name, util::string_view options_str )
{
  if ( name == "void_spike" )
  {
    return new void_spike_t( *this, options_str );
  }

  return priest_pet_t::create_action( name, options_str );
}

// Returns mindbender or shadowfiend, depending on talent choice. The returned pointer can be null if no fiend is
// summoned through the action list, so please check for null.
fiend::base_fiend_pet_t* get_current_main_pet( priest_t& priest )
{
  pet_t* current_main_pet =
      priest.talents.shadow.mindbender.enabled() ? priest.pets.mindbender : priest.pets.shadowfiend;

  return debug_cast<fiend::base_fiend_pet_t*>( current_main_pet );
}

}  // namespace

namespace priestspace
{
pet_t* priest_t::create_pet( util::string_view pet_name, util::string_view /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p )
    return p;

  if ( pet_name == "shadowfiend" )
  {
    return new fiend::shadowfiend_pet_t( sim, *this );
  }
  if ( pet_name == "mindbender" )
  {
    return new fiend::mindbender_pet_t( sim, *this );
  }

  sim->error( "{} Tried to create unknown priest pet {}.", *this, pet_name );

  return nullptr;
}

void priest_t::trigger_shadowflame_prism( player_t* target )
{
  auto current_pet = get_current_main_pet( *this );
  if ( current_pet && !current_pet->is_sleeping() )
  {
    if ( current_pet->o().talents.shadow.inescapable_torment.enabled() )
    {
      assert( current_pet->inescapable_torment );
      current_pet->inescapable_torment->set_target( target );
      current_pet->inescapable_torment->execute();
    }
    else
    {
      assert( current_pet->shadowflame_prism_legendary );
      current_pet->shadowflame_prism_legendary->set_target( target );
      current_pet->shadowflame_prism_legendary->execute();
    }
  }
}

std::unique_ptr<expr_t> priest_t::create_pet_expression( util::string_view expression_str,
                                                         util::span<util::string_view> splits )
{
  if ( splits.size() < 2 )
  {
    return {};
  }

  if ( util::str_compare_ci( splits[ 0 ], "pet" ) )
  {
    if ( util::str_compare_ci( splits[ 1 ], "fiend" ) )
    {
      // pet.fiend.X refers to either shadowfiend or mindbender
      pet_t* pet = get_current_main_pet( *this );
      if ( !pet )
      {
        return expr_t::create_constant( "no_fiend", 0.0 );
      }
      if ( splits.size() == 2 )
      {
        return expr_t::create_constant( "pet_index_expr", static_cast<double>( pet->actor_index ) );
      }
      // pet.foo.blah
      else
      {
        if ( splits[ 2 ] == "active" )
        {
          return make_fn_expr( expression_str, [ pet ] { return !pet->is_sleeping(); } );
        }
        else if ( splits[ 2 ] == "remains" )
        {
          return make_fn_expr( expression_str, [ pet ] {
            if ( pet->expiration && pet->expiration->remains() > timespan_t::zero() )
            {
              return pet->expiration->remains().total_seconds();
            }
            else
            {
              return 0.0;
            };
          } );
        }

        // build player/pet expression from the tail of the expression string.
        auto tail = expression_str.substr( splits[ 0 ].length() + splits[ 1 ].length() + 2 );
        if ( auto e = pet->create_expression( tail ) )
        {
          return e;
        }

        throw std::invalid_argument( fmt::format( "Unsupported pet expression '{}'.", tail ) );
      }
    }
  }
  else if ( splits.size() == 3 && util::str_compare_ci( splits[ 0 ], "cooldown" ) )
  {
    if ( util::str_compare_ci( splits[ 1 ], "fiend" ) || util::str_compare_ci( splits[ 1 ], "shadowfiend" ) ||
         util::str_compare_ci( splits[ 1 ], "bender" ) || util::str_compare_ci( splits[ 1 ], "mindbender" ) )
    {
      pet_t* pet = get_current_main_pet( *this );
      if ( !pet )
      {
        return expr_t::create_constant( "no_fiend", 0.0 );
      }
      if ( cooldown_t* cooldown = get_cooldown( pet->name_str ) )
      {
        return cooldown->create_expression( splits[ 2 ] );
      }
      throw std::invalid_argument( fmt::format( "Cannot find any cooldown with name '{}'.", pet->name_str ) );
    }
  }

  return {};
}

priest_t::priest_pets_t::priest_pets_t( priest_t& p )
  : shadowfiend(),
    mindbender(),
    void_tendril( "void_tendril", &p, []( priest_t* priest ) { return new void_tendril_t( priest ); } ),
    void_lasher( "void_lasher", &p, []( priest_t* priest ) { return new void_lasher_t( priest ); } ),
    rattling_mage( "rattling_mage", &p, []( priest_t* priest ) { return new rattling_mage_t( priest ); } ),
    cackling_chemist( "cackling_chemist", &p, []( priest_t* priest ) { return new cackling_chemist_t( priest ); } ),
    thing_from_beyond( "thing_from_beyond", &p, []( priest_t* priest ) { return new thing_from_beyond_t( priest ); } )
{
  // Void Tendril: legendary=193473 | cthun=377355
  // Void Lasher: legendary=336216 | cthun=377355
  auto idol_of_cthun = p.find_spell( 377355 );
  // Add 1ms to ensure pet is dismissed after last dot tick.
  void_tendril.set_default_duration( idol_of_cthun->duration() + timespan_t::from_millis( 1 ) );
  void_lasher.set_default_duration( idol_of_cthun->duration() + timespan_t::from_millis( 1 ) );

  auto rigor_mortis_duration = p.find_spell( 356467 )->duration();
  rattling_mage.set_default_duration( rigor_mortis_duration );
  cackling_chemist.set_default_duration( rigor_mortis_duration );

  auto thing_from_beyond_spell = p.find_spell( 373277 );
  thing_from_beyond.set_default_duration( thing_from_beyond_spell->duration() );
}

}  // namespace priestspace