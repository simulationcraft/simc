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

// spell_t::haste ============================================================

double spell_t::haste() SC_CONST
{
  player_t* p = player;
  double h = p -> spell_haste;

  if ( p -> type != PLAYER_GUARDIAN )
  {
    if (      p -> buffs.bloodlust      -> check() ) h *= 1.0 / ( 1.0 + 0.30 );
    else if ( p -> buffs.power_infusion -> check() ) h *= 1.0 / ( 1.0 + 0.20 );

    if ( p -> buffs.berserking -> check() )          h *= 1.0 / ( 1.0 + 0.20 );

    if ( sim -> auras.swift_retribution -> check() || sim -> auras.improved_moonkin -> check() )
    {
      h *= 1.0 / ( 1.0 + 0.03 );
    }

    if ( sim -> auras.wrath_of_air -> check() )
    {
      h *= 1.0 / ( 1.0 + 0.05 );
    }

    if ( sim -> auras.celerity -> check() )
    {
      h *= 1.0 / ( 1.0 + 0.20 );
    }
  }

  return h;
}

// spell_t::gcd =============================================================

double spell_t::gcd() SC_CONST
{
  double t = trigger_gcd;
  if ( t == 0 ) return 0;

  t *= haste();
  if ( t < min_gcd ) t = min_gcd;

  return t;
}

// spell_t::execute_time =====================================================

double spell_t::execute_time() SC_CONST
{
  double t = base_execute_time;
  if ( t <= 0 ) return 0;
  t *= haste();
  return t;
}

// spell_t::tick_time ========================================================

double spell_t::tick_time() SC_CONST
{
  double t = base_tick_time;
  if ( channeled ) t *= haste();
  return t;
}

// spell_t::player_buff ======================================================

void spell_t::player_buff()
{
  action_t::player_buff();

  player_t* p = player;

  player_hit  = p -> composite_spell_hit();
  player_crit = p -> composite_spell_crit();

  if ( p -> meta_gem == META_CHAOTIC_SKYFIRE       ||
       p -> meta_gem == META_CHAOTIC_SKYFLARE      ||
       p -> meta_gem == META_RELENTLESS_EARTHSIEGE ||
       p -> meta_gem == META_RELENTLESS_EARTHSTORM )
  {
    player_crit_multiplier *= 1.03;
  }

  if ( sim -> debug ) log_t::output( sim, "spell_t::player_buff: %s hit=%.2f crit=%.2f crit_multiplier=%.2f",
                                       name(), player_hit, player_crit, player_crit_multiplier );
}

// spell_t::target_debuff =====================================================

void spell_t::target_debuff( int dmg_type )
{
  action_t::target_debuff( dmg_type );

  target_t* t = sim -> target;

  target_hit += std::max( t -> debuffs.improved_faerie_fire -> value(), t -> debuffs.misery -> value() ) * 0.01;

  int crit_debuff = std::max( std::max( t -> debuffs.winters_chill -> stack(), 
					t -> debuffs.improved_scorch -> stack() ),
			                t -> debuffs.improved_shadow_bolt -> stack() * 5 );
  target_crit += crit_debuff * 0.01;

  if ( sim -> debug )
    log_t::output( sim, "spell_t::target_debuff: %s multiplier=%.2f hit=%.2f crit=%.2f",
                   name(), target_multiplier, target_hit, target_crit );
}

// spell_t::level_based_miss_chance ==========================================

double spell_t::level_based_miss_chance( int player,
                                         int target ) SC_CONST
{
  int delta_level = target - player;
  double miss=0;

  if ( delta_level > 2 )
  {
    miss = 0.17 + ( delta_level - 3 ) * 0.11;
  }
  else
  {
    miss = 0.04 + delta_level * 0.01;
  }

  if ( miss < 0.00 ) miss = 0.00;
  if ( miss > 0.99 ) miss = 0.99;

  return miss;
}

// spell_t::crit_chance ====================================================

double spell_t::crit_chance( int player_level,
			     int target_level ) SC_CONST
{
  int delta_level = target_level - player_level;
  double chance = total_crit();
	
  if ( ! player -> is_pet() && delta_level > 2 && sim -> spell_crit_suppression )
  {
    chance -= 0.03;
  }
	
  return chance;
}

// spell_t::caclulate_result ================================================

void spell_t::calculate_result()
{
  direct_dmg = 0;
  double crit = 0;

  result = RESULT_NONE;

  if ( ! harmful ) return;

  if ( ( result == RESULT_NONE ) && may_miss )
  {
    double miss_chance = level_based_miss_chance( player -> level, sim -> target -> level );

    miss_chance -= total_hit();

    if ( rng[ RESULT_MISS ] -> roll( miss_chance ) )
    {
      result = RESULT_MISS;
    }
  }

  if ( ( result == RESULT_NONE ) && may_resist && binary )
  {
    if ( rng[ RESULT_RESIST ] -> roll( resistance() ) )
    {
      result = RESULT_RESIST;
    }
  }

  if ( result == RESULT_NONE )
  {
    result = RESULT_HIT;

    if ( may_crit )
    {
      // Experimental check for spell crit suppression
      crit = crit_chance( player -> level, sim -> target -> level );
      if ( rng[ RESULT_CRIT ] -> roll( crit ) )
      {
        result = RESULT_CRIT;
      }
    }
  }

  if ( sim -> debug ) log_t::output( sim, "%s result for %s is %s", player -> name(), name(), util_t::result_type_string( result ) );
}

// spell_t::execute =========================================================

void spell_t::execute()
{
  action_t::execute();

  action_callback_t::trigger( player -> spell_result_callbacks[ result ], this );
}
