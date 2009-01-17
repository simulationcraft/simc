// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

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
  binary(false), channeled(false), background(false), repeating(false), aoe(false), harmful(true), proc(false), heal(false),
  may_miss(false), may_resist(false), may_dodge(false), may_parry(false), 
  may_glance(false), may_block(false), may_crush(false), may_crit(false),
  min_gcd(0), trigger_gcd(0),
  base_execute_time(0), base_tick_time(0), base_cost(0),
  base_dd_min(0), base_dd_max(0), base_td_init(0),
  base_dd_multiplier(1), base_td_multiplier(1), power_multiplier(1),
    base_multiplier(1),   base_hit(0),   base_crit(0),   base_power(0),   base_penetration(0),
  player_multiplier(1), player_hit(0), player_crit(0), player_power(0), player_penetration(0),
  target_multiplier(1), target_hit(0), target_crit(0), target_power(0), target_penetration(0),
    base_crit_multiplier(1),   base_crit_bonus_multiplier(1),
  player_crit_multiplier(1), player_crit_bonus_multiplier(1),
  target_crit_multiplier(1), target_crit_bonus_multiplier(1),
  resource_consumed(0),
  direct_dmg(0), base_direct_dmg(0), direct_power_mod(0), 
  tick_dmg(0), base_tick_dmg(0), tick_power_mod(0),
  num_ticks(0), current_tick(0), added_ticks(0), ticking(0), 
  cooldown_group(n), duration_group(n), cooldown(0), cooldown_ready(0), duration_ready(0),
  weapon(0), weapon_multiplier(1), normalize_weapon_speed( false ), normalize_weapon_damage( false ),
  stats(0), rank_index(-1), event(0), time_to_execute(0), time_to_tick(0), sync_action(0), observer(0), next(0), sequence(0)
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
    std::string n;
    std::string v;

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

// action_t::merge_options ==================================================

option_t* action_t::merge_options( std::vector<option_t>& merged_options,
                                   option_t*              options1, 
                                   option_t*              options2 )
{
  if( ! options1 ) return options2;
  if( ! options2 ) return options1;

  merged_options.clear();

  for( int i=0; options1[ i ].name; i++ ) merged_options.push_back( options1[ i ] );
  for( int i=0; options2[ i ].name; i++ ) merged_options.push_back( options2[ i ] );

  option_t null_option;
  null_option.name = 0;
  merged_options.push_back( null_option );

  return &( merged_options[ 0 ] );
}

// action_t::init_rank ======================================================

rank_t* action_t::init_rank( rank_t* rank_list )
{
  if( resource == RESOURCE_MANA )
  {
    for( int i=0; rank_list[ i ].level; i++ )
    {
      rank_t& r = rank_list[ i ];

      // Look for ranks in which the cost of an action is a percentage of base mana
      if( r.cost > 0 && r.cost < 1 )
      {
        r.cost *= player -> resource_base[ RESOURCE_MANA ];
      }
    }
  }

   for( int i=0; rank_list[ i ].level; i++ )
   {
     if( ( rank_index <= 0 && player -> level >= rank_list[ i ].level ) ||
         ( rank_index >  0 &&      rank_index == rank_list[ i ].index  ) )
     {
       rank_t* rank = &( rank_list[ i ] );

       base_dd_min  = rank -> dd_min;
       base_dd_max  = rank -> dd_max;
       base_td_init = rank -> tick;
       base_cost    = rank -> cost;

       return rank;
     }
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
   
// action_t::player_buff =====================================================

void action_t::player_buff()
{
  player_multiplier            = 1.0;
  player_hit                   = 0;
  player_crit                  = 0;
  player_crit_multiplier       = 1.0;
  player_crit_bonus_multiplier = 1.0;
  player_power                 = 0;
  player_penetration           = 0;
  power_multiplier             = 1.0;

  // 'multiplier' and 'penetration' handled here, all others handled in attack_t/spell_t

  player_t* p = player;

  if( school == SCHOOL_BLEED )
  {
    // Not applicable
  }
  else if( school == SCHOOL_PHYSICAL )
  {
    player_penetration = p -> composite_attack_penetration();
  }
  else
  {
    player_penetration = p -> composite_spell_penetration();
  }

  if( p -> type == PLAYER_GUARDIAN ) return;  // Guardians do not benefit from auras

  if( school == SCHOOL_SHADOW )
  {
    if( p -> buffs.shadow_form )
    {
      player_multiplier *= 1.15;
    }
  }

  if( p -> buffs.sanctified_retribution ) player_multiplier *= 1.03;

  if( p -> buffs.tricks_of_the_trade ) player_multiplier *= 1.15;

  if( sim -> debug ) 
    report_t::log( sim, "action_t::player_buff: %s hit=%.2f crit=%.2f power=%.2f penetration=%.0f", 
                   name(), player_hit, player_crit, player_power, player_penetration );
}

// action_t::target_debuff ==================================================

void action_t::target_debuff( int8_t dmg_type )
{
  target_multiplier            = 1.0;
  target_hit                   = 0;
  target_crit                  = 0;
  target_crit_multiplier       = 1.0;
  target_crit_bonus_multiplier = 1.0;
  target_power                 = 0;
  target_penetration           = 0;

  // 'multiplier' and 'penetration' handled here, all others handled in attack_t/spell_t

  target_t* t = sim -> target;

  if( school == SCHOOL_PHYSICAL || 
      school == SCHOOL_BLEED    )
  {
    if( t -> debuffs.blood_frenzy  || 
        t -> debuffs.savage_combat ) 
    {
      target_multiplier *= 1.02;
    }
  }
  else
  {
    target_multiplier *= 1.0 + ( std::max( t -> debuffs.curse_of_elements, t -> debuffs.earth_and_moon ) * 0.01 );
    if( t -> debuffs.curse_of_elements ) target_penetration += 88;
  }

  if( t -> debuffs.mangle )
  {
    if( school == SCHOOL_BLEED ) 
    {
      target_multiplier *= 1.30;
    }
  }

  if( t -> debuffs.razorice )
  {
    if( school == SCHOOL_FROST ||
        school == SCHOOL_FROSTFIRE )
    {
      target_multiplier *= 1.05;
    }
  }

  if( t -> debuffs.winters_grasp ) target_hit += 0.02;
  t -> uptimes.winters_grasp -> update( t -> debuffs.winters_grasp != 0 );

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

// action_t::armor ==========================================================

double action_t::armor()
{
  double adjusted_armor = sim -> target -> composite_armor();

  if( player -> buffs.executioner ) adjusted_armor -= 840;

  return adjusted_armor;
}

// action_t::resistance =====================================================

double action_t::resistance()
{
  if( ! may_resist ) return 0;

  target_t* t = sim -> target;
  double resist=0;

  double penetration = base_penetration + player_penetration + target_penetration;

  if( school == SCHOOL_BLEED )
  {
    // Bleeds cannot be resisted
  }
  else if( school == SCHOOL_PHYSICAL )
  {
    double adjusted_armor = armor() * ( 1.0 - penetration );

    if( adjusted_armor <= 0 ) return 0;

    double adjusted_level = player -> level + 4.5 * ( player -> level - 59 );

    resist = adjusted_armor / ( adjusted_armor + 400 + 85.0 * adjusted_level );
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

// action_t::total_crit_bonus ================================================

double action_t::total_crit_bonus()
{
  double crit_multiplier = (   base_crit_multiplier * 
                             player_crit_multiplier * 
                             target_crit_multiplier );
  
  double crit_bonus_multiplier = (   base_crit_bonus_multiplier * 
                                   player_crit_bonus_multiplier * 
                                   target_crit_bonus_multiplier );

  double crit_bonus = ( ( 1.0 + base_crit_bonus ) * crit_multiplier - 1.0 ) * crit_bonus_multiplier;

  if( sim -> debug ) 
  {
    report_t::log( sim, "%s crit_bonus for %s: cb=%.0f b_cb=%.2f b_cm=%.2f p_cm=%.2f t_cm=%.2f b_cbm=%.2f p_cbm=%.2f t_cbm=%.2f", 
                   player -> name(), name(), crit_bonus, base_crit_bonus,
                   base_crit_multiplier,       player_crit_multiplier,       target_crit_multiplier,
                   base_crit_bonus_multiplier, player_crit_bonus_multiplier, target_crit_bonus_multiplier );
  }

  return crit_bonus;
}

// action_t::calculate_tick_damage ===========================================

double action_t::calculate_tick_damage()
{
  if( base_tick_dmg == 0 ) base_tick_dmg = base_td_init;

  if( base_tick_dmg == 0 ) return 0;

  // FIXME! Are there DoT effects that include weapon damage/modifiers?

  tick_dmg  = base_tick_dmg + total_power() * tick_power_mod;
  tick_dmg *= total_td_multiplier();
  
  double init_tick_dmg = tick_dmg;

  if( ! binary ) tick_dmg *= 1.0 - resistance();

  if( sim -> debug ) 
  {
    report_t::log( sim, "%s dmg for %s: td=%.0f i_td=%.0f b_td=%.0f mod=%.2f b_power=%.0f p_power=%.0f t_power=%.0f b_mult=%.2f p_mult=%.2f t_mult=%.2f", 
                   player -> name(), name(), tick_dmg, init_tick_dmg, base_tick_dmg, tick_power_mod, 
                   base_power, player_power, target_power, base_multiplier * base_td_multiplier, player_multiplier, target_multiplier );
  }

  return tick_dmg;
}

// action_t::calculate_direct_damage =========================================

double action_t::calculate_direct_damage()
{
  if( base_dd_max > 0 )
  {
    if( sim -> average_dmg )
    {
      if( base_direct_dmg == 0 )
      {
        base_direct_dmg = ( base_dd_min + base_dd_max ) / 2.0;
      }
    }
    else
    {
      double delta = base_dd_max - base_dd_min;
      base_direct_dmg = base_dd_min + delta * sim -> rng -> real();
    }
  }

  if( base_direct_dmg == 0 ) return 0;

  if( weapon )
  {
    double weapon_damage = normalize_weapon_damage ? weapon -> damage * 2.8 / weapon -> swing_time : weapon -> damage;
    double weapon_speed  = normalize_weapon_speed  ? weapon -> normalized_weapon_speed() : weapon -> swing_time;

    double hand_multiplier = ( weapon -> slot == SLOT_OFF_HAND ) ? 0.5 : 1.0;

    double power_damage = weapon_speed * direct_power_mod * total_power();
    
    direct_dmg  = base_direct_dmg + ( weapon_damage + power_damage ) * weapon_multiplier * hand_multiplier;
    direct_dmg *= total_dd_multiplier();
  }
  else
  {
    direct_dmg  = base_direct_dmg + direct_power_mod * total_power();
    direct_dmg *= total_dd_multiplier();
  }
  
  double init_direct_dmg = direct_dmg;
    
  if( result == RESULT_GLANCE )
  {
    direct_dmg *= 0.75;
  }
  else if( result == RESULT_CRIT )
  {
    direct_dmg *= 1.0 + total_crit_bonus();
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
                   base_power, player_power, target_power, base_multiplier * base_dd_multiplier, player_multiplier, target_multiplier );
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

  player_buff();

  target_debuff( DMG_DIRECT );

  calculate_result();

  consume_resource();

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

  if( sequence ) sequence -> schedule_execute();

  player -> in_combat = true;
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
  
  event = new ( sim ) action_execute_event_t( sim, this, time_to_execute );

  if( observer ) *observer = this;

  player -> action_start( this );
}

// action_t::schedule_tick =================================================

void action_t::schedule_tick()
{
  if( sim -> debug ) report_t::log( sim, "%s schedules tick for %s", player -> name(), name() );

  ticking = 1;

  time_to_tick = tick_time();

  event = new ( sim ) action_tick_event_t( sim, this, time_to_tick );

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
  added_ticks += extra_ticks;
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

  if( sync_action && ! sync_action -> ready() )
    return false;

  return true;
}

// action_t::reset ==========================================================

void action_t::reset()
{
  if( ! sync_str.empty() && ! sync_action )
  {
    sync_action = player -> find_action( sync_str );

    if( ! sync_action )
    {
      printf( "simcraft: Unable to find sync action '%s' for primary action '%s'\n", sync_str.c_str(), name() );
      exit(0);
    }
  }

  ticking = 0;
  current_tick = 0;
  cooldown_ready = 0;
  duration_ready = 0;
  result = RESULT_NONE;
  event = 0;

  stats -> reset( this );
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

