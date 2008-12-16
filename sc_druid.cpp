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
    int8_t innervate;
    int8_t insect_swarm;
    int8_t moonfire;
    int8_t starfire;
    glyphs_t() { memset( (void*) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  struct idols_t
  {
    int8_t shooting_star;
    int8_t steadfast_renewal;
    idols_t() { memset( (void*) this, 0x0, sizeof( idols_t ) ); }
  };
  idols_t idols;

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
    cooldowns_eclipse = 0;

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
  virtual bool      get_talent_trees( std::vector<int8_t*>& balance, std::vector<int8_t*>& feral, std::vector<int8_t*>& restoration );
  virtual bool      parse_talents_mmo( const std::string& talent_string );
  virtual bool      parse_option ( const std::string& name, const std::string& value );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name );
  virtual int       primary_resource() { return RESOURCE_MANA; }
};

namespace { // ANONYMOUS NAMESPACE ==========================================

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
  virtual void   execute();
  virtual void   schedule_execute();
  virtual void   consume_resource();
  virtual void   player_buff();
  virtual void   target_debuff( int8_t dmg_type );

  // Passthru Methods
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

  if( a -> sim -> rng -> roll( proc_chance ) )
  {
    p -> buffs_omen_of_clarity = 1;
    p -> procs_omen_of_clarity -> occur();
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
      sim -> target -> debuffs.earth_and_moon = (int8_t) util_t::talent_rank( p -> talents.earth_and_moon, 3, 4, 9, 13 );
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
      s -> sim -> roll( p -> talents.eclipse * 0.20 ) )
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
      s -> sim -> roll( p -> talents.eclipse * 1.0/3 ) )
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
    if( p -> buffs.moonkin_aura )
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

  if( p -> gear.tier4_2pc && sim -> roll( 0.05 ) )
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

void druid_spell_t::target_debuff( int8_t dmg_type )
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
  int16_t armor_penetration;
  int8_t  bonus_hit;

  faerie_fire_t( player_t* player, const std::string& options_str ) : 
    druid_spell_t( "faerie_fire", player, SCHOOL_NATURE, TREE_BALANCE )
  {
    druid_t* p = player -> cast_druid();

    base_execute_time = 0;
    base_cost         = p -> resource_base[ RESOURCE_MANA ] * 0.07;
    armor_penetration = (int16_t) util_t::ability_rank( p -> level,  1260,76,  610,66,  505,0 );
    bonus_hit         = p -> talents.improved_faerie_fire;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      faerie_fire_t* faerie_fire;
      expiration_t( sim_t* sim, player_t* p, faerie_fire_t* ff ) : event_t( sim, p ), faerie_fire( ff )
      {
	target_t* t = sim -> target;
	t -> debuffs.faerie_fire = faerie_fire -> armor_penetration;
	t -> debuffs.improved_faerie_fire = faerie_fire -> bonus_hit;
	sim -> add_event( this, 40.0 );
      }
      virtual void execute()
      {
	target_t* t = sim -> target;
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
    t -> expirations.faerie_fire = new ( sim ) expiration_t( sim, player, this );
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
  
  player_t* innervate_target;
  
  innervate_t( player_t* player, const std::string& options_str ) : 
    druid_spell_t( "innervate", player, SCHOOL_NATURE, TREE_BALANCE ), trigger(0)
  {
  	std::string target_str;
    option_t options[] =
    {
      { "trigger", OPT_INT16,  &trigger    },
      { "target",  OPT_STRING, &target_str },
      { NULL }
    };
    parse_options( options, options_str );

    base_cost = player -> resource_base[ RESOURCE_MANA ] * 0.04;
    cooldown  = 480;
    harmful   = false;
    if( player -> gear.tier4_4pc ) cooldown -= 48.0;

    // If no target is set, assume we have innervate for ourself
    innervate_target = sim -> find_player(  (target_str.empty()) ? player -> name() : target_str);
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
        player -> buffs.glyph_of_innervate = 0;
      }
    };
    
    consume_resource();
    update_ready();
    innervate_target -> buffs.innervate = 1;
    if (player -> cast_druid() -> glyphs.innervate)
    {
      player -> buffs.glyph_of_innervate = 1; 
    }
    player -> action_finish( this );
    new ( sim ) expiration_t( sim, player);
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
  int8_t wrath_ready;
  action_t* active_wrath;

  insect_swarm_t( player_t* player, const std::string& options_str ) : 
    druid_spell_t( "insect_swarm", player, SCHOOL_NATURE, TREE_BALANCE ), wrath_ready(0), active_wrath(0)
  {
    druid_t* p = player -> cast_druid();
    assert( p -> talents.insect_swarm );

    option_t options[] =
    {
      { "rank",        OPT_INT8, &rank_index  },
      { "wrath_ready", OPT_INT8, &wrath_ready },
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
                            (p -> glyphs.insect_swarm ? 0.30 : 0.00 ) +
                            (p -> gear.tier7_2pc      ? 0.10 : 0.00 );
   

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
    init_rank( ranks );
     
    base_execute_time = 0;
    base_tick_time    = 3.0; 
    num_ticks         = 4;
    direct_power_mod  = 0.15; 
    tick_power_mod    = 0.13; 
    may_crit          = true;

    base_cost       *= 1.0 - util_t::talent_rank( p -> talents.moonglow,          3, 0.03 );
    base_crit       += util_t::talent_rank( p -> talents.improved_moonfire,       2, 0.05 );
    base_crit_bonus *= 1.0 + util_t::talent_rank( p -> talents.vengeance,         5, 0.20 );

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
    if( p -> gear.tier6_2pc           ) num_ticks++;
    druid_spell_t::execute();
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
  std::string prev_str;

  starfire_t( player_t* player, const std::string& options_str ) : 
    druid_spell_t( "starfire", player, SCHOOL_ARCANE, TREE_BALANCE ), eclipse_benefit(0), eclipse_trigger(0)
  {
    druid_t* p = player -> cast_druid();

    std::string eclipse_str;
    option_t options[] =
    {
      { "rank",    OPT_INT8,   &rank_index  },
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
    base_crit_bonus   *= 1.0 + util_t::talent_rank(p -> talents.vengeance, 5, 0.20);
    direct_power_mod  += util_t::talent_rank(p -> talents.wrath_of_cenarius, 5, 0.04);

    if ( p -> idols.shooting_star )
    {
      // Equip: Increases the spell power of your Starfire spell by 165.
      base_power += 165;
    }
    if( p -> gear.tier6_4pc ) base_crit += 0.05;
    if( p -> gear.tier7_4pc ) base_crit += 0.05;
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
  int8_t eclipse_benefit;
  int8_t eclipse_trigger;
  std::string prev_str;

  wrath_t( player_t* player, const std::string& options_str ) : 
    druid_spell_t( "wrath", player, SCHOOL_NATURE, TREE_BALANCE ), eclipse_benefit(0), eclipse_trigger(0)
  {
    druid_t* p = player -> cast_druid();

    std::string eclipse_str;
    option_t options[] =
    {
      { "rank",    OPT_INT8,   &rank_index  },
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
    base_crit_bonus   *= 1.0 + util_t::talent_rank(p -> talents.vengeance, 5, 0.20);
    direct_power_mod  += util_t::talent_rank(p -> talents.wrath_of_cenarius, 5, 0.02);

    if( p -> gear.tier7_4pc ) base_crit += 0.05;

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
      trigger_eclipse_starfire( this );
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

    bonus = ( player -> level == 80 ) ? 37 :
            ( player -> level >= 70 ) ? 14 : 12;

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
    new ( sim ) treants_expiration_t( sim, player );
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

} // ANONYMOUS NAMESPACE ===================================================

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

// druid_t::get_talent_trees ===============================================

bool druid_t::get_talent_trees( std::vector<int8_t*>& balance,
				std::vector<int8_t*>& feral,
				std::vector<int8_t*>& restoration )
{
  talent_translation_t translation[][3] =
  {
    { {  1, &( talents.starlight_wrath       ) }, {  1, NULL }, {  1, &( talents.improved_mark_of_the_wild ) } },
    { {  2, &( talents.genesis               ) }, {  2, NULL }, {  2, NULL                                   } },
    { {  3, &( talents.moonglow              ) }, {  3, NULL }, {  3, &( talents.furor                     ) } },
    { {  4, &( talents.natures_majesty       ) }, {  4, NULL }, {  4, NULL                                   } },
    { {  5, &( talents.improved_moonfire     ) }, {  5, NULL }, {  5, NULL                                   } },
    { {  6, &( talents.brambles              ) }, {  6, NULL }, {  6, NULL                                   } },
    { {  7, &( talents.natures_grace         ) }, {  7, NULL }, {  7, &( talents.intensity                 ) } },
    { {  8, &( talents.natures_splendor      ) }, {  8, NULL }, {  8, &( talents.omen_of_clarity           ) } },
    { {  9, &( talents.natures_reach         ) }, {  9, NULL }, {  9, &( talents.master_shapeshifter       ) } },
    { { 10, &( talents.vengeance             ) }, { 10, NULL }, { 10, NULL                                   } },
    { { 11, &( talents.celestial_focus       ) }, { 11, NULL }, { 11, NULL                                   } },
    { { 12, &( talents.lunar_guidance        ) }, { 12, NULL }, { 12, &( talents.natures_swiftness         ) } },
    { { 13, &( talents.insect_swarm          ) }, { 13, NULL }, { 13, NULL                                   } },
    { { 14, &( talents.improved_insect_swarm ) }, { 14, NULL }, { 14, NULL                                   } },
    { { 15, &( talents.dreamstate            ) }, { 15, NULL }, { 15, NULL                                   } },
    { { 16, &( talents.moonfury              ) }, { 16, NULL }, { 16, NULL                                   } },
    { { 17, &( talents.balance_of_power      ) }, { 17, NULL }, { 17, &( talents.living_spirit             ) } },
    { { 18, &( talents.moonkin_form          ) }, { 18, NULL }, { 18, NULL                                   } },
    { { 19, &( talents.improved_moonkin_form ) }, { 19, NULL }, { 19, &( talents.natural_perfection        ) } },
    { { 20, &( talents.improved_faerie_fire  ) }, { 20, NULL }, { 20, NULL                                   } },
    { { 21, NULL                               }, { 21, NULL }, { 21, NULL                                   } },
    { { 22, &( talents.wrath_of_cenarius     ) }, { 22, NULL }, { 22, NULL                                   } },
    { { 23, &( talents.eclipse               ) }, { 23, NULL }, { 23, NULL                                   } },
    { { 24, NULL                               }, { 24, NULL }, { 24, NULL                                   } },
    { { 25, &( talents.force_of_nature       ) }, { 25, NULL }, { 25, NULL                                   } },
    { { 26, NULL                               }, { 26, NULL }, { 26, NULL                                   } },
    { { 27, &( talents.earth_and_moon        ) }, { 27, NULL }, {  0, NULL                                   } },
    { { 28, NULL                               }, { 28, NULL }, {  0, NULL                                   } },
    { {  0, NULL                               }, { 29, NULL }, {  0, NULL                                   } },
    { {  0, NULL                               }, {  0, NULL }, {  0, NULL                                   } },
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
    { "balance_of_power",          OPT_INT8,  &( talents.balance_of_power          ) },
    { "brambles",                  OPT_INT8,  &( talents.brambles                  ) },
    { "celestial_focus",           OPT_INT8,  &( talents.celestial_focus           ) },
    { "dreamstate",                OPT_INT8,  &( talents.dreamstate                ) },
    { "earth_and_moon",            OPT_INT8,  &( talents.earth_and_moon            ) },
    { "eclipse",                   OPT_INT8,  &( talents.eclipse                   ) },
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
    { "glyph_insect_swarm",        OPT_INT8,  &( glyphs.insect_swarm               ) },
    { "glyph_innervate",           OPT_INT8,  &( glyphs.innervate                  ) },
    { "glyph_moonfire",            OPT_INT8,  &( glyphs.moonfire                   ) },
    { "glyph_starfire",            OPT_INT8,  &( glyphs.starfire                   ) },
    // Idols
    { "idol_of_steadfast_renewal", OPT_INT8,  &( idols.steadfast_renewal           ) },
    { "idol_of_the_shooting_star", OPT_INT8,  &( idols.shooting_star               ) },
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

