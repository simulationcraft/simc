// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

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
    dual( false ), special( sp ), binary( false ), channeled( false ), background( false ), repeating( false ), aoe( false ), harmful( true ), proc( false ),
    may_miss( false ), may_resist( false ), may_dodge( false ), may_parry( false ),
    may_glance( false ), may_block( false ), may_crush( false ), may_crit( false ),
    tick_may_crit( false ), tick_zero( false ), clip_dot( false ),
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
    cooldown_group( n ), duration_group( n ), cooldown( 0 ), cooldown_ready( 0 ), duration_ready( 0 ),
    weapon( 0 ), weapon_multiplier( 1 ), normalize_weapon_damage( false ), normalize_weapon_speed( false ),
    rng_travel( 0 ), stats( 0 ), execute_event( 0 ), tick_event( 0 ),
    time_to_execute( 0 ), time_to_tick( 0 ), time_to_travel( 0 ), travel_speed( 0 ),
    rank_index( -1 ), bloodlust_active( 0 ),
    min_current_time( 0 ), max_current_time( 0 ),
    min_time_to_die( 0 ), max_time_to_die( 0 ),
    min_health_percentage( 0 ), max_health_percentage( 0 ),
    vulnerable( 0 ), invulnerable( 0 ), wait_on_ready( -1 ), has_if_exp(-1), is_ifall(0), if_exp(NULL),
    sync_action( 0 ), observer( 0 ), next( 0 )
{
  if ( sim -> debug ) log_t::output( sim, "Player %s creates action %s", p -> name(), name() );

  if ( ! player -> initialized )
  {
    printf( "simcraft: Actions must not be created before player_t::init().  Culprit: %s %s\n", player -> name(), name() );
    assert( 0 );
  }

  action_t** last = &( p -> action_list );
  while ( *last ) last = &( ( *last ) -> next );
  *last = this;

  std::string buffer;
  for ( int i=0; i < RESULT_MAX; i++ )
  {
    buffer  = name_str;
    buffer += "_";
    buffer += util_t::result_type_string( i );
    rng[ i ] = player -> get_rng( buffer, ( ( i == RESULT_CRIT ) ? RNG_DISTRIBUTED : RNG_CYCLIC ) );
  }

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
  std::string if_all;
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
      { "bloodlust",          OPT_BOOL,   &bloodlust_active      },
      { "travel_speed",       OPT_FLT,    &travel_speed          },
      { "vulnerable",         OPT_BOOL,   &vulnerable            },
      { "invulnerable",       OPT_BOOL,   &invulnerable          },
      { "wait_on_ready",      OPT_BOOL,   &wait_on_ready         },
      { "if_buff",            OPT_STRING, &if_expression         },
      { "if",                 OPT_STRING, &if_expression         },
      { "allow_early_recast", OPT_INT,    &is_ifall              },
      { "allow_early_cast",   OPT_INT,    &is_ifall              },
      { NULL }
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

  if( ! option_t::parse( sim, name(), merged_options, options_buffer ) )
  {
    fprintf( sim -> output_file, "action_t: %s: Unable to parse options str '%s'.\n", name(), options_str.c_str() );
    assert( false );
  }
  if (if_all!=""){
    is_ifall=1;
    if (if_expression=="") if_expression=if_all;
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

  fprintf( sim -> output_file, "%s unable to find valid rank for %s\n", player -> name(), name() );
  assert( 0 );
  return 0;
}

// action_t::cost ======================================================

double action_t::cost()
{
  double c = base_cost;

  if ( resource == RESOURCE_MANA )
  {
    c -= player -> buffs.mana_cost_reduction;
    if ( c < 0 ) c = 0;

    if ( player -> buffs.power_infusion ) c *= 0.80;
  }

  if ( sim -> debug ) log_t::output( sim, "action_t::cost: %s %.2f %.2f %s", name(), base_cost, c, util_t::resource_type_string( resource ) );

  return c;
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

  if ( p -> type != PLAYER_GUARDIAN )
  {
    if ( school == SCHOOL_SHADOW )
    {
      // That needs to be here because shadow form affects ALL shadow damage (e.g. trinkets)
      if ( p -> buffs.shadow_form )
      {
        player_multiplier *= 1.15;
      }
    }
    else if ( p -> buffs.hysteria && school == SCHOOL_PHYSICAL )
    {
      player_multiplier *= 1.2;
    }

    if ( sim -> auras.sanctified_retribution || p -> buffs.ferocious_inspiration  )
    {
      player_multiplier *= 1.03;
    }

    if ( p -> buffs.tricks_of_the_trade )
    {
      player_multiplier *= 1.0 + p -> buffs.tricks_of_the_trade * 0.01;
    }
  }

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
    if ( t -> debuffs.blood_frenzy  ||
         t -> debuffs.savage_combat )
    {
      target_multiplier *= 1.04;
    }
    t -> uptimes.blood_frenzy  -> update( t -> debuffs.blood_frenzy  != 0 );
    t -> uptimes.savage_combat -> update( t -> debuffs.savage_combat != 0 );
  }
  else
  {
    target_multiplier *= 1.0 + ( std::max( t -> debuffs.curse_of_elements, t -> debuffs.earth_and_moon ) * 0.01 );
    if ( t -> debuffs.curse_of_elements ) target_penetration += 88;
  }

  if ( school == SCHOOL_BLEED )
  {
    if ( t -> debuffs.mangle || t -> debuffs.trauma )
    {
      target_multiplier *= 1.30;
    }
    t -> uptimes.mangle -> update( t -> debuffs.mangle != 0 );
    t -> uptimes.trauma -> update( t -> debuffs.trauma != 0 );
  }

  if ( school == SCHOOL_PHYSICAL )
  {
    target_dd_adder += t -> debuffs.hemorrhage;
  }

  if ( t -> debuffs.totem_of_wrath  ||
       t -> debuffs.master_poisoner )
  {
    target_crit += 0.03;
  }

  if ( base_attack_power_multiplier > 0 )
  {
    if ( p -> position == POSITION_RANGED )
    {
      target_attack_power += t -> debuffs.hunters_mark;
    }
  }
  if ( base_spell_power_multiplier > 0 )
  {
    // no spell power based debuffs at this time
  }

  if( t -> vulnerable ) target_multiplier *= 2.0;

  t -> uptimes.totem_of_wrath  -> update( t -> debuffs.totem_of_wrath  != 0 );
  t -> uptimes.master_poisoner -> update( t -> debuffs.master_poisoner != 0 );

  if ( t -> debuffs.winters_grasp ) target_hit += 0.02;
  t -> uptimes.winters_grasp -> update( t -> debuffs.winters_grasp != 0 );

  if ( sim -> debug )
    log_t::output( sim, "action_t::target_debuff: %s multiplier=%.2f hit=%.2f crit=%.2f attack_power=%.2f spell_power=%.2f penetration=%.0f",
                   name(), target_multiplier, target_hit, target_crit, target_attack_power, target_spell_power, target_penetration );
}

// action_t::result_is_hit ==================================================

bool action_t::result_is_hit()
{
  return( result == RESULT_HIT    ||
          result == RESULT_CRIT   ||
          result == RESULT_GLANCE ||
          result == RESULT_BLOCK  ||
          result == RESULT_NONE   );
}

// action_t::result_is_miss =================================================

bool action_t::result_is_miss()
{
  return( result == RESULT_MISS   ||
          result == RESULT_DODGE  ||
          result == RESULT_RESIST );
}

// action_t::armor ==========================================================

double action_t::armor()
{
  target_t* t = sim -> target;

  double adjusted_armor =  t -> base_armor();

  adjusted_armor *= 1.0 - std::max( t -> debuffs.sunder_armor, t -> debuffs.expose_armor );
  adjusted_armor *= 1.0 - t -> debuffs.faerie_fire;

  return adjusted_armor;
}

// action_t::resistance =====================================================

double action_t::resistance()
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

    //if ( penetration > 1 ) penetration = 1;

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
    double resist_rating = t -> spell_resistance[ school ];

    if ( school == SCHOOL_FROSTFIRE )
    {
      resist_rating = std::min( t -> spell_resistance[ SCHOOL_FROST ],
                                t -> spell_resistance[ SCHOOL_FIRE  ] );
    }

    resist_rating -= penetration;
    if ( resist_rating < 0 ) resist_rating = 0;

    resist = resist_rating / ( player -> level * 5.0 );

    if ( resist > 1.0 ) resist = 1.0;

    if ( ! binary )
    {
      int delta_level = t -> level - player -> level;
      if ( delta_level > 0 )
      {
        double level_resist = delta_level * 0.02;
        if ( level_resist > resist ) resist = level_resist;
      }
    }
  }

  if ( sim -> debug ) log_t::output( sim, "%s queries resistance for %s: %.2f", player -> name(), name(), resist );

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

double action_t::total_power()
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

  double weapon_damage = normalize_weapon_damage ? weapon -> damage * 2.8 / weapon -> swing_time : weapon -> damage;
  double weapon_speed  = normalize_weapon_speed  ? weapon -> normalized_weapon_speed() : weapon -> swing_time;

  double hand_multiplier = ( weapon -> slot == SLOT_OFF_HAND ) ? 0.5 : 1.0;

  double power_damage = weapon_speed * weapon_power_mod * total_attack_power();

  return ( weapon_damage + power_damage ) * weapon_multiplier * hand_multiplier;
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

  direct_dmg  = base_direct_dmg + base_dd_adder;
  direct_dmg += calculate_weapon_damage();
  direct_dmg += direct_power_mod * total_power();
  direct_dmg *= total_dd_multiplier();

  double init_direct_dmg = direct_dmg;

  direct_dmg += player_dd_adder + target_dd_adder;  // FIXME! Does this occur before/after crit adjustment?

  if ( result == RESULT_GLANCE )
  {
    direct_dmg *= 0.75;
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
  if ( sim -> log && ! dual ) log_t::output( sim, "%s performs %s", player -> name(), name() );

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
      if ( ticking ) cancel();
      current_tick = 0;
      added_ticks = 0;
      schedule_tick();
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

  target_debuff( DMG_OVER_TIME );

  if ( tick_may_crit )
  {
    if ( rng[ RESULT_CRIT ] -> roll( total_crit() ) )
    {
      result = RESULT_CRIT;
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

  if ( school == SCHOOL_BLEED ) sim -> target -> debuffs.bleeding--;

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
  if ( sim -> log )
  {
    log_t::output( sim, "%s %s %ss %s for %.0f %s damage (%s)",
                   player -> name(), name(),
                   util_t::dmg_type_string( dmg_type ),
                   sim -> target -> name(), amount,
                   util_t::school_type_string( school ),
                   util_t::result_type_string( result ) );
    log_t::damage_event( this, amount, dmg_type );
  }

  sim -> target -> assess_damage( amount, school, dmg_type );

  if ( dmg_type == DMG_DIRECT )
  {
    action_callback_t::trigger( player -> direct_damage_callbacks, this, &amount );
  }
  else // DMG_OVER_TIME
  {
    action_callback_t::trigger( player -> tick_damage_callbacks, this, &amount );
  }
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
    player -> gcd_ready = sim -> current_time + gcd();
    player -> executing = this;
  }
}

// action_t::schedule_tick =================================================

void action_t::schedule_tick()
{
  if ( sim -> debug ) log_t::output( sim, "%s schedules tick for %s", player -> name(), name() );

  if ( current_tick == 0 )
  {
    if ( school == SCHOOL_BLEED ) sim -> target -> debuffs.bleeding++;

    if ( tick_zero )
    {
      time_to_tick = 0;
      tick();
    }
  }

  ticking = 1;

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

// action_t::refresh_duration ================================================

void action_t::refresh_duration()
{
  if ( sim -> debug ) log_t::output( sim, "%s refreshes duration of %s", player -> name(), name() );

  // Make sure this DoT is still ticking......
  assert( tick_event );

  // Recalculate state of current player buffs.
  player_buff();

  if ( ! clip_dot )
  {
    // Refreshing a DoT does not interfere with the next tick event.  Ticks will stil occur
    // every "base_tick_time" seconds.  To determine the new finish time for the DoT, start
    // from the time of the next tick and add the time for the remaining ticks to that event.

    duration_ready = tick_event -> time + base_tick_time * ( num_ticks - 1 );
    player -> share_duration( duration_group, duration_ready );
  }

  current_tick = 0;
}

// action_t::extend_duration =================================================

void action_t::extend_duration( int extra_ticks )
{
  num_ticks += extra_ticks;
  added_ticks += extra_ticks;

  if ( ! clip_dot )
  {
    duration_ready += tick_time() * extra_ticks;
    player -> share_duration( duration_group, duration_ready );
  }

  if ( sim -> debug ) log_t::output( sim, "%s extends duration of %s, adding %d tick(s), totalling %d ticks", player -> name(), name(), extra_ticks, num_ticks );
}

// action_t::update_ready ====================================================

void action_t::update_ready()
{
  if ( cooldown > 0 )
  {
    if ( sim -> debug ) log_t::output( sim, "%s shares cooldown for %s (%s)", player -> name(), name(), cooldown_group.c_str() );

    player -> share_cooldown( cooldown_group, cooldown );
  }
  if ( ! channeled && num_ticks > 0 && ! result_is_miss() )
  {
    if ( sim -> debug ) log_t::output( sim, "%s shares duration for %s (%s)", player -> name(), name(), duration_group.c_str() );

    player -> share_duration( duration_group, sim -> current_time + 0.01 + tick_time() * num_ticks );
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

// action_t::update_time ===================================================

void action_t::update_time( int type )
{
  if ( type == DMG_DIRECT )
  {
    stats -> total_execute_time += time_to_execute;
  }
  else if ( type == DMG_OVER_TIME )
  {
    stats -> total_tick_time += time_to_tick;
  }
  else assert( 0 );
}

// action_t::ready ==========================================================

bool action_t::ready()
{
  target_t* t = sim -> target;

  if (( duration_ready > 0 )&&(!is_ifall))
    if ( duration_ready > ( sim -> current_time + execute_time() ) )
      return false;

  if ( cooldown_ready > sim -> current_time )
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

  if ( bloodlust_active > 0 )
    if ( ! player -> buffs.bloodlust )
      return false;

  if ( bloodlust_active < 0 )
    if ( player -> buffs.bloodlust )
      return false;

  if ( sync_action && ! sync_action -> ready() )
    return false;

  if( sim -> target -> invulnerable )
    if( harmful )
      return false;

  if ( player -> moving )
    if( channeled || ( range == 0 ) || ( execute_time() > 0 ) )
      return false;

  if ( vulnerable )
    if( ! t -> vulnerable )
      return false;

  if ( invulnerable )
    if( ! t -> invulnerable )
      return false;

  //initialize expression if not already done -> should be done in some init_expressions()
  if (has_if_exp<0){
    if (if_expression!="")   if_exp=act_expression_t::create(this, if_expression);
    has_if_exp= (if_exp!=0);
  }
  //check action expression if any
  if (has_if_exp) 
    if (! if_exp->ok())
      return false;

  return true;
}

// action_t::reset ==========================================================

void action_t::reset()
{
  if ( ! sync_str.empty() && ! sync_action )
  {
    sync_action = player -> find_action( sync_str );

    if ( ! sync_action )
    {
      printf( "simcraft: Unable to find sync action '%s' for primary action '%s'\n", sync_str.c_str(), name() );
      exit( 0 );
    }
  }

  ticking = 0;
  current_tick = 0;
  added_ticks = 0;
  cooldown_ready = 0;
  duration_ready = 0;
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

  reset();
}


//
// Expressions 
//

  enum exp_type { AEXP_NONE=0, 
                  AEXP_AND, AEXP_OR, AEXP_NOT, AEXP_EQ, AEXP_NEQ, AEXP_GREATER, AEXP_LESS, AEXP_GE, AEXP_LE, // these operations result in boolean
                  AEXP_PLUS, AEXP_MINUS, AEXP_MUL, AEXP_DIV, // these operations result in double
                  AEXP_VALUE, AEXP_INT_PTR, AEXP_DOUBLE_PTR, // these are "direct" value return (no operations)
                  AEXP_CUSTOM, // this one presume overriden class
                  AEXP_MAX
  };

  enum exp_func_glob { EFG_NONE=0, EFG_GCD, EFG_TIME, EFG_TTD, EFG_HP, EFG_VULN, EFG_INVUL, 
                       EFG_TICKING, EFG_CTICK,EFG_NTICKS, EFG_EXPIRE, EFG_REMAINS, EFG_TCAST, EFG_MOVING, EFG_MAX };

  // custom class to invoke pbuff_t expiration
  struct pbuff_expression: public act_expression_t{
    pbuff_t* buff;
    virtual ~pbuff_expression(){};
    virtual double evaluate() {
      return buff->expiration_time();
    }
  };

  //custom class to return global functions
  struct global_expression_t: public act_expression_t{
    action_t* action;
    virtual ~global_expression_t(){};
    virtual double evaluate() {
      switch (type){
        case EFG_GCD:        return action->gcd();  
        case EFG_TIME:       return action->sim->current_time; 
        case EFG_TTD:        return action->sim -> target ->time_to_die(); 
        case EFG_HP:         return action->sim -> target -> health_percentage();  
        case EFG_VULN:       return action->sim -> target ->vulnerable;  
        case EFG_INVUL:      return action->sim -> target ->invulnerable;  
        case EFG_TICKING:    return action->ticking;
        case EFG_CTICK:      return action->current_tick;
        case EFG_NTICKS:     return action->num_ticks;
        case EFG_EXPIRE:     return action->duration_ready;
        case EFG_REMAINS:    {
                                double rem=action->duration_ready-action->sim->current_time;
                                return rem>0? rem:0;
                             }
        case EFG_TCAST:      return action->execute_time();
        case EFG_MOVING:     return action->player->moving;
      }
      return 0;
    }
  };


  // exp.class that evaluate to constant value
  struct expression_value_t: public act_expression_t
  {
    double value;
    virtual double evaluate(){ return value;}
  };

  // exp.class that evaluate to value from pointer to double
  struct expression_double_prt_t: public act_expression_t
  {
    double* p_value;
    virtual double evaluate(){ return *p_value;}
  };

  // exp.class that evaluate to value from pointer to int
  struct expression_int_prt_t: public act_expression_t
  {
    int* p_value;
    virtual double evaluate(){ return *p_value;}
  };


  act_expression_t::act_expression_t()
  {
    type=AEXP_NONE;
    operand_1=NULL;
    operand_2=NULL;
    p_value=NULL;
    value=0;
  }

  // evaluate expressions based on 2 or 1 operand
  double act_expression_t::evaluate()
  {
    switch (type){
      case AEXP_AND:        return operand_1->ok() && operand_2->ok();  
      case AEXP_OR:         return operand_1->ok() || operand_2->ok();  
      case AEXP_NOT:        return !operand_1->ok();  
      case AEXP_EQ:         return operand_1->evaluate() == operand_2->evaluate();  
      case AEXP_NEQ:        return operand_1->evaluate() != operand_2->evaluate();  
      case AEXP_GREATER:    return operand_1->evaluate() >  operand_2->evaluate();    
      case AEXP_LESS:       return operand_1->evaluate() <  operand_2->evaluate();  
      case AEXP_GE:         return operand_1->evaluate() >= operand_2->evaluate();    
      case AEXP_LE:         return operand_1->evaluate() <= operand_2->evaluate();  
      case AEXP_PLUS:       return operand_1->evaluate() +  operand_2->evaluate();  
      case AEXP_MINUS:      return operand_1->evaluate() -  operand_2->evaluate();  
      case AEXP_MUL:        return operand_1->evaluate() *  operand_2->evaluate();  
      case AEXP_DIV:        return operand_2->evaluate()? operand_1->evaluate()/  operand_2->evaluate() : 0 ;  
      case AEXP_VALUE:      return value;  
      case AEXP_INT_PTR:    return *((int*)p_value);  
      case AEXP_DOUBLE_PTR: return *((double*)p_value);  
    }
    return 0;
  }

  bool   act_expression_t::ok()
  {
    return evaluate()!=0; 
  }

  //handle warnings/eeor reporting, mainly in parse phase
  // Severity:  0= ignore warning
  //            1= just warning
  //            2= report error, but do not break
  //            3= breaking error, assert
  void act_expression_t::warn(int severity, action_t* action, std::string msg)
  {
    std::string e_msg;
    if (severity<2)  e_msg="Warning";  else  e_msg="Error";
    e_msg+="("+action->name_str+"): "+msg;
    printf("%s\n", e_msg.c_str());
    if ( action->sim -> debug ) log_t::output( action->sim, "Exp.parser warning: %s", e_msg.c_str() );
    if (severity==3)
    {
      printf("Expression parser breaking error\n");
      assert(false);
    }
  }


  act_expression_t* act_expression_t::find_operator(action_t* action, std::string unmasked, std::string expression, std::string op_str, 
                                                    int op_type, bool binary, bool can_miss_left)
  {
    act_expression_t* node=0;
    size_t p=0;
    p=expression.find(op_str);
    if (p!=std::string::npos){
      std::string left =trim(unmasked.substr(0,p));
      std::string right=trim(unmasked.substr(p+op_str.length()));
      act_expression_t* op1= 0;
      act_expression_t* op2= 0;
      if (binary){
        op1= act_expression_t::create(action, left);
        op2= act_expression_t::create(action, right);
      }else{
        op1= act_expression_t::create(action, right);
        if (left!="") warn(2,action,"Unexpected text to the left of unary operator "+op_str+" in : "+unmasked);
      }
      //check allowed cases to miss left operand
      if (binary && (op1==0) && can_miss_left){
        op1= new act_expression_t();
        op1->type=AEXP_VALUE;
        op1->value= 0;
      }
      // error handling
      if ((op1==0)||((op2==0)&&binary))
        warn(3,action,"Left or right operand for "+op_str+" missing in : "+unmasked);
      // create new node
      node= new act_expression_t();
      node->type= op_type;
      node->operand_1=op1;
      node->operand_2=op2;
      // optimize if both operands are constants
      if ( (op1->type==AEXP_VALUE) && ((!binary)||(op2->type==AEXP_VALUE)) ){
        double val= node->evaluate();
        delete node;
        node= new act_expression_t();
        node->type=AEXP_VALUE;
        node->value= val;
      }
    }
    return node;
  }

  // mark bracket "groups" as special chars (255..128, ie negative chars)
  std::string bracket_mask(std::string& src, std::string& dst){
    std::string err="";
    dst="";
    int b_lvl=0;
    char b_num=0;
    for (size_t p=0; p<src.length(); p++){
      char ch=src[p];
      if (ch=='('){
        b_lvl++;
        if (b_lvl==1) b_num--;
      }
      if (b_lvl>50) return "too many opening brackets!";
      if (b_lvl==0)
        dst+=ch;
      else
        dst+=b_num;
      if (ch==')') b_lvl--;
      if (b_lvl<0)  return "Too many closing brackets!";
    }
    if (b_lvl!=0) return "Brackets not closed!";
    return "";
  }


  // this parse expression string and create needed exp.tree
  // using different types of "expression nodes"
  act_expression_t* act_expression_t::create(action_t* action, std::string expression)
  {
    act_expression_t* root=0;
    std::string e=trim(tolower(expression));
    if (e=="") return root;   // if empty expression, fail
    replace_char(e,'~','=');  // since options do not allow =, so ~ can be used instead
    replace_str(e,"!=","<>"); // since ! has to be searched before !=
    // process parenthesses
    std::string m_err, mask;
    bool is_all_enclosed;
    do{
      is_all_enclosed=false;
      m_err= bracket_mask(e,mask);
      if (m_err!="")  warn(3,action,m_err);
      //remove enclosing parentheses if any
      if (mask.length()>1){
        int start_bracket= (mask[0]<0)||((unsigned char)mask[0]>240)?mask[0]:0;
        if (start_bracket && (mask[mask.length()-1]==start_bracket)){
          is_all_enclosed=true;
          e.erase(0,1);
          e.erase(e.length()-1);
          e=trim(e);
        }
      }
    } while (is_all_enclosed);

    // search for operators, starting with lowest priority operator
    if (!root) root=find_operator(action,e,mask, "&&", AEXP_AND,     true);
    if (!root) root=find_operator(action,e,mask, "&",  AEXP_AND,     true);
    if (!root) root=find_operator(action,e,mask, "||", AEXP_OR,      true);
    if (!root) root=find_operator(action,e,mask, "|",  AEXP_OR,      true);
    if (!root) root=find_operator(action,e,mask, "!",  AEXP_NOT,     false);
    if (!root) root=find_operator(action,e,mask, "==", AEXP_EQ,      true);
    if (!root) root=find_operator(action,e,mask, "<>", AEXP_NEQ,     true);
    if (!root) root=find_operator(action,e,mask, ">=", AEXP_GE,      true);
    if (!root) root=find_operator(action,e,mask, "<=", AEXP_LE,      true);
    if (!root) root=find_operator(action,e,mask, ">",  AEXP_GREATER, true);
    if (!root) root=find_operator(action,e,mask, "<",  AEXP_LESS,    true);
    if (!root) root=find_operator(action,e,mask, "+",  AEXP_PLUS,    true,true);
    if (!root) root=find_operator(action,e,mask, "-",  AEXP_MINUS,   true,true);
    if (!root) root=find_operator(action,e,mask, "*",  AEXP_MUL,     true);
    if (!root) root=find_operator(action,e,mask, "/",  AEXP_DIV,     true);
    if (!root) root=find_operator(action,e,mask, "\\", AEXP_DIV,     true);


    // check if this is fixed value/number 
    if (!root){
      double val;
      if (str_to_float(e, val)){
        root= new act_expression_t();
        root->type=AEXP_VALUE;
        root->value= val;
      }
    }

    // search for "named value", if nothing above found
    if (!root){
      std::vector<std::string> parts;
      unsigned int num_parts = util_t::string_split( parts, e, "." );
      // check for known suffix
      int suffix=0;
      if (num_parts>1){
        std::string sfx_candidate=parts[num_parts-1];
        if (sfx_candidate=="value") suffix=1;
        if (sfx_candidate=="buff") suffix=2;
        if (sfx_candidate=="stacks") suffix=2;
        if (sfx_candidate=="duration") suffix=3;
        if (sfx_candidate=="dur") suffix=3;
        if (sfx_candidate=="time") suffix=3;
      }
      if (suffix>0) num_parts--; // if recognized, remove from list
      // check for known prefix
      int prefix=0;
      if (num_parts>1){
        std::string pfx_candidate=parts[0];
        if (pfx_candidate=="buff") prefix=1;
        if (pfx_candidate=="buffs") prefix=1;
        if (pfx_candidate=="talent") prefix=2;
        if (pfx_candidate=="talents") prefix=2;
        if (pfx_candidate=="gear") prefix=3;
        if (pfx_candidate=="option") prefix=4;
        if (pfx_candidate=="options") prefix=4;
        if (pfx_candidate=="global") prefix=5;
      }
      // get name of value
      std::string name="";
      if ((prefix>0)&&(num_parts==2)) 
        name=parts[1]; 
      else
        if (num_parts==1) 
          name=parts[0]; 
        else
          warn(3,action,"wrong prefix.sufix combination for : "+e);
      // now search for name in categories
      if (name!=""){
        // Buffs
        if ( ((prefix==1)||(prefix==0)) && !root) {
          pbuff_t* buff=action->player->buff_list.find_buff(name);
          if (buff){
            if ((suffix==0)||(suffix==3)){
              pbuff_expression* e_buff= new pbuff_expression();
              e_buff->type=AEXP_CUSTOM;
              e_buff->buff= buff;
              root= e_buff;
            }else{
              root= new act_expression_t();
              root->type= AEXP_DOUBLE_PTR;
              root->p_value= &buff->buff_value;
            }
          }
        }
        // Global
        if ( ((prefix==5)||(prefix==0)) && !root) {
          int glob_type=0;
          //first names that are not expressions, but option settings
          if (name=="allow_early_recast"){
            action->is_ifall=1;
            e="1";
          }
          // now regular names
          if (name=="gcd") glob_type=EFG_GCD;
          if (name=="time") glob_type=EFG_TIME;
          if (name=="time_to_die") glob_type=EFG_TTD;
          if (name=="health_percentage") glob_type=EFG_HP;
          if (name=="vulnerable") glob_type=EFG_VULN;
          if (name=="invulnerable") glob_type=EFG_INVUL;
          if (name=="active") glob_type=EFG_TICKING;
          if (name=="ticking") glob_type=EFG_TICKING;
          if (name=="current_tick") glob_type=EFG_CTICK;
          if (name=="tick") glob_type=EFG_CTICK;
          if (name=="num_ticks") glob_type=EFG_NTICKS;
          if (name=="expire_time") glob_type=EFG_EXPIRE;
          if (name=="expire") glob_type=EFG_EXPIRE;
          if (name=="remains_time") glob_type=EFG_REMAINS;
          if (name=="remains") glob_type=EFG_REMAINS;
          if (name=="cast_time") glob_type=EFG_TCAST;
          if (name=="execute_time") glob_type=EFG_TCAST;
          if (name=="cast") glob_type=EFG_TCAST;
          if (name=="moving") glob_type=EFG_MOVING;
          if (name=="move") glob_type=EFG_MOVING;
          // now create node if known global value
          if (glob_type>0){
            global_expression_t* g_func= new global_expression_t();
            g_func->type= glob_type;
            g_func->action=action;
            root= g_func;
          }
        }
      }

    }

    //return result
    return root;
  }


