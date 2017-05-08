// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "sc_report.hpp"
#include "interfaces/sc_js.hpp"
#include "simulationcraft.hpp"

#include "util/rapidjson/filewritestream.h"
#include "util/rapidjson/document.h"
#include "util/rapidjson/stringbuffer.h"
#include "util/rapidjson/prettywriter.h"

using namespace rapidjson;
using namespace js;

namespace
{
double to_json( const timespan_t& t )
{
  return t.total_seconds();
}

/**
 * Template helper to add only non "default value" containers (sample data essentially) to the JSON
 * root.
 */
template <typename T>
void add_non_default( JsonOutput root, const char* name, const T& container )
{
  if ( container.mean() != 0 )
  {
    root[ name ] = container;
  }
}

template <typename T>
void add_non_default( JsonOutput root, const char* name, const T& v, const T& default_value )
{
  if ( v != default_value )
  {
    root[ name ] = v;
  }
}

/* Template helper to add only non-zero "containers" (sample data essentially) to the JSON root. */
template <typename T>
void add_non_zero( JsonOutput root, const char* name, const T& container )
{ add_non_default( root, name, container ); }

void add_non_zero( JsonOutput root, const char* name, const timespan_t& v )
{ add_non_default( root, name, v, timespan_t::zero() ); }

void add_non_zero( JsonOutput root, const char* name, double v )
{ add_non_default( root, name, v, 0.0 ); }

void add_non_zero( JsonOutput root, const char* name, unsigned v )
{ add_non_default( root, name, v, 0U ); }

void add_non_zero( JsonOutput root, const char* name, int v )
{ add_non_default( root, name, v, 0 ); }

void add_non_zero( JsonOutput root, const char* name, bool v )
{ add_non_default( root, name, v, false ); }

void add_non_zero( JsonOutput root, const char* name, const std::string& v )
{ add_non_default( root, name, v, std::string() ); }

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

bool has_resources( const gain_t* gain )
{
  return range::find_if( gain -> count, []( double v ) { return v != 0; } ) != gain -> count.end();
}

bool has_resources( const gain_t& gain )
{ return has_resources( &gain ); }

void gain_to_json( JsonOutput root, const gain_t* g )
{
  root[ "name" ] = g -> name();

  for ( resource_e r = RESOURCE_NONE; r < RESOURCE_MAX; ++r )
  {
    if ( g -> count[ r ] == 0 )
    {
      continue;
    }

    root[ util::resource_type_string( r ) ][ "actual" ] = g -> actual[ r ];
    root[ util::resource_type_string( r ) ][ "overflow" ] = g -> overflow[ r ];
    root[ util::resource_type_string( r ) ][ "count" ] = g -> count[ r ];
  }
}

void gain_to_json( JsonOutput root, const gain_t& g )
{ gain_to_json( root, &g ); }


void gains_to_json( JsonOutput root, const player_t& p )
{
  root.make_array();

  range::for_each( p.gain_list, [ & ]( const gain_t* g ) {
    if ( ! has_resources( g ) )
    {
      return;
    }

    gain_to_json( root.add(), g );
  } );
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

void to_json( JsonOutput root, const buff_t* b )
{
  root[ "name" ] = b -> name();
  if ( b -> data().id() )
  {
    root[ "spell" ] = b -> data().id();
  }
  if ( b -> item )
  {
    root[ "item" ][ "id" ] = b -> item -> parsed.data.id;
    root[ "item" ][ "ilevel" ] = b -> item -> item_level();
  }

  if ( b -> source && b -> source != b -> player )
  {
    root[ "source" ] = b -> source -> name();
  }

  if ( b -> cooldown && b -> cooldown -> duration > timespan_t::zero() )
  {
    root[ "cooldown" ] = *b -> cooldown;
  }

  root[ "start_count" ] = b -> avg_start.mean();
  add_non_zero( root, "refresh_count", b -> avg_refresh.mean() );
  add_non_zero( root, "interval", b -> start_intervals.mean() );
  add_non_zero( root, "trigger", b -> trigger_intervals.mean() );
  add_non_zero( root, "uptime", b -> uptime_pct.mean() );
  add_non_zero( root, "benefit", b -> benefit_pct.mean() );
  add_non_zero( root, "overflow_stacks", b -> avg_overflow_count.mean() );
  add_non_zero( root, "overflow_total" , b -> avg_overflow_total.mean() );
  add_non_zero( root, "expire_count", b -> avg_expire.mean() );
  if ( b -> default_value != buff_t::DEFAULT_VALUE() )
  {
    root[ "default_value" ] = b -> default_value;
  }

  if ( b -> sim -> buff_uptime_timeline != 0 && b -> uptime_array.mean() != 0 )
  {
    root[ "stack_uptime" ] = b -> uptime_array;
  }
}

void buffs_to_json( JsonOutput root, const player_t& p )
{
  root.make_array();
  range::for_each( p.buff_list, [ &root, &p ]( const buff_t* b ) {
    if ( b -> avg_start.mean() == 0 )
    {
      return;
    }
    to_json( root.add(), b );
  } );
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

void to_json( JsonOutput root, const stats_t::stats_results_t& sr )
{
  root[ "actual_amount" ] = sr.actual_amount;
  root[ "avg_actual_amount" ] = sr.avg_actual_amount;
  root[ "total_amount" ] = sr.total_amount;
  root[ "fight_actual_amount" ] = sr.fight_actual_amount;
  root[ "fight_total_amount" ] = sr.fight_total_amount;
  root[ "overkill_pct" ] = sr.overkill_pct;
  root[ "count" ] = sr.count;
  root[ "pct" ] = sr.pct;
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

void procs_to_json( JsonOutput root, const player_t& p )
{
  root.make_array();
  range::for_each( p.proc_list, [ & ]( const proc_t* proc ) {
    if ( proc -> count.mean() == 0 )
    {
      return;
    }

    auto node = root.add();
    node[ "name" ] = proc -> name();
    node[ "interval" ] = proc -> interval_sum.mean();
    node[ "count" ] = proc -> count.mean();
  } );
}

js::sc_js_t to_json( const proc_t& p )
{
  js::sc_js_t node;
  node.set( "name", p.name() );
  node.set( "interval_sum", to_json( p.interval_sum ) );
  node.set( "count", to_json( p.count ) );
  return node;
}

void stats_to_json( JsonOutput root, const player_t& p )
{
  root.make_array();

  range::for_each( p.stats_list, [ & ]( const stats_t* s ) {
    if ( s -> quiet || s -> num_executes.mean() == 0 )
    {
      return;
    }

    auto node = root.add();

    node[ "name" ] = s -> name();
    if ( s -> school != SCHOOL_NONE )
    {
      node[ "school" ] = util::school_type_string( s -> school );
    }
    node[ "type" ] = util::stats_type_string( s -> type );

    if ( has_resources( s -> resource_gain ) )
    {
      gain_to_json( node[ "resource_gain" ], s -> resource_gain );
    }

    node[ "num_executes" ] = s -> num_executes;

    add_non_zero( node, "total_execute_time", s -> total_execute_time );
    add_non_zero( node, "portion_aps", s -> portion_aps );
    add_non_zero( node, "portion_apse", s -> portion_apse );
    add_non_zero( node, "portion_amount", s -> portion_amount );
    add_non_zero( node, "actual_amount", s -> actual_amount );
    add_non_zero( node, "total_amount", s -> total_amount );

    if ( s -> num_executes.mean() > 1 )
    {
      node[ "total_intervals" ] = s -> total_intervals;
    }

    add_non_zero( node, "num_ticks", s -> num_ticks );
    add_non_zero( node, "num_tick_results", s -> num_tick_results );
    add_non_zero( node, "total_tick_time", s -> total_tick_time );
    add_non_zero( node, "num_refreshes", s -> num_refreshes );

    add_non_zero( node, "num_direct_results", s -> num_direct_results );

    for ( full_result_e r = FULLTYPE_NONE; r < FULLTYPE_MAX; ++r )
    {
      if ( s -> direct_results_detail[ r ].count.mean() != 0 )
      {
        to_json( node[ "direct_results" ][ util::full_result_type_string( r ) ],
                 s -> direct_results_detail[ r ] );
      }

      if ( s -> tick_results_detail[ r ].count.mean() != 0 )
      {
        to_json( node[ "tick_results" ][ util::full_result_type_string( r ) ],
                 s -> tick_results_detail[ r ] );
      }
    }
  } );
}

void gear_to_json( JsonOutput root, const player_t& p )
{
  for ( slot_e slot = SLOT_MIN; slot < SLOT_MAX; slot++ )
  {
    const item_t& item = p.items[ slot ];
    if ( !item.active() || !item.has_stats() )
    {
      continue;
    }
    auto slotnode = root[ item.slot_name() ];

    slotnode[ "name" ] = item.name_str;
    slotnode[ "encoded_item" ] = item.encoded_item();
    slotnode[ "ilevel" ] = item.item_level();

    for ( size_t i = 0; i < sizeof_array( item.parsed.data.stat_val ); i++ )
    {
      auto val = item.stat_value( i );
      if ( val <= 0)
      {
        continue;
      }
      slotnode[ util::stat_type_string( item.stat( i ) ) ] = val;
    }
  }
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

void to_json( JsonOutput root, const player_collected_data_t::buffed_stats_t& bs,
                               const std::vector<resource_e>& relevant_resources )
{
  for ( attribute_e a = ATTRIBUTE_NONE; a < ATTRIBUTE_MAX; ++a )
  {
    if ( bs.attribute[ a ] != 0.0 )
    {
      root[ "attribute" ][ util::attribute_type_string( a ) ] = bs.attribute[ a ];
    }
  }

  range::for_each( relevant_resources, [ &root, &bs ]( resource_e r ) {
    root[ "resources" ][ util::resource_type_string( r ) ] = bs.resource[ r ];
  } );

  add_non_zero( root[ "stats" ], "spell_power", bs.spell_power );
  add_non_zero( root[ "stats" ], "attack_power", bs.attack_power );
  add_non_zero( root[ "stats" ], "spell_crit", bs.spell_crit_chance );
  add_non_zero( root[ "stats" ], "attack_crit", bs.attack_crit_chance );
  add_non_zero( root[ "stats" ], "spell_haste", bs.spell_haste );
  add_non_zero( root[ "stats" ], "attack_haste", bs.attack_haste );
  add_non_zero( root[ "stats" ], "spell_speed", bs.spell_speed );
  add_non_zero( root[ "stats" ], "attack_speed", bs.attack_speed );

  add_non_zero( root[ "stats" ], "mastery_value", bs.mastery_value );
  add_non_zero( root[ "stats" ], "damage_versatility", bs.damage_versatility );
  add_non_zero( root[ "stats" ], "heal_versatility", bs.heal_versatility );
  add_non_zero( root[ "stats" ], "mitigation_versatility", bs.mitigation_versatility );

  add_non_zero( root[ "stats" ], "leech", bs.leech );
  add_non_zero( root[ "stats" ], "speed", bs.run_speed );
  add_non_zero( root[ "stats" ], "avoidance", bs.avoidance );

  add_non_zero( root[ "stats" ], "manareg_per_second", bs.manareg_per_second );

  add_non_zero( root[ "stats" ], "armor", bs.armor );
  add_non_zero( root[ "stats" ], "dodge", bs.dodge );
  add_non_zero( root[ "stats" ], "parry", bs.parry );
  add_non_zero( root[ "stats" ], "block", bs.block );
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

void to_json( JsonOutput root,
              const std::vector<player_collected_data_t::action_sequence_data_t*>& asd,
              const std::vector<resource_e>& relevant_resources )
{
  root.make_array();

  range::for_each( asd, [ &root, &relevant_resources ]( const player_collected_data_t::action_sequence_data_t* entry ) {
    auto json = root.add();

    json[ "time" ] = entry -> time;
    if ( entry -> action )
    {
      json[ "name" ] = entry -> action -> name();
      json[ "target" ] = entry -> action -> target -> name();
    }
    else
    {
      json[ "wait" ] = entry -> wait_time;
    }

    if ( entry -> buff_list.size() > 0 )
    {
      auto buffs = json[ "buffs" ];
      buffs.make_array();
      range::for_each( entry -> buff_list, [ &buffs ]( const std::pair<buff_t*, int> data ) {
        auto entry = buffs.add();

        entry[ "name" ] = data.first -> name();
        entry[ "stacks" ] = data.second;
      } );
    }

    auto resources = json[ "resources" ];
    auto resources_max = json[ "resources_max" ];
    range::for_each( relevant_resources, [ &json, &resources, &resources_max, &entry ]( resource_e r ) {
      resources[ util::resource_type_string( r ) ] = entry -> resource_snapshot[ r ];
      // TODO: Why do we have this instead of using some static one?
      resources_max[ util::resource_type_string( r ) ] = entry -> resource_max_snapshot[ r ];
    } );
  } );
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

void collected_data_to_json( JsonOutput root, const player_t& p )
{
  const auto& sim = *p.sim;
  const auto& cd = p.collected_data;

  add_non_zero( root, "fight_length", cd.fight_length );
  add_non_zero( root, "waiting_time", cd.waiting_time );
  add_non_zero( root, "executed_foreground_actions", cd.executed_foreground_actions );
  add_non_zero( root, "dmg", cd.dmg );
  add_non_zero( root, "compound_dmg", cd.compound_dmg );
  add_non_zero( root, "timeline_dmg", cd.timeline_dmg );
  if ( !p.is_enemy() && sim.target_list.size() > 1 )
  {
    add_non_zero( root, "prioritydps", cd.prioritydps );
  }
  add_non_zero( root, "dps", cd.dps );
  add_non_zero( root, "dpse", cd.dpse );

  if ( p.role == ROLE_TANK || p.role == ROLE_HEAL )
  {
    if ( p.role == ROLE_TANK )
    {
      root[ "dtps" ] = cd.dmg_taken;
      root[ "timeline_dmg_taken" ] = cd.timeline_dmg_taken;
      root[ "deaths" ] = cd.deaths;
      root[ "theck_meloree_index" ] = cd.theck_meloree_index;
      root[ "effective_theck_meloree_index" ] = cd.effective_theck_meloree_index;
      root[ "max_spike_amount" ] = cd.max_spike_amount;
    }

    root[ "heal" ] = cd.heal;
    root[ "compound_heal" ] = cd.compound_heal;
    root[ "hps" ] = cd.hps;
    root[ "hpse" ] = cd.hpse;
    root[ "absorb" ] = cd.absorb;
    root[ "compound_absorb" ] = cd.compound_absorb;
    root[ "aps" ] = cd.aps;

    if ( p.role == ROLE_TANK )
    {
      root[ "htps" ] = cd.htps;
      root[ "heal_taken" ] = cd.heal_taken;
      root[ "timeline_healing_taken" ] = cd.timeline_healing_taken;
      root[ "atps" ] = cd.atps;
      root[ "absorb_taken" ] = cd.absorb_taken;
    }
  }

  // No point printing out target metric if target_error is not used
  if ( cd.target_metric.count() > 0 )
  {
    root[ "target_metric" ] = cd.target_metric;
  }

  if ( sim.report_details != 0 )
  {
    // Key off of resource loss to figure out what resources are even relevant
    std::vector<resource_e> relevant_resources;
    for ( size_t i = 0, end = cd.resource_lost.size(); i < end; ++i )
    {
      auto r = static_cast<resource_e>( i );

      if ( cd.resource_lost[ r ].mean() > 0 )
      {
        root[ "resource_lost" ][ util::resource_type_string( r ) ] = cd.resource_lost[ r ];
        relevant_resources.push_back( r );
      }
    }

    // Rest of the resource summaries are printed only based on relevant resources
    range::for_each( relevant_resources, [ &root, &cd ]( resource_e r ) {
      if ( r < cd.combat_end_resource.size() )
      {
        // Combat ending resources
        add_non_zero( root[ "combat_end_resource" ], util::resource_type_string( r ), cd.combat_end_resource[ r ] );
      }

      // Resource timeline for a given resource with loss
      auto it = range::find_if( cd.resource_timelines, [ r ]( const player_collected_data_t::resource_timeline_t& rtl ) {
        return rtl.type == r;
      } );

      if ( it != cd.resource_timelines.end() )
      {
        root[ "resource_timelines" ][ util::resource_type_string( r ) ] = it -> timeline;
      }
    } );

    // Stat timelines, if they exist
    range::for_each( cd.stat_timelines, [ &root, &cd ]( const player_collected_data_t::stat_timeline_t& stl ) {
      add_non_zero( root[ "stat_timelines" ], util::stat_type_string( stl.type ), stl.timeline );
    } );

    if ( cd.health_changes.collect )
    {
      root[ "health_changes" ] = cd.health_changes.merged_timeline;
    }

    if ( cd.health_changes_tmi.collect )
    {
      root[ "health_changes_tmi" ] = cd.health_changes_tmi.merged_timeline;
    }

    if ( ! cd.action_sequence_precombat.empty() )
    {
      to_json( root[ "action_sequence_precombat" ], cd.action_sequence_precombat, relevant_resources );
    }

    if ( ! cd.action_sequence.empty() )
    {
      to_json( root[ "action_sequence" ], cd.action_sequence, relevant_resources );
    }

    to_json( root[ "buffed_stats" ], cd.buffed_stats_snapshot, relevant_resources );
  }
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

void to_json( JsonOutput root, const dbc_t& dbc )
{
  bool versions[] = { false, true };
  for ( const auto& ptr : versions )
  {
    root[ dbc::wow_ptr_status( ptr ) ][ "build_level" ] = dbc::build_level( ptr );
    root[ dbc::wow_ptr_status( ptr ) ][ "wow_version" ] = StringRef( dbc::wow_version( ptr ) );
  }

  root[ "version_used" ] = StringRef( dbc::wow_ptr_status( dbc.ptr ) );
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
                                 ? p.scaling->scaling_normalized[ sm ]
                                 : p.scaling->scaling[ sm ];

    for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    {
      if ( p.scaling->scales_with[ i ] )
      {
        node.set( util::stat_type_abbrev( i ), sf.get_stat( i ) );
      }
    }
  }
  return node;
}


void scale_factors_to_json( JsonOutput root, const player_t& p )
{
  if ( p.sim -> report_precision < 0 )
    p.sim -> report_precision = 2;

  auto sm = p.sim -> scaling -> scaling_metric;
  const auto& sf = ( p.sim -> scaling -> normalize_scale_factors )
                   ? p.scaling->scaling_normalized[ sm ]
                   : p.scaling->scaling[ sm ];

  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    if ( p.scaling->scales_with[ i ] )
    {
      root[ util::stat_type_abbrev( i ) ] = sf.get_stat( i );
    }
  }
}

void to_json( JsonOutput& arr, const player_t& p )
{
  auto root = arr.add(); // Add a fresh object to the players array and use it as root

  root[ "name" ] = p.name();
  root[ "race" ] = util::race_type_string( p.race );
  root[ "role" ] = util::role_type_string( p.role );
  root[ "specialization" ] = util::specialization_string( p.specialization() );

  root[ "level" ] = p.true_level;
  root[ "party" ] = p.party;
  root[ "ready_type" ] = p.ready_type;
  root[ "bugs" ] = p.bugs;
  root[ "scale_player" ] = p.scale_player;
  root[ "potion_used" ] = p.potion_used;
  root[ "timeofday" ] = p.timeofday == player_t::NIGHT_TIME ? "NIGHT_TIME" : "DAY_TIME";

  if ( p.is_enemy() )
  {
    root[ "death_pct" ] = p.death_pct;
    root[ "height" ] = p.height;
    root[ "combat_reach" ] = p.combat_reach;
  }

  if ( p.sim -> report_pets_separately )
  {
    // TODO: Pet reporting
  }

  root[ "invert_scaling" ] = p.invert_scaling;
  root[ "reaction_offset" ] = p.reaction_offset;
  root[ "reaction_mean" ] = p.reaction_mean;
  root[ "reaction_stddev" ] = p.reaction_stddev;
  root[ "reaction_nu" ] = p.reaction_nu;
  root[ "world_lag" ] = p.world_lag;
  root[ "brain_lag" ] = p.brain_lag;
  root[ "brain_lag_stddev" ] = p.brain_lag_stddev;
  root[ "world_lag_override" ] = p.world_lag_override;
  root[ "world_lag_stddev_override" ] = p.world_lag_stddev_override;

  to_json( root[ "dbc" ], p.dbc );

  for ( auto i = PROFESSION_NONE; i < PROFESSION_MAX; ++i )
  {
    if ( p.profession[ i ] > 0 )
    {
      root[ "professions" ][ util::profession_type_string( i ) ] = p.profession[ i ];
    }
  }

  add_non_zero( root, "base_energy_regen_per_second", p.base_energy_regen_per_second );
  add_non_zero( root, "base_focus_regen_per_second", p.base_focus_regen_per_second );
  add_non_zero( root, "base_chi_regen_per_second", p.base_chi_regen_per_second );

  /* TODO: Not implemented reporting begins here

  to_json( root[ "base_stats" ], p.base );
  to_json( root[ "initial_stats" ], p.initial );
  to_json( root[ "current_stats" ], p.current );

  if ( p.main_hand_weapon -> slot != SLOT_NONE )
  {
    to_json( root[ "main_hand_weapon" ], p.main_hand_weapon );
  }

  if ( p.off_hand_weapon -> slot != SLOT_NONE )
  {
    to_json( root[ "off_hand_weapon" ], p.off_hand_weapon );
  }

  to_json( root[ "resources" ], p.resources );
  to_json( root[ "consumables" ], p.consumables );
  */

  if ( p.sim -> scaling -> has_scale_factors() )
  {
    scale_factors_to_json( root[ "scale_factors" ], p );
  }

  collected_data_to_json( root[ "collected_data" ], p );

  if ( p.sim -> report_details != 0 )
  {
    buffs_to_json( root[ "buffs" ], p );

    if ( p.proc_list.size() > 0 )
    {
      procs_to_json( root[ "procs" ], p );
    }

    if ( p.gain_list.size() > 0 )
    {
      gains_to_json( root[ "gains" ], p );
    }

    stats_to_json( root[ "stats" ], p );
  }

  gear_to_json( root[ "gear" ], p );

  JsonOutput custom = root[ "custom" ];
  p.output_json_report( custom );
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

void to_json( JsonOutput& arr, const raid_event_t& event )
{
  auto root = arr.add();
  root[ "name" ] = event.name();
  add_non_zero( root, "first", event.first );
  add_non_zero( root, "last", event.last );
  add_non_zero( root, "cooldown", event.cooldown );
  add_non_zero( root, "cooldown_stddev", event.cooldown_stddev );
  add_non_zero( root, "cooldown_min", event.cooldown_min );
  add_non_zero( root, "cooldown_max", event.cooldown_max );
  add_non_zero( root, "duration", event.duration );
  add_non_zero( root, "duration_stddev", event.duration_stddev );
  add_non_zero( root, "duration_min", event.duration_min );
  add_non_zero( root, "duration_max", event.duration_max );
  add_non_zero( root, "distance_min", event.distance_min );
  add_non_zero( root, "distance_max", event.distance_max );
  add_non_zero( root, "players_only", event.players_only );
  add_non_default( root, "player_chance", event.player_chance, 1.0 );
  add_non_default( root, "affected_role", event.affected_role, ROLE_NONE );
  add_non_zero( root, "saved_duration", event.saved_duration );
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

void iteration_data_to_json( JsonOutput root, const std::vector<iteration_data_entry_t>& entries )
{
  root.make_array();

  range::for_each( entries, [ &root ]( const iteration_data_entry_t& entry ) {
    auto json_entry = root.add();

    json_entry[ "metric" ] = entry.metric;
    json_entry[ "seed" ] = entry.seed;
    json_entry[ "target_health" ] = entry.target_health;
  } );
}

void to_json( JsonOutput root, const sim_t& sim )
{
  // Sim-scope options
  auto options_root = root[ "options" ];

  options_root[ "debug" ] = sim.debug;
  options_root[ "max_time" ] = sim.max_time.total_seconds();
  options_root[ "expected_iteration_time" ] = sim.expected_iteration_time.total_seconds();
  options_root[ "vary_combat_length" ] = sim.vary_combat_length;
  options_root[ "iterations" ] = sim.iterations;
  options_root[ "target_error" ] = sim.target_error;
  options_root[ "threads" ] = sim.threads;
  options_root[ "seed" ] = sim.seed;
  options_root[ "single_actor_batch" ] = sim.single_actor_batch;
  options_root[ "queue_lag" ] = sim.queue_lag;
  options_root[ "queue_lag_stddev" ] = sim.queue_lag_stddev;
  options_root[ "gcd_lag" ] = sim.gcd_lag;
  options_root[ "gcd_lag_stddev" ] = sim.gcd_lag_stddev;
  options_root[ "channel_lag" ] = sim.channel_lag;
  options_root[ "channel_lag_stddev" ] = sim.channel_lag_stddev;
  options_root[ "queue_gcd_reduction" ] = sim.queue_gcd_reduction;
  options_root[ "strict_gcd_queue" ] = sim.strict_gcd_queue;
  options_root[ "confidence" ] = sim.confidence;
  options_root[ "confidence_estimator" ] = sim.confidence_estimator;
  options_root[ "world_lag" ] =  sim.world_lag;
  options_root[ "world_lag_stddev" ] =  sim.world_lag_stddev;
  options_root[ "travel_variance" ] = sim.travel_variance;
  options_root[ "default_skill" ] = sim.default_skill;
  options_root[ "reaction_time" ] =  sim.reaction_time;
  options_root[ "regen_periodicity" ] =  sim.regen_periodicity;
  options_root[ "ignite_sampling_delta" ] =  sim.ignite_sampling_delta;
  options_root[ "fixed_time" ] = sim.fixed_time;
  options_root[ "optimize_expressions" ] = sim.optimize_expressions;
  options_root[ "optimal_raid" ] = sim.optimal_raid;
  options_root[ "log" ] = sim.log;
  options_root[ "debug_each" ] = sim.debug_each;
  options_root[ "auto_ready_trigger" ] = sim.auto_ready_trigger;
  options_root[ "stat_cache" ] = sim.stat_cache;
  options_root[ "max_aoe_enemies" ] = sim.max_aoe_enemies;
  options_root[ "show_etmi" ] = sim.show_etmi;
  options_root[ "tmi_window_global" ] = sim.tmi_window_global;
  options_root[ "tmi_bin_size" ] = sim.tmi_bin_size;
  options_root[ "enemy_death_pct" ] = sim.enemy_death_pct;
  options_root[ "challenge_mode" ] = sim.challenge_mode;
  options_root[ "timewalk" ] = sim.timewalk;
  options_root[ "pvp_crit" ] = sim.pvp_crit;
  options_root[ "rng" ] = sim.rng();
  options_root[ "deterministic" ] = sim.deterministic;
  options_root[ "average_range" ] = sim.average_range;
  options_root[ "average_gauss" ] = sim.average_gauss;
  options_root[ "fight_style" ] = sim.fight_style;
  options_root[ "default_aura_delay" ] = sim.default_aura_delay;
  options_root[ "default_aura_delay_stddev" ] = sim.default_aura_delay_stddev;

  to_json( options_root[ "dbc" ], sim.dbc );

  if ( sim.scaling -> calculate_scale_factors )
  {
    auto scaling_root = options_root[ "scaling" ];
    scaling_root[ "calculate_scale_factors" ] = sim.scaling -> calculate_scale_factors;
    scaling_root[ "normalize_scale_factors" ] = sim.scaling -> normalize_scale_factors;
    add_non_zero( scaling_root, "scale_only", sim.scaling -> scale_only_str );
    add_non_zero( scaling_root, "scale_over",  sim.scaling -> scale_over );
    add_non_zero( scaling_root, "scale_over_player", sim.scaling -> scale_over_player );
    add_non_default( scaling_root, "scale_delta_multiplier", sim.scaling -> scale_delta_multiplier, 1.0 );
    add_non_zero( scaling_root, "positive_scale_delta", sim.scaling -> positive_scale_delta );
    add_non_zero( scaling_root, "scale_lag", sim.scaling -> scale_lag );
    add_non_zero( scaling_root, "center_scale_delta", sim.scaling -> center_scale_delta );
  }

  // Overrides
  auto overrides = root[ "overrides" ];
  add_non_zero( overrides, "mortal_wounds", sim.overrides.mortal_wounds );
  add_non_zero( overrides, "bleeding", sim.overrides.bleeding );
  add_non_zero( overrides, "bloodlust", sim.overrides.bloodlust );
  if ( sim.overrides.bloodlust )
  {
    add_non_zero( overrides, "bloodlust_percent", sim.bloodlust_percent );
    add_non_zero( overrides, "bloodlust_time", sim.bloodlust_time );
  }

  if ( ! sim.overrides.target_health.empty() )
  {
    overrides[ "target_health" ] = sim.overrides.target_health;
  }

  // Players
  JsonOutput players_arr = root[ "players" ].make_array();

  range::for_each( sim.player_no_pet_list.data(), [ &players_arr ]( const player_t* p ) {
    to_json( players_arr, *p );
  } );

  if ( sim.report_details != 0 )
  {
    // Targets
    JsonOutput targets_arr = root[ "targets" ].make_array();

    range::for_each( sim.target_list.data(), [ &targets_arr ]( const player_t* p ) {
      to_json( targets_arr, *p );
    } );

    // Raid events
    if ( ! sim.raid_events.empty() )
    {
      auto arr = root[ "raid_events" ].make_array();

      range::for_each( sim.raid_events, [ &arr ]( const std::unique_ptr<raid_event_t>& event ) {
        to_json( arr, *event );
      } );
    }

    if ( sim.buff_list.size() > 0 )
    {
      JsonOutput buffs_arr = root[ "sim_auras" ].make_array();
      range::for_each( sim.buff_list, [ &buffs_arr ]( const buff_t* b ) {
        if ( b -> avg_start.mean() == 0 )
        {
          return;
        }
        to_json( buffs_arr.add(), b );
      } );
    }

    auto stats_root = root[ "statistics" ];
    stats_root[ "elapsed_cpu_seconds" ] = sim.elapsed_cpu;
    stats_root[ "elapsed_time_seconds" ] = sim.elapsed_time;
    stats_root[ "simulation_length" ] = sim.simulation_length;
    add_non_zero( stats_root, "raid_dps", sim.raid_dps );
    add_non_zero( stats_root, "raid_hps", sim.raid_hps );
    add_non_zero( stats_root, "raid_aps", sim.raid_aps );
    add_non_zero( stats_root, "total_dmg", sim.total_dmg );
    add_non_zero( stats_root, "total_heal", sim.total_heal );
    add_non_zero( stats_root, "total_absorb", sim.total_absorb );

    if ( sim.low_iteration_data.size() > 0 )
    {
      iteration_data_to_json( root[ "iteration_data" ][ "low" ], sim.low_iteration_data );
    }

    if ( sim.high_iteration_data.size() > 0 )
    {
      iteration_data_to_json( root[ "iteration_data" ][ "high" ], sim.high_iteration_data );
    }
  }
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

void print_json2_pretty( FILE* o, const sim_t& sim )
{
  Document doc;
  Value& v = doc;
  v.SetObject();

  JsonOutput root( doc, v );

  root[ "version" ] = SC_VERSION;
  root[ "ptr_enabled" ] = SC_USE_PTR;
  root[ "beta_enabled" ] = SC_BETA;
  root[ "build_date" ] = __DATE__;
  root[ "build_time" ] = __TIME__;
#if defined( SC_GIT_REV )
  root[ "git_revision" ] = SC_GIT_REV;
#endif

  to_json( root[ "sim" ], sim );

  if ( sim.error_list.size() > 0 )
  {
    root[ "notifications" ] = sim.error_list;
  }

  std::array<char, 65536> buffer;
  FileWriteStream b( o, buffer.data(), buffer.size() );
  PrettyWriter<FileWriteStream> writer( b );
  doc.Accept( writer );
}

void print_json_pretty( FILE* o, const sim_t& sim )
{
  js::sc_js_t root = get_root( sim );
  std::array<char, 1024> buffer;
  FileWriteStream b( o, buffer.data(), buffer.size() );
  PrettyWriter<FileWriteStream> writer( b );
  root.js_.Accept( writer );
}
}  // unnamed namespace

namespace report
{
void print_json( sim_t& sim )
{
  if ( ! sim.json_file_str.empty() )
  {
    // Setup file stream and open file
    io::cfile s( sim.json_file_str, "w" );
    if ( !s )
    {
      sim.errorf( "Failed to open JSON output file '%s'.",
                  sim.json_file_str.c_str() );
      return;
    }

    sim.errorf( "JSON v1 (json=) report is deprecated. Please use JSON v2 (json2=) report.");

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

  if ( ! sim.json2_file_str.empty() )
  {
    // Setup file stream and open file
    io::cfile s( sim.json2_file_str, "w" );
    if ( !s )
    {
      sim.errorf( "Failed to open JSON output file '%s'.",
                  sim.json2_file_str.c_str() );
      return;
    }

    // Print JSON report
    try
    {
      Timer t( "JSON-New report" );
      print_json2_pretty( s, sim );
    }
    catch ( const std::exception& e )
    {
      sim.errorf( "Failed to print JSON output! %s", e.what() );
    }
  }
}

}  // report
