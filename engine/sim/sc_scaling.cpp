// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE ==========================================

// is_scaling_stat ==========================================================

bool is_scaling_stat( sim_t* sim,
                      int    stat )
{
  if ( ! sim -> scaling -> scale_only_str.empty() )
  {
    std::vector<std::string> stat_list = util::string_split( sim -> scaling -> scale_only_str, ",:;/|" );
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

    if ( p -> scales_with[ stat ] ) return true;
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
                                    const std::string& name,
                                    const std::string& value )
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
  bool normalized;

  compare_scale_factors( player_t* p, bool use_normalized ) :
    player( p ), normalized( use_normalized ) {}

  bool operator()( const stat_e& l, const stat_e& r ) const
  {
    if ( normalized )
      return player -> scaling_normalized.get_stat( l ) >  player -> scaling_normalized.get_stat( r );
    else
      return player -> scaling.get_stat( l ) >  player -> scaling.get_stat( r );
  }
};

} // UNNAMED NAMESPACE ====================================================

// ==========================================================================
// Scaling
// ==========================================================================

// scaling_t::scaling_t =====================================================

scaling_t::scaling_t( sim_t* s ) :
  sim( s ), baseline_sim( 0 ), ref_sim( 0 ), delta_sim( 0 ), ref_sim2( 0 ),
  delta_sim2( 0 ),
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
  scale_over(), scale_over_player()
{
  create_options();
}

// scaling_t::progress ======================================================

double scaling_t::progress( std::string& phase, std::string* detailed )
{
  if ( ! calculate_scale_factors ) return 1.0;

  if ( num_scaling_stats <= 0 ) return 0.0;

  if ( current_scaling_stat <= 0 )
  {
    phase = "Baseline";
    if ( ! baseline_sim ) return 0;
    sim -> detailed_progress( detailed, baseline_sim -> current_iteration , baseline_sim -> iterations );
    return baseline_sim -> current_iteration / static_cast<double>( baseline_sim -> iterations );
  }

  phase  = "Scaling - ";
  phase += util::stat_type_abbrev( current_scaling_stat );

  int completed_scaling_stats = ( num_scaling_stats - remaining_scaling_stats );

  double stat_progress = completed_scaling_stats / static_cast<double>( num_scaling_stats );

  sim -> detailed_progress( detailed, completed_scaling_stats, num_scaling_stats );

  double divisor = num_scaling_stats * 2.0;

  if ( ref_sim  ) stat_progress += ref_sim  -> current_iteration / ( ref_sim  -> iterations * divisor );
  if ( ref_sim2 ) stat_progress += ref_sim2 -> current_iteration / ( ref_sim2 -> iterations * divisor );

  if ( delta_sim  ) stat_progress += delta_sim  -> current_iteration / ( delta_sim  -> iterations * divisor );
  if ( delta_sim2 ) stat_progress += delta_sim2 -> current_iteration / ( delta_sim2 -> iterations * divisor );

  return stat_progress;
}

// scaling_t::init_deltas ===================================================

void scaling_t::init_deltas()
{
  assert ( scale_delta_multiplier != 0 );

  // Use block rating coefficient * 3.39 because it happens to result in nice round numbers both at level 85 and at level 90
  double default_delta = util::round( sim -> dbc.combat_rating( RATING_BLOCK, sim -> max_player_level ) * 0.0339 ) * scale_delta_multiplier;

  if ( stats.attribute[ ATTR_SPIRIT ] == 0 ) stats.attribute[ ATTR_SPIRIT ] = default_delta;

  for ( int i = ATTRIBUTE_NONE + 1; i < ATTRIBUTE_MAX; i++ )
  {
    if ( stats.attribute[ i ] == 0 ) stats.attribute[ i ] = default_delta;
  }

  if ( stats.spell_power == 0 ) stats.spell_power = default_delta;

  if ( stats.attack_power == 0 ) stats.attack_power = default_delta;

  if ( stats.expertise_rating == 0 )
  {
    stats.expertise_rating = default_delta;
    if ( ! positive_scale_delta ) stats.expertise_rating *= -1;
  }
  if ( stats.expertise_rating2 == 0 )
  {
    stats.expertise_rating2 = default_delta;
    if ( positive_scale_delta ) stats.expertise_rating2 *= -1;
  }

  if ( stats.hit_rating == 0 )
  {
    stats.hit_rating = default_delta;
    if ( ! positive_scale_delta ) stats.hit_rating *= -1;
  }
  if ( stats.hit_rating2 == 0 )
  {
    stats.hit_rating2 = default_delta;
    if ( positive_scale_delta ) stats.hit_rating2 *= -1;
  }

  if ( stats.crit_rating  == 0 ) stats.crit_rating  = default_delta;
  if ( stats.haste_rating == 0 ) stats.haste_rating = default_delta;
  if ( stats.mastery_rating == 0 ) stats.mastery_rating = default_delta;
  if ( stats.multistrike_rating == 0 ) stats.multistrike_rating = default_delta;
  if ( stats.versatility_rating == 0 ) stats.versatility_rating = default_delta;
  if ( stats.readiness_rating == 0 ) stats.readiness_rating = default_delta;

  // Defensive
  if ( stats.armor == 0 ) stats.armor = default_delta;
  if ( stats.bonus_armor == 0 ) stats.bonus_armor = default_delta;
  if ( stats.dodge_rating  == 0 ) stats.dodge_rating  = default_delta;
  if ( stats.parry_rating  == 0 ) stats.parry_rating  = default_delta;
  if ( stats.block_rating  == 0 ) stats.block_rating  = default_delta;


  if ( stats.weapon_dps            == 0 ) stats.weapon_dps            = default_delta * 0.3;
  if ( stats.weapon_offhand_dps    == 0 ) stats.weapon_offhand_dps    = default_delta * 0.3;
}

// scaling_t::analyze_stats =================================================

void scaling_t::analyze_stats()
{
  if ( ! calculate_scale_factors ) return;

  if ( sim -> players_by_name.empty() ) return; // No Players

  std::vector<stat_e> stats_to_scale;
  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
    if ( is_scaling_stat( sim, i ) && ( stats.get_stat( i ) != 0 ) )
      stats_to_scale.push_back( i );

  num_scaling_stats = remaining_scaling_stats = as<int>( stats_to_scale.size() );

  if ( ! num_scaling_stats ) return; // No Stats to scale

  baseline_sim = sim; // Take the current sim as baseline

  for ( size_t k = 0; k < stats_to_scale.size(); ++k )
  {
    if ( sim -> is_canceled() ) break;

    current_scaling_stat = stats_to_scale[ k ]; // Stat we're scaling over
    const stat_e& stat = current_scaling_stat;

    double scale_delta = stats.get_stat( stat );
    assert ( scale_delta );


    bool center = center_scale_delta && ! stat_may_cap( stat );

    ref_sim = baseline_sim;

    delta_sim = new sim_t( sim );
    delta_sim -> scaling -> scale_stat = stat;
    delta_sim -> scaling -> scale_value = +scale_delta / ( center ? 2 : 1 );
    if ( sim -> report_progress )
    {
      std::stringstream  stat_name; stat_name.width( 12 );
      stat_name << std::left << std::string( util::stat_type_abbrev( stat ) ) + ":";
      delta_sim -> sim_phase_str = "Generating " + stat_name.str();
      //util::fprintf( stdout, "\nGenerating scale factors for %s...\n", util::stat_type_string( stat ) );
      //fflush( stdout );
    }
    delta_sim -> execute();

    if ( center )
    {
      ref_sim = new sim_t( sim );
      if ( sim -> report_progress )
      {
        std::stringstream  stat_name; stat_name.width( 8 );
        stat_name << std::left << std::string( util::stat_type_abbrev( stat ) ) + ":";
        ref_sim -> sim_phase_str = "Generating ref " + stat_name.str();
      }
      ref_sim -> scaling -> scale_stat = stat;
      ref_sim -> scaling -> scale_value = center ? -( scale_delta / 2 ) : 0;
      ref_sim -> execute();
    }

    for ( size_t j = 0; j < sim -> players_by_name.size(); j++ )
    {
      player_t* p = sim -> players_by_name[ j ];

      if ( ! p -> scales_with[ stat ] ) continue;

      player_t*   ref_p =   ref_sim -> find_player( p -> name() );
      player_t* delta_p = delta_sim -> find_player( p -> name() );
      assert( ref_p && "Reference Player not found" );
      assert( delta_p && "Delta player not found" );

      double divisor = scale_delta;

      if ( delta_p -> invert_scaling )
        divisor = -divisor;

      if ( divisor < 0.0 ) divisor += ref_p -> over_cap[ stat ];

      double delta_score = delta_p -> scales_over().value;
      double   ref_score = ref_p -> scales_over().value;

      double delta_error = delta_p -> scales_over().stddev * delta_sim -> confidence_estimator;
      double   ref_error = ref_p -> scales_over().stddev * ref_sim -> confidence_estimator;

      p -> scaling_delta_dps.set_stat( stat, delta_score );
      
      double score = ( delta_score - ref_score ) / divisor;
      double error = delta_error * delta_error + ref_error * ref_error;
      
      if ( error > 0  )
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
        p -> scaling_compare_error.set_stat( stat, error );
      else
        p -> scaling_compare_error.set_stat( stat, delta_error / divisor );

      p -> scaling.set_stat( stat, score );
      p -> scaling_error.set_stat( stat, error );
    }

    if ( debug_scale_factors )
    {
      io::cfile report_f( sim -> output_file_str, "a" );
      if ( report_f )
      {
        ref_sim -> out_std.printf( "\nref_sim report for %s...\n", util::stat_type_string( stat ) );
        report::print_text( report_f,   ref_sim, true );
        delta_sim -> out_std.printf( "\ndelta_sim report for %s...\n", util::stat_type_string( stat ) );
        report::print_text( report_f, delta_sim, true );
      }
    }

    if ( ref_sim != baseline_sim && ref_sim != sim )
    {
      delete ref_sim;
      ref_sim = 0;
    }
    delete delta_sim;  delta_sim  = 0;

    remaining_scaling_stats--;
  }

  if ( baseline_sim != sim ) delete baseline_sim;
  baseline_sim = 0;
}

/* Creates scale factors for stats_t objects
 *
 */

void scaling_t::analyze_ability_stats( stat_e stat, double delta, player_t* p, player_t* ref_p, player_t* delta_p )
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
    s -> scaling.set_stat( stat, score );
    double x = p -> sim -> confidence_estimator;
    double error = fabs( sqrt ( delta_s -> portion_aps.mean_std_dev * x * delta_s -> portion_aps.mean_std_dev * x + ref_s -> portion_aps.mean_std_dev * x * ref_s -> portion_aps.mean_std_dev * x ) / delta );
    s -> scaling_error.set_stat( stat, error );
  }
}

// scaling_t::analyze_lag ===================================================

void scaling_t::analyze_lag()
{
  if ( ! calculate_scale_factors || ! scale_lag ) return;

  if ( sim -> players_by_name.empty() ) return;

  if ( sim -> report_progress )
  {
    util::fprintf( stdout, "\nGenerating scale factors for lag...\n" );
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

    double delta_score = delta_p -> scales_over().value;
    double   ref_score = ref_p -> scales_over().value;

    double delta_error = delta_p -> scales_over().stddev * delta_sim -> confidence_estimator;
    double ref_error = ref_p -> scales_over().stddev * ref_sim -> confidence_estimator;
    double error = sqrt ( delta_error * delta_error + ref_error * ref_error );

    double score = ( delta_score - ref_score ) / divisor;

    error = fabs( error / divisor );
    p -> scaling_lag = score;
    p -> scaling_lag_error = error;
  }

  if ( ref_sim != sim ) delete ref_sim;
  delete delta_sim;
  delta_sim = ref_sim = 0;
}


// scaling_t::normalize =====================================================

void scaling_t::normalize()
{
  if ( num_scaling_stats <= 0 ) return;

  for ( size_t i = 0; i < sim -> player_list.size(); ++i )
  {
    player_t* p = sim -> player_list[ i ];
    if ( p -> quiet ) continue;

    double divisor = p -> scaling.get_stat( p -> normalize_by() );

    if ( divisor == 0 ) continue;

    // hack to deal with weirdness in TMI calculations - always normalize using negative divisor
    if ( util::str_compare_ci( scale_over, "tmi" ) || util::str_compare_ci( scale_over, "theck_meloree_index" ) )
      divisor = - std::abs( divisor );

    for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
    {
      if ( ! p -> scales_with[ j ] ) continue;

      p -> scaling_normalized.set_stat( j, p -> scaling.get_stat( j ) / divisor );
    }
  }
}

// scaling_t::analyze =======================================================

void scaling_t::analyze()
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

    // Sort scaling results
    for ( stat_e j = STAT_NONE; j < STAT_MAX; j++ )
    {
      if ( p -> scales_with[ j ] )
      {
        double s = p -> scaling.get_stat( j );

        if ( s ) p -> scaling_stats.push_back( j );
      }
    }
    // more hack to deal with TMI weirdness, this just determines sorting order, not what gets displayed on the chart
    bool use_normalized = p -> scaling_normalized.get_stat( p -> normalize_by() ) > 0 || util::str_compare_ci( scale_over, "tmi" ) || util::str_compare_ci( scale_over, "theck_meloree_index" ) || util::str_compare_ci( scale_over, "etmi" );
    range::sort( p -> scaling_stats, compare_scale_factors( p, use_normalized ) );
  }
}

// scaling_t::create_options ================================================

void scaling_t::create_options()
{
  option_t scaling_options[] =
  {
    // @option_doc loc=global/scale_factors title="Scale Factors"
    opt_bool( "calculate_scale_factors", calculate_scale_factors ),
    opt_func( "normalize_scale_factors", parse_normalize_scale_factors ),
    opt_bool( "debug_scale_factors", debug_scale_factors ),
    opt_bool( "center_scale_delta", center_scale_delta ),
    opt_float( "scale_delta_multiplier", scale_delta_multiplier ), // multiplies all default scale deltas
    opt_bool( "positive_scale_delta", positive_scale_delta ),
    opt_bool( "scale_lag", scale_lag ),
    opt_float( "scale_factor_noise", scale_factor_noise ),
    opt_float( "scale_strength",  stats.attribute[ ATTR_STRENGTH  ] ),
    opt_float( "scale_agility",   stats.attribute[ ATTR_AGILITY   ] ),
    opt_float( "scale_stamina",   stats.attribute[ ATTR_STAMINA   ] ),
    opt_float( "scale_intellect", stats.attribute[ ATTR_INTELLECT ] ),
    opt_float( "scale_spirit",    stats.attribute[ ATTR_SPIRIT    ] ),
    opt_float( "scale_spell_power", stats.spell_power ),
    opt_float( "scale_attack_power", stats.attack_power ),
    opt_float( "scale_crit_rating", stats.crit_rating ),
    opt_float( "scale_haste_rating", stats.haste_rating ),
    opt_float( "scale_mastery_rating", stats.mastery_rating ),
    opt_float( "scale_multistrike_rating", stats.multistrike_rating ),
    opt_float( "scale_readiness_rating", stats.readiness_rating ),
    opt_float( "scale_versatility_rating", stats.versatility_rating ),
    opt_float( "scale_weapon_dps", stats.weapon_dps ),
    opt_float( "scale_offhand_weapon_dps", stats.weapon_offhand_dps ),
    opt_string( "scale_only", scale_only_str ),
    opt_string( "scale_over", scale_over ),
    opt_string( "scale_over_player", scale_over_player ),
    opt_null()
  };

  option_t::copy( sim -> options, scaling_options );
}

// scaling_t::has_scale_factors =============================================

bool scaling_t::has_scale_factors()
{
  if ( ! calculate_scale_factors ) return false;

  for ( stat_e i = STAT_NONE; i < STAT_MAX; i++ )
  {
    if ( stats.get_stat( i ) != 0 )
    {
      return true;
    }
  }

  return false;
}

