// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Spell
// ==========================================================================

// spell_t::spell_t ==========================================================

spell_t::spell_t( const char* n, player_t* p, int r, int s, int t ) :
  action_t( ACTION_SPELL, n, p, r, s, t, true )
{
  may_miss = may_resist = true;
  base_spell_power_multiplier = 1.0;
  base_crit_bonus = 0.5;
  trigger_gcd = p -> base_gcd;
  min_gcd = 1.0;
}   

// spell_t::parse_options ==================================================

void spell_t::parse_options( option_t*          options,
			     const std::string& options_str )
{
  option_t base_options[] =
  {
    { "rank",               OPT_INT,    &rank_index            },
    { "sync",               OPT_STRING, &sync_str              },
    { "time>",              OPT_FLT,    &min_current_time      },
    { "time<",              OPT_FLT,    &max_current_time      },
    { "time_to_die>",       OPT_FLT,    &min_time_to_die       },
    { "time_to_die<",       OPT_FLT,    &max_time_to_die       },
    { "health_percentage>", OPT_FLT,    &min_health_percentage },
    { "health_percentage<", OPT_FLT,    &max_health_percentage },
    { "bloodlust",          OPT_INT,    &bloodlust_active      },
    { "travel_speed",       OPT_FLT,    &travel_speed          },
    { NULL }
  };
  std::vector<option_t> merged_options;
  action_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

// spell_t::haste ============================================================

double spell_t::haste()
{
  player_t* p = player;
  double h = p -> spell_haste;

  if( p -> type != PLAYER_GUARDIAN )
  {
    if(      p -> buffs.bloodlust      ) h *= 1.0 / ( 1.0 + 0.30 );
    else if( p -> buffs.power_infusion ) h *= 1.0 / ( 1.0 + 0.20 );

    if( sim -> auras.swift_retribution || sim -> auras.improved_moonkin ) 
    {
      h *= 1.0 / ( 1.0 + 0.03 );
    }

    if( p -> buffs.wrath_of_air ) 
    {
      h *= 1.0 / ( 1.0 + 0.05 );
    }
  }

  return h;
}

// spell_t::gcd =============================================================

double spell_t::gcd()
{
  double t = trigger_gcd;
  if( t == 0 ) return 0;

  t *= haste();
  if( t < min_gcd ) t = min_gcd;

  return t;
}

// spell_t::execute_time =====================================================

double spell_t::execute_time()
{
  if( base_execute_time == 0 ) return 0;

  double t = base_execute_time - player -> buffs.cast_time_reduction;
  if( t < 0 ) t = 0;

  t *= haste();

  return t;
}

// spell_t::tick_time ========================================================

double spell_t::tick_time()
{
  double t = base_tick_time;
  if( channeled ) t *= haste();
  return t;
}

// spell_t::player_buff ======================================================

void spell_t::player_buff()
{
  action_t::player_buff();

  player_t* p = player;

  player_hit  = p -> composite_spell_hit();
  player_crit = p -> composite_spell_crit();

  if( p -> unique_gear -> chaotic_skyflare      ||
      p -> unique_gear -> relentless_earthstorm ) 
  {
    player_crit_multiplier *= 1.03;
  }

  if( sim -> debug ) report_t::log( sim, "spell_t::player_buff: %s hit=%.2f crit=%.2f crit_multiplier=%.2f", 
				    name(), player_hit, player_crit, player_crit_multiplier );
}

// spell_t::target_debuff =====================================================

void spell_t::target_debuff( int dmg_type )
{
  action_t::target_debuff( dmg_type );

  target_t* t = sim -> target;
 
  target_hit += std::max( t -> debuffs.improved_faerie_fire, t -> debuffs.misery ) * 0.01; 
  
  int crit_debuff = std::max( t -> debuffs.winters_chill, t -> debuffs.improved_scorch );
  crit_debuff = std::max( crit_debuff, t -> debuffs.improved_shadow_bolt );
  target_crit += crit_debuff * ( sim -> P309 ? 0.02 : 0.01 );
  
  t -> uptimes.winters_chill        -> update( t -> debuffs.winters_chill        != 0 );
  t -> uptimes.improved_scorch      -> update( t -> debuffs.improved_scorch      != 0 );
  t -> uptimes.improved_shadow_bolt -> update( t -> debuffs.improved_shadow_bolt != 0 );

  if( sim -> debug ) 
    report_t::log( sim, "spell_t::target_debuff: %s multiplier=%.2f hit=%.2f crit=%.2f", 
		   name(), target_multiplier, target_hit, target_crit );
}
   
// spell_t::level_based_miss_chance ==========================================

double spell_t::level_based_miss_chance( int player, 
					 int target )
{
  int delta_level = target - player;
  double miss=0;

  if( delta_level > 2 )
  {
    miss = 0.17 + ( delta_level - 3 ) * 0.11;
  }
  else
  {
    miss = 0.04 + delta_level * 0.01;
  }

  if( miss < 0.00 ) miss = 0.00;
  if( miss > 0.99 ) miss = 0.99;

  return miss;
}

// spell_t::caclulate_result ================================================

void spell_t::calculate_result()
{
  direct_dmg = 0;

  result = RESULT_NONE;

  if( ! harmful ) return;

  if( ( result == RESULT_NONE ) && may_miss )
  {
    double miss_chance = level_based_miss_chance( player -> level, sim -> target -> level );

    miss_chance -= total_hit();

    if( sim -> roll( miss_chance ) )
    {
      result = RESULT_MISS;
    }
  }

  if( ( result == RESULT_NONE ) && may_resist && binary )
  {
    if( sim -> roll( resistance() ) )
    {
      result = RESULT_RESIST;
    }
  }

  if( result == RESULT_NONE )
  {
    result = RESULT_HIT;

    if( may_crit )
    {
      if( sim -> roll( total_crit() ) )
      {
	result = RESULT_CRIT;
      }
    }
  }

  if( sim -> debug ) report_t::log( sim, "%s result for %s is %s", player -> name(), name(), util_t::result_type_string( result ) );
}

// spell_t::execute =========================================================

void spell_t::execute()
{
  action_t::execute();

  action_callback_t::trigger( player -> spell_result_callbacks[ result ], this );
}
