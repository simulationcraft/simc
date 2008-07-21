// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

// ==========================================================================
// Action
// ==========================================================================

// action_t::action_t =======================================================

action_t::action_t( int8_t      ty,
		    const char* n,
		    player_t*   p,
		    int8_t      r,
		    int8_t      s,
		    int8_t      tr ) :
  sim(p->sim), type(ty), name_str(n), player(p), school(s), resource(r), tree(tr), result(RESULT_NONE),
  bleed(false), binary(false), channeled(false), background(false), repeating(false), aoe(false), harmful(true),
  may_miss(false), may_resist(false), may_dodge(false), may_parry(false), 
  may_glance(false), may_block(false), may_crush(false), may_crit(false),
  base_execute_time(0), base_duration(0), base_cost(0),
    base_multiplier(1),   base_hit(0),   base_crit(0),   base_crit_bonus(1.0),   base_power(0),   base_penetration(0),
  player_multiplier(1), player_hit(0), player_crit(0), player_crit_bonus(1.0), player_power(0), player_penetration(0),
  target_multiplier(1), target_hit(0), target_crit(0), target_crit_bonus(1.0), target_power(0), target_penetration(0),
   dd(0), base_dd(0) ,  dd_power_mod(0), 
  dot(0), base_dot(0), dot_power_mod(0),
  dot_tick(0), time_remaining(0), num_ticks(0), current_tick(0), 
  cooldown_group(n), duration_group(n), cooldown(0), cooldown_ready(0), duration_ready(0),
  stats(0), rank(0), rank_index(-1), event(0), time_to_execute(0), time_to_tick(0), next(0)
{
  if( name_str == "" ) return;
  report_t::debug( sim, "Player %s creates action %s", p -> name(), name() );
  action_t** last = &( p -> action_list );
  while( *last ) last = &( (*last) -> next );
  *last = this;
  trigger_gcd = p -> base_gcd;
  for( stats = player -> stats_list; stats; stats = stats -> next )
  {
    if( stats -> name == n )
      break;
  }
  if( ! stats )
  {
    stats = new stats_t( this );
    stats -> init();
    stats_t** tail= &( p -> stats_list );
    while( *tail && name_str > ( (*tail) -> name ) )
    {
      tail = &( (*tail) -> next );
    }
    stats -> next = *tail;
    *tail = stats;
  }
}

// action_t::parse_options ==================================================

void action_t::parse_options( option_t*          options, 
			      const std::string& options_str )
{
  if( options_str.empty()     ) return;
  if( options_str.size() == 0 ) return;

  std::vector<std::string> splits;

  int size = wow_string_split( splits, options_str, "-" );

  for( int i=0; i < size; i++ )
  {
    static std::string n;
    static std::string v;

    if( 2 != wow_string_split( splits[ i ], "_", "S S", &n, &v ) )
    {
      printf( "wow_action: %s: Unexpected parameter '%s'.  Expected format: name=value\n", name(), splits[ i ].c_str() );
      assert( false );
    }
    
    if( ! option_t::parse( options, n, v ) )
    {
      printf( "wow_spell: %s: Unexpected parameter '%s'.\n", name(), n.c_str() );
      assert( false );
    }
  }
}

// action_t::choose_rank ====================================================

rank_t* action_t::choose_rank( rank_t* rank_list )
{
   for( int i=0; rank_list[ i ].level; i++ )
   {
     if( ( rank_index <= 0 && player -> level >= rank_list[ i ].level ) ||
         ( rank_index >  0 &&      rank_index == rank_list[ i ].index  ) )
       return &( rank_list[ i ] );
   }

   printf( "%s unable to find valid rank for %s\n", player -> name(), name() );
   assert( 0 );
   return 0;
}
   
// action_t::cost ======================================================

double action_t::cost()
{
  double c = base_cost;

  if( resource == RESOURCE_MANA ) 
  {
    c -= player -> buffs.mana_cost_reduction;
    if( c < 0 ) c = 0;

    if( player -> buffs.power_infusion ) c *= 0.80;
  }

   return c;
}
   
// aaction_t::player_buff ====================================================

void action_t::player_buff()
{
  player_multiplier  = 1.0;
  player_hit         = 0;
  player_crit        = 0;
  player_crit_bonus  = 1.0;
  player_power       = 0;
  player_penetration = 0;

  // 'multiplier' and 'penetration' handled here, all others handled in attack_t/spell_t

  player_t* p = player;

  if( school == SCHOOL_PHYSICAL )
  {
    player_penetration = p -> attack_penetration;
  }
  else
  {
    if( school == SCHOOL_HOLY )
    {
      if( p -> buffs.sanctity_aura ) player_multiplier *= 1.10 + 0.01 * ( p -> buffs.sanctity_aura - 1 );
    }
    else if( school == SCHOOL_SHADOW )
    {
      if( player -> buffs.shadow_form ) player_multiplier *= 1.15;
    }
    player_penetration = p -> spell_penetration;
  }

  report_t::debug( sim, "action_t::player_buff: %s hit=%.2f crit=%.2f power=%.2f penetration=%.0f", 
		   name(), player_hit, player_crit, player_power, player_penetration );
}

// action_t::target_debuff ==================================================

void action_t::target_debuff( int8_t dmg_type )
{
  static bool AFTER_3_0_0 = sim -> patch.after( 3, 0, 0 );

  target_multiplier  = 1.0;
  target_hit         = 0;
  target_crit        = 0;
  target_crit_bonus  = 1.0;
  target_power       = 0;
  target_penetration = 0;

  // 'multiplier' and 'penetration' handled here, all others handled in attack_t/spell_t

  target_t* t = sim -> target;

  if( bleed && t -> debuffs.mangle ) target_multiplier *= 1.30;

  if( school == SCHOOL_PHYSICAL )
  {
    target_penetration += t -> debuffs.sunder_armor * 520;

    
  }
  else 
  {
    if( school == SCHOOL_SHADOW )
    {
      target_multiplier *= 1.0 + ( t -> debuffs.curse_of_shadows * 0.01 );
      target_multiplier *= 1.0 + ( t -> debuffs.shadow_weaving   * 0.02 );
      if( ! AFTER_3_0_0 || dmg_type == DMG_DIRECT )
      {
	if( t -> debuffs.shadow_vulnerability ) target_multiplier *= 1.20;
	static uptime_t* sv_uptime = sim -> get_uptime( "shadow_vulnerability" );
	sv_uptime -> update( t -> debuffs.shadow_vulnerability != 0 );
      }
      if( t -> debuffs.curse_of_shadows ) target_penetration += 88;
    }
    else if( school == SCHOOL_ARCANE )
    {
      target_multiplier *= 1.0 + ( t -> debuffs.curse_of_shadows * 0.01 );       
      target_multiplier *= 1.0 + ( t -> debuffs.earth_and_moon   * 0.02 );
      if( t -> debuffs.curse_of_shadows ) target_penetration += 88;
    }
    else if( school == SCHOOL_FROST )
    {
      target_multiplier *= 1.0 + ( t -> debuffs.curse_of_elements * 0.01 );
      if( t -> debuffs.curse_of_elements ) target_penetration += 88;
    }
    else if( school == SCHOOL_FIRE )
    {
      target_multiplier *= 1.0 + ( t -> debuffs.curse_of_elements  * 0.01 );
      target_multiplier *= 1.0 + ( t -> debuffs.fire_vulnerability * 0.01 );
      if( t -> debuffs.curse_of_elements ) target_penetration += 88;
    }
    else if( school == SCHOOL_NATURE )
    {
      target_multiplier *= 1.0 + ( t -> debuffs.earth_and_moon * 0.02 );
      if( ! AFTER_3_0_0 || dmg_type == DMG_DIRECT )
      {
	if( t -> debuffs.nature_vulnerability ) target_multiplier *= 1.20;
	static uptime_t* sv_uptime = sim -> get_uptime( "nature_vulnerability" );
	sv_uptime -> update( t -> debuffs.nature_vulnerability != 0 );
      }
    }
    target_multiplier *= 1.0 + ( t -> debuffs.misery * 0.01 );
  }

  if( t -> debuffs.judgement_of_crusader ) target_crit += 0.03;

  report_t::debug( sim, "action_t::target_debuff: %s multiplier=%.2f hit=%.2f crit=%.2f power=%.2f penetration=%.0f", 
		   name(), target_multiplier, target_hit, target_crit, target_power, target_penetration );
}

// action_t::result_is_hit ==================================================

bool action_t::result_is_hit()
{
  report_t::debug( sim, "%s result for %s is %s", player -> name(), name(), wow_result_type_string( result ) );

  return( result == RESULT_HIT    || 
	  result == RESULT_CRIT   || 
	  result == RESULT_GLANCE || 
	  result == RESULT_BLOCK  );
}

// action_t::get_base_damage ================================================

void action_t::get_base_damage()
{
  if( ! rank ) return;

  if( sim -> average_dmg )
  {
    if( base_dd == 0 )
    {
      base_dd = ( rank -> dd_min + rank -> dd_max ) / 2.0;
    }
  }
  else
  {
    double delta = rank -> dd_max - rank -> dd_min;
    base_dd = rank -> dd_min + delta * wow_random();
  }

  if( base_dot  == 0 ) base_dot  = rank -> dot;
  if( base_cost == 0 ) base_cost = rank -> cost;
}

// action_t::calculate_damage ===============================================

void action_t::calculate_damage()
{
  get_base_damage();

  double power = base_power + player_power + target_power;

  if( base_dd > 0 )  
  {
    dd  = base_dd + power * dd_power_mod;
    dd *= base_multiplier * player_multiplier * target_multiplier;
  }

  if( base_dot > 0 ) 
  {
    dot  = base_dot + power * dot_power_mod;
    dot *= base_multiplier * player_multiplier;
  }

  report_t::debug( sim, "%s dmg for %s: dd=%.1f dot=%.1f", player -> name(), name(), dd, dot );
}

// action_t::resistance =====================================================

double action_t::resistance()
{
  if( bleed ) return 0;

  double resist=0;

  double penetration = base_penetration + player_penetration + target_penetration;

  if( school == SCHOOL_PHYSICAL )
  {
    double adjusted_armor = sim -> target -> armor - penetration;

    if( adjusted_armor < 0 ) adjusted_armor = 0;

    resist = adjusted_armor / ( ( 467.5 * player -> level ) + adjusted_armor - 22167.5 );
  }
  else
  {
    double resist_rating = sim -> target -> spell_resistance[ school ] - penetration;
    if( resist_rating < 0 ) resist_rating = 0;

    resist = resist_rating / ( player -> level * 5.0 );

    if( ! binary )
    {
      int delta_level = sim -> target -> level - player -> level;
      if( delta_level > 0 )
      {
	double level_resist = delta_level * 0.02;
	if( level_resist > resist ) resist = level_resist;
      }
    }
  }

  report_t::debug( sim, "%s queries resistance for %s: %.1f", player -> name(), name(), resist );

  return resist;
}

// action_t::adjust_damage_for_result =======================================

void action_t::adjust_damage_for_result()
{
  double dd_init = dd;

  if( result == RESULT_GLANCE )
  {
    dd *= 0.75;
  }
  else if( result == RESULT_CRIT )
  {
    double bonus = base_crit_bonus * player_crit_bonus * target_crit_bonus;

    dd  *= 1.0 + bonus;
  }

  if( may_resist )
  {
    if( ! binary && dd > 0 ) 
    {
      dd *= 1.0 - resistance();
    }
  }

  if( result == RESULT_BLOCK )
  {
    double blocked = sim -> target -> block_value;

    dd -= blocked;
    if( dd  < 0 ) dd  = 0;
  }

  report_t::debug( sim, "%s adjusts dmg for %s: %.1f -> %.1f", player -> name(), name(), dd_init, dd );
}

// action_t::consume_resource ===============================================

void action_t::consume_resource()
{
  double c = cost();

  report_t::debug( sim, "%s consumes %.1f %s for %s", player -> name(), c, wow_resource_type_string( resource ), name() );

  player -> resource_loss( resource, c );
}

// action_t::execute ========================================================

void action_t::execute()
{
  report_t::log( sim, "%s performs %s", player -> name(), name() );

  consume_resource();

  calculate_result();

  if( result_is_hit() )
  {
    calculate_damage();

    adjust_damage_for_result();

    player -> action_hit( this );
    
    if( dd > 0 )
    {
      assess_damage( dd, DMG_DIRECT );
    }

    if( num_ticks > 0 )
    {
      schedule_tick();
    }
  }
  else
  {
    report_t::log( sim, "%s avoids %s (%s)", sim -> target -> name(), name(), wow_result_type_string( result ) );

    player -> action_miss( this );
  }

  update_ready();

  update_stats( DMG_DIRECT );

  player -> action_finish( this );

  if( repeating && background ) schedule_execute();
}

// action_t::tick ===========================================================

void action_t::tick()
{
  report_t::debug( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );

  target_debuff( DMG_OVER_TIME );

  dot_tick = ( dot / num_ticks ) * target_multiplier;

  if( ! binary ) dot_tick *= 1.0 - resistance();

  result = RESULT_HIT; // Normal DoT ticks can only "hit"
  
  assess_damage( dot_tick, DMG_OVER_TIME );

  update_stats( DMG_OVER_TIME );

  if( current_tick == num_ticks ) last_tick();

  player -> action_tick( this );
}

// action_t::last_tick=======================================================

void action_t::last_tick()
{
  report_t::debug( sim, "%s fades from %s", name(), sim -> target -> name() );
}

// action_t::assess_damage ==================================================

void action_t::assess_damage( double amount, 
			      int8_t dmg_type )
{
   report_t::log( sim, "%s %s %ss %s for %.0f %s damage (%s)",
		  player -> name(), name(), 
		  wow_dmg_type_string( dmg_type ),
		  sim -> target -> name(), amount, 
		  wow_school_type_string( school ),
		  wow_result_type_string( result ) );

   sim -> target -> assess_damage( amount, school, dmg_type );

   player -> action_damage( this, amount, dmg_type );
}

// action_t::schedule_execute ==============================================

void action_t::schedule_execute()
{
  report_t::debug( sim, "%s schedules execute for %s", player -> name(), name() );

  time_to_execute = execute_time();
  
  event = new action_execute_event_t( sim, this, time_to_execute );

  player -> action_start( this );
}

// action_t::schedule_tick =================================================

void action_t::schedule_tick()
{
  report_t::debug( sim, "%s schedules tick for %s", player -> name(), name() );

  if( current_tick == 0 )
  {
    time_remaining = duration();
  }

  time_to_tick = time_remaining / ( num_ticks - current_tick );

  event = new action_tick_event_t( sim, this, time_to_tick );

  if( channeled ) player -> channeling = event;
}

// action_t::update_ready ====================================================

void action_t::update_ready()
{
  if( cooldown > 0 )
  {
    report_t::debug( sim, "%s shares cooldown for %s (%s)", player -> name(), name(), cooldown_group.c_str() );

    player -> share_cooldown( cooldown_group, cooldown );
  }
  if( ! channeled && time_remaining > 0 )
  {
    report_t::debug( sim, "%s shares duration for %s (%s)", player -> name(), name(), duration_group.c_str() );

    player -> share_duration( duration_group, sim -> current_time + time_remaining + 0.01 );
  }
}

// action_t::update_stats ===================================================

void action_t::update_stats( int8_t type )
{
  if( type == DMG_DIRECT )
  {
    stats -> add( dd, type, result, time_to_execute );
  }
  else if( type == DMG_OVER_TIME )
  {
    stats -> add( dot_tick, type, result, time_to_tick );
  }
  else assert(0);
}

// action_t::ready ==========================================================

bool action_t::ready()
{
  if( duration_ready > 0 )
    if( duration_ready > ( sim -> current_time + execute_time() ) )
      return false;

  if( cooldown_ready > sim -> current_time ) 
    return false;

  if( ! player -> resource_available( resource, cost() ) )
    return false;

  return true;
}

// action_t::reset ==========================================================

void action_t::reset()
{
  stats -> channeled = channeled; // FIXME! Move this to init_stats()
  current_tick = 0;
  time_remaining = 0;
  cooldown_ready = 0;
  duration_ready = 0;
  result = RESULT_NONE;
  event = 0;
}

// ==========================================================================
// action_t::create_action
// ==========================================================================

action_t* action_t::create_action( player_t*          p,
				   const std::string& name, 
				   const std::string& options )
{
  action_t*  a = p -> create_action( name, options );

  if( ! a ) a = unique_gear_t::create_action( p, name, options );
  if( ! a ) a =  consumable_t::create_action( p, name, options );

  return a;
}

