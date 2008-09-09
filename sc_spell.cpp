// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

// ==========================================================================
// Spell
// ==========================================================================

// spell_t::spell_t ==========================================================

spell_t::spell_t( const char* n, player_t* p, int8_t r, int8_t s, int8_t t ) :
  action_t( ACTION_SPELL, n, p, r, s, t )
{
  may_miss = may_resist = true;
  base_crit_bonus = 0.5;
  trigger_gcd = p -> base_gcd;
  min_gcd = 1.0;
}   

// spell_t::haste ============================================================

double spell_t::haste()
{
  double h = player -> haste;
  if( player -> buffs.bloodlust                    ) h *= 1.0 / ( 1.0 + 0.30 );
  else if ( player -> buffs.power_infusion         ) h *= 1.0 / ( 1.0 + 0.20 );
  if( player -> buffs.swift_retribution ||
      player -> buffs.improved_moonkin_aura        ) h *= 1.0 / ( 1.0 + 0.03 );
  if( player -> buffs.totem_of_wrath_haste         ) h *= 1.0 / ( 1.0 + 0.01 );
  if( sim_t::WotLK && player -> buffs.wrath_of_air ) h *= 1.0 / ( 1.0 + 0.05 );
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

// spell_t::duration =========================================================

double spell_t::duration()
{
  double t = base_duration;
  if( channeled ) t *= haste();
  return t;
}

// spell_t::player_buff ======================================================

void spell_t::player_buff()
{
  action_t::player_buff();

  player_t* p = player;

  player_hit   = p -> composite_spell_hit();
  player_crit  = p -> composite_spell_crit();
  player_power = p -> composite_spell_power( school );

  if( p -> gear.chaotic_skyfire ) player_crit_bonus *= 1.09;

  if( p -> buffs.elemental_oath ||
      p -> buffs.moonkin_aura   )
  {
    player_crit += 0.05;
  }

  if( sim_t::WotLK )
  {
    player_power += std::max( ( p -> buffs.totem_of_wrath ? 160.0 : 0.0 ), p -> buffs.flametongue_totem );
  }
  else
  {
    if( p -> buffs.totem_of_wrath ) player_hit += 0.03;
  }
  
  if( p -> buffs.totem_of_wrath ) player_crit += 0.03;

  if( sim -> debug ) report_t::log( sim, "spell_t::player_buff: %s hit=%.2f crit=%.2f power=%.2f penetration=%.0f", 
		   name(), player_hit, player_crit, player_power, player_penetration );
}

// spell_t::target_debuff =====================================================

void spell_t::target_debuff( int8_t dmg_type )
{
  action_t::target_debuff( dmg_type );

  static uptime_t* wc_uptime = sim -> get_uptime( "winters_chill" );
  static uptime_t* is_uptime = sim -> get_uptime( "improved_scorch" );

  target_t* t = sim -> target;
   
  if( sim_t::WotLK )
  {
    target_hit += std::max( t -> debuffs.improved_faerie_fire, t -> debuffs.misery ) * 0.01; 
  }

  if( school == SCHOOL_SHADOW )
  {
  }
  else if( school == SCHOOL_ARCANE )
  {
  }
  else if( school == SCHOOL_FIRE )
  {
  }
  else if( school == SCHOOL_FROST )
  {
    if( ! sim_t::WotLK )
    {
      target_crit += ( t -> debuffs.winters_chill * 0.01 );
      wc_uptime -> update( t -> debuffs.winters_chill != 0 );
    }
  }
  else if( school == SCHOOL_FROSTFIRE )
  {
  }
  else if( school == SCHOOL_NATURE )
  {
  }
  else if( school == SCHOOL_HOLY )
  {
    if( t -> debuffs.judgement_of_crusader ) target_power += 218;
  }      

  if( sim_t::WotLK )
  {
    target_crit += ( std::max( t -> debuffs.winters_chill, t -> debuffs.improved_scorch ) * 0.01 );
    wc_uptime -> update( t -> debuffs.winters_chill != 0 );
    is_uptime -> update( t -> debuffs.improved_scorch != 0 );
  }

  target_power += t -> debuffs.focus_magic;

  if( sim -> debug ) 
    report_t::log( sim, "spell_t::target_debuff: %s multiplier=%.2f hit=%.2f crit=%.2f power=%.2f penetration=%.0f", 
		   name(), target_multiplier, target_hit, target_crit, target_power, target_penetration );
}
   
// spell_t::level_based_miss_chance ==========================================

double spell_t::level_based_miss_chance( int8_t player, 
					 int8_t target )
{
  int8_t delta_level = target - player;
  double miss=0;

  if( false && sim_t::WotLK ) // FIXME! Currently back to 17% miss
  {
    if( delta_level > 2 )
    {
      miss  = 0.07 + ( delta_level - 2 ) * 0.02;
    }
    else
    {
      miss = 0.05 + delta_level * 0.005;
    }
  }
  else
  {
    if( delta_level > 2 )
    {
      miss = 0.17 + ( delta_level - 3 ) * 0.11;
    }
    else
    {
      miss = 0.04 + delta_level * 0.01;
    }
  }

  if( miss < 0.01 ) miss = 0.01;
  if( miss > 0.99 ) miss = 0.99;

  return miss;
}

// spell_t::caclulate_result ================================================

void spell_t::calculate_result()
{
  dd = dot = dot_tick = 0;

  result = RESULT_NONE;

  if( ( result == RESULT_NONE ) && may_miss )
  {
    double miss_chance = level_based_miss_chance( player -> level, sim -> target -> level );

    miss_chance -= base_hit + player_hit + target_hit;

    if( ! sim_t::WotLK && miss_chance < 0.01 ) miss_chance = 0.01;

    if( rand_t::roll( miss_chance ) )
    {
      result = RESULT_MISS;
    }
  }

  if( ( result == RESULT_NONE ) && may_resist )
  {
    if( binary || num_ticks )
    {
      if( rand_t::roll( resistance() ) )
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
      double crit_chance = base_crit + player_crit + target_crit;

      if( rand_t::roll( crit_chance ) )
      {
	result = RESULT_CRIT;
      }
    }
  }

  if( sim -> debug ) report_t::log( sim, "%s result for %s is %s", player -> name(), name(), util_t::result_type_string( result ) );
}

