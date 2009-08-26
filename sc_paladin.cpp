// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Paladin
// ==========================================================================

enum { SEAL_NONE=0, SEAL_OF_COMMAND, SEAL_OF_VENGEANCE, SEAL_MAX };

struct paladin_t : public player_t
{
  // Active
  int           active_seal;
  attack_t*     active_seal_of_command;
  attack_t*     active_seal_of_vengeance;
  attack_t*     active_seal_of_vengeance_dot;
  attack_t*     active_righteous_vengeance;

  // Buffs
  struct _buffs_t
  {
    int avenging_wrath;
    int seal_of_vengeance_stacks;       // really a debuff
    int the_art_of_war;
    int vengeance;
    void reset() { memset( ( void* ) this, 0x00, sizeof( _buffs_t ) ); }
    _buffs_t() { reset(); }
  };
  _buffs_t _buffs;

  // Expirations
  struct _expirations_t
  {
    event_t*    the_art_of_war;
    event_t*    vengeance;
    void reset() { memset( ( void* ) this, 0x00, sizeof( _expirations_t ) ); }
    _expirations_t() { reset(); }
  };
  _expirations_t _expirations;

  // Gains
  gain_t* gains_jotw;

  // Procs

  // Uptimes

  // Random Number Generation
  rng_t* rng_jotw;

  // Auto-Attack
  attack_t* auto_attack;

  struct talents_t
  {
    int benediction;            // -2-10% mana on instants
    int conviction;             // +1-5% crit
    int crusade;                // +1-3% dmg, +1-3% on humanoids, demons, undead, elemental
    int crusader_strike;
    int divine_storm;
    int divine_strength;
    int fanaticism;             // +6/12/18% crit on judgement
    int improved_judgements;    // -1/2 cd on judgements
    int judgements_of_the_wise; // 25% basemana
    int righteous_vengeance;    // judgement/DS crit -> 30% damage over 8s
    int sanctified_wrath;       // +25/50% crit on HoW, -30/60s AW cd
    int sanctity_of_battle;     // +1-3% crit, +5-15% dmg on exorcism/CS
    int seals_of_the_pure;      // +3-15% dmg on SoR/SoV
    int sheath_of_light;        // 10/20/30% AP as SP
    int the_art_of_war;         // +5-10% dmg on judgement, CS, DS
    int two_handed_weapon_spec; // +2-6% dmg with 2h weapons
    int vengeance;              // +1-3% dmg for 30s after crit, stacks 3 times

    // NYI
    int heart_of_the_crusader;  // 1-3%crit debuff on judgement
    int improved_bom;           // +12/25% on BoM
    int sanctified_retribution; // aura gives +3% dmg
    int seal_of_command;
    int swift_retribution;      // auras give 1-3% haste
    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int consecration;
    int exorcism;
    int judgement;
    int seal_of_vengeance;
    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  paladin_t( sim_t* sim, const std::string& name, int race_type = RACE_NONE ) : player_t( sim, PALADIN, name, race_type )
  {
    active_seal                  = SEAL_NONE;
    active_seal_of_vengeance     = 0;
    active_seal_of_vengeance_dot = 0;
    active_righteous_vengeance   = 0;

    auto_attack = 0;
  }

  virtual void      init_race();
  virtual void      init_base();
  virtual void      init_gains();
  virtual void      init_glyphs();
//virtual void      init_procs();
//virtual void      init_uptimes();
  virtual void      init_rng();
  virtual void      init_scaling();
  virtual void      init_actions();
  virtual void      reset();
  virtual void      interrupt();
  virtual double    composite_spell_power( int school ) SC_CONST;
  virtual bool      get_talent_trees( std::vector<int*>& holy, std::vector<int*>& protection, std::vector<int*>& retribution );
  virtual std::vector<option_t>& get_options();
  virtual action_t* create_action( const std::string& name, const std::string& options_str );
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST { return RESOURCE_MANA; }
  virtual int       primary_role() SC_CONST     { return ROLE_HYBRID; }
  virtual int       primary_tree() SC_CONST     { return TREE_RETRIBUTION; }
};

struct paladin_attack_t : public attack_t
{
  bool trigger_seal;

  paladin_attack_t( const char* n, paladin_t* p, int s=SCHOOL_PHYSICAL, int t=TREE_NONE, bool special=true ) :
      attack_t( n, p, RESOURCE_MANA, s, t, special ),
      trigger_seal( true )
  {
    may_crit = true;
    base_dd_min = base_dd_max = 1;
    base_crit       += 0.01 * ( p -> talents.conviction + p -> talents.sanctity_of_battle );
    base_multiplier *= ( 1.0 + 0.01 * p -> talents.crusade * 2 ); // FIXME: target type
    base_multiplier *= ( 1.0 + 0.02 * p -> talents.two_handed_weapon_spec ); // FIXME: anything it doesn't affect?
  }

  virtual void execute();

  virtual double cost() SC_CONST
  {
    paladin_t* p = player -> cast_paladin();
    double c = attack_t::cost();
    return c * ( 1 - 0.02 * p -> talents.benediction );
  }

  virtual void player_buff()
  {
    paladin_t* p = player -> cast_paladin();

    attack_t::player_buff();

    if ( p -> glyphs.seal_of_vengeance && p -> active_seal == SEAL_OF_VENGEANCE )
      player_expertise += 0.0025 * 10;

    player_multiplier *= 1.0 + 0.01 * p -> talents.vengeance * p -> _buffs.vengeance;
    if ( p -> _buffs.avenging_wrath )
      player_multiplier *= 1.20;
  }
};

struct paladin_spell_t : public spell_t
{
  paladin_spell_t( const char* n, paladin_t* p, int s=SCHOOL_HOLY, int t=TREE_NONE ) :
      spell_t( n, p, RESOURCE_MANA, s, t )
  {
    base_crit       += 0.01 * ( p -> talents.conviction + p -> talents.sanctity_of_battle );
    base_multiplier *= ( 1.0 + 0.01 * p -> talents.crusade * 2 ); // FIXME: target type
  }

  virtual void player_buff()
  {
    paladin_t* p = player -> cast_paladin();

    spell_t::player_buff();

    player_multiplier *= 1.0 + 0.01 * p -> talents.vengeance * p -> _buffs.vengeance;
    if ( p -> _buffs.avenging_wrath )
      player_multiplier *= 1.20;
  }
};

static void trigger_judgements_of_the_wise( attack_t* a )
{
  paladin_t* p = a -> player -> cast_paladin();

  if ( ! p -> talents.judgements_of_the_wise ) return;
  if ( ! p -> rng_jotw -> roll( p -> talents.judgements_of_the_wise / 3.0 ) ) return;

  p -> resource_gain( RESOURCE_MANA, 0.25 * p -> resource_base[ RESOURCE_MANA ], p -> gains_jotw );
  p -> trigger_replenishment();
}

static void trigger_righteous_vengeance( attack_t* a )
{
  paladin_t* p = a -> player -> cast_paladin();

  if ( ! p -> talents.righteous_vengeance ) return;

  struct righteous_vengeance_t : public attack_t
  {
    righteous_vengeance_t( paladin_t* p ) : attack_t( "righteous_vengeance", p, RESOURCE_NONE, SCHOOL_HOLY )
    {
      may_miss    = false;
      background  = true;
      proc        = true;
      trigger_gcd = 0;
      base_cost   = 0;

      base_tick_time = 1;
      num_ticks      = 8;
      tick_power_mod = 0;

      reset();
    }
    void player_buff()   {}
    void target_debuff( int dmg_type ) {}
  };

  double dmg = p -> talents.righteous_vengeance * 0.1 * a -> direct_dmg;

  if ( ! p -> active_righteous_vengeance ) p -> active_righteous_vengeance = new righteous_vengeance_t( p );

  if ( p -> active_righteous_vengeance -> ticking )
  {
    int num_ticks = p -> active_righteous_vengeance -> num_ticks;
    int remaining_ticks = num_ticks - p -> active_righteous_vengeance -> current_tick;

    dmg += p -> active_righteous_vengeance -> base_td * remaining_ticks;

    p -> active_righteous_vengeance -> cancel();
  }
  p -> active_righteous_vengeance -> base_td = dmg / 8;
  p -> active_righteous_vengeance -> schedule_tick();
}

static void trigger_seal_of_command( attack_t* a )
{
  if ( a -> proc ) return;

  struct seal_of_command_attack_t : public paladin_attack_t
  {
    seal_of_command_attack_t( paladin_t* p ) :
        paladin_attack_t( "seal_of_command_atk", p, SCHOOL_HOLY )
    {
      background  = true;
      proc        = true;
      trigger_gcd = 0;
      base_cost   = 0;

      // TODO
      weapon                 = &( p -> main_hand_weapon );
      normalize_weapon_speed = true;
      weapon_multiplier      = 0.33;

      reset();
    }
  };

  paladin_t* p = a -> player -> cast_paladin();

  if ( p -> active_seal != SEAL_OF_COMMAND ) return;

  if ( ! p -> active_seal_of_command )   p -> active_seal_of_command = new seal_of_command_attack_t( p );

  p -> active_seal_of_command -> schedule_execute();
}

// Clunky mechanics:
// * The dot application/stacking can miss and is only triggered by autoattack and HotR
// * The proc at 5 stacks cannot miss

static void trigger_seal_of_vengeance_dot( attack_t* a )
{
  if ( a -> proc ) return;

  struct seal_of_vengeance_dot_t : public paladin_attack_t
  {
    seal_of_vengeance_dot_t( paladin_t* p ) :
        paladin_attack_t( "seal_of_vengeance_dot", p, SCHOOL_HOLY )
    {
      background  = true;
      proc        = true;
      trigger_gcd = 0;
      base_cost   = 0;
      may_crit    = false;

      base_dd_min = base_dd_max = 0;
      base_td = 0;
      tick_power_mod = 0.025;
      num_ticks = 6;
      tick_zero = true;
      base_tick_time = 3;
      base_spell_power_multiplier = 1;

      base_multiplier *= ( 1 + p -> talents.seals_of_the_pure * 0.03 );

      reset();
    }

    virtual void execute()
    {
      paladin_t* p = player -> cast_paladin();

      if ( sim -> log && ! dual ) log_t::output( sim, "%s performs %s", player -> name(), name() );

      player_buff();

      target_debuff( DMG_OVER_TIME );

      calculate_result();

      if ( result_is_hit() )
      {
        if ( p -> _buffs.seal_of_vengeance_stacks < 5 )
        {
          p -> _buffs.seal_of_vengeance_stacks++;
        }

        if ( ticking )
        {
          refresh_duration();
        }
        else
        {
          schedule_tick();
        }
      }
    }

    virtual void player_buff()
    {
      paladin_t* p = player -> cast_paladin();

      paladin_attack_t::player_buff();

      player_multiplier *= p -> _buffs.seal_of_vengeance_stacks;
    }

    virtual double calculate_tick_damage()
    {
      base_td = 0.013 * total_spell_power();

      return paladin_attack_t::calculate_tick_damage();
    }

    virtual void last_tick()
    {
      paladin_t* p = player -> cast_paladin();

      paladin_attack_t::last_tick();

      p -> _buffs.seal_of_vengeance_stacks = 0;
    }
  };

  paladin_t* p = a -> player -> cast_paladin();

  if ( p -> active_seal != SEAL_OF_VENGEANCE ) return;

  if ( ! p -> active_seal_of_vengeance_dot )
    p -> active_seal_of_vengeance_dot = new seal_of_vengeance_dot_t( p );
  p -> active_seal_of_vengeance_dot -> schedule_execute();
}

static void trigger_seal_of_vengeance_attack( attack_t* a )
{
  if ( a -> proc ) return;

  // doesn't scale with seals of the pure
  struct seal_of_vengeance_attack_t : public paladin_attack_t
  {
    seal_of_vengeance_attack_t( paladin_t* p ) :
        paladin_attack_t( "seal_of_vengeance_atk", p, SCHOOL_HOLY )
    {
      background  = true;
      proc        = true;
      trigger_gcd = 0;
      base_cost   = 0;

      may_miss = may_dodge = may_parry = false;

      weapon                 = &( p -> main_hand_weapon );
      normalize_weapon_speed = true;
      weapon_multiplier      = 0.33;

      reset();
    }
  };

  paladin_t* p = a -> player -> cast_paladin();

  if ( p -> active_seal != SEAL_OF_VENGEANCE ) return;

  if ( p -> _buffs.seal_of_vengeance_stacks == 5 )
  {
    if ( ! p -> active_seal_of_vengeance )
      p -> active_seal_of_vengeance = new seal_of_vengeance_attack_t( p );
    p -> active_seal_of_vengeance -> schedule_execute();
  }
}

static void trigger_the_art_of_war( attack_t* a )
{
  paladin_t* p = a -> player -> cast_paladin();

  if ( ! p -> talents.the_art_of_war ) return;

  struct the_art_of_war_expiration_t : public event_t
  {
    the_art_of_war_expiration_t( sim_t* sim, paladin_t* p ) : event_t( sim, p )
    {
      name = "The Art of War Expiration";
      p -> aura_gain( "The Art of War" );
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      paladin_t* p = player -> cast_paladin();
      p -> aura_loss( "The Art of War" );
      p -> _buffs.the_art_of_war = 0;
      p -> _expirations.the_art_of_war = 0;
    }
  };

  p -> _buffs.the_art_of_war = 1;

  event_t*& e = p -> _expirations.the_art_of_war;

  if ( e )
  {
    e -> reschedule( 15.0 );
  }
  else
  {
    e = new ( a -> sim ) the_art_of_war_expiration_t( a -> sim, p );
  }
}

static void trigger_vengeance( attack_t* a )
{
  paladin_t* p = a -> player -> cast_paladin();

  if ( ! p -> talents.vengeance ) return;

  struct vengeance_expiration_t : public event_t
  {
    vengeance_expiration_t( sim_t* sim, paladin_t* p ) : event_t( sim, p )
    {
      name = "Vengeance Expiration";
      p -> aura_gain( "Vengeance" );
      sim -> add_event( this, 30.0 );
    }
    virtual void execute()
    {
      paladin_t* p = player -> cast_paladin();
      p -> aura_loss( "Vengeance" );
      p -> _buffs.vengeance = 0;
      p -> _expirations.vengeance = 0;
    }
  };

  if ( p -> _buffs.vengeance < 3 ) p -> _buffs.vengeance++;

  event_t*& e = p -> _expirations.vengeance;

  if ( e )
  {
    e -> reschedule( 30.0 );
  }
  else
  {
    e = new ( a -> sim ) vengeance_expiration_t( a -> sim, p );
  }
}

void paladin_attack_t::execute()
{
  attack_t::execute();

  if ( result_is_hit() )
  {
    if ( result == RESULT_CRIT )
    {
      trigger_vengeance( this );
    }

    if ( trigger_seal )
    {
      paladin_t* p = player -> cast_paladin();

      switch ( p -> active_seal )
      {
      case SEAL_OF_COMMAND:
        trigger_seal_of_command( this );
        break;
      case SEAL_OF_VENGEANCE:
        trigger_seal_of_vengeance_attack( this );
        break;
      default:
        assert( 0 );
        break;
      }
    }
  }
}

// Melee Attack ============================================================

struct melee_t : public paladin_attack_t
{
  melee_t( paladin_t* p ) :
      paladin_attack_t( "melee", p, SCHOOL_PHYSICAL, TREE_NONE, false )
  {
    background        = true;
    repeating         = true;
    trigger_gcd       = 0;
    base_cost         = 0;
    weapon            = &( p -> main_hand_weapon );
    base_execute_time = p -> main_hand_weapon.swing_time;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();

    paladin_attack_t::execute();

    if ( result_is_hit() )
    {
      if ( result == RESULT_CRIT )
      {
        trigger_the_art_of_war( this );
      }

      if ( p -> active_seal == SEAL_OF_VENGEANCE )
        trigger_seal_of_vengeance_dot( this );
    }
  }
};

// Auto Attack =============================================================

struct auto_attack_t : public paladin_attack_t
{
  auto_attack_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "auto_attack", p )
  {
    assert( p -> main_hand_weapon.type != WEAPON_NONE );
    p -> auto_attack = new melee_t( p );

    trigger_gcd = 0;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();
    p -> auto_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();
    if ( p -> moving ) return false;
    return( p -> auto_attack -> execute_event == 0 ); // not swinging
  }
};


// Seals last 30min, don't bother tracking expiration

struct seal_of_command_t : public paladin_attack_t
{
  seal_of_command_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "seal_of_command", p )
  {
    assert( p -> talents.seal_of_command );

    harmful = false;
    base_cost = p -> resource_base[ RESOURCE_MANA ] * 0.14; // FIXME
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    consume_resource();

    paladin_t* p = player -> cast_paladin();
    p -> active_seal = SEAL_OF_COMMAND;
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();
    if ( p -> active_seal == SEAL_OF_COMMAND )
      return false;

    return paladin_attack_t::ready();
  }

  virtual double cost() SC_CONST { return player -> in_combat ? paladin_attack_t::cost() : 0; }
};

struct seal_of_vengeance_t : public paladin_attack_t
{
  seal_of_vengeance_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "seal_of_vengeance", p )
  {
    harmful = false;
    base_cost = p -> resource_base[ RESOURCE_MANA ] * 0.14;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    consume_resource();

    paladin_t* p = player -> cast_paladin();
    p -> active_seal = SEAL_OF_VENGEANCE;
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();
    if ( p -> active_seal == SEAL_OF_VENGEANCE )
      return false;

    return paladin_attack_t::ready();
  }

  virtual double cost() SC_CONST { return player -> in_combat ? paladin_attack_t::cost() : 0; }
};

// Avenging Wrath ==========================================================

struct avenging_wrath_t : public paladin_spell_t
{
  avenging_wrath_t( paladin_t* p, const std::string& options_str ) :
      paladin_spell_t( "avenging_wrath", p )
  {
    harmful     = false;
    cooldown    = 180 - 30 * p -> talents.sanctified_wrath;
    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.08;
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    struct avenging_wrath_expiration_t : public event_t
    {
      avenging_wrath_expiration_t( sim_t* sim, paladin_t* p ) : event_t( sim, p )
      {
        name = "Avenging Wrath Expiration";
        p -> aura_gain( "Avenging Wrath" );
        sim -> add_event( this, 20.0 );
      }
      virtual void execute()
      {
        player -> aura_loss( "Avenging Wrath" );
        player -> cast_paladin() -> _buffs.avenging_wrath = 0;
      }
    };

    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    consume_resource();
    update_ready();

    paladin_t* p = player -> cast_paladin();
    p -> _buffs.avenging_wrath = 1;

    new ( sim ) avenging_wrath_expiration_t( sim, p );
  }
};

// Consecration ============================================================
struct consecration_tick_t : public paladin_spell_t
{
  consecration_tick_t( paladin_t* p ) :
      paladin_spell_t( "consecration", p )
  {
    aoe        = true;
    dual       = true;
    background = true;
    may_crit   = false;
    may_miss   = true;
    direct_power_mod = 0.04;
    base_attack_power_multiplier = 1;
  }

  virtual void execute()
  {
    paladin_spell_t::execute();
    if ( result_is_hit() )
    {
      tick_dmg = direct_dmg;
      update_stats( DMG_OVER_TIME );
    }
  }

  virtual double calculate_direct_damage()
  {
    // FIXME ranks
    base_dd_min = base_dd_max = 113;
    base_dd_min += 0.04 * total_attack_power();
    base_dd_max += 0.04 * total_attack_power();

    return paladin_spell_t::calculate_direct_damage();
  }
};

struct consecration_t : public paladin_spell_t
{
  action_t* consecration_tick;

  consecration_t( paladin_t* p, const std::string& options_str ) :
      paladin_spell_t( "consecration", p )
  {
    num_ticks      = 8 + ( p -> glyphs.consecration ? 2 : 0 );
    base_tick_time = 1;
    cooldown       = num_ticks;
    base_cost      = p -> resource_base[ RESOURCE_MANA ] * 0.22;
    may_miss       = false;

    consecration_tick = new consecration_tick_t( p );
  }

  virtual void tick()
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );
    consecration_tick -> execute();
    update_time( DMG_OVER_TIME );
  }
};

// Crusader Strike =========================================================

// 75% wpn damage, 4s cd, 5%basemana
struct crusader_strike_t : public paladin_attack_t
{
  crusader_strike_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "crusader_strike", p )
  {
    check_talent( p -> talents.crusader_strike );

    weapon                 = &( p -> main_hand_weapon );
    weapon_multiplier     *= 0.75;
    normalize_weapon_speed = true;

    cooldown    = 4;
    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.05;

    base_multiplier *= 1.0 + 0.05 * p -> talents.sanctity_of_battle;
    base_multiplier *= 1.0 + 0.05 * p -> talents.the_art_of_war;
    if ( p -> set_bonus.tier8_4pc() ) base_crit += 0.10;
  }

  virtual void execute()
  {
    paladin_attack_t::execute();

    if ( result == RESULT_CRIT )
    {
      trigger_righteous_vengeance( this );
    }
  }
};

// Divine Storm ============================================================

// 110% damage, 10s cd, 12%basemana
struct divine_storm_t : public paladin_attack_t
{
  divine_storm_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "divine_storm", p )
  {
    check_talent( p -> talents.divine_storm );

    weapon                 = &( p -> main_hand_weapon );
    weapon_multiplier     *= 1.1;
    normalize_weapon_speed = true;

    cooldown    = 10;
    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.12;

    base_multiplier *= 1.0 + 0.05 * p -> talents.the_art_of_war;
    if ( p -> set_bonus.tier7_2pc() ) base_multiplier *= 1.1;
    if ( p -> set_bonus.tier8_4pc() ) base_crit += 0.10;
  }

  virtual void execute()
  {
    paladin_attack_t::execute();

    if ( result == RESULT_CRIT )
    {
      trigger_righteous_vengeance( this );
    }
  }
};

// Exorcism ================================================================
struct exorcism_t : public paladin_spell_t
{
  exorcism_t( paladin_t* p, const std::string& options_str ) :
      paladin_spell_t( "exorcism", p )
  {
    base_execute_time = 1.5;
    cooldown = 15;
    may_crit = true;

    direct_power_mod = 0.15;

    base_multiplier *= 1.0 + 0.05 * p -> talents.sanctity_of_battle;
    if ( p -> glyphs.exorcism )
      base_multiplier *= 1.2;
    base_attack_power_multiplier = 1;
  }

  virtual double execute_time() SC_CONST
  {
    paladin_t* p = player -> cast_paladin();

    double t = paladin_spell_t::execute_time();
    if ( p -> _buffs.the_art_of_war )
    {
      t -= 0.75 * p -> talents.the_art_of_war;
    }
    return ( t < 0 ) ? 0 : t;
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();

    if ( ! paladin_spell_t::ready() )
      return false;

    return p -> _buffs.the_art_of_war != 0;
  }

  virtual void execute()
  {
    paladin_t* p = player -> cast_paladin();

    paladin_spell_t::execute();

    if ( p -> _expirations.the_art_of_war )
      event_t::early( p -> _expirations.the_art_of_war );
  }

  virtual double calculate_direct_damage()
  {
    static rank_t ranks[] =
    {
      { 79, 9, 1028, 1146, 0, 0.08 },
    };
    init_rank( ranks );
    base_dd_min += 0.15 * total_attack_power();
    base_dd_max += 0.15 * total_attack_power();

    return paladin_spell_t::calculate_direct_damage();
  }
};

// Hammer of Wrath =========================================================

struct hammer_of_wrath_t : public paladin_attack_t
{
  hammer_of_wrath_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "hammer_of_wrath", p, SCHOOL_HOLY )
  {
    trigger_seal = false;

    may_parry = false;
    may_dodge = false;

    cooldown         = 6;
    direct_power_mod = 0.15;
    base_spell_power_multiplier = 1;

    base_crit += 0.25 * p -> talents.sanctified_wrath;
  }

  virtual double calculate_direct_damage()
  {
    static rank_t ranks[] =
    {
      { 80, 6, 1139, 1257, 0, 0.12 },
      { 74, 5,  878,  970, 0, 0.12 },
      { 68, 4,  733,  809, 0, 0.12 },
      { 60, 3,  570,  628, 0, 0.14 },
      { 52, 2,  459,  507, 0, 0.14 },
      { 44, 1,  351,  387, 0, 0.14 },
    };
    init_rank( ranks );
    base_dd_min += 0.15 * total_spell_power();
    base_dd_max += 0.15 * total_spell_power();

    return paladin_attack_t::calculate_direct_damage();
  }

  virtual bool ready()
  {
    if ( sim -> target -> health_percentage() > 20 )
      return false;

    return paladin_attack_t::ready();
  }
};

// Judgement ===============================================================

struct judgement_t : public paladin_attack_t
{
  judgement_t( paladin_t* p, const std::string& options_str ) :
      paladin_attack_t( "judgement", p, SCHOOL_HOLY )
  {
    may_parry = false;
    may_dodge = false;
    may_block = false;

    trigger_seal = false;

    cooldown         = 10 - p -> talents.improved_judgements - p -> set_bonus.tier7_4pc();
    base_cost        = p -> resource_base[ RESOURCE_MANA ] * 0.05;

    base_crit       += 0.06 * p -> talents.fanaticism;
    base_multiplier *= 1.0 + 0.05 * p -> talents.the_art_of_war;
    base_multiplier *= 1.0 + 0.1 * p -> glyphs.judgement;
    base_spell_power_multiplier = 1;
  }

  virtual void player_buff()
  {
    paladin_t* p = player -> cast_paladin();

    paladin_attack_t::player_buff();

    switch ( p -> active_seal )
    {
    case SEAL_OF_COMMAND:
      // FIXME
      break;
    case SEAL_OF_VENGEANCE:
      // (1 + 14% AP + 22% SP) * (1 + num_stacks * 0.1)
      player_multiplier *= 1 + p -> talents.seals_of_the_pure * 0.03;
      player_multiplier *= 1 + p -> _buffs.seal_of_vengeance_stacks * 0.1;
      break;
    default:
      assert( 0 );
      break;
    }
  }

  virtual double calculate_direct_damage()
  {
    paladin_t* p = player -> cast_paladin();

    switch ( p -> active_seal )
    {
    case SEAL_OF_COMMAND:
      // FIXME
      break;
    case SEAL_OF_VENGEANCE:
      // (1 + 14% AP + 22% SP) * (1 + num_stacks * 0.1)
      base_dd_min = base_dd_max = ( 1 + 0.22 * total_spell_power() );
      direct_power_mod = 0.14;
      break;
    default:
      assert( 0 );
      break;
    }

    return paladin_attack_t::calculate_direct_damage();
  }

  virtual void execute()
  {
    paladin_attack_t::execute();

    if ( result_is_hit() )
    {
      trigger_judgements_of_the_wise( this );

      if ( result == RESULT_CRIT )
      {
        trigger_righteous_vengeance( this );
      }
    }
  }

  virtual bool ready()
  {
    paladin_t* p = player -> cast_paladin();
    if ( p -> active_seal == SEAL_NONE )
      return false;

    return paladin_attack_t::ready();
  }
};

// paladin_t::init_race ======================================================

void paladin_t::init_race()
{
  race = util_t::parse_race_type( race_str );
  switch ( race )
  {
  case RACE_HUMAN:
  case RACE_DWARF:
  case RACE_DRAENEI:
  case RACE_BLOOD_ELF:
    break;
  default:
    race = RACE_DWARF;
    race_str = util_t::race_type_string( race );
  }

  player_t::init_race();
}


void paladin_t::init_base()
{
  attribute_base[ ATTR_STRENGTH  ] = rating_t::get_attribute_base( level, PALADIN, race, BASE_STAT_STRENGTH );
  attribute_base[ ATTR_AGILITY   ] = rating_t::get_attribute_base( level, PALADIN, race, BASE_STAT_AGILITY );
  attribute_base[ ATTR_STAMINA   ] = rating_t::get_attribute_base( level, PALADIN, race, BASE_STAT_STAMINA );
  attribute_base[ ATTR_INTELLECT ] = rating_t::get_attribute_base( level, PALADIN, race, BASE_STAT_INTELLECT );
  attribute_base[ ATTR_SPIRIT    ] = rating_t::get_attribute_base( level, PALADIN, race, BASE_STAT_SPIRIT );
  resource_base[ RESOURCE_HEALTH ] = rating_t::get_attribute_base( level, PALADIN, race, BASE_STAT_HEALTH );
  resource_base[ RESOURCE_MANA   ] = rating_t::get_attribute_base( level, PALADIN, race, BASE_STAT_MANA );
  base_spell_crit                  = rating_t::get_attribute_base( level, PALADIN, race, BASE_STAT_SPELL_CRIT );
  base_attack_crit                 = rating_t::get_attribute_base( level, PALADIN, race, BASE_STAT_MELEE_CRIT );
  initial_spell_crit_per_intellect = rating_t::get_attribute_base( level, PALADIN, race, BASE_STAT_SPELL_CRIT_PER_INT );
  initial_attack_crit_per_agility  = rating_t::get_attribute_base( level, PALADIN, race, BASE_STAT_MELEE_CRIT_PER_AGI );

  attribute_multiplier_initial[ ATTR_STRENGTH ] *= 1.0 + talents.divine_strength * 0.03;

  base_attack_power = ( level * 3 ) - 20;
  initial_attack_power_per_strength = 2.0;
  initial_attack_power_per_agility  = 0.0;

  health_per_stamina = 10;
  mana_per_intellect = 15;
}

void paladin_t::init_gains()
{
  player_t::init_gains();

  gains_jotw = get_gain( "judgements_of_the_wise" );
}

void paladin_t::init_glyphs()
{
  memset( ( void* ) &glyphs, 0x0, sizeof( glyphs_t ) );

  std::vector<std::string> glyph_names;
  int num_glyphs = util_t::string_split( glyph_names, glyphs_str, ",/" );

  for ( int i=0; i < num_glyphs; i++ )
  {
    std::string& n = glyph_names[ i ];

    if     ( n == "consecration"        ) glyphs.consecration = 1;
    else if ( n == "exorcism"            ) glyphs.exorcism = 1;
    else if ( n == "judgement"           ) glyphs.judgement = 1;
    else if ( n == "seal_of_vengeance"   ) glyphs.seal_of_vengeance = 1;
    else if ( ! sim -> parent ) util_t::printf( "simcraft: Player %s has unrecognized glyph %s\n", name(), n.c_str() );
  }
}

void paladin_t::init_rng()
{
  player_t::init_rng();

  rng_jotw = get_rng( "judgements_of_the_wise" );
}

void paladin_t::init_scaling()
{
  player_t::init_scaling();

  scales_with[ STAT_SPIRIT ] = 0;
}

void paladin_t::init_actions()
{
  if ( action_list_str.empty() )
  {
    action_list_str = "flask,type=endless_rage/food,type=fish_feast/seal_of_vengeance/auto_attack";
    int num_items = items.size();
    for ( int i=0; i < num_items; i++ )
    {
      if ( items[ i ].use.active() )
      {
        action_list_str += "/use_item,name=";
        action_list_str += items[ i ].name();
      }
    }
    action_list_str += "/avenging_wrath";
    action_list_str += "/hammer_of_wrath/judgement";
    if ( talents.divine_storm )
      action_list_str += "/divine_storm";
    if ( talents.crusader_strike )
      action_list_str += "/crusader_strike";
    action_list_str += "/exorcism";
    action_list_str += "/consecration";
    action_list_str += "/mana_potion";

    action_list_default = 1;
  }

  player_t::init_actions();
}

void paladin_t::reset()
{
  player_t::reset();

  active_seal = SEAL_NONE;

  _buffs.reset();
  _expirations.reset();
}

void paladin_t::interrupt()
{
  player_t::interrupt();

  if ( auto_attack ) auto_attack -> cancel();
}

double paladin_t::composite_spell_power( int school ) SC_CONST
{
  double sp = player_t::composite_spell_power( school );
  sp += composite_attack_power_multiplier() * composite_attack_power() * talents.sheath_of_light * 0.1;
  return sp;
}

bool paladin_t::get_talent_trees( std::vector<int*>& holy, std::vector<int*>& protection, std::vector<int*>& retribution )
{
  talent_translation_t translation[][3] =
  {
    { {  1, NULL                                     }, {  1, NULL                                      }, {  1, NULL                                  } },
    { {  2, &( talents.seals_of_the_pure           ) }, {  2, &( talents.divine_strength              ) }, {  2, &( talents.benediction              ) } },
    { {  3, NULL                                     }, {  3, NULL                                      }, {  3, &( talents.improved_judgements      ) } },
    { {  4, NULL                                     }, {  4, NULL                                      }, {  4, &( talents.heart_of_the_crusader    ) } },
    { {  5, NULL                                     }, {  5, NULL                                      }, {  5, &( talents.improved_bom             ) } },
    { {  6, NULL                                     }, {  6, NULL                                      }, {  6, NULL                                  } },
    { {  7, NULL                                     }, {  7, NULL                                      }, {  7, &( talents.conviction               ) } },
    { {  8, NULL                                     }, {  8, NULL                                      }, {  8, &( talents.seal_of_command          ) } },
    { {  9, NULL                                     }, {  9, NULL                                      }, {  9, NULL                                  } },
    { { 10, NULL                                     }, { 10, NULL                                      }, { 10, NULL                                  } },
    { { 11, NULL                                     }, { 11, NULL                                      }, { 11, &( talents.sanctity_of_battle       ) } },
    { { 12, NULL                                     }, { 12, NULL                                      }, { 12, &( talents.crusade                  ) } },
    { { 13, NULL                                     }, { 13, NULL                                      }, { 13, &( talents.two_handed_weapon_spec   ) } },
    { { 14, NULL                                     }, { 14, NULL                                      }, { 14, &( talents.sanctified_retribution   ) } },
    { { 15, NULL                                     }, { 15, NULL                                      }, { 15, &( talents.vengeance                ) } },
    { { 16, NULL                                     }, { 16, NULL                                      }, { 16, NULL                                  } },
    { { 17, NULL                                     }, { 17, NULL                                      }, { 17, &( talents.the_art_of_war           ) } },
    { { 18, NULL                                     }, { 18, NULL                                      }, { 18, NULL                                  } },
    { { 19, NULL                                     }, { 19, NULL                                      }, { 19, &( talents.judgements_of_the_wise   ) } },
    { { 20, NULL                                     }, { 20, NULL                                      }, { 20, &( talents.fanaticism               ) } },
    { { 21, NULL                                     }, { 21, NULL                                      }, { 21, &( talents.sanctified_wrath         ) } },
    { { 22, NULL                                     }, { 22, NULL                                      }, { 22, &( talents.swift_retribution        ) } },
    { { 23, NULL                                     }, { 23, NULL                                      }, { 23, &( talents.crusader_strike          ) } },
    { { 24, NULL                                     }, { 24, NULL                                      }, { 24, &( talents.sheath_of_light          ) } },
    { { 25, NULL                                     }, { 25, NULL                                      }, { 25, &( talents.righteous_vengeance      ) } },
    { { 26, NULL                                     }, { 26, NULL                                      }, { 26, &( talents.divine_storm             ) } },
    { {  0, NULL                                     }, {  0, NULL                                      }, {  0, NULL                                  } }
  };

  return get_talent_translation( holy, protection, retribution, translation );
}

std::vector<option_t>& paladin_t::get_options()
{
  if ( options.empty() )
  {
    player_t::get_options();

    option_t paladin_options[] =
    {
      // @option_doc loc=player/paladin/talents title="Talents"
      { "benediction",                    OPT_INT, &( talents.benediction                 ) },
      { "conviction",                     OPT_INT, &( talents.conviction                  ) },
      { "crusade",                        OPT_INT, &( talents.crusade                     ) },
      { "crusader_strike",                OPT_INT, &( talents.crusader_strike             ) },
      { "divine_storm",                   OPT_INT, &( talents.divine_storm                ) },
      { "divine_strength",                OPT_INT, &( talents.divine_strength             ) },
      { "fanaticism",                     OPT_INT, &( talents.fanaticism                  ) },
      { "heart_of_the_crusader",          OPT_INT, &( talents.heart_of_the_crusader       ) },
      { "improved_bom",                   OPT_INT, &( talents.improved_bom                ) },
      { "improved_judgements",            OPT_INT, &( talents.improved_judgements         ) },
      { "judgements_of_the_wise",         OPT_INT, &( talents.judgements_of_the_wise      ) },
      { "righteous_vengeance",            OPT_INT, &( talents.righteous_vengeance         ) },
      { "sanctified_retribution",         OPT_INT, &( talents.sanctified_retribution      ) },
      { "sanctified_wrath",               OPT_INT, &( talents.sanctified_wrath            ) },
      { "sanctity_of_battle",             OPT_INT, &( talents.sanctity_of_battle          ) },
      { "seal_of_command",                OPT_INT, &( talents.seal_of_command             ) },
      { "seals_of_the_pure",              OPT_INT, &( talents.seals_of_the_pure           ) },
      { "sheath_of_light",                OPT_INT, &( talents.sheath_of_light             ) },
      { "swift_retribution",              OPT_INT, &( talents.swift_retribution           ) },
      { "the_art_of_war",                 OPT_INT, &( talents.the_art_of_war              ) },
      { "two_handed_weapon_spec",         OPT_INT, &( talents.two_handed_weapon_spec      ) },
      { "vengeance",                      OPT_INT, &( talents.vengeance                   ) },

      { NULL, OPT_UNKNOWN, NULL }
    };

    option_t::copy( options, paladin_options );
  }
  return options;
}

action_t* paladin_t::create_action( const std::string& name, const std::string& options_str )
{
  if ( name == "auto_attack"             ) return new auto_attack_t              ( this, options_str );
  if ( name == "avenging_wrath"          ) return new avenging_wrath_t           ( this, options_str );
  if ( name == "consecration"            ) return new consecration_t             ( this, options_str );
  if ( name == "crusader_strike"         ) return new crusader_strike_t          ( this, options_str );
  if ( name == "divine_storm"            ) return new divine_storm_t             ( this, options_str );
  if ( name == "exorcism"                ) return new exorcism_t                 ( this, options_str );
  if ( name == "hammer_of_wrath"         ) return new hammer_of_wrath_t          ( this, options_str );
  if ( name == "judgement"               ) return new judgement_t                ( this, options_str );
  if ( name == "seal_of_command"         ) return new seal_of_command_t          ( this, options_str );
  if ( name == "seal_of_corruption"      ) return new seal_of_vengeance_t        ( this, options_str );
  if ( name == "seal_of_vengeance"       ) return new seal_of_vengeance_t        ( this, options_str );
  return player_t::create_action( name, options_str );
}

// paladin_t::decode_set ====================================================

int paladin_t::decode_set( item_t& item )
{
  if ( strstr( item.name(), "redemption" ) ) return SET_T7;
  if ( strstr( item.name(), "aegis"      ) ) return SET_T8;
  if ( strstr( item.name(), "turalyon"   ) ) return SET_T9;
  if ( strstr( item.name(), "liadrin"    ) ) return SET_T9;
  return SET_NONE;
}

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

player_t* player_t::create_paladin( sim_t* sim, const std::string& name, int race_type )
{
  return new paladin_t( sim, name, race_type );
}

// player_t::paladin_init ====================================================

void player_t::paladin_init( sim_t* sim )
{
  sim -> auras.sanctified_retribution = new buff_t( sim, "sanctified_retribution", 1 );
  sim -> auras.swift_retribution      = new buff_t( sim, "swift_retribution",      1 );

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    p -> buffs.blessing_of_kings  = new buff_t( p, "blessing_of_kings",  1 );
    p -> buffs.blessing_of_might  = new buff_t( p, "blessing_of_might",  1 );
    p -> buffs.blessing_of_wisdom = new buff_t( p, "blessing_of_wisdom", 1 );
  }

}

// player_t::paladin_combat_begin ============================================

void player_t::paladin_combat_begin( sim_t* sim )
{
  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( p -> ooc_buffs() )
    {
      if ( sim -> overrides.blessing_of_kings  ) p -> buffs.blessing_of_kings  -> override();
      if ( sim -> overrides.blessing_of_might  ) p -> buffs.blessing_of_might  -> override( 1, 688 );
      if ( sim -> overrides.blessing_of_wisdom ) p -> buffs.blessing_of_wisdom -> override( 1, 91 * 1.2 );
    }
  }

  if( sim -> overrides.sanctified_retribution ) sim -> auras.sanctified_retribution -> override();
  if( sim -> overrides.swift_retribution      ) sim -> auras.swift_retribution      -> override();
}
