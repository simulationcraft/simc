// ==========================================================================
// Priest Pets Sim File
// Contact: https://github.com/orgs/simulationcraft/teams/priest/members
// Wiki: https://github.com/simulationcraft/simc/wiki/Priests
// ==========================================================================

#include "sc_priest.hpp"
#include "util/util.hpp"

#include "simulationcraft.hpp"

namespace priestspace
{
namespace pets
{
/**
 * Pet base class
 *
 * Defines characteristics common to ALL priest pets.
 */
struct priest_pet_t : public pet_t
{
  priest_pet_t( sim_t* sim, priest_t& owner, util::string_view pet_name, pet_e pt, bool guardian = false )
    : pet_t( sim, &owner, pet_name, pt, guardian )
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

    // Orc racial
    m *= 1.0 + o().racials.command->effectN( 1 ).percent();

    return m;
  }

  resource_e primary_resource() const override
  {
    return RESOURCE_ENERGY;
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

  double composite_target_multiplier( player_t* target ) const override
  {
    double mul = attack_t::composite_target_multiplier( target );

    mul *= debug_cast<priest_t*>( debug_cast<priest_pet_t*>( player )->owner )->shadow_weaving_multiplier( target, 0 );

    return mul;
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
};

struct priest_pet_spell_t : public spell_t
{
  bool affected_by_shadow_weaving;

  priest_pet_spell_t( util::string_view token, priest_pet_t& p, const spell_data_t* s )
    : spell_t( token, &p, s ), affected_by_shadow_weaving( false )
  {
    may_crit = true;
  }

  priest_pet_t& p()
  {
    return static_cast<priest_pet_t&>( *player );
  }
  const priest_pet_t& p() const
  {
    return static_cast<priest_pet_t&>( *player );
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
    double ttm = action_t::composite_target_ta_multiplier( t );

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
struct shadowflame_prism_t;
}

/**
 * Abstract base class for Shadowfiend and Mindbender
 */
struct base_fiend_pet_t : public priest_pet_t
{
  propagate_const<actions::shadowflame_prism_t*> shadowflame_prism;

  struct gains_t
  {
    propagate_const<gain_t*> fiend;
  } gains;

  double direct_power_mod;

  base_fiend_pet_t( sim_t* sim, priest_t& owner, pet_e pt, util::string_view name )
    : priest_pet_t( sim, owner, name, pt ), shadowflame_prism( nullptr ), gains(), direct_power_mod( 0.0 )
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

    if ( o().specialization() == PRIEST_SHADOW )
    {
      gains.fiend = o().gains.insanity_pet;
    }
    else
    {
      switch ( pet_type )
      {
        case PET_MINDBENDER:
        {
          gains.fiend = o().gains.mindbender;
        }
        break;
        default:
          gains.fiend = get_gain( "basefiend" );
          break;
      }
    }
  }

  void init_resources( bool force ) override
  {
    priest_pet_t::init_resources( force );

    resources.initial[ RESOURCE_MANA ] = owner->resources.max[ RESOURCE_MANA ];
    resources.current = resources.max = resources.initial;
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override;
};

struct shadowfiend_pet_t final : public base_fiend_pet_t
{
  double power_leech_insanity;

  shadowfiend_pet_t( sim_t* sim, priest_t& owner, util::string_view name = "shadowfiend" )
    : base_fiend_pet_t( sim, owner, PET_SHADOWFIEND, name ),
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
    return power_leech_insanity;
  }
};

struct mindbender_pet_t final : public base_fiend_pet_t
{
  const spell_data_t* mindbender_spell;
  double power_leech_insanity;

  mindbender_pet_t( sim_t* sim, priest_t& owner, util::string_view name = "mindbender" )
    : base_fiend_pet_t( sim, owner, PET_MINDBENDER, name ),
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

    if ( p().o().conduits.rabid_shadows->ok() )
    {
      hasted_time /= 1.0 + p().o().conduits.rabid_shadows.percent();
    }

    return hasted_time;
  }

  void impact( action_state_t* s ) override
  {
    priest_pet_melee_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      if ( p().o().specialization() == PRIEST_SHADOW )
      {
        double amount = p().insanity_gain();
        if ( p().o().buffs.surrender_to_madness->up() )
        {
          p().o().resource_gain(
              RESOURCE_INSANITY,
              ( amount * ( 1.0 + p().o().talents.surrender_to_madness->effectN( 2 ).percent() ) ) - amount,
              p().o().gains.insanity_surrender_to_madness );
        }
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
};

// ==========================================================================
// Shadowflame Rift
// ==========================================================================
struct shadowflame_rift_t final : public priest_pet_spell_t
{
  shadowflame_rift_t( base_fiend_pet_t& p ) : priest_pet_spell_t( "shadowflame_rift", p, p.o().find_spell( 344748 ) )
  {
    background                 = true;
    affected_by_shadow_weaving = true;
    
    // This is hard coded in the spell
    // Depending on Mindbender or Shadowfiend this hits differently
    switch ( p.pet_type )
    {
      case PET_MINDBENDER:
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
// Shadowflame Prism
// ==========================================================================
struct shadowflame_prism_t final : public priest_pet_spell_t
{
  timespan_t duration;

  shadowflame_prism_t( base_fiend_pet_t& p )
    : priest_pet_spell_t( "shadowflame_prism", p, p.o().find_spell( 336143 ) ),
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

  shadowflame_prism = new fiend::actions::shadowflame_prism_t( *this );
}

action_t* base_fiend_pet_t::create_action( util::string_view name, const std::string& options_str )
{
  return priest_pet_t::create_action( name, options_str );
}

}  // namespace fiend

struct void_tendril_t final : public priest_pet_t
{
  void_tendril_t( priest_t* owner ) : priest_pet_t( owner->sim, *owner, "void_tendril", PET_VOID_TENDRIL, true )
  {
  }

  void init_action_list() override
  {
    priest_pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "mind_flay" );
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override;
};

struct void_tendril_mind_flay_t final : public priest_pet_spell_t
{
  const spell_data_t* void_tendril_insanity;

  void_tendril_mind_flay_t( void_tendril_t& p )
    : priest_pet_spell_t( "mind_flay", p, p.o().find_spell( 193473 ) ),
      void_tendril_insanity( p.o().find_spell( 336214 ) )
  {
    channeled                  = true;
    hasted_ticks               = false;
    affected_by_shadow_weaving = true;

    // Merge the stats object with other instances of the pet
    auto first_pet = p.o().find_pet( p.name_str );
    if ( first_pet )
    {
      auto first_pet_action = first_pet->find_action( name_str );
      if ( first_pet_action )
      {
        if ( stats == first_pet_action->stats )
        {
          // This is the first pet created. Add its stat as a child to priest mind_flay
          auto owner_ecttv_action = p.o().find_action( "eternal_call_to_the_void" );
          if ( owner_ecttv_action )
          {
            owner_ecttv_action->add_child( this );
          }
        }
        if ( !sim->report_pets_separately )
        {
          stats = first_pet_action->stats;
        }
      }
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

    p().o().generate_insanity( void_tendril_insanity->effectN( 1 ).base_value(),
                               p().o().gains.insanity_eternal_call_to_the_void_mind_flay, d->state->action );
  }
};

action_t* void_tendril_t::create_action( util::string_view name, const std::string& options_str )
{
  if ( name == "mind_flay" )
  {
    return new void_tendril_mind_flay_t( *this );
  }

  return priest_pet_t::create_action( name, options_str );
}

struct void_lasher_t final : public priest_pet_t
{
  void_lasher_t( priest_t* owner ) : priest_pet_t( owner->sim, *owner, "void_lasher", PET_VOID_LASHER, true )
  {
  }

  void init_action_list() override
  {
    priest_pet_t::init_action_list();

    action_priority_list_t* def = get_action_priority_list( "default" );
    def->add_action( "mind_sear" );
  }

  action_t* create_action( util::string_view name, const std::string& options_str ) override;
};

struct void_lasher_mind_sear_tick_t final : public priest_pet_spell_t
{
  const double void_lasher_insanity;

  void_lasher_mind_sear_tick_t( void_lasher_t& p, const spell_data_t* s )
    : priest_pet_spell_t( "mind_sear_tick", p, s ),
      void_lasher_insanity( p.o().find_spell( 208232 )->effectN( 1 ).resource( RESOURCE_INSANITY ) )
  {
    background = true;
    dual       = true;
    aoe        = -1;
    radius     = data().effectN( 2 ).radius_max();  // base radius is 100yd, actual is stored in effect 2
    affected_by_shadow_weaving = true;
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

  void impact( action_state_t* s ) override
  {
    priest_pet_spell_t::impact( s );

    p().o().generate_insanity( void_lasher_insanity, p().o().gains.insanity_eternal_call_to_the_void_mind_sear,
                               s->action );
  }
};

struct void_lasher_mind_sear_t final : public priest_pet_spell_t
{
  void_lasher_mind_sear_t( void_lasher_t& p ) : priest_pet_spell_t( "mind_sear", p, p.o().find_spell( 344754 ) )
  {
    channeled    = true;
    hasted_ticks = false;
    tick_action  = new void_lasher_mind_sear_tick_t( p, data().effectN( 1 ).trigger() );

    // Merge the stats object with other instances of the pet
    auto first_pet = p.o().find_pet( p.name_str );
    if ( first_pet )
    {
      auto first_pet_action = first_pet->find_action( name_str );
      if ( first_pet_action )
      {
        if ( stats == first_pet_action->stats )
        {
          // This is the first pet created. Add its stat as a child to priest mind_sear
          auto owner_ecttv_action = p.o().find_action( "eternal_call_to_the_void" );
          if ( owner_ecttv_action )
          {
            owner_ecttv_action->add_child( this );
          }
        }
        if ( !sim->report_pets_separately )
        {
          stats = first_pet_action->stats;
        }
      }
    }
  }
};

action_t* void_lasher_t::create_action( util::string_view name, const std::string& options_str )
{
  if ( name == "mind_sear" )
  {
    return new void_lasher_mind_sear_t( *this );
  }

  return priest_pet_t::create_action( name, options_str );
}

}  // namespace pets

pet_t* priest_t::create_pet( util::string_view pet_name, util::string_view /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p )
    return p;

  if ( pet_name == "shadowfiend" )
  {
    return new pets::fiend::shadowfiend_pet_t( sim, *this );
  }
  if ( pet_name == "mindbender" )
  {
    return new pets::fiend::mindbender_pet_t( sim, *this );
  }

  sim->error( "{} Tried to create unknown priest pet {}.", *this, pet_name );

  return nullptr;
}

// Returns mindbender or shadowfiend, depending on talent choice. The returned pointer can be null if no fiend is
// summoned through the action list, so please check for null.
static pets::fiend::base_fiend_pet_t* get_current_main_pet( priest_t& priest )
{
  pet_t* current_main_pet = priest.talents.mindbender->ok() ? priest.pets.mindbender : priest.pets.shadowfiend;
  return debug_cast<pets::fiend::base_fiend_pet_t*>( current_main_pet );
}

void priest_t::trigger_shadowflame_prism( player_t* target )
{
  auto current_pet = get_current_main_pet( *this );
  if ( current_pet && !current_pet->is_sleeping() )
  {
    assert( current_pet->shadowflame_prism );
    current_pet->shadowflame_prism->set_target( target );
    current_pet->shadowflame_prism->execute();
  }
}

std::unique_ptr<expr_t> priest_t::create_pet_expression( util::string_view expression_str,
                                                         util::span<util::string_view> splits )
{
  if ( splits.size() < 2 )
  {
    return nullptr;
  }

  if ( util::str_compare_ci( splits[ 0 ], "pet" ) )
  {
    if ( util::str_compare_ci( splits[ 1 ], "fiend" ) )
    {
      // pet.fiend.X refers to either shadowfiend or mindbender
      pet_t* pet = get_current_main_pet( *this );
      if ( !pet )
      {
        throw std::invalid_argument( "Cannot find any summoned fiend (shadowfiend/mindbender) pet ." );
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
        throw std::invalid_argument( "Cannot find any summoned fiend (shadowfiend/mindbender) pet." );
      }
      if ( cooldown_t* cooldown = get_cooldown( pet->name_str ) )
      {
        return cooldown->create_expression( splits[ 2 ] );
      }
      throw std::invalid_argument( fmt::format( "Cannot find any cooldown with name '{}'.", pet->name_str ) );
    }
  }

  return nullptr;
}

priest_t::priest_pets_t::priest_pets_t( priest_t& p )
  : shadowfiend(),
    mindbender(),
    void_tendril( "void_tendril", &p, []( priest_t* priest ) { return new pets::void_tendril_t( priest ); } ),
    void_lasher( "void_lasher", &p, []( priest_t* priest ) { return new pets::void_lasher_t( priest ); } )
{
  auto void_tendril_spell = p.find_spell( 193473 );
  // Add 1ms to ensure pet is dismissed after last dot tick.
  void_tendril.set_default_duration( void_tendril_spell->duration() + timespan_t::from_millis( 1 ) );

  auto void_lasher_spell = p.find_spell( 336216 );
  // Add 1ms to ensure pet is dismissed after last dot tick.
  void_lasher.set_default_duration( void_lasher_spell->duration() + timespan_t::from_millis( 1 ) );
}

}  // namespace priestspace