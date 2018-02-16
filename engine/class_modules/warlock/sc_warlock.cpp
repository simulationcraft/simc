#include "simulationcraft.hpp"
#include "sc_warlock.hpp"

namespace warlock { 
#define MAX_UAS 5
// Pets
namespace pets {
struct thalkiels_ascendance_pet_spell_t : public warlock_pet_spell_t
{
    bool is_usebale;
    thalkiels_ascendance_pet_spell_t( warlock_pet_t * p) :
        warlock_pet_spell_t("thalkiels_ascendance", p, p->o()->find_spell( 242832 ))
    {
        if(p->o()->specialization() == WARLOCK_DEMONOLOGY)
            is_usebale = true;
        else
            is_usebale = false;
        //?? Think thats all we need?
    }

    void execute() override
    {
        if(is_usebale)
            warlock_pet_spell_t::execute();
    }
};

struct rift_shadow_bolt_t: public warlock_pet_spell_t
{
  struct rift_shadow_bolt_tick_t : public warlock_pet_spell_t
  {
    rift_shadow_bolt_tick_t( warlock_pet_t* p ) :
      warlock_pet_spell_t( "shadow_bolt", p, p -> find_spell( 196657 ) )
    {
      background = dual = true;
      base_execute_time = timespan_t::zero();
    }

    virtual double composite_target_multiplier( player_t* target ) const override
    {
      double m = warlock_pet_spell_t::composite_target_multiplier( target );

      if ( target == p() -> o() -> havoc_target && p() -> o() -> legendary.odr_shawl_of_the_ymirjar )
        m *= 1.0 + p() -> find_spell( 212173 ) -> effectN( 1 ).percent();

      return m;
    }
  };

  rift_shadow_bolt_t( warlock_pet_t* p ) :
    warlock_pet_spell_t( "shadow_bolt", p, p -> find_spell( 196657 ) )
  {
    may_crit = may_miss = false;
    tick_may_crit = hasted_ticks = true;
    spell_power_mod.direct = 0;
    base_dd_min = base_dd_max = 0;
    dot_duration = timespan_t::from_millis( 14000 );
    base_tick_time = timespan_t::from_millis( 2000 );
    base_execute_time = timespan_t::zero();

    tick_action = new rift_shadow_bolt_tick_t( p );
  }

  void execute() override
  {
    warlock_pet_spell_t::execute();

    cooldown -> start( timespan_t::from_seconds( 16 ) );
  }
};

struct chaos_barrage_t : public warlock_pet_spell_t
{
  struct chaos_barrage_tick_t : public warlock_pet_spell_t
  {
    chaos_barrage_tick_t( warlock_pet_t* p ) :
      warlock_pet_spell_t( "chaos_barrage", p, p -> find_spell( 187394 ) )
    {
      background = dual = true;
      base_execute_time = timespan_t::zero();
    }

    virtual double composite_target_multiplier( player_t* target ) const override
    {
      double m = warlock_pet_spell_t::composite_target_multiplier( target );

      if ( target == p() -> o() -> havoc_target && p() -> o() -> legendary.odr_shawl_of_the_ymirjar )
        m *= 1.0 + p() -> find_spell( 212173 ) -> effectN( 1 ).percent();

      return m;
    }
  };

  chaos_barrage_t( warlock_pet_t* p ) :
    warlock_pet_spell_t( "chaos_barrage", p, p -> find_spell( 187394 ) )
  {
    may_crit = may_miss = false;
    tick_may_crit = hasted_ticks = true;
    spell_power_mod.direct = 0;
    base_dd_min = base_dd_max = 0;
    dot_duration = timespan_t::from_millis( 5500 );
    base_tick_time = timespan_t::from_millis( 250 );
    base_execute_time = timespan_t::zero();

    tick_action = new chaos_barrage_tick_t( p );
  }

  void execute() override
  {
    warlock_pet_spell_t::execute();

    cooldown -> start( timespan_t::from_seconds( 6 ) );
  }
};

struct rift_chaos_bolt_t : public warlock_pet_spell_t
{
  rift_chaos_bolt_t( warlock_pet_t* p ) :
    warlock_pet_spell_t( "chaos_bolt", p, p -> find_spell( 215279 ) )
  {
    base_execute_time = timespan_t::from_millis( 3000 );
    cooldown->duration = timespan_t::from_seconds( 5.5 );
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double m = warlock_pet_spell_t::composite_target_multiplier( target );

    if ( target == p() -> o() -> havoc_target && p() -> o() -> legendary.odr_shawl_of_the_ymirjar )
      m *= 1.0 + p() -> find_spell( 212173 ) -> effectN( 1 ).percent();

    return m;
  }

  // Force spell to always crit
  double composite_crit_chance() const override
  {
    return 1.0;
  }

  double calculate_direct_amount( action_state_t* state ) const override
  {
    warlock_pet_spell_t::calculate_direct_amount( state );

    // Can't use player-based crit chance from the state object as it's hardcoded to 1.0. Use cached
    // player spell crit instead. The state target crit chance of the state object is correct.
    // Targeted Crit debuffs function as a separate multiplier.
    state -> result_total *= 1.0 + player -> cache.spell_crit_chance() + state -> target_crit_chance;

    return state -> result_total;
  }
};

struct searing_bolt_t : public warlock_pet_spell_t
{
  searing_bolt_t( warlock_pet_t* p ) :
    warlock_pet_spell_t( "searing_bolt", p, p -> find_spell( 243050 ) )
  {
    may_crit = may_miss = hasted_ticks = false;
    tick_may_crit = true;
    base_execute_time = timespan_t::from_millis( 500 );
    base_costs[RESOURCE_ENERGY] = 1.0;
    resource_current = RESOURCE_ENERGY;
    dot_max_stack = 20;
  }
};

struct firebolt_t: public warlock_pet_spell_t
{
  firebolt_t( warlock_pet_t* p ):
    warlock_pet_spell_t( "Firebolt", p, p -> find_spell( 3110 ) )
  {
    base_multiplier *= 1.0;
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_pet_spell_t::action_multiplier();

    m *= 1.0 + p() -> o() -> spec.destruction -> effectN( 3 ).percent();

    return m;
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double m = warlock_pet_spell_t::composite_target_multiplier( target );

    warlock_td_t* td = this -> td( target );

    double immolate = 0;
    double multiplier = p() -> o() -> spec.firebolt_2 -> effectN( 1 ).percent();

    if( td -> dots_immolate -> is_ticking() )
      immolate += multiplier;

    m *= 1.0 + immolate;

    return m;
  }
};

struct dreadbite_t : public warlock_pet_melee_attack_t
{
  timespan_t dreadstalker_duration;
  double t21_4pc_increase;

  dreadbite_t( warlock_pet_t* p ) :
    warlock_pet_melee_attack_t( "Dreadbite", p, p -> find_spell( 205196 ) )
  {
    weapon = &( p -> main_hand_weapon );
    dreadstalker_duration = p -> find_spell( 193332 ) -> duration() +
                            ( p -> o() -> sets->has_set_bonus( WARLOCK_DEMONOLOGY, T19, B4 )
                              ? p -> o() -> sets->set( WARLOCK_DEMONOLOGY, T19, B4 ) -> effectN( 1 ).time_value()
                              : timespan_t::zero() );
    t21_4pc_increase = p->o()->sets->set( WARLOCK_DEMONOLOGY, T21, B4 )->effectN( 1 ).percent();
  }

  virtual bool ready() override
  {
    if ( p()->dreadbite_executes <= 0 )
      return false;

    return warlock_pet_melee_attack_t::ready();
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_pet_melee_attack_t::action_multiplier();

    if ( p()->o()->sets->has_set_bonus( WARLOCK_DEMONOLOGY, T21, B4 ) && p()->bites_executed == 1 )
      m *= 1.0 + t21_4pc_increase;

    return m;
  }

  void execute() override
  {
    warlock_pet_melee_attack_t::execute();

    p()->dreadbite_executes--;
  }

  void impact( action_state_t* s ) override
  {
    warlock_pet_melee_attack_t::impact( s );

    p()->bites_executed++;
  }
};

struct legion_strike_t: public warlock_pet_melee_attack_t
{
  legion_strike_t( warlock_pet_t* p ):
    warlock_pet_melee_attack_t( p, "Legion Strike" )
  {
    aoe = -1;
    weapon = &( p -> main_hand_weapon );
  }

  virtual bool ready() override
  {
    if ( p() -> special_action -> get_dot() -> is_ticking() ) return false;

    return warlock_pet_melee_attack_t::ready();
  }
};

struct axe_toss_t : public warlock_pet_spell_t
{
  axe_toss_t( warlock_pet_t* p ) :
    warlock_pet_spell_t( "Axe Toss", p, p -> find_spell( 89766 ) )
  {
  }

  void execute() override
  {
    warlock_pet_spell_t::execute();

    p() -> trigger_sephuzs_secret( execute_state, MECHANIC_STUN );
  }
};

struct felstorm_tick_t: public warlock_pet_melee_attack_t
{
  felstorm_tick_t( warlock_pet_t* p, const spell_data_t& s ):
    warlock_pet_melee_attack_t( "felstorm_tick", p, s.effectN( 1 ).trigger() )
  {
    aoe = -1;
    background = true;
    weapon = &( p -> main_hand_weapon );
  }
};

struct felstorm_t: public warlock_pet_melee_attack_t
{
  felstorm_t( warlock_pet_t* p ) :
    warlock_pet_melee_attack_t( "felstorm", p, p -> find_spell( 89751 ) )
  {
    tick_zero = true;
    hasted_ticks = false;
    may_miss = false;
    may_crit = false;
    weapon_multiplier = 0;

    dynamic_tick_action = true;
    tick_action = new felstorm_tick_t( p, data() );
  }

  virtual void cancel() override
  {
    warlock_pet_melee_attack_t::cancel();

    get_dot() -> cancel();
  }

  virtual void execute() override
  {
    warlock_pet_melee_attack_t::execute();

    p() -> melee_attack -> cancel();
  }

  virtual void last_tick( dot_t* d ) override
  {
    warlock_pet_melee_attack_t::last_tick( d );

    if ( ! p() -> is_sleeping() && ! p() -> melee_attack -> target -> is_sleeping() )
      p() -> melee_attack -> execute();
  }
};

struct shadow_bite_t: public warlock_pet_spell_t
{
  double shadow_bite_mult;
  shadow_bite_t( warlock_pet_t* p ):
    warlock_pet_spell_t( p, "Shadow Bite" ),
    shadow_bite_mult( 0.0 )
  {
    shadow_bite_mult = p -> o() -> spec.shadow_bite_2 -> effectN( 1 ).percent();
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double m = warlock_pet_spell_t::composite_target_multiplier( target );

    warlock_td_t* td = this -> td( target );

    double dots = 0;

    for ( int i = 0; i < MAX_UAS; i++ )
      if ( td -> dots_unstable_affliction[i] -> is_ticking() )
        dots += shadow_bite_mult;

    if ( td -> dots_agony -> is_ticking() )
      dots += shadow_bite_mult;

    if ( td -> dots_corruption -> is_ticking() )
      dots += shadow_bite_mult;

    m *= 1.0 + dots;

    return m;
  }
};

struct lash_of_pain_t: public warlock_pet_spell_t
{
  lash_of_pain_t( warlock_pet_t* p ):
    warlock_pet_spell_t( p, "Lash of Pain" )
  {
  }
};

struct whiplash_t: public warlock_pet_spell_t
{
  whiplash_t( warlock_pet_t* p ):
    warlock_pet_spell_t( p, "Whiplash" )
  {
    aoe = -1;
  }
};

struct torment_t: public warlock_pet_spell_t
{
  torment_t( warlock_pet_t* p ):
    warlock_pet_spell_t( p, "Torment" )
  { }
};

struct immolation_tick_t : public warlock_pet_spell_t
{
  immolation_tick_t( warlock_pet_t* p, const spell_data_t& s ) :
    warlock_pet_spell_t( "immolation_tick", p, s.effectN( 1 ).trigger() )
  {
    aoe = -1;
    background = true;
    may_crit = true;
  }
};

struct immolation_t : public warlock_pet_spell_t
{
  immolation_t( warlock_pet_t* p, const std::string& options_str ) :
    warlock_pet_spell_t( "immolation", p, p -> find_spell( 19483 ) )
  {
    parse_options( options_str );

    dynamic_tick_action = hasted_ticks = true;
    tick_action = new immolation_tick_t( p, data() );
  }

  void init() override
  {
    warlock_pet_spell_t::init();

    // Explicitly snapshot haste, as the spell actually has no duration in spell data
    snapshot_flags |= STATE_HASTE;
    update_flags |= STATE_HASTE;
  }

  timespan_t composite_dot_duration( const action_state_t* ) const override
  {
    return player -> sim -> expected_iteration_time * 2;
  }

  virtual void cancel() override
  {
    dot_t* dot = find_dot( target );
    if ( dot && dot -> is_ticking() )
    {
      dot -> cancel();
    }
    action_t::cancel();
  }
};

struct doom_bolt_t: public warlock_pet_spell_t
{
  doom_bolt_t( warlock_pet_t* p ):
    warlock_pet_spell_t( "Doom Bolt", p, p -> find_spell( 85692 ) )
  {
    if ( p -> o() -> talents.grimoire_of_supremacy -> ok() )
      base_multiplier *= 1.0;

    if ( p -> o() -> talents.grimoire_of_supremacy -> ok() )
      base_multiplier *= 1.0 + p -> find_spell( 152107 ) -> effectN( 6 ).percent();
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_pet_spell_t::action_multiplier();

    if ( p() -> o() -> specialization() == WARLOCK_DESTRUCTION )
      m *= 1.0 + p() -> o() -> spec.destruction -> effectN( 3 ).percent();

    return m;
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double m = warlock_pet_spell_t::composite_target_multiplier( target );

    if ( target -> health_percentage() < 20 )
    {
      m *= 1.0 + data().effectN( 2 ).percent();
    }
    return m;
  }
};

struct shadow_lock_t : public warlock_pet_spell_t
{
  shadow_lock_t( warlock_pet_t* p ) :
    warlock_pet_spell_t( "Shadow Lock", p, p -> find_spell( 171138 ) )
  {
  }

  void execute() override
  {
    warlock_pet_spell_t::execute();

    p() -> trigger_sephuzs_secret( execute_state, MECHANIC_INTERRUPT );
  }
};

struct meteor_strike_t: public warlock_pet_spell_t
{
  meteor_strike_t( warlock_pet_t* p, const std::string& options_str ):
    warlock_pet_spell_t( "Meteor Strike", p, p -> find_spell( 171018 ) )
  {
    parse_options( options_str );
    aoe = -1;
  }

  void execute() override
  {
    warlock_pet_spell_t::execute();

    p() -> trigger_sephuzs_secret( execute_state, MECHANIC_STUN );
  }
};

struct fel_firebolt_t: public warlock_pet_spell_t
{
  double jaws_of_shadow_multiplier;
  fel_firebolt_t( warlock_pet_t* p ):
    warlock_pet_spell_t( "fel_firebolt", p, p -> find_spell( 104318 ) )
  {
      base_multiplier *= 1.0;
      jaws_of_shadow_multiplier = p -> o() -> find_spell( 238109 ) -> effectN( 1 ).percent();
  }

  virtual bool ready() override
  {
    return spell_t::ready();
  }

  void execute() override
  {
    warlock_pet_spell_t::execute();
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_pet_spell_t::impact( s );
    if ( result_is_hit( s -> result ) )
    {
      if ( rng().roll( p() -> o() -> sets->set( WARLOCK_DEMONOLOGY, T18, B4 ) -> effectN( 1 ).percent() ) )
      {
        p() -> o() -> buffs.t18_4pc_driver -> trigger();
      }
    }
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double m = warlock_pet_spell_t::composite_target_multiplier( target );

    warlock_td_t* td = this -> td( target );

    m *= td -> agony_stack;

    if ( td -> debuffs_jaws_of_shadow -> check() )
      m *= 1.0 + jaws_of_shadow_multiplier;

    return m;
  }
};

struct eye_laser_t : public warlock_pet_spell_t
{
  eye_laser_t( warlock_pet_t* p ) :
    warlock_pet_spell_t( "eye_laser", p, p -> find_spell( 205231 ) )
  { }

  void schedule_travel( action_state_t* state ) override
  {
    if ( td( state -> target ) -> dots_doom -> is_ticking() )
    {
      warlock_pet_spell_t::schedule_travel( state );
    }
    // Need to release the state since it's not going to be used by a travel event, nor impacting of
    // any kind.
    else
    {
      action_state_t::release( state );
    }
  }
};

warlock_pet_t::warlock_pet_t( sim_t* sim, warlock_t* owner, const std::string& pet_name, pet_e pt, bool guardian ):
pet_t( sim, owner, pet_name, pt, guardian ), special_action( nullptr ), special_action_two( nullptr ), melee_attack( nullptr ), summon_stats( nullptr ), ascendance( nullptr )
{
  owner_coeff.ap_from_sp = 1.0;
  owner_coeff.sp_from_sp = 1.0;
  owner_coeff.health = 0.5;

//  ascendance = new thalkiels_ascendance_pet_spell_t( this );
}

void warlock_pet_t::init_base_stats()
{
  pet_t::init_base_stats();

  resources.base[RESOURCE_ENERGY] = 200;
  base_energy_regen_per_second = 10;

  base.spell_power_per_intellect = 1;

  intellect_per_owner = 0;
  stamina_per_owner = 0;

  main_hand_weapon.type = WEAPON_BEAST;

  //double dmg = dbc.spell_scaling( owner -> type, owner -> level );

  main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
}

bool warlock_pet_t::create_actions()
{
    bool check = pet_t::create_actions();
    if(check)
    {
        ascendance = new thalkiels_ascendance_pet_spell_t( this ); //????
        return true;
    }
    else
        return false;
}

void warlock_pet_t::init_action_list()
{
  if ( special_action )
  {
    if ( type == PLAYER_PET )
      special_action -> background = true;
    else
      special_action -> action_list = get_action_priority_list( "default" );
  }

  if ( special_action_two )
  {
    if ( type == PLAYER_PET )
      special_action_two -> background = true;
    else
      special_action_two -> action_list = get_action_priority_list( "default" );
  }

  pet_t::init_action_list();

  if ( summon_stats )
    for ( size_t i = 0; i < action_list.size(); ++i )
      summon_stats -> add_child( action_list[i] -> stats );
}

void warlock_pet_t::create_buffs()
{
  pet_t::create_buffs();

  buffs.demonic_synergy = buff_creator_t( this, "demonic_synergy", find_spell( 171982 ) )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .chance( 1 );

  buffs.demonic_empowerment = haste_buff_creator_t( this, "demonic_empowerment", find_spell( 193396 ) )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .chance( 1 );

  buffs.the_expendables = buff_creator_t( this, "the_expendables", find_spell( 211218 ))
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .chance( 1 )
    .default_value( find_spell( 211218 ) -> effectN( 1 ).percent() );
  buffs.rage_of_guldan = buff_creator_t( this, "rage_of_guldan", find_spell( 257926 ) )
	  .add_invalidate(CACHE_PLAYER_DAMAGE_MULTIPLIER); //change spell id to 253014 when whitelisted
}

void warlock_pet_t::schedule_ready( timespan_t delta_time, bool waiting )
{
  dot_t* d;
  if ( melee_attack && ! melee_attack -> execute_event && ! ( special_action && ( d = special_action -> get_dot() ) && d -> is_ticking() ) )
  {
    melee_attack -> schedule_execute();
  }

  pet_t::schedule_ready( delta_time, waiting );
}

double warlock_pet_t::composite_player_multiplier( school_e school ) const
{
  double m = pet_t::composite_player_multiplier( school );

  m *= 1.0 + o() -> buffs.tier18_2pc_demonology -> stack_value();

  if ( buffs.demonic_synergy -> up() )
    m *= 1.0 + buffs.demonic_synergy -> data().effectN( 1 ).percent();

  if( buffs.the_expendables -> up() )
  {
      m*= 1.0 + buffs.the_expendables -> stack_value();
  }

  if ( buffs.rage_of_guldan->up() )
	  m *= 1.0 + ( buffs.rage_of_guldan->default_value / 100 );

  if ( o() -> buffs.soul_harvest -> check() )
    m *= 1.0 + o() -> buffs.soul_harvest -> stack_value() ;

  if ( is_grimoire_of_service )
  {
      m *= 1.0 + o() -> find_spell( 216187 ) -> effectN( 1 ).percent();
  }

  if( o() -> mastery_spells.master_demonologist -> ok() && buffs.demonic_empowerment -> check() )
  {
     m *= 1.0 +  o() -> cache.mastery_value();
  }

  m *= 1.0 + o() -> buffs.sindorei_spite -> check_stack_value();
  m *= 1.0 + o() -> buffs.lessons_of_spacetime -> check_stack_value();

  if ( o() -> specialization() == WARLOCK_AFFLICTION )
    m *= 1.0 + o() -> spec.affliction -> effectN( 3 ).percent();

  return m;
}

double warlock_pet_t::composite_melee_crit_chance() const
{
  double mc = pet_t::composite_melee_crit_chance();
  return mc;
}

double warlock_pet_t::composite_spell_crit_chance() const
{
  double sc = pet_t::composite_spell_crit_chance();

  return sc;
}

double warlock_pet_t::composite_melee_haste() const
{
  double mh = pet_t::composite_melee_haste();

  if ( buffs.demonic_empowerment -> up() )
  {
    mh /= 1.0 + buffs.demonic_empowerment -> data().effectN( 2 ).percent();
  }

  return mh;
}

double warlock_pet_t::composite_spell_haste() const
{
  double sh = pet_t::composite_spell_haste();

  if ( buffs.demonic_empowerment -> up() )
    sh /= 1.0 + buffs.demonic_empowerment -> data().effectN( 2 ).percent();

  return sh;
}

double warlock_pet_t::composite_melee_speed() const
{
  // Make sure we get our overridden haste values applied to melee_speed
  double cmh = pet_t::composite_melee_speed();

  if ( buffs.demonic_empowerment->up() )
    cmh /= 1.0 + buffs.demonic_empowerment->data().effectN( 2 ).percent();

  return cmh;
}

double warlock_pet_t::composite_spell_speed() const
{
  // Make sure we get our overridden haste values applied to spell_speed
  double css = pet_t::composite_spell_speed();

  if ( buffs.demonic_empowerment->up() )
    css /= 1.0 + buffs.demonic_empowerment->data().effectN( 2 ).percent();

  return css;
}

struct imp_pet_t: public warlock_pet_t
{
  imp_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "imp" ):
    warlock_pet_t( sim, owner, name, PET_IMP, name != "imp" )
  {
    action_list_str = "firebolt";
    //owner_coeff.sp_from_sp *= 1.2;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "firebolt" ) return new firebolt_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct felguard_pet_t: public warlock_pet_t
{
  felguard_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "felguard" ):
    warlock_pet_t( sim, owner, name, PET_FELGUARD, name != "felguard" )
  {
    action_list_str += "/felstorm";
    action_list_str += "/legion_strike,if=cooldown.felstorm.remains";
    owner_coeff.ap_from_sp = 1.1; // HOTFIX
    owner_coeff.ap_from_sp *= 1.2; // PTR
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    melee_attack = new warlock_pet_melee_t( this );
    special_action = new felstorm_t( this );
    special_action_two = new axe_toss_t( this );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = warlock_pet_t::composite_player_multiplier( school );

    if ( !is_grimoire_of_service )
      m *= 1.0;

    return m;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "legion_strike" ) return new legion_strike_t( this );
    if ( name == "felstorm" ) return new felstorm_t( this );
    if ( name == "axe_toss" ) return new axe_toss_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct t18_illidari_satyr_t: public warlock_pet_t
{
  t18_illidari_satyr_t(sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "illidari_satyr", PET_FELGUARD, true )
  {
    owner_coeff.ap_from_sp = 1;
    is_demonbolt_enabled = false;
    regen_type = REGEN_DISABLED;
    action_list_str = "travel";
  }

  void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    base_energy_regen_per_second = 0;
    melee_attack = new warlock_pet_melee_t( this );
  }
};

struct t18_prince_malchezaar_t: public warlock_pet_t
{
  t18_prince_malchezaar_t(  sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "prince_malchezaar", PET_GHOUL, true )
  {
    owner_coeff.ap_from_sp = 1;
    regen_type = REGEN_DISABLED;
    action_list_str = "travel";
  }

  void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    base_energy_regen_per_second = 0;
    melee_attack = new warlock_pet_melee_t( this );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = warlock_pet_t::composite_player_multiplier( school );
    m *= 9.45; // Prince deals 9.45 times normal damage.. you know.. for reasons.
    return m;
  }
};

struct t18_vicious_hellhound_t: public warlock_pet_t
{
  t18_vicious_hellhound_t( sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "vicious_hellhound", PET_DOG, true )
  {
    owner_coeff.ap_from_sp = 1;
    is_demonbolt_enabled = false;
    regen_type = REGEN_DISABLED;
    action_list_str = "travel";
  }

  void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    base_energy_regen_per_second = 0;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.0 );
    melee_attack = new warlock_pet_melee_t( this );
    melee_attack -> base_execute_time = timespan_t::from_seconds( 1.0 );
  }
};

struct chaos_tear_t : public warlock_pet_t
{
  chaos_tear_t( sim_t* sim, warlock_t* owner ) :
    warlock_pet_t( sim, owner, "chaos_tear", PET_NONE, true )
  {
    action_list_str = "chaos_bolt";
    regen_type = REGEN_DISABLED;
  }

  void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    base_energy_regen_per_second = 0;
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str ) override
  {
    if ( name == "chaos_bolt" ) return new rift_chaos_bolt_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

namespace shadowy_tear {

  struct shadowy_tear_t;

  struct shadowy_tear_td_t : public actor_target_data_t
  {
    dot_t* dots_shadow_bolt;

  public:
    shadowy_tear_td_t( player_t* target, shadowy_tear_t* shadowy );
  };

  struct shadowy_tear_t : public warlock_pet_t
  {
    target_specific_t<shadowy_tear_td_t> target_data;

    shadowy_tear_t( sim_t* sim, warlock_t* owner ) :
      warlock_pet_t( sim, owner, "shadowy_tear", PET_NONE, true )
    {
      action_list_str = "shadow_bolt";
      regen_type = REGEN_DISABLED;
    }

    void init_base_stats() override
    {
      warlock_pet_t::init_base_stats();
      base_energy_regen_per_second = 0;
    }

    shadowy_tear_td_t* td( player_t* t ) const
    {
      return get_target_data( t );
    }

    virtual shadowy_tear_td_t* get_target_data( player_t* target ) const override
    {
      shadowy_tear_td_t*& td = target_data[target];
      if ( !td )
        td = new shadowy_tear_td_t( target, const_cast< shadowy_tear_t* >( this ) );
      return td;
    }

    virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
    {
      if ( name == "shadow_bolt" ) return new rift_shadow_bolt_t( this );

      return warlock_pet_t::create_action( name, options_str );
    }
  };

  shadowy_tear_td_t::shadowy_tear_td_t( player_t* target, shadowy_tear_t* shadowy ):
    actor_target_data_t( target, shadowy )
  {
    dots_shadow_bolt = target -> get_dot( "shadow_bolt", shadowy );
  }
}

namespace flame_rift {

  struct flame_rift_t : public warlock_pet_t
  {
    flame_rift_t( sim_t* sim, warlock_t* owner ) :
      warlock_pet_t( sim, owner, "flame_rift", PET_NONE, true )
    {
      action_list_str = "searing_bolt";
    }

    void init_base_stats() override
    {
      warlock_pet_t::init_base_stats();
      base_energy_regen_per_second = 0;
      resources.base[RESOURCE_ENERGY] = 20.0;
    }

    virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
    {
      if ( name == "searing_bolt" ) return new searing_bolt_t( this );

      return warlock_pet_t::create_action( name, options_str );
    }
  };
}

namespace chaos_portal {

  struct chaos_portal_td_t : public actor_target_data_t
  {
    dot_t* dots_chaos_barrage;

  public:
    chaos_portal_td_t( player_t* target, chaos_portal_t* chaosy );
  };

  struct chaos_portal_t : public warlock_pet_t
  {
    target_specific_t<chaos_portal_td_t> target_data;
    stats_t** chaos_barrage_stats;
    stats_t* regular_stats;

    chaos_portal_t( sim_t* sim, warlock_t* owner ) :
      warlock_pet_t( sim, owner, "chaos_portal", PET_NONE, true ), chaos_barrage_stats( nullptr ), regular_stats( nullptr )
    {
      action_list_str = "chaos_barrage";
      regen_type = REGEN_DISABLED;
    }

    void init_base_stats() override
    {
      warlock_pet_t::init_base_stats();
      base_energy_regen_per_second = 0;
    }

    chaos_portal_td_t* td( player_t* t ) const
    {
      return get_target_data( t );
    }

    virtual chaos_portal_td_t* get_target_data( player_t* target ) const override
    {
      chaos_portal_td_t*& td = target_data[target];
      if ( !td )
        td = new chaos_portal_td_t( target, const_cast< chaos_portal_t* >( this ) );
      return td;
    }

    virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
    {
      if ( name == "chaos_barrage" ) return new chaos_barrage_t( this );

      return warlock_pet_t::create_action( name, options_str );
    }
  };

  chaos_portal_td_t::chaos_portal_td_t( player_t* target, chaos_portal_t* chaosy ):
    actor_target_data_t( target, chaosy )
  {
    dots_chaos_barrage = target -> get_dot( "chaos_barrage", chaosy );
  }
}

struct felhunter_pet_t: public warlock_pet_t
{
  felhunter_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "felhunter" ):
    warlock_pet_t( sim, owner, name, PET_FELHUNTER, name != "felhunter" )
  {
    action_list_str = "shadow_bite";

    owner_coeff.ap_from_sp *= 1.2; //Hotfixed no spelldata, live as of 05-24-2017
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    melee_attack = new warlock_pet_melee_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "shadow_bite" ) return new shadow_bite_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct succubus_pet_t: public warlock_pet_t
{
  succubus_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "succubus" ):
    warlock_pet_t( sim, owner, name, PET_SUCCUBUS, name != "succubus" )
  {
    action_list_str = "lash_of_pain";
    owner_coeff.ap_from_sp = 0.5;
    owner_coeff.ap_from_sp *= 1.2;
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    main_hand_weapon.swing_time = timespan_t::from_seconds( 3.0 );
    melee_attack = new warlock_pet_melee_t( this );
    if ( ! util::str_compare_ci( name_str, "service_succubus" ) )
      special_action = new whiplash_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "lash_of_pain" ) return new lash_of_pain_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct voidwalker_pet_t: public warlock_pet_t
{
  voidwalker_pet_t( sim_t* sim, warlock_t* owner, const std::string& name = "voidwalker" ):
    warlock_pet_t( sim, owner, name, PET_VOIDWALKER, name != "voidwalker" )
  {
    action_list_str = "torment";
    owner_coeff.ap_from_sp *= 1.2; // PTR
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    melee_attack = new warlock_pet_melee_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "torment" ) return new torment_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct infernal_t: public warlock_pet_t
{
  infernal_t( sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "infernal", PET_INFERNAL )
  {
    owner_coeff.health = 0.4;
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    action_list_str = "immolation,if=!ticking";
    if ( o() -> talents.grimoire_of_supremacy -> ok() )
      action_list_str += "/meteor_strike,if=time>1";
    melee_attack = new warlock_pet_melee_t( this );

    resources.base[RESOURCE_ENERGY] = 0;
    base_energy_regen_per_second = 0;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = warlock_pet_t::composite_player_multiplier( school );

    if ( o() -> talents.grimoire_of_supremacy -> ok() )
      m *= 1.0 / 2.0;
    return m;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "immolation" ) return new immolation_t( this, options_str );
    if ( name == "meteor_strike" ) return new meteor_strike_t( this, options_str );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct lord_of_flames_infernal_t : public warlock_pet_t
{
  timespan_t duration;

  lord_of_flames_infernal_t( sim_t* sim, warlock_t* owner ) :
    warlock_pet_t( sim, owner, "lord_of_flames_infernal", PET_INFERNAL )
  {
    duration = o() -> find_spell( 226804 ) -> duration() + timespan_t::from_millis( 1 );
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    action_list_str = "immolation,if=!ticking";
    resources.base[RESOURCE_ENERGY] = 0;
    base_energy_regen_per_second = 0;
    melee_attack = new warlock_pet_melee_t( this );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "immolation" ) return new immolation_t( this, options_str );

    return warlock_pet_t::create_action( name, options_str );
  }

  void trigger()
  {
    if ( ! o() -> buffs.lord_of_flames -> up() )
      summon( duration );
  }
};

struct doomguard_t: public warlock_pet_t
{
    doomguard_t( sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "doomguard", PET_DOOMGUARD )
  {
    owner_coeff.health = 0.4;
    action_list_str = "doom_bolt";
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    resources.base[RESOURCE_ENERGY] = 100;
    base_energy_regen_per_second = 12;

    special_action = new shadow_lock_t( this );
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = warlock_pet_t::composite_player_multiplier( school );

    if ( o() -> talents.grimoire_of_supremacy -> ok() )
      m *= 1.0 / 2.0;
    return m;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "doom_bolt" ) return new doom_bolt_t( this );
    if ( name == "shadow_lock" ) return new shadow_lock_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct wild_imp_pet_t: public warlock_pet_t
{
  action_t* firebolt;
  bool isnotdoge;

  wild_imp_pet_t( sim_t* sim, warlock_t* owner ):
    warlock_pet_t( sim, owner, "wild_imp", PET_WILD_IMP )
  {
  }

  virtual void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();

    action_list_str = "fel_firebolt";

    resources.base[RESOURCE_ENERGY] = 1000;
    base_energy_regen_per_second = 0;
  }

  void dismiss( bool expired ) override
  {
      pet_t::dismiss( expired );
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str ) override
  {
    if ( name == "fel_firebolt" )
    {
      firebolt = new fel_firebolt_t( this );
      return firebolt;
    }

    return warlock_pet_t::create_action( name, options_str );
  }

  void arise() override
  {
      warlock_pet_t::arise();

      if( isnotdoge )
      {
         firebolt -> cooldown -> start( timespan_t::from_millis( rng().range( 500 , 1500 )));
      }
  }

  void trigger(int timespan, bool isdoge = false )
  {
    isnotdoge = !isdoge;
    summon( timespan_t::from_millis( timespan ) );
  }
};

struct dreadstalker_t : public warlock_pet_t
{
  dreadstalker_t( sim_t* sim, warlock_t* owner ) :
    warlock_pet_t( sim, owner, "dreadstalker", PET_DREADSTALKER )
  {
    action_list_str = "travel/dreadbite";
    regen_type = REGEN_DISABLED;
    owner_coeff.health = 0.4;
    owner_coeff.ap_from_sp = 1.1; // HOTFIX
  }

  virtual double composite_melee_crit_chance() const override
  {
      double pw = warlock_pet_t::composite_melee_crit_chance();

      return pw;
  }

  virtual double composite_spell_crit_chance() const override
  {
      double pw = warlock_pet_t::composite_spell_crit_chance();
      return pw;
  }

  double composite_player_multiplier( school_e school ) const override
  {
    double m = warlock_pet_t::composite_player_multiplier( school );
    m *= 0.76; // FIXME dreadstalkers do 76% damage for no apparent reason, thanks blizzard.
    return m;
  }

  void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    resources.base[RESOURCE_ENERGY] = 0;
    base_energy_regen_per_second = 0;
    melee_attack = new warlock_pet_melee_t( this );
  }

  void arise() override
  {
    warlock_pet_t::arise();

    dreadbite_executes = 1;
    bites_executed = 0;

    if ( o()->sets->has_set_bonus( WARLOCK_DEMONOLOGY, T21, B4 ) )
      t21_4pc_reset = false;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "dreadbite" ) return new dreadbite_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

struct darkglare_t : public warlock_pet_t
{
  darkglare_t( sim_t* sim, warlock_t* owner ) :
    warlock_pet_t( sim, owner, "darkglare", PET_OBSERVER )
  {
    action_list_str = "eye_laser";
    regen_type = REGEN_DISABLED;
    owner_coeff.health = 0.4;
  }

  void init_base_stats() override
  {
    warlock_pet_t::init_base_stats();
    resources.base[RESOURCE_ENERGY] = 0;
    base_energy_regen_per_second = 0;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str ) override
  {
    if ( name == "eye_laser" ) return new eye_laser_t( this );

    return warlock_pet_t::create_action( name, options_str );
  }
};

} // end namespace pets

void parse_spell_coefficient(action_t& a)
{
    for (size_t i = 1; i <= a.data()._effects->size(); i++)
    {
        if (a.data().effectN(i).type() == E_SCHOOL_DAMAGE)
            a.spell_power_mod.direct = a.data().effectN(i).sp_coeff();
        else if (a.data().effectN(i).type() == E_APPLY_AURA && a.data().effectN(i).subtype() == A_PERIODIC_DAMAGE)
            a.spell_power_mod.tick = a.data().effectN(i).sp_coeff();
    }
}
// Spells
namespace actions {
// Demonology Spells

struct shadow_bolt_t: public warlock_spell_t
{
  cooldown_t* icd;

  shadow_bolt_t( warlock_t* p ):
    warlock_spell_t( p, "Shadow Bolt" )
  {
    energize_type = ENERGIZE_ON_CAST;
    energize_resource = RESOURCE_SOUL_SHARD;
    energize_amount = 1;

    icd = p -> get_cooldown( "discord_icd" );

    if ( p -> sets->set( WARLOCK_DEMONOLOGY, T17, B4 ) )
    {
      if ( rng().roll( p->sets->set( WARLOCK_DEMONOLOGY, T17, B4 )->effectN( 1 ).percent() ) )
      {
        energize_amount++;
      }
    }
  }

  virtual bool ready() override
  {
    if ( p() -> talents.demonbolt -> ok() )
      return false;

    return warlock_spell_t::ready();
  }

  virtual timespan_t execute_time() const override
  {
    if ( p() -> buffs.shadowy_inspiration -> check() )
    {
      return timespan_t::zero();
    }
    return warlock_spell_t::execute_time();
  }

  virtual double action_multiplier()const override
  {
      double m = warlock_spell_t::action_multiplier();
      if( p() -> buffs.stolen_power -> up() )
      {
          p() -> procs.stolen_power_used -> occur();
          m *= 1.0 + p() -> buffs.stolen_power -> data().effectN( 1 ).percent();
          p() -> buffs.stolen_power->reset();
      }
      return m;
  }

  void execute() override
  {
    warlock_spell_t::execute();

    if ( p() -> talents.demonic_calling -> ok() && rng().roll( p() -> talents.demonic_calling -> proc_chance() ) )
      p() -> buffs.demonic_calling -> trigger();

    if ( p() -> buffs.shadowy_inspiration -> check() )
      p() -> buffs.shadowy_inspiration -> expire();

    if( p() -> sets->has_set_bonus( WARLOCK_DEMONOLOGY, T18, B2 ) )
    {
        p() -> buffs.tier18_2pc_demonology -> trigger( 1 );
    }

    if ( p() -> sets -> has_set_bonus( WARLOCK_DEMONOLOGY, T20, B2 ) && p() -> rng().roll( p() -> sets -> set( WARLOCK_DEMONOLOGY, T20, B2 ) -> proc_chance() ) )
    {
        p() -> cooldowns.call_dreadstalkers -> reset( true );
        p() -> procs.demonology_t20_2pc -> occur();
    }
  }
};

struct doom_t: public warlock_spell_t
{
  double kazzaks_final_curse_multiplier;

  doom_t( warlock_t* p ):
    warlock_spell_t( "doom", p, p -> spec.doom ),
    kazzaks_final_curse_multiplier( 0.0 )
  {
    base_tick_time = p -> find_spell( 603 ) -> effectN( 1 ).period();
    dot_duration = p -> find_spell( 603 ) -> duration();
    spell_power_mod.tick = p -> spec.doom -> effectN( 1 ).sp_coeff();

    base_multiplier *= 1.0;

    may_crit = true;
    hasted_ticks = true;

    energize_type = ENERGIZE_PER_TICK;
    energize_resource = RESOURCE_SOUL_SHARD;
    energize_amount = 1;

    if ( p -> talents.impending_doom -> ok() )
    {
      base_tick_time += p -> find_spell( 196270 ) -> effectN( 1 ).time_value();
      dot_duration += p -> find_spell( 196270 ) -> effectN( 1 ).time_value();
    }
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t duration = warlock_spell_t::composite_dot_duration( s );
    return duration * p() -> cache.spell_haste();
  }

  virtual double action_multiplier()const override
  {
    double m = warlock_spell_t::action_multiplier();
    double pet_counter = 0.0;

    if ( kazzaks_final_curse_multiplier > 0.0 )
    {
      for ( auto& pet : p() -> pet_list )
      {
        pets::warlock_pet_t *lock_pet = static_cast< pets::warlock_pet_t* > ( pet );

        if ( lock_pet != NULL )
        {
          if ( !lock_pet -> is_sleeping() )
          {
            pet_counter += kazzaks_final_curse_multiplier;
          }
        }
      }
      m *= 1.0 + pet_counter;
    }
    return m;
  }

  virtual void tick( dot_t* d ) override
  {
    warlock_spell_t::tick( d );

    if(  d -> state -> result == RESULT_HIT || result_is_hit( d -> state -> result) )
    {
      if( p() -> talents.impending_doom -> ok() )
      {
        //trigger_wild_imp( p() );
        p() -> procs.impending_doom -> occur();
      }

      if ( p()->sets->has_set_bonus( WARLOCK_DEMONOLOGY, T19, B2 ) && rng().roll( p() -> sets->set( WARLOCK_DEMONOLOGY, T19, B2 ) -> effectN( 1 ).percent() ) )
        p() -> resource_gain( RESOURCE_SOUL_SHARD, 1, p() -> gains.t19_2pc_demonology );
    }
  }
};

struct demonic_empowerment_t: public warlock_spell_t
{
  double power_trip_rng;

  demonic_empowerment_t (warlock_t* p) :
    warlock_spell_t( "demonic empowerment", p, p -> spec.demonic_empowerment )
  {
    may_crit = false;
    harmful = false;
    dot_duration = timespan_t::zero();

    power_trip_rng = p -> talents.power_trip -> effectN( 1 ).percent();
  }

  void execute() override
  {
    warlock_spell_t::execute();
    for( auto& pet : p() -> pet_list )
    {
      pets::warlock_pet_t *lock_pet = static_cast<pets::warlock_pet_t*> ( pet );

      if( lock_pet != nullptr )
      {
        if( !lock_pet -> is_sleeping() )
        {
          lock_pet -> buffs.demonic_empowerment -> trigger();
        }
      }
    }

    if ( p()->sets->has_set_bonus( WARLOCK_DEMONOLOGY, T21, B4 ) )
    {
      for ( size_t i = 0; i < p()->warlock_pet_list.dreadstalkers.size(); i++ )
      {
        if ( !p()->warlock_pet_list.dreadstalkers[i]->is_sleeping() )
        {
          if ( !p()->warlock_pet_list.dreadstalkers[i]->t21_4pc_reset )
          {
            p()->warlock_pet_list.dreadstalkers[i]->dreadbite_executes++;
            p()->warlock_pet_list.dreadstalkers[i]->t21_4pc_reset = true;
          }
        }
      }
    }

    if ( p() -> talents.power_trip -> ok() && rng().roll( power_trip_rng ) )
    {
        struct pt_delay_event: public player_event_t
        {
          gain_t* shard_gain;
          warlock_t* pl;


          pt_delay_event( warlock_t* p ):
            player_event_t( *p, timespan_t::from_millis(100) ), shard_gain( p -> gains.soul_conduit ), pl(p)
          {
          }
          virtual const char* name() const override
          { return "powertrip_delay_event"; }
          virtual void execute() override
          {
              pl -> resource_gain( RESOURCE_SOUL_SHARD, 1, pl -> gains.power_trip );

          }
        };

        make_event<pt_delay_event>( *p()->sim, p());


//        p() -> resource_gain( RESOURCE_SOUL_SHARD, 1, p() -> gains.power_trip );
    }

    if ( p() -> talents.shadowy_inspiration -> ok() )
      p() -> buffs.shadowy_inspiration -> trigger();

  }
};

struct hand_of_guldan_t: public warlock_spell_t
{
  struct trigger_imp_event_t : public player_event_t
  {
    bool initiator;
    int count;
    trigger_imp_event_t( warlock_t* p, int c, bool init = false ) :
      player_event_t( *p, timespan_t::from_millis(1) ), initiator( init ), count( c )//Use original corruption until DBC acts more friendly.
    {
      //add_event( rng().range( timespan_t::from_millis( 500 ),
      //  timespan_t::from_millis( 1500 ) ) );
    }

    virtual const char* name() const override
    {
      return  "trigger_imp";
    }

    virtual void execute() override
    {
      warlock_t* p = static_cast< warlock_t* >( player() );
    }
  };

  trigger_imp_event_t* imp_event;
  int shards_used;
  double demonology_trinket_chance;
  doom_t* doom;

  hand_of_guldan_t( warlock_t* p ):
    warlock_spell_t( p, "Hand of Gul'dan" ),
    demonology_trinket_chance( 0.0 ),
    doom( new doom_t( p ) )
  {
    aoe = -1;
    shards_used = 0;

    parse_effect_data( p -> find_spell( 86040 ) -> effectN( 1 ) );

    doom -> background = true;
    doom -> dual = true;
    doom -> base_costs[RESOURCE_MANA] = 0;
    base_multiplier *= 1.0;
  }

  virtual timespan_t travel_time() const override
  {
    return timespan_t::from_millis( 700 );
  }

  virtual bool ready() override
  {
    bool r = warlock_spell_t::ready();

    if ( p() -> resources.current[RESOURCE_SOUL_SHARD] == 0.0 )
      r = false;

    return r;
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    m *= last_resource_cost;

    return m;
  }

  void consume_resource() override
  {
    warlock_spell_t::consume_resource();

    shards_used = last_resource_cost;

    if ( last_resource_cost == 1.0 )
      p() -> procs.one_shard_hog -> occur();
    if ( last_resource_cost == 2.0 )
      p() -> procs.two_shard_hog -> occur();
    if ( last_resource_cost == 3.0 )
      p() -> procs.three_shard_hog -> occur();
    if ( last_resource_cost == 4.0 )
      p() -> procs.four_shard_hog -> occur();
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if( p() -> talents.hand_of_doom -> ok() )
      {
        doom -> target = s -> target;
        doom -> execute();
      }
      if ( p() -> sets->set( WARLOCK_DEMONOLOGY, T17, B2 ) )
      {
        if ( rng().roll( p() -> sets->set( WARLOCK_DEMONOLOGY, T17, B2 ) -> proc_chance() ) )
        {
          shards_used *= 1.5;
        }
      }

      if ( s -> chain_target == 0 )
        imp_event =  make_event<trigger_imp_event_t>( *sim, p(), floor( shards_used ), true);

	    if ( p()->sets->has_set_bonus(WARLOCK_DEMONOLOGY, T21, B2))
      {
		    for (int i = 0; i < shards_used; i++) 
        {
			    p()->buffs.rage_of_guldan->trigger();
		    }
	    }
    }
  }
};

//Destruction Spells

struct flames_of_argus_t: public residual_action_t
{
  flames_of_argus_t( warlock_t* player ) :
    residual_action_t( "flames_of_argus", player, player -> find_spell( 253097 ) )
  {
    background = true;
    may_miss = may_crit = false;
    school = SCHOOL_CHROMATIC;
  }
};

struct havoc_t: public warlock_spell_t
{
  timespan_t havoc_duration;

  havoc_t( warlock_t* p ):
    warlock_spell_t( p, "Havoc" )
  {
    may_crit = false;

    havoc_duration = p -> find_spell( 80240 ) -> duration();
    if ( p -> talents.wreak_havoc -> ok() )
    {
      cooldown -> duration += p -> find_spell( 196410 ) -> effectN( 1 ).time_value();
    }
  }

  void execute() override
  {
    warlock_spell_t::execute();
    p() -> havoc_target = execute_state -> target;

    p() -> buffs.active_havoc -> trigger();
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    td( s -> target ) -> debuffs_havoc -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, havoc_duration );
  }
};

struct immolate_t: public warlock_spell_t
{
  double roaring_blaze;

  immolate_t( warlock_t* p ):
    warlock_spell_t( "immolate", p, p -> find_spell( 348 ) )
  {
    const spell_data_t* dmg_spell = player -> find_spell( 157736 );

    can_havoc = true;

    base_tick_time = dmg_spell -> effectN( 1 ).period();
    dot_duration = dmg_spell -> duration();
    spell_power_mod.tick = dmg_spell -> effectN( 1 ).sp_coeff();
    spell_power_mod.direct = data().effectN( 1 ).sp_coeff();
    hasted_ticks = true;
    tick_may_crit = true;

    roaring_blaze = 1.0 + p -> find_spell( 205690 ) -> effectN( 1 ).percent();
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target ) -> debuffs_roaring_blaze -> expire();
    }
  }

  virtual double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m = warlock_spell_t::composite_ta_multiplier( state );

    if ( td( state -> target ) -> dots_immolate -> is_ticking() && p() -> talents.roaring_blaze -> ok() )
      m *= std::pow( roaring_blaze, td( state -> target ) -> debuffs_roaring_blaze -> stack() );

    return m;
  }

  void last_tick( dot_t* d ) override
  {
    warlock_spell_t::last_tick( d );

      td( d -> target ) -> debuffs_roaring_blaze -> expire();
  }

  virtual void tick( dot_t* d ) override
  {
    warlock_spell_t::tick( d );

    if ( d -> state -> result == RESULT_CRIT && rng().roll( 0.5 ) )
      p() -> resource_gain( RESOURCE_SOUL_SHARD, 0.1, p() -> gains.immolate_crits );

    p() -> resource_gain( RESOURCE_SOUL_SHARD, 0.1, p() -> gains.immolate );
  }
};

struct conflagrate_t: public warlock_spell_t
{
  timespan_t total_duration;
  timespan_t base_duration;
  conflagrate_t( warlock_t* p ):
    warlock_spell_t( "Conflagrate", p, p -> find_spell( 17962 ) )
  {
    energize_type = ENERGIZE_NONE;
    base_duration = p -> find_spell( 117828 ) -> duration();
    base_multiplier *= 1.0;

    can_havoc = true;

    cooldown -> charges += p -> spec.conflagrate_2 -> effectN( 1 ).base_value();

    cooldown -> charges += p -> sets->set( WARLOCK_DESTRUCTION, T19, B4 ) -> effectN( 1 ).base_value();
    cooldown -> duration += p -> sets->set( WARLOCK_DESTRUCTION, T19, B4 ) -> effectN( 2 ).time_value();
  }

  bool ready() override
  {
    if ( p() -> talents.shadowburn -> ok() )
      return false;

    return warlock_spell_t::ready();
  }

  void init() override
  {
    warlock_spell_t::init();

    cooldown -> hasted = true;
  }

  // Force spell to always crit
  double composite_crit_chance() const override
  {
    double cc = warlock_spell_t::composite_crit_chance();

    if ( p() -> buffs.conflagration_of_chaos -> check() )
      cc = 1.0;

    return cc;
  }

  double calculate_direct_amount( action_state_t* state ) const override
  {
    warlock_spell_t::calculate_direct_amount( state );

    // Can't use player-based crit chance from the state object as it's hardcoded to 1.0. Use cached
    // player spell crit instead. The state target crit chance of the state object is correct.
    // Targeted Crit debuffs function as a separate multiplier.
    if ( p() -> buffs.conflagration_of_chaos -> check() )
      state -> result_total *= 1.0 + player -> cache.spell_crit_chance() + state -> target_crit_chance;

    return state -> result_total;
  }

  void execute() override
  {
    warlock_spell_t::execute();

    if ( p() -> talents.backdraft -> ok() )
    {
      total_duration = base_duration * p() -> cache.spell_haste();
      p() -> buffs.backdraft -> trigger( 2, buff_t::DEFAULT_VALUE(), -1.0, total_duration );
    }

    if ( p() -> buffs.conflagration_of_chaos -> up() )
      p() -> buffs.conflagration_of_chaos -> expire();

    p() -> buffs.conflagration_of_chaos -> trigger();
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> talents.roaring_blaze -> ok() && td( s -> target ) -> dots_immolate -> is_ticking() )
      {
        td( s -> target ) -> debuffs_roaring_blaze -> trigger( 1 );
      }

      p() -> resource_gain( RESOURCE_SOUL_SHARD, 0.5, p() -> gains.conflagrate );
    }
  }
};

struct incinerate_t: public warlock_spell_t
{
  double backdraft_gcd;
  double backdraft_cast_time;
  double dimension_ripper;
  incinerate_t( warlock_t* p ):
    warlock_spell_t( p, "Incinerate" )
  {
    if ( p -> talents.fire_and_brimstone -> ok() )
      aoe = -1;

    can_havoc = true;

    dimension_ripper = p -> find_spell( 219415 ) -> proc_chance();

    backdraft_cast_time = 1.0 + p -> buffs.backdraft -> data().effectN( 1 ).percent();
    backdraft_gcd = 1.0 + p -> buffs.backdraft -> data().effectN( 2 ).percent();
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t h = spell_t::execute_time();

    if ( p() -> buffs.backdraft -> up() )
      h *= backdraft_cast_time;

    return h;
  }

  timespan_t gcd() const override
  {
    timespan_t t = action_t::gcd();

    if ( t == timespan_t::zero() )
      return t;

    if ( p() -> buffs.backdraft -> check() )
      t *= backdraft_gcd;
    if ( t < min_gcd )
      t = min_gcd;

    return t;
  }

  void execute() override
  {
    warlock_spell_t::execute();

    p() -> buffs.backdraft -> decrement();

    p() -> resource_gain( RESOURCE_SOUL_SHARD, 0.2 * ( p() -> talents.fire_and_brimstone -> ok() ? execute_state -> n_targets : 1 ), p() -> gains.incinerate );
    if ( execute_state -> result == RESULT_CRIT )
      p() -> resource_gain( RESOURCE_SOUL_SHARD, 0.1 * ( p() -> talents.fire_and_brimstone -> ok() ? execute_state -> n_targets : 1 ), p() -> gains.incinerate_crits );
    if ( p() -> sets -> has_set_bonus( WARLOCK_DESTRUCTION, T20, B2 ) )
      p() -> resource_gain( RESOURCE_SOUL_SHARD, 0.1 * ( p() -> talents.fire_and_brimstone -> ok() ? execute_state -> n_targets : 1 ), p() -> gains.destruction_t20_2pc );
  }

  virtual double composite_crit_chance() const override
  {
    double cc = warlock_spell_t::composite_crit_chance();

    return cc;
  }

  virtual double composite_target_crit_chance(player_t* t) const override
  {
	  double cc = warlock_spell_t::composite_target_crit_chance(t);
	  warlock_td_t* td = this->td(t);

	  if (td->debuffs_chaotic_flames->check())
		  cc += p()->find_spell(253092)->effectN(1).percent();

	  return cc;
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );
  }
};

struct duplicate_chaos_bolt_t : public warlock_spell_t
{
  player_t* original_target;
  flames_of_argus_t* flames_of_argus;

  duplicate_chaos_bolt_t( warlock_t* p ) :
    warlock_spell_t( "chaos_bolt_magistrike", p, p -> find_spell( 213229 ) ),
    original_target( nullptr ),
    flames_of_argus( nullptr )
  {
    background = dual = true;
    crit_bonus_multiplier *= 1.0;
    base_multiplier *= 1.0 + ( p -> sets->set( WARLOCK_DESTRUCTION, T18, B2 ) -> effectN( 2 ).percent() );
    base_multiplier *= 1.0 + ( p -> sets->set( WARLOCK_DESTRUCTION, T17, B4 ) -> effectN( 1 ).percent() );
    base_multiplier *= 1.0 + ( p -> talents.reverse_entropy -> effectN( 2 ).percent() );

    if ( p->sets->has_set_bonus( WARLOCK_DESTRUCTION, T21, B4 ) )
    {
      flames_of_argus = new flames_of_argus_t( p );
    }
  }

  timespan_t travel_time() const override
  {
    double distance;
    distance = original_target -> get_player_distance( *target );

    if ( execute_state && execute_state -> target )
      distance += execute_state -> target -> height;

    if ( distance == 0 ) return timespan_t::zero();

    double t = distance / travel_speed;

    double v = sim -> travel_variance;

    if ( v )
      t = rng().gauss( t, v );

    return timespan_t::from_seconds( t );
  }

  std::vector< player_t* >& target_list() const override
  {
    target_cache.list.clear();
    for ( size_t j = 0; j < sim -> target_non_sleeping_list.size(); ++j )
    {
      player_t* duplicate_target = sim -> target_non_sleeping_list[j];
      if ( target == duplicate_target )
        continue;
      if ( target -> get_player_distance( *duplicate_target ) <= 30 )
        target_cache.list.push_back( duplicate_target );
    }
    return target_cache.list;
  }

  // Force spell to always crit
  double composite_crit_chance() const override
  {
    return 1.0;
  }

  double calculate_direct_amount( action_state_t* state ) const override
  {
    warlock_spell_t::calculate_direct_amount( state );

    // Can't use player-based crit chance from the state object as it's hardcoded to 1.0. Use cached
    // player spell crit instead. The state target crit chance of the state object is correct.
    // Targeted Crit debuffs function as a separate multiplier.
    state -> result_total *= 1.0 + player -> cache.spell_crit_chance() + state -> target_crit_chance;

    return state -> result_total;
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T21, B2 ) )
      td( s->target )->debuffs_chaotic_flames->trigger();

    if ( p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T21, B4 ) )
    {
      // double amount = s->result_amount;
      // amount *= p()->find_spell( 251855 )->effectN( 1 ).percent();

      residual_action::trigger( flames_of_argus, s->target, s->result_amount * p()->sets->set( WARLOCK_DESTRUCTION, T21, B4 )->effectN( 1 ).percent() );
    }
  }
};

struct chaos_bolt_t: public warlock_spell_t
{
  double backdraft_gcd;
  double backdraft_cast_time;
  double refund;
  duplicate_chaos_bolt_t* duplicate;
  double duplicate_chance;
  flames_of_argus_t* flames_of_argus;

  chaos_bolt_t( warlock_t* p ) :
    warlock_spell_t( p, "Chaos Bolt" ),
    refund( 0 ), duplicate( nullptr ), duplicate_chance( 0 ), flames_of_argus( nullptr )
  {
    can_havoc = true;
    affected_by_destruction_t20_4pc = true;

    crit_bonus_multiplier *= 1.0;

    base_execute_time += p -> sets->set( WARLOCK_DESTRUCTION, T18, B2 ) -> effectN( 1 ).time_value();
    base_multiplier *= 1.0 + ( p -> sets->set( WARLOCK_DESTRUCTION, T18, B2 ) -> effectN( 2 ).percent() );
    base_multiplier *= 1.0 + ( p -> sets->set( WARLOCK_DESTRUCTION, T17, B4 ) -> effectN( 1 ).percent() );
    base_multiplier *= 1.0 + ( p -> talents.reverse_entropy -> effectN( 2 ).percent() );

    backdraft_cast_time = 1.0 + p -> buffs.backdraft -> data().effectN( 1 ).percent();
    backdraft_gcd = 1.0 + p -> buffs.backdraft -> data().effectN( 2 ).percent();

    duplicate = new duplicate_chaos_bolt_t( p );
    duplicate_chance = p -> find_spell( 213014 ) -> proc_chance();
    duplicate -> travel_speed = travel_speed;
    add_child( duplicate );

    if ( p->sets->has_set_bonus( WARLOCK_DESTRUCTION, T21, B4 ) )
    {
      flames_of_argus = new flames_of_argus_t( p );
      add_child( flames_of_argus );
    }
  }

  virtual void schedule_execute( action_state_t* state = nullptr ) override
  {
    warlock_spell_t::schedule_execute( state );

    if ( p() -> buffs.embrace_chaos -> check() )
    {
      p() -> procs.t19_2pc_chaos_bolts -> occur();
    }
  }

  virtual timespan_t execute_time() const override
  {
    timespan_t h = spell_t::execute_time();

    if ( p() -> buffs.backdraft -> up() )
      h *= backdraft_cast_time;

    if ( p() -> buffs.embrace_chaos -> up() )
      h *= 1.0 + p() -> buffs.embrace_chaos -> data().effectN( 1 ).percent();

    return h;
  }

  timespan_t gcd() const override
  {
    timespan_t t = action_t::gcd();

    if ( t == timespan_t::zero() )
      return t;

    if ( p() -> buffs.backdraft -> check() )
      t *= backdraft_gcd;
    if ( t < min_gcd )
      t = min_gcd;

    return t;
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );
    if ( p() -> talents.eradication -> ok() && result_is_hit( s -> result ) )
      td( s -> target ) -> debuffs_eradication -> trigger();
	if (p()->sets->has_set_bonus(WARLOCK_DESTRUCTION, T21, B2))
		td(s->target)->debuffs_chaotic_flames->trigger();
    if ( p() -> legendary.magistrike && rng().roll( duplicate_chance ) )
    {
      duplicate -> original_target = s -> target;
      duplicate -> target = s -> target;
      duplicate -> target_cache.is_valid = false;
      duplicate -> target_list();
      duplicate ->target_cache.is_valid = true;
      if ( duplicate -> target_cache.list.size() > 0 )
      {
        size_t target_to_strike = static_cast<size_t>( rng().range( 0.0, duplicate -> target_cache.list.size() ) );
        duplicate -> target = duplicate -> target_cache.list[target_to_strike];
        duplicate -> execute();
      }
    }
    if ( p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T21, B4 ) )
    {
      // double amount = s->result_amount;
      // amount *= p()->find_spell( 251855 )->effectN( 1 ).percent();

      residual_action::trigger( flames_of_argus, s->target, s->result_amount * p()->sets->set( WARLOCK_DESTRUCTION, T21, B4 )->effectN( 1 ).percent() );
    }
  }

  void execute() override
  {
    warlock_spell_t::execute();

    if ( p() -> talents.reverse_entropy -> ok() )
    {
      refund = p() -> resources.max[RESOURCE_MANA] * p() -> talents.reverse_entropy -> effectN( 1 ).percent();
      p() -> resource_gain( RESOURCE_MANA, refund, p() -> gains.reverse_entropy );
    }

    p() -> buffs.embrace_chaos -> trigger();
    p() -> buffs.backdraft -> decrement();
  }

  // Force spell to always crit
  double composite_crit_chance() const override
  {
    return 1.0;
  }

  double calculate_direct_amount( action_state_t* state ) const override
  {
    warlock_spell_t::calculate_direct_amount( state );

    // Can't use player-based crit chance from the state object as it's hardcoded to 1.0. Use cached
    // player spell crit instead. The state target crit chance of the state object is correct.
    // Targeted Crit debuffs function as a separate multiplier.
    state -> result_total *= 1.0 + player -> cache.spell_crit_chance() + state -> target_crit_chance;

    return state -> result_total;
  }

  double cost() const override
  {
    double c = warlock_spell_t::cost();

    double t18_4pc_rng = p() -> sets->set( WARLOCK_DESTRUCTION, T18, B4 ) -> effectN( 1 ).percent();

    if ( rng().roll( t18_4pc_rng ) )
    {
      p() -> procs.t18_4pc_destruction -> occur();
      return 1;
    }

    return c;
  }
};

// ARTIFACT SPELLS

struct thalkiels_discord_t : public warlock_spell_t
{
  thalkiels_discord_t( warlock_t* p ) :
    warlock_spell_t( "thalkiels_discord", p, p -> find_spell( 211727 ) )
  {
    aoe = -1;
    background = dual = true;
    callbacks = false;
  }

  void init() override
  {
    warlock_spell_t::init();

    // Explicitly snapshot haste, as the spell actually has no duration in spell data
    snapshot_flags |= STATE_HASTE;
  }
};

// AOE SPELLS
struct rain_of_fire_t : public warlock_spell_t
{
  struct rain_of_fire_tick_t : public warlock_spell_t
  {
    rain_of_fire_tick_t( warlock_t* p ) :
      warlock_spell_t( "rain_of_fire_tick", p, p -> find_spell( 42223 ) )
    {
      aoe = -1;
      background = dual = direct_tick = true; // Legion TOCHECK
      callbacks = false;
      radius = p -> find_spell( 5740 ) -> effectN( 1 ).radius();

      base_multiplier *= 1.0 + p -> talents.reverse_entropy -> effectN( 2 ).percent();
    }
  };

  rain_of_fire_t( warlock_t* p, const std::string& options_str ) :
    warlock_spell_t( "rain_of_fire", p, p -> find_spell( 5740 ) )
  {
    parse_options( options_str );
    dot_duration = timespan_t::zero();
    may_miss = may_crit = false;
    base_tick_time = data().duration() / 8.0; // ticks 8 times (missing from spell data)
    base_execute_time = timespan_t::zero(); // HOTFIX

    if ( !p -> active.rain_of_fire )
    {
      p -> active.rain_of_fire = new rain_of_fire_tick_t( p );
      p -> active.rain_of_fire -> stats = stats;
    }
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .target( execute_state -> target )
      .x( execute_state -> target -> x_position )
      .y( execute_state -> target -> y_position )
      .pulse_time( base_tick_time * player -> cache.spell_haste() )
      .duration( data().duration() * player -> cache.spell_haste() )
      .start_time( sim -> current_time() )
      .action( p() -> active.rain_of_fire ) );

    if ( p() -> legendary.alythesss_pyrogenics )
      p() -> buffs.alythesss_pyrogenics -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, data().duration() * player->cache.spell_haste() );
  }
};

struct demonwrath_tick_t: public warlock_spell_t
{
  cooldown_t* icd;

  demonwrath_tick_t( warlock_t* p, const spell_data_t& ):
    warlock_spell_t( "demonwrath_tick", p, p -> find_spell( 193439 ) )
  {
    aoe = -1;
    background = true;

    icd = p -> get_cooldown( "discord_icd" );
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( p() -> talents.demonic_calling -> ok() && rng().roll( p() -> talents.demonic_calling -> effectN( 2 ).percent() ) )
      p() -> buffs.demonic_calling -> trigger();

    double accumulator_increment = rng().range( 0.0, 0.3 );

    p() -> demonwrath_accumulator += accumulator_increment;

    if ( p() -> demonwrath_accumulator >= 1 )
    {
      p() -> resource_gain( RESOURCE_SOUL_SHARD, 1.0, p() -> gains.demonwrath );
      p() -> demonwrath_accumulator -= 1.0;

      // If going from 0 to 1 shard was a surprise, the player would have to react to it
      if ( p() -> resources.current[RESOURCE_SOUL_SHARD] == 1 )
        p() -> shard_react = p() -> sim -> current_time() + p() -> total_reaction_time();
      else if ( p() -> resources.current[RESOURCE_SOUL_SHARD] >= 1 )
        p() -> shard_react = p() -> sim -> current_time();
      else
        p() -> shard_react = timespan_t::max();
    }

    if ( p() -> sets -> has_set_bonus( WARLOCK_DEMONOLOGY, T20, B2 ) && p() -> rng().roll( p() -> sets -> set( WARLOCK_DEMONOLOGY, T20, B2 ) -> proc_chance() / 3.0 ) )
    {
      p() -> cooldowns.call_dreadstalkers -> reset( true );
      p() -> procs.demonology_t20_2pc -> occur();
    }
  }
};

struct demonwrath_t: public warlock_spell_t
{
  demonwrath_t( warlock_t* p ):
    warlock_spell_t( "Demonwrath", p, p -> find_spell( 193440 ) )
  {
    tick_zero = false;
    may_miss = false;
    channeled = true;
    may_crit = false;

    spell_power_mod.tick = base_td = 0;

    base_multiplier *= 1.0;

    dynamic_tick_action = true;
    tick_action = new demonwrath_tick_t( p, data() );
  }

  virtual bool usable_moving() const override
  {
    return true;
  }
};

// SUMMONING SPELLS

struct summon_pet_t: public warlock_spell_t
{
  timespan_t summoning_duration;
  std::string pet_name;
  pets::warlock_pet_t* pet;

private:
  void _init_summon_pet_t()
  {
    util::tokenize( pet_name );
    harmful = false;

    if ( data().ok() &&
         std::find( p() -> pet_name_list.begin(), p() -> pet_name_list.end(), pet_name ) ==
         p() -> pet_name_list.end() )
    {
      p() -> pet_name_list.push_back( pet_name );
    }
  }

public:
  summon_pet_t( const std::string& n, warlock_t* p, const std::string& sname = "" ):
    warlock_spell_t( p, sname.empty() ? "Summon " + n : sname ),
    summoning_duration( timespan_t::zero() ),
    pet_name( sname.empty() ? n : sname ), pet( nullptr )
  {
    _init_summon_pet_t();
  }

  summon_pet_t( const std::string& n, warlock_t* p, int id ):
    warlock_spell_t( n, p, p -> find_spell( id ) ),
    summoning_duration( timespan_t::zero() ),
    pet_name( n ), pet( nullptr )
  {
    _init_summon_pet_t();
  }

  summon_pet_t( const std::string& n, warlock_t* p, const spell_data_t* sd ):
    warlock_spell_t( n, p, sd ),
    summoning_duration( timespan_t::zero() ),
    pet_name( n ), pet( nullptr )
  {
    _init_summon_pet_t();
  }

  bool init_finished() override
  {
    pet = debug_cast<pets::warlock_pet_t*>( player -> find_pet( pet_name ) );
    return warlock_spell_t::init_finished();
  }

  virtual void execute() override
  {
    pet -> summon( summoning_duration );

    warlock_spell_t::execute();
  }

  bool ready() override
  {
    if ( ! pet )
    {
      return false;
    }

    return warlock_spell_t::ready();
  }
};

struct summon_main_pet_t: public summon_pet_t
{
  cooldown_t* instant_cooldown;

  summon_main_pet_t( const std::string& n, warlock_t* p ):
    summon_pet_t( n, p ), instant_cooldown( p -> get_cooldown( "instant_summon_pet" ) )
  {
    instant_cooldown -> duration = timespan_t::from_seconds( 60 );
    ignore_false_positive = true;
  }

  virtual void schedule_execute( action_state_t* state = nullptr ) override
  {
    warlock_spell_t::schedule_execute( state );

    if ( p() -> warlock_pet_list.active )
    {
      p() -> warlock_pet_list.active -> dismiss();
      p() -> warlock_pet_list.active = nullptr;
    }
  }

  virtual bool ready() override
  {
    if ( p() -> warlock_pet_list.active == pet )
      return false;

    if ( p() -> talents.grimoire_of_supremacy -> ok() ) //if we have the uberpets, we can't summon our standard pets
      return false;
    return summon_pet_t::ready();
  }

  virtual void execute() override
  {
    summon_pet_t::execute();

    p() -> warlock_pet_list.active = p() -> warlock_pet_list.last = pet;

    if ( p() -> buffs.demonic_power -> check() )
      p() -> buffs.demonic_power -> expire();
  }
};

struct summon_doomguard_t: public warlock_spell_t
{
  timespan_t doomguard_duration;

  summon_doomguard_t( warlock_t* p ):
    warlock_spell_t( "summon_doomguard", p, p -> find_spell( 18540 ) )
  {
    harmful = may_crit = false;

    cooldown = p -> cooldowns.doomguard;
    if ( !p -> talents.grimoire_of_supremacy -> ok() )
      cooldown -> duration = data().cooldown();
    else
      cooldown -> duration = timespan_t::zero();

    if ( p -> talents.grimoire_of_supremacy -> ok() )
      doomguard_duration = timespan_t::from_seconds( -1 );
    else
      doomguard_duration = p -> find_spell( 111685 ) -> duration() + timespan_t::from_millis( 1 );
  }

  virtual void schedule_execute( action_state_t* state = nullptr ) override
  {
    warlock_spell_t::schedule_execute( state );

    if ( p() -> talents.grimoire_of_supremacy -> ok() )
    {
      for ( auto infernal : p() -> warlock_pet_list.infernal )
      {
        if ( !infernal -> is_sleeping() )
        {
          infernal -> dismiss();
        }
      }
    }
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    if ( !p() -> talents.grimoire_of_supremacy -> ok() )
      p() -> cooldowns.infernal -> start();

    for ( size_t i = 0; i < p() -> warlock_pet_list.doomguard.size(); i++ )
    {
      if ( p() -> warlock_pet_list.doomguard[i] -> is_sleeping() )
      {
        p() -> warlock_pet_list.doomguard[i] -> summon( doomguard_duration );
      }
    }
    if ( p() -> cooldowns.sindorei_spite_icd -> up() )
    {
      p() -> buffs.sindorei_spite -> up();
      p() -> buffs.sindorei_spite -> trigger();
      p() -> cooldowns.sindorei_spite_icd -> start( timespan_t::from_seconds( 180.0 ) );
    }
  }
};

struct infernal_awakening_t : public warlock_spell_t
{
  infernal_awakening_t( warlock_t* p, spell_data_t* spell ) :
    warlock_spell_t( "infernal_awakening", p, spell )
  {
    aoe = -1;
    background = true;
    dual = true;
    trigger_gcd = timespan_t::zero();
  }
};

struct summon_infernal_t : public warlock_spell_t
{
  infernal_awakening_t* infernal_awakening;
  timespan_t infernal_duration;

  summon_infernal_t( warlock_t* p ) :
    warlock_spell_t( "Summon_Infernal", p, p -> find_spell( 1122 ) ),
    infernal_awakening( nullptr )
  {
    harmful = may_crit = false;

    cooldown = p -> cooldowns.infernal;
    if ( !p -> talents.grimoire_of_supremacy -> ok() )
      cooldown -> duration = data().cooldown();
    else
      cooldown -> duration = timespan_t::zero();

    if ( p -> talents.grimoire_of_supremacy -> ok() )
      infernal_duration = timespan_t::from_seconds( -1 );
    else
    {
      infernal_duration = p -> find_spell( 111685 ) -> duration() + timespan_t::from_millis( 1 );
      infernal_awakening = new infernal_awakening_t( p, data().effectN( 1 ).trigger() );
      infernal_awakening -> stats = stats;
      radius = infernal_awakening -> radius;
    }
  }

  virtual void schedule_execute( action_state_t* state = nullptr ) override
  {
    warlock_spell_t::schedule_execute( state );

    if ( p() -> talents.grimoire_of_supremacy -> ok() )
    {
      for ( auto doomguard : p() -> warlock_pet_list.doomguard )
      {
        if ( !doomguard -> is_sleeping() )
        {
          doomguard -> dismiss();
        }
      }
    }
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    if ( !p() -> talents.grimoire_of_supremacy -> ok() )
      p() -> cooldowns.doomguard -> start();

    if ( infernal_awakening )
      infernal_awakening -> execute();

    for ( size_t i = 0; i < p() -> warlock_pet_list.infernal.size(); i++ )
    {
      if ( p() -> warlock_pet_list.infernal[i] -> is_sleeping() )
      {
        p() -> warlock_pet_list.infernal[i] -> summon( infernal_duration );
      }
    }

    if ( p() -> cooldowns.sindorei_spite_icd -> up() )
    {
      p() -> buffs.sindorei_spite -> up();
      p() -> buffs.sindorei_spite -> trigger();
      p() -> cooldowns.sindorei_spite_icd -> start( timespan_t::from_seconds( 180.0 ) );
    }
  }
};

struct summon_darkglare_t : public warlock_spell_t
{
  timespan_t darkglare_duration;

  summon_darkglare_t( warlock_t* p ) :
    warlock_spell_t( "summon_darkglare", p, p -> talents.summon_darkglare )
  {
    harmful = may_crit = may_miss = false;

    darkglare_duration = data().duration() + timespan_t::from_millis( 1 );
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    for ( size_t i = 0; i < p() -> warlock_pet_list.darkglare.size(); i++ )
    {
      if ( p() -> warlock_pet_list.darkglare[i] -> is_sleeping() )
      {
        p() -> warlock_pet_list.darkglare[i] -> summon( darkglare_duration );
        if(p()->legendary.wilfreds_sigil_of_superior_summoning_flag && !p()->talents.grimoire_of_supremacy->ok())
        {
            p()->cooldowns.doomguard->adjust(p()->legendary.wilfreds_sigil_of_superior_summoning);
            p()->cooldowns.infernal->adjust(p()->legendary.wilfreds_sigil_of_superior_summoning);
            p()->procs.wilfreds_darkglare->occur();
        }
      }
    }
  }
};

struct call_dreadstalkers_t : public warlock_spell_t
{
  timespan_t dreadstalker_duration;
  int dreadstalker_count;
  size_t improved_dreadstalkers;
  double recurrent_ritual;

  call_dreadstalkers_t( warlock_t* p ) :
    warlock_spell_t( "Call_Dreadstalkers", p, p -> find_spell( 104316 ) ),
    recurrent_ritual( 0.0 )
  {
    may_crit = false;
    dreadstalker_duration = p -> find_spell( 193332 ) -> duration() + ( p -> sets->has_set_bonus( WARLOCK_DEMONOLOGY, T19, B4 ) ? p -> sets->set( WARLOCK_DEMONOLOGY, T19, B4 ) -> effectN( 1 ).time_value() : timespan_t::zero() );
    dreadstalker_count = data().effectN( 1 ).base_value();
    improved_dreadstalkers = p -> talents.improved_dreadstalkers -> effectN( 1 ).base_value();
  }

  double cost() const override
  {
    double c = warlock_spell_t::cost();

    if ( p() -> buffs.demonic_calling -> check() )
    {
      return 0;
    }

    return c;
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    int j = 0;

    for ( size_t i = 0; i < p() -> warlock_pet_list.dreadstalkers.size(); i++ )
    {
      if ( p() -> warlock_pet_list.dreadstalkers[i] -> is_sleeping() )
      {
        p() -> warlock_pet_list.dreadstalkers[i] -> summon( dreadstalker_duration );
        p()->procs.dreadstalker_debug->occur();

        if ( p()->sets->has_set_bonus( WARLOCK_DEMONOLOGY, T21, B2 ))
        { 
		      p() -> warlock_pet_list.dreadstalkers[i] -> buffs.rage_of_guldan -> set_duration( dreadstalker_duration );
		      p() -> warlock_pet_list.dreadstalkers[i] -> buffs.rage_of_guldan -> set_default_value( p() -> buffs.rage_of_guldan -> stack_value());
		      p() -> warlock_pet_list.dreadstalkers[i] -> buffs.rage_of_guldan -> trigger();
        }
        if(p()->legendary.wilfreds_sigil_of_superior_summoning_flag && !p()->talents.grimoire_of_supremacy->ok())
        {
            p()->cooldowns.doomguard->adjust(p()->legendary.wilfreds_sigil_of_superior_summoning);
            p()->cooldowns.infernal->adjust(p()->legendary.wilfreds_sigil_of_superior_summoning);
            p()->procs.wilfreds_dog->occur();
        }
        if ( ++j == dreadstalker_count ) break;
      }
    }

    if ( p() -> talents.improved_dreadstalkers -> ok() )
    {
      for ( size_t i = 0; i < improved_dreadstalkers; i++ )
      {
        //trigger_wild_imp( p(), true, dreadstalker_duration.total_millis() );
        p() -> procs.improved_dreadstalkers -> occur();
      }
    }

    p() -> buffs.demonic_calling -> expire();
	  p()->buffs.rage_of_guldan->expire();

    if ( recurrent_ritual > 0 )
    {
      p() -> resource_gain( RESOURCE_SOUL_SHARD, recurrent_ritual, p() -> gains.recurrent_ritual );
    }

    if ( p() -> sets -> has_set_bonus( WARLOCK_DEMONOLOGY, T20, B4 ) )
    {
      p() -> buffs.dreaded_haste -> trigger();
    }
  }
};

// TALENT SPELLS

// DEMONOLOGY



struct demonbolt_t : public warlock_spell_t
{
  cooldown_t* icd;

  demonbolt_t( warlock_t* p ) :
    warlock_spell_t( "demonbolt", p, p -> talents.demonbolt )
  {
    energize_type = ENERGIZE_ON_CAST;
    energize_resource = RESOURCE_SOUL_SHARD;
    energize_amount = 1;

    icd = p -> get_cooldown( "discord_icd" );

    if ( p -> sets->set( WARLOCK_DEMONOLOGY, T17, B4 ) )
    {
      if ( rng().roll( p -> sets->set( WARLOCK_DEMONOLOGY, T17, B4 ) -> effectN( 1 ).percent() ) )
      {
        energize_amount++;
      }
    }
  }

  //fix this
  virtual double action_multiplier() const override
  {
    double pm = spell_t::action_multiplier();
    double pet_counter = 0.0;

    for ( auto& pet : p()->pet_list )
    {
      pets::warlock_pet_t *lock_pet = static_cast< pets::warlock_pet_t* > ( pet );

      if ( lock_pet != NULL )
      {
        if ( !lock_pet->is_sleeping() )
        {
          pet_counter += data().effectN( 3 ).percent();
        }
      }
    }
    if ( p()->buffs.stolen_power->up() )
    {
      p()->procs.stolen_power_used->occur();
      pm *= 1.0 + p()->buffs.stolen_power->data().effectN( 1 ).percent();
      p()->buffs.stolen_power->reset();
    }

    pm *= 1 + pet_counter;
    return pm;
  }

  virtual timespan_t execute_time() const override
  {
    if ( p()->buffs.shadowy_inspiration->check() )
    {
      return timespan_t::zero();
    }
    return warlock_spell_t::execute_time();
  }

  void execute() override
  {
    warlock_spell_t::execute();

    if ( p() -> talents.demonic_calling -> ok() && rng().roll( p() -> talents.demonic_calling -> proc_chance() ) )
      p() -> buffs.demonic_calling -> trigger();

    if ( p() -> buffs.shadowy_inspiration -> check() )
      p() -> buffs.shadowy_inspiration -> expire();

    if ( p() -> sets->set( WARLOCK_DEMONOLOGY, T18, B2 ) )
    {
      p() -> buffs.tier18_2pc_demonology -> trigger( 1 );
    }

    if ( p() -> sets -> has_set_bonus( WARLOCK_DEMONOLOGY, T20, B2 ) && p() -> rng().roll( p() -> sets -> set( WARLOCK_DEMONOLOGY, T20, B2 ) -> proc_chance() ) )
    {
      p() -> cooldowns.call_dreadstalkers -> reset( true );
      p() -> procs.demonology_t20_2pc -> occur();
    }
  }
};

struct implosion_t : public warlock_spell_t
{
    struct implosion_aoe_t: public warlock_spell_t
    {

      implosion_aoe_t( warlock_t* p ):
        warlock_spell_t( "implosion_aoe", p, p -> find_spell( 196278 ) )
      {
        aoe = -1;
        dual = true;
        background = true;
        callbacks = false;

        p -> spells.implosion_aoe = this;
      }
    };

    implosion_aoe_t* explosion;

    implosion_t(warlock_t* p) :
      warlock_spell_t( "implosion", p, p -> talents.implosion ),
      explosion( new implosion_aoe_t( p ) )
    {
      aoe = -1;
      add_child( explosion );
    }

    virtual bool ready() override
    {
      bool r = warlock_spell_t::ready();

      if(r)
      {
          for ( auto imp : p() -> warlock_pet_list.wild_imps )
          {
            if ( !imp -> is_sleeping() )
              return true;
          }
      }
      return false;
    }

    virtual void execute() override
    {
      warlock_spell_t::execute();
      for( auto imp : p() -> warlock_pet_list.wild_imps )
      {
        if( !imp -> is_sleeping() )
        {
          explosion -> execute();
          imp -> dismiss(true);
        }
      }
    }
};

struct shadowflame_t : public warlock_spell_t
{
  shadowflame_t( warlock_t* p ) :
    warlock_spell_t( "shadowflame", p, p -> talents.shadowflame )
  {
    hasted_ticks = tick_may_crit = true;

    dot_duration = timespan_t::from_seconds( 8.0 );
    spell_power_mod.tick = data().effectN( 2 ).sp_coeff();
    base_tick_time = data().effectN( 2 ).period();
    energize_amount = 1;
  }

  timespan_t calculate_dot_refresh_duration( const dot_t* dot,
                                             timespan_t triggered_duration ) const override
  { return dot -> time_to_next_tick() + triggered_duration; }

  double composite_ta_multiplier( const action_state_t* state ) const override
  {
    double m = warlock_spell_t::composite_ta_multiplier( state );

    if ( td( state -> target ) -> dots_shadowflame -> is_ticking() )
      m *= td( target ) -> debuffs_shadowflame -> stack();

    return m;
  }

  void last_tick( dot_t* d ) override
  {
    warlock_spell_t::last_tick( d );

    td( d -> state -> target ) -> debuffs_shadowflame -> expire();
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      td( s -> target  ) -> debuffs_shadowflame -> trigger();
    }
  }
};

struct drain_soul_t: public warlock_spell_t
{
  double rend_soul_proc_chance;
  drain_soul_t( warlock_t* p ):
    warlock_spell_t( "drain_soul", p, p -> find_specialization_spell( "Drain Soul" ) )
  {
    channeled = true;
    hasted_ticks = false;
    may_crit = false;
    affected_by_deaths_embrace = true;
  }

  virtual double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    m *= 1.0;

    return m;
  }

  double composite_crit_chance() const override
  {
    double cc = warlock_spell_t::composite_crit_chance();
    return cc;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double cd = warlock_spell_t::composite_crit_damage_bonus_multiplier();
    return cd;
  }

  virtual void tick( dot_t* d ) override
  {
    if ( p() -> sets->has_set_bonus( WARLOCK_AFFLICTION, T18, B2 ) )
    {
      p() -> buffs.shard_instability -> trigger();
    }
    warlock_spell_t::tick( d );
  }
};

struct cataclysm_t : public warlock_spell_t
{
  immolate_t* immolate;

  cataclysm_t( warlock_t* p ) :
    warlock_spell_t( "cataclysm", p, p -> talents.cataclysm ),
    immolate( new immolate_t( p ) )
  {
    aoe = -1;

    immolate -> background = true;
    immolate -> dual = true;
    immolate -> base_costs[RESOURCE_MANA] = 0;
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      immolate -> target = s -> target;
      immolate -> execute();
    }
  }
};

struct shadowburn_t: public warlock_spell_t
{
  struct resource_event_t: public player_event_t
  {
    shadowburn_t* spell;
    gain_t* shard_gain;
    player_t* target;

    resource_event_t( warlock_t* p, shadowburn_t* s, player_t* t ):
      player_event_t( *p, s -> delay ), spell( s ), shard_gain( p -> gains.shadowburn_shard ), target(t)
    {
    }
    virtual const char* name() const override
    { return "shadowburn_execute_gain"; }
    virtual void execute() override
    {
      if ( target -> is_sleeping() )
      {
        p() -> resource_gain( RESOURCE_SOUL_SHARD, 0.6, shard_gain );
      }
    }
  };
  resource_event_t* resource_event;
  timespan_t delay;
  timespan_t total_duration;
  timespan_t base_duration;
  shadowburn_t( warlock_t* p ):
    warlock_spell_t( "shadowburn", p, p -> talents.shadowburn ), resource_event( nullptr )
  {
    delay = data().effectN( 1 ).trigger() -> duration();

    energize_type = ENERGIZE_ON_CAST;
    base_duration = p -> find_spell( 117828 ) -> duration();
    base_multiplier *= 1.0;

    can_havoc = true;

    cooldown -> charges += p -> spec.conflagrate_2 -> effectN( 1 ).base_value();

    cooldown -> charges += p -> sets->set( WARLOCK_DESTRUCTION, T19, B4 ) -> effectN( 1 ).base_value();
    cooldown -> duration += p -> sets->set( WARLOCK_DESTRUCTION, T19, B4 ) -> effectN( 2 ).time_value();
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    resource_event = make_event<resource_event_t>( *sim, p(), this, s -> target );

    p() -> resource_gain( RESOURCE_SOUL_SHARD, 0.5, p() -> gains.shadowburn );
  }

  void init() override
  {
    warlock_spell_t::init();

    cooldown -> hasted = true;
  }

// Force spell to always crit
  double composite_crit_chance() const override
  {
    double cc = warlock_spell_t::composite_crit_chance();

    if ( p() -> buffs.conflagration_of_chaos -> check() )
      cc = 1.0;

    return cc;
  }

  double calculate_direct_amount( action_state_t* state ) const override
  {
    warlock_spell_t::calculate_direct_amount( state );

    // Can't use player-based crit chance from the state object as it's hardcoded to 1.0. Use cached
    // player spell crit instead. The state target crit chance of the state object is correct.
    // Targeted Crit debuffs function as a separate multiplier.
    if ( p() -> buffs.conflagration_of_chaos -> check() )
      state -> result_total *= 1.0 + player -> cache.spell_crit_chance() + state -> target_crit_chance;

    return state -> result_total;
  }

  void execute() override
  {
    warlock_spell_t::execute();

    if ( p() -> buffs.conflagration_of_chaos -> check() )
      p()->buffs.conflagration_of_chaos -> expire();

    p() -> buffs.conflagration_of_chaos -> trigger();
  }
};

struct soul_harvest_t : public warlock_spell_t
{
  int agony_action_id;
  int doom_action_id;
  int immolate_action_id;
  timespan_t base_duration;
  timespan_t total_duration;
  timespan_t time_per_agony;
  timespan_t max_duration;

  soul_harvest_t( warlock_t* p ) :
    warlock_spell_t( "soul_harvest", p, p -> talents.soul_harvest )
  {
    harmful = may_crit = may_miss = false;
    base_duration = data().duration();
    time_per_agony = timespan_t::from_seconds( data().effectN( 2 ).base_value() );
    max_duration = timespan_t::from_seconds( data().effectN( 3 ).base_value() );
  }

  virtual void execute() override
  {
    warlock_spell_t::execute();

    p() -> buffs.soul_harvest -> expire(); //Potentially bugged check when live

    if ( p() -> specialization() == WARLOCK_AFFLICTION )
    {
      total_duration = base_duration + time_per_agony * p() -> get_active_dots( agony_action_id );
      p() -> buffs.soul_harvest -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, std::min( total_duration, max_duration ) );
    }

    if ( p() -> specialization() == WARLOCK_DEMONOLOGY )
    {
      total_duration = base_duration + time_per_agony * p() -> get_active_dots( doom_action_id );
      p() -> buffs.soul_harvest -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, std::min( total_duration, max_duration ) );
    }

    if ( p() -> specialization() == WARLOCK_DESTRUCTION )
    {
      total_duration = base_duration + time_per_agony * p() -> get_active_dots( immolate_action_id );
      p() -> buffs.soul_harvest -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, std::min( total_duration, max_duration ) );
    }
  }

  virtual void init() override
  {
    warlock_spell_t::init();

    agony_action_id = p() -> find_action_id( "agony" );
    doom_action_id = p() -> find_action_id( "doom" );
    immolate_action_id = p() -> find_action_id( "immolate" );
  }
};

struct grimoire_of_sacrifice_t: public warlock_spell_t
{
  grimoire_of_sacrifice_t( warlock_t* p ):
    warlock_spell_t( "grimoire_of_sacrifice", p, p -> talents.grimoire_of_sacrifice )
  {
    harmful = false;
    ignore_false_positive = true;
  }

  virtual bool ready() override
  {
    if ( ! p() -> warlock_pet_list.active ) return false;

    return warlock_spell_t::ready();
  }

  virtual void execute() override
  {
    if ( p() -> warlock_pet_list.active )
    {
      warlock_spell_t::execute();

      p() -> warlock_pet_list.active -> dismiss();
      p() -> warlock_pet_list.active = nullptr;
      p() -> buffs.demonic_power -> trigger();

    }
  }
};

struct demonic_power_damage_t : public warlock_spell_t
{
  demonic_power_damage_t( warlock_t* p ) :
    warlock_spell_t( "demonic_power", p, p -> find_spell( 196100 ) )
  {
    aoe = -1;
    background = true;
    proc = true;
    base_multiplier *= 1.0;
    destro_mastery = false;

    // Hotfix on the aff hotfix spell, check regularly.
    if ( p -> specialization() == WARLOCK_AFFLICTION )
    {
      base_multiplier *= 1.0 + p -> spec.affliction -> effectN( 1 ).percent();
    }
  }
};

struct harvester_of_souls_t : public warlock_spell_t
{
  harvester_of_souls_t( warlock_t* p ) :
    warlock_spell_t( "harvester_of_souls", p, p -> find_spell( 218615 ) )
  {
    background = true;
    //proc = true; Harvester of Souls can proc trinkets and has no resource cost so no need.
    callbacks = true;
  }
};

struct rend_soul_t : public warlock_spell_t
{
  rend_soul_t( warlock_t* p ) :
    warlock_spell_t( "rend_soul", p, p -> find_spell( 242834 ) )
  {
    background = true;
    //proc = true;
    callbacks = true;
  }
};

struct cry_havoc_t : public warlock_spell_t
{
  cry_havoc_t( warlock_t* p ) :
    warlock_spell_t( "cry_havoc", p, p -> find_spell( 243011 ) )
  {
    background = true;
    //proc = true;
    callbacks = true;
    aoe = -1;
    destro_mastery = false;
  }
};

struct grimoire_of_service_t: public summon_pet_t
{
  grimoire_of_service_t( warlock_t* p, const std::string& pet_name ):
    summon_pet_t( "service_" + pet_name, p, p -> talents.grimoire_of_service -> ok() ? p -> find_class_spell( "Grimoire: " + pet_name ) : spell_data_t::not_found() )
  {
    cooldown = p -> get_cooldown( "grimoire_of_service" );
    cooldown -> duration = data().cooldown();
    summoning_duration = data().duration() + timespan_t::from_millis( 1 );

  }

  virtual void execute() override
  {
    pet -> is_grimoire_of_service = true;

    summon_pet_t::execute();
  }

  bool init_finished() override
  {
    if ( pet )
    {
      pet -> summon_stats = stats;
    }
    return summon_pet_t::init_finished();
  }
};

struct mortal_coil_heal_t: public warlock_heal_t
{
  mortal_coil_heal_t( warlock_t* p, const spell_data_t& s ):
    warlock_heal_t( "mortal_coil_heal", p, s.effectN( 3 ).trigger_spell_id() )
  {
    background = true;
    may_miss = false;
  }

  virtual void execute() override
  {
    double heal_pct = data().effectN( 1 ).percent();
    base_dd_min = base_dd_max = player -> resources.max[RESOURCE_HEALTH] * heal_pct;

    warlock_heal_t::execute();
  }
};

struct mortal_coil_t: public warlock_spell_t
{
  mortal_coil_heal_t* heal;

  mortal_coil_t( warlock_t* p ):
    warlock_spell_t( "mortal_coil", p, p -> talents.mortal_coil ), heal( nullptr )
  {
    base_dd_min = base_dd_max = 0;
    heal = new mortal_coil_heal_t( p, data() );
  }

  virtual void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( result_is_hit( s -> result ) )
      heal -> execute();
  }
};

struct channel_demonfire_tick_t : public warlock_spell_t
{
  channel_demonfire_tick_t( warlock_t* p ):
    warlock_spell_t( "channel_demonfire_tick", p, p -> find_spell( 196448 ) )
  {
    background = true;
    may_miss = false;
    dual = true;

    can_feretory = false;

    spell_power_mod.direct = data().effectN( 1 ).sp_coeff();

    aoe = -1;
    base_aoe_multiplier = data().effectN( 2 ).sp_coeff() / data().effectN( 1 ).sp_coeff();
  }
};

struct channel_demonfire_t: public warlock_spell_t
{
  channel_demonfire_tick_t* channel_demonfire;
  int immolate_action_id;

  channel_demonfire_t( warlock_t* p ):
    warlock_spell_t( "channel_demonfire", p, p -> find_spell( 196447 ) ),
    immolate_action_id( 0 )
  {
    channeled = true;
    hasted_ticks = true;
    may_crit = false;
    //can_havoc = true;

    channel_demonfire = new channel_demonfire_tick_t( p );
    add_child( channel_demonfire );

  }

  void init() override
  {
    warlock_spell_t::init();

    cooldown -> hasted = true;
    immolate_action_id = p() -> find_action_id( "immolate" );
  }

  std::vector< player_t* >& target_list() const override
  {
    target_cache.list = warlock_spell_t::target_list();

    size_t i = target_cache.list.size();
    while ( i > 0 )
    {
      i--;
      player_t* target_ = target_cache.list[i];
      if ( !td( target_ ) -> dots_immolate -> is_ticking() )
        target_cache.list.erase( target_cache.list.begin() + i );
    }
    return target_cache.list;
  }

  void tick( dot_t* d ) override
  {
    // Need to invalidate the target cache to figure out immolated targets.
    target_cache.is_valid = false;

    const auto& targets = target_list();

    if ( targets.size() > 0 )
    {
      channel_demonfire -> set_target( targets[ rng().range( 0, targets.size() ) ] );
      channel_demonfire -> execute();
    }

    warlock_spell_t::tick( d );
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return s -> action -> tick_time( s ) * 15.0;
  }

  virtual bool ready() override
  {
    double active_immolates = p() -> get_active_dots( immolate_action_id );

    if ( active_immolates == 0 )
      return false;

    if ( !p() -> talents.channel_demonfire -> ok() )
      return false;

    return warlock_spell_t::ready();
  }
};

} // end actions namespace

namespace buffs
{
struct debuff_havoc_t: public warlock_buff_t < buff_t >
{
  debuff_havoc_t( warlock_td_t& p ):
    base_t( p, buff_creator_t( static_cast<actor_pair_t>( p ), "havoc", p.source -> find_spell( 80240 ) ) )
  {
  }

  void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );
    warlock.havoc_target = nullptr;
  }
};
}

warlock_td_t::warlock_td_t( player_t* target, warlock_t& p ):
actor_target_data_t( target, &p ),
agony_stack( 1 ),
soc_threshold( 0 ),
warlock( p )
{
  using namespace buffs;
  dots_corruption = target -> get_dot( "corruption", &p );
  for ( int i = 0; i < MAX_UAS; i++ )
    dots_unstable_affliction[i] = target -> get_dot( "unstable_affliction_" + std::to_string( i + 1 ), &p );
  dots_agony = target -> get_dot( "agony", &p );
  dots_doom = target -> get_dot( "doom", &p );
  dots_drain_life = target -> get_dot( "drain_life", &p );
  dots_drain_soul = target -> get_dot( "drain_soul", &p );
  dots_immolate = target -> get_dot( "immolate", &p );
  dots_shadowflame = target -> get_dot( "shadowflame", &p );
  dots_seed_of_corruption = target -> get_dot( "seed_of_corruption", &p );
  dots_phantom_singularity = target -> get_dot( "phantom_singularity", &p );
  dots_channel_demonfire = target -> get_dot( "channel_demonfire", &p );

  debuffs_haunt = buff_creator_t( *this, "haunt", source -> find_spell( 48181 ) )
    .refresh_behavior( BUFF_REFRESH_PANDEMIC );
  debuffs_shadowflame = buff_creator_t( *this, "shadowflame", source -> find_spell( 205181 ) );
  debuffs_agony = buff_creator_t( *this, "agony", source -> find_spell( 980 ) )
    .refresh_behavior( BUFF_REFRESH_PANDEMIC )
    .max_stack( ( warlock.talents.writhe_in_agony -> ok() ? warlock.talents.writhe_in_agony -> effectN( 2 ).base_value() : 10 ) );
  debuffs_eradication = buff_creator_t( *this, "eradication", source -> find_spell( 196414 ) )
    .refresh_behavior( BUFF_REFRESH_PANDEMIC );
  debuffs_roaring_blaze = buff_creator_t( *this, "roaring_blaze", source -> find_spell( 205690 ) )
    .max_stack( 100 );
  debuffs_jaws_of_shadow = buff_creator_t( *this, "jaws_of_shadow", source -> find_spell( 242922 ) );
  debuffs_tormented_agony = buff_creator_t( *this, "tormented_agony", source -> find_spell( 252938 ) );
  debuffs_chaotic_flames = buff_creator_t( *this, "chaotic_flames", source -> find_spell( 253092 ) );


  debuffs_havoc = new buffs::debuff_havoc_t( *this );

  debuffs_flamelicked = buff_creator_t( *this, "flamelicked" )
      .chance( 0 );

  target -> callbacks_on_demise.push_back( std::bind( &warlock_td_t::target_demise, this ) );
}

void warlock_td_t::target_demise()
{
  if ( !( target -> is_enemy() ) )
  {
    return;
  }
  if ( warlock.specialization() == WARLOCK_AFFLICTION && dots_drain_soul -> is_ticking() )
  {
    if ( warlock.sim -> log )
    {
      warlock.sim -> out_debug.printf( "Player %s demised. Warlock %s gains a shard by channeling drain soul during this.", target -> name(), warlock.name() );
    }
    warlock.resource_gain( RESOURCE_SOUL_SHARD, 1, warlock.gains.drain_soul );
  }
  if ( warlock.specialization() == WARLOCK_AFFLICTION )
  {
    for ( int i = 0; i < MAX_UAS; ++i )
    {
      if ( dots_unstable_affliction[i] -> is_ticking() )
      {
        if ( warlock.sim -> log )
        {
          warlock.sim -> out_debug.printf( "Player %s demised. Warlock %s gains a shard from unstable affliction.", target -> name(), warlock.name() );
        }
        warlock.resource_gain( RESOURCE_SOUL_SHARD, 1, warlock.gains.unstable_affliction_refund );

        // you can only get one soul shard per death from UA refunds
        break;
      }
    }
  }
  if ( warlock.specialization() == WARLOCK_AFFLICTION && debuffs_haunt -> check() )
  {
    if ( warlock.sim -> log )
    {
      warlock.sim -> out_debug.printf( "Player %s demised. Warlock %s reset haunt's cooldown.", target -> name(), warlock.name() );
    }
    warlock.cooldowns.haunt -> reset( true );
  }
}

warlock_t::warlock_t( sim_t* sim, const std::string& name, race_e r ):
  player_t( sim, WARLOCK, name, r ),
    havoc_target( nullptr ),
    agony_accumulator( 0 ),
    free_souls( 3 ),
    warlock_pet_list( pets_t() ),
    active( active_t() ),
    talents( talents_t() ),
    legendary( legendary_t() ),
    mastery_spells( mastery_spells_t() ),
    cooldowns( cooldowns_t() ),
    spec( specs_t() ),
    buffs( buffs_t() ),
    gains( gains_t() ),
    procs( procs_t() ),
    spells( spells_t() ),
    initial_soul_shards( 3 ),
    allow_sephuz( false ),
    deaths_embrace_fixed_time( true ),
    default_pet( "" ),
    shard_react( timespan_t::zero() )
  {
    cooldowns.infernal = get_cooldown( "summon_infernal" );
    cooldowns.doomguard = get_cooldown( "summon_doomguard" );
    cooldowns.dimensional_rift = get_cooldown( "dimensional_rift" );
    cooldowns.haunt = get_cooldown( "haunt" );
    cooldowns.sindorei_spite_icd = get_cooldown( "sindorei_spite_icd" );
    cooldowns.call_dreadstalkers = get_cooldown( "call_dreadstalkers" );

    regen_type = REGEN_DYNAMIC;
    regen_caches[CACHE_HASTE] = true;
    regen_caches[CACHE_SPELL_HASTE] = true;
    reap_souls_modifier = 2.0;

    talent_points.register_validity_fn( [this]( const spell_data_t* spell )
    {
      if ( find_item( 151649 ) ) // Soul of the Netherlord
      {
        switch ( specialization() )
        {
          case WARLOCK_AFFLICTION:
            return spell -> id() == 234876; // Death's Embrace
          case WARLOCK_DEMONOLOGY:
            return spell -> id() == 171975; // Shadowy Inspiration
          case WARLOCK_DESTRUCTION:
            return spell -> id() == 196412; // Eradication
          default:
            return false;
        }
      }

      return false;
    } );
  }

double warlock_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( target, school );

  warlock_td_t* td = get_target_data( target );

  if ( td -> debuffs_haunt -> check() )
    m *= 1.0 + find_spell( 48181 ) -> effectN( 2 ).percent();

  if ( talents.contagion -> ok() )
  {
    for ( int i = 0; i < MAX_UAS; i++ )
    {
      if ( td -> dots_unstable_affliction[i] -> is_ticking() )
      {
        m *= 1.0 + talents.contagion -> effectN( 1 ).percent();
        break;
      }
    }
  }

  return m;
}


double warlock_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( buffs.demonic_synergy -> check() )
    m *= 1.0 + buffs.demonic_synergy -> data().effectN( 1 ).percent();

  if ( legendary.stretens_insanity )
    m *= 1.0 + buffs.stretens_insanity -> stack() * buffs.stretens_insanity -> data().effectN( 1 ).percent();

  if ( buffs.soul_harvest -> check() )
    m *= 1.0 + buffs.soul_harvest -> stack_value();

  if ( buffs.instability -> check() )
    m *= 1.0 + find_spell( 216472 ) -> effectN( 1 ).percent();

  if ( specialization() == WARLOCK_DESTRUCTION && dbc::is_school( school, SCHOOL_FIRE ) )
  {
    m *= 1.0 + buffs.alythesss_pyrogenics -> stack_value();
  }

  if ( buffs.deadwind_harvester -> check() )
  {
    m *= 1.0 + buffs.deadwind_harvester -> data().effectN( 1 ).percent();
  }

  m *= 1.0 + buffs.sindorei_spite -> check_stack_value();
  m *= 1.0 + buffs.lessons_of_spacetime -> check_stack_value();

  return m;
}

void warlock_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
  case CACHE_MASTERY:
    if ( mastery_spells.master_demonologist -> ok() )
      player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    break;
  default: break;
  }
}

double warlock_t::composite_spell_crit_chance() const
{
  double sc = player_t::composite_spell_crit_chance();

  return sc;
}

double warlock_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buffs.sephuzs_secret -> check() )
  {
    h *= 1.0 / ( 1.0 + buffs.sephuzs_secret -> check_value() );
  }

  if ( legendary.sephuzs_secret )
  {
    h *= 1.0 / ( 1.0 + legendary.sephuzs_passive );
  }

  if ( buffs.demonic_speed -> check() )
    h *= 1.0 / ( 1.0 + buffs.demonic_speed -> check_value() );

  if ( buffs.dreaded_haste -> check() )
    h *= 1.0 / ( 1.0 + buffs.dreaded_haste -> check_value() );

  return h;
}

double warlock_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( buffs.sephuzs_secret -> check() )
  {
    h *= 1.0 / ( 1.0 + buffs.sephuzs_secret -> check_value() );
  }

  if ( legendary.sephuzs_secret )
  {
    h *= 1.0 / ( 1.0 + legendary.sephuzs_passive );
  }

  if ( buffs.demonic_speed -> check() )
    h *= 1.0 / ( 1.0 + buffs.demonic_speed -> check_value() );

  if ( buffs.dreaded_haste -> check() )
    h *= 1.0 / ( 1.0 + buffs.dreaded_haste -> check_value() );

  return h;
}

double warlock_t::composite_melee_crit_chance() const
{
  double mc = player_t::composite_melee_crit_chance();

  return mc;
}

double warlock_t::composite_mastery() const
{
  double m = player_t::composite_mastery();

  return m;
}

double warlock_t::composite_rating_multiplier( rating_e rating ) const
{
  double m = player_t::composite_rating_multiplier( rating );

  return m;
}

double warlock_t::resource_gain( resource_e resource_type, double amount, gain_t* source, action_t* action )
{

  return player_t::resource_gain( resource_type, amount, source, action );
}

double warlock_t::mana_regen_per_second() const
{
  double mp5 = player_t::mana_regen_per_second();

  mp5 /= cache.spell_haste();

  return mp5;
}

double warlock_t::composite_armor() const
{
  return player_t::composite_armor() + spec.fel_armor -> effectN( 2 ).base_value();
}

void warlock_t::halt()
{
  player_t::halt();

  if ( spells.melee ) spells.melee -> cancel();
}

double warlock_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_INTELLECT )
    return spec.nethermancy -> effectN( 1 ).percent();

  return 0.0;
}

action_t* warlock_t::create_action( const std::string& action_name,const std::string& options_str ) {
  using namespace actions;

  if ((action_name == "summon_pet" || action_name == "service_pet") && default_pet.empty()) {
      sim->errorf("Player %s used a generic pet summoning action without specifying a default_pet.\n", name());
      return nullptr;
  }

  if ( action_name == "conflagrate"           ) return new                       conflagrate_t( this );
  if ( action_name == "demonbolt"             ) return new                         demonbolt_t( this );
  if ( action_name == "doom"                  ) return new                              doom_t( this );
  if ( action_name == "chaos_bolt"            ) return new                        chaos_bolt_t( this );
  if ( action_name == "demonic_empowerment"   ) return new               demonic_empowerment_t( this );
  if ( action_name == "drain_soul"            ) return new                        drain_soul_t( this );
  if ( action_name == "grimoire_of_sacrifice" ) return new             grimoire_of_sacrifice_t( this );
  if ( action_name == "channel_demonfire"     ) return new                 channel_demonfire_t( this );
  if ( action_name == "soul_harvest"          ) return new                      soul_harvest_t( this );
  if ( action_name == "immolate"              ) return new                          immolate_t( this );
  if ( action_name == "incinerate"            ) return new                        incinerate_t( this );
  if ( action_name == "mortal_coil"           ) return new                       mortal_coil_t( this );
  if ( action_name == "shadow_bolt"           ) return new                       shadow_bolt_t( this );
  if ( action_name == "shadowburn"            ) return new                        shadowburn_t( this );
  if ( action_name == "hand_of_guldan"        ) return new                    hand_of_guldan_t( this );
  if ( action_name == "implosion"             ) return new                         implosion_t( this );
  if ( action_name == "havoc"                 ) return new                             havoc_t( this );
  if ( action_name == "cataclysm"             ) return new                         cataclysm_t( this );
  if ( action_name == "rain_of_fire"          ) return new         rain_of_fire_t( this, options_str );
  if ( action_name == "demonwrath"            ) return new                        demonwrath_t( this );
  if ( action_name == "shadowflame"           ) return new                       shadowflame_t( this );
  if ( action_name == "call_dreadstalkers"    ) return new                call_dreadstalkers_t( this );
  if ( action_name == "summon_infernal"       ) return new                   summon_infernal_t( this );
  if ( action_name == "summon_doomguard"      ) return new                  summon_doomguard_t( this );
  if ( action_name == "summon_darkglare"      ) return new                  summon_darkglare_t( this );
  if ( action_name == "summon_felhunter"      ) return new      summon_main_pet_t( "felhunter", this );
  if ( action_name == "summon_felguard"       ) return new       summon_main_pet_t( "felguard", this );
  if ( action_name == "summon_succubus"       ) return new       summon_main_pet_t( "succubus", this );
  if ( action_name == "summon_voidwalker"     ) return new     summon_main_pet_t( "voidwalker", this );
  if ( action_name == "summon_imp"            ) return new            summon_main_pet_t( "imp", this );
  if ( action_name == "summon_pet"            ) return new      summon_main_pet_t( default_pet, this );
  if ( action_name == "service_felguard"      ) return new               grimoire_of_service_t( this, "felguard" );
  if ( action_name == "service_felhunter"     ) return new               grimoire_of_service_t( this, "felhunter" );
  if ( action_name == "service_imp"           ) return new               grimoire_of_service_t( this, "imp" );
  if ( action_name == "service_succubus"      ) return new               grimoire_of_service_t( this, "succubus" );
  if ( action_name == "service_voidwalker"    ) return new               grimoire_of_service_t( this, "voidwalker" );
  if ( action_name == "service_pet"           ) return new               grimoire_of_service_t( this,  talents.grimoire_of_supremacy -> ok() ? "doomguard" : default_pet );

  action_t* aff_action = create_action_affliction(action_name, options_str);
  if (aff_action)
      return aff_action;
  
  return player_t::create_action(action_name, options_str);
}

bool warlock_t::create_actions()
{
	using namespace actions;

	return player_t::create_actions();
}

pet_t* warlock_t::create_pet( const std::string& pet_name,
                              const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  using namespace pets;

  if ( pet_name == "felguard"     ) return new    felguard_pet_t( sim, this );
  if ( pet_name == "felhunter"    ) return new   felhunter_pet_t( sim, this );
  if ( pet_name == "imp"          ) return new         imp_pet_t( sim, this );
  if ( pet_name == "succubus"     ) return new    succubus_pet_t( sim, this );
  if ( pet_name == "voidwalker"   ) return new  voidwalker_pet_t( sim, this );

  if ( pet_name == "service_felguard"     ) return new    felguard_pet_t( sim, this, pet_name );
  if ( pet_name == "service_felhunter"    ) return new   felhunter_pet_t( sim, this, pet_name );
  if ( pet_name == "service_imp"          ) return new         imp_pet_t( sim, this, pet_name );
  if ( pet_name == "service_succubus"     ) return new    succubus_pet_t( sim, this, pet_name );
  if ( pet_name == "service_voidwalker"   ) return new  voidwalker_pet_t( sim, this, pet_name );

  return nullptr;
}

void warlock_t::create_pets()
{
  for ( size_t i = 0; i < pet_name_list.size(); ++i )
  {
    create_pet( pet_name_list[ i ] );
  }

  for ( size_t i = 0; i < warlock_pet_list.infernal.size(); i++ )
  {
    warlock_pet_list.infernal[i] = new pets::infernal_t( sim, this );
  }
  for ( size_t i = 0; i < warlock_pet_list.doomguard.size(); i++ )
  {
    warlock_pet_list.doomguard[i] = new pets::doomguard_t( sim, this );
  }

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    for ( size_t i = 0; i < warlock_pet_list.wild_imps.size(); i++ )
    {
      warlock_pet_list.wild_imps[ i ] = new pets::wild_imp_pet_t( sim, this );
      if ( i > 0 )
        warlock_pet_list.wild_imps[ i ] -> quiet = 1;
      //warlock_pet_list.wild_imps [ i ].ascendance = new thalkiels_ascendance_pet_spell_t( *warlock_pet_list.wild_imps [ i ] );
    }
    for ( size_t i = 0; i < warlock_pet_list.dreadstalkers.size(); i++ )
    {
      warlock_pet_list.dreadstalkers[ i ] = new pets::dreadstalker_t( sim, this );
    }
    for ( size_t i = 0; i < warlock_pet_list.darkglare.size(); i++ )
    {
      warlock_pet_list.darkglare[i] = new pets::darkglare_t( sim, this );
    }
    if ( sets->has_set_bonus( WARLOCK_DEMONOLOGY, T18, B4 ) )
    {
      for ( size_t i = 0; i < warlock_pet_list.t18_illidari_satyr.size(); i++ )
      {
        warlock_pet_list.t18_illidari_satyr[i] = new pets::t18_illidari_satyr_t( sim, this );
      }
      for ( size_t i = 0; i < warlock_pet_list.t18_prince_malchezaar.size(); i++ )
      {
        warlock_pet_list.t18_prince_malchezaar[i] = new pets::t18_prince_malchezaar_t( sim, this );
      }
      for ( size_t i = 0; i < warlock_pet_list.t18_vicious_hellhound.size(); i++ )
      {
        warlock_pet_list.t18_vicious_hellhound[i] = new pets::t18_vicious_hellhound_t( sim, this );
      }
    }
  }
}

void warlock_t::init_spells()
{
  player_t::init_spells();

  warlock_t::init_spells_affliction();

  // General
  spec.fel_armor   = find_spell( 104938 );
  spec.nethermancy = find_spell( 86091 );
  spec.affliction = find_specialization_spell( 137043 );
  spec.demonology = find_specialization_spell( 137044 );
  spec.destruction = find_specialization_spell( 137046 );

  // Specialization Spells
  // PTR
  spec.drain_soul             = find_specialization_spell( "Drain Soul" );

  spec.immolate               = find_specialization_spell( "Immolate" );
  spec.nightfall              = find_specialization_spell( "Nightfall" );
  spec.demonic_empowerment    = find_specialization_spell( "Demonic Empowerment" );
  spec.wild_imps              = find_specialization_spell( "Wild Imps" );
  spec.shadow_bite            = find_specialization_spell( "Shadow Bite" );
  spec.shadow_bite_2          = find_specialization_spell( 231799 );
  spec.conflagrate            = find_specialization_spell( "Conflagrate" );
  spec.conflagrate_2          = find_specialization_spell( 231793 );
  spec.unending_resolve       = find_specialization_spell( "Unending Resolve" );
  spec.unending_resolve_2     = find_specialization_spell( 231794 );
  spec.firebolt               = find_specialization_spell( "Firebolt" );
  spec.firebolt_2             = find_specialization_spell( 231795 );

  // Removed terniary for compat.
  spec.doom                   = find_spell( 603 );

  // Mastery
  mastery_spells.chaotic_energies    = find_mastery_spell( WARLOCK_DESTRUCTION );
  mastery_spells.master_demonologist = find_mastery_spell( WARLOCK_DEMONOLOGY );

  // Talents

  // Ptr
  talents.drain_soul             = find_talent_spell( "Drain Soul" );

  talents.backdraft              = find_talent_spell( "Backdraft" );
  talents.fire_and_brimstone     = find_talent_spell( "Fire and Brimstone" );
  talents.shadowburn             = find_talent_spell( "Shadowburn" );

  talents.shadowy_inspiration    = find_talent_spell( "Shadowy Inspiration" );
  talents.shadowflame            = find_talent_spell( "Shadowflame" );
  talents.demonic_calling        = find_talent_spell( "Demonic Calling" );

  talents.contagion              = find_talent_spell( "Contagion" );

  talents.reverse_entropy        = find_talent_spell( "Reverse Entropy" );
  talents.roaring_blaze          = find_talent_spell( "Roaring Blaze" );

  talents.impending_doom         = find_talent_spell( "Impending Doom" );
  talents.improved_dreadstalkers = find_talent_spell( "Improved Dreadstalkers" );
  talents.implosion              = find_talent_spell( "Implosion" );

  talents.demon_skin             = find_talent_spell( "Soul Leech" );
  talents.mortal_coil            = find_talent_spell( "Mortal Coil" );
  talents.howl_of_terror         = find_talent_spell( "Howl of Terror" );
  talents.shadowfury             = find_talent_spell( "Shadowfury" );

  talents.eradication            = find_talent_spell( "Eradication" );
  talents.cataclysm              = find_talent_spell( "Cataclysm" );

  talents.hand_of_doom           = find_talent_spell( "Hand of Doom" );
  talents.power_trip			       = find_talent_spell( "Power Trip" );

  talents.soul_harvest           = find_talent_spell( "Soul Harvest" );

  talents.demonic_circle         = find_talent_spell( "Demonic Circle" );
  talents.burning_rush           = find_talent_spell( "Burning Rush" );
  talents.dark_pact              = find_talent_spell( "Dark Pact" );

  talents.grimoire_of_supremacy  = find_talent_spell( "Grimoire of Supremacy" );
  talents.grimoire_of_service    = find_talent_spell( "Grimoire of Service" );
  talents.grimoire_of_sacrifice  = find_talent_spell( "Grimoire of Sacrifice" );
  talents.grimoire_of_synergy    = find_talent_spell( "Grimoire of Synergy" );

  talents.deaths_embrace         = find_talent_spell( "Death's Embrace" );

  talents.wreak_havoc            = find_talent_spell( "Wreak Havoc" );
  talents.channel_demonfire      = find_talent_spell( "Channel Demonfire" );

  talents.summon_darkglare       = find_talent_spell( "Summon Darkglare" );
  talents.demonbolt              = find_talent_spell( "Demonbolt" );

  talents.soul_conduit           = find_talent_spell( "Soul Conduit" );

  // Active Spells
  active.demonic_power_proc = new actions::demonic_power_damage_t( this );
  active.thalkiels_discord = new actions::thalkiels_discord_t( this );
  active.harvester_of_souls = new actions::harvester_of_souls_t( this );
  active.cry_havoc = new actions::cry_havoc_t( this );
  active.rend_soul = new actions::rend_soul_t( this );
}

void warlock_t::init_base_stats()
{
  if ( base.distance < 1 )
    base.distance = 40;

  player_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility = 0.0;
  base.spell_power_per_intellect = 1.0;

  base.attribute_multiplier[ATTR_STAMINA] *= 1.0 + spec.fel_armor -> effectN( 1 ).percent();

  base.mana_regen_per_second = resources.base[RESOURCE_MANA] * 0.01;

  resources.base[RESOURCE_SOUL_SHARD] = 5;

  if ( default_pet.empty() )
  {
    if ( specialization() == WARLOCK_AFFLICTION )
      default_pet = "felhunter";
    else if ( specialization() == WARLOCK_DEMONOLOGY )
      default_pet = "felguard";
    else if ( specialization() == WARLOCK_DESTRUCTION )
      default_pet = "imp";
  }
}

void warlock_t::init_scaling()
{
  player_t::init_scaling();
}

struct stolen_power_stack_t : public buff_t
{
  stolen_power_stack_t( warlock_t* p ) :
    buff_t( buff_creator_t( p, "stolen_power_stack", p -> find_spell( 211529 ) ).chance( 1.0 ) )
  {
  }

  void execute( int a, double b, timespan_t t ) override
  {
    warlock_t* p = debug_cast<warlock_t*>( player );

    buff_t::execute( a, b, t );

    if ( p -> buffs.stolen_power_stacks -> stack() == 100 )
    {
      p -> buffs.stolen_power_stacks -> reset();
      p -> buffs.stolen_power -> trigger();
    }
  }
};

struct t18_4pc_driver_t : public buff_t        //kept to force imps to proc
{
  timespan_t illidari_satyr_duration;
  timespan_t vicious_hellhound_duration;

  t18_4pc_driver_t( warlock_t* p ) :
    buff_t( buff_creator_t( p, "t18_4pc_driver" ).activated( true ).duration( timespan_t::from_millis( 500 ) ).tick_behavior( BUFF_TICK_NONE ) )
  {
    vicious_hellhound_duration = p -> find_spell( 189298 ) -> duration();
    illidari_satyr_duration = p -> find_spell( 189297 ) -> duration();
  }

  void execute( int a, double b, timespan_t t ) override
  {
    warlock_t* p = debug_cast<warlock_t*>( player );

    buff_t::execute( a, b, t );

    //Which pet will we spawn?
    double pet = rng().range( 0.0, 1.0 );
    if ( pet <= 0.6 ) // 60% chance to spawn hellhound
    {
      for ( size_t i = 0; i < p -> warlock_pet_list.t18_vicious_hellhound.size(); i++ )
      {
        if ( p -> warlock_pet_list.t18_vicious_hellhound[i] -> is_sleeping() )
        {
          p -> warlock_pet_list.t18_vicious_hellhound[i] -> summon( vicious_hellhound_duration );
          p -> procs.t18_vicious_hellhound -> occur();
          break;
        }
      }
    }
    else // 40% chance to spawn illidari
    {
      for ( size_t i = 0; i < p -> warlock_pet_list.t18_illidari_satyr.size(); i++ )
      {
        if ( p -> warlock_pet_list.t18_illidari_satyr[i] -> is_sleeping() )
        {
          p -> warlock_pet_list.t18_illidari_satyr[i] -> summon( illidari_satyr_duration );
          p -> procs.t18_illidari_satyr -> occur();
          break;
        }
      }
    }
  }
};

void warlock_t::create_buffs()
{
  player_t::create_buffs();

  create_buffs_affliction();

  buffs.demonic_power = buff_creator_t( this, "demonic_power", talents.grimoire_of_sacrifice -> effectN( 2 ).trigger() );
  buffs.soul_harvest = buff_creator_t( this, "soul_harvest", find_spell( 196098 ) )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .refresh_behavior( BUFF_REFRESH_EXTEND )
    .cd( timespan_t::zero() )
    .default_value( find_spell( 196098 ) -> effectN( 1 ).percent() );

  //legendary buffs
  buffs.stretens_insanity = buff_creator_t( this, "stretens_insanity", find_spell( 208822 ) )
    .default_value( find_spell( 208822 ) -> effectN( 1 ).percent() )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .tick_behavior( BUFF_TICK_NONE );
  buffs.lessons_of_spacetime = buff_creator_t( this, "lessons_of_spacetime", find_spell( 236176 ) )
    .default_value( find_spell( 236176 ) -> effectN( 1 ).percent() )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .refresh_behavior( BUFF_REFRESH_DURATION )
    .tick_behavior( BUFF_TICK_NONE );
  buffs.sephuzs_secret =
    haste_buff_creator_t( this, "sephuzs_secret", find_spell( 208052 ) )
    .default_value( find_spell( 208052 ) -> effectN( 2 ).percent() )
    .cd( find_spell( 226262 ) -> duration() );
  buffs.alythesss_pyrogenics = buff_creator_t( this, "alythesss_pyrogenics", find_spell( 205675 ) )
    .default_value( find_spell( 205675 ) -> effectN( 1 ).percent() )
    .refresh_behavior( BUFF_REFRESH_DURATION );
  buffs.wakeners_loyalty = buff_creator_t( this, "wakeners_loyalty", find_spell( 236200 ) )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .default_value( find_spell( 236200 ) -> effectN( 1 ).percent() );

  //affliction buffs
  buffs.shard_instability = buff_creator_t( this, "shard_instability", find_spell( 216457 ) )
    .chance( sets->set( WARLOCK_AFFLICTION, T18, B2 ) -> proc_chance() );
  buffs.instability = buff_creator_t( this, "instability", sets->set( WARLOCK_AFFLICTION, T18, B4 ) -> effectN( 1 ).trigger() )
    .chance( sets->set( WARLOCK_AFFLICTION, T18, B4 ) -> proc_chance() )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.deadwind_harvester = buff_creator_t( this, "deadwind_harvester", find_spell( 216708 ) )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .refresh_behavior( BUFF_REFRESH_EXTEND );
  buffs.tormented_souls = buff_creator_t( this, "tormented_souls", find_spell( 216695 ) )
    .tick_behavior( BUFF_TICK_NONE );
  buffs.compounding_horror = buff_creator_t( this, "compounding_horror", find_spell( 199281 ) );
  buffs.demonic_speed = haste_buff_creator_t( this, "demonic_speed", sets -> set( WARLOCK_AFFLICTION, T20, B4 ) -> effectN( 1 ).trigger() )
    .chance( sets -> set( WARLOCK_AFFLICTION, T20, B4 ) -> proc_chance() )
    .default_value( sets -> set( WARLOCK_AFFLICTION, T20, B4 ) -> effectN( 1 ).trigger() -> effectN( 1 ).percent() );
  buffs.wrath_of_consumption = buff_creator_t( this, "wrath_of_consumption", find_spell( 199646 ) )
    .refresh_behavior( BUFF_REFRESH_DURATION )
    .max_stack( 5 );


  //demonology buffs
  buffs.demonic_synergy = buff_creator_t( this, "demonic_synergy", find_spell( 171982 ) )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .chance( 1 );
  buffs.tier18_2pc_demonology = buff_creator_t( this, "demon_rush", sets->set( WARLOCK_DEMONOLOGY, T18, B2 ) -> effectN( 1 ).trigger() )
    .default_value( sets->set( WARLOCK_DEMONOLOGY, T18, B2 ) -> effectN( 1 ).trigger() -> effectN( 1 ).percent() );
  buffs.shadowy_inspiration = buff_creator_t( this, "shadowy_inspiration", find_spell( 196606 ) );
  buffs.demonic_calling = buff_creator_t( this, "demonic_calling", talents.demonic_calling -> effectN( 1 ).trigger() );
  buffs.t18_4pc_driver = new t18_4pc_driver_t( this );
  buffs.stolen_power_stacks = new stolen_power_stack_t( this );
  buffs.stolen_power = buff_creator_t( this, "stolen_power", find_spell( 211583 ) )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buffs.dreaded_haste = haste_buff_creator_t( this, "dreaded_haste", sets -> set( WARLOCK_DEMONOLOGY, T20, B4 ) -> effectN( 1 ).trigger() )
    .default_value( sets -> set( WARLOCK_DEMONOLOGY, T20, B4 ) -> effectN( 1 ).trigger() -> effectN( 1 ).percent() );
  buffs.rage_of_guldan = buff_creator_t(this, "rage_of_guldan", sets->set( WARLOCK_DEMONOLOGY, T21, B2 ) -> effectN( 1 ).trigger() )
	  .duration( find_spell( 257926 ) -> duration() )
	  .max_stack( find_spell( 257926 ) -> max_stacks() )
	  .default_value( find_spell( 257926 ) -> effectN( 1 ).base_value() )
	  .refresh_behavior( BUFF_REFRESH_DURATION );


  //destruction buffs
  buffs.backdraft = buff_creator_t( this, "backdraft", find_spell( 117828 ) );
  buffs.lord_of_flames = buff_creator_t( this, "lord_of_flames", find_spell( 226802 ) )
    .tick_behavior( BUFF_TICK_NONE );
  buffs.embrace_chaos = buff_creator_t( this, "embrace_chaos", sets->set( WARLOCK_DESTRUCTION,T19, B2 ) -> effectN( 1 ).trigger() )
    .chance( sets->set( WARLOCK_DESTRUCTION, T19, B2 ) -> proc_chance() );
  buffs.active_havoc = buff_creator_t( this, "active_havoc" )
    .tick_behavior( BUFF_TICK_NONE )
    .refresh_behavior( BUFF_REFRESH_DURATION )
    .duration( timespan_t::from_seconds( 10 ) );
}

void warlock_t::init_rng()
{
  player_t::init_rng();

  init_rng_affliction();

  demonic_power_rppm = get_rppm( "demonic_power", find_spell( 196099 ) );
  grimoire_of_synergy = get_rppm( "grimoire_of_synergy", talents.grimoire_of_synergy );
  grimoire_of_synergy_pet = get_rppm( "grimoire_of_synergy_pet", talents.grimoire_of_synergy );
}

void warlock_t::init_gains()
{
  player_t::init_gains();

  gains.life_tap                    = get_gain( "life_tap" );
  gains.agony                       = get_gain( "agony" );
  gains.conflagrate                 = get_gain( "conflagrate" );
  gains.shadowburn                  = get_gain( "shadowburn" );
  gains.immolate                    = get_gain( "immolate" );
  gains.immolate_crits              = get_gain( "immolate_crits" );
  gains.shadowburn_shard            = get_gain( "shadowburn_shard" );
  gains.miss_refund                 = get_gain( "miss_refund" );
  gains.seed_of_corruption          = get_gain( "seed_of_corruption" );
  gains.drain_soul                  = get_gain( "drain_soul" );
  gains.unstable_affliction_refund  = get_gain( "unstable_affliction_refund" );
  gains.shadow_bolt                 = get_gain( "shadow_bolt" );
  gains.soul_conduit                = get_gain( "soul_conduit" );
  gains.reverse_entropy             = get_gain( "reverse_entropy" );
  gains.soulsnatcher                = get_gain( "soulsnatcher" );
  gains.power_trip                  = get_gain( "power_trip" );
  gains.t18_4pc_destruction         = get_gain( "t18_4pc_destruction" );
  gains.demonwrath                  = get_gain( "demonwrath" );
  gains.t19_2pc_demonology          = get_gain( "t19_2pc_demonology" );
  gains.recurrent_ritual            = get_gain( "recurrent_ritual" );
  gains.feretory_of_souls           = get_gain( "feretory_of_souls" );
  gains.power_cord_of_lethtendris   = get_gain( "power_cord_of_lethtendris" );
  gains.incinerate                  = get_gain( "incinerate" );
  gains.incinerate_crits            = get_gain( "incinerate_crits" );
  gains.dimensional_rift            = get_gain( "dimensional_rift" );
  gains.affliction_t20_2pc          = get_gain( "affliction_t20_2pc" );
  gains.destruction_t20_2pc         = get_gain( "destruction_t20_2pc" );
}

void warlock_t::init_procs()
{
  player_t::init_procs();

  procs.fatal_echos = get_proc( "fatal_echos" );
  procs.wild_imp = get_proc( "wild_imp" );
  procs.fragment_wild_imp = get_proc( "fragment_wild_imp" );
  procs.t18_4pc_destruction = get_proc( "t18_4pc_destruction" );
  procs.t18_prince_malchezaar = get_proc( "t18_prince_malchezaar" );
  procs.t18_vicious_hellhound = get_proc( "t18_vicious_hellhound" );
  procs.t18_illidari_satyr = get_proc( "t18_illidari_satyr" );
  procs.shadowy_tear = get_proc( "shadowy_tear" );
  procs.flame_rift = get_proc( "flame_rift" );
  procs.chaos_tear = get_proc( "chaos_tear" );
  procs.chaos_portal = get_proc( "chaos_portal" );
  procs.dreadstalker_debug = get_proc( "dreadstalker_debug" );
  procs.dimension_ripper = get_proc( "dimension_ripper" );
  procs.one_shard_hog = get_proc( "one_shard_hog" );
  procs.two_shard_hog = get_proc( "two_shard_hog" );
  procs.three_shard_hog = get_proc( "three_shard_hog" );
  procs.four_shard_hog = get_proc( "four_shard_hog" );
  procs.impending_doom = get_proc( "impending_doom" );
  procs.improved_dreadstalkers = get_proc( "improved_dreadstalkers" );
  procs.thalkiels_discord = get_proc( "thalkiels_discord" );
  procs.demonic_calling = get_proc( "demonic_calling" );
  procs.power_trip = get_proc( "power_trip" );
  procs.stolen_power_stack = get_proc( "stolen_power_proc" );
  procs.stolen_power_used = get_proc( "stolen_power_used" );
  procs.soul_conduit = get_proc( "soul_conduit" );
  procs.the_master_harvester = get_proc( "the_master_harvester" );
  procs.t18_demo_4p = get_proc( "t18_demo_4p" );
  procs.souls_consumed = get_proc( "souls_consumed" );
  procs.the_expendables = get_proc( "the_expendables" );
  procs.wilfreds_dog = get_proc( "wilfreds_dog" );
  procs.wilfreds_imp = get_proc( "wilfreds_imp" );
  procs.wilfreds_darkglare = get_proc( "wilfreds_darkglare" );
  procs.t19_2pc_chaos_bolts = get_proc( "t19_2pc_chaos_bolt" );
  procs.demonology_t20_2pc = get_proc( "demonology_t20_2pc" );
  procs.ua_tick_no_mg = get_proc( "ua_tick_no_mg" );
  procs.ua_tick_mg = get_proc( "ua_tick_mg" );
  procs.affliction_t21_2pc = get_proc("affliction_t21_2pc");
}

void warlock_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );

  precombat->add_action( "snapshot_stats" );

  if ( sim -> allow_potions )
  {
    precombat->add_action( "potion" );
  }
}

std::string warlock_t::default_potion() const
{
  std::string lvl110_potion = "prolonged_power";

  return ( true_level >= 100 ) ? lvl110_potion :
         ( true_level >=  90 ) ? "draenic_intellect" :
         ( true_level >=  85 ) ? "jade_serpent" :
         ( true_level >=  80 ) ? "volcanic" :
                                 "disabled";
}

std::string warlock_t::default_flask() const
{
  return ( true_level >= 100 ) ? "whispered_pact" :
         ( true_level >=  90 ) ? "greater_draenic_intellect_flask" :
         ( true_level >=  85 ) ? "warm_sun" :
         ( true_level >=  80 ) ? "draconic_mind" :
                                 "disabled";
}

std::string warlock_t::default_food() const
{
  std::string lvl100_food =
    (specialization() == WARLOCK_DESTRUCTION) ?   "frosty_stew" :
    (specialization() == WARLOCK_DEMONOLOGY) ?    "frosty_stew" :
    (specialization() == WARLOCK_AFFLICTION) ?    "felmouth_frenzy" :
                                                  "felmouth_frenzy";

  std::string lvl110_food =
    (specialization() == WARLOCK_AFFLICTION) ?    "nightborne_delicacy_platter" :
                                                  "azshari_salad";

  return ( true_level > 100 ) ? lvl110_food :
         ( true_level >  90 ) ? lvl100_food :
                                "disabled";
}

std::string warlock_t::default_rune() const
{
  return ( true_level >= 110 ) ? "defiled" :
         ( true_level >= 100 ) ? "focus" :
                                 "disabled";
}

void warlock_t::apl_global_filler()
{
  add_action( "Life Tap" );
}

void warlock_t::apl_default()
{
}

void warlock_t::apl_affliction()
{
    create_apl_aff();
}

void warlock_t::apl_demonology()
{
  action_priority_list_t* default_list = get_action_priority_list("default");

  default_list->add_action("shadow_bolt");
}

void warlock_t::apl_destruction()
{
    action_priority_list_t* default_list = get_action_priority_list("default");

    default_list->add_action("incinerate");
}

void warlock_t::init_action_list()
{
  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    apl_precombat();

    switch ( specialization() )
    {
    case WARLOCK_AFFLICTION:
      apl_affliction();
      break;
    case WARLOCK_DESTRUCTION:
      apl_destruction();
      break;
    case WARLOCK_DEMONOLOGY:
      apl_demonology();
      break;
    default:
      apl_default();
      break;
    }

    use_default_action_list = true;
  }

  player_t::init_action_list();
}

void warlock_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[RESOURCE_SOUL_SHARD] = initial_soul_shards;

  if ( warlock_pet_list.active )
    warlock_pet_list.active -> init_resources( force );
}

void warlock_t::combat_begin()
{
  if ( !sim -> fixed_time )
  {
    if ( deaths_embrace_fixed_time )
    {
      for ( size_t i = 0; i < sim -> player_list.size(); ++i )
      {
        player_t* p = sim -> player_list[i];
        if ( p -> specialization() != WARLOCK_AFFLICTION && p -> type != PLAYER_PET )
        {
          deaths_embrace_fixed_time = false;
          break;
        }
      }
      if ( deaths_embrace_fixed_time )
      {
        sim -> fixed_time = true;
        sim->errorf( "To fix issues with the target exploding <20% range due to execute, fixed_time=1 has been enabled. This gives similar results" );
        sim->errorf( "to execute's usage in a raid sim, without taking an eternity to simulate. To disable this option, add warrior_fixed_time=0 to your sim." );
      }
    }
  }
  player_t::combat_begin();
}

void warlock_t::reset()
{
  player_t::reset();

  // Figure out up to what actor ID we should reset. This is the max of target list actors, and
  // their pets
//  size_t max_idx = sim -> target_list.data().back() -> actor_index + 1;
//  if ( sim -> target_list.data().back() -> pet_list.size() > 0 )
//  {
//    max_idx = sim -> target_list.data().back() -> pet_list.back() -> actor_index + 1;
//  }

  range::for_each( sim -> target_list, [ this ]( const player_t* t ) {
    if ( auto td = target_data[ t ] )
    {
      td -> reset();
    }

    range::for_each( t -> pet_list, [ this ]( const player_t* add ) {
      if ( auto td = target_data[ add ] )
      {
        td -> reset();
      }
    } );
  } );

  warlock_pet_list.active = nullptr;
  shard_react = timespan_t::zero();
  havoc_target = nullptr;
  agony_accumulator = rng().range( 0.0, 0.99 );
  demonwrath_accumulator = 0.0;
  free_souls = 3;
}

void warlock_t::create_options()
{
  player_t::create_options();

  add_option( opt_int( "soul_shards", initial_soul_shards ) );
  add_option( opt_string( "default_pet", default_pet ) );
  add_option( opt_bool( "allow_sephuz", allow_sephuz ) );
  add_option( opt_bool( "deaths_embrace_fixed_time", deaths_embrace_fixed_time ) );
}

std::string warlock_t::create_profile( save_e stype )
{
  std::string profile_str = player_t::create_profile( stype );

  if ( stype == SAVE_ALL )
  {
    if ( initial_soul_shards != 3 )    profile_str += "soul_shards=" + util::to_string( initial_soul_shards ) + "\n";
    if ( ! default_pet.empty() )       profile_str += "default_pet=" + default_pet + "\n";
    if ( allow_sephuz != 0 )           profile_str += "allow_sephuz=" + util::to_string( allow_sephuz ) + "\n";
  }

  return profile_str;
}

void warlock_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  warlock_t* p = debug_cast<warlock_t*>( source );

  initial_soul_shards = p -> initial_soul_shards;
  allow_sephuz = p -> allow_sephuz;
  default_pet = p -> default_pet;
  deaths_embrace_fixed_time = p->deaths_embrace_fixed_time;
}

stat_e warlock_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
    // This is all a guess at how the hybrid primaries will work, since they
    // don't actually appear on cloth gear yet. TODO: confirm behavior
  case STAT_STR_AGI_INT:
  case STAT_AGI_INT:
  case STAT_STR_INT:
    return STAT_INTELLECT;
  case STAT_STR_AGI:
    return STAT_NONE;
  case STAT_SPIRIT:
    return STAT_NONE;
  case STAT_BONUS_ARMOR:
    return STAT_NONE;
  default: return s;
  }
}

expr_t* warlock_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "shard_react" )
  {
    struct shard_react_expr_t: public expr_t
    {
      warlock_t& player;
      shard_react_expr_t( warlock_t& p ):
        expr_t( "shard_react" ), player( p ) { }
      virtual double evaluate() override { return player.resources.current[RESOURCE_SOUL_SHARD] >= 1 && player.sim -> current_time() >= player.shard_react; }
    };
    return new shard_react_expr_t( *this );
  }

  else if ( name_str == "felstorm_is_ticking" )
  {
    struct felstorm_is_ticking_expr_t: public expr_t
    {
      pets::warlock_pet_t* felguard;
      felstorm_is_ticking_expr_t( pets::warlock_pet_t* f ):
        expr_t( "felstorm_is_ticking" ), felguard( f ) { }
      virtual double evaluate() override { return ( felguard ) ? felguard -> special_action -> get_dot() -> is_ticking() : false; }
    };
    return new felstorm_is_ticking_expr_t( debug_cast<pets::warlock_pet_t*>( find_pet( "felguard" ) ) );
  }

  else if( name_str == "wild_imp_count")
  {
    struct wild_imp_count_expr_t: public expr_t
    {
        warlock_t& player;

        wild_imp_count_expr_t( warlock_t& p ):
          expr_t( "wild_imp_count" ), player( p ) { }
        virtual double evaluate() override
        {
            double t = 0;
            for(auto& pet : player.warlock_pet_list.wild_imps)
            {
                if(!pet->is_sleeping())
                    t++;
            }
            return t;
        }

    };
    return new wild_imp_count_expr_t( *this );
  }

  else if ( name_str == "darkglare_count" )
  {
    struct darkglare_count_expr_t : public expr_t
    {
      warlock_t& player;

      darkglare_count_expr_t( warlock_t& p ) :
        expr_t( "darkglare_count" ), player( p ) { }
      virtual double evaluate() override
      {
        double t = 0;
        for ( auto& pet : player.warlock_pet_list.darkglare )
        {
          if ( !pet -> is_sleeping() )
            t++;
        }
        return t;
      }

    };
    return new darkglare_count_expr_t( *this );
  }

  else if( name_str == "dreadstalker_count")
  {
      struct dreadstalker_count_expr_t: public expr_t
      {
          warlock_t& player;

          dreadstalker_count_expr_t( warlock_t& p ):
            expr_t( "dreadstalker_count" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 0;
              for(auto& pet : player.warlock_pet_list.dreadstalkers)
              {
                  if(!pet->is_sleeping())
                      t++;
              }
              return t;
          }

      };
      return new dreadstalker_count_expr_t( *this );
  }
  else if( name_str == "doomguard_count")
  {
      struct doomguard_count_expr_t: public expr_t
      {
          warlock_t& player;

          doomguard_count_expr_t( warlock_t& p ):
            expr_t( "doomguard_count" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 0;
              for(auto& pet : player.warlock_pet_list.doomguard)
              {
                  if(!pet->is_sleeping())
                      t++;
              }
              return t;
          }

      };
      return new doomguard_count_expr_t( *this );
  }
  else if( name_str == "infernal_count" )
  {
      struct infernal_count_expr_t: public expr_t
      {
          warlock_t& player;

          infernal_count_expr_t( warlock_t& p ):
            expr_t( "infernal_count" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 0;
              for(auto& pet : player.warlock_pet_list.infernal)
              {
                  if(!pet->is_sleeping())
                      t++;
              }
              return t;
          }

      };
      return new infernal_count_expr_t( *this );
  }
  else if( name_str == "service_count" )
  {
      struct service_count_expr_t: public expr_t
      {
          warlock_t& player;

          service_count_expr_t( warlock_t& p ):
            expr_t( "service_count" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 0;
              for(auto& pet : player.pet_list)
              {
                  pets::warlock_pet_t *lock_pet = dynamic_cast<pets::warlock_pet_t*> ( pet );
                  if(lock_pet != nullptr)
                  {
                      if(lock_pet->is_grimoire_of_service)
                      {
                          if( !lock_pet->is_sleeping() )
                          {
                              t++;
                          }
                      }
                  }
              }
              return t;
          }

      };
      return new service_count_expr_t( *this );
  }
  else if ( name_str == "pet_count" )
  {
    struct pet_count_expr_t : public expr_t
    {
      warlock_t& player;

      pet_count_expr_t( warlock_t& p ) :
        expr_t( "pet_count" ), player( p ) { }
      virtual double evaluate() override
      {
        double t = 0;
        for ( auto& pet : player.pet_list )
        {
          pets::warlock_pet_t *lock_pet = dynamic_cast<pets::warlock_pet_t*> ( pet );
          if ( lock_pet != nullptr )
          {
            if ( !lock_pet->is_sleeping() )
            {
              t++;
            }
          }
        }
        return t;
      }

    };
    return new pet_count_expr_t( *this );
  }
  else if( name_str == "wild_imp_no_de" )
  {
      struct wild_imp_without_de_expr_t: public expr_t
      {
          warlock_t& player;

          wild_imp_without_de_expr_t( warlock_t& p ):
            expr_t( "wild_imp_no_de" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 0;
              for(auto& pet : player.warlock_pet_list.wild_imps)
              {
                  if(!pet->is_sleeping() & !pet->buffs.demonic_empowerment->up())
                      t++;
              }
              return t;
          }

      };
      return new wild_imp_without_de_expr_t( *this );
  }

  else if ( name_str == "darkglare_no_de" )
  {
    struct darkglare_without_de_expr_t : public expr_t
    {
      warlock_t& player;

      darkglare_without_de_expr_t( warlock_t& p ) :
        expr_t( "darkglare_no_de" ), player( p ) { }
      virtual double evaluate() override
      {
        double t = 0;
        for ( auto& pet : player.warlock_pet_list.darkglare )
        {
          if ( !pet -> is_sleeping() & !pet -> buffs.demonic_empowerment -> up() )
            t++;
        }
        return t;
      }

    };
    return new darkglare_without_de_expr_t( *this );
  }

  else if( name_str == "dreadstalker_no_de" )
  {
      struct dreadstalker_without_de_expr_t: public expr_t
      {
          warlock_t& player;

          dreadstalker_without_de_expr_t( warlock_t& p ):
            expr_t( "dreadstalker_no_de" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 0;
              for(auto& pet : player.warlock_pet_list.dreadstalkers)
              {
                  if(!pet->is_sleeping() & !pet->buffs.demonic_empowerment->up())
                      t++;
              }
              return t;
          }

      };
      return new dreadstalker_without_de_expr_t( *this );
  }
  else if( name_str == "doomguard_no_de" )
  {
      struct doomguard_without_de_expr_t: public expr_t
      {
          warlock_t& player;

          doomguard_without_de_expr_t( warlock_t& p ):
            expr_t( "doomguard_no_de" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 0;
              for(auto& pet : player.warlock_pet_list.doomguard)
              {
                  if(!pet->is_sleeping() & !pet->buffs.demonic_empowerment->up())
                      t++;
              }
              return t;
          }

      };
      return new doomguard_without_de_expr_t( *this );
  }
  else if( name_str == "infernal_no_de" )
  {
      struct infernal_without_de_expr_t: public expr_t
      {
          warlock_t& player;

          infernal_without_de_expr_t( warlock_t& p ):
            expr_t( "infernal_no_de" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 0;
              for(auto& pet : player.warlock_pet_list.infernal)
              {
                  if(!pet->is_sleeping() & !pet->buffs.demonic_empowerment->up())
                      t++;
              }
              return t;
          }

      };
      return new infernal_without_de_expr_t( *this );
  }
  else if( name_str == "service_no_de" )
  {
      struct service_without_de_expr_t: public expr_t
      {
          warlock_t& player;

          service_without_de_expr_t( warlock_t& p ):
            expr_t( "service_no_de" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 0;
              for(auto& pet : player.pet_list)
              {
                  pets::warlock_pet_t *lock_pet = dynamic_cast<pets::warlock_pet_t*> ( pet );
                  if(lock_pet != nullptr)
                  {
                      if(lock_pet->is_grimoire_of_service)
                      {
                          if(!lock_pet->is_sleeping() & !lock_pet->buffs.demonic_empowerment->up())
                              t++;
                      }
                  }
              }
              return t;
          }

      };
      return new service_without_de_expr_t( *this );
  }

  else if( name_str == "wild_imp_de_duration" )
  {
      struct wild_imp_de_duration_expression_t: public expr_t
      {
          warlock_t& player;

          wild_imp_de_duration_expression_t( warlock_t& p ):
            expr_t( "wild_imp_de_duration" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 150000;
              for(auto& pet : player.warlock_pet_list.wild_imps)
              {
                  if(!pet->is_sleeping() & !pet->buffs.demonic_empowerment->up())
                  {
                    if(pet->buffs.demonic_empowerment->buff_duration.total_seconds() < t)
                        t = pet->buffs.demonic_empowerment->buff_duration.total_seconds();
                  }
              }
              return t;
          }

      };
      return new wild_imp_de_duration_expression_t( *this );
  }

  else if ( name_str == "darkglare_de_duration" )
  {
    struct darkglare_de_duration_expression_t : public expr_t
    {
      warlock_t& player;

      darkglare_de_duration_expression_t( warlock_t& p ) :
        expr_t( "darkglare_de_duration" ), player( p ) { }
      virtual double evaluate() override
      {
        double t = 150000;
        for ( auto& pet : player.warlock_pet_list.darkglare )
        {
          if ( !pet -> is_sleeping() & !pet -> buffs.demonic_empowerment -> up() )
          {
            if ( pet -> buffs.demonic_empowerment -> buff_duration.total_seconds() < t )
              t = pet -> buffs.demonic_empowerment -> buff_duration.total_seconds();
          }
        }
        return t;
      }

    };
    return new darkglare_de_duration_expression_t( *this );
  }

  else if( name_str == "dreadstalkers_de_duration" )
  {
      struct dreadstalkers_de_duration_expression_t: public expr_t
      {
          warlock_t& player;

          dreadstalkers_de_duration_expression_t( warlock_t& p ):
            expr_t( "dreadstalkers_de_duration" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 150000;
              for(auto& pet : player.warlock_pet_list.dreadstalkers)
              {
                  if(!pet->is_sleeping() & !pet->buffs.demonic_empowerment->up())
                  {
                    if(pet->buffs.demonic_empowerment->buff_duration.total_seconds() < t)
                        t = pet->buffs.demonic_empowerment->buff_duration.total_seconds();
                  }
              }
              return t;
          }

      };
      return new dreadstalkers_de_duration_expression_t( *this );
  }
  else if( name_str == "infernal_de_duration" )
      {
          struct infernal_de_duration_expression_t: public expr_t
          {
              warlock_t& player;

              infernal_de_duration_expression_t( warlock_t& p ):
                expr_t( "infernal_de_duration" ), player( p ) { }
              virtual double evaluate() override
              {
                  double t = 150000;
                  for(auto& pet : player.warlock_pet_list.infernal)
                  {
                      if(!pet->is_sleeping() & !pet->buffs.demonic_empowerment->up())
                      {
                        if(pet->buffs.demonic_empowerment->buff_duration.total_seconds() < t)
                            t = pet->buffs.demonic_empowerment->buff_duration.total_seconds();
                      }
                  }
                  return t;
              }

          };
          return new infernal_de_duration_expression_t( *this );
      }
  else if( name_str == "doomguard_de_duration" )
      {
          struct doomguard_de_duration_expression_t: public expr_t
          {
              warlock_t& player;

              doomguard_de_duration_expression_t( warlock_t& p ):
                expr_t( "doomguard_de_duration" ), player( p ) { }
              virtual double evaluate() override
              {
                  double t = 150000;
                  for(auto& pet : player.warlock_pet_list.doomguard)
                  {
                      if(!pet->is_sleeping() & !pet->buffs.demonic_empowerment->up())
                      {
                        if(pet->buffs.demonic_empowerment->buff_duration.total_seconds() < t)
                            t = pet->buffs.demonic_empowerment->buff_duration.total_seconds();
                      }
                  }
                  return t;
              }

          };
          return new doomguard_de_duration_expression_t( *this );
      }
  else if( name_str == "service_de_duration" )
  {
      struct service_de_duration: public expr_t
      {
          warlock_t& player;

          service_de_duration( warlock_t& p ):
            expr_t( "service_de_duration" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = 500000;
              for(auto& pet : player.pet_list)
              {
                  pets::warlock_pet_t *lock_pet = dynamic_cast<pets::warlock_pet_t*> ( pet );
                  if(lock_pet != nullptr)
                  {
                      if(lock_pet->is_grimoire_of_service)
                      {
                          if(!lock_pet->is_sleeping() & !lock_pet->buffs.demonic_empowerment->up())
                          {
                              if(lock_pet->buffs.demonic_empowerment->buff_duration.total_seconds() )
                                  t = lock_pet->buffs.demonic_empowerment->buff_duration.total_seconds();
                          }
                      }
                  }
              }
              return t;
          }

      };
      return new service_de_duration( *this );
  }

  else if( name_str == "wild_imp_remaining_duration" )
  {
    struct wild_imp_remaining_duration_expression_t: public expr_t
    {
      warlock_t& player;

      wild_imp_remaining_duration_expression_t( warlock_t& p ):
        expr_t( "wild_imp_remaining_duration" ), player( p ) { }
      virtual double evaluate() override
      {
        double t = 5000;
        for( auto& pet : player.warlock_pet_list.wild_imps )
        {
          if( !pet -> is_sleeping() )
          {
            if( t > pet -> expiration->remains().total_seconds() )
            {
              t = pet->expiration->remains().total_seconds();
            }
          }
        }
        if( t == 5000 )
        {
          t = -1;
        }
        return t;
      }
    };

    return new wild_imp_remaining_duration_expression_t( *this );
  }

  else if ( name_str == "darkglare_remaining_duration" )
  {
    struct darkglare_remaining_duration_expression_t : public expr_t
    {
      warlock_t& player;

      darkglare_remaining_duration_expression_t( warlock_t& p ) :
        expr_t( "darkglare_remaining_duration" ), player( p ) { }
      virtual double evaluate() override
      {
        double t = 5000;
        for ( auto& pet : player.warlock_pet_list.darkglare )
        {
          if ( !pet -> is_sleeping() )
          {
            if ( t > pet -> expiration -> remains().total_seconds() )
            {
              t = pet -> expiration -> remains().total_seconds();
            }
          }
        }
        if ( t == 5000 )
          t = -1;
        return t;
      }
    };

    return new darkglare_remaining_duration_expression_t( *this );
  }

  else if( name_str == "dreadstalker_remaining_duration" )
      {
          struct dreadstalker_remaining_duration_expression_t: public expr_t
          {
              warlock_t& player;

              dreadstalker_remaining_duration_expression_t( warlock_t& p ):
                expr_t( "dreadstalker_remaining_duration" ), player( p ) { }
              virtual double evaluate() override
              {
                  double t = 5000;
                  for(auto& pet : player.warlock_pet_list.dreadstalkers)
                  {
                      if( !pet->is_sleeping() )
                      {
                          if( t > pet->expiration->remains().total_seconds() )
                          t = pet->expiration->remains().total_seconds();
                      }
                  }
                  if( t==5000 )
                      t = -1;
                  return t;
              }

          };
          return new dreadstalker_remaining_duration_expression_t( *this );
      }
  else if( name_str == "infernal_remaining_duration" )
      {
          struct infernal_remaining_duration_expression_t: public expr_t
          {
              warlock_t& player;

              infernal_remaining_duration_expression_t( warlock_t& p ):
                expr_t( "infernal_remaining_duration" ), player( p ) { }
              virtual double evaluate() override
              {
                  double t = -1;
                  for(auto& pet : player.warlock_pet_list.infernal)
                  {
                      if(!pet->is_sleeping() )
                      {
                          t = pet->expiration->remains().total_seconds();
                      }
                  }
                  return t;
              }

          };
          return new infernal_remaining_duration_expression_t( *this );
      }
  else if( name_str == "doomguard_remaining_duration" )
      {
          struct doomguard_remaining_duration_expression_t: public expr_t
          {
              warlock_t& player;

              doomguard_remaining_duration_expression_t( warlock_t& p ):
                expr_t( "doomguard_remaining_duration" ), player( p ) { }
              virtual double evaluate() override
              {
                  double t = -1;
                  for(auto& pet : player.warlock_pet_list.doomguard)
                  {
                      if(!pet->is_sleeping() )
                      {
                          t = pet->expiration->remains().total_seconds();
                      }
                  }
                  return t;
              }

          };
          return new doomguard_remaining_duration_expression_t( *this );
      }
  else if( name_str == "service_remaining_duration" )
  {
      struct service_remaining_duration_expr_t: public expr_t
      {
          warlock_t& player;

          service_remaining_duration_expr_t( warlock_t& p ):
            expr_t( "service_remaining_duration" ), player( p ) { }
          virtual double evaluate() override
          {
              double t = -1;
              for(auto& pet : player.pet_list)
              {
                  pets::warlock_pet_t *lock_pet = dynamic_cast<pets::warlock_pet_t*> ( pet );
                  if(lock_pet != nullptr)
                  {
                      if(lock_pet->is_grimoire_of_service)
                      {
                          if(!lock_pet->is_sleeping() )
                          {
                              t=lock_pet->expiration->remains().total_seconds();
                          }
                      }
                  }
              }
              return t;
          }

      };
      return new service_remaining_duration_expr_t( *this );
  }

  else
  {
    return player_t::create_expression( a, name_str );
  }
}

void warlock_t::trigger_lof_infernal()
{
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class warlock_report_t: public player_report_extension_t
{
public:
  warlock_report_t( warlock_t& player ):
    p( player )
  {

  }

  virtual void html_customsection( report::sc_html_stream& /* os*/ ) override
  {
    (void)p;
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
    << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
    << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";*/
  }
private:
  warlock_t& p;
};

using namespace unique_gear;
using namespace actions;

struct power_cord_of_lethtendris_t : public scoped_actor_callback_t<warlock_t>
{
    power_cord_of_lethtendris_t() : super( WARLOCK )
    {}

    void manipulate (warlock_t* p, const special_effect_t& e) override
    {
        p -> legendary.power_cord_of_lethtendris_chance = e.driver() -> effectN( 1 ).percent();
    }
};
/*
struct reap_and_sow_t : public scoped_action_callback_t<reap_souls_t>
{
    reap_and_sow_t() : super ( WARLOCK, "reap_souls" )
    {}

    void manipulate (reap_souls_t* a, const special_effect_t& e) override
    {
        a->reap_and_sow_bonus = timespan_t::from_millis(e.driver()->effectN(1).base_value());
    }
};
*/

struct wakeners_loyalty_t : public scoped_actor_callback_t<warlock_t>
{
    wakeners_loyalty_t() : super ( WARLOCK ){}

    void manipulate ( warlock_t* p, const special_effect_t& ) override
    {
        p -> legendary.wakeners_loyalty_enabled = true;
    }
};
/*
struct hood_of_eternal_disdain_t : public scoped_action_callback_t<agony_t>
{
  hood_of_eternal_disdain_t() : super( WARLOCK, "agony" )
  {}

  void manipulate( agony_t* a, const special_effect_t& e ) override
  {
    a -> base_tick_time *= 1.0 + e.driver() -> effectN( 2 ).percent();
    a -> dot_duration *= 1.0 + e.driver() -> effectN( 1 ).percent();
  }
};

struct sacrolashs_dark_strike_t : public scoped_action_callback_t<corruption_t>
{
  sacrolashs_dark_strike_t() : super( WARLOCK, "corruption" )
  {}

  void manipulate( corruption_t* a, const special_effect_t& e ) override
  {
    a -> base_multiplier *= 1.0 + e.driver() -> effectN( 1 ).percent();
  }
};
*/
struct kazzaks_final_curse_t : public scoped_action_callback_t<doom_t>
{
  kazzaks_final_curse_t() : super( WARLOCK, "doom" )
  {}

  void manipulate( doom_t* a, const special_effect_t& e ) override
  {
    a -> kazzaks_final_curse_multiplier = e.driver() -> effectN( 1 ).percent();
  }
};

struct recurrent_ritual_t : public scoped_action_callback_t<call_dreadstalkers_t>
{
  recurrent_ritual_t() : super( WARLOCK, "call_dreadstalkers" )
  { }

  void manipulate( call_dreadstalkers_t* a, const special_effect_t& e ) override
  {
    a -> recurrent_ritual = e.driver() -> effectN( 1 ).trigger() -> effectN( 1 ).base_value() / 10.0;
  }
};

struct wilfreds_sigil_of_superior_summoning_t : public scoped_actor_callback_t<warlock_t>
{
  wilfreds_sigil_of_superior_summoning_t() : super( WARLOCK )
  {}

  void manipulate( warlock_t* p, const special_effect_t& e ) override
  {
    p -> legendary.wilfreds_sigil_of_superior_summoning_flag = true;
    p -> legendary.wilfreds_sigil_of_superior_summoning = e.driver() -> effectN( 1 ).time_value();
  }
};

struct sindorei_spite_t : public class_buff_cb_t<warlock_t>
{
  sindorei_spite_t() : super( WARLOCK, "sindorei_spite" )
  {}

  buff_t*& buff_ptr( const special_effect_t& e ) override
  {
    return actor( e ) -> buffs.sindorei_spite;
  }

  buff_creator_t creator( const special_effect_t& e ) const override
  {
    return super::creator( e )
      .spell( e.driver() -> effectN( 1 ).trigger() )
      .default_value( e.driver() -> effectN( 1 ).trigger() -> effectN( 1 ).percent() )
      .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }
};

struct lessons_of_spacetime_t : public scoped_actor_callback_t<warlock_t>
{
  lessons_of_spacetime_t() : super( WARLOCK ){}

  void manipulate( warlock_t* p, const special_effect_t& ) override
  {
    //const spell_data_t * tmp = p -> find_spell( 236176 );
    p -> legendary.lessons_of_spacetime = true;
    p -> legendary.lessons_of_spacetime1 = timespan_t::from_seconds( 5 );
    p -> legendary.lessons_of_spacetime2 = timespan_t::from_seconds( 9 );
    p -> legendary.lessons_of_spacetime3 = timespan_t::from_seconds( 16 );
  }
};

struct odr_shawl_of_the_ymirjar_t : public scoped_actor_callback_t<warlock_t>
{
  odr_shawl_of_the_ymirjar_t() : super( WARLOCK_DESTRUCTION )
  { }

  void manipulate( warlock_t* a, const special_effect_t&  ) override
  {
    a -> legendary.odr_shawl_of_the_ymirjar = true;
  }
};

struct stretens_insanity_t: public scoped_actor_callback_t<warlock_t>
{
  stretens_insanity_t(): super( WARLOCK_AFFLICTION )
  {}

  void manipulate( warlock_t* a, const special_effect_t&  ) override
  {
    a -> legendary.stretens_insanity = true;
  }
};

struct feretory_of_souls_t : public scoped_actor_callback_t<warlock_t>
{
  feretory_of_souls_t() : super( WARLOCK )
  { }

  void manipulate( warlock_t* a, const special_effect_t& ) override
  {
    a -> legendary.feretory_of_souls = true;
  }
};

struct sephuzs_secret_t : public scoped_actor_callback_t<warlock_t>
{
  sephuzs_secret_t() : super( WARLOCK ){}

  void manipulate( warlock_t* a, const special_effect_t& e ) override
  {
    a -> legendary.sephuzs_secret = true;
    a -> legendary.sephuzs_passive = e.driver() -> effectN( 3 ).percent();
  }
};

struct magistrike_t : public scoped_actor_callback_t<warlock_t>
{
  magistrike_t() : super( WARLOCK ) {}

  void manipulate( warlock_t* a, const special_effect_t& ) override
  {
    a -> legendary.magistrike = true;
  }
};

struct the_master_harvester_t : public scoped_actor_callback_t<warlock_t>
{
  the_master_harvester_t() : super( WARLOCK ){}

  void manipulate( warlock_t* a, const special_effect_t& /* e */ ) override
  {
    a -> legendary.the_master_harvester = true;
  }
};

struct alythesss_pyrogenics_t : public scoped_actor_callback_t<warlock_t>
{
  alythesss_pyrogenics_t() : super( WARLOCK ){}

  void manipulate( warlock_t* a, const special_effect_t& /* e */ ) override
  {
    a -> legendary.alythesss_pyrogenics = true;
  }
};

struct warlock_module_t: public module_t
{
  warlock_module_t(): module_t( WARLOCK ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto  p = new warlock_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new warlock_report_t( *p ) );
    return p;
  }

  virtual void static_init() const override {
    // Legendaries

    //register_special_effect( 205797, hood_of_eternal_disdain_t() );
    register_special_effect( 214225, kazzaks_final_curse_t() );
    register_special_effect( 205721, recurrent_ritual_t() );
    register_special_effect( 214345, wilfreds_sigil_of_superior_summoning_t() );
    register_special_effect( 208868, sindorei_spite_t(), true );
    register_special_effect( 212172, odr_shawl_of_the_ymirjar_t() );
    register_special_effect( 205702, feretory_of_souls_t() );
    register_special_effect( 208821, stretens_insanity_t() );
    register_special_effect( 205753, power_cord_of_lethtendris_t() );
    //register_special_effect( 236114, reap_and_sow_t() );
    register_special_effect( 236199, wakeners_loyalty_t() );
    register_special_effect( 236174, lessons_of_spacetime_t() );
    register_special_effect( 208051, sephuzs_secret_t() );
    //register_special_effect( 207952, sacrolashs_dark_strike_t() );
    register_special_effect( 213014, magistrike_t() );
    register_special_effect( 248113, the_master_harvester_t() );
    register_special_effect( 205678, alythesss_pyrogenics_t() );
  }

  virtual void register_hotfixes() const override
  {
    //hotfix::register_effect( "Warlock", "2017-12-04", "Diabolic Raiment 4-piece – Spending a Soul Shard now has a chance to grant you 10% Haste (was 15%).", 367884 )
    //  .field( "base_value" )
    //  .operation( hotfix::HOTFIX_SET )
    //  .modifier( 10 )
    //  .verification_value( 15 );

    //hotfix::register_effect( "Warlock", "2017-12-04", "Grim Inquisitor’s Regalia 4-piece – Unstable Affliction and Seed of Corruption now increase the damage targets take from your Agony and Corruption by 15% (was 10%).", 473114 )
    //  .field( "base_value" )
    //  .operation( hotfix::HOTFIX_SET )
    //  .modifier( 15 )
    //  .verification_value( 10 );

    //hotfix::register_effect( "Warlock", "2017-12-04", "Grim Inquisitor’s Regalia 4-piece – Demonic Empowerment now causes your Dreadstalkers to immediately cast another Dreadbite with 75% increased damage (was 50%).", 471461 )
    //  .field( "base_value" )
    //  .operation( hotfix::HOTFIX_SET )
    //  .modifier( 75 )
    //  .verification_value( 50 );

    //hotfix::register_effect( "Warlock", "2018-02-05", "Corruption damage reduced by 6%.", 198369 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 0.94 )
    //  .verification_value( 0.324 );

    //hotfix::register_effect( "Warlock", "2018-02-05", "Agony damage reduced by 6%.", 374 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 0.94 )
    //  .verification_value( 0.03540 );

    //hotfix::register_effect( "Warlock", "2017-12-04", "Unstable Affliction 1 damage reduced by 4%", 352664 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 0.96 )
    //  .verification_value( 0.966 );

    //hotfix::register_effect( "Warlock", "2017-12-04", "Unstable Affliction 2 damage reduced by 4%", 352671 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 0.96 )
    //  .verification_value( 0.966 );

    //hotfix::register_effect( "Warlock", "2017-12-04", "Unstable Affliction 3 damage reduced by 4%", 352672 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 0.96 )
    //  .verification_value( 0.966 );

    //hotfix::register_effect( "Warlock", "2017-12-04", "Unstable Affliction 4 damage reduced by 4%", 352673 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 0.96 )
    //  .verification_value( 0.966 );

    //hotfix::register_effect( "Warlock", "2017-12-04", "Unstable Affliction 5 damage reduced by 4%", 352674 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 0.96 )
    //  .verification_value( 0.966 );


    //hotfix::register_effect( "Warlock", "2016-09-23", "Drain Life damage increased by 10%", 271 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.10 )
    //  .verification_value( 0.35 );

    //hotfix::register_effect( "Warlock", "2016-09-23", "Drain Soul damage increased by 10%", 291909 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.10 )
    //  .verification_value( 0.52 );

    //hotfix::register_effect( "Warlock", "2016-09-23", "Seed of Corruption damage increased by 15%", 16922 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.15 )
    //  .verification_value( 1.2 );

    //hotfix::register_effect( "Warlock", "2016-09-23", "Siphon Life damage increased by 10%", 57197 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.1 )
    //  .verification_value( 0.5 );

    //hotfix::register_effect( "Warlock", "2016-09-23", "Haunt damage increased by 15%", 40331 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.15 )
    //  .verification_value( 7.0 );

    //hotfix::register_effect( "Warlock", "2016-09-23", "Phantom Singularity damage increased by 15%", 303063 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.15 )
    //  .verification_value( 1.44 );

    //hotfix::register_effect( "Warlock", "2015-09-23", "Hand of Gul�dan impact damage increased by 20%", 87492 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.2 )
    //  .verification_value( 0.36 );

    //hotfix::register_effect( "Warlock", "2015-09-23", " Demonwrath damage increased by 15%", 283783 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.15 )
    //  .verification_value( 0.3 );

    //hotfix::register_effect( "Warlock", "2015-09-23", "Shadowbolt damage increased by 10%", 267 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.1 )
    //  .verification_value( 0.8 );

    //hotfix::register_effect( "Warlock", "2015-09-23", "Doom damage increased by 10%", 246 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.1 )
    //  .verification_value( 5.0 );

    //hotfix::register_effect( "Warlock", "2015-09-23", "Wild Imps damage increased by 10%", 113740 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.1 )
    //  .verification_value( 0.14 );

    //hotfix::register_effect( "Warlock", "2015-09-23", "Demonbolt (Talent) damage increased by 10%", 219885 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.1 )
    //  .verification_value( 0.8 );

    //hotfix::register_effect( "Warlock", "2015-09-23", "Implosion (Talent) damage increased by 15%", 288085 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.15 )
    //  .verification_value( 2.0 );

    //hotfix::register_effect( "Warlock", "2015-09-23", "Shadowflame (Talent) damage increased by 10%", 302909 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.1 )
    //  .verification_value( 1.0 );

    //hotfix::register_effect( "Warlock", "2015-09-23", "Shadowflame (Talent) damage increased by 10%-2", 302911 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.1 )
    //  .verification_value( 0.35 );

    //hotfix::register_effect( "Warlock", "2015-09-23", "Darkglare (Talent) damage increased by 10%", 302984 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.1 )
    //  .verification_value( 1.0 );

    //hotfix::register_effect( "Warlock", "2016-09-23", "Chaos bolt damage increased by 11%", 132079 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.11 )
    //  .verification_value( 3.3 );

    //hotfix::register_effect( "Warlock", "2016-09-23", "Incinerate damage increased by 11%", 288276 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.11 )
    //  .verification_value( 2.1 );

    //hotfix::register_effect( "Warlock", "2016-09-23", "Immolate damage increased by 11%", 145 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.11 )
    //  .verification_value( 1.2 );

    //hotfix::register_effect( "Warlock", "2016-09-23", "Conflagrate damage increased by 11%", 9553 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.11 )
    //  .verification_value( 2.041 );

    //hotfix::register_effect( "Warlock", "2016-09-23", "Rain of Fire damage increased by 11%, and cast time removed", 33883 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.11 )
    //  .verification_value( 0.5 );

    //hotfix::register_effect( "Warlock", "2016-09-23", "Cataclysm damage increased by 11%", 210584 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.11 )
    //  .verification_value( 7.0 );

    //hotfix::register_effect( "Warlock", "2016-09-23", "Channel Demonfire damage increased by 11%", 288343 )
    //  .field( "sp_coefficient" )
    //  .operation( hotfix::HOTFIX_MUL )
    //  .modifier( 1.11 )
    //  .verification_value( 0.42 );
    //

  }

  virtual bool valid() const override { return true; }
  virtual void init( player_t* ) const override {}
  virtual void combat_begin( sim_t* ) const override {}
  virtual void combat_end( sim_t* ) const override {}
};

}

const module_t* module_t::warlock()
{
  static warlock::warlock_module_t m;
  return &m;
}
