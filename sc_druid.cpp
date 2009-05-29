// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Druid
// ==========================================================================

struct druid_t : public player_t
{
  // Active
  action_t* active_insect_swarm;
  action_t* active_moonfire;
  action_t* active_rake;
  action_t* active_rip;

  // Buffs
  struct _buffs_t
  {
    int    berserk;
    int    bear_form;
    int    cat_form;
    int    combo_points;
    double eclipse_starfire;
    double eclipse_wrath;
    int    moonkin_form;
    double natures_grace;
    int    natures_swiftness;
    int    omen_of_clarity;
    int    savage_roar;
    int    starfall;
    int    stealthed;
    int    t8_4pc_balance;
    double tigers_fury;

    void reset() { memset( ( void* ) this, 0x00, sizeof( _buffs_t ) ); }
    _buffs_t() { reset(); }
  };
  _buffs_t _buffs;

  // Cooldowns
  struct _cooldowns_t
  {
    double eclipse;

    void reset() { memset( ( void* ) this, 0x00, sizeof( _cooldowns_t ) ); }
    _cooldowns_t() { reset(); }
  };
  _cooldowns_t _cooldowns;

  // Expirations
  struct _expirations_t
  {
    event_t* berserk;
    event_t* eclipse;
    event_t* idol_of_the_corruptor;
    event_t* natures_grace;
    event_t* mangle;
    event_t* savage_roar;
    event_t* t8_4pc_balance;
    event_t* unseen_moon;

    void reset() { memset( ( void* ) this, 0x00, sizeof( _expirations_t ) ); }
    _expirations_t() { reset(); }
  };
  _expirations_t _expirations;

  // Gains
  gain_t* gains_energy_refund;
  gain_t* gains_moonkin_form;
  gain_t* gains_omen_of_clarity;
  gain_t* gains_primal_precision;
  gain_t* gains_tigers_fury;

  // Procs
  proc_t* procs_combo_points;
  proc_t* procs_combo_points_wasted;
  proc_t* procs_omen_of_clarity;
  proc_t* procs_primal_fury;
  proc_t* procs_unseen_moon;

  // Up-Times
  uptime_t* uptimes_eclipse_starfire;
  uptime_t* uptimes_eclipse_wrath;
  uptime_t* uptimes_energy_cap;
  uptime_t* uptimes_natures_grace;
  uptime_t* uptimes_savage_roar;
  uptime_t* uptimes_rip;
  uptime_t* uptimes_rake;

  // Random Number Generation
  rng_t* rng_eclipse;
  rng_t* rng_natures_grace;
  rng_t* rng_omen_of_clarity;
  rng_t* rng_primal_fury;
  rng_t* rng_unseen_moon;

  attack_t* melee_attack;

  double equipped_weapon_dps;

  struct talents_t
  {
    int  balance_of_power;
    int  berserk;
    int  brambles;
    int  celestial_focus;
    int  dreamstate;
    int  earth_and_moon;
    int  eclipse;
    int  feral_aggression;
    int  ferocity;
    int  force_of_nature;
    int  furor;
    int  genesis;
    int  heart_of_the_wild;
    int  improved_faerie_fire;
    int  improved_insect_swarm;
    int  improved_mangle;
    int  improved_mark_of_the_wild;
    int  improved_moonfire;
    int  improved_moonkin_form;
    int  insect_swarm;
    int  intensity;
    int  king_of_the_jungle;
    int  leader_of_the_pack;
    int  living_spirit;
    int  lunar_guidance;
    int  mangle;
    int  master_shapeshifter;
    int  moonfury;
    int  moonglow;
    int  moonkin_form;
    int  natural_perfection;
    int  naturalist;
    int  natures_grace;
    int  natures_majesty;
    int  natures_reach;
    int  natures_splendor;
    int  natures_swiftness;
    int  omen_of_clarity;
    int  predatory_instincts;
    int  predatory_strikes;
    int  primal_fury;
    int  primal_gore;
    int  primal_precision;
    int  rend_and_tear;
    int  savage_fury;
    int  sharpened_claws;
    int  shredding_attacks;
    int  starfall;
    int  starlight_wrath;
    int  survival_of_the_fittest;
    int  vengeance;
    int  wrath_of_cenarius;

    // Not Yet Implemented
    int  feral_instinct;
    int  infected_wounds;
    int  protector_of_the_pack;

    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int berserk;
    int focus;
    int innervate;
    int insect_swarm;
    int mangle;
    int moonfire;
    int rip;
    int savage_roar;
    int shred;
    int starfire;
    int starfall;
    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  struct idols_t
  {
    int corruptor;
    int crying_wind;
    int ravenous_beast;
    int shooting_star;
    int steadfast_renewal;
    int unseen_moon;
    int worship;
    idols_t() { memset( ( void* ) this, 0x0, sizeof( idols_t ) ); }
  };
  idols_t idols;

  struct tiers_t
  {
    int t4_2pc_balance;
    int t4_4pc_balance;
    int t5_2pc_balance;
    int t5_4pc_balance;
    int t6_2pc_balance;
    int t6_4pc_balance;
    int t7_2pc_balance;
    int t7_4pc_balance;
    int t8_2pc_balance;
    int t8_4pc_balance;
    int t4_2pc_feral;
    int t4_4pc_feral;
    int t5_2pc_feral;
    int t5_4pc_feral;
    int t6_2pc_feral;
    int t6_4pc_feral;
    int t7_2pc_feral;
    int t7_4pc_feral;
    int t8_2pc_feral;
    int t8_4pc_feral;
    tiers_t() { memset( ( void* ) this, 0x0, sizeof( tiers_t ) ); }
  };
  tiers_t tiers;

  druid_t( sim_t* sim, const std::string& name ) : player_t( sim, DRUID, name )
  {
    active_insect_swarm   = 0;
    active_moonfire       = 0;
    active_rake           = 0;
    active_rip            = 0;

    melee_attack = 0;

    equipped_weapon_dps = 0;
  }

  // Character Definition
  virtual void      init_actions();
  virtual void      init_base();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_rating();
  virtual void      init_rng();
  virtual void      init_unique_gear();
  virtual void      init_uptimes();
  virtual void      reset();
  virtual void      interrupt();
  virtual void      regen( double periodicity );
  virtual double    composite_attack_power();
  virtual double    composite_attack_power_multiplier();
  virtual double    composite_attack_crit();
  virtual double    composite_spell_hit();
  virtual double    composite_spell_crit();
  virtual bool      get_talent_trees( std::vector<int*>& balance, std::vector<int*>& feral, std::vector<int*>& restoration );
  virtual bool      parse_talents_mmo( const std::string& talent_string );
  virtual bool      parse_option ( const std::string& name, const std::string& value );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name );
  virtual int       primary_resource() { return talents.moonkin_form ? RESOURCE_MANA : RESOURCE_ENERGY; }
  virtual int       primary_role()     { return talents.moonkin_form ? ROLE_SPELL    : ROLE_ATTACK;     }

  // Utilities
  double combo_point_rank( double* cp_list )
  {
    assert( _buffs.combo_points > 0 );
    return cp_list[ _buffs.combo_points-1 ];
  }
  double combo_point_rank( double cp1, double cp2, double cp3, double cp4, double cp5 )
  {
    double cp_list[] = { cp1, cp2, cp3, cp4, cp5 };
    return combo_point_rank( cp_list );
  }
  void reset_gcd()
  {
    for ( action_t* a=action_list; a; a = a -> next )
    {
      if ( a -> trigger_gcd != 0 ) a -> trigger_gcd = base_gcd;
    }
  }
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// ==========================================================================
// Druid Attack
// ==========================================================================

struct druid_attack_t : public attack_t
{
  int requires_stealth;
  int requires_position;
  bool requires_combo_points;
  bool adds_combo_points;
  int berserk, min_combo_points, max_combo_points;
  double min_energy, max_energy;
  double min_mangle_expire, max_mangle_expire;
  double min_savage_roar_expire, max_savage_roar_expire;
  double min_rip_expire, max_rip_expire;
  double min_rake_expire, max_rake_expire;

  druid_attack_t( const char* n, player_t* player, int s=SCHOOL_PHYSICAL, int t=TREE_NONE, bool special=true ) :
      attack_t( n, player, RESOURCE_ENERGY, s, t, special ),
      requires_stealth( 0 ),
      requires_position( POSITION_NONE ),
      requires_combo_points( false ),
      adds_combo_points( false ),
      berserk( 0 ),
      min_combo_points( 0 ), max_combo_points( 0 ),
      min_energy( 0 ), max_energy( 0 ),
      min_mangle_expire( 0 ), max_mangle_expire( 0 ),
      min_savage_roar_expire( 0 ), max_savage_roar_expire( 0 ),
      min_rip_expire( 0 ), max_rip_expire( 0 ),
      min_rake_expire( 0 ), max_rake_expire( 0 )
  {
    druid_t* p = player -> cast_druid();

    base_crit_multiplier *= 1.0 + util_t::talent_rank( p -> talents.predatory_instincts, 3, 0.03, 0.07, 0.10 );
  }

  virtual void   parse_options( option_t*, const std::string& options_str );
  virtual double cost();
  virtual void   execute();
  virtual void   consume_resource();
  virtual double calculate_direct_damage();
  virtual void   player_buff();
  virtual bool   ready();
  virtual void   tick();
};

// ==========================================================================
// Pet Treants
// ==========================================================================

struct treants_pet_t : public pet_t
{
  struct melee_t : public attack_t
  {
    melee_t( player_t* player ) :
        attack_t( "treant_melee", player )
    {
      druid_t* o = player -> cast_pet() -> owner -> cast_druid();

      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      base_dd_min = base_dd_max = 1;
      background = true;
      repeating = true;
      may_crit = true;

      base_multiplier *= 1.0 + o -> talents.brambles * 0.05;

      // Model the three Treants as one actor hitting 3x hard
      base_multiplier *= 3.0;
    }
  };

  melee_t* melee;

  treants_pet_t( sim_t* sim, player_t* owner, const std::string& pet_name ) :
      pet_t( sim, owner, pet_name ), melee( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.damage     = 340;
    main_hand_weapon.swing_time = 1.8;
  }
  virtual void init_base()
  {
    attribute_base[ ATTR_STRENGTH  ] = 331;
    attribute_base[ ATTR_AGILITY   ] = 113;
    attribute_base[ ATTR_STAMINA   ] = 361;
    attribute_base[ ATTR_INTELLECT ] = 65;
    attribute_base[ ATTR_SPIRIT    ] = 109;

    base_attack_power = -20;
    initial_attack_power_per_strength = 2.0;

    melee = new melee_t( this );
  }
  virtual double composite_attack_power()
  {
    double ap = pet_t::composite_attack_power();
    ap += 0.57 * owner -> composite_spell_power( SCHOOL_MAX );
    return ap;
  }
  virtual void schedule_ready( double delta_time=0,
			       bool   waiting=false )
  {
    pet_t::schedule_ready( delta_time, waiting );
    if( ! melee -> execute_event ) melee -> execute();
  }
  virtual void interrupt()
  {
    pet_t::interrupt();
    melee -> cancel();
  }
};

// enter_stealth ===========================================================

static void enter_stealth( druid_t* p )
{
  p -> _buffs.stealthed = 1;
}

// break_stealth ===========================================================

static void break_stealth( druid_t* p )
{
  if ( p -> _buffs.stealthed == 1 )
  {
    p -> _buffs.stealthed = -1;
  }
}

// clear_combo_points ======================================================

static void clear_combo_points( druid_t* p )
{
  if ( p -> _buffs.combo_points <= 0 ) return;

  const char* name[] =
    { "Combo Points (1)",
      "Combo Points (2)",
      "Combo Points (3)",
      "Combo Points (4)",
      "Combo Points (5)"
    };

  p -> aura_loss( name[ p -> _buffs.combo_points - 1 ] );

  p -> _buffs.combo_points = 0;
}

// add_combo_point=== ======================================================

static void add_combo_point( druid_t* p )
{
  if ( p -> _buffs.combo_points >= 5 )
  {
    p -> procs_combo_points_wasted -> occur();
    return;
  }

  const char* name[] =
    { "Combo Points (1)",
      "Combo Points (2)",
      "Combo Points (3)",
      "Combo Points (4)",
      "Combo Points (5)"
    };

  p -> _buffs.combo_points++;

  p -> aura_gain( name[ p -> _buffs.combo_points - 1 ] );

  p -> procs_combo_points -> occur();
}


// trigger_omen_of_clarity ==================================================

static void trigger_omen_of_clarity( action_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( p -> talents.omen_of_clarity == 0 ) return;

  // Convert to fixed ~6% proc rate

  if ( p -> rng_omen_of_clarity -> roll( 3.5 / 60.0 ) )
  {
    p -> aura_gain( "Omen of Clarity" );
    p -> _buffs.omen_of_clarity = 1;
    p -> procs_omen_of_clarity -> occur();
  }
}

// trigger_faerie_fire =======================================================

static void trigger_faerie_fire( action_t* a )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      target_t* t = sim -> target;
      t -> debuffs.faerie_fire = 0.05;
      sim -> add_event( this, 300.0 );
    }

    virtual void execute()
    {
      target_t* t = sim -> target;
      t -> debuffs.faerie_fire = 0;
      t -> expirations.faerie_fire = 0;
    }
  };

  event_t*& e = a -> sim -> target -> expirations.faerie_fire;

  if ( e )
  {
    e -> reschedule( 300.0 );
  }
  else
  {
    e = new ( a -> sim ) expiration_t( a -> sim, a -> player );
  }
}

// trigger_improved_faerie_fire ==============================================

static void trigger_improved_faerie_fire( action_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( ! p -> talents.improved_faerie_fire )
    return;

  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      sim -> target -> debuffs.improved_faerie_fire = 3;
      sim -> add_event( this, 300.0 );
    }
    virtual void execute()
    {
      sim -> target -> debuffs.improved_faerie_fire = 0;
      sim -> target -> expirations.improved_faerie_fire = 0;
    }
  };

  event_t*& e = a -> sim -> target -> expirations.improved_faerie_fire;

  if ( e )
  {
    e -> reschedule( 300.0 );
  }
  else
  {
    e = new ( a -> sim ) expiration_t( a -> sim, a -> player );
  }
}

// trigger_mangle ============================================================

static void trigger_mangle( attack_t* a )
{
  struct mangle_expiration_t : public event_t
  {
    mangle_expiration_t( sim_t* sim, druid_t* p, double duration ): event_t( sim, p, 0 )
    {
      name = "Mangle Cat Expiration";
      sim -> target -> debuffs.mangle++;
      sim -> add_event( this, duration );
    }
    virtual void execute()
    {
      druid_t* p = player -> cast_druid();
      sim -> target -> debuffs.mangle--;
      p -> _expirations.mangle = 0;
    }
  };

  druid_t* p = a -> player -> cast_druid();

  double duration = 12.0 + ( p -> glyphs.mangle ? 6.0 : 0.0 );

  event_t*& e = p -> _expirations.mangle;

  if ( e )
  {
    if ( e -> occurs() < ( a -> sim -> current_time + duration ) )
    {
      e -> reschedule( duration );
    }
  }
  else
  {
    e = new ( a -> sim ) mangle_expiration_t( a -> sim, p, duration );
  }
}

// trigger_earth_and_moon ===================================================

static void trigger_earth_and_moon( spell_t* s )
{
  druid_t* p = s -> player -> cast_druid();

  if ( p -> talents.earth_and_moon == 0 ) return;

  struct earth_and_moon_expiration_t : public event_t
  {
    earth_and_moon_expiration_t( sim_t* sim, druid_t* p ) : event_t( sim )
    {
      name = "Earth and Moon Expiration";
      if ( sim -> log ) log_t::output( sim, "%s gains Earth and Moon", sim -> target -> name() );
      sim -> target -> debuffs.earth_and_moon = ( int ) util_t::talent_rank( p -> talents.earth_and_moon, 3, 4, 9, 13 );
      sim -> add_event( this, 12.0 );
    }
    virtual void execute()
    {
      if ( sim -> log ) log_t::output( sim, "%s loses Earth and Moon", sim -> target -> name() );
      sim -> target -> debuffs.earth_and_moon = 0;
      sim -> target -> expirations.earth_and_moon = 0;
    }
  };

  event_t*& e = s -> sim -> target -> expirations.earth_and_moon;

  if ( e )
  {
    e -> reschedule( 12.0 );
  }
  else
  {
    e = new ( s -> sim ) earth_and_moon_expiration_t( s -> sim, p );
  }
}

// trigger_natures_grace ====================================================

static void trigger_natures_grace( spell_t* s )
{
  druid_t* p = s -> player -> cast_druid();

  if ( ! p -> rng_natures_grace -> roll( p -> talents.natures_grace / 3.0 ) )
    return;

  struct natures_grace_expiration_t : public event_t
  {
    natures_grace_expiration_t( sim_t* sim, druid_t* p ) : event_t( sim, p )
    {
      name = "Nature's Grace Expiration";
      p -> aura_gain( "Nature's Grace" );
      p -> _buffs.natures_grace = 0.20;
      sim -> add_event( this, 3.0 );
    }
    virtual void execute()
    {
      druid_t* p = player -> cast_druid();
      p -> _buffs.natures_grace       = 0;
      p -> _expirations.natures_grace = 0;
      p -> aura_loss( "Nature's Grace" );
    }
  };

  event_t*& e = p -> _expirations.natures_grace;

  if ( e )
  {
    e -> reschedule( 3.0 );
  }
  else
  {
    e = new ( s -> sim ) natures_grace_expiration_t( s -> sim, p );
  }
}

// trigger_eclipse_wrath ===================================================

static void trigger_eclipse_wrath( spell_t* s )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, druid_t* p ) : event_t( sim, p )
    {
      name = "Eclipse Wrath Expiration";
      p -> aura_gain( "Eclipse Wrath" );
      p -> _buffs.eclipse_wrath = sim -> current_time;
      p -> _cooldowns.eclipse = sim -> current_time + 30;
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      druid_t* p = player -> cast_druid();
      p -> aura_loss( "Eclipse Wrath" );
      p -> _buffs.eclipse_wrath = 0;
      p -> _expirations.eclipse = 0;
    }
  };

  druid_t* p = s -> player -> cast_druid();

  if ( p -> talents.eclipse != 0 &&
       s -> sim -> cooldown_ready( p -> _cooldowns.eclipse ) &&
       p -> rng_eclipse -> roll( p -> talents.eclipse * 1.0/3 ) )
  {
    p -> _expirations.eclipse = new ( s -> sim ) expiration_t( s -> sim, p );
  }
}

// trigger_eclipse_starfire =================================================

static void trigger_eclipse_starfire( spell_t* s )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, druid_t* p ) : event_t( sim, p )
    {
      name = "Eclipse Starfire Expiration";
      p -> aura_gain( "Eclipse Starfire" );
      p -> _buffs.eclipse_starfire = sim -> current_time;
      p -> _cooldowns.eclipse = sim -> current_time + 30;
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      druid_t* p = player -> cast_druid();
      p -> aura_loss( "Eclipse Starfire" );
      p -> _buffs.eclipse_starfire = 0;
      p -> _expirations.eclipse = 0;
    }
  };

  druid_t* p = s -> player -> cast_druid();

  if ( p -> talents.eclipse != 0 &&
       s -> sim -> cooldown_ready( p -> _cooldowns.eclipse ) &&
       p -> rng_eclipse -> roll( p -> talents.eclipse * 0.2 ) )
  {
    p -> _expirations.eclipse = new ( s -> sim ) expiration_t( s -> sim, p );
  }
}

// trigger_t8_4pc_balance ===================================================

static void trigger_t8_4pc_balance( spell_t* s )
{
  druid_t* p = s -> player -> cast_druid();

  // http://thottbot.com/test/s64824
  double chance = (s -> sim -> P313 ? 0.08 : 0.15);
  if ( ! p -> rngs.tier8_4pc -> roll( chance ) )
    return;

  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, druid_t* p ) : event_t( sim, p )
    {
      name = "Tier 8 4 Piece Balance";
      p -> aura_gain( "Tier 8 4 Piece Balance" );
      p -> _buffs.t8_4pc_balance = 1;
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      druid_t* p = player -> cast_druid();
      p -> aura_loss( "Tier 8 4 Piece Balance" );
      p -> _buffs.t8_4pc_balance       = 0;
      p -> _expirations.t8_4pc_balance = 0;
    }
  };

  p -> procs.tier8_4pc -> occur();

  event_t*& e = p -> _expirations.t8_4pc_balance;

  if ( e )
  {
    e -> reschedule( 10.0 );
  }
  else
  {
    e = new ( s -> sim ) expiration_t( s -> sim, p );
  }
}

// trigger_t8_2pc_feral =====================================================

static void trigger_t8_2pc_feral( action_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( ! p -> tiers.t8_2pc_feral )
    return;

  if ( p -> rngs.tier8_2pc -> roll( 0.02 ) )
  {
    /* 2% chance on tick, http://ptr.wowhead.com/?spell=64752
    /  It is just another chance to procc the same clearcasting
    /  buff that OoC provides. OoC and t8_2pc_feral overwrite
    /  each other. Easier to treat it just (ab)use the OoC handling */
    druid_t* p = a -> player -> cast_druid();
    p -> aura_gain( "Omen of Clarity" );
    p -> _buffs.omen_of_clarity = 1;
    p -> procs.tier8_2pc -> occur();
  }
}

// trigger_ashtongue_talisman =================================================

static void trigger_ashtongue_talisman( spell_t* s )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Ashtongue Talisman Expiration";
      player -> aura_gain( "Ashtongue Talisman" );
      player -> spell_power[ SCHOOL_MAX ] += 150;
      sim -> add_event( this, 8.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Ashtongue Talisman" );
      player -> spell_power[ SCHOOL_MAX ] -= 150;
      player -> expirations.ashtongue_talisman = 0;
    }
  };

  player_t* p = s -> player;

  if ( p -> unique_gear -> ashtongue_talisman &&
       p -> rngs.ashtongue_talisman -> roll( 0.25 ) )
  {
    p -> procs.ashtongue_talisman -> occur();

    event_t*& e = p -> expirations.ashtongue_talisman;

    if ( e )
    {
      e -> reschedule( 8.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim, p );
    }
  }
}

// trigger_unseen_moon ========================================================

static void trigger_unseen_moon( spell_t* s )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, druid_t* p ) : event_t( sim, p )
    {
      name = "Idol of the Unseen Moon";
      p -> aura_gain( "Idol of the Unseen Moon" );
      p -> spell_power[ SCHOOL_MAX ] += 140;
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      druid_t* p = player -> cast_druid();
      p -> aura_loss( "Idol of the Unseen Moon" );
      p -> spell_power[ SCHOOL_MAX ] -= 140;
      p -> _expirations.unseen_moon = 0;
    }
  };

  druid_t* p = s -> player -> cast_druid();

  if ( p -> rng_unseen_moon -> roll( 0.5 ) )
  {
    p -> procs_unseen_moon -> occur();

    event_t*& e = p -> _expirations.unseen_moon;

    if ( e )
    {
      e -> reschedule( 10.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim, p );
    }
  }
}

// trigger_primal_fury =====================================================

static void trigger_primal_fury( druid_attack_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( ! p -> talents.primal_fury )
    return;

  if ( ! a -> adds_combo_points )
    return;

  if ( p -> rng_primal_fury -> roll( p -> talents.primal_fury * 0.5 ) )
  {
    add_combo_point( p );
    p -> procs_primal_fury -> occur();
  }
}

// trigger_energy_refund ===================================================

static void trigger_energy_refund( druid_attack_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( ! a -> adds_combo_points )
    return;

  double energy_restored = a -> resource_consumed * 0.80;

  p -> resource_gain( RESOURCE_ENERGY, energy_restored, p -> gains_energy_refund );
}

// trigger_primal_precision ================================================

static void trigger_primal_precision( druid_attack_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( ! p -> talents.primal_precision )
    return;

  if ( ! a -> requires_combo_points )
    return;

  double energy_restored = a -> resource_consumed * p -> talents.primal_precision * 0.40;

  p -> resource_gain( RESOURCE_ENERGY, energy_restored, p -> gains_primal_precision );
}

// trigger_idol_of_the_corruptor ===========================================

static void trigger_idol_of_the_corruptor( attack_t* a )
{

  druid_t* p = a -> player -> cast_druid();

  struct idol_of_the_corruptor_expiration_t : public event_t
  {
    idol_of_the_corruptor_expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
    {
      name = "Idol of the Corruptor Expiration";
      player -> aura_gain( "Idol of the Corruptor" );
      player -> attribute[ ATTR_AGILITY ] += 153;
      sim -> add_event( this, 12.0 );
    }
    virtual void execute()
    {
      druid_t* p = player -> cast_druid();
      p -> aura_loss( "Idol of the Corruptor" );
      p -> attribute[ ATTR_AGILITY ] -= 153;
      p -> _expirations.idol_of_the_corruptor = 0;
    }
  };

  event_t*& e = p -> _expirations.idol_of_the_corruptor;

  if ( e )
  {
    e -> reschedule( 12.0 );
  }
  else
  {
    e = new ( a -> sim ) idol_of_the_corruptor_expiration_t( a -> sim, p );
  }
}

// =========================================================================
// Druid Attack
// =========================================================================

// druid_attack_t::parse_options ===========================================

void druid_attack_t::parse_options( option_t*          options,
                                    const std::string& options_str )
{
  option_t base_options[] =
    {
      { "berserk",          OPT_BOOL, &berserk                },
      { "min_combo_points", OPT_INT,  &min_combo_points       },
      { "max_combo_points", OPT_INT,  &max_combo_points       },
      { "cp>",              OPT_INT,  &min_combo_points       },
      { "cp<",              OPT_INT,  &max_combo_points       },
      { "min_energy",       OPT_FLT,  &min_energy             },
      { "max_energy",       OPT_FLT,  &max_energy             },
      { "energy>",          OPT_FLT,  &min_energy             },
      { "energy<",          OPT_FLT,  &max_energy             },
      { "rip>",             OPT_FLT,  &min_rip_expire         },
      { "rip<",             OPT_FLT,  &max_rip_expire         },
      { "rake>",            OPT_FLT,  &min_rake_expire        },
      { "rake<",            OPT_FLT,  &max_rake_expire        },
      { "mangle>",          OPT_FLT,  &min_mangle_expire      },
      { "mangle<",          OPT_FLT,  &max_mangle_expire      },
      { "savage_roar>",     OPT_FLT,  &min_savage_roar_expire },
      { "savage_roar<",     OPT_FLT,  &max_savage_roar_expire },
      { NULL }
    };
  std::vector<option_t> merged_options;
  attack_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

// druid_attack_t::cost ====================================================

double druid_attack_t::cost()
{
  druid_t* p = player -> cast_druid();
  double c = attack_t::cost();
  if ( c == 0 ) return 0;
  if ( p -> _buffs.omen_of_clarity ) return 0;
  if ( p -> _buffs.berserk ) c *= 0.5;
  return c;
}

// druid_attack_t::consume_resource ========================================

void druid_attack_t::consume_resource()
{
  druid_t* p = player -> cast_druid();
  attack_t::consume_resource();
  if ( p -> _buffs.omen_of_clarity )
  {
    // Treat the savings like a energy gain.
    double amount = attack_t::cost();
    if ( amount > 0 )
    {
      p -> gains_omen_of_clarity -> add( amount, 0 );
      p -> _buffs.omen_of_clarity = 0;
      p -> aura_loss( "Omen of Clarity" );
    }
  }
}

// druid_attack_t::execute =================================================

void druid_attack_t::execute()
{
  druid_t* p = player -> cast_druid();

  attack_t::execute();

  if ( result_is_hit() )
  {
    if ( requires_combo_points ) clear_combo_points( p );
    if (     adds_combo_points )   add_combo_point ( p );

    if ( result == RESULT_CRIT )
    {
      trigger_primal_fury( this );
    }
  }
  else
  {
    trigger_energy_refund( this );
    trigger_primal_precision( this );
  }

  break_stealth( p );
}

// druid_attack_t::calculate_direct_damage =================================

double druid_attack_t::calculate_direct_damage()
{
  druid_t* p = player -> cast_druid();
  base_dd_adder = p -> _buffs.tigers_fury;
  return attack_t::calculate_direct_damage();
}

// druid_attack_t::player_buff =============================================

void druid_attack_t::player_buff()
{
  druid_t* p = player -> cast_druid();

  attack_t::player_buff();

  if ( p -> _buffs.savage_roar )
  {
    // sr glyph seems to be additive
    player_multiplier *= 1.3 + 0.03 * p -> glyphs.savage_roar;
  }

  if ( p -> talents.naturalist )
  {
    player_multiplier *= 1 + p -> talents.naturalist * 0.02;
  }

  p -> uptimes_savage_roar -> update( p -> _buffs.savage_roar != 0 );
  p -> uptimes_rip -> update( p -> active_rip != 0 );
  p -> uptimes_rake -> update( p -> active_rake != 0 );
}

// druid_attack_t::ready ===================================================

bool druid_attack_t::ready()
{
  if ( ! attack_t::ready() )
    return false;

  druid_t*  p = player -> cast_druid();

  if ( requires_position != POSITION_NONE )
    if ( p -> position != requires_position )
      return false;

  if ( requires_stealth )
    if ( p -> _buffs.stealthed <= 0 )
      return false;

  if ( requires_combo_points && ( p -> _buffs.combo_points == 0 ) )
    return false;

  if ( berserk && ! p -> _buffs.berserk )
    return false;

  if ( min_combo_points > 0 )
    if ( p -> _buffs.combo_points < min_combo_points )
      return false;

  if ( max_combo_points > 0 )
    if ( p -> _buffs.combo_points > max_combo_points )
      return false;

  if ( min_energy > 0 )
    if ( p -> resource_current[ RESOURCE_ENERGY ] < min_energy )
      return false;

  if ( max_energy > 0 )
    if ( p -> resource_current[ RESOURCE_ENERGY ] > max_energy )
      return false;

  double ct = sim -> current_time;

  if ( min_mangle_expire > 0 )
    if ( ! p -> _expirations.mangle || ( ( p -> _expirations.mangle -> occurs() - ct ) < min_mangle_expire ) )
      return false;

  if ( max_mangle_expire > 0 )
    if ( p -> _expirations.mangle && ( ( p -> _expirations.mangle -> occurs() - ct ) > max_mangle_expire ) )
      return false;

  if ( min_savage_roar_expire > 0 )
    if ( ! p -> _expirations.savage_roar || ( ( p -> _expirations.savage_roar -> occurs() - ct ) < min_savage_roar_expire ) )
      return false;

  if ( max_savage_roar_expire > 0 )
    if ( p -> _expirations.savage_roar && ( ( p -> _expirations.savage_roar -> occurs() - ct ) > max_savage_roar_expire ) )
      return false;

  if ( min_rip_expire > 0 )
    if ( ! p -> active_rip || ( ( p -> active_rip -> duration_ready - ct ) < min_rip_expire ) )
      return false;

  if ( max_rip_expire > 0 )
    if ( p -> active_rip && ( ( p -> active_rip -> duration_ready - ct ) > max_rip_expire ) )
      return false;

  if ( min_rake_expire > 0 )
    if ( ! p -> active_rake || ( ( p -> active_rake -> duration_ready - ct ) < min_rake_expire ) )
      return false;

  if ( max_rake_expire > 0 )
    if ( p -> active_rake && ( ( p -> active_rake -> duration_ready - ct ) > max_rake_expire ) )
      return false;

  return true;
}

// druid_attack_t::tick ====================================================

void druid_attack_t::tick()
{
  attack_t::tick();
  // FIX ME! Get info on t8_2pc_feral and implement the procc, could be as easy as trigger_omen_of_clarity?
}

// Melee Attack ============================================================

struct melee_t : public druid_attack_t
{
  melee_t( const char* name, player_t* player ) :
      druid_attack_t( name, player, SCHOOL_PHYSICAL, TREE_NONE, /*special*/false )
  {
    base_dd_min = base_dd_max = 1;
    background  = true;
    repeating   = true;
    trigger_gcd = 0;
    base_cost   = 0;
    may_crit    = true;
  }

  virtual void execute()
  {
    druid_attack_t::execute();
    if ( result_is_hit() )
    {
      trigger_omen_of_clarity( this );
    }
  }
};

// Auto Attack =============================================================

struct auto_attack_t : public druid_attack_t
{
  auto_attack_t( player_t* player, const std::string& options_str ) :
      druid_attack_t( "auto_attack", player )
  {
    druid_t* p = player -> cast_druid();
    p -> melee_attack = new melee_t( "melee", player );
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    p -> melee_attack -> weapon = &( p -> main_hand_weapon );
    p -> melee_attack -> base_execute_time = p -> main_hand_weapon.swing_time;
    p -> melee_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();
    return( p -> melee_attack -> execute_event == 0 ); // not swinging
  }
};

// Claw ====================================================================

struct claw_t : public druid_attack_t
{
  claw_t( player_t* player, const std::string& options_str ) :
      druid_attack_t( "claw", player, SCHOOL_PHYSICAL, TREE_FERAL )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
      {
        { NULL }
      };
    parse_options( options, options_str );

    static rank_t ranks[] =
      {
        { 79, 8, 370, 370, 0, 45 },
        { 73, 7, 300, 300, 0, 45 },
        { 67, 6, 190, 190, 0, 45 },
        { 58, 7, 115, 115, 0, 45 },
        { 0, 0 }
      };
    init_rank( ranks, 48570 );

    weapon = &( p -> main_hand_weapon );
    adds_combo_points = true;
    may_crit          = true;
    base_multiplier  *= 1.0 + p -> talents.savage_fury * 0.1;
  }
};

// Faerie Fire (Feral) ======================================================

struct faerie_fire_feral_t : public druid_attack_t
{
  int debuff_only;

  faerie_fire_feral_t( player_t* player, const std::string& options_str ) :
      druid_attack_t( "faerie_fire_feral", player, SCHOOL_PHYSICAL, TREE_FERAL ), debuff_only( 0 )
  {
    option_t options[] =
      {
        { "debuff_only", OPT_BOOL, &debuff_only },
        { NULL }
      };
    parse_options( options, options_str );

    base_dd_min = base_dd_max = 1;
    direct_power_mod = 0.05;
    cooldown = 6.0;
    may_crit = true;
    id = 16857;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    if ( p -> _buffs.cat_form )
    {
      // The damage component is only active in (Dire) Bear Form
      direct_power_mod = 0;
      base_dd_min = base_dd_max = 0;
    }
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    if ( p -> _buffs.bear_form ) druid_attack_t::execute();
    trigger_faerie_fire( this );
    update_ready();
  }

  virtual bool ready()
  {
    if ( ! druid_attack_t::ready() )
      return false;

    if ( debuff_only && sim -> target -> debuffs.faerie_fire )
      return false;

    return true;
  }
};

// Mangle (Cat) ============================================================

struct mangle_cat_t : public druid_attack_t
{
  mangle_cat_t( player_t* player, const std::string& options_str ) :
      druid_attack_t( "mangle_cat", player, SCHOOL_PHYSICAL, TREE_FERAL )
  {
    druid_t* p = player -> cast_druid();
    assert( p -> talents.mangle );

    // By default, do not overwrite Mangle
    max_mangle_expire = 0.001;

    option_t options[] =
      {
        { NULL }
      };
    parse_options( options, options_str );

    static rank_t ranks[] =
      {
        { 80, 5, 634, 634, 0, 45 },
        { 75, 4, 536, 536, 0, 45 },
        { 68, 3, 330, 330, 0, 45 },
        { 58, 2, 256, 256, 0, 45 },
        { 0, 0 }
      };
    init_rank( ranks, 48566 );

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier *= 2.0;

    adds_combo_points = true;
    may_crit          = true;

    base_cost -= p -> talents.ferocity;
    base_cost -= p -> talents.improved_mangle * 2;

    base_multiplier  *= 1.0 + p -> talents.savage_fury * 0.1;
  }

  virtual void execute()
  {
    druid_attack_t::execute();
    if ( result_is_hit() )
    {
      trigger_mangle( this );
      if ( player -> cast_druid() -> idols.corruptor )
      {
        trigger_idol_of_the_corruptor( this );
      }
    }
  }
};

// Rake ====================================================================

struct rake_t : public druid_attack_t
{
  rake_t( player_t* player, const std::string& options_str ) :
      druid_attack_t( "rake", player, SCHOOL_BLEED, TREE_FERAL )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
      {
        { NULL }
      };
    parse_options( options, options_str );

    static rank_t ranks[] =
      {
        { 78, 7, 190, 190, 387, 40 },
        { 72, 6, 150, 150, 321, 40 },
        { 64, 5,  90,  90, 138, 40 },
        { 54, 4,  64,  90,  99, 40 },
        { 0, 0 }
      };
    init_rank( ranks, 48574 );

    adds_combo_points = true;
    may_crit          = true;
    base_tick_time    = 3.0;
    num_ticks         = 3;
    direct_power_mod  = 0.01;
    tick_power_mod    = 0.06;
    base_cost        -= p -> talents.ferocity;
    base_multiplier  *= 1.0 + p -> talents.savage_fury * 0.1;

    observer = &( p -> active_rake );
  }
  virtual void tick()
  {
    druid_attack_t::tick();
    trigger_t8_2pc_feral( this );
  }
};

// Rip ======================================================================

struct rip_t : public druid_attack_t
{
  double* combo_point_dmg;

  rip_t( player_t* player, const std::string& options_str ) :
      druid_attack_t( "rip", player, SCHOOL_BLEED, TREE_FERAL )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
      {
        { NULL }
      };
    parse_options( options, options_str );

    id = 49800;

    may_crit              = false;
    requires_combo_points = true;
    base_cost             = 30;
    base_tick_time        = 2.0;

    num_ticks = 6 + ( p -> glyphs.rip ? 2 : 0 ) + ( p -> tiers.t7_2pc_feral ? 2 : 0 );

    static double dmg_80[] = { 39+99*1, 39+99*2, 39+99*3, 39+99*4, 39+99*5 };
    static double dmg_71[] = { 32+72*1, 32+72*2, 32+72*3, 32+72*4, 32+72*5 };
    static double dmg_67[] = { 24+48*1, 24+48*2, 24+48*3, 24+48*4, 24+48*5 };
    static double dmg_60[] = { 17+28*1, 17+28*2, 17+28*3, 17+28*4, 17+28*5 };

    combo_point_dmg = ( p -> level >= 80 ? dmg_80 :
                        p -> level >= 71 ? dmg_71 :
                        p -> level >= 67 ? dmg_67 :
                        dmg_60 );

    tick_may_crit = ( p -> talents.primal_gore != 0 );

    observer = &( p -> active_rip );
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    added_ticks = 0;
    num_ticks = 6 + ( p -> glyphs.rip ? 2 : 0 ) + ( p -> tiers.t7_2pc_feral ? 2 : 0 );
    druid_attack_t::execute();
  }

  virtual void tick()
  {
    druid_attack_t::tick();
    trigger_t8_2pc_feral( this );
  }

  virtual void player_buff()
  {
    druid_t* p = player -> cast_druid();
    tick_power_mod = p -> _buffs.combo_points * 0.01;
    base_td_init   = p -> combo_point_rank( combo_point_dmg );
    if ( p -> idols.worship )
      base_td_init += p -> _buffs.combo_points * 21;
    druid_attack_t::player_buff();
  }
};

// Savage Roar =============================================================

struct savage_roar_t : public druid_attack_t
{
  savage_roar_t( player_t* player, const std::string& options_str ) :
      druid_attack_t( "savage_roar", player, SCHOOL_PHYSICAL, TREE_FERAL )
  {
    //druid_t* p = player -> cast_druid();

    option_t options[] =
      {
        { NULL }
      };
    parse_options( options, options_str );
    requires_combo_points = true;
    base_cost = 25;
    id = 52610;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, druid_t* p, double duration ): event_t( sim, p )
      {
        p -> aura_gain( "Savage Roar" );
        p -> _buffs.savage_roar = 1;
        sim -> add_event( this, duration );
      }
      virtual void execute()
      {
        druid_t* p = player -> cast_druid();
        p -> aura_loss( "Savage Roar" );
        p -> _buffs.savage_roar = 0;
        p -> _expirations.savage_roar = 0;
      }
    };

    druid_t* p = player -> cast_druid();

    double duration = 9.0 + p -> _buffs.combo_points * 5.0;
    if ( p -> tiers.t8_4pc_feral ) duration += 8.0;

    clear_combo_points( p );

    event_t*& e = p -> _expirations.savage_roar;

    if ( e )
    {
      e -> reschedule( duration );
    }
    else
    {
      e = new ( sim ) expiration_t( sim, p, duration );
    }
  }
};

// Shred ===================================================================

struct shred_t : public druid_attack_t
{
  int omen_of_clarity;

  shred_t( player_t* player, const std::string& options_str ) :
      druid_attack_t( "shred", player, SCHOOL_PHYSICAL, TREE_FERAL ), omen_of_clarity( 0 )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
      {
        { "omen_of_clarity", OPT_BOOL, &omen_of_clarity },
        { NULL }
      };
    parse_options( options, options_str );

    static rank_t ranks[] =
      {
        { 80, 9, 742, 742, 0, 60 },
        { 75, 8, 630, 630, 0, 60 },
        { 70, 7, 405, 405, 0, 60 },
        { 61, 6, 236, 236, 0, 60 },
        { 54, 5, 180, 180, 0, 60 },
        { 0, 0 }
      };
    init_rank( ranks, 48572 );

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier *= 2.25;
    requires_position  = POSITION_BACK;
    adds_combo_points  = true;
    may_crit           = true;
    base_cost         -= 9 * p -> talents.shredding_attacks;

    if ( p -> idols.ravenous_beast )
    {
      base_dd_min += 203;
      base_dd_max += 203;
    }
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    druid_attack_t::execute();
    if ( p -> glyphs.shred &&
         p -> active_rip  &&
         p -> active_rip -> added_ticks < 3 )
    {
      p -> active_rip -> extend_duration( 1 );
    }
  }

  virtual void player_buff()
  {
    druid_t*  p = player -> cast_druid();
    target_t* t = sim -> target;

    druid_attack_t::player_buff();

    if ( t -> debuffs.mangle || t -> debuffs.trauma ) player_multiplier *= 1.30;

    if ( t -> debuffs.bleeding )
    {
      player_multiplier *= 1 + 0.04 * p -> talents.rend_and_tear;
    }

  }

  virtual bool ready()
  {
    if ( ! druid_attack_t::ready() )
      return false;

    druid_t* p = player -> cast_druid();

    if ( omen_of_clarity && ! p -> _buffs.omen_of_clarity )
      return false;

    return true;
  }
};

// Berserk =================================================================

struct berserk_t : public druid_attack_t
{
  int tigers_fury;

  berserk_t( player_t* player, const std::string& options_str ) :
      druid_attack_t( "berserk", player )
  {
    druid_t* p = player -> cast_druid();
    assert( p -> talents.berserk );

    option_t options[] =
      {
        { "tigers_fury", OPT_BOOL, &tigers_fury },
        { NULL }
      };

    parse_options( options, options_str );

    cooldown = 180;
    id = 50334;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, druid_t* p ) : event_t( sim, p )
      {
        name = "Berserk Expiration";
        p -> aura_gain( "Berserk" );
        p -> _buffs.berserk = 1;
        sim -> add_event( this, 15.0 + ( p -> glyphs.berserk ? 5.0 : 0.0 ) );
      }
      virtual void execute()
      {
        druid_t* p = player -> cast_druid();
        p -> aura_loss( "Berserk" );
        p -> _buffs.berserk = 0;
        p -> _expirations.berserk = 0;
      }
    };

    druid_t* p = player -> cast_druid();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    p -> _expirations.berserk = new ( sim ) expiration_t( sim, p );
  }

  virtual bool ready()
  {
    if ( ! druid_attack_t::ready() )
      return false;

    druid_t* p = player -> cast_druid();

    if ( tigers_fury && ! p -> _buffs.tigers_fury )
      return false;

    return true;
  }
};

// Tigers Fury =============================================================

struct tigers_fury_t : public druid_attack_t
{
  tigers_fury_t( player_t* player, const std::string& options_str ) :
      druid_attack_t( "tigers_fury", player, SCHOOL_PHYSICAL, TREE_FERAL )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
      {
        { NULL }
      };
    parse_options( options, options_str );

    cooldown    = 30.0 - 3.0 * p -> tiers.t7_4pc_feral;
    trigger_gcd = 0;
    id = 50213;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    druid_t* p = player -> cast_druid();

    if ( p -> talents.king_of_the_jungle )
    {
      p -> resource_gain( RESOURCE_ENERGY, p -> talents.king_of_the_jungle * 20, p -> gains_tigers_fury );
    }
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* player ): event_t( sim, player )
      {
        name = "Tigers Fury Expiration";
        druid_t* p = player -> cast_druid();
        p -> aura_gain( "Tigers Fury" );
        p -> _buffs.tigers_fury = util_t::ability_rank( p -> level,  80.0,79, 60.0,71,  40.0,0 );
        sim -> add_event( this, 6.0 );
      }

      virtual void execute()
      {
        druid_t* p = player -> cast_druid();
        p -> aura_loss( "Tigers Fury" );
        p -> _buffs.tigers_fury = 0;

      }
    };

    new ( sim ) expiration_t( sim, player );

    update_ready();
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();

    if ( p -> _buffs.berserk )
      return false;

    return druid_attack_t::ready();
  }
};

// Ferocious Bite ============================================================

struct ferocious_bite_t : public druid_attack_t
{
  struct double_pair { double min, max; };
  double excess_engery_mod, excess_energy;
  double_pair* combo_point_dmg;

  ferocious_bite_t( player_t* player, const std::string& options_str ) :
      druid_attack_t( "ferocious_bite", player, SCHOOL_PHYSICAL, TREE_FERAL )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
      {
        { NULL }
      };
    parse_options( options, options_str );

    id = 48577;

    requires_combo_points = true;
    may_crit  = true;
    base_cost = 35;

    base_multiplier *= 1.0 + ( p -> talents.feral_aggression * 0.03 );

    static double_pair dmg_78[] = { { 410, 550 }, { 700, 840 }, { 990, 1130 }, { 1280, 1420 }, { 1570, 1710 } };
    static double_pair dmg_72[] = { { 334, 682 }, { 570, 682 }, { 806,  918 }, { 1042, 1154 }, { 1278, 1390 } };
    static double_pair dmg_63[] = { { 226, 292 }, { 395, 461 }, { 564,  630 }, {  733,  799 }, {  902,  968 } };
    static double_pair dmg_60[] = { { 199, 259 }, { 346, 406 }, { 493,  533 }, {  640,  700 }, {  787,  847 } };

    combo_point_dmg   = ( p -> level >= 78 ? dmg_78 :
                          p -> level >= 72 ? dmg_72 :
                          p -> level >= 63 ? dmg_63 :
                          dmg_60 );
    excess_engery_mod = ( p -> level >= 78 ? 9.4 :
                          p -> level >= 72 ? 7.7 :
                          p -> level >= 63 ? 3.4 :
                          2.1 );  // Up to 30 additional energy is converted into damage
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();

    base_dd_min = combo_point_dmg[ p -> _buffs.combo_points - 1 ].min;
    base_dd_max = combo_point_dmg[ p -> _buffs.combo_points - 1 ].max;

    direct_power_mod = 0.07 * p -> _buffs.combo_points;

    excess_energy = ( p -> resource_current[ RESOURCE_ENERGY ] - druid_attack_t::cost() );

    if ( excess_energy > 0 )
    {
      // There will be energy left after the Ferocious Bite of which up to 30 will also be converted into damage.
      // Additional damage AND additinal scaling from AP.
      // druid_attack_t::cost() takes care of OoC handling.

      excess_energy     = ( excess_energy > 30 ? 30 : excess_energy );
      direct_power_mod += excess_energy / 410.0;
      base_dd_adder     = excess_engery_mod * excess_energy;
    }
    else
    {
      excess_energy = 0;
      base_dd_adder = 0;
    }

    druid_attack_t::execute();
    if ( excess_energy > 0 )
    {
      direct_power_mod -= excess_energy / 410.0;
    }
  }

  virtual void consume_resource()
  {
    // Ferocious Bite consumes 35+x energy, with 0 <= x <= 30.
    // Consumes the base_cost and handles Omen of Clarity
    druid_attack_t::consume_resource();

    if ( result_is_hit() )
    {
      // Let the additional energy consumption create it's own debug log entries.
      if ( sim -> debug )
        log_t::output( sim, "%s consumes an additional %.1f %s for %s", player -> name(),
                       excess_energy, util_t::resource_type_string( resource ), name() );

      player -> resource_loss( resource, excess_energy );
      stats -> consume_resource( excess_energy );
    }

  }

  virtual void player_buff()
  {
    druid_t* p = player -> cast_druid();

    druid_attack_t::player_buff();

    if ( sim -> target -> debuffs.bleeding > 0 )
    {
      player_crit += 0.05 * p -> talents.rend_and_tear;
    }

  }
};


// ==========================================================================
// Druid Spell
// ==========================================================================

struct druid_spell_t : public spell_t
{
  int skip_on_eclipse;
  druid_spell_t( const char* n, player_t* p, int s, int t ) :
      spell_t( n, p, RESOURCE_MANA, s, t ), skip_on_eclipse( 0 ) {}
  virtual void   consume_resource();
  virtual double cost();
  virtual void   execute();
  virtual double execute_time();
  virtual double haste();
  virtual void   parse_options( option_t*, const std::string& options_str );
  virtual void   player_buff();
  virtual bool   ready();
  virtual void   schedule_execute();
  virtual void   target_debuff( int dmg_type );

};

// druid_spell_t::parse_options ============================================

void druid_spell_t::parse_options( option_t*          options,
                                   const std::string& options_str )
{
  option_t base_options[] =
    {
      { "skip_on_eclipse",  OPT_INT, &skip_on_eclipse        },
      { NULL }
    };
  std::vector<option_t> merged_options;
  spell_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

// druid_spell_t::ready ====================================================

bool druid_spell_t::ready()
{
  if ( ! spell_t::ready() )
    return false;

  druid_t*  p = player -> cast_druid();

  if ( skip_on_eclipse > 0 )
    if ( p -> _buffs.eclipse_starfire || p -> _buffs.eclipse_wrath )
      return false;

  return true;
}

// druid_spell_t::cost =====================================================

double druid_spell_t::cost()
{
  druid_t* p = player -> cast_druid();
  if ( p -> _buffs.omen_of_clarity ) return 0;
  return spell_t::cost();
}

// druid_spell_t::haste ====================================================

double druid_spell_t::haste()
{
  druid_t* p = player -> cast_druid();
  double h = spell_t::haste();
  if ( p -> talents.celestial_focus ) h *= 1.0 / ( 1.0 + p -> talents.celestial_focus * 0.01 );
  if ( p -> _buffs.natures_grace )
  {
    h *= 1.0 / ( 1.0 + p -> _buffs.natures_grace );
  }
  return h;
}

// druid_spell_t::execute_time =============================================

double druid_spell_t::execute_time()
{
  druid_t* p = player -> cast_druid();
  if ( p -> _buffs.natures_swiftness ) return 0;
  return spell_t::execute_time();
}

// druid_spell_t::schedule_execute =========================================

void druid_spell_t::schedule_execute()
{
  druid_t* p = player -> cast_druid();

  if ( druid_spell_t::execute_time() > 0 )
    p -> uptimes_natures_grace -> update( p -> _buffs.natures_grace != 0 );

  spell_t::schedule_execute();

  if ( base_execute_time > 0 )
  {
    if ( p -> _buffs.natures_swiftness )
    {
      p -> _buffs.natures_swiftness = 0;
    }
  }
}

// druid_spell_t::execute ==================================================

void druid_spell_t::execute()
{
  druid_t* p = player -> cast_druid();

  spell_t::execute();

  if ( result == RESULT_CRIT )
  {
    if ( p -> _buffs.moonkin_form && ! aoe )
    {
      p -> resource_gain( RESOURCE_MANA, p -> resource_max[ RESOURCE_MANA ] * 0.02, p -> gains_moonkin_form );
    }

    trigger_natures_grace( this );
  }

  if ( ! aoe )
  {
    trigger_omen_of_clarity( this );
  }

  if ( p -> tiers.t4_2pc_balance &&
       p -> rngs.tier4_2pc -> roll( 0.05 ) )
  {
    p -> resource_gain( RESOURCE_MANA, 120.0, p -> gains.tier4_2pc );
  }

}

// druid_spell_t::consume_resource =========================================

void druid_spell_t::consume_resource()
{
  druid_t* p = player -> cast_druid();
  spell_t::consume_resource();
  if ( p -> _buffs.omen_of_clarity )
  {
    // Treat the savings like a mana gain.
    double amount = spell_t::cost();
    if ( amount > 0 )
    {
      p -> gains_omen_of_clarity -> add( amount );
      p -> _buffs.omen_of_clarity = 0;
    }
  }
}

// druid_spell_t::player_buff ==============================================

void druid_spell_t::player_buff()
{
  druid_t* p = player -> cast_druid();
  spell_t::player_buff();
  if ( p -> _buffs.moonkin_form )
  {
    player_multiplier *= 1.0 + p -> talents.master_shapeshifter * 0.02;
  }
  player_multiplier *= 1.0 + p -> talents.earth_and_moon * 0.01;
}

// druid_spell_t::target_debuff ============================================

void druid_spell_t::target_debuff( int dmg_type )
{
  druid_t* p = player -> cast_druid();
  spell_t::target_debuff( dmg_type );
  target_t* t = sim -> target;
  if ( t -> debuffs.faerie_fire )
  {
    target_crit += p -> talents.improved_faerie_fire * 0.01;
  }
}

// Faerie Fire Spell =======================================================

struct faerie_fire_t : public druid_spell_t
{
  faerie_fire_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "faerie_fire", player, SCHOOL_NATURE, TREE_BALANCE )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
      {
        { NULL }
      };
    parse_options( options, options_str );

    base_cost = p -> resource_base[ RESOURCE_MANA ] * 0.07;
    id = 770;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    trigger_faerie_fire( this );
    trigger_improved_faerie_fire( this );
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();

    if ( ! druid_spell_t::ready() )
      return false;

    if ( ! sim -> target -> debuffs.faerie_fire )
      return true;

    if ( ! sim -> target -> debuffs.improved_faerie_fire && p -> talents.improved_faerie_fire )
      return true;

    return false;
  }
};

// Innervate Spell =========================================================

struct innervate_t : public druid_spell_t
{
  int trigger;
  player_t* innervate_target;

  innervate_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "innervate", player, SCHOOL_NATURE, TREE_BALANCE ), trigger( 0 )
  {
    druid_t* p = player -> cast_druid();

    std::string target_str;
    option_t options[] =
      {
        { "trigger", OPT_INT,    &trigger    },
        { "target",  OPT_STRING, &target_str },
        { NULL }
      };
    parse_options( options, options_str );

    id = 29166;

    base_cost = 0.0;
    base_execute_time = 0;
    cooldown  = 480;
    harmful   = false;
    if ( p -> tiers.t4_4pc_balance ) cooldown -= 48.0;

    // If no target is set, assume we have innervate for ourself
    innervate_target = target_str.empty() ? p : sim -> find_player( target_str );
    assert ( innervate_target != 0 );
  }
  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(),name() );
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
      {
        sim -> add_event( this, 20.0 );
      }
      virtual void execute()
      {
        player -> buffs.innervate = 0;
        player -> aura_loss( "Innervate", 29166 );
      }
    };
    struct expiration_glyph_t : public event_t
    {
      expiration_glyph_t( sim_t* sim, player_t* p ) : event_t( sim, p )
      {
        sim -> add_event( this, 20.0 );
      }
      virtual void execute()
      {
        player -> buffs.glyph_of_innervate = 0;
        player -> aura_loss( "Glyph of Innervate" );
      }
    };

    consume_resource();
    update_ready();

    // In 3.1.2 Innervate is changed to 450% of base mana of the caster
    // Per second: player -> resource_base[ RESOURCE_MANA ]* 4.5 / 20.0
    // ~
    // Glyph of Innervate: 90%
    // Per second: player -> resource_base[ RESOURCE_MANA ]* 0.9 / 20.0
    // In ::regen() we then just give the player:
    // buffs.innervate * periodicity mana

    innervate_target -> buffs.innervate = player -> resource_base[ RESOURCE_MANA ]* 4.5 / 20.0;
    innervate_target -> aura_gain( "Innervate", 29166 );

    if ( player -> cast_druid() -> glyphs.innervate )
    {
      player -> buffs.glyph_of_innervate = player -> resource_base[ RESOURCE_MANA ]* 0.9 / 20.0;
      player -> aura_gain( "Glyph of Innervate" );
      new ( sim ) expiration_glyph_t( sim, player );
    }
    new ( sim ) expiration_t( sim, innervate_target );
  }

  virtual bool ready()
  {
    if ( ! druid_spell_t::ready() )
      return false;

    if ( trigger < 0 )
      return ( player -> resource_current[ RESOURCE_MANA ] + trigger ) < 0;

    return ( player -> resource_max    [ RESOURCE_MANA ] -
             player -> resource_current[ RESOURCE_MANA ] ) > trigger;
  }
};

// Insect Swarm Spell ======================================================

struct insect_swarm_t : public druid_spell_t
{
  double min_eclipse_left;
  int wrath_ready;
  action_t* active_wrath;

  insect_swarm_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "insect_swarm", player, SCHOOL_NATURE, TREE_BALANCE ), wrath_ready( 0 ), active_wrath( 0 )
  {
    druid_t* p = player -> cast_druid();
    assert( p -> talents.insect_swarm );

    option_t options[] =
      {
        { "wrath_ready",     OPT_BOOL, &wrath_ready      },
        { "eclipse_left>",   OPT_FLT,  &min_eclipse_left },
        { NULL }
      };
    parse_options( options, options_str );

    static rank_t ranks[] =
      {
        { 80, 7, 0, 0, 215, 0.08 },
        { 70, 6, 0, 0, 172, 175  },
        { 60, 5, 0, 0, 124, 155  },
        { 0, 0 }
      };
    init_rank( ranks, 48468 );

    base_execute_time = 0;
    base_tick_time    = 2.0;
    num_ticks         = 6;
    tick_power_mod    = 0.2;

    base_multiplier *= 1.0 + util_t::talent_rank( p -> talents.genesis, 5, 0.01 ) +
                       ( p -> glyphs.insect_swarm  ? 0.30 : 0.00 ) +
                       ( p -> tiers.t7_2pc_balance ? 0.10 : 0.00 );

    if ( p -> idols.crying_wind )
    {
      // Druid T8 Balance Relic -- Increases the spell power of your Insect Swarm by 374.
      base_spell_power += 374;
    }

    if ( p -> talents.natures_splendor ) num_ticks++;

    observer = &( p -> active_insect_swarm );
  }
  virtual void tick()
  {
    druid_spell_t::tick();
    druid_t* p = player -> cast_druid();
    if ( p -> tiers.t8_4pc_balance )
    {
      trigger_t8_4pc_balance( this );
    }
  }
  virtual bool ready()
  {
    if ( ! druid_spell_t::ready() )
      return false;

    if ( wrath_ready && ! active_wrath )
    {
      for ( active_wrath = next; active_wrath; active_wrath = active_wrath -> next )
        if ( active_wrath -> name_str == "wrath" )
          break;

      if ( ! active_wrath ) wrath_ready = 0;
    }

    if ( wrath_ready )
      if ( ! active_wrath -> ready() )
        return false;

    druid_t* p = player -> cast_druid();

    if ( skip_on_eclipse < 0 )
      if ( p -> _buffs.eclipse_starfire )
        return false;

    // p  -> _buffs.eclipse_wrath: the time eclipse proced, but 0 if eclipse is not up.
    if ( min_eclipse_left > 0 && p  -> _buffs.eclipse_wrath )
      //                     ( 15   -   time elapsed on eclipse  ) = time left on eclipse buff
      if ( min_eclipse_left < ( 15.0 - ( sim -> current_time - p -> _buffs.eclipse_wrath ) ) )
        return false;

    return true;
  }
};

// Moonfire Spell ===========================================================

struct moonfire_t : public druid_spell_t
{
  double min_eclipse_left;
  moonfire_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "moonfire", player, SCHOOL_ARCANE, TREE_BALANCE )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
      {
        { "eclipse_left>",   OPT_FLT, &min_eclipse_left },
        { NULL }
      };
    parse_options( options, options_str );

    static rank_t ranks[] =
      {
        { 80, 14, 406, 476, 200, 0.21 },
        { 75, 13, 347, 407, 171, 0.21 },
        { 70, 12, 305, 357, 150, 495  },
        { 64, 11, 220, 220, 111, 430  },
        { 58, 10, 189, 221,  96, 375  },
        { 0, 0 }
      };
    init_rank( ranks, 48463 );

    base_execute_time = 0;
    base_tick_time    = 3.0;
    num_ticks         = 4;
    direct_power_mod  = 0.15;
    tick_power_mod    = 0.13;
    may_crit          = true;

    base_cost *= 1.0 - util_t::talent_rank( p -> talents.moonglow,    3, 0.03 );
    base_crit += util_t::talent_rank( p -> talents.improved_moonfire, 2, 0.05 );

    base_crit_bonus_multiplier *= 1.0 + util_t::talent_rank( p -> talents.vengeance, 5, 0.20 );

    double multiplier_td = ( util_t::talent_rank( p -> talents.moonfury,          3, 0.03, 0.06, 0.10 ) +
                             util_t::talent_rank( p -> talents.improved_moonfire, 2, 0.05 ) +
                             util_t::talent_rank( p -> talents.genesis,           5, 0.01 ) );

    double multiplier_dd = ( util_t::talent_rank( p -> talents.moonfury,          3, 0.03, 0.06, 0.10 ) +
                             util_t::talent_rank( p -> talents.improved_moonfire, 2, 0.05 ) );

    if ( p -> glyphs.moonfire )
    {
      multiplier_dd -= 0.90;
      multiplier_td += 0.75;
    }
    base_dd_multiplier *= 1.0 + multiplier_dd;
    base_td_multiplier *= 1.0 + multiplier_td;

    observer = &( p -> active_moonfire );
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    num_ticks = 4;
    added_ticks = 0;
    if ( p -> talents.natures_splendor ) num_ticks++;
    if ( p -> tiers.t6_2pc_balance     ) num_ticks++;
    druid_spell_t::execute();
    if ( p -> idols.unseen_moon )
      trigger_unseen_moon( this );
  }

  virtual bool ready()
  {
    if ( ! druid_spell_t::ready() )
      return false;

    druid_t* p = player -> cast_druid();

    if ( skip_on_eclipse < 0 )
      if ( p -> _buffs.eclipse_wrath )
        return false;

    // p  -> _buffs.eclipse_starfire the time eclipse proced, but 0 if eclipse is not up.
    if ( min_eclipse_left > 0 && p  -> _buffs.eclipse_starfire )
      //                     ( 15   -   time elapsed on eclipse  ) = time left on eclipse buff
      if ( min_eclipse_left < ( 15.0 - ( sim -> current_time - p -> _buffs.eclipse_starfire ) ) )
        return false;

    return true;
  }
};

// Cat Form Spell =========================================================

struct cat_form_t : public druid_spell_t
{
  cat_form_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "cat_form", player, SCHOOL_NATURE, TREE_FERAL )
  {
    trigger_gcd = 0;
    base_execute_time = 0;
    base_cost = 0;
    id = 768;
  }

  virtual void execute()
  {
    druid_t* d = player -> cast_druid();

    if ( sim -> log ) log_t::output( sim, "%s performs %s", d -> name(),name() );

    weapon_t* w = &( d -> main_hand_weapon );

    if ( w -> type != WEAPON_BEAST )
    {
      w -> type = WEAPON_BEAST;
      w -> school = SCHOOL_PHYSICAL;
      w -> damage = 54.8;
      w -> swing_time = 1.0;
      if ( d -> melee_attack )
        d -> melee_attack -> cancel(); // Force melee swing to restart if necessary
    }

    d -> _buffs.cat_form = 1;
    d -> base_gcd = 1.0;
    d -> reset_gcd();

    if ( d -> talents.leader_of_the_pack )
    {
      sim -> auras.leader_of_the_pack = 1;

      for ( player_t* p = sim -> player_list; p; p = p -> next )
      {
        if ( ! p -> sleeping ) p -> aura_gain( "Leader of the Pack" );
      }
    }
  }

  virtual bool ready()
  {
    druid_t* d = player -> cast_druid();
    return( d -> _buffs.cat_form == 0 );
  }
};

// Moonkin Form Spell =====================================================

struct moonkin_form_t : public druid_spell_t
{
  moonkin_form_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "moonkin_form", player, SCHOOL_NATURE, TREE_BALANCE )
  {
    druid_t* p = player -> cast_druid();
    assert( p -> talents.moonkin_form );
    trigger_gcd = 0;
    base_execute_time = 0;
    base_cost = 0;
    id = 24858;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();

    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );

    p -> _buffs.moonkin_form = 1;

    sim -> auras.moonkin = 1;

    if ( p -> talents.improved_moonkin_form )
    {
      sim -> auras.improved_moonkin = 1;
    }

    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if ( ! p -> sleeping ) p -> aura_gain( "Moonkin Aura" );
    }
  }

  virtual bool ready()
  {
    druid_t* d = player -> cast_druid();
    return( d -> _buffs.moonkin_form == 0 );
  }
};

// Natures Swiftness Spell ==================================================

struct druids_swiftness_t : public druid_spell_t
{
  druids_swiftness_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "natures_swiftness", player, SCHOOL_NATURE, TREE_RESTORATION )
  {
    druid_t* p = player -> cast_druid();
    assert( p -> talents.natures_swiftness );
    trigger_gcd = 0;
    cooldown = 180.0;
    if ( ! options_str.empty() )
    {
      // This will prevent Natures Swiftness from being called before the desired "free spell" is ready to be cast.
      cooldown_group = options_str;
      duration_group = options_str;
    }
    id = 17116;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    druid_t* p = player -> cast_druid();
    p -> aura_gain( "Natures Swiftness" );
    p -> _buffs.natures_swiftness = 1;
    cooldown_ready = sim -> current_time + cooldown;
  }
};

// Starfire Spell ===========================================================

struct starfire_t : public druid_spell_t
{
  int eclipse_benefit;
  int eclipse_trigger;
  std::string prev_str;
  int extend_moonfire;
  int instant;

  starfire_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "starfire", player, SCHOOL_ARCANE, TREE_BALANCE ),
      eclipse_benefit( 0 ), eclipse_trigger( 0 ), extend_moonfire( 0 ), instant( 0 )
  {
    druid_t* p = player -> cast_druid();

    std::string eclipse_str;
    option_t options[] =
      {
        { "extendmf", OPT_BOOL,   &extend_moonfire },
        { "eclipse",  OPT_STRING, &eclipse_str     },
        { "prev",     OPT_STRING, &prev_str        },
        { "instant",  OPT_BOOL,   &instant         },
        { NULL }
      };
    parse_options( options, options_str );

    if ( ! eclipse_str.empty() )
    {
      eclipse_benefit = ( eclipse_str == "benefit" );
      eclipse_trigger = ( eclipse_str == "trigger" );
    }

    // Starfire is leanred at level 78, but gains 5 damage per level
    // so the actual damage range at 80 is: 1038 to 1222
    static rank_t ranks[] =
      {
        { 80, 11, 1038, 1222, 0, 0.16 },
        { 78, 10, 1028, 1212, 0, 0.16 },
        { 72,  9,  854, 1006, 0, 0.16 },
        { 67,  8,  818,  964, 0, 370  },
        { 60,  7,  693,  817, 0, 340  },
        { 0, 0 }
      };
    init_rank( ranks, 48465 );

    base_execute_time = 3.5;
    direct_power_mod  = ( base_execute_time / 3.5 );
    may_crit          = true;

    base_cost         *= 1.0 - util_t::talent_rank( p -> talents.moonglow, 3, 0.03 );
    base_execute_time -= util_t::talent_rank( p -> talents.starlight_wrath, 5, 0.1 );
    base_multiplier   *= 1.0 + util_t::talent_rank( p -> talents.moonfury, 3, 0.03, 0.06, 0.10 );
    base_crit         += util_t::talent_rank( p -> talents.natures_majesty, 2, 0.02 );
    direct_power_mod  += util_t::talent_rank( p -> talents.wrath_of_cenarius, 5, 0.04 );
    base_crit_bonus_multiplier *= 1.0 + util_t::talent_rank( p -> talents.vengeance, 5, 0.20 );

    if ( p -> idols.shooting_star )
    {
      // Equip: Increases the damage dealt by Starfire by 165.
      base_dd_min += 165;
      base_dd_max += 165;
    }
    if ( p -> tiers.t6_4pc_balance ) base_crit += 0.05;
    if ( p -> tiers.t7_4pc_balance ) base_crit += 0.05;
  }

  virtual void player_buff()
  {
    druid_spell_t::player_buff();
    druid_t* p = player -> cast_druid();
    p -> uptimes_eclipse_starfire -> update( p -> _buffs.eclipse_starfire != 0 );
    if ( p -> _buffs.eclipse_starfire )
    {
      player_crit += 0.30 + p -> tiers.t8_2pc_balance * 0.15;
    }
    if ( p -> active_moonfire )
    {
      player_crit += 0.01 * p -> talents.improved_insect_swarm;
    }
    if ( p -> tiers.t5_4pc_balance )
    {
      if ( p -> active_moonfire     ||
           p -> active_insect_swarm )
      {
        player_multiplier *= 1.10;
      }
    }
  }
  
  virtual void schedule_execute()
  {
    druid_spell_t::schedule_execute();
    if( sim -> P313 )
    {
      druid_t* p = player -> cast_druid();
      event_t::early( p -> _expirations.t8_4pc_balance );
    }
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    druid_spell_t::execute();
    
    if( ! sim -> P313 )
      event_t::early( p -> _expirations.t8_4pc_balance );

    if ( result_is_hit() )
    {
      trigger_ashtongue_talisman( this );
      trigger_earth_and_moon( this );
      if ( result == RESULT_CRIT )
      {
        trigger_eclipse_wrath( this );
      }

      if ( p -> glyphs.starfire && p -> active_moonfire )
      {
        if ( p -> active_moonfire -> added_ticks < 3 )
          p -> active_moonfire -> extend_duration( 1 );
      }
    }
  }
  virtual double execute_time()
  {
    druid_t* p = player -> cast_druid();
    if ( p -> _buffs.t8_4pc_balance )
      return 0;

    return druid_spell_t::execute_time();
  }
  virtual bool ready()
  {
    if ( ! druid_spell_t::ready() )
      return false;

    druid_t* p = player -> cast_druid();

    if ( instant && p -> _buffs.t8_4pc_balance == 0 )
      return false;

    if ( extend_moonfire && p -> glyphs.starfire )
      if ( p -> active_moonfire && p -> active_moonfire -> added_ticks > 2 )
        return false;

    if ( eclipse_benefit )
    {
      if ( ! sim -> time_to_think( p -> _buffs.eclipse_starfire ) )
        return false;

      // Don't cast starfire if eclipse will fade before the cast finished
      if ( p -> _expirations.eclipse )
        if ( execute_time() > p -> _expirations.eclipse -> occurs() - sim -> current_time )
          return false;
    }

    if ( eclipse_trigger )
    {
      if ( p -> talents.eclipse == 0 )
        return false;

      if ( sim -> current_time + 1.5 < p -> _cooldowns.eclipse )
      {
        // Did the player have enough time by now to realise he procced eclipse?
        // If so, return false as we only want to cast to procc
        if ( sim -> time_to_think( p -> _buffs.eclipse_wrath ) )
          return false;

        // This is for the time when eclipse buff faded, but it is still on cd.
        if ( ! p -> _buffs.eclipse_wrath )
          return false;
      }
    }

    if ( ! prev_str.empty() )
    {
      if ( ! p -> last_foreground_action )
        return false;

      if ( p -> last_foreground_action -> name_str != prev_str )
        return false;
    }

    return true;
  }
};

// Wrath Spell ==============================================================

struct wrath_t : public druid_spell_t
{
  int eclipse_benefit;
  int eclipse_trigger;
  std::string prev_str;

  wrath_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "wrath", player, SCHOOL_NATURE, TREE_BALANCE ), eclipse_benefit( 0 ), eclipse_trigger( 0 )
  {
    druid_t* p = player -> cast_druid();

    std::string eclipse_str;
    option_t options[] =
      {
        { "eclipse", OPT_STRING, &eclipse_str },
        { "prev",    OPT_STRING, &prev_str    },
        { NULL }
      };
    parse_options( options, options_str );

    if ( ! eclipse_str.empty() )
    {
      eclipse_benefit = ( eclipse_str == "benefit" );
      eclipse_trigger = ( eclipse_str == "trigger" );
    }

    // Wrath is leanred at level 79, but gains 4 damage per level
    // so the actual damage range at 80 is: 557 to 627
    static rank_t ranks[] =
      {
        { 80, 13, 557, 627, 0, 0.11 },
        { 79, 12, 553, 623, 0, 0.11 },
        { 74, 11, 504, 568, 0, 0.11 },
        { 69, 10, 431, 485, 0, 255  },
        { 61,  9, 397, 447, 0, 210  },
        { 0, 0 }
      };
    init_rank( ranks, 48461 );

    base_execute_time = 2.0;
    direct_power_mod  = ( base_execute_time / 3.5 );
    may_crit          = true;

    base_cost         *= 1.0 - util_t::talent_rank( p -> talents.moonglow, 3, 0.03 );
    base_execute_time -= util_t::talent_rank( p -> talents.starlight_wrath, 5, 0.1 );
    base_multiplier   *= 1.0 + util_t::talent_rank( p -> talents.moonfury, 3, 0.03, 0.06, 0.10 );
    base_crit         += util_t::talent_rank( p -> talents.natures_majesty, 2, 0.02 );
    direct_power_mod  += util_t::talent_rank( p -> talents.wrath_of_cenarius, 5, 0.02 );
    base_crit_bonus_multiplier *= 1.0 + util_t::talent_rank( p -> talents.vengeance, 5, 0.20 );

    if ( p -> tiers.t7_4pc_balance ) base_crit += 0.05;

    if ( p -> idols.steadfast_renewal )
    {
      // Equip: Increases the damage dealt by Wrath by 70.
      base_dd_min += 70;
      base_dd_max += 70;
    }
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    if ( result_is_hit() )
    {
      if ( result == RESULT_CRIT )
      {
        trigger_eclipse_starfire( this );
      }
      trigger_earth_and_moon( this );
    }
  }

  virtual void player_buff()
  {
    druid_spell_t::player_buff();
    druid_t* p = player -> cast_druid();
    p -> uptimes_eclipse_wrath -> update( p -> _buffs.eclipse_wrath != 0 );
    if ( p -> _buffs.eclipse_wrath )
    {
      player_multiplier *= 1.3 + p -> tiers.t8_2pc_balance * 0.15;
    }
    if ( p -> active_insect_swarm )
    {
      player_multiplier *= 1.0 + p -> talents.improved_insect_swarm * 0.01;
    }
  }

  virtual bool ready()
  {
    if ( ! druid_spell_t::ready() )
      return false;

    druid_t* p = player -> cast_druid();

    if ( eclipse_benefit )
    {
      if ( ! sim -> time_to_think( p -> _buffs.eclipse_wrath ) )
        return false;

      // Don't cast wrath if eclipse will fade before the cast finished.
      if ( p -> _expirations.eclipse )
        if ( execute_time() > p -> _expirations.eclipse -> occurs() - sim -> current_time )
          return false;
    }


    if ( eclipse_trigger )
    {
      if ( p -> talents.eclipse == 0 )
        return false;

      if ( sim -> current_time + 3.0 < p -> _cooldowns.eclipse )
      {
        // Did the player have enough time by now to realise he procced eclipse?
        // If so, return false as we only want to cast to procc
        if ( sim -> time_to_think( p -> _buffs.eclipse_starfire ) )
          return false;

        // This is for the time when eclipse buff faded, but it is still on cd.
        if ( ! p -> _buffs.eclipse_starfire )
          return false;
      }
    }

    if ( ! prev_str.empty() )
    {
      if ( ! p -> last_foreground_action )
        return false;

      if ( p -> last_foreground_action -> name_str != prev_str )
        return false;
    }

    return true;
  }
};

// Starfall Spell ===========================================================

struct starfall_t : public druid_spell_t
{
  spell_t* starfall_star;
  starfall_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "starfall", player, SCHOOL_ARCANE, TREE_BALANCE )
  {
    struct starfall_star_splash_t : public druid_spell_t
    {
      starfall_star_splash_t( player_t* player ) : druid_spell_t( "starfall", player, SCHOOL_ARCANE, TREE_BALANCE )
      {
        druid_t* p = player -> cast_druid();

        static rank_t ranks[] =
          {
            { 80, 4, 78, 78 },
            { 75, 3, 66, 66 },
            { 70, 2, 45, 45 },
            { 60, 1, 20, 20 },
            { 0, 0 }
          };
        init_rank( ranks );
        direct_power_mod  = 0.012;
        may_crit          = true;
        may_miss          = true;
        may_resist        = true;
        background        = true;
        aoe               = true; // Prevents procing Omen or Moonkin Form mana gains.
        dual = true;

        base_crit                  += util_t::talent_rank( p -> talents.natures_majesty, 2, 0.02 );
        base_crit_bonus_multiplier *= 1.0 + util_t::talent_rank( p -> talents.vengeance, 5, 0.20 );
        if ( p -> glyphs.focus )
          base_multiplier *= 1.2;
      }
      virtual void execute()
      {
        druid_spell_t::execute();
        tick_dmg = direct_dmg;
        update_stats( DMG_OVER_TIME );
      }

    };

    struct starfall_star_t : public druid_spell_t
    {
      action_t* starfall_star_splash;
      starfall_star_t( player_t* player ) : druid_spell_t( "starfall", player, SCHOOL_ARCANE, TREE_BALANCE )
      {
        druid_t* p = player -> cast_druid();

        direct_power_mod  = 0.05;
        may_crit          = true;
        may_miss          = true;
        may_resist        = true;
        background        = true;
        aoe               = true; // Prevents procing Omen or Moonkin Form mana gains.
        dual              = true;

        base_dd_min = base_dd_max  = 0;
        base_crit                  += util_t::talent_rank( p -> talents.natures_majesty, 2, 0.02 );
        base_crit_bonus_multiplier *= 1.0 + util_t::talent_rank( p -> talents.vengeance, 5, 0.20 );

        if ( p -> glyphs.focus )
          base_multiplier *= 1.2;

        starfall_star_splash = new starfall_star_splash_t( p );

        id = 53201;
      }

      virtual void execute()
      {
        druid_spell_t::execute();
        tick_dmg = direct_dmg;
        update_stats( DMG_OVER_TIME );

        if ( result_is_hit() )
        {
          // FIXME! Just an assumption that the splash damage only occurs if the star did not miss. (
          starfall_star_splash -> execute();
        }
      }
    };
    druid_t* p = player -> cast_druid();

    option_t options[] =
      {
        { NULL }
      };
    parse_options( options, options_str );

    num_ticks      = 10;
    base_tick_time = 1.0;

    cooldown          = 90;
    base_execute_time = 0;
    aoe               = true; // Can the actual CAST of Starfall trigger omen? FIX ME!

    static rank_t ranks[] =
      {
        { 80, 4, 433, 503, 0, 0.39 },
        { 75, 3, 366, 424, 0, 0.39 },
        { 70, 2, 250, 290, 0, 0.39 },
        { 60, 1, 111, 129, 0, 0.39 },
        { 0, 0 }
      };
    init_rank( ranks );


    starfall_star = new starfall_star_t( p );
    starfall_star -> base_dd_max = base_dd_max;
    starfall_star -> base_dd_min = base_dd_min;
    base_dd_min = base_dd_max = 0;

    if ( p -> glyphs.starfall )
      cooldown  -= 30;
  }
  virtual void tick()
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );
    starfall_star -> execute();
    update_time( DMG_OVER_TIME );
  }
};

// Mark of the Wild Spell =====================================================

struct mark_of_the_wild_t : public druid_spell_t
{
  double bonus;

  mark_of_the_wild_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "mark_of_the_wild", player, SCHOOL_NATURE, TREE_RESTORATION ), bonus( 0 )
  {
    druid_t* p = player -> cast_druid();

    trigger_gcd = 0;
    bonus  = util_t::ability_rank( player -> level,  37.0,80, 14.0,70,  12.0,0 );
    bonus *= 1.0 + p -> talents.improved_mark_of_the_wild * 0.20;
    id = 48469;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      p -> buffs.mark_of_the_wild = bonus;
      p -> init_resources( true );
    }
  }

  virtual bool ready()
  {
    return( player -> buffs.mark_of_the_wild < bonus );
  }
};

// Treants Spell ============================================================

struct treants_spell_t : public druid_spell_t
{
  int target_pct;

  treants_spell_t( player_t* player, const std::string& options_str ) :
      druid_spell_t( "treants", player, SCHOOL_NATURE, TREE_BALANCE ), target_pct( 0 )
  {
    druid_t* p = player -> cast_druid();
    assert( p -> talents.force_of_nature );

    option_t options[] =
      {
        { "target_pct", OPT_DEPRECATED, ( void* ) "health_percentage<" },
        { NULL }
      };
    parse_options( options, options_str );

    cooldown = 180.0;
    base_cost = p -> resource_base[ RESOURCE_MANA ] * 0.12;
    id = 33831;
  }

  virtual void execute()
  {
    struct treants_expiration_t : public event_t
    {
      treants_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
      {
        sim -> add_event( this, 30.0 );
      }
      virtual void execute()
      {
        player -> dismiss_pet( "treants" );
      }
    };

    consume_resource();
    update_ready();
    player -> summon_pet( "treants" );
    new ( sim ) treants_expiration_t( sim, player );
  }

  virtual bool ready()
  {
    if ( ! druid_spell_t::ready() )
      return false;

    if ( target_pct > 0 )
      if ( sim -> target -> health_percentage() > target_pct )
        return false;

    return true;
  }
};

// Stealth ==================================================================

struct stealth_t : public spell_t
{
  stealth_t( player_t* player, const std::string& options_str ) :
      spell_t( "stealth", player )
  {
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    enter_stealth( p );
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();
    return p -> _buffs.stealthed == 0;
  }
};

} // ANONYMOUS NAMESPACE ===================================================

// ==========================================================================
// Druid Character Definition
// ==========================================================================

// druid_t::create_action  ==================================================

action_t* druid_t::create_action( const std::string& name,
                                  const std::string& options_str )
{
  if ( name == "auto_attack"       ) return new       auto_attack_t( this, options_str );
  if ( name == "berserk"           ) return new           berserk_t( this, options_str );
  if ( name == "cat_form"          ) return new          cat_form_t( this, options_str );
  if ( name == "claw"              ) return new              claw_t( this, options_str );
  if ( name == "faerie_fire"       ) return new       faerie_fire_t( this, options_str );
  if ( name == "faerie_fire_feral" ) return new faerie_fire_feral_t( this, options_str );
  if ( name == "ferocious_bite"    ) return new    ferocious_bite_t( this, options_str );
  if ( name == "insect_swarm"      ) return new      insect_swarm_t( this, options_str );
  if ( name == "innervate"         ) return new         innervate_t( this, options_str );
  if ( name == "mangle_cat"        ) return new        mangle_cat_t( this, options_str );
  if ( name == "mark_of_the_wild"  ) return new  mark_of_the_wild_t( this, options_str );
  if ( name == "moonfire"          ) return new          moonfire_t( this, options_str );
  if ( name == "moonkin_form"      ) return new      moonkin_form_t( this, options_str );
  if ( name == "natures_swiftness" ) return new  druids_swiftness_t( this, options_str );
  if ( name == "rake"              ) return new              rake_t( this, options_str );
  if ( name == "rip"               ) return new               rip_t( this, options_str );
  if ( name == "savage_roar"       ) return new       savage_roar_t( this, options_str );
  if ( name == "shred"             ) return new             shred_t( this, options_str );
  if ( name == "starfire"          ) return new          starfire_t( this, options_str );
  if ( name == "starfall"          ) return new          starfall_t( this, options_str );
  if ( name == "stealth"           ) return new           stealth_t( this, options_str );
  if ( name == "tigers_fury"       ) return new       tigers_fury_t( this, options_str );
  if ( name == "treants"           ) return new     treants_spell_t( this, options_str );
  if ( name == "wrath"             ) return new             wrath_t( this, options_str );
#if 0
  if ( name == "cower"             ) return new             cower_t( this, options_str );
  if ( name == "maim"              ) return new              maim_t( this, options_str );
  if ( name == "prowl"             ) return new             prowl_t( this, options_str );
  if ( name == "ravage"            ) return new            ravage_t( this, options_str );
  if ( name == "swipe_cat"         ) return new         swipe_cat_t( this, options_str );
#endif

  return player_t::create_action( name, options_str );
}

// druid_t::create_pet ======================================================

pet_t* druid_t::create_pet( const std::string& pet_name )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "treants" ) return new treants_pet_t( sim, this, pet_name );

  return 0;
}


// druid_t::init_rating =====================================================

void druid_t::init_rating()
{
  player_t::init_rating();

  rating.attack_haste *= 1.0 / 1.30;
}

// druid_t::init_base =======================================================

void druid_t::init_base()
{
  attribute_base[ ATTR_STRENGTH  ] =  94;
  attribute_base[ ATTR_AGILITY   ] =  77;
  attribute_base[ ATTR_STAMINA   ] = 100;
  attribute_base[ ATTR_INTELLECT ] = 138;
  attribute_base[ ATTR_SPIRIT    ] = 161;

  if ( talents.moonkin_form && talents.furor )
  {
    attribute_multiplier_initial[ ATTR_INTELLECT ] *= 1.0 + talents.furor * 0.02;
  }
  for ( int i=0; i < ATTRIBUTE_MAX; i++ )
  {
    attribute_multiplier[ i ] *= 1.0 + 0.02 * talents.survival_of_the_fittest;

    attribute_multiplier[ i ] *= 1.0 + 0.01 * talents.improved_mark_of_the_wild;
  }

  base_spell_crit = 0.0185298;
  initial_spell_crit_per_intellect = rating_t::interpolate( level, 0.01/60.0, 0.01/80.0, 0.01/166.79732 );
  initial_spell_power_per_intellect = talents.lunar_guidance * 0.04;
  initial_spell_power_per_spirit = ( talents.improved_moonkin_form * 0.10 );

  base_attack_power = ( level * 2 ) - 20;
  base_attack_crit  = 0.0747516;
  base_attack_expertise = 0.25 * talents.primal_precision * 0.05;

  initial_attack_power_per_agility  = 1.0;
  initial_attack_power_per_strength = 2.0;
  initial_attack_crit_per_agility = rating_t::interpolate( level, 0.01/25.0, 0.01/40.0, 0.01/83.3104 );

  // FIXME! Make this level-specific.
  resource_base[ RESOURCE_HEALTH ] = 3600;
  if ( talents.moonkin_form )
  {
    resource_base[ RESOURCE_MANA ] = rating_t::interpolate( level, 1103, 2090, 3496 );
  }
  else
  {
    resource_base[ RESOURCE_ENERGY ] = 100;
  }

  health_per_stamina      = 10;
  mana_per_intellect      = 15;
  energy_regen_per_second = 10;

  mana_regen_while_casting = util_t::talent_rank( talents.intensity,  3, 0.17, 0.33, 0.50 );

  mp5_per_intellect = util_t::talent_rank( talents.dreamstate, 3, 0.04, 0.07, 0.10 );

  base_gcd = 1.5;
}

// druid_t::init_gains ======================================================

void druid_t::init_gains()
{
  player_t::init_gains();

  gains_energy_refund    = get_gain( "energy_refund"    );
  gains_moonkin_form     = get_gain( "moonkin_form"     );
  gains_omen_of_clarity  = get_gain( "omen_of_clarity"  );
  gains_primal_precision = get_gain( "primal_precision" );
  gains_tigers_fury      = get_gain( "tigers_fury"      );
}

// druid_t::init_procs ======================================================

void druid_t::init_procs()
{
  player_t::init_procs();

  procs_combo_points        = get_proc( "combo_points"        );
  procs_combo_points_wasted = get_proc( "combo_points_wasted" );
  procs_omen_of_clarity     = get_proc( "omen_of_clarity"     );
  procs_primal_fury         = get_proc( "primal_fury"         );
  procs_unseen_moon         = get_proc( "unseen_moon"         );
}

// druid_t::init_uptimes ====================================================

void druid_t::init_uptimes()
{
  player_t::init_uptimes();

  uptimes_eclipse_starfire = get_uptime( "eclipse_starfire" );
  uptimes_eclipse_wrath    = get_uptime( "eclipse_wrath"    );
  uptimes_energy_cap       = get_uptime( "energy_cap"       );
  uptimes_natures_grace    = get_uptime( "natures_grace"    );
  uptimes_savage_roar      = get_uptime( "savage_roar"      );
  uptimes_rip              = get_uptime( "rip"              );
  uptimes_rake             = get_uptime( "rake"             );
}

// druid_t::init_rng ========================================================

void druid_t::init_rng()
{
  player_t::init_rng();

  rng_eclipse         = get_rng( "eclipse"         );
  rng_natures_grace   = get_rng( "natures_grace"   );
  rng_omen_of_clarity = get_rng( "omen_of_clarity" );
  rng_primal_fury     = get_rng( "primal_fury"     );
  rng_unseen_moon     = get_rng( "unseen_moon"     );
} 

// druid_t::init_unique_gear ================================================

void druid_t::init_unique_gear()
{
  player_t::init_unique_gear();

  if ( talents.moonkin_form )
  {
    if ( unique_gear -> tier4_2pc ) tiers.t4_2pc_balance = 1;
    if ( unique_gear -> tier4_4pc ) tiers.t4_4pc_balance = 1;
    if ( unique_gear -> tier5_2pc ) tiers.t5_2pc_balance = 1;
    if ( unique_gear -> tier5_4pc ) tiers.t5_4pc_balance = 1;
    if ( unique_gear -> tier6_2pc ) tiers.t6_2pc_balance = 1;
    if ( unique_gear -> tier6_4pc ) tiers.t6_4pc_balance = 1;
    if ( unique_gear -> tier7_2pc ) tiers.t7_2pc_balance = 1;
    if ( unique_gear -> tier7_4pc ) tiers.t7_4pc_balance = 1;
    if ( unique_gear -> tier8_2pc ) tiers.t8_2pc_balance = 1;
    if ( unique_gear -> tier8_4pc ) tiers.t8_4pc_balance = 1;
  }
  else
  {
    if ( unique_gear -> tier4_2pc ) tiers.t4_2pc_feral = 1;
    if ( unique_gear -> tier4_4pc ) tiers.t4_4pc_feral = 1;
    if ( unique_gear -> tier5_2pc ) tiers.t5_2pc_feral = 1;
    if ( unique_gear -> tier5_4pc ) tiers.t5_4pc_feral = 1;
    if ( unique_gear -> tier6_2pc ) tiers.t6_2pc_feral = 1;
    if ( unique_gear -> tier6_4pc ) tiers.t6_4pc_feral = 1;
    if ( unique_gear -> tier7_2pc ) tiers.t7_2pc_feral = 1;
    if ( unique_gear -> tier7_4pc ) tiers.t7_4pc_feral = 1;
    if ( unique_gear -> tier8_2pc ) tiers.t8_2pc_feral = 1;
    if ( unique_gear -> tier8_4pc ) tiers.t8_4pc_feral = 1;

    equipped_weapon_dps = main_hand_weapon.damage / main_hand_weapon.swing_time;
  }
}

// druid_t::init_actions ====================================================

void druid_t::init_actions()
{
  if( action_list_str.empty() )
  {
    if( talents.moonkin_form )
    {
      // Assume balance
      action_list_str+="flask,type=frost_wyrm/food,type=fish_feast/mark_of_the_wild/moonkin_form/mana_potion";
      action_list_str+="/innervate,trigger=19000";
      if( talents.force_of_nature ) 
        action_list_str+="/treants";
      if( talents.starfall ) 
        action_list_str+="/starfall,skip_on_eclipse=1";
      action_list_str+="/moonfire,eclipse_left>=12";
      if( talents.insect_swarm )
        action_list_str+="/insect_swarm,skip_on_eclipse=1";
      action_list_str+="/wrath,eclipse=trigger/starfire";
    }
    else if( talents.mangle )
    {
      // Assume feral
      action_list_str+="flask,type=endless_rage/food,type=blackened_dragonfin";
      action_list_str+="/cat_form/auto_attack/shred,omen_of_clarity=1/tigers_fury,energy<=40";
      if( talents.berserk )
        action_list_str+="/berserk,tigers_fury=1";
      action_list_str+="/savage_roar,cp>=1,savage_roar<=4/rip,cp>=5,time_to_die>=10";
      action_list_str+="/ferocious_bite,cp>=5,rip>=5,savage_roar>=6/mangle_cat,mangle<=2/rake/shred";
    }

    if ( sim -> debug ) log_t::output( sim, "Player %s using default actions: %s", name(), action_list_str.c_str()  );
  }

  player_t::init_actions();
}

// druid_t::reset ===========================================================

void druid_t::reset()
{
  player_t::reset();

  // Spells
  active_insect_swarm = 0;
  active_moonfire     = 0;
  active_rake         = 0;
  active_rip          = 0;

  _buffs.reset();
  _cooldowns.reset();
  _expirations.reset();

  base_gcd = 1.5;
}

// druid_t::interrupt =======================================================

void druid_t::interrupt()
{
  player_t::interrupt();

  if( melee_attack ) melee_attack -> cancel();
}

// druid_t::reset ===========================================================

void druid_t::regen( double periodicity )
{
  player_t::regen( periodicity );

  if ( resource_max[ RESOURCE_ENERGY ] > 0 )
  {
    uptimes_energy_cap -> update( resource_current[ RESOURCE_ENERGY ] ==
                                  resource_max    [ RESOURCE_ENERGY ] );
  }
}

// druid_t::composite_attack_power ==========================================

double druid_t::composite_attack_power()
{
  double ap = player_t::composite_attack_power();

  double weapon_ap = ( equipped_weapon_dps - 54.8 ) * 14;

  if ( talents.predatory_strikes )
  {
    ap += level * talents.predatory_strikes * 0.5;
    weapon_ap *= 1 + util_t::talent_rank( talents.predatory_strikes, 3, 0.07, 0.14, 0.20 );
  }

  if ( _buffs.cat_form ) ap += 160;

  ap += weapon_ap;

  return ap;
}

// druid_t::composite_attack_power_multiplier ===============================

double druid_t::composite_attack_power_multiplier()
{
  double multiplier = player_t::composite_attack_power_multiplier();

  if ( _buffs.cat_form && talents.heart_of_the_wild )
  {
    multiplier *= 1 + talents.heart_of_the_wild * 0.02;
  }

  return multiplier;
}

// druid_t::composite_attack_crit ==========================================

double druid_t::composite_attack_crit()
{
  double c = player_t::composite_attack_crit();

  if ( _buffs.cat_form )
  {
    c += 0.02 * talents.sharpened_claws;
    c += 0.02 * talents.master_shapeshifter;
  }

  return c;
}

// druid_t::composite_spell_hit =============================================

double druid_t::composite_spell_hit()
{
  double hit = player_t::composite_spell_hit();

  if ( talents.balance_of_power )
  {
    hit += talents.balance_of_power * 0.02;
  }

  return hit;
}

// druid_t::composite_spell_crit ============================================

double druid_t::composite_spell_crit()
{
  double crit = player_t::composite_spell_crit();

  if ( talents.natural_perfection )
  {
    crit += talents.natural_perfection * 0.01;
  }

  return crit;
}

// druid_t::get_talent_trees ===============================================

bool druid_t::get_talent_trees( std::vector<int*>& balance,
                                std::vector<int*>& feral,
                                std::vector<int*>& restoration )
{
  talent_translation_t translation[][3] =
    {
      { {  1, &( talents.starlight_wrath       ) }, {  1, &( talents.ferocity                ) }, {  1, &( talents.improved_mark_of_the_wild ) } },
      { {  2, &( talents.genesis               ) }, {  2, &( talents.feral_aggression        ) }, {  2, NULL                                   } },
      { {  3, &( talents.moonglow              ) }, {  3, &( talents.feral_instinct          ) }, {  3, &( talents.furor                     ) } },
      { {  4, &( talents.natures_majesty       ) }, {  4, &( talents.savage_fury             ) }, {  4, &( talents.naturalist                ) } },
      { {  5, &( talents.improved_moonfire     ) }, {  5, NULL                                 }, {  5, NULL                                   } },
      { {  6, &( talents.brambles              ) }, {  6, NULL                                 }, {  6, NULL                                   } },
      { {  7, &( talents.natures_grace         ) }, {  7, NULL                                 }, {  7, &( talents.intensity                 ) } },
      { {  8, &( talents.natures_splendor      ) }, {  8, &( talents.sharpened_claws         ) }, {  8, &( talents.omen_of_clarity           ) } },
      { {  9, &( talents.natures_reach         ) }, {  9, &( talents.shredding_attacks       ) }, {  9, &( talents.master_shapeshifter       ) } },
      { { 10, &( talents.vengeance             ) }, { 10, &( talents.predatory_strikes       ) }, { 10, NULL                                   } },
      { { 11, &( talents.celestial_focus       ) }, { 11, &( talents.primal_fury             ) }, { 11, NULL                                   } },
      { { 12, &( talents.lunar_guidance        ) }, { 12, &( talents.primal_precision        ) }, { 12, &( talents.natures_swiftness         ) } },
      { { 13, &( talents.insect_swarm          ) }, { 13, NULL                                 }, { 13, NULL                                   } },
      { { 14, &( talents.improved_insect_swarm ) }, { 14, NULL                                 }, { 14, NULL                                   } },
      { { 15, &( talents.dreamstate            ) }, { 15, NULL                                 }, { 15, NULL                                   } },
      { { 16, &( talents.moonfury              ) }, { 16, NULL                                 }, { 16, NULL                                   } },
      { { 17, &( talents.balance_of_power      ) }, { 17, &( talents.heart_of_the_wild       ) }, { 17, &( talents.living_spirit             ) } },
      { { 18, &( talents.moonkin_form          ) }, { 18, &( talents.survival_of_the_fittest ) }, { 18, NULL                                   } },
      { { 19, &( talents.improved_moonkin_form ) }, { 19, &( talents.leader_of_the_pack      ) }, { 19, &( talents.natural_perfection        ) } },
      { { 20, &( talents.improved_faerie_fire  ) }, { 20, NULL                                 }, { 20, NULL                                   } },
      { { 21, NULL                               }, { 21, NULL                                 }, { 21, NULL                                   } },
      { { 22, &( talents.wrath_of_cenarius     ) }, { 22, &( talents.protector_of_the_pack   ) }, { 22, NULL                                   } },
      { { 23, &( talents.eclipse               ) }, { 23, &( talents.predatory_instincts     ) }, { 23, NULL                                   } },
      { { 24, NULL                               }, { 24, &( talents.infected_wounds         ) }, { 24, NULL                                   } },
      { { 25, &( talents.force_of_nature       ) }, { 25, &( talents.king_of_the_jungle      ) }, { 25, NULL                                   } },
      { { 26, NULL                               }, { 26, &( talents.mangle                  ) }, { 26, NULL                                   } },
      { { 27, &( talents.earth_and_moon        ) }, { 27, &( talents.improved_mangle         ) }, {  0, NULL                                   } },
      { { 28, &( talents.starfall              ) }, { 28, &( talents.rend_and_tear           ) }, {  0, NULL                                   } },
      { {  0, NULL                               }, { 29, &( talents.primal_gore             ) }, {  0, NULL                                   } },
      { {  0, NULL                               }, { 30, &( talents.berserk                 ) }, {  0, NULL                                   } },
      { {  0, NULL                               }, {  0, NULL                                 }, {  0, NULL                                   } },
    };

  return player_t::get_talent_trees( balance, feral, restoration, translation );
}

// druid_t::parse_talents_mmo =============================================

bool druid_t::parse_talents_mmo( const std::string& talent_string )
{
  // druid mmo encoding: Feral-Restoration-Balance

  int size1 = 29;
  int size2 = 26;

  std::string       feral_string( talent_string,     0,  size1 );
  std::string restoration_string( talent_string, size1,  size2 );
  std::string     balance_string( talent_string, size1 + size2 );

  return parse_talents( balance_string + feral_string + restoration_string );
}

// druid_t::parse_option  ==============================================

bool druid_t::parse_option( const std::string& name,
                            const std::string& value )
{
  option_t options[] =
    {
      // @option_doc loc=skip
      { "balance_of_power",          OPT_INT,  &( talents.balance_of_power          ) },
      { "berserk",                   OPT_INT,  &( talents.berserk                   ) },
      { "brambles",                  OPT_INT,  &( talents.brambles                  ) },
      { "celestial_focus",           OPT_INT,  &( talents.celestial_focus           ) },
      { "dreamstate",                OPT_INT,  &( talents.dreamstate                ) },
      { "earth_and_moon",            OPT_INT,  &( talents.earth_and_moon            ) },
      { "eclipse",                   OPT_INT,  &( talents.eclipse                   ) },
      { "feral_aggression",          OPT_INT,  &( talents.feral_aggression          ) },
      { "feral_instinct",            OPT_INT,  &( talents.feral_instinct            ) },
      { "ferocity",                  OPT_INT,  &( talents.ferocity                  ) },
      { "force_of_nature",           OPT_INT,  &( talents.force_of_nature           ) },
      { "furor",                     OPT_INT,  &( talents.furor                     ) },
      { "genesis",                   OPT_INT,  &( talents.genesis                   ) },
      { "heart_of_the_wild",         OPT_INT,  &( talents.heart_of_the_wild         ) },
      { "improved_faerie_fire",      OPT_INT,  &( talents.improved_faerie_fire      ) },
      { "improved_insect_swarm",     OPT_INT,  &( talents.improved_insect_swarm     ) },
      { "improved_mangle",           OPT_INT,  &( talents.improved_mangle           ) },
      { "improved_mark_of_the_wild", OPT_INT,  &( talents.improved_mark_of_the_wild ) },
      { "improved_moonfire",         OPT_INT,  &( talents.improved_moonfire         ) },
      { "improved_moonkin_form",     OPT_INT,  &( talents.improved_moonkin_form     ) },
      { "infected_wounds",           OPT_INT,  &( talents.infected_wounds           ) },
      { "insect_swarm",              OPT_INT,  &( talents.insect_swarm              ) },
      { "intensity",                 OPT_INT,  &( talents.intensity                 ) },
      { "king_of_the_jungle",        OPT_INT,  &( talents.king_of_the_jungle        ) },
      { "leader_of_the_pack",        OPT_INT,  &( talents.leader_of_the_pack        ) },
      { "living_spirit",             OPT_INT,  &( talents.living_spirit             ) },
      { "lunar_guidance",            OPT_INT,  &( talents.lunar_guidance            ) },
      { "mangle",                    OPT_INT,  &( talents.mangle                    ) },
      { "master_shapeshifter",       OPT_INT,  &( talents.master_shapeshifter       ) },
      { "moonfury",                  OPT_INT,  &( talents.moonfury                  ) },
      { "moonglow",                  OPT_INT,  &( talents.moonglow                  ) },
      { "moonkin_form",              OPT_INT,  &( talents.moonkin_form              ) },
      { "natural_perfection",        OPT_INT,  &( talents.natural_perfection        ) },
      { "naturalist",                OPT_INT,  &( talents.naturalist                ) },
      { "natures_grace",             OPT_INT,  &( talents.natures_grace             ) },
      { "natures_majesty",           OPT_INT,  &( talents.natures_majesty           ) },
      { "natures_reach",             OPT_INT,  &( talents.natures_reach             ) },
      { "natures_splendor",          OPT_INT,  &( talents.natures_splendor          ) },
      { "natures_swiftness",         OPT_INT,  &( talents.natures_swiftness         ) },
      { "omen_of_clarity",           OPT_INT,  &( talents.omen_of_clarity           ) },
      { "predatory_instincts",       OPT_INT,  &( talents.predatory_instincts       ) },
      { "predatory_strikes",         OPT_INT,  &( talents.predatory_strikes         ) },
      { "primal_fury",               OPT_INT,  &( talents.primal_fury               ) },
      { "primal_gore",               OPT_INT,  &( talents.primal_gore               ) },
      { "primal_precision",          OPT_INT,  &( talents.primal_precision          ) },
      { "protector_of_the_pack",     OPT_INT,  &( talents.protector_of_the_pack     ) },
      { "rend_and_tear",             OPT_INT,  &( talents.rend_and_tear             ) },
      { "savage_fury",               OPT_INT,  &( talents.savage_fury               ) },
      { "sharpened_claws",           OPT_INT,  &( talents.sharpened_claws           ) },
      { "shredding_attacks",         OPT_INT,  &( talents.shredding_attacks         ) },
      { "survival_of_the_fittest",   OPT_INT,  &( talents.survival_of_the_fittest   ) },
      { "starlight_wrath",           OPT_INT,  &( talents.starlight_wrath           ) },
      { "vengeance",                 OPT_INT,  &( talents.vengeance                 ) },
      { "wrath_of_cenarius",         OPT_INT,  &( talents.wrath_of_cenarius         ) },
      // @option_doc loc=player/druid/glyphs title="Glyphs"
      { "glyph_berserk",             OPT_BOOL,  &( glyphs.berserk                   ) },
      { "glyph_insect_swarm",        OPT_BOOL,  &( glyphs.insect_swarm              ) },
      { "glyph_innervate",           OPT_BOOL,  &( glyphs.innervate                 ) },
      { "glyph_mangle",              OPT_BOOL,  &( glyphs.mangle                    ) },
      { "glyph_moonfire",            OPT_BOOL,  &( glyphs.moonfire                  ) },
      { "glyph_rip",                 OPT_BOOL,  &( glyphs.rip                       ) },
      { "glyph_savage_roar",         OPT_BOOL,  &( glyphs.savage_roar               ) },
      { "glyph_shred",               OPT_BOOL,  &( glyphs.shred                     ) },
      { "glyph_starfire",            OPT_BOOL,  &( glyphs.starfire                  ) },
      { "glyph_starfall",            OPT_BOOL,  &( glyphs.starfall                  ) },
      // @option_doc loc=player/druid/idols title="Idols"
      { "idol_of_the_corruptor",      OPT_BOOL,  &( idols.corruptor                 ) },
      { "idol_of_the_crying_wind",    OPT_BOOL,  &( idols.crying_wind               ) },
      { "idol_of_the_ravenous_beast", OPT_BOOL,  &( idols.ravenous_beast            ) },
      { "idol_of_steadfast_renewal",  OPT_BOOL,  &( idols.steadfast_renewal         ) },
      { "idol_of_the_shooting_star",  OPT_BOOL,  &( idols.shooting_star             ) },
      { "idol_of_the_unseen_moon",    OPT_BOOL,  &( idols.unseen_moon               ) },
      { "idol_of_worship",            OPT_BOOL,  &( idols.worship                   ) },
      // @option_doc loc=skip
      { "tier4_2pc_balance",          OPT_BOOL,  &( tiers.t4_2pc_balance            ) },
      { "tier4_4pc_balance",          OPT_BOOL,  &( tiers.t4_4pc_balance            ) },
      { "tier5_2pc_balance",          OPT_BOOL,  &( tiers.t5_2pc_balance            ) },
      { "tier5_4pc_balance",          OPT_BOOL,  &( tiers.t5_4pc_balance            ) },
      { "tier6_2pc_balance",          OPT_BOOL,  &( tiers.t6_2pc_balance            ) },
      { "tier6_4pc_balance",          OPT_BOOL,  &( tiers.t6_4pc_balance            ) },
      { "tier7_2pc_balance",          OPT_BOOL,  &( tiers.t7_2pc_balance            ) },
      { "tier7_4pc_balance",          OPT_BOOL,  &( tiers.t7_4pc_balance            ) },
      { "tier4_2pc_feral",            OPT_BOOL,  &( tiers.t4_2pc_feral              ) },
      { "tier4_4pc_feral",            OPT_BOOL,  &( tiers.t4_4pc_feral              ) },
      { "tier5_2pc_feral",            OPT_BOOL,  &( tiers.t5_2pc_feral              ) },
      { "tier5_4pc_feral",            OPT_BOOL,  &( tiers.t5_4pc_feral              ) },
      { "tier6_2pc_feral",            OPT_BOOL,  &( tiers.t6_2pc_feral              ) },
      { "tier6_4pc_feral",            OPT_BOOL,  &( tiers.t6_4pc_feral              ) },
      { "tier7_2pc_feral",            OPT_BOOL,  &( tiers.t7_2pc_feral              ) },
      { "tier7_4pc_feral",            OPT_BOOL,  &( tiers.t7_4pc_feral              ) },
      { NULL, OPT_UNKNOWN }
    };

  if ( name.empty() )
  {
    player_t::parse_option( std::string(), std::string() );
    option_t::print( sim -> output_file, options );
    return false;
  }

  if ( option_t::parse( sim, options, name, value ) ) return true;

  return player_t::parse_option( name, value );
}

// player_t::create_druid  ==================================================

player_t* player_t::create_druid( sim_t*             sim,
                                  const std::string& name )
{
  druid_t* p = new druid_t( sim, name );

  new treants_pet_t( sim, p, "treants" );

  return p;
}

