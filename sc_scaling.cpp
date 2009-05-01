// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

namespace { // ANONYMOUS NAMESPACE ==========================================

// is_scaling_stat ===========================================================

static bool is_scaling_stat( sim_t* sim,
			     int    stat )
{
  for( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if( p -> quiet ) continue;
    if( p -> is_pet() ) continue;

    if( p -> scales_with[ stat ] ) return true;
  }
  
  return false;
}

// stat_may_cap ==============================================================

static bool stat_may_cap( int stat )
{
  if( stat == STAT_HIT_RATING ) return true;
  if( stat == STAT_EXPERTISE_RATING ) return true;
  return false;
}

} // ANONYMOUS NAMESPACE =====================================================

// ==========================================================================
// Scaling
// ==========================================================================

// scaling_t::scaling_t =====================================================

scaling_t::scaling_t( sim_t* s ) : 
  sim(s), 
  calculate_scale_factors(0), 
  center_scale_delta(0),
  scale_factor_noise(0.10)
{
  for( int i=ATTRIBUTE_NONE+1; i < ATTRIBUTE_MAX; i++ )
  {
    stats.attribute[ i ] = 250;
  }
  stats.armor_penetration_rating =  250;
  stats.attack_power             =  250;
  stats.crit_rating              =  250;
  stats.expertise_rating         = -150;
  stats.haste_rating             =  250;
  stats.hit_rating               = -200;
  stats.spell_power              =  250;
}

// scaling_t::analyze =======================================================

void scaling_t::analyze()
{
  if( ! calculate_scale_factors ) return;

  int num_players = sim -> players_by_name.size();
  if( num_players == 0 ) return;

  for( int i=0; i < STAT_MAX; i++ )
  {
    if( ! is_scaling_stat( sim, i ) ) continue;

    int scale_delta = stats.get_stat( i );
    if( scale_delta == 0 ) continue;

    fprintf( stdout, "\nGenerating scale factors for %s...\n", util_t::stat_type_string( i ) );
    fflush( stdout );

    bool center = center_scale_delta && ! stat_may_cap( i );

    sim_t* child_sim = new sim_t( sim );
    child_sim -> gear_delta.set_stat( i, scale_delta / ( center ? 2 : 1 ) );
    child_sim -> execute();

    sim_t* ref_sim = sim;
    if( center )
    {
      ref_sim = new sim_t( sim );
      ref_sim -> gear_delta.set_stat( i, -( scale_delta / 2 ) );
      ref_sim -> execute();
    }

    for( int j=0; j < num_players; j++ )
    {
      player_t* p = sim -> players_by_name[ j ];

      if( ! p -> scales_with[ i ] ) continue;

      player_t*   ref_p =   ref_sim -> find_player( p -> name() );
      player_t* child_p = child_sim -> find_player( p -> name() );

      double f = ( child_p -> dps - ref_p -> dps ) / scale_delta;

      if( f >= scale_factor_noise ) p -> scaling.set_stat( i, f );
    }

    if( ref_sim != sim ) delete ref_sim;
    delete child_sim;
  }

  for( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if( p -> quiet ) continue;

    chart_t::gear_weights_lootrank( p -> gear_weights_lootrank_link, p );
    chart_t::gear_weights_wowhead ( p -> gear_weights_wowhead_link,  p );
  }
}

// scaling_t::parse_option ==================================================

bool scaling_t::parse_option( const std::string& name,
                              const std::string& value )
{
  option_t options[] =
  {
    { "calculate_scale_factors",        OPT_INT, &( calculate_scale_factors              ) },
    { "center_scale_delta",             OPT_INT, &( center_scale_delta                   ) },
    { "scale_factor_noise",             OPT_FLT, &( scale_factor_noise                   ) },
    { "scale_strength",                 OPT_FLT, &( stats.attribute[ ATTR_STRENGTH  ]    ) },
    { "scale_agility",                  OPT_FLT, &( stats.attribute[ ATTR_AGILITY   ]    ) },
    { "scale_stamina",                  OPT_FLT, &( stats.attribute[ ATTR_STAMINA   ]    ) },
    { "scale_intellect",                OPT_FLT, &( stats.attribute[ ATTR_INTELLECT ]    ) },
    { "scale_spirit",                   OPT_FLT, &( stats.attribute[ ATTR_SPIRIT    ]    ) },
    { "scale_spell_power",              OPT_FLT, &( stats.spell_power                    ) },
    { "scale_attack_power",             OPT_FLT, &( stats.attack_power                   ) },
    { "scale_expertise_rating",         OPT_FLT, &( stats.expertise_rating               ) },
    { "scale_armor_penetration_rating", OPT_FLT, &( stats.armor_penetration_rating       ) },
    { "scale_hit_rating",               OPT_FLT, &( stats.hit_rating                     ) },
    { "scale_crit_rating",              OPT_FLT, &( stats.crit_rating                    ) },
    { "scale_haste_rating",             OPT_FLT, &( stats.haste_rating                   ) },
    { NULL, OPT_UNKNOWN }
  };

  if( name.empty() )
  {
    option_t::print( sim, options );
    return false;
  }

  return option_t::parse( sim, options, name, value );
}
