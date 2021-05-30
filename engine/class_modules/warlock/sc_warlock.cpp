#include "simulationcraft.hpp"

#include "sc_warlock.hpp"

#include "sc_warlock_pets.hpp"
#include "util/util.hpp"

#include <queue>

namespace warlock
{
// Spells
namespace actions
{
struct drain_life_t : public warlock_spell_t
{
  //Note: Soul Rot (Night Fae Covenant) turns Drain Life into a multi-target channeled spell. Nothing else in simc behaves this way and
  //we currently do not have core support for it. Applying this dot to the secondary targets should cover most of the behavior, although
  //it will be unable to handle the case where primary channel target dies (in-game, this appears to force-swap primary target to another
  //target currently affected by Drain Life if possible).
  struct drain_life_dot_t : public warlock_spell_t
  {
    drain_life_dot_t( warlock_t* p, util::string_view options_str) : warlock_spell_t( "drain_life_aoe", p, p->find_spell( "Drain Life" ) )
    {
      parse_options( options_str );
      dual = true;
      background = true;
      may_crit = false;
      dot_behavior = DOT_REFRESH;

      // SL - Legendary
      dot_duration *= 1.0 + p->legendary.claw_of_endereth->effectN( 1 ).percent();
      base_tick_time *= 1.0 + p->legendary.claw_of_endereth->effectN( 1 ).percent();
    }

    double bonus_ta( const action_state_t* s ) const override
    {
      double ta = warlock_spell_t::bonus_ta( s );

      if ( p()->talents.inevitable_demise->ok() && p()->buffs.inevitable_demise->check() )
        ta = ta / ( 1.0 + p()->buffs.inevitable_demise->check_stack_value() );

      return ta;
    }

    double cost_per_tick( resource_e ) const override
    {
      return 0.0;
    }

    double action_multiplier() const override
    {
      double m = warlock_spell_t::action_multiplier();

      if ( p()->talents.inevitable_demise->ok() && p()->buffs.inevitable_demise->check() )
      {
        m *= 1.0 + p()->buffs.inevitable_demise->check_stack_value();
      }
      return m;
    }

    timespan_t composite_dot_duration(const action_state_t* s) const override
    {
        return dot_duration * ( tick_time( s ) / base_tick_time);
    }
  };

  drain_life_dot_t* aoe_dot;

  drain_life_t( warlock_t* p, util::string_view options_str ) : warlock_spell_t( p, "Drain Life" )
  {
    parse_options( options_str );

    aoe_dot = new drain_life_dot_t( p , options_str );
    add_child( aoe_dot );
    
    channeled    = true;
    hasted_ticks = false;
    may_crit     = false;

    // SL - Legendary
    dot_duration *= 1.0 + p->legendary.claw_of_endereth->effectN( 1 ).percent();
    base_tick_time *= 1.0 + p->legendary.claw_of_endereth->effectN( 1 ).percent();
  }

  void execute() override
  {
    if ( p()->talents.inevitable_demise->ok() && p()->buffs.inevitable_demise->check() > 0 )
    {
      if ( p()->buffs.drain_life->check() )
        p()->buffs.inevitable_demise->expire();
    }

    warlock_spell_t::execute();

    p()->buffs.drain_life->trigger();

    if ( p()->covenant.soul_rot->ok() && p()->buffs.soul_rot->check() )
    {
      const auto& tl = target_list();
      
      for ( auto& t : tl )
      {
        //Don't apply aoe version to primary target
        if ( t == target )
          continue;

        auto data = td( t );
        if ( data->dots_soul_rot->is_ticking() )
        {
          aoe_dot->set_target( t );
          aoe_dot->execute();
        }
      }
    }
  }

  double bonus_ta( const action_state_t* s ) const override
  {
    double ta = warlock_spell_t::bonus_ta( s );

    if ( p()->talents.inevitable_demise->ok() && p()->buffs.inevitable_demise->check() )
      ta = ta / ( 1.0 + p()->buffs.inevitable_demise->check_stack_value() );

    return ta;
  }

  double cost_per_tick( resource_e r ) const override
  {
    if ( r == RESOURCE_MANA && p()->buffs.soul_rot->check() )
    {
      return 0.0;
    }
    else
    {
      return warlock_spell_t::cost_per_tick( r );
    }
  }

  double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p()->talents.inevitable_demise->ok() && p()->buffs.inevitable_demise->check() )
    {
      m *= 1.0 + p()->buffs.inevitable_demise->check_stack_value();
    }
    return m;
  }

  void last_tick( dot_t* d ) override
  {
    p()->buffs.drain_life->expire();
    p()->buffs.inevitable_demise->expire();

    warlock_spell_t::last_tick( d );

    if ( p()->covenant.soul_rot->ok() )
    {
      const auto& tl = target_list();

      for ( auto& t : tl )
      {
        auto data = td( t );
        if ( data->dots_drain_life_aoe->is_ticking() )
          data->dots_drain_life_aoe->cancel();
      }
    }
  }
};  

//Not implemented: Impending Catastrophe applies a random curse in addition to the DoT
struct impending_catastrophe_dot_t : public warlock_spell_t
{
  impending_catastrophe_dot_t( warlock_t* p )
    : warlock_spell_t( "impending_catastrophe_dot", p, p->find_spell( 322170 ) )
  {
    background = true;
    may_miss   = false;
    dual       = true;
  }
  
  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
   if ( s->chain_target == 0 )
     return dot_duration * ( 1.0 + p()->conduit.catastrophic_origin.percent() );

   return dot_duration;
  }

  double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p()->specialization() == WARLOCK_DESTRUCTION && p()->mastery_spells.chaotic_energies->ok() )
    {
      double destro_mastery_value = p()->cache.mastery_value() / 2.0;
      double chaotic_energies_rng = rng().range( 0, destro_mastery_value );

      m *= 1.0 + chaotic_energies_rng + ( destro_mastery_value );
    }

    return m;
  }
};

struct impending_catastrophe_impact_t : public warlock_spell_t
{
  impending_catastrophe_impact_t( warlock_t* p )
    : warlock_spell_t( "impending_catastrophe_impact", p, p->find_spell( 322167 ) )
  {
    background = true;
    may_miss   = false;
    dual       = true;
  }

  double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p()->specialization() == WARLOCK_DESTRUCTION && p()->mastery_spells.chaotic_energies->ok() )
    {
      double destro_mastery_value = p()->cache.mastery_value() / 2.0;
      double chaotic_energies_rng = rng().range( 0, destro_mastery_value );

      m *= 1.0 + chaotic_energies_rng + ( destro_mastery_value );
    }

    return m;
  }
};

struct impending_catastrophe_t : public warlock_spell_t
{
  action_t* impending_catastrophe_impact;
  action_t* impending_catastrophe_dot;

  impending_catastrophe_t( warlock_t* p, util::string_view options_str ) : 
    warlock_spell_t( "impending_catastrophe", p, p->covenant.impending_catastrophe ),
    impending_catastrophe_impact( new impending_catastrophe_impact_t( p ) ),
    impending_catastrophe_dot( new impending_catastrophe_dot_t( p ) )
  {
    parse_options( options_str );
    travel_speed = 16;
    aoe = -1;
   
    add_child( impending_catastrophe_impact );
    add_child( impending_catastrophe_dot );
  }

  //Currently we model the "pass-through" damage as occuring on all targets when the projectile hits
  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    impending_catastrophe_dot->set_target( s->target );
    impending_catastrophe_dot->execute();

    impending_catastrophe_impact->set_target( s->target );
    impending_catastrophe_impact->execute();
  }
};

struct scouring_tithe_t : public warlock_spell_t
{
  scouring_tithe_t( warlock_t* p, util::string_view options_str )
    : warlock_spell_t( "scouring_tithe", p, p->covenant.scouring_tithe ) 
  {
    parse_options( options_str );
    can_havoc = true;
  }

  void last_tick( dot_t* d ) override
  {
    warlock_spell_t::last_tick( d );

    if ( !d->target->is_sleeping() )
    {
      p()->cooldowns.scouring_tithe->reset( true );
    }
  }

  double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p()->specialization() == WARLOCK_DESTRUCTION && p()->mastery_spells.chaotic_energies->ok() )
    {
      double destro_mastery_value = p()->cache.mastery_value() / 2.0;
      double chaotic_energies_rng = rng().range( 0, destro_mastery_value );

      m *= 1.0 + chaotic_energies_rng + ( destro_mastery_value );
    }

    return m;
  }
};

struct soul_rot_t : public warlock_spell_t
{
  soul_rot_t( warlock_t* p, util::string_view options_str )
    : warlock_spell_t( "soul_rot", p, p->covenant.soul_rot )

  {
    parse_options( options_str );
    aoe = 1 + as<int>( p->covenant.soul_rot->effectN( 3 ).base_value() );
    radius *= 1.0 + p->conduit.soul_eater.percent();
  }

  void execute() override
  {
    warlock_spell_t::execute();

    p()->buffs.soul_rot->trigger();
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( p()->legendary.decaying_soul_satchel.ok() )
    {
      p()->buffs.decaying_soul_satchel_haste->trigger();
      p()->buffs.decaying_soul_satchel_crit->trigger();
    }
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double pm = warlock_spell_t::composite_ta_multiplier( s );
    if ( s->chain_target == 0 )
    {
      pm *= 2.0; //Hardcoded in tooltip, primary takes double damage
    }

    pm *= 1.0 + p()->conduit.soul_eater.percent();

    return pm;
  }

  double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    if ( p()->specialization() == WARLOCK_DESTRUCTION && p()->mastery_spells.chaotic_energies->ok() )
    {
      double destro_mastery_value = p()->cache.mastery_value() / 2.0;
      double chaotic_energies_rng = rng().range( 0, destro_mastery_value );

      m *= 1.0 + chaotic_energies_rng + ( destro_mastery_value );
    }

    return m;
  }
};

struct decimating_bolt_dmg_t : public warlock_spell_t
{
  decimating_bolt_dmg_t( warlock_t* p ) : warlock_spell_t( "decimating_bolt_tick_t", p, p->find_spell( 327059 ) )
  {
    background = true;
    may_miss   = false;
  }

  double composite_target_multiplier( player_t* target ) const override
  {
    double m = warlock_spell_t::composite_target_multiplier( target );

    //This currently matches the bonus multiplier to the spec spells, but is not guaranteed to stay this way. Last checked on PTR 2021-03-07
    m *= 2.0 - target->health_percentage() * 0.01;

    return m;
  };

  double action_multiplier() const override
  {
    double m = warlock_spell_t::action_multiplier();

    m *= 1.0 + p()->conduit.fatal_decimation.percent();

    return m;
  }
};

struct decimating_bolt_t : public warlock_spell_t
{
  action_t* decimating_bolt_dmg;

  decimating_bolt_t( warlock_t* p, util::string_view options_str ) : 
    warlock_spell_t( "decimating_bolt", p, p->covenant.decimating_bolt ),
    decimating_bolt_dmg( new decimating_bolt_dmg_t( p ) )

  {
    parse_options( options_str );
    can_havoc = true;
    travel_speed = p->find_spell( 327072 )->missile_speed();

    add_child( decimating_bolt_dmg );
  }

  void impact( action_state_t* s ) override
  {
    //TOCHECK: the formulae for Decimating Bolt bonus damage does not appear in spell data, and should be
    //checked regularly to ensure accuracy
    double value = p()->buffs.decimating_bolt->default_value - 0.01 * s->target->health_percentage();
    if ( p()->talents.fire_and_brimstone->ok() )
      value *= 0.4;
    p()->buffs.decimating_bolt->trigger( 3, value );
    
    if ( p()->legendary.shard_of_annihilation.ok() )
    {
      //Note: For Drain Soul, 3 stacks appear to be triggered but all are removed when the Decimating Bolt buff is
      p()->buffs.shard_of_annihilation->trigger( 3 );
    }

    warlock_spell_t::impact( s );
    
    auto e = make_event<ground_aoe_event_t>( *sim, p(), ground_aoe_params_t()
      .pulse_time( 0.1_s )
      .target( s->target )
      .n_pulses( 4 )
      .action( decimating_bolt_dmg ), true );

    if ( s->chain_target > 0 )
      e->pulse_state->persistent_multiplier *= base_aoe_multiplier;

  };

};

// TOCHECK: Does the damage proc affect Seed of Corruption? If so, this needs to be split into specs as well
struct grimoire_of_sacrifice_t : public warlock_spell_t
{
  grimoire_of_sacrifice_t( warlock_t* p, util::string_view options_str )
    : warlock_spell_t( "grimoire_of_sacrifice", p, p->talents.grimoire_of_sacrifice )
  {
    parse_options( options_str );
    harmful               = false;
    ignore_false_positive = true;
  }

  bool ready() override
  {
    if ( !p()->warlock_pet_list.active )
      return false;

    return warlock_spell_t::ready();
  }

  void execute() override
  {
    warlock_spell_t::execute();

    if ( p()->warlock_pet_list.active )
    {
      p()->warlock_pet_list.active->dismiss();
      p()->warlock_pet_list.active = nullptr;
      p()->buffs.grimoire_of_sacrifice->trigger();
    }
  }
};
}  // namespace actions

warlock_td_t::warlock_td_t( player_t* target, warlock_t& p )
  : actor_target_data_t( target, &p ), soc_threshold( 0.0 ), warlock( p )
{
  dots_drain_life = target->get_dot( "drain_life", &p );
  dots_drain_life_aoe = target->get_dot( "drain_life_aoe", &p );
  dots_scouring_tithe = target->get_dot( "scouring_tithe", &p );
  dots_impending_catastrophe = target->get_dot( "impending_catastrophe_dot", &p );
  dots_soul_rot       = target->get_dot( "soul_rot", &p );

  // Aff
  dots_corruption          = target->get_dot( "corruption", &p );
  dots_agony               = target->get_dot( "agony", &p );
  dots_drain_soul          = target->get_dot( "drain_soul", &p );
  dots_phantom_singularity = target->get_dot( "phantom_singularity", &p );
  dots_siphon_life         = target->get_dot( "siphon_life", &p );
  dots_seed_of_corruption  = target->get_dot( "seed_of_corruption", &p );
  dots_unstable_affliction = target->get_dot( "unstable_affliction", &p );
  dots_vile_taint          = target->get_dot( "vile_taint", &p );

  debuffs_haunt = make_buff( *this, "haunt", source->find_spell( 48181 ) )
                      ->set_refresh_behavior( buff_refresh_behavior::PANDEMIC )
                      ->set_default_value_from_effect( 2 );
  debuffs_shadow_embrace = make_buff( *this, "shadow_embrace", source->find_spell( 32390 ) )
                               ->set_default_value_from_effect( 1 )
                               ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                               ->set_max_stack( 3 );

  // Destro
  dots_immolate          = target->get_dot( "immolate", &p );

  debuffs_eradication = make_buff( *this, "eradication", source->find_spell( 196414 ) )
                            ->set_refresh_behavior( buff_refresh_behavior::DURATION )
                            ->set_default_value_from_effect( 1 );
  debuffs_roaring_blaze = make_buff( *this, "roaring_blaze", source->find_spell( 265931 ) );
  debuffs_shadowburn    = make_buff( *this, "shadowburn", source->find_spell( 17877 ) )
                              ->set_default_value( source->find_spell( 245731 )->effectN( 1 ).base_value() );
  debuffs_havoc         = make_buff( *this, "havoc", source->find_specialization_spell( 80240 ) )
                      ->set_duration( source->find_specialization_spell( 80240 )->duration() +
                                      source->find_specialization_spell( 335174 )->effectN( 1 ).time_value() )
                      ->set_cooldown( 0_ms )
                      ->set_stack_change_callback( [ &p ]( buff_t* b, int, int cur ) {
                        if ( cur == 0 )
                        {
                          p.get_target_data( p.havoc_target )->debuffs_odr->expire();
                          p.havoc_target = nullptr;
                        }
                        else
                          p.havoc_target = b->player;

                        range::for_each( p.havoc_spells, []( action_t* a ) { a->target_cache.is_valid = false; } );
                      } );

  // SL - Legendary
  debuffs_odr = make_buff( *this, "odr_shawl_of_the_ymirjar", source->find_spell(337164) );

  // SL - Conduit
  //Spell data appears to be missing for a "debuff" type effect, creating a fake one to model the behavior
  //TOCHECK regularly to see if this can be less kludged
  debuffs_combusting_engine = make_buff( *this, "combusting_engine" )
                                  ->set_duration( 30_s )
                                  ->set_max_stack( 40 )
                                  ->set_default_value( source->find_conduit_spell("Combusting Engine").percent() );

  // Demo
  dots_doom         = target->get_dot( "doom", &p );

  debuffs_from_the_shadows = make_buff( *this, "from_the_shadows", source->find_spell( 270569 ) )
                                 ->set_default_value_from_effect( 1 );

  target->register_on_demise_callback( &p, [ this ]( player_t* ) { target_demise(); } );
}

void warlock_td_t::target_demise()
{
  if ( !( target->is_enemy() ) )
  {
    return;
  }

  if ( warlock.spec.unstable_affliction_2->ok() )
  {
    if ( dots_unstable_affliction->is_ticking() )
    {
      warlock.sim->print_log( "Player {} demised. Warlock {} gains a shard from Unstable Affliction.", target->name(),
                              warlock.name() );

      warlock.resource_gain( RESOURCE_SOUL_SHARD, 1, warlock.gains.unstable_affliction_refund );
    }
  }
  if ( dots_drain_soul->is_ticking() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} gains a shard from Drain Soul.", target->name(),
                            warlock.name() );

    warlock.resource_gain( RESOURCE_SOUL_SHARD, 1, warlock.gains.drain_soul );
  }

  if ( dots_scouring_tithe->is_ticking() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} gains 5 shards from Scouring Tithe.", target->name(),
                            warlock.name() );

    warlock.resource_gain( RESOURCE_SOUL_SHARD, 5, warlock.gains.scouring_tithe );

    if ( warlock.conduit.soul_tithe.value() > 0 )
    {
      warlock.buffs.soul_tithe->trigger();
    }

    if ( warlock.legendary.languishing_soul_detritus.ok() )
    {
      warlock.buffs.languishing_soul_detritus->trigger();
    }
  }

  if ( debuffs_haunt->check() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} reset Haunt's cooldown.", target->name(), warlock.name() );

    warlock.cooldowns.haunt->reset( true );
  }

  if ( debuffs_shadowburn->check() )
  {
    if ( warlock.min_version_check( VERSION_9_1_0 ) )
    {
      warlock.sim->print_log( "Player {} demised. Warlock {} refunds one charge of Shadowburn.", target->name(),
                            warlock.name() );
      
      warlock.cooldowns.shadowburn->reset( true );
    }
   
    warlock.sim->print_log( "Player {} demised. Warlock {} gains 1 shard from Shadowburn.", target->name(), warlock.name() );

    warlock.resource_gain( RESOURCE_SOUL_SHARD, debuffs_shadowburn->check_value() / 10,
                           warlock.gains.shadowburn_refund );
  }

  if ( dots_agony->is_ticking() && warlock.legendary.wrath_of_consumption.ok() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} triggers Wrath of Consumption from Agony.", target->name(), warlock.name() );

    warlock.buffs.wrath_of_consumption->trigger();
  }

  if ( dots_corruption->is_ticking() && warlock.legendary.wrath_of_consumption.ok() )
  {
    warlock.sim->print_log( "Player {} demised. Warlock {} triggers Wrath of Consumption from Corruption.", target->name(), warlock.name() );

    warlock.buffs.wrath_of_consumption->trigger();
  }
}

static void accumulate_seed_of_corruption( warlock_td_t* td, double amount )
{
  td->soc_threshold -= amount;

  if ( td->soc_threshold <= 0 )
  {
    td->dots_seed_of_corruption->cancel();
  }
  else
  {
    if ( td->source->sim->log )
      td->source->sim->out_log.printf( "remaining damage to explode seed %f", td->soc_threshold );
  }
}

int warlock_td_t::count_affliction_dots()
{
  int count = 0;

  if ( dots_agony->is_ticking() )
    count++;

  if ( dots_corruption->is_ticking() )
    count++;

  if ( dots_unstable_affliction->is_ticking() )
    count++;

  if ( dots_vile_taint->is_ticking() )
    count++;

  if ( dots_phantom_singularity->is_ticking() )
    count++;

  if ( dots_soul_rot->is_ticking() )
    count++;

  if ( dots_siphon_life->is_ticking() )
    count++;

  if ( dots_scouring_tithe->is_ticking() )
    count++;

  if ( dots_impending_catastrophe->is_ticking() )
    count++;

  return count;
}


warlock_t::warlock_t( sim_t* sim, util::string_view name, race_e r )
  : player_t( sim, WARLOCK, name, r ),
    havoc_target( nullptr ),
    ua_target( nullptr ),
    havoc_spells(),
    agony_accumulator( 0.0 ),
    corruption_accumulator( 0.0 ),
    active_pets( 0 ),
    warlock_pet_list( this ),
    active(),
    talents(),
    legendary(),
    conduit(),
    covenant(),
    mastery_spells(),
    cooldowns(),
    spec(),
    buffs(),
    gains(),
    procs(),
    initial_soul_shards( 3 ),
    default_pet()
{
  cooldowns.haunt               = get_cooldown( "haunt" );
  cooldowns.phantom_singularity = get_cooldown( "phantom_singularity" );
  cooldowns.darkglare           = get_cooldown( "summon_darkglare" );
  cooldowns.demonic_tyrant      = get_cooldown( "summon_demonic_tyrant" );
  cooldowns.scouring_tithe      = get_cooldown( "scouring_tithe" );
  cooldowns.infernal            = get_cooldown( "summon_infernal" );
  cooldowns.shadowburn          = get_cooldown( "shadowburn" );

  resource_regeneration             = regen_type::DYNAMIC;
  regen_caches[ CACHE_HASTE ]       = true;
  regen_caches[ CACHE_SPELL_HASTE ] = true;
}

void warlock_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_MASTERY:
      if ( mastery_spells.master_demonologist->ok() )
        player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      break;

    default:
      break;
  }
}

double warlock_t::composite_player_target_multiplier( player_t* target, school_e school ) const
{
  double m = player_t::composite_player_target_multiplier( target, school );

  const warlock_td_t* td = get_target_data( target );

  if ( specialization() == WARLOCK_AFFLICTION )
  {
    if ( td->debuffs_haunt->check() )
      m *= 1.0 + td->debuffs_haunt->check_value();
	  
	if ( !min_version_check( VERSION_9_1_0 ) )
    {
      m *= 1.0 + ( ( td->debuffs_shadow_embrace->check_value() ) * ( 1 + conduit.cold_embrace.percent() )
           * td->debuffs_shadow_embrace->check() );
    }

    if ( min_version_check( VERSION_9_1_0 ) && talents.shadow_embrace->ok() )
    {
      m *= 1.0 + td->debuffs_shadow_embrace->check_stack_value();
    }
  }

  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    if ( td->debuffs_eradication->check() )
      m *= 1.0 + td->debuffs_eradication->check_value();
  }

  return m;
}

double warlock_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  return m;
}

double warlock_t::composite_player_pet_damage_multiplier( const action_state_t* s, bool guardian ) const
{
  double m = player_t::composite_player_pet_damage_multiplier( s, guardian );

  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    m *= 1.0 + spec.destruction->effectN( 3 ).percent();
  }
  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    m *= 1.0 + spec.demonology->effectN( 3 ).percent();
    m *= 1.0 + cache.mastery_value();

    if ( buffs.demonic_power->check() )
      m *= 1.0 + buffs.demonic_power->default_value;

    if ( buffs.tyrants_soul->check() )
      m *= 1.0 + buffs.tyrants_soul->current_value;

    if ( buffs.soul_tithe->check() )
      m *= 1.0 + buffs.soul_tithe->check_stack_value();

  }
  if ( specialization() == WARLOCK_AFFLICTION )
  {
    m *= 1.0 + spec.affliction->effectN( 3 ).percent();
  }
  return m;
}

double warlock_t::composite_player_target_pet_damage_multiplier( player_t* target, bool guardian ) const
{
  double m = player_t::composite_player_target_pet_damage_multiplier( target, guardian );

  if ( !min_version_check( VERSION_9_1_0 ) )
    return m;

  const warlock_td_t* td = get_target_data( target );

  if ( specialization() == WARLOCK_AFFLICTION )
  {
    if ( td->debuffs_haunt->check() )
    {
      m *= 1.0 + td->debuffs_haunt->data().effectN( guardian ? 4 : 3 ).percent();
    }

    if ( talents.shadow_embrace->ok() )
    {
      m *= 1.0 + td->debuffs_shadow_embrace->data().effectN( guardian ? 3 : 2 ).percent();
    }
  }

  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    if ( td->debuffs_eradication->check() )
    {
      m *= 1.0 + td->debuffs_eradication->data().effectN( guardian ? 3 : 2 ).percent();
    }
  }

  return m;
}

double warlock_t::composite_spell_crit_chance() const
{
  double sc = player_t::composite_spell_crit_chance();

  if ( buffs.dark_soul_instability->check() )
    sc += buffs.dark_soul_instability->check_value();

  return sc;
}

double warlock_t::composite_spell_haste() const
{
  double h = player_t::composite_spell_haste();

  if ( buffs.reverse_entropy->check() )
    h *= 1.0 / ( 1.0 + buffs.reverse_entropy->check_value() );

  return h;
}

double warlock_t::composite_melee_haste() const
{
  double h = player_t::composite_melee_haste();

  if ( buffs.reverse_entropy->check() )
    h *= 1.0 / ( 1.0 + buffs.reverse_entropy->check_value() );

  return h;
}

double warlock_t::composite_melee_crit_chance() const
{
  double mc = player_t::composite_melee_crit_chance();

  if ( buffs.dark_soul_instability->check() )
    mc += buffs.dark_soul_instability->check_value();

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

double warlock_t::resource_regen_per_second( resource_e r ) const
{
  double reg = player_t::resource_regen_per_second( r );
  return reg;
}

double warlock_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );
  if ( attr == ATTR_STAMINA )
    m *= 1.0 + spec.demonic_embrace->effectN( 1 ).percent();
    
  return m;
}

//Note: Level is checked to be >=27 by the function calling this. This is technically wrong for warlocks due to
//a missing level requirement in data, but correct generally.
double warlock_t::matching_gear_multiplier( attribute_e attr ) const
{
  if ( attr == ATTR_INTELLECT )
    return spec.nethermancy->effectN( 1 ).percent();

  return 0.0;
}

action_t* warlock_t::create_action( util::string_view action_name, const std::string& options_str )
{
  using namespace actions;

  if ( ( action_name == "summon_pet" ) && default_pet.empty() )
  {
    sim->errorf( "Player %s used a generic pet summoning action without specifying a default_pet.\n", name() );
    return nullptr;
  }
  // Pets
  if ( action_name == "summon_felhunter" )
    return new summon_main_pet_t( "felhunter", this );
  if ( action_name == "summon_felguard" )
    return new summon_main_pet_t( "felguard", this );
  if ( action_name == "summon_succubus" )
    return new summon_main_pet_t( "succubus", this );
  if ( action_name == "summon_voidwalker" )
    return new summon_main_pet_t( "voidwalker", this );
  if ( action_name == "summon_imp" )
    return new summon_main_pet_t( "imp", this );
  if ( action_name == "summon_pet" )
    return new summon_main_pet_t( default_pet, this );
  // Base Spells
  if ( action_name == "drain_life" )
    return new drain_life_t( this, options_str );
  if ( action_name == "grimoire_of_sacrifice" )
    return new grimoire_of_sacrifice_t( this, options_str );  // aff and destro
  if ( action_name == "decimating_bolt" )
    return new decimating_bolt_t( this, options_str );
  if ( action_name == "scouring_tithe" )
    return new scouring_tithe_t( this, options_str );
  if ( action_name == "impending_catastrophe" )
    return new impending_catastrophe_t( this, options_str );
  if ( action_name == "soul_rot" )
    return new soul_rot_t( this, options_str );

  if ( specialization() == WARLOCK_AFFLICTION )
  {
    action_t* aff_action = create_action_affliction( action_name, options_str );
    if ( aff_action )
      return aff_action;
  }

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    action_t* demo_action = create_action_demonology( action_name, options_str );
    if ( demo_action )
      return demo_action;
  }

  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    action_t* destro_action = create_action_destruction( action_name, options_str );
    if ( destro_action )
      return destro_action;
  }

  return player_t::create_action( action_name, options_str );
}

pet_t* warlock_t::create_pet( util::string_view pet_name, util::string_view pet_type )
{
  pet_t* p = find_pet( pet_name );
  if ( p )
    return p;
  using namespace pets;

  pet_t* summon_pet = create_main_pet( pet_name, pet_type );
  if ( summon_pet )
    return summon_pet;

  return nullptr;
}

void warlock_t::create_pets()
{
  create_all_pets();

  for ( auto& pet : pet_name_list )
  {
    create_pet( pet );
  }
}

void warlock_t::create_buffs()
{
  player_t::create_buffs();

  create_buffs_affliction();
  create_buffs_demonology();
  create_buffs_destruction();

  buffs.grimoire_of_sacrifice =
      make_buff( this, "grimoire_of_sacrifice", talents.grimoire_of_sacrifice->effectN( 2 ).trigger() )
          ->set_chance( 1.0 );

  // Covenants
  buffs.soul_rot = make_buff(this, "soul_rot", covenant.soul_rot);

  // 4.0 is the multiplier for a 0% health mob
  buffs.decimating_bolt =
      make_buff( this, "decimating_bolt", find_spell( 325299 ) )
                              ->set_default_value( 2.0 )
                              ->set_max_stack( talents.drain_soul->ok() ? 1 : 3 );

  // Conduits
  buffs.soul_tithe = make_buff(this, "soul_tithe", find_spell(340238))
    ->set_default_value(conduit.soul_tithe.percent());

  // Legendaries
  buffs.wrath_of_consumption = make_buff( this, "wrath_of_consumption", find_spell( 337130 ) )
                               ->set_default_value_from_effect( 1 );

  buffs.demonic_synergy = make_buff( this, "demonic_synergy", find_spell( 337060 ) )
                              ->set_default_value( legendary.relic_of_demonic_synergy->effectN( 1 ).percent() * ( this->specialization() == WARLOCK_DEMONOLOGY ? 1.5 : 1.0 ) );

  buffs.languishing_soul_detritus = make_buff( this, "languishing_soul_detritus", find_spell( 356255 ) )
                                        ->set_pct_buff_type( STAT_PCT_BUFF_CRIT )
                                        ->set_default_value( find_spell( 356255 )->effectN( 2 ).percent() );

  buffs.shard_of_annihilation = make_buff( this, "shard_of_annihilation", find_spell( 356342 ) );

  buffs.decaying_soul_satchel_haste = make_buff( this, "decaying_soul_satchel_haste", find_spell( 356369 ) )
                                          ->set_pct_buff_type( STAT_PCT_BUFF_HASTE )
                                          ->set_default_value( find_spell( 356369 )->effectN( 1 ).percent() );

  buffs.decaying_soul_satchel_crit = make_buff( this, "decaying_soul_satchel_crit", find_spell( 356369 ) )
                                         ->set_pct_buff_type( STAT_PCT_BUFF_CRIT )
                                         ->set_default_value( find_spell( 356369 )->effectN( 2 ).percent() );
}

void warlock_t::init_spells()
{
  player_t::init_spells();

  warlock_t::init_spells_affliction();
  warlock_t::init_spells_demonology();
  warlock_t::init_spells_destruction();

  // General
  spec.nethermancy = find_spell( 86091 );
  spec.demonic_embrace = find_spell( 288843 );

  version_9_1_0_data = find_spell( 356342 ); //For 9.1 PTR version checking, Shard of Annihilation data

  // Specialization Spells
  spec.immolate         = find_specialization_spell( "Immolate" );
  spec.demonic_core     = find_specialization_spell( "Demonic Core" );

  // Talents
  talents.demon_skin                = find_talent_spell( "Demon Skin" );
  talents.burning_rush              = find_talent_spell( "Burning Rush" );
  talents.dark_pact                 = find_talent_spell( "Dark Pact" );
  talents.darkfury                  = find_talent_spell( "Darkfury" );
  talents.mortal_coil               = find_talent_spell( "Mortal Coil" );
  talents.howl_of_terror            = find_talent_spell( "Howl of Terror" );
  talents.grimoire_of_sacrifice     = find_talent_spell( "Grimoire of Sacrifice" );       // Aff/Destro
  talents.soul_conduit              = find_talent_spell( "Soul Conduit" );

  // Legendaries
  legendary.claw_of_endereth                     = find_runeforge_legendary( "Claw of Endereth" );
  legendary.relic_of_demonic_synergy             = find_runeforge_legendary( "Relic of Demonic Synergy" );
  legendary.wilfreds_sigil_of_superior_summoning = find_runeforge_legendary( "Wilfred's Sigil of Superior Summoning" );
  // Sacrolash is the only spec-specific legendary that can be used by other specs.
  legendary.sacrolashs_dark_strike = find_runeforge_legendary( "Sacrolash's Dark Strike" );
  //Wrath is implemented here to catch any potential cross-spec periodic effects
  legendary.wrath_of_consumption = find_runeforge_legendary("Wrath of Consumption");

  legendary.languishing_soul_detritus = find_runeforge_legendary( "Languishing Soul Detritus" );
  legendary.shard_of_annihilation = find_runeforge_legendary( "Shard of Annihilation" );
  legendary.decaying_soul_satchel = find_runeforge_legendary( "Decaying Soul Satchel" );
  legendary.contained_perpetual_explosion = find_runeforge_legendary( "Contained Perpetual Explosion" );

  // Conduits
  conduit.catastrophic_origin  = find_conduit_spell( "Catastrophic Origin" );   // Venthyr
  conduit.soul_eater           = find_conduit_spell( "Soul Eater" );            // Night Fae
  conduit.fatal_decimation     = find_conduit_spell( "Fatal Decimation" );      // Necrolord
  conduit.soul_tithe           = find_conduit_spell( "Soul Tithe" );            // Kyrian
  conduit.duplicitous_havoc    = find_conduit_spell("Duplicitous Havoc");       // Needed in main for covenants

  // Covenant Abilities
  covenant.decimating_bolt       = find_covenant_spell( "Decimating Bolt" );        // Necrolord
  covenant.impending_catastrophe = find_covenant_spell( "Impending Catastrophe" );  // Venthyr
  covenant.scouring_tithe        = find_covenant_spell( "Scouring Tithe" );         // Kyrian
  covenant.soul_rot              = find_covenant_spell( "Soul Rot" );               // Night Fae
}

void warlock_t::init_rng()
{
  if ( specialization() == WARLOCK_AFFLICTION )
    init_rng_affliction();
  if ( specialization() == WARLOCK_DEMONOLOGY )
    init_rng_demonology();
  if ( specialization() == WARLOCK_DESTRUCTION )
    init_rng_destruction();

  player_t::init_rng();
}

void warlock_t::init_gains()
{
  player_t::init_gains();

  if ( specialization() == WARLOCK_AFFLICTION )
    init_gains_affliction();
  if ( specialization() == WARLOCK_DEMONOLOGY )
    init_gains_demonology();
  if ( specialization() == WARLOCK_DESTRUCTION )
    init_gains_destruction();

  gains.miss_refund  = get_gain( "miss_refund" );
  gains.shadow_bolt  = get_gain( "shadow_bolt" );
  gains.soul_conduit = get_gain( "soul_conduit" );
  gains.scouring_tithe = get_gain( "souring_tithe" );
}

void warlock_t::init_procs()
{
  player_t::init_procs();

  if ( specialization() == WARLOCK_AFFLICTION )
    init_procs_affliction();
  if ( specialization() == WARLOCK_DEMONOLOGY )
    init_procs_demonology();
  if ( specialization() == WARLOCK_DESTRUCTION )
    init_procs_destruction();

  procs.one_shard_hog   = get_proc( "one_shard_hog" );
  procs.two_shard_hog   = get_proc( "two_shard_hog" );
  procs.three_shard_hog = get_proc( "three_shard_hog" );
  procs.portal_summon   = get_proc( "portal_summon" );
  procs.demonic_calling = get_proc( "demonic_calling" );
  procs.soul_conduit    = get_proc( "soul_conduit" );
  procs.corrupting_leer = get_proc( "corrupting_leer" );
  procs.carnivorous_stalkers = get_proc( "carnivorous_stalkers" );
  procs.horned_nightmare = get_proc( "horned_nightmare" );
}

void warlock_t::init_base_stats()
{
  if ( base.distance < 1.0 )
    base.distance = 40.0;

  player_t::init_base_stats();

  base.attack_power_per_strength = 0.0;
  base.attack_power_per_agility  = 0.0;
  base.spell_power_per_intellect = 1.0;

  resources.base[ RESOURCE_SOUL_SHARD ] = 5;

  if ( default_pet.empty() )
  {
    if ( specialization() == WARLOCK_AFFLICTION )
      default_pet = "imp";
    else if ( specialization() == WARLOCK_DEMONOLOGY )
      default_pet = "felguard";
    else if ( specialization() == WARLOCK_DESTRUCTION )
      default_pet = "imp";
  }
}

void warlock_t::init_scaling()
{
  player_t::init_scaling();

  if ( specialization() == WARLOCK_DEMONOLOGY )
    scaling->enable( STAT_STAMINA );
}

void warlock_t::init_assessors()
{
  player_t::init_assessors();

  auto assessor_fn = [ this ]( result_amount_type, action_state_t* s ) {
    if ( get_target_data( s->target )->dots_seed_of_corruption->is_ticking() )
    {
      accumulate_seed_of_corruption( get_target_data( s->target ), s->result_total );
    }

    return assessor::CONTINUE;
  };

  assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, assessor_fn );

  for ( auto pet : pet_list )
  {
    pet->assessor_out_damage.add( assessor::TARGET_DAMAGE - 1, assessor_fn );
  }
}

void warlock_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  precombat->add_action( "flask" );
  precombat->add_action( "food" );
  precombat->add_action( "augmentation" );
  precombat->add_action( "summon_pet" );
  if ( specialization() != WARLOCK_DEMONOLOGY )
    precombat->add_action( "grimoire_of_sacrifice,if=talent.grimoire_of_sacrifice.enabled" );

  precombat->add_action( "snapshot_stats" );

  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    precombat->add_action( "demonbolt" );
    precombat->add_action( "variable,name=tyrant_ready,value=0" );
  }
  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    precombat->add_talent( this, "Soul Fire" );
    precombat->add_action( "incinerate,if=!talent.soul_fire.enabled" );
  }
  if ( specialization() == WARLOCK_AFFLICTION )
  {
    precombat->add_action( "seed_of_corruption,if=spell_targets.seed_of_corruption_aoe>=3" );
    precombat->add_action( "haunt" );
    precombat->add_action( "unstable_affliction" );
  }
}

std::string warlock_t::default_potion() const
{
  return ( true_level >= 60 ) ? "spectral_intellect" : "disabled";
}

std::string warlock_t::default_flask() const
{
  return ( true_level >= 60 ) ? "spectral_flask_of_power" : "disabled";
}

std::string warlock_t::default_food() const
{
  return ( true_level >= 60 ) ? "feast_of_gluttonous_hedonism" : "disabled";
}

std::string warlock_t::default_rune() const
{
  return ( true_level >= 60 ) ? "veiled" : "disabled";
}

std::string warlock_t::default_temporary_enchant() const
{
  return ( true_level >= 60 ) ? "main_hand:shadowcore_oil" : "disabled";
}

void warlock_t::apl_global_filler()
{
}

void warlock_t::apl_default()
{
  if ( specialization() == WARLOCK_AFFLICTION )
    create_apl_affliction();
  else if ( specialization() == WARLOCK_DEMONOLOGY )
    create_apl_demonology();
  else if ( specialization() == WARLOCK_DESTRUCTION )
    create_apl_destruction();
}

void warlock_t::init_action_list()
{
  if ( action_list_str.empty() )
  {
    clear_action_priority_lists();

    apl_precombat();

    apl_default();

    use_default_action_list = true;
  }

  player_t::init_action_list();
}

void warlock_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_SOUL_SHARD ] = initial_soul_shards;
}

void warlock_t::init_special_effects()
{
  player_t::init_special_effects();

  if ( talents.grimoire_of_sacrifice->ok() )
  {
    auto const sac_effect = new special_effect_t( this );
    sac_effect->name_str = "grimoire_of_sacrifice_effect";
    sac_effect->spell_id = 196099;
    sac_effect->execute_action = new warlock::actions::grimoire_of_sacrifice_damage_t( this );
    special_effects.push_back( sac_effect );

    auto cb = new dbc_proc_callback_t( this, *sac_effect );

    cb->initialize();
    cb->deactivate();

    buffs.grimoire_of_sacrifice->set_stack_change_callback( [ cb ]( buff_t*, int, int new_ ){
        if ( new_ == 1 ) cb->activate();
        else cb->deactivate();
      } );
  }

  if ( legendary.relic_of_demonic_synergy->ok() )
  {
    auto const syn_effect = new special_effect_t( this );
    syn_effect->name_str = "demonic_synergy_effect";
    syn_effect->spell_id = 337057;
    special_effects.push_back( syn_effect );

    auto cb = new warlock::actions::demonic_synergy_callback_t( this, *syn_effect );

    cb->initialize();
  }
}

void warlock_t::combat_begin()
{
  player_t::combat_begin();

  if ( specialization() == WARLOCK_DEMONOLOGY && buffs.inner_demons && talents.inner_demons->ok() )
    buffs.inner_demons->trigger();
}

void warlock_t::reset()
{
  player_t::reset();

  range::for_each( sim->target_list, [ this ]( const player_t* t ) {
    if ( auto td = target_data[ t ] )
    {
      td->reset();
    }

    range::for_each( t->pet_list, [ this ]( const player_t* add ) {
      if ( auto td = target_data[ add ] )
      {
        td->reset();
      }
    } );
  } );

  warlock_pet_list.active            = nullptr;
  havoc_target                       = nullptr;
  ua_target                          = nullptr;
  agony_accumulator                  = rng().range( 0.0, 0.99 );
  corruption_accumulator             = rng().range( 0.0, 0.99 ); // TOCHECK - Unsure if it procs on application
  wild_imp_spawns.clear();
}

void warlock_t::create_options()
{
  player_t::create_options();

  add_option( opt_int( "soul_shards", initial_soul_shards ) );
  add_option( opt_string( "default_pet", default_pet ) );
}

// Used to determine how many Wild Imps are waiting to be spawned from Hand of Guldan
int warlock_t::get_spawning_imp_count()
{
  return as<int>( wild_imp_spawns.size() );
}

// Function for returning the time until a certain number of imps will have spawned
// In the case where count is equal to or greater than number of incoming imps, time to last imp is returned
// Otherwise, time to the Nth imp spawn will be returned
// All other cases will return 0. A negative (or zero) count value will behave as "all"
timespan_t warlock_t::time_to_imps( int count )
{
  timespan_t max = 0_ms;
  if ( count >= as<int>( wild_imp_spawns.size() ) || count <= 0 )
  {
    for ( auto ev : wild_imp_spawns )
    {
      timespan_t ex = debug_cast<actions::imp_delay_event_t*>( ev )->expected_time();
      if ( ex > max )
        max = ex;
    }
    return max;
  }
  else
  {
    std::priority_queue<timespan_t> shortest;
    for ( auto ev : wild_imp_spawns )
    {
      timespan_t ex = debug_cast<actions::imp_delay_event_t*>( ev )->expected_time();
      if ( as<int>( shortest.size() ) >= count && ex < shortest.top() )
      {
        shortest.pop();
        shortest.push( ex );
      }
      else if ( as<int>( shortest.size() ) < count )
      {
        shortest.push( ex );
      }
    }

    if ( !shortest.empty() )
    {
      return shortest.top();
    }
    else
    {
      return 0_ms;
    }
  }
}

void warlock_t::darkglare_extension_helper( warlock_t* p, timespan_t darkglare_extension )
{
  for ( const auto target : p->sim->target_non_sleeping_list )
  {
    warlock_td_t* td = p->get_target_data( target );
    if ( !td )
    {
      continue;
    }
    td->dots_agony->adjust_duration( darkglare_extension );
    td->dots_corruption->adjust_duration( darkglare_extension );
    td->dots_siphon_life->adjust_duration( darkglare_extension );
    td->dots_phantom_singularity->adjust_duration( darkglare_extension );
    td->dots_vile_taint->adjust_duration( darkglare_extension );
    td->dots_unstable_affliction->adjust_duration( darkglare_extension );
    td->dots_impending_catastrophe->adjust_duration( darkglare_extension );
    td->dots_scouring_tithe->adjust_duration( darkglare_extension );
    td->dots_soul_rot->adjust_duration( darkglare_extension );
  }
}

void warlock_t::malignancy_reduction_helper()
{
  if ( rng().roll( conduit.corrupting_leer.percent() ) )
  {
    procs.corrupting_leer->occur();
    cooldowns.darkglare->adjust( -5.0_s );  // Value is in the description so had to hardcode it
  }
}

// Use this as a helper function when two versions are needed simultaneously (ie a PTR cycle)
// It must be adjusted manually over time, and any use of it should be removed once a patch goes live
// Returns TRUE if actor's dbc version >= version specified
// When checking VERSION_PTR, will only return true if PTR dbc is being used, regardless of version number
bool warlock_t::min_version_check( version_check_e version ) const
{
  //If we ever get a full DBC version string checker, replace these returns with that function
  switch ( version )
  {
    case VERSION_PTR:
      return is_ptr();
    case VERSION_9_1_0:
      return !( version_9_1_0_data == spell_data_t::not_found() );
    case VERSION_9_0_5:
    case VERSION_9_0_0:
    case VERSION_ANY:
      return true;
  }

  return false;
}

// Function for returning the the number of imps that will spawn in a specified time period.
int warlock_t::imps_spawned_during( timespan_t period )
{
  int count = 0;

  for ( auto ev : wild_imp_spawns )
  {
    timespan_t ex = debug_cast<actions::imp_delay_event_t*>( ev )->expected_time();

    if ( ex < period )
      count++;
  }

  return count;
}

std::string warlock_t::create_profile( save_e stype )
{
  std::string profile_str = player_t::create_profile( stype );

  if ( stype & SAVE_PLAYER )
  {
    if ( initial_soul_shards != 3 )
      profile_str += "soul_shards=" + util::to_string( initial_soul_shards ) + "\n";
    if ( !default_pet.empty() )
      profile_str += "default_pet=" + default_pet + "\n";
  }

  return profile_str;
}

void warlock_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  warlock_t* p = debug_cast<warlock_t*>( source );

  initial_soul_shards  = p->initial_soul_shards;
  default_pet          = p->default_pet;
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
    default:
      return s;
  }
}

pet_t* warlock_t::create_main_pet( util::string_view pet_name, util::string_view pet_type )
{
  pet_t* p = find_pet( pet_name );
  if ( p )
    return p;
  using namespace pets;

  if ( pet_name == "felhunter" )
    return new pets::base::felhunter_pet_t( this, pet_name );
  if ( pet_name == "imp" )
    return new pets::base::imp_pet_t( this, pet_name );
  if ( pet_name == "succubus" )
    return new pets::base::succubus_pet_t( this, pet_name );
  if ( pet_name == "voidwalker" )
    return new pets::base::voidwalker_pet_t( this, pet_name );
  if ( specialization() == WARLOCK_DEMONOLOGY )
  {
    return create_demo_pet( pet_name, pet_type );
  }

  return nullptr;
}

void warlock_t::create_all_pets()
{
  if ( specialization() == WARLOCK_DESTRUCTION )
  {
    for ( size_t i = 0; i < warlock_pet_list.infernals.size(); i++ )
    {
      warlock_pet_list.infernals[ i ] = new pets::destruction::infernal_t( this );
    }
  }

  if ( specialization() == WARLOCK_AFFLICTION )
  {
    for ( size_t i = 0; i < warlock_pet_list.darkglare.size(); i++ )
    {
      warlock_pet_list.darkglare[ i ] = new pets::affliction::darkglare_t( this );
    }
  }
}

//TODO: Are these expressions outdated?
std::unique_ptr<expr_t> warlock_t::create_pet_expression( util::string_view name_str )
{
  if ( name_str == "last_cast_imps" )
  {
    return make_fn_expr( "last_cast_imps", [ this ]() {
      return warlock_pet_list.wild_imps.n_active_pets( []( const pets::demonology::wild_imp_pet_t* pet ) {
        return pet->resources.current[ RESOURCE_ENERGY ] <= 20;
      } );
    } );
  }
  else if ( name_str == "two_cast_imps" )
  {
    return make_fn_expr( "two_cast_imps", [ this ]() {
      return warlock_pet_list.wild_imps.n_active_pets( []( const pets::demonology::wild_imp_pet_t* pet ) {
        return pet->resources.current[ RESOURCE_ENERGY ] <= 40;
      } );
    } );
  }

  return player_t::create_expression( name_str );
}

std::unique_ptr<expr_t> warlock_t::create_expression( util::string_view name_str )
{
  if ( name_str == "time_to_shard" )
  {
    auto agony_id = find_action_id( "agony" );

    return make_fn_expr( name_str, [ this, agony_id ]() {
      auto td               = get_target_data( target );
      double active_agonies = get_active_dots( agony_id );
      if ( sim->debug )
        sim->out_debug.printf( "active agonies: %f", active_agonies );
      dot_t* agony = td->dots_agony;
      if ( active_agonies == 0 || !agony->current_action )
      {
        return std::numeric_limits<double>::infinity();
      }
      action_state_t* agony_state = agony->current_action->get_state( agony->state );
      timespan_t dot_tick_time    = agony->current_action->tick_time( agony_state );

      // Seeks to return the average expected time for the player to generate a single soul shard.
      // TOCHECK regularly.

      double average =
          1.0 / ( 0.184 * std::pow( active_agonies, -2.0 / 3.0 ) ) * dot_tick_time.total_seconds() / active_agonies;

      if ( talents.creeping_death->ok() )
        average /= 1.0 + talents.creeping_death->effectN( 1 ).percent();

      if ( sim->debug )
        sim->out_debug.printf( "time to shard return: %f", average );
      action_state_t::release( agony_state );
      return average;
    } );
  }
  else if ( name_str == "pet_count" )
  {
    return make_ref_expr( name_str, active_pets );
  }
  else if ( name_str == "last_cast_imps" )
  {
    return create_pet_expression( name_str );
  }
  else if ( name_str == "two_cast_imps" )
  {
    return create_pet_expression( name_str );
  }
  // TODO: Remove the deprecated buff expressions once the APL is adjusted for
  // havoc_active/havoc_remains.
  else if ( name_str == "havoc_active" || name_str == "buff.active_havoc.up" )
  {
    return make_fn_expr( name_str, [ this ] { return havoc_target != nullptr; } );
  }
  else if ( name_str == "havoc_remains" || name_str == "buff.active_havoc.remains" )
  {
    return make_fn_expr( name_str, [ this ] {
      return havoc_target ? get_target_data( havoc_target )->debuffs_havoc->remains().total_seconds() : 0.0;
    } );
  }
  else if ( name_str == "incoming_imps" )
  {
    return make_fn_expr( name_str, [ this ] { return this->get_spawning_imp_count(); } );
  }
  else if ( name_str == "can_seed" )
  {
    std::vector<action_t*> soc_list;
    for ( auto a : action_list )
    {
      if ( a->name_str == "seed_of_corruption" )
        soc_list.push_back( a );
    }

    return make_fn_expr( name_str, [this, soc_list] {
      std::vector<player_t*> no_dots;

      if ( soc_list.empty() ) 
        return false;

      //All the actions should have the same target list, so do this once only
      auto tl = soc_list[ 0 ]->target_list();

      for ( auto t : tl )
      {
        if ( !get_target_data( t )->dots_seed_of_corruption->is_ticking() )
          no_dots.push_back( t );
      }

      //If there are no targets without a seed already, this expression should be false
      if ( no_dots.empty() )
        return false;

      //If all of the remaining unseeded targets have a seed in flight, we should also return false
      for ( auto t : no_dots )
      {
        bool can_seed = true;

        for ( auto s : soc_list )
        {
          if ( s->has_travel_events_for( t ) )
          {
            can_seed = false;
            break;
          }
        }

        if ( can_seed )
          return true;
      }

      return false;
    });
  }

  auto splits = util::string_split<util::string_view>( name_str, "." );

  if ( splits.size() == 3 && splits[ 0 ] == "time_to_imps" && splits[ 2 ] == "remains" )
  {
    auto amt = splits[ 1 ] == "all" ? -1 : util::to_int( splits[ 1 ] );

    return make_fn_expr( name_str, [ this, amt ]() {
      return this->time_to_imps( amt );
    } );
  }
  //TODO: Is this outdated in Shadowlands?
  else if ( splits.size() == 2 && util::str_compare_ci( splits[ 0 ], "imps_spawned_during" ) )
  {
    auto period = util::to_double( splits[ 1 ] );

    return make_fn_expr( name_str, [ this, period ]() {
      // Add a custom split .summon_demonic_tyrant which returns its cast time.
      return this->imps_spawned_during( timespan_t::from_millis( period ) );
    } );
  }

  return player_t::create_expression( name_str );
}

void warlock_t::apply_affecting_auras( action_t& action )
{
  player_t::apply_affecting_auras( action );

  if ( spec.demonology )
  {
    action.apply_affecting_aura( spec.demonology );
  }
  if ( spec.destruction )
  {
    action.apply_affecting_aura( spec.destruction );
  }
  if ( spec.affliction )
  {
    action.apply_affecting_aura( spec.affliction );
  }
}

struct warlock_module_t : public module_t
{
  warlock_module_t() : module_t( WARLOCK )
  {
  }

  player_t* create_player( sim_t* sim, util::string_view name, race_e r = RACE_NONE ) const override
  {
    return new warlock_t( sim, name, r );
  }

  //TODO: Hotfix may not be needed any longer, if so leave this function empty instead
  void register_hotfixes() const override
  {
    hotfix::register_spell("Warlock", "2020-11-15", "Manually set secondary Malefic Rapture level requirement", 324540)
      .field( "spell_level" )
      .operation( hotfix::HOTFIX_SET )
      .modifier( 11.0 )
      .verification_value( 43.0 );
  }

  bool valid() const override
  {
    return true;
  }
  void init( player_t* ) const override
  {
  }
  void combat_begin( sim_t* ) const override
  {
  }
  void combat_end( sim_t* ) const override
  {
  }
};

warlock::warlock_t::pets_t::pets_t( warlock_t* w )
  : active( nullptr ),
    last( nullptr ),
    roc_infernals( "roc_infernal", w ),
    dreadstalkers( "dreadstalker", w ),
    vilefiends( "vilefiend", w ),
    demonic_tyrants( "demonic_tyrant", w ),
    grimoire_felguards( "grimoire_felguard", w ),
    wild_imps( "wild_imp", w ),
    shivarra( "shivarra", w ),
    darkhounds( "darkhound", w ),
    bilescourges( "bilescourge", w ),
    urzuls( "urzul", w ),
    void_terrors( "void_terror", w ),
    wrathguards( "wrathguard", w ),
    vicious_hellhounds( "vicious_hellhound", w ),
    illidari_satyrs( "illidari_satyr", w ),
    eyes_of_guldan( "eye_of_guldan", w ),
    prince_malchezaar( "prince_malchezaar", w )
{
}
}  // namespace warlock

const module_t* module_t::warlock()
{
  static warlock::warlock_module_t m;
  return &m;
}
