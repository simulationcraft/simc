// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#pragma once

#include "config.hpp"
#include "util/sample_data.hpp"
#include "util/timeline.hpp"
#include "util/concurrency.hpp"
#include "sc_enums.hpp"

struct action_t;
struct buff_t;
struct cooldown_t;
struct player_t;

/* Contains any data collected during / at the end of combat
 * Mostly statistical data collection, represented as sample data containers
 */
struct player_collected_data_t
{
  extended_sample_data_t fight_length;
  extended_sample_data_t waiting_time, pooling_time, executed_foreground_actions;

  // DMG
  extended_sample_data_t dmg;
  extended_sample_data_t compound_dmg;
  extended_sample_data_t prioritydps;
  extended_sample_data_t dps;
  extended_sample_data_t dpse;
  extended_sample_data_t dtps;
  extended_sample_data_t dmg_taken;
  sc_timeline_t timeline_dmg;
  sc_timeline_t timeline_dmg_taken;
  // Heal
  extended_sample_data_t heal;
  extended_sample_data_t compound_heal;
  extended_sample_data_t hps;
  extended_sample_data_t hpse;
  extended_sample_data_t htps;
  extended_sample_data_t heal_taken;
  sc_timeline_t timeline_healing_taken;
  // Absorb
  extended_sample_data_t absorb;
  extended_sample_data_t compound_absorb;
  extended_sample_data_t aps;
  extended_sample_data_t atps;
  extended_sample_data_t absorb_taken;
  // Tank
  extended_sample_data_t deaths;
  extended_sample_data_t theck_meloree_index;
  extended_sample_data_t effective_theck_meloree_index;
  extended_sample_data_t max_spike_amount;

  // Metric used to end simulations early
  extended_sample_data_t target_metric;
  mutex_t target_metric_mutex;

  std::vector<simple_sample_data_t> resource_lost, resource_gained, resource_overflowed;
  struct resource_timeline_t
  {
    resource_e type;
    sc_timeline_t timeline;

    resource_timeline_t( resource_e t = RESOURCE_NONE ) : type( t ) {}
  };
  // Druid requires 4 resource timelines health/mana/energy/rage
  std::vector<resource_timeline_t> resource_timelines;

  std::vector<simple_sample_data_t> combat_start_resource;
  std::vector<simple_sample_data_with_min_max_t> combat_end_resource;

  struct stat_timeline_t
  {
    stat_e type;
    sc_timeline_t timeline;

    stat_timeline_t( stat_e t = STAT_NONE ) : type( t ) {}
  };

  std::vector<stat_timeline_t> stat_timelines;

  // hooked up in resource timeline collection event
  struct health_changes_timeline_t
  {
    double previous_loss_level, previous_gain_level;
    sc_timeline_t timeline; // keeps only data per iteration
    sc_timeline_t timeline_normalized; // same as above, but normalized to current player health
    sc_timeline_t merged_timeline;
    bool collect; // whether we collect all this or not.
    health_changes_timeline_t() : previous_loss_level( 0.0 ), previous_gain_level( 0.0 ), collect( false ) {}

    void set_bin_size( double bin )
    {
      timeline.set_bin_size( bin );
      timeline_normalized.set_bin_size( bin );
      merged_timeline.set_bin_size( bin );
    }

    double get_bin_size() const
    {
      if ( timeline.bin_size() != timeline_normalized.bin_size() || timeline.bin_size() != merged_timeline.bin_size() )
      {
        assert( false );
        return 0.0;
      }
      else
        return timeline.bin_size();
    }
  };

  health_changes_timeline_t health_changes;     //records all health changes
  health_changes_timeline_t health_changes_tmi; //records only health changes due to damage and self-healng/self-absorb

  // Total number of iterations for the actor. Needed for single_actor_batch if target_error is
  // used.
  int total_iterations;

  struct action_sequence_data_t : noncopyable
  {
    template <typename T>
    struct record_t {
      T* object;
      int value;
      timespan_t time_value;

      record_t( T* o, int v, timespan_t tv )
        : object( o ), value( v ), time_value( tv ) {}
    };

    const action_t* action;
    const player_t* target;
    const timespan_t time;
    timespan_t wait_time;
    std::vector< record_t<buff_t> > buff_list;
    std::vector< record_t<cooldown_t> > cooldown_list;
    std::vector< std::pair<player_t*, std::vector< record_t<buff_t> > > > target_list;
    std::array<double, RESOURCE_MAX> resource_snapshot;
    std::array<double, RESOURCE_MAX> resource_max_snapshot;

    action_sequence_data_t( const action_t* a, const player_t* t, timespan_t ts, timespan_t wait, const player_t* p );

    action_sequence_data_t( timespan_t ts, timespan_t wait, const player_t* p )
      : action_sequence_data_t( nullptr, nullptr, ts, wait, p ) {}
    action_sequence_data_t( const action_t* a, const player_t* t, timespan_t ts, const player_t* p )
      : action_sequence_data_t( a, t, ts, timespan_t::zero(), p ) {}
  };
  std::vector<action_sequence_data_t> action_sequence;
  std::vector<action_sequence_data_t> action_sequence_precombat;

  // Buffed snapshot_stats (for reporting)
  struct buffed_stats_t
  {
    std::array< double, ATTRIBUTE_MAX > attribute;
    std::array< double, RESOURCE_MAX > resource;

    double spell_power, spell_hit, spell_crit_chance, manareg_per_second;
    double attack_power,  attack_hit,  mh_attack_expertise,  oh_attack_expertise, attack_crit_chance;
    double armor, miss, crit, dodge, parry, block, bonus_armor;
    double spell_haste, spell_speed, attack_haste, attack_speed;
    double mastery_value;
    double damage_versatility, heal_versatility, mitigation_versatility;
    double leech, run_speed, avoidance;
    double corruption, corruption_resistance;
  } buffed_stats_snapshot;

  player_collected_data_t( const player_t* player );
  void reserve_memory( const player_t& );
  void merge( const player_t& );
  void analyze( const player_t& );
  void collect_data( const player_t& );
  void print_tmi_debug_csv( const sc_timeline_t* nma, const std::vector<double>& weighted_value, const player_t& p );
  double calculate_tmi( const health_changes_timeline_t& tl, int window, double f_length, const player_t& p );
  double calculate_max_spike_damage( const health_changes_timeline_t& tl, int window );
  std::ostream& data_str( std::ostream& s ) const;

};
