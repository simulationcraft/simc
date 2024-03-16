#include "simulationcraft.hpp"

#include "sc_warlock.hpp"

namespace warlock
{
namespace actions_demonology
{
using namespace actions;

struct demonology_spell_t : public warlock_spell_t
{

bool procs_shadow_invocation_direct;
bool procs_shadow_invocation_tick;

public:
  demonology_spell_t( util::string_view token, warlock_t* p, const spell_data_t* s = spell_data_t::nil() )
    : warlock_spell_t( token, p, s )
  {
    procs_shadow_invocation_direct = false;
    procs_shadow_invocation_tick = false;
  }

  void consume_resource() override
  {
    warlock_spell_t::consume_resource();

    if ( resource_current == RESOURCE_SOUL_SHARD && p()->in_combat )
    {
      if ( p()->buffs.nether_portal->up() )
      {
        p()->proc_actions.summon_nether_portal_demon->execute();
        p()->procs.portal_summon->occur();

        if ( p()->talents.guldans_ambition->ok() && !p()->min_version_check( VERSION_10_2_0 ) )
          p()->buffs.nether_portal_total->trigger();

        if ( !p()->min_version_check( VERSION_10_2_0 ) && p()->talents.nerzhuls_volition->ok() && rng().roll( p()->talents.nerzhuls_volition->effectN( 1 ).percent() ) )
        {
          p()->proc_actions.summon_nether_portal_demon->execute();
          p()->procs.nerzhuls_volition->occur();

          if ( p()->talents.guldans_ambition->ok() )
            p()->buffs.nether_portal_total->trigger();
        }
      }
    }
  }

  double composite_target_multiplier( player_t* t ) const override
  {
    double m = warlock_spell_t::composite_target_multiplier( t );

    if ( p()->talents.the_houndmasters_stratagem->ok() && data().affected_by( p()->talents.the_houndmasters_stratagem_debuff->effectN( 1 ) ) )
    {
      m *= 1.0 + td( t )->debuffs_the_houndmasters_stratagem->check_value();
    }

    return m;
  }

  double action_multiplier() const override
  {
    double pm = warlock_spell_t::action_multiplier();

    if ( data().affected_by( p()->warlock_base.master_demonologist->effectN( 2 ) ) )
      pm *= 1.0 + p()->cache.mastery_value();

    return pm;
  }

  void impact( action_state_t* s ) override
  {
    warlock_spell_t::impact( s );

    if ( procs_shadow_invocation_direct && p()->talents.shadow_invocation->ok() && rng().roll( p()->shadow_invocation_proc_chance ) )
    {
      p()->proc_actions.bilescourge_bombers_proc->execute_on_target( s->target );
      p()->procs.shadow_invocation->occur();
    }
  }

  void tick( dot_t* d ) override
  {
    warlock_spell_t::tick( d );

    if ( procs_shadow_invocation_tick && p()->talents.shadow_invocation->ok() && rng().roll( p()->shadow_invocation_proc_chance ) )
    {
      p()->proc_actions.bilescourge_bombers_proc->execute_on_target( d->target );
      p()->procs.shadow_invocation->occur();
    }
  }
};

struct hand_of_guldan_t : public demonology_spell_t
{
  struct umbral_blaze_dot_t : public demonology_spell_t
  {
    umbral_blaze_dot_t( warlock_t* p ) : demonology_spell_t( "Umbral Blaze", p, p->talents.umbral_blaze_dot )
    {
      background = dual = true;
      hasted_ticks = false;
      base_td_multiplier = 1.0 + p->talents.umbral_blaze->effectN( 2 ).percent();
      procs_shadow_invocation_tick = true;
    }
  };

  struct hog_impact_t : public demonology_spell_t
  {
    int shards_used;
    timespan_t meteor_time;
    umbral_blaze_dot_t* blaze;

    hog_impact_t( warlock_t* p )
      : demonology_spell_t( "Hand of Gul'dan (Impact)", p, p->warlock_base.hog_impact ),
        shards_used( 0 ),
        meteor_time( 400_ms )
    {
      aoe = -1;
      dual = true;

      procs_shadow_invocation_direct = true;

      if ( p->talents.umbral_blaze->ok() )
      {
        blaze = new umbral_blaze_dot_t( p );
        add_child( blaze );
      }
    }

    timespan_t travel_time() const override
    {
      return meteor_time;
    }

    double action_multiplier() const override
    {
      double m = demonology_spell_t::action_multiplier();

      m *= shards_used;

      if ( p()->buffs.blazing_meteor->check() )
        m *= 1.0 + p()->buffs.blazing_meteor->check_value();

      if ( p()->talents.malefic_impact->ok() )
        m *= 1.0 + p()->talents.malefic_impact->effectN( 1 ).percent();

      return m;
    }

    double composite_crit_chance() const override
    {
      double m = demonology_spell_t::composite_crit_chance();

      if ( p()->talents.malefic_impact->ok() )
        m += p()->talents.malefic_impact->effectN( 2 ).percent();

      return m;
    }

    void execute() override
    {
      demonology_spell_t::execute();

      // 2022-10-17: NOTE in game the buff is consumed on a delay which can cause Demonbolt -> HoG queuing to consume the buff
      // If this bug is not fixed closer to release this must be modeled
      p()->buffs.blazing_meteor->expire();
    }

    void impact( action_state_t* s ) override
    {
      demonology_spell_t::impact( s );

      // Only trigger Wild Imps once for the original target impact.
      // Still keep it in impact instead of execute because of travel delay.
      if ( result_is_hit( s->result ) && s->target == target )
      {

        // Wild Imp spawns appear to have been sped up in Shadowlands. Last tested 2021-04-16.
        // Current behavior: HoG will spawn a meteor on cast finish. Travel time in spell data is 0.7 seconds.
        // However, damage event occurs before spell effect lands, happening 0.4 seconds after cast.
        // Imps then spawn roughly every 0.18 seconds seconds after the damage event.
        for ( int i = 1; i <= shards_used; i++ )
        {
          auto ev = make_event<imp_delay_event_t>( *sim, p(), rng().gauss( 180.0 * i, 25.0 ), 180.0 * i );
          this->p()->wild_imp_spawns.push_back( ev );
        }

        // Umbral Blaze only triggers on primary target
        if ( p()->talents.umbral_blaze->ok() && rng().roll( p()->talents.umbral_blaze->effectN( 1 ).percent() ) )
        {
          blaze->execute_on_target( s->target );
          p()->procs.umbral_blaze->occur();
        }
      }
    }
  };

  hog_impact_t* impact_spell;

  hand_of_guldan_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( "Hand of Gul'dan", p, p->warlock_base.hand_of_guldan ),
      impact_spell( new hog_impact_t( p ) )
  {
    parse_options( options_str );

    add_child( impact_spell );
  }

  timespan_t travel_time() const override
  {
    return 0_ms;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = demonology_spell_t::execute_time();

    if ( p()->buffs.blazing_meteor->check() )
      t *= 1.0 + p()->tier.blazing_meteor->effectN( 2 ).percent();

    return t;
  }

  bool ready() override
  {
    if ( p()->resources.current[ RESOURCE_SOUL_SHARD ] < 1.0 )
    {
      return false;
    }

    return demonology_spell_t::ready();
  }

  void execute() override
  {
    int shards_used = as<int>( cost() );
    impact_spell->shards_used = shards_used;

    demonology_spell_t::execute();

    if ( ( p()->talents.demoniac || !p()->min_version_check( VERSION_10_2_0 ) ) && p()->talents.demonic_knowledge->ok() && rng().roll( p()->talents.demonic_knowledge->effectN( 1 ).percent() ) )
    {
      p()->buffs.demonic_core->trigger();
      p()->procs.demonic_knowledge->occur();
    }

    if ( p()->talents.dread_calling->ok() )
      p()->buffs.dread_calling->trigger( shards_used );

    if ( p()->sets->has_set_bonus( WARLOCK_DEMONOLOGY, T31, B2 ) )
    {
      for ( const auto target : p()->sim->target_non_sleeping_list )
      {
        warlock_td_t* td = p()->get_target_data( target );
        if ( !td )
          continue;

        if ( td->debuffs_doom_brand->check() )
          td->debuffs_doom_brand->extend_duration( p(), -p()->sets->set( WARLOCK_DEMONOLOGY, T31, B2 )->effectN( 1 ).time_value() * shards_used );
      }
    }
  }

  void consume_resource() override
  {
    demonology_spell_t::consume_resource();

    if ( last_resource_cost == 1.0 )
      p()->procs.one_shard_hog->occur();
    if ( last_resource_cost == 2.0 )
      p()->procs.two_shard_hog->occur();
    if ( last_resource_cost == 3.0 )
      p()->procs.three_shard_hog->occur();
  }

  void impact( action_state_t* s ) override
  {
    demonology_spell_t::impact( s );

    impact_spell->execute_on_target( s->target );

    if ( p()->talents.pact_of_the_imp_mother->ok() && rng().roll( p()->talents.pact_of_the_imp_mother->effectN( 1 ).percent() ) )
    {
      // Event seems near-instant, without separate travel time
      make_event( *sim, 0_ms, [this, t = target ] { impact_spell->execute_on_target( t ); } );
      p()->procs.pact_of_the_imp_mother->occur();
    }
  }
};

struct doom_brand_t : public demonology_spell_t
{
  doom_brand_t( warlock_t* p ) : demonology_spell_t( "Doom Brand", p, p->tier.doom_brand_aoe )
  {
    aoe = -1;
    reduced_aoe_targets = 8.0; // 2023-10-17: This is not seen in spell data but was mentioned in a PTR update note
    background = dual = true;
    callbacks = false;
  }

  void execute() override
  {
    demonology_spell_t::execute();

    if ( p()->sets->has_set_bonus( WARLOCK_DEMONOLOGY, T31, B4 ) )
    {
      double increment_max = 0.5;

      int debuff_count = 1; // This brand that is exploding counts as 1, but is already expired
      for ( auto t : target_list() )
      {
        if ( td( t )->debuffs_doom_brand->check() )
          debuff_count++;
      }

      increment_max *= std::pow( debuff_count, -1.0 / 2.0 );

      p()->doom_brand_accumulator += rng().range( 0.0, increment_max );

      if ( p()->doom_brand_accumulator >= 1.0 )
      {
        p()->warlock_pet_list.doomfiends.spawn();
        p()->procs.doomfiend->occur();
        p()->doom_brand_accumulator -= 1.0;
      }
    }
  }

  double composite_da_multiplier( const action_state_t* s ) const override
  {
    double m = demonology_spell_t::composite_da_multiplier( s );

    if ( s->n_targets == 1 )
      m *= 1.0 + p()->sets->set( WARLOCK_DEMONOLOGY, T31, B2 )->effectN( 2 ).percent();

    return m;
  }
};

struct demonbolt_t : public demonology_spell_t
{
  demonbolt_t( warlock_t* p, util::string_view options_str ) : demonology_spell_t( "Demonbolt", p, ( p->min_version_check( VERSION_10_2_0 ) && p->talents.demoniac->ok() ) ? p->talents.demonbolt_spell : p->talents.demonbolt )
  {
    parse_options( options_str );
    energize_type = action_energize::ON_CAST;
    energize_resource = RESOURCE_SOUL_SHARD;
    energize_amount = 2.0;

    procs_shadow_invocation_direct = true;
  }

  timespan_t execute_time() const override
  {
    auto et = demonology_spell_t::execute_time();

    if ( p()->buffs.demonic_core->check() )
    {
      if ( p()->min_version_check( VERSION_10_2_0 ) )
      {
        et *= 1.0 + p()->talents.demonic_core_buff->effectN( 1 ).percent();
      }
      else
      {
        et *= 1.0 + p()->warlock_base.demonic_core_buff->effectN( 1 ).percent();
      }
    }

    return et;
  }

  void execute() override
  {
    demonology_spell_t::execute();

    p()->buffs.demonic_core->up();  // benefit tracking

    if ( p()->buffs.demonic_core->check() )
    {
      // TOCHECK: Once again the actual proc chance is missing from spell data, so we're just guessing with a hardcoded value...
      if ( p()->talents.spiteful_reconstitution->ok() && rng().roll( 0.3 ) )
      {
        p()->warlock_pet_list.wild_imps.spawn( 1u );
        p()->procs.spiteful_reconstitution->occur();
      }

      if ( p()->talents.immutable_hatred->ok() && p()->min_version_check( VERSION_10_2_0 ) )
      {
        auto active_pet = p()->warlock_pet_list.active;

        if ( active_pet->pet_type == PET_FELGUARD )
        {
          debug_cast<pets::demonology::felguard_pet_t*>( active_pet )->hatred_proc->execute_on_target( execute_state->target );
        }
      }

      // TODO: Technically the debuff is still applied on impact, and somehow "knows" that a core was consumed.
      if ( p()->sets->has_set_bonus( WARLOCK_DEMONOLOGY, T31, B2 ) )
        td( target )->debuffs_doom_brand->trigger();
    }
    else
    {
      if ( p()->talents.immutable_hatred->ok() && p()->bugs )
      {
        auto active_pet = p()->warlock_pet_list.active;

        if ( active_pet->pet_type == PET_FELGUARD )
        {
          debug_cast<pets::demonology::felguard_pet_t*>( active_pet )->hatred_proc->execute_on_target( execute_state->target );
        }
      }
    }

    p()->buffs.demonic_core->decrement();

    if ( p()->sets->has_set_bonus( WARLOCK_DEMONOLOGY, T30, B2 ) )
    {
      auto reduction = -p()->sets->set( WARLOCK_DEMONOLOGY, T30, B2 )->effectN( 2 ).time_value();

      p()->cooldowns.grimoire_felguard->adjust( reduction );
    }

    if ( p()->talents.power_siphon->ok() )
      p()->buffs.power_siphon->decrement();

    if ( p()->talents.demonic_calling->ok() )
      p()->buffs.demonic_calling->trigger();

    p()->buffs.stolen_power_final->expire();

    // 2023-01-05 Percent chance for this effect is still not in spell data! Update since beta appears to have adjusted it to 30%
    if ( p()->sets->has_set_bonus( WARLOCK_DEMONOLOGY, T29, B4 ) && rng().roll( 0.30 ) )
    {
      p()->buffs.blazing_meteor->trigger();
      p()->procs.blazing_meteor->occur();
    }
  }

  double action_multiplier() const override
  {
    double m = demonology_spell_t::action_multiplier();

    if ( p()->talents.sacrificed_souls->ok() )
    {
      m *= 1.0 + p()->talents.sacrificed_souls->effectN( 1 ).percent() * p()->active_demon_count();
    }

    if ( p()->talents.power_siphon->ok() )
      m *= 1.0 + p()->buffs.power_siphon->check_value();

    if ( p()->talents.shadows_bite->ok() )
      m *= 1.0 + p()->buffs.shadows_bite->check_value();

    if ( p()->talents.stolen_power->ok() && p()->buffs.stolen_power_final->check() )
      m *= 1.0 + p()->talents.stolen_power_final_buff->effectN( 2 ).percent();

    if ( p()->sets->has_set_bonus( WARLOCK_DEMONOLOGY, T29, B2 ) )
      m *= 1.0 + p()->sets->set( WARLOCK_DEMONOLOGY, T29, B2 )->effectN( 1 ).percent();

    if ( p()->sets->has_set_bonus( WARLOCK_DEMONOLOGY, T30, B2 ) )
      m *= 1.0 + p()->sets->set( WARLOCK_DEMONOLOGY, T30, B2 )->effectN( 1 ).percent();

    return m;
  }
};

struct call_dreadstalkers_t : public demonology_spell_t
{
  call_dreadstalkers_t( warlock_t* p, util::string_view options_str ) : demonology_spell_t( "Call Dreadstalkers", p, p->talents.call_dreadstalkers )
  {
    parse_options( options_str );
    may_crit = false;
  }

  double cost() const override
  {
    double c = demonology_spell_t::cost();

    if ( p()->buffs.demonic_calling->check() )
    {
      c *= 1.0 + p()->talents.demonic_calling_buff->effectN( 1 ).percent();
    }

    return c;
  }

  timespan_t execute_time() const override
  {
    timespan_t t = demonology_spell_t::execute_time();

    if ( p()->buffs.demonic_calling->check() )
    {
      t *= 1.0 + p()->talents.demonic_calling_buff->effectN( 2 ).percent();
    }

    return t;
  }

  void execute() override
  {
    demonology_spell_t::execute();

    unsigned count = as<unsigned>( p()->talents.call_dreadstalkers->effectN( 1 ).base_value() );

    // Set a randomized offset on first melee attacks after travel time. Make sure it's the same value for each dog so they're synced
    timespan_t delay = rng().range( 0_s, 1_s );

    timespan_t dur_adjust = duration_adjustment( delay );

    auto dogs = p()->warlock_pet_list.dreadstalkers.spawn( p()->talents.call_dreadstalkers_2->duration() + dur_adjust, count );


    for ( auto d : dogs )
    {
      if ( d->is_active() )
        d->server_action_delay = delay;
    }

    if ( p()->buffs.demonic_calling->up() )
    {  // benefit tracking

      //Despite having no cost when Demonic Calling is up, this spell will still proc effects based on shard spending (last checked 2022-10-04)
      double base_cost = demonology_spell_t::cost();

      if ( p()->talents.grand_warlocks_design->ok() && !p()->min_version_check( VERSION_10_2_0 ) )
        p()->cooldowns.demonic_tyrant->adjust( -base_cost * p()->talents.grand_warlocks_design->effectN( 2 ).time_value(), false );

      if ( p()->buffs.nether_portal->up() )
      {
        p()->proc_actions.summon_nether_portal_demon->execute();
        p()->procs.portal_summon->occur();

        if ( p()->talents.guldans_ambition->ok() && !p()->min_version_check( VERSION_10_2_0 ) )
          p()->buffs.nether_portal_total->trigger();

        if ( !p()->min_version_check( VERSION_10_2_0 ) && p()->talents.nerzhuls_volition->ok() && rng().roll( p()->talents.nerzhuls_volition->effectN( 1 ).percent() ) )
        {
          p()->proc_actions.summon_nether_portal_demon->execute();
          p()->procs.nerzhuls_volition->occur();

          if ( p()->talents.guldans_ambition->ok() )
            p()->buffs.nether_portal_total->trigger();
        }
      }

      if ( p()->talents.soul_conduit->ok() )
      {
        make_event<sc_event_t>( *p()->sim, p(), as<int>( base_cost ) );
      }

      p()->buffs.demonic_calling->decrement();
    }

    if ( p()->talents.dread_calling->ok() )
    {
      for ( auto d : dogs )
      {
        if ( d->is_active() && !d->buffs.dread_calling->check() )
        {
          d->buffs.dread_calling->trigger( 1, p()->buffs.dread_calling->check_stack_value() );
        }
      }

      p()->buffs.dread_calling->expire();
    }
  }

  timespan_t duration_adjustment( timespan_t delay )
  {
    // Despawn events appear to be offset from the melee attack check in a correlated manner
    // Starting with this function which mimics despawns on the "off-beats" compared to the 1s heartbeat for the melee attack
    // This may require updating if better understanding is found for the behavior, such as a fudge from Blizzard related to player distance
    return ( delay + 500_ms ) % 1_s;
  }
};

struct implosion_t : public demonology_spell_t
{
  struct implosion_aoe_t : public demonology_spell_t
  {
    double energy_remaining = 0.0;
    warlock_pet_t* next_imp;

    implosion_aoe_t( warlock_t* p ) : demonology_spell_t( "Implosion (AoE)", p, p->talents.implosion_aoe )
    {
      aoe = -1;
      background = dual = true;
      callbacks = false;
    }

    double action_multiplier() const override
    {
      double m = demonology_spell_t::action_multiplier();

      if ( p()->min_version_check( VERSION_10_2_0 ) && debug_cast<pets::demonology::wild_imp_pet_t*>( next_imp )->buffs.imp_gang_boss->check() )
        m *= 1.0 + p()->talents.imp_gang_boss->effectN( 2 ).percent();

      if ( p()->talents.spiteful_reconstitution->ok() )
        m *= 1.0 + p()->talents.spiteful_reconstitution->effectN( 1 ).percent();

      return m;
    }

    double composite_target_multiplier( player_t* t ) const override
    {
      double m = demonology_spell_t::composite_target_multiplier( t );

      if ( t == this->target )
      {
        m *= ( energy_remaining / 100.0 ); // 2022-10-03 - Wild Imps deal 84% damage to primary after first cast, which costs 16 energy
      }

      return m;
    }

    void execute() override
    {
      demonology_spell_t::execute();
      next_imp->dismiss();
    }
  };

  implosion_aoe_t* explosion;

  implosion_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( "Implosion", p, p->talents.implosion ), explosion( new implosion_aoe_t( p ) )
  {
    parse_options( options_str );
    add_child( explosion );
  }

  bool ready() override
  {
    bool r = demonology_spell_t::ready();

    if ( r )
    {
      return p()->warlock_pet_list.wild_imps.n_active_pets() > 0;
    }

    return false;
  }

  void execute() override
  {
    // Travel speed is not in spell data, in game test appears to be 65 yds/sec as of 2020-12-04
    timespan_t imp_travel_time = calc_imp_travel_time( 65 );

    int launch_counter = 0;
    for ( auto imp : p()->warlock_pet_list.wild_imps )
    {
      if ( !imp->is_sleeping() )
      {
        implosion_aoe_t* ex = explosion;
        player_t* tar = this->target;
        double dist = p()->get_player_distance( *tar );

        imp->trigger_movement( dist, movement_direction_type::TOWARDS );
        imp->interrupt();
        imp->imploded = true;

        // Imps launched with Implosion appear to be staggered and snapshot when they impact
        // 2020-12-04: Implosion may have been made quicker in Shadowlands, too fast to easily discern with combat log
        // Going to set the interval to 10 ms, which should keep all but the most extreme imp counts from bleeding into the next GCD
        // TODO: There's an awkward possibility of Implosion seeming "ready" after casting it if all the imps have not imploded yet. Find a workaround
        make_event( sim, 50_ms * launch_counter + imp_travel_time, [ ex, tar, imp ] {
          if ( imp && !imp->is_sleeping() )
          {
            ex->energy_remaining = ( imp->resources.current[ RESOURCE_ENERGY ] );
            ex->set_target( tar );
            ex->next_imp = imp;
            ex->execute();
          }
        } );

        launch_counter++;
      }
    }

    demonology_spell_t::execute();
  }

  // A shortened version of action_t::travel_time() for use in calculating the time until the imp is dismissed.
  // Note that imps explode at the bottom of the target, so no height component is accounted for.
  timespan_t calc_imp_travel_time( double speed )
  {
    double t = 0;

    if ( speed > 0 )
    {
      double distance = player->get_player_distance( *target );

      if ( distance > 0 )
        t += distance / speed;
    }

    double v = sim->travel_variance;

    if ( v )
      t = rng().gauss( t, v );

    t = std::max( t, min_travel_time );

    return timespan_t::from_seconds( t );
  }
};

struct summon_demonic_tyrant_t : public demonology_spell_t
{
  summon_demonic_tyrant_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( "Summon Demonic Tyrant", p, p->talents.summon_demonic_tyrant )
  {
    parse_options( options_str );
    harmful = true; // Needs to be enabled specifically for 10.1 class trinket
    may_crit = false;

    if ( p->min_version_check( VERSION_10_2_0 ) && p->talents.grand_warlocks_design->ok() )
      cooldown->duration += p->talents.grand_warlocks_design->effectN( 2 ).time_value();
  }

  void execute() override
  {
    demonology_spell_t::execute();

    // Last tested 2021-07-13
    // Ingame there is a chance for tyrant to get an extra cast off before reaching the required haste breakpoint. In-game testing
    // found the tyrant sometimes stayed longer than the specified duration and can be modelled fairly closely using a normal distribution.
    timespan_t extraTyrantTime = timespan_t::from_millis( rng().gauss_a( 380.0, 220.0, 0.0 ) );
    auto tyrants = p()->warlock_pet_list.demonic_tyrants.spawn( data().duration() + extraTyrantTime );

    if ( !p()->min_version_check( VERSION_10_2_0 ) )
      p()->buffs.demonic_power->trigger();

    if ( p()->talents.soulbound_tyrant->ok() )
      p()->resource_gain( RESOURCE_SOUL_SHARD, p()->talents.soulbound_tyrant->effectN( 1 ).base_value() / 10.0, p()->gains.soulbound_tyrant );

    timespan_t extension_time = p()->talents.demonic_power_buff->effectN( 3 ).time_value();

    int wild_imp_counter = 0;
    int demon_counter = 0;
    int imp_cap = 0;

    if ( p()->min_version_check( VERSION_10_2_0 ) )
      imp_cap = as<int>( p()->talents.summon_demonic_tyrant->effectN( 3 ).base_value() + p()->talents.reign_of_tyranny->effectN( 1 ).base_value() );
    
    for ( auto& pet : p()->pet_list )
    {
      auto lock_pet = dynamic_cast<warlock_pet_t*>( pet );

      if ( lock_pet == nullptr )
        continue;
      if ( lock_pet->is_sleeping() )
        continue;

      if ( lock_pet->pet_type == PET_DEMONIC_TYRANT )
        continue;

      if ( p()->min_version_check( VERSION_10_2_0 ) )
      {
        if ( lock_pet->pet_type == PET_WILD_IMP && wild_imp_counter < imp_cap )
        {
          if ( lock_pet->expiration )
            lock_pet->expiration->reschedule_time = lock_pet->expiration->time + extension_time;

          lock_pet->buffs.demonic_power->trigger();
          wild_imp_counter++;
          demon_counter++;
        }
        else if ( lock_pet->pet_type == PET_DREADSTALKER || lock_pet->pet_type == PET_VILEFIEND || lock_pet->pet_type == PET_SERVICE_FELGUARD || lock_pet->pet_type == PET_FELGUARD )
        {
          if ( lock_pet->expiration )
            lock_pet->expiration->reschedule_time = lock_pet->expiration->time + extension_time;

          lock_pet->buffs.demonic_power->trigger();
          demon_counter++;
        }
      }
      else
      {
        // Pit Lord is not extended by Demonic Tyrant
        if ( lock_pet->pet_type == PET_PIT_LORD )
          continue;

        if ( lock_pet->expiration )
        {
          lock_pet->expiration->reschedule_time = lock_pet->expiration->time + extension_time;
        }
      }
    }

    p()->buffs.tyrant->trigger();

    if ( p()->buffs.dreadstalkers->check() )
    {
      p()->buffs.dreadstalkers->extend_duration( p(), extension_time );
    }
    if ( p()->buffs.grimoire_felguard->check() )
    {
      p()->buffs.grimoire_felguard->extend_duration( p(), extension_time );

      if ( p()->sets->has_set_bonus( WARLOCK_DEMONOLOGY, T30, B4 ) )
        p()->buffs.rite_of_ruvaraad->extend_duration( p(), extension_time );
    }
    if ( p()->buffs.vilefiend->check() )
    {
      p()->buffs.vilefiend->extend_duration( p(), extension_time );
    }

    if ( p()->min_version_check( VERSION_10_2_0 ) && p()->talents.reign_of_tyranny->ok() )
    {
      for ( auto t : tyrants )
      {
        if ( t->is_active() )
          t->buffs.reign_of_tyranny->trigger( demon_counter );
      }
    }

  }
};

// Talents
struct demonic_strength_t : public demonology_spell_t
{
  demonic_strength_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( "Demonic Strength", p, p->talents.demonic_strength )
  {
    parse_options( options_str );
    
    internal_cooldown = p->get_cooldown( "felstorm_icd" );
  }

  // If you have Guillotine or regular Felstorm active, attempting to activate Demonic Strength will fail
  // TOCHECK: New cooldown handling should render these redundant
  bool ready() override
  {
    auto active_pet = p()->warlock_pet_list.active;

    if ( !active_pet )
      return false;

    if ( active_pet->pet_type != PET_FELGUARD )
      return false;

    return demonology_spell_t::ready();
  }

  void execute() override
  {
    auto active_pet = p()->warlock_pet_list.active;

    demonology_spell_t::execute();

    if ( active_pet->pet_type == PET_FELGUARD )
    {
      active_pet->buffs.demonic_strength->trigger();

      debug_cast<pets::demonology::felguard_pet_t*>( active_pet )->queue_ds_felstorm();

      // New in 10.0.5 - Hardcoded scripted shared cooldowns while one of Felstorm, Demonic Strength, or Guillotine is active
      internal_cooldown->start( 5_s * p()->composite_spell_haste() );

    }
  }
};

// Separate struct because you can get the procs without the talent so we'll be safe
struct bilescourge_bombers_proc_t : public demonology_spell_t
{
  bilescourge_bombers_proc_t( warlock_t* p )
    : demonology_spell_t( "Bilescourge Bombers (proc)", p, p->find_spell( 267213 ) )
  {
    aoe = -1;
    background = dual = direct_tick = true;
    callbacks = false;
    radius = p->find_spell( 267211 )->effectN( 1 ).radius();

    base_dd_multiplier *= 1.0 + p->talents.shadow_invocation->effectN( 1 ).percent();
  }
};

struct bilescourge_bombers_t : public demonology_spell_t
{
  struct bilescourge_bombers_tick_t : public demonology_spell_t
  {
    bilescourge_bombers_tick_t( warlock_t* p )
      : demonology_spell_t( "Bilescourge Bombers (tick)", p, p->talents.bilescourge_bombers_aoe )
    {
      aoe = -1;
      background = dual = direct_tick = true;
      callbacks = false;
      radius = p->talents.bilescourge_bombers->effectN( 1 ).radius();

      base_dd_multiplier *= 1.0 + p->talents.shadow_invocation->effectN( 1 ).percent();
    }
  };

  bilescourge_bombers_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( "Bilescourge Bombers", p, p->talents.bilescourge_bombers )
  {
    parse_options( options_str );
    dot_duration = 0_ms;
    may_miss = may_crit = false;
    base_tick_time = 500_ms;
    
    if ( !p->proc_actions.bilescourge_bombers_aoe_tick )
    {
      p->proc_actions.bilescourge_bombers_aoe_tick = new bilescourge_bombers_tick_t( p );
      p->proc_actions.bilescourge_bombers_aoe_tick->stats = stats;
    }
  }

  void execute() override
  {
    demonology_spell_t::execute();

    make_event<ground_aoe_event_t>( *sim, p(),
                                    ground_aoe_params_t()
                                        .target( execute_state->target )
                                        .x( execute_state->target->x_position )
                                        .y( execute_state->target->y_position )
                                        .pulse_time( base_tick_time )
                                        .duration( p()->talents.bilescourge_bombers->duration() )
                                        .start_time( sim->current_time() )
                                        .action( p()->proc_actions.bilescourge_bombers_aoe_tick ) );
  }
};

struct power_siphon_t : public demonology_spell_t
{
  power_siphon_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( "Power Siphon", p, p->talents.power_siphon )
  {
    parse_options( options_str );
    harmful = false;
    ignore_false_positive = true;

    target = player;
  }

  bool ready() override
  {
    if ( is_precombat && p()->talents.inner_demons->ok() )
    {
      return demonology_spell_t::ready(); // Since Inner Demons spawns Wild Imps out of combat, this can be used to pre-stack Demonic Cores before combat
    }

    if ( p()->warlock_pet_list.wild_imps.n_active_pets() < 1 )
      return false;

    return demonology_spell_t::ready();
  }

  void execute() override
  {
    demonology_spell_t::execute();

    if ( is_precombat )
    {
      p()->buffs.power_siphon->trigger( 2, p()->talents.power_siphon_buff->duration() );

      if ( p()->min_version_check( VERSION_10_2_0 ) )
      {
        p()->buffs.demonic_core->trigger( 2, p()->talents.demonic_core_buff->duration() );
      }
      else
      {
        p()->buffs.demonic_core->trigger( 2, p()->warlock_base.demonic_core_buff->duration() );
      }

      return;
    }

    auto imps = p()->warlock_pet_list.wild_imps.active_pets();

    range::sort(
        imps, []( const pets::demonology::wild_imp_pet_t* imp1, const pets::demonology::wild_imp_pet_t* imp2 ) {
          double lv = imp1->resources.current[ RESOURCE_ENERGY ];
          double rv = imp2->resources.current[ RESOURCE_ENERGY ];

          // Starting in 10.2, Power Siphon deprioritizes Wild Imps that are Gang Bosses or empowered by Summon Demonic Tyrant
          // Pad them with a value larger than the energy cap so that they are still sorted against each other at the end of the list
          // TOCHECK: A bug was observed on the PTR where a Gang Boss empowered by Tyrant was not benefitting from this. Not currently implemented here.
          if ( imp1->o()->min_version_check( VERSION_10_2_0 ) )
          {
            lv += ( imp1->buffs.imp_gang_boss->check() || imp1->buffs.demonic_power->check() ) ? 200.0 : 0.0;
            rv += ( imp2->buffs.imp_gang_boss->check() || imp2->buffs.demonic_power->check() ) ? 200.0 : 0.0;
          }

          if ( lv == rv )
          {
            timespan_t lr = imp1->expiration->remains();
            timespan_t rr = imp2->expiration->remains();
            if ( lr == rr )
            {
              return imp1->actor_spawn_index < imp2->actor_spawn_index;
            }

            return lr < rr;
          }

          return lv < rv;
        } );

    unsigned max_imps = as<int>( p()->talents.power_siphon->effectN( 1 ).base_value() );
    if ( imps.size() > max_imps )
      imps.resize( max_imps );

    while ( !imps.empty() )
    {
      p()->buffs.power_siphon->trigger();
      p()->buffs.demonic_core->trigger();
      pets::demonology::wild_imp_pet_t* imp = imps.front();
      imps.erase( imps.begin() );
      imp->power_siphon = true;
      imp->dismiss();
    }
  }
};

struct doom_t : public demonology_spell_t
{
  doom_t( warlock_t* p, util::string_view options_str ) : demonology_spell_t( "doom", p, p->talents.doom )
  {
    parse_options( options_str );

    energize_type = action_energize::PER_TICK;
    energize_resource = RESOURCE_SOUL_SHARD;
    energize_amount = 1;

    hasted_ticks = true;

    procs_shadow_invocation_tick = true;
  }

  timespan_t composite_dot_duration( const action_state_t* s ) const override
  {
    return s->action->tick_time( s ); // Doom is a case where dot duration scales with haste so use the tick time to get the current correct value
  }

  double composite_ta_multiplier( const action_state_t* s ) const override
  {
    double m = demonology_spell_t::composite_ta_multiplier( s );

    if ( p()->talents.kazaaks_final_curse->ok() )
      m *= 1.0 + td( s->target )->debuffs_kazaaks_final_curse->check_value();

    return m;
  }

  void impact( action_state_t* s ) override
  {
    demonology_spell_t::impact( s );

    if ( p()->talents.kazaaks_final_curse->ok() )
    {
      // Count demons
      int count = pet_counter();
      td( s->target )->debuffs_kazaaks_final_curse->trigger( 1, count * p()->talents.kazaaks_final_curse->effectN( 1 ).percent() );
    }
  }

  void last_tick( dot_t* d ) override
  {
    if ( d->time_to_next_full_tick() > 0_ms )
      gain_energize_resource( RESOURCE_SOUL_SHARD, energize_amount, p()->gains.doom ); // 2022-10-06: Doom appears to always give a full shard on its partial tick

    demonology_spell_t::last_tick( d );

    td( d->target )->debuffs_kazaaks_final_curse->expire();
  }

private:
  int pet_counter()
  {
    int count = 0;

    for ( auto& pet : p()->pet_list )
    {
      auto lock_pet = dynamic_cast<warlock_pet_t*>( pet );

      if ( lock_pet == nullptr )
        continue;
      if ( lock_pet->is_sleeping() )
        continue;

      // 2022-11-28: Many pets are not currently counted
      if ( lock_pet->pet_type == PET_DEMONIC_TYRANT || lock_pet->pet_type == PET_PIT_LORD || lock_pet->pet_type == PET_WARLOCK_RANDOM )
        continue;

      if ( lock_pet->pet_type == PET_VILEFIEND )
        continue;

      // Note: Some Wild Imps (possibly due to the Inner Demons NPC ID being different) aren't counting, but we won't model that... yet

      count++;
    }

    return count;
  }
};

struct soul_strike_t : public demonology_spell_t
{
  soul_strike_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( "Soul Strike", p, p->min_version_check( VERSION_10_2_0 ) ? spell_data_t::not_found() : p->talents.soul_strike )
  {
    parse_options( options_str );
    energize_type = action_energize::ON_CAST;
    energize_resource = RESOURCE_SOUL_SHARD;
    energize_amount = 1.0;
  }

  void execute() override
  {
    demonology_spell_t::execute();

    auto active_pet = p()->warlock_pet_list.active;

    if ( active_pet->pet_type == PET_FELGUARD )
    {
      debug_cast<pets::demonology::felguard_pet_t*>( active_pet )->soul_strike->execute_on_target( execute_state->target );
    }
  }

  bool ready() override
  {
    if ( p()->min_version_check( VERSION_10_2_0 ) )
      return false;

    auto active_pet = p()->warlock_pet_list.active;

    if ( !active_pet )
      return false;

    if ( active_pet->pet_type == PET_FELGUARD )
      return demonology_spell_t::ready();

    return false;
  }
};

struct summon_vilefiend_t : public demonology_spell_t
{
  summon_vilefiend_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( "Summon Vilefiend", p, p->talents.summon_vilefiend )
  {
    parse_options( options_str );
    harmful = may_crit = false;

    if ( p->talents.fel_invocation->ok() )
      base_execute_time += p->talents.fel_invocation->effectN( 2 ).time_value();
  }

  void execute() override
  {
    demonology_spell_t::execute();
    p()->buffs.vilefiend->trigger();
    p()->warlock_pet_list.vilefiends.spawn( p()->talents.summon_vilefiend->duration() );
  }
};

struct grimoire_felguard_t : public demonology_spell_t
{
  grimoire_felguard_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( "Grimoire: Felguard", p, p->talents.grimoire_felguard )
  {
    parse_options( options_str );
    harmful = may_crit = false;
  }

  void execute() override
  {
    demonology_spell_t::execute();
    
    p()->warlock_pet_list.grimoire_felguards.spawn( p()->talents.grimoire_felguard->duration() );
    p()->buffs.grimoire_felguard->trigger();
  }
};

struct nether_portal_t : public demonology_spell_t
{
  nether_portal_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( "Nether Portal", p, p->talents.nether_portal )
  {
    parse_options( options_str );
    harmful = may_crit = may_miss = false;
  }

  void execute() override
  {
    p()->buffs.nether_portal->trigger(); // TODO: There is a SimC bug where using Nether Portal at time 0 is not triggering the summon from the shard spent casting itself.
    demonology_spell_t::execute();
  }
};

struct guillotine_t : public demonology_spell_t
{
  guillotine_t( warlock_t* p, util::string_view options_str )
    : demonology_spell_t( "Guillotine", p, p->talents.guillotine )
  {
    parse_options( options_str );
    may_crit = false;

    internal_cooldown = p->get_cooldown( "felstorm_icd" );
  }

  bool ready() override
  {
    auto active_pet = p()->warlock_pet_list.active;

    if ( !active_pet )
      return false;

    if ( active_pet->pet_type != PET_FELGUARD )
      return false;
    
    return demonology_spell_t::ready();
  }

  void execute() override
  {
    auto active_pet = p()->warlock_pet_list.active;

    demonology_spell_t::execute();

    if ( active_pet->pet_type == PET_FELGUARD )
    {
      active_pet->buffs.fiendish_wrath->trigger();

      debug_cast<pets::demonology::felguard_pet_t*>( active_pet )->felguard_guillotine->execute_on_target( execute_state->target );

      // New in 10.0.5 - Hardcoded scripted shared cooldowns while one of Felstorm, Demonic Strength, or Guillotine is active
      internal_cooldown->start( 6_s ); // TOCHECK: Is there a reasonable way to get the duration instead of hardcoding
    }
  }
};

struct summon_random_demon_t : public demonology_spell_t
{
  enum class random_pet_type : int
  {
    shivarra           = 0,
    darkhounds         = 1,
    bilescourges       = 2,
    urzuls             = 3,
    void_terrors       = 4,
    wrathguards        = 5,
    vicious_hellhounds = 6,
    illidari_satyrs    = 7,
    eyes_of_guldan     = 8,
    prince_malchezaar  = 9,
  };

  timespan_t summon_duration;
  bool from_nether_portal;

  summon_random_demon_t( warlock_t* p )
    : demonology_spell_t( "Summon Random Demon", p ), summon_duration( timespan_t::from_seconds( p->talents.inner_demons->effectN( 2 ).base_value() ) )
  {
    background = true;

    from_nether_portal = false;

    // Fallback if Inner Demons data is not present
    if ( p->talents.nether_portal.ok() )
      summon_duration = p->talents.nether_portal->duration();
  }

  summon_random_demon_t( warlock_t* p, bool from_np )
    : summon_random_demon_t( p )
  {
    from_nether_portal = from_np;
  }

  void execute() override
  {
    demonology_spell_t::execute();

    auto random_pet = roll_random_pet();
    summon_random_pet( random_pet, from_nether_portal );
    p()->procs.summon_random_demon->occur();
  }

private:
  /**
   * Helper function to actually summon 'num_summons' random pets from a given pet list.
   */
  template <typename pet_list_type>
  void summon_random_pet_helper( pet_list_type& pet_list, int num_summons = 1, bool from_nether_portal = false )
  {
    auto list = pet_list.spawn( summon_duration, as<unsigned>( num_summons ) );

    if ( p()->min_version_check( VERSION_10_2_0 ) && from_nether_portal && p()->talents.nerzhuls_volition->ok() )
    {
      for ( auto& pet : list )
      {
        debug_cast<warlock_pet_t*>( pet )->buffs.nerzhuls_volition->trigger();
      }
    }
  }

  /**
   * Summon the random pet(s) specified.
   */
  void summon_random_pet( random_pet_type random_pet, bool from_nether_portal = false )
  {
    switch ( random_pet )
    {
      case random_pet_type::prince_malchezaar:
        summon_random_pet_helper( p()->warlock_pet_list.prince_malchezaar );
        break;
      case random_pet_type::eyes_of_guldan:
      {
        // TODO - eyes appear to spawn in #s randomly between 1 and 4 in SL beta 09-24-2020.
        // eyes summon in groups of 4. Confirmed by pip 2018-06-23.
        summon_random_pet_helper( p()->warlock_pet_list.eyes_of_guldan, 4, from_nether_portal );
        break;
        case random_pet_type::shivarra:
          summon_random_pet_helper( p()->warlock_pet_list.shivarra, 1, from_nether_portal );
          break;
        case random_pet_type::darkhounds:
          summon_random_pet_helper( p()->warlock_pet_list.darkhounds, 1, from_nether_portal );
          break;
        case random_pet_type::bilescourges:
          summon_random_pet_helper( p()->warlock_pet_list.bilescourges, 1, from_nether_portal );
          break;
        case random_pet_type::urzuls:
          summon_random_pet_helper( p()->warlock_pet_list.urzuls, 1, from_nether_portal );
          break;
        case random_pet_type::void_terrors:
          summon_random_pet_helper( p()->warlock_pet_list.void_terrors, 1, from_nether_portal );
          break;
        case random_pet_type::wrathguards:
          summon_random_pet_helper( p()->warlock_pet_list.wrathguards, 1, from_nether_portal );
          break;
        case random_pet_type::vicious_hellhounds:
          summon_random_pet_helper( p()->warlock_pet_list.vicious_hellhounds, 1, from_nether_portal );
          break;
        case random_pet_type::illidari_satyrs:
          summon_random_pet_helper( p()->warlock_pet_list.illidari_satyrs, 1, from_nether_portal );
          break;
        default:
          assert( false && "trying to summon invalid random demon." );
          break;
      }
    }
  }

  /**
   * Roll the dice and determine which random pet(s) to summon.
   */
  random_pet_type roll_random_pet()
  {
    int demon_int = rng().range( 9 ); //Malchezaar is disabled in instances. TODO: Add option to reenable?
    int rare_check;
    if ( demon_int > 7 )
    {
      rare_check = rng().range( 10 );
      if ( rare_check > 0 )
      {
        demon_int = rng().range( 8 );
      }
    }
    assert( demon_int >= 0 && demon_int <= 9 );
    return static_cast<random_pet_type>( demon_int );
  }
};

}  // namespace actions_demonology

namespace buffs
{
}  // namespace buffs

pet_t* warlock_t::create_demo_pet( util::string_view pet_name, util::string_view /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );
  if ( p )
    return p;
  using namespace pets;

  if ( pet_name == "felguard" )
    return new pets::demonology::felguard_pet_t( this, pet_name );

  return nullptr;
}
// add actions
action_t* warlock_t::create_action_demonology( util::string_view action_name, util::string_view options_str )
{
  using namespace actions_demonology;

  if ( action_name == "demonbolt" )
    return new demonbolt_t( this, options_str );
  if ( action_name == "hand_of_guldan" )
    return new hand_of_guldan_t( this, options_str );
  if ( action_name == "implosion" )
    return new implosion_t( this, options_str );
  if ( action_name == "demonic_strength" )
    return new demonic_strength_t( this, options_str );
  if ( action_name == "bilescourge_bombers" )
    return new bilescourge_bombers_t( this, options_str );
  if ( action_name == "doom" )
    return new doom_t( this, options_str );
  if ( action_name == "power_siphon" )
    return new power_siphon_t( this, options_str );
  if ( action_name == "soul_strike" )
    return new soul_strike_t( this, options_str );
  if ( action_name == "nether_portal" )
    return new nether_portal_t( this, options_str );
  if ( action_name == "call_dreadstalkers" )
    return new call_dreadstalkers_t( this, options_str );
  if ( action_name == "summon_felguard" )
    return new summon_main_pet_t( "felguard", this );
  if ( action_name == "summon_demonic_tyrant" )
    return new summon_demonic_tyrant_t( this, options_str );
  if ( action_name == "summon_vilefiend" )
    return new summon_vilefiend_t( this, options_str );
  if ( action_name == "grimoire_felguard" )
    return new grimoire_felguard_t( this, options_str );
  if ( action_name == "guillotine" )
    return new guillotine_t( this, options_str );

  return nullptr;
}

void warlock_t::create_buffs_demonology()
{
  buffs.demonic_core = make_buff( this, "demonic_core", min_version_check( VERSION_10_2_0 ) ? talents.demonic_core_buff : warlock_base.demonic_core_buff );

  buffs.power_siphon = make_buff( this, "power_siphon", talents.power_siphon_buff )
                           ->set_default_value_from_effect( 1 );

  buffs.demonic_power = make_buff( this, "demonic_power", talents.demonic_power_buff )
                            ->set_default_value_from_effect( 2 );

  buffs.demonic_calling = make_buff( this, "demonic_calling", talents.demonic_calling_buff )
                              ->set_chance( talents.demonic_calling->effectN( 3 ).percent() );

  buffs.inner_demons = make_buff( this, "inner_demons", talents.inner_demons )
                           ->set_period( talents.inner_demons->effectN( 1 ).period() )
                           ->set_tick_time_behavior( buff_tick_time_behavior::UNHASTED )
                           ->set_tick_callback( [ this ]( buff_t*, int, timespan_t ) {
                             warlock_pet_list.wild_imps.spawn();
                             if ( !min_version_check( VERSION_10_2_0 ) && rng().roll( talents.inner_demons->effectN( 1 ).percent() ) )
                             {
                               proc_actions.summon_random_demon->execute();
                             }
                           } );

  buffs.nether_portal = make_buff( this, "nether_portal", talents.nether_portal_buff )
                            ->set_stack_change_callback( [ this ]( buff_t*, int, int cur ) {
                              if ( !sim->event_mgr.canceled && cur == 0 && talents.guldans_ambition.ok() )
                              {
                                warlock_pet_list.pit_lords.spawn( talents.guldans_ambition_summon->duration(), 1u );
                              };
                            } );

  buffs.dread_calling = make_buff<buff_t>( this, "dread_calling", talents.dread_calling_buff )
                            ->set_default_value( talents.dread_calling->effectN( 1 ).percent() );

  buffs.shadows_bite = make_buff( this, "shadows_bite", talents.shadows_bite_buff )
                           ->set_default_value( talents.shadows_bite->effectN( 1 ).percent() );

  buffs.stolen_power_building = make_buff( this, "stolen_power_building", talents.stolen_power_stacking_buff )
                                    ->set_stack_change_callback( [ this ]( buff_t* b, int, int cur )
                                    {
                                      if ( cur == b->max_stack() )
                                      {
                                        make_event( sim, 0_ms, [ this, b ] { 
                                          buffs.stolen_power_final->trigger();
                                          b->expire();
                                        } );
                                      };
                                    } );

  buffs.stolen_power_final = make_buff( this, "stolen_power_final", talents.stolen_power_final_buff );

  // TODO: This can be removed once 10.2 goes live
  buffs.nether_portal_total = make_buff( this, "nether_portal_total" )
                                  ->set_max_stack( !min_version_check( VERSION_10_2_0 ) ? talents.soul_glutton->max_stacks() : 1 )
                                  ->set_refresh_behavior( buff_refresh_behavior::NONE );

  buffs.demonic_servitude = make_buff( this, "demonic_servitude", talents.demonic_servitude )
                                ->set_default_value( talents.reign_of_tyranny->effectN( 2 ).percent() );  // TODO: temp fix for 10.2 PTR data

  buffs.blazing_meteor = make_buff( this, "blazing_meteor", tier.blazing_meteor )
                             ->set_default_value_from_effect( 1 );

  buffs.rite_of_ruvaraad = make_buff( this, "rite_of_ruvaraad", tier.rite_of_ruvaraad )
                               ->set_default_value( tier.rite_of_ruvaraad->effectN( 1 ).percent() );

  // Pet tracking buffs
  buffs.wild_imps = make_buff( this, "wild_imps" )->set_max_stack( 40 );

  buffs.dreadstalkers = make_buff( this, "dreadstalkers" )->set_max_stack( 8 )
                        ->set_duration( talents.call_dreadstalkers_2->duration() );

  buffs.vilefiend = make_buff( this, "vilefiend" )->set_max_stack( 1 )
                    ->set_duration( talents.summon_vilefiend->duration() );

  buffs.tyrant = make_buff( this, "tyrant" )->set_max_stack( 1 )
                 ->set_duration( find_spell( 265187 )->duration() );

  buffs.grimoire_felguard = make_buff( this, "grimoire_felguard" )->set_max_stack( 1 )
                            ->set_duration( talents.grimoire_felguard->duration() );

  buffs.prince_malchezaar = make_buff( this, "prince_malchezaar" )->set_max_stack( 1 );

  buffs.eyes_of_guldan = make_buff( this, "eyes_of_guldan" )->set_max_stack( 4 );
}

void warlock_t::init_spells_demonology()
{
  // Talents
  talents.call_dreadstalkers = find_talent_spell( talent_tree::SPECIALIZATION, "Call Dreadstalkers" ); // Should be ID 104316
  talents.call_dreadstalkers_2 = find_spell( 193332 ); // Duration data

  talents.demonbolt = find_talent_spell( talent_tree::SPECIALIZATION, "Demonbolt" ); // Should be ID 264178

  talents.demoniac = find_talent_spell( talent_tree::SPECIALIZATION, "Demoniac" ); // Should be ID 426115
  talents.demonbolt_spell = find_spell( 264178 );
  talents.demonic_core_spell = find_spell( 267102 );
  talents.demonic_core_buff = find_spell( 264173 );

  talents.dreadlash = find_talent_spell( talent_tree::SPECIALIZATION, "Dreadlash" ); // Should be ID 264078

  talents.annihilan_training = find_talent_spell( talent_tree::SPECIALIZATION, "Annihilan Training" ); // Should be ID 386174
  talents.annihilan_training_buff = find_spell( 386176 );

  talents.demonic_knowledge = find_talent_spell( talent_tree::SPECIALIZATION, "Demonic Knowledge" ); // Should be ID 386185

  talents.summon_vilefiend = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Vilefiend" ); // Should be ID 264119

  talents.soul_strike = find_talent_spell( talent_tree::SPECIALIZATION, "Soul Strike" ); // Should be ID 264057. NOTE: Updated to 428344 in 10.2

  talents.bilescourge_bombers = find_talent_spell( talent_tree::SPECIALIZATION, "Bilescourge Bombers" ); // Should be ID 267211
  talents.bilescourge_bombers_aoe = find_spell( 267213 );

  talents.demonic_strength = find_talent_spell( talent_tree::SPECIALIZATION, "Demonic Strength" ); // Should be ID 267171
  
  talents.the_houndmasters_stratagem = find_talent_spell( talent_tree::SPECIALIZATION, "The Houndmaster's Stratagem" ); // Should be ID 267170
  talents.the_houndmasters_stratagem_debuff = find_spell( 270569 );

  talents.implosion = find_talent_spell( talent_tree::SPECIALIZATION, "Implosion" ); // Should be ID 196277
  talents.implosion_aoe = find_spell( 196278 );

  talents.shadows_bite = find_talent_spell( talent_tree::SPECIALIZATION, "Shadow's Bite" ); // Should be ID 387322
  talents.shadows_bite_buff = find_spell( 272945 );

  talents.fel_invocation = find_talent_spell( talent_tree::SPECIALIZATION, "Fel Invocation" ); // Should be ID 428351

  talents.carnivorous_stalkers = find_talent_spell( talent_tree::SPECIALIZATION, "Carnivorous Stalkers" ); // Should be ID 386194

  talents.shadow_invocation = find_talent_spell( talent_tree::SPECIALIZATION, "Shadow Invocation" ); // Should be ID 422054

  talents.fel_and_steel = find_talent_spell( talent_tree::SPECIALIZATION, "Fel and Steel" ); // Should be ID 386200

  talents.heavy_handed = find_talent_spell( talent_tree::SPECIALIZATION, "Heavy Handed" ); // Should be ID 416183

  talents.power_siphon = find_talent_spell( talent_tree::SPECIALIZATION, "Power Siphon" ); // Should be ID 264130
  talents.power_siphon_buff = find_spell( 334581 );

  talents.malefic_impact = find_talent_spell( talent_tree::SPECIALIZATION, "Malefic Impact" ); // Should be ID 416341

  talents.imperator = find_talent_spell( talent_tree::SPECIALIZATION, "Imp-erator" ); // Should be ID 416230

  talents.grimoire_felguard = find_talent_spell( talent_tree::SPECIALIZATION, "Grimoire: Felguard" ); // Should be ID 111898

  talents.bloodbound_imps = find_talent_spell( talent_tree::SPECIALIZATION, "Bloodbound Imps" ); // Should be ID 387349

  talents.spiteful_reconstitution = find_talent_spell( talent_tree::SPECIALIZATION, "Spiteful Reconstitution" ); // Should be ID 428394
  
  talents.inner_demons = find_talent_spell( talent_tree::SPECIALIZATION, "Inner Demons" ); // Should be ID 267216

  talents.doom = find_talent_spell( talent_tree::SPECIALIZATION, "Doom" ); // Should be ID 603
  
  talents.demonic_calling = find_talent_spell( talent_tree::SPECIALIZATION, "Demonic Calling" ); // Should be ID 205145
  talents.demonic_calling_buff = find_spell( 205146 );

  talents.fel_sunder = find_talent_spell( talent_tree::SPECIALIZATION, "Fel Sunder" ); // Should be ID 387399
  talents.fel_sunder_debuff = find_spell( 387402 );

  talents.umbral_blaze = find_talent_spell( talent_tree::SPECIALIZATION, "Umbral Blaze" ); // Should be ID 405798
  talents.umbral_blaze_dot = find_spell( 405802 );

  talents.imp_gang_boss = find_talent_spell( talent_tree::SPECIALIZATION, "Imp Gang Boss" ); // Should be ID 387445

  talents.kazaaks_final_curse = find_talent_spell( talent_tree::SPECIALIZATION, "Kazaak's Final Curse" ); // Should be ID 387483

  talents.dread_calling = find_talent_spell( talent_tree::SPECIALIZATION, "Dread Calling" ); // Should be ID 387391
  talents.dread_calling_buff = find_spell( 387393 );

  talents.cavitation = find_talent_spell( talent_tree::SPECIALIZATION, "Cavitation" ); // Should be ID 416154

  talents.nether_portal = find_talent_spell( talent_tree::SPECIALIZATION, "Nether Portal" ); // Should be ID 267217
  talents.nether_portal_buff = find_spell( 267218 );

  talents.summon_demonic_tyrant = find_talent_spell( talent_tree::SPECIALIZATION, "Summon Demonic Tyrant" ); // Should be ID 265187
  talents.demonic_power_buff = find_spell( 265273 );

  talents.antoran_armaments = find_talent_spell( talent_tree::SPECIALIZATION, "Antoran Armaments" ); // Should be ID 387494

  talents.nerzhuls_volition = find_talent_spell( talent_tree::SPECIALIZATION, "Ner'zhul's Volition" ); // Should be ID 387526
  talents.nerzhuls_volition_buff = find_spell( 421970 );

  talents.stolen_power = find_talent_spell( talent_tree::SPECIALIZATION, "Stolen Power" ); // Should be ID 387602
  talents.stolen_power_stacking_buff = find_spell( 387603 );
  talents.stolen_power_final_buff = find_spell( 387604 );

  talents.sacrificed_souls = find_talent_spell( talent_tree::SPECIALIZATION, "Sacrificed Souls" ); // Should be ID 267214

  talents.soulbound_tyrant = find_talent_spell( talent_tree::SPECIALIZATION, "Soulbound Tyrant" ); // Should be ID 334585

  talents.pact_of_the_imp_mother = find_talent_spell( talent_tree::SPECIALIZATION, "Pact of the Imp Mother" ); // Should be ID 387541

  talents.the_expendables = find_talent_spell( talent_tree::SPECIALIZATION, "The Expendables" ); // Should be ID 387600

  talents.infernal_command = find_talent_spell( talent_tree::SPECIALIZATION, "Infernal Command" ); // Should be ID 387549

  talents.guldans_ambition = find_talent_spell( talent_tree::SPECIALIZATION, "Gul'dan's Ambition" ); // Should be ID 387578
  talents.guldans_ambition_summon = find_spell( 387590 );
  talents.soul_glutton = find_spell( 387595 );

  talents.reign_of_tyranny = find_talent_spell( talent_tree::SPECIALIZATION, "Reign of Tyranny" ); // Should be ID 390173
  talents.demonic_servitude = find_spell( 390193 );

  talents.immutable_hatred = find_talent_spell( talent_tree::SPECIALIZATION, "Immutable Hatred" ); // Should be ID 405670

  talents.guillotine = find_talent_spell( talent_tree::SPECIALIZATION, "Guillotine" ); // Should be ID 386833

  // Additional Tier Set spell data

  // T29 (Vault of the Incarnates)
  tier.blazing_meteor = find_spell( 394776 );

  // T30 (Aberrus, the Shadowed Crucible)
  tier.rite_of_ruvaraad = find_spell( 409725 );

  // T31 (Amirdrassil, the Dream's Hope)
  tier.doom_brand = find_spell( 423585 );
  tier.doom_brand_debuff = find_spell( 423583 );
  tier.doom_brand_aoe = find_spell( 423584 );
  tier.doom_bolt_volley = find_spell( 423734 );

  proc_actions.summon_random_demon = new actions_demonology::summon_random_demon_t( this );
  proc_actions.summon_nether_portal_demon = new actions_demonology::summon_random_demon_t( this, true );
  proc_actions.bilescourge_bombers_proc = new actions_demonology::bilescourge_bombers_proc_t( this );
  proc_actions.doom_brand_explosion = new actions_demonology::doom_brand_t( this );

  // Initialize some default values for pet spawners
  warlock_pet_list.wild_imps.set_default_duration( warlock_base.wild_imp->duration() );

  warlock_pet_list.dreadstalkers.set_default_duration( talents.call_dreadstalkers_2->duration() );
}

void warlock_t::init_gains_demonology()
{
  gains.soulbound_tyrant = get_gain( "soulbound_tyrant" );
  gains.doom = get_gain( "doom" );
  gains.soul_strike = get_gain( "soul_strike" );
}

void warlock_t::init_rng_demonology()
{
}

void warlock_t::init_procs_demonology()
{
  procs.summon_random_demon = get_proc( "summon_random_demon" );
  procs.demonic_knowledge = get_proc( "demonic_knowledge" );
  procs.shadow_invocation = get_proc( "shadow_invocation" );
  procs.imp_gang_boss = get_proc( "imp_gang_boss" );
  procs.spiteful_reconstitution = get_proc( "spiteful_reconstitution" );
  procs.umbral_blaze = get_proc( "umbral_blaze" );
  procs.nerzhuls_volition = get_proc( "nerzhuls_volition" );
  procs.pact_of_the_imp_mother = get_proc( "pact_of_the_imp_mother" );
  procs.blazing_meteor = get_proc( "blazing_meteor" );
  procs.doomfiend = get_proc( "doomfiend" );
}

}  // namespace warlock
