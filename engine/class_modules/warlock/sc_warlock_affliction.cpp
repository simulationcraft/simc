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
  affliction_spell_t( util::string_view n, warlock_t* p, const spell_data_t* s = spell_data_t::nil() )
    : warlock_spell_t( n, p, s )
  {  }

  void init() override
  {
    warlock_spell_t::init();

    if ( p()->talents.creeping_death->ok() )
    {
      if ( data().affected_by( p()->talents.creeping_death->effectN( 1 ) ) )
        base_tick_time *= 1.0 + p()->talents.creeping_death->effectN( 1 ).percent();
    }
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double pm = warlock_spell_t::composite_da_multiplier( s );

    if ( this->data().affected_by( p()->warlock_base.potent_afflictions->effectN( 2 ) ) )
    {
      pm *= 1.0 + p()->cache.mastery_value();
    }
    return pm;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double pm = warlock_spell_t::composite_ta_multiplier( s );

    if ( this->data().affected_by( p()->warlock_base.potent_afflictions->effectN( 1 ) ) )
    {
      pm *= 1.0 + p()->cache.mastery_value();
    }

    return pm;
  }
};

struct agony_t : public affliction_spell_t
{
  agony_t( warlock_t* p, util::string_view options_str ) : affliction_spell_t( "Agony", p, p->warlock_base.agony )
  {
    parse_options( options_str );
    may_crit = false;

    dot_max_stack = as<int>( data().max_stacks() + p->warlock_base.agony_2->effectN( 1 ).base_value() );
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
    dot_max_stack += as<int>( p()->talents.writhe_in_agony->effectN( 1 ).base_value() );

    affliction_spell_t::init();
  }

  void execute() override
  {
    affliction_spell_t::execute();

    if ( p()->talents.writhe_in_agony->ok() )
    {
      int delta = (int)( p()->talents.writhe_in_agony->effectN( 3 ).base_value() ) - td( execute_state->target )->dots_agony->current_stack();

      if ( delta > 0 )
        td( execute_state->target )->dots_agony->increment( delta );
    }
  }

  void impact( action_state_t* s ) override
  {
    bool pi_trigger = p()->talents.pandemic_invocation->ok() && td( s->target )->dots_agony->is_ticking()
      && td( s->target )->dots_agony->remains() < p()->talents.pandemic_invocation->effectN( 1 ).time_value();

    affliction_spell_t::impact( s );

    if ( pi_trigger )
      p()->proc_actions.pandemic_invocation_proc->execute_on_target( s->target );
  }

  void tick( dot_t* d ) override
  {
    // Blizzard has not publicly released the formula for Agony's chance to generate a Soul Shard.
    // This set of code is based on results from 500+ Soul Shard sample sizes, and matches in-game
    // results to within 0.1% of accuracy in all tests conducted on all targets numbers up to 8.
    // Accurate as of 08-24-2018. TOCHECK regularly. If any changes are made to this section of
    // code, please also update the Time_to_Shard expression in sc_warlock.cpp.
    double increment_max = 0.368;

    double active_agonies = p()->get_active_dots( internal_id );
    increment_max *= std::pow( active_agonies, -2.0 / 3.0 );

    p()->agony_accumulator += rng().range( 0.0, increment_max );

    if ( p()->agony_accumulator >= 1 )
    {
      p()->resource_gain( RESOURCE_SOUL_SHARD, 1.0, p()->gains.agony );
      p()->agony_accumulator -= 1.0;

      // TOCHECK 2022-10-17: % chance for the tier bonus is not in spell data
      // There may also be some sneaky bad luck protection. Further testing needed.
      if ( p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T29, B2 ) && rng().roll( 0.3 ) )
      {
        p()->buffs.cruel_inspiration->trigger();
        p()->procs.cruel_inspiration->occur();

        if ( p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T29, B4 ) )
          p()->buffs.cruel_epiphany->trigger( 2 );
      }
    }

    if ( result_is_hit( d->state->result ) && p()->talents.inevitable_demise->ok() && !p()->buffs.drain_life->check() )
    {
      p()->buffs.inevitable_demise->trigger();
    }

    affliction_spell_t::tick( d );

    td( d->state->target )->dots_agony->increment( 1 );
  }
};

struct unstable_affliction_t : public affliction_spell_t
{
  unstable_affliction_t( warlock_t* p, util::string_view options_str )
    : affliction_spell_t( "Unstable Affliction", p, p->talents.unstable_affliction )
  {
    parse_options( options_str );

    dot_duration += p->talents.unstable_affliction_3->effectN( 1 ).time_value();
  }

  unstable_affliction_t( warlock_t* p )
    : affliction_spell_t( "Unstable Affliction", p, p->talents.soul_swap_ua )
  {
    dot_duration += p->talents.unstable_affliction_3->effectN( 1 ).time_value();
  }

  void execute() override
  {
    if ( p()->ua_target && p()->ua_target != target )
    {
      td( p()->ua_target )->dots_unstable_affliction->cancel();
      p()->buffs.malefic_affliction->expire();
    }

    p()->ua_target = target;

    affliction_spell_t::execute();
  }

  void impact( action_state_t* s ) override
  {
    bool pi_trigger = p()->talents.pandemic_invocation->ok() && td( s->target )->dots_unstable_affliction->is_ticking()
      && td( s->target )->dots_unstable_affliction->remains() < p()->talents.pandemic_invocation->effectN( 1 ).time_value();

    affliction_spell_t::impact( s );

    if ( pi_trigger )
      p()->proc_actions.pandemic_invocation_proc->execute_on_target( s->target );
  }

  void tick( dot_t* d ) override
  {
    affliction_spell_t::tick( d );

    if ( p()->talents.souleaters_gluttony->ok() )
    {
      timespan_t adjustment = timespan_t::from_seconds( p()->talents.souleaters_gluttony->effectN( 1 ).base_value() );
      adjustment = adjustment / p()->talents.souleaters_gluttony->effectN( 2 ).base_value();
      p()->cooldowns.soul_rot->adjust( -adjustment );
    }
  }

  void last_tick( dot_t* d) override
  {
    affliction_spell_t::last_tick( d );

    p()->buffs.malefic_affliction->expire();
    p()->ua_target = nullptr;
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double m = affliction_spell_t::composite_ta_multiplier( s );

    if ( p()->talents.malefic_affliction->ok() && p()->buffs.malefic_affliction->check() )
      m *= 1.0 + p()->buffs.malefic_affliction->check_stack_value();

    return m;
  }
};

struct summon_darkglare_t : public affliction_spell_t
{
  summon_darkglare_t( warlock_t* p, util::string_view options_str )
    : affliction_spell_t( "Summon Darkglare", p, p->talents.summon_darkglare )
  {
    parse_options( options_str );
    harmful = true; // This is currently set to true specifically for the 10.1 class trinket
    callbacks = true; // 2023-04-22 This was recently modified to false in spell data but we need it to be true for 10.1 trinket
    may_crit = may_miss = false;
  }

  void execute() override
  {
    affliction_spell_t::execute();

    timespan_t summon_duration = p()->talents.summon_darkglare->duration();

    if ( p()->talents.malevolent_visionary->ok() )
    {
      summon_duration += p()->talents.malevolent_visionary->effectN( 2 ).time_value();
    }

    p()->warlock_pet_list.darkglares.spawn( summon_duration );

    timespan_t darkglare_extension = timespan_t::from_seconds( p()->talents.summon_darkglare->effectN( 2 ).base_value() );

    p()->darkglare_extension_helper( p(), darkglare_extension );

    p()->buffs.soul_rot->extend_duration( p(), darkglare_extension ); // This dummy buff is active while Soul Rot is ticking
  }
};

struct malefic_rapture_t : public affliction_spell_t
{
  struct malefic_rapture_damage_t : public affliction_spell_t
  {
    malefic_rapture_damage_t( warlock_t* p )
      : affliction_spell_t( "Malefic Rapture (hit)", p, p->talents.malefic_rapture_dmg )
    {
      background = dual = true;
      spell_power_mod.direct = p->talents.malefic_rapture->effectN( 1 ).sp_coeff();
      callbacks = false; // Malefic Rapture did not proc Psyche Shredder, it may not cause any procs at all
    }

    double composite_da_multiplier( const action_state_t* s ) const override
    {
      double m = affliction_spell_t::composite_da_multiplier( s );

      m *= p()->get_target_data( s->target )->count_affliction_dots();

      m *= 1.0 + p()->buffs.cruel_epiphany->check_value();

      if ( p()->talents.focused_malignancy->ok() && td( s->target )->dots_unstable_affliction->is_ticking() )
        m *= 1.0 + p()->talents.focused_malignancy->effectN( 1 ).percent();

      return m;
    }

    void impact( action_state_t* s ) override
    {
      affliction_spell_t::impact( s );

      auto target_data = td( s->target );

      if ( p()->talents.dread_touch->ok() && p()->min_version_check( VERSION_10_1_5 ) )
      {
        if ( target_data->dots_unstable_affliction->is_ticking() )
          target_data->debuffs_dread_touch->trigger();
      }
      else if ( p()->talents.malefic_affliction->ok() && target_data->dots_unstable_affliction->is_ticking() )
      {
        if ( ( p()->talents.dread_touch->ok() &&
               p()->buffs.malefic_affliction->check() >= (int)p()->talents.malefic_affliction_buff->max_stacks() ) )
          target_data->debuffs_dread_touch->trigger();

        p()->buffs.malefic_affliction->trigger();
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
    : affliction_spell_t( "Malefic Rapture", p, p->talents.malefic_rapture )
  {
    parse_options( options_str );
    aoe = -1;

    impact_action = new malefic_rapture_damage_t( p );
    add_child( impact_action );
  }

  double cost() const override
  {
    double c = affliction_spell_t::cost();

    if ( p()->buffs.tormented_crescendo->check() )
      c *= 1.0 + p()->talents.tormented_crescendo_buff->effectN( 4 ).percent();

    return c;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = affliction_spell_t::execute_time();

    if ( p()->buffs.tormented_crescendo->check() )
      t *= 1.0 + p()->talents.tormented_crescendo_buff->effectN( 3 ).percent();

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

    p()->buffs.tormented_crescendo->decrement();
    p()->buffs.cruel_epiphany->decrement();
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    affliction_spell_t::available_targets( tl );

    tl.erase( std::remove_if( tl.begin(), tl.end(), [ this ]( player_t* target ) { return p()->get_target_data( target )->count_affliction_dots() == 0; } ), tl.end() );

    return tl.size();
  }
};

struct drain_soul_t : public affliction_spell_t
{
  struct drain_soul_state_t : public action_state_t
  {
    double tick_time_multiplier;

    drain_soul_state_t( action_t* action, player_t* target )
      : action_state_t( action, target ),
      tick_time_multiplier( 1.0 )
    { }

    void initialize() override
    {
      action_state_t::initialize();
      tick_time_multiplier = 1.0;
    }

    std::ostringstream& debug_str( std::ostringstream& s ) override
    {
      action_state_t::debug_str( s ) << " tick_time_multiplier=" << tick_time_multiplier;
      return s;
    }

    void copy_state( const action_state_t* s ) override
    {
      action_state_t::copy_state( s );
      tick_time_multiplier = debug_cast<const drain_soul_state_t*>( s )->tick_time_multiplier;
    }
  };

  drain_soul_t( warlock_t* p, util::string_view options_str )
    : affliction_spell_t( "Drain Soul", p, p->talents.drain_soul->ok() ? p->talents.drain_soul_dot : spell_data_t::not_found() )
  {
    parse_options( options_str );
    channeled = true;
  }

  action_state_t* new_state() override
  { return new drain_soul_state_t( this, target ); }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    debug_cast<drain_soul_state_t*>( s )->tick_time_multiplier = 1.0 + ( p()->buffs.nightfall->check() ? p()->talents.nightfall_buff->effectN( 3 ).percent() : 0 );
    affliction_spell_t::snapshot_state( s, rt );
  }

  timespan_t tick_time( const action_state_t* s ) const override
  {
    timespan_t t = affliction_spell_t::tick_time( s );

    t *= debug_cast<const drain_soul_state_t*>( s )->tick_time_multiplier;

    return t;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    timespan_t dur = dot_duration * s->haste;

    if ( p()->buffs.nightfall->check() )
      dur *= 1.0 + p()->talents.nightfall_buff->effectN( 4 ).percent();

    return dur;
  }

  void execute() override
  {
    affliction_spell_t::execute();

    p()->buffs.nightfall->decrement();
  }

  void tick( dot_t* d ) override
  {
    affliction_spell_t::tick( d );

    if ( result_is_hit( d->state->result ) )
    {
      if ( p()->talents.shadow_embrace->ok() )
        td( d->target )->debuffs_shadow_embrace->trigger();

      if ( p()->talents.tormented_crescendo->ok() )
      {
        if ( p()->crescendo_check( p() ) && rng().roll( p()->talents.tormented_crescendo->effectN( 2 ).percent() ) )
        {
          p()->procs.tormented_crescendo->occur();
          p()->buffs.tormented_crescendo->trigger();
        }
      }
    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = affliction_spell_t::composite_target_multiplier( t );

    if ( t->health_percentage() < p()->talents.drain_soul_dot->effectN( 3 ).base_value() )
      m *= 1.0 + p()->talents.drain_soul_dot->effectN( 2 ).percent();

    if ( p()->talents.withering_bolt->ok() )
      m *= 1.0 + p()->talents.withering_bolt->effectN( 1 ).percent() * std::min( (int)p()->talents.withering_bolt->effectN( 2 ).base_value(), p()->get_target_data( t )->count_affliction_dots() );

    return m;
  }
};

struct haunt_t : public affliction_spell_t
{
  haunt_t( warlock_t* p, util::string_view options_str ) : affliction_spell_t( "Haunt", p, p->talents.haunt )
  {
    parse_options( options_str );

    if ( p->talents.seized_vitality->ok() )
      base_dd_multiplier *= 1.0 + p->talents.seized_vitality->effectN( 1 ).percent();
  }

  void impact( action_state_t* s ) override
  {
    affliction_spell_t::impact( s );

    if ( result_is_hit( s->result ) )
    {
      td( s->target )->debuffs_haunt->trigger();

      if ( p()->talents.shadow_embrace->ok() )
        td( s->target )->debuffs_shadow_embrace->trigger();
    }
  }
};

struct siphon_life_t : public affliction_spell_t
{
  siphon_life_t( warlock_t* p, util::string_view options_str )
    : affliction_spell_t( "Siphon Life", p, p->talents.siphon_life )
  {
    parse_options( options_str );
  }
  
  void impact( action_state_t* s ) override
  {
    bool pi_trigger = p()->talents.pandemic_invocation->ok() && td( s->target )->dots_siphon_life->is_ticking()
      && td( s->target )->dots_siphon_life->remains() < p()->talents.pandemic_invocation->effectN( 1 ).time_value();

    affliction_spell_t::impact( s );

    if ( pi_trigger )
      p()->proc_actions.pandemic_invocation_proc->execute_on_target( s->target );
  }
};

struct phantom_singularity_t : public affliction_spell_t
{
  struct phantom_singularity_tick_t : public affliction_spell_t
  {
    phantom_singularity_tick_t( warlock_t* p )
      : affliction_spell_t( "Phantom Singularity (tick)", p, p->talents.phantom_singularity_tick )
    {
      background = dual = true;
      may_miss = false;
      aoe = -1;

      if ( p->sets->has_set_bonus( WARLOCK_AFFLICTION, T30, B2 ) )
        base_dd_multiplier *= 1.0 + p->sets->set( WARLOCK_AFFLICTION, T30, B2 )->effectN( 3 ).percent();
    }

    void impact( action_state_t* s ) override
    {
      affliction_spell_t::impact( s );

      if ( s->chain_target == 0 && !p()->min_version_check(VERSION_10_1_5) && p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T30, B4 ) )
      {
        // Debuff lasts 2 seconds but refreshes on every tick. 2023-04-04 PTR: Currently only applies to target with PS DoT
        td( s->target )->debuffs_infirmity->trigger();
      }
    }
  };
  
  phantom_singularity_t( warlock_t* p, util::string_view options_str )
    : affliction_spell_t( "Phantom Singularity", p, p->talents.phantom_singularity )
  {
    parse_options( options_str );
    callbacks = false;
    hasted_ticks = true;
    tick_action = new phantom_singularity_tick_t( p );

    spell_power_mod.tick = 0;

    if ( p->sets->has_set_bonus( WARLOCK_AFFLICTION, T30, B2 ) )
      cooldown->duration += p->sets->set( WARLOCK_AFFLICTION, T30, B2 )->effectN( 2 ).time_value();
  }

  void init() override
  {
    affliction_spell_t::init();

    update_flags &= ~STATE_HASTE;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return ( s->action->tick_time( s ) / base_tick_time ) * dot_duration ;
  }

  // 2023-07-06 PTR: PS applying Infirmity is currently EXTREMELY bugged, doing such things as applying to
  // player's current focused target rather than the enemy the DoT is ticking on. There are also conflicting
  // behaviors regarding whether it applies a variable duration per-tick (matching the remaining duration of PS)
  // or a single infinite duration on application (like Vile Taint). Implementing like Vile Taint for now
  // TOCHECK: A week or two after 10.1.5 goes live, verify if fixed
  void impact( action_state_t* s ) override
  {
    affliction_spell_t::impact( s );

    if ( p()->min_version_check( VERSION_10_1_5 ) && p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T30, B4 ) )
    {
      td( s->target )->debuffs_infirmity->trigger();
    }
  }

  void last_tick( dot_t* d ) override
  {
    affliction_spell_t::last_tick( d );

    if ( p()->min_version_check( VERSION_10_1_5 ) && p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T30, B4 ) )
    {
      td( d->target )->debuffs_infirmity->expire();
    }
  }
};

struct vile_taint_t : public affliction_spell_t
{
  struct vile_taint_dot_t : public affliction_spell_t
  {
    vile_taint_dot_t( warlock_t* p ) : affliction_spell_t( "Vile Taint (DoT)", p, p->talents.vile_taint_dot )
    {
      tick_zero = background = true;
      execute_action = new agony_t( p, "" );
      execute_action->background = true;
      execute_action->dual = true;
      execute_action->base_costs[ RESOURCE_MANA ] = 0.0;

      if ( p->sets->has_set_bonus( WARLOCK_AFFLICTION, T30, B2 ) )
        base_td_multiplier *= 1.0 + p->sets->set( WARLOCK_AFFLICTION, T30, B2 )->effectN( 4 ).percent();
    }

    void tick( dot_t* d ) override
    {
      affliction_spell_t::tick( d );

      if ( !p()->min_version_check( VERSION_10_1_5 ) && p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T30, B4 ) )
      {
        td( d->target )->debuffs_infirmity->trigger();
      }
    }

    void last_tick( dot_t* d ) override
    {
      affliction_spell_t::last_tick( d );

      if ( p()->min_version_check( VERSION_10_1_5 ) && p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T30, B4 ) )
      {
        td( d->target )->debuffs_infirmity->expire();
      }
    }
  };

  vile_taint_t( warlock_t* p, util::string_view options_str ) : affliction_spell_t( "Vile Taint", p, p->talents.vile_taint )
  {
    parse_options( options_str );

    impact_action = new vile_taint_dot_t( p );
    add_child( impact_action );

    if ( p->sets->has_set_bonus( WARLOCK_AFFLICTION, T30, B2 ) )
      cooldown->duration += p->sets->set( WARLOCK_AFFLICTION, T30, B2 )->effectN( 1 ).time_value();
  }

  vile_taint_t( warlock_t* p, util::string_view opt, bool soul_swap ) : vile_taint_t( p, opt )
  {
    if ( soul_swap )
    {
      impact_action->execute_action = nullptr; // Only want to apply Vile Taint DoT, not secondary effects
      aoe = 1;
    }
  }

  void impact( action_state_t* s ) override
  {
    affliction_spell_t::impact( s );

    if ( p()->min_version_check( VERSION_10_1_5 ) && p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T30, B4 ) )
    {
      td( s->target )->debuffs_infirmity->trigger();
    }
  }
};

struct soul_swap_t : public affliction_spell_t
{
  soul_swap_t( warlock_t* p, util::string_view options_str ) : affliction_spell_t( "Soul Swap", p, p->talents.soul_swap )
  {
    parse_options( options_str );
    may_crit = false;
  }

  bool ready() override
  {
    if ( td( target )->dots_corruption->is_ticking() || td( target )->dots_agony->is_ticking() 
      || td( target )->dots_unstable_affliction->is_ticking() || td( target )->dots_siphon_life->is_ticking()
      || td( target )->debuffs_haunt->check() )
      return affliction_spell_t::ready();

    return false;
  }

  void execute() override
  {
    affliction_spell_t::execute();

    // Loop through relevant DoTs and store states
    // (DoTs that are not present have action_copied set to false)
    auto tar = td( target );

    if ( tar->dots_corruption->is_ticking() )
    {
      p()->soul_swap_state.corruption.action_copied = true;
      p()->soul_swap_state.corruption.duration = tar->dots_corruption->remains();
    }

    if ( tar->dots_agony->is_ticking() )
    {
      p()->soul_swap_state.agony.action_copied = true;
      p()->soul_swap_state.agony.duration = tar->dots_agony->remains();
      p()->soul_swap_state.agony.stacks = tar->dots_agony->current_stack();
    }

    if ( tar->dots_unstable_affliction->is_ticking() )
    {
      p()->soul_swap_state.unstable_affliction.action_copied = true;
      p()->soul_swap_state.unstable_affliction.duration = tar->dots_unstable_affliction->remains();
      p()->soul_swap_state.unstable_affliction.stacks = p()->buffs.malefic_affliction->check(); // While there are no stacks for UA, Soul Swap *will* reapply Malefic Affliction if UA is put on another target
      tar->dots_unstable_affliction->cancel();
    }

    if ( tar->dots_siphon_life->is_ticking() )
    {
      p()->soul_swap_state.siphon_life.action_copied = true;
      p()->soul_swap_state.siphon_life.duration = tar->dots_siphon_life->remains();
    }

    if ( tar->debuffs_haunt->check() )
    {
      p()->soul_swap_state.haunt.action_copied = true;
      p()->soul_swap_state.haunt.duration = tar->debuffs_haunt->remains();
    }

    if ( tar->dots_soul_rot->is_ticking() )
    {
      p()->soul_swap_state.soul_rot.action_copied = true;
      p()->soul_swap_state.soul_rot.duration = tar->dots_soul_rot->remains();
    }

    if ( tar->dots_phantom_singularity->is_ticking() )
    {
      p()->soul_swap_state.phantom_singularity.action_copied = true;
      p()->soul_swap_state.phantom_singularity.duration = tar->dots_phantom_singularity->remains();
    }

    if ( tar->dots_vile_taint->is_ticking() )
    {
      p()->soul_swap_state.vile_taint.action_copied = true;
      p()->soul_swap_state.vile_taint.duration = tar->dots_vile_taint->remains();
    }

    p()->buffs.soul_swap->trigger();
  }
};

struct soul_swap_exhale_t : public affliction_spell_t
{
  soul_swap_exhale_t( warlock_t* p, util::string_view options_str ) : affliction_spell_t( "Soul Swap: Exhale", p, p->talents.soul_swap_exhale )
  {
    parse_options( options_str );
    harmful = true;
  }

  timespan_t travel_time() const override
  {
    return 0_ms; // Despite a missile animation taking place, DoTs are seemingly applied immediately
  }

  bool ready() override
  {
    if ( p()->buffs.soul_swap->check() )
      return affliction_spell_t::ready();

    return false;
  }

  size_t available_targets( std::vector<player_t*>& tl ) const override
  {
    affliction_spell_t::available_targets( tl );

    // Soul Swap cannot be cast on the target it copied from
    // This is set during the use of Soul Swap, and reset to nullptr when the buff expires
    if ( p()->ss_source )
    {
      auto it = range::find( tl, p()->ss_source );
      if ( it != tl.end() )
      {
        tl.erase( it );
      }
    }

    return tl.size();
  }

  void impact( action_state_t* s ) override
  {
    affliction_spell_t::impact( s );

    if ( p()->soul_swap_state.corruption.action_copied )
    {
      td( s->target )->dots_corruption->cancel();

      p()->soul_swap_state.corruption.action->execute_on_target( s->target );
      td( s->target )->dots_corruption->adjust_duration( p()->soul_swap_state.corruption.duration - td( s->target )->dots_corruption->remains() );
    }

    if ( p()->soul_swap_state.agony.action_copied )
    {
      td( s->target )->dots_agony->cancel();

      p()->soul_swap_state.agony.action->execute_on_target( s->target );
      td( s->target )->dots_agony->adjust_duration( p()->soul_swap_state.agony.duration - td( s->target )->dots_agony->remains() );
      td( s->target )->dots_agony->increment( p()->soul_swap_state.agony.stacks - 1 );
    }

    if ( p()->soul_swap_state.unstable_affliction.action_copied )
    {
      td( s->target )->dots_unstable_affliction->cancel();

      p()->soul_swap_state.unstable_affliction.action->execute_on_target( s->target );
      td( s->target )->dots_unstable_affliction->adjust_duration( p()->soul_swap_state.unstable_affliction.duration - td( s->target)->dots_unstable_affliction->remains() );

      if ( p()->soul_swap_state.unstable_affliction.stacks > 0 )
      {
        p()->buffs.malefic_affliction->trigger( p()->soul_swap_state.unstable_affliction.stacks );
      }
    }

    if ( p()->soul_swap_state.siphon_life.action_copied )
    {
      td( s->target )->dots_siphon_life->cancel();

      p()->soul_swap_state.siphon_life.action->execute_on_target( s->target );
      td( s->target )->dots_siphon_life->adjust_duration( p()->soul_swap_state.siphon_life.duration - td( s->target )->dots_siphon_life->remains() );
    }

    if ( p()->soul_swap_state.haunt.action_copied )
    {
      td( s->target )->debuffs_haunt->expire();

      // Need to handle Haunt trigger manually instead of via action
      td( s->target )->debuffs_haunt->trigger( p()->soul_swap_state.haunt.duration );
    }

    if ( p()->soul_swap_state.soul_rot.action_copied )
    {
      td( s->target )->dots_soul_rot->cancel();

      p()->soul_swap_state.soul_rot.action->execute_on_target( s->target );
      td( s->target )->dots_soul_rot->adjust_duration( p()->soul_swap_state.soul_rot.duration - td( s->target )->dots_soul_rot->remains() );
    }

    if ( p()->soul_swap_state.phantom_singularity.action_copied )
    {
      td( s->target )->dots_phantom_singularity->cancel();

      p()->soul_swap_state.phantom_singularity.action->execute_on_target( s->target );
      
      // Copied PS appears to fudge duration to force only full ticks
      auto tick_count = std::ceil( p()->soul_swap_state.phantom_singularity.duration / ( p()->soul_swap_state.phantom_singularity.action->base_tick_time * s->haste ) );
      auto dur = tick_count * p()->soul_swap_state.phantom_singularity.action->base_tick_time * s->haste;
      td( s->target )->dots_phantom_singularity->adjust_duration( p()->soul_swap_state.phantom_singularity.duration - dur );
    }

    if ( p()->soul_swap_state.vile_taint.action_copied )
    {
      td( s->target )->dots_vile_taint->cancel();

      p()->soul_swap_state.vile_taint.action->execute_on_target( s->target );
      td( s->target )->dots_vile_taint->adjust_duration( p()->soul_swap_state.vile_taint.duration - td( s->target )->dots_vile_taint->remains() );
    }

    p()->buffs.soul_swap->expire();
  }
};

}  // namespace actions_affliction

action_t* warlock_t::create_action_affliction( util::string_view action_name, util::string_view options_str )
{
  using namespace actions_affliction;

  if ( action_name == "agony" )
    return new agony_t( this, options_str );
  if ( action_name == "unstable_affliction" )
    return new unstable_affliction_t( this, options_str );
  if ( action_name == "summon_darkglare" )
    return new summon_darkglare_t( this, options_str );
  if ( action_name == "drain_soul" )
    return new drain_soul_t( this, options_str );
  if ( action_name == "haunt" )
    return new haunt_t( this, options_str );
  if ( action_name == "phantom_singularity" )
    return new phantom_singularity_t( this, options_str );
  if ( action_name == "siphon_life" )
    return new siphon_life_t( this, options_str );
  if ( action_name == "vile_taint" )
    return new vile_taint_t( this, options_str );
  if ( action_name == "malefic_rapture" )
    return new malefic_rapture_t( this, options_str );
  if ( action_name == "soul_swap" )
    return new soul_swap_t( this, options_str );
  if ( action_name == "soul_swap_exhale" )
    return new soul_swap_exhale_t( this, options_str );

  return nullptr;
}

void warlock_t::create_buffs_affliction()
{
  buffs.drain_life = make_buff( this, "drain_life" );
  buffs.drain_life->quiet = true;

  buffs.nightfall = make_buff( this, "nightfall", talents.nightfall_buff )
                        ->set_trigger_spell( talents.nightfall );

  buffs.inevitable_demise = make_buff( this, "inevitable_demise", talents.inevitable_demise_buff )
                                ->set_default_value( talents.inevitable_demise->effectN( 1 ).percent() );

  buffs.soul_swap = make_buff( this, "soul_swap", talents.soul_swap_buff )
                        ->set_stack_change_callback( [ this ]( buff_t*, int, int cur )
                          {
                            if ( cur == 0 )
                            {
                              ss_source = nullptr;
                              soul_swap_state.corruption.action_copied = false;
                              soul_swap_state.agony.action_copied = false;
                              soul_swap_state.unstable_affliction.action_copied = false;
                              soul_swap_state.siphon_life.action_copied = false;
                              soul_swap_state.haunt.action_copied = false;
                              soul_swap_state.soul_rot.action_copied = false;
                              soul_swap_state.phantom_singularity.action_copied = false;
                              soul_swap_state.vile_taint.action_copied = false;
                            }
                          } );

  buffs.tormented_crescendo = make_buff( this, "tormented_crescendo", talents.tormented_crescendo_buff );

  buffs.malefic_affliction = make_buff( this, "malefic_affliction", talents.malefic_affliction_buff )
                                 ->set_default_value( talents.malefic_affliction->effectN( 1 ).percent() );

  buffs.haunted_soul = make_buff( this, "haunted_soul", talents.haunted_soul_buff )
                           ->set_default_value( talents.haunted_soul_buff->effectN( 1 ).percent() );

  buffs.active_haunts = make_buff( this, "active_haunts" )
                            ->set_max_stack( 20 )
                            ->set_stack_change_callback( [ this ]( buff_t*, int prev, int cur )
                              {
                                if ( talents.haunted_soul->ok() )
                                {
                                  if ( cur == 0 )
                                    buffs.haunted_soul->expire();
                                  else if ( cur > 0 && prev == 0 )
                                    buffs.haunted_soul->trigger();
                                }
                              } );
  buffs.active_haunts->quiet = true;

  buffs.cruel_inspiration = make_buff( this, "cruel_inspiration", tier.cruel_inspiration )
                                ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                                ->set_default_value_from_effect( 1 );

  buffs.cruel_epiphany = make_buff( this, "cruel_epiphany", tier.cruel_epiphany )
                             ->set_default_value_from_effect( 1 );
}

void warlock_t::init_spells_affliction()
{
  using namespace actions_affliction;

  // Talents
  talents.malefic_rapture = find_talent_spell( talent_tree::SPECIALIZATION, "Malefic Rapture" ); // Should be ID 324536
  talents.malefic_rapture_dmg = find_spell( 324540 ); // This spell is the ID seen in logs, but the spcoeff is in the primary talent spell

  talents.unstable_affliction = find_talent_spell( talent_tree::SPECIALIZATION, "Unstable Affliction" ); // Should be ID 316099
  talents.unstable_affliction_2 = find_spell( 231791 ); // Soul Shard on demise
  talents.unstable_affliction_3 = find_spell( 334315 ); // +5 seconds duration
  
  talents.nightfall = find_talent_spell( talent_tree::SPECIALIZATION, "Nightfall" ); // Should be ID 108558
  talents.nightfall_buff = find_spell( 264571 );

  talents.writhe_in_agony = find_talent_spell( talent_tree::SPECIALIZATION, "Writhe in Agony" ); // Should be ID 196102
  
  talents.sow_the_seeds = find_talent_spell( talent_tree::SPECIALIZATION, "Sow the Seeds" ); // Should be ID 196226

  talents.shadow_embrace = find_talent_spell( talent_tree::SPECIALIZATION, "Shadow Embrace" ); // Should be ID 32388
  talents.shadow_embrace_debuff = find_spell( 32390 );

  talents.dark_virtuosity = find_talent_spell( talent_tree::SPECIALIZATION, "Dark Virtuosity" ); // Should be ID 405327

  talents.kindled_malice = find_talent_spell( talent_tree::SPECIALIZATION, "Kindled Malice" );  // Should be ID 405330

  talents.agonizing_corruption = find_talent_spell( talent_tree::SPECIALIZATION, "Agonizing Corruption" ); // Should be ID 386922

  talents.drain_soul = find_talent_spell( talent_tree::SPECIALIZATION, "Drain Soul" ); // Should be ID 388667
  talents.drain_soul_dot = find_spell( 198590 ); // This contains all the channel data

  talents.absolute_corruption = find_talent_spell( talent_tree::SPECIALIZATION, "Absolute Corruption" ); // Should be ID 196103

  talents.siphon_life = find_talent_spell( talent_tree::SPECIALIZATION, "Siphon Life" ); // Should be ID 63106

  talents.phantom_singularity = find_talent_spell( talent_tree::SPECIALIZATION, "Phantom Singularity" ); // Should be ID 205179
  talents.phantom_singularity_tick = find_spell( 205246 ); // AoE damage info

  talents.vile_taint = find_talent_spell( talent_tree::SPECIALIZATION, "Vile Taint" ); // Should be ID 278350
  talents.vile_taint_dot = find_spell( 386931 ); // DoT info here

  talents.pandemic_invocation = find_talent_spell( talent_tree::SPECIALIZATION, "Pandemic Invocation" ); // Should be ID 386759
  talents.pandemic_invocation_proc = find_spell( 386760 ); // Proc damage data

  talents.inevitable_demise = find_talent_spell( talent_tree::SPECIALIZATION, "Inevitable Demise" ); // Should be ID 334319
  talents.inevitable_demise_buff = find_spell( 334320 ); // Buff data

  talents.soul_swap = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Swap" ); // Should be ID 386951
  talents.soul_swap_ua = find_spell( 316099 ); // Unnecessary in 10.0.5 due to spell changes
  talents.soul_swap_buff = find_spell( 86211 ); // Buff data
  talents.soul_swap_exhale = find_spell( 86213 ); // Replacement action while buff active

  talents.soul_flame = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Flame" ); // Should be ID 199471
  talents.soul_flame_proc = find_spell( 199581 ); // AoE damage data

  talents.focused_malignancy = find_talent_spell( talent_tree::SPECIALIZATION, "Focused Malignancy" ); // Should be ID 399668

  talents.withering_bolt = find_talent_spell( talent_tree::SPECIALIZATION, "Withering Bolt" ); // Should be ID 386976

  talents.sacrolashs_dark_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Sacrolash's Dark Strike" ); // Should be ID 386986

  talents.creeping_death = find_talent_spell( talent_tree::SPECIALIZATION, "Creeping Death" ); // Should be ID 264000

  talents.haunt = find_talent_spell( talent_tree::SPECIALIZATION, "Haunt" ); // Should be ID 48181

  talents.summon_darkglare = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Darkglare" ); // Should be ID 205180

  talents.soul_rot = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Rot" ); // Should be ID 386997
 
  talents.malefic_affliction = find_talent_spell( talent_tree::SPECIALIZATION, "Malefic Affliction" ); // Should be ID 389761
  talents.malefic_affliction_buff = find_spell( 389845 ); // Buff data, infinite duration, cancelled by UA ending

  talents.xavius_gambit = find_talent_spell( talent_tree::SPECIALIZATION, "Xavius' Gambit" ); // Should be ID 416615

  talents.tormented_crescendo = find_talent_spell( talent_tree::SPECIALIZATION, "Tormented Crescendo" ); // Should be ID 387075
  talents.tormented_crescendo_buff = find_spell( 387079 );

  talents.seized_vitality = find_talent_spell( talent_tree::SPECIALIZATION, "Seized Vitality" ); // Should be ID 387250

  talents.malevolent_visionary = find_talent_spell( talent_tree::SPECIALIZATION, "Malevolent Visionary" ); // Should be ID 387273

  talents.wrath_of_consumption = find_talent_spell( talent_tree::SPECIALIZATION, "Wrath of Consumption" ); // Should be ID 387065
  talents.wrath_of_consumption_buff = find_spell( 387066 );

  talents.souleaters_gluttony = find_talent_spell( talent_tree::SPECIALIZATION, "Soul-Eater's Gluttony" ); // Should be ID 389630

  talents.doom_blossom = find_talent_spell( talent_tree::SPECIALIZATION, "Doom Blossom" ); // Should be ID 389764
  talents.doom_blossom_proc = find_spell( min_version_check( VERSION_10_1_5 ) ? 416699 : 389869 ); // AoE damage data

  talents.dread_touch = find_talent_spell( talent_tree::SPECIALIZATION, "Dread Touch" ); // Should be ID 389775
  talents.dread_touch_debuff = find_spell( 389868 ); // Applied to target on proc

  talents.haunted_soul = find_talent_spell( talent_tree::SPECIALIZATION, "Haunted Soul" ); // Should be ID 387301
  talents.haunted_soul_buff = find_spell( 387310 );

  talents.grim_reach = find_talent_spell( talent_tree::SPECIALIZATION, "Grim Reach" ); // Should be ID 389992

  talents.dark_harvest = find_talent_spell( talent_tree::SPECIALIZATION, "Dark Harvest" ); // Should be ID 387016
  talents.dark_harvest_buff = find_spell( 387018 );

  // Additional Tier Set spell data

  // T29 (Vault of the Incarnates)
  tier.cruel_inspiration = find_spell( 394215 );
  tier.cruel_epiphany = find_spell( 394253 );

  // T30 (Aberrus, the Shadowed Crucible)
  tier.infirmity = find_spell( 409765 );
}

void warlock_t::create_soul_swap_actions()
{
  soul_swap_state.corruption.action = this->pass_corruption_action( this );
  soul_swap_state.corruption.action->dual = true;
  soul_swap_state.corruption.action_copied = false;

  soul_swap_state.agony.action = new warlock::actions_affliction::agony_t( this, "" );
  soul_swap_state.agony.action->dual = true;
  soul_swap_state.agony.action_copied = false;

  soul_swap_state.unstable_affliction.action = new warlock::actions_affliction::unstable_affliction_t( this, "" );
  soul_swap_state.unstable_affliction.action->dual = true;
  soul_swap_state.unstable_affliction.action_copied = false;

  soul_swap_state.siphon_life.action = new warlock::actions_affliction::siphon_life_t( this, "" );
  soul_swap_state.siphon_life.action->dual = true;
  soul_swap_state.siphon_life.action_copied = false;

  soul_swap_state.haunt.action = new warlock::actions_affliction::haunt_t( this, "" );
  soul_swap_state.haunt.action->dual = true;
  soul_swap_state.haunt.action_copied = false;

  soul_swap_state.soul_rot.action = this->pass_soul_rot_action( this );
  soul_swap_state.soul_rot.action->dual = true;
  soul_swap_state.soul_rot.action_copied = false;

  soul_swap_state.phantom_singularity.action = new warlock::actions_affliction::phantom_singularity_t( this, "" );
  soul_swap_state.phantom_singularity.action->dual = true;
  soul_swap_state.phantom_singularity.action_copied = false;

  soul_swap_state.vile_taint.action = new warlock::actions_affliction::vile_taint_t( this, "", true );
  soul_swap_state.vile_taint.action->dual = true;
  soul_swap_state.vile_taint.action_copied = false;
}

void warlock_t::init_gains_affliction()
{
  gains.agony = get_gain( "agony" );
  gains.unstable_affliction_refund = get_gain( "unstable_affliction_refund" );
  gains.drain_soul = get_gain( "drain_soul" );
  gains.pandemic_invocation = get_gain( "pandemic_invocation" );
}

void warlock_t::init_rng_affliction()
{
}

void warlock_t::init_procs_affliction()
{
  procs.nightfall = get_proc( "nightfall" );
  procs.pandemic_invocation_shard = get_proc( "pandemic_invocation_shard" );
  procs.tormented_crescendo = get_proc( "tormented_crescendo" );
  procs.doom_blossom = get_proc( "doom_blossom" );
  procs.cruel_inspiration = get_proc( "cruel_inspiration" );

  for ( size_t i = 0; i < procs.malefic_rapture.size(); i++ )
  {
    procs.malefic_rapture[ i ] = get_proc( fmt::format( "Malefic Rapture {}", i + 1 ) );
  }
}

}  // namespace warlock
