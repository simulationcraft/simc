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

    if ( this->data().affected_by( p()->warlock_base.potent_afflictions->effectN( 2 ) ) )
    {
      pm *= 1.0 + p()->cache.mastery_value();
    }
    return pm;
  }

  // tick action multiplier
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

// Dots
struct agony_t : public affliction_spell_t
{
  double chance;

  agony_t( warlock_t* p, util::string_view options_str ) : affliction_spell_t( "Agony", p, p->warlock_base.agony )
  {
    parse_options( options_str );
    may_crit = false;

    // Unclear in DF beta if Agony Rank 2 is intended to still be learned
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
    auto dot_data = td( s->target )->dots_agony;

    bool pi_trigger = p()->talents.pandemic_invocation.ok() && dot_data->is_ticking()
      && dot_data->remains() < timespan_t::from_millis( p()->talents.pandemic_invocation->effectN( 1 ).base_value() );

    affliction_spell_t::impact( s );

    if ( pi_trigger )
      p()->proc_actions.pandemic_invocation_proc->execute_on_target( s->target );
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

    //if ( p()->talents.creeping_death->ok() )
    //{
    //  increment_max *= 1.0 + p()->talents.creeping_death->effectN( 1 ).percent();
    //}

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

    affliction_spell_t::tick( d );
  }
};

struct unstable_affliction_t : public affliction_spell_t
{
  unstable_affliction_t( warlock_t* p, util::string_view options_str )
    : affliction_spell_t( "unstable_affliction", p, p->talents.unstable_affliction )
  {
    parse_options( options_str );

    // DF - In beta the rank 3 passive appears to be learned as part of the spec automatically
    dot_duration += timespan_t::from_millis( p->talents.unstable_affliction_3->effectN( 1 ).base_value() );
  }

  unstable_affliction_t( warlock_t* p )
    : affliction_spell_t( "unstable_affliction", p, p->talents.soul_swap_ua )
  {
    dot_duration += timespan_t::from_millis( p->talents.unstable_affliction_3->effectN( 1 ).base_value() );
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

  void impact( action_state_t* s ) override
  {
    auto dot_data = td( s->target )->dots_unstable_affliction;

    bool pi_trigger = p()->talents.pandemic_invocation.ok() && dot_data->is_ticking()
      && dot_data->remains() < timespan_t::from_millis( p()->talents.pandemic_invocation->effectN( 1 ).base_value() );

    affliction_spell_t::impact( s );

    if ( pi_trigger )
      p()->proc_actions.pandemic_invocation_proc->execute_on_target( s->target );
  }

  void last_tick( dot_t* d) override
  {
    affliction_spell_t::last_tick( d );

    p()->ua_target = nullptr;
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

struct malefic_rapture_t : public affliction_spell_t
{
    struct malefic_rapture_damage_instance_t : public affliction_spell_t
    {
      malefic_rapture_damage_instance_t( warlock_t *p, double spc ) :
          affliction_spell_t( "malefic_rapture_damage", p, p->talents.malefic_rapture_dmg )
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

        //if ( p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T28, B2 ) )
        //{
        //  m *= 1.0 + p()->sets->set( WARLOCK_AFFLICTION, T28, B2 )->effectN( 1 ).percent();
        //}

        return m;
      }

      void impact ( action_state_t* s ) override
      {
        affliction_spell_t::impact( s );

        //if ( p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T28, B2 ) )
        //{
        //  timespan_t dot_extension =  p()->sets->set( WARLOCK_AFFLICTION, T28, B2 )->effectN( 2 ).time_value() * 1000;
        //  warlock_td_t* td = p()->get_target_data( s->target );

        //  td->dots_agony->adjust_duration( dot_extension );
        //  td->dots_unstable_affliction->adjust_duration(dot_extension);

        //  if ( !p()->talents.absolute_corruption->ok() )
        //  {
        //    td->dots_corruption->adjust_duration( dot_extension );
        //  }
        //}
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
      : affliction_spell_t( "malefic_rapture", p, p->talents.malefic_rapture )
    {
      parse_options( options_str );
      aoe = -1;

      impact_action = new malefic_rapture_damage_instance_t( p, data().effectN( 1 ).sp_coeff() );
      add_child( impact_action );
    }

    double cost() const override
    {
      double c = affliction_spell_t::cost();

      //if ( p()->buffs.calamitous_crescendo->check() )
      //  c *= 1.0 + p()->buffs.calamitous_crescendo->data().effectN( 4 ).percent();
        
      return c;      
    }

    timespan_t execute_time() const override
    {
      timespan_t t = affliction_spell_t::execute_time();

      //if ( p()->buffs.calamitous_crescendo->check() )
      //  t *= 1.0 + p()->buffs.calamitous_crescendo->data().effectN( 3 ).percent();

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

      //p()->buffs.calamitous_crescendo->expire();
    }

    size_t available_targets( std::vector<player_t*>& tl ) const override
    {
      affliction_spell_t::available_targets( tl );

      tl.erase( std::remove_if( tl.begin(), tl.end(), [ this ]( player_t* target ){ return p()->get_target_data( target )->count_affliction_dots() == 0; } ), tl.end() );

      return tl.size();
    }
};

// Talents

struct drain_soul_t : public affliction_spell_t
{
  // 2022-09-25 Current Drain Soul behavior with Nightfall:
  // Drain Soul ordinarily behaves like a hasted duration DoT, using pandemic behavior for chaining
  // However, Nightfall is increasing tick rate without affecting duration, creating more ticks than usual
  // In most cases, the regular duration is used when calculating the refreshed duration
  // The one exception is a fresh cast of a Nightfall-buffed Drain Soul, which lasts slightly longer to ensure each tick is a full tick
  struct drain_soul_state_t : public action_state_t
  {
    double tick_time_multiplier;
    bool rounded_channel;

    drain_soul_state_t( action_t* action, player_t* target )
      : action_state_t( action, target ),
      tick_time_multiplier( 1.0 ),
      rounded_channel( false )
    { }

    void initialize() override
    {
      action_state_t::initialize();
      tick_time_multiplier = 1.0;
      rounded_channel = false;
    }

    std::ostringstream& debug_str( std::ostringstream& s ) override
    {
      action_state_t::debug_str( s ) << " tick_time_multiplier=" << tick_time_multiplier;
      s << " rounded_channel=" << rounded_channel;
      return s;
    }

    void copy_state( const action_state_t* s ) override
    {
      action_state_t::copy_state( s );
      tick_time_multiplier = debug_cast<const drain_soul_state_t*>( s )->tick_time_multiplier;
      rounded_channel = debug_cast<const drain_soul_state_t*>( s )->rounded_channel;
    }
  };

  drain_soul_t( warlock_t* p, util::string_view options_str )
    : affliction_spell_t( "drain_soul", p, p->talents.drain_soul.ok() ? p->talents.drain_soul_dot : spell_data_t::not_found() )
  {
    parse_options( options_str );
    channeled    = true;
  }

  action_state_t* new_state() override
  { return new drain_soul_state_t( this, target ); }

  void snapshot_state( action_state_t* s, result_amount_type rt ) override
  {
    debug_cast<drain_soul_state_t*>( s )->tick_time_multiplier = 1.0 + ( p()->buffs.nightfall->check() ? p()->buffs.nightfall->data().effectN( 3 ).percent() : 0 );
    debug_cast<drain_soul_state_t*>( s )->rounded_channel = ( !td( s->target )->dots_drain_soul->is_ticking() && p()->buffs.nightfall->check() );
    affliction_spell_t::snapshot_state( s, rt );
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    auto dur = ( dot_duration * s->haste );

    if ( debug_cast<const drain_soul_state_t*>( s )->rounded_channel )
      dur = tick_time( s ) * std::ceil( dur / tick_time( s ) );

    return dur;
  }

  timespan_t tick_time( const action_state_t* s ) const override
  {
    timespan_t t = affliction_spell_t::tick_time( s );

    t *= debug_cast<const drain_soul_state_t*>( s )->tick_time_multiplier;

    return t;
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

      //if ( p()->sets->has_set_bonus( WARLOCK_AFFLICTION, T28, B4 ) )
      //{
      //  // TOFIX - As of 2022-02-03 PTR, the bonus appears to still be only checking that *any* target has these dots. May need to implement this behavior.
      //  bool tierDotsActive = td( d->target )->dots_agony->is_ticking() 
      //                     && td( d->target )->dots_corruption->is_ticking()
      //                     && td( d->target )->dots_unstable_affliction->is_ticking();

      //  if ( tierDotsActive && rng().roll( p()->sets->set( WARLOCK_AFFLICTION, T28, B4 )->effectN( 2 ).percent() ) )
      //  {
      //    p()->procs.calamitous_crescendo->occur();
      //    p()->buffs.calamitous_crescendo->trigger();
      //  }
      //}
    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = affliction_spell_t::composite_target_multiplier( t );

    if ( t->health_percentage() < p()->talents.drain_soul_dot->effectN( 3 ).base_value() )
      m *= 1.0 + p()->talents.drain_soul->effectN( 2 ).percent();

    if ( p()->talents.withering_bolt.ok() )
      m *= 1.0 + p()->talents.withering_bolt->effectN( 1 ).percent() * std::min( (int)p()->talents.withering_bolt->effectN( 2 ).base_value(), p()->get_target_data( t )->count_affliction_dots() );

    return m;
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
  }
  
  void impact( action_state_t* s ) override
  {
    auto dot_data = td( s->target )->dots_siphon_life;

    bool pi_trigger = p()->talents.pandemic_invocation.ok() && dot_data->is_ticking()
      && dot_data->remains() < timespan_t::from_millis( p()->talents.pandemic_invocation->effectN( 1 ).base_value() );

    affliction_spell_t::impact( s );

    if ( pi_trigger )
      p()->proc_actions.pandemic_invocation_proc->execute_on_target( s->target );
  }
};

struct phantom_singularity_tick_t : public affliction_spell_t
{
  phantom_singularity_tick_t( warlock_t* p )
    : affliction_spell_t( "phantom_singularity_tick", p, p->talents.phantom_singularity_tick )
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
    return ( s->action->tick_time( s ) / base_tick_time ) * dot_duration ;
  }
};

struct vile_taint_t : public affliction_spell_t
{
  struct vile_taint_dot_t : public affliction_spell_t
  {
    vile_taint_dot_t( warlock_t* p ) : affliction_spell_t( "Vile Taint (DoT)", p, p->talents.vile_taint_dot )
    {
      tick_zero = true;
      execute_action = new agony_t( p, "" );
      execute_action->dual = true;
      execute_action->base_costs[ RESOURCE_MANA ] = 0.0;
    }
  };

  vile_taint_t( warlock_t* p, util::string_view options_str )
    : affliction_spell_t( "vile_taint", p, p->talents.vile_taint )
  {
    parse_options( options_str );

    impact_action = new vile_taint_dot_t( p );
    add_child( impact_action );
  }
};

struct soul_tap_t : public affliction_spell_t
{
  soul_tap_t( warlock_t* p, util::string_view options_str )
    : affliction_spell_t( "Soul Tap", p, p->talents.soul_tap )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    affliction_spell_t::execute();

    // 1 Soul Shard is hardcoded, not in spell data
    p()->resource_gain( RESOURCE_SOUL_SHARD, 1, p()->gains.soul_tap );
  }
};

struct soul_swap_t : public affliction_spell_t
{
  action_t* corruption;
  agony_t* agony;
  unstable_affliction_t* ua;

  soul_swap_t( warlock_t* p, util::string_view options_str )
    : affliction_spell_t( "Soul Swap", p, p->talents.soul_swap )
  {
    may_crit = false;

    corruption = p->pass_corruption_action( p );
    corruption->dual = true;

    agony = new agony_t( p, "" );
    agony->dual = true;

    ua = new unstable_affliction_t( p );
    ua->dual = true;
  }

  void execute() override
  {
    affliction_spell_t::execute();

    corruption->execute_on_target( target );
    agony->execute_on_target( target );
    ua->execute_on_target( target );
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

  if ( action_name == "agony" )
    return new agony_t( this, options_str );
  if ( action_name == "unstable_affliction" )
    return new unstable_affliction_t( this, options_str );
  if ( action_name == "summon_darkglare" )
    return new summon_darkglare_t( this, options_str );
  // talents
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
  if ( action_name == "soul_tap" )
    return new soul_tap_t( this, options_str );
  if ( action_name == "soul_swap" )
    return new soul_swap_t( this, options_str );

  return nullptr;
}

void warlock_t::create_buffs_affliction()
{
  // spells
  buffs.drain_life = make_buff( this, "drain_life" );
  // talents
  buffs.nightfall = make_buff( this, "nightfall", talents.nightfall_buff )
                        ->set_trigger_spell( talents.nightfall );

  buffs.inevitable_demise = make_buff( this, "inevitable_demise", talents.inevitable_demise_buff )
                                ->set_default_value( talents.inevitable_demise->effectN( 1 ).percent() ); // There are effects in the buff data, but are they unused for the damage?

  buffs.calamitous_crescendo = make_buff( this, "calamitous_crescendo", find_spell( 364322 ) );
}

void warlock_t::init_spells_affliction()
{
  using namespace actions_affliction;

  // Specialization Spells
  spec.summon_darkglare    = find_specialization_spell( "Summon Darkglare" );
  spec.corruption_2        = find_specialization_spell( "Corruption", "Rank 2" );
  spec.corruption_3        = find_specialization_spell( "Corruption", "Rank 3" );
  spec.summon_darkglare_2         = find_specialization_spell( "Summon Darkglare", "Rank 2" ); //9.1 PTR - Now a passive learned at level 58

  // Talents
  talents.malefic_rapture = find_talent_spell( talent_tree::SPECIALIZATION, "Malefic Rapture" );
  talents.malefic_rapture_dmg = find_spell( 324540 ); // This spell is the ID seen in logs, but the spcoeff is in the primary talent spell

  talents.unstable_affliction = find_talent_spell( talent_tree::SPECIALIZATION, "Unstable Affliction" );
  talents.unstable_affliction_2 = find_spell( 231791 ); // Soul Shard on demise
  talents.unstable_affliction_3 = find_spell( 334315 ); // +5 seconds duration
  
  talents.nightfall = find_talent_spell( talent_tree::SPECIALIZATION, "Nightfall" ); // Should be ID 108558
  talents.nightfall_buff = find_spell( 264571 );
  
  talents.xavian_teachings = find_talent_spell( talent_tree::SPECIALIZATION, "Xavian Teachings" ); // Should be ID 317031

  talents.sow_the_seeds = find_talent_spell( talent_tree::SPECIALIZATION, "Sow the Seeds" ); // Should be ID 196226

  talents.shadow_embrace = find_talent_spell( talent_tree::SPECIALIZATION, "Shadow Embrace" ); // Should be ID 32388
  talents.shadow_embrace_debuff = find_spell( 32390 );

  talents.harvester_of_souls = find_talent_spell( talent_tree::SPECIALIZATION, "Harvester of Souls" ); // Should be ID 201424
  talents.harvester_of_souls_dmg = find_spell( 218615 ); // Damage and projectile data

  talents.writhe_in_agony = find_talent_spell( talent_tree::SPECIALIZATION, "Writhe in Agony" ); // Should be ID 196102

  talents.agonizing_corruption = find_talent_spell( talent_tree::SPECIALIZATION, "Agonizing Corruption" ); // Should be ID 386922

  talents.drain_soul = find_talent_spell( talent_tree::SPECIALIZATION, "Drain Soul" ); // Should be ID 388667
  talents.drain_soul_dot = find_spell( 198590 ); // This contains all the channel data

  talents.absolute_corruption = find_talent_spell( talent_tree::SPECIALIZATION, "Absolute Corruption" ); // Should be ID 196103

  talents.siphon_life = find_talent_spell( talent_tree::SPECIALIZATION, "Siphon Life" ); // Should be ID 63106

  talents.phantom_singularity = find_talent_spell( talent_tree::SPECIALIZATION, "Phantom Singularity" ); // Should be ID 205179
  talents.phantom_singularity_tick = find_spell( 205246 ); // AoE damage info

  talents.vile_taint = find_talent_spell( talent_tree::SPECIALIZATION, "Vile Taint" ); // Should be ID 278350
  talents.vile_taint_dot = find_spell( 386931 ); // DoT info here

  talents.soul_tap = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Tap" ); // Should be ID 387073

  talents.inevitable_demise = find_talent_spell( talent_tree::SPECIALIZATION, "Inevitable Demise" ); // Should be ID 334319
  talents.inevitable_demise_buff = find_spell( 334320 ); // Buff data

  talents.soul_swap = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Swap" ); // Should be ID 386951
  talents.soul_swap_ua = find_spell( 316099 ); // Needed for when you have Soul Swap but not UA talented

  talents.soul_flame = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Flame" ); // Should be ID 199471
  talents.soul_flame_proc = find_spell( 199581 ); // AoE damage data

  talents.pandemic_invocation = find_talent_spell( talent_tree::SPECIALIZATION, "Pandemic Invocation" ); // Should be ID 386759
  talents.pandemic_invocation_proc = find_spell( 386760 ); // Proc damage data

  talents.withering_bolt = find_talent_spell( talent_tree::SPECIALIZATION, "Withering Bolt" ); // Should be ID 386976

  talents.sacrolashs_dark_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Sacrolash's Dark Strike" ); // Should be ID 386986

  talents.haunt               = find_talent_spell( "Haunt" );




 
  talents.creeping_death      = find_talent_spell( "Creeping Death" );

  // Conduits
  conduit.withering_bolt     = find_conduit_spell( "Withering Bolt" ); //9.1 PTR - New, replaces Cold Embrace
}

void warlock_t::init_gains_affliction()
{
  gains.agony                      = get_gain( "agony" );
  gains.unstable_affliction_refund = get_gain( "unstable_affliction_refund" );
  gains.drain_soul                 = get_gain( "drain_soul" );
  gains.soul_tap = get_gain( "soul_tap" );
  gains.pandemic_invocation = get_gain( "pandemic_invocation" );
}

void warlock_t::init_rng_affliction()
{
}

void warlock_t::init_procs_affliction()
{
  procs.nightfall            = get_proc( "nightfall" );
  procs.calamitous_crescendo = get_proc( "calamitous_crescendo" );
  procs.harvester_of_souls = get_proc( "harvester_of_souls" );
  procs.pandemic_invocation_shard = get_proc( "pandemic_invocation_shard" );

  for ( size_t i = 0; i < procs.malefic_rapture.size(); i++ )
  {
    procs.malefic_rapture[ i ] = get_proc( fmt::format( "Malefic Rapture {}", i + 1 ) );
  }
}

}  // namespace warlock
