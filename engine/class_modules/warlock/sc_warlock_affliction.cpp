#include "simulationcraft.hpp"

#include "sc_warlock.hpp"

namespace warlock
{
namespace actions_affliction
{
using namespace actions;

struct affliction_spell_t : public warlock_spell_t
{
public:
  gain_t* gain;

  affliction_spell_t( warlock_t* p, util::string_view n ) : affliction_spell_t( n, p, p->find_class_spell( n ) )
  {
  }

  affliction_spell_t( warlock_t* p, util::string_view n, specialization_e s )
    : affliction_spell_t( n, p, p->find_class_spell( n, s ) )
  {
  }

  affliction_spell_t( util::string_view token, warlock_t* p, const spell_data_t* s = spell_data_t::nil() )
    : warlock_spell_t( token, p, s )
  {
    may_crit          = true;
    tick_may_crit     = true;
    weapon_multiplier = 0.0;
    gain              = player->get_gain( name_str );
  }

  void init() override
  {
    warlock_spell_t::init();

    if ( p()->talents.creeping_death->ok() )
    {
      if ( data().affected_by( p()->talents.creeping_death->effectN( 1 ) ) )
        base_tick_time *= 1.0 + p()->talents.creeping_death->effectN( 1 ).percent();
      if ( data().affected_by( p()->talents.creeping_death->effectN( 2 ) ) )
        dot_duration *= 1.0 + p()->talents.creeping_death->effectN( 2 ).percent();
    }
  }

  double action_multiplier() const override
  {
    double pm = warlock_spell_t::action_multiplier();

    return pm;
  }

  // direct action multiplier
  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double pm = warlock_spell_t::composite_da_multiplier( s );
        
    if ( this->data().affected_by( p()->mastery_spells.potent_afflictions->effectN( 2 ) ) )
    {
      pm *= 1.0 + p()->cache.mastery_value();
    }
    return pm;
  }

  // tick action multiplier
  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double pm = warlock_spell_t::composite_ta_multiplier( s );

    if ( this->data().affected_by( p()->mastery_spells.potent_afflictions->effectN( 1 ) ) )
    {
      pm *= 1.0 + p()->cache.mastery_value();
    }

    return pm;
  }
};

struct shadow_bolt_t : public affliction_spell_t
{
  shadow_bolt_t( warlock_t* p, const std::string& options_str )
    : affliction_spell_t( "Shadow Bolt", p, p->find_spell( 686 ) )
  {
    parse_options( options_str );
  }

  timespan_t execute_time() const override
  {
    if ( p()->buffs.nightfall->check() )
    {
      return 0_ms;
    }

    return affliction_spell_t::execute_time();
  }

  bool ready() override
  {
    if ( p()->talents.drain_soul->ok() )
      return false;

    return affliction_spell_t::ready();
  }

  void impact( action_state_t* s ) override
  {
    affliction_spell_t::impact( s );
    if ( result_is_hit( s->result ) )
    {
      // Add passive check
      td( s->target )->debuffs_shadow_embrace->trigger();
    }
  }

  double action_multiplier() const override
  {
    double m = affliction_spell_t::action_multiplier();

    if ( time_to_execute == 0_ms && p()->buffs.nightfall->check() )
      m *= 1.0 + p()->buffs.nightfall->default_value;

    m *= 1 + p()->buffs.decimating_bolt->check_value();

    return m;
  }

  void schedule_execute( action_state_t* s ) override
  {
    affliction_spell_t::schedule_execute( s );
  }

  void execute() override
  {
    affliction_spell_t::execute();
    if ( time_to_execute == 0_ms )
      p()->buffs.nightfall->decrement();

    p()->buffs.decimating_bolt->decrement();
  }
};

// Dots
struct agony_t : public affliction_spell_t
{
  double chance;
  bool pandemic_invocation_usable;  // BFA - Azerite

  agony_t( warlock_t* p, util::string_view options_str ) : affliction_spell_t( "Agony", p, p->spec.agony )
  {
    parse_options( options_str );
    may_crit                   = false;
    pandemic_invocation_usable = false;  // BFA - Azerite

    dot_max_stack = as<int>( data().max_stacks() + p->spec.agony_2->effectN( 1 ).base_value() );
    dot_duration += p->conduit.rolling_agony.time_value();
  }

  void last_tick( dot_t* d ) override
  {
    if ( p()->get_active_dots( internal_id ) == 1 )
    {
      p()->agony_accumulator = rng().range( 0.0, 0.99 );
    }

    affliction_spell_t::last_tick( d );
  }

  void init() override
  {
    dot_max_stack +=
        as<int>( p()->talents.writhe_in_agony->ok() ? p()->talents.writhe_in_agony->effectN( 1 ).base_value() : 0 );

    affliction_spell_t::init();
  }

  void execute() override
  {
    // BFA - Azerite
    // Do checks for Pandemic Invocation before parent execute() is called so we get the correct DoT states.
    if ( p()->azerite.pandemic_invocation.ok() && td( target )->dots_agony->is_ticking() &&
         td( target )->dots_agony->remains() <= p()->azerite.pandemic_invocation.spell_ref().effectN( 2 ).time_value() )
      pandemic_invocation_usable = true;

    affliction_spell_t::execute();

    if ( pandemic_invocation_usable )
    {
      p()->active.pandemic_invocation->schedule_execute();
      pandemic_invocation_usable = false;
    }

    //There is TECHNICALLY a prepatch bug on PTR (as of 9/23) where having both the talent and the azerite starts at 3 stacks
    //Making a note of it here in this comment but not going to implement it at this time
    if ( p()->talents.writhe_in_agony->ok() && td( execute_state->target )->dots_agony->current_stack() < 
      (int)p()->talents.writhe_in_agony->effectN( 3 ).base_value() )
    {
      td ( execute_state->target )
        ->dots_agony->increment( (int)p()->talents.writhe_in_agony->effectN( 3 ).base_value() - 
          td( execute_state->target )->dots_agony->current_stack() );
    }
    else if ( p()->azerite.sudden_onset.ok() && td( execute_state->target )->dots_agony->current_stack() <
                                               (int)p()->azerite.sudden_onset.spell_ref().effectN( 2 ).base_value() )
    {
      td( execute_state->target )
          ->dots_agony->increment( (int)p()->azerite.sudden_onset.spell_ref().effectN( 2 ).base_value() -
                                   td( execute_state->target )->dots_agony->current_stack() );
    }
  }

  double bonus_ta( const action_state_t* s ) const override
  {
    double ta = affliction_spell_t::bonus_ta( s );
    // BFA - Azerite
    // TOCHECK: How does Sudden Onset behave with Writhe in Agony's increased stack cap?
    ta += p()->azerite.sudden_onset.value();
    return ta;
  }

  void tick( dot_t* d ) override
  {
    td( d->state->target )->dots_agony->increment( 1 );

    // Blizzard has not publicly released the formula for Agony's chance to generate a Soul Shard.
    // This set of code is based on results from 500+ Soul Shard sample sizes, and matches in-game
    // results to within 0.1% of accuracy in all tests conducted on all targets numbers up to 8.
    // Accurate as of 08-24-2018. TOCHECK regularly. If any changes are made to this section of
    // code, please also update the Time_to_Shard expression in sc_warlock.cpp.
    double increment_max = 0.368;

    double active_agonies = p()->get_active_dots( internal_id );
    increment_max *= std::pow( active_agonies, -2.0 / 3.0 );

    if ( p()->talents.creeping_death->ok() )
    {
      increment_max *= 1.0 + p()->talents.creeping_death->effectN( 1 ).percent();
    }

    if ( p()->legendary.perpetual_agony_of_azjaqir->ok() )
      increment_max *= 1.0 + p()->legendary.perpetual_agony_of_azjaqir->effectN( 1 ).percent();

    p()->agony_accumulator += rng().range( 0.0, increment_max );

    if ( p()->agony_accumulator >= 1 )
    {
      // BFA - Azerite
      if ( p()->azerite.wracking_brilliance.ok() )
      {
        if ( p()->wracking_brilliance )
        {
          p()->wracking_brilliance = false;
          p()->buffs.wracking_brilliance->trigger();
        }
        else
        {
          p()->wracking_brilliance = true;
        }
      }

      p()->resource_gain( RESOURCE_SOUL_SHARD, 1.0, p()->gains.agony );
      p()->agony_accumulator -= 1.0;
    }

    if ( result_is_hit( d->state->result ) && p()->talents.inevitable_demise->ok() && !p()->buffs.drain_life->check() )
    {
      p()->buffs.inevitable_demise->trigger();
    }

    p()->malignancy_reduction_helper();

    affliction_spell_t::tick( d );
  }
};

struct corruption_t : public affliction_spell_t
{
  bool pandemic_invocation_usable;

  corruption_t( warlock_t* p, util::string_view options_str )
    : affliction_spell_t( "corruption", p, p->find_spell( 172 ) )   // 172 triggers 146739
  {
    auto otherSP = p->find_spell( 146739 );
    parse_options( options_str );
    may_crit                   = false;
    tick_zero                  = false;
    pandemic_invocation_usable = false;  // BFA - Azerite

    
    if ( !p->spec.corruption_3->ok() )
    {
      spell_power_mod.direct = 0; //Rank 3 is required for direct damage
    }
    

    spell_power_mod.tick       = data().effectN( 1 ).trigger()->effectN( 1 ).sp_coeff();
    base_tick_time             = data().effectN( 1 ).trigger()->effectN( 1 ).period();

    // 172 does not have spell duration any more.
    // were still lazy though so we aren't making a seperate spell for this.
    dot_duration               = otherSP->duration();
    // TOCHECK see if we can redo corruption in a way that spec aura applies to corruption naturally in init.
    base_multiplier *= 1.0 + p->spec.affliction->effectN( 2 ).percent();

    if ( p->talents.absolute_corruption->ok() )
    {
      dot_duration = sim->expected_iteration_time > 0_ms
                         ? 2 * sim->expected_iteration_time
                         : 2 * sim->max_time * ( 1.0 + sim->vary_combat_length );  // "infinite" duration
      base_multiplier *= 1.0 + p->talents.absolute_corruption->effectN( 2 ).percent();
    }

    if ( p->spec.corruption_2->ok())
    {
      base_execute_time *= 1.0 * p->spec.corruption_2->effectN( 1 ).percent();
    }
    
    affected_by_woc = true; //Hardcoding this in for now because of how this spell is hacked together!
  }

  void tick( dot_t* d ) override
  {
    if ( result_is_hit( d->state->result ) && p()->talents.nightfall->ok() )
    {
      // TOCHECK regularly. 
      // Blizzard did not publicly release how nightfall was changed. 
      // We determined this is the probable functionality copied from Agony by first confirming the 
      // DR formula was the same and then confirming that you can get procs on 1st tick.
      // The procs also have a regularity that suggest it does not use a proc chance or rppm. 
      // Last checked 09-28-2020.
      double increment_max = 0.13;

      double active_corruptions = p()->get_active_dots( internal_id );
      increment_max *= std::pow( active_corruptions, -2.0 / 3.0 );

      p()->corruption_accumulator += rng().range( 0.0, increment_max );

      if ( p()->corruption_accumulator >= 1 )
      {
        p()->procs.nightfall->occur();
        p()->buffs.nightfall->trigger();
        p()->corruption_accumulator -= 1.0;

      }
    }
    affliction_spell_t::tick( d );
  }

  void execute() override
  {
    // BFA - Azerite
    // Do checks for Pandemic Invocation before parent execute() is called so we get the correct DoT states.
    if ( p()->azerite.pandemic_invocation.ok() && td( target )->dots_corruption->is_ticking() &&
         td( target )->dots_corruption->remains() <=
             p()->azerite.pandemic_invocation.spell_ref().effectN( 2 ).time_value() )
      pandemic_invocation_usable = true;

    affliction_spell_t::execute();

    if ( pandemic_invocation_usable )
    {
      p()->active.pandemic_invocation->schedule_execute();
      pandemic_invocation_usable = false;
    }


  }

  double composite_ta_multiplier(const action_state_t* s) const override
  {
    double m = affliction_spell_t::composite_ta_multiplier( s );

    // SL - Legendary
    if ( p()->legendary.sacrolashs_dark_strike->ok() )
      m *= 1.0 + p()->legendary.sacrolashs_dark_strike->effectN( 1 ).percent();

    return m;
  }
};

struct unstable_affliction_t : public affliction_spell_t
{
  unstable_affliction_t( warlock_t* p, util::string_view options_str )
    : affliction_spell_t( "unstable_affliction", p, p->spec.unstable_affliction )
  {
    parse_options( options_str );

    if ( p->spec.unstable_affliction_3->ok() )
    {
      dot_duration += timespan_t::from_millis( p->spec.unstable_affliction_3->effectN( 1 ).base_value() );
    }
  }

  void execute() override
  {
    if ( p()->ua_target && p()->ua_target != target )
    {
      td( p()->ua_target )->dots_unstable_affliction->cancel();
    }

    p()->ua_target = target;

    affliction_spell_t::execute();
  }

  void last_tick( dot_t* d) override
  {
    affliction_spell_t::last_tick( d );

    p()->ua_target = nullptr;
  }

  void tick( dot_t* d ) override
  {
    p()->malignancy_reduction_helper();

    affliction_spell_t::tick( d );
  }
};

struct summon_darkglare_t : public affliction_spell_t   
{
  summon_darkglare_t( warlock_t* p, util::string_view options_str )
    : affliction_spell_t( "summon_darkglare", p, p->spec.summon_darkglare )
  {
    parse_options( options_str );
    harmful = may_crit = may_miss = false;
    
    cooldown->duration += timespan_t::from_millis( p->talents.dark_caller->effectN( 1 ).base_value() );

    //PTR for prepatch presumably does additive CDR, then multiplicative
    cooldown->duration *= 1.0 + azerite::vision_of_perfection_cdr( p->azerite_essence.vision_of_perfection );
  }

  void execute() override
  {
    affliction_spell_t::execute();

    for ( auto& darkglare : p()->warlock_pet_list.darkglare )
    {
      if ( darkglare->is_sleeping() )
      {
        darkglare->summon( data().duration() );
      }
    }
    timespan_t darkglare_extension = timespan_t::from_seconds( p()->spec.summon_darkglare->effectN( 2 ).base_value() );

    p()->darkglare_extension_helper( p(), darkglare_extension );
  }
};

// AOE
struct seed_of_corruption_t : public affliction_spell_t
{
  struct seed_of_corruption_aoe_t : public affliction_spell_t
  {
    corruption_t* corruption;

    seed_of_corruption_aoe_t( warlock_t* p )
      : affliction_spell_t( "seed_of_corruption_aoe", p, p->find_spell( 27285 ) ),
        corruption( new corruption_t( p, "" ) )
    {
      aoe                              = -1;
      background                       = true;
      p->spells.seed_of_corruption_aoe = this;
      base_costs[ RESOURCE_MANA ]      = 0;

      corruption->background                  = true;
      corruption->dual                        = true;
      corruption->base_costs[ RESOURCE_MANA ] = 0;
    }

    double bonus_da( const action_state_t* s ) const override
    {
      double da = affliction_spell_t::bonus_da( s );
      return da;
    }

    void impact( action_state_t* s ) override
    {
      affliction_spell_t::impact( s );

      if ( result_is_hit( s->result ) )
      {
        auto tdata = this->td( s->target );
        if ( tdata->dots_seed_of_corruption->is_ticking() && tdata->soc_threshold > 0 )
        {
          tdata->soc_threshold = 0;
          tdata->dots_seed_of_corruption->cancel();
        }

        corruption->set_target( s->target );
        corruption->execute();
      }
    }
  };

  double threshold_mod;
  double sow_the_seeds_targets;
  seed_of_corruption_aoe_t* explosion;

  seed_of_corruption_t( warlock_t* p, util::string_view options_str )
    : affliction_spell_t( "seed_of_corruption", p, p->find_spell( 27243 ) ),
      threshold_mod( 3.0 ),
      sow_the_seeds_targets( p->talents.sow_the_seeds->effectN( 1 ).base_value() ),
      explosion( new seed_of_corruption_aoe_t( p ) )
  {
    parse_options( options_str );
    may_crit       = false;
    tick_zero      = false;
    base_tick_time = dot_duration;
    hasted_ticks   = false;
    add_child( explosion );
    if ( p->talents.sow_the_seeds->ok() )
      aoe = 2;
  }

  void init() override
  {
    affliction_spell_t::init();
    snapshot_flags |= STATE_SP;
  }

  void execute() override
  {
    if ( td( target )->dots_seed_of_corruption->is_ticking() || has_travel_events_for( target ) )
    {
      for ( auto& possible : target_list() )
      {
        if ( !( td( possible )->dots_seed_of_corruption->is_ticking() || has_travel_events_for( possible ) ) )
        {
          set_target( possible );
          break;
        }
      }
    }

    affliction_spell_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    // TOCHECK: Does the threshold reset if the duration is refreshed before explosion?
    if ( result_is_hit( s->result ) )
    {
      td( s->target )->soc_threshold = s->composite_spell_power();
    }

    affliction_spell_t::impact( s );
  }

  // Seed of Corruption is currently bugged on pure single target, extending the duration
  // but still exploding at the original time, wiping the debuff. tick() should be used instead
  // of last_tick() for now to model this more appropriately. TOCHECK regularly
  void tick( dot_t* d ) override
  {
    affliction_spell_t::tick( d );

    if ( d->remains() > 0_ms )
      d->cancel();
  }

  void last_tick( dot_t* d ) override
  {
    explosion->set_target( d->target );
    explosion->schedule_execute();

    affliction_spell_t::last_tick( d );
  }
};

struct malefic_rapture_t : public affliction_spell_t
{
    struct malefic_rapture_damage_instance_t : public affliction_spell_t
    {

      malefic_rapture_damage_instance_t(warlock_t *p) : 
          affliction_spell_t( "malefic_rapture_aoe", p, p->find_spell( 324536 ) )
      {
        aoe = 1;
        background = true;
        spell_power_mod.direct = data().effectN( 1 ).sp_coeff();
        base_costs[ RESOURCE_SOUL_SHARD ] = 0;

        p->spells.malefic_rapture_aoe = this;
      }
       
      double get_dots_ticking(player_t *target) const
      {
        double mult = 0.0;
        auto td = this->td( target );
        if ( td->dots_agony->is_ticking() )
          mult += 1.0;

        if ( td->dots_corruption->is_ticking() )
          mult += 1.0;

        if ( td->dots_unstable_affliction->is_ticking() )
          mult += 1.0;

        if ( td->dots_vile_taint->is_ticking() )
          mult += 1.0;

        if ( td->dots_phantom_singularity->is_ticking() )
          mult += 1.0;

        if ( td->dots_siphon_life->is_ticking() )
          mult += 1.0;

        if ( td->dots_scouring_tithe->is_ticking() )
          mult += 1.0;

        // TODO:
        // Impending catastrophe
        // Soul Rot
        return mult;
      }

      double composite_da_multiplier( const action_state_t* s ) const override
      {
        double m = affliction_spell_t::composite_da_multiplier( s );
        m *= get_dots_ticking( s->target );

        if ( td( s->target )->dots_unstable_affliction->is_ticking() ) 
        {
          m *= 1 + p()->conduit.focused_malignancy.percent(); 
        }

        return m;
      }

      void execute() override
      {
        if ( p()->legendary.malefic_wrath->ok() )
        {
          p()->buffs.malefic_wrath->trigger();
          p()->procs.malefic_wrath->occur();
        }

          affliction_spell_t::execute();
      }

    };

    malefic_rapture_damage_instance_t* damage_instance;

    malefic_rapture_t( warlock_t* p, util::string_view options_str )
      : affliction_spell_t( "malefic_rapture", p, p->find_spell( 324536 ) ), 
        damage_instance( new malefic_rapture_damage_instance_t(p) )
    {
      parse_options( options_str );
      aoe = -1;

      impact_action = damage_instance;
      add_child( impact_action );

    }

    void consume_resource() override
    {
      affliction_spell_t::consume_resource();

      if ( p()->legendary.mark_of_borrowed_power->ok() )
      {
        double chance = p()->legendary.mark_of_borrowed_power->effectN(1).percent();
        make_event<borrowed_power_event_t>(*p()->sim, p(), as<int>(last_resource_cost), chance);
      }
    }

};

// Talents

// lvl 15 - nightfall|drain soul|haunt
struct drain_soul_t : public affliction_spell_t
{
  drain_soul_t( warlock_t* p, util::string_view options_str )
    : affliction_spell_t( "drain_soul", p, p->talents.drain_soul )
  {
    parse_options( options_str );
    channeled    = true;
    hasted_ticks = may_crit = true;
  }

  void execute() override
  {
    dot_t* dot = get_dot( target );
    if ( dot->is_ticking() )
      p()->buffs.decimating_bolt->decrement();

    affliction_spell_t::execute();
  }

  void tick( dot_t* d ) override
  {
    affliction_spell_t::tick( d );
    if ( result_is_hit( d->state->result ) )
    {
      // TODO - Add passive check
      td( d->target )->debuffs_shadow_embrace->trigger();
    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = affliction_spell_t::composite_target_multiplier( t );

    if ( t->health_percentage() < p()->talents.drain_soul->effectN( 3 ).base_value() )
      m *= 1.0 + p()->talents.drain_soul->effectN( 2 ).percent();

    m *= 1 + p()->buffs.decimating_bolt->check_value();
    m *= 1.0 + p()->buffs.malefic_wrath->check_stack_value();

    return m;
  }

  void last_tick( dot_t* d ) override
  {
    affliction_spell_t::last_tick( d );
    p()->buffs.decimating_bolt->decrement();
  }

};

struct haunt_t : public affliction_spell_t
{
  haunt_t( warlock_t* p, util::string_view options_str ) : affliction_spell_t( "haunt", p, p->talents.haunt )
  {
    parse_options( options_str );
  }

  void impact( action_state_t* s ) override
  {
    affliction_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      td( s->target )->debuffs_haunt->trigger();
    }

    td( s->target )->debuffs_shadow_embrace->trigger();
  }
};

struct siphon_life_t : public affliction_spell_t
{
  bool pandemic_invocation_usable;

  siphon_life_t( warlock_t* p, util::string_view options_str )
    : affliction_spell_t( "siphon_life", p, p->talents.siphon_life )
  {
    parse_options( options_str );
    may_crit                   = false;
    pandemic_invocation_usable = false;  // BFA - Azerite
  }

  void execute() override
  {
    // BFA - Azerite
    // Do checks for Pandemic Invocation before parent execute() is called so we get the correct DoT states.
    if ( p()->azerite.pandemic_invocation.ok() && td( target )->dots_siphon_life->is_ticking() &&
         td( target )->dots_siphon_life->remains() <=
             p()->azerite.pandemic_invocation.spell_ref().effectN( 2 ).time_value() )
      pandemic_invocation_usable = true;

    affliction_spell_t::execute();

    if ( pandemic_invocation_usable )
    {
      p()->active.pandemic_invocation->schedule_execute();
      pandemic_invocation_usable = false;
    }
  }
};

struct phantom_singularity_tick_t : public affliction_spell_t
{
  phantom_singularity_tick_t( warlock_t* p )
    : affliction_spell_t( "phantom_singularity_tick", p, p->find_spell( 205246 ) )
  {
    background = true;
    may_miss   = false;
    dual       = true;
    aoe        = -1;
  }
};

struct phantom_singularity_t : public affliction_spell_t
{
  phantom_singularity_t( warlock_t* p, util::string_view options_str )
    : affliction_spell_t( "phantom_singularity", p, p->talents.phantom_singularity )
  {
    parse_options( options_str );
    callbacks    = false;
    hasted_ticks = true;
    tick_action  = new phantom_singularity_tick_t( p );

    spell_power_mod.tick = 0;
  }

  void init() override
  {
    affliction_spell_t::init();

    update_flags &= ~STATE_HASTE;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return s->action->tick_time( s ) * 8.0;
  }
};

struct vile_taint_t : public affliction_spell_t
{
  vile_taint_t( warlock_t* p, util::string_view options_str )
    : affliction_spell_t( "vile_taint", p, p->talents.vile_taint )
  {
    parse_options( options_str );

    hasted_ticks = tick_zero = true;
    aoe                      = -1;
  }
};

// BFA - Azerite
// TOCHECK: Does this damage proc affect Seed of Corruption?
struct pandemic_invocation_t : public affliction_spell_t
{
  pandemic_invocation_t( warlock_t* p ) : affliction_spell_t( "Pandemic Invocation", p, p->find_spell( 289367 ) )
  {
    background = true;

    base_dd_min = base_dd_max = p->azerite.pandemic_invocation.value();
  }

  void execute() override
  {
    affliction_spell_t::execute();

    if ( p()->rng().roll( p()->azerite.pandemic_invocation.spell_ref().effectN( 3 ).percent() / 100.0 ) )
      p()->resource_gain( RESOURCE_SOUL_SHARD, 1.0, p()->gains.pandemic_invocation );
  }
};

struct dark_soul_t : public affliction_spell_t
{
  dark_soul_t( warlock_t* p, const std::string& options_str )
    : affliction_spell_t( "dark_soul", p, p->talents.dark_soul_misery )
  {
    parse_options( options_str );
    harmful = may_crit = may_miss = false;
  }

  void execute() override
  {
    affliction_spell_t::execute();

    p()->buffs.dark_soul_misery->trigger();
  }
};
}  // namespace actions_affliction

namespace buffs_affliction
{
using namespace buffs;

}  // namespace buffs_affliction

// add actions
action_t* warlock_t::create_action_affliction( util::string_view action_name, const std::string& options_str )
{
  using namespace actions_affliction;

  if ( action_name == "shadow_bolt" )
    return new shadow_bolt_t( this, options_str );
  if ( action_name == "corruption" )
    return new corruption_t( this, options_str );
  if ( action_name == "agony" )
    return new agony_t( this, options_str );
  if ( action_name == "unstable_affliction" )
    return new unstable_affliction_t( this, options_str );
  if ( action_name == "summon_darkglare" )
    return new summon_darkglare_t( this, options_str );
  // aoe
  if ( action_name == "seed_of_corruption" )
    return new seed_of_corruption_t( this, options_str );
  // talents
  if ( action_name == "drain_soul" )
    return new drain_soul_t( this, options_str );
  if ( action_name == "haunt" )
    return new haunt_t( this, options_str );
  if ( action_name == "phantom_singularity" )
    return new phantom_singularity_t( this, options_str );
  if ( action_name == "siphon_life" )
    return new siphon_life_t( this, options_str );
  if ( action_name == "dark_soul" )
    return new dark_soul_t( this, options_str );
  if ( action_name == "vile_taint" )
    return new vile_taint_t( this, options_str );
  if ( action_name == "malefic_rapture" )
    return new malefic_rapture_t( this, options_str );

  return nullptr;
}

void warlock_t::create_buffs_affliction()
{
  // spells
  buffs.drain_life = make_buff( this, "drain_life" );
  // talents
  buffs.dark_soul_misery = make_buff( this, "dark_soul", talents.dark_soul_misery )
                               ->set_default_value( talents.dark_soul_misery->effectN( 1 ).percent() )
                               ->set_pct_buff_type( STAT_PCT_BUFF_HASTE );
  buffs.nightfall = make_buff( this, "nightfall", find_spell( 264571 ) )
                        ->set_default_value( find_spell( 264571 )->effectN( 2 ).percent() )
                        ->set_trigger_spell( talents.nightfall );
  buffs.inevitable_demise = make_buff( this, "inevitable_demise", talents.inevitable_demise )
                                ->set_max_stack( find_spell( 334320 )->max_stacks() )
                                ->set_default_value( talents.inevitable_demise->effectN( 1 ).percent() );
  // BFA - Azerite
  buffs.cascading_calamity = make_buff<stat_buff_t>( this, "cascading_calamity", azerite.cascading_calamity )
                                 ->add_stat( STAT_HASTE_RATING, azerite.cascading_calamity.value() )
                                 ->set_duration( find_spell( 275378 )->duration() )
                                 ->set_refresh_behavior( buff_refresh_behavior::DURATION );
  buffs.wracking_brilliance = make_buff<stat_buff_t>( this, "wracking_brilliance", azerite.wracking_brilliance )
                                  ->add_stat( STAT_INTELLECT, azerite.wracking_brilliance.value() )
                                  ->set_duration( find_spell( 272893 )->duration() )
                                  ->set_refresh_behavior( buff_refresh_behavior::DURATION );
  buffs.malefic_wrath = make_buff( this, "malefic_wrath", find_spell( 337125 ) )->set_default_value_from_effect( 1 );
}
 
void warlock_t::vision_of_perfection_proc_aff()
{
  timespan_t summon_duration = spec.summon_darkglare->duration() * vision_of_perfection_multiplier;

  warlock_pet_list.vop_darkglares.spawn( summon_duration, 1u );

  auto vop = find_azerite_essence( "Vision of Perfection" );

  timespan_t darkglare_extension =
      timespan_t::from_seconds( vop.spell_ref( vop.rank(), essence_type::MAJOR ).effectN( 3 ).base_value() / 1000 );

  darkglare_extension_helper( this, darkglare_extension );
}

void warlock_t::init_spells_affliction()
{
  using namespace actions_affliction;
  // General
  spec.affliction                   = find_specialization_spell( 137043 );
  mastery_spells.potent_afflictions = find_mastery_spell( WARLOCK_AFFLICTION );

  // Specialization Spells
  spec.unstable_affliction = find_specialization_spell( "Unstable Affliction" );
  spec.agony               = find_specialization_spell( "Agony" );
  spec.agony_2             = find_spell( 231792 );
  spec.summon_darkglare    = find_specialization_spell( "Summon Darkglare" );
  spec.corruption_2        = find_specialization_spell( "Corruption", "Rank 2" );
  spec.corruption_3        = find_specialization_spell( "Corruption", "Rank 3" );
  spec.unstable_affliction_2 = find_specialization_spell( "Unstable Affliction", "Rank 2" );
  spec.unstable_affliction_3 = find_specialization_spell( "Unstable Affliction", "Rank 3" );
  spec.shadow_bolt            = find_specialization_spell( "Shadow Bolt" );

  // Talents
  talents.nightfall           = find_talent_spell( "Nightfall" );
  talents.inevitable_demise   = find_talent_spell( "Inevitable Demise" );
  talents.drain_soul          = find_talent_spell( "Drain Soul" );
  talents.haunt               = find_talent_spell( "Haunt" );
  talents.writhe_in_agony     = find_talent_spell( "Writhe in Agony" );
  talents.absolute_corruption = find_talent_spell( "Absolute Corruption" );
  talents.siphon_life         = find_talent_spell( "Siphon Life" );
  talents.sow_the_seeds       = find_talent_spell( "Sow the Seeds" );
  talents.phantom_singularity = find_talent_spell( "Phantom Singularity" );
  talents.vile_taint          = find_talent_spell( "Vile Taint" );
  talents.dark_caller         = find_talent_spell( "Dark Caller" );
  talents.creeping_death      = find_talent_spell( "Creeping Death" );
  talents.dark_soul_misery    = find_talent_spell( "Dark Soul: Misery" );

  // BFA - Azerite
  azerite.cascading_calamity  = find_azerite_spell( "Cascading Calamity" );
  azerite.dreadful_calling    = find_azerite_spell( "Dreadful Calling" );
  azerite.inevitable_demise   = find_azerite_spell( "Inevitable Demise" );
  azerite.sudden_onset        = find_azerite_spell( "Sudden Onset" );
  azerite.wracking_brilliance = find_azerite_spell( "Wracking Brilliance" );
  azerite.pandemic_invocation = find_azerite_spell( "Pandemic Invocation" );

  // Legendaries
  legendary.malefic_wrath              = find_runeforge_legendary( "Malefic Wrath" );
  legendary.perpetual_agony_of_azjaqir = find_runeforge_legendary( "Perpetual Agony of Azj'Aqir" );
  //Wrath of Consumption and Sacrolash's Dark Strike are implemented in main module

  // Conduits
  conduit.cold_embrace       = find_conduit_spell( "Cold Embrace" );
  conduit.corrupting_leer    = find_conduit_spell( "Corrupting Leer" );
  conduit.focused_malignancy = find_conduit_spell( "Focused Malignancy" );
  conduit.rolling_agony      = find_conduit_spell( "Rolling Agony" );

  // Actives
  active.pandemic_invocation = new pandemic_invocation_t( this );
}

void warlock_t::init_gains_affliction()
{
  gains.agony                      = get_gain( "agony" );
  gains.seed_of_corruption         = get_gain( "seed_of_corruption" );
  gains.unstable_affliction_refund = get_gain( "unstable_affliction_refund" );
  gains.drain_soul                 = get_gain( "drain_soul" );
  // BFA - Azerite
  gains.pandemic_invocation = get_gain( "pandemic_invocation" );
}

void warlock_t::init_rng_affliction()
{
}

void warlock_t::init_procs_affliction()
{
  procs.nightfall = get_proc( "nightfall" );
  procs.corrupting_leer = get_proc( "corrupting_leer" );
  procs.malefic_wrath   = get_proc( "malefic_wrath" );
}

void warlock_t::create_apl_affliction()
{
  action_priority_list_t* def   = get_action_priority_list( "default" );
  action_priority_list_t* prep   = get_action_priority_list( "darkglare_prep" );
  action_priority_list_t* cds   = get_action_priority_list( "cooldowns" );

  def->add_action("phantom_singularity");
  def->add_action("vile_taint,if=soul_shard>1");
  def->add_action("siphon_life,if=refreshable");
  def->add_action("agony,if=refreshable");
  def->add_action("unstable_affliction,if=refreshable");
  def->add_action("corruption,if=refreshable");
  def->add_action("haunt");

  def->add_action("call_action_list,name=darkglare_prep,if=cooldown.summon_darkglare.remains<2&(dot.phantom_singularity.remains>2|!talent.phantom_singularity.enabled)");
  def->add_action("dark_soul,if=cooldown.summon_darkglare.remains>time_to_die");
  def->add_action("call_action_list,name=cooldowns");

  def->add_action("malefic_rapture,if=dot.vile_taint.ticking");
  def->add_action("malefic_rapture,if=!talent.vile_taint.enabled");

  def->add_action("drain_life,if=buff.inevitable_demise.stack>30");
  def->add_action("drain_soul");
  def->add_action("shadow_bolt");

  prep->add_action("vile_taint");
  prep->add_action("dark_soul");
  prep->add_action("potion");
  prep->add_action("use_items");
  prep->add_action("fireblood");
  prep->add_action("blood_fury");
  prep->add_action("berserking");
  prep->add_action("summon_darkglare");

  cds->add_action("worldvein_resonance");
  cds->add_action("memory_of_lucid_dreams");
  cds->add_action("blood_of_the_enemy");
  cds->add_action("guardian_of_azeroth");
  cds->add_action("ripple_in_space");
  cds->add_action("focused_azerite_beam");
  cds->add_action("purifying_blast");
  cds->add_action("reaping_flames");
  cds->add_action("concentrated_flame");
  cds->add_action("the_unbound_force,if=buff.reckless_force.remains");
}

std::unique_ptr<expr_t> warlock_t::create_aff_expression( util::string_view name_str )
{
  return player_t::create_expression( name_str );
}
}  // namespace warlock
