// ==========================================================================
// Dedmonwakeen's Raid DPS/TPS Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// stat_proc_callback =======================================================

struct stat_proc_callback_t : public action_callback_t
{
  std::string name_str;
  int stat;
  double amount;
  double tick;
  stat_buff_t* buff;

  stat_proc_callback_t( const std::string& n, player_t* p, int s, int max_stacks, double a,
                        double proc_chance, double duration, double cooldown,
                        double t=0, bool reverse=false, int rng_type=RNG_DEFAULT, bool activated=true ) :
    action_callback_t( p -> sim, p ),
    name_str( n ), stat( s ), amount( a ), tick( t )
  {
    if ( max_stacks == 0 ) max_stacks = 1;
    if ( proc_chance == 0 ) proc_chance = 1;
    if ( rng_type == RNG_DEFAULT ) rng_type = RNG_DISTRIBUTED;

    buff = new stat_buff_t( p, n, stat, amount, max_stacks, duration, cooldown, proc_chance, false, reverse, rng_type );
    buff -> activated = activated;
  }

  virtual void activate()
  {
    action_callback_t::activate();
    if ( buff -> reverse ) buff -> trigger( buff -> max_stack );
  }

  virtual void deactivate()
  {
    action_callback_t::deactivate();
    buff -> expire();
  }

  virtual void trigger( action_t* a, void* /* call_data */ )
  {
    if ( buff -> trigger( a ) )
    {
      if ( tick > 0 ) // The buff stacks over time.
      {
        struct tick_stack_t : public event_t
        {
          stat_proc_callback_t* callback;
          tick_stack_t( sim_t* sim, player_t* p, stat_proc_callback_t* cb ) : event_t( sim, p ), callback( cb )
          {
            name = callback -> buff -> name();
            sim -> add_event( this, callback -> tick );
          }
          virtual void execute()
          {
            stat_buff_t* b = callback -> buff;
            if ( b -> current_stack > 0 &&
                 b -> current_stack < b -> max_stack )
            {
              b -> bump();
              new ( sim ) tick_stack_t( sim, player, callback );
            }
          }
        };

        new ( sim ) tick_stack_t( sim, a -> player, this );
      }
    }
  }
};

// cost_reduction_proc_callback =============================================

struct cost_reduction_proc_callback_t : public action_callback_t
{
  std::string name_str;
  int school;
  double amount;
  cost_reduction_buff_t* buff;

  cost_reduction_proc_callback_t( const std::string& n, player_t* p, int s, int max_stacks, double a,
                                  double proc_chance, double duration, double cooldown,
                                  bool refreshes=false, bool reverse=false, int rng_type=RNG_DEFAULT, bool activated=true ) :
    action_callback_t( p -> sim, p ),
    name_str( n ), school( s ), amount( a )
  {
    if ( max_stacks == 0 ) max_stacks = 1;
    if ( proc_chance == 0 ) proc_chance = 1;
    if ( rng_type == RNG_DEFAULT ) rng_type = RNG_DISTRIBUTED;

    buff = new cost_reduction_buff_t( p, n, school, amount, max_stacks, duration, cooldown, proc_chance, refreshes, false, reverse, rng_type );
    buff -> activated = activated;
  }

  virtual void activate()
  {
    action_callback_t::activate();
    if ( buff -> reverse ) buff -> trigger( buff -> max_stack );
  }

  virtual void deactivate()
  {
    action_callback_t::deactivate();
    buff -> expire();
  }

  virtual void trigger( action_t* a, void* /* call_data */ )
  {
    buff -> trigger( a );
  }
};

// discharge_proc_callback ==================================================

struct discharge_proc_callback_t : public action_callback_t
{
  std::string name_str;
  int stacks, max_stacks;
  double proc_chance;
  cooldown_t* cooldown;
  action_t* discharge_action;
  proc_t* proc;
  rng_t* rng;

  discharge_proc_callback_t( const std::string& n, player_t* p, int ms,
                             const school_type school, double amount, double scaling,
                             double pc, double cd, bool no_crit, bool no_buffs, bool no_debuffs, int rng_type=RNG_DEFAULT ) :
    action_callback_t( p -> sim, p ),
    name_str( n ), stacks( 0 ), max_stacks( ms ), proc_chance( pc ), cooldown( 0 ), discharge_action( 0 ), proc( 0 ), rng( 0 )
  {
    if ( rng_type == RNG_DEFAULT ) rng_type = RNG_DISTRIBUTED;

    struct discharge_spell_t : public spell_t
    {
      discharge_spell_t( const char* n, player_t* p, double amount, double scaling, const school_type s, bool no_crit, bool nb, bool nd ) :
        spell_t( n, p, RESOURCE_NONE, ( s == SCHOOL_DRAIN ) ? SCHOOL_SHADOW : s )
      {
        discharge_proc = true;
        item_proc = true;
        trigger_gcd = 0;
        base_dd_min = amount;
        base_dd_max = amount;
        may_trigger_dtr = false;
        direct_power_mod = scaling;
        may_crit = ( s != SCHOOL_DRAIN ) && ! no_crit;
        background  = true;
        no_buffs = nb;
        no_debuffs = nd;
        init();
      }
    };

    struct discharge_attack_t : public attack_t
    {
      discharge_attack_t( const char* n, player_t* p, double amount, double scaling, const school_type s, bool no_crit, bool nb, bool nd ) :
        attack_t( n, p, RESOURCE_NONE, ( s == SCHOOL_DRAIN ) ? SCHOOL_SHADOW : s )
      {
        discharge_proc = true;
        item_proc = true;
        trigger_gcd = 0;
        base_dd_min = amount;
        base_dd_max = amount;
        may_trigger_dtr = false;
        direct_power_mod = scaling;
        may_crit = ( s != SCHOOL_DRAIN ) && ! no_crit;
        may_dodge = ( s == SCHOOL_PHYSICAL && ! no_crit );
        may_parry = ( s == SCHOOL_PHYSICAL && ! no_crit && ( p -> position == POSITION_FRONT || p -> position == POSITION_RANGED_FRONT ) );
        may_glance = false;
        background  = true;
        no_buffs = nb;
        no_debuffs = nd;
        init();
      }
    };

    cooldown = p -> get_cooldown( name_str );
    cooldown -> duration = cd;

    if ( amount > 0 )
    {
      discharge_action = new discharge_spell_t( name_str.c_str(), p, amount, scaling, school, no_crit, no_buffs, no_debuffs );
    }
    else
    {
      discharge_action = new discharge_attack_t( name_str.c_str(), p, -amount, scaling, school, no_crit, no_buffs, no_debuffs );
    }

    proc = p -> get_proc( name_str.c_str() );
    rng  = p -> get_rng ( name_str.c_str(), rng_type );  // default is CYCLIC since discharge should not have duration
  }

  virtual void reset() { stacks=0; }

  virtual void deactivate() { action_callback_t::deactivate(); stacks=0; }

  virtual void trigger( action_t* a, void* /* call_data */ )
  {
    if ( cooldown -> remains() > 0 )
      return;

    if ( ! allow_self_procs && ( a == discharge_action ) ) return;

    if ( a -> discharge_proc ) return;

    if ( proc_chance )
    {
      if ( proc_chance < 0 )
      {
        if ( ! rng -> roll( a -> ppm_proc_chance( -proc_chance ) ) )
          return;
      }
      else
      {
        if ( ! rng -> roll( proc_chance ) )
          return;
      }
    }
    else
      return;

    cooldown -> start();

    if ( ++stacks < max_stacks )
    {
      listener -> aura_gain( name_str.c_str(), stacks );
    }
    else
    {
      stacks = 0;
      if ( sim -> debug ) log_t::output( sim, "%s procs %s", a -> name(), discharge_action -> name() );
      discharge_action -> execute();
      proc -> occur();
    }
  }
};

// chance_discharge_proc_callback ===========================================

struct chance_discharge_proc_callback_t : public action_callback_t
{
  std::string name_str;
  int stacks, max_stacks;
  double proc_chance;
  cooldown_t* cooldown;
  action_t* discharge_action;
  proc_t* proc;
  rng_t* rng;

  chance_discharge_proc_callback_t( const std::string& n, player_t* p, int ms,
                                    const school_type school, double amount, double scaling,
                                    double pc, double cd, bool no_crit, bool no_buffs, bool no_debuffs, int rng_type=RNG_DEFAULT ) :
    action_callback_t( p -> sim, p ),
    name_str( n ), stacks( 0 ), max_stacks( ms ), proc_chance( pc )
  {
    if ( rng_type == RNG_DEFAULT ) rng_type = RNG_DISTRIBUTED;

    struct discharge_spell_t : public spell_t
    {
      discharge_spell_t( const char* n, player_t* p, double amount, double scaling, const school_type s, bool no_crit, bool nb, bool nd ) :
        spell_t( n, p, RESOURCE_NONE, ( s == SCHOOL_DRAIN ) ? SCHOOL_SHADOW : s )
      {
        discharge_proc = true;
        item_proc = true;
        trigger_gcd = 0;
        base_dd_min = amount;
        base_dd_max = amount;
        may_trigger_dtr = false;
        direct_power_mod = scaling;
        may_crit = ( s != SCHOOL_DRAIN ) && ! no_crit;
        background  = true;
        no_buffs = nb;
        no_debuffs = nd;
        init();
      }
    };

    struct discharge_attack_t : public attack_t
    {
      discharge_attack_t( const char* n, player_t* p, double amount, double scaling, const school_type s, bool no_crit, bool nb, bool nd ) :
        attack_t( n, p, RESOURCE_NONE, ( s == SCHOOL_DRAIN ) ? SCHOOL_SHADOW : s )
      {
        discharge_proc = true;
        item_proc = true;
        trigger_gcd = 0;
        base_dd_min = amount;
        base_dd_max = amount;
        may_trigger_dtr = false;
        direct_power_mod = scaling;
        may_crit = ( s != SCHOOL_DRAIN ) && ! no_crit;
        may_dodge = ( s == SCHOOL_PHYSICAL && ! no_crit );
        may_parry = ( s == SCHOOL_PHYSICAL && ! no_crit && ( p -> position == POSITION_FRONT || p -> position == POSITION_RANGED_FRONT ) );
        may_glance = false;
        background  = true;
        no_buffs = nb;
        no_debuffs = nd;
        init();
      }
    };

    cooldown = p -> get_cooldown( name_str );
    cooldown -> duration = cd;

    if ( amount > 0 )
    {
      discharge_action = new discharge_spell_t( name_str.c_str(), p, amount, scaling, school, no_crit, no_buffs, no_debuffs );
    }
    else
    {
      discharge_action = new discharge_attack_t( name_str.c_str(), p, -amount, scaling, school, no_crit, no_buffs, no_debuffs );
    }

    proc = p -> get_proc( name_str.c_str() );
    rng  = p -> get_rng ( name_str.c_str(), rng_type );  // default is CYCLIC since discharge should not have duration
  }

  virtual void reset() { stacks=0; }

  virtual void deactivate() { action_callback_t::deactivate(); stacks=0; }

  virtual void trigger( action_t* a, void* /* call_data */ )
  {
    /* Always adds a stack if not on cooldown. The proc chance is the chance to discharge */
    if ( cooldown -> remains() > 0 )
      return;

    if ( ! allow_self_procs && ( a == discharge_action ) ) return;

    cooldown -> start();

    if ( ++stacks < max_stacks )
    {
      listener -> aura_gain( name_str.c_str(), stacks );
      if ( proc_chance )
      {
        if ( proc_chance < 0 )
        {
          if ( ! rng -> roll( a -> ppm_proc_chance( -proc_chance ) ) )
            return;
        }
        else
        {
          if ( ! rng -> roll( proc_chance ) )
            return;
        }
      }
      discharge_action -> base_dd_multiplier = stacks;
      discharge_action -> execute();
      stacks = 0;
      proc -> occur();
    }
    else
    {
      discharge_action -> base_dd_multiplier = stacks;
      discharge_action -> execute();
      stacks = 0;
      proc -> occur();
    }
  }
};

// stat_discharge_proc_callback =============================================

struct stat_discharge_proc_callback_t : public action_callback_t
{
  std::string name_str;
  stat_buff_t* buff;
  action_t* discharge_action;

  stat_discharge_proc_callback_t( const std::string& n, player_t* p,
                                  int stat, int max_stacks, double stat_amount,
                                  const school_type school, double discharge_amount, double discharge_scaling,
                                  double proc_chance, double duration, double cooldown, bool no_crit, bool no_buffs, bool no_debuffs, bool activated=true ) :
    action_callback_t( p -> sim, p ), name_str( n )
  {
    if ( max_stacks == 0 ) max_stacks = 1;
    if ( proc_chance == 0 ) proc_chance = 1;

    buff = new stat_buff_t( p, n, stat, stat_amount, max_stacks, duration, cooldown, proc_chance );
    buff -> activated = activated;

    struct discharge_spell_t : public spell_t
    {
      discharge_spell_t( const char* n, player_t* p, double amount, double scaling, const school_type s, bool no_crit, bool nb, bool nd ) :
        spell_t( n, p, RESOURCE_NONE, ( s == SCHOOL_DRAIN ) ? SCHOOL_SHADOW : s )
      {
        discharge_proc = true;
        item_proc = true;
        trigger_gcd = 0;
        base_dd_min = amount;
        base_dd_max = amount;
        may_trigger_dtr = false;
        direct_power_mod = scaling;
        may_crit = ( s != SCHOOL_DRAIN ) && ! no_crit;
        background  = true;
        no_buffs = nb;
        no_debuffs = nd;
        init();
      }
    };

    struct discharge_attack_t : public attack_t
    {
      bool no_buffs;

      discharge_attack_t( const char* n, player_t* p, double amount, double scaling, const school_type s, bool no_crit, bool nb, bool nd ) :
        attack_t( n, p, RESOURCE_NONE, ( s == SCHOOL_DRAIN ) ? SCHOOL_SHADOW : s )
      {
        discharge_proc = true;
        item_proc = true;
        trigger_gcd = 0;
        base_dd_min = amount;
        base_dd_max = amount;
        may_trigger_dtr = false;
        direct_power_mod = scaling;
        may_crit = ( s != SCHOOL_DRAIN ) && ! no_crit;
        may_dodge = ( s == SCHOOL_PHYSICAL && ! no_crit );
        may_parry = ( s == SCHOOL_PHYSICAL && ! no_crit && ( p -> position == POSITION_FRONT || p -> position == POSITION_RANGED_FRONT ) );
        may_glance = false;
        background  = true;
        no_buffs = nb;
        no_debuffs = nd;
        init();
      }
    };

    if ( discharge_amount > 0 )
    {
      discharge_action = new discharge_spell_t( name_str.c_str(), p, discharge_amount, discharge_scaling, school, no_crit, no_buffs, no_debuffs );
    }
    else
    {
      discharge_action = new discharge_attack_t( name_str.c_str(), p, -discharge_amount, discharge_scaling, school, no_crit, no_buffs, no_debuffs );
    }
  }

  virtual void deactivate()
  {
    action_callback_t::deactivate();
    buff -> expire();
  }

  virtual void trigger( action_t* a, void* /* call_data */ )
  {
    if ( buff -> trigger( a ) )
    {
      if ( ! allow_self_procs && ( a == discharge_action ) ) return;
      discharge_action -> execute();
    }
  }
};

// register_apparatus_of_khazgoroth =========================================

static void register_apparatus_of_khazgoroth( item_t* item )
{
  player_t* p = item -> player;

  item -> unique = true;
  bool heroic = item -> heroic();

  struct apparatus_of_khazgoroth_callback_t : public action_callback_t
  {
    bool heroic;
    buff_t* apparatus_of_khazgoroth;
    stat_buff_t* blessing_of_khazgoroth;
    proc_t* proc_apparatus_of_khazgoroth_haste;
    proc_t* proc_apparatus_of_khazgoroth_crit;
    proc_t* proc_apparatus_of_khazgoroth_mastery;

    apparatus_of_khazgoroth_callback_t( player_t* p, bool h ) :
      action_callback_t( p -> sim, p ), heroic( h )
    {
      double amount = heroic ? 2875 : 2540;

      apparatus_of_khazgoroth = new buff_t( p, 96923, "titanic_power" ); // TODO: Duration, cd, etc.?
      apparatus_of_khazgoroth -> activated = false;
      blessing_of_khazgoroth  = new stat_buff_t( p, "blessing_of_khazgoroth", STAT_CRIT_RATING, amount, 1, 15.0, 120.0 );
      proc_apparatus_of_khazgoroth_haste   = p -> get_proc( "apparatus_of_khazgoroth_haste"   );
      proc_apparatus_of_khazgoroth_crit    = p -> get_proc( "apparatus_of_khazgoroth_crit"    );
      proc_apparatus_of_khazgoroth_mastery = p -> get_proc( "apparatus_of_khazgoroth_mastery" );
    }

    virtual void trigger( action_t* a, void* /* call_data */ )
    {
      if ( ! a -> weapon ) return;
      if ( a -> proc ) return;

      if ( apparatus_of_khazgoroth -> trigger() )
      {
        if ( blessing_of_khazgoroth -> cooldown -> remains() > 0 ) return;

        // FIXME: This really should be a /use action
        if ( apparatus_of_khazgoroth -> check() == 5 )
        {
          // Highest of Crits/Master/haste is chosen
          blessing_of_khazgoroth -> stat = STAT_CRIT_RATING;
          if ( a -> player -> stats.haste_rating   > a -> player -> stats.crit_rating ||
               a -> player -> stats.mastery_rating > a -> player -> stats.crit_rating )
          {
            if ( a -> player -> stats.mastery_rating > a -> player -> stats.haste_rating )
            {
              blessing_of_khazgoroth -> stat = STAT_MASTERY_RATING;
              proc_apparatus_of_khazgoroth_mastery -> occur();
            }
            else
            {
              blessing_of_khazgoroth -> stat = STAT_HASTE_RATING;
              proc_apparatus_of_khazgoroth_haste -> occur();
            }
          }
          else
          {
            proc_apparatus_of_khazgoroth_crit -> occur();
          }
          apparatus_of_khazgoroth -> expire();
          blessing_of_khazgoroth -> trigger();
        }
      }
    }
  };

  p -> register_attack_callback( RESULT_CRIT_MASK, new apparatus_of_khazgoroth_callback_t( p, heroic ) );
}

// register_darkmoon_card_greatness =========================================

static void register_darkmoon_card_greatness( item_t* item )
{
  player_t* p = item -> player;

  item -> unique = true;

  int attr[] = { ATTR_STRENGTH, ATTR_AGILITY, ATTR_INTELLECT, ATTR_SPIRIT };
  int stat[] = { STAT_STRENGTH, STAT_AGILITY, STAT_INTELLECT, STAT_SPIRIT };

  int max_stat=-1;
  double max_value=0;

  for ( int i=0; i < 4; i++ )
  {
    if ( p -> attribute[ attr[ i ] ] > max_value )
    {
      max_value = p -> attribute[ attr[ i ] ];
      max_stat = stat[ i ];
    }
  }
  action_callback_t* cb = new stat_proc_callback_t( "darkmoon_card_greatness", p, max_stat, 1, 300, 0.35, 15.0, 45.0, 0, false, RNG_DEFAULT, false );

  p -> register_tick_damage_callback( SCHOOL_ALL_MASK, cb );
  p -> register_direct_damage_callback( SCHOOL_ALL_MASK, cb );
}

// register_fury_of_angerforge ==============================================

static void register_fury_of_angerforge( item_t* item )
{
  player_t* p = item -> player;

  item -> unique = true;

  struct fury_of_angerforge_callback_t : public action_callback_t
  {
    buff_t* raw_fury;
    stat_buff_t* blackwing_dragonkin;

    fury_of_angerforge_callback_t( player_t* p ) :
      action_callback_t( p -> sim, p )
    {
      raw_fury = new buff_t( p, "raw_fury", 5, 15.0, 5.0, 0.5, true );
      raw_fury -> activated = false;
      blackwing_dragonkin = new stat_buff_t( p, "blackwing_dragonkin", STAT_STRENGTH, 1926, 1, 20.0, 120.0 );
    }

    virtual void trigger( action_t* a, void* /* call_data */ )
    {
      if ( ! a -> weapon ) return;
      if ( a -> proc ) return;

      if ( raw_fury -> trigger() )
      {
        if ( blackwing_dragonkin -> cooldown -> remains() > 0 ) return;

        // FIXME: This really should be a /use action
        if ( raw_fury -> check() == 5 )
        {
          raw_fury -> expire();
          blackwing_dragonkin -> trigger();
        }
      }
    }
  };

  p -> register_attack_callback( RESULT_HIT_MASK, new fury_of_angerforge_callback_t( p ) );
}

// register_heart_of_ignacious ==============================================

static void register_heart_of_ignacious( item_t* item )
{
  player_t* p = item -> player;

  item -> unique = true;

  struct heart_of_ignacious_callback_t : public stat_proc_callback_t
  {
    stat_buff_t* haste_buff;
    bool heroic;

    heart_of_ignacious_callback_t( player_t* p, bool h ) :
      stat_proc_callback_t( "heart_of_ignacious", p, STAT_SPELL_POWER, 5, h ? 87 : 77, 1.0, 15.0, 2.0, 0, false, RNG_DEFAULT, false ), heroic( h )
    {
      haste_buff = new stat_buff_t( p, "hearts_judgement", STAT_HASTE_RATING, heroic ? 363 : 321, 5, 20.0, 120.0 );
    }

    virtual void trigger( action_t* /* a */, void* /* call_data */ )
    {
      buff -> trigger();
      if ( buff -> stack() == buff -> max_stack )
      {
        if ( haste_buff -> trigger( buff -> max_stack ) )
        {
          buff -> expire();
        }
      }
    }
  };

  stat_proc_callback_t* cb = new heart_of_ignacious_callback_t( p, item -> heroic() );
  p -> register_tick_damage_callback( RESULT_ALL_MASK, cb );
  p -> register_direct_damage_callback( RESULT_ALL_MASK, cb  );
}

// register_matrix_restabilizer =============================================

static void register_matrix_restabilizer( item_t* item )
{
  player_t* p = item -> player;

  item -> unique = true;

  struct matrix_restabilizer_callback_t : public stat_proc_callback_t
  {
    bool heroic;
    stat_buff_t* buff_matrix_restabilizer_crit;
    stat_buff_t* buff_matrix_restabilizer_haste;
    stat_buff_t* buff_matrix_restabilizer_mastery;

    matrix_restabilizer_callback_t( player_t* p, bool h ) :
      stat_proc_callback_t( "matrix_restabilizer", p, STAT_CRIT_RATING, 1, 0, 0, 0, 0, 0, false, RNG_DEFAULT, false ),
      heroic( h ), buff_matrix_restabilizer_crit( 0 ), buff_matrix_restabilizer_haste( 0 ), buff_matrix_restabilizer_mastery( 0 )
    {
      buff_matrix_restabilizer_crit     = new stat_buff_t( p, "matrix_restabilizer_crit",    STAT_CRIT_RATING,    heroic ? 1834 : 1624, 1, 30, 105, .15, false, RNG_DEFAULT, false );
      buff_matrix_restabilizer_haste    = new stat_buff_t( p, "matrix_restabilizer_haste",   STAT_HASTE_RATING,   heroic ? 1834 : 1624, 1, 30, 105, .15, false, RNG_DEFAULT, false );
      buff_matrix_restabilizer_mastery  = new stat_buff_t( p, "matrix_restabilizer_mastery", STAT_MASTERY_RATING, heroic ? 1834 : 1624, 1, 30, 105, .15, false, RNG_DEFAULT, false );
    }

    virtual void trigger( action_t* a, void* call_data )
    {
      if ( buff -> cooldown -> remains() > 0 ) return;

      player_t* p = a -> player;

      if ( p -> stats.crit_rating > p -> stats.haste_rating )
      {
        if ( p -> stats.crit_rating > p -> stats.mastery_rating )
        {
          buff = buff_matrix_restabilizer_crit;
        }
        else
        {
          buff = buff_matrix_restabilizer_mastery;
        }
      }
      else if ( p -> stats.haste_rating > p -> stats.mastery_rating )
      {
        buff = buff_matrix_restabilizer_haste;
      }
      else
      {
        buff = buff_matrix_restabilizer_mastery;
      }

      stat_proc_callback_t::trigger( a, call_data );
    }
  };

  p -> register_attack_callback( RESULT_HIT_MASK, new matrix_restabilizer_callback_t( p, item -> heroic() ) );
}

// register_shard_of_woe ====================================================

static void register_shard_of_woe( item_t* item )
{
  // February 15 - Hotfix
  // Shard of Woe (Heroic trinket) now reduces the base mana cost of
  // spells by 205, with the exception of Holy and Nature spells --
  // the base mana cost of Holy and Nature spells remains reduced
  // by 405 with this trinket.
  player_t* p = item -> player;

  item -> unique = true;

  for ( int i = 0; i < SCHOOL_MAX; i++ )
  {
    p -> initial_resource_reduction[ i ] += 205;
  }
}

// register_sorrowsong ======================================================

static void register_sorrowsong( item_t* item )
{
  // H: http://www.wowhead.com/item=56400
  player_t* p = item -> player;

  item -> unique = true;

  struct sorrowsong_callback_t : public stat_proc_callback_t
  {
    sorrowsong_callback_t( player_t* p, bool h ) :
      stat_proc_callback_t( "sorrowsong", p, STAT_SPELL_POWER, 1, h ? 1710 : 1512, 1.0, 10.0, 20.0, 0, false, RNG_DEFAULT, false )
    {}

    virtual void trigger( action_t* a, void* call_data )
    {
      if ( a -> target -> health_percentage() < 35 )
      {
        stat_proc_callback_t::trigger( a, call_data );
      }
    }
  };

  stat_proc_callback_t* cb = new sorrowsong_callback_t( p, item -> heroic() );
  p -> register_tick_damage_callback( RESULT_ALL_MASK, cb );
  p -> register_direct_damage_callback( RESULT_ALL_MASK, cb  );
}

// register_tyrandes_favorite_doll ==========================================

static void register_tyrandes_favorite_doll( item_t* item )
{
  player_t* p = item -> player;

  item -> unique = true;

  struct tyrandes_spell_t : public spell_t
  {
    tyrandes_spell_t( player_t* p, double max_mana ) :
      spell_t( "tyrandes_doll", p, RESOURCE_NONE, SCHOOL_ARCANE )
    {
      trigger_gcd = 0;
      base_dd_min = max_mana;
      base_dd_max = max_mana;
      may_crit = true;
      aoe = -1;
      background = true;
      base_spell_power_multiplier = 0;
      cooldown -> duration = 60.0;
      init();
    }
  };

  struct tyrandes_callback_t : public action_callback_t
  {
    double max_mana;
    double mana_stored;
    spell_t* discharge_spell;
    gain_t* gain_source;
    player_t* player;

    tyrandes_callback_t( player_t* p ) :
      action_callback_t( p -> sim, p ), max_mana( 4200 ), mana_stored( 0 ), player( p )
    {
      discharge_spell = new tyrandes_spell_t( p, max_mana );
      gain_source = p -> get_gain( "tyrandes_doll" );
    }

    virtual void reset() { action_callback_t::reset(); mana_stored=0; }

    virtual void trigger( action_t* /* a */, void* call_data )
    {
      double mana_spent = *( ( double* ) call_data );

      mana_stored += mana_spent * 0.20;
      if ( mana_stored > max_mana ) mana_stored = max_mana;

      // FIXME! For now trigger as soon as the cooldown is up.
      if ( ( mana_stored >= max_mana ) && ( discharge_spell -> cooldown -> remains() <= 0 ) )
      {
        discharge_spell -> execute();
        player -> resource_gain( RESOURCE_MANA, mana_stored, gain_source, discharge_spell );
        mana_stored = 0;
      }
    }
  };

  p -> register_resource_loss_callback( RESOURCE_MANA, new tyrandes_callback_t( p ) );
}

// register_dragonwrath_tarecgosas_rest =====================================

static void register_dragonwrath_tarecgosas_rest( item_t* item )
{
  player_t* p = item -> player;

  item -> unique = true;

  struct dragonwrath_tarecgosas_rest_callback_t : public discharge_proc_callback_t
  {
    rng_t* rng;

    dragonwrath_tarecgosas_rest_callback_t( player_t* p, double pc ) :
      discharge_proc_callback_t( "dragonwrath_tarecgosas_rest", p, 1, SCHOOL_ARCANE, 1.0, 0.0, pc, 0.0, true, true, true ), rng( 0 )
    {
      rng = p -> get_rng( "dragonwrath_tarecgosas_rest" );
    }

    virtual void trigger( action_t* a, void* call_data )
    {
      double dmg;

      if ( ! a -> may_trigger_dtr ) return;

      if ( a -> tick_dmg > 0 )
        dmg = a -> tick_dmg;
      else
        dmg = a -> direct_dmg;

      discharge_action -> base_dd_min = dmg;
      discharge_action -> base_dd_max = dmg;

      discharge_proc_callback_t::trigger( a, call_data );
    }
  };

  struct dragonwrath_tarecgosas_rest_dd_callback_t : public action_callback_t
  {
    rng_t* rng;
    double chance;

    dragonwrath_tarecgosas_rest_dd_callback_t( player_t* p, double pc ) :
      action_callback_t( p -> sim, p ), rng( 0 ), chance( pc )
    {
      rng = p -> get_rng( "dragonwrath_tarecgosas_rest_dd" );
    }

    virtual void trigger( action_t* a, void* /* call_data */ )
    {
      if ( a -> is_dtr_action )
        return;

      if ( ! a -> dtr_action )
        return;

      if ( chance <= 0 )
        return;

      if ( rng -> roll( chance ) )
      {
        if ( sim -> log )
        {
          log_t::output( sim, "%s action %s procs Dragonwrath Tarecgosas Rest.", a -> player -> name(), a -> name() );
        }
        new ( sim ) action_execute_event_t( sim, a -> dtr_action, 0 /* Add DTR Proc Delay here */ );
      }
    }
  };

  double chance = 0.10;

  if ( p -> sim -> dtr_proc_chance >= 0.0 )
  {
    chance = p -> sim-> dtr_proc_chance;
  }
  if ( p -> dtr_base_proc_chance >= 0.0 )
  {
    chance = p -> dtr_base_proc_chance;
  }

  // FIXME: Need the proper chances here
  switch ( p -> type )
  {
  case DRUID:
  case MAGE:
  case PRIEST:
  case SHAMAN:
  case WARLOCK:
    switch ( p -> primary_tree() )
    {
    // Until we get actual numbers adjust each spec's chance based off testing done against Tier 11 sets.
    // Should probably be re-done when all the Tier 12 sets are available.
    case TREE_BALANCE:      chance = 0.11; break; // http://elitistjerks.com/f73/t110353-balance_cataclysm_4_2_a/p13/#post1998686
    case TREE_ARCANE:       chance *= 1.25; break;
    case TREE_FIRE:         chance *= 1.25; break;
    case TREE_FROST:        chance *= 1.25; break;
    case TREE_SHADOW:
    case TREE_DISCIPLINE:   chance = 0.136; break;
    case TREE_ELEMENTAL:    chance = 0.17; break; // Needs more data
    case TREE_AFFLICTION:   chance = 0.17; break;
    case TREE_DEMONOLOGY:   chance = 0.17; break;
    case TREE_DESTRUCTION:  chance = 0.17; break;
    default:
      // Get a real spec...
      break;
    }
    break;
  default:
    // Seriously?
    break;
  }

  // 4.3 PTR nerf seems to be roughly a 2/3rding of the proc chance - only tested warlocks so far, 2011/10/26
  chance *= 0.67;

  // Moonkin was nerfed to 8% http://elitistjerks.com/f73/t114017-balance_wrathcalcs/p14/#post2041883 and followup posts
  if ( p -> primary_tree() == TREE_BALANCE )
    chance = 0.08;

  // Allow for override
  if ( p -> dtr_proc_chance >= 0.0 )
  {
    chance = p -> dtr_proc_chance;
  }

  action_callback_t* cb = new dragonwrath_tarecgosas_rest_callback_t( p, chance );
  action_callback_t* cb_dd = new dragonwrath_tarecgosas_rest_dd_callback_t( p, chance );

  p -> register_tick_damage_callback( SCHOOL_SPELL_MASK, cb );
  p -> register_direct_damage_callback( SCHOOL_SPELL_MASK, cb_dd );
}

// register_blazing_power ===================================================

static void register_blazing_power( item_t* item )
{
  player_t* p = item -> player;

  item -> unique = true;

  struct blazing_power_heal_t : public heal_t
  {
    blazing_power_heal_t( player_t* p, bool heroic ) :
      heal_t( "blaze_of_life", p, heroic ? 97136 : 96966 )
    {
      trigger_gcd = 0;
      background  = true;
      may_miss = false;
      may_crit = true;
      callbacks = false;
      init();
    }
  };

  struct blazing_power_callback_t : public action_callback_t
  {
    heal_t* heal;
    cooldown_t* cd;
    proc_t* proc;
    rng_t* rng;

    blazing_power_callback_t( player_t* p, heal_t* s ) :
      action_callback_t( p -> sim, p ), heal( s ), proc( 0 ), rng( 0 )
    {
      proc = p -> get_proc( "blazing_power" );
      rng  = p -> get_rng ( "blazing_power" );
      cd = p -> get_cooldown( "blazing_power_callback" );
      cd -> duration = 45.0;
    }

    virtual void trigger( action_t* a, void* /* call_data */ )
    {
      if ( a -> aoe      ||
           a -> proc     ||
           a -> dual     ||
           ! a -> harmful   )
        return;

      if ( cd -> remains() > 0 )
        return;

      if ( rng -> roll( 0.10 ) )
      {
        heal -> heal_target.clear();
        heal -> heal_target.push_back( heal -> find_lowest_player() );
        heal -> execute();
        proc -> occur();
        cd -> start();
      }
    }
  };

  // FIXME: Observe if it procs of non-direct healing spells
  p -> register_heal_callback( RESULT_ALL_MASK, new blazing_power_callback_t( p, new blazing_power_heal_t( p, item -> heroic() ) )  );
}

// register_windward_heart ==================================================

static void register_windward_heart( item_t* item )
{
  player_t* p = item -> player;

  item -> unique = true;

  struct windward_heart_heal_t : public heal_t
  {
    windward_heart_heal_t( player_t* p, bool heroic, bool lfr ) :
      heal_t( "windward", p, heroic ? 109825 : lfr ? 109822 : 108000 )
    {
      trigger_gcd = 0;
      background  = true;
      may_miss = false;
      may_crit = true;
      callbacks = false;
      init();
    }
  };

  struct windward_heart_callback_t : public action_callback_t
  {
    heal_t* heal;
    cooldown_t* cd;
    proc_t* proc;
    rng_t* rng;

    windward_heart_callback_t( player_t* p, heal_t* s ) :
      action_callback_t( p -> sim, p ), heal( s ), proc( 0 ), rng( 0 )
    {
      proc = p -> get_proc( "windward_heart" );
      rng  = p -> get_rng ( "windward_heart" );
      cd = p -> get_cooldown( "windward_heart_callback" );
      cd -> duration = 20.0;
    }

    virtual void trigger( action_t* a, void* /* call_data */ )
    {
      if ( a -> aoe      ||
           a -> proc     ||
           a -> dual     ||
           ! a -> harmful   )
        return;

      if ( cd -> remains() > 0 )
        return;

      if ( rng -> roll( 0.10 ) )
      {
        heal -> heal_target.clear();
        heal -> heal_target.push_back( heal -> find_lowest_player() );
        heal -> execute();
        proc -> occur();
        cd -> start();
      }
    }
  };

  p -> register_heal_callback( RESULT_CRIT_MASK, new windward_heart_callback_t( p, new windward_heart_heal_t( p, item -> heroic(), item -> lfr() ) )  );
}

// register_symbiotic_worm ==================================================

static void register_symbiotic_worm( item_t* item )
{
  player_t* p = item -> player;

  item -> unique = true;

  struct symbiotic_worm_callback_t : public stat_proc_callback_t
  {
    symbiotic_worm_callback_t( player_t* p, bool h ) :
      stat_proc_callback_t( "symbiotic_worm", p, STAT_MASTERY_RATING, 1, h ? 1089 : 963, 1.0, 10.0, 30.0, 0, false, RNG_DEFAULT, false )
    {}

    virtual void trigger( action_t* a, void* call_data )
    {
      if ( listener -> health_percentage() < 35 )
      {
        stat_proc_callback_t::trigger( a, call_data );
      }
    }
  };

  stat_proc_callback_t* cb = new symbiotic_worm_callback_t( p, item -> heroic() );
  p -> register_resource_loss_callback( RESOURCE_HEALTH, cb );
}

// register_indomitable_pride ================================================

static void register_indomitable_pride( item_t* item )
{
  player_t* p = item -> player;

  item -> unique = true;

  struct indomitable_pride_callback_t : public action_callback_t
  {
    buff_t* buff;
    bool heroic, lfr;
    cooldown_t* cd;
    stats_t* stats;
    indomitable_pride_callback_t( player_t* p, bool h, bool l ) :
      action_callback_t( p -> sim, p ), heroic( h ), lfr( l ), cd ( 0 ), stats( 0 )
    {
      // Looks like there is no spell_id_t for the buff
      buff = new buff_t( p, "indomitable_pride", 1, 6.0 );
      buff -> activated = false;
      cd = listener -> get_cooldown( "indomitable_pride" );
      cd -> duration = 60.0;
      p -> absorb_buffs.push_back( buff );
      stats = listener -> get_stats( "indomitable_pride" );
      stats -> type = STATS_ABSORB;
    }

    virtual void trigger( action_t* /* a */, void*  call_data )
    {
      if ( cd -> remains() <= 0 && listener -> health_percentage() < 50 )
      {
        cd -> start();
        double amount = heroic ? 0.56 : lfr ? 0.43 : 0.50;
        if ( call_data )
          amount *= *( ( double * ) call_data );
        else
          assert( 0 );
        buff -> trigger( 1, amount );
        stats -> add_result( amount, amount, ABSORB, RESULT_HIT );
        stats -> add_execute( 0 );
      }
    }
  };

  action_callback_t* cb = new indomitable_pride_callback_t( p, item -> heroic(), item -> lfr() );
  p -> register_resource_loss_callback( RESOURCE_HEALTH, cb );
}

// register_spidersilk_spindle ==============================================

static void register_spidersilk_spindle( item_t* item )
{
  player_t* p = item -> player;

  item -> unique = true;

  struct spidersilk_spindle_callback_t : public action_callback_t
  {
    buff_t* buff;
    cooldown_t* cd;
    stats_t* stats;
    spidersilk_spindle_callback_t( player_t* p, bool h ) :
      action_callback_t( p -> sim, p ), cd ( 0 ), stats( 0 )
    {
      buff = new buff_t( p, h ? 97129 : 96945, "loom_of_fate" );
      buff -> activated = false;
      cd = listener -> get_cooldown( "spidersilk_spindle" );
      cd -> duration = 60.0;
      p -> absorb_buffs.push_back( buff );
      stats = listener -> get_stats( "loom_of_fate" );
      stats -> type = STATS_ABSORB;
    }

    virtual void trigger( action_t* /* a */, void* /* call_data */ )
    {
      if ( cd -> remains() <= 0 && listener -> health_percentage() < 35 )
      {
        cd -> start();
        double amount = buff -> effect1().base_value();
        buff -> trigger( 1, amount );
        stats -> add_result( amount, amount, ABSORB, RESULT_HIT );
        stats -> add_execute( 0 );
      }
    }
  };

  action_callback_t* cb = new spidersilk_spindle_callback_t( p, item -> heroic() );
  p -> register_resource_loss_callback( RESOURCE_HEALTH, cb );
}

// register_bonelink_fetish =================================================

static void register_bonelink_fetish( item_t* item )
{
  player_t* p   = item -> player;
  bool heroic   = item -> heroic();
  bool lfr      = item -> lfr();

  uint32_t spell_id = heroic ? 109755 : lfr ? 109753 : 107998;

  struct bonelink_fetish_callback_t : public action_callback_t
  {
    double chance;
    attack_t* attack;
    cooldown_t* cooldown;
    rng_t* rng;

    struct whirling_maw_t : public attack_t
    {
      whirling_maw_t( player_t* p, uint32_t spell_id ) :
        attack_t( "bonelink_fetish", spell_id, p )
      {
        trigger_gcd = 0;
        background = true;
        may_miss = false;
        may_glance = false;
        may_crit = true;
        proc = true;
        aoe = -1;
        direct_power_mod = extra_coeff();
        init();
      }
    };

    bonelink_fetish_callback_t( player_t* p, uint32_t id ) :
      action_callback_t( p -> sim, p ), chance( p -> dbc.spell( id ) -> proc_chance() )
    {
      attack = new whirling_maw_t( p, p -> dbc.spell( id ) -> effect1().trigger_spell_id() );

      cooldown = p -> get_cooldown( "bonelink_fetish" );
      cooldown -> duration = 25.0; // 25 second ICD

      rng = p -> get_rng ( "bonelink_fetish" );
    }

    virtual void trigger( action_t* a, void* /* call_data */ )
    {
      if ( a -> proc )
        return;

      if ( cooldown -> remains() > 0 )
        return;

      if ( rng -> roll( chance ) )
      {
        attack -> execute();
        cooldown -> start();
      }
    }
  };

  p -> register_attack_callback( RESULT_HIT_MASK, new bonelink_fetish_callback_t( p, spell_id ) );
}

// register_fury_of_the_beast ===============================================

static void register_fury_of_the_beast( item_t* item )
{
  player_t* p = item -> player;

  item -> unique = true;

  struct fury_of_the_beast_callback_t : public action_callback_t
  {
    buff_t* fury_of_the_beast;
    stat_buff_t* fury_of_the_beast_stack;

    // Event
    struct fury_of_the_beast_event_t : public event_t
    {
      buff_t* buff;
      buff_t* buff_stack;

      fury_of_the_beast_event_t ( player_t* player,buff_t* b, buff_t* q ) :
        event_t( player -> sim, player ), buff( b ), buff_stack( q )
      {
        name = "fury_of_the_beast";
        sim -> add_event( this, 1.0 );
      }

      virtual void execute()
      {
        if ( buff -> check() && buff_stack -> check() < buff_stack -> max_stack )
        {
          buff_stack -> buff_duration = buff -> remains(); // hack instead of overriding fury_of_the_beast::expire()
          buff_stack -> trigger();
          new ( sim ) fury_of_the_beast_event_t( player, buff, buff_stack );
        }
      }
    };

    fury_of_the_beast_callback_t( player_t* p, bool h, bool lfr ) :
      action_callback_t( p -> sim, p )
    {
      double amount = h ? 120 : lfr ? 95 : 107; // Amount saved in the stat buff

      fury_of_the_beast = new buff_t( p, h ? 109864 : lfr ? 109861 : 108011, "fury_of_the_beast", 0.15 );
      fury_of_the_beast -> cooldown -> duration = 55.0; // FIXME: Confirm ICD
      fury_of_the_beast_stack  = new stat_buff_t( p, h ? 109863 : lfr ? 109860 : 108016, "fury_of_the_beast_stack", STAT_AGILITY, amount );
      fury_of_the_beast_stack -> activated = false;
    }

    virtual void trigger( action_t* a, void* /* call_data */ )
    {
      if ( ! a -> weapon ) return;
      if ( a -> proc ) return;

      if ( fury_of_the_beast -> cooldown -> remains() > 0 ) return;

      if ( fury_of_the_beast -> trigger() )
      {
        // FIXME: check if the stacking buff ticks at 0s or 1s
        new ( sim ) fury_of_the_beast_event_t( listener, fury_of_the_beast, fury_of_the_beast_stack );
      }
    }
  };

  p -> register_attack_callback( RESULT_HIT_MASK, new fury_of_the_beast_callback_t( p, item -> heroic(), item -> lfr() ) );
}

// register_gurthalak =======================================================

static void register_gurthalak( item_t* item )
{
  player_t* p   = item -> player;
  bool heroic   = item -> heroic();
  bool lfr      = item -> lfr();
  int  slot     = item -> slot;

  uint32_t tick_damage = heroic ? 12591 : lfr ? 9881 : 11155;

  uint32_t proc_spell_id = heroic ? 109839 : lfr ? 109841 : 107810;

  struct gurthalak_callback_t : public action_callback_t
  {
    double chance;
    spell_t* spell[5];
    rng_t* rng;
    int slot;
    int current_spell;

    // FIXME: This should be converted to a pet, which casts 3 Mind Flays,
    // of 3 ticks each, with the last being 2-3 ticks
    // Reia is working on it and it's proving difficult
    // So until that can be done, use 5 spells to act as 5 spawns
    // and simulate 8-9 ticks on one mind flay, with a dot refresh
    // OH modeling of this for Fury is undervalued
    struct gurthalak_t : public spell_t
    {
      gurthalak_t( player_t* p, uint32_t tick_damage, const char* name ) :
        spell_t( name, 52586, p )
      {
        trigger_gcd = 0;
        background = true;
        tick_may_crit = true;
        hasted_ticks = false;
        proc = true;
        num_ticks = 9; // Casts 3 mind flays, resulting in 8-9 ticks on average
        base_attack_power_multiplier = 1.0;
        base_spell_power_multiplier = 0;

        // Override stats so all 5 tentacles are merged into 1
        stats = p -> get_stats( "gurthalak_voice_of_the_deeps" );
        stats -> school = SCHOOL_SHADOW; // Fix for reporting

        // While this spell ID is the one used by all of the tentacles,
        // It doesn't have a coeff and each version has static damage
        tick_power_mod = 0;
        base_td = tick_damage;
        base_cost = 0; // Override this, otherwise it screws up reporting

        // Change to DOT_REFRESH in-case we somehow RNG to all holy hell and get 6 up at once
        dot_behavior = DOT_REFRESH;

        init();
      }

      virtual void execute()
      {
        // Casts either 8 or 9 ticks, roughly equal chance for both
        num_ticks = sim -> roll( 0.5 ) ? 9 : 8;

        spell_t::execute();
      }
    };

    gurthalak_callback_t( player_t* p, uint32_t tick_damage, uint32_t proc_spell_id, int slot ) :
      action_callback_t( p -> sim, p ), chance( p -> dbc.spell( proc_spell_id ) -> proc_chance() ),
      slot( slot ), current_spell( 0 )
    {
      // Init 5 Gurths to act like multiple tentacles up at once
      spell[0] = new gurthalak_t( p, tick_damage, "gurthalak_voice_of_the_deeps0" );
      spell[1] = new gurthalak_t( p, tick_damage, "gurthalak_voice_of_the_deeps1" );
      spell[2] = new gurthalak_t( p, tick_damage, "gurthalak_voice_of_the_deeps2" );
      spell[3] = new gurthalak_t( p, tick_damage, "gurthalak_voice_of_the_deeps3" );
      spell[4] = new gurthalak_t( p, tick_damage, "gurthalak_voice_of_the_deeps4" );

      rng = p -> get_rng ( "gurthalak" );
    }

    virtual void trigger( action_t* a, void* /* call_data */ )
    {
      if ( a -> proc )
        return;

      // This currently has some odd spawning when equipped in the OH
      // We don't currently have a way to replicate this behavior perfectly in the sim, that I'm aware of
      // Changing to it to slot based will under value the weapon whenever it is in the OH for Fury Warriors
      /* 
        Bloodthirst? Yes.
        Heroic Strike? Yes.
        Cleave? Yes.
        Slam? No.
        Raging Blow? Offhand hit can proc it, main hand can't.
        Whirlwind? Same as Raging Blow. 1 roll per offhand hit, can proc multiple times from the same ability if it hits multiple times.
        Colossus Smash? No.
        Thunder Clap? Yes.
        Heroic Leap? Yes.
      */

      if ( ! a -> weapon ) return;
      if ( a -> weapon -> slot != slot ) return;

      if ( rng -> roll( chance ) )
      {
        // Control our spawns of Gurth
        current_spell++;
        if ( current_spell > 4 )
          current_spell = 0;

        spell[ current_spell ] -> execute();
      }
    }
  };

  p -> register_attack_callback( RESULT_HIT_MASK, new gurthalak_callback_t( p, tick_damage, proc_spell_id, slot ) );
}

// register_nokaled =========================================================

static void register_nokaled( item_t* item )
{
  player_t* p   = item -> player;
  bool heroic   = item -> heroic();
  bool lfr      = item -> lfr();
  int slot      = item -> slot;
                                      //Fire  Frost   Shadow
  static uint32_t    lfr_spells[] = { 109871, 109869, 109867 };
  static uint32_t normal_spells[] = { 107785, 107789, 107787 };
  static uint32_t heroic_spells[] = { 109872, 109870, 109868 };

  uint32_t* spell_ids = heroic ? heroic_spells : lfr ? lfr_spells : normal_spells;

  uint32_t proc_spell_id = heroic ? 109873 : lfr ? 109866 : 107786;

  struct nokaled_callback_t : public action_callback_t
  {
    double chance;
    spell_t* spells[3];
    rng_t* rng;
    int slot;

    // FIXME: Verify if spells can miss
    struct nokaled_fire_t : public spell_t
    {
      nokaled_fire_t( player_t* p, uint32_t spell_id ) :
        spell_t( "nokaled_fireblast", spell_id, p )
      {
        trigger_gcd = 0;
        background = true;
        may_miss = false;
        may_crit = true;
        proc = true;
        init();
      }
    };

    struct nokaled_frost_t : public spell_t
    {
      nokaled_frost_t( player_t* p, uint32_t spell_id ) :
        spell_t( "nokaled_iceblast", spell_id, p )
      {
        trigger_gcd = 0;
        background = true;
        may_miss = false;
        may_crit = true;
        proc = true;
        init();
      }
    };

    struct nokaled_shadow_t : public spell_t
    {
      nokaled_shadow_t( player_t* p, uint32_t spell_id ) :
        spell_t( "nokaled_shadowblast", spell_id, p )
      {
        trigger_gcd = 0;
        background = true;
        may_miss = false;
        may_crit = true;
        proc = true;
        init();
      }
    };

    nokaled_callback_t( player_t* p, uint32_t ids[], uint32_t proc_spell_id, int slot ) :
      action_callback_t( p -> sim, p ), chance( p -> dbc.spell( proc_spell_id ) -> proc_chance() ),
      slot( slot )
    {
      spells[ 0 ] = new   nokaled_fire_t( p, ids[ 0 ] );
      spells[ 1 ] = new  nokaled_frost_t( p, ids[ 1 ] );
      spells[ 2 ] = new nokaled_shadow_t( p, ids[ 2 ] );

      rng = p -> get_rng ( "nokaled" );
      rng -> average_range = false; // Otherwise we'll always get the mean
    }

    virtual void trigger( action_t* a, void* /* call_data */ )
    {
      if ( a -> proc )
        return;

      // Have to have a weapon to trigger
      if ( ! a -> weapon )
        return;

      // Only attacks from the weapon that have No'Kaled can trigger it
      if ( a -> weapon -> slot != slot )
        return;

      if ( rng -> roll( chance ) )
      {
        spells[ ( int ) ( rng -> range( 0.0, 2.999 ) ) ] -> execute();
      }
    }
  };

  p -> register_attack_callback( RESULT_HIT_MASK, new nokaled_callback_t( p, spell_ids, proc_spell_id, slot ) );
}

// register_rathrak =========================================================

static void register_rathrak( item_t* item )
{
  player_t* p = item -> player;
  bool heroic = item -> heroic();
  bool lfr    = item -> lfr();
  uint32_t trigger_spell_id = heroic ? 109854 : lfr ? 109851 : 107831;

  struct rathrak_poison_t : public spell_t
  {
    rathrak_poison_t( player_t* p, uint32_t spell_id ) :
      spell_t( "rathrak", spell_id, p )
    {
      trigger_gcd = 0;
      background = true;
      may_miss = false; // FIXME: Verify this
      tick_may_crit = true; 
      proc = true;
      hasted_ticks = false;
      init();
      cooldown -> duration = 17.0; // FIXME: Verify this. Got 17.188sec after 545 procs.
    }
    // Testing shows this is affected by CoE and Spell Dmg modifiers (Shadow Power, etc)
  };

  struct rathrak_callback_t : public action_callback_t
  {
    spell_t* spell;
    rng_t* rng;

    rathrak_callback_t( player_t* p, spell_t* s ) :
      action_callback_t( p -> sim, p ), spell( s )
    {
      rng  = p -> get_rng ( "rathrak", RNG_DEFAULT );
    }

    virtual void trigger( action_t* /* a */, void* /* call_data */ )
    {
      if ( ( spell -> cooldown -> remains() <= 0.0 ) && rng -> roll( 0.15 ) )
      {
        spell -> execute();
      }
    }
  };

  p -> register_harmful_spell_callback( RESULT_HIT_MASK, new rathrak_callback_t( p, new rathrak_poison_t( p, trigger_spell_id ) ) );
}

// register_souldrinker =====================================================

static void register_souldrinker( item_t* item )
{
  player_t* p = item -> player;
  bool heroic = item -> heroic();
  bool lfr    = item -> lfr();
  int slot    = item -> slot;

  struct souldrinker_spell_t : public spell_t
  {
    souldrinker_spell_t( player_t* p, bool h, bool lfr ) :
      spell_t( "souldrinker", h ? 109831 : lfr ? 109828 : 108022, p )
    {
      trigger_gcd = 0;
      background = true;
      may_miss = false;
      may_crit = false;
      proc = true;
      init();
    }

    virtual void execute()
    {
      base_dd_min = base_dd_max = effect1().percent() / 10.0 * player -> resource_max[ RESOURCE_HEALTH ];
      spell_t::execute();
    }

    virtual void player_buff() { }

    virtual double total_dd_multiplier() const { return 1.0; }
  };

  struct souldrinker_callback_t : public action_callback_t
  {
    spell_t* spell;
    rng_t* rng;
    int slot;

    souldrinker_callback_t( player_t* p, spell_t* s, int slot ) :
      action_callback_t( p -> sim, p ), spell( s ), slot( slot )
    {
      rng  = p -> get_rng ( "souldrinker", RNG_DEFAULT );
    }

    virtual void trigger( action_t* a, void* /* call_data */ )
    {
      // Only the slot the weapon is in can trigger it, e.g. a Souldrinker in the MH can't proc from OH attacks
      // http://elitistjerks.com/f72/t125291-frost_dps_winter_discontent_4_3_a/p12/#post2055642
      if ( ! a -> weapon ) return;
      if ( a -> weapon -> slot != slot ) return;

      // No ICD
      if ( rng -> roll( 0.15 ) )
      {
        spell -> execute();
      }
    }
  };

  p -> register_attack_callback( RESULT_HIT_MASK, new souldrinker_callback_t( p, new souldrinker_spell_t( p, heroic, lfr ), slot ) );
}

// register_titahk ==========================================================

static void register_titahk( item_t* item )
{
  player_t* p = item -> player;
  bool heroic = item -> heroic();
  bool lfr    = item -> lfr();

  uint32_t spell_id = heroic ? 109846 : lfr ? 109843 : 107805;
  uint32_t buff_id = p -> dbc.spell( spell_id ) -> effect1().trigger_spell_id();

  const spell_data_t* spell = p -> dbc.spell( spell_id );
  const spell_data_t* buff  = p -> dbc.spell( buff_id );

  struct titahk_callback_t : public action_callback_t
  {
    double proc_chance;
    rng_t* rng;
    buff_t* buff_self;
    buff_t* buff_radius; // This buff should be in 20 yards radius but it is contained only on the player for simulation.

    titahk_callback_t( player_t* p, const spell_data_t* spell, const spell_data_t* buff ) :
      action_callback_t( p -> sim, p ),
      proc_chance( spell -> proc_chance() ),
      rng( p -> get_rng( "titahk" ) )
    {
      double duration = buff -> duration();
      buff_self   = new stat_buff_t( p, "titahk_self", STAT_HASTE_RATING, buff -> effect1().base_value(), 1, duration );
      buff_self -> cooldown -> duration = 45.0; // FIXME: Confirm ICD
      buff_radius = new stat_buff_t( p, "titahk_aoe",  STAT_HASTE_RATING, buff -> effect2().base_value(), 1, duration ); // FIXME: Apply aoe buff to other players
      buff_radius -> cooldown -> duration = 45.0; // FIXME: Confirm ICD
    }

    virtual void trigger( action_t* /* a */, void* /* call_data */ )
    {
      // FIXME: Does this have an ICD?
      if ( ( buff_self -> cooldown -> remains() <= 0.0 ) && rng -> roll( proc_chance ) )
      {
        buff_self -> trigger();
        buff_radius -> trigger();
      }
    }
  };

  p -> register_spell_callback( SCHOOL_SPELL_MASK, new titahk_callback_t( p, spell, buff ) );
}

// ==========================================================================
// unique_gear_t::init
// ==========================================================================

void unique_gear_t::init( player_t* p )
{
  if ( p -> is_pet() ) return;

  int num_items = ( int ) p -> items.size();

  for ( int i=0; i < num_items; i++ )
  {
    item_t& item = p -> items[ i ];

    if ( item.equip.stat && item.equip.school )
    {
      register_stat_discharge_proc( item, item.equip );
    }
    else if ( item.equip.stat )
    {
      register_stat_proc( item, item.equip );
    }
    else if ( item.equip.cost_reduction && item.equip.school )
    {
      register_cost_reduction_proc( item, item.equip );
    }
    else if ( item.equip.school && item.equip.proc_chance && item.equip.chance_to_discharge )
    {
      register_chance_discharge_proc( item, item.equip );
    }
    else if ( item.equip.school )
    {
      register_discharge_proc( item, item.equip );
    }

    if ( ! strcmp( item.name(), "apparatus_of_khazgoroth"             ) ) register_apparatus_of_khazgoroth           ( &item );
    if ( ! strcmp( item.name(), "bonelink_fetish"                     ) ) register_bonelink_fetish                   ( &item );
    if ( ! strcmp( item.name(), "darkmoon_card_greatness"             ) ) register_darkmoon_card_greatness           ( &item );
    if ( ! strcmp( item.name(), "dragonwrath_tarecgosas_rest"         ) ) register_dragonwrath_tarecgosas_rest       ( &item );
    if ( ! strcmp( item.name(), "eye_of_blazing_power"                ) ) register_blazing_power                     ( &item );
    if ( ! strcmp( item.name(), "fury_of_angerforge"                  ) ) register_fury_of_angerforge                ( &item );
    if ( ! strcmp( item.name(), "gurthalak_voice_of_the_deeps"        ) ) register_gurthalak                         ( &item );
    if ( ! strcmp( item.name(), "heart_of_ignacious"                  ) ) register_heart_of_ignacious                ( &item );
    if ( ! strcmp( item.name(), "indomitable_pride"                   ) ) register_indomitable_pride                 ( &item );
    if ( ! strcmp( item.name(), "kiril_fury_of_beasts"                ) ) register_fury_of_the_beast                 ( &item );
    if ( ! strcmp( item.name(), "matrix_restabilizer"                 ) ) register_matrix_restabilizer               ( &item );
    if ( ! strcmp( item.name(), "nokaled_the_elements_of_death"       ) ) register_nokaled                           ( &item );
    if ( ! strcmp( item.name(), "rathrak_the_poisonous_mind"          ) ) register_rathrak                           ( &item );
    if ( ! strcmp( item.name(), "shard_of_woe"                        ) ) register_shard_of_woe                      ( &item );
    if ( ! strcmp( item.name(), "sorrowsong"                          ) ) register_sorrowsong                        ( &item );
    if ( ! strcmp( item.name(), "souldrinker"                         ) ) register_souldrinker                       ( &item );
    if ( ! strcmp( item.name(), "spidersilk_spindle"                  ) ) register_spidersilk_spindle                ( &item );
    if ( ! strcmp( item.name(), "symbiotic_worm"                      ) ) register_symbiotic_worm                    ( &item );
    if ( ! strcmp( item.name(), "tyrandes_favorite_doll"              ) ) register_tyrandes_favorite_doll            ( &item );
    if ( ! strcmp( item.name(), "windward_heart"                      ) ) register_windward_heart                    ( &item );
    if ( ! strcmp( item.name(), "titahk_the_steps_of_time"            ) ) register_titahk                            ( &item );
  }
}

// ==========================================================================
// unique_gear_t::register_stat_proc
// ==========================================================================

action_callback_t* unique_gear_t::register_stat_proc( int                type,
                                                      int64_t            mask,
                                                      const std::string& name,
                                                      player_t*          player,
                                                      int                stat,
                                                      int                max_stacks,
                                                      double             amount,
                                                      double             proc_chance,
                                                      double             duration,
                                                      double             cooldown,
                                                      double             tick,
                                                      bool               reverse,
                                                      int                rng_type )
{
  action_callback_t* cb = new stat_proc_callback_t( name, player, stat, max_stacks, amount, proc_chance, duration, cooldown, tick, reverse, rng_type, type == PROC_NONE );

  if ( type == PROC_DAMAGE || type == PROC_DAMAGE_HEAL )
  {
    player -> register_tick_damage_callback( mask, cb );
    player -> register_direct_damage_callback( mask, cb );
  }
  if ( type == PROC_HEAL || type == PROC_DAMAGE_HEAL )
  {
    player -> register_tick_heal_callback( mask, cb );
    player -> register_direct_heal_callback( mask, cb );
  }
  else if ( type == PROC_TICK_DAMAGE )
  {
    player -> register_tick_damage_callback( mask, cb );
  }
  else if ( type == PROC_DIRECT_DAMAGE )
  {
    player -> register_direct_damage_callback( mask, cb );
  }
  else if ( type == PROC_ATTACK )
  {
    player -> register_attack_callback( mask, cb );
  }
  else if ( type == PROC_SPELL )
  {
    player -> register_spell_callback( mask, cb );
  }
  else if ( type == PROC_TICK )
  {
    player -> register_tick_callback( mask, cb );
  }
  else if ( type == PROC_HARMFUL_SPELL )
  {
    player -> register_harmful_spell_callback( mask, cb );
  }
  else if ( type == PROC_HEAL_SPELL )
  {
    player -> register_heal_callback( mask, cb );
  }

  return cb;
}

// ==========================================================================
// unique_gear_t::register_cost_reduction_proc
// ==========================================================================

action_callback_t* unique_gear_t::register_cost_reduction_proc( int                type,
                                                                int64_t            mask,
                                                                const std::string& name,
                                                                player_t*          player,
                                                                int                school,
                                                                int                max_stacks,
                                                                double             amount,
                                                                double             proc_chance,
                                                                double             duration,
                                                                double             cooldown,
                                                                bool               refreshes,
                                                                bool               reverse,
                                                                int                rng_type )
{
  action_callback_t* cb = new cost_reduction_proc_callback_t( name, player, school, max_stacks, amount, proc_chance, duration, cooldown, refreshes, reverse, rng_type, type == PROC_NONE );

  if ( type == PROC_DAMAGE || type == PROC_DAMAGE_HEAL )
  {
    player -> register_tick_damage_callback( mask, cb );
    player -> register_direct_damage_callback( mask, cb );
  }
  if ( type == PROC_HEAL || type == PROC_DAMAGE_HEAL )
  {
    player -> register_tick_heal_callback( mask, cb );
    player -> register_direct_heal_callback( mask, cb );
  }
  else if ( type == PROC_TICK_DAMAGE )
  {
    player -> register_tick_damage_callback( mask, cb );
  }
  else if ( type == PROC_DIRECT_DAMAGE )
  {
    player -> register_direct_damage_callback( mask, cb );
  }
  else if ( type == PROC_TICK )
  {
    player -> register_tick_callback( mask, cb );
  }
  else if ( type == PROC_ATTACK )
  {
    player -> register_attack_callback( mask, cb );
  }
  else if ( type == PROC_SPELL )
  {
    player -> register_spell_callback( mask, cb );
  }
  else if ( type == PROC_HARMFUL_SPELL )
  {
    player -> register_harmful_spell_callback( mask, cb );
  }
  else if ( type == PROC_HEAL_SPELL )
  {
    player -> register_heal_callback( mask, cb );
  }

  return cb;
}

// ==========================================================================
// unique_gear_t::register_discharge_proc
// ==========================================================================

action_callback_t* unique_gear_t::register_discharge_proc( int                type,
                                                           int64_t            mask,
                                                           const std::string& name,
                                                           player_t*          player,
                                                           int                max_stacks,
                                                           const school_type  school,
                                                           double             amount,
                                                           double             scaling,
                                                           double             proc_chance,
                                                           double             cooldown,
                                                           bool               no_crit,
                                                           bool               no_buffs,
                                                           bool               no_debuffs,
                                                           int                rng_type )
{
  action_callback_t* cb = new discharge_proc_callback_t( name, player, max_stacks, school, amount, scaling, proc_chance, cooldown,
                                                         no_crit, no_buffs, no_debuffs, rng_type );

  if ( type == PROC_DAMAGE || type == PROC_DAMAGE_HEAL )
  {
    player -> register_tick_damage_callback( mask, cb );
    player -> register_direct_damage_callback( mask, cb );
  }
  if ( type == PROC_HEAL || type == PROC_DAMAGE_HEAL )
  {
    player -> register_tick_heal_callback( mask, cb );
    player -> register_direct_heal_callback( mask, cb );
  }
  else if ( type == PROC_TICK_DAMAGE )
  {
    player -> register_tick_damage_callback( mask, cb );
  }
  else if ( type == PROC_DIRECT_DAMAGE )
  {
    player -> register_direct_damage_callback( mask, cb );
  }
  else if ( type == PROC_TICK )
  {
    player -> register_tick_callback( mask, cb );
  }
  else if ( type == PROC_ATTACK )
  {
    player -> register_attack_callback( mask, cb );
  }
  else if ( type == PROC_SPELL )
  {
    player -> register_spell_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_AND_TICK )
  {
    player -> register_spell_callback( mask, cb );
    player -> register_tick_callback( mask, cb );
  }
  else if ( type == PROC_HARMFUL_SPELL )
  {
    player -> register_harmful_spell_callback( mask, cb );
  }
  else if ( type == PROC_HEAL_SPELL )
  {
    player -> register_heal_callback( mask, cb );
  }

  return cb;
}

// ==========================================================================
// unique_gear_t::register_chance_discharge_proc
// ==========================================================================

action_callback_t* unique_gear_t::register_chance_discharge_proc( int                type,
                                                                  int64_t            mask,
                                                                  const std::string& name,
                                                                  player_t*          player,
                                                                  int                max_stacks,
                                                                  const school_type  school,
                                                                  double             amount,
                                                                  double             scaling,
                                                                  double             proc_chance,
                                                                  double             cooldown,
                                                                  bool               no_crit,
                                                                  bool               no_buffs,
                                                                  bool               no_debuffs,
                                                                  int                rng_type )
{
  action_callback_t* cb = new chance_discharge_proc_callback_t( name, player, max_stacks, school, amount, scaling, proc_chance, cooldown,
                                                                no_crit, no_buffs, no_debuffs, rng_type );

  if ( type == PROC_DAMAGE || type == PROC_DAMAGE_HEAL )
  {
    player -> register_tick_damage_callback( mask, cb );
    player -> register_direct_damage_callback( mask, cb );
  }
  if ( type == PROC_HEAL  || type == PROC_DAMAGE_HEAL )
  {
    player -> register_tick_heal_callback( mask, cb );
    player -> register_direct_heal_callback( mask, cb );
  }
  else if ( type == PROC_TICK_DAMAGE )
  {
    player -> register_tick_damage_callback( mask, cb );
  }
  else if ( type == PROC_DIRECT_DAMAGE )
  {
    player -> register_direct_damage_callback( mask, cb );
  }
  else if ( type == PROC_TICK )
  {
    player -> register_tick_callback( mask, cb );
  }
  else if ( type == PROC_ATTACK )
  {
    player -> register_attack_callback( mask, cb );
  }
  else if ( type == PROC_SPELL )
  {
    player -> register_spell_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_AND_TICK )
  {
    player -> register_spell_callback( mask, cb );
    player -> register_tick_callback( mask, cb );
  }
  else if ( type == PROC_HARMFUL_SPELL )
  {
    player -> register_harmful_spell_callback( mask, cb );
  }
  else if ( type == PROC_HEAL_SPELL )
  {
    player -> register_heal_callback( mask, cb );
  }

  return cb;
}

// ==========================================================================
// unique_gear_t::register_stat_discharge_proc
// ==========================================================================

action_callback_t* unique_gear_t::register_stat_discharge_proc( int                type,
                                                                int64_t            mask,
                                                                const std::string& name,
                                                                player_t*          player,
                                                                int                max_stacks,
                                                                int                stat,
                                                                double             stat_amount,
                                                                const school_type  school,
                                                                double             min_dmg,
                                                                double             max_dmg,
                                                                double             proc_chance,
                                                                double             duration,
                                                                double             cooldown,
                                                                bool               no_crit,
                                                                bool               no_buffs,
                                                                bool               no_debuffs )
{
  action_callback_t* cb = new stat_discharge_proc_callback_t( name, player, stat, max_stacks, stat_amount, school, min_dmg, max_dmg, proc_chance,
                                                              duration, cooldown, no_crit, no_buffs, no_debuffs, type == PROC_NONE );

  if ( type == PROC_DAMAGE || type == PROC_DAMAGE_HEAL )
  {
    player -> register_tick_damage_callback( mask, cb );
    player -> register_direct_damage_callback( mask, cb );
  }
  if ( type == PROC_HEAL  || type == PROC_DAMAGE_HEAL )
  {
    player -> register_tick_heal_callback( mask, cb );
    player -> register_direct_heal_callback( mask, cb );
  }
  else if ( type == PROC_TICK_DAMAGE )
  {
    player -> register_tick_damage_callback( mask, cb );
  }
  else if ( type == PROC_DIRECT_DAMAGE )
  {
    player -> register_direct_damage_callback( mask, cb );
  }
  else if ( type == PROC_TICK )
  {
    player -> register_tick_callback( mask, cb );
  }
  else if ( type == PROC_ATTACK )
  {
    player -> register_attack_callback( mask, cb );
  }
  else if ( type == PROC_SPELL )
  {
    player -> register_spell_callback( mask, cb );
  }
  else if ( type == PROC_HARMFUL_SPELL )
  {
    player -> register_harmful_spell_callback( mask, cb );
  }
  else if ( type == PROC_HEAL_SPELL )
  {
    player -> register_heal_callback( mask, cb );
  }

  return cb;
}

// ==========================================================================
// unique_gear_t::register_stat_proc
// ==========================================================================

action_callback_t* unique_gear_t::register_stat_proc( item_t& i,
                                                      item_t::special_effect_t& e )
{
  const char* name = e.name_str.empty() ? i.name() : e.name_str.c_str();

  return register_stat_proc( e.trigger_type, e.trigger_mask, name, i.player,
                             e.stat, e.max_stacks, e.stat_amount,
                             e.proc_chance, e.duration, e.cooldown, e.tick, e.reverse, RNG_DEFAULT );
}

// ==========================================================================
// unique_gear_t::register_cost_reduction_proc
// ==========================================================================

action_callback_t* unique_gear_t::register_cost_reduction_proc( item_t& i,
                                                                item_t::special_effect_t& e )
{
  const char* name = e.name_str.empty() ? i.name() : e.name_str.c_str();

  return register_cost_reduction_proc( e.trigger_type, e.trigger_mask, name, i.player,
                                       e.school, e.max_stacks, e.discharge_amount,
                                       e.proc_chance, e.duration, e.cooldown, ! e.no_refresh, e.reverse, RNG_DEFAULT );
}

// ==========================================================================
// unique_gear_t::register_discharge_proc
// ==========================================================================

action_callback_t* unique_gear_t::register_discharge_proc( item_t& i,
                                                           item_t::special_effect_t& e )
{
  const char* name = e.name_str.empty() ? i.name() : e.name_str.c_str();

  return register_discharge_proc( e.trigger_type, e.trigger_mask, name, i.player,
                                  e.max_stacks, e.school, e.discharge_amount, e.discharge_scaling,
                                  e.proc_chance, e.cooldown, e.no_crit, e.no_player_benefits, e.no_debuffs );
}

// ==========================================================================
// unique_gear_t::register_chance_discharge_proc
// ==========================================================================

action_callback_t* unique_gear_t::register_chance_discharge_proc( item_t& i,
                                                                  item_t::special_effect_t& e )
{
  const char* name = e.name_str.empty() ? i.name() : e.name_str.c_str();

  return register_chance_discharge_proc( e.trigger_type, e.trigger_mask, name, i.player,
                                         e.max_stacks, e.school, e.discharge_amount, e.discharge_scaling,
                                         e.proc_chance, e.cooldown, e.no_crit, e.no_player_benefits, e.no_debuffs );
}

// ==========================================================================
// unique_gear_t::register_stat_discharge_proc
// ==========================================================================

action_callback_t* unique_gear_t::register_stat_discharge_proc( item_t& i,
                                                                item_t::special_effect_t& e )
{
  const char* name = e.name_str.empty() ? i.name() : e.name_str.c_str();

  return register_stat_discharge_proc( e.trigger_type, e.trigger_mask, name, i.player,
                                       e.max_stacks, e.stat, e.stat_amount,
                                       e.school, e.discharge_amount, e.discharge_scaling,
                                       e.proc_chance, e.duration, e.cooldown, e.no_crit, e.no_player_benefits, e.no_debuffs );
}

// ==========================================================================
// unique_gear_t::get_equip_encoding
// ==========================================================================

bool unique_gear_t::get_equip_encoding( std::string&       encoding,
                                        const std::string& name,
                                        const bool         heroic,
                                        const bool         lfr,
                                        const bool         /* ptr */,
                                        const std::string& /* id */ )
{
  std::string e;

  // Stat Procs
  if      ( name == "abyssal_rune"                        ) e = "OnSpellCast_590SP_25%_10Dur_45Cd";
  else if ( name == "anhuurs_hymnal"                      ) e = ( heroic ? "OnSpellCast_1710SP_10%_10Dur_50Cd" : "OnSpellCast_1512SP_10%_10Dur_50Cd" );
  else if ( name == "ashen_band_of_endless_destruction"   ) e = "OnSpellHit_285SP_10%_10Dur_60Cd";
  else if ( name == "ashen_band_of_unmatched_destruction" ) e = "OnSpellHit_285SP_10%_10Dur_60Cd";
  else if ( name == "ashen_band_of_endless_vengeance"     ) e = "OnAttackHit_480AP_1PPM_10Dur_60Cd";
  else if ( name == "ashen_band_of_unmatched_vengeance"   ) e = "OnAttackHit_480AP_1PPM_10Dur_60Cd";
  else if ( name == "ashen_band_of_endless_might"         ) e = "OnAttackHit_480AP_1PPM_10Dur_60Cd";
  else if ( name == "ashen_band_of_unmatched_might"       ) e = "OnAttackHit_480AP_1PPM_10Dur_60Cd";
  else if ( name == "banner_of_victory"                   ) e = "OnAttackHit_1008AP_20%_10Dur_50Cd";
  else if ( name == "bell_of_enraging_resonance"          ) e = ( heroic ? "OnSpellCast_2178SP_30%_20Dur_100Cd" : "OnSpellCast_1926SP_30%_20Dur_100Cd" );
  else if ( name == "black_magic"                         ) e = "OnSpellHit_250Haste_35%_10Dur_35Cd";
  else if ( name == "blood_of_the_old_god"                ) e = "OnAttackCrit_1284AP_10%_10Dur_50Cd";
  else if ( name == "chuchus_tiny_box_of_horrors"         ) e = "OnAttackHit_258Crit_15%_10Dur_45Cd";
  else if ( name == "comets_trail"                        ) e = "OnAttackHit_726Haste_10%_10Dur_45Cd";
  else if ( name == "corens_chromium_coaster"             ) e = "OnAttackCrit_1000AP_10%_10Dur_50Cd";
  else if ( name == "corens_chilled_chromium_coaster"     ) e = "OnAttackCrit_4000AP_10%_10Dur_50Cd";
  else if ( name == "crushing_weight"                     ) e = ( heroic ? "OnAttackHit_2178Haste_10%_15Dur_75Cd" : "OnAttackHit_1926Haste_10%_15Dur_75Cd" );
  else if ( name == "dark_matter"                         ) e = "OnAttackHit_612Crit_15%_10Dur_45Cd";
  else if ( name == "darkmoon_card_crusade"               ) e = "OnDamage_8SP_10Stack_10Dur";
  else if ( name == "dwyers_caber"                        ) e = "OnDamage_1020Crit_15%_20Dur_50Cd";
  else if ( name == "dying_curse"                         ) e = "OnSpellCast_765SP_15%_10Dur_45Cd";
  else if ( name == "elemental_focus_stone"               ) e = "OnSpellCast_522Haste_10%_10Dur_45Cd";
  else if ( name == "embrace_of_the_spider"               ) e = "OnSpellCast_505Haste_10%_10Dur_45Cd";
  else if ( name == "essence_of_the_cyclone"              ) e = ( heroic ? "OnAttackHit_2178Crit_10%_10Dur_50Cd" : "OnAttackHit_1926Crit_10%_10Dur_50Cd" );
  else if ( name == "eye_of_magtheridon"                  ) e = "OnSpellMiss_170SP_10Dur";
  else if ( name == "eye_of_the_broodmother"              ) e = "OnSpellDamageHeal_25SP_5Stack_10Dur";
  else if ( name == "flare_of_the_heavens"                ) e = "OnSpellCast_850SP_10%_10Dur_45Cd";
  else if ( name == "fluid_death"                         ) e = "OnAttackHit_38Agi_10Stack_15Dur";
  else if ( name == "forge_ember"                         ) e = "OnSpellHit_512SP_10%_10Dur_45Cd";
  else if ( name == "fury_of_the_five_flights"            ) e = "OnAttackHit_16AP_20Stack_10Dur";
  else if ( name == "gale_of_shadows"                     ) e = ( heroic ? "OnSpellTickDamage_17SP_20Stack_15Dur" : "OnSpellTickDamage_15SP_20Stack_15Dur" );
  else if ( name == "grace_of_the_herald"                 ) e = ( heroic ? "OnAttackHit_1710Crit_10%_10Dur_75Cd" : "OnAttackHit_924Crit_10%_10Dur_75Cd" );
  else if ( name == "grim_toll"                           ) e = "OnAttackHit_612Crit_15%_10Dur_45Cd";
  else if ( name == "harrisons_insignia_of_panache"       ) e = "OnAttackHit_918Mastery_10%_20Dur_95Cd"; // TO-DO: Confirm ICD
  else if ( name == "heart_of_rage"                       ) e = ( heroic ? "OnAttackHit_2178Str_10%_20Dur_100Cd" : "OnAttackHit_1926Str_10%_20Dur_100Cd" ); // TO-DO: Confirm ICD.
  else if ( name == "heart_of_solace"                     ) e = ( heroic ? "OnAttackHit_1710Str_10%_20Dur_100Cd" : "OnAttackHit_1512Str_10%_20Dur_100Cd" );
  else if ( name == "heart_of_the_vile"                   ) e = "OnAttackHit_924Crit_10%_10Dur_75Cd";
  else if ( name == "heartsong"                           ) e = "OnSpellCast_200Spi_25%_15Dur_20Cd";
  else if ( name == "herkuml_war_token"                   ) e = "OnAttackHit_17AP_20Stack_10Dur";
  else if ( name == "illustration_of_the_dragon_soul"     ) e = "OnSpellCast_20SP_10Stack_10Dur";
  else if ( name == "key_to_the_endless_chamber"          ) e = ( heroic ? "OnAttackHit_1710Agi_10%_15Dur_75Cd" : "OnAttackHit_1290Agi_10%_15Dur_75Cd" );
  else if ( name == "left_eye_of_rajh"                    ) e = ( heroic ? "OnAttackCrit_1710Agi_50%_10Dur_50Cd" : "OnAttackCrit_1512Agi_50%_10Dur_50Cd" );
  else if ( name == "license_to_slay"                     ) e = "OnAttackHit_38Str_10Stack_15Dur";
  else if ( name == "mark_of_defiance"                    ) e = "OnSpellHit_150Mana_15%_15Cd";
  else if ( name == "mirror_of_truth"                     ) e = "OnAttackCrit_1000AP_10%_10Dur_50Cd";
  else if ( name == "mithril_pocketwatch"                 ) e = "OnSpellCast_590SP_10%_10Dur_45Cd";
  else if ( name == "mithril_stopwatch"                   ) e = "OnSpellCast_2040SP_10%_10Dur_45Cd"; // FIXME: Confirm ICD
  else if ( name == "mjolnir_runestone"                   ) e = "OnAttackHit_665Haste_15%_10Dur_45Cd";
  else if ( name == "muradins_spyglass"                   ) e = ( heroic ? "OnSpellDamage_20SP_10Stack_10Dur" : "OnSpellDamage_18SP_10Stack_10Dur" );
  else if ( name == "necromantic_focus"                   ) e = ( heroic ? "OnSpellTickDamage_44Mastery_10Stack_10Dur" : "OnSpellTickDamage_39Mastery_10Stack_10Dur" );
  else if ( name == "needleencrusted_scorpion"            ) e = "OnAttackCrit_678crit_10%_10Dur_50Cd";
  else if ( name == "pandoras_plea"                       ) e = "OnSpellCast_751SP_10%_10Dur_45Cd";
  else if ( name == "petrified_pickled_egg"               ) e = "OnHeal_2040Haste_10%_10Dur_50Cd"; // FIXME: Confirm ICD
  else if ( name == "porcelain_crab"                      ) e = ( heroic ? "OnAttackHit_1710Mastery_10%_20Dur_95Cd" : "OnAttackHit_918Mastery_10%_20Dur_95Cd" ); // TO-DO: Confirm ICD.
  else if ( name == "prestors_talisman_of_machination"    ) e = ( heroic ? "OnAttackHit_2178Haste_10%_15Dur_75Cd" : "OnAttackHit_1926Haste_10%_15Dur_75Cd" ); // TO-DO: Confirm ICD.
  else if ( name == "purified_lunar_dust"                 ) e = "OnSpellCast_304MP5_10%_15Dur_45Cd";
  else if ( name == "pyrite_infuser"                      ) e = "OnAttackCrit_1234AP_10%_10Dur_50Cd";
  else if ( name == "quagmirrans_eye"                     ) e = "OnSpellCast_320Haste_10%_6Dur_45Cd";
  else if ( name == "right_eye_of_rajh"                   ) e = ( heroic ? "OnAttackCrit_1710Str_50%_10Dur_50Cd" : "OnAttackCrit_1512Str_50%_10Dur_50Cd" );
  else if ( name == "schnotzzs_medallion_of_command"      ) e = "OnAttackHit_918Mastery_10%_20Dur_95Cd"; // TO-DO: Confirm ICD.
  else if ( name == "sextant_of_unstable_currents"        ) e = "OnSpellCrit_190SP_20%_15Dur_45Cd";
  else if ( name == "shiffars_nexus_horn"                 ) e = "OnSpellCrit_225SP_20%_10Dur_45Cd";
  else if ( name == "stonemothers_kiss"                   ) e = "OnSpellCast_1164Crit_10%_20Dur_75Cd";
  else if ( name == "stump_of_time"                       ) e = "OnSpellCast_1926SP_10%_15Dur_75Cd";
  else if ( name == "sundial_of_the_exiled"               ) e = "OnSpellCast_590SP_10%_10Dur_45Cd";
  else if ( name == "talisman_of_sinister_order"          ) e = "OnSpellCast_918Mastery_10%_20Dur_95Cd"; // TO-DO: Confirm ICD.
  else if ( name == "tendrils_of_burrowing_dark"          ) e = ( heroic ? "OnSpellCast_1710SP_10%_15Dur_75Cd" : "OnSpellCast_1290SP_10%_15Dur_75Cd" ); // TO-DO: Confirm ICD
  else if ( name == "the_hungerer"                        ) e = ( heroic ? "OnAttackHit_1730Haste_100%_15Dur_60Cd" : "OnAttackHit_1532Haste_100%_15Dur_60Cd" );
  else if ( name == "theralions_mirror"                   ) e = ( heroic ? "OnHarmfulSpellCast_2178Mastery_10%_20Dur_100Cd" : "OnHarmfulSpellCast_1926Mastery_10%_20Dur_100Cd" );
  else if ( name == "tias_grace"                          ) e = ( heroic ? "OnAttackHit_34Agi_10Stack_15Dur" : "OnAttackHit_34Agi_10Stack_15Dur" );
  else if ( name == "unheeded_warning"                    ) e = "OnAttackHit_1926AP_10%_10Dur_50Cd";
  else if ( name == "vessel_of_acceleration"              ) e = ( heroic ? "OnAttackCrit_93Crit_5Stack_20Dur" : "OnAttackCrit_82Crit_5Stack_20Dur" );
  else if ( name == "witching_hourglass"                  ) e = ( heroic ? "OnSpellCast_1710Haste_10%_15Dur_75Cd" : "OnSpellCast_918Haste_10%_15Dur_75Cd" );
  else if ( name == "wrath_of_cenarius"                   ) e = "OnSpellHit_132SP_5%_10Dur";
  else if ( name == "fall_of_mortality"                   ) e = ( heroic ? "OnHealCast_2178Spi_15Dur_75Cd" : "OnHealCast_1926Spi_15Dur_75Cd" );
  else if ( name == "darkmoon_card_tsunami"               ) e = "OnHeal_80Spi_5Stack_20Dur";
  else if ( name == "phylactery_of_the_nameless_lich"     ) e = ( heroic ? "OnSpellTickDamage_1206SP_30%_20Dur_100Cd" : "OnSpellTickDamage_1073SP_30%_20Dur_100Cd" );
  else if ( name == "whispering_fanged_skull"             ) e = ( heroic ? "OnAttackHit_1250AP_35%_15Dur_45Cd" : "OnAttackHit_1110AP_35%_15Dur_45Cd" );
  else if ( name == "charred_twilight_scale"              ) e = ( heroic ? "OnSpellCast_861SP_10%_15Dur_45Cd" : "OnSpellCast_763SP_10%_15Dur_45Cd" );
  else if ( name == "sharpened_twilight_scale"            ) e = ( heroic ? "OnAttackHit_1472AP_35%_15Dur_45Cd" : "OnAttackHit_1304AP_35%_15Dur_45Cd" );

  // 4.3
  else if ( name == "will_of_unbinding"                   ) e = ( heroic ? "OnHarmfulSpellCast_99Int_100%_10Dur_10Stack" : lfr ? "OnHarmfulSpellCast_78Int_100%_10Dur_10Stack" : "OnHarmfulSpellCast_88Int_100%_10Dur_10Stack" );
  else if ( name == "eye_of_unmaking"                     ) e = ( heroic ? "OnAttackHit_99Str_100%_10Dur_10Stack" : lfr ? "OnAttackHit_78Str_100%_10Dur_10Stack" : "OnAttackHit_88Str_100%_10Dur_10Stack" );
  else if ( name == "wrath_of_unchaining"                 ) e = ( heroic ? "OnAttackHit_99Agi_100%_10Dur_10Stack" : lfr ? "OnAttackHit_78Agi_100%_10Dur_10Stack" : "OnAttackHit_88Agi_100%_10Dur_10Stack" );
  else if ( name == "heart_of_unliving"                   ) e = ( heroic ? "OnHealCast_99Spi_100%_10Dur_10Stack" : lfr ? "OnHealCast_78Spi_100%_10Dur_10Stack" : "OnHealCast_88Spi_100%_10Dur_10Stack" );
  else if ( name == "resolve_of_undying"                  ) e = ( heroic ? "OnAttackHit_99Dodge_100%_10Dur_10Stack" : lfr ? "OnAttackHit_99Dodge_100%_10Dur_10Stack" : "OnAttackHit_88Dodge_100%_10Dur_10Stack" );
  else if ( name == "creche_of_the_final_dragon"          ) e = ( heroic ? "OnDamage_3278Crit_15%_20Dur_115Cd" : lfr ? "OnDamage_2573Crit_15%_20Dur_115Cd" : "OnDamage_2904Crit_15%_20Dur_115Cd" ); // FIXME: ICD needs testing, assuming same as insignia of corrupted mind
  else if ( name == "starcatcher_compass"                 ) e = ( heroic ? "OnDamage_3278Haste_15%_20Dur_115Cd" : lfr ? "OnDamage_2573Haste_15%_20Dur_115Cd" : "OnDamage_2904Haste_15%_20Dur_115Cd" ); // FIXME: ICD needs testing, assuming same as insignia of corrupted mind
  else if ( name == "seal_of_the_seven_signs"             ) e = ( heroic ? "OnHeal_3278Haste_15%_20Dur_115Cd" : lfr ? "OnHeal_2573Haste_15%_20Dur_115Cd" : "OnHeal_2904Haste_15%_20Dur_115Cd" ); // FIXME: ICD needs testing, assuming same as insignia of corrupted mind
  else if ( name == "soulshifter_vortex"                  ) e = ( heroic ? "OnDamage_3278Mastery_15%_20Dur_115Cd" : lfr ? "OnDamage_2573Mastery_15%_20Dur_115Cd" : "OnDamage_2904Mastery_15%_20Dur_115Cd" ); // FIXME: ICD needs testing, assuming same as insignia of corrupted mind
  else if ( name == "insignia_of_the_corrupted_mind"      ) e = ( heroic ? "OnDamage_3278Haste_15%_20Dur_115Cd" : lfr ? "OnDamage_2573Haste_15%_20Dur_115Cd" : "OnDamage_2904Haste_15%_20Dur_115Cd" ); // ICD confirmed on PTR 2011/11/14
  else if ( name == "foul_gift_of_the_demon_lord"         ) e = "OnSpellCast_1149Mastery_15%_20Dur_45Cd"; // TO-DO: Confirm ICD. Min seen 50.112sec
  else if ( name == "arrow_of_time"                       ) e = "OnAttackHit_1149Haste_20%_20Dur_45Cd";   // TO-DO: Confirm ICD
  else if ( name == "rosary_of_light"                     ) e = "OnAttackHit_1149Crit_15%_20Dur_45Cd";   // TO-DO: Confirm ICD. Min seen 50.934sec.
  else if ( name == "varothens_brooch"                    ) e = "OnAttackHit_1149Mastery_20%_20Dur_45Cd";   // TO-DO: Confirm ICD. Min seen 50.192sec.

  // Stat Procs with Tick Increases
  else if ( name == "dislodged_foreign_object"            ) e = ( heroic ? "OnSpellCast_121SP_10Stack_10%_20Dur_45Cd_2Tick" : "OnSpellCast_105SP_10Stack_10%_20Dur_45Cd_2Tick" );

  // Discharge Procs
  else if ( name == "bandits_insignia"                    ) e = "OnAttackHit_1880Arcane_15%_45Cd";
  else if ( name == "darkmoon_card_hurricane"             ) e = "OnAttackHit_-7000Nature_1PPM_nocrit_nobuffs";
  else if ( name == "darkmoon_card_volcano"               ) e = "OnSpellDamage_1200+10Fire_1600Int_30%_12Dur_45Cd";
  else if ( name == "extract_of_necromantic_power"        ) e = "OnSpellTickDamage_1050Shadow_10%_15Cd";
  else if ( name == "lightning_capacitor"                 ) e = "OnSpellCrit_750Nature_3Stack_2.5Cd";
  else if ( name == "timbals_crystal"                     ) e = "OnSpellTickDamage_380Shadow_10%_15Cd";
  else if ( name == "thunder_capacitor"                   ) e = "OnSpellCrit_1276Nature_4Stack_2.5Cd";
  else if ( name == "bryntroll_the_bone_arbiter"          ) e = ( heroic ? "OnAttackHit_2538Drain_11%" : "OnAttackHit_2250Drain_11%" );
  else if ( name == "cunning_of_the_cruel"                ) e = ( heroic ? "OnSpellDamage_11937+105.8Shadow_15%_25Cd" : lfr ? "OnSpellDamage_9369+83.1Shadow_15%_25Cd" : "OnSpellDamage_10575+93.8Shadow_15%_25Cd" ); // Confirm ICD, AoE? Saw a 26.862sec ICD on PTR.
  else if ( name == "vial_of_shadows"                     ) e = ( heroic ? "OnAttackHit_-17051+101.6Physical_15%_25Cd" : lfr ? "OnAttackHit_-13383+79.7Physical_15%_25Cd" : "OnAttackHit_-15106+90Physical_15%_25Cd" ); // Confirm ICD. Saw a 27.271sec ICD on PTR.
  else if ( name == "reign_of_the_unliving"               ) e = ( heroic ? "OnSpellDirectCrit_2117Fire_3Stack_2.0Cd" : "OnSpellDirectCrit_1882Fire_3Stack_2.0Cd" );
  else if ( name == "reign_of_the_dead"                   ) e = ( heroic ? "OnSpellDirectCrit_2117Fire_3Stack_2.0Cd" : "OnSpellDirectCrit_1882Fire_3Stack_2.0Cd" );
  else if ( name == "solace_of_the_defeated"              ) e = ( heroic ? "OnSpellCast_18MP5_8Stack_10Dur" : "OnSpellCast_16MP5_8Stack_10Dur" );
  else if ( name == "solace_of_the_fallen"                ) e = ( heroic ? "OnSpellCast_18MP5_8Stack_10Dur" : "OnSpellCast_16MP5_8Stack_10Dur" );

  // Variable Stack Discharge Procs
  else if ( name == "variable_pulse_lightning_capacitor"  ) e = ( heroic ? "OnSpellCrit_3300.7Nature_15%_10Stack_2.5Cd_chance" : "OnSpellCrit_2926.3Nature_15%_10Stack_2.5Cd_chance" );

  // Enchants
  else if ( name == "lightweave_old"                      ) e = "OnSpellCast_295SP_35%_15Dur_60Cd";
  else if ( name == "lightweave_embroidery_old"           ) e = "OnSpellCast_295SP_35%_15Dur_60Cd";
  else if ( name == "lightweave" ||
            name == "lightweave_embroidery"               ) e = "OnSpellDamageHeal_580Int_25%_15Dur_64Cd";
  else if ( name == "darkglow_embroidery_old"             ) e = "OnSpellCast_400Mana_35%_15Dur_60Cd";
  else if ( name == "darkglow_embroidery"                 ) e = "OnSpellCast_800Mana_30%_15Dur_45Cd";       // TO-DO: Confirm ICD.
  else if ( name == "swordguard_embroidery_old"           ) e = "OnAttackHit_400AP_20%_15Dur_60Cd";
  else if ( name == "swordguard_embroidery"               ) e = "OnAttackHit_1000AP_15%_15Dur_55Cd";
  else if ( name == "flintlockes_woodchucker"             ) e = "OnAttackHit_1100Physical_300Agi_10%_10Dur_40Cd_nocrit"; // TO-DO: Confirm ICD.

  // DK Runeforges
  else if ( name == "rune_of_cinderglacier"               ) e = "custom";
  else if ( name == "rune_of_razorice"                    ) e = "custom";
  else if ( name == "rune_of_the_fallen_crusader"         ) e = "custom";

  if ( e.empty() ) return false;

  util_t::tolower( e );

  encoding = e;

  return true;
}

// ==========================================================================
// unique_gear_t::get_use_encoding
// ==========================================================================

bool unique_gear_t::get_use_encoding( std::string&       encoding,
                                      const std::string& name,
                                      const bool         heroic,
                                      const bool         lfr,
                                      const bool         /* ptr */,
                                      const std::string& /* id */ )
{
  std::string e;

  // Simple
  if      ( name == "ancient_petrified_seed"       ) e = ( heroic ? "1441Agi_15Dur_60Cd"  : "1277Agi_15Dur_60Cd" );
  else if ( name == "brawlers_trophy"              ) e = "1700Dodge_20Dur_120Cd";
  else if ( name == "core_of_ripeness"             ) e = "1926Spi_20Dur_120Cd";
  else if ( name == "electrospark_heartstarter"    ) e = "567Int_20Dur_120Cd";
  else if ( name == "energy_siphon"                ) e = "408SP_20Dur_120Cd";
  else if ( name == "ephemeral_snowflake"          ) e = "464Haste_20Dur_120Cd";
  else if ( name == "essence_of_the_eternal_flame" ) e = ( heroic ? "1441Str_15Dur_60Cd" : "1277Str_15Dur_60Cd" );
  else if ( name == "fiery_quintessence"           ) e = ( heroic ? "1297Int_25Dur_90Cd"  : "1149Int_25Dur_90Cd" );
  else if ( name == "figurine__demon_panther"      ) e = "1425Agi_20Dur_120Cd";
  else if ( name == "figurine__dream_owl"          ) e = "1425Spi_20Dur_120Cd";
  else if ( name == "figurine__jeweled_serpent"    ) e = "1425Sp_20Dur_120Cd";
  else if ( name == "figurine__king_of_boars"      ) e = "1425Str_20Dur_120Cd";
  else if ( name == "impatience_of_youth"          ) e = "1605Str_20Dur_120Cd";
  else if ( name == "jaws_of_defeat"               ) e = ( heroic ? "OnSpellCast_125HolyStorm_CostRd_10Stack_20Dur_120Cd" : "OnSpellCast_110HolyStorm_CostRd_10Stack_20Dur_120Cd" );
  else if ( name == "living_flame"                 ) e = "505SP_20Dur_120Cd";
  else if ( name == "maghias_misguided_quill"      ) e = "716SP_20Dur_120Cd";
  else if ( name == "magnetite_mirror"             ) e = ( heroic ? "1425Str_15Dur_90Cd" : "1075Str_15Dur_90Cd" );
  else if ( name == "mark_of_khardros"             ) e = ( heroic ? "1425Mastery_15Dur_90Cd" : "1260Mastery_15Dur_90Cd" );
  else if ( name == "mark_of_norgannon"            ) e = "491Haste_20Dur_120Cd";
  else if ( name == "mark_of_supremacy"            ) e = "1024AP_20Dur_120Cd";
  else if ( name == "mark_of_the_firelord"         ) e = ( heroic ? "1441Int_15Dur_60Cd"  : "1277Int_15Dur_60Cd" );
  else if ( name == "moonwell_chalice"             ) e = "1700Mastery_20Dur_120Cd";
  else if ( name == "moonwell_phial"               ) e = "1700Dodge_20Dur_120Cd";
  else if ( name == "might_of_the_ocean"           ) e = ( heroic ? "1425Str_15Dur_90Cd" : "765Str_15Dur_90Cd" );
  else if ( name == "platinum_disks_of_battle"     ) e = "752AP_20Dur_120Cd";
  else if ( name == "platinum_disks_of_sorcery"    ) e = "440SP_20Dur_120Cd";
  else if ( name == "platinum_disks_of_swiftness"  ) e = "375Haste_20Dur_120Cd";
  else if ( name == "rickets_magnetic_fireball"    ) e = "1700Crit_20Dur_120Cd"; // FIXME: "Your attacks may occasionally attract small celestial objects."
  else if ( name == "rune_of_zeth"                 ) e = ( heroic ? "1441Int_15Dur_60Cd" : "1277Int_15Dur_60Cd" );
  else if ( name == "scale_of_fates"               ) e = "432Haste_20Dur_120Cd";
  else if ( name == "sea_star"                     ) e = ( heroic ? "1425Sp_20Dur_120Cd" : "765Sp_20Dur_120Cd" );
  else if ( name == "shard_of_the_crystal_heart"   ) e = "512Haste_20Dur_120Cd";
  else if ( name == "shard_of_woe"                 ) e = "1935Haste_10Dur_60Cd";
  else if ( name == "skardyns_grace"               ) e = ( heroic ? "1425Mastery_20Dur_120Cd" : "1260Mastery_20Dur_120Cd" );
  else if ( name == "sliver_of_pure_ice"           ) e = "1625Mana_120Cd";
  else if ( name == "soul_casket"                  ) e = "1926Sp_20Dur_120Cd";
  else if ( name == "souls_anguish"                ) e = "765Str_15Dur_90Cd";
  else if ( name == "spirit_world_glass"           ) e = "336Spi_20Dur_120Cd";
  else if ( name == "talisman_of_resurgence"       ) e = "599SP_20Dur_120Cd";
  else if ( name == "unsolvable_riddle"            ) e = "1605Agi_20Dur_120Cd";
  else if ( name == "wrathstone"                   ) e = "856AP_20Dur_120Cd";
  // 4.3
  else if ( name == "bottled_wishes"               ) e = ( heroic ? "2585SP_15Dur_90Cd" : lfr ? "2029SP_15Dur_90Cd" : "2290SP_15Dur_90Cd" );
  else if ( name == "kiroptyric_sigil"             ) e = ( heroic ? "2585Agi_15Dur_90Cd" : lfr ? "2029Agi_15Dur_90Cd" : "2290Agi_15Dur_90Cd" );
  else if ( name == "rotting_skull"                ) e = ( heroic ? "2585Str_15Dur_90Cd" : lfr ? "2029Str_15Dur_90Cd" : "2290Str_15Dur_90Cd" );
  else if ( name == "fire_of_the_deep"             ) e = ( heroic ? "2585Dodge_15Dur_90Cd" : lfr ? "2029Dodge_15Dur_90Cd" : "2290Dodge_15Dur_90Cd" );
  else if ( name == "reflection_of_the_light"      ) e = ( heroic ? "2585SP_15Dur_90Cd" : lfr ? "2029SP_15Dur_90Cd" : "2290SP_15Dur_90Cd" );

  // Hybrid
  else if ( name == "fetish_of_volatile_power"   ) e = ( heroic ? "OnSpellCast_64Haste_8Stack_20Dur_120Cd" : "OnSpellCast_57Haste_8Stack_20Dur_120Cd" );
  else if ( name == "talisman_of_volatile_power" ) e = ( heroic ? "OnSpellCast_64Haste_8Stack_20Dur_120Cd" : "OnSpellCast_57Haste_8Stack_20Dur_120Cd" );
  else if ( name == "vengeance_of_the_forsaken"  ) e = ( heroic ? "OnAttackHit_250AP_5Stack_20Dur_120Cd" : "OnAttackHit_215AP_5Stack_20Dur_120Cd" );
  else if ( name == "victors_call"               ) e = ( heroic ? "OnAttackHit_250AP_5Stack_20Dur_120Cd" : "OnAttackHit_215AP_5Stack_20Dur_120Cd" );
  else if ( name == "nevermelting_ice_crystal"   ) e = "OnSpellCrit_184Crit_5Stack_20Dur_180Cd_reverse";

  // Engineering Tinkers
  else if ( name == "pyrorocket"                   ) e = "1165Fire_45Cd";  // temporary for backwards compatibility
  else if ( name == "hand_mounted_pyro_rocket"     ) e = "1165Fire_45Cd";
  else if ( name == "hyperspeed_accelerators"      ) e = "240Haste_12Dur_60Cd";
  else if ( name == "tazik_shocker"                ) e = "4800Nature_120Cd";
  else if ( name == "quickflip_deflection_plates"  ) e = "1500Armor_12Dur_60Cd";

  if ( e.empty() ) return false;

  util_t::tolower( e );

  encoding = e;

  return true;
}
