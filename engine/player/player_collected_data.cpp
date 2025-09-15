// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "player_collected_data.hpp"

#include "action/action.hpp"
#include "buff/buff.hpp"
#include "pet.hpp"
#include "player.hpp"
#include "sim/cooldown.hpp"
#include "sim/sim.hpp"

void player_collected_data_t::health_changes_timeline_t::set_bin_size( double bin )
{
  timeline.set_bin_size( bin );
  timeline_normalized.set_bin_size( bin );
  merged_timeline.set_bin_size( bin );
}

double player_collected_data_t::health_changes_timeline_t::get_bin_size() const
{
  if ( timeline.bin_size() != timeline_normalized.bin_size() || timeline.bin_size() != merged_timeline.bin_size() )
  {
    assert( false );
    return 0.0;
  }
  else
    return timeline.bin_size();
}

player_collected_data_t::action_sequence_data_t::action_sequence_data_t( const action_t* a, const player_t* t,
                                                                         timespan_t ts, timespan_t wait,
                                                                         const player_t* p )
  : action( a ), target( t ), time( ts ), wait_time( wait ), queue_failed( false )
{
  for ( buff_t* b : p->buff_list )
  {
    if ( b->check() && !b->quiet && !b->constant )
    {
      buff_list.emplace_back( b, b->check(), b->remains() );
    }
  }

  // Adding cooldown and debuffs snapshots if asking for json full states
  if ( p->sim->json_full_states )
  {
    for ( cooldown_t* c : p->cooldown_list )
    {
      if ( c->down() )
      {
        cooldown_list.emplace_back( c, c->charges, c->remains() );
      }
    }
    for ( player_t* current_target : p->sim->target_list )
    {
      decltype( target_list )::value_type::second_type debuff_list;
      for ( buff_t* d : current_target->buff_list )
      {
        if ( d->check() && !d->quiet && !d->constant )
        {
          debuff_list.emplace_back( d, d->check(), d->remains() );
        }
      }
      target_list.emplace_back( current_target, std::move( debuff_list ) );
    }
  }

  range::fill( resource_snapshot, -1 );
  range::fill( resource_max_snapshot, -1 );

  for ( resource_e i = RESOURCE_HEALTH; i < RESOURCE_MAX; ++i )
  {
    if ( p->resources.max[ i ] > 0.0 )
    {
      resource_snapshot[ i ] = p->resources.current[ i ];
      resource_max_snapshot[ i ] = p->resources.max[ i ];
    }
  }
}

player_collected_data_t::player_collected_data_t( const player_t* player )
  : fight_length( player->name_str + " Fight Length", generic_container_type( player, 2 ) ),
    waiting_time( player->name_str + " Waiting Time", generic_container_type( player, 2 ) ),
    pooling_time( player->name_str + " Pooling Time", generic_container_type( player, 4 ) ),
    executed_foreground_actions( player->name_str + " Executed Foreground Actions",
                                 generic_container_type( player, 4 ) ),
    dmg( player->name_str + " Damage", generic_container_type( player, 2 ) ),
    compound_dmg( player->name_str + " Total Damage", generic_container_type( player, 2 ) ),
    prioritydps( player->name_str + " Priority Target Damage Per Second", generic_container_type( player, 1 ) ),
    dps( player->name_str + " Damage Per Second", generic_container_type( player, 1 ) ),
    dpse( player->name_str + " Damage Per Second (Effective)", generic_container_type( player, 2 ) ),
    dtps( player->name_str + " Damage Taken Per Second", tank_container_type( player, 2 ) ),
    dmg_taken( player->name_str + " Damage Taken", tank_container_type( player, 2 ) ),
    timeline_dmg(),
    heal( player->name_str + " Heal", generic_container_type( player, 2 ) ),
    compound_heal( player->name_str + " Total Heal", generic_container_type( player, 2 ) ),
    hps( player->name_str + " Healing Per Second", generic_container_type( player, 1 ) ),
    hpse( player->name_str + " Healing Per Second (Effective)", generic_container_type( player, 2 ) ),
    htps( player->name_str + " Healing Taken Per Second", tank_container_type( player, 2 ) ),
    heal_taken( player->name_str + " Healing Taken", tank_container_type( player, 2 ) ),
    absorb( player->name_str + " Absorb", generic_container_type( player, 2 ) ),
    compound_absorb( player->name_str + " Total Absorb", generic_container_type( player, 2 ) ),
    aps( player->name_str + " Absorb Per Second", generic_container_type( player, 1 ) ),
    atps( player->name_str + " Absorb Taken Per Second", tank_container_type( player, 2 ) ),
    absorb_taken( player->name_str + " Absorb Taken", tank_container_type( player, 2 ) ),
    deaths( player->name_str + " Deaths", tank_container_type( player, 2 ) ),
    target_metric( player->name_str + " Target Metric", generic_container_type( player, 1 ) ),
    resource_timelines(),
    health_pct(),
    combat_start_resource(
      ( !player->is_enemy() && ( !player->is_pet() || player->sim->report_pets_separately ) ) ? RESOURCE_MAX : 0 ),
    combat_end_resource(
      ( !player->is_enemy() && ( !player->is_pet() || player->sim->report_pets_separately ) ) ? RESOURCE_MAX : 0 ),
    stat_timelines(),
    health_changes(),
    total_iterations( 0 ),
    buffed_stats_snapshot()
{
  if ( !player->is_enemy() && ( !player->is_pet() || player->sim->report_pets_separately ) )
  {
    resource_lost.resize( RESOURCE_MAX );
    resource_gained.resize( RESOURCE_MAX );
    resource_overflowed.resize( RESOURCE_MAX );
  }

  // Enemies only have health
  if ( player->is_enemy() )
  {
    resource_lost.resize( RESOURCE_HEALTH + 1 );
    resource_gained.resize( RESOURCE_HEALTH + 1 );
    resource_overflowed.resize( RESOURCE_HEALTH + 1 );
  }
}

void player_collected_data_t::reserve_memory( const player_t& p )
{
  unsigned size = std::min( as<unsigned>( p.sim->iterations ), 2048U );
  fight_length.reserve( size );
  // DMG
  dmg.reserve( size );
  compound_dmg.reserve( size );
  dps.reserve( size );
  prioritydps.reserve( size );
  dpse.reserve( size );
  dtps.reserve( size );
  // HEAL
  heal.reserve( size );
  compound_heal.reserve( size );
  hps.reserve( size );
  hpse.reserve( size );
  htps.reserve( size );
  heal_taken.reserve( size );
  deaths.reserve( size );

  if ( !p.is_pet() && p.primary_role() == ROLE_TANK && p.type != PLAYER_SIMPLIFIED )
    p.sim->num_tanks++;
}

void player_collected_data_t::merge( const player_t& other_player )
{
  const auto& other = other_player.collected_data;
  // No data got collected for this player in this thread, so skip merging player collected data
  // entirely
  if ( other.fight_length.count() == 0 )
  {
    return;
  }

  total_iterations += other.total_iterations;

  fight_length.merge( other.fight_length );
  waiting_time.merge( other.waiting_time );
  executed_foreground_actions.merge( other.executed_foreground_actions );
  // DMG
  dmg.merge( other.dmg );
  compound_dmg.merge( other.compound_dmg );
  dps.merge( other.dps );
  prioritydps.merge( other.prioritydps );
  dtps.merge( other.dtps );
  dpse.merge( other.dpse );
  dmg_taken.merge( other.dmg_taken );
  timeline_dmg.merge( other.timeline_dmg );
  // HEAL
  heal.merge( other.heal );
  compound_heal.merge( other.compound_heal );
  hps.merge( other.hps );
  htps.merge( other.htps );
  hpse.merge( other.hpse );
  heal_taken.merge( other.heal_taken );
  // Tank
  deaths.merge( other.deaths );
  timeline_dmg_taken.merge( other.timeline_dmg_taken );
  timeline_healing_taken.merge( other.timeline_healing_taken );

  for ( size_t i = 0, end = resource_lost.size(); i < end; ++i )
  {
    resource_lost[ i ].merge( other.resource_lost[ i ] );
    resource_gained[ i ].merge( other.resource_gained[ i ] );
    resource_overflowed[ i ].merge( other.resource_overflowed[ i ] );
  }

  if ( resource_timelines.size() == other.resource_timelines.size() )
  {
    for ( size_t i = 0; i < resource_timelines.size(); ++i )
    {
      assert( resource_timelines[ i ].type == other.resource_timelines[ i ].type );
      assert( resource_timelines[ i ].type != RESOURCE_NONE );
      resource_timelines[ i ].timeline.merge( other.resource_timelines[ i ].timeline );
    }
  }

  health_pct.merge( other.health_pct );

  for ( size_t i = 0, end = combat_start_resource.size(); i < end; ++i )
  {
    combat_start_resource[ i ].merge( other.combat_start_resource[ i ] );
    combat_end_resource[ i ].merge( other.combat_end_resource[ i ] );
  }

  assert( stat_timelines.size() == other.stat_timelines.size() );
  for ( size_t i = 0; i < stat_timelines.size(); ++i )
  {
    assert( stat_timelines[ i ].type == other.stat_timelines[ i ].type );
    stat_timelines[ i ].timeline.merge( other.stat_timelines[ i ].timeline );
  }

  health_changes.merged_timeline.merge( other.health_changes.merged_timeline );
}

void player_collected_data_t::analyze( const player_t& p )
{
  fight_length.analyze();
  // DMG
  dmg.analyze();
  compound_dmg.analyze();
  dps.analyze();
  prioritydps.analyze();
  dpse.analyze();
  dmg_taken.analyze();
  dtps.analyze();
  // Heal
  heal.analyze();
  compound_heal.analyze();
  hps.analyze();
  hpse.analyze();
  heal_taken.analyze();
  htps.analyze();
  // Absorb
  absorb.analyze();
  compound_absorb.analyze();
  aps.analyze();
  absorb_taken.analyze();
  atps.analyze();
  // Tank
  deaths.analyze();

  if ( !p.sim->single_actor_batch )
  {
    timeline_dmg_taken.adjust( *p.sim );
    timeline_healing_taken.adjust( *p.sim );

    health_pct.adjust( *p.sim );
    range::for_each( resource_timelines, [ &p ]( resource_timeline_t& tl ) {
      tl.timeline.adjust( *p.sim );
    } );
    range::for_each( stat_timelines, [ &p ]( stat_timeline_t& tl ) {
      tl.timeline.adjust( *p.sim );
    } );

    // health changes need their own divisor
    health_changes.merged_timeline.adjust( *p.sim );
  }
  // Single actor batch mode has to analyze the timelines in relation to their own fight lengths,
  // instead of the simulation-wide fight length.
  else
  {
    timeline_dmg_taken.adjust( fight_length );
    timeline_healing_taken.adjust( fight_length );

    health_pct.adjust( fight_length );
    range::for_each( resource_timelines, [ this ]( resource_timeline_t& tl ) {
      tl.timeline.adjust( fight_length );
    } );
    range::for_each( stat_timelines, [ this ]( stat_timeline_t& tl ) {
      tl.timeline.adjust( fight_length );
    } );

    // health changes need their own divisor
    health_changes.merged_timeline.adjust( fight_length );
  }
}

double player_collected_data_t::calculate_max_spike_damage( const health_changes_timeline_t& tl, int window )
{
  double max_spike = 0;

  // declare sliding average timeline
  sc_timeline_t sliding_average_tl;

  // create sliding average timelines from data
  tl.timeline_normalized.build_sliding_average_timeline( sliding_average_tl, window );

  // pull the data out of the normalized sliding average timeline
  std::vector<double> weighted_value = sliding_average_tl.data();

  // extract the max spike size from the sliding average timeline
  max_spike = *std::max_element( weighted_value.begin(), weighted_value.end() );  // todo: remove weighted_value here
  max_spike *= window;

  return max_spike;
}

void player_collected_data_t::collect_data( const player_t& p )
{
  double f_length = p.iteration_fight_length.total_seconds();
  // Use a composite uptime for the amount per second calculations to accurately take into account
  // dynamic pets (for example wild imps) spawned through the pet spawner system. Only used for
  // output metrics.
  double uptime = p.composite_active_time().total_seconds();
  double sim_length = p.sim->current_time().total_seconds();
  double w_time = p.iteration_waiting_time.total_seconds();
  double p_time = p.iteration_pooling_time.total_seconds();
  assert( p.iteration_fight_length <= p.sim->current_time() );

  fight_length.add( f_length );
  waiting_time.add( w_time );
  pooling_time.add( p_time );

  executed_foreground_actions.add( p.iteration_executed_foreground_actions );

  // Player only dmg/heal
  dmg.add( p.iteration_dmg );
  heal.add( p.iteration_heal );
  absorb.add( p.iteration_absorb );

  // player + pet dmg
  double total_iteration_dmg = range::accumulate( p.pet_list, p.iteration_dmg, &player_t::iteration_dmg );

  double total_priority_iteration_dmg =
    range::accumulate( p.pet_list, p.priority_iteration_dmg, &player_t::priority_iteration_dmg );

  // player + pet heal
  double total_iteration_heal = range::accumulate( p.pet_list, p.iteration_heal, &player_t::iteration_heal );

  double total_iteration_absorb = range::accumulate( p.pet_list, p.iteration_absorb, &player_t::iteration_absorb );

  compound_dmg.add( total_iteration_dmg );
  prioritydps.add( uptime ? total_priority_iteration_dmg / uptime : 0 );
  dps.add( uptime ? total_iteration_dmg / uptime : 0 );
  dpse.add( sim_length ? total_iteration_dmg / sim_length : 0 );
  double dps_metric = uptime ? ( total_iteration_dmg / uptime ) : 0;

  compound_heal.add( total_iteration_heal );
  hps.add( uptime ? total_iteration_heal / uptime : 0 );
  hpse.add( sim_length ? total_iteration_heal / sim_length : 0 );
  compound_absorb.add( total_iteration_absorb );
  aps.add( uptime ? total_iteration_absorb / uptime : 0.0 );
  double heal_metric = uptime ? ( ( total_iteration_heal + total_iteration_absorb ) / uptime ) : 0;

  heal_taken.add( p.iteration_heal_taken );
  htps.add( f_length ? p.iteration_heal_taken / f_length : 0 );
  absorb_taken.add( p.iteration_absorb_taken );
  atps.add( f_length ? p.iteration_absorb_taken / f_length : 0.0 );

  dmg_taken.add( p.iteration_dmg_taken );
  dtps.add( f_length ? p.iteration_dmg_taken / f_length : 0 );

  for ( size_t i = 0, end = resource_lost.size(); i < end; ++i )
  {
    resource_lost[ i ].add( p.iteration_resource_lost[ i ] );
  }
  for ( size_t i = 0, end = resource_gained.size(); i < end; ++i )
  {
    resource_gained[ i ].add( p.iteration_resource_gained[ i ] );
  }
  for ( size_t i = 0, end = resource_overflowed.size(); i < end; ++i )
  {
    resource_overflowed[ i ].add( p.iteration_resource_overflowed[ i ] );
  }

  for ( size_t i = 0, end = combat_end_resource.size(); i < end; ++i )
  {
    combat_end_resource[ i ].add( p.resources.current[ i ] );
  }

  if ( !p.is_pet() && p.primary_role() == ROLE_TANK && p.type != PLAYER_SIMPLIFIED )
    health_changes.merged_timeline.merge( health_changes.timeline );

  if ( p.sim->target_error > 0 && !p.is_pet() && !p.is_enemy() )
  {
    double metric = 0;

    role_e target_error_role = p.sim->target_error_role;
    // use player's role if sim didn't provide an override
    if ( target_error_role == ROLE_NONE )
    {
      target_error_role = p.primary_role();
      // exception for tanks - use DPS by default.
      if ( target_error_role == ROLE_TANK )
      {
        target_error_role = ROLE_DPS;
      }
    }

    // ROLE is used here primarily to stay in-line with the previous version of the code.
    // An ideal implementation is probably to rewrite this to allow specification of a scale_metric_e
    // to make it more flexible. That was beyond my capability/available time and it would also likely be
    // very, very low use (as of legion/bfa, almost all tanks are simming DPS, not survival).
    switch ( target_error_role )
    {
      case ROLE_ATTACK:
      case ROLE_SPELL:
      case ROLE_HYBRID:
      case ROLE_DPS:    metric = dps_metric; break;
      case ROLE_TANK:   metric = dps_metric; break;
      case ROLE_HEAL:   metric = heal_metric; break;
      default:          break;
    }

    player_collected_data_t& cd = p.parent ? p.parent->collected_data : *this;

    AUTO_LOCK( cd.target_metric_mutex );
    cd.target_metric.add( metric );
  }
}

bool player_collected_data_t::tank_container_type( const player_t* for_actor, int target_statistics_level )
{
  if ( true )  // FIXME: cannot use virtual function calls here! if ( for_actor->primary_role() == ROLE_TANK )
  {
    return for_actor->sim->statistics_level < target_statistics_level;
  }

  return true;
}

bool player_collected_data_t::generic_container_type( const player_t* for_actor, int target_statistics_level )
{
  if ( !for_actor->is_enemy() && ( !for_actor->is_pet() || for_actor->sim->report_pets_separately ) )
  {
    return for_actor->sim->statistics_level < target_statistics_level;
  }

  return true;
}
