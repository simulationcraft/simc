// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

// ==========================================================================
// Attack
// ==========================================================================

// attack_t::attack_t =======================================================

attack_t::attack_t( const char* n, player_t* p, int8_t r, int8_t s, int8_t t ) :
  action_t( ACTION_ATTACK, n, p, r, s, t ),
  base_expertise(0), player_expertise(0), target_expertise(0), 
  normalize_weapon_speed(false), weapon(0)
{
  may_miss = may_dodge = may_parry = may_glance = may_block = true;

  if( player -> position == POSITION_BACK )
  {
    may_parry = false;
    may_block = false;
  }
  if( weapon && weapon -> group() == WEAPON_RANGED )
  {
    may_dodge  = false;
    may_parry  = false;
    may_glance = false;
  }

  base_crit_bonus = 1.0;

  dd_power_mod = dot_power_mod = 1.0 / 14;
}
  
// attack_t::execute_time ===================================================

double attack_t::execute_time()
{
  if( base_execute_time == 0 ) return 0;

  double t = base_execute_time;

  t *= player -> haste;
  if( player -> buffs.bloodlust ) t *= 0.7;

  return t;
}

// attack_t::duration =======================================================

double attack_t::duration()
{
  double t = base_duration;

  if( channeled ) 
  {
    t *= player -> haste;
    if( player -> buffs.bloodlust ) t *= 0.7;
  }

  return t;
}

// attack_t::player_buff ====================================================

void attack_t::player_buff()
{
  player_t* p = player;

  player_hit        = p -> attack_hit;
  player_expertise  = p -> attack_expertise;
  player_crit       = p -> attack_crit + p -> attack_crit_per_agility  * p -> agility();
  player_crit_bonus = 1.0;
  player_power      = p -> attack_power +
                      p -> attack_power_per_strength * p -> strength() +
                      p -> attack_power_per_agility  * p -> agility();
  player_power     *= p -> attack_power_multiplier;
}

// attack_t::target_debuff ==================================================

void attack_t::target_debuff()
{
  target_expertise = 0;

  
}

// attack_t::build_table ====================================================

void attack_t::build_table( std::vector<double>& chances, 
			    std::vector<int>&    results )
{
  chances.clear();
  results.clear();

  double miss=0, dodge=0, parry=0, glance=0, block=0, crit=0;

  double expertise = base_expertise + player_expertise + target_expertise;

  int delta_level = sim -> target -> level - player -> level;

  if( may_miss  ) 
  {
    if( delta_level > 2 )
    {
      miss  = 0.07 + ( delta_level - 2 ) * 0.02;
    }
    else
    {
      miss = 0.05 + delta_level * 0.005;
    }
    miss -= base_hit + player_hit + target_hit;
  }

  if( may_dodge ) 
  {
    if( delta_level > 2 )
    {
      dodge = 0.07 + ( delta_level - 2 ) * 0.02;
    }
    else
    {
      dodge  = 0.05 + delta_level * 0.005;
    }
    dodge -= expertise * 0.0025;
  }

  if( may_parry ) 
  {
    parry  = 0.05 + delta_level * 0.005;
    parry -= expertise * 0.0025;
  }

  if( may_glance ) 
  {
    glance = 0.24;
  }

  if( may_block && sim -> target -> shield ) 
  {
    block = 0.05 + delta_level * 0.005;
  }

  if( may_crit )
  {
    crit = base_crit + player_crit + target_crit;
  }

  double total = 0;

  if( miss > 0 )
  {
    total += miss;
    chances.push_back( total );
    results.push_back( RESULT_MISS );
  }
  if( dodge > 0 )
  {
    total += dodge;
    chances.push_back( total );
    results.push_back( RESULT_DODGE );
  }
  if( parry > 0 )
  {
    total += parry;
    chances.push_back( total );
    results.push_back( RESULT_PARRY );
  }
  if( glance > 0 )
  {
    total += glance;
    chances.push_back( total );
    results.push_back( RESULT_GLANCE );
  }
  if( block > 0 )
  {
    total += block;
    chances.push_back( total );
    results.push_back( RESULT_BLOCK );
  }
  if( crit > 0 )
  {
    total += crit;
    chances.push_back( total );
    results.push_back( RESULT_CRIT );
  }
  if( total < 1.0 )
  {
    chances.push_back( 1.0 );
    results.push_back( RESULT_HIT );
  }
}

// attack_t::caclulate_result ===============================================

void attack_t::calculate_result()
{
  dd = dot = dot_tick = 0;

  static std::vector<double> chances;
  static std::vector<int>    results;

  result = RESULT_NONE;

  player_buff();
  target_debuff();
      
  build_table( chances, results );

  double random = wow_random();
  int num_results = results.size();

  for( int i=0; i < num_results; i++ )
  {
    if( random <= chances[ i ] )
    {
      result = results[ i ];
      break;
    }
  }
  assert( result != RESULT_NONE );

  if( binary && result_is_hit() )
  {
    if( wow_random( resistance() ) )
    {
      result = RESULT_RESIST;
    }
  }
}

// attack_t::calculate_damage ===============================================

void attack_t::calculate_damage()
{
  get_base_damage();

  double weapon_speed  = 0;
  double weapon_damage = 0;

  if( weapon )
  {
    weapon_damage = weapon -> damage;
    weapon_speed  = normalize_weapon_speed ? weapon -> normalized_weapon_speed() : weapon -> swing_time;
  }

  double attack_power = base_power + player_power + target_power;

  if( base_dd > 0 )  
  {
    dd  = base_dd + weapon_damage + attack_power * dd_power_mod * weapon_speed;
    dd *= base_multiplier * player_multiplier * target_multiplier;
  }

  if( base_dot > 0 ) 
  {
    dot  = base_dot + weapon_damage + attack_power * dot_power_mod * weapon_speed;
    dot *= base_multiplier * player_multiplier;
  }
}

