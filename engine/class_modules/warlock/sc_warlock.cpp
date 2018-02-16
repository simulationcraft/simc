#include "simulationcraft.hpp"
#include "sc_warlock.hpp"

namespace warlock { 
#define MAX_UAS 5
// Pets
namespace pets {
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

  if ( specialization() == WARLOCK_DESTRUCTION && dbc::is_school( school, SCHOOL_FIRE ) )
  {
    m *= 1.0 + buffs.alythesss_pyrogenics -> stack_value();
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
  if ( action_name == "channel_demonfire"     ) return new                 channel_demonfire_t( this );
  if ( action_name == "soul_harvest"          ) return new                      soul_harvest_t( this );
  if ( action_name == "immolate"              ) return new                          immolate_t( this );
  if ( action_name == "incinerate"            ) return new                        incinerate_t( this );
  if ( action_name == "mortal_coil"           ) return new                       mortal_coil_t( this );
  if ( action_name == "shadow_bolt"           ) return new                       shadow_bolt_t( this );
  if ( action_name == "shadowburn"            ) return new                        shadowburn_t( this );
  if ( action_name == "havoc"                 ) return new                             havoc_t( this );
  if ( action_name == "cataclysm"             ) return new                         cataclysm_t( this );
  if ( action_name == "rain_of_fire"          ) return new         rain_of_fire_t( this, options_str );
  if ( action_name == "demonwrath"            ) return new                        demonwrath_t( this );
  if ( action_name == "shadowflame"           ) return new                       shadowflame_t( this );
  if ( action_name == "summon_felhunter"      ) return new      summon_main_pet_t( "felhunter", this );
  if ( action_name == "summon_felguard"       ) return new       summon_main_pet_t( "felguard", this );
  if ( action_name == "summon_succubus"       ) return new       summon_main_pet_t( "succubus", this );
  if ( action_name == "summon_voidwalker"     ) return new     summon_main_pet_t( "voidwalker", this );
  if ( action_name == "summon_imp"            ) return new            summon_main_pet_t( "imp", this );
  if ( action_name == "summon_pet"            ) return new      summon_main_pet_t( default_pet, this );

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

  return nullptr;
}

void warlock_t::create_pets()
{
  for ( size_t i = 0; i < pet_name_list.size(); ++i )
  {
    create_pet( pet_name_list[ i ] );
  }

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
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
  talents.power_trip			 = find_talent_spell( "Power Trip" );

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
  buffs.demonic_speed = haste_buff_creator_t( this, "demonic_speed", sets -> set( WARLOCK_AFFLICTION, T20, B4 ) -> effectN( 1 ).trigger() )
    .chance( sets -> set( WARLOCK_AFFLICTION, T20, B4 ) -> proc_chance() )
    .default_value( sets -> set( WARLOCK_AFFLICTION, T20, B4 ) -> effectN( 1 ).trigger() -> effectN( 1 ).percent() );

  //demonology buffs
  buffs.demonic_synergy = buff_creator_t( this, "demonic_synergy", find_spell( 171982 ) )
    .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
    .chance( 1 );
  buffs.shadowy_inspiration = buff_creator_t( this, "shadowy_inspiration", find_spell( 196606 ) );
  buffs.demonic_calling = buff_creator_t( this, "demonic_calling", talents.demonic_calling -> effectN( 1 ).trigger() );
  buffs.dreaded_haste = haste_buff_creator_t( this, "dreaded_haste", sets -> set( WARLOCK_DEMONOLOGY, T20, B4 ) -> effectN( 1 ).trigger() )
    .default_value( sets -> set( WARLOCK_DEMONOLOGY, T20, B4 ) -> effectN( 1 ).trigger() -> effectN( 1 ).percent() );
  buffs.rage_of_guldan = buff_creator_t(this, "rage_of_guldan", sets->set( WARLOCK_DEMONOLOGY, T21, B2 ) -> effectN( 1 ).trigger() )
	  .duration( find_spell( 257926 ) -> duration() )
	  .max_stack( find_spell( 257926 ) -> max_stacks() )
	  .default_value( find_spell( 257926 ) -> effectN( 1 ).base_value() )
	  .refresh_behavior( BUFF_REFRESH_DURATION );

  //destruction buffs
  buffs.backdraft = buff_creator_t( this, "backdraft", find_spell( 117828 ) );
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

  procs.t18_4pc_destruction = get_proc( "t18_4pc_destruction" );
  procs.t18_prince_malchezaar = get_proc( "t18_prince_malchezaar" );
  procs.t18_vicious_hellhound = get_proc( "t18_vicious_hellhound" );
  procs.t18_illidari_satyr = get_proc( "t18_illidari_satyr" );
  procs.one_shard_hog = get_proc( "one_shard_hog" );
  procs.two_shard_hog = get_proc( "two_shard_hog" );
  procs.three_shard_hog = get_proc( "three_shard_hog" );
  procs.four_shard_hog = get_proc( "four_shard_hog" );
  procs.impending_doom = get_proc( "impending_doom" );
  procs.demonic_calling = get_proc( "demonic_calling" );
  procs.power_trip = get_proc( "power_trip" );
  procs.soul_conduit = get_proc( "soul_conduit" );
  procs.the_master_harvester = get_proc( "the_master_harvester" );
  procs.souls_consumed = get_proc( "souls_consumed" );
  procs.t19_2pc_chaos_bolts = get_proc( "t19_2pc_chaos_bolt" );
  procs.demonology_t20_2pc = get_proc( "demonology_t20_2pc" );
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
}

void warlock_t::apl_default()
{
}

void warlock_t::apl_affliction()
{
    create_apl_affliction();
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
  else if (name_str == "pet_count")
  {
      struct pet_count_expr_t : public expr_t
      {
          warlock_t& player;

          pet_count_expr_t(warlock_t& p) :
              expr_t("pet_count"), player(p) { }
          virtual double evaluate() override
          {
              double t = 0;
              for (auto& pet : player.pet_list)
              {
                  pets::warlock_pet_t *lock_pet = dynamic_cast<pets::warlock_pet_t*> (pet);
                  if (lock_pet != nullptr)
                  {
                      if (!lock_pet->is_sleeping())
                      {
                          t++;
                      }
                  }
              }
              return t;
          }

      };
      return new pet_count_expr_t(*this);
  }
  else
  {
    return player_t::create_expression( a, name_str );
  }
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

struct wilfreds_sigil_of_superior_summoning_t : public scoped_actor_callback_t<warlock_t>
{
  wilfreds_sigil_of_superior_summoning_t() : super( WARLOCK )
  {}

  void manipulate( warlock_t* p, const special_effect_t& e ) override
  {

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
    //register_special_effect( 205721, recurrent_ritual_t() );
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
