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
  action_t( ACTION_SPELL, n, p, r, s, t )
{
  may_miss = may_resist = true;
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
    { "time_to_die>",       OPT_FLT,    &min_time_to_die       },
    { "time_to_die<",       OPT_FLT,    &max_time_to_die       },
    { "health_percentage>", OPT_FLT,    &min_health_percentage },
    { "health_percentage<", OPT_FLT,    &max_health_percentage },
    { "bloodlust",          OPT_INT,    &bloodlust_active      },
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

    if( p -> buffs.swift_retribution || p -> buffs.improved_moonkin_aura ) 
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

  player_hit       = p -> composite_spell_hit();
  player_crit      = p -> composite_spell_crit();
  player_power     = p -> composite_spell_power( school );
  power_multiplier = p -> composite_spell_power_multiplier();

  if( p -> type != PLAYER_GUARDIAN )
  {
    if( p -> gear.chaotic_skyflare      ||
	p -> gear.relentless_earthstorm ) 
    {
      player_crit_multiplier *= 1.03;
    }

    if( p -> buffs.focus_magic ) player_crit += 0.03;

    if( p -> buffs.elemental_oath ||
	p -> buffs.moonkin_aura   )
    {
      player_crit += 0.05;
    }

    double best_buff = 0;
    if( p -> buffs.totem_of_wrath )
    {
      if( best_buff < p -> buffs.totem_of_wrath ) best_buff = p -> buffs.totem_of_wrath;
    }
    if( p -> buffs.flametongue_totem ) 
    {
      if( best_buff < p -> buffs.flametongue_totem ) best_buff = p -> buffs.flametongue_totem;
    }
    if( p -> buffs.demonic_pact )
    {
      if( best_buff < p -> buffs.demonic_pact ) best_buff = p -> buffs.demonic_pact;
    }
    if( p -> buffs.improved_divine_spirit )
    {
      if( best_buff < p -> buffs.improved_divine_spirit ) best_buff = p -> buffs.improved_divine_spirit;
    }
    player_power += best_buff;
  }

  if( sim -> debug ) report_t::log( sim, "spell_t::player_buff: %s hit=%.2f crit=%.2f power=%.2f penetration=%.0f", 
				    name(), player_hit, player_crit, player_power, player_penetration );
}

// spell_t::target_debuff =====================================================

void spell_t::target_debuff( int dmg_type )
{
  action_t::target_debuff( dmg_type );

  target_t* t = sim -> target;
 
  target_hit += std::max( t -> debuffs.improved_faerie_fire, t -> debuffs.misery ) * 0.01; 

  target_crit += std::max( t -> debuffs.winters_chill, t -> debuffs.improved_scorch ) * ( sim -> P309 ? 0.02 : 0.01 );
  t -> uptimes.winters_chill   -> update( t -> debuffs.winters_chill   != 0 );
  t -> uptimes.improved_scorch -> update( t -> debuffs.improved_scorch != 0 );

  if( sim -> debug ) 
    report_t::log( sim, "spell_t::target_debuff: %s multiplier=%.2f hit=%.2f crit=%.2f power=%.2f penetration=%.0f", 
		   name(), target_multiplier, target_hit, target_crit, target_power, target_penetration );
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
  direct_dmg = tick_dmg = 0;

  result = RESULT_NONE;

  if( ( result == RESULT_NONE ) && may_miss )
  {
    double miss_chance = level_based_miss_chance( player -> level, sim -> target -> level );

    miss_chance -= total_hit();

    if( sim -> roll( miss_chance ) )
    {
      result = RESULT_MISS;
    }
  }

  if( ( result == RESULT_NONE ) && may_resist )
  {
    if( binary || num_ticks )
    {
      if( sim -> roll( resistance() ) )
      {
	result = RESULT_RESIST;
      }
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

  for( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if( ! p -> sleeping) p -> raid_event( this );
  }

  if( sim -> debug ) report_t::log( sim, "%s result for %s is %s", player -> name(), name(), util_t::result_type_string( result ) );
}

