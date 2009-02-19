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
  action_t* active_rip;

  // Buffs
  int    buffs_berserk;
  int    buffs_bear_form;
  int    buffs_cat_form;
  int    buffs_combo_points;
  double buffs_eclipse_starfire;
  double buffs_eclipse_wrath;
  int    buffs_moonkin_form;
  int    buffs_natures_grace;
  int    buffs_natures_swiftness;
  int    buffs_omen_of_clarity;
  int    buffs_savage_roar;
  int    buffs_stealthed;
  double buffs_tigers_fury;

  // Expirations
  event_t* expirations_eclipse;
  event_t* expirations_savage_roar;

  // Cooldowns
  double cooldowns_eclipse;
  double cooldowns_omen_of_clarity;

  // Gains
  gain_t* gains_moonkin_form;
  gain_t* gains_omen_of_clarity;

  // Procs
  proc_t* procs_combo_points;
  proc_t* procs_omen_of_clarity;
  proc_t* procs_primal_fury;

  // Up-Times
  uptime_t* uptimes_eclipse_starfire;
  uptime_t* uptimes_eclipse_wrath;
  uptime_t* uptimes_savage_roar;

  attack_t* melee_attack;

  struct talents_t
  {
    int  balance_of_power;
    int  berserk;
    int  brambles;
    int  celestial_focus;
    int  dreamstate;
    int  earth_and_moon;
    int  eclipse;
    int  force_of_nature;
    int  furor;
    int  genesis;
    int  improved_faerie_fire;
    int  improved_insect_swarm;
    int  improved_mark_of_the_wild;
    int  improved_moonfire;
    int  improved_moonkin_form;
    int  insect_swarm;
    int  intensity;
    int  living_spirit;
    int  lunar_guidance;
    int  master_shapeshifter;
    int  moonfury;
    int  moonglow;
    int  moonkin_form;
    int  natural_perfection;
    int  natures_grace;
    int  natures_majesty;
    int  natures_reach;
    int  natures_splendor;
    int  natures_swiftness;
    int  omen_of_clarity;
    int  starfall;
    int  starlight_wrath;
    int  vengeance;
    int  wrath_of_cenarius;

    int  ferocity;
    int  improved_mangle;
    int  leader_of_the_pack;
    int  mangle;
    int  naturalist;
    int  primal_fury;
    int  savage_fury;
    int  sharpened_claws;
    int  shredding_attacks;
    int  survival_of_the_fittest;
    int  heart_of_the_wild;

    // partially implemented
    int  primal_precision; // FIXME energy reduction on miss NYI
    int  rend_and_tear;    // FIXME fb crit chance NYI

    // has no effect on dps
    int  infected_wounds;
    int  protector_of_the_pack;

    // Not Yet Implemented
    int  feral_aggression; // FIXME fb implementation missing
    int  feral_instinct;   // FIXME swipe (cat) implementation missing
    int  king_of_the_jungle;
    int  predatory_instincts;
    int  predatory_strikes;
    
    talents_t() { memset( (void*) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int innervate;
    int insect_swarm;
    int mangle;
    int moonfire;
    int rip;
    int starfire;
    int starfall;
    glyphs_t() { memset( (void*) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  struct idols_t
  {
    int ravenous_beast;
    int shooting_star;
    int steadfast_renewal;
    int worship;
    idols_t() { memset( (void*) this, 0x0, sizeof( idols_t ) ); }
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
    int t4_2pc_feral;
    int t4_4pc_feral;
    int t5_2pc_feral;
    int t5_4pc_feral;
    int t6_2pc_feral;
    int t6_4pc_feral;
    int t7_2pc_feral;
    int t7_4pc_feral;
    tiers_t() { memset( (void*) this, 0x0, sizeof( tiers_t ) ); }
  };
  tiers_t tiers;

  druid_t( sim_t* sim, std::string& name ) : player_t( sim, DRUID, name ) 
  {
    active_insect_swarm = 0;
    active_moonfire     = 0;
    active_rip          = 0;

    // Buffs
    buffs_berserk           = 0;
    buffs_bear_form         = 0;
    buffs_cat_form          = 0;
    buffs_combo_points      = 0;
    buffs_eclipse_starfire  = 0;
    buffs_eclipse_wrath     = 0;
    buffs_moonkin_form      = 0;
    buffs_natures_grace     = 0;
    buffs_natures_swiftness = 0;
    buffs_omen_of_clarity   = 0;
    buffs_savage_roar       = 0;
    buffs_stealthed         = 0;
    buffs_tigers_fury       = 0;

    // Expirations
    expirations_eclipse     = 0;
    expirations_savage_roar = 0;

    // Cooldowns
    cooldowns_eclipse = 0;

    // Gains
    gains_moonkin_form    = get_gain( "moonkin_form"    );
    gains_omen_of_clarity = get_gain( "omen_of_clarity" );

    // Procs
    procs_combo_points    = get_proc( "combo_points" );
    procs_omen_of_clarity = get_proc( "omen_of_clarity" );
    procs_primal_fury     = get_proc( "primal_fury" );

    // Up-Times
    uptimes_eclipse_starfire = get_uptime( "eclipse_starfire" );
    uptimes_eclipse_wrath    = get_uptime( "eclipse_wrath"    );
    uptimes_savage_roar      = get_uptime( "savage_roar"      );

    melee_attack = 0;
  }

  // Character Definition
  virtual void      init_base();
  virtual void      init_unique_gear();
  virtual void      reset();
  virtual double    composite_attack_power();
  virtual double    composite_spell_hit();
  virtual double    composite_spell_crit();
  virtual bool      get_talent_trees( std::vector<int*>& balance, std::vector<int*>& feral, std::vector<int*>& restoration );
  virtual bool      parse_talents_mmo( const std::string& talent_string );
  virtual bool      parse_option ( const std::string& name, const std::string& value );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name );
  virtual int       primary_resource() { return talents.moonkin_form ? RESOURCE_MANA : RESOURCE_ENERGY; }
  virtual double    composite_attribute_multiplier( int attr );
  virtual double    composite_attack_crit();

  // Utilities 
  double combo_point_rank( double* cp_list )
  {
    assert( buffs_combo_points > 0 );
    return cp_list[ buffs_combo_points-1 ];
  }
  double combo_point_rank( double cp1, double cp2, double cp3, double cp4, double cp5 )
  {
    double cp_list[] = { cp1, cp2, cp3, cp4, cp5 };
    return combo_point_rank( cp_list );
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
  int  min_combo_points, max_combo_points;
  double min_energy, max_energy;
  double min_mangle_expire, max_mangle_expire;
  double min_savage_roar_expire, max_savage_roar_expire;
  double min_rip_expire, max_rip_expire;

  druid_attack_t( const char* n, player_t* player, int s=SCHOOL_PHYSICAL, int t=TREE_NONE ) :
    attack_t( n, player, RESOURCE_ENERGY, s, t ),
    requires_stealth(0),
    requires_position(POSITION_NONE),
    requires_combo_points(false),
    adds_combo_points(false),
    min_combo_points(0), max_combo_points(0),
    min_energy(0), max_energy(0),
    min_mangle_expire(0), max_mangle_expire(0),
    min_savage_roar_expire(0), max_savage_roar_expire(0),
    min_rip_expire(0), max_rip_expire(0)
  {
    may_glance = false;
  }

  virtual void   parse_options( option_t*, const std::string& options_str );
  virtual double cost();
  virtual void   execute();
  virtual void   consume_resource();
  virtual double calculate_direct_damage();
  virtual void   player_buff();
  virtual bool   ready();
};

// ==========================================================================
// Druid Spell
// ==========================================================================

struct druid_spell_t : public spell_t
{
  druid_spell_t( const char* n, player_t* p, int s, int t ) : 
    spell_t( n, p, RESOURCE_MANA, s, t ) {}

  virtual double cost();
  virtual double haste();
  virtual double execute_time();
  virtual void   execute();
  virtual void   schedule_execute();
  virtual void   consume_resource();
  virtual void   player_buff();
  virtual void   target_debuff( int dmg_type );
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
      
      base_multiplier *= 1.0 + o -> talents.brambles * 0.05;

      // Model the three Treants as one actor hitting 3x hard
      base_multiplier *= 3.0;
    }
    void player_buff()
    {
      attack_t::player_buff();
      player_t* o = player -> cast_pet() -> owner;
      player_power += 0.57 * o -> composite_spell_power( SCHOOL_MAX );
    }
  };

  melee_t* melee;

  treants_pet_t( sim_t* sim, player_t* owner, const std::string& pet_name ) :
    pet_t( sim, owner, pet_name ), melee(0)
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
  virtual void summon()
  {
    pet_t::summon();
    melee -> execute(); // Kick-off repeating attack
  }
};

// enter_stealth ===========================================================

static void enter_stealth( druid_t* p )
{
  p -> buffs_stealthed = 1;
}

// break_stealth ===========================================================

static void break_stealth( druid_t* p )
{
  if( p -> buffs_stealthed == 1 )
  {
    p -> buffs_stealthed = -1;
  }
}

// clear_combo_points ======================================================

static void clear_combo_points( druid_t* p )
{
  if( p -> buffs_combo_points <= 0 ) return;

  const char* name[] = { "Combo Points (1)",
                         "Combo Points (2)",
                         "Combo Points (3)",
                         "Combo Points (4)",
                         "Combo Points (5)" };

  p -> aura_loss( name[ p -> buffs_combo_points - 1 ] );

  p -> buffs_combo_points = 0;
}

// add_combo_point=== ======================================================

static void add_combo_point( druid_t* p )
{
  if( p -> buffs_combo_points >= 5 ) return;

  const char* name[] = { "Combo Points (1)",
                         "Combo Points (2)",
                         "Combo Points (3)",
                         "Combo Points (4)",
                         "Combo Points (5)" };

  p -> buffs_combo_points++;

  p -> aura_gain( name[ p -> buffs_combo_points - 1 ] );

  p -> procs_combo_points -> occur();
}


// trigger_omen_of_clarity ==================================================

static void trigger_omen_of_clarity( action_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if( p -> talents.omen_of_clarity == 0 ) return;

  double execute_time = a -> time_to_execute;

  if( execute_time == 0 ) execute_time = a -> gcd();

  double PPM = 3.5;
  double time_to_proc = 60.0 / PPM;
  double proc_chance = execute_time / time_to_proc;

  if( a -> sim -> roll( proc_chance ) )
  {
    p -> buffs_omen_of_clarity = 1;
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
      t -> debuffs.faerie_fire = util_t::ability_rank( p -> level,  1260,76,  610,66,  505,0 );
      sim -> add_event( this, 40.0 );
    }

    virtual void execute()
    {
      target_t* t = sim -> target;
      t -> debuffs.faerie_fire = 0;
      t -> expirations.faerie_fire = 0;
    }
  };
  
  event_t*& e = a -> sim -> target -> expirations.faerie_fire;

  if( e )
  {
    e -> reschedule( 40.0 );
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

  if( ! p -> talents.improved_faerie_fire )
    return;
  
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      sim -> target -> debuffs.improved_faerie_fire = 3;
      sim -> add_event( this, 40.0 );
    }
    virtual void execute()
    {
      sim -> target -> debuffs.improved_faerie_fire = 0;
      sim -> target -> expirations.improved_faerie_fire = 0;
    }
  };
  
  event_t*& e = a -> sim -> target -> expirations.improved_faerie_fire;

  if( e )
  {
    e -> reschedule( 40.0 );
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
    mangle_expiration_t( sim_t* sim, double duration ): event_t( sim, 0 )
    {
      sim -> target -> debuffs.mangle++;
      sim -> add_event( this, duration );
    }
    virtual void execute()
    {
      sim -> target -> debuffs.mangle--;
      sim -> target -> expirations.mangle = 0;
    }
  };

  druid_t* p = a -> player -> cast_druid();
      
  double duration = 12.0 + ( p -> glyphs.mangle ? 6.0 : 0.0 );

  event_t*& e = a -> sim -> target -> expirations.mangle;

  if( e )
  {
    if( e -> occurs() < ( a -> sim -> current_time + duration ) )
    {
      e -> reschedule( duration );
    }
  }
  else
  {
    e = new ( a -> sim ) mangle_expiration_t( a -> sim, duration );
  }
}

// trigger_earth_and_moon ===================================================

static void trigger_earth_and_moon( spell_t* s )
{
  druid_t* p = s -> player -> cast_druid();

  if( p -> talents.earth_and_moon == 0 ) return;

  struct earth_and_moon_expiration_t : public event_t
  {
    earth_and_moon_expiration_t( sim_t* sim, druid_t* p ) : event_t( sim )
    {
      name = "Earth and Moon Expiration";
      if( sim -> log ) report_t::log( sim, "%s gains Earth and Moon", sim -> target -> name() );
      sim -> target -> debuffs.earth_and_moon = (int) util_t::talent_rank( p -> talents.earth_and_moon, 3, 4, 9, 13 );
      sim -> add_event( this, 12.0 );
    }
    virtual void execute()
    {
      if( sim -> log ) report_t::log( sim, "%s loses Earth and Moon", sim -> target -> name() );
      sim -> target -> debuffs.earth_and_moon = 0;
      sim -> target -> expirations.earth_and_moon = 0;
    }
  };

  event_t*& e = s -> sim -> target -> expirations.earth_and_moon;
  
  if( e )
  {
    e -> reschedule( 12.0 );
  }
  else
  {
    e = new ( s -> sim ) earth_and_moon_expiration_t( s -> sim, p );
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
      p -> buffs_eclipse_wrath = sim -> current_time;
      p -> cooldowns_eclipse = sim -> current_time + 30;
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      druid_t* p = player -> cast_druid();
      p -> aura_loss( "Eclipse Wrath" );
      p -> buffs_eclipse_wrath = 0;
      p -> expirations_eclipse = 0;
    }
  };

  druid_t* p = s -> player -> cast_druid();

  if( p -> talents.eclipse != 0 && 
      s -> sim -> cooldown_ready( p -> cooldowns_eclipse ) &&
      s -> sim -> roll( p -> talents.eclipse * 1.0/3 ) )
  {
    p -> expirations_eclipse = new ( s -> sim ) expiration_t( s -> sim, p );
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
      p -> buffs_eclipse_starfire = sim -> current_time;
      p -> cooldowns_eclipse = sim -> current_time + 30;
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      druid_t* p = player -> cast_druid();
      p -> aura_loss( "Eclipse Starfire" );
      p -> buffs_eclipse_starfire = 0;
      p -> expirations_eclipse = 0;
    }
  };

  druid_t* p = s -> player -> cast_druid();

  if( p -> talents.eclipse != 0 && 
      s -> sim -> cooldown_ready( p -> cooldowns_eclipse ) &&
      s -> sim -> roll( p -> talents.eclipse * 0.2 ) )
  {
    p -> expirations_eclipse = new ( s -> sim ) expiration_t( s -> sim, p );
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

  if( p -> gear.ashtongue_talisman && s -> sim -> roll( 0.25 ) )
  {
    p -> procs.ashtongue_talisman -> occur();

    event_t*& e = p -> expirations.ashtongue_talisman;

    if( e )
    {
      e -> reschedule( 8.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim, p );
    }
  }
}

static void trigger_primal_fury( druid_attack_t* a )
{
  druid_t* p = a -> player -> cast_druid();
  
  if ( ! p -> talents.primal_fury )
    return;

  if ( ! a -> adds_combo_points )
    return;

  if ( a -> sim -> roll( p -> talents.primal_fury * 0.5 ) )
  {
    add_combo_point( p );
    p -> procs_primal_fury -> occur();
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
    { "min_combo_points", OPT_INT, &min_combo_points       },
    { "max_combo_points", OPT_INT, &max_combo_points       },
    { "cp>",              OPT_INT, &min_combo_points       },
    { "cp<",              OPT_INT, &max_combo_points       },
    { "min_energy",       OPT_FLT, &min_energy             },
    { "max_energy",       OPT_FLT, &max_energy             },
    { "energy>",          OPT_FLT, &min_energy             },
    { "energy<",          OPT_FLT, &max_energy             },
    { "rip>",             OPT_FLT, &min_rip_expire         },
    { "rip<",             OPT_FLT, &max_rip_expire         },
    { "mangle>",          OPT_FLT, &min_mangle_expire      },
    { "mangle<",          OPT_FLT, &max_mangle_expire      },
    { "savage_roar>",     OPT_FLT, &min_savage_roar_expire },
    { "savage_roar<",     OPT_FLT, &max_savage_roar_expire },
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
  if ( p -> buffs_omen_of_clarity ) return 0;
  if ( p -> buffs_berserk) c *= 0.5;
  return c;
}

// druid_attack_t::consume_resource ========================================

void druid_attack_t::consume_resource()
{
  druid_t* p = player -> cast_druid();
  attack_t::consume_resource();
  if( p -> buffs_omen_of_clarity )
  {
    // Treat the savings like a mana gain.
    double amount = attack_t::cost();
    if( amount > 0 )
    {
      p -> gains_omen_of_clarity -> add( amount );
      p -> buffs_omen_of_clarity = 0;
    }
  }
}

// druid_attack_t::execute =================================================

void druid_attack_t::execute()
{
  druid_t* p = player -> cast_druid();

  attack_t::execute();
 
  if( result_is_hit() )
  {
    if( requires_combo_points ) clear_combo_points( p );
    if(     adds_combo_points )   add_combo_point ( p );

    if( result == RESULT_CRIT )
    {
	trigger_primal_fury( this );
    }
  }

  break_stealth( p );
}

// druid_attack_t::calculate_direct_damage =================================

double druid_attack_t::calculate_direct_damage()
{
  druid_t* p = player -> cast_druid();

  if( base_dd_min > 0 && base_dd_max > 0 )
  {
    if( sim -> average_dmg )
    {
      base_direct_dmg = ( base_dd_min + base_dd_max ) / 2.0;
    }
    else
    {
      double delta = base_dd_max - base_dd_min;
      base_direct_dmg = base_dd_min + delta * sim -> rng -> real();
    }

    base_direct_dmg += p -> buffs_tigers_fury;
  }

  return attack_t::calculate_direct_damage();
}

// druid_attack_t::player_buff =============================================

void druid_attack_t::player_buff()
{
  druid_t* p = player -> cast_druid();

  attack_t::player_buff();

  p -> uptimes_savage_roar -> update( p -> buffs_savage_roar != 0 );

  if( p -> talents.naturalist )
  {
    player_multiplier *= 1 + p -> talents.naturalist * 0.02;
  }
}

// druid_attack_t::ready ===================================================

bool druid_attack_t::ready()
{
  if( ! attack_t::ready() )
    return false;

  druid_t*  p = player -> cast_druid();
  target_t* t = sim -> target;

  if( requires_position != POSITION_NONE )
    if( p -> position != requires_position )
      return false;

  if( requires_stealth )
    if( p -> buffs_stealthed <= 0 )
      return false;

  if( requires_combo_points && ( p -> buffs_combo_points == 0 ) )
    return false;

  if( min_combo_points > 0 )
    if( p -> buffs_combo_points < min_combo_points )
      return false;

  if( max_combo_points > 0 )
    if( p -> buffs_combo_points > max_combo_points )
      return false;

  if( min_energy > 0 )
    if( p -> resource_current[ RESOURCE_ENERGY ] < min_energy )
      return false;

  if( max_energy > 0 )
    if( p -> resource_current[ RESOURCE_ENERGY ] > max_energy )
      return false;

  double ct = sim -> current_time;

  if( min_mangle_expire > 0 )
    if( ! t -> expirations.mangle || ( ( t -> expirations.mangle -> occurs() - ct ) < min_mangle_expire ) )
      return false;

  if( max_mangle_expire > 0 )
    if( t -> expirations.mangle && ( ( t -> expirations.mangle -> occurs() - ct ) > max_mangle_expire ) )
      return false;

  if( min_savage_roar_expire > 0 )
    if( ! p -> expirations_savage_roar || ( ( p -> expirations_savage_roar -> occurs() - ct ) < min_savage_roar_expire ) )
      return false;

  if( max_savage_roar_expire > 0 )
    if( p -> expirations_savage_roar && ( ( p -> expirations_savage_roar -> occurs() - ct ) > max_savage_roar_expire ) )
      return false;

  if( min_rip_expire > 0 )
    if( ! p -> active_rip || ( ( p -> active_rip -> duration_ready - ct ) < min_rip_expire ) )
      return false;

  if( max_rip_expire > 0 )
    if( p -> active_rip && ( ( p -> active_rip -> duration_ready - ct ) > max_rip_expire ) )
      return false;

  return true;
}

// Melee Attack ============================================================

struct melee_t : public druid_attack_t
{
  melee_t( const char* name, player_t* player ) :
    druid_attack_t( name, player )
  {
    base_dd_min = base_dd_max = 1;
    may_glance  = true;
    background  = true;
    repeating   = true;
    trigger_gcd = 0;
    base_cost   = 0;
  }

  virtual void execute()
  {
    druid_attack_t::execute();
    if( result_is_hit() )
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
    init_rank( ranks );

    weapon = &( p -> main_hand_weapon );
    adds_combo_points = true;
    may_crit          = true;
  }

  virtual void player_buff()
  {
    druid_t* p = player -> cast_druid();
    
    if( p -> talents.savage_fury )
    {
      player_multiplier *= 1 + p -> talents.savage_fury * 0.1;
    }
  }

};

// Faerie Fire (Feral) ======================================================

struct faerie_fire_feral_t : public druid_attack_t
{
  int debuff_only;

  faerie_fire_feral_t( player_t* player, const std::string& options_str ) :
    druid_attack_t( "faerie_fire_feral", player, SCHOOL_PHYSICAL, TREE_FERAL ), debuff_only(0)
  {
    option_t options[] =
    {
      { "debuff_only", OPT_INT, &debuff_only },
      { NULL }
    };
    parse_options( options, options_str );

    base_dd_min = base_dd_max = 1;
    direct_power_mod = 0.05;
    cooldown = 6.0;
    may_crit = true;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    if( p -> buffs_bear_form ) druid_attack_t::execute();
    trigger_faerie_fire( this );
    update_ready();
  }

  virtual bool ready() 
  {
    if( ! druid_attack_t::ready() )
      return false;

    if( debuff_only && sim -> target -> debuffs.faerie_fire )
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
    init_rank( ranks );

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier *= 2.0;

    adds_combo_points = true;
    may_crit          = true;
  }

  virtual void execute()
  {
    druid_attack_t::execute();
    if( result_is_hit() )
    {
      trigger_mangle( this );
    }
  }

  virtual double cost()
  {
    double c = attack_t::cost();

    druid_t* p = player -> cast_druid();

    if( p -> talents.ferocity )
    {
      c -= p -> talents.ferocity;
    }

    if( p -> talents.improved_mangle )
    {
      c -= p -> talents.improved_mangle * 2;
    }

    return c;
  }

  virtual void player_buff()
  {
    druid_t* p = player -> cast_druid();
    
    if( p -> talents.savage_fury )
    {
      player_multiplier *= 1 + p -> talents.savage_fury * 0.1;
    }
  }
};

// Rake ====================================================================

struct rake_t : public druid_attack_t
{
  rake_t( player_t* player, const std::string& options_str ) :
    druid_attack_t( "rake", player, SCHOOL_BLEED, TREE_FERAL )
  {
    //druid_t* p = player -> cast_druid();

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
    init_rank( ranks );

    adds_combo_points = true;
    may_crit          = true;
    base_tick_time    = 3.0;
    num_ticks         = 3;
    direct_power_mod  = 0.01;
    tick_power_mod    = 0.06;
  }

  virtual double cost()
  {
    double c = attack_t::cost();

    druid_t* p = player -> cast_druid();

    if( p -> talents.ferocity )
    {
      c -= p -> talents.ferocity;
    }

    return c;
  }

  virtual void player_buff()
  {
    druid_t* p = player -> cast_druid();
    
    if( p -> talents.savage_fury )
    {
      player_multiplier *= 1 + p -> talents.savage_fury * 0.1;
    }
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

    observer = &( p -> active_rip );    
  }

  virtual void player_buff()
  {
    druid_t* p = player -> cast_druid();
    tick_power_mod = p -> buffs_combo_points * 0.01;
    base_td_init   = p -> combo_point_rank( combo_point_dmg );
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
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, druid_t* p, double duration ): event_t( sim, p )
      {
	p -> aura_gain( "Savage Roar" );
	p -> buffs_savage_roar = 1;
	sim -> add_event( this, duration );
      }
      virtual void execute()
      {
	druid_t* p = player -> cast_druid();
	p -> aura_loss( "Savage Roar" );
	p -> buffs_savage_roar = 0;
	p -> expirations_savage_roar = 0;
      }
    };

    druid_t* p = player -> cast_druid();
      
    double duration = 9.0 + p -> buffs_combo_points * 5.0;

    clear_combo_points( p );    

    event_t*& e = p -> expirations_savage_roar;

    if( e )
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
  shred_t( player_t* player, const std::string& options_str ) :
    druid_attack_t( "shred", player, SCHOOL_PHYSICAL, TREE_FERAL )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
    {
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
    init_rank( ranks );

    weapon = &( p -> main_hand_weapon );
    weapon_multiplier *= 2.25;
    requires_position = POSITION_BACK;
    adds_combo_points = true;
    may_crit          = true;
  }

  virtual void player_buff()
  {
    druid_attack_t::player_buff();
    if( sim -> target -> debuffs.mangle ) player_multiplier *= 1.30;
    
    // FIXME: Need bleed detection (rogues, warrs, hunter pets, other ferals, etc?)
    //if( talents.rend_and_tear && sim -> target -> bleeding )
    //{
    //  player_multiplier *= 0.04 * talents.rend_and_tear;
    //} 
  }

  virtual double cost()
  {
    double c = attack_t::cost();
    
    druid_t* p = player -> cast_druid();

    if ( p -> talents.shredding_attacks )
    {
      c -= 9 * p -> talents.shredding_attacks;
    }

    return c;
  }
};

// Berserk =================================================================

struct berserk_t : public druid_attack_t
{
   berserk_t( player_t* player, const std::string& options_str ) : 
    druid_attack_t( "berserk", player )
  {
    druid_t* p = player -> cast_druid();
    assert( p -> talents.berserk );

    option_t options[] =
    {
      { NULL }
    };
    
    parse_options( options, options_str );
    
    
    base_cost   = 0;
    trigger_gcd = 0;
    cooldown    = 180;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, druid_t* p ) : event_t( sim, p )
      {
        name = "Berserk Expiration";
        p -> aura_gain( "Berserk" );
        p -> buffs_berserk = 1;
        sim -> add_event( this, 15.0 );
      }
      virtual void execute()
      {
        druid_t* p = player -> cast_druid();
        p -> aura_loss( "Berserk" );
        p -> buffs_berserk = 0;
      }
    };

    druid_t* p = player -> cast_druid();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    new ( sim ) expiration_t( sim, p );
  }
};

// Tigers Fury =============================================================

struct tigers_fury_t : public druid_attack_t
{
  tigers_fury_t( player_t* player, const std::string& options_str ) :
    druid_attack_t( "tigers_fury", player, SCHOOL_PHYSICAL, TREE_FERAL )
  {
    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );

    cooldown       = 30.0;
    trigger_gcd    = 0;
  }

  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );

    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* player ): event_t( sim, player )
      {
        druid_t* p = player -> cast_druid();
        p -> aura_gain( "Tigers Fury" );
        p -> buffs_tigers_fury = util_t::ability_rank( p -> level,  80.0,79, 60.0,71,  40.0,0 );
        sim -> add_event( this, 6.0 );
      }
      virtual void execute()
      {
        druid_t* p = player -> cast_druid();
        p -> aura_loss( "Tigers Fury" );
        p -> buffs_tigers_fury = 0;
      }
    };

    new ( sim ) expiration_t( sim, player );

    update_ready();
  }
  virtual bool ready() 
  {
    druid_t* p = player -> cast_druid();
    if( ! druid_attack_t::ready() )
      return false;
      
    if( p -> buffs_berserk == 1)
      return false;

    return true;
  }
};

// =========================================================================
// Druid Spell
// =========================================================================

// druid_spell_t::cost =====================================================

double druid_spell_t::cost()
{
  druid_t* p = player -> cast_druid();
  if( p -> buffs_omen_of_clarity ) return 0;
  return spell_t::cost();
}

// druid_spell_t::haste ====================================================

double druid_spell_t::haste()
{
  druid_t* p = player -> cast_druid();
  double h = spell_t::haste();
  if( p -> talents.celestial_focus ) h *= 1.0 / ( 1.0 + p -> talents.celestial_focus * 0.01 );
  return h;
}

// druid_spell_t::execute_time =============================================

double druid_spell_t::execute_time()
{
  druid_t* p = player -> cast_druid();
  if( p -> buffs_natures_swiftness ) return 0;
  return spell_t::execute_time();
}

// druid_spell_t::schedule_execute =========================================

void druid_spell_t::schedule_execute()
{
  druid_t* p = player -> cast_druid();

  spell_t::schedule_execute();

  if( base_execute_time > 0 )
  {
    if( p -> buffs_natures_swiftness )
    {
      p -> buffs_natures_swiftness = 0;
    }
    else if( p -> buffs_natures_grace )
    {
      p -> aura_loss( "Natures Grace" );
      p -> buffs_natures_grace = 0;
      p -> buffs.cast_time_reduction -= 0.5;
    }
  }
}

// druid_spell_t::execute ==================================================

void druid_spell_t::execute()
{
  druid_t* p = player -> cast_druid();

  spell_t::execute();

  if( result == RESULT_CRIT )
  {
    if( p -> buffs_moonkin_form )
    {
      p -> resource_gain( RESOURCE_MANA, p -> resource_max[ RESOURCE_MANA ] * 0.02, p -> gains_moonkin_form );
    }
    if( p -> buffs_natures_grace == 0 )
    {
      if( sim -> roll( p -> talents.natures_grace / 3.0 ) )
      {
        p -> aura_gain( "Natures Grace" );
        p -> buffs_natures_grace = 1;
        p -> buffs.cast_time_reduction += 0.5;
      }
    }
  }

  trigger_omen_of_clarity( this );

  if( p -> tiers.t4_2pc_balance && sim -> roll( 0.05 ) )
  {
    p -> resource_gain( RESOURCE_MANA, 120.0, p -> gains.tier4_2pc );
  }

}

// druid_spell_t::consume_resource =========================================

void druid_spell_t::consume_resource()
{
  druid_t* p = player -> cast_druid();
  spell_t::consume_resource();
  if( p -> buffs_omen_of_clarity )
  {
    // Treat the savings like a mana gain.
    double amount = spell_t::cost();
    if( amount > 0 )
    {
      p -> gains_omen_of_clarity -> add( amount );
      p -> buffs_omen_of_clarity = 0;
    }
  }
}

// druid_spell_t::player_buff ==============================================

void druid_spell_t::player_buff()
{
  druid_t* p = player -> cast_druid();
  spell_t::player_buff();
  if( p -> buffs.moonkin_aura )
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
  if( t -> debuffs.faerie_fire )
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

    base_cost = p -> resource_base[ RESOURCE_MANA ] * 0.07;
  }

  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    trigger_faerie_fire( this );
    trigger_improved_faerie_fire( this );
  }

  virtual bool ready() 
  {
    druid_t* p = player -> cast_druid();

    if( ! druid_spell_t::ready() )
      return false;

    if( ! sim -> target -> debuffs.faerie_fire )
      return true;

    if( ! sim -> target -> debuffs.improved_faerie_fire && p -> talents.improved_faerie_fire )
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
    druid_spell_t( "innervate", player, SCHOOL_NATURE, TREE_BALANCE ), trigger(0)
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

    base_cost = p -> resource_base[ RESOURCE_MANA ] * 0.04;
    base_execute_time = 0;
    cooldown  = 480;
    harmful   = false;
    if( p -> tiers.t4_4pc_balance ) cooldown -= 48.0;

    // If no target is set, assume we have innervate for ourself
    innervate_target = target_str.empty() ? p : sim -> find_player( target_str );
    assert ( innervate_target != 0 );    
  }
  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* p) : event_t( sim, p)
      {
        sim -> add_event( this, 20.0 );
      }
      virtual void execute()
      {
        player -> buffs.innervate = 0;
        player -> aura_loss("Innervate");
      }
    };
    struct expiration_glyph_t : public event_t
    {
      expiration_glyph_t( sim_t* sim, player_t* p) : event_t( sim, p)
      {
        sim -> add_event( this, 20.0 );
      }
      virtual void execute()
      {
        player -> buffs.glyph_of_innervate = 0;
        player -> aura_loss("Glyph of Innervate");
      }
    };
    
    consume_resource();
    update_ready();
    
    innervate_target -> buffs.innervate = 1;
    innervate_target -> aura_gain("Innervate");
    
    if( player -> cast_druid() -> glyphs.innervate )
    {
      player -> buffs.glyph_of_innervate = 1; 
      player -> aura_gain("Glyph of Innervate");
      new ( sim ) expiration_glyph_t( sim, player);
    }
    player -> action_finish( this );
    new ( sim ) expiration_t( sim, innervate_target);
  }

  virtual bool ready()
  {
    if( ! druid_spell_t::ready() )
      return false;

    return( player -> resource_max    [ RESOURCE_MANA ] - 
        player -> resource_current[ RESOURCE_MANA ] ) > trigger;
  }
};

// Insect Swarm Spell ======================================================

struct insect_swarm_t : public druid_spell_t
{
  int wrath_ready;
  action_t* active_wrath;

  insect_swarm_t( player_t* player, const std::string& options_str ) : 
    druid_spell_t( "insect_swarm", player, SCHOOL_NATURE, TREE_BALANCE ), wrath_ready(0), active_wrath(0)
  {
    druid_t* p = player -> cast_druid();
    assert( p -> talents.insect_swarm );

    option_t options[] =
    {
      { "wrath_ready", OPT_INT, &wrath_ready },
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
    init_rank( ranks );
     
    base_execute_time = 0;
    base_tick_time    = 2.0;
    num_ticks         = 6;
    tick_power_mod    = ( base_tick_time / 15.0 ) * 0.95;

    base_multiplier *= 1.0 + util_t::talent_rank(p -> talents.genesis, 5, 0.01) +
                            ( p -> glyphs.insect_swarm  ? 0.30 : 0.00 ) +
                            ( p -> tiers.t7_2pc_balance ? 0.10 : 0.00 );
   

    if( p -> talents.natures_splendor ) num_ticks++;

    observer = &( p -> active_insect_swarm );
  }
  virtual bool ready() 
  {
    if( ! druid_spell_t::ready() )
      return false;

    if( wrath_ready && ! active_wrath )
    {
      for( active_wrath = next; active_wrath; active_wrath = active_wrath -> next )
        if( active_wrath -> name_str == "wrath" )
          break;

      if( ! active_wrath ) wrath_ready = 0;
    }

    if( wrath_ready )
      if( ! active_wrath -> ready() )
        return false;

    return true;
  }
};

// Moonfire Spell ===========================================================

struct moonfire_t : public druid_spell_t
{
  moonfire_t( player_t* player, const std::string& options_str ) : 
    druid_spell_t( "moonfire", player, SCHOOL_ARCANE, TREE_BALANCE )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
    {
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
    init_rank( ranks );
     
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

    if( p -> glyphs.moonfire )
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
    if( p -> talents.natures_splendor ) num_ticks++;
    if( p -> tiers.t6_2pc_balance     ) num_ticks++;
    druid_spell_t::execute();
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
  }
   
  virtual void execute()
  {
    druid_t* d = player -> cast_druid();
    if( sim -> log ) report_t::log( sim, "%s performs %s", d -> name(),name() );

    weapon_t* w = &( d -> main_hand_weapon );

    if( w -> type != WEAPON_BEAST )
    {
      w -> type = WEAPON_BEAST;
      w -> school = SCHOOL_PHYSICAL;
      w -> damage = 55.0;
      w -> swing_time = 1.0;

      d -> melee_attack -> cancel(); // Force melee swing to restart if necessary
    }

    d -> buffs_cat_form = 1;

    for( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if( d -> talents.leader_of_the_pack )
      {
        if( ! p -> sleeping ) p -> aura_gain( "Leader of the Pack" );
        p -> buffs.leader_of_the_pack = 1;
      }
    }
  }

  virtual bool ready()
  {
    druid_t* d = player -> cast_druid();
    return( d -> buffs_cat_form == 0 );
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
  }
   
  virtual void execute()
  {
    druid_t* d = player -> cast_druid();
    if( sim -> log ) report_t::log( sim, "%s performs %s", d -> name(), name() );

    d -> buffs_moonkin_form = 1;

    for( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if( ! p -> sleeping ) p -> aura_gain( "Moonkin Aura" );

      p -> buffs.moonkin_aura = 1;

      if( d -> talents.improved_moonkin_form )
      {
        p -> buffs.improved_moonkin_aura = 1;
      }
    }
  }

  virtual bool ready()
  {
    druid_t* d = player -> cast_druid();
    return( d -> buffs_moonkin_form == 0 );
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
    if( ! options_str.empty() )
    {
      // This will prevent Natures Swiftness from being called before the desired "free spell" is ready to be cast.
      cooldown_group = options_str;
      duration_group = options_str;
    }
  }
   
  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    druid_t* p = player -> cast_druid();
    p -> aura_gain( "Natures Swiftness" );
    p -> buffs_natures_swiftness = 1;
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

  starfire_t( player_t* player, const std::string& options_str ) : 
    druid_spell_t( "starfire", player, SCHOOL_ARCANE, TREE_BALANCE ), eclipse_benefit(0), eclipse_trigger(0), extend_moonfire(0)
  {
    druid_t* p = player -> cast_druid();

    std::string eclipse_str;
    option_t options[] =
    {
      { "extendmf", OPT_INT,   &extend_moonfire },
      { "eclipse",  OPT_STRING, &eclipse_str     },
      { "prev",     OPT_STRING, &prev_str        },
      { NULL }
    };
    parse_options( options, options_str );

    if( ! eclipse_str.empty() )
    {
      eclipse_benefit = ( eclipse_str == "benefit" );
      eclipse_trigger = ( eclipse_str == "trigger" );
    }

    static rank_t ranks[] =
    {
      { 78, 10, 1028, 1212, 0, 0.16 },
      { 72,  9,  854, 1006, 0, 0.16 },
      { 67,  8,  818,  964, 0, 370  },
      { 60,  7,  693,  817, 0, 340  },
      { 0, 0 }
    };
    init_rank( ranks );
    
    base_execute_time = 3.5; 
    direct_power_mod  = ( base_execute_time / 3.5 ); 
    may_crit          = true;
      
    base_cost         *= 1.0 - util_t::talent_rank(p -> talents.moonglow, 3, 0.03);
    base_execute_time -= util_t::talent_rank(p -> talents.starlight_wrath, 5, 0.1);
    base_multiplier   *= 1.0 + util_t::talent_rank( p -> talents.moonfury, 3, 0.03, 0.06, 0.10 );
    base_crit         += util_t::talent_rank(p -> talents.natures_majesty, 2, 0.02);
    direct_power_mod  += util_t::talent_rank(p -> talents.wrath_of_cenarius, 5, 0.04);
    base_crit_bonus_multiplier *= 1.0 + util_t::talent_rank(p -> talents.vengeance, 5, 0.20);

    if ( p -> idols.shooting_star )
    {
      // Equip: Increases the spell power of your Starfire spell by 165.
      base_power += 165;
    }
    if( p -> tiers.t6_4pc_balance ) base_crit += 0.05;
    if( p -> tiers.t7_4pc_balance ) base_crit += 0.05;
  }

  virtual void player_buff()
  {
    druid_spell_t::player_buff();
    druid_t* p = player -> cast_druid();
    p -> uptimes_eclipse_starfire -> update( p -> buffs_eclipse_starfire != 0 );
    if( p -> buffs_eclipse_starfire )
    {
      player_crit += 0.30;
    }
    if( p -> active_moonfire )
    {
      player_crit += 0.01 * p -> talents.improved_insect_swarm;
    }
    if( p -> tiers.t5_4pc_balance )
    {
      if( p -> active_moonfire     ||
          p -> active_insect_swarm )
      {
        player_multiplier *= 1.10;
      }
    }
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    druid_spell_t::execute();
    if( result_is_hit() )
    {
      trigger_ashtongue_talisman( this );
      trigger_earth_and_moon( this );
      if( result == RESULT_CRIT )
      {
        trigger_eclipse_wrath( this );
      }

      if( p -> glyphs.starfire && p -> active_moonfire )
      {
        if ( p -> active_moonfire -> added_ticks < 3 )
          p -> active_moonfire -> extend_duration( 1 );
      }
    }
  }

  virtual bool ready()
  {
    if( ! druid_spell_t::ready() )
      return false;

    druid_t* p = player -> cast_druid();
 
    if ( extend_moonfire && p -> glyphs.starfire )
      if ( p -> active_moonfire -> added_ticks > 2 )
        return false;

    if( eclipse_benefit )
      if( ! sim -> time_to_think( p -> buffs_eclipse_starfire ) )
        return false;

    if( eclipse_trigger )
    {
      if( p -> talents.eclipse == 0 )
        return false;

      if( sim -> current_time + 1.5 < p -> cooldowns_eclipse )
        if( ! sim -> time_to_think( p -> buffs_eclipse_starfire ) )
          return false;
    }

    if( ! prev_str.empty() )
    {
      if( ! p -> last_foreground_action )
        return false;

      if( p -> last_foreground_action -> name_str != prev_str )
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
    druid_spell_t( "wrath", player, SCHOOL_NATURE, TREE_BALANCE ), eclipse_benefit(0), eclipse_trigger(0)
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

    if( ! eclipse_str.empty() )
    {
      eclipse_benefit = ( eclipse_str == "benefit" );
      eclipse_trigger = ( eclipse_str == "trigger" );
    }

    static rank_t ranks[] =
    {
      { 79, 12, 553, 623, 0, 0.11 },
      { 74, 11, 504, 568, 0, 0.11 },
      { 69, 10, 431, 485, 0, 255  },
      { 61,  9, 397, 447, 0, 210  },
      { 0, 0 }
    };
    init_rank( ranks );
    
    base_execute_time = 2.0; 
    direct_power_mod  = ( base_execute_time / 3.5 ); 
    may_crit          = true;
      
    base_cost         *= 1.0 - util_t::talent_rank(p -> talents.moonglow, 3, 0.03);
    base_execute_time -= util_t::talent_rank(p -> talents.starlight_wrath, 5, 0.1);
    base_multiplier   *= 1.0 + util_t::talent_rank(p -> talents.moonfury, 3, 0.03, 0.06, 0.10 );
    base_crit         += util_t::talent_rank(p -> talents.natures_majesty, 2, 0.02);
    direct_power_mod  += util_t::talent_rank(p -> talents.wrath_of_cenarius, 5, 0.02);
    base_crit_bonus_multiplier *= 1.0 + util_t::talent_rank(p -> talents.vengeance, 5, 0.20);

    if( p -> tiers.t7_4pc_balance ) base_crit += 0.05;

    if ( p -> idols.steadfast_renewal )
    {
      // Equip: Increases the damage dealt by Wrath by 70. 
      base_dd_min       += 70;
      base_dd_max       += 70;
    }
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    if( result_is_hit() )
    {
      if( result == RESULT_CRIT )
      {
        trigger_eclipse_starfire( this );
      }
      trigger_earth_and_moon( this );
    }
  }

  virtual void schedule_execute()
  {
    druid_t* p = player -> cast_druid();
    trigger_gcd = ( p -> buffs_natures_grace ) ? p -> base_gcd - 0.5 : p -> base_gcd;
    druid_spell_t::schedule_execute();
  }

  virtual void player_buff()
  {
    druid_spell_t::player_buff();
    druid_t* p = player -> cast_druid();
    p -> uptimes_eclipse_wrath -> update( p -> buffs_eclipse_wrath != 0 );
    if( p -> buffs_eclipse_wrath )
    {
      player_multiplier *= 1.20;
    }
    if( p -> active_insect_swarm )
    {
      player_multiplier *= 1.0 + p -> talents.improved_insect_swarm * 0.01;
    }
  }

  virtual bool ready()
  {
    if( ! druid_spell_t::ready() )
      return false;

    druid_t* p = player -> cast_druid();

    if( eclipse_benefit )
      if( ! sim -> time_to_think( p -> buffs_eclipse_wrath ) )
        return false;

    if( eclipse_trigger )
    {
      if( p -> talents.eclipse == 0 )
        return false;

      if( sim -> current_time + 3.0 < p -> cooldowns_eclipse )
        if( ! sim -> time_to_think( p -> buffs_eclipse_wrath ) )
          return false;
    }

    if( ! prev_str.empty() )
    {
      if( ! p -> last_foreground_action )
        return false;

      if( p -> last_foreground_action -> name_str != prev_str )
        return false;
    }

    return true;
  }
};

// Mark of the Wild Spell =====================================================

struct mark_of_the_wild_t : public druid_spell_t
{
  double bonus;

  mark_of_the_wild_t( player_t* player, const std::string& options_str ) : 
    druid_spell_t( "mark_of_the_wild", player, SCHOOL_NATURE, TREE_RESTORATION ), bonus(0)
  {
    druid_t* p = player -> cast_druid();

    trigger_gcd = 0;

    bonus = util_t::ability_rank( player -> level,  37.0,80, 14.0,70,  12.0,0 );

    bonus *= 1.0 + p -> talents.improved_mark_of_the_wild * 0.20;
  }
   
  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );

    for( player_t* p = sim -> player_list; p; p = p -> next )
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
    druid_spell_t( "treants", player, SCHOOL_NATURE, TREE_BALANCE ), target_pct(0)
  {
    druid_t* p = player -> cast_druid();
    assert( p -> talents.force_of_nature );

    option_t options[] =
    {
      { "target_pct", OPT_DEPRECATED, (void*) "health_percentage<" },
      { NULL }
    };
    parse_options( options, options_str );

    cooldown = 180.0;
    base_cost = p -> resource_base[ RESOURCE_MANA ] * 0.12;  }

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
    player -> action_finish( this );
    new ( sim ) treants_expiration_t( sim, player );
  }

  virtual bool ready()
  {
    if( ! druid_spell_t::ready() )
      return false;

    if( target_pct > 0 )
      if( sim -> target -> health_percentage() > target_pct )
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
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    enter_stealth( p );
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();
    return p -> buffs_stealthed == 0;
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
  if( name == "auto_attack"       ) return new       auto_attack_t( this, options_str );
  if( name == "berserk"           ) return new           berserk_t( this, options_str );
  if( name == "cat_form"          ) return new          cat_form_t( this, options_str );
  if( name == "claw"              ) return new              claw_t( this, options_str );
  if( name == "faerie_fire"       ) return new       faerie_fire_t( this, options_str );
  if( name == "faerie_fire_feral" ) return new faerie_fire_feral_t( this, options_str );
  if( name == "insect_swarm"      ) return new      insect_swarm_t( this, options_str );
  if( name == "innervate"         ) return new         innervate_t( this, options_str );
  if( name == "mangle_cat"        ) return new        mangle_cat_t( this, options_str );
  if( name == "mark_of_the_wild"  ) return new  mark_of_the_wild_t( this, options_str );
  if( name == "moonfire"          ) return new          moonfire_t( this, options_str );
  if( name == "moonkin_form"      ) return new      moonkin_form_t( this, options_str );
  if( name == "natures_swiftness" ) return new  druids_swiftness_t( this, options_str );
  if( name == "rake"              ) return new              rake_t( this, options_str );
  if( name == "rip"               ) return new               rip_t( this, options_str );
  if( name == "savage_roar"       ) return new       savage_roar_t( this, options_str );
  if( name == "shred"             ) return new             shred_t( this, options_str );
  if( name == "starfire"          ) return new          starfire_t( this, options_str );
  if( name == "stealth"           ) return new           stealth_t( this, options_str );
  if( name == "tigers_fury"       ) return new       tigers_fury_t( this, options_str );
  if( name == "treants"           ) return new     treants_spell_t( this, options_str );
  if( name == "wrath"             ) return new             wrath_t( this, options_str );
#if 0
  if( name == "cower"             ) return new             cower_t( this, options_str );
  if( name == "ferocious_bite"    ) return new    ferocious_bite_t( this, options_str );
  if( name == "maim"              ) return new              maim_t( this, options_str );
  if( name == "prowl"             ) return new             prowl_t( this, options_str );
  if( name == "ravage"            ) return new            ravage_t( this, options_str );
  if( name == "swipe_cat"         ) return new         swipe_cat_t( this, options_str );
#endif

  return player_t::create_action( name, options_str );
}

// druid_t::create_pet ======================================================

pet_t* druid_t::create_pet( const std::string& pet_name )
{
  pet_t* p = find_pet( pet_name );

  if( p ) return p;

  if( pet_name == "treants" ) return new treants_pet_t( sim, this, pet_name );

  return 0;
}


// druid_t::init_base =======================================================

void druid_t::init_base()
{
  attribute_base[ ATTR_STRENGTH  ] =  81;
  attribute_base[ ATTR_AGILITY   ] =  65;
  attribute_base[ ATTR_STAMINA   ] =  85;
  attribute_base[ ATTR_INTELLECT ] = 115;
  attribute_base[ ATTR_SPIRIT    ] = 135;

  if( talents.moonkin_form && talents.furor )
  {
    attribute_multiplier_initial[ ATTR_INTELLECT ] *= 1.0 + talents.furor * 0.02;
  }

  base_spell_crit = 0.0185;
  initial_spell_crit_per_intellect = rating_t::interpolate( level, 0.01/60.0, 0.01/80.0, 0.01/166.6 );
  initial_spell_power_per_intellect = talents.lunar_guidance * 0.04;
  initial_spell_power_per_spirit = ( talents.improved_moonkin_form * 0.05 );

  base_attack_power = ( level * 2 ) - 20;
  base_attack_crit  = 0.01;
  initial_attack_power_per_agility  = 1.0;
  initial_attack_power_per_strength = 2.0;
  initial_attack_crit_per_agility = rating_t::interpolate( level, 0.01/25.0, 0.01/40.0, 0.01/83.3 );

  // FIXME! Make this level-specific.
  resource_base[ RESOURCE_HEALTH ] = 3600;
  resource_base[ RESOURCE_MANA   ] = rating_t::interpolate( level, 1103, 2090, 3796 );
  resource_base[ RESOURCE_ENERGY ] = 100;

  health_per_stamina      = 10;
  mana_per_intellect      = 15;
  energy_regen_per_second = 10;

  spirit_regen_while_casting = util_t::talent_rank(talents.intensity,  3, 0.10);
  mp5_per_intellect          = util_t::talent_rank(talents.dreamstate, 3, 0.04, 0.07, 0.10);

  if ( talents.primal_precision )
  {
    base_attack_expertise += talents.primal_precision * 0.05;
  }

  if ( talents.heart_of_the_wild )
  {
    attack_power_multiplier *= 1 + 0.02 * talents.heart_of_the_wild;
  }
}

// druid_t::init_unique_gear ================================================

void druid_t::init_unique_gear()
{
  player_t::init_unique_gear();

  if( talents.moonkin_form )
  {
    if( gear.tier4_2pc ) tiers.t4_2pc_balance = 1;
    if( gear.tier4_4pc ) tiers.t4_4pc_balance = 1;
    if( gear.tier5_2pc ) tiers.t5_2pc_balance = 1;
    if( gear.tier5_4pc ) tiers.t5_4pc_balance = 1;
    if( gear.tier6_2pc ) tiers.t6_2pc_balance = 1;
    if( gear.tier6_4pc ) tiers.t6_4pc_balance = 1;
    if( gear.tier7_2pc ) tiers.t7_2pc_balance = 1;
    if( gear.tier7_4pc ) tiers.t7_4pc_balance = 1;
  }
  else
  {
    if( gear.tier4_2pc ) tiers.t4_2pc_feral = 1;
    if( gear.tier4_4pc ) tiers.t4_4pc_feral = 1;
    if( gear.tier5_2pc ) tiers.t5_2pc_feral = 1;
    if( gear.tier5_4pc ) tiers.t5_4pc_feral = 1;
    if( gear.tier6_2pc ) tiers.t6_2pc_feral = 1;
    if( gear.tier6_4pc ) tiers.t6_4pc_feral = 1;
    if( gear.tier7_2pc ) tiers.t7_2pc_feral = 1;
    if( gear.tier7_4pc ) tiers.t7_4pc_feral = 1;
  }
}

// druid_t::reset ===========================================================

void druid_t::reset()
{
  player_t::reset();

  // Spells
  active_insect_swarm = 0;
  active_moonfire     = 0;
  active_rip          = 0;

  // Buffs
  buffs_bear_form         = 0;
  buffs_cat_form          = 0;
  buffs_combo_points      = 0;
  buffs_eclipse_starfire  = 0;
  buffs_eclipse_wrath     = 0;
  buffs_moonkin_form      = 0;
  buffs_natures_grace     = 0;
  buffs_natures_swiftness = 0;
  buffs_omen_of_clarity   = 0;
  buffs_savage_roar       = 0;
  buffs_stealthed         = 0;
  buffs_tigers_fury       = 0;

  // Expirations
  expirations_eclipse     = 0;
  expirations_savage_roar = 0;

  // Cooldowns
  cooldowns_eclipse         = 0;
  cooldowns_omen_of_clarity = 0;
}

// druid_t::composite_attack_power ==========================================

double druid_t::composite_attack_power()
{
  double ap = player_t::composite_attack_power();

  if( buffs_savage_roar ) ap *= 1.40;

  return ap;
}

// druid_t::composite_spell_hit =============================================

double druid_t::composite_spell_hit()
{
  double hit = player_t::composite_spell_hit();

  if( talents.balance_of_power )
  {
    hit += talents.balance_of_power * 0.02;
  }

  return hit;
}

// druid_t::composite_spell_crit ============================================

double druid_t::composite_spell_crit()
{
  double crit = player_t::composite_spell_crit();

  if( talents.natural_perfection )
  {
    crit += talents.natural_perfection * 0.01;
  }

  return crit;
}

double druid_t::composite_attribute_multiplier( int attr )
{
  double a = player_t::composite_attribute_multiplier( attr );
  
  if ( talents.survival_of_the_fittest )
  {
    a *= 0.02 * talents.survival_of_the_fittest;
  }

  return a;
}

double druid_t::composite_attack_crit()
{
  double c = player_t::composite_attack_crit();

  if ( talents.sharpened_claws ) 
  {
    c += 0.02 * talents.sharpened_claws;
  }

  if ( talents.master_shapeshifter )
  {
    c += 0.02 * talents.master_shapeshifter;
  }

  return c;
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
    { {  0, NULL                               }, { 29, &( talents.berserk                 ) }, {  0, NULL                                   } },
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
    // Glyphs
    { "glyph_insect_swarm",        OPT_INT,  &( glyphs.insect_swarm               ) },
    { "glyph_innervate",           OPT_INT,  &( glyphs.innervate                  ) },
    { "glyph_mangle",              OPT_INT,  &( glyphs.mangle                     ) },
    { "glyph_moonfire",            OPT_INT,  &( glyphs.moonfire                   ) },
    { "glyph_rip",                 OPT_INT,  &( glyphs.rip                        ) },
    { "glyph_starfire",            OPT_INT,  &( glyphs.starfire                   ) },
    { "glyph_starfall",            OPT_INT,  &( glyphs.starfall                   ) },
    // Idols
    { "idol_of_the_ravenous_beast", OPT_INT,  &( idols.ravenous_beast             ) },
    { "idol_of_steadfast_renewal",  OPT_INT,  &( idols.steadfast_renewal          ) },
    { "idol_of_the_shooting_star",  OPT_INT,  &( idols.shooting_star              ) },
    { "idol_of_worship",            OPT_INT,  &( idols.worship                    ) },
    // Tier Bonuses
    { "tier4_2pc_balance",       OPT_INT,  &( tiers.t4_2pc_balance                ) },
    { "tier4_4pc_balance",       OPT_INT,  &( tiers.t4_4pc_balance                ) },
    { "tier5_2pc_balance",       OPT_INT,  &( tiers.t5_2pc_balance                ) },
    { "tier5_4pc_balance",       OPT_INT,  &( tiers.t5_4pc_balance                ) },
    { "tier6_2pc_balance",       OPT_INT,  &( tiers.t6_2pc_balance                ) },
    { "tier6_4pc_balance",       OPT_INT,  &( tiers.t6_4pc_balance                ) },
    { "tier7_2pc_balance",       OPT_INT,  &( tiers.t7_2pc_balance                ) },
    { "tier7_4pc_balance",       OPT_INT,  &( tiers.t7_4pc_balance                ) },
    { "tier4_2pc_feral",         OPT_INT,  &( tiers.t4_2pc_feral                  ) },
    { "tier4_4pc_feral",         OPT_INT,  &( tiers.t4_4pc_feral                  ) },
    { "tier5_2pc_feral",         OPT_INT,  &( tiers.t5_2pc_feral                  ) },
    { "tier5_4pc_feral",         OPT_INT,  &( tiers.t5_4pc_feral                  ) },
    { "tier6_2pc_feral",         OPT_INT,  &( tiers.t6_2pc_feral                  ) },
    { "tier6_4pc_feral",         OPT_INT,  &( tiers.t6_4pc_feral                  ) },
    { "tier7_2pc_feral",         OPT_INT,  &( tiers.t7_2pc_feral                  ) },
    { "tier7_4pc_feral",         OPT_INT,  &( tiers.t7_4pc_feral                  ) },
    { NULL, OPT_UNKNOWN }
  };

  if( name.empty() )
  {
    player_t::parse_option( std::string(), std::string() );
    option_t::print( sim, options );
    return false;
  }

  if( option_t::parse( sim, options, name, value ) ) return true;

  return player_t::parse_option( name, value );
}

// player_t::create_druid  ==================================================

player_t* player_t::create_druid( sim_t*       sim, 
                                  std::string& name ) 
{
  druid_t* p = new druid_t( sim, name );

  new treants_pet_t( sim, p, "treants" );

  return p;
}

