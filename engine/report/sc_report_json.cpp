// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_report.hpp"
#include "interfaces/sc_js.hpp"
#include "simulationcraft.hpp"
#include "util/rapidjson/filewritestream.h"

namespace
{
double to_json( const timespan_t& t )
{
  return t.total_seconds();
}

// js::sc_js_t to_json( const timespan_t& t )
//{
//  js::sc_js_t node;
//  node.set( "seconds", t.total_seconds() );
//  std::string formatted_time;
//  str::format( formatted_time, "%d:%02d.%03d</td>\n", (int)t.total_minutes(),
//               (int)t.total_seconds() % 60, (int)t.total_millis() % 1000 );
//  node.set( "formatted", formatted_time );
//  return node;
//}

js::sc_js_t to_json( const simple_sample_data_t& sd )
{
  js::sc_js_t node;
  node.set( "sum", sd.sum() );
  node.set( "count", sd.count() );
  node.set( "mean", sd.mean() );
  return node;
}

js::sc_js_t to_json( const ::simple_sample_data_with_min_max_t& sd )
{
  js::sc_js_t node;
  node.set( "sum", sd.sum() );
  node.set( "count", sd.count() );
  node.set( "mean", sd.mean() );
  node.set( "min", sd.min() );
  node.set( "max", sd.max() );
  return node;
}

js::sc_js_t to_json( const ::extended_sample_data_t& sd )
{
  js::sc_js_t node;
  node.set( "name", sd.name_str );
  node.set( "sum", sd.sum() );
  node.set( "count", sd.count() );
  node.set( "mean", sd.mean() );
  node.set( "min", sd.min() );
  node.set( "max", sd.max() );
  if ( !sd.simple )
  {
    node.set( "variance", sd.variance );
    node.set( "std_dev", sd.std_dev );
    node.set( "mean_variance", sd.mean_variance );
    node.set( "mean_std_dev", sd.mean_std_dev );
  }
  // node.set( "data", sd.data() );
  // node.set( "distribution", sd.distribution );
  return node;
}

js::sc_js_t to_json( const sc_timeline_t& tl )
{
  js::sc_js_t node;
  node.set( "mean", tl.mean() );
  node.set( "mean_std_dev", tl.mean_stddev() );
  node.set( "min", tl.min() );
  node.set( "max", tl.max() );
  node.set( "data", tl.data() );
  return node;
}

js::sc_js_t to_json( const gain_t& g )
{
  js::sc_js_t node;
  node.set( "name", g.name() );
  js::sc_js_t data;
  for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; ++r )
  {
    if ( g.count[ r ] > 0 )
    {
      js::sc_js_t node2;
      node2.set( "actual", g.actual[ r ] );
      node2.set( "overflow", g.overflow[ r ] );
      node2.set( "count", g.count[ r ] );
      data.set( util::resource_type_string( r ), node2 );
    }
  }
  node.set( "data", data );
  return node;
}

js::sc_js_t to_json( const spelleffect_data_t& /* sed */ )
{
  js::sc_js_t node;
  // TODO
  return node;
}

js::sc_js_t to_json( const spellpower_data_t& /* spd */ )
{
  js::sc_js_t node;
  // TODO
  return node;
}

js::sc_js_t to_json( const spell_data_t& sd )
{
  js::sc_js_t node;
  node.set( "id", sd.id() );
  node.set( "found", sd.found() );
  node.set( "ok", sd.ok() );
  if ( !sd.ok() )
    return node;
  node.set( "category", sd.category() );
  node.set( "class_mask", sd.class_mask() );
  node.set( "cooldown", to_json( sd.cooldown() ) );
  node.set( "charges", sd.charges() );
  node.set( "charge_cooldown", to_json( sd.charge_cooldown() ) );
  if ( sd.desc() )
    node.set( "desc", sd.desc() );
  if ( sd.desc_vars() )
    node.set( "desc_vars", sd.desc_vars() );
  node.set( "duration", to_json( sd.duration() ) );
  node.set( "gcd", to_json( sd.gcd() ) );
  node.set( "initial_stacks", sd.initial_stacks() );
  node.set( "race_mask", sd.race_mask() );
  node.set( "level", sd.level() );
  node.set( "name", sd.name_cstr() );
  node.set( "max_level", sd.max_level() );
  node.set( "max_stacks", sd.max_stacks() );
  node.set( "missile_speed", sd.missile_speed() );
  node.set( "min_range", sd.min_range() );
  node.set( "max_range", sd.max_range() );
  node.set( "proc_chance", sd.proc_chance() );
  node.set( "proc_flags", sd.proc_flags() );
  node.set( "internal_cooldown", to_json( sd.internal_cooldown() ) );
  node.set( "real_ppm", sd.real_ppm() );
  if ( sd.rank_str() )
    node.set( "rank_str", sd.rank_str() );
  node.set( "replace_spell_id", sd.replace_spell_id() );
  node.set( "scaling_multiplier", sd.scaling_multiplier() );
  node.set( "scaling_threshold", sd.scaling_threshold() );
  node.set( "school_mask", sd.school_mask() );
  if ( sd.tooltip() )
    node.set( "tooltip", sd.tooltip() );
  node.set( "school_type", util::school_type_string( sd.get_school_type() ) );
  node.set( "scaling_class", util::player_type_string( sd.scaling_class() ) );
  node.set( "max_scaling_level", sd.max_scaling_level() );
  for ( size_t i = 0u; i < sd.effect_count(); ++i )
  {
    node.add( "effects", to_json( sd.effectN( i + 1 ) ) );
  }
  for ( size_t i = 0u; i < sd.power_count(); ++i )
  {
    node.add( "powers", to_json( sd.powerN( i + 1 ) ) );
  }

  return node;
}

js::sc_js_t to_json( const cooldown_t& cd )
{
  js::sc_js_t node;
  node.set( "name", cd.name() );
  node.set( "duration", to_json( cd.duration ) );
  node.set( "charges", cd.charges );
  return node;
}

js::sc_js_t to_json( const buff_t& b )
{
  js::sc_js_t node;
  node.set( "name", b.name() );
  node.set( "spell_data", to_json( b.data() ) );
  if ( b.source )
    node.set( "source", b.source->name() );
  if ( b.cooldown )
    node.set( "cooldown", to_json( *b.cooldown ) );
  node.set( "uptime_array", to_json( b.uptime_array ) );
  node.set( "default_value", b.default_value );
  node.set( "activated", b.activated );
  node.set( "reactable", b.reactable );
  node.set( "reverse", b.reverse );
  node.set( "constant", b.constant );
  node.set( "quiet", b.quiet );
  node.set( "overridden", b.overridden );
  node.set( "can_cancel", b.can_cancel );
  node.set( "default_chance", b.default_chance );
  // TODO
  return node;
}

js::sc_js_t to_json( result_e i, const stats_t::stats_results_t& sr )
{
  js::sc_js_t node;
  node.set( "result", util::result_type_string( i ) );
  node.set( "actual_amount", to_json( sr.actual_amount ) );
  node.set( "avg_actual_amount", to_json( sr.avg_actual_amount ) );
  node.set( "total_amount", to_json( sr.total_amount ) );
  node.set( "fight_actual_amount", to_json( sr.fight_actual_amount ) );
  node.set( "fight_total_amount", to_json( sr.fight_total_amount ) );
  node.set( "overkill_pct", to_json( sr.overkill_pct ) );
  node.set( "count", to_json( sr.count ) );
  node.set( "pct", sr.pct );
  return node;
}
/*
js::sc_js_t to_json( const benefit_t& b )
{
  js::sc_js_t node;
  node.set( "name", b.name() );
  node.set( "ration", to_json( b.ratio ) );
  return node;
}
*/
js::sc_js_t to_json( const proc_t& p )
{
  js::sc_js_t node;
  node.set( "name", p.name() );
  node.set( "interval_sum", to_json( p.interval_sum ) );
  node.set( "count", to_json( p.count ) );
  return node;
}

js::sc_js_t to_json( const stats_t& s )
{
  js::sc_js_t node;
  node.set( "name", s.name() );
  node.set( "school", util::school_type_string( s.school ) );
  node.set( "type", util::stats_type_string( s.type ) );
  node.set( "resource_gain", to_json( s.resource_gain ) );
  node.set( "num_executes", to_json( s.num_executes ) );
  node.set( "num_ticks", to_json( s.num_ticks ) );
  node.set( "num_refreshes", to_json( s.num_refreshes ) );
  node.set( "num_executes", to_json( s.num_direct_results ) );
  node.set( "num_tick_results", to_json( s.num_tick_results ) );
  node.set( "total_execute_time", to_json( s.total_execute_time ) );
  node.set( "total_tick_time", to_json( s.total_tick_time ) );
  node.set( "portion_amount", s.portion_amount );
  node.set( "total_intervals", to_json( s.total_intervals ) );
  node.set( "actual_amount", to_json( s.actual_amount ) );
  node.set( "total_amount", to_json( s.total_amount ) );
  node.set( "portion_aps", to_json( s.portion_aps ) );
  node.set( "portion_apse", to_json( s.portion_apse ) );
  for ( result_e i = RESULT_NONE; i < RESULT_MAX; ++i )
  {
    node.add( "direct_results", to_json( i, s.direct_results[ i ] ) );
    node.add( "direct_results_detail",
              to_json( i, s.direct_results_detail[ i ] ) );
    node.add( "tick_results", to_json( i, s.tick_results[ i ] ) );
    node.add( "tick_results_detail", to_json( i, s.tick_results_detail[ i ] ) );
  }
  return node;
}

js::sc_js_t to_json( const player_collected_data_t::resource_timeline_t& rtl )
{
  js::sc_js_t node;
  node.set( "resource", util::resource_type_string( rtl.type ) );
  node.set( "timeline", to_json( rtl.timeline ) );
  return node;
}

js::sc_js_t to_json( const player_collected_data_t::stat_timeline_t& stl )
{
  js::sc_js_t node;
  node.set( "stat", util::stat_type_string( stl.type ) );
  node.set( "timeline", to_json( stl.timeline ) );
  return node;
}

js::sc_js_t to_json(
    const player_collected_data_t::health_changes_timeline_t& hctl )
{
  js::sc_js_t node;
  if ( hctl.collect )
  {
    node.set( "timeline", to_json( hctl.merged_timeline ) );
  }
  return node;
}

js::sc_js_t to_json( const player_collected_data_t::buffed_stats_t& bs )
{
  js::sc_js_t node;
  for ( attribute_e a = ATTRIBUTE_NONE; a < ATTRIBUTE_MAX; ++a )
  {
    if ( bs.attribute[ a ] > 0.0 )
    {
      js::sc_js_t anode;
      anode.set( "attribute", util::attribute_type_string( a ) );
      anode.set( "value", bs.attribute[ a ] );
      node.add( "attributes", anode );
    }
  }
  for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; ++r )
  {
    if ( bs.resource[ r ] > 0.0 )
    {
      js::sc_js_t rnode;
      rnode.set( "resource", util::resource_type_string( r ) );
      rnode.set( "value", bs.resource[ r ] );
      node.add( "resource_gained", rnode );
    }
  }
  node.add( "spell_power", bs.spell_power );
  node.add( "spell_hit", bs.spell_hit );
  node.add( "spell_crit", bs.spell_crit_chance );
  node.add( "manareg_per_second", bs.manareg_per_second );
  node.add( "attack_power", bs.attack_power );
  node.add( "attack_hit", bs.attack_hit );
  node.add( "mh_attack_expertise", bs.mh_attack_expertise );
  node.add( "oh_attack_expertise", bs.oh_attack_expertise );
  node.add( "armor", bs.armor );
  node.add( "miss", bs.miss );
  node.add( "crit", bs.crit );
  node.add( "dodge", bs.dodge );
  node.add( "parry", bs.parry );
  node.add( "block", bs.block );
  node.add( "bonus_armor", bs.bonus_armor );
  node.add( "spell_haste", bs.spell_haste );
  node.add( "spell_speed", bs.spell_speed );
  node.add( "attack_haste", bs.attack_haste );
  node.add( "attack_speed", bs.attack_speed );
  node.add( "mastery_value", bs.mastery_value );
  node.add( "damage_versatility", bs.damage_versatility );
  node.add( "heal_versatility", bs.heal_versatility );
  node.add( "mitigation_versatility", bs.mitigation_versatility );
  node.add( "leech", bs.leech );
  node.add( "run_speed", bs.run_speed );
  node.add( "avoidance", bs.avoidance );
  node.add( "leech", bs.leech );
  return node;
}

js::sc_js_t to_json(
    const player_collected_data_t::action_sequence_data_t& asd )
{
  js::sc_js_t node;
  node.set( "time", to_json( asd.time ) );
  if ( asd.action )
  {
    node.set( "action_name", asd.action->name() );
    node.set( "target_name", asd.target->name() );
  }
  else
  {
    node.set( "wait_time", to_json( asd.wait_time ) );
  }
  for ( const auto& buff : asd.buff_list )
  {
    js::sc_js_t bnode;
    bnode.set( "name", buff.first->name() );
    bnode.set( "stacks", buff.second );
    node.add( "buffs", bnode );
  }

  js::sc_js_t resource_snapshot;
  for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; ++r )
  {
    if ( asd.resource_snapshot[ r ] >= 0.0 )
    {
      resource_snapshot.set( util::resource_type_string( r ),
                             asd.resource_snapshot[ r ] );
    }
  }
  node.set( "resource_snapshot", resource_snapshot );

  js::sc_js_t resource_max_snapshot;
  for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; ++r )
  {
    if ( asd.resource_max_snapshot[ r ] >= 0.0 )
    {
      resource_max_snapshot.set( util::resource_type_string( r ),
                                 asd.resource_max_snapshot[ r ] );
    }
  }
  node.set( "resource_max_snapshot", resource_max_snapshot );

  return node;
}

js::sc_js_t to_json( const player_collected_data_t& cd, const sim_t& sim )
{
  js::sc_js_t node;
  node.set( "fight_length", to_json( cd.fight_length ) );
  node.set( "waiting_time", to_json( cd.waiting_time ) );
  node.set( "executed_foreground_actions",
            to_json( cd.executed_foreground_actions ) );
  node.set( "dmg", to_json( cd.dmg ) );
  node.set( "compound_dmg", to_json( cd.compound_dmg ) );
  node.set( "prioritydps", to_json( cd.prioritydps ) );
  node.set( "dps", to_json( cd.dps ) );
  node.set( "dpse", to_json( cd.dpse ) );
  node.set( "dtps", to_json( cd.dtps ) );
  node.set( "dmg_taken", to_json( cd.dmg_taken ) );
  if ( sim.report_details != 0)
  {
    node.set( "timeline_dmg", to_json( cd.timeline_dmg ) );
    node.set( "timeline_dmg_taken", to_json( cd.timeline_dmg_taken ) );
  }

  node.set( "heal", to_json( cd.heal ) );
  node.set( "compound_heal", to_json( cd.compound_heal ) );
  node.set( "hps", to_json( cd.hps ) );
  node.set( "hpse", to_json( cd.hpse ) );
  node.set( "htps", to_json( cd.htps ) );
  node.set( "heal_taken", to_json( cd.heal_taken ) );
  node.set( "timeline_healing_taken", to_json( cd.timeline_healing_taken ) );

  node.set( "absorb", to_json( cd.absorb ) );
  node.set( "compound_absorb", to_json( cd.compound_absorb ) );
  node.set( "aps", to_json( cd.aps ) );
  node.set( "atps", to_json( cd.atps ) );
  node.set( "absorb_taken", to_json( cd.absorb_taken ) );

  node.set( "deaths", to_json( cd.deaths ) );
  node.set( "theck_meloree_index", to_json( cd.theck_meloree_index ) );
  node.set( "effective_theck_meloree_index",
            to_json( cd.effective_theck_meloree_index ) );
  node.set( "max_spike_amount", to_json( cd.max_spike_amount ) );

  node.set( "target_metric", to_json( cd.target_metric ) );

  if ( sim.report_details != 0 )
  {
    js::sc_js_t resource_lost;
    for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; ++r )
    {
      resource_lost.set( util::resource_type_string( r ),
                         to_json( cd.resource_lost[ r ] ) );
    }
    node.set( "resource_lost", resource_lost );

    js::sc_js_t combat_end_resource;
    for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; ++r )
    {
      combat_end_resource.set( util::resource_type_string( r ),
                               to_json( cd.combat_end_resource[ r ] ) );
    }
    node.set( "combat_end_resource", combat_end_resource );

    for ( const auto& rtl : cd.resource_timelines )
    {
      node.add( "resource_timelines", to_json( rtl ) );
    }
    for ( const auto& stl : cd.stat_timelines )
    {
      node.add( "stat_timelines", to_json( stl ) );
    }
    node.set( "health_changes", to_json( cd.health_changes ) );
    node.set( "health_changes", to_json( cd.health_changes_tmi ) );
    for ( const auto& asd : cd.action_sequence )
    {
      node.add( "action_sequence", to_json( *asd ) );
    }
    for ( const auto& asd : cd.action_sequence_precombat )
    {
      node.add( "action_sequence_precombat", to_json( *asd ) );
    }
    node.set( "buffed_stats_snapshot", to_json( cd.buffed_stats_snapshot ) );
  }

  return node;
}

// Forward declaration
js::sc_js_t to_json( const player_t& p );

js::sc_js_t to_json( const pet_t::owner_coefficients_t& oc )
{
  js::sc_js_t node;
  node.set( "armor", oc.armor );
  node.set( "health", oc.health );
  node.set( "ap_from_ap", oc.ap_from_ap );
  node.set( "ap_from_sp", oc.ap_from_sp );
  node.set( "sp_from_ap", oc.sp_from_ap );
  node.set( "sp_from_sp", oc.sp_from_sp );
  return node;
}

js::sc_js_t to_json( const pet_t& p )
{
  js::sc_js_t node;
  node.set( "stamina_per_owner", p.stamina_per_owner );
  node.set( "intellect_per_owner", p.intellect_per_owner );
  node.set( "pet_type", util::pet_type_string( p.pet_type ) );
  node.set( "stamina_per_owner", p.stamina_per_owner );
  node.set( "stamina_per_owner", p.stamina_per_owner );
  node.set( "owner_coefficients", to_json( p.owner_coeff ) );
  node.add( "", to_json( static_cast<const player_t&>( p ) ) );
  return node;
}

js::sc_js_t to_json( const dbc_t& dbc )
{
  js::sc_js_t node;
  bool versions[] = {false, true};
  for ( const auto& ptr : versions )
  {
    js::sc_js_t subnode;
    subnode.set( "build_level", dbc::build_level( ptr ) );
    subnode.set( "wow_version", dbc::wow_version( ptr ) );

    node.set( dbc::wow_ptr_status( ptr ), subnode );
  }
  node.set( "version_used", dbc::wow_ptr_status( dbc.ptr ) );
  return node;
}

js::sc_js_t to_json( const player_t::base_initial_current_t& )
{
  js::sc_js_t node;
  // TODO
  return node;
}

js::sc_js_t to_json( const player_t::diminishing_returns_constants_t& )
{
  js::sc_js_t node;
  // TODO
  return node;
}

js::sc_js_t to_json( const weapon_t& )
{
  js::sc_js_t node;
  // TODO
  return node;
}

js::sc_js_t to_json( const player_t::resources_t& )
{
  js::sc_js_t node;
  // TODO
  return node;
}

js::sc_js_t to_json( const player_t::consumables_t& )
{
  js::sc_js_t node;
  // TODO
  return node;
}

js::sc_js_t to_json( const player_t& p,
                     const player_processed_report_information_t& /* ri */ )
{
  js::sc_js_t node;
  if ( p.sim->scaling->has_scale_factors() )
  {
    if ( p.sim->report_precision < 0 )
      p.sim->report_precision = 2;

    scale_metric_e sm      = p.sim->scaling->scaling_metric;
    const gear_stats_t& sf = ( p.sim->scaling->normalize_scale_factors )
                                 ? p.scaling_normalized[ sm ]
                                 : p.scaling[ sm ];

    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      if ( p.scales_with[ i ] )
      {
        node.set( util::stat_type_abbrev( i ), sf.get_stat( i ) );
      }
    }
  }
  return node;
}

js::sc_js_t to_json( const player_t& p )
{
  js::sc_js_t node;
  node.set( "name", p.name() );
  node.set( "race", util::race_type_string( p.race ) );
  node.set( "role", util::role_type_string( p.role ) );
  node.set( "level", p.true_level );
  node.set( "party", p.party );
  node.set( "ready_type", p.ready_type );
  node.set( "specialization",
            util::specialization_string( p.specialization() ) );
  node.set( "bugs", p.bugs );
  node.set( "scale_player", p.scale_player );
  node.set( "death_pct", p.death_pct );
  node.set( "height", p.height );
  node.set( "combat_reach", p.combat_reach );
  node.set( "potion_used", p.potion_used );
  node.set( "timeofday", ( p.timeofday == player_t::NIGHT_TIME ? "NIGHT_TIME"
                                                               : "DAY_TIME" ) );
  node.set( "gcd_ready", to_json( p.gcd_ready ) );
  node.set( "base_gcd", to_json( p.base_gcd ) );
  node.set( "started_waiting", to_json( p.started_waiting ) );
  if (!p.report_information.thumbnail_url.empty()) {
    node.set( "thumbnail_url", p.report_information.thumbnail_url.c_str() );
  }
  for ( const auto& pet : p.pet_list )
  {
    node.add( "pets", to_json( *pet ) );
  }
  node.set( "invert_scaling", p.invert_scaling );
  node.set( "reaction_offset", to_json( p.reaction_offset ) );
  node.set( "reaction_mean", to_json( p.reaction_mean ) );
  node.set( "reaction_stddev", to_json( p.reaction_stddev ) );
  node.set( "reaction_nu", to_json( p.reaction_nu ) );
  node.set( "world_lag", to_json( p.world_lag ) );
  node.set( "world_lag_stddev", to_json( p.world_lag_stddev ) );
  node.set( "brain_lag", to_json( p.brain_lag ) );
  node.set( "brain_lag_stddev", to_json( p.brain_lag_stddev ) );
  node.set( "world_lag_override", p.world_lag_override );
  node.set( "world_lag_stddev_override", p.world_lag_stddev_override );
  node.set( "dbc", to_json( p.dbc ) );
  for ( auto i = PROFESSION_NONE; i < PROFESSION_MAX; ++i )
  {
    if ( p.profession[ i ] > 0 )
    {
      js::sc_js_t pnode;
      pnode.set( util::profession_type_string( i ), p.profession[ i ] );
      node.add( "professions", pnode );
    }
  }
  node.set( "base_stats", to_json( p.base ) );
  node.set( "initial_stats", to_json( p.initial ) );
  node.set( "current_stats", to_json( p.current ) );
  node.set( "base_energy_regen_per_second", p.base_energy_regen_per_second );
  node.set( "base_focus_regen_per_second", p.base_focus_regen_per_second );
  node.set( "base_chi_regen_per_second", p.base_chi_regen_per_second );
  node.set( "diminishing_returns_constants", to_json( p.def_dr ) );
  node.set( "main_hand_weapon", to_json( p.main_hand_weapon ) );
  node.set( "off_hand_weapon", to_json( p.off_hand_weapon ) );
  node.set( "resources", to_json( p.resources ) );
  node.set( "consumables", to_json( p.consumables ) );
  node.set( "scale_factors", to_json( p, p.report_information ) );
  // TODO : "plot_data"

  // TODO

  node.set( "collected_data", to_json( p.collected_data, p.sim ) );
  // TODO

  if ( p.sim->report_details != 0 )
  {
    for ( const auto& buff : p.buff_list )
    {
      node.add( "buffs", to_json( *buff ) );
    }
    for ( const auto& proc : p.proc_list )
    {
      node.add( "procs", to_json( *proc ) );
    }
    for ( const auto& gain : p.gain_list )
    {
      node.add( "gains", to_json( *gain ) );
    }
    for ( const auto& stat : p.stats_list )
    {
      node.add( "stats", to_json( *stat ) );
    }
  }

  return node;
}

js::sc_js_t to_json( const rng::rng_t& rng )
{
  js::sc_js_t node;
  node.set( "name", rng.name() );
  return node;
}

js::sc_js_t to_json( const raid_event_t& re )
{
  js::sc_js_t node;
  node.set( "name", re.name() );
  node.set( "first", to_json( re.first ) );
  node.set( "last", to_json( re.last ) );
  node.set( "next", to_json( re.next ) );
  node.set( "cooldown", to_json( re.cooldown ) );
  node.set( "cooldown_stddev", to_json( re.cooldown_stddev ) );
  node.set( "cooldown_min", to_json( re.cooldown_min ) );
  node.set( "cooldown_max", to_json( re.cooldown_max ) );
  node.set( "duration", to_json( re.duration ) );
  node.set( "duration_stddev", to_json( re.duration_stddev ) );
  node.set( "duration_min", to_json( re.duration_min ) );
  node.set( "duration_max", to_json( re.duration_max ) );
  node.set( "distance_min", re.distance_min );
  node.set( "distance_max", re.distance_max );
  node.set( "players_only", re.players_only );
  node.set( "player_chance", re.player_chance );
  node.set( "affected_role", util::role_type_string( re.affected_role ) );
  node.set( "saved_duration", to_json( re.saved_duration ) );
  return node;
}

js::sc_js_t to_json( const sim_t::overrides_t& o )
{
  js::sc_js_t node;
  node.set( "mortal_wounds", o.mortal_wounds );
  node.set( "bleeding", o.bleeding );
  node.set( "bloodlust", o.bloodlust );
  node.set( "target_health", o.target_health );
  return node;
}

js::sc_js_t to_json( const scaling_t& /* o */ )
{
  js::sc_js_t node;
  // TODO
  return node;
}

js::sc_js_t to_json( const plot_t& o )
{
  js::sc_js_t node;
  node.set( "dps_plot_stat_str", o.dps_plot_stat_str );
  node.set( "dps_plot_step", o.dps_plot_step );
  node.set( "dps_plot_points", o.dps_plot_points );
  node.set( "dps_plot_iterations", o.dps_plot_iterations );
  node.set( "dps_plot_target_error", o.dps_plot_target_error );
  node.set( "dps_plot_debug", o.dps_plot_debug );
  node.set( "dps_plot_positive", o.dps_plot_positive );
  node.set( "dps_plot_negative", o.dps_plot_negative );
  return node;
}

js::sc_js_t to_json( const reforge_plot_t& /* o */ )
{
  js::sc_js_t node;
  // TODO
  return node;
}

js::sc_js_t to_json( const iteration_data_entry_t& ide )
{
  js::sc_js_t node;
  node.set( "metric", ide.metric );
  node.set( "seed", ide.seed );
  node.set( "target_health", ide.target_health );
  return node;
}

js::sc_js_t to_json( const sim_t& sim )
{
  js::sc_js_t node;
  node.set( "debug", sim.debug );
  node.set( "max_time", to_json( sim.max_time ) );
  node.set( "expected_iteration_time", to_json( sim.expected_iteration_time ) );
  node.set( "vary_combat_length", sim.vary_combat_length );
  node.set( "iterations", sim.iterations );
  node.set( "target_error", sim.target_error );
  for ( const auto& player : sim.player_no_pet_list.data() )
  {
    node.add( "players", to_json( *player ) );
  }
  if ( sim.report_details != 0 )
  {
    for ( const auto& healing : sim.healing_no_pet_list.data() )
    {
      node.add( "healing_players", to_json( *healing ) );
    }
    for ( const auto& target : sim.target_list.data() )
    {
      node.add( "target", to_json( *target ) );
    }
  }
  node.set( "queue_lag", to_json( sim.queue_lag ) );
  node.set( "queue_lag_stddev", to_json( sim.queue_lag_stddev ) );
  node.set( "gcd_lag", to_json( sim.gcd_lag ) );
  node.set( "gcd_lag_stddev", to_json( sim.gcd_lag_stddev ) );
  node.set( "channel_lag", to_json( sim.channel_lag ) );
  node.set( "channel_lag_stddev", to_json( sim.channel_lag_stddev ) );
  node.set( "queue_gcd_reduction", to_json( sim.queue_gcd_reduction ) );
  node.set( "strict_gcd_queue", sim.strict_gcd_queue );
  node.set( "confidence", sim.confidence );
  node.set( "confidence_estimator", sim.confidence_estimator );
  node.set( "world_lag", to_json( sim.world_lag ) );
  node.set( "world_lag_stddev", to_json( sim.world_lag_stddev ) );
  node.set( "travel_variance", sim.travel_variance );
  node.set( "default_skill", sim.default_skill );
  node.set( "reaction_time", to_json( sim.reaction_time ) );
  node.set( "regen_periodicity", to_json( sim.regen_periodicity ) );
  node.set( "ignite_sampling_delta", to_json( sim.ignite_sampling_delta ) );
  node.set( "fixed_time", sim.fixed_time );
  node.set( "optimize_expressions", sim.optimize_expressions );
  node.set( "optimal_raid", sim.optimal_raid );
  node.set( "log", sim.log );
  node.set( "debug_each", sim.debug_each );
  node.set( "auto_ready_trigger", sim.auto_ready_trigger );
  node.set( "stat_cache", sim.stat_cache );
  node.set( "max_aoe_enemies", sim.max_aoe_enemies );
  node.set( "show_etmi", sim.show_etmi );
  node.set( "tmi_window_global", sim.tmi_window_global );
  node.set( "tmi_bin_size", sim.tmi_bin_size );
  node.set( "requires_regen_event", sim.requires_regen_event );
  node.set( "enemy_death_pct", sim.enemy_death_pct );
  node.set( "dbc", to_json( sim.dbc ) );
  node.set( "challenge_mode", sim.challenge_mode );
  node.set( "pvp_crit", sim.pvp_crit );
  node.set( "rng", to_json( sim.rng() ) );
  node.set( "rng_seed", sim.seed );
  node.set( "deterministic", sim.deterministic );
  node.set( "average_range", sim.average_range );
  node.set( "average_gauss", sim.average_gauss );
  for ( const auto& re : sim.raid_events )
  {
    node.add( "raid_events", to_json( *re ) );
  }
  node.set( "fight_style", sim.fight_style );
  node.set( "overrides", to_json( sim.overrides ) );
  for ( const auto& buff : sim.buff_list )
  {
    node.add( "buffs", to_json( *buff ) );
  }
  node.set( "default_aura_delay", to_json( sim.default_aura_delay ) );
  node.set( "default_aura_delay_stddev",
            to_json( sim.default_aura_delay_stddev ) );
  for ( const auto& cooldown : sim.cooldown_list )
  {
    node.add( "cooldowns", to_json( *cooldown ) );
  }
  node.set( "scaling", to_json( *sim.scaling ) );
  node.set( "plot", to_json( *sim.plot ) );
  node.set( "reforge_plot", to_json( *sim.reforge_plot ) );
  node.set( "elapsed_cpu", sim.elapsed_cpu );
  node.set( "elapsed_time", sim.elapsed_time );
  node.set( "raid_dps", to_json( sim.raid_dps ) );
  node.set( "total_dmg", to_json( sim.total_dmg ) );
  node.set( "raid_hps", to_json( sim.raid_hps ) );
  node.set( "total_heal", to_json( sim.total_heal ) );
  node.set( "total_absorb", to_json( sim.total_absorb ) );
  node.set( "raid_aps", to_json( sim.raid_aps ) );
  node.set( "simulation_length", to_json( sim.simulation_length ) );
  for ( const auto& id : sim.iteration_data )
  {
    node.add( "iteration_data", to_json( id ) );
  }
  for ( const auto& id : sim.low_iteration_data )
  {
    node.add( "low_iteration_data", to_json( id ) );
  }
  for ( const auto& id : sim.high_iteration_data )
  {
    node.add( "high_iteration_data", to_json( id ) );
  }
  for ( const auto& error : sim.error_list )
  {
    node.add( "errors", error );
  }

  return node;
}

js::sc_js_t get_root( const sim_t& sim )
{
  js::sc_js_t root;
  root.set( "version", SC_VERSION );
#if defined( SC_GIT_REV )
  root.set( "git_rev", SC_GIT_REV );
#endif
  root.set( "ptr_enabled", SC_USE_PTR );
  root.set( "beta_enabled", SC_BETA );
  root.set( "build_date", __DATE__ );
  root.set( "build_time", __TIME__ );
  root.set( "sim", to_json( sim ) );
  return root;
}
/*
void print_json_raw( FILE* o, const sim_t& sim )
{
  js::sc_js_t root = get_root( sim );
  std::array<char, 1024> buffer;
  rapidjson::FileWriteStream b( o, buffer.data(), buffer.size() );
  rapidjson::Writer<rapidjson::FileWriteStream> writer( b );
  root.js_.Accept( writer );
}*/

void print_json_pretty( FILE* o, const sim_t& sim )
{
  js::sc_js_t root = get_root( sim );
  std::array<char, 1024> buffer;
  rapidjson::FileWriteStream b( o, buffer.data(), buffer.size() );
  rapidjson::PrettyWriter<rapidjson::FileWriteStream> writer( b );
  root.js_.Accept( writer );
}
/*
std::string print_json_string_raw( const sim_t& sim )
{
  js::sc_js_t root = get_root( sim );
  rapidjson::StringBuffer b;
  rapidjson::Writer<rapidjson::StringBuffer> writer( b );
  root.js_.Accept( writer );
  return b.GetString();
}

std::string print_json_string_pretty( const sim_t& sim )
{
  js::sc_js_t root = get_root( sim );
  rapidjson::StringBuffer b;
  rapidjson::PrettyWriter<rapidjson::StringBuffer> writer( b );
  root.js_.Accept( writer );
  return b.GetString();
}
*/
}  // unnamed namespace

namespace report
{
void print_json( sim_t& sim )
{
  if ( sim.json_file_str.empty() )
    return;
  // Setup file stream and open file
  io::cfile s( sim.json_file_str, "w" );
  if ( !s )
  {
    sim.errorf( "Failed to open JSON output file '%s'.",
                sim.json_file_str.c_str() );
    return;
  }

  // Print JSON report
  try
  {
    Timer t( "JSON report" );
    print_json_pretty( s, sim );
  }
  catch ( const std::exception& e )
  {
    sim.errorf( "Failed to print JSON output! %s", e.what() );
  }
}

}  // report
