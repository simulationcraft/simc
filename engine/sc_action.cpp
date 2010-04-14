// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Action
// ==========================================================================

// action_t::action_t =======================================================

action_t::action_t( int         ty,
                    const char* n,
                    player_t*   p,
                    int         r,
                    int         s,
                    int         tr,
                    bool        sp ) :
    sim( p->sim ), type( ty ), name_str( n ), player( p ), id( 0 ), school( s ), resource( r ), tree( tr ), result( RESULT_NONE ),
    dual( false ), special( sp ), binary( false ), channeled( false ), background( false ), sequence( false ), direct_tick( false ),
    repeating( false ), aoe( false ), harmful( true ), proc( false ), pseudo_pet( false ), auto_cast( false ),
    may_miss( false ), may_resist( false ), may_dodge( false ), may_parry( false ),
    may_glance( false ), may_block( false ), may_crush( false ), may_crit( false ),
    tick_may_crit( false ), tick_zero( false ), dot_behavior( DOT_WAIT ),
    min_gcd( 0 ), trigger_gcd( 0 ), range( -1 ),
    weapon_power_mod( 1.0/14 ), direct_power_mod( 0 ), tick_power_mod( 0 ),
    base_execute_time( 0 ), base_tick_time( 0 ), base_cost( 0 ),
    base_dd_min( 0 ), base_dd_max( 0 ), base_td( 0 ), base_td_init( 0 ),
    base_dd_multiplier( 1 ), base_td_multiplier( 1 ),
    base_multiplier( 1 ),   base_hit( 0 ),   base_crit( 0 ),   base_penetration( 0 ),
    player_multiplier( 1 ), player_hit( 0 ), player_crit( 0 ), player_penetration( 0 ),
    target_multiplier( 1 ), target_hit( 0 ), target_crit( 0 ), target_penetration( 0 ),
    base_spell_power( 0 ),   base_attack_power( 0 ),
    player_spell_power( 0 ), player_attack_power( 0 ),
    target_spell_power( 0 ), target_attack_power( 0 ),
    base_spell_power_multiplier( 0 ),   base_attack_power_multiplier( 0 ),
    player_spell_power_multiplier( 1 ), player_attack_power_multiplier( 1 ),
    base_crit_multiplier( 1 ),   base_crit_bonus_multiplier( 1 ),
    player_crit_multiplier( 1 ), player_crit_bonus_multiplier( 1 ),
    target_crit_multiplier( 1 ), target_crit_bonus_multiplier( 1 ),
    base_dd_adder( 0 ), player_dd_adder( 0 ), target_dd_adder( 0 ),
    resource_consumed( 0 ),
    direct_dmg( 0 ), tick_dmg( 0 ),
    resisted_dmg( 0 ), blocked_dmg( 0 ),
    num_ticks( 0 ), current_tick( 0 ), added_ticks( 0 ), ticking( 0 ),
    weapon( 0 ), weapon_multiplier( 1 ), normalize_weapon_damage( false ), normalize_weapon_speed( false ),
    rng_travel( 0 ), stats( 0 ), execute_event( 0 ), tick_event( 0 ),
    time_to_execute( 0 ), time_to_tick( 0 ), time_to_travel( 0 ), travel_speed( 0 ),
    rank_index( -1 ), bloodlust_active( 0 ), max_haste( 0 ), haste_gain_percentage( 0.0 ),
    min_current_time( 0 ), max_current_time( 0 ),
    min_time_to_die( 0 ), max_time_to_die( 0 ),
    min_health_percentage( 0 ), max_health_percentage( 0 ),
    P400( -1 ), moving( -1 ), vulnerable( 0 ), invulnerable( 0 ), wait_on_ready( -1 ), 
    snapshot_haste( -1.0 ),
    recast( false ),
    if_expr( NULL ),
    sync_action( 0 ), observer( 0 ), next( 0 )
{
  if ( sim -> debug ) log_t::output( sim, "Player %s creates action %s", p -> name(), name() );

  if ( ! player -> initialized )
  {
    sim -> errorf( "Actions must not be created before player_t::init().  Culprit: %s %s\n", player -> name(), name() );
    sim -> cancel();
  }

  action_t** last = &( p -> action_list );
  while ( *last ) last = &( ( *last ) -> next );
  *last = this;

  for ( int i=0; i < RESULT_MAX; i++ ) rng[ i ] = 0;

  cooldown = p -> get_cooldown( n );
  dot      = p -> get_dot     ( n );

  stats = p -> get_stats( n );
  stats -> school = school;
}

// action_t::merge_options ==================================================

option_t* action_t::merge_options( std::vector<option_t>& merged_options,
                                   option_t*              options1,
                                   option_t*              options2 )
{
  if ( ! options1 ) return options2;
  if ( ! options2 ) return options1;

  merged_options.clear();

  for ( int i=0; options1[ i ].name; i++ ) merged_options.push_back( options1[ i ] );
  for ( int i=0; options2[ i ].name; i++ ) merged_options.push_back( options2[ i ] );

  option_t null_option;
  null_option.name = 0;
  merged_options.push_back( null_option );

  return &( merged_options[ 0 ] );
}

// action_t::parse_options =================================================

void action_t::parse_options( option_t*          options,
                              const std::string& options_str )
{
  option_t base_options[] =
  {
    { "P400",                   OPT_BOOL,   &P400                  },
    { "bloodlust",              OPT_BOOL,   &bloodlust_active      },
    { "haste<",                 OPT_FLT,    &max_haste             },
    { "haste_gain_percentage>", OPT_FLT,    &haste_gain_percentage },
    { "health_percentage<",     OPT_FLT,    &max_health_percentage },
    { "health_percentage>",     OPT_FLT,    &min_health_percentage },
    { "if",                     OPT_STRING, &if_expr_str           },
    { "invulnerable",           OPT_BOOL,   &invulnerable          },
    { "moving",                 OPT_BOOL,   &moving                },
    { "rank",                   OPT_INT,    &rank_index            },
    { "sync",                   OPT_STRING, &sync_str              },
    { "time<",                  OPT_FLT,    &max_current_time      },
    { "time>",                  OPT_FLT,    &min_current_time      },
    { "time_to_die<",           OPT_FLT,    &max_time_to_die       },
    { "time_to_die>",           OPT_FLT,    &min_time_to_die       },
    { "travel_speed",           OPT_FLT,    &travel_speed          },
    { "vulnerable",             OPT_BOOL,   &vulnerable            },
    { "wait_on_ready",          OPT_BOOL,   &wait_on_ready         },
    { NULL,                     0,          NULL                   }
  };

  std::vector<option_t> merged_options;
  merge_options( merged_options, options, base_options );

  std::string::size_type cut_pt = options_str.find_first_of( ":" );

  std::string options_buffer;
  if ( cut_pt != options_str.npos )
  {
    options_buffer = options_str.substr( cut_pt + 1 );
  }
  else options_buffer = options_str;

  if ( options_buffer.empty()     ) return;
  if ( options_buffer.size() == 0 ) return;

  if ( ! option_t::parse( sim, name(), merged_options, options_buffer ) )
  {
    sim -> errorf( "%s %s: Unable to parse options str '%s'.\n", player -> name(), name(), options_str.c_str() );
    sim -> cancel();
  }
}

// action_t::init_rank ======================================================

rank_t* action_t::init_rank( rank_t* rank_list,
                             int     id_override )
{
  if ( resource == RESOURCE_MANA )
  {
    for ( int i=0; rank_list[ i ].level; i++ )
    {
      rank_t& r = rank_list[ i ];

      // Look for ranks in which the cost of an action is a percentage of base mana
      if ( r.cost > 0 && r.cost < 1 )
      {
        r.cost *= player -> resource_base[ RESOURCE_MANA ];
      }
    }
  }

  for ( int i=0; rank_list[ i ].level; i++ )
  {
    if ( ( rank_index <= 0 && player -> level >= rank_list[ i ].level ) ||
         ( rank_index >  0 &&      rank_index == rank_list[ i ].index  ) )
    {
      rank_t* rank = &( rank_list[ i ] );

      base_dd_min  = rank -> dd_min;
      base_dd_max  = rank -> dd_max;
      base_td_init = rank -> tick;
      base_cost    = rank -> cost;
      id           = id_override ? id_override : 0; //FIXME! rank -> id;

      return rank;
    }
  }

  sim -> errorf( "%s unable to find valid rank for %s\n", player -> name(), name() );
  sim -> cancel();

  return 0;
}

// action_t::cost ======================================================

double action_t::cost() SC_CONST
{
  double c = base_cost;

  if ( resource == RESOURCE_MANA )
  {
    if ( player -> buffs.power_infusion -> check() ) c *= 0.80;
  }

  if ( sim -> debug ) log_t::output( sim, "action_t::cost: %s %.2f %.2f %s", name(), base_cost, c, util_t::resource_type_string( resource ) );

  return floor( c );
}

// action_t::travel_time =====================================================

double action_t::travel_time()
{
  if ( travel_speed == 0 ) return 0;

  if ( player -> distance == 0 ) return 0;

  double t = player -> distance / travel_speed;

  double v = sim -> travel_variance;

  if ( v )
  {
    if ( ! rng_travel )
    {
      std::string buffer = name_str + "_travel";
      rng_travel = player -> get_rng( buffer, RNG_DISTRIBUTED );
    }
    t = rng_travel -> gauss( t, v );
  }

  return t;
}

// action_t::player_buff =====================================================

void action_t::player_buff()
{
  player_multiplier              = 1.0;
  player_hit                     = 0;
  player_crit                    = 0;
  player_crit_multiplier         = 1.0;
  player_crit_bonus_multiplier   = 1.0;
  player_penetration             = 0;
  player_dd_adder                = 0;
  player_spell_power             = 0;
  player_attack_power            = 0;
  player_spell_power_multiplier  = 1.0;
  player_attack_power_multiplier = 1.0;

  player_t* p = player;

  if ( school == SCHOOL_BLEED )
  {
    // Not applicable
  }
  else if ( school == SCHOOL_PHYSICAL )
  {
    player_penetration = p -> composite_attack_penetration();
  }
  else
  {
    player_penetration = p -> composite_spell_penetration();
  }

  player_multiplier = p -> composite_player_multiplier( school );

  if ( base_attack_power_multiplier > 0 )
  {
    player_attack_power            = p -> composite_attack_power();
    player_attack_power_multiplier = p -> composite_attack_power_multiplier();
  }

  if ( base_spell_power_multiplier > 0 )
  {
    player_spell_power            = p -> composite_spell_power( school );
    player_spell_power_multiplier = p -> composite_spell_power_multiplier();
  }

  if ( sim -> debug )
    log_t::output( sim, "action_t::player_buff: %s hit=%.2f crit=%.2f penetration=%.0f spell_power=%.2f attack_power=%.2f ",
                   name(), player_hit, player_crit, player_penetration, player_spell_power, player_attack_power );
}

// action_t::target_debuff ==================================================

void action_t::target_debuff( int dmg_type )
{
  target_multiplier            = 1.0;
  target_hit                   = 0;
  target_crit                  = 0;
  target_crit_multiplier       = 1.0;
  target_crit_bonus_multiplier = 1.0;
  target_attack_power          = 0;
  target_spell_power           = 0;
  target_penetration           = 0;
  target_dd_adder              = 0;

  player_t* p = player;
  target_t* t = sim -> target;

  if ( school == SCHOOL_PHYSICAL ||
       school == SCHOOL_BLEED    )
  {
    if ( t -> debuffs.blood_frenzy  -> up() ||
         t -> debuffs.savage_combat -> up() )
    {
      target_multiplier *= 1.04;
    }
  }
  else
  {
    target_multiplier *= 1.0 + ( std::max( t -> debuffs.curse_of_elements  -> value(), 
                                 std::max( t -> debuffs.earth_and_moon     -> value(),
                                           t -> debuffs.ebon_plaguebringer -> value() ) ) * 0.01 );

    if ( t -> debuffs.curse_of_elements -> check() ) target_penetration += 88;
  }

  if ( school == SCHOOL_BLEED )
  {
    if ( t -> debuffs.mangle -> up() || t -> debuffs.trauma -> up() )
    {
      target_multiplier *= 1.30;
    }
  }

  if ( school == SCHOOL_PHYSICAL )
  {
    target_dd_adder += t -> debuffs.hemorrhage -> value();
  }

  // FIXME! HotC and MP are 1%/2%/3%
  if ( t -> debuffs.heart_of_the_crusader -> up() ||
       t -> debuffs.totem_of_wrath        -> up() ||
       t -> debuffs.master_poisoner       -> up() )
  {
    target_crit += 0.03;
  }

  if ( base_attack_power_multiplier > 0 )
  {
    if ( p -> position == POSITION_RANGED )
    {
      target_attack_power += t -> debuffs.hunters_mark -> value();
    }
  }
  if ( base_spell_power_multiplier > 0 )
  {
    // no spell power based debuffs at this time
  }

  if ( t -> debuffs.vulnerable -> up() ) target_multiplier *= 2.0;

  if ( t -> debuffs.winters_grasp -> up() ) target_hit += 0.02;

  if ( t -> resilience > 0 )
  {
  	double percent_resil = t -> resilience / 94.3;
	double crit_chance_reduce = percent_resil * 0.01;
	double crit_damage_reduce = percent_resil * 2.2 * 0.01;
	double damage_reduce = percent_resil * 2 * 0.01;
	target_crit -= crit_chance_reduce;
	target_crit_multiplier *= 1 - crit_damage_reduce;
	target_multiplier *= 1 - damage_reduce;
  }

  if ( sim -> debug )
    log_t::output( sim, "action_t::target_debuff: %s multiplier=%.2f hit=%.2f crit=%.2f attack_power=%.2f spell_power=%.2f penetration=%.0f",
                   name(), target_multiplier, target_hit, target_crit, target_attack_power, target_spell_power, target_penetration );
}

// action_t::result_is_hit ==================================================

bool action_t::result_is_hit() SC_CONST
{
  return( result == RESULT_HIT    ||
          result == RESULT_CRIT   ||
          result == RESULT_GLANCE ||
          result == RESULT_BLOCK  ||
          result == RESULT_NONE   );
}

// action_t::result_is_miss =================================================

bool action_t::result_is_miss() SC_CONST
{
  return( result == RESULT_MISS   ||
  result == RESULT_DODGE  ||
  result == RESULT_RESIST );
}

// action_t::armor ==========================================================

double action_t::armor() SC_CONST
{
  target_t* t = sim -> target;

  double adjusted_armor =  t -> base_armor();

  adjusted_armor *= 1.0 - std::max( t -> debuffs.sunder_armor -> value(), 
                                    t -> debuffs.expose_armor -> value() );

  if( t -> debuffs.faerie_fire -> up() ) adjusted_armor *= 0.95;

  return adjusted_armor;
}

// action_t::resistance =====================================================

double action_t::resistance() SC_CONST
{
  if ( ! may_resist ) return 0;

  target_t* t = sim -> target;
  double resist=0;

  double penetration = base_penetration + player_penetration + target_penetration;

  if ( school == SCHOOL_BLEED )
  {
    // Bleeds cannot be resisted
  }
  else if ( school == SCHOOL_PHYSICAL )
  {
    double half_reduction = 400 + 85.0 * ( player -> level + 4.5 * ( player -> level - 59 ) );
    double reduced_armor = armor();
    double penetration_max = std::min( reduced_armor, ( reduced_armor + half_reduction ) / 3.0 );

    if ( penetration > 1 ) penetration = 1;

    double adjusted_armor = reduced_armor - penetration_max * penetration;

    if ( adjusted_armor <= 0 )
    {
      resist = 0.0;
    }
    else
    {
      resist = adjusted_armor / ( adjusted_armor + half_reduction );
    }
  }
  else
  {
    double player_skill = player -> level * 5.0;
    double resist_rating = t -> spell_resistance[ school ];

    if ( school == SCHOOL_FROSTFIRE )
    {
      resist_rating = std::min( t -> spell_resistance[ SCHOOL_FROST ],
                                t -> spell_resistance[ SCHOOL_FIRE  ] );
    }

    resist_rating -= penetration;
    if ( resist_rating < 0 ) resist_rating = 0;
    if ( resist_rating > 0 ) resist = resist_rating / player_skill;

    if ( ! binary )
    {
      int delta_level = t -> level - player -> level;
      if ( delta_level > 0 )
      {
        resist += delta_level * 0.02;
      }
    }

    if ( resist > 1.0 ) resist = 1.0;
  }

  if ( sim -> debug ) log_t::output( sim, "%s queries resistance for %s: %.2f", player -> name(), name(), resist );

  return resist;
}

// action_t::total_crit_bonus ================================================

double action_t::total_crit_bonus() SC_CONST
{
  double crit_multiplier = (   base_crit_multiplier *
                             player_crit_multiplier *
                             target_crit_multiplier );

  double crit_bonus_multiplier = (   base_crit_bonus_multiplier *
                                   player_crit_bonus_multiplier *
                                   target_crit_bonus_multiplier );

  double crit_bonus = ( ( 1.0 + base_crit_bonus ) * crit_multiplier - 1.0 ) * crit_bonus_multiplier;

  if ( sim -> debug )
  {
    log_t::output( sim, "%s crit_bonus for %s: cb=%.3f b_cb=%.2f b_cm=%.2f p_cm=%.2f t_cm=%.2f b_cbm=%.2f p_cbm=%.2f t_cbm=%.2f",
                   player -> name(), name(), crit_bonus, base_crit_bonus,
                   base_crit_multiplier,       player_crit_multiplier,       target_crit_multiplier,
                   base_crit_bonus_multiplier, player_crit_bonus_multiplier, target_crit_bonus_multiplier );
  }

  return crit_bonus;
}

// action_t::total_power =====================================================

double action_t::total_power() SC_CONST
{
  double power=0;

  if ( base_spell_power_multiplier  > 0 ) power += total_spell_power();
  if ( base_attack_power_multiplier > 0 ) power += total_attack_power();

  return power;
}

// action_t::calculate_weapon_damage =========================================

double action_t::calculate_weapon_damage()
{
  if ( ! weapon || weapon_multiplier <= 0 ) return 0;

  double dmg = sim -> range( weapon -> min_dmg, weapon -> max_dmg );

  double weapon_damage = normalize_weapon_damage ? dmg * 2.8 / weapon -> swing_time : dmg;
  double weapon_speed  = normalize_weapon_speed  ? weapon -> normalized_weapon_speed() : weapon -> swing_time;

  double power_damage = weapon_speed * weapon_power_mod * total_attack_power();

  return ( weapon_damage + power_damage );
}

// action_t::calculate_tick_damage ===========================================

double action_t::calculate_tick_damage()
{
  tick_dmg = resisted_dmg = blocked_dmg = 0;

  if ( base_td == 0 ) base_td = base_td_init;

  if ( base_td == 0 ) return 0;

  tick_dmg  = base_td + total_power() * tick_power_mod;
  tick_dmg *= total_td_multiplier();

  double init_tick_dmg = tick_dmg;

  if ( result == RESULT_CRIT )
  {
    tick_dmg *= 1.0 + total_crit_bonus();
  }

  if ( ! binary )
  {
    resisted_dmg = resistance() * tick_dmg;
    tick_dmg -= resisted_dmg;
  }

  if ( sim -> debug )
  {
    log_t::output( sim, "%s dmg for %s: td=%.0f i_td=%.0f b_td=%.0f mod=%.2f power=%.0f b_mult=%.2f p_mult=%.2f t_mult=%.2f",
                   player -> name(), name(), tick_dmg, init_tick_dmg, base_td, tick_power_mod,
                   total_power(), base_multiplier * base_td_multiplier, player_multiplier, target_multiplier );
  }

  return tick_dmg;
}

// action_t::calculate_direct_damage =========================================

double action_t::calculate_direct_damage()
{
  direct_dmg = resisted_dmg = blocked_dmg = 0;

  double base_direct_dmg = sim -> range( base_dd_min, base_dd_max );

  if ( base_direct_dmg == 0 ) return 0;
  
  direct_dmg  = base_direct_dmg + base_dd_adder + player_dd_adder + target_dd_adder;
  if ( weapon_multiplier > 0 )
  {
    // x% weapon damage + Y
    // e.g. Obliterate, Shred, Backstab
    direct_dmg += calculate_weapon_damage();
    direct_dmg *= weapon_multiplier;
    
    // OH penalty
    if ( weapon && weapon -> slot == SLOT_OFF_HAND )
      direct_dmg *= 0.5;
  }
  direct_dmg += direct_power_mod * total_power();
  direct_dmg *= total_dd_multiplier();

  double init_direct_dmg = direct_dmg;

  if ( result == RESULT_GLANCE )
  {
    double delta_skill = ( sim -> target -> level - player -> level ) * 5.0;

    if ( delta_skill < 0.0 )
      delta_skill = 0.0;

    double max_glance = 1.3 - 0.03 * delta_skill;

    if ( max_glance > 0.99 ) 
      max_glance = 0.99;
    else if ( max_glance < 0.2 )
      max_glance = 0.20;

    double min_glance = 1.4 - 0.05 * delta_skill;

    if ( min_glance > 0.91 )
      min_glance = 0.91;
    else if ( min_glance < 0.01 )
      min_glance = 0.01;

    if ( min_glance > max_glance )
    {
      double temp = min_glance;
      min_glance = max_glance;
      max_glance = temp;
    }

    direct_dmg *= sim -> range( min_glance, max_glance ); // 0.75 against +3 targets.
  }
  else if ( result == RESULT_CRIT )
  {
    direct_dmg *= 1.0 + total_crit_bonus();
  }

  if ( ! binary )
  {
    resisted_dmg = resistance() * direct_dmg;
    direct_dmg -= resisted_dmg;
  }

  if ( result == RESULT_BLOCK )
  {
    blocked_dmg = sim -> target -> block_value;
    direct_dmg -= blocked_dmg;
    if ( direct_dmg < 0 ) direct_dmg = 0;
  }

  if ( sim -> debug )
  {
    log_t::output( sim, "%s dmg for %s: dd=%.0f i_dd=%.0f b_dd=%.0f mod=%.2f power=%.0f b_mult=%.2f p_mult=%.2f t_mult=%.2f",
                   player -> name(), name(), direct_dmg, init_direct_dmg, base_direct_dmg, direct_power_mod,
                   total_power(), base_multiplier * base_dd_multiplier, player_multiplier, target_multiplier );
  }

  return direct_dmg;
}

// action_t::consume_resource ===============================================

void action_t::consume_resource()
{
  resource_consumed = cost();

  if ( sim -> debug )
    log_t::output( sim, "%s consumes %.1f %s for %s", player -> name(),
                   resource_consumed, util_t::resource_type_string( resource ), name() );

  player -> resource_loss( resource, resource_consumed );

  stats -> consume_resource( resource_consumed );
}

// action_t::execute ========================================================

void action_t::execute()
{
  if ( sim -> log && ! dual ) 
  {
    log_t::output( sim, "%s performs %s (%.0f)", player -> name(), name(), 
                   player -> resource_current[ player -> primary_resource() ] );
  }

  if ( observer ) *observer = 0;

  player_buff();

  target_debuff( DMG_DIRECT );

  calculate_result();

  consume_resource();

  if ( result_is_hit() )
  {
    calculate_direct_damage();

    if ( direct_dmg > 0 )
    {
      assess_damage( direct_dmg, DMG_DIRECT );
    }
    if ( num_ticks > 0 )
    {
      if ( dot_behavior == DOT_REFRESH )
      {
        current_tick = 0;
        if ( ! ticking ) schedule_tick();
      }
      else
      {
        if ( ticking ) cancel();
        snapshot_haste = haste();
        schedule_tick();
      }
    }
  }
  else
  {
    if ( sim -> log )
    {
      log_t::output( sim, "%s avoids %s (%s)", sim -> target -> name(), name(), util_t::result_type_string( result ) );
      log_t::miss_event( this );
    }
  }

  update_ready();

  if ( ! dual ) update_stats( DMG_DIRECT );

  schedule_travel();

  if ( repeating && ! proc ) schedule_execute();

  if ( harmful ) player -> in_combat = true;
}

// action_t::tick ===========================================================

void action_t::tick()
{
  if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );

  result = RESULT_HIT;

  // Older tests indicated that crit debuffs are calculated at the time of the cast, not the ticks.
  // It's possible that this has now changed, but would require testing to be certain.
  double save_target_crit = target_crit;

  target_debuff( DMG_OVER_TIME );

  target_crit = save_target_crit;

  if ( tick_may_crit )
  {
    int delta_level = sim -> target -> level - player -> level;
    
    if ( rng[ RESULT_CRIT ] -> roll( crit_chance( delta_level ) ) )
    {
      result = RESULT_CRIT;
      action_callback_t::trigger( player -> spell_result_callbacks[ RESULT_CRIT ], this );
      if ( channeled )
      {
        action_callback_t::trigger( player -> spell_direct_result_callbacks[ RESULT_CRIT ], this );
      }
    }
  }

  calculate_tick_damage();

  assess_damage( tick_dmg, DMG_OVER_TIME );

  action_callback_t::trigger( player -> tick_callbacks, this );

  update_stats( DMG_OVER_TIME );
}

// action_t::last_tick =======================================================

void action_t::last_tick()
{
  if ( sim -> debug ) log_t::output( sim, "%s fades from %s", name(), sim -> target -> name() );

  ticking = 0;
  time_to_tick = 0;

  if ( school == SCHOOL_BLEED ) sim -> target -> debuffs.bleeding -> decrement();

  if ( observer ) *observer = 0;
}

// action_t::travel ==========================================================

void action_t::travel( int    travel_result,
                       double travel_dmg )
{
  if ( sim -> log )
    log_t::output( sim, "%s from %s arrives on target (%s for %.0f)",
                   name(), player -> name(),
                   util_t::result_type_string( travel_result ), travel_dmg );

  // FIXME! Damage still applied at execute().  This is just a place to model travel-time effects.
}

// action_t::assess_damage ==================================================

void action_t::assess_damage( double amount,
                              int    dmg_type )
{
  sim -> target -> assess_damage( amount, school, dmg_type );

  if ( dmg_type == DMG_DIRECT )
  {
    if ( sim -> log )
    {
      log_t::output( sim, "%s %s hits %s for %.0f %s damage (%s)",
                     player -> name(), name(),
                     sim -> target -> name(), amount,
                     util_t::school_type_string( school ),
                     util_t::result_type_string( result ) );
      log_t::damage_event( this, amount, dmg_type );
    }

    action_callback_t::trigger( player -> direct_damage_callbacks[ school ], this );
  }
  else // DMG_OVER_TIME
  {
    if ( sim -> log )
    {
      log_t::output( sim, "%s %s ticks (%d of %d) %s for %.0f %s damage (%s)",
                     player -> name(), name(),
                     current_tick, num_ticks,
                     sim -> target -> name(), amount,
                     util_t::school_type_string( school ),
                     util_t::result_type_string( result ) );
      log_t::damage_event( this, amount, dmg_type );
    }

    action_callback_t::trigger( player -> tick_damage_callbacks[ school ], this );
  }

  if ( aoe && sim -> target -> adds_nearby )
  {
    for ( int i=0; i < sim -> target -> adds_nearby && i < 9; i++ )
    {
      additional_damage( amount, dmg_type );
    }
  }
}

// action_t::additional_damage =============================================

void action_t::additional_damage( double amount,
                                  int    dmg_type )
{
  amount /= target_multiplier; // FIXME! Weak lip-service to the fact that the adds probably will not be properly debuffed.
  sim -> target -> assess_damage( amount, school, dmg_type );
  stats -> add_result( amount, dmg_type, result );
}

// action_t::schedule_execute ==============================================

void action_t::schedule_execute()
{
  if ( sim -> log )
  {
    log_t::output( sim, "%s schedules execute for %s", player -> name(), name() );
    log_t::start_event( this );
  }

  time_to_execute = execute_time();

  execute_event = new ( sim ) action_execute_event_t( sim, this, time_to_execute );

  if ( observer ) *observer = this;

  if ( ! background )
  {
    player -> executing = this;
    player -> gcd_ready = sim -> current_time + gcd();
    if( player -> action_queued )
    {
      player -> gcd_ready -= sim -> queue_gcd_reduction;
    }
  }
  if ( special && execute_time() > 0 && ! proc )
  {
    // While an ability is casting, the auto_attack is paused
    // So we simply reschedule the auto_attack by the ability's casttime
    double time_to_next_hit;
    // Mainhand
    if ( player -> main_hand_attack )
    {
      time_to_next_hit  = player -> main_hand_attack -> execute_event -> occurs();
      time_to_next_hit -= sim -> current_time;
      time_to_next_hit += execute_time();
      player -> main_hand_attack -> execute_event -> reschedule( time_to_next_hit );
    }
    // Offhand
    if ( player -> off_hand_attack )
    {
      time_to_next_hit  = player -> off_hand_attack -> execute_event -> occurs();
      time_to_next_hit -= sim -> current_time;
      time_to_next_hit += execute_time();
      player -> off_hand_attack -> execute_event -> reschedule( time_to_next_hit );
    }
  }
}

// action_t::schedule_tick =================================================

void action_t::schedule_tick()
{
  if ( sim -> debug ) log_t::output( sim, "%s schedules tick for %s", player -> name(), name() );

  if ( current_tick == 0 )
  {
    if ( school == SCHOOL_BLEED ) sim -> target -> debuffs.bleeding -> increment();

    if ( tick_zero )
    {
      time_to_tick = 0;
      tick();
    }
  }

  ticking = 1;

  if ( snapshot_haste <= 0.0 )
  {
    snapshot_haste = haste();
  }

  time_to_tick = tick_time();

  tick_event = new ( sim ) action_tick_event_t( sim, this, time_to_tick );

  if ( channeled ) player -> channeling = this;

  if ( observer ) *observer = this;
}

// action_t::schedule_travel ===============================================

void action_t::schedule_travel()
{
  time_to_travel = travel_time();

  if ( time_to_travel == 0 ) return;

  if ( sim -> log )
  {
    log_t::output( sim, "%s schedules travel for %s", player -> name(), name() );
  }

  new ( sim ) action_travel_event_t( sim, this, time_to_travel );
}

// action_t::reschedule_execute ============================================

void action_t::reschedule_execute( double time )
{
  if ( sim -> log )
  {
    log_t::output( sim, "%s reschedules execute for %s", player -> name(), name() );
  }

  double delta_time = sim -> current_time + time - execute_event -> occurs();

  time_to_execute += delta_time;

  if ( delta_time > 0 )
  {
    execute_event -> reschedule( time );
  }
  else // Impossible to reschedule events "early".  Need to be canceled and re-created.
  {
    event_t::cancel( execute_event );
    execute_event = new ( sim ) action_execute_event_t( sim, this, time );
  }
}

// action_t::refresh_duration ================================================

void action_t::refresh_duration()
{
  if ( sim -> debug ) log_t::output( sim, "%s refreshes duration of %s", player -> name(), name() );

  // Make sure this DoT is still ticking......
  assert( tick_event );

  // Recalculate player power
  if ( base_attack_power_multiplier > 0 )
  {
    player_attack_power            = player -> composite_attack_power();
    player_attack_power_multiplier = player -> composite_attack_power_multiplier();
  }
  if ( base_spell_power_multiplier > 0 )
  {
    player_spell_power            = player -> composite_spell_power( school );
    player_spell_power_multiplier = player -> composite_spell_power_multiplier();
  }

  if ( dot_behavior == DOT_WAIT )
  {
    // Refreshing a DoT does not interfere with the next tick event.  Ticks will stil occur
    // every "base_tick_time" seconds.  To determine the new finish time for the DoT, start
    // from the time of the next tick and add the time for the remaining ticks to that event.

    double duration = tick_event -> time + tick_time() * ( num_ticks - 1 );

    dot -> start( this, duration );
  }

  current_tick = 0;
}

// action_t::extend_duration =================================================

void action_t::extend_duration( int extra_ticks )
{
  num_ticks += extra_ticks;
  added_ticks += extra_ticks;

  if ( dot_behavior == DOT_WAIT )
  {
    dot -> ready += tick_time() * extra_ticks;
  }

  if ( sim -> debug ) log_t::output( sim, "%s extends duration of %s, adding %d tick(s), totalling %d ticks", player -> name(), name(), extra_ticks, num_ticks );
}

// action_t::update_ready ====================================================

void action_t::update_ready()
{
  if ( cooldown -> duration > 0 && ! dual )
  {
    if ( sim -> debug ) log_t::output( sim, "%s starts cooldown for %s (%s)", player -> name(), name(), cooldown -> name() );

    cooldown -> start();
  }
  if ( num_ticks && ( dot_behavior == DOT_WAIT ) )
  {
    if ( ticking && ! channeled )
    {
      assert( num_ticks && tick_event );

      int remaining_ticks = num_ticks - current_tick;

      double next_tick = tick_event -> occurs() - sim -> current_time;

      double duration = 0.01 + next_tick + tick_time() * ( remaining_ticks - 1 );

      if ( sim -> debug )
        log_t::output( sim, "%s shares duration (%.2f) for %s (%s)", 
                       player -> name(), duration, name(), dot -> name() );

      dot -> start( this, duration );
    }
    else if( result_is_miss() )
    {
      if ( sim -> debug ) 
        log_t::output( sim, "%s pushes out re-cast (%.2f) on miss for %s (%s)", 
                       player -> name(), sim -> reaction_time, name(), dot -> name() );

      dot -> start( this, sim -> reaction_time );
    }
  }
}

// action_t::update_stats ===================================================

void action_t::update_stats( int type )
{
  if ( type == DMG_DIRECT )
  {
    stats -> add( direct_dmg, type, result, time_to_execute );
  }
  else if ( type == DMG_OVER_TIME )
  {
    stats -> add( tick_dmg, type, result, time_to_tick );
  }
  else assert( 0 );
}

// action_t::update_result ==================================================

void action_t::update_result( int type )
{
  if ( type == DMG_DIRECT )
  {
    stats -> add_result( direct_dmg, type, result );
  }
  else if ( type == DMG_OVER_TIME )
  {
    stats -> add_result( tick_dmg, type, result );
  }
  else assert( 0 );
}

// action_t::update_time ===================================================

void action_t::update_time( int type )
{
  if ( type == DMG_DIRECT )
  {
    stats -> add_time( time_to_execute, type );
  }
  else if ( type == DMG_OVER_TIME )
  {
    stats -> add_time( time_to_tick, type );
  }
  else assert( 0 );
}

// action_t::ready ==========================================================

bool action_t::ready()
{
  target_t* t = sim -> target;

  if ( player -> skill < 1.0 )
    if ( ! sim -> roll( player -> skill ) )
      return false;

  bool check_duration = true;

  if ( ( scale_ticks_with_haste() > 0 ) && ( haste_gain_percentage > 0.0 ) && ( ticking ) )
  {
    check_duration = ( ( snapshot_haste / haste() - 1.0 ) * 100.0 ) <= haste_gain_percentage;
  }

  if ( recast )
  {
    check_duration = false;
  }

  if ( check_duration )
  {
    double remains = dot -> remains();

    if ( remains > 0 )
    {
      double delta = remains - execute_time();

      if ( delta > 3.0 )
        return false;

      if ( delta > 0 && sim -> roll( player -> skill ) )
        return false;
    }
  }

  if ( cooldown -> remains() > 0 )
    return false;

  if ( ! player -> resource_available( resource, cost() ) )
    return false;

  if ( min_current_time > 0 )
    if ( sim -> current_time < min_current_time )
      return false;

  if ( max_current_time > 0 )
    if ( sim -> current_time > max_current_time )
      return false;

  if ( min_time_to_die > 0 )
    if ( sim -> target -> time_to_die() < min_time_to_die )
      return false;

  if ( max_time_to_die > 0 )
    if ( sim -> target -> time_to_die() > max_time_to_die )
      return false;

  if ( min_health_percentage > 0 )
    if ( sim -> target -> health_percentage() < min_health_percentage )
      return false;

  if ( max_health_percentage > 0 )
    if ( sim -> target -> health_percentage() > max_health_percentage )
      return false;

  if ( max_haste > 0 )
    if ( ( ( 1.0 / haste() ) - 1.0 ) > max_haste )
      return false;

  if ( bloodlust_active > 0 )
    if ( ! player -> buffs.bloodlust -> check() )
      return false;

  if ( bloodlust_active < 0 )
    if ( player -> buffs.bloodlust -> check() )
      return false;

  if ( sync_action && ! sync_action -> ready() )
    return false;

  if ( sim -> target -> debuffs.invulnerable -> check() )
    if ( harmful )
      return false;

  if ( player -> buffs.moving -> check() )
    if ( channeled || ( range == 0 ) || ( execute_time() > 0 ) )
      return false;

  if ( P400 != -1 )
    if ( P400 != sim -> P400 )
      return false;

  if ( moving != -1 )
    if ( moving != ( player -> buffs.moving -> check() ? 1 : 0 ) )
      return false;

  if ( vulnerable )
    if ( ! t -> debuffs.vulnerable -> check() )
      return false;

  if ( invulnerable )
    if ( ! t -> debuffs.invulnerable -> check() )
      return false;

  if ( if_expr )
  {
    int result_type = if_expr -> evaluate();
    if ( result_type == TOK_NUM     ) return if_expr -> result_num != 0;
    if ( result_type == TOK_STR     ) return true;
    if ( result_type == TOK_UNKNOWN ) return false;
  }

  return true;
}

// action_t::reset ==========================================================

void action_t::reset()
{
  if( ! rng[ 0 ] )
  {
    std::string buffer;
    for ( int i=0; i < RESULT_MAX; i++ )
    {
      buffer  = name();
      buffer += "_";
      buffer += util_t::result_type_string( i );
      rng[ i ] = player -> get_rng( buffer, ( ( i == RESULT_CRIT ) ? RNG_DISTRIBUTED : RNG_CYCLIC ) );
    }
  }

  if ( ! sync_str.empty() && ! sync_action )
  {
    sync_action = player -> find_action( sync_str );

    if ( ! sync_action )
    {
      sim -> errorf( "Unable to find sync action '%s' for primary action '%s'\n", sync_str.c_str(), name() );
      sim -> cancel();
    }
  }

  ticking = 0;
  current_tick = 0;
  added_ticks = 0;
  cooldown -> reset();
  dot -> reset();
  result = RESULT_NONE;
  execute_event = 0;
  tick_event = 0;
  if ( observer ) *observer = 0;

  if ( ! dual ) stats -> reset( this );
}

// action_t::cancel =========================================================

void action_t::cancel()
{
  if ( ticking ) last_tick();

  if ( player -> executing  == this ) player -> executing  = 0;
  if ( player -> channeling == this ) player -> channeling = 0;

  event_t::cancel( execute_event );
  event_t::cancel(    tick_event );

  ticking = 0;
  current_tick = 0;
  added_ticks = 0;
  cooldown -> reset();
  dot -> reset();
  execute_event = 0;
  tick_event = 0;
  snapshot_haste = -1.0;
  if ( observer ) *observer = 0;
}

// action_t::check_talent ===================================================

void action_t::check_talent( int talent_rank )
{
  if ( talent_rank != 0 ) return;

  if ( player -> is_pet() )
  {
    pet_t* p = player -> cast_pet();
    sim -> errorf( "Player %s has pet %s attempting to execute action %s without the required talent.\n", 
		   p -> owner -> name(), p -> name(), name() );
  }
  else
  {
    sim -> errorf( "Player %s attempting to execute action %s without the required talent.\n", player -> name(), name() );
  }

  background = true; // prevent action from being executed
}

// action_t::create_expression ==============================================

action_expr_t* action_t::create_expression( const std::string& name_str )
{
  if ( name_str == "ticking" )
  {
    struct ticking_expr_t : public action_expr_t
    {
      ticking_expr_t( action_t* a ) : action_expr_t( a, "ticking", TOK_NUM ) {}
      virtual int evaluate() { result_num = ( action -> dot -> ticking() ? 1 : 0 ); return TOK_NUM; }
    };
    return new ticking_expr_t( this );
  }
  if ( name_str == "ticks" )
  {
    struct ticks_expr_t : public action_expr_t
    {
      ticks_expr_t( action_t* a ) : action_expr_t( a, "ticks", TOK_NUM ) {}
      virtual int evaluate() { result_num = ( action -> dot -> ticking() ? action -> dot -> action -> current_tick : 0 ); return TOK_NUM; }
    };
    return new ticks_expr_t( this );
  }
  if ( name_str == "ticks_remain" )
  {
    struct ticks_remain_expr_t : public action_expr_t
    {
      ticks_remain_expr_t( action_t* a ) : action_expr_t( a, "ticks_remain", TOK_NUM ) {}
      virtual int evaluate() { action_t* a = action -> dot -> action; result_num = ( action -> dot -> ticking() ? ( a -> num_ticks - a -> current_tick ) : 0 ); return TOK_NUM; }
    };
    return new ticks_remain_expr_t( this );
  }
  if ( name_str == "remains" )
  {
    struct remains_expr_t : public action_expr_t
    {
      remains_expr_t( action_t* a ) : action_expr_t( a, "remains", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> dot -> remains(); return TOK_NUM; }
    };
    return new remains_expr_t( this );
  }
  if ( name_str == "cast_time" )
  {
    struct cast_time_expr_t : public action_expr_t
    {
      cast_time_expr_t( action_t* a ) : action_expr_t( a, "cast_time", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> execute_time(); return TOK_NUM; }
    };
    return new cast_time_expr_t( this );
  }

  return player -> create_expression( this, name_str );
}

// action_t::ppm_proc_chance ================================================

double action_t::ppm_proc_chance( double PPM ) SC_CONST
{
  if ( weapon )
  {
    return weapon -> proc_chance_on_swing( PPM, time_to_execute );
  }
  else
  {
    double time = channeled ? time_to_tick : time_to_execute;

    if ( time == 0 ) time = player -> base_gcd;

    return( PPM * time / 60.0 );
  }
}

