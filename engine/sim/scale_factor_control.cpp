// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "scale_factor_control.hpp"
#include "sc_sim.hpp"
#include "dbc/dbc.hpp"
#include "player/sc_player.hpp"
#include "player/player_scaling.hpp"
#include "player/stats.hpp"
#include "report/reports.hpp"
#include "util/util.hpp"

#include <iostream>
#include <memory>

namespace { // UNNAMED NAMESPACE ==========================================

// is_scaling_stat ==========================================================

bool is_scaling_stat( sim_t* sim,
                      int    stat )
{
  if ( ! sim -> scaling -> scale_only_str.empty() )
  {
    auto stat_list = util::string_split<util::string_view>( sim -> scaling -> scale_only_str, ",:;/|" );
    bool found = false;
    for ( size_t i = 0; i < stat_list.size() && ! found; i++ )
    {
      found = ( util::parse_stat_type( stat_list[ i ] ) == stat );
    }
    if ( ! found ) return false;
  }

  for ( size_t i = 0; i < sim -> player_no_pet_list.size(); ++i )
  {
    player_t* p = sim -> player_no_pet_list[ i ];
    if ( p -> quiet ) continue;
    if ( ! p -> scale_player ) continue;

    if ( p -> scaling -> scales_with[ stat ] ) return true;
  }

  return false;
}

// stat_may_cap =============================================================

bool stat_may_cap( int stat )
{
  if ( stat == STAT_HIT_RATING ) return true;
  if ( stat == STAT_EXPERTISE_RATING ) return true;
  if ( stat == STAT_SPIRIT ) return true;
  return false;
}

// parse_normalize_scale_factors ============================================

bool parse_normalize_scale_factors( sim_t* sim,
                                    util::string_view name,
                                    util::string_view value )
{
  if ( name != "normalize_scale_factors" ) return false;

  if ( value == "1" )
  {
    sim -> scaling -> normalize_scale_factors = 1;
  }
  else
  {
    if ( ( sim -> normalized_stat = util::parse_stat_type( value ) ) == STAT_NONE )
    {
      return false;
    }

    sim -> scaling -> normalize_scale_factors = 1;
  }

  return true;
}

struct compare_scale_factors
{
  player_t* player;
  scale_metric_e scale_metric;
  bool normalized;

  compare_scale_factors( player_t* p, scale_metric_e sm, bool use_normalized ) :
    player( p ), scale_metric( sm ), normalized( use_normalized ) {}

  bool operator()( const stat_e& l, const stat_e& r ) const
  {
    double rv;
    double lv;

    if ( normalized )
    {
      lv = player -> scaling -> scaling_normalized[ scale_metric ].get_stat( l );
      rv = player -> scaling -> scaling_normalized[ scale_metric ].get_stat( r );
    }
    else
    {
      lv = player -> scaling -> scaling[ scale_metric ].get_stat( l );
      rv = player -> scaling -> scaling[ scale_metric ].get_stat( r );
    }

    if ( lv == rv )
    {
      return l < r;
    }

    return lv > rv;
  }
};

} // UNNAMED NAMESPACE ====================================================

// ==========================================================================
// Scaling
// ==========================================================================

// scaling_t::scaling_t =====================================================

scale_factor_control_t::scale_factor_control_t( sim_t* s ) :
  sim( s ), baseline_sim( nullptr ), ref_sim( nullptr ), delta_sim( nullptr ), ref_sim2( nullptr ),
  delta_sim2( nullptr ),
  scale_stat( STAT_NONE ),
  scale_value( 0 ),
  scale_delta_multiplier( 1.0 ),
  calculate_scale_factors( 0 ),
  center_scale_delta( 0 ),
  positive_scale_delta( 0 ),
  scale_lag( 0 ),
  scale_factor_noise( 0.10 ),
  normalize_scale_factors( 0 ),
  debug_scale_factors( 0 ),
  current_scaling_stat( STAT_NONE ),
  num_scaling_stats( 0 ),
  remaining_scaling_stats( 0 ),
  scale_over(), scaling_metric( SCALE_METRIC_DPS ), scale_over_player(),
  stats(new gear_stats_t())
{
  create_options();
}

// scaling_t::progress ======================================================

double scale_factor_control_t::progress( std::string& phase, std::string* detailed )
{
  AUTO_LOCK( mutex );

  if ( ! calculate_scale_factors ) return 1.0;

  if ( num_scaling_stats <= 0 ) return 0.0;

  if ( current_scaling_stat <= 0 )
  {
    phase = "Baseline";
    if ( ! baseline_sim ) return 0;
    return baseline_sim -> progress(detailed ).pct();
  }

  phase  = "Scaling - ";
  phase += util::stat_type_abbrev( current_scaling_stat );

  int completed_scaling_stats = ( num_scaling_stats - remaining_scaling_stats );

  double stat_progress = completed_scaling_stats / static_cast<double>( num_scaling_stats );

  sim -> detailed_progress( detailed, completed_scaling_stats, num_scaling_stats );

  double divisor = num_scaling_stats * 2.0;

  if ( ref_sim  ) stat_progress += divisor * ref_sim  -> progress().pct();
  if ( ref_sim2 ) stat_progress += divisor * ref_sim2 -> progress().pct();

  if ( delta_sim  ) stat_progress += divisor * delta_sim  -> progress().pct();
  if ( delta_sim2 ) stat_progress += divisor * delta_sim2 -> progress().pct();

  return stat_progress;
}

// scaling_t::init_deltas ===================================================

void scale_factor_control_t::init_deltas()
{
  assert ( scale_delta_multiplier != 0 );

  // Use haste rating coefficient * 3.5 to determine default delta
  double default_delta = util::round( sim -> dbc->combat_rating( RATING_MELEE_HASTE, sim -> max_player_level ) * 0.035 ) * scale_delta_multiplier;

  if ( stats->attribute[ ATTR_SPIRIT ] == 0 ) stats->attribute[ ATTR_SPIRIT ] = default_delta;

  for ( int i = ATTRIBUTE_NONE + 1; i < ATTRIBUTE_MAX; i++ )
  {
    if ( stats->attribute[ i ] == 0 ) stats->attribute[ i ] = default_delta;
  }

  if ( stats->spell_power == 0 ) stats->spell_power = default_delta;
  if ( stats->attack_power == 0 ) stats->attack_power = default_delta;
  if ( stats->crit_rating  == 0 ) stats->crit_rating  = default_delta;
  if ( stats->haste_rating == 0 ) stats->haste_rating = default_delta;
  if ( stats->mastery_rating == 0 ) stats->mastery_rating = default_delta;
  if ( stats->versatility_rating == 0 ) stats->versatility_rating = default_delta;

  // Defensive
  if ( stats->armor == 0 ) stats->armor = default_delta;
  if ( stats->bonus_armor == 0 ) stats->bonus_armor = default_delta;
  if ( stats->dodge_rating  == 0 ) stats->dodge_rating  = default_delta;
  if ( stats->parry_rating  == 0 ) stats->parry_rating  = default_delta;
  if ( stats->block_rating  == 0 ) stats->block_rating  = default_delta;

  if ( stats->weapon_dps            == 0 ) stats->weapon_dps            = default_delta;
  if ( stats->weapon_offhand_dps    == 0 ) stats->weapon_offhand_dps    = default_delta;

  if ( stats->leech_rating          == 0 ) stats->leech_rating          = default_delta;
  if ( stats->avoidance_rating      == 0 ) stats->avoidance_rating      = default_delta;
  if ( stats->speed_rating          == 0 ) stats->speed_rating          = default_delta;
}

// scaling_t::analyze_stats =================================================

void scale_factor_control_t::analyze_stats()
{
  if ( ! calculate_scale_factors ) return;

  if ( sim -> players_by_name.empty() ) return; // No Players

  std::vector<stat_e> stats_to_scale;
  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    if ( is_scaling_stat( sim, i ) && ( stats->get_stat( i ) != 0 ) )
      stats_to_scale.push_back( i );

  mutex.lock();
  num_scaling_stats = remaining_scaling_stats = as<int>( stats_to_scale.size() );
  mutex.unlock();

  if ( ! num_scaling_stats ) return; // No Stats to scale

  mutex.lock();
  baseline_sim = sim; // Take the current sim as baseline
  mutex.unlock();

  for ( size_t k = 0; k < stats_to_scale.size(); ++k )
  {
    if ( sim -> is_canceled() ) break;

    current_scaling_stat = stats_to_scale[ k ]; // Stat we're scaling over
    const stat_e& stat = current_scaling_stat;

    double scale_delta = stats->get_stat( stat );
    assert ( scale_delta );

    bool center = center_scale_delta && ! stat_may_cap( stat );

    mutex.lock();
    ref_sim = baseline_sim;
    delta_sim = new sim_t( sim );
    mutex.unlock();

    delta_sim -> progress_bar.set_base( util::stat_type_abbrev( stat ) );

    delta_sim -> scaling -> scale_stat = stat;
    delta_sim -> scaling -> scale_value = +scale_delta / ( center ? 2 : 1 );
    delta_sim -> execute();

    if ( center )
    {
      mutex.lock();
      ref_sim = new sim_t( sim );
      mutex.unlock();

      ref_sim -> progress_bar.set_base( std::string( "Ref " ) + util::stat_type_abbrev( stat ) );

      ref_sim -> scaling -> scale_stat = stat;
      ref_sim -> scaling -> scale_value = center ? -( scale_delta / 2 ) : 0;
      ref_sim -> execute();
    }

    for ( size_t j = 0; j < sim -> players_by_name.size(); j++ )
    {
      player_t* p = sim -> players_by_name[ j ];

      if ( ! p -> scaling -> scales_with[ stat ] ) continue;

      player_t*   ref_p =   ref_sim -> find_player( p -> name() );
      player_t* delta_p = delta_sim -> find_player( p -> name() );
      assert( ref_p && "Reference Player not found" );
      assert( delta_p && "Delta player not found" );

      double divisor = scale_delta;

      if ( delta_p -> invert_scaling )
        divisor = -divisor;

      if ( divisor < 0.0 ) divisor += ref_p -> scaling -> over_cap[ stat ];

      for ( scale_metric_e sm = SCALE_METRIC_NONE; sm < SCALE_METRIC_MAX; sm++ )
      {

        double delta_score = delta_p -> scaling_for_metric( sm ).value;
        double   ref_score = ref_p -> scaling_for_metric( sm ).value;

        double delta_error = delta_p -> scaling_for_metric( sm ).stddev * delta_sim -> confidence_estimator;
        double   ref_error = ref_p -> scaling_for_metric( sm ).stddev * ref_sim -> confidence_estimator;

        // TODO: this is the only place in the entire code base where scaling_delta_dps shows up, 
        // apart from declaration in simulationcraft.hpp line 4535. Possible to remove?
        p -> scaling -> scaling_delta_dps[ sm ].set_stat( stat, delta_score );

        double score = ( delta_score - ref_score ) / divisor;
        double error = delta_error * delta_error + ref_error * ref_error;

        if ( error > 0 )
          error = sqrt( error );

        error = fabs( error / divisor );

        if ( fabs( divisor ) < 1.0 ) // For things like Weapon Speed, show the gain per 0.1 speed gain rather than every 1.0.
        {
          score /= 10.0;
          error /= 10.0;
          delta_error /= 10.0;
        }

        analyze_ability_stats( stat, divisor, p, ref_p, delta_p );

        if ( center )
          p -> scaling -> scaling_compare_error[ sm ].set_stat( stat, error );
        else
          p -> scaling -> scaling_compare_error[ sm ].set_stat( stat, delta_error / divisor );

        p -> scaling -> scaling[ sm ].set_stat( stat, score );
        p -> scaling -> scaling_error[ sm ].set_stat( stat, error );
      }
    }

    if ( debug_scale_factors )
    {
      std::cout << "\nref_sim report for '" << util::stat_type_string( stat ) << "'..." << std::endl;
      report::print_text( ref_sim, true );
      std::cout << "\ndelta_sim report for '" << util::stat_type_string( stat ) << "'..." << std::endl;
      report::print_text( delta_sim, true );
    }

    mutex.lock();
    if ( ref_sim != baseline_sim && ref_sim != sim )
    {
      delete ref_sim;
      ref_sim = nullptr;
    }
    delete delta_sim;  
    delta_sim  = nullptr;
    remaining_scaling_stats--;
    mutex.unlock();
  }

  if ( baseline_sim != sim ) delete baseline_sim;
  baseline_sim = nullptr;
}

/* Creates scale factors for stats_t objects
 *
 */

void scale_factor_control_t::analyze_ability_stats( stat_e stat, double delta, player_t* p, player_t* ref_p, player_t* delta_p )
{
  if ( p -> sim -> statistics_level < 3 )
    return;

  for ( size_t i = 0; i < p -> stats_list.size(); ++i )
  {
    stats_t* s = p -> stats_list[ i ];
    stats_t* ref_s = ref_p -> find_stats( s -> name_str );
    stats_t* delta_s = delta_p -> find_stats( s -> name_str );
    assert( ref_s && delta_s );
    double score = ( delta_s -> portion_aps.mean() - ref_s -> portion_aps.mean() ) / delta;
    if ( !s -> scaling ) {
      s -> scaling = std::make_unique<stats_t::stats_scaling_t>( );
    }
    s -> scaling -> value.set_stat( stat, score );
    double x = p -> sim -> confidence_estimator;
    double error = fabs( sqrt ( delta_s -> portion_aps.mean_std_dev * x * delta_s -> portion_aps.mean_std_dev * x + ref_s -> portion_aps.mean_std_dev * x * ref_s -> portion_aps.mean_std_dev * x ) / delta );
    s -> scaling -> error.set_stat( stat, error );
  }
}

// scaling_t::analyze_lag ===================================================

void scale_factor_control_t::analyze_lag()
{
  if ( ! calculate_scale_factors || ! scale_lag ) return;

  if ( sim -> players_by_name.empty() ) return;

  if ( sim -> report_progress )
  {
    fmt::print( "\nGenerating scale factors for lag...\n" );
    fflush( stdout );
  }

  if ( center_scale_delta )
  {
    ref_sim = new sim_t( sim );
    ref_sim -> scaling -> scale_stat = STAT_MAX;
    ref_sim -> execute();
  }
  else
  {
    ref_sim = sim;
    ref_sim -> scaling -> scale_stat = STAT_MAX;
  }

  delta_sim = new sim_t( sim );
  delta_sim ->     gcd_lag += timespan_t::from_seconds( 0.100 );
  delta_sim -> channel_lag += timespan_t::from_seconds( 0.200 );
  delta_sim -> scaling -> scale_stat = STAT_MAX;
  delta_sim -> execute();

  for ( size_t i = 0; i < sim -> players_by_name.size(); i++ )
  {
    player_t*       p =       sim -> players_by_name[ i ];
    player_t*   ref_p =   ref_sim -> find_player( p -> name() );
    player_t* delta_p = delta_sim -> find_player( p -> name() );

    // Calculate DPS difference per millisecond of lag
    double divisor = ( double ) ( delta_sim -> gcd_lag - ref_sim -> gcd_lag ).total_millis();
    
    for ( scale_metric_e sm = SCALE_METRIC_NONE; sm < SCALE_METRIC_MAX; sm++ )
    {
      double delta_score = delta_p -> scaling_for_metric( sm ).value;
      double   ref_score = ref_p -> scaling_for_metric( sm ).value;

      double delta_error = delta_p -> scaling_for_metric( sm ).stddev * delta_sim -> confidence_estimator;
      double ref_error = ref_p -> scaling_for_metric( sm ).stddev * ref_sim -> confidence_estimator;
      double error = sqrt( delta_error * delta_error + ref_error * ref_error );

      double score = ( delta_score - ref_score ) / divisor;

      error = fabs( error / divisor );
      p -> scaling -> scaling_lag[ sm ]       = score;
      p -> scaling -> scaling_lag_error[ sm ] = error;
    }
  }

  if ( ref_sim != sim ) delete ref_sim;
  delete delta_sim;
  delta_sim = ref_sim = nullptr;
}

// scaling_t::normalize =====================================================

void scale_factor_control_t::normalize()
{
  if ( num_scaling_stats <= 0 ) return;

  for ( size_t i = 0; i < sim -> player_list.size(); ++i )
  {
    player_t* p = sim -> player_list[ i ];
    if ( p -> quiet ) continue;
    
    for ( scale_metric_e sm = SCALE_METRIC_NONE; sm < SCALE_METRIC_MAX; sm++ )
    {
      double divisor = p -> scaling -> scaling[ sm ].get_stat( p -> normalize_by() );

      divisor *= ( 1 / sim -> scaling_normalized );

      if ( divisor == 0 ) continue;

      // hack to deal with weirdness in TMI calculations - always normalize using negative divisor
      if ( sm == SCALE_METRIC_TMI || sm == SCALE_METRIC_ETMI )
        divisor = -std::abs( divisor );

      for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
      {
        if ( !p -> scaling -> scales_with[ j ] ) continue;

        p -> scaling -> scaling_normalized[ sm ].set_stat( j,
            p -> scaling -> scaling[ sm ].get_stat( j ) / divisor );
      }
    }
  }
}

// scaling_t::analyze =======================================================

void scale_factor_control_t::analyze()
{
  if ( sim -> is_canceled() ) return;
  init_deltas();
  analyze_stats();
  analyze_lag();
  normalize();

  for ( size_t i = 0; i < sim -> player_list.size(); ++i )
  {
    player_t* p = sim -> player_list[ i ];
    if ( p -> quiet ) continue;
    if ( p -> scaling == nullptr ) continue;

    for ( scale_metric_e sm = SCALE_METRIC_NONE; sm < SCALE_METRIC_MAX; sm++ )
    {
      // Sort scaling results
      for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
      {
        if ( p -> scaling -> scales_with[ j ] )
        {
          double s = p -> scaling -> scaling[ sm ].get_stat( j );

          if ( s ) p -> scaling -> scaling_stats[ sm ].push_back( j );
        }
      }
      // more hack to deal with TMI weirdness, this just determines sorting order, not what gets displayed on the chart
      bool use_normalized = p -> scaling -> scaling_normalized[ sm ].get_stat( p -> normalize_by() ) > 0 || sm == SCALE_METRIC_TMI || sm == SCALE_METRIC_ETMI;
      range::sort( p -> scaling -> scaling_stats[ sm ], compare_scale_factors( p, sm, use_normalized ) );
    }
  }
}

// scaling_t::create_options ================================================

void scale_factor_control_t::create_options()
{
  sim->add_option(opt_bool("calculate_scale_factors", calculate_scale_factors));
  sim->add_option(opt_func("normalize_scale_factors", parse_normalize_scale_factors));
  sim->add_option(opt_bool("debug_scale_factors", debug_scale_factors));
  sim->add_option(opt_bool("center_scale_delta", center_scale_delta));
  sim->add_option(opt_float("scale_delta_multiplier", scale_delta_multiplier)); // multiplies all default scale deltas
  sim->add_option(opt_bool("positive_scale_delta", positive_scale_delta));
  sim->add_option(opt_bool("scale_lag", scale_lag));
  sim->add_option(opt_float("scale_factor_noise", scale_factor_noise));
  sim->add_option(opt_float("scale_strength", stats->attribute[ATTR_STRENGTH]));
  sim->add_option(opt_float("scale_agility", stats->attribute[ATTR_AGILITY]));
  sim->add_option(opt_float("scale_stamina", stats->attribute[ATTR_STAMINA]));
  sim->add_option(opt_float("scale_intellect", stats->attribute[ATTR_INTELLECT]));
  sim->add_option(opt_float("scale_spirit", stats->attribute[ATTR_SPIRIT]));
  sim->add_option(opt_float("scale_spell_power", stats->spell_power));
  sim->add_option(opt_float("scale_attack_power", stats->attack_power));
  sim->add_option(opt_float("scale_crit_rating", stats->crit_rating));
  sim->add_option(opt_float("scale_haste_rating", stats->haste_rating));
  sim->add_option(opt_float("scale_mastery_rating", stats->mastery_rating));
  sim->add_option(opt_float("scale_versatility_rating", stats->versatility_rating));
  sim->add_option(opt_float("scale_weapon_dps", stats->weapon_dps));
  sim->add_option(opt_float("scale_speed_rating", stats->speed_rating));
  sim->add_option(opt_float("scale_offhand_weapon_dps", stats->weapon_offhand_dps));
  sim->add_option(opt_string("scale_only", scale_only_str));
  sim->add_option(opt_string("scale_over", scale_over));
  sim->add_option(opt_string("scale_over_player", scale_over_player));
}

// scaling_t::has_scale_factors =============================================

bool scale_factor_control_t::has_scale_factors()
{
  if ( ! calculate_scale_factors ) return false;

  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    if ( stats->get_stat( i ) != 0 )
    {
      return true;
    }
  }

  return false;
}

