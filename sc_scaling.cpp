// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

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

// scaling_t::analyze_attributes ============================================

void scaling_t::analyze_attributes()
{
  int num_players = sim -> players_by_name.size();
  if( num_players == 0 ) return;

  for( int i=0; i < ATTRIBUTE_MAX; i++ )
  {
    if( stats.attribute[ i ] != 0 )
    {
      fprintf( stdout, "\nGenerating scale factors for %s...\n", util_t::attribute_type_string( i ) );
      fflush( stdout );

      sim_t* child_sim = new sim_t( sim );
      child_sim -> gear_delta.attribute[ i ] = stats.attribute[ i ] / ( center_scale_delta ? 2 : 1 );
      child_sim -> execute();

      sim_t* ref_sim = sim;
      if( center_scale_delta )
      {
        ref_sim = new sim_t( sim );
        ref_sim -> gear_delta.attribute[ i ] = -( stats.attribute[ i ] / 2 );
        ref_sim -> execute();
      }

      for( int j=0; j < num_players; j++ )
      {
	      player_t*       p =       sim -> players_by_name[ j ];
	      player_t*   ref_p =   ref_sim -> find_player( p -> name() );
	      player_t* child_p = child_sim -> find_player( p -> name() );
	
	      double f = ( child_p -> dps - ref_p -> dps ) / stats.attribute[ i ];

	      if( f >= scale_factor_noise ) p -> scaling.attribute[ i ] = f;
      }

      if( ref_sim != sim ) delete ref_sim;
      delete child_sim;
    }
  }
}

// scaling_t::analyze_spell_power ===========================================

void scaling_t::analyze_spell_power()
{
  int num_players = sim -> players_by_name.size();
  if( num_players == 0 ) return;

  if( stats.spell_power != 0 )
  {
    fprintf( stdout, "\nGenerating scale factors for spell power...\n" );
    fflush( stdout );

    sim_t* child_sim = new sim_t( sim );
    child_sim -> gear_delta.spell_power = stats.spell_power / ( center_scale_delta ? 2 : 1 );
    child_sim -> execute();

    sim_t* ref_sim = sim;
    if( center_scale_delta )
    {
      ref_sim = new sim_t( sim );
      ref_sim -> gear_delta.spell_power = -( stats.spell_power / 2 );
      ref_sim -> execute();
    }

    for( int j=0; j < num_players; j++ )
    {
      player_t*       p =       sim -> players_by_name[ j ];
      player_t*   ref_p =   ref_sim -> find_player( p -> name() );
      player_t* child_p = child_sim -> find_player( p -> name() );

      double f = ( child_p -> dps - ref_p -> dps ) / stats.spell_power;

      if( f >= scale_factor_noise ) p -> scaling.spell_power = f;
    }

    if( ref_sim != sim ) delete ref_sim;
    delete child_sim;
  }
}

// scaling_t::analyze_attack_power ==========================================

void scaling_t::analyze_attack_power()
{
  int num_players = sim -> players_by_name.size();
  if( num_players == 0 ) return;

  if( stats.attack_power != 0 )
  {
    fprintf( stdout, "\nGenerating scale factors for attack power...\n" );
    fflush( stdout );

    sim_t* child_sim = new sim_t( sim );
    child_sim -> gear_delta.attack_power = stats.attack_power / ( center_scale_delta ? 2 : 1 );
    child_sim -> execute();

    sim_t* ref_sim = sim;
    if( center_scale_delta )
    {
      ref_sim = new sim_t( sim );
      ref_sim -> gear_delta.attack_power = -( stats.attack_power / 2 );
      ref_sim -> execute();
    }

    for( int j=0; j < num_players; j++ )
    {
      player_t*       p =       sim -> players_by_name[ j ];
      player_t*   ref_p =   ref_sim -> find_player( p -> name() );
      player_t* child_p = child_sim -> find_player( p -> name() );

      double f = ( child_p -> dps - ref_p -> dps ) / stats.attack_power;

      if( f >= scale_factor_noise ) p -> scaling.attack_power = f;
    }

    if( ref_sim != sim ) delete ref_sim;
    delete child_sim;
  }
}

// scaling_t::analyze_expertise =============================================

void scaling_t::analyze_expertise()
{
  int num_players = sim -> players_by_name.size();
  if( num_players == 0 ) return;

  if( stats.expertise_rating != 0 )
  {
    fprintf( stdout, "\nGenerating scale factors for expertise rating...\n" );
    fflush( stdout );

    sim_t* child_sim = new sim_t( sim );
    child_sim -> gear_delta.expertise_rating = stats.expertise_rating;
    child_sim -> execute();

    sim_t* ref_sim = sim;
    // No "center_scale_delta" support for Expertise or Hit

    for( int j=0; j < num_players; j++ )
    {
      player_t*       p =       sim -> players_by_name[ j ];
      player_t*   ref_p =   ref_sim -> find_player( p -> name() );
      player_t* child_p = child_sim -> find_player( p -> name() );
	
      double f = ( child_p -> dps - ref_p -> dps ) / stats.expertise_rating;

      if( f >= scale_factor_noise ) p -> scaling.expertise_rating = f;
    }

    if( ref_sim != sim ) delete sim;
    delete child_sim;
  }
}

// scaling_t::analyze_armor_penetration =====================================

void scaling_t::analyze_armor_penetration()
{
  int num_players = sim -> players_by_name.size();
  if( num_players == 0 ) return;

  if( stats.armor_penetration_rating != 0 )
  {
    fprintf( stdout, "\nGenerating scale factors for armor penetration rating...\n" );
    fflush( stdout );

    sim_t* child_sim = new sim_t( sim );
    child_sim -> gear_delta.armor_penetration_rating = stats.armor_penetration_rating / ( center_scale_delta ? 2 : 1 );
    child_sim -> execute();

    sim_t* ref_sim = sim;
    if( center_scale_delta )
    {
      ref_sim = new sim_t( sim );
      ref_sim -> gear_delta.armor_penetration_rating = -( stats.armor_penetration_rating / 2 );
      ref_sim -> execute();
    }

    for( int j=0; j < num_players; j++ )
    {
      player_t*       p =       sim -> players_by_name[ j ];
      player_t*   ref_p =   ref_sim -> find_player( p -> name() );
      player_t* child_p = child_sim -> find_player( p -> name() );
	
      double f = ( child_p -> dps - ref_p -> dps ) / stats.armor_penetration_rating;

      if( f >= scale_factor_noise ) p -> scaling.armor_penetration_rating = f;
    }

    if( ref_sim != sim ) delete ref_sim;
    delete child_sim;
  }
}

// scaling_t::analyze_hit ===================================================

void scaling_t::analyze_hit()
{
  int num_players = sim -> players_by_name.size();
  if( num_players == 0 ) return;

  if( stats.hit_rating != 0 )
  {
    fprintf( stdout, "\nGenerating scale factors for hit rating...\n" );
    fflush( stdout );

    sim_t* child_sim = new sim_t( sim );
    child_sim -> gear_delta.hit_rating = stats.hit_rating;
    child_sim -> execute();

    sim_t* ref_sim = sim;
    // No "center_scale_delta" support for Expertise or Hit

    for( int j=0; j < num_players; j++ )
    {
      player_t*       p =       sim -> players_by_name[ j ];
      player_t*   ref_p =   ref_sim -> find_player( p -> name() );
      player_t* child_p = child_sim -> find_player( p -> name() );
	
      double f = ( child_p -> dps - ref_p -> dps ) / stats.hit_rating;

      if( f >= scale_factor_noise ) p -> scaling.hit_rating = f;
    }

    if( ref_sim != sim ) delete sim;
    delete child_sim;
  }

}

// scaling_t::analyze_crit ==================================================

void scaling_t::analyze_crit()
{
  int num_players = sim -> players_by_name.size();
  if( num_players == 0 ) return;

  if( stats.crit_rating != 0 )
  {
    fprintf( stdout, "\nGenerating scale factors for crit rating...\n" );
    fflush( stdout );

    sim_t* child_sim = new sim_t( sim );
    child_sim -> gear_delta.crit_rating = stats.crit_rating / ( center_scale_delta ? 2 : 1 );
    child_sim -> execute();

    sim_t* ref_sim = sim;
    if( center_scale_delta )
    {
      ref_sim = new sim_t( sim );
      ref_sim -> gear_delta.crit_rating = -( stats.crit_rating / 2 );
      ref_sim -> execute();
    }

    for( int j=0; j < num_players; j++ )
    {
      player_t*       p =       sim -> players_by_name[ j ];
      player_t*   ref_p =   ref_sim -> find_player( p -> name() );
      player_t* child_p = child_sim -> find_player( p -> name() );
	
      double f = ( child_p -> dps - ref_p -> dps ) / stats.crit_rating;

      if( f >= scale_factor_noise ) p -> scaling.crit_rating = f;
    }

    if( ref_sim != sim ) delete ref_sim;
    delete child_sim;
  }
}

// scaling_t::analyze_haste =================================================

void scaling_t::analyze_haste()
{
  int num_players = sim -> players_by_name.size();
  if( num_players == 0 ) return;

  if( stats.haste_rating != 0 )
  {
    fprintf( stdout, "\nGenerating scale factors for haste rating...\n" );
    fflush( stdout );

    sim_t* child_sim = new sim_t( sim );
    child_sim -> gear_delta.haste_rating = stats.haste_rating / ( center_scale_delta ? 2 : 1 );
    child_sim -> execute();

    sim_t* ref_sim = sim;
    if( center_scale_delta )
    {
      ref_sim = new sim_t( sim );
      ref_sim -> gear_delta.haste_rating = -( stats.haste_rating / 2 );
      ref_sim -> execute();
    }

    for( int j=0; j < num_players; j++ )
    {
      player_t*       p =       sim -> players_by_name[ j ];
      player_t*   ref_p =   ref_sim -> find_player( p -> name() );
      player_t* child_p = child_sim -> find_player( p -> name() );
	
      double f = ( child_p -> dps - ref_p -> dps ) / stats.haste_rating;

      if( f >= scale_factor_noise ) p -> scaling.haste_rating = f;
    }

    if( ref_sim != sim ) delete ref_sim;
    delete child_sim;
  }
}

// scaling_t::analyze =======================================================

void scaling_t::analyze()
{
  if( ! calculate_scale_factors ) return;

  analyze_attributes();
  analyze_spell_power();
  analyze_attack_power();
  analyze_expertise();
  analyze_armor_penetration();
  analyze_hit();
  analyze_crit();
  analyze_haste();
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
    { "scale_strength",                 OPT_INT, &( stats.attribute[ ATTR_STRENGTH  ]    ) },
    { "scale_agility",                  OPT_INT, &( stats.attribute[ ATTR_AGILITY   ]    ) },
    { "scale_stamina",                  OPT_INT, &( stats.attribute[ ATTR_STAMINA   ]    ) },
    { "scale_intellect",                OPT_INT, &( stats.attribute[ ATTR_INTELLECT ]    ) },
    { "scale_spirit",                   OPT_INT, &( stats.attribute[ ATTR_SPIRIT    ]    ) },
    { "scale_spell_power",              OPT_INT, &( stats.spell_power                    ) },
    { "scale_attack_power",             OPT_INT, &( stats.attack_power                   ) },
    { "scale_expertise_rating",         OPT_INT, &( stats.expertise_rating               ) },
    { "scale_armor_penetration_rating", OPT_INT, &( stats.armor_penetration_rating       ) },
    { "scale_hit_rating",               OPT_INT, &( stats.hit_rating                     ) },
    { "scale_crit_rating",              OPT_INT, &( stats.crit_rating                    ) },
    { "scale_haste_rating",             OPT_INT, &( stats.haste_rating                   ) },
    { NULL, OPT_UNKNOWN }
  };

  if( name.empty() )
  {
    option_t::print( sim, options );
    return false;
  }

  return option_t::parse( sim, options, name, value );
}
