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
  binary(false), channeled(false), background(false), repeating(false), aoe(false), harmful(true), proc(false),
  may_miss(false), may_resist(false), may_dodge(false), may_parry(false), 
  may_glance(false), may_block(false), may_crush(false), may_crit(false),
  min_gcd(0), trigger_gcd(0),
  base_execute_time(0), base_tick_time(0), base_cost(0),
    base_multiplier(1),   base_hit(0),   base_crit(0),   base_crit_bonus(1.0),   base_power(0),   base_penetration(0),
  player_multiplier(1), player_hit(0), player_crit(0), player_crit_bonus(1.0), player_power(0), player_penetration(0),
  target_multiplier(1), target_hit(0), target_crit(0), target_crit_bonus(1.0), target_power(0), target_penetration(0),
  resource_consumed(0),
  direct_dmg(0), base_direct_dmg(0), direct_power_mod(0), 
  tick_dmg(0), base_tick_dmg(0), tick_power_mod(0),
  num_ticks(0), current_tick(0), ticking(0), 
  cooldown_group(n), duration_group(n), cooldown(0), cooldown_ready(0), duration_ready(0),
  weapon( 0 ), normalize_weapon_speed( false ),
  stats(0), rank(0), rank_index(-1), event(0), time_to_execute(0), time_to_tick(0), observer(0), next(0)
{
  if( sim -> debug ) report_t::log( sim, "Player %s creates action %s", p -> name(), name() );
  action_t** last = &( p -> action_list );
  while( *last ) last = &( (*last) -> next );
  *last = this;
  stats = p -> get_stats( n );
  stats -> school = school;
}

// action_t::parse_options ==================================================

void action_t::parse_options( option_t*          options, 
			      const std::string& options_str )
{
  if( options_str.empty()     ) return;
  if( options_str.size() == 0 ) return;

  std::vector<std::string> splits;

  int size = util_t::string_split( splits, options_str, "," );

  for( int i=0; i < size; i++ )
  {
    static std::string n;
    static std::string v;

    if( 2 != util_t::string_split( splits[ i ], "=", "S S", &n, &v ) )
    {
      fprintf( sim -> output_file, "util_t::action: %s: Unexpected parameter '%s'.  Expected format: name=value\n", name(), splits[ i ].c_str() );
      assert( false );
    }
    
    if( ! option_t::parse( sim, options, n, v ) )
    {
      fprintf( sim -> output_file, "util_t::spell: %s: Unexpected parameter '%s'.\n", name(), n.c_str() );
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

   fprintf( sim -> output_file, "%s unable to find valid rank for %s\n", player -> name(), name() );
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

  if( sim -> debug ) report_t::log( sim, "action_t::cost: %s %.2f %.2f %s", name(), base_cost, c, util_t::resource_type_string( resource ) );

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
    player_penetration = p -> composite_attack_penetration();
  }
  else
  {
    player_penetration = p -> composite_spell_penetration();

    if( school == SCHOOL_SHADOW )
    {
      if( p -> buffs.shadow_form )
      {
	player_multiplier *= 1.15;
      }
    }
    else if( school == SCHOOL_HOLY )
    {
      if( p -> buffs.sanctity_aura ) player_multiplier *= 1.10 + 0.01 * ( p -> buffs.sanctity_aura - 1 );
    }
  }

  if( p -> buffs.sanctified_retribution ) player_multiplier *= 1.02;

  if( sim -> debug ) 
    report_t::log( sim, "action_t::player_buff: %s hit=%.2f crit=%.2f power=%.2f penetration=%.0f", 
		   name(), player_hit, player_crit, player_power, player_penetration );
}

// action_t::target_debuff ==================================================

void action_t::target_debuff( int8_t dmg_type )
{
  target_multiplier  = 1.0;
  target_hit         = 0;
  target_crit        = 0;
  target_crit_bonus  = 1.0;
  target_power       = 0;
  target_penetration = 0;

  // 'multiplier' and 'penetration' handled here, all others handled in attack_t/spell_t

  static uptime_t* wg_uptime = sim -> get_uptime( "winters_grasp"        );
  
  target_t* t = sim -> target;

  if( school == SCHOOL_PHYSICAL )
  {
    target_penetration += t -> debuffs.sunder_armor * 520;
  }
  else 
  {
    target_multiplier *= 1.0 + ( std::max( t -> debuffs.curse_of_elements, t -> debuffs.earth_and_moon ) * 0.01 );
    if( t -> debuffs.curse_of_elements ) target_penetration += 88;
  }

  if( t -> debuffs.winters_grasp ) target_hit += 0.02;
  wg_uptime -> update( t -> debuffs.winters_grasp != 0 );

  if( sim -> debug ) 
    report_t::log( sim, "action_t::target_debuff: %s multiplier=%.2f hit=%.2f crit=%.2f power=%.2f penetration=%.0f", 
		   name(), target_multiplier, target_hit, target_crit, target_power, target_penetration );
}

// action_t::result_is_hit ==================================================

bool action_t::result_is_hit()
{
  return( result == RESULT_HIT    || 
	  result == RESULT_CRIT   || 
	  result == RESULT_GLANCE || 
	  result == RESULT_BLOCK  );
}

// action_t::result_is_miss =================================================

bool action_t::result_is_miss()
{
  return( result == RESULT_MISS   || 
	  result == RESULT_RESIST );
}

// action_t::resistance =====================================================

double action_t::resistance()
{
  if( ! may_resist ) return 0;

  target_t* t = sim -> target;
  double resist=0;

  double penetration = base_penetration + player_penetration + target_penetration;

  if( school == SCHOOL_PHYSICAL )
  {
    double adjusted_armor = t -> armor - penetration;

    if( adjusted_armor < 0 ) adjusted_armor = 0;

    resist = adjusted_armor / ( ( 467.5 * player -> level ) + adjusted_armor - 22167.5 );
  }
  else
  {
    double resist_rating = t -> spell_resistance[ school ];

    if( school == SCHOOL_FROSTFIRE )
    {
      resist_rating = std::min( t -> spell_resistance[ SCHOOL_FROST ],
				t -> spell_resistance[ SCHOOL_FIRE  ] );
    }

    resist_rating -= penetration;
    if( resist_rating < 0 ) resist_rating = 0;

    resist = resist_rating / ( player -> level * 5.0 );

    if( ! binary )
    {
      int delta_level = t -> level - player -> level;
      if( delta_level > 0 )
      {
	double level_resist = delta_level * 0.02;
	if( level_resist > resist ) resist = level_resist;
      }
    }
  }

  if( sim -> debug ) report_t::log( sim, "%s queries resistance for %s: %.2f", player -> name(), name(), resist );

  return resist;
}

// action_t::calculate_tick_damage ===========================================

double action_t::calculate_tick_damage()
{
  if( rank && base_tick_dmg == 0 ) base_tick_dmg = rank -> tick;

  if( base_tick_dmg == 0 ) return 0;

  double power = base_power + player_power + target_power;

  // FIXME! Are there DoT effects that include weapon damage/modifiers?

  tick_dmg  = base_tick_dmg + power * tick_power_mod;
  tick_dmg *= base_multiplier * player_multiplier * target_multiplier;

  double init_tick_dmg = tick_dmg;

  if( ! binary ) tick_dmg *= 1.0 - resistance();

  if( sim -> debug ) 
  {
    report_t::log( sim, "%s dmg for %s: td=%.0f i_td=%.0f b_td=%.0f mod=%.2f b_power=%.0f p_power=%.0f t_power=%.0f b_mult=%.2f p_mult=%.2f t_mult=%.2f", 
		   player -> name(), name(), tick_dmg, init_tick_dmg, base_tick_dmg, tick_power_mod, 
		   base_power, player_power, target_power, base_multiplier, player_multiplier, target_multiplier );
  }

  return tick_dmg;
}

// action_t::calculate_direct_damage =========================================

double action_t::calculate_direct_damage()
{
  if( rank )
  {
    if( sim -> average_dmg )
    {
      if( base_direct_dmg == 0 )
      {
	base_direct_dmg = ( rank -> dd_min + rank -> dd_max ) / 2.0;
      }
    }
    else
    {
      double delta = rank -> dd_max - rank -> dd_min;
      base_direct_dmg = rank -> dd_min + delta * rand_t::gen_float();
    }
  }

  if( base_direct_dmg == 0 ) return 0;

  double power = base_power + player_power + target_power;

  if( weapon )
  {
    double weapon_damage = weapon -> damage;
    double weapon_speed  = normalize_weapon_speed ? weapon -> normalized_weapon_speed() : weapon -> swing_time;

    direct_dmg  = base_direct_dmg + weapon_speed * ( weapon_damage + ( power * direct_power_mod ) );
    direct_dmg *= base_multiplier * player_multiplier * target_multiplier;
    if( ! weapon -> main ) direct_dmg *= 0.5;
  }
  else
  {
    direct_dmg  = base_direct_dmg + power * direct_power_mod;
    direct_dmg *= base_multiplier * player_multiplier * target_multiplier;
  }
  
  double init_direct_dmg = direct_dmg;
    
  if( result == RESULT_GLANCE )
  {
    direct_dmg *= 0.75;
  }
  else if( result == RESULT_CRIT )
  {
    double bonus = base_crit_bonus * player_crit_bonus * target_crit_bonus;
    
    direct_dmg *= 1.0 + bonus;
  }

  if( ! binary ) direct_dmg *= 1.0 - resistance();

  if( result == RESULT_BLOCK )
  {
    double blocked = sim -> target -> block_value;

    direct_dmg -= blocked;
    if( direct_dmg < 0 ) direct_dmg = 0;
  }

  if( sim -> debug ) 
  {
    report_t::log( sim, "%s dmg for %s: dd=%.0f i_dd=%.0f b_dd=%.0f mod=%.2f b_power=%.0f p_power=%.0f t_power=%.0f b_mult=%.2f p_mult=%.2f t_mult=%.2f", 
		   player -> name(), name(), direct_dmg, init_direct_dmg, base_direct_dmg, direct_power_mod, 
		   base_power, player_power, target_power, base_multiplier, player_multiplier, target_multiplier );
  }

  return direct_dmg;
}

// action_t::consume_resource ===============================================

void action_t::consume_resource()
{
  resource_consumed = cost();

  if( sim -> debug ) 
    report_t::log( sim, "%s consumes %.1f %s for %s", player -> name(), 
		   resource_consumed, util_t::resource_type_string( resource ), name() );

  player -> resource_loss( resource, resource_consumed );

  stats -> consume_resource( resource_consumed );
}

// action_t::execute ========================================================

void action_t::execute()
{
  if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );

  if( observer ) *observer = 0;

  consume_resource();

  player_buff();

  target_debuff( DMG_DIRECT );

  calculate_result();

  if( result_is_hit() )
  {
    calculate_direct_damage();

    player -> action_hit( this );
    
    if( direct_dmg > 0 )
    {
      assess_damage( direct_dmg, DMG_DIRECT );
    }

    if( num_ticks > 0 )
    {
      current_tick = 0;
      schedule_tick();
    }
  }
  else
  {
    if( sim -> log ) report_t::log( sim, "%s avoids %s (%s)", sim -> target -> name(), name(), util_t::result_type_string( result ) );

    player -> action_miss( this );
  }

  update_ready();

  update_stats( DMG_DIRECT );

  player -> action_finish( this );

  if( repeating && ! proc ) schedule_execute();
}

// action_t::tick ===========================================================

void action_t::tick()
{
  if( sim -> debug ) report_t::log( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );

  result = RESULT_HIT; // Normal DoT ticks can only "hit"

  target_debuff( DMG_OVER_TIME );

  calculate_tick_damage();
  
  assess_damage( tick_dmg, DMG_OVER_TIME );

  update_stats( DMG_OVER_TIME );

  if( current_tick == num_ticks ) last_tick();

  player -> action_tick( this );
}

// action_t::last_tick=======================================================

void action_t::last_tick()
{
  if( sim -> debug ) report_t::log( sim, "%s fades from %s", name(), sim -> target -> name() );

  ticking = 0;

  if( observer ) *observer = 0;
}

// action_t::assess_damage ==================================================

void action_t::assess_damage( double amount, 
			      int8_t dmg_type )
{
   if( sim -> log )
     report_t::log( sim, "%s %s %ss %s for %.0f %s damage (%s)",
		    player -> name(), name(), 
		    util_t::dmg_type_string( dmg_type ),
		    sim -> target -> name(), amount, 
		    util_t::school_type_string( school ),
		    util_t::result_type_string( result ) );

   sim -> target -> assess_damage( amount, school, dmg_type );

   player -> action_damage( this, amount, dmg_type );
}

// action_t::schedule_execute ==============================================

void action_t::schedule_execute()
{
  if( sim -> log ) report_t::log( sim, "%s schedules execute for %s", player -> name(), name() );

  time_to_execute = execute_time();
  
  event = new action_execute_event_t( sim, this, time_to_execute );

  if( observer ) *observer = this;

  player -> action_start( this );
}

// action_t::schedule_tick =================================================

void action_t::schedule_tick()
{
  if( sim -> debug ) report_t::log( sim, "%s schedules tick for %s", player -> name(), name() );

  ticking = 1;

  time_to_tick = tick_time();

  event = new action_tick_event_t( sim, this, time_to_tick );

  if( channeled ) player -> channeling = event;

  if( observer ) *observer = this;
}

// action_t::refresh_duration ================================================

void action_t::refresh_duration()
{
  if( sim -> debug ) report_t::log( sim, "%s refreshes duration of %s", player -> name(), name() );

  // Make sure this DoT is still ticking......
  assert( event );

  // Recalculate state of current player buffs.
  player_buff();

  // Refreshing a DoT does not interfere with the next tick event.  Ticks will stil occur
  // every "base_tick_time" seconds.  To determine the new finish time for the DoT, start 
  // from the time of the next tick and add the time for the remaining ticks to that event.
  duration_ready = event -> time + base_tick_time * ( num_ticks - 1 );

  current_tick = 0;
}

// action_t::extend_duration =================================================

void action_t::extend_duration( int8_t extra_ticks )
{
  if( sim -> debug ) report_t::log( sim, "%s extends duration of %s", player -> name(), name() );

  num_ticks += extra_ticks;
  duration_ready += tick_time() * extra_ticks;
}

// action_t::update_ready ====================================================

void action_t::update_ready()
{
  if( cooldown > 0 )
  {
    if( sim -> debug ) report_t::log( sim, "%s shares cooldown for %s (%s)", player -> name(), name(), cooldown_group.c_str() );

    player -> share_cooldown( cooldown_group, cooldown );
  }
  if( ! channeled && num_ticks > 0 && ! result_is_miss() )
  {
    if( sim -> debug ) report_t::log( sim, "%s shares duration for %s (%s)", player -> name(), name(), duration_group.c_str() );

    player -> share_duration( duration_group, sim -> current_time + 0.01 + tick_time() * num_ticks );
  }
}

// action_t::update_stats ===================================================

void action_t::update_stats( int8_t type )
{
  if( type == DMG_DIRECT )
  {
    stats -> add( direct_dmg, type, result, time_to_execute );
  }
  else if( type == DMG_OVER_TIME )
  {
    stats -> add( tick_dmg, type, result, time_to_tick );
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
  ticking = 0;
  current_tick = 0;
  cooldown_ready = 0;
  duration_ready = 0;
  result = RESULT_NONE;
  event = 0;

  // FIXME! stats gets initialized inside action_t constructor which occurs BEFORE the 
  // spell_t/attack_t derivative constructor gets executed.  But the "channeled" and "background"
  // fields are set in the derived sub-class which is too late.  So set them here.....
  stats -> channeled = channeled;  
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

