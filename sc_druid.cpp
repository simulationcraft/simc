// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

// ==========================================================================
// Druid
// ==========================================================================

struct druid_t : public player_t
{
  // Active
  action_t* active_moonfire;
  action_t* active_insect_swarm;

  // Buffs
  double buffs_eclipse_starfire;
  double buffs_eclipse_wrath;
  int8_t buffs_natures_grace;
  int8_t buffs_natures_swiftness;
  int8_t buffs_omen_of_clarity;

  // Expirations
  event_t* expirations_eclipse;

  // Cooldowns
  double cooldowns_eclipse;
  double cooldowns_omen_of_clarity;

  // Gains
  gain_t* gains_moonkin_form;
  gain_t* gains_omen_of_clarity;

  // Procs
  proc_t* procs_omen_of_clarity;

  // Up-Times
  uptime_t* uptimes_eclipse_starfire;
  uptime_t* uptimes_eclipse_wrath;

  struct talents_t
  {
    int8_t  balance_of_power;
    int8_t  brambles;
    int8_t  celestial_focus;
    int8_t  dreamstate;
    int8_t  earth_and_moon;
    int8_t  eclipse;
    int8_t  focused_starlight;
    int8_t  force_of_nature;
    int8_t  furor;
    int8_t  genesis;
    int8_t  improved_faerie_fire;
    int8_t  improved_insect_swarm;
    int8_t  improved_mark_of_the_wild;
    int8_t  improved_moonfire;
    int8_t  improved_moonkin_form;
    int8_t  insect_swarm;
    int8_t  intensity;
    int8_t  living_spirit;
    int8_t  lunar_guidance;
    int8_t  master_shapeshifter;
    int8_t  moonfury;
    int8_t  moonglow;
    int8_t  moonkin_form;
    int8_t  natural_perfection;
    int8_t  natures_grace;
    int8_t  natures_majesty;
    int8_t  natures_reach;
    int8_t  natures_splendor;
    int8_t  natures_swiftness;
    int8_t  omen_of_clarity;
    int8_t  starlight_wrath;
    int8_t  vengeance;
    int8_t  wrath_of_cenarius;
    
    talents_t() { memset( (void*) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int8_t blue_promises;
    int8_t innervate;
    int8_t insect_swarm;
    int8_t moonfire;
    int8_t starfire;
    glyphs_t() { memset( (void*) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  druid_t( sim_t* sim, std::string& name ) : player_t( sim, DRUID, name ) 
  {
    active_moonfire = 0;
    active_insect_swarm = 0;

    // Buffs
    buffs_eclipse_starfire  = 0;
    buffs_eclipse_wrath     = 0;
    buffs_natures_grace     = 0;
    buffs_natures_swiftness = 0;
    buffs_omen_of_clarity   = 0;

    // Expirations
    expirations_eclipse = 0;

    // Cooldowns
    cooldowns_eclipse         = 0;
    cooldowns_omen_of_clarity = 0;

    // Gains
    gains_moonkin_form    = get_gain( "moonkin_form"    );
    gains_omen_of_clarity = get_gain( "omen_of_clarity" );

    // Procs
    procs_omen_of_clarity = get_proc( "omen_of_clarity" );

    // Up-Times
    uptimes_eclipse_starfire = get_uptime( "eclipse_starfire" );
    uptimes_eclipse_wrath    = get_uptime( "eclipse_wrath"    );
  }

  // Character Definition
  virtual void      init_base();
  virtual void      reset();
  virtual double    composite_spell_hit();
  virtual double    composite_spell_crit();
  virtual bool      parse_talents( const std::string& talent_string, int encoding );
  virtual bool      parse_option ( const std::string& name, const std::string& value );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name );
  virtual int       primary_resource() { return RESOURCE_MANA; }

  // Event Tracking
  virtual void spell_start_event ( spell_t* );
  virtual void spell_hit_event   ( spell_t* );
  virtual void spell_finish_event( spell_t* );
};

// ==========================================================================
// Druid Spell
// ==========================================================================

struct druid_spell_t : public spell_t
{
  druid_spell_t( const char* n, player_t* p, int8_t s, int8_t t ) : 
    spell_t( n, p, RESOURCE_MANA, s, t ) {}

  virtual double cost();
  virtual double haste();
  virtual double execute_time();
  virtual void   consume_resource();
  virtual void   player_buff();
  virtual void   target_debuff( int8_t dmg_type );

  // Passthru Methods
  virtual void execute()          { spell_t::execute();          }
  virtual void schedule_execute() { spell_t::schedule_execute(); }
  virtual void last_tick()        { spell_t::last_tick();        }
  virtual bool ready()            { return spell_t::ready();     }
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
      base_direct_dmg = 1;
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
    main_hand_weapon.damage     = 100;
    main_hand_weapon.swing_time = 1.6;
  }
  virtual void init_base()
  {
    attribute_base[ ATTR_STRENGTH  ] = 153;
    attribute_base[ ATTR_AGILITY   ] = 108;
    attribute_base[ ATTR_STAMINA   ] = 280;
    attribute_base[ ATTR_INTELLECT ] = 133;

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

namespace { // ANONYMOUS NAMESPACE ==========================================

// trigger_omen_of_clarity ==================================================

static void trigger_omen_of_clarity( action_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if( p -> talents.omen_of_clarity == 0 ) return;

  if( a -> sim -> cooldown_ready( p -> cooldowns_omen_of_clarity ) && rand_t::roll( 0.06 ) )
  {
    p -> buffs_omen_of_clarity = 1;
    p -> procs_omen_of_clarity -> occur();
    p -> cooldowns_omen_of_clarity = a -> sim -> current_time;
  }
}

// trigger_earth_and_moon ===================================================

static void trigger_earth_and_moon( spell_t* s )
{
  druid_t* p = s -> player -> cast_druid();

  if( p -> talents.earth_and_moon == 0 ) return;

  struct earth_and_moon_expiration_t : public event_t
  {
    earth_and_moon_expiration_t( sim_t* sim ) : event_t( sim )
    {
      name = "Earth and Moon Expiration";
      if( sim -> log ) report_t::log( sim, "%s gains Earth and Moon", sim -> target -> name() );
      sim -> target -> debuffs.earth_and_moon = 13;
      sim -> add_event( this, 12.0 );
    }
    virtual void execute()
    {
      if( sim -> log ) report_t::log( sim, "%s loses Earth and Moon", sim -> target -> name() );
      sim -> target -> debuffs.earth_and_moon = 0;
      sim -> target -> expirations.earth_and_moon = 0;
    }
  };

  if( rand_t::roll( p -> talents.earth_and_moon * 0.20 ) )
  {
    event_t*& e = s -> sim -> target -> expirations.earth_and_moon;
    
    if( e )
    {
      e -> reschedule( 12.0 );
    }
    else
    {
      e = new earth_and_moon_expiration_t( s -> sim );
    }
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
      p -> cooldowns_eclipse = sim -> current_time + 40;
      sim -> add_event( this, 10.0 );
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

  if( p -> talents.eclipse && 
      s -> sim -> cooldown_ready( p -> cooldowns_eclipse ) &&
      rand_t::roll( p -> talents.eclipse * 0.20 ) )
  {
    p -> expirations_eclipse = new expiration_t( s -> sim, p );
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
      p -> cooldowns_eclipse = sim -> current_time + 40;
      sim -> add_event( this, 10.0 );
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

  if( p -> talents.eclipse && 
      s -> sim -> cooldown_ready( p -> cooldowns_eclipse ) &&
      rand_t::roll( p -> talents.eclipse * 1.0/3 ) )
  {
    p -> expirations_eclipse = new expiration_t( s -> sim, p );
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

  if( p -> gear.ashtongue_talisman && rand_t::roll( 0.25 ) )
  {
    p -> procs.ashtongue_talisman -> occur();

    event_t*& e = p -> expirations.ashtongue_talisman;

    if( e )
    {
      e -> reschedule( 8.0 );
    }
    else
    {
      e = new expiration_t( s -> sim, p );
    }
  }
}

} // ANONYMOUS NAMESPACE ===================================================

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
  if( p -> talents.celestial_focus ) h *= 1.0 / ( 1.0 + 0.03 );
  return h;
}

// druid_spell_t::execute_time =============================================

double druid_spell_t::execute_time()
{
  druid_t* p = player -> cast_druid();
  if( p -> buffs_natures_swiftness ) return 0;
  return spell_t::execute_time();
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
  spell_t::player_buff();
  druid_t* p = player -> cast_druid();
  if( p -> buffs.moonkin_aura )
  {
    player_multiplier *= 1.0 + p -> talents.master_shapeshifter * 0.02;
  }

  player_multiplier *= 1.0 + p -> talents.earth_and_moon * 0.01;
}

// druid_spell_t::target_debuff ============================================

void druid_spell_t::target_debuff( int8_t dmg_type )
{
  spell_t::target_debuff( dmg_type );

  target_t* t = sim -> target;
  target_crit += t -> debuffs.improved_faerie_fire * 0.01; 
}

// Faerie Fire Spell =======================================================

struct faerie_fire_t : public druid_spell_t
{
  int16_t armor_penetration;
  int8_t  bonus_hit;

  faerie_fire_t( player_t* player, const std::string& options_str ) : 
    druid_spell_t( "faerie_fire", player, SCHOOL_NATURE, TREE_BALANCE )
  {
    druid_t* p = player -> cast_druid();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 76, 6, 0, 0, 1260, 0.07 },
      { 66, 5, 0, 0,  610, 145  },
      { 54, 4, 0, 0,  505, 115  },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
     
    base_cost = rank -> cost;
    armor_penetration = (int16_t) rank -> tick;
    bonus_hit = p -> talents.improved_faerie_fire;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      faerie_fire_t* faerie_fire;
      expiration_t( sim_t* sim, player_t* p, faerie_fire_t* ff ) : event_t( sim, p ), faerie_fire( ff )
      {
	target_t* t = sim -> target;
	t -> armor -= faerie_fire -> armor_penetration;
	t -> debuffs.faerie_fire = 1;
	t -> debuffs.improved_faerie_fire = faerie_fire -> bonus_hit;
	sim -> add_event( this, 40.0 );
      }
      virtual void execute()
      {
	target_t* t = sim -> target;
	t -> armor += faerie_fire -> armor_penetration;
	t -> debuffs.faerie_fire = 0;
	t -> debuffs.improved_faerie_fire = 0;
	t -> expirations.faerie_fire = 0;
      }
    };

    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();

    target_t* t = sim -> target;
    if( t -> debuffs.faerie_fire )
    {
      // We are overriding an existing debuff......
      t -> expirations.faerie_fire -> execute();
      t -> expirations.faerie_fire -> canceled = 1;
    }
    t -> expirations.faerie_fire = new expiration_t( sim, player, this );
  }

  virtual bool ready() 
  {
    if( ! druid_spell_t::ready() )
      return false;

    if( ! sim -> target -> debuffs.faerie_fire )
      return true;

    return bonus_hit > sim -> target -> debuffs.improved_faerie_fire;
  }
};

// Innervate Spell =========================================================

struct innervate_t : public druid_spell_t
{
  int16_t trigger;

  innervate_t( player_t* player, const std::string& options_str ) : 
    druid_spell_t( "innervate", player, SCHOOL_NATURE, TREE_BALANCE ), trigger(0)
  {
    option_t options[] =
    {
      { "trigger", OPT_INT16, &trigger },
      { NULL }
    };
    parse_options( options, options_str );

    base_cost = 300;
    cooldown  = 480;
    harmful   = false;
    
    if( player -> gear.tier4_4pc ) cooldown -= 48.0;

    // FIXME! Innervate cannot be currently cast on other players.
    // FIXME! Glyph not supported.
  }
  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
      {
	sim -> add_event( this, 20.0 );
      }
      virtual void execute()
      {
	player -> buffs.innervate = 0;
      }
    };

    consume_resource();
    update_ready();
    player -> buffs.innervate = 1;
    player -> action_finish( this );
    new expiration_t( sim, player );
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
  insect_swarm_t( player_t* player, const std::string& options_str ) : 
    druid_spell_t( "insect_swarm", player, SCHOOL_NATURE, TREE_BALANCE )
  {
    druid_t* p = player -> cast_druid();
    assert( p -> talents.insect_swarm );

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
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
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
     
    base_execute_time = 0; 
    base_cost         = rank -> cost;
    base_tick_time    = 2.0;
    num_ticks         = 6;
    tick_power_mod    = base_tick_time / 15.0;

    base_multiplier *= 1.0 + p -> talents.genesis * 0.01;
    if( p -> talents.natures_splendor ) num_ticks++;
    if( p -> glyphs.insect_swarm ) base_multiplier *= 1.30;

    observer = &( p -> active_insect_swarm );
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
      { "rank", OPT_INT8, &rank_index },
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
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
     
    base_execute_time = 0; 
    base_tick_time    = 3.0; 
    num_ticks         = 4;
    direct_power_mod  = 0.28; 
    tick_power_mod    = 0.13; 
    may_crit          = true;

    base_cost        = rank -> cost;
    base_cost       *= 1.0 - p -> talents.moonglow * 0.03;
    base_multiplier *= 1.0 + ( p -> talents.moonfury          * 0.02 +
			       p -> talents.improved_moonfire * 0.05 +
			       p -> talents.genesis           * 0.01 );
    base_crit       += p -> talents.improved_moonfire * 0.05;
    base_crit_bonus *= 1.0 + p -> talents.vengeance * 0.20;

    observer = &( p -> active_moonfire );
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    num_ticks = 4;
    if( p -> talents.natures_splendor ) num_ticks++;
    if( p -> gear.tier6_2pc           ) num_ticks++;
    druid_spell_t::execute();
  }

  virtual double calculate_direct_damage()
  {
    druid_t* p = player -> cast_druid();
    spell_t::calculate_direct_damage();
    if( p -> glyphs.moonfire ) direct_dmg *= 0.10;
    return direct_dmg;
  }

  virtual double calculate_tick_damage()
  {
    druid_t* p = player -> cast_druid();
    spell_t::calculate_tick_damage();
    if( p -> glyphs.moonfire ) tick_dmg *= 1.75;
    return tick_dmg;
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
    if( sim -> log ) report_t::log( sim, "%s performs moonkin_form", player -> name() );
    for( player_t* p = sim -> player_list; p; p = p -> next )
    {
      p -> aura_gain( "Moonkin Aura" );
      p -> buffs.moonkin_aura = 1;

      if( player -> cast_druid() -> talents.improved_moonkin_form )
      {
	p -> buffs.improved_moonkin_aura = 1;
      }
    }
  }

  virtual bool ready()
  {
    return( player -> buffs.moonkin_aura == 0 );
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
  int8_t eclipse_benefit;
  int8_t eclipse_trigger;

  starfire_t( player_t* player, const std::string& options_str ) : 
    druid_spell_t( "starfire", player, SCHOOL_ARCANE, TREE_BALANCE ), eclipse_benefit(0), eclipse_trigger(0)
  {
    druid_t* p = player -> cast_druid();

    std::string eclipse_str;
    option_t options[] =
    {
      { "rank",    OPT_INT8,   &rank_index  },
      { "eclipse", OPT_STRING, &eclipse_str },
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
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 3.5; 
    direct_power_mod  = 1.0; 
    may_crit          = true;
      
    base_cost          = rank -> cost;
    base_cost         *= 1.0 - p -> talents.moonglow * 0.03;
    base_execute_time -= p -> talents.starlight_wrath * 0.1;
    base_multiplier   *= 1.0 + p -> talents.moonfury * 0.02;
    base_crit         += p -> talents.focused_starlight * 0.02;
    base_crit         += p -> talents.natures_majesty * 0.02;
    base_crit_bonus   *= 1.0 + p -> talents.vengeance * 0.20;
    direct_power_mod  += p -> talents.wrath_of_cenarius * 0.04;

    if( p -> gear.tier6_4pc ) base_crit += 0.05;
  }

  virtual void player_buff()
  {
    druid_spell_t::player_buff();
    druid_t* p = player -> cast_druid();
    p -> uptimes_eclipse_starfire -> update( p -> buffs_eclipse_starfire );
    if( p -> buffs_eclipse_starfire )
    {
      player_crit += 0.30;
    }
    if( p -> active_moonfire )
    {
      player_crit += 0.01 * p -> talents.improved_insect_swarm;
    }
    if( p -> gear.tier5_4pc )
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
      trigger_eclipse_wrath( this );
      trigger_earth_and_moon( this );

      if( p -> glyphs.starfire && p -> active_moonfire )
      {
	p -> active_moonfire -> extend_duration( 1 );
      }
    }
  }

  virtual bool ready()
  {
    if( ! druid_spell_t::ready() )
      return false;

    druid_t* p = player -> cast_druid();

    if( eclipse_benefit )
      if( ! sim -> time_to_think( p -> buffs_eclipse_starfire ) )
	return false;

    if( eclipse_trigger )
    {
      if( p -> talents.eclipse == 0 )
	return false;

      if( sim -> current_time + 1.5 < p -> cooldowns_eclipse )
	  return false;
    }

    return true;
  }
};

// Wrath Spell ==============================================================

struct wrath_t : public druid_spell_t
{
  int8_t eclipse_benefit;
  int8_t eclipse_trigger;

  wrath_t( player_t* player, const std::string& options_str ) : 
    druid_spell_t( "wrath", player, SCHOOL_NATURE, TREE_BALANCE ), eclipse_benefit(0), eclipse_trigger(0)
  {
    druid_t* p = player -> cast_druid();

    std::string eclipse_str;
    option_t options[] =
    {
      { "rank",    OPT_INT8,   &rank_index  },
      { "eclipse", OPT_STRING, &eclipse_str },
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
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 2.0; 
    direct_power_mod  = ( 1.5 / 3.5 ); 
    may_crit          = true;
      
    base_cost          = rank -> cost;
    base_cost         *= 1.0 - p -> talents.moonglow * 0.03;
    base_execute_time -= p -> talents.starlight_wrath * 0.1;
    base_multiplier   *= 1.0 + p -> talents.moonfury * 0.02;
    base_crit         += p -> talents.focused_starlight * 0.02;
    base_crit         += p -> talents.natures_majesty * 0.02;
    base_crit_bonus   *= 1.0 + p -> talents.vengeance * 0.20;
    direct_power_mod  += p -> talents.wrath_of_cenarius * 0.02;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    if( result_is_hit() )
    {
      trigger_eclipse_starfire( this );
      trigger_earth_and_moon( this );
    }
  }

  virtual void schedule_execute()
  {
    if( player -> cast_druid() -> buffs_natures_grace ) 
      trigger_gcd *= 0.5;
    druid_spell_t::schedule_execute();
    trigger_gcd = player -> base_gcd;
  }

  virtual void player_buff()
  {
    druid_spell_t::player_buff();
    druid_t* p = player -> cast_druid();
    p -> uptimes_eclipse_wrath -> update( p -> buffs_eclipse_wrath );
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
    trigger_gcd = 0;

    bonus = ( player -> level == 80 ) ? 37 :
            ( player -> level >= 70 ) ? 14 : 12;

    int8_t improved = player -> cast_druid() -> talents.improved_mark_of_the_wild;

    if( improved )
    {
      bonus *= 1.0 + improved * 0.20;
    }
  }
   
  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );

    double delta = bonus - player -> buffs.mark_of_the_wild;

    for( player_t* p = sim -> player_list; p; p = p -> next )
    {
      for( int i=0; i < ATTRIBUTE_MAX; i++ )
      {
	p -> attribute[ i ] += delta;
	p -> buffs.mark_of_the_wild = bonus;
      }
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

  int8_t target_pct;

  treants_spell_t( player_t* player, const std::string& options_str ) : 
    druid_spell_t( "treants", player, SCHOOL_NATURE, TREE_BALANCE ), target_pct(0)
  {
    druid_t* p = player -> cast_druid();
    assert( p -> talents.force_of_nature );

    option_t options[] =
    {
      { "target_pct", OPT_INT8, &target_pct },
      { NULL }
    };
    parse_options( options, options_str );

    cooldown = 180.0;
    base_cost = p -> resource_base[ RESOURCE_MANA ] * 0.12;  }

  virtual void execute() 
  {
    consume_resource();
    update_ready();
    player -> summon_pet( "treants" );
    player -> action_finish( this );
    new treants_expiration_t( sim, player );
  }

  virtual bool ready()
  {
    if( ! druid_spell_t::ready() )
      return false;

    target_t* t = sim -> target;

    if( target_pct == 0 )
      return true;

    if( t -> initial_health <= 0 )
      return false;

    return( ( t -> current_health / t -> initial_health ) < ( target_pct / 100.0 ) );
  }
};

// ============================================================================
// Druid Event Tracking
// ============================================================================

// druid_t::spell_start_event =================================================

void druid_t::spell_start_event( spell_t* s )
{
  player_t::spell_start_event( s );

  if( s -> base_execute_time > 0 )
  {
    if( buffs_natures_swiftness )
    {
      buffs_natures_swiftness = 0;
    }
    else if( buffs_natures_grace )
    {
      aura_loss( "Natures Grace" );
      buffs_natures_grace = 0;
      buffs.cast_time_reduction -= 0.5;
    }
  }
}

// druid_t::spell_hit_event ===================================================

void druid_t::spell_hit_event( spell_t* s )
{
  player_t::spell_hit_event( s );

  if( s -> result == RESULT_CRIT )
  {
    if( buffs.moonkin_aura )
    {
      resource_gain( RESOURCE_MANA, resource_max[ RESOURCE_MANA ] * 0.02, gains_moonkin_form );
    }
    if( ! buffs_natures_grace )
    {
      if( rand_t::roll( talents.natures_grace / 3.0 ) )
      {
	aura_gain( "Natures Grace" );
	buffs_natures_grace = 1;
	buffs.cast_time_reduction += 0.5;
      }
    }
  }
}

// druid_t::spell_finish_event ================================================

void druid_t::spell_finish_event( spell_t* s )
{
  player_t::spell_finish_event( s );

  trigger_omen_of_clarity( s );

  if( gear.tier4_2pc && rand_t::roll( 0.05 ) )
  {
    resource_gain( RESOURCE_MANA, 120.0, gains.tier4_2pc );
  }
}


// ==========================================================================
// Druid Character Definition
// ==========================================================================

// druid_t::create_action  ==================================================

action_t* druid_t::create_action( const std::string& name,
				  const std::string& options_str )
{
  if( name == "faerie_fire"       ) return new      faerie_fire_t( this, options_str );
  if( name == "insect_swarm"      ) return new     insect_swarm_t( this, options_str );
  if( name == "innervate"         ) return new        innervate_t( this, options_str );
  if( name == "mark_of_the_wild"  ) return new mark_of_the_wild_t( this, options_str );
  if( name == "moonfire"          ) return new         moonfire_t( this, options_str );
  if( name == "moonkin_form"      ) return new     moonkin_form_t( this, options_str );
  if( name == "natures_swiftness" ) return new druids_swiftness_t( this, options_str );
  if( name == "starfire"          ) return new         starfire_t( this, options_str );
  if( name == "treants"           ) return new    treants_spell_t( this, options_str );
  if( name == "wrath"             ) return new            wrath_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// druid_t::create_pet ======================================================

pet_t* druid_t::create_pet( const std::string& pet_name )
{
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

  base_attack_power = -20;
  base_attack_crit  = 0.01;
  initial_attack_power_per_strength = 2.0;
  initial_attack_crit_per_agility = rating_t::interpolate( level, 0.01/25.0, 0.01/40.0, 0.01/83.3 );

  // FIXME! Make this level-specific.
  resource_base[ RESOURCE_HEALTH ] = 3600;
  resource_base[ RESOURCE_MANA   ] = rating_t::interpolate( level, 1103, 2090, 3796 );

  health_per_stamina = 10;
  mana_per_intellect = 15;

  spirit_regen_while_casting = talents.intensity  * 0.10;
  mp5_per_intellect          = talents.dreamstate * 0.04;
}

// druid_t::reset ===========================================================

void druid_t::reset()
{
  player_t::reset();

  // Spells
  active_moonfire     = 0;
  active_insect_swarm = 0;

  // Buffs
  buffs_eclipse_starfire  = 0;
  buffs_eclipse_wrath     = 0;
  buffs_natures_grace     = 0;
  buffs_natures_swiftness = 0;
  buffs_omen_of_clarity   = 0;

  // Expirations
  expirations_eclipse = 0;

  // Cooldowns
  cooldowns_eclipse         = 0;
  cooldowns_omen_of_clarity = 0;
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

// druid_t::parse_talents =================================================

bool druid_t::parse_talents( const std::string& talent_string,
			     int                encoding )
{
  if( encoding == ENCODING_BLIZZARD )
  {
    if( talent_string.size() != 83 ) return false;

    talent_translation_t translation[] =
    {
      // Balance
      {  1,  &( talents.starlight_wrath           ) },
      {  2,  &( talents.genesis                   ) },
      {  3,  &( talents.moonglow                  ) },
      {  4,  &( talents.natures_majesty           ) },
      {  5,  &( talents.improved_moonfire         ) },
      {  6,  &( talents.brambles                  ) },
      {  7,  &( talents.natures_grace             ) },
      {  8,  &( talents.natures_splendor          ) },
      {  9,  &( talents.natures_reach             ) },
      { 10,  &( talents.vengeance                 ) },
      { 11,  &( talents.celestial_focus           ) },
      { 12,  &( talents.lunar_guidance            ) },
      { 13,  &( talents.insect_swarm              ) },
      { 14,  &( talents.improved_insect_swarm     ) },
      { 15,  &( talents.dreamstate                ) },
      { 16,  &( talents.moonfury                  ) },
      { 17,  &( talents.balance_of_power          ) },
      { 18,  &( talents.moonkin_form              ) },
      { 19,  &( talents.improved_moonkin_form     ) },
      { 20,  &( talents.improved_faerie_fire      ) },
      { 22,  &( talents.wrath_of_cenarius         ) },
      { 23,  &( talents.eclipse                   ) },
      { 25,  &( talents.force_of_nature           ) },
      { 27,  &( talents.earth_and_moon            ) },
      // Restoration
      { 58,  &( talents.improved_mark_of_the_wild ) },
      { 59,  &( talents.furor                     ) },
      { 64,  &( talents.intensity                 ) },
      { 65,  &( talents.omen_of_clarity           ) },
      { 66,  &( talents.master_shapeshifter       ) },
      { 69,  &( talents.natures_swiftness         ) },
      { 74,  &( talents.living_spirit             ) },
      { 76,  &( talents.natural_perfection        ) },
      { 0, NULL }
    };
    player_t::parse_talents( translation, talent_string );
  }
  else if( encoding == ENCODING_MMO )
  {
    if( talent_string.size() != 83 ) return false;

    talent_translation_t translation[] =
    {
      // Restoration
      { 30,  &( talents.improved_mark_of_the_wild ) },
      { 31,  &( talents.furor                     ) },
      { 36,  &( talents.intensity                 ) },
      { 37,  &( talents.omen_of_clarity           ) },
      { 38,  &( talents.master_shapeshifter       ) },
      { 41,  &( talents.natures_swiftness         ) },
      { 46,  &( talents.living_spirit             ) },
      { 58,  &( talents.natural_perfection        ) },
      // Balance
      { 56,  &( talents.starlight_wrath           ) },
      { 57,  &( talents.genesis                   ) },
      { 58,  &( talents.moonglow                  ) },
      { 59,  &( talents.natures_majesty           ) },
      { 60,  &( talents.improved_moonfire         ) },
      { 61,  &( talents.brambles                  ) },
      { 62,  &( talents.natures_grace             ) },
      { 63,  &( talents.natures_splendor          ) },
      { 64,  &( talents.natures_reach             ) },
      { 65,  &( talents.vengeance                 ) },
      { 66,  &( talents.celestial_focus           ) },
      { 67,  &( talents.lunar_guidance            ) },
      { 68,  &( talents.insect_swarm              ) },
      { 69,  &( talents.improved_insect_swarm     ) },
      { 70,  &( talents.dreamstate                ) },
      { 71,  &( talents.moonfury                  ) },
      { 72,  &( talents.balance_of_power          ) },
      { 73,  &( talents.moonkin_form              ) },
      { 74,  &( talents.improved_moonkin_form     ) },
      { 75,  &( talents.improved_faerie_fire      ) },
      { 77,  &( talents.wrath_of_cenarius         ) },
      { 78,  &( talents.eclipse                   ) },
      { 80,  &( talents.force_of_nature           ) },
      { 82,  &( talents.earth_and_moon            ) },
      { 0, NULL }
    };
    player_t::parse_talents( translation, talent_string );
  }
  else if( encoding == ENCODING_WOWHEAD )
  {
    return false;
  }
  else assert( 0 );

  return true;
}

// druid_t::parse_option  ==============================================

bool druid_t::parse_option( const std::string& name,
			    const std::string& value )
{
  option_t options[] =
  {
    { "balance_of_power",          OPT_INT8,  &( talents.balance_of_power          ) },
    { "brambles",                  OPT_INT8,  &( talents.brambles                  ) },
    { "celestial_focus",           OPT_INT8,  &( talents.celestial_focus           ) },
    { "dreamstate",                OPT_INT8,  &( talents.dreamstate                ) },
    { "earth_and_moon",            OPT_INT8,  &( talents.earth_and_moon            ) },
    { "eclipse",                   OPT_INT8,  &( talents.eclipse                   ) },
    { "focused_starlight",         OPT_INT8,  &( talents.focused_starlight         ) },
    { "force_of_nature",           OPT_INT8,  &( talents.force_of_nature           ) },
    { "furor",                     OPT_INT8,  &( talents.furor                     ) },
    { "genesis",                   OPT_INT8,  &( talents.genesis                   ) },
    { "improved_faerie_fire",      OPT_INT8,  &( talents.improved_faerie_fire      ) },
    { "improved_insect_swarm",     OPT_INT8,  &( talents.improved_insect_swarm     ) },
    { "improved_mark_of_the_wild", OPT_INT8,  &( talents.improved_mark_of_the_wild ) },
    { "improved_moonfire",         OPT_INT8,  &( talents.improved_moonfire         ) },
    { "improved_moonkin_form",     OPT_INT8,  &( talents.improved_moonkin_form     ) },
    { "insect_swarm",              OPT_INT8,  &( talents.insect_swarm              ) },
    { "intensity",                 OPT_INT8,  &( talents.intensity                 ) },
    { "living_spirit",             OPT_INT8,  &( talents.living_spirit             ) },
    { "lunar_guidance",            OPT_INT8,  &( talents.lunar_guidance            ) },
    { "master_shapeshifter",       OPT_INT8,  &( talents.master_shapeshifter       ) },
    { "moonfury",                  OPT_INT8,  &( talents.moonfury                  ) },
    { "moonglow",                  OPT_INT8,  &( talents.moonglow                  ) },
    { "moonkin_form",              OPT_INT8,  &( talents.moonkin_form              ) },
    { "natural_perfection",        OPT_INT8,  &( talents.natural_perfection        ) },
    { "natures_grace",             OPT_INT8,  &( talents.natures_grace             ) },
    { "natures_majesty",           OPT_INT8,  &( talents.natures_majesty           ) },
    { "natures_reach",             OPT_INT8,  &( talents.natures_reach             ) },
    { "natures_splendor",          OPT_INT8,  &( talents.natures_splendor          ) },
    { "natures_swiftness",         OPT_INT8,  &( talents.natures_swiftness         ) },
    { "omen_of_clarity",           OPT_INT8,  &( talents.omen_of_clarity           ) },
    { "starlight_wrath",           OPT_INT8,  &( talents.starlight_wrath           ) },
    { "vengeance",                 OPT_INT8,  &( talents.vengeance                 ) },
    { "wrath_of_cenarius",         OPT_INT8,  &( talents.wrath_of_cenarius         ) },
    // Glyphs
    { "glyph_blue_promises",       OPT_INT8,  &( glyphs.blue_promises              ) },
    { "glyph_innervate",           OPT_INT8,  &( glyphs.innervate                  ) },
    { "glyph_insect_swarm",        OPT_INT8,  &( glyphs.insect_swarm               ) },
    { "glyph_moonfire",            OPT_INT8,  &( glyphs.moonfire                   ) },
    { "glyph_starfire",            OPT_INT8,  &( glyphs.starfire                   ) },
    { NULL, OPT_UNKNOWN }
  };

  if( name.empty() )
  {
    player_t::parse_option( std::string(), std::string() );
    option_t::print( sim, options );
    return false;
  }

  if( player_t::parse_option( name, value ) ) return true;

  return option_t::parse( sim, options, name, value );
}

// player_t::create_druid  ==================================================

player_t* player_t::create_druid( sim_t*       sim, 
				  std::string& name ) 
{
  return new druid_t( sim, name );
}

