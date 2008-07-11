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
  spell_t* moonfire_active;
  spell_t* insect_swarm_active;

  // Buffs
  int8_t buffs_eclipse_starfire;
  int8_t buffs_eclipse_wrath;
  int8_t buffs_natures_grace;
  int8_t buffs_natures_swiftness;

  // Expirations
  event_t* expirations_eclipse;

  // Up-Times
  uptime_t* uptimes_eclipse_starfire;
  uptime_t* uptimes_eclipse_wrath;

  struct talents_t
  {
    int8_t  balance_of_power;
    int8_t  dreamstate;
    int8_t  eclipse;
    int8_t  focused_starlight;
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
    int8_t  natures_fury;
    int8_t  natures_grace;
    int8_t  natures_swiftness;
    int8_t  natural_perfection;
    int8_t  starlight_wrath;
    int8_t  vengeance;
    int8_t  wrath_of_cenarius;
    
    talents_t() { memset( (void*) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  druid_t( sim_t* sim, std::string& name ) : player_t( sim, DRUID, name ) 
  {
    moonfire_active = 0;
    insect_swarm_active = 0;

    // Buffs
    buffs_eclipse_starfire = 0;
    buffs_eclipse_wrath = 0;
    buffs_natures_grace = 0;
    buffs_natures_swiftness = 0;

    // Expirations
    expirations_eclipse = 0;

    // Up-Times
    uptimes_eclipse_starfire = sim -> get_uptime( name + "_eclipse_starfire" );
    uptimes_eclipse_wrath    = sim -> get_uptime( name + "_eclipse_wrath"    );
  }

  // Character Definition
  virtual void      init_base();
  virtual void      reset();
  virtual void      parse_talents( const std::string& talent_string );
  virtual bool      parse_option ( const std::string& name, const std::string& value );
  virtual action_t* create_action( const std::string& name, const std::string& options );

  // Event Tracking
  virtual void regen();
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

  virtual double execute_time();
  virtual void   player_buff();

  // Passthru Methods
  virtual void execute()     { spell_t::execute();      }
  virtual void last_tick()   { spell_t::last_tick();    }
  virtual bool ready()       { return spell_t::ready(); }
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// stack_shadow_weaving =====================================================

static void stack_natures_fury( spell_t* s )
{
  druid_t* p = s -> player -> cast_druid();

  if( p -> talents.natures_fury == 0 ) return;

  struct natures_fury_expiration_t : public event_t
  {
    natures_fury_expiration_t( sim_t* sim ) : event_t( sim )
    {
      name = "Natures Fury Expiration";
      time = 12.01;
      sim -> add_event( this );
    }
    virtual void execute()
    {
      report_t::log( sim, "%s loses Natures Fury", sim -> target -> name() );
      sim -> target -> debuffs.natures_fury = 0;
      sim -> target -> expirations.natures_fury = 0;
    }
  };

  if( wow_random( p -> talents.natures_fury * 0.20 ) )
  {
    target_t* t = s -> sim -> target;

    if( t -> debuffs.natures_fury < 3 ) 
    {
      t -> debuffs.natures_fury++;
      report_t::log( s -> sim, "%s gains Natures Fury %d", t -> name(), t -> debuffs.natures_fury );
    }

    event_t*& e = t -> expirations.natures_fury;
    
    if( e )
    {
      e -> reschedule( s -> sim -> current_time + 12.0 );
    }
    else
    {
      e = new natures_fury_expiration_t( s -> sim );
    }
  }
}

// trigger_eclipse_wrath ===================================================

static void trigger_eclipse_wrath( spell_t* s )
{
  struct cooldown_t : public event_t
  {
    cooldown_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Eclipse Wrath Cooldown";
      time = 90.0;
      sim -> add_event( this );
    }
    virtual void execute()
    {
      druid_t* p = player -> cast_druid();
      p -> expirations_eclipse = 0;
    }
  };

  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, druid_t* p ) : event_t( sim, p )
    {
      name = "Eclipse Wrath Expiration";
      p -> aura_gain( "Eclipse Wrath" );
      p -> buffs_eclipse_wrath = 1;
      time = 30.0;
      sim -> add_event( this );
    }
    virtual void execute()
    {
      druid_t* p = player -> cast_druid();
      p -> aura_loss( "Eclipse Wrath" );
      p -> buffs_eclipse_wrath = 0;
      p -> expirations_eclipse = new cooldown_t( sim, p );;
    }
  };

  druid_t* p = s -> player -> cast_druid();

  if( p -> talents.eclipse && 
      p -> expirations_eclipse == 0 &&
      wow_random( p -> talents.eclipse * 0.20 ) )
  {
    p -> proc( "eclipse_wrath" );
    p -> expirations_eclipse = new expiration_t( s -> sim, p );
  }
}

// trigger_eclipse_starfire =================================================

static void trigger_eclipse_starfire( spell_t* s )
{
  struct cooldown_t : public event_t
  {
    cooldown_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Eclipse Starfire Cooldown";
      time = 90.0;
      sim -> add_event( this );
    }
    virtual void execute()
    {
      druid_t* p = player -> cast_druid();
      p -> expirations_eclipse = 0;
    }
  };

  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, druid_t* p ) : event_t( sim, p )
    {
      name = "Eclipse Starfire Expiration";
      p -> aura_gain( "Eclipse Starfire" );
      p -> buffs_eclipse_starfire = 1;
      time = 30.0;
      sim -> add_event( this );
    }
    virtual void execute()
    {
      druid_t* p = player -> cast_druid();
      p -> aura_loss( "Eclipse Starfire" );
      p -> buffs_eclipse_starfire = 0;
      p -> expirations_eclipse = new cooldown_t( sim, p );;
    }
  };

  druid_t* p = s -> player -> cast_druid();

  if( p -> talents.eclipse && 
      p -> expirations_eclipse == 0 &&
      wow_random( p -> talents.eclipse * 0.20 ) )
  {
    p -> proc( "eclipse_starfire" );
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
      time = 8.0;
      sim -> add_event( this );
    }
    virtual void execute()
    {
      player -> aura_loss( "Ashtongue Talisman" );
      player -> spell_power[ SCHOOL_MAX ] -= 150;
      player -> expirations.ashtongue_talisman = 0;
    }
  };

  player_t* p = s -> player;

  if( p -> gear.ashtongue_talisman && wow_random( 0.25 ) )
  {
    p -> proc( "ashtongue_talisman" );

    event_t*& e = p -> expirations.ashtongue_talisman;

    if( e )
    {
      e -> reschedule( s -> sim -> current_time + 8.0 );
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

// druid_spell_t::execute_time =============================================

double druid_spell_t::execute_time()
{
  druid_t* p = player -> cast_druid();
  if( p -> buffs_natures_swiftness ) return 0;
  return spell_t::execute_time();
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
}

// Innervate Spell =========================================================

struct innervate_t : public druid_spell_t
{
  innervate_t( player_t* player, const std::string& options_str ) : 
    druid_spell_t( "innervate", player, SCHOOL_NATURE, TREE_BALANCE )
  {
    base_cost = 300;
    cooldown  = 480;
    harmful   = false;
    
    if( player -> gear.tier4_4pc ) cooldown -= 48.0;
  }
  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
      {
	time = 20;
	sim -> add_event( this );
      }
      virtual void execute()
      {
	player -> buffs.innervate = 0;
      }
    };

    update_cooldowns();
    consume_resource();
    player -> buffs.innervate = 1;
    player -> action_finish( this );
    new expiration_t( sim, player );
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
      { 80, 7, 0, 0, 1050, 265 },
      { 70, 6, 0, 0,  792, 175 },
      { 60, 5, 0, 0,  594, 155 },
      { 50, 4, 0, 0,  432, 135 },
      { 40, 3, 0, 0,  300, 110 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );
     
    base_execute_time = 0; 
    base_duration     = 12.0; 
    num_ticks         = 6;
    dot_power_mod     = ( 12.0 / 15.0 );
    base_cost         = rank -> cost;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    if( result_is_hit() )
    {
      player -> cast_druid() -> insect_swarm_active = this;
    }
  }

  virtual void last_tick() 
  {
    druid_spell_t::last_tick(); 
    player -> cast_druid() -> insect_swarm_active = 0;
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
      { 80, 14, 406, 476, 800, 745 },
      { 75, 13, 347, 407, 684, 640 },
      { 70, 12, 305, 357, 600, 495 },
      { 64, 11, 220, 220, 444, 430 },
      { 58, 10, 189, 221, 384, 375 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );
     
    base_dot          = rank -> dot;
    base_execute_time = 0; 
    may_crit          = true;
    base_duration     = 12.0; 
    num_ticks         = 4;
    dd_power_mod      = 0.28; 
    dot_power_mod     = 0.52; 

    if( p -> gear.tier6_2pc )
    {
      double adjust = ( num_ticks + 1 ) / (double) num_ticks;

      base_dot      *= adjust;
      dot_power_mod *= adjust;
      base_duration *= adjust;
      num_ticks++;
    }

    base_cost        = rank -> cost;
    base_cost       *= 1.0 - p -> talents.moonglow * 0.03;
    base_multiplier *= 1.0 + p -> talents.moonfury * 0.02;
    base_multiplier *= 1.0 + p -> talents.improved_moonfire * 0.05;
    base_crit       += p -> talents.natural_perfection * 0.01;
    base_crit       += p -> talents.improved_moonfire * 0.05;
    base_hit        += p -> talents.balance_of_power * 0.02;
    base_crit_bonus *= 1.0 + p -> talents.vengeance * 0.20;
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
    trigger_gcd = false;
    base_execute_time = 0;
    base_cost = 0;
  }
   
  virtual void execute()
  {
    report_t::log( sim, "%s performs moonkin_form", player -> name() );
    for( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if( player -> party == p -> party )
      {
	p -> aura_gain( "Moonkin Aura" );
	p -> buffs.moonkin_aura = 0.05;

	if( player -> cast_druid() -> talents.improved_moonkin_form )
	{
	  p -> buffs.improved_moonkin_aura = 1;
	}
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
    trigger_gcd = false;  
    cooldown = 180.0;
    if( ! options_str.empty() )
    {
      // This will prevent InnerFocus from being called before the desired "free spell" is ready to be cast.
      cooldown_group = options_str;
      debuff_group   = options_str;
    }
  }
   
  virtual void execute()
  {
    report_t::log( sim, "%s performs natures_swiftness", player -> name() );
    druid_t* p = player -> cast_druid();
    p -> aura_gain( "Natures Swiftness" );
    p -> buffs_natures_swiftness = 1;
    cooldown_ready = sim -> current_time + cooldown;
  }
};

// Starfire Spell ===========================================================

struct starfire_t : public druid_spell_t
{
  int8_t eclipse_trigger;
  double hasted;

  starfire_t( player_t* player, const std::string& options_str ) : 
    druid_spell_t( "starfire", player, SCHOOL_ARCANE, TREE_BALANCE ), eclipse_trigger(0), hasted( 0 )
  {
    druid_t* p = player -> cast_druid();

    std::string eclipse_str;
    option_t options[] =
    {
      { "rank",    OPT_INT8,   &rank_index  },
      { "hasted",  OPT_FLT,    &hasted      },
      { "eclipse", OPT_STRING, &eclipse_str },
      { NULL }
    };
    parse_options( options, options_str );

    if( ! eclipse_str.empty() )
    {
      eclipse_trigger = ( eclipse_str == "trigger" );
    }

    static rank_t ranks[] =
    {
      { 80, 8, 661, 779, 0, 555 },
      { 73, 8, 554, 652, 0, 455 },
      { 66, 8, 540, 636, 0, 370 },
      { 60, 7, 496, 584, 0, 340 },
      { 58, 6, 445, 525, 0, 315 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );
    
    base_execute_time = 3.5; 
    dd_power_mod      = 1.0; 
    may_crit          = true;
      
    base_cost          = rank -> cost;
    base_cost         *= 1.0 - p -> talents.moonglow * 0.03;
    base_execute_time -= p -> talents.starlight_wrath * 0.1;
    base_multiplier   *= 1.0 + p -> talents.moonfury * 0.02;
    base_crit         += p -> talents.natural_perfection * 0.01;
    base_crit         += p -> talents.focused_starlight * 0.02;
    base_hit          += p -> talents.balance_of_power * 0.02;
    base_crit_bonus   *= 1.0 + p -> talents.vengeance * 0.20;
    dd_power_mod      += p -> talents.wrath_of_cenarius * 0.04;

    if( p -> gear.tier6_4pc ) base_crit += 0.05;
  }

  virtual void player_buff()
  {
    druid_spell_t::player_buff();
    druid_t* p = player -> cast_druid();
    p -> uptimes_eclipse_starfire -> update( p -> buffs_eclipse_starfire );
    if( p -> buffs_eclipse_starfire )
    {
      player_crit += 0.10;
    }
    if( p -> gear.tier5_4pc )
    {
      if( p -> moonfire_active     ||
	  p -> insect_swarm_active )
      {
	player_multiplier *= 1.10;
      }
    }
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    if( result_is_hit() )
    {
      trigger_ashtongue_talisman( this );
      trigger_eclipse_wrath( this );
      stack_natures_fury( this );
    }
  }

  virtual bool ready()
  {
    if( ! spell_t::ready() )
      return false;

    if( hasted > 0 ) 
      if( execute_time() > hasted || ! player -> time_to_think() )
	return false;

    if( eclipse_trigger )
    {
      druid_t* p = player -> cast_druid();

      if( p -> talents.eclipse == 0 )
	return false;

      if( p -> expirations_eclipse )
	if( p -> expirations_eclipse -> time > sim -> current_time + 1.5 )
	  return false;
    }

    return true;
  }
};

// Wrath Spell ==============================================================

struct wrath_t : public druid_spell_t
{
  int8_t eclipse_trigger;

  wrath_t( player_t* player, const std::string& options_str ) : 
    druid_spell_t( "wrath", player, SCHOOL_NATURE, TREE_BALANCE ), eclipse_trigger(0)
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
      eclipse_trigger = ( eclipse_str == "trigger" );
    }

    static rank_t ranks[] =
    {
      { 80, 12, 489, 551, 0, 380 },
      { 75, 11, 414, 466, 0, 320 },
      { 70, 10, 381, 429, 0, 255 },
      { 62,  9, 278, 312, 0, 210 },
      { 54,  8, 236, 264, 0, 180 },
      { 0, 0 }
    };
    rank = choose_rank( ranks );
    
    base_execute_time = 2.0; 
    dd_power_mod      = ( 1.5 / 3.5 ); 
    may_crit          = true;
      
    base_cost          = rank -> cost;
    base_cost         *= 1.0 - p -> talents.moonglow * 0.03;
    base_execute_time -= p -> talents.starlight_wrath * 0.1;
    base_multiplier   *= 1.0 + p -> talents.moonfury * 0.02;
    base_crit         += p -> talents.natural_perfection * 0.01;
    base_crit         += p -> talents.focused_starlight * 0.02;
    base_hit          += p -> talents.balance_of_power * 0.02;
    base_crit_bonus   *= 1.0 + p -> talents.vengeance * 0.20;
    dd_power_mod      += p -> talents.wrath_of_cenarius * 0.02;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    if( result_is_hit() )
    {
      trigger_eclipse_starfire( this );
      stack_natures_fury( this );
    }
  }

  virtual void player_buff()
  {
    druid_spell_t::player_buff();
    druid_t* p = player -> cast_druid();
    p -> uptimes_eclipse_wrath -> update( p -> buffs_eclipse_wrath );
    if( p -> buffs_eclipse_wrath )
    {
      player_multiplier *= 0.10;
    }
  }

  virtual bool ready()
  {
    if( ! spell_t::ready() )
      return false;

    if( eclipse_trigger )
    {
      druid_t* p = player -> cast_druid();

      if( p -> talents.eclipse == 0 )
	return false;

      if( p -> expirations_eclipse )
	if( p -> expirations_eclipse -> time > sim -> current_time + 3.0 )
	  return false;
    }

    return true;
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
    if( talents.natures_grace && ! buffs_natures_grace )
    {
      aura_gain( "Natures Grace" );
      buffs_natures_grace = 1;
      buffs.cast_time_reduction += 0.5;
    }
  }
}

// druid_t::spell_finish_event ================================================

void druid_t::spell_finish_event( spell_t* s )
{
  player_t::spell_finish_event( s );

  if( gear.tier4_2pc && wow_random( 0.05 ) )
  {
    resource_gain( RESOURCE_MANA, 120.0, "tier4_2pc" );
  }
}


// ==========================================================================
// Druid Character Definition
// ==========================================================================

// druid_t::create_action  ==================================================

action_t* druid_t::create_action( const std::string& name,
				  const std::string& options )
{
  if( name == "insect_swarm"      ) return new     insect_swarm_t( this, options );
  if( name == "innervate"         ) return new        innervate_t( this, options );
  if( name == "moonfire"          ) return new         moonfire_t( this, options );
  if( name == "moonkin_form"      ) return new     moonkin_form_t( this, options );
  if( name == "natures_swiftness" ) return new druids_swiftness_t( this, options );
  if( name == "starfire"          ) return new         starfire_t( this, options );
  if( name == "wrath"             ) return new            wrath_t( this, options );

  return 0;
}

// druid_t::init_base =======================================================

void druid_t::init_base()
{
  static bool AFTER_3_0_0 = sim -> patch.after( 3, 0, 0 );

  attribute_base[ ATTR_STRENGTH  ] =  81;
  attribute_base[ ATTR_AGILITY   ] =  65;
  attribute_base[ ATTR_STAMINA   ] =  85;
  attribute_base[ ATTR_INTELLECT ] = 115;
  attribute_base[ ATTR_SPIRIT    ] = 135;

  base_spell_crit = 0.0185;
  spell_crit_per_intellect = 0.01 / ( level + 10 );
  spell_power_per_intellect = talents.lunar_guidance * ( AFTER_3_0_0 ? 0.04 : 0.08 );

  base_attack_power = -20;
  base_attack_crit  = 0.01;
  attack_power_per_strength = 2.0;
  attack_crit_per_agility = 1.0 / ( 25 + ( level - 70 ) * 0.5 );

  // FIXME! Make this level-specific.
  resource_base[ RESOURCE_HEALTH ] = 3600;
  resource_base[ RESOURCE_MANA   ] = 2090;

  health_per_stamina = 10;
  mana_per_intellect = 15;
}

// druid_t::reset ===========================================================

void druid_t::reset()
{
  player_t::reset();

  moonfire_active = 0;
  insect_swarm_active = 0;

  buffs_eclipse_starfire = 0;
  buffs_eclipse_wrath = 0;
  buffs_natures_grace = 0;
  buffs_natures_swiftness = 0;

  expirations_eclipse = 0;
}

// druid_t::regen  ==========================================================

void druid_t::regen()
{
  double spirit_regen = spirit_regen_per_second() * 2.0;

  if( buffs.innervate )
  {
    spirit_regen *= 4.0;
  }
  else if( recent_cast() )
  {
    spirit_regen *= talents.intensity * 0.10;
  }

  double mp5_regen = ( mp5 + intellect() * talents.dreamstate * 0.04 ) / 2.5;

  resource_gain( RESOURCE_MANA, spirit_regen, "spirit_regen" );
  resource_gain( RESOURCE_MANA,    mp5_regen, "mp5_regen"    );
}

// druid_t::parse_talents =================================================

void druid_t::parse_talents( const std::string& talent_string )
{
  assert( talent_string.size() == 62 );
  
  talent_translation_t translation[] =
  {
    {  1,  &( talents.starlight_wrath    ) },
    {  5,  &( talents.focused_starlight  ) },
    {  6,  &( talents.improved_moonfire  ) },
    {  8,  &( talents.insect_swarm       ) },
    { 10,  &( talents.vengeance          ) },
    { 12,  &( talents.lunar_guidance     ) },
    { 13,  &( talents.natures_grace      ) },
    { 14,  &( talents.moonglow           ) },
    { 15,  &( talents.moonfury           ) },
    { 16,  &( talents.balance_of_power   ) },
    { 17,  &( talents.dreamstate         ) },
    { 18,  &( talents.moonkin_form       ) },
    { 20,  &( talents.wrath_of_cenarius  ) },
    { 48,  &( talents.intensity          ) },
    { 53,  &( talents.natures_swiftness  ) },
    { 58,  &( talents.living_spirit      ) },
    { 60,  &( talents.natural_perfection ) },
    { 0, NULL }
  };

  player_t::parse_talents( translation, talent_string );
}

// druid_t::parse_option  ==============================================

bool druid_t::parse_option( const std::string& name,
			    const std::string& value )
{
  option_t options[] =
  {
    { "balance_of_power",       OPT_INT8,  &( talents.balance_of_power      ) },
    { "dreamstate",             OPT_INT8,  &( talents.dreamstate            ) },
    { "eclipse",                OPT_INT8,  &( talents.eclipse               ) },
    { "focused_starlight",      OPT_INT8,  &( talents.focused_starlight     ) },
    { "improved_moonfire",      OPT_INT8,  &( talents.improved_moonfire     ) },
    { "improved_moonkin_form",  OPT_INT8,  &( talents.improved_moonkin_form ) },
    { "insect_swarm",           OPT_INT8,  &( talents.insect_swarm          ) },
    { "intensity",              OPT_INT8,  &( talents.intensity             ) },
    { "living_spirit",          OPT_INT8,  &( talents.living_spirit         ) },
    { "lunar_guidance",         OPT_INT8,  &( talents.lunar_guidance        ) },
    { "master_shapeshifter",    OPT_INT8,  &( talents.master_shapeshifter   ) },
    { "moonfury",               OPT_INT8,  &( talents.moonfury              ) },
    { "moonglow",               OPT_INT8,  &( talents.moonglow              ) },
    { "moonkin_form",           OPT_INT8,  &( talents.moonkin_form          ) },
    { "natural_perfection",     OPT_INT8,  &( talents.natural_perfection    ) },
    { "natures_fury",           OPT_INT8,  &( talents.natures_fury          ) },
    { "natures_grace",          OPT_INT8,  &( talents.natures_grace         ) },
    { "natures_swiftness",      OPT_INT8,  &( talents.natures_swiftness     ) },
    { "starlight_wrath",        OPT_INT8,  &( talents.starlight_wrath       ) },
    { "vengeance",              OPT_INT8,  &( talents.vengeance             ) },
    { "wrath_of_cenarius",      OPT_INT8,  &( talents.wrath_of_cenarius     ) },
    { NULL, OPT_UNKNOWN }
  };

  if( name.empty() )
  {
    player_t::parse_option( std::string(), std::string() );
    option_t::print( options );
    return false;
  }

  if( player_t::parse_option( name, value ) ) return true;

  return option_t::parse( options, name, value );
}

// player_t::create_druid  ==================================================

player_t* player_t::create_druid( sim_t*       sim, 
				  std::string& name ) 
{
  return new druid_t( sim, name );
}

