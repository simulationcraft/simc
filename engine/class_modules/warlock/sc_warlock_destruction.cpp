#include "simulationcraft.hpp"

#include "sc_warlock.hpp"

namespace warlock
{
namespace actions_destruction
{
using namespace actions;

struct destruction_spell_t : public warlock_spell_t
{
public:
  bool destro_mastery;
  bool chaos_incarnate;

  destruction_spell_t( util::string_view token, warlock_t* p, const spell_data_t* s = spell_data_t::nil() )
    : warlock_spell_t( token, p, s )
  {
    destro_mastery = true;
    chaos_incarnate = false;
  }

  void consume_resource() override
  {
    warlock_spell_t::consume_resource();

    int shards_used = as<int>( cost() );
    int base_shards = as<int>( base_cost() ); // Power Overwhelming is ignoring any cost changes

    // Do cost changes reduce number of draws appropriately? This may be difficult to check
    if ( resource_current == RESOURCE_SOUL_SHARD && p()->buffs.rain_of_chaos->check() && shards_used > 0 )
    {
      for ( int i = 0; i < shards_used; i++ )
      {
        if ( p()->rain_of_chaos_rng->trigger() )
        {
          p()->warlock_pet_list.infernals.spawn( p()->talents.summon_infernal_roc->duration() );
          p()->procs.rain_of_chaos->occur();
        }
      }
    }

    if ( p()->talents.ritual_of_ruin->ok() && resource_current == RESOURCE_SOUL_SHARD && shards_used > 0 )
    {
      int overflow = p()->buffs.impending_ruin->check() + shards_used - p()->buffs.impending_ruin->max_stack();
      p()->buffs.impending_ruin->trigger( shards_used ); // Stack change callback should switch Impending Ruin to Ritual of Ruin if max stacks reached
      if ( overflow > 0 )
        make_event( sim, 1_ms, [ this, overflow ] { p()->buffs.impending_ruin->trigger( overflow ); } );
    }

    if ( p()->talents.power_overwhelming->ok() && resource_current == RESOURCE_SOUL_SHARD && base_shards > 0 )
    {
      p()->buffs.power_overwhelming->trigger( base_shards );
    }

    // 2022-10-17: Spell data is missing the % chance!
    // Need to test further, but chance appears independent of shard cost
    // Also procs even if the cast is free due to other effects
    if ( resource_current == RESOURCE_SOUL_SHARD && base_shards > 0 && p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T29, B2 ) && rng().roll( 0.2 ) )
    {
      p()->buffs.chaos_maelstrom->trigger();
      p()->procs.chaos_maelstrom->occur();
    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = warlock_spell_t::composite_target_multiplier( t );

    if ( p()->talents.roaring_blaze->ok() && td( t )->debuffs_conflagrate->check() && data().affected_by( p()->talents.conflagrate_debuff->effectN( 1 ) ) )
      m *= 1.0 + td( t )->debuffs_conflagrate->check_value();

    if ( p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T30, B4 ) && p()->buffs.umbrafire_embers->check() )
    {
      // Umbrafire Embers has a whitelist split into two effects for direct damage/periodic damage
      if ( data().affected_by( p()->tier.umbrafire_embers->effectN( 1 ) ) || data().affected_by( p()->tier.umbrafire_embers->effectN( 2 ) ) )
        m *= 1.0 + p()->buffs.umbrafire_embers->check_stack_value();
    }

    return m;
  }

  double action_multiplier() const override
  {
    double pm = warlock_spell_t::action_multiplier();

    if ( p()->warlock_base.chaotic_energies->ok() && destro_mastery )
    {
      double destro_mastery_value = p()->cache.mastery_value() / 2.0;
      double chaotic_energies_rng = rng().range( 0, destro_mastery_value );

      if ( chaos_incarnate )
        chaotic_energies_rng = destro_mastery_value;

      pm *= 1.0 + chaotic_energies_rng + ( destro_mastery_value );
    }

    return pm;
  }
};

struct internal_combustion_t : public destruction_spell_t
{
  internal_combustion_t( warlock_t* p )
    : destruction_spell_t( "Internal Combustion", p, p->talents.internal_combustion )
  {
    background = dual = true;
    destro_mastery = false;
  }

  void init() override
  {
    destruction_spell_t::init();

    snapshot_flags &= STATE_NO_MULTIPLIER;
  }

  void execute() override
  {
    warlock_td_t* td = p()->get_target_data( target );
    dot_t* dot = td->dots_immolate;

    assert( dot->current_action );
    action_state_t* state = dot->current_action->get_state( dot->state );
    dot->current_action->calculate_tick_amount( state, 1.0 );

    double tick_base_damage = state->result_raw;

    // 2023-04-29 Internal Combustion does not benefit from the Roaring Blaze multiplier only
    if ( td->debuffs_conflagrate->up() )
      tick_base_damage /= 1.0 + td->debuffs_conflagrate->check_value();

    timespan_t remaining = std::min( dot->remains(), timespan_t::from_seconds( p()->talents.internal_combustion->effectN( 1 ).base_value() ) );
    timespan_t dot_tick_time = dot->current_action->tick_time( state );
    double ticks_left = remaining / dot_tick_time;
    double total_damage = ticks_left * tick_base_damage;

    action_state_t::release( state );
    
    base_dd_min = base_dd_max = total_damage;
    destruction_spell_t::execute();
    td->dots_immolate->adjust_duration( -remaining );
  }
};

struct shadowburn_t : public destruction_spell_t
{
  shadowburn_t( warlock_t* p, util::string_view options_str )
    : destruction_spell_t( "Shadowburn", p, p->talents.shadowburn )
  {
    parse_options( options_str );
    can_havoc = true;
    cooldown->hasted = true;
    chaos_incarnate = p->talents.chaos_incarnate->ok();

    base_multiplier *= 1.0 + p->talents.ruin->effectN( 1 ).percent();

    if ( p->talents.chaosbringer->ok() )
      base_multiplier *= 1.0 + p->talents.chaosbringer->effectN( 3 ).percent();
  }

  void impact( action_state_t* s ) override
  {
    destruction_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      td( s->target )->debuffs_shadowburn->trigger();

      if ( p()->talents.eradication->ok() )
        td( s->target )->debuffs_eradication->trigger();
    }
  }

  void execute() override
  {
    destruction_spell_t::execute();

    // Conflagration of Chaos can proc from a spell that consumes it
    p()->buffs.conflagration_of_chaos_sb->expire();

    if ( p()->talents.conflagration_of_chaos->ok() && rng().roll( p()->talents.conflagration_of_chaos->effectN( 1 ).percent() ) )
    {
      p()->buffs.conflagration_of_chaos_sb->trigger();
      p()->procs.conflagration_of_chaos_sb->occur();
    }

    if ( p()->talents.madness_of_the_azjaqir->ok() )
      p()->buffs.madness_sb->trigger();

    if ( p()->talents.burn_to_ashes->ok() )
      p()->buffs.burn_to_ashes->trigger( as<int>( p()->talents.burn_to_ashes->effectN( 4 ).base_value() ) );
  }

  double action_multiplier() const override
  {
    double m = destruction_spell_t::action_multiplier();

    if ( p()->talents.madness_of_the_azjaqir->ok() )
      m *= 1.0 + p()->buffs.madness_sb->check_value();

    return m;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = destruction_spell_t::composite_target_multiplier( t );

    if ( p()->talents.ashen_remains->ok() && td( t )->dots_immolate->is_ticking() )
      m *= 1.0 + p()->talents.ashen_remains->effectN( 1 ).percent();

    return m;
  }

  double composite_target_crit_chance( player_t* target ) const override
  {
    double m = destruction_spell_t::composite_target_crit_chance( target );

    // No spelldata for the health threshold
    if ( target->health_percentage() <= 20.0 )
      m += p()->talents.shadowburn->effectN( 3 ).percent();

    return m;
  }

  // The critical strike behavior for Conflagration of Chaos is based off of Chaos Bolt handling
  double composite_crit_chance() const override
  {
    if ( p()->buffs.conflagration_of_chaos_sb->check() )
      return 1.0;

    return destruction_spell_t::composite_crit_chance();
  }

  double calculate_direct_amount( action_state_t* state ) const override
  {
    double amt = destruction_spell_t::calculate_direct_amount( state );
    
    if ( p()->buffs.conflagration_of_chaos_sb->check() )
    {
      state->result_total *= 1.0 + player->cache.spell_crit_chance();
      return state->result_total;
    }

    return amt;
  }
};

struct havoc_t : public destruction_spell_t
{
  havoc_t( warlock_t* p, util::string_view options_str ) : destruction_spell_t( "Havoc", p, p->talents.havoc )
  {
    parse_options( options_str );
    may_crit = false;
  }

  void impact( action_state_t* s ) override
  {
    destruction_spell_t::impact( s );

    td( s->target )->debuffs_havoc->trigger();
  }
};

struct immolate_t : public destruction_spell_t
{
  struct immolate_dot_t : public destruction_spell_t
  {
    immolate_dot_t( warlock_t* p ) : destruction_spell_t( "Immolate", p, p->warlock_base.immolate_dot )
    {
      background = dual = true;
      spell_power_mod.tick = p->warlock_base.immolate_dot->effectN( 1 ).sp_coeff();

      dot_duration += p->talents.improved_immolate->effectN( 1 ).time_value();

      base_multiplier *= 1.0 + p->talents.scalding_flames->effectN( 2 ).percent();
    }

    void tick( dot_t* d ) override
    {
      destruction_spell_t::tick( d );

      if ( d->state->result == RESULT_CRIT && rng().roll( p()->warlock_base.immolate->effectN( 2 ).percent() ) )
        p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1, p()->gains.immolate_crits );

      p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1, p()->gains.immolate );

      if ( p()->talents.flashpoint->ok() && d->state->target->health_percentage() >= p()->talents.flashpoint->effectN( 2 ).base_value() )
        p()->buffs.flashpoint->trigger();

      // TOCHECK: Does Immolate direct damage also proc the tier bonus? 
      // This could affect the value for increment_max obtained from logs
      if ( p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T30, B2 ) )
      {
        double increment_max = 0.07;

        increment_max *= std::pow( p()->get_active_dots( d ), -2.0 / 3.0 );

        p()->cdf_accumulator += rng().range( 0.0, increment_max );

        if ( p()->cdf_accumulator >= 1.0 )
        {
          p()->proc_actions.channel_demonfire->execute_on_target( d->target );
          p()->procs.channel_demonfire->occur();
          p()->cdf_accumulator -= 1.0;
        }
      }

      if ( p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T31, B2 ) )
      {
        double increment_max = 0.16;

        increment_max *= std::pow( p()->get_active_dots( d ), -4.0 / 9.0 );

        p()->dimensional_accumulator += rng().range( 0.0, increment_max );

        if ( p()->dimensional_accumulator >= 1.0 )
        {
          p()->cooldowns.dimensional_rift->reset( true, 1 );
          p()->procs.dimensional_refund->occur();
          p()->dimensional_accumulator -= 1.0;
        }
      }
    }

    void last_tick( dot_t* d ) override
    {
      if ( p()->get_active_dots( d ) == 1 )
        p()->dimensional_accumulator = rng().range( 0.0, 0.99 );

      destruction_spell_t::last_tick( d );
    }
  };

  immolate_t( warlock_t* p, util::string_view options_str ) : destruction_spell_t( "Immolate (direct)", p, p->warlock_base.immolate )
  {
    parse_options( options_str );

    can_havoc = true;

    impact_action = new immolate_dot_t( p );
    add_child( impact_action );

    base_multiplier *= 1.0 + p->talents.scalding_flames->effectN( 1 ).percent();
  }

  dot_t* get_dot( player_t* t ) override
  {
    return impact_action->get_dot( t );
  }
};

struct conflagrate_t : public destruction_spell_t
{
  conflagrate_t( warlock_t* p, util::string_view options_str )
    : destruction_spell_t( "Conflagrate", p, p->talents.conflagrate )
  {
    parse_options( options_str );
    can_havoc = true;

    energize_type = action_energize::PER_HIT;
    energize_resource = RESOURCE_SOUL_SHARD;
    energize_amount = ( p->talents.conflagrate_2->effectN( 1 ).base_value() ) / 10.0;

    cooldown->hasted = true;
    cooldown->charges += as<int>( p->talents.improved_conflagrate->effectN( 1 ).base_value() );
    cooldown->duration += p->talents.explosive_potential->effectN( 1 ).time_value();

    base_multiplier *= 1.0 + p->talents.ruin->effectN( 1 ).percent();
  }

  void impact( action_state_t* s ) override
  {
    destruction_spell_t::impact( s );

    if ( p()->talents.roaring_blaze->ok() && result_is_hit( s->result ) )
      td( s->target )->debuffs_conflagrate->trigger();
  }

  void execute() override
  {
    destruction_spell_t::execute();

    // Conflagration of Chaos can proc from a spell that consumes it
    p()->buffs.conflagration_of_chaos_cf->expire();

    if ( p()->talents.conflagration_of_chaos->ok() && rng().roll( p()->talents.conflagration_of_chaos->effectN( 1 ).percent() ) )
    {
      p()->buffs.conflagration_of_chaos_cf->trigger();
      p()->procs.conflagration_of_chaos_cf->occur();
    }

    if ( p()->talents.backdraft->ok() )
      p()->buffs.backdraft->trigger();

    if ( p()->talents.decimation->ok() && target->health_percentage() <= p()->talents.decimation->effectN( 2 ).base_value() )
    {
      p()->cooldowns.soul_fire->adjust( p()->talents.decimation->effectN( 1 ).time_value() );
    }
  }

  // The critical strike behavior for Conflagration of Chaos is based off of Chaos Bolt handling
  double composite_crit_chance() const override
  {
    if ( p()->buffs.conflagration_of_chaos_cf->check() )
      return 1.0;

    return destruction_spell_t::composite_crit_chance();
  }

  double calculate_direct_amount( action_state_t* state ) const override
  {
    double amt = destruction_spell_t::calculate_direct_amount( state );
    
    if ( p()->buffs.conflagration_of_chaos_cf->check() )
    {
      state->result_total *= 1.0 + player->cache.spell_crit_chance();
      return state->result_total;
    }

    return amt;
  }
};

struct incinerate_state_t final : public action_state_t
{
  int total_hit;

  incinerate_state_t( action_t* action, player_t* target ) :
    action_state_t( action, target ), total_hit( 0 )
  {  }

  void initialize() override
  {
    action_state_t::initialize();
    total_hit = 0;
  }

  std::ostringstream& debug_str( std::ostringstream& s ) override
  {
    action_state_t::debug_str( s ) << " total_hit=" << total_hit;
    return s;
  }

  void copy_state( const action_state_t* s ) override
  {
    action_state_t::copy_state( s );
    total_hit = debug_cast<const incinerate_state_t*>( s )->total_hit;
  }
};

struct incinerate_fnb_t : public destruction_spell_t
{
  incinerate_fnb_t( warlock_t* p ) : destruction_spell_t( "Incinerate (Fire and Brimstone)", p, p->warlock_base.incinerate )
  {
    aoe = -1;
    background = dual = true;

    base_multiplier *= p->talents.fire_and_brimstone->effectN( 1 ).percent();
  }

  void init() override
  {
    destruction_spell_t::init();

    // F&B Incinerate's target list depends on the current Havoc target, so it
    // needs to invalidate its target list with the rest of the Havoc spells.
    p()->havoc_spells.push_back( this );
  }

  action_state_t* new_state() override
  { return new incinerate_state_t( this, target ); }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    debug_cast<incinerate_state_t*>( s )->total_hit = p()->incinerate_last_target_count;
    destruction_spell_t::snapshot_state( s, rt );
  }

  double cost() const override
  {
    return 0.0;
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    destruction_spell_t::available_targets( tl );

    // Does not hit the main target
    auto it = range::find( tl, target );
    if ( it != tl.end() )
    {
      tl.erase( it );
    }

    // nor the havoced target
    it = range::find( tl, p()->havoc_target );
    if ( it != tl.end() )
    {
      tl.erase( it );
    }

    return tl.size();
  }

  double action_multiplier() const override
  {
    double m = destruction_spell_t::action_multiplier();

    if ( p()->talents.burn_to_ashes->ok() )
      m *= 1.0 + p()->buffs.burn_to_ashes->check_value();

    return m;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = destruction_spell_t::composite_target_multiplier( t );

    if ( p()->talents.ashen_remains.ok() && td( t )->dots_immolate->is_ticking() )
      m *= 1.0 + p()->talents.ashen_remains->effectN( 1 ).percent();

    return m;
  }

  void impact( action_state_t* s ) override
  {
    destruction_spell_t::impact( s );

    // 2023-01-24 Fire and Brimstone crits are apparently still yielding fragments (unaffected by Diabolic Embers)
    if ( s->result == RESULT_CRIT )
      p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1, p()->gains.incinerate_fnb_crits );

    if ( p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T30, B2 ) )
    {
      auto inc_state = debug_cast<incinerate_state_t*>( s );
      double increment_max = 0.4;

      increment_max *= std::pow( inc_state->total_hit, -2.0 / 3.0 );

      p()->cdf_accumulator += rng().range( 0.0, increment_max );

      if ( p()->cdf_accumulator >= 1.0 )
      {
        p()->proc_actions.channel_demonfire->execute_on_target( inc_state->target );
        p()->procs.channel_demonfire->occur();
        p()->cdf_accumulator -= 1.0;
      }
    }
  }
};

struct incinerate_t : public destruction_spell_t
{
  double energize_mult;
  incinerate_fnb_t* fnb_action;
  
  incinerate_t( warlock_t* p, util::string_view options_str )
    : destruction_spell_t( "Incinerate", p, p->warlock_base.incinerate ), fnb_action( new incinerate_fnb_t( p ) )
  {
    parse_options( options_str );

    add_child( fnb_action );

    can_havoc = true;

    parse_effect_data( p->warlock_base.incinerate_energize->effectN( 1 ) );

    energize_mult = 1.0 + p->talents.diabolic_embers->effectN( 1 ).percent();
    energize_amount *= energize_mult;
  }

  action_state_t* new_state() override
  { return new incinerate_state_t( this, target ); }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    debug_cast<incinerate_state_t*>( s )->total_hit = p()->incinerate_last_target_count;
    destruction_spell_t::snapshot_state( s, rt );
  }

  timespan_t execute_time() const override
  {
    timespan_t h = spell_t::execute_time();

    if ( p()->buffs.backdraft->check() )
      h *= 1.0 + p()->talents.backdraft_buff->effectN( 1 ).percent();

    return h;
  }

  timespan_t gcd() const override
  {
    timespan_t t = action_t::gcd();

    if ( t == 0_ms )
      return t;

    if ( p()->buffs.backdraft->check() )
      t *= 1.0 + p()->talents.backdraft_buff->effectN( 2 ).percent();

    if ( t < min_gcd )
      t = min_gcd;

    return t;
  }

  void execute() override
  {
    // Need to calculate and store the total number of targets from parent + child
    // Currently only necessary for T30 2pc procs
    int n = std::max( n_targets(), 1 );
    if ( p()->talents.fire_and_brimstone->ok() ) 
      n += as<int>( fnb_action->target_list().size() );
    p()->incinerate_last_target_count = n;

    destruction_spell_t::execute();

    p()->buffs.backdraft->decrement();

    if ( p()->talents.fire_and_brimstone->ok() )
    {
      fnb_action->execute_on_target( target );
    }

    if ( p()->talents.decimation->ok() && target->health_percentage() <= p()->talents.decimation->effectN( 2 ).base_value() )
    {
      p()->cooldowns.soul_fire->adjust( p()->talents.decimation->effectN( 1 ).time_value() );
    }

    p()->buffs.burn_to_ashes->decrement(); // Decrement after Fire and Brimstone execute to ensure it's picked up
  }

  void impact( action_state_t* s ) override
  {
    destruction_spell_t::impact( s );

    if ( s->result == RESULT_CRIT )
      p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1 * energize_mult, p()->gains.incinerate_crits );

    if ( p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T30, B2 ) )
    {
      auto inc_state = debug_cast<incinerate_state_t*>( s );
      double increment_max = 0.4;

      increment_max *= std::pow( inc_state->total_hit, -2.0 / 3.0 );

      p()->cdf_accumulator += rng().range( 0.0, increment_max );

      if ( p()->cdf_accumulator >= 1.0 )
      {
        p()->proc_actions.channel_demonfire->execute_on_target( inc_state->target );
        p()->procs.channel_demonfire->occur();
        p()->cdf_accumulator -= 1.0;
      }
    }
  }

  double action_multiplier() const override
  {
    double m = destruction_spell_t::action_multiplier();

    if ( p()->talents.burn_to_ashes->ok() )
      m *= 1.0 + p()->buffs.burn_to_ashes->check_value();

    return m;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = destruction_spell_t::composite_target_multiplier( t );

    if ( p()->talents.ashen_remains->ok() && td( t )->dots_immolate->is_ticking() )
      m *= 1.0 + p()->talents.ashen_remains->effectN( 1 ).percent();

    return m;
  }
};

struct chaos_bolt_t : public destruction_spell_t
{
  struct cry_havoc_t : public destruction_spell_t
  {
    cry_havoc_t( warlock_t* p ) : destruction_spell_t( "Cry Havoc", p, p->talents.cry_havoc_proc )
    {
      background = dual = true;
      aoe = -1;
      destro_mastery = false;

      base_multiplier *= 1.0 + p->talents.cry_havoc->effectN( 1 ).percent(); // Unclear why, but the base talent is doubling the actual spell coeff
    }
  };

  internal_combustion_t* internal_combustion;
  cry_havoc_t* cry_havoc;

  chaos_bolt_t( warlock_t* p, util::string_view options_str )
    : destruction_spell_t( "Chaos Bolt", p, p->talents.chaos_bolt )
  {
    parse_options( options_str );
    can_havoc = true;
    chaos_incarnate = p->talents.chaos_incarnate->ok();

    if ( p->talents.chaosbringer->ok() )
      base_dd_multiplier *= 1.0 + p->talents.chaosbringer->effectN( 1 ).percent();

    if ( p->talents.internal_combustion->ok() )
    {
      internal_combustion = new internal_combustion_t( p );
      add_child( internal_combustion );
    }

    if ( p->talents.cry_havoc->ok() )
    {
      cry_havoc = new cry_havoc_t( p );
      add_child( cry_havoc );
    }
  }

  double cost_pct_multiplier() const override
  {
    double c = destruction_spell_t::cost_pct_multiplier();

    if ( p()->buffs.ritual_of_ruin->check() )
      c *= 1.0 + p()->talents.ritual_of_ruin_buff->effectN( 2 ).percent();

    return c;      
  }

  timespan_t execute_time() const override
  {
    timespan_t t = destruction_spell_t::execute_time();
    
    // 2022-10-15: Backdraft is not consumed for Ritual of Ruin empowered casts, but IS hasted by it
    if ( p()->buffs.ritual_of_ruin->check() )
      t *= 1.0 + p()->talents.ritual_of_ruin_buff->effectN( 3 ).percent();
    
    if ( p()->buffs.backdraft->check() )
      t *= 1.0 + p()->talents.backdraft_buff->effectN( 1 ).percent();

    if ( p()->buffs.madness_cb->check() )
      t *= 1.0 + p()->talents.madness_of_the_azjaqir->effectN( 2 ).percent();

    return t;
  }

  double action_multiplier() const override
  {
    double m = destruction_spell_t::action_multiplier();

    if ( p()->talents.madness_of_the_azjaqir->ok() )
      m *= 1.0 + p()->buffs.madness_cb->check_value();

    if ( p()->buffs.crashing_chaos->check() )
      m *= 1.0 + p()->talents.crashing_chaos->effectN( 2 ).percent();

    return m;
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = destruction_spell_t::composite_target_multiplier( t );

    if ( p()->talents.ashen_remains->ok() && td( t )->dots_immolate->is_ticking() )
      m *= 1.0 + p()->talents.ashen_remains->effectN( 1 ).percent();

    return m;
  }

  timespan_t gcd() const override
  {
    timespan_t t = destruction_spell_t::gcd();

    if ( t == 0_ms )
      return t;

    // 2022-10-15: Backdraft is not consumed for Ritual of Ruin empowered casts, but IS hasted by it
    if ( p()->buffs.backdraft->check() )
      t *= 1.0 + p()->talents.backdraft_buff->effectN( 2 ).percent();

    if ( t < min_gcd )
      t = min_gcd;

    return t;
  }

  void impact( action_state_t* s ) override
  {
    destruction_spell_t::impact( s );

    if ( p()->talents.internal_combustion->ok() && result_is_hit( s->result ) && p()->get_target_data( s->target )->dots_immolate->is_ticking() )
    {
      internal_combustion->execute_on_target( s->target );
    }

    if ( p()->talents.cry_havoc->ok() && td( s->target )->debuffs_havoc->check() )
    {
      cry_havoc->execute_on_target( s->target );
    }

    if ( p()->talents.eradication->ok() && result_is_hit( s->result ) )
      td( s->target )->debuffs_eradication->trigger();
  }

  void execute() override
  {
    destruction_spell_t::execute();

    // 2022-10-15: Backdraft is not consumed for Ritual of Ruin empowered casts, but IS hasted by it
    if ( !p()->buffs.ritual_of_ruin->check() )
      p()->buffs.backdraft->decrement();

    if ( p()->talents.avatar_of_destruction->ok() && p()->buffs.ritual_of_ruin->check() )
    {
      p()->proc_actions.avatar_of_destruction->execute_on_target( target );
    }

    p()->buffs.ritual_of_ruin->expire();

    p()->buffs.crashing_chaos->decrement();

    if ( p()->talents.madness_of_the_azjaqir->ok() )
      p()->buffs.madness_cb->trigger();

    if ( p()->talents.burn_to_ashes->ok() )
      p()->buffs.burn_to_ashes->trigger( as<int>( p()->talents.burn_to_ashes->effectN( 3 ).base_value() ) );
  }

  // Force spell to always crit
  double composite_crit_chance() const override
  {
    return 1.0;
  }

  double calculate_direct_amount( action_state_t* state ) const override
  {
    destruction_spell_t::calculate_direct_amount( state );

    // Can't use player-based crit chance from the state object as it's hardcoded to 1.0. Use cached
    // player spell crit instead. The state target crit chance of the state object is correct.
    // Targeted Crit debuffs function as a separate multiplier.
    // 2019-06-24: Target crit chance appears to no longer increase Chaos Bolt damage.
    state->result_total *= 1.0 + player->cache.spell_crit_chance();  //+ state->target_crit_chance;

    return state->result_total;
  }
};

struct infernal_awakening_t : public destruction_spell_t
{
  infernal_awakening_t( warlock_t* p ) : destruction_spell_t( "Infernal Awakening", p, p->talents.infernal_awakening )
  {
    destro_mastery = false;
    aoe = -1;
    background = dual = true;
  }

  void execute() override
  {
    destruction_spell_t::execute();

    p()->warlock_pet_list.infernals.spawn( p()->talents.summon_infernal_main->duration() ); // TOCHECK: Infernal duration may be fuzzy
  }
};

struct summon_infernal_t : public destruction_spell_t
{
  summon_infernal_t( warlock_t* p, util::string_view options_str )
    : destruction_spell_t( "Summon Infernal", p, p->talents.summon_infernal )
  {
    parse_options( options_str );

    may_crit = false;
    impact_action = new infernal_awakening_t( p );
    add_child( impact_action );

    if ( p->min_version_check( VERSION_10_2_0 ) && p->talents.grand_warlocks_design->ok() )
      cooldown->duration += p->talents.grand_warlocks_design->effectN( 3 ).time_value();
  }

  void execute() override
  {
    destruction_spell_t::execute();

    if ( p()->talents.crashing_chaos->ok() )
      p()->buffs.crashing_chaos->trigger();

    if ( p()->talents.rain_of_chaos->ok() )
      p()->buffs.rain_of_chaos->trigger();
  }
};

struct rain_of_fire_t : public destruction_spell_t
{
  struct rain_of_fire_tick_t : public destruction_spell_t
  {
    rain_of_fire_tick_t( warlock_t* p ) : destruction_spell_t( "Rain of Fire (tick)", p, p->talents.rain_of_fire_tick )
    {
      aoe = -1;
      background = dual = direct_tick = true;
      radius = p->talents.rain_of_fire->effectN( 1 ).radius();
      base_multiplier *= 1.0 + p->talents.inferno->effectN( 2 ).percent();

      if ( p->talents.chaosbringer->ok() )
        base_multiplier *= 1.0 + p->talents.chaosbringer->effectN( 2 ).percent();

      chaos_incarnate = p->talents.chaos_incarnate->ok();
    }

    void impact( action_state_t* s ) override
    {
      destruction_spell_t::impact( s );

      if ( p()->talents.inferno->ok() && result_is_hit( s->result ) )
      {
        auto target_scaling = p()->bugs ? 1.0 : ( 5.0 / std::max( 5u, s->n_targets ) );

        if ( rng().roll( p()->talents.inferno->effectN( 1 ).percent() * target_scaling ) )
        {
          p()->resource_gain( RESOURCE_SOUL_SHARD, 0.1, p()->gains.inferno );
        }
      }

      if ( p()->talents.pyrogenics->ok() )
        td( s->target )->debuffs_pyrogenics->trigger();
    }

    double composite_persistent_multiplier( const action_state_t* s ) const override
    {
      double m = destruction_spell_t::composite_persistent_multiplier( s );

      if ( p()->buffs.crashing_chaos->check() )
        m *= 1.0 + p()->talents.crashing_chaos->effectN( 1 ).percent();

      return m;
    }

    double action_multiplier() const override
    {
      double m = destruction_spell_t::action_multiplier();

      if ( p()->buffs.madness_rof_snapshot->check() )
        m *= 1.0 + p()->talents.madness_of_the_azjaqir->effectN( 1 ).percent();

      return m;
    }
  };

  rain_of_fire_t( warlock_t* p, util::string_view options_str )
    : destruction_spell_t( "Rain of Fire", p, p->talents.rain_of_fire )
  {
    parse_options( options_str );
    may_miss = may_crit = false;
    base_tick_time = 1_s;
    dot_duration = 0_s;

    aoe = -1; // Needed to apply Pyrogenics on cast

    if ( !p->proc_actions.rain_of_fire_tick )
    {
      p->proc_actions.rain_of_fire_tick = new rain_of_fire_tick_t( p );
      p->proc_actions.rain_of_fire_tick->stats = stats;
    }
  }

  double cost_pct_multiplier() const override
  {
    double c = destruction_spell_t::cost_pct_multiplier();

    if ( p()->buffs.ritual_of_ruin->check() )
      c *= 1.0 + p()->talents.ritual_of_ruin_buff->effectN( 5 ).percent();

    return c;        
  }

  void execute() override
  {
    destruction_spell_t::execute();

    // 2022-10-16: Madness of the Azj'Aqir buffs all active ticks of Rain of Fire when Rain of Fire is cast during the buff
    // This persists even after the Madness buff expires, so we store this in a dummy snapshot buff
    p()->buffs.madness_rof_snapshot->expire();

    if ( p()->buffs.madness_rof->check() )
      p()->buffs.madness_rof_snapshot->trigger();

    if ( p()->talents.madness_of_the_azjaqir->ok() )
      p()->buffs.madness_rof->trigger();

    if ( p()->talents.burn_to_ashes->ok() )
      p()->buffs.burn_to_ashes->trigger( as<int>( p()->talents.burn_to_ashes->effectN( 3 ).base_value() ) );

    make_event<ground_aoe_event_t>( *sim, p(),
                                    ground_aoe_params_t()
                                        .target( execute_state->target )
                                        .x( execute_state->target->x_position )
                                        .y( execute_state->target->y_position )
                                        .pulse_time( base_tick_time * player->cache.spell_haste() )
                                        .duration( p()->talents.rain_of_fire->duration() * player->cache.spell_haste() )
                                        .start_time( sim->current_time() )
                                        .action( p()->proc_actions.rain_of_fire_tick ) );

    if ( p()->talents.avatar_of_destruction->ok() && p()->buffs.ritual_of_ruin->check() )
    {
      p()->proc_actions.avatar_of_destruction->execute_on_target( target );
    }

    p()->buffs.ritual_of_ruin->expire();

    p()->buffs.crashing_chaos->decrement();
  }

  void impact( action_state_t* s ) override
  {
    destruction_spell_t::impact( s );

    if ( p()->talents.pyrogenics->ok() )
      td( s->target )->debuffs_pyrogenics->trigger();
  }
};

struct channel_demonfire_tick_t : public destruction_spell_t
{
  channel_demonfire_tick_t( warlock_t* p ) : destruction_spell_t( "Channel Demonfire (tick)", p, p->talents.channel_demonfire_tick )
  {
    background = dual = true;
    may_miss = false;

    spell_power_mod.direct = p->talents.channel_demonfire_tick->effectN( 1 ).sp_coeff();

    aoe = -1;

    base_aoe_multiplier = p->talents.channel_demonfire_tick->effectN( 2 ).sp_coeff() / p->talents.channel_demonfire_tick->effectN( 1 ).sp_coeff();

    travel_speed = p->talents.channel_demonfire_travel->missile_speed();
  }

  void impact( action_state_t* s ) override
  {
    destruction_spell_t::impact( s );

    // Raging Demonfire will adjust the time remaining on all targets hit by an AoE pulse
    if ( p()->talents.raging_demonfire->ok() && td( s->target )->dots_immolate->is_ticking() )
    {
      td( s->target )->dots_immolate->adjust_duration( p()->talents.raging_demonfire->effectN( 2 ).time_value() );
    }

    if ( s->chain_target == 0 && p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T30, B2 ) )
    {
      double increment_max = 0.4;

      // Presumably there is no target scaling for this ability, as it only procs off the main target of the bolt

      p()->cdf_accumulator += rng().range( 0.0, increment_max );

      if ( p()->cdf_accumulator >= 1.0 )
      {
        p()->proc_actions.channel_demonfire->execute_on_target( s->target );
        p()->procs.channel_demonfire->occur();
        p()->cdf_accumulator -= 1.0;
      }

      if ( p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T30, B4 ) )
        p()->buffs.umbrafire_embers->trigger();
    }
  }
};

struct channel_demonfire_t : public destruction_spell_t
{
  channel_demonfire_tick_t* channel_demonfire;

  channel_demonfire_t( warlock_t* p, util::string_view options_str )
    : destruction_spell_t( "Channel Demonfire", p, p->talents.channel_demonfire ),
      channel_demonfire( new channel_demonfire_tick_t( p ) )
  {
    parse_options( options_str );
    channeled = true;
    hasted_ticks = true;
    may_crit = false;

    add_child( channel_demonfire );

    // We need to fudge the duration to ensure the right number of ticks occur
    if ( p->talents.raging_demonfire->ok() )
    {
      int num_ticks = as<int>( dot_duration / base_tick_time + p->talents.raging_demonfire->effectN( 1 ).base_value() );
      base_tick_time *= 1.0 + p->talents.raging_demonfire->effectN( 3 ).percent();
      dot_duration = num_ticks * base_tick_time;
    }
  }

  void init() override
  {
    destruction_spell_t::init();

    cooldown->hasted = true;
  }

  std::vector<player_t*>& target_list() const override
  {
    target_cache.list = destruction_spell_t::target_list();

    size_t i = target_cache.list.size();
    while ( i > 0 )
    {
      i--;

      if ( !td( target_cache.list[ i ] )->dots_immolate->is_ticking() )
        target_cache.list.erase( target_cache.list.begin() + i );
    }
    return target_cache.list;
  }

  void execute() override
  {
    destruction_spell_t::execute();

    if ( p()->buffs.umbrafire_embers->check() )
    {
      timespan_t base = p()->buffs.umbrafire_embers->base_buff_duration;
      timespan_t remains = p()->buffs.umbrafire_embers->remains();
      p()->buffs.umbrafire_embers->extend_duration( p(), base - remains ); // Buff is reset on execute, not on each bolt impact
    }
  }

  void tick( dot_t* d ) override
  {
    // Need to invalidate the target cache to figure out immolated targets.
    target_cache.is_valid = false;

    const auto& targets = target_list();

    if ( !targets.empty() )
    {
      channel_demonfire->set_target( targets[ rng().range( size_t(), targets.size() ) ] );
      channel_demonfire->execute();
    }

    destruction_spell_t::tick( d );
  }

  bool ready() override
  {
    unsigned active_immolates = p()->get_active_dots( td( target )->dots_immolate );

    if ( active_immolates == 0 )
      return false;

    return destruction_spell_t::ready();
  }
};

struct soul_fire_t : public destruction_spell_t
{
  immolate_t* immolate;

  soul_fire_t( warlock_t* p, util::string_view options_str )
    : destruction_spell_t( "Soul Fire", p, p->talents.soul_fire ), immolate( new immolate_t( p, "" ) )
  {
    parse_options( options_str );
    energize_type = action_energize::PER_HIT;
    energize_resource = RESOURCE_SOUL_SHARD;
    energize_amount = ( p->talents.soul_fire_2->effectN( 1 ).base_value() ) / 10.0;

    base_multiplier *= 1.0 + p->talents.ruin->effectN( 1 ).percent();

    can_havoc = true;

    immolate->background = true;
    immolate->dual = true;
    immolate->base_costs[ RESOURCE_MANA ] = 0;
    immolate->base_dd_multiplier *= 0.0;
  }

  timespan_t execute_time() const override
  {
    timespan_t h = destruction_spell_t::execute_time();

    if ( p()->buffs.backdraft->check() )
      h *= 1.0 + p()->talents.backdraft_buff->effectN( 1 ).percent();

    return h;
  }

  timespan_t gcd() const override
  {
    timespan_t t = destruction_spell_t::gcd();

    if ( t == 0_ms )
      return t;

    if ( p()->buffs.backdraft->check() )
      t *= 1.0 + p()->talents.backdraft_buff->effectN( 2 ).percent();

    if ( t < min_gcd )
      t = min_gcd;

    return t;
  }

  void execute() override
  {
    destruction_spell_t::execute();

    immolate->execute_on_target( target );

    p()->buffs.backdraft->decrement();
  }
};

struct cataclysm_t : public destruction_spell_t
{
  immolate_t* immolate;

  cataclysm_t( warlock_t* p, util::string_view options_str )
    : destruction_spell_t( "Cataclysm", p, p->talents.cataclysm ), immolate( new immolate_t( p, "" ) )
  {
    parse_options( options_str );
    aoe = -1;

    immolate->background = true;
    immolate->dual = true;
    immolate->base_costs[ RESOURCE_MANA ] = 0;
    immolate->base_dd_multiplier *= 0.0;
  }

  void impact( action_state_t* s ) override
  {
    destruction_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      immolate->execute_on_target( s->target );
    }
  }
};

struct dimensional_cinder_t : public destruction_spell_t
{
  dimensional_cinder_t( warlock_t* p ) : destruction_spell_t( "Dimensional Cinder", p, p->tier.dimensional_cinder )
  {
    destro_mastery = may_crit = false;
    background = dual = true;
    aoe = -1;
    base_dd_min = base_dd_max = 0.0;
  }
};

// Dimensional Rift's portals are "pet damage" according to combat log behavior, but appear to be benefitting from
// buffs to the *Warlock's* damage specifically (i.e. Grimoire of Synergy). For now, we will model them as Warlock spells

struct shadowy_tear_t : public destruction_spell_t
{
  struct rift_shadow_bolt_t : public destruction_spell_t
  {
    dimensional_cinder_t* cinder;

    rift_shadow_bolt_t( warlock_t* p ) : destruction_spell_t( "Rift Shadow Bolt", p, p->talents.rift_shadow_bolt )
    {
      destro_mastery = false;
      background = dual = true;

      // Though this behaves like a direct damage spell, it is also whitelisted under the periodic spec aura and benefits as such in game
      base_dd_multiplier *= 1.0 + p->warlock_base.destruction_warlock->effectN( 2 ).percent();

      cinder = new dimensional_cinder_t( p );
    }

    void impact( action_state_t* s ) override
    {
      destruction_spell_t::impact( s );

      auto raw_damage = s->result_total;

      if ( p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T31, B2 ) )
      {
        cinder->base_dd_min = cinder->base_dd_max = raw_damage * p()->sets->set( WARLOCK_DESTRUCTION, T31, B2 )->effectN( 1 ).percent();
        cinder->execute_on_target( s->target );
      }
    }
  };

  struct shadow_barrage_t : public destruction_spell_t
  {
    shadow_barrage_t( warlock_t* p ) : destruction_spell_t( "Shadow Barrage", p, p->talents.shadow_barrage )
    {
      background = true;

      tick_action = new rift_shadow_bolt_t( p );
    }

    double last_tick_factor( const dot_t*, timespan_t, timespan_t ) const override
    {
      // 2022-10-16: Once you pass a haste breakpoint, you always get another full damage Shadow Bolt
      return 1.0;
    }
  };

  shadowy_tear_t( warlock_t* p ) : destruction_spell_t( "Shadowy Tear", p, p->talents.shadowy_tear_summon )
  {
    background = true;

    impact_action = new shadow_barrage_t( p );
  }
};

struct unstable_tear_t : public destruction_spell_t
{
  // TOCHECK: Partial ticks are not matching up in game!
  struct chaos_barrage_tick_t : public destruction_spell_t
  {
    dimensional_cinder_t* cinder;

    chaos_barrage_tick_t( warlock_t* p ) : destruction_spell_t( "Chaos Barrage (tick)", p, p->talents.chaos_barrage_tick )
    {
      destro_mastery = false;
      background = dual = true;

      // Though this behaves like a direct damage spell, it is also whitelisted under the periodic spec aura and benefits as such in game
      base_dd_multiplier *= 1.0 + p->warlock_base.destruction_warlock->effectN( 2 ).percent();

      cinder = new dimensional_cinder_t( p );
    }

    void impact( action_state_t* s ) override
    {
      destruction_spell_t::impact( s );

      auto raw_damage = s->result_total;

      if ( p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T31, B2 ) )
      {
        cinder->base_dd_min = cinder->base_dd_max = raw_damage * p()->sets->set( WARLOCK_DESTRUCTION, T31, B2 )->effectN( 1 ).percent();
        cinder->execute_on_target( s->target );
      }
    }
  };

  struct chaos_barrage_t : public destruction_spell_t
  {
    chaos_barrage_t( warlock_t* p ) : destruction_spell_t( "Chaos Barrage", p, p->talents.chaos_barrage )
    {
      background = true;

      tick_action = new chaos_barrage_tick_t( p );
    }
  };

  unstable_tear_t( warlock_t* p ) : destruction_spell_t( "Unstable Tear", p, p->talents.unstable_tear_summon )
  {
    background = true;

    impact_action = new chaos_barrage_t( p );
  }
};

struct chaos_tear_t : public destruction_spell_t
{
  struct rift_chaos_bolt_t : public destruction_spell_t
  {
    dimensional_cinder_t* cinder;

    rift_chaos_bolt_t( warlock_t* p ) : destruction_spell_t( "Rift Chaos Bolt", p, p->talents.rift_chaos_bolt )
    {
      destro_mastery = false;
      background = true;

      // Though this behaves like a direct damage spell, it is also whitelisted under the periodic spec aura and benefits as such in game
      base_dd_multiplier *= 1.0 + p->warlock_base.destruction_warlock->effectN( 2 ).percent();

      cinder = new dimensional_cinder_t( p );
    }

    double composite_crit_chance() const override
    {
      return 1.0;
    }

    double calculate_direct_amount( action_state_t* state ) const override
    {
      destruction_spell_t::calculate_direct_amount( state );

      state->result_total *= 1.0 + player->cache.spell_crit_chance();

      return state->result_total;
    }

    void impact( action_state_t* s ) override
    {
      destruction_spell_t::impact( s );

      auto raw_damage = s->result_total;

      if ( p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T31, B2 ) )
      {
        cinder->base_dd_min = cinder->base_dd_max = raw_damage * p()->sets->set( WARLOCK_DESTRUCTION, T31, B2 )->effectN( 1 ).percent();
        cinder->execute_on_target( s->target );
      }
    }
  };

  chaos_tear_t( warlock_t* p ) : destruction_spell_t( "Chaos Tear", p, p->talents.chaos_tear_summon )
  {
    background = true;

    impact_action = new rift_chaos_bolt_t( p );
  }
};

struct flame_rift_t : public destruction_spell_t
{
  struct searing_bolt_t : public destruction_spell_t
  {
    dimensional_cinder_t* cinder;

    searing_bolt_t( warlock_t* p ) : destruction_spell_t( "Searing Bolt", p, p->tier.searing_bolt )
    {
      destro_mastery = false;
      background = true;
      dot_behavior = dot_behavior_e::DOT_REFRESH_DURATION;

      // Though this behaves like a direct damage spell, it is also whitelisted under the periodic spec aura and benefits as such in game
      base_dd_multiplier *= 1.0 + p->warlock_base.destruction_warlock->effectN( 2 ).percent();

      cinder = new dimensional_cinder_t( p );
    }

    void impact( action_state_t* s ) override
    {
      destruction_spell_t::impact( s );

      auto raw_damage = s->result_total;

      if ( p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T31, B2 ) )
      {
        cinder->base_dd_min = cinder->base_dd_max = raw_damage * p()->sets->set( WARLOCK_DESTRUCTION, T31, B2 )->effectN( 1 ).percent();
        cinder->execute_on_target( s->target );
      }
    }
  };

  searing_bolt_t* bolt;

  flame_rift_t( warlock_t* p ) : destruction_spell_t( "Flame Rift", p, p->tier.flame_rift )
  {
    background = true;
    may_miss = false;

    bolt = new searing_bolt_t( p );
    add_child( bolt );
  }

  void execute() override
  {
    destruction_spell_t::execute();

    // Flame Rift fires 20 Searing Bolts erratically
    timespan_t min_delay = 0_ms;
    timespan_t max_delay = timespan_t::from_seconds( p()->sets->set( WARLOCK_DESTRUCTION, T31, B4 )->effectN( 1 ).base_value() );
    player_t* tar = target;

    for ( int i = 0; i < p()->sets->set( WARLOCK_DESTRUCTION, T31, B4 )->effectN( 1 ).base_value(); i++ )
    {
      timespan_t delay = rng().gauss( i * 500_ms, 500_ms );
      delay = std::min( std::max( delay, min_delay ), max_delay );
      make_event( *sim, delay, [ this, tar ] { this->bolt->execute_on_target( tar ); } );
    }
  }
};

struct dimensional_rift_t : public destruction_spell_t
{
  shadowy_tear_t* shadowy_tear;
  unstable_tear_t* unstable_tear;
  chaos_tear_t* chaos_tear;
  flame_rift_t* flame_rift;

  dimensional_rift_t( warlock_t* p, util::string_view options_str )
    : destruction_spell_t( "Dimensional Rift", p, p->talents.dimensional_rift )
  {
    parse_options( options_str );

    harmful = true;

    energize_type = action_energize::ON_CAST;
    energize_amount = p->talents.dimensional_rift->effectN( 2 ).base_value() / 10.0;

    shadowy_tear = new shadowy_tear_t( p );
    unstable_tear = new unstable_tear_t( p );
    chaos_tear = new chaos_tear_t( p );
    flame_rift = new flame_rift_t( p );

    add_child( shadowy_tear );
    add_child( unstable_tear );
    add_child( chaos_tear );
    add_child( flame_rift );
  }

  void execute() override
  {
    destruction_spell_t::execute();

    int rift = rng().range( p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T31, B4 ) ? 4 : 3 );

    switch ( rift )
    {
    case 0:
      shadowy_tear->execute_on_target( target );
      break;
    case 1:
      unstable_tear->execute_on_target( target );
      break;
    case 2:
      chaos_tear->execute_on_target( target );
      break;
    case 3:
      flame_rift->execute_on_target( target );
      break;
    default:
      break;
    }
  }
};

struct avatar_of_destruction_t : public destruction_spell_t
{
  struct infernal_awakening_proc_t : public destruction_spell_t
  {
    infernal_awakening_proc_t( warlock_t* p ) : destruction_spell_t( "Infernal Awakening (Blasphemy)", p, p->talents.infernal_awakening )
    {
      destro_mastery = false;
      aoe = -1;
      background = dual = true;
    }
  };

  infernal_awakening_proc_t* infernal_awakening;

  avatar_of_destruction_t( warlock_t* p ) : destruction_spell_t( "avatar_of_destruction", p, p->talents.summon_blasphemy )
  {
    background = dual = true;
    infernal_awakening = new infernal_awakening_proc_t( p );
  }

  void execute() override
  {
    destruction_spell_t::execute();

    if ( p()->warlock_pet_list.blasphemy.active_pet() )
    {
      p()->warlock_pet_list.blasphemy.active_pet()->adjust_duration( p()->talents.avatar_of_destruction->effectN( 1 ).time_value() * 1000 );
      p()->warlock_pet_list.blasphemy.active_pet()->blasphemous_existence->execute(); // Blasphemy only triggers this when extended
    }
    else
    {
      p()->warlock_pet_list.blasphemy.spawn( p()->talents.avatar_of_destruction->effectN( 1 ).time_value() * 1000 );
      infernal_awakening->execute_on_target( target ); // Initial Blasphemy summons trigger this damage from the Warlock
    }
  }
};

struct channel_demonfire_tier_t : public destruction_spell_t
{
  channel_demonfire_tier_t( warlock_t* p ) : destruction_spell_t( "Channel Demonfire (Tier)", p, p->tier.channel_demonfire )
  {
    background = dual = true;

    spell_power_mod.direct = p->tier.channel_demonfire->effectN( 1 ).sp_coeff();

    aoe = -1;
    base_aoe_multiplier = p->tier.channel_demonfire->effectN( 2 ).sp_coeff() / p->tier.channel_demonfire->effectN( 1 ).sp_coeff();

    travel_speed = p->talents.channel_demonfire_travel->missile_speed();
  }

  void impact( action_state_t* s ) override
  {
    destruction_spell_t::impact( s );

    // Raging Demonfire will adjust the time remaining on all targets hit by an AoE pulse
    if ( p()->talents.raging_demonfire->ok() && td( s->target )->dots_immolate->is_ticking() )
    {
      td( s->target )->dots_immolate->adjust_duration( p()->talents.raging_demonfire->effectN( 2 ).time_value() );
    }

    if ( s->chain_target == 0 && p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T30, B4 ) )
      p()->buffs.umbrafire_embers->trigger();
  }
};

}  // namespace actions_destruction

namespace buffs
{
}  // namespace buffs

// add actions
action_t* warlock_t::create_action_destruction( util::string_view action_name, util::string_view options_str )
{
  using namespace actions_destruction;

  if ( action_name == "conflagrate" )
    return new conflagrate_t( this, options_str );
  if ( action_name == "incinerate" )
    return new incinerate_t( this, options_str );
  if ( action_name == "immolate" )
    return new immolate_t( this, options_str );
  if ( action_name == "chaos_bolt" )
    return new chaos_bolt_t( this, options_str );
  if ( action_name == "rain_of_fire" )
    return new rain_of_fire_t( this, options_str );
  if ( action_name == "havoc" )
    return new havoc_t( this, options_str );
  if ( action_name == "summon_infernal" )
    return new summon_infernal_t( this, options_str );

  // Talents
  if ( action_name == "soul_fire" )
    return new soul_fire_t( this, options_str );
  if ( action_name == "shadowburn" )
    return new shadowburn_t( this, options_str );
  if ( action_name == "cataclysm" )
    return new cataclysm_t( this, options_str );
  if ( action_name == "channel_demonfire" )
    return new channel_demonfire_t( this, options_str );
  if ( action_name == "dimensional_rift" )
    return new dimensional_rift_t( this, options_str );

  return nullptr;
}

void warlock_t::create_destruction_proc_actions()
{
  proc_actions.avatar_of_destruction = new actions_destruction::avatar_of_destruction_t( this );
  proc_actions.channel_demonfire = new actions_destruction::channel_demonfire_tier_t( this );
}
}  // namespace warlock
