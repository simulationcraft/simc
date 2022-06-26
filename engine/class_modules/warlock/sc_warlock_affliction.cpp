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
  shadow_bolt_t( warlock_t* p, util::string_view options_str )
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
      if ( p()->talents.shadow_embrace->ok() )
        td( s->target )->debuffs_shadow_embrace->trigger();

      if ( p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T28, B4 ) )
      {        
        auto tdata = this->td( s->target );
        // TOFIX - As of 2022-02-03 PTR, the bonus appears to still be only checking that *any* target has these dots. May need to implement this behavior.
        bool tierDotsActive = tdata->dots_agony->is_ticking()
                           && tdata->dots_corruption->is_ticking()
                           && tdata->dots_unstable_affliction->is_ticking();

        if ( tierDotsActive && rng().roll( p()->sets->set(WARLOCK_AFFLICTION, T28, B4 )->effectN( 1 ).percent() ) )
        {
          p()->procs.calamitous_crescendo->occur();
          p()->buffs.calamitous_crescendo->trigger();
        }
      }
    }

    if ( p()->legendary.shard_of_annihilation.ok() )
      p()->buffs.shard_of_annihilation->decrement();
  }

  double action_multiplier() const override
  {
    double m = affliction_spell_t::action_multiplier();

    if ( time_to_execute == 0_ms && p()->buffs.nightfall->check() )
      m *= 1.0 + p()->buffs.nightfall->default_value;

    m *= 1.0 + p()->buffs.decimating_bolt->check_value();
    m *= 1.0 + p()->buffs.malefic_wrath->check_stack_value();

    return m;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = affliction_spell_t::composite_target_multiplier( t );

    //Withering Bolt does 2x% more per DoT on the target for Shadow Bolt
    //TODO: Check what happens if a DoT falls off mid-cast and mid-flight
    m *= 1.0 + p()->conduit.withering_bolt.percent() * 2.0 * p()->get_target_data( t )->count_affliction_dots();

    return m;
  }

  double composite_crit_chance() const override
  {
    double c = affliction_spell_t::composite_crit_chance();

    if ( p()->buffs.shard_of_annihilation->check() )
      c += p()->buffs.shard_of_annihilation->data().effectN( 1 ).percent();

    return c;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double m = affliction_spell_t::composite_crit_damage_bonus_multiplier();

    if ( p()->buffs.shard_of_annihilation->check() )
      m += p()->buffs.shard_of_annihilation->data().effectN( 2 ).percent();

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

  agony_t( warlock_t* p, util::string_view options_str ) : affliction_spell_t( "Agony", p, p->spec.agony )
  {
    parse_options( options_str );
    may_crit                   = false;

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

  //TODO - Agony gains a stack when executed on a target that already has Agony
  void execute() override
  {
    affliction_spell_t::execute();

    if ( p()->talents.writhe_in_agony->ok() && td( execute_state->target )->dots_agony->current_stack() <
      (int)p()->talents.writhe_in_agony->effectN( 3 ).base_value() )
    {
      td ( execute_state->target )
        ->dots_agony->increment( (int)p()->talents.writhe_in_agony->effectN( 3 ).base_value() -
          td( execute_state->target )->dots_agony->current_stack() );
    }
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
  corruption_t( warlock_t* p, util::string_view options_str, bool seed_action )
    : affliction_spell_t( "corruption", p, p->find_spell( 172 ) )   // 172 triggers 146739
  {
    auto otherSP = p->find_spell( 146739 );
    parse_options( options_str );
    tick_zero                  = false;

    if ( !p->spec.corruption_3->ok() || seed_action )
    {
      spell_power_mod.direct = 0; //Rank 3 is required for direct damage
    }

    spell_power_mod.tick       = data().effectN( 1 ).trigger()->effectN( 1 ).sp_coeff();
    base_tick_time             = data().effectN( 1 ).trigger()->effectN( 1 ).period();

    // 172 does not have spell duration any more.
    // were still lazy though so we aren't making a seperate spell for this.
    dot_duration               = otherSP->duration();
    
    // 2021-10-03 : Corruption's direct damage does not appear to be included in spec aura, only tick damage
    base_td_multiplier *= 1.0 + p->spec.affliction->effectN(2).percent();

    if ( p->talents.absolute_corruption->ok() )
    {
      dot_duration = sim->expected_iteration_time > 0_ms
                         ? 2 * sim->expected_iteration_time
                         : 2 * sim->max_time * ( 1.0 + sim->vary_combat_length );  // "infinite" duration
      base_td_multiplier *= 1.0 + p->talents.absolute_corruption->effectN( 2 ).percent(); // 2021-10-03: Only tick damage is affected
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

  if ( p->spec.summon_darkglare_2->ok() )
      cooldown->duration += timespan_t::from_millis( p->spec.summon_darkglare_2->effectN( 1 ).base_value() );
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
        corruption( new corruption_t( p, "", true ) )
    {
      aoe                              = -1;
      background                       = true;
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
      aoe = 1 + as<int>( p->talents.sow_the_seeds->effectN( 1 ).base_value() );
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

// 9.2 Tier Set
struct deliberate_corruption_t : public affliction_spell_t
{
  deliberate_corruption_t( warlock_t* p )
    : affliction_spell_t( "deliberate_corruption", p, p->find_spell( 367831 ) )
  {
    background      = true;
    affected_by_woc = false;
    
    // 2022-02-03 PTR - Deliberate Corruption was changed to a single tick DoT, presumably to automatically pick up this modifier in-game
    if ( p->talents.absolute_corruption->ok() )
      base_td_multiplier *= 1.0 + p->talents.absolute_corruption->effectN( 2 ).percent();
  }
};

struct malefic_rapture_t : public affliction_spell_t
{
    struct malefic_rapture_damage_instance_t : public affliction_spell_t
    {
      deliberate_corruption_t* deliberate_corruption;

      malefic_rapture_damage_instance_t( warlock_t *p, double spc ) :
          affliction_spell_t( "malefic_rapture_damage", p, p->find_spell( 324540 ) ), 
          deliberate_corruption( new deliberate_corruption_t( p ) )
      {
        aoe = 1;
        background = true;
        spell_power_mod.direct = spc;
        callbacks = false; //TOCHECK: Malefic Rapture did not proc Psyche Shredder, it may not cause any procs at all
      }

      double composite_da_multiplier( const action_state_t* s ) const override
      {
        double m = affliction_spell_t::composite_da_multiplier( s );

        m *= p()->get_target_data( s->target )->count_affliction_dots();

        if ( td( s->target )->dots_unstable_affliction->is_ticking() )
        {
          m *= 1.0 + p()->conduit.focused_malignancy.percent();
        }

        if ( p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T28, B2 ) )
        {
          m *= 1.0 + p()->sets->set( WARLOCK_AFFLICTION, T28, B2 )->effectN( 1 ).percent();
        }

        return m;
      }

      void impact ( action_state_t* s ) override
      {
        affliction_spell_t::impact( s );

        if ( p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T28, B2 ) )
        {
          timespan_t dot_extension =  p()->sets->set( WARLOCK_AFFLICTION, T28, B2 )->effectN( 2 ).time_value() * 1000;
          warlock_td_t* td = p()->get_target_data( s->target );

          td->dots_agony->adjust_duration( dot_extension );
          td->dots_unstable_affliction->adjust_duration(dot_extension);

          if ( !p()->talents.absolute_corruption->ok() )
          {
            td->dots_corruption->adjust_duration( dot_extension );
          }
          else
          {
            auto td = this->td( s->target );
            if ( td->dots_corruption->is_ticking() )
            {
              deliberate_corruption->execute_on_target( s->target );
            }
          }
        }
      }

      void execute() override
      {
        int d = p()->get_target_data( target )->count_affliction_dots() - 1;
        assert( d < as<int>( p()->procs.malefic_rapture.size() ) && "The procs.malefic_rapture array needs to be expanded." );

        if ( d >= 0 && d < as<int>( p()->procs.malefic_rapture.size() ) )
        {
          p()->procs.malefic_rapture[ d ]->occur();
        }

        affliction_spell_t::execute();
      }
    };

    malefic_rapture_t( warlock_t* p, util::string_view options_str )
      : affliction_spell_t( "malefic_rapture", p, p->find_spell( 324536 ) )
    {
      parse_options( options_str );
      aoe = -1;

      impact_action = new malefic_rapture_damage_instance_t( p, data().effectN( 1 ).sp_coeff() );
      add_child( impact_action );
    }

    double cost() const override
    {
      double c = affliction_spell_t::cost();

      if ( p()->buffs.calamitous_crescendo->check() )
        c *= 1.0 + p()->buffs.calamitous_crescendo->data().effectN( 4 ).percent();
        
      return c;      
    }

    timespan_t execute_time() const override
    {
      timespan_t t = affliction_spell_t::execute_time();

      if ( p()->buffs.calamitous_crescendo->check() )
        t *= 1.0 + p()->buffs.calamitous_crescendo->data().effectN( 3 ).percent();

      return t;
    }

    bool ready() override
    {
      if ( !affliction_spell_t::ready() )
       return false;

      target_cache.is_valid = false;
      return target_list().size() > 0;
    }

    void execute() override
    {
      affliction_spell_t::execute();

      if (p()->legendary.malefic_wrath->ok())
      {
        p()->buffs.malefic_wrath->trigger();
        p()->procs.malefic_wrath->occur();
      }

      p()->buffs.calamitous_crescendo->expire();
    }

    size_t available_targets( std::vector<player_t*>& tl ) const override
    {
      affliction_spell_t::available_targets( tl );

      tl.erase( std::remove_if( tl.begin(), tl.end(), [ this ]( player_t* target ){ return p()->get_target_data( target )->count_affliction_dots() == 0; } ), tl.end() );

      return tl.size();
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
    {
      p()->buffs.decimating_bolt->decrement();

      if ( p()->legendary.shard_of_annihilation.ok() )
        p()->buffs.shard_of_annihilation->decrement( 3 );
    }
    affliction_spell_t::execute();
  }

  void tick( dot_t* d ) override
  {
    affliction_spell_t::tick( d );
    if ( result_is_hit( d->state->result ) )
    {
      if ( p()->talents.shadow_embrace->ok() )
        td( d->target )->debuffs_shadow_embrace->trigger();

      if ( p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T28, B4 ) )
      {
        // TOFIX - As of 2022-02-03 PTR, the bonus appears to still be only checking that *any* target has these dots. May need to implement this behavior.
        bool tierDotsActive = td( d->target )->dots_agony->is_ticking() 
                           && td( d->target )->dots_corruption->is_ticking()
                           && td( d->target )->dots_unstable_affliction->is_ticking();

        if ( tierDotsActive && rng().roll( p()->sets->set( WARLOCK_AFFLICTION, T28, B4 )->effectN( 2 ).percent() ) )
        {
          p()->procs.calamitous_crescendo->occur();
          p()->buffs.calamitous_crescendo->trigger();
        }
      }
    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = affliction_spell_t::composite_target_multiplier( t );

    if ( t->health_percentage() < p()->talents.drain_soul->effectN( 3 ).base_value() )
      m *= 1.0 + p()->talents.drain_soul->effectN( 2 ).percent();

    m *= 1.0 + p()->buffs.decimating_bolt->check_value();
    m *= 1.0 + p()->buffs.malefic_wrath->check_stack_value();

    //Withering Bolt does x% more damage per DoT on the target
    //TODO: Check what happens if a DoT falls off mid-channel
    m *= 1.0 + p()->conduit.withering_bolt.percent() * p()->get_target_data( t )->count_affliction_dots();

    return m;
  }

  double composite_crit_chance() const override
  {
    double c = affliction_spell_t::composite_crit_chance();

    if ( p()->buffs.shard_of_annihilation->check() )
      c += p()->buffs.shard_of_annihilation->data().effectN( 3 ).percent();

    return c;
  }

  double composite_crit_damage_bonus_multiplier() const override
  {
    double m = affliction_spell_t::composite_crit_damage_bonus_multiplier();

    if ( p()->buffs.shard_of_annihilation->check() )
      m += p()->buffs.shard_of_annihilation->data().effectN( 4 ).percent();

    return m;
  }

  void last_tick( dot_t* d ) override
  {
    affliction_spell_t::last_tick( d );

    p()->buffs.decimating_bolt->decrement();

    if ( p()->legendary.shard_of_annihilation.ok() )
      p()->buffs.shard_of_annihilation->decrement( 3 );
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
  }
};

struct siphon_life_t : public affliction_spell_t
{
  siphon_life_t( warlock_t* p, util::string_view options_str )
    : affliction_spell_t( "siphon_life", p, p->talents.siphon_life )
  {
    parse_options( options_str );
    may_crit                   = false;
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

struct dark_soul_t : public affliction_spell_t
{
  dark_soul_t( warlock_t* p, util::string_view options_str )
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
action_t* warlock_t::create_action_affliction( util::string_view action_name, util::string_view options_str )
{
  using namespace actions_affliction;

  if ( action_name == "shadow_bolt" )
    return new shadow_bolt_t( this, options_str );
  if ( action_name == "corruption" )
    return new corruption_t( this, options_str, false );
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
  buffs.inevitable_demise = make_buff( this, "inevitable_demise", find_spell( 334320 ) )
                                ->set_default_value( talents.inevitable_demise->effectN( 1 ).percent() )
                                ->set_trigger_spell( talents.inevitable_demise );

  buffs.malefic_wrath = make_buff( this, "malefic_wrath", find_spell( 337125 ) )->set_default_value_from_effect( 1 );

  buffs.calamitous_crescendo = make_buff( this, "calamitous_crescendo", find_spell( 364322 ) );
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
  spec.summon_darkglare_2         = find_specialization_spell( "Summon Darkglare", "Rank 2" ); //9.1 PTR - Now a passive learned at level 58

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
  talents.dark_caller         = find_talent_spell( "Dark Caller" ); //9.1 PTR - Removed as talent
  talents.shadow_embrace      = find_talent_spell( "Shadow Embrace" ); //9.1 PTR - Replaces Dark Caller
  talents.creeping_death      = find_talent_spell( "Creeping Death" );
  talents.dark_soul_misery    = find_talent_spell( "Dark Soul: Misery" );

  // Legendaries
  legendary.malefic_wrath              = find_runeforge_legendary( "Malefic Wrath" );
  legendary.perpetual_agony_of_azjaqir = find_runeforge_legendary( "Perpetual Agony of Azj'Aqir" );
  //Wrath of Consumption and Sacrolash's Dark Strike are implemented in main module

  // Conduits
  conduit.cold_embrace       = find_conduit_spell( "Cold Embrace" ); //9.1 PTR - Removed
  conduit.corrupting_leer    = find_conduit_spell( "Corrupting Leer" );
  conduit.focused_malignancy = find_conduit_spell( "Focused Malignancy" );
  conduit.rolling_agony      = find_conduit_spell( "Rolling Agony" );
  conduit.withering_bolt     = find_conduit_spell( "Withering Bolt" ); //9.1 PTR - New, replaces Cold Embrace
}

void warlock_t::init_gains_affliction()
{
  gains.agony                      = get_gain( "agony" );
  gains.unstable_affliction_refund = get_gain( "unstable_affliction_refund" );
  gains.drain_soul                 = get_gain( "drain_soul" );
}

void warlock_t::init_rng_affliction()
{
}

void warlock_t::init_procs_affliction()
{
  procs.nightfall            = get_proc( "nightfall" );
  procs.corrupting_leer      = get_proc( "corrupting_leer" );
  procs.malefic_wrath        = get_proc( "malefic_wrath" );
  procs.calamitous_crescendo = get_proc( "calamitous_crescendo" );

  for ( size_t i = 0; i < procs.malefic_rapture.size(); i++ )
  {
    procs.malefic_rapture[ i ] = get_proc( fmt::format( "Malefic Rapture {}", i + 1 ) );
  }
}

}  // namespace warlock
