// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Heal
// ==========================================================================

// ==========================================================================
// Created by philoptik@gmail.com
//
// heal_target is set to player for now.
// ==========================================================================



// heal_t::_init_heal_t == Heal Constructor Initializations =================

  void heal_t::_init_heal_t()
  {

    player_t* p = sim -> find_player( "Fluffy_Tank" );
    if ( p ) heal_target.push_back( p );
    else heal_target.push_back( player );

    target=0;
    target_str = "";
    total_heal = actual_heal = overheal = 0;


    dot_behavior      = DOT_REFRESH;
    weapon_multiplier = 0.0;
    may_crit          = true;
    tick_may_crit     = true;

    stats -> type = STATS_HEAL;

  }

// heal_t::heal_t ======== Heal Constructor by Spell Name ===================

  heal_t::heal_t( const char* n, player_t* player, const char* sname, int t ) :
      spell_t( n, sname, player, t )
  {
    _init_heal_t();
  }



// heal_t::heal_t ======== Heal Constructor by Spell ID =====================

  heal_t::heal_t( const char* n, player_t* player, const uint32_t id, int t ) :
      spell_t( n, id, player, t )
  {
    _init_heal_t();
  }

// heal_t::player_buff ======================================================

  void heal_t::player_buff()
  {
    player_multiplier              = 1.0;
    player_dd_multiplier           = 1.0;
    player_td_multiplier           = 1.0;
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

    if ( player -> meta_gem == META_REVITALIZING_SHADOWSPIRIT )
    {
      player_crit_multiplier *= 1.03;
    }

    player_hit  = p -> composite_spell_hit();
    player_crit = p -> composite_spell_crit();

    player_multiplier    = p -> composite_player_multiplier   ( school );
    player_dd_multiplier = p -> composite_player_dd_multiplier( school );
    player_td_multiplier = p -> composite_player_td_multiplier( school );

    if ( base_spell_power_multiplier > 0 )
    {
      player_spell_power            = p -> composite_spell_power( school );
      player_spell_power_multiplier = p -> composite_spell_power_multiplier();
    }

    player_haste = haste();

    if ( sim -> debug )
      log_t::output( sim, "heal_t::player_buff: %s hit=%.2f crit=%.2f penetration=%.0f spell_power=%.2f attack_power=%.2f ",
                     name(), player_hit, player_crit, player_penetration, player_spell_power, player_attack_power );
  }

// heal_t::target_buff ====================================================

  void heal_t::target_debuff( player_t* t, int dmg_type )
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

    if ( t -> buffs.grace -> up() )
      target_multiplier *= 1.0 + t -> buffs.grace -> value();

    if ( sim -> debug )
      log_t::output( sim, "heal_t::target_buff: %s multiplier=%.2f hit=%.2f crit=%.2f attack_power=%.2f spell_power=%.2f penetration=%.0f",
                     name(), target_multiplier, target_hit, target_crit, target_attack_power, target_spell_power, target_penetration );
  }

// heal_t::haste ============================================================

  double heal_t::haste() SC_CONST
  {
    double h = spell_t::haste();

    return h;
  }

// heal_t::execute ===========================================================

  void heal_t::execute()
  {
    if ( sim -> log && ! dual )
    {
      log_t::output( sim, "%s performs %s (%.0f)", player -> name(), name(),
                     player -> resource_current[ player -> primary_resource() ] );
    }

    if ( observer ) *observer = 0;

    player_buff();

    total_heal = 0;

    for ( unsigned int i = 0; i < heal_target.size(); i++ )
    {
      target_debuff( heal_target[i], HEAL_DIRECT );

      calculate_result();

      direct_dmg = calculate_direct_damage();
      schedule_travel( heal_target[i] );

      if ( ! dual ) stats -> add_result( direct_dmg, HEAL_DIRECT, result );
    }

    consume_resource();

    update_ready();

    if ( ! dual ) stats -> add_execute( HEAL_DIRECT, time_to_execute );

    if ( harmful ) player -> in_combat = true;

    if ( repeating && ! proc ) schedule_execute();
  }

// heal_t::asses_damage ========================================================

  void heal_t::assess_damage( player_t* t, double amount,
                                int    dmg_type, int travel_result )
  {
    total_heal += amount;
    actual_heal += t -> resource_gain( RESOURCE_HEALTH, amount, 0, this );


    if ( dmg_type == HEAL_DIRECT )
    {

      if ( sim -> log )
      {
        log_t::output( sim, "%s %s heals %s for %.0f (%s)",
                       player -> name(), name(),
                       t -> name(), amount,
                       util_t::result_type_string( result ) );
      }
      if ( sim -> csv_file )
      {
        fprintf( sim -> csv_file, "%d;%s;%s;%s;%s;%s;%.1f\n",
                 sim->current_iteration, player->name(), name(), t->name(),
                 util_t::result_type_string( result ), util_t::school_type_string( school ), amount );
      }

      action_callback_t::trigger( player -> direct_damage_callbacks[ school ], this );
    }
    else if ( dmg_type == HEAL_OVER_TIME ) // DMG_OVER_TIME
    {
      overheal = tick_dmg - actual_heal;

      if ( sim -> log )
      {
        log_t::output( sim, "%s %s ticks (%d of %d) %s for %.0f heal (%s)",
                       player -> name(), name(),
                       dot -> current_tick, dot -> num_ticks,
                       heal_target[0] -> name(), amount,
                       util_t::result_type_string( result ) );
        log_t::damage_event( this, amount, dmg_type );
      }
      if ( sim -> csv_file )
      {
        fprintf( sim -> csv_file, "%d;%s;%s;%s;tick_%s;%s;%.1f\n",
                 sim->current_iteration, player->name(), name(), target->name(),
                 util_t::result_type_string( result ), util_t::school_type_string( school ), amount );
      }

      action_callback_t::trigger( player -> tick_damage_callbacks[ school ], this );
    }
    else
      assert( 0 );

  }

// heal_t::ready ============================================================

  bool heal_t::ready()
  {
    if ( player -> skill < 1.0 )
      if ( ! sim -> roll( player -> skill ) )
        return false;

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

    if ( player -> buffs.moving -> check() )
      if ( ! usable_moving && ( channeled || ( range == 0 ) || ( execute_time() > 0 ) ) )
        return false;

    if ( moving != -1 )
      if ( moving != ( player -> buffs.moving -> check() ? 1 : 0 ) )
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

// heal_t::caclulate_result ==================================================

  void heal_t::calculate_result()
  {
    direct_dmg = 0;
    result = RESULT_NONE;

    if ( ! harmful ) return;

    if ( result == RESULT_NONE )
    {
      result = RESULT_HIT;

      if ( may_crit )
      {
        if ( rng[ RESULT_CRIT ] -> roll( crit_chance( 0 ) ) )
        {
          result = RESULT_CRIT;
        }
      }
    }

    if ( sim -> debug ) log_t::output( sim, "%s result for %s is %s", player -> name(), name(), util_t::result_type_string( result ) );
  }

// heal_t::calculate_direct_damage ===========================================

  double heal_t::calculate_direct_damage()
  {
    double dmg = sim -> range( base_dd_min, base_dd_max );

    if ( round_base_dmg ) dmg = floor( dmg + 0.5 );

    if ( dmg == 0 && weapon_multiplier == 0 && direct_power_mod == 0 ) return 0;

    double base_direct_dmg = dmg;

    dmg += base_dd_adder + player_dd_adder + target_dd_adder;

    dmg += direct_power_mod * total_power();
    dmg *= total_dd_multiplier();

    double init_direct_dmg = dmg;


    if ( result == RESULT_CRIT )
    {
      dmg *= 1.0 + total_crit_bonus();
    }

    if ( ! sim -> average_range ) dmg = floor( dmg + sim -> real() );

    if ( sim -> debug )
    {
      log_t::output( sim, "%s heal for %s: dd=%.0f i_dd=%.0f b_dd=%.0f mod=%.2f power=%.0f b_mult=%.2f p_mult=%.2f t_mult=%.2f",
                     player -> name(), name(), dmg, init_direct_dmg, base_direct_dmg, direct_power_mod,
                     total_power(), base_multiplier * base_dd_multiplier, player_multiplier, target_multiplier );
    }

    return dmg;
  }

// heal_t::calculate_tick_damage =============================================

  double heal_t::calculate_tick_damage()
  {
    double dmg = 0;

    if ( base_td == 0 ) base_td = base_td_init;

    if ( base_td == 0 && tick_power_mod == 0 ) return 0;

    dmg  = floor( base_td + 0.5 ) + total_power() * tick_power_mod;
    dmg *= total_td_multiplier();

    double init_tick_dmg = dmg;

    if ( result == RESULT_CRIT )
    {
      dmg *= 1.0 + total_crit_bonus();
    }

    if ( ! sim -> average_range ) dmg = floor( dmg + sim -> real() );

    if ( sim -> debug )
    {
      log_t::output( sim, "%s heal for %s: td=%.0f i_td=%.0f b_td=%.0f mod=%.2f power=%.0f b_mult=%.2f p_mult=%.2f t_mult=%.2f",
                     player -> name(), name(), tick_dmg, init_tick_dmg, base_td, tick_power_mod,
                     total_power(), base_multiplier * base_td_multiplier, player_multiplier, target_multiplier );
    }

    return dmg;
  }

// heal_t::update_stats =====================================================

  void heal_t::update_stats( int type )
  {
    if ( type == HEAL_OVER_TIME )
    {
      stats -> add( tick_dmg, type, RESULT_HIT, time_to_tick );
    }
    else assert( 0 );
  }

  // heal_t::schedule_travel ===============================================

  void heal_t::schedule_travel( player_t* p )
  {
    time_to_travel = travel_time();

    if ( time_to_travel == 0 )
    {
      travel( p, result, direct_dmg );
    }
    else
    {
      if ( sim -> log )
      {
        log_t::output( sim, "%s schedules travel (%.2f) for %s", player -> name(),time_to_travel, name() );
      }

      travel_event = new ( sim ) action_travel_event_t( sim, p, this, time_to_travel );
    }
  }

// heal_t::travel ============================================================

  void heal_t::travel( player_t* t, int travel_result, double travel_dmg=0 )
  {
    if ( travel_dmg > 0 )
    {
      assess_damage( t, travel_dmg, HEAL_DIRECT, travel_result );
    }
    if ( num_ticks > 0 )
    {
      if ( dot_behavior != DOT_REFRESH ) cancel();

      dot -> action = this;
      dot -> num_ticks = hasted_num_ticks();
      dot -> current_tick = 0;
      dot -> added_ticks = 0;
      if ( dot -> ticking )
      {
        assert( dot -> tick_event );
        if ( ! channeled )
        {
          // Recasting a dot while it's still ticking gives it an extra tick in total
          dot -> num_ticks++;
        }
      }
      else
      {
        schedule_tick();
      }
      dot -> recalculate_ready();
      if ( sim -> debug )
        log_t::output( sim, "%s extends dot-ready to %.2f for %s (%s)",
                       player -> name(), dot -> ready, name(), dot -> name() );
    }
  }

// heal_t::tick =============================================================

  void heal_t::tick()
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), dot -> current_tick, dot -> num_ticks );

    result = RESULT_HIT;

    double save_target_crit = target_crit;

    target_debuff( heal_target[0], HEAL_OVER_TIME );

    target_crit = save_target_crit;

    if ( tick_may_crit )
    {

      if ( rng[ RESULT_CRIT ] -> roll( crit_chance( 0 ) ) )
      {
        result = RESULT_CRIT;
        action_callback_t::trigger( player -> spell_result_callbacks[ RESULT_CRIT ], this );
        if ( direct_tick )
        {
          action_callback_t::trigger( player -> spell_direct_result_callbacks[ RESULT_CRIT ], this );
        }
      }
    }

    tick_dmg = calculate_tick_damage();

    assess_damage( heal_target[0], tick_dmg, HEAL_OVER_TIME, result );

    action_callback_t::trigger( player -> tick_callbacks, this );

    update_stats( HEAL_OVER_TIME );
  }

// heal_t::last_tick =======================================================

  void heal_t::last_tick()
  {
    if ( sim -> debug ) log_t::output( sim, "%s fades from %s", name(), heal_target[0] -> name() );

    dot -> ticking = 0;
    time_to_tick = 0;

    if ( observer ) *observer = 0;
  }

// heal_t::find_greatest_difference_player ==================================

  player_t* heal_t::find_greatest_difference_player()
  {
    double diff=0;
    double max=0;
    player_t* max_player = player;
    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      // No love for pet's right now
      diff = p -> is_pet() ? 0 : p -> resource_max[ RESOURCE_HEALTH ] - p -> resource_current[ RESOURCE_HEALTH];
      if ( diff > max )
      {
        max = diff;
        max_player = p;
      }
    }
    return max_player;
  }

  player_t* heal_t::find_lowest_player()
  {
    double diff=0;
    double max=0;
    player_t* max_player = player;
    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      // No love for pet's right now
      diff =  p -> is_pet() ? 0 : 1.0 / p -> resource_current[ RESOURCE_HEALTH];
      if ( diff > max )
      {
        max = diff;
        max_player = p;
      }
    }
    return max_player;
  }


  void heal_t::refresh_duration()
  {
    if ( sim -> log ) log_t::output( sim, "%s refreshes duration of %s", player -> name(), name() );

    // Make sure this DoT is still ticking......
    assert( dot -> tick_event );

    player_buff();
    target_debuff( heal_target[0], HEAL_OVER_TIME );

    dot -> action = this;
    dot -> current_tick = 0;
    dot -> added_ticks = 0;
    dot -> num_ticks = hasted_num_ticks();
    dot -> recalculate_ready();
  }


  // ==========================================================================
  // Absorb
  // ==========================================================================

  // ==========================================================================
  // Created by philoptik@gmail.com
  //
  // heal_target is set to player for now.
  // dmg_type = ABSORB, all crits killed
  // ==========================================================================



  // absorb_t::_init_absorb_t == Absorb Constructor Initializations ===========

    void absorb_t::_init_absorb_t()
    {
      player_t* p = sim -> find_player( "Fluffy_Tank" );
      if ( p ) heal_target.push_back( p );
      else heal_target.push_back( player );

      target=0;
      target_str = "";
      total_heal = actual_heal = overheal = 0;

      weapon_multiplier = 0.0;

      stats -> type = STATS_ABSORB;
    }

  // absorb_t::absorb_t ======== Absorb Constructor by Spell Name =============

    absorb_t::absorb_t( const char* n, player_t* player, const char* sname, int t ) :
        spell_t( n, sname, player, t )
    {
      _init_absorb_t();
    }

  // absorb_t::absorb_t ======== absorb Constructor by Spell ID =====================

    absorb_t::absorb_t( const char* n, player_t* player, const uint32_t id, int t ) :
        spell_t( n, id, player, t )
    {
      _init_absorb_t();
    }

  // absorb_t::player_buff ======================================================

    void absorb_t::player_buff()
    {
      player_multiplier              = 1.0;
      player_dd_multiplier           = 1.0;
      player_td_multiplier           = 1.0;
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

      player_multiplier    = p -> composite_player_multiplier   ( school );
      player_dd_multiplier = p -> composite_player_dd_multiplier( school );
      player_td_multiplier = p -> composite_player_td_multiplier( school );

      if ( base_spell_power_multiplier > 0 )
      {
        player_spell_power            = p -> composite_spell_power( school );
        player_spell_power_multiplier = p -> composite_spell_power_multiplier();
      }

      player_haste = haste();

      if ( sim -> debug )
        log_t::output( sim, "absorb_t::player_buff: %s hit=%.2f crit=%.2f penetration=%.0f spell_power=%.2f attack_power=%.2f ",
                       name(), player_hit, player_crit, player_penetration, player_spell_power, player_attack_power );
    }

  // absorb_t::target_debu ====================================================

    void absorb_t::target_debuff( player_t* t, int dmg_type )
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

      if ( sim -> debug )
        log_t::output( sim, "absorb_t::target_debuff: %s multiplier=%.2f hit=%.2f crit=%.2f attack_power=%.2f spell_power=%.2f penetration=%.0f",
                       name(), target_multiplier, target_hit, target_crit, target_attack_power, target_spell_power, target_penetration );
    }

  // absorb_t::haste ============================================================

    double absorb_t::haste() SC_CONST
    {
      double h = spell_t::haste();

      return h;
    }

  // absorb_t::execute ===========================================================

    void absorb_t::execute()
    {
      if ( sim -> log && ! dual )
      {
        log_t::output( sim, "%s performs %s (%.0f)", player -> name(), name(),
                       player -> resource_current[ player -> primary_resource() ] );
      }

      if ( observer ) *observer = 0;

      player_buff();

      for ( unsigned int i = 0; i < heal_target.size(); i++ )
      {
         target_debuff( heal_target[i], HEAL_DIRECT );

         calculate_result();

         direct_dmg = calculate_direct_damage();
         schedule_travel( heal_target[i] );

         if ( ! dual ) stats -> add_result( direct_dmg, HEAL_DIRECT, result );
       }

      consume_resource();

      update_ready();

      if ( ! dual ) stats -> add_execute( ABSORB, time_to_execute );

      if ( harmful ) player -> in_combat = true;

      if ( repeating && ! proc ) schedule_execute();
    }

// absorb_t::asses_heal ========================================================

    void absorb_t::assess_damage( player_t* t, double amount,
                                  int    dmg_type, int travel_result )
    {
      total_heal += amount;
      actual_heal += t -> resource_gain( RESOURCE_HEALTH, amount, 0, this );

      if ( dmg_type == ABSORB )
      {
        if ( sim -> log )
        {
          log_t::output( sim, "%s %s heals %s for %.0f (%s)",
                         player -> name(), name(),
                         t -> name(), amount,
                         util_t::result_type_string( result ) );
        }
        if ( sim -> csv_file )
        {
          fprintf( sim -> csv_file, "%d;%s;%s;%s;%s;%s;%.1f\n",
                   sim->current_iteration, player->name(), name(), t->name(),
                   util_t::result_type_string( result ), util_t::school_type_string( school ), amount );
        }

        action_callback_t::trigger( player -> direct_damage_callbacks[ school ], this );
      }

      else
        assert( 0 );

    }

  // absorb_t::ready ============================================================

    bool absorb_t::ready()
    {
      if ( player -> skill < 1.0 )
        if ( ! sim -> roll( player -> skill ) )
          return false;

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

      if ( player -> buffs.moving -> check() )
        if ( ! usable_moving && ( channeled || ( range == 0 ) || ( execute_time() > 0 ) ) )
          return false;

      if ( moving != -1 )
        if ( moving != ( player -> buffs.moving -> check() ? 1 : 0 ) )
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

  // absorb_t::caclulate_result ==================================================

    void absorb_t::calculate_result()
    {
      direct_dmg = 0;
      result = RESULT_NONE;

      if ( ! harmful ) return;

      if ( result == RESULT_NONE )
      {
        result = RESULT_HIT;

      }

      if ( sim -> debug ) log_t::output( sim, "%s result for %s is %s", player -> name(), name(), util_t::result_type_string( result ) );
    }

  // absorb_t::calculate_direct_damage ===========================================

    double absorb_t::calculate_direct_damage()
    {
      double dmg = sim -> range( base_dd_min, base_dd_max );

      if ( round_base_dmg ) dmg = floor( dmg + 0.5 );

      if ( dmg == 0 && weapon_multiplier == 0 && direct_power_mod == 0 ) return 0;

      double base_direct_dmg = dmg;

      dmg += base_dd_adder + player_dd_adder + target_dd_adder;

      dmg += direct_power_mod * total_power();
      dmg *= total_dd_multiplier();

      double init_direct_dmg = dmg;


      if ( ! sim -> average_range ) dmg = floor( dmg + sim -> real() );

      if ( sim -> debug )
      {
        log_t::output( sim, "%s heal for %s: dd=%.0f i_dd=%.0f b_dd=%.0f mod=%.2f power=%.0f b_mult=%.2f p_mult=%.2f t_mult=%.2f",
                       player -> name(), name(), dmg, init_direct_dmg, base_direct_dmg, direct_power_mod,
                       total_power(), base_multiplier * base_dd_multiplier, player_multiplier, target_multiplier );
      }

      return dmg;
    }


    // absorb_t::schedule_travel ===============================================

      void absorb_t::schedule_travel( player_t* p )
      {
        time_to_travel = travel_time();

        travel( p, result, direct_dmg );
      }

    // absorb_t::travel ============================================================

      void absorb_t::travel( player_t* t, int travel_result, double travel_dmg=0 )
      {
        if ( travel_dmg > 0 )
        {
          assess_damage( t, travel_dmg, ABSORB, travel_result );
        }
      }
