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

    parse_buff_effects( p().o().buffs.voidform, 0x4U, false, false );  // Skip E3 for AM
    parse_buff_effects( p().o().buffs.shadowform );
    parse_buff_effects( p().o().buffs.twist_of_fate, p().o().talents.twist_of_fate );
    parse_buff_effects( p().o().buffs.devoured_pride );
    parse_buff_effects( p().o().buffs.dark_ascension, 0b1000U, false, false );  // Buffs non-periodic spells - Skip E4

    if ( p().o().talents.shadow.ancient_madness.enabled() )
    {
      // We use DA or VF spelldata to construct Ancient Madness to use the correct spell pass-list
      if ( p().o().talents.shadow.dark_ascension.enabled() )
      {
        parse_buff_effects( p().o().buffs.ancient_madness, 0b0001U, true, true );  // Skip E1
      }
      else
      {
        parse_buff_effects( p().o().buffs.ancient_madness, 0b0011U, true, true );  // Skip E1 and E2
      }
    }
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
}  // namespace actions

/**
 * Abstract base class for Shadowfiend and Mindbender
 */
struct base_fiend_pet_t : public priest_pet_t
{
  propagate_const<actions::inescapable_torment_t*> inescapable_torment;

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

  base_fiend_pet_t( priest_t* owner, util::string_view name, enum fiend_type type )
    : priest_pet_t( owner->sim, *owner, name ),
      inescapable_torment( nullptr ),
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
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override;
};

struct shadowfiend_pet_t final : public base_fiend_pet_t
{
  double power_leech_insanity;

  shadowfiend_pet_t( priest_t* owner, util::string_view name = "shadowfiend" )
    : base_fiend_pet_t( owner, name, fiend_type::Shadowfiend ),
      power_leech_insanity( o().find_spell( 262485 )->effectN( 1 ).resource( RESOURCE_INSANITY ) )
  {
    direct_power_mod = 0.408;  // New modifier after Spec Spell has been 0'd -- Anshlun 2020-10-06

    // Empirically tested to match 3/10/2023, actual value not available in spell data
    if ( owner->specialization() == PRIEST_DISCIPLINE )
    {
      direct_power_mod = 0.445;
    }
    npc_id = 19668;

    main_hand_weapon.min_dmg = owner->dbc->spell_scaling( owner->type, owner->level() ) * 2;
    main_hand_weapon.max_dmg = owner->dbc->spell_scaling( owner->type, owner->level() ) * 2;

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

  mindbender_pet_t( priest_t* owner, util::string_view name = "mindbender" )
    : base_fiend_pet_t( owner, name, fiend_type::Mindbender ),
      mindbender_spell( owner->find_spell( 123051 ) ),
      power_leech_insanity( o().find_spell( 200010 )->effectN( 1 ).resource( RESOURCE_INSANITY ) )
  {
    direct_power_mod = 0.442;  // New modifier after Spec Spell has been 0'd -- Anshlun 2020-10-06

    // Empirically tested to match 3/10/2023, actual value not available in spell data
    if ( owner->specialization() == PRIEST_DISCIPLINE )
    {
      direct_power_mod = 0.326;
    }
    npc_id = 62982;

    main_hand_weapon.min_dmg = owner->dbc->spell_scaling( owner->type, owner->level() ) * 2;
    main_hand_weapon.max_dmg = owner->dbc->spell_scaling( owner->type, owner->level() ) * 2;
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

    if ( o().talents.shadow.inescapable_torment.enabled() || o().talents.discipline.inescapable_torment.enabled() )
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

  void init() override
  {
    priest_pet_melee_t::init();

    merge_pet_stats_to_owner_action( p().o(), p(), *this, "Mindbender" );
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
        p().o().trigger_essence_devourer();
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
    spell_power_mod.direct *= ( 1 + p.o().talents.shadow.inescapable_torment->effectN( 3 ).percent() );

    // Tuning modifier effect
    apply_affecting_aura( p.o().specs.shadow_priest );
  }

  void init() override
  {
    priest_pet_spell_t::init();

    merge_pet_stats_to_owner_action( p().o(), p(), *this, "Mindbender" );
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
      duration( data().effectN( 2 ).time_value() )
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

  void init() override
  {
    priest_pet_spell_t::init();

    merge_pet_stats_to_owner_action( p().o(), p(), *this, "Mindbender" );
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

  inescapable_torment = new fiend::actions::inescapable_torment_t( *this );
}

action_t* base_fiend_pet_t::create_action( util::string_view name, util::string_view options_str )
{
  return priest_pet_t::create_action( name, options_str );
}

}  // namespace fiend

// ==========================================================================
// Idol of C'Thun
// ==========================================================================
struct void_tendril_t final : public priest_pet_t
{
  void_tendril_t( priest_t* owner ) : priest_pet_t( owner->sim, *owner, "void_tendril", true )
  {
    npc_id = 192337;
  }

  void init_action_list() override
  {
    priest_pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "mind_flay" );
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override;
};

// Insanity gain (377358)
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
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = priest_pet_spell_t::composite_da_multiplier( s );

    return m;
  }

  void init() override
  {
    priest_pet_spell_t::init();

    merge_pet_stats_to_owner_action( p().o(), p(), *this, "idol_of_cthun" );
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

    p().o().generate_insanity( void_tendril_insanity_gain, p().o().gains.insanity_idol_of_cthun_mind_flay,
                               d->state->action );
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
    npc_id = 198757;
  }

  void init_action_list() override
  {
    priest_pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "mind_sear" );
  }

  action_t* create_action( util::string_view name, util::string_view options_str ) override;
};

// Insanity gain (377358)
struct void_lasher_mind_sear_tick_t final : public priest_pet_spell_t
{
  void_lasher_mind_sear_tick_t( void_lasher_t& p, const spell_data_t* s ) : priest_pet_spell_t( "mind_sear_tick", p, s )
  {
    background = true;
    dual       = true;
    aoe        = -1;
    radius     = data().effectN( 2 ).radius_max();  // base radius is 100yd, actual is stored in effect 2
    affected_by_shadow_weaving = true;
    reduced_aoe_targets        = data().effectN( 3 ).base_value();

    if ( p.o().bugs )
    {
      da_multiplier_buffeffects.clear();  // This is in spelldata to scale with things but it does not in game
    }
  }

  void init() override
  {
    priest_pet_spell_t::init();

    merge_pet_stats_to_owner_action( p().o(), p(), *this, "idol_of_cthun" );
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

    p().o().generate_insanity( void_lasher_insanity_gain, p().o().gains.insanity_idol_of_cthun_mind_sear,
                               d->state->action );
  }

  void init() override
  {
    priest_pet_spell_t::init();

    merge_pet_stats_to_owner_action( p().o(), p(), *this, "idol_of_cthun" );
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

struct void_spike_t final : public priest_pet_spell_t
{
  void_spike_t( thing_from_beyond_t& p, util::string_view options )
    : priest_pet_spell_t( "void_spike", p, p.o().find_spell( 373279 ) )
  {
    parse_options( options );

    gcd_type = gcd_haste_type::SPELL_HASTE;

    aoe                        = -1;
    reduced_aoe_targets        = data().effectN( 3 ).base_value();
    radius                     = data().effectN( 1 ).radius_max();
    affected_by_shadow_weaving = true;
  }

  void init() override
  {
    priest_pet_spell_t::init();

    merge_pet_stats( p().o(), p(), *this );
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = priest_pet_spell_t::composite_da_multiplier( s );

    // Only hit for 30% of the damage when hitting off-targets.
    if ( target != s->target )
    {
      m *= data().effectN( 2 ).percent();
    }

    return m;
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
spawner::pet_spawner_t<pet_t, priest_t>& get_current_main_pet( priest_t& priest )
{
  return priest.talents.shadow.mindbender.enabled() ? priest.pets.mindbender : priest.pets.shadowfiend;
}

}  // namespace

namespace priestspace
{
void priest_t::trigger_inescapable_torment( player_t* target )
{
  if ( !talents.shadow.inescapable_torment.enabled() || !talents.discipline.inescapable_torment.enabled() )
    return;

  if ( get_current_main_pet( *this ).n_active_pets() > 0 )
  {
    auto extend = talents.shadow.inescapable_torment->effectN( 2 ).time_value();
    buffs.devoured_pride->extend_duration( this, extend );
    buffs.devoured_despair->extend_duration( this, extend );

    for ( auto a_pet : get_current_main_pet( *this ) )
    {
      auto pet = debug_cast<fiend::base_fiend_pet_t*>( a_pet );
      assert( pet->inescapable_torment );
      pet->inescapable_torment->set_target( target );
      pet->inescapable_torment->execute();
    }
  }
}

void priest_t::trigger_idol_of_yshaarj( player_t* target )
{
  if ( !talents.shadow.idol_of_yshaarj.enabled() )
    return;

  // TODO: Use Spell Data. Health threshold from blizzard post, no spell data yet.

  if ( target->buffs.stunned->check() )
  {
    buffs.devoured_despair->trigger();
  }
  else if ( target->health_percentage() >= 80.0 )
  {
    buffs.devoured_pride->trigger();
  }
  else
  {
    auto duration = timespan_t::from_seconds( talents.shadow.devoured_violence->effectN( 1 ).base_value() );

    for ( auto pet : get_current_main_pet( *this ) )
    {
      pet->adjust_duration( duration );
      procs.idol_of_yshaarj_extra_duration->occur();
    }
  }
}

std::unique_ptr<expr_t> priest_t::create_pet_expression( util::string_view expression_str,
                                                         util::span<util::string_view> splits )
{
  if ( splits.size() <= 2 )
  {
    return {};
  }

  if ( util::str_compare_ci( splits[ 0 ], "pet" ) )
  {
    if ( util::str_compare_ci( splits[ 1 ], "fiend" ) )
    {
      // pet.fiend.X refers to either shadowfiend or mindbender

      auto expr =
          get_current_main_pet( *this ).create_expression( util::make_span( splits ).subspan( 2 ), expression_str );
      if ( expr )
      {
        return expr;
      }

      auto tail = expression_str.substr( splits[ 0 ].length() + splits[ 1 ].length() + 2 );

      throw std::invalid_argument( fmt::format( "Unsupported pet expression '{}'.", tail ) );
    }
  }
  else if ( splits.size() == 3 && util::str_compare_ci( splits[ 0 ], "cooldown" ) )
  {
    if ( util::str_compare_ci( splits[ 1 ], "fiend" ) || util::str_compare_ci( splits[ 1 ], "shadowfiend" ) ||
         util::str_compare_ci( splits[ 1 ], "bender" ) || util::str_compare_ci( splits[ 1 ], "mindbender" ) )
    {
      if ( cooldown_t* cooldown = get_cooldown( talents.shadow.mindbender.enabled() ? "mindbender" : "shadowfiend" ) )
      {
        return cooldown->create_expression( splits[ 2 ] );
      }
      throw std::invalid_argument( fmt::format( "Cannot find any cooldown with name '{}'.",
                                                talents.shadow.mindbender.enabled() ? "mindbender" : "shadowfiend" ) );
    }
  }

  return {};
}

priest_t::priest_pets_t::priest_pets_t( priest_t& p )
  : shadowfiend( "shadowfiend", &p, []( priest_t* priest ) { return new fiend::shadowfiend_pet_t( priest ); } ),
    mindbender( "mindbender", &p, []( priest_t* priest ) { return new fiend::mindbender_pet_t( priest ); } ),
    void_tendril( "void_tendril", &p, []( priest_t* priest ) { return new void_tendril_t( priest ); } ),
    void_lasher( "void_lasher", &p, []( priest_t* priest ) { return new void_lasher_t( priest ); } ),
    thing_from_beyond( "thing_from_beyond", &p, []( priest_t* priest ) { return new thing_from_beyond_t( priest ); } )
{
  // Void Tendril: 377355
  // Void Lasher: 377355
  auto idol_of_cthun = p.find_spell( 377355 );
  // Add 1ms to ensure pet is dismissed after last dot tick.
  void_tendril.set_default_duration( idol_of_cthun->duration() + timespan_t::from_millis( 1 ) );
  void_lasher.set_default_duration( idol_of_cthun->duration() + timespan_t::from_millis( 1 ) );

  auto thing_from_beyond_spell = p.find_spell( 373277 );
  thing_from_beyond.set_default_duration( thing_from_beyond_spell->duration() );
}

}  // namespace priestspace