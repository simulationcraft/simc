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
    int base_cost = as<int>( destruction_spell_t::cost() ); // Power Overwhelming is ignoring any cost changes

    // The shard cost reduction from Crashing Chaos is "undone" for Impending Ruin stacking
    // This can be observed during the free Ritual of Ruin cast, which always increments by 1 stack regardless of spell
    // TOCHECK: Does this apply for Rain of Chaos draws?
    if ( !p()->min_version_check( VERSION_10_1_5 ) && p()->buffs.crashing_chaos->check() )
      shards_used -= as<int>( p()->buffs.crashing_chaos->check_value() );

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

    if ( p()->talents.power_overwhelming->ok() && resource_current == RESOURCE_SOUL_SHARD && base_cost > 0 )
    {
      p()->buffs.power_overwhelming->trigger( base_cost );
    }

    // 2022-10-17: Spell data is missing the % chance!
    // Need to test further, but chance appears independent of shard cost
    // Also procs even if the cast is free due to other effects
    if ( resource_current == RESOURCE_SOUL_SHARD && base_cost > 0 && p()->sets->has_set_bonus( WARLOCK_DESTRUCTION, T29, B2 ) && rng().roll( 0.2 ) )
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
  }

  double cost() const override
  {
    double c = destruction_spell_t::cost();

    if ( c > 0.0 && !p()->min_version_check(VERSION_10_1_5) && p()->talents.crashing_chaos->ok() )
      c += p()->buffs.crashing_chaos->check_value();

    return c;        
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

    if ( !p()->min_version_check( VERSION_10_1_5 ) )
      p()->buffs.crashing_chaos->decrement();
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

        increment_max *= std::pow( p()->get_active_dots( internal_id ), -2.0 / 3.0 );

        p()->cdf_accumulator += rng().range( 0.0, increment_max );

        if ( p()->cdf_accumulator >= 1.0 )
        {
          p()->proc_actions.channel_demonfire->execute_on_target( d->target );
          p()->procs.channel_demonfire->occur();
          p()->cdf_accumulator -= 1.0;
        }
      }
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

  double cost() const override
  {
    double c = destruction_spell_t::cost();

    if ( p()->buffs.ritual_of_ruin->check() )
      c *= 1.0 + p()->talents.ritual_of_ruin_buff->effectN( 2 ).percent();

    if ( c > 0.0 && !p()->min_version_check( VERSION_10_1_5 ) && p()->talents.crashing_chaos->ok() )
      c += p()->buffs.crashing_chaos->check_value();

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

    if ( p()->min_version_check( VERSION_10_1_5 ) && p()->buffs.crashing_chaos->check() )
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
      chaos_incarnate = p->talents.chaos_incarnate->ok();
    }

    void impact( action_state_t* s ) override
    {
      destruction_spell_t::impact( s );

      if ( p()->talents.inferno->ok() && result_is_hit( s->result ) )
      {
        if ( rng().roll( p()->talents.inferno->effectN( 1 ).percent() * ( 5.0 / std::max(5u, s->n_targets ) ) ) )
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

      if ( p()->min_version_check( VERSION_10_1_5 ) && p()->buffs.crashing_chaos->check() )
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

  double cost() const override
  {
    double c = destruction_spell_t::cost();

    if ( p()->buffs.ritual_of_ruin->check() )
      c *= 1.0 + p()->talents.ritual_of_ruin_buff->effectN( 5 ).percent();
    
    if ( c > 0.0 && !p()->min_version_check( VERSION_10_1_5 ) && p()->talents.crashing_chaos->ok() )
      c += p()->buffs.crashing_chaos->check_value();

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
  int immolate_action_id;

  channel_demonfire_t( warlock_t* p, util::string_view options_str )
    : destruction_spell_t( "Channel Demonfire", p, p->talents.channel_demonfire ),
      channel_demonfire( new channel_demonfire_tick_t( p ) ),
      immolate_action_id( 0 )
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
    immolate_action_id = p()->find_action_id( "immolate" );
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
    double active_immolates = p()->get_active_dots( immolate_action_id );

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

// Dimensional Rift's portals are "pet damage" according to combat log behavior, but appear to be benefitting from
// buffs to the *Warlock's* damage specifically (i.e. Grimoire of Synergy). For now, we will model them as Warlock spells

struct shadowy_tear_t : public destruction_spell_t
{
  struct rift_shadow_bolt_t : public destruction_spell_t
  {
    rift_shadow_bolt_t( warlock_t* p ) : destruction_spell_t( "Rift Shadow Bolt", p, p->talents.rift_shadow_bolt )
    {
      destro_mastery = false;
      background = dual = true;

      // Though this behaves like a direct damage spell, it is also whitelisted under the periodic spec aura and benefits as such in game
      base_dd_multiplier *= 1.0 + p->warlock_base.destruction_warlock->effectN( 2 ).percent(); 
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
    chaos_barrage_tick_t( warlock_t* p ) : destruction_spell_t( "Chaos Barrage (tick)", p, p->talents.chaos_barrage_tick )
    {
      destro_mastery = false;
      background = dual = true;

      // Though this behaves like a direct damage spell, it is also whitelisted under the periodic spec aura and benefits as such in game
      base_dd_multiplier *= 1.0 + p->warlock_base.destruction_warlock->effectN( 2 ).percent(); 
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
    rift_chaos_bolt_t( warlock_t* p ) : destruction_spell_t( "Rift Chaos Bolt", p, p->talents.rift_chaos_bolt )
    {
      destro_mastery = false;
      background = true;

      // Though this behaves like a direct damage spell, it is also whitelisted under the periodic spec aura and benefits as such in game
      base_dd_multiplier *= 1.0 + p->warlock_base.destruction_warlock->effectN( 2 ).percent(); 
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
  };

  chaos_tear_t( warlock_t* p ) : destruction_spell_t( "Chaos Tear", p, p->talents.chaos_tear_summon )
  {
    background = true;

    impact_action = new rift_chaos_bolt_t( p );
  }
};

struct dimensional_rift_t : public destruction_spell_t
{
  shadowy_tear_t* shadowy_tear;
  unstable_tear_t* unstable_tear;
  chaos_tear_t* chaos_tear;

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

    add_child( shadowy_tear );
    add_child( unstable_tear );
    add_child( chaos_tear );
  }

  void execute() override
  {
    destruction_spell_t::execute();

    // TOCHECK: Are all rift types equally likely?
    int rift = rng().range( 3 );

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
void warlock_t::create_buffs_destruction()
{
  // destruction buffs
  buffs.backdraft = make_buff( this, "backdraft", talents.backdraft_buff );

  buffs.reverse_entropy = make_buff( this, "reverse_entropy", talents.reverse_entropy_buff )
                              ->set_default_value_from_effect( 1 )
                              ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                              ->set_trigger_spell( talents.reverse_entropy )
                              ->set_rppm( RPPM_NONE, talents.reverse_entropy->real_ppm() );

  buffs.rain_of_chaos = make_buff( this, "rain_of_chaos", talents.rain_of_chaos_buff );

  buffs.impending_ruin = make_buff ( this, "impending_ruin", talents.impending_ruin_buff )
                             ->set_max_stack( talents.impending_ruin_buff->max_stacks() + as<int>( talents.master_ritualist->effectN( 2 ).base_value() ) )
                             ->set_stack_change_callback( [ this ]( buff_t* b, int, int cur )
                               {
                                 if ( cur == b->max_stack() )
                                 {
                                   make_event( sim, 0_ms, [ this, b ] { 
                                     buffs.ritual_of_ruin->trigger();
                                     b->expire();
                                     });
                                 };
                               });

  buffs.ritual_of_ruin = make_buff ( this, "ritual_of_ruin", talents.ritual_of_ruin_buff );

  buffs.conflagration_of_chaos_cf = make_buff( this, "conflagration_of_chaos_cf", talents.conflagration_of_chaos_cf )
                                        ->set_default_value_from_effect( 1 );

  buffs.conflagration_of_chaos_sb = make_buff( this, "conflagration_of_chaos_sb", talents.conflagration_of_chaos_sb )
                                        ->set_default_value_from_effect( 1 );

  buffs.flashpoint = make_buff( this, "flashpoint", talents.flashpoint_buff )
                         ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                         ->set_default_value( talents.flashpoint->effectN( 1 ).percent() );

  if ( min_version_check( VERSION_10_1_5 ) )
  {
    buffs.crashing_chaos = make_buff( this, "crashing_chaos", talents.crashing_chaos_buff )
                               ->set_max_stack( std::max( as<int>( talents.crashing_chaos->effectN( 3 ).base_value() ), 1 ) )
                               ->set_reverse( true );
  }
  else
  {
    buffs.crashing_chaos = make_buff( this, "crashing_chaos", talents.crashing_chaos_buff )
                               ->set_max_stack( std::max( as<int>( talents.crashing_chaos->effectN( 1 ).base_value() ), 1 ) )
                               ->set_reverse( true )
                               ->set_default_value( talents.crashing_chaos_buff->effectN( 1 ).base_value() / 10.0 );
  }


  buffs.power_overwhelming = make_buff( this, "power_overwhelming", talents.power_overwhelming_buff )
                                 ->set_pct_buff_type( STAT_PCT_BUFF_MASTERY )
                                 ->set_default_value( talents.power_overwhelming->effectN( 2 ).base_value() / 10.0 )
                                 ->set_refresh_behavior( buff_refresh_behavior::DISABLED );

  buffs.madness_cb = make_buff( this, "madness_cb", talents.madness_cb )
                         ->set_default_value( talents.madness_of_the_azjaqir->effectN( 1 ).percent() );

  buffs.madness_rof = make_buff( this, "madness_rof", talents.madness_rof )
                          ->set_default_value( talents.madness_of_the_azjaqir->effectN( 1 ).percent() );

  buffs.madness_sb = make_buff( this, "madness_sb", talents.madness_sb )
                         ->set_default_value( talents.madness_of_the_azjaqir->effectN( 1 ).percent() );

  buffs.madness_rof_snapshot = make_buff( this, "madness_rof_snapshot" );

  buffs.burn_to_ashes = make_buff( this, "burn_to_ashes", talents.burn_to_ashes_buff )
                            ->set_default_value( talents.burn_to_ashes->effectN( 1 ).percent() );

  buffs.chaos_maelstrom = make_buff( this, "chaos_maelstrom", tier.chaos_maelstrom )
                              ->set_pct_buff_type( STAT_PCT_BUFF_CRIT )
                              ->set_default_value_from_effect( 1 );

  buffs.umbrafire_embers = make_buff( this, "umbrafire_embers", tier.umbrafire_embers )
                               ->set_default_value_from_effect( 1 )
                               ->set_refresh_behavior( buff_refresh_behavior::DISABLED );
}
void warlock_t::init_spells_destruction()
{
  using namespace actions_destruction;

  // Talents
  talents.chaos_bolt = find_talent_spell( talent_tree::SPECIALIZATION, "Chaos Bolt" ); // Should be ID 116858

  talents.conflagrate = find_talent_spell( talent_tree::SPECIALIZATION, "Conflagrate" ); // Should be ID 17962
  talents.conflagrate_2 = find_spell( 245330 );

  talents.reverse_entropy = find_talent_spell( talent_tree::SPECIALIZATION, "Reverse Entropy" ); // Should be ID 205148
  talents.reverse_entropy_buff = find_spell( 266030 );

  talents.internal_combustion = find_talent_spell( talent_tree::SPECIALIZATION, "Internal Combustion" ); // Should be ID 266134

  talents.rain_of_fire = find_talent_spell( talent_tree::SPECIALIZATION, "Rain of Fire" ); // Should be ID 5740
  talents.rain_of_fire_tick = find_spell( 42223 );

  talents.backdraft = find_talent_spell( talent_tree::SPECIALIZATION, "Backdraft" ); // Should be ID 196406
  talents.backdraft_buff = find_spell( 117828 );

  talents.mayhem = find_talent_spell( talent_tree::SPECIALIZATION, "Mayhem" ); // Should be ID 387506

  talents.pyrogenics = find_talent_spell( talent_tree::SPECIALIZATION, "Pyrogenics" ); // Should be ID 387095
  talents.pyrogenics_debuff = find_spell( 387096 );

  talents.roaring_blaze = find_talent_spell( talent_tree::SPECIALIZATION, "Roaring Blaze" ); // Should be ID 205184
  talents.conflagrate_debuff = find_spell( 265931 );

  talents.improved_conflagrate = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Conflagrate" ); // Should be ID 231793

  talents.explosive_potential = find_talent_spell( talent_tree::SPECIALIZATION, "Explosive Potential" ); // Should be ID 388827

  talents.channel_demonfire = find_talent_spell( talent_tree::SPECIALIZATION, "Channel Demonfire" ); // Should be ID 196447
  talents.channel_demonfire_tick = find_spell( 196448 ); // Includes both direct and splash damage values
  talents.channel_demonfire_travel = find_spell( 196449 );

  talents.pandemonium = find_talent_spell( talent_tree::SPECIALIZATION, "Pandemonium" ); // Should be ID 387509

  talents.cry_havoc = find_talent_spell( talent_tree::SPECIALIZATION, "Cry Havoc" ); // Should be ID 387522
  talents.cry_havoc_proc = find_spell( 387547 );

  talents.improved_immolate = find_talent_spell( talent_tree::SPECIALIZATION, "Improved Immolate" ); // Should be ID 387093

  talents.inferno = find_talent_spell( talent_tree::SPECIALIZATION, "Inferno" ); // Should be ID 270545

  talents.cataclysm = find_talent_spell( talent_tree::SPECIALIZATION, "Cataclysm" ); // Should be ID 152108

  talents.soul_fire = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Fire" ); // Should be ID 6353
  talents.soul_fire_2 = find_spell( 281490 );
  
  talents.shadowburn = find_talent_spell( talent_tree::SPECIALIZATION, "Shadowburn" ); // Should be ID 17877
  talents.shadowburn_2 = find_spell( 245731 );

  talents.raging_demonfire = find_talent_spell( talent_tree::SPECIALIZATION, "Raging Demonfire" ); // Should be ID 387166

  talents.rolling_havoc = find_talent_spell( talent_tree::SPECIALIZATION, "Rolling Havoc" ); // Should be ID 387569
  talents.rolling_havoc_buff = find_spell( 387570 );

  talents.backlash = find_talent_spell( talent_tree::SPECIALIZATION, "Backlash" ); // Should be ID 387384

  talents.fire_and_brimstone = find_talent_spell( talent_tree::SPECIALIZATION, "Fire and Brimstone" ); // Should be ID 196408

  talents.decimation = find_talent_spell( talent_tree::SPECIALIZATION, "Decimation" ); // Should be ID 387176

  talents.conflagration_of_chaos = find_talent_spell( talent_tree::SPECIALIZATION, "Conflagration of Chaos" ); // Should be ID 387108
  talents.conflagration_of_chaos_cf = find_spell( 387109 );
  talents.conflagration_of_chaos_sb = find_spell( 387110 );

  talents.flashpoint = find_talent_spell( talent_tree::SPECIALIZATION, "Flashpoint" ); // Should be 387259
  talents.flashpoint_buff = find_spell( 387263 );

  talents.scalding_flames = find_talent_spell( talent_tree::SPECIALIZATION, "Scalding Flames" ); // Should be ID 388832

  talents.ruin = find_talent_spell( talent_tree::SPECIALIZATION, "Ruin" ); // Should be ID 387103

  talents.eradication = find_talent_spell( talent_tree::SPECIALIZATION, "Eradication" ); // Should be ID 196412
  talents.eradication_debuff = find_spell( 196414 );

  talents.ashen_remains = find_talent_spell( talent_tree::SPECIALIZATION, "Ashen Remains" ); // Should be ID 387252

  talents.summon_infernal = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Infernal" ); // Should be ID 1122
  talents.summon_infernal_main = find_spell( 111685 );
  talents.infernal_awakening = find_spell( 22703 );

  talents.diabolic_embers = find_talent_spell( talent_tree::SPECIALIZATION, "Diabolic Embers" ); // Should be ID 387173

  talents.ritual_of_ruin = find_talent_spell( talent_tree::SPECIALIZATION, "Ritual of Ruin" ); // Should be ID 387156
  talents.impending_ruin_buff = find_spell( 387158 );
  talents.ritual_of_ruin_buff = find_spell( 387157 );

  talents.crashing_chaos = find_talent_spell( talent_tree::SPECIALIZATION, "Crashing Chaos" ); // Should be ID 387355
  talents.crashing_chaos_buff = find_spell( min_version_check( VERSION_10_1_5 ) ? 417282 : 387356 );

  talents.infernal_brand = find_talent_spell( talent_tree::SPECIALIZATION, "Infernal Brand" ); // Should be ID 387475

  talents.power_overwhelming = find_talent_spell( talent_tree::SPECIALIZATION, "Power Overwhelming" ); // Should be ID 387279
  talents.power_overwhelming_buff = find_spell( 387283 );

  talents.madness_of_the_azjaqir = find_talent_spell( talent_tree::SPECIALIZATION, "Madness of the Azj'Aqir" ); // Should be ID 387400
  talents.madness_cb = find_spell( 387409 );
  talents.madness_rof = find_spell( 387413 );
  talents.madness_sb = find_spell( 387414 );

  talents.master_ritualist = find_talent_spell( talent_tree::SPECIALIZATION, "Master Ritualist" ); // Should be ID 387165

  talents.burn_to_ashes = find_talent_spell( talent_tree::SPECIALIZATION, "Burn to Ashes" ); // Should be ID 387153
  talents.burn_to_ashes_buff = find_spell( 387154 );

  talents.rain_of_chaos = find_talent_spell( talent_tree::SPECIALIZATION, "Rain of Chaos" ); // Should be ID 266086
  talents.rain_of_chaos_buff = find_spell( 266087 );
  talents.summon_infernal_roc = find_spell( 335236 );

  talents.chaos_incarnate = find_talent_spell( talent_tree::SPECIALIZATION, "Chaos Incarnate" ); // Should be ID 387275

  talents.dimensional_rift = find_talent_spell( talent_tree::SPECIALIZATION, "Dimensional Rift" ); // Should be ID 387976
  talents.shadowy_tear_summon = find_spell( 394235 );
  talents.shadow_barrage = find_spell( 394237 );
  talents.rift_shadow_bolt = find_spell( 394238 );
  talents.unstable_tear_summon = find_spell( 387979 );
  talents.chaos_barrage = find_spell( 387984 );
  talents.chaos_barrage_tick = find_spell( 387985 );
  talents.chaos_tear_summon = find_spell( 394243 );
  talents.rift_chaos_bolt = find_spell( 394246 );

  talents.avatar_of_destruction = find_talent_spell( talent_tree::SPECIALIZATION, "Avatar of Destruction" ); // Should be ID 387159
  talents.summon_blasphemy = find_spell( 387160 );

  // Additional Tier Set spell data

  // T29 (Vault of the Incarnates)
  tier.chaos_maelstrom = find_spell( 394679 );

  // T30 (Aberrus, the Shadowed Crucible)
  tier.channel_demonfire = find_spell( 409890 );
  tier.umbrafire_embers = find_spell( 409652 );

  // Proc action initialization
  proc_actions.avatar_of_destruction = new avatar_of_destruction_t( this );
  proc_actions.channel_demonfire = new channel_demonfire_tier_t( this );
}

void warlock_t::init_gains_destruction()
{
  gains.immolate = get_gain( "immolate" );
  gains.immolate_crits = get_gain( "immolate_crits" );
  gains.incinerate_crits = get_gain( "incinerate_crits" );
  gains.incinerate_fnb_crits = get_gain( "incinerate_fnb_crits" );
  gains.infernal = get_gain( "infernal" );
  gains.shadowburn_refund = get_gain( "shadowburn_refund" );
  gains.inferno = get_gain( "inferno" );
}

void warlock_t::init_rng_destruction()
{
  // TOCHECK: 15% chance is what is listed in spell data but during SL this was presumed to use deck of cards at 3 out of 20
  // May need rechecking in DF
  rain_of_chaos_rng = get_shuffled_rng( "rain_of_chaos", 3, 20 );
}

void warlock_t::init_procs_destruction()
{
  procs.reverse_entropy = get_proc( "reverse_entropy" );
  procs.rain_of_chaos = get_proc( "rain_of_chaos" );
  procs.chaos_maelstrom = get_proc( "chaos_maelstrom" );
  procs.channel_demonfire = get_proc( "channel_demonfire_tier" );
}

}  // namespace warlock
