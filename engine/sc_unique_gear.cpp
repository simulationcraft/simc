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
                        double t=0, bool reverse=false, int rng_type=RNG_DEFAULT ) :
      action_callback_t( p -> sim, p ),
      name_str( n ), stat( s ), amount( a ), tick( t )
  {
    if ( max_stacks == 0 ) max_stacks = 1;
    if ( proc_chance == 0 ) proc_chance = 1;
    if ( rng_type == RNG_DEFAULT ) rng_type = RNG_DISTRIBUTED;

    buff = new stat_buff_t( p, n, stat, amount, max_stacks, duration, cooldown, proc_chance, false, reverse, rng_type );
  }

  virtual void activate()
  {
    action_callback_t::activate();
    if ( buff -> reverse ) buff -> start( buff -> max_stack );
  }

  virtual void deactivate()
  {
    action_callback_t::deactivate();
    buff -> expire();
  }

  virtual void trigger( action_t* a )
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

// discharge_proc_callback ==================================================

struct discharge_proc_callback_t : public action_callback_t
{
  std::string name_str;
  int stacks, max_stacks;
  double proc_chance;
  double cooldown, cooldown_ready;
  spell_t* spell;
  proc_t* proc;
  rng_t* rng;

  discharge_proc_callback_t( const std::string& n, player_t* p, int ms, int school, double min, double max, double pc, double cd, int rng_type=RNG_DEFAULT ) :
      action_callback_t( p -> sim, p ),
      name_str( n ), stacks( 0 ), max_stacks( ms ), proc_chance( pc ), cooldown( cd ), cooldown_ready( 0 )
  {
    if ( rng_type == RNG_DEFAULT ) rng_type = RNG_DISTRIBUTED;

    struct discharge_spell_t : public spell_t
    {
      discharge_spell_t( const char* n, player_t* p, double min, double max, int s ) :
          spell_t( n, p, RESOURCE_NONE, ( s == SCHOOL_DRAIN ) ? SCHOOL_SHADOW : s )
      {
        trigger_gcd = 0;
        base_dd_min = min;
        base_dd_max = max;
        may_crit = ( s != SCHOOL_DRAIN );
        background  = true;
        base_spell_power_multiplier = 0;
        reset();
      }
    };

    spell = new discharge_spell_t( name_str.c_str(), p, min, max, school );

    proc = p -> get_proc( name_str.c_str() );
    rng  = p -> get_rng ( name_str.c_str(), rng_type );  // default is CYCLIC since discharge should not have duration
  }

  virtual void reset() { stacks=0; cooldown_ready=0; }

  virtual void deactivate() { action_callback_t::deactivate(); stacks=0; }

  virtual void trigger( action_t* a )
  {
    if ( cooldown )
      if ( sim -> current_time < cooldown_ready )
        return;

    if ( proc_chance )
      if ( ! rng -> roll( proc_chance ) )
        return;

    if ( cooldown )
      cooldown_ready = sim -> current_time + cooldown;

    if ( ++stacks < max_stacks )
    {
      listener -> aura_gain( name_str.c_str() );
    }
    else
    {
      stacks = 0;
      spell -> execute();
      proc -> occur();
    }
  }
};

// register_black_bruise ====================================================

static void register_black_bruise( item_t* item )
{
  // http://ptr.wowhead.com/?item=50035

  player_t* p = item -> player;

  item -> unique = true;

  bool heroic = item -> heroic();

  buff_t* buff = new buff_t( p, "black_bruise", 1, 10, 0.0, 0.03 ); // FIXME!! Cooldown?

  struct black_bruise_spell_t : public spell_t
  {
    bool heroic;
    black_bruise_spell_t( player_t* player, bool h ) : spell_t( "black_bruise", player, RESOURCE_NONE, SCHOOL_SHADOW ), heroic( h )
    {
      may_miss    = false;
      may_crit    = false; // FIXME!!  Can the damage crit?
      background  = true;
      proc        = true;
      trigger_gcd = 0;
      base_dd_min = base_dd_max = 1;
      base_dd_multiplier *= ( heroic ? 0.10 : 0.09 );
      reset();
    }
    virtual void player_buff() { }
    virtual double total_dd_multiplier() SC_CONST { return base_dd_multiplier; }
  };

  struct black_bruise_trigger_t : public action_callback_t
  {
    buff_t* buff;
    black_bruise_trigger_t( player_t* p, buff_t* b ) : action_callback_t( p -> sim, p ), buff( b ) {}
    virtual void trigger( action_t* a )
    {
      // FIXME! Can specials trigger the proc?
      // FIXME! Apparently both hands proc the buff even though only equipped in main hand
      if ( ! a -> weapon ) return;
      if ( a -> proc ) return;

      buff -> trigger();
    }
  };

  struct black_bruise_damage_t : public action_callback_t
  {
    buff_t* buff;
    spell_t* spell;
    black_bruise_damage_t( player_t* p, buff_t* b, spell_t* s ) : action_callback_t( p -> sim, p ), buff( b ), spell( s ) {}
    virtual void trigger( action_t* a )
    {
      // FIXME! Can specials trigger the damage?
      // FIXME! What about melee attacks that do no weapon damage?
      if ( ! a -> weapon ) return;
      if ( a -> proc ) return;

      if ( buff -> up() )
      {
        spell -> base_dd_adder = a -> direct_dmg - 1;
        spell -> execute();
      }
    }
  };

  p -> register_attack_result_callback( RESULT_HIT_MASK, new black_bruise_trigger_t( p, buff ) );
  p -> register_direct_damage_callback( -1, new black_bruise_damage_t( p, buff, new black_bruise_spell_t( p, heroic ) ) );
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
  action_callback_t* cb = new stat_proc_callback_t( "darkmoon_card_greatness", p, max_stat, 1, 300, 0.35, 15.0, 45.0 );

  p -> register_tick_damage_callback( SCHOOL_ALL_MASK, cb );
  p -> register_direct_damage_callback( SCHOOL_ALL_MASK, cb );
}

// register_deaths_choice ===================================================

static void register_deaths_choice( item_t* item )
{
  player_t* p = item -> player;

  item -> unique = true;

  double value = ( item -> heroic() ) ? 510.0 : 450.0;

  int stat = ( p -> attribute[ ATTR_STRENGTH ] > p -> attribute[ ATTR_AGILITY ] ) ? STAT_STRENGTH : STAT_AGILITY;

  action_callback_t* cb = new stat_proc_callback_t( item -> name(), p, stat, 1, value, 0.35, 15.0, 45.0 );

  p -> register_tick_damage_callback( SCHOOL_ALL_MASK, cb );
  p -> register_direct_damage_callback( SCHOOL_ALL_MASK, cb );
}

// register_deathbringers_will ==============================================

static void register_deathbringers_will( item_t* item )
{
  player_t* p = item -> player;

  item -> unique = true;

  struct deathbringers_will_callback_t : public stat_proc_callback_t
  {
    bool heroic;
    rng_t* rng;

    deathbringers_will_callback_t( player_t* p, bool h ) :
        stat_proc_callback_t( "deathbringers_will", p, STAT_STRENGTH, 1, 600, 0.35, 30.0, 105.0 ), heroic( h )
    {
      rng = p -> get_rng( "deathbringers_will_stat" );
    }

    virtual void trigger( action_t* a )
    {
      if ( buff -> cooldown_ready > sim -> current_time ) return;

      // Unholy Death Knights are different than Frost and Blood, so they get a special proc.
      static int uh_death_knight_stats[] = { STAT_STRENGTH, STAT_HASTE_RATING, STAT_CRIT_RATING  };
      static int    death_knight_stats[] = { STAT_STRENGTH, STAT_HASTE_RATING, STAT_CRIT_RATING  };
      static int           druid_stats[] = { STAT_STRENGTH, STAT_AGILITY,      STAT_HASTE_RATING };
      static int          hunter_stats[] = { STAT_AGILITY,  STAT_CRIT_RATING,  STAT_ATTACK_POWER };
      static int         paladin_stats[] = { STAT_STRENGTH, STAT_HASTE_RATING, STAT_CRIT_RATING  };
      static int           rogue_stats[] = { STAT_AGILITY,  STAT_ATTACK_POWER, STAT_HASTE_RATING };
      static int          shaman_stats[] = { STAT_AGILITY,  STAT_ATTACK_POWER, STAT_HASTE_RATING };
      static int         warrior_stats[] = { STAT_STRENGTH, STAT_CRIT_RATING,  STAT_HASTE_RATING };

      int* stats=0;

      switch ( a -> player -> type )
      {
      case DEATH_KNIGHT: stats = ( a -> player -> primary_tree() == TREE_UNHOLY ) ? uh_death_knight_stats : death_knight_stats; break;
      case DRUID:        stats =   druid_stats; break;
      case HUNTER:       stats =  hunter_stats; break;
      case PALADIN:      stats = paladin_stats; break;
      case ROGUE:        stats =   rogue_stats; break;
      case SHAMAN:       stats =  shaman_stats; break;
      case WARRIOR:      stats = warrior_stats; break;
      }

      if ( ! stats ) return;

      buff -> stat = stats[ ( int ) ( rng -> range( 0.0, 2.999 ) ) ];

      if ( buff -> stat == STAT_ATTACK_POWER )
      {
        buff -> amount = heroic ? 1400 : 1200;
      }
      else
      {
        buff -> amount = heroic ? 700 : 600;
      }

      stat_proc_callback_t::trigger( a );
    }
  };

  p -> register_attack_result_callback( RESULT_HIT_MASK, new deathbringers_will_callback_t( p, item -> heroic() ) );
}

// register_empowered_deathbringer ==========================================

static void register_empowered_deathbringer( item_t* item )
{
  player_t* p = item -> player;

  item -> unique = true;

  struct deathbringer_spell_t : public spell_t
  {
    deathbringer_spell_t( player_t* p ) :
        spell_t( "empowered_deathbringer", p, RESOURCE_NONE, SCHOOL_SHADOW )
    {
      trigger_gcd = 0;
      background  = true;
      may_miss = false;
      may_crit = true;
      base_dd_min = 1313;
      base_dd_max = 1687;
      base_spell_power_multiplier = 0;
      reset();
    }
    virtual void player_buff()
    {
      spell_t::player_buff();
      player_crit = player -> composite_attack_crit(); // FIXME! Works like a spell in most ways accept source of crit.
    }
  };

  struct deathbringer_callback_t : public action_callback_t
  {
    spell_t* spell;
    rng_t* rng;

    deathbringer_callback_t( player_t* p, spell_t* s ) :
        action_callback_t( p -> sim, p ), spell( s )
    {
      rng  = p -> get_rng ( "empowered_deathbringer", RNG_DEFAULT );
    }

    virtual void trigger( action_t* a )
    {
      if ( rng -> roll( 0.08 ) ) // FIXME!! Using 8% of -hits-
      {
        spell -> execute();
      }
    }
  };

  p -> register_attack_result_callback( RESULT_HIT_MASK, new deathbringer_callback_t( p, new deathbringer_spell_t( p ) ) );
}

// register_raging_deathbringer ==========================================

static void register_raging_deathbringer( item_t* item )
{
  player_t* p = item -> player;

  item -> unique = true;

  struct deathbringer_spell_t : public spell_t
  {
    deathbringer_spell_t( player_t* p ) :
        spell_t( "raging_deathbringer", p, RESOURCE_NONE, SCHOOL_SHADOW )
    {
      trigger_gcd = 0;
      background  = true;
      may_miss = false;
      may_crit = true;
      base_dd_min = 1458;
      base_dd_max = 1874;
      base_spell_power_multiplier = 0;
      reset();
    }
    virtual void player_buff()
    {
      spell_t::player_buff();
      player_crit = player -> composite_attack_crit(); // FIXME! Works like a spell in most ways accept source of crit.
    }
  };

  struct deathbringer_callback_t : public action_callback_t
  {
    spell_t* spell;
    rng_t* rng;

    deathbringer_callback_t( player_t* p, spell_t* s ) :
        action_callback_t( p -> sim, p ), spell( s )
    {
      rng  = p -> get_rng ( "raging_deathbringer", RNG_DEFAULT );
    }

    virtual void trigger( action_t* a )
    {
      if ( rng -> roll( 0.08 ) ) // FIXME!! Using 8% of -hits-
      {
        spell -> execute();
      }
    }
  };

  p -> register_attack_result_callback( RESULT_HIT_MASK, new deathbringer_callback_t( p, new deathbringer_spell_t( p ) ) );
}

// register_nibelung ========================================================

static void register_nibelung( item_t* item )
{
  player_t* p = item -> player;

  item -> unique = true;

  struct nibelung_spell_t : public spell_t
  {
    bool heroic;

    nibelung_spell_t( player_t* p, bool h ) :
        spell_t( "valkyr_smite", p, RESOURCE_NONE, SCHOOL_HOLY ), heroic( h )
    {
      trigger_gcd = 0;
      background  = true;
      may_miss = false;
      may_crit = true;
      base_dd_min = heroic ? 1803 : 1591;
      base_dd_max = heroic ? 2022 : 1785;
      base_crit = 0.05;
      reset();
    }
    virtual void player_buff() {}
  };

  struct nibelung_event_t : public event_t
  {
    spell_t* spell;
    int remaining;

    nibelung_event_t( sim_t* sim, player_t* p, spell_t* s, int r=0 ) : event_t( sim, p ), spell( s ), remaining( r )
    {
      name = "nibelung";
      // Valkyr gets off about 16 casts in 30sec
      if ( remaining == 0 ) remaining = 16;
      sim -> add_event( this, 1.85 );
    }
    virtual void execute()
    {
      spell -> execute();
      if ( --remaining > 0 ) new ( sim ) nibelung_event_t( sim, player, spell, remaining );
    }
  };

  struct nibelung_callback_t : public action_callback_t
  {
    spell_t* spell;
    proc_t* proc;
    rng_t* rng;

    nibelung_callback_t( player_t* p, spell_t* s ) :
        action_callback_t( p -> sim, p ), spell( s )
    {
      proc = p -> get_proc( "nibelung" );
      rng  = p -> get_rng ( "nibelung" );
    }

    virtual void trigger( action_t* a )
    {
      if (   a -> aoe      ||
             a -> proc     ||
             a -> dual     ||
             ! a -> harmful  ||
             a -> pseudo_pet )
        return;

      if ( rng -> roll( 0.02 ) )
      {
        if ( sim -> log ) log_t::output( sim, "%s summons a Valkyr from the Halls of Valhalla", a -> player -> name() );
        new ( sim ) nibelung_event_t( sim, a -> player, spell );
        proc -> occur();
      }
    }
  };

  p -> register_spell_direct_result_callback( RESULT_HIT_MASK, new nibelung_callback_t( p, new nibelung_spell_t( p, item -> heroic() ) ) );
}

// register_shadowmourne ====================================================

static void register_shadowmourne( item_t* item )
{
  player_t* p = item -> player;

  item -> unique = true;

  // http://ptr.wowhead.com/?spell=71903
  // FIX ME! Duration? Colldown? Chance?
  buff_t* buff_stacks = new stat_buff_t( p, "shadowmourne_stacks", STAT_STRENGTH,  30, 10, 60.0,  0.0, 1 );
  buff_t* buff_final  = new stat_buff_t( p, "shadowmourne_final",  STAT_STRENGTH, 270, 10, 10.0, 10.0, 1 );

  struct shadowmourne_spell_t : public spell_t
  {
    shadowmourne_spell_t( player_t* player ) : spell_t( "shadowmourne", player, RESOURCE_NONE, SCHOOL_SHADOW )
    {
      may_miss    = false;
      may_crit    = false; // FIXME!!  Can the damage crit?
      background  = true;
      proc        = true;
      trigger_gcd = 0;
      base_dd_min = 1900;
      base_dd_max = 2100;
      reset();
    }
    virtual void player_buff() { }
  };

  struct shadowmourne_trigger_t : public action_callback_t
  {
    buff_t* buff_stacks;
    buff_t* buff_final;
    spell_t* spell;
    int slot;

    shadowmourne_trigger_t( player_t* p, buff_t* b1, buff_t* b2, spell_t* sp, int s ) :
      action_callback_t( p -> sim, p ), buff_stacks( b1 ), buff_final( b2 ), spell( sp ), slot( s ) {}
    virtual void trigger( action_t* a )
    {
      // FIXME! Can specials trigger the proc?
      if ( ! a -> weapon ) return;
      if ( a -> weapon -> slot != slot ) return;
      // http://elitistjerks.com/f15/t89289-shadowmourne/p6/#post1592143
      // Full Stack => Final buff + 10s cd on stack gain
      // Basically means no now stacks if the final buff is up
      if ( ! buff_final -> check() ) buff_stacks -> trigger();
      if ( buff_stacks -> stack() == buff_stacks -> max_stack )
      {
        buff_stacks -> expire();
        buff_final  -> trigger();
        spell -> execute();
      }
    }
  };

  p -> register_attack_result_callback( RESULT_HIT_MASK, new shadowmourne_trigger_t( p, buff_stacks, buff_final, new shadowmourne_spell_t( p ), item -> slot ) );
}

// register_tiny_abom ====================================================

static void register_tiny_abom( item_t* item )
{
  player_t* p = item -> player;

  item -> unique = true;

  bool heroic = item -> heroic();

  buff_t* buff = new buff_t( p, "mote_of_anger", 8 - heroic, 0, 0, 0.5 );

  struct tiny_abom_trigger_t : public action_callback_t
  {
    buff_t* buff;
    std::string proc_name;
    stats_t *old_stats;
    attack_t *first_stack_attack;
    int result;
    double resource_consumed, direct_dmg, resisted_dmg, blocked_dmg;
    double time_to_execute;
    bool manifesting_anger;

    tiny_abom_trigger_t( player_t* p, buff_t* b ) : action_callback_t( p -> sim, p ), buff( b ), proc_name( "manifest_anger" ),
        old_stats( p -> get_stats( "manifest_anger" ) ), first_stack_attack( NULL )
    {
      old_stats -> school = SCHOOL_PHYSICAL;
      manifesting_anger = false;
    }
    virtual void reset()
    {
      first_stack_attack = NULL;
    }
    virtual void trigger( action_t* a )
    {
      if ( manifesting_anger ) return;
      if ( ! a -> weapon ) return;
      if ( ! buff -> trigger() ) return;

      // If this is the first stack, save the weapon which made the attack for use as the proc later.
      if ( buff -> stack() == 1 )
      {
        assert( first_stack_attack == NULL );
        if ( a -> weapon -> slot == SLOT_OFF_HAND )
        {
          first_stack_attack = a -> player -> off_hand_attack;
        }
        else
        {
          first_stack_attack = a -> player -> main_hand_attack;
        }
      }
      if ( buff -> stack() == buff -> max_stack )
      {
        attack_t *attack = first_stack_attack;
        assert( attack != NULL );
        first_stack_attack = NULL;
        buff -> expire();

        // This is pretty much a hack to repeat a melee attack under a
        // different name with slightly different parameters.
        //
        // Additionally, we need to restore the result of the
        // attack that caused manifest_anger. Otherwise direct damage
        // callbacks that are triggered after manifest_anger has executed,
        // will use the results of the manifest_anger attack.

        std::swap( proc_name, attack -> name_str );
        std::swap( old_stats, attack -> stats );
        result            = attack -> result;
        resource_consumed = attack -> resource_consumed;
        direct_dmg        = attack -> direct_dmg;
        resisted_dmg      = attack -> resisted_dmg;
        blocked_dmg       = attack -> blocked_dmg;
	time_to_execute   = attack -> time_to_execute;

	attack -> time_to_execute = 0;
        attack -> base_hit += ( attack -> player -> dual_wield() ) ? 0.19 : 0;
        attack -> repeating = false;
        attack -> may_glance = false;
        attack -> special = true;
        attack -> base_multiplier *= 0.5;
	manifesting_anger = true;
        attack -> execute();
	manifesting_anger = false;
        attack -> base_multiplier /= 0.5;
        attack -> special = false;
        attack -> may_glance = true;
        attack -> repeating = true;
        attack -> base_hit -= ( attack -> player -> dual_wield() ) ? 0.19 : 0;

        std::swap( proc_name, attack -> name_str );
        std::swap( old_stats, attack -> stats );
        attack -> result            = result;
        attack -> resource_consumed = resource_consumed;
        attack -> direct_dmg        = direct_dmg;
        attack -> resisted_dmg      = resisted_dmg;
        attack -> blocked_dmg       = blocked_dmg;
	attack -> time_to_execute   = time_to_execute;
      }
    }
  };

  p -> register_direct_damage_callback( -1, new tiny_abom_trigger_t( p, buff ) );
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

    if ( item.equip.stat )
    {
      register_stat_proc( item, item.equip );
    }
    else if ( item.equip.school )
    {
      register_discharge_proc( item, item.equip );
    }

    if ( ! strcmp( item.name(), "black_bruise"              ) ) register_black_bruise           ( &item );
    if ( ! strcmp( item.name(), "darkmoon_card_greatness"   ) ) register_darkmoon_card_greatness( &item );
    if ( ! strcmp( item.name(), "deathbringers_will"        ) ) register_deathbringers_will     ( &item );
    if ( ! strcmp( item.name(), "deaths_choice"             ) ) register_deaths_choice          ( &item );
    if ( ! strcmp( item.name(), "deaths_verdict"            ) ) register_deaths_choice          ( &item );
    if ( ! strcmp( item.name(), "empowered_deathbringer"    ) ) register_empowered_deathbringer ( &item );
    if ( ! strcmp( item.name(), "raging_deathbringer"       ) ) register_raging_deathbringer    ( &item );
    if ( ! strcmp( item.name(), "nibelung"                  ) ) register_nibelung               ( &item );
    if ( ! strcmp( item.name(), "shadowmourne"              ) ) register_shadowmourne           ( &item );
    if ( ! strcmp( item.name(), "tiny_abomination_in_a_jar" ) ) register_tiny_abom              ( &item );
  }

  if ( p -> set_bonus.spellstrike() )
  {
    unique_gear_t::register_stat_proc( PROC_SPELL, RESULT_HIT_MASK, "spellstrike", p, STAT_SPELL_POWER, 1, 92, 0.05, 10.0, 0, 0 );
  }
}

// ==========================================================================
// unique_gear_t::register_stat_proc
// ==========================================================================

action_callback_t* unique_gear_t::register_stat_proc( int                type,
						      int                mask,
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
  action_callback_t* cb = new stat_proc_callback_t( name, player, stat, max_stacks, amount, proc_chance, duration, cooldown, tick, reverse, rng_type );

  if ( type == PROC_DAMAGE )
  {
    player -> register_tick_damage_callback( mask, cb );
    player -> register_direct_damage_callback( mask, cb );
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
    player -> register_tick_callback( cb );
  }
  else if ( type == PROC_ATTACK )
  {
    player -> register_attack_result_callback( mask, cb );
  }
  else if ( type == PROC_SPELL )
  {
    player -> register_spell_result_callback( mask, cb );
  }
  else if ( type == PROC_ATTACK_DIRECT )
  {
    player -> register_attack_direct_result_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_DIRECT )
  {
    player -> register_spell_direct_result_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_CAST )
  {
    player -> register_spell_cast_result_callback( mask, cb );
  }

  return cb;
}

// ==========================================================================
// unique_gear_t::register_discharge_proc
// ==========================================================================

action_callback_t* unique_gear_t::register_discharge_proc( int                type,
    int                mask,
    const std::string& name,
    player_t*          player,
    int                max_stacks,
    int                school,
    double             min_dmg,
    double             max_dmg,
    double             proc_chance,
    double             cooldown,
    int                rng_type )
{
  action_callback_t* cb = new discharge_proc_callback_t( name, player, max_stacks, school, min_dmg, max_dmg, proc_chance, cooldown, rng_type );

  if ( type == PROC_DAMAGE )
  {
    player -> register_tick_damage_callback( mask, cb );
    player -> register_direct_damage_callback( mask, cb );
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
    player -> register_tick_callback( cb );
  }
  else if ( type == PROC_ATTACK )
  {
    player -> register_attack_result_callback( mask, cb );
  }
  else if ( type == PROC_SPELL )
  {
    player -> register_spell_result_callback( mask, cb );
  }
  else if ( type == PROC_ATTACK_DIRECT )
  {
    player -> register_attack_direct_result_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_DIRECT )
  {
    player -> register_spell_direct_result_callback( mask, cb );
  }
  else if ( type == PROC_SPELL_CAST )
  {
    player -> register_spell_cast_result_callback( mask, cb );
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

  return register_stat_proc( e.trigger_type, e.trigger_mask, name, i.player, e.stat, e.max_stacks, e.amount, e.proc_chance, e.duration, e.cooldown, e.tick, e.reverse );
}

// ==========================================================================
// unique_gear_t::register_discharge_proc
// ==========================================================================

action_callback_t* unique_gear_t::register_discharge_proc( item_t& i,
    item_t::special_effect_t& e )
{
  const char* name = e.name_str.empty() ? i.name() : e.name_str.c_str();

  return register_discharge_proc( e.trigger_type, e.trigger_mask, name, i.player, e.max_stacks, e.school, e.amount, e.amount, e.proc_chance, e.cooldown );
}

// ==========================================================================
// unique_gear_t::get_equip_encoding
// ==========================================================================

bool unique_gear_t::get_equip_encoding( std::string&       encoding,
                                        const std::string& name,
                                        const bool         heroic,
                                        const std::string& id )
{
  std::string e;

  // Stat Procs
  if      ( name == "abyssal_rune"                        ) e = "OnSpellCast_590SP_25%_10Dur_45Cd";
  else if ( name == "ashen_band_of_endless_destruction"   ) e = "OnSpellCastHit_285SP_10%_10Dur_60Cd";
  else if ( name == "ashen_band_of_unmatched_destruction" ) e = "OnSpellCastHit_285SP_10%_10Dur_60Cd";
  else if ( name == "ashen_band_of_endless_vengeance"     ) e = "OnAttackHit_480AP_1PPM_10Dur_60Cd";
  else if ( name == "ashen_band_of_unmatched_vengeance"   ) e = "OnAttackHit_480AP_1PPM_10Dur_60Cd";
  else if ( name == "ashen_band_of_endless_might"         ) e = "OnAttackHit_480AP_1PPM_10Dur_60Cd";
  else if ( name == "ashen_band_of_unmatched_might"       ) e = "OnAttackHit_480AP_1PPM_10Dur_60Cd";
  else if ( name == "banner_of_victory"                   ) e = "OnAttackHit_1008AP_20%_10Dur_50Cd";
  else if ( name == "black_magic"                         ) e = "OnSpellDirectHit_250Haste_35%_10Dur_35Cd";
  else if ( name == "blood_of_the_old_god"                ) e = "OnAttackCrit_1284AP_10%_10Dur_50Cd";
  else if ( name == "chuchus_tiny_box_of_horrors"         ) e = "OnAttackHit_258Crit_15%_10Dur_45Cd";
  else if ( name == "comets_trail"                        ) e = "OnAttackHit_726Haste_10%_10Dur_45Cd";
  else if ( name == "corens_chromium_coaster"             ) e = "OnAttackCrit_1000AP_10%_10Dur_50Cd";
  else if ( name == "dark_matter"                         ) e = "OnAttackHit_612Crit_15%_10Dur_45Cd";
  else if ( name == "darkmoon_card_crusade"               ) e = "OnDamage_8SP_10Stack_10Dur";
  else if ( name == "dying_curse"                         ) e = "OnSpellCast_765SP_15%_10Dur_45Cd";
  else if ( name == "elemental_focus_stone"               ) e = "OnSpellCast_522Haste_10%_10Dur_45Cd";
  else if ( name == "embrace_of_the_spider"               ) e = "OnSpellCast_505Haste_10%_10Dur_45Cd";
  else if ( name == "eye_of_magtheridon"                  ) e = "OnSpellCastMiss_170SP_10Dur";
  else if ( name == "eye_of_the_broodmother"              ) e = "OnSpellDamage_25SP_5Stack_10Dur";
  else if ( name == "flare_of_the_heavens"                ) e = "OnSpellCast_850SP_10%_10Dur_45Cd";
  else if ( name == "forge_ember"                         ) e = "OnSpellCastHit_512SP_10%_10Dur_45Cd";
  else if ( name == "fury_of_the_five_flights"            ) e = "OnAttackHit_16AP_20Stack_10Dur";
  else if ( name == "grim_toll"                           ) e = "OnAttackHit_612ArPen_15%_10Dur_45Cd";
  else if ( name == "herkuml_war_token"                   ) e = "OnAttackHit_17AP_20Stack_10Dur";
  else if ( name == "illustration_of_the_dragon_soul"     ) e = "OnSpellCast_20SP_10Stack_10Dur";
  else if ( name == "mark_of_defiance"                    ) e = "OnSpellCastHit_150Mana_15%_15Cd";
  else if ( name == "mirror_of_truth"                     ) e = "OnAttackCrit_1000AP_10%_10Dur_50Cd";
  else if ( name == "mithril_pocketwatch"                 ) e = "OnSpellCast_590SP_10%_10Dur_45Cd";
  else if ( name == "mjolnir_runestone"                   ) e = "OnAttackHit_665ArPen_15%_10Dur_45Cd";
  else if ( name == "muradins_spyglass"                   ) e = ( heroic ? "OnSpellDamage_20SP_10Stack_10Dur" : "OnSpellDamage_18SP_10Stack_10Dur" );
  else if ( name == "needleencrusted_scorpion"            ) e = "OnAttackCrit_678ArPen_10%_10Dur_50Cd";
  else if ( name == "pandoras_plea"                       ) e = "OnSpellCast_751SP_10%_10Dur_45Cd";
  else if ( name == "purified_lunar_dust"                 ) e = "OnSpellCast_304MP5_10%_15Dur_45Cd";
  else if ( name == "pyrite_infuser"                      ) e = "OnAttackCrit_1234AP_10%_10Dur_50Cd";
  else if ( name == "quagmirrans_eye"                     ) e = "OnSpellCast_320Haste_10%_6Dur_45Cd";
  else if ( name == "sextant_of_unstable_currents"        ) e = "OnSpellCrit_190SP_20%_15Dur_45Cd";
  else if ( name == "shiffars_nexus_horn"                 ) e = "OnSpellCrit_225SP_20%_10Dur_45Cd";
  else if ( name == "sundial_of_the_exiled"               ) e = "OnSpellCast_590SP_10%_10Dur_45Cd";
  else if ( name == "wrath_of_cenarius"                   ) e = "OnSpellCastHit_132SP_5%_10Dur";

  // Some Normal/Heroic items have same name
  else if ( name == "phylactery_of_the_nameless_lich"     ) e = ( heroic ? "OnSpellTickDamage_1206SP_30%_20Dur_100Cd" : "OnSpellTickDamage_1073SP_30%_20Dur_100Cd" );
  else if ( name == "whispering_fanged_skull"             ) e = ( heroic ? "OnAttackHit_1250AP_35%_15Dur_45Cd" : "OnAttackHit_1110AP_35%_15Dur_45Cd" );
  else if ( name == "charred_twilight_scale"              ) e = ( heroic ? "OnSpellCast_861SP_10%_15Dur_45Cd" : "OnSpellCast_763SP_10%_15Dur_45Cd" );
  else if ( name == "sharpened_twilight_scale"            ) e = ( heroic ? "OnAttackHit_1472AP_35%_15Dur_45Cd" : "OnAttackHit_1304AP_35%_15Dur_45Cd" );

  // Stat Procs with Tick Increases
  else if ( name == "dislodged_foreign_object"            ) e = ( heroic ? "OnSpellCast_121SP_10Stack_10%_20Dur_45Cd_2Tick" : "OnSpellCast_105SP_10Stack_10%_20Dur_45Cd_2Tick" );

  // Discharge Procs
  else if ( name == "bandits_insignia"                    ) e = "OnAttackHit_1880Arcane_15%_45Cd";
  else if ( name == "extract_of_necromantic_power"        ) e = "OnSpellTickDamage_1050Shadow_10%_15Cd";
  else if ( name == "lightning_capacitor"                 ) e = "OnSpellDirectCrit_750Nature_3Stack_2.5Cd";
  else if ( name == "timbals_crystal"                     ) e = "OnSpellTickDamage_380Shadow_10%_15Cd";
  else if ( name == "thunder_capacitor"                   ) e = "OnSpellDirectCrit_1276Nature_4Stack_2.5Cd";
  else if ( name == "bryntroll_the_bone_arbiter"          ) e = ( heroic ? "OnAttackHit_2538Drain_11%" : "OnAttackHit_2250Drain_11%" );

  // Some Normal/Heroic items have same name
  else if ( name == "reign_of_the_unliving"               ) e = ( heroic ? "OnSpellDirectCrit_2117Fire_3Stack_2.0Cd" : "OnSpellDirectCrit_1882Fire_3Stack_2.0Cd" );
  else if ( name == "reign_of_the_dead"                   ) e = ( heroic ? "OnSpellDirectCrit_2117Fire_3Stack_2.0Cd" : "OnSpellDirectCrit_1882Fire_3Stack_2.0Cd" );
  else if ( name == "solace_of_the_defeated"              ) e = ( heroic ? "OnSpellCast_18MP5_8Stack_10Dur" : "OnSpellCast_16MP5_8Stack_10Dur" );
  else if ( name == "solace_of_the_fallen"                ) e = ( heroic ? "OnSpellCast_18MP5_8Stack_10Dur" : "OnSpellCast_16MP5_8Stack_10Dur" );

  // Enchants
  else if ( name == "lightweave"                          ) e = "OnSpellCast_295SP_35%_15Dur_60Cd";  // temporary for backwards compatibility
  else if ( name == "lightweave_embroidery"               ) e = "OnSpellCast_295SP_35%_15Dur_60Cd";
  else if ( name == "darkglow_embroidery"                 ) e = "OnSpellCast_400Mana_35%_60Cd";
  else if ( name == "swordguard_embroidery"               ) e = "OnAttackHit_400AP_25%_60Cd";

  // DK Runeforges
  else if ( name == "rune_of_the_fallen_crusader"         ) e = "custom";
  else if ( name == "rune_of_razorice"                    ) e = "custom";

  if ( e.empty() ) return false;

  armory_t::format( e );
  encoding = e;

  return true;
}

// ==========================================================================
// unique_gear_t::get_hidden_encoding
// ==========================================================================

bool unique_gear_t::get_hidden_encoding( std::string&       encoding,
    const std::string& name,
    const bool         heroic,
    const std::string& id )
{
  std::string e;
  if      ( name == "lightweave"                 ) e = "1spi";
  else if ( name == "lightweave_embroidery"      ) e = "1spi";
  else if ( name == "darkglow_embroidery"        ) e = "1spi";

  if ( e.empty() ) return false;

  armory_t::format( e );
  encoding = e;

  return true;
}

// ==========================================================================
// unique_gear_t::get_use_encoding
// ==========================================================================

bool unique_gear_t::get_use_encoding( std::string&       encoding,
                                      const std::string& name,
                                      const bool         heroic,
                                      const std::string& id )
{
  std::string e;

  // Simple
  if      ( name == "energy_siphon"               ) e = "408SP_20Dur_120Cd";
  else if ( name == "ephemeral_snowflake"         ) e = "464Haste_20Dur_120Cd";
  else if ( name == "living_flame"                ) e = "505SP_20Dur_120Cd";
  else if ( name == "maghias_misguided_quill"     ) e = "716SP_20Dur_120Cd";
  else if ( name == "mark_of_norgannon"           ) e = "491Haste_20Dur_120Cd";
  else if ( name == "mark_of_supremacy"           ) e = "1024AP_20Dur_120Cd";
  else if ( name == "platinum_disks_of_battle"    ) e = "752AP_20Dur_120Cd";
  else if ( name == "platinum_disks_of_sorcery"   ) e = "440SP_20Dur_120Cd";
  else if ( name == "platinum_disks_of_swiftness" ) e = "375Haste_20Dur_120Cd";
  else if ( name == "scale_of_fates"              ) e = "432Haste_20Dur_120Cd";
  else if ( name == "shard_of_the_crystal_heart"  ) e = "512Haste_20Dur_120Cd";
  else if ( name == "sliver_of_pure_ice"          ) e = "1625Mana_120Cd";
  else if ( name == "spirit_world_glass"          ) e = "336Spi_20Dur_120Cd";
  else if ( name == "talisman_of_resurgence"      ) e = "599SP_20Dur_120Cd";
  else if ( name == "wrathstone"                  ) e = "856AP_20Dur_120Cd";

  // Hybrid
  else if ( name == "fetish_of_volatile_power"   ) e = ( heroic ? "OnSpellCast_64Haste_8Stack_20Dur_120Cd" : "OnSpellCast_57Haste_8Stack_20Dur_120Cd" );
  else if ( name == "talisman_of_volatile_power" ) e = ( heroic ? "OnSpellCast_64Haste_8Stack_20Dur_120Cd" : "OnSpellCast_57Haste_8Stack_20Dur_120Cd" );
  else if ( name == "vengeance_of_the_forsaken"  ) e = ( heroic ? "OnAttackHit_250AP_5Stack_20Dur_120Cd" : "OnAttackHit_215AP_5Stack_20Dur_120Cd" );
  else if ( name == "victors_call"               ) e = ( heroic ? "OnAttackHit_250AP_5Stack_20Dur_120Cd" : "OnAttackHit_215AP_5Stack_20Dur_120Cd" );
  else if ( name == "nevermelting_ice_crystal"   ) e = "OnSpellDirectCrit_184Crit_5Stack_20Dur_180Cd_reverse";

  // Enchants
  else if ( name == "pyrorocket"                   ) e = "1837Fire_45Cd";  // temporary for backwards compatibility
  else if ( name == "hand_mounted_pyro_rocket"     ) e = "1837Fire_45Cd";
  else if ( name == "hyperspeed_accelerators"      ) e = "340Haste_12Dur_60Cd";

  if ( e.empty() ) return false;

  armory_t::format( e );
  encoding = e;

  return true;
}

