// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

namespace   // ANONYMOUS NAMESPACE ==========================================
{

// is_scaling_stat ===========================================================

static bool is_scaling_stat( sim_t* sim,
                             int    stat )
{
  if ( ! sim -> scaling -> scale_only_str.empty() )
  {
    std::vector<std::string> stat_list;
    int num_stats = util_t::string_split( stat_list, sim -> scaling -> scale_only_str, ",:;/|" );
    bool found = false;
    for( int i=0; i < num_stats && ! found; i++ )
    {
      found = ( util_t::parse_stat_type( stat_list[ i ] ) == stat );
    }
    if( ! found ) return false;
  }

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( p -> quiet ) continue;
    if ( p -> is_pet() ) continue;

    if ( p -> scales_with[ stat ] ) return true;
  }

  return false;
}

// stat_may_cap ==============================================================

static bool stat_may_cap( int stat )
{
  if ( stat == STAT_HIT_RATING ) return true;
  if ( stat == STAT_EXPERTISE_RATING ) return true;
  return false;
}

} // ANONYMOUS NAMESPACE =====================================================

// ==========================================================================
// Scaling
// ==========================================================================

// scaling_t::scaling_t =====================================================

scaling_t::scaling_t( sim_t* s ) :
    sim( s ),
    scale_stat( STAT_NONE ),
    scale_value( 0 ),
    calculate_scale_factors( 0 ),
    center_scale_delta( 0 ),
    scale_lag( 0 ),
    scale_factor_noise( 0.05 ),
    normalize_scale_factors( 0 ),
    smooth_scale_factors( 1 ),
    debug_scale_factors( 0 )
{}

// scaling_t::init_deltas ===================================================

void scaling_t::init_deltas()
{
  for ( int i=ATTRIBUTE_NONE+1; i < ATTRIBUTE_MAX; i++ )
  {
    if ( stats.attribute[ i ] == 0 ) stats.attribute[ i ] = smooth_scale_factors ? 100 : 250;
  }

  if ( stats.spell_power == 0 ) stats.spell_power = smooth_scale_factors ? 100 : 250;

  if ( stats.attack_power             == 0 ) stats.attack_power             =  smooth_scale_factors ?  100 :  250;
  if ( stats.armor_penetration_rating == 0 ) stats.armor_penetration_rating =  smooth_scale_factors ?  100 :  250;
  if ( stats.expertise_rating         == 0 ) stats.expertise_rating         =  smooth_scale_factors ?  -75 : -150;

  if ( stats.hit_rating   == 0 ) stats.hit_rating   = smooth_scale_factors ? -100 : -200;
  if ( stats.crit_rating  == 0 ) stats.crit_rating  = smooth_scale_factors ?  100 :  250;
  if ( stats.haste_rating == 0 ) stats.haste_rating = smooth_scale_factors ?  250 :  250;

  if ( stats.weapon_dps == 0 ) stats.weapon_dps = smooth_scale_factors ? 25 : 50;
}

// scaling_t::analyze_stats =================================================

void scaling_t::analyze_stats()
{
  if ( ! calculate_scale_factors ) return;

  int num_players = sim -> players_by_name.size();
  if ( num_players == 0 ) return;

  sim_t* baseline_sim = sim;
  if ( smooth_scale_factors )
  {
    util_t::fprintf( stdout, "\nGenerating smooth baseline...\n" );
    fflush( stdout );

    baseline_sim = new sim_t( sim );
    baseline_sim -> scaling -> scale_stat = STAT_MAX-1;
    baseline_sim -> execute();
  }

  for ( int i=0; i < STAT_MAX; i++ )
  {
    if ( ! is_scaling_stat( sim, i ) ) continue;

    int scale_delta = ( int ) stats.get_stat( i );
    if ( scale_delta == 0 ) continue;

    util_t::fprintf( stdout, "\nGenerating scale factors for %s...\n", util_t::stat_type_string( i ) );
    fflush( stdout );

    bool center = center_scale_delta && ! stat_may_cap( i );

    sim_t* child_sim = new sim_t( sim );
    child_sim -> scaling -> scale_stat = i;
    child_sim -> scaling -> scale_value = +scale_delta / ( center ? 2 : 1 );
    child_sim -> execute();

    sim_t* ref_sim = ( i == STAT_HASTE_RATING ) ? sim : baseline_sim;
    if ( center )
    {
      ref_sim = new sim_t( sim );
      ref_sim -> scaling -> scale_stat = i;
      ref_sim -> scaling -> scale_value = center ? -( scale_delta / 2 ) : 0;
      ref_sim -> execute();
    }

    for ( int j=0; j < num_players; j++ )
    {
      player_t* p = sim -> players_by_name[ j ];

      if ( ! p -> scales_with[ i ] ) continue;

      player_t*   ref_p =   ref_sim -> find_player( p -> name() );
      player_t* child_p = child_sim -> find_player( p -> name() );

      double f = ( child_p -> dps - ref_p -> dps ) / scale_delta;

      if ( f >= scale_factor_noise ) p -> scaling.set_stat( i, f );
    }

    if ( debug_scale_factors )
    {
      report_t::print_text( sim -> output_file,   ref_sim, true );
      report_t::print_text( sim -> output_file, child_sim, true );
    }

    if ( ref_sim != baseline_sim && ref_sim != sim ) delete ref_sim;
    delete child_sim;
  }

  if ( baseline_sim != sim ) delete baseline_sim;
}

// scaling_t::analyze_lag ===================================================

void scaling_t::analyze_lag()
{
  if ( ! calculate_scale_factors || ! scale_lag ) return;

  int num_players = sim -> players_by_name.size();
  if ( num_players == 0 ) return;

  util_t::fprintf( stdout, "\nGenerating scale factors for lag...\n" );
  fflush( stdout );

  sim_t* ref_sim = new sim_t( sim );
  ref_sim -> scaling -> scale_stat = STAT_MAX;
  ref_sim -> execute();

  sim_t* child_sim = new sim_t( sim );
  child_sim ->   queue_lag += 0.100;
  child_sim ->     gcd_lag += 0.100;
  child_sim -> channel_lag += 0.100;
  child_sim -> scaling -> scale_stat = STAT_MAX;
  child_sim -> execute();

  for ( int i=0; i < num_players; i++ )
  {
    player_t*       p =       sim -> players_by_name[ i ];
    player_t*   ref_p =   ref_sim -> find_player( p -> name() );
    player_t* child_p = child_sim -> find_player( p -> name() );

    // Calculate DPS difference per millisecond of lag
    double f = ( ref_p -> dps - child_p -> dps ) / ( ( child_sim -> queue_lag - sim -> queue_lag ) * 1000 );

    if ( f >= scale_factor_noise ) p -> scaling_lag = f;
  }

  if ( ref_sim != sim ) delete ref_sim;
  delete child_sim;
}

// scaling_t::analyze_gear_weights ==========================================

void scaling_t::analyze_gear_weights()
{
  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( p -> quiet ) continue;

    chart_t::gear_weights_lootrank( p -> gear_weights_lootrank_link, p );
    chart_t::gear_weights_wowhead ( p -> gear_weights_wowhead_link,  p );
    chart_t::gear_weights_pawn    ( p -> gear_weights_pawn_string,   p );
  }
}

// scaling_t::normalize =====================================================

void scaling_t::normalize()
{
  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( p -> quiet ) continue;

    double sp = p -> scaling.get_stat( STAT_SPELL_POWER );
    double ap = p -> scaling.get_stat( STAT_ATTACK_POWER );

    p -> normalized_to = ( sp > ap ) ? STAT_SPELL_POWER : STAT_ATTACK_POWER;

    double divisor = p -> scaling.get_stat( p -> normalized_to );

    if ( divisor == 0 ) continue;

    for ( int i=0; i < STAT_MAX; i++ )
    {
      if ( p -> scales_with[ i ] == 0 ) continue;

      p -> normalized_scaling.set_stat( i, p -> scaling.get_stat( i ) / divisor );
    }

  }
}

// scaling_t::analyze =======================================================

void scaling_t::analyze()
{
  init_deltas();
  analyze_stats();
  analyze_lag();
  analyze_gear_weights();
  normalize();
}

// scaling_t::get_options ===================================================

int scaling_t::get_options( std::vector<option_t>& options )
{
  option_t scaling_options[] =
  {
    // @option_doc loc=global/scale_factors title="Scale Factors"
    { "calculate_scale_factors",        OPT_BOOL,   &( calculate_scale_factors              ) },
    { "smooth_scale_factors",           OPT_BOOL,   &( smooth_scale_factors                 ) },
    { "normalize_scale_factors",        OPT_BOOL,   &( normalize_scale_factors              ) },
    { "debug_scale_factors",            OPT_BOOL,   &( debug_scale_factors                  ) },
    { "center_scale_delta",             OPT_BOOL,   &( center_scale_delta                   ) },
    { "scale_lag",                      OPT_BOOL,   &( scale_lag                            ) },
    { "scale_factor_noise",             OPT_FLT,    &( scale_factor_noise                   ) },
    { "scale_strength",                 OPT_FLT,    &( stats.attribute[ ATTR_STRENGTH  ]    ) },
    { "scale_agility",                  OPT_FLT,    &( stats.attribute[ ATTR_AGILITY   ]    ) },
    { "scale_stamina",                  OPT_FLT,    &( stats.attribute[ ATTR_STAMINA   ]    ) },
    { "scale_intellect",                OPT_FLT,    &( stats.attribute[ ATTR_INTELLECT ]    ) },
    { "scale_spirit",                   OPT_FLT,    &( stats.attribute[ ATTR_SPIRIT    ]    ) },
    { "scale_spell_power",              OPT_FLT,    &( stats.spell_power                    ) },
    { "scale_attack_power",             OPT_FLT,    &( stats.attack_power                   ) },
    { "scale_expertise_rating",         OPT_FLT,    &( stats.expertise_rating               ) },
    { "scale_armor_penetration_rating", OPT_FLT,    &( stats.armor_penetration_rating       ) },
    { "scale_hit_rating",               OPT_FLT,    &( stats.hit_rating                     ) },
    { "scale_crit_rating",              OPT_FLT,    &( stats.crit_rating                    ) },
    { "scale_haste_rating",             OPT_FLT,    &( stats.haste_rating                   ) },
    { "scale_weapon_dps",               OPT_FLT,    &( stats.weapon_dps                     ) },
    { "scale_only",                     OPT_STRING, &( scale_only_str                       ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, scaling_options );

  return options.size();
}
