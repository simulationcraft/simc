// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include <simcraft.h>

// ==========================================================================
// Druid
// ==========================================================================

struct shaman_t : public player_t
{
  // Totems
  spell_t* fire_totem;
  spell_t* air_totem;
  spell_t* water_totem;
  spell_t* earth_totem;

  // Active
  spell_t* active_flame_shock;
  spell_t* active_lightning_charge;

  // Buffs
  int8_t buffs_elemental_devastation;
  int8_t buffs_elemental_focus;
  int8_t buffs_elemental_mastery;
  int8_t buffs_flurry;
  int8_t buffs_lightning_charges;
  int8_t buffs_maelstrom_weapon;
  int8_t buffs_nature_vulnerability;
  int8_t buffs_nature_vulnerability_charges;
  int8_t buffs_natures_swiftness;
  int8_t buffs_shamanistic_focus;
  int8_t buffs_shamanistic_rage;

  // Cooldowns
  double cooldowns_windfury_weapon;

  // Expirations
  event_t* expirations_elemental_devastation;
  event_t* expirations_elemental_oath;
  event_t* expirations_maelstrom_weapon;
  event_t* expirations_nature_vulnerability;
  event_t* expirations_unleashed_rage;

  // Gains
  gain_t* gains_shamanistic_rage;

  // Procs
  proc_t* procs_lightning_overload;
  
  // Up-Times
  uptime_t* uptimes_elemental_devastation;
  uptime_t* uptimes_elemental_oath;
  uptime_t* uptimes_flurry;
  uptime_t* uptimes_nature_vulnerability;
  uptime_t* uptimes_unleashed_rage;
  
  // Auto-Attack
  attack_t* main_hand_attack;
  attack_t*  off_hand_attack;

  // Weapon Enchants
  attack_t* windfury_weapon_attack;
  spell_t*  flametongue_weapon_spell;

  struct talents_t
  {
    int8_t  ancestral_knowledge;
    int8_t  blessing_of_the_eternals;
    int8_t  call_of_flame;
    int8_t  call_of_thunder;
    int8_t  concussion;
    int8_t  convection;
    int8_t  dual_wield;
    int8_t  dual_wield_specialization;
    int8_t  elemental_devastation;
    int8_t  elemental_focus;
    int8_t  elemental_fury;
    int8_t  elemental_mastery;
    int8_t  elemental_oath;
    int8_t  elemental_precision;
    int8_t  elemental_weapons;
    int8_t  enhancing_totems;
    int8_t  feral_spirit;
    int8_t  flurry;
    int8_t  improved_shields;
    int8_t  improved_stormstrike;
    int8_t  improved_windfury_totem;
    int8_t  lava_flows;
    int8_t  lava_lash;
    int8_t  lightning_mastery;
    int8_t  lightning_overload;
    int8_t  maelstrom_weapon;
    int8_t  mana_tide_totem;
    int8_t  mental_dexterity;
    int8_t  mental_quickness;
    int8_t  natures_blessing;
    int8_t  natures_swiftness;
    int8_t  restorative_totems;
    int8_t  reverberation;
    int8_t  shamanistic_focus;
    int8_t  shamanistic_rage;
    int8_t  spirit_weapons;
    int8_t  static_shock;
    int8_t  stormstrike;
    int8_t  storm_earth_and_fire;
    int8_t  thundering_strikes;
    int8_t  thunderstorm;
    int8_t  tidal_mastery;
    int8_t  totem_of_wrath;
    int8_t  totemic_focus;
    int8_t  unrelenting_storm;
    int8_t  unleashed_rage;
    int8_t  weapon_mastery;
    
    talents_t() { memset( (void*) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int8_t blue_promises;
    int8_t earth_shock;
    int8_t flametongue_weapon;
    int8_t lightning_bolt;
    int8_t lightning_shield;
    int8_t mana_tide;
    int8_t stormstrike;
    int8_t strength_of_earth;
    int8_t totem_of_wrath;
    int8_t windfury_weapon;
    int8_t flame_shock;
    glyphs_t() { memset( (void*) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  shaman_t( sim_t* sim, std::string& name ) : player_t( sim, SHAMAN, name )
  {
    // Totems
    fire_totem  = 0;
    air_totem   = 0;
    water_totem = 0;
    earth_totem = 0;;

    // Active
    active_flame_shock      = 0;
    active_lightning_charge = 0;

    // Buffs
    buffs_elemental_devastation        = 0;
    buffs_elemental_focus              = 0;
    buffs_elemental_mastery            = 0;
    buffs_flurry                       = 0;
    buffs_lightning_charges            = 0;
    buffs_maelstrom_weapon             = 0;
    buffs_nature_vulnerability         = 0;
    buffs_nature_vulnerability_charges = 0;
    buffs_natures_swiftness            = 0;
    buffs_shamanistic_focus            = 0;
    buffs_shamanistic_rage             = 0;

    // Cooldowns
    cooldowns_windfury_weapon = 0;

    // Expirations
    expirations_elemental_devastation = 0;
    expirations_elemental_oath        = 0;
    expirations_maelstrom_weapon      = 0;
    expirations_nature_vulnerability  = 0;
    expirations_unleashed_rage        = 0;
  
    // Gains
    gains_shamanistic_rage = get_gain( "shamanistic_rage" );

    // Procs
    procs_lightning_overload = get_proc( "lightning_overload" );

    // Up-Times
    uptimes_elemental_devastation = get_uptime( "elemental_devastation"    );
    uptimes_elemental_oath        = get_uptime( "elemental_oath"           );
    uptimes_flurry                = get_uptime( "flurry"                   );
    uptimes_nature_vulnerability  = get_uptime( "nature_vulnerability"  );
    uptimes_unleashed_rage        = get_uptime( "unleashed_rage"           );
  
    // Auto-Attack
    main_hand_attack = 0;
    off_hand_attack  = 0;

    // Weapon Enchants
    windfury_weapon_attack   = 0;
    flametongue_weapon_spell = 0;
  }

  // Character Definition
  virtual void      init_base();
  virtual void      reset();
  virtual double    composite_attack_power();
  virtual double    composite_spell_power( int8_t school );
  virtual bool      parse_talents( const std::string& talent_string, int encoding );
  virtual bool      parse_option( const std::string& name, const std::string& value );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name );
  virtual int       primary_resource() { return RESOURCE_MANA; }
};

// ==========================================================================
// Shaman Attack
// ==========================================================================

struct shaman_attack_t : public attack_t
{
  shaman_attack_t( const char* n, player_t* player, int8_t s=SCHOOL_PHYSICAL, int8_t t=TREE_NONE ) : 
    attack_t( n, player, RESOURCE_MANA, s, t ) 
  {
    base_direct_dmg = 1;
    shaman_t* p = player -> cast_shaman();
    base_multiplier *= 1.0 + p -> talents.weapon_mastery * 0.1/3;
    base_crit += p -> talents.thundering_strikes * 0.01;
    if( p -> dual_wield() ) base_hit += p -> talents.dual_wield_specialization * 0.01;
  }

  virtual void execute();
  virtual void player_buff();
  virtual void assess_damage( double amount, int8_t dmg_type );
};

// ==========================================================================
// Shaman Spell
// ==========================================================================

struct shaman_spell_t : public spell_t
{
  shaman_spell_t( const char* n, player_t* p, int8_t s, int8_t t ) : 
    spell_t( n, p, RESOURCE_MANA, s, t ) 
  {
    shaman_t* shaman = p -> cast_shaman();
    base_crit += shaman -> talents.thundering_strikes * 0.01;
    base_crit += shaman -> talents.blessing_of_the_eternals * 0.02;
  }

  virtual double cost();
  virtual void   consume_resource();
  virtual double execute_time();
  virtual void   execute();
  virtual void   player_buff();
  virtual void   schedule_execute();
  virtual void   assess_damage( double amount, int8_t dmg_type );

  // Passthru Methods
  virtual double calculate_direct_damage() { return spell_t::calculate_direct_damage(); }
  virtual void last_tick()                 { spell_t::last_tick();                      }
  virtual bool ready()                     { return spell_t::ready();                   }
};

// ==========================================================================
// Pet Spirit Wolf
// ==========================================================================

struct spirit_wolf_pet_t : public pet_t
{
  struct melee_t : public attack_t
  {
    melee_t( player_t* player ) : 
      attack_t( "wolf_melee", player )
    {
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      base_direct_dmg = 1;
      background = true;
      repeating = true;

      // There are actually two wolves.....
      base_multiplier *= 2.0;
    }
  };

  melee_t* melee;

  spirit_wolf_pet_t( sim_t* sim, player_t* owner, const std::string& pet_name ) :
    pet_t( sim, owner, pet_name ), melee(0)
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.damage     = 310;
    main_hand_weapon.swing_time = 1.5;
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

namespace { // ANONYMOUS NAMESPACE ==========================================

// trigger_flametongue_weapon ===============================================

static void trigger_flametongue_weapon( attack_t* a )
{
  struct flametongue_weapon_spell_t : public shaman_spell_t
  {
    flametongue_weapon_spell_t( player_t* player ) :
      shaman_spell_t( "flametongue", player, SCHOOL_FIRE, TREE_ENHANCEMENT )
    {
      shaman_t* p = player -> cast_shaman();

      background       = true;
      proc             = true;
      may_crit         = true;
      trigger_gcd      = 0;
      direct_power_mod = 0.10;
      base_hit        += p -> talents.elemental_precision * 0.01;

      reset();
    }
  };

  if( a -> weapon &&
      a -> weapon -> buff == FLAMETONGUE )
  {
    shaman_t* p = a -> player -> cast_shaman();

    if( ! p -> flametongue_weapon_spell )
    {
      p -> flametongue_weapon_spell = new flametongue_weapon_spell_t( p );
    }

    double fire_dmg  = ( ( p -> level < 64 ) ? 30 : 
			 ( p -> level < 71 ) ? 35 : 
			 ( p -> level < 76 ) ? 60 : 
			 ( p -> level < 80 ) ? 70 : 80 );

    p -> flametongue_weapon_spell -> base_direct_dmg = fire_dmg * a -> weapon -> swing_time;
    p -> flametongue_weapon_spell -> execute();
  }
}

// trigger_windfury_weapon ================================================

static void trigger_windfury_weapon( attack_t* a )
{
  struct windfury_weapon_attack_t : public shaman_attack_t
  {
    windfury_weapon_attack_t( player_t* player ) :
      shaman_attack_t( "windfury", player )
    {
      shaman_t* p = player -> cast_shaman();
      may_glance  = false;
      background  = true;
      trigger_gcd = 0;
      base_multiplier *= 1.0 + p -> talents.elemental_weapons * 0.133333;
      reset();
    }
    virtual void player_buff()
    {
      shaman_attack_t::player_buff();
      player_power += weapon -> buff_bonus;
    }
  };

  shaman_t* p = a -> player -> cast_shaman();

  if( a -> weapon == 0 ) return;
  if( a -> weapon -> buff != WINDFURY ) return;

  if( ! a -> sim -> cooldown_ready( p -> cooldowns_windfury_weapon ) ) return;

  if( rand_t::roll( 0.20 ) )
  {
    if( ! p -> windfury_weapon_attack )
    {
      p -> windfury_weapon_attack = new windfury_weapon_attack_t( p );
    }
    p -> cooldowns_windfury_weapon = a -> sim -> current_time;

    p -> procs.windfury -> occur();
    p -> windfury_weapon_attack -> weapon = a -> weapon;
    p -> windfury_weapon_attack -> execute();
    p -> windfury_weapon_attack -> execute();
  }
}

// stack_maelstrom_weapon ==================================================

static void stack_maelstrom_weapon( attack_t* a )
{
  shaman_t* p = a -> player -> cast_shaman();

  if( p -> talents.maelstrom_weapon == 0 ) return;

  struct maelstrom_weapon_expiration_t : public event_t
  {
    maelstrom_weapon_expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
    {
      name = "Maelstrom Weapon Expiration";
      player -> aura_gain( "Maelstrom Weapon" );
      sim -> add_event( this, 30.0 );
    }
    virtual void execute()
    {
      shaman_t* p = player -> cast_shaman();
      p -> aura_loss( "Maelstrom Weapon" );
      p -> buffs_maelstrom_weapon = 0;
      p -> expirations_maelstrom_weapon = 0;
    }
  };

  if( rand_t::roll( p -> talents.maelstrom_weapon * 0.03 ) )
  {
    if( p -> buffs_maelstrom_weapon < 5 ) 
    {
      p -> buffs_maelstrom_weapon++;
      if( a -> sim -> log ) report_t::log( a -> sim, "%s gains Maelstrom Weapon %d", p -> name(), p -> buffs_maelstrom_weapon );
    }

    event_t*& e = p -> expirations_maelstrom_weapon;
    
    if( e )
    {
      e -> reschedule( 30.0 );
    }
    else
    {
      e = new maelstrom_weapon_expiration_t( a -> sim, p );
    }
  }
}

// trigger_unleashed_rage =================================================

static void trigger_unleashed_rage( attack_t* a )
{
  struct unleashed_rage_expiration_t : public event_t
  {
    unleashed_rage_expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
    {
      name = "Unleashed Rage Expiration";
      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
	if( p -> buffs.unleashed_rage == 0 ) p -> aura_gain( "Unleashed Rage" );
	p -> buffs.unleashed_rage++;
      }
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
	p -> buffs.unleashed_rage--;
	if( p -> buffs.unleashed_rage == 0 ) p -> aura_loss( "Unleashed Rage" );
      }
      player -> cast_shaman() -> expirations_unleashed_rage = 0;
    }
  };

  if( a -> proc ) return;

  shaman_t* p = a -> player -> cast_shaman();
  
  if( p -> talents.unleashed_rage == 0 ) return;

  event_t*& e = p -> expirations_unleashed_rage;

  if( e )
  {
    e -> reschedule( 10.0 );
  }
  else
  {
    e = new unleashed_rage_expiration_t( a -> sim, p );
  }
}

// trigger_nature_vulnerability =============================================

static void trigger_nature_vulnerability( attack_t* a )
{
  struct nature_vulnerability_expiration_t : public event_t
  {
    nature_vulnerability_expiration_t( sim_t* sim, shaman_t* p ) : event_t( sim, p )
    {
      name = "Nature Vulnerability Expiration";
      p -> aura_gain( "Nature Vulnerability" );
      p -> buffs_nature_vulnerability = p -> glyphs.stormstrike ? 28 : 20;
      sim -> add_event( this, 12.0 );
    }
    virtual void execute()
    {
      shaman_t* p = player -> cast_shaman();
      p -> aura_loss( "Nature Vulnerability" );
      p -> buffs_nature_vulnerability = 0;
      p -> buffs_nature_vulnerability_charges = 0;
      p -> expirations_nature_vulnerability = 0;
    }
  };

  shaman_t* p = a -> player -> cast_shaman();
  event_t*& e = p -> expirations_nature_vulnerability;

  p -> buffs_nature_vulnerability_charges = 2 + p -> talents.improved_stormstrike;
   
  if( e )
  {
    e -> reschedule( 12.0 );
  }
  else
  {
    e = new nature_vulnerability_expiration_t( a -> sim, p );
  }
}

// trigger_tier5_4pc ========================================================

static void trigger_tier5_4pc( spell_t* s )
{
  if( s -> result != RESULT_CRIT ) return;

  shaman_t* p = s -> player -> cast_shaman();

  if( p -> gear.tier5_4pc && rand_t::roll( 0.25 ) )
  {
    p -> resource_gain( RESOURCE_MANA, 120.0, p -> gains.tier5_4pc );
  }
}

// trigger_ashtongue_talisman ===============================================

static void trigger_ashtongue_talisman( spell_t* s )
{
  shaman_t* p = s -> player -> cast_shaman();

  if( p -> gear.ashtongue_talisman && rand_t::roll( 0.15 ) )
  {
    p -> resource_gain( RESOURCE_MANA, 110.0, p -> gains.ashtongue_talisman );
  }
}

// trigger_lightning_overload ===============================================

static void trigger_lightning_overload( spell_t* s,
					stats_t* lightning_overload_stats )
{
  shaman_t* p = s -> player -> cast_shaman();

  if( p -> talents.lightning_overload == 0 ) return;

  if( rand_t::roll( p -> talents.lightning_overload * 0.04 ) )
  {
    p -> procs_lightning_overload -> occur();

    double   cost        = s -> base_cost;
    double   multiplier  = s -> base_multiplier;
    stats_t* stats       = s -> stats;

    s -> base_cost        = 0;
    s -> base_multiplier /= 2.0;
    s -> stats            = lightning_overload_stats;

    s -> time_to_execute = 0;
    s -> execute();

    s -> base_cost       = cost;
    s -> base_multiplier = multiplier;
    s -> stats           = stats;
  }
}

// trigger_elemental_devastation =================================================

static void trigger_elemental_devastation( spell_t* s )
{
  shaman_t* p = s -> player -> cast_shaman();

  if( p -> talents.elemental_devastation == 0 ) return;

  if( s -> proc ) return;

  struct elemental_devastation_expiration_t : public event_t
  {
    elemental_devastation_expiration_t( sim_t* sim, shaman_t* p ) : event_t( sim, p )
    {
      name = "Elemental Devastation Expiration";
      p -> buffs_elemental_devastation = 1;
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      shaman_t* p = player -> cast_shaman();
      p -> buffs_elemental_devastation = 0;
      p -> expirations_elemental_devastation = 0;
    }
  };

  event_t*& e = p -> expirations_elemental_devastation;
    
  if( e )
  {
    e -> reschedule( 10.0 );
  }
  else
  {
    e = new elemental_devastation_expiration_t( s -> sim, p );
  }
}

// trigger_elemental_oath =====================================================

static void trigger_elemental_oath( spell_t* s )
{
  struct elemental_oath_expiration_t : public event_t
  {
    elemental_oath_expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
    {
      name = "Elemental Oath Expiration";
      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
	if( p -> buffs.elemental_oath == 0 ) p -> aura_gain( "Elemental Oath" );
	p -> buffs.elemental_oath++;
      }
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
	p -> buffs.elemental_oath--;
	if( p -> buffs.elemental_oath == 0 ) p -> aura_loss( "Elemental Oath" );
      }
      player -> cast_shaman() -> expirations_elemental_oath = 0;
    }
  };

  if( s -> proc ) return;

  shaman_t* p = s -> player -> cast_shaman();
  
  if( p -> talents.elemental_oath == 0 ) return;

  event_t*& e = p -> expirations_elemental_oath;
    
  if( e )
  {
    e -> reschedule( 15.0 );
  }
  else
  {
    e = new elemental_oath_expiration_t( s -> sim, p );
  }
}

} // ANONYMOUS NAMESPACE ===================================================

// =========================================================================
// Shaman Attack
// =========================================================================

// shaman_attack_t::execute ================================================

void shaman_attack_t::execute()
{
  shaman_t* p = player -> cast_shaman();

  attack_t::execute();

  if( result_is_hit() )
  {
    if( result == RESULT_CRIT )
    {
      trigger_unleashed_rage( this );
      if( p -> talents.flurry ) p -> buffs_flurry = 3;
    }
    if( p -> buffs_shamanistic_rage ) 
    {
      p -> resource_gain( RESOURCE_MANA, player_power * 0.30, p -> gains_shamanistic_rage );
    }
    trigger_flametongue_weapon( this );
    trigger_windfury_weapon( this );
    stack_maelstrom_weapon( this );
  }
}

// shaman_attack_t::player_buff ============================================

void shaman_attack_t::player_buff()
{
  attack_t::player_buff();
  shaman_t* p = player -> cast_shaman();
  if( p -> buffs_elemental_devastation ) 
  {
    player_crit += p -> talents.elemental_devastation * 0.03;
  }
  p -> uptimes_elemental_devastation -> update( p -> buffs_elemental_devastation );
  p -> uptimes_unleashed_rage        -> update( p -> buffs.unleashed_rage        );
}

// shaman_attack_t::assess_damage ==========================================

void shaman_attack_t::assess_damage( double amount, 
				     int8_t dmg_type )
{
  shaman_t* p = player -> cast_shaman();

  attack_t::assess_damage( amount, dmg_type );

  if( ! proc && num_ticks == 0 && p -> buffs_lightning_charges > 0 )
  {
    if( rand_t::roll( p -> talents.static_shock * 0.02 ) )
    {
      p -> buffs_lightning_charges--;
      p -> active_lightning_charge -> execute();
    }
  }
}

// =========================================================================
// Shaman Attacks
// =========================================================================

// Melee Attack ============================================================

struct melee_t : public shaman_attack_t
{
  melee_t( const char* name, player_t* player ) : 
    shaman_attack_t( name, player )
  {
    shaman_t* p = player -> cast_shaman();

    background  = true;
    repeating   = true;
    trigger_gcd = 0;
    base_cost   = 0;

    if( p -> dual_wield() ) base_hit -= 0.18;
  }

  virtual double execute_time()
  {
    double t = base_execute_time * haste();
    shaman_t* p = player -> cast_shaman();
    if( p -> buffs_flurry > 0 ) 
    {
      t *= 1.0 / ( 1.0 + 0.05 * p -> talents.flurry );
    }
    p -> uptimes_flurry -> update( p -> buffs_flurry > 0 );
    return t;
  }

  void schedule_execute()
  {
    attack_t::schedule_execute();
    shaman_t* p = player -> cast_shaman();
    if( p -> buffs_flurry > 0 ) p -> buffs_flurry--;
  }
};

// Auto Attack =============================================================

struct auto_attack_t : public shaman_attack_t
{
  auto_attack_t( player_t* player, const std::string& options_str ) : 
    shaman_attack_t( "auto_attack", player )
  {
    shaman_t* p = player -> cast_shaman();

    assert( p -> main_hand_weapon.type != WEAPON_NONE );
    p -> main_hand_attack = new melee_t( "melee_main_hand", player );
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    if( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      assert( p -> talents.dual_wield );
      p -> off_hand_attack = new melee_t( "melee_off_hand", player );
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
    }
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    p -> main_hand_attack -> schedule_execute();
    if( p -> off_hand_attack ) p -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();
    return( p -> main_hand_attack -> event == 0 ); // not swinging
  }
};

// Lava Lash Attack =========================================================

struct lava_lash_t : public shaman_attack_t
{
  lava_lash_t( player_t* player, const std::string& options_str ) : 
    shaman_attack_t( "lava_lash", player, SCHOOL_FIRE, TREE_ENHANCEMENT )
  {
    shaman_t* p = player -> cast_shaman();

    weapon     = &( player -> off_hand_weapon );
    may_glance = false;
    cooldown   = 6;
    base_cost  = p -> resource_base[ RESOURCE_MANA ] * 0.04;
  }

  virtual void player_buff()
  {
    shaman_attack_t::player_buff();
    if( weapon -> buff == FLAMETONGUE ) player_multiplier *= 1.25;
  }
};

// Stormstrike Attack =======================================================

struct stormstrike_t : public shaman_attack_t
{
  stormstrike_t( player_t* player, const std::string& options_str ) : 
    shaman_attack_t( "stormstrike", player, SCHOOL_PHYSICAL, TREE_ENHANCEMENT )
  {
    shaman_t* p = player -> cast_shaman();

    may_glance       = false;
    cooldown         = 10.0 - p -> talents.improved_stormstrike;
    base_cost        = p -> resource_base[ RESOURCE_MANA ] * 0.08;
  }

  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();

    double mh_dd=0, oh_dd=0;
    int8_t mh_result=RESULT_NONE, oh_result=RESULT_NONE;

    weapon = &( player -> main_hand_weapon );
    shaman_attack_t::execute();
    mh_dd = direct_dmg;
    mh_result = result;

    if( result_is_hit() )
    {
      trigger_nature_vulnerability( this );

      if( player -> off_hand_weapon.type != WEAPON_NONE )
      {
        weapon = &( player -> off_hand_weapon );
        shaman_attack_t::execute();
        oh_dd = direct_dmg;
        oh_result = result;
      }
    }

    direct_dmg = mh_dd + oh_dd;
    result     = mh_result;
    attack_t::update_stats( DMG_DIRECT );

    player -> action_finish( this );
  }
  virtual void update_stats( int8_t type ) { }
};

// =========================================================================
// Shaman Spell
// =========================================================================

// shaman_spell_t::cost ====================================================

double shaman_spell_t::cost()
{
  shaman_t* p = player -> cast_shaman();
  if( p -> buffs_elemental_mastery ) return 0;
  double c = spell_t::cost();
  if( p -> buffs_elemental_focus ) c *= 0.60;
  if( p -> buffs.tier4_4pc )
  {
    c -= 270;
    if( c < 0 ) c = 0;
  }
  return c;
}

// shaman_spell_t::consume_resource ========================================

void shaman_spell_t::consume_resource()
{
  spell_t::consume_resource();
  shaman_t* p = player -> cast_shaman();
  if( p -> buffs_elemental_focus > 0 ) 
  {
    p -> buffs_elemental_focus--;
  }
  p -> buffs.tier4_4pc = 0;
}

// shaman_spell_t::execute_time ============================================

double shaman_spell_t::execute_time()
{
  shaman_t* p = player -> cast_shaman();
  if( p -> buffs_natures_swiftness ) return 0;
  return spell_t::execute_time();
}

// shaman_spell_t::player_buff =============================================

void shaman_spell_t::player_buff()
{
  shaman_t* p = player -> cast_shaman();
  spell_t::player_buff();
  if( p -> buffs_elemental_mastery )
  {
    player_crit += 1.0;
  }
  if( p -> buffs_nature_vulnerability && school == SCHOOL_NATURE )
  {
    player_multiplier *= 1.0 + p -> buffs_nature_vulnerability * 0.01;
  }
  p -> uptimes_nature_vulnerability -> update( p -> buffs_nature_vulnerability );

  if( p -> glyphs.flametongue_weapon )
  {
    if( p -> main_hand_weapon.buff == FLAMETONGUE ||
	p ->  off_hand_weapon.buff == FLAMETONGUE ) 
    {
      player_crit += 0.02;
    }
  }

  if( p -> talents.elemental_oath ) p -> uptimes_elemental_oath -> update( p -> buffs.elemental_oath );
}

// shaman_spell_t::execute =================================================

void shaman_spell_t::execute()
{
  shaman_t* p = player -> cast_shaman();
  spell_t::execute();
  if( result_is_hit() )
  {
    if( result == RESULT_CRIT )
    {
      trigger_elemental_devastation( this );
      trigger_elemental_oath( this );

      p -> buffs_elemental_focus = 2;

      if( p -> gear.tier4_4pc )
      {
	p -> buffs.tier4_4pc = rand_t::roll( 0.11 ) ;
      }
    }
  }
  p -> buffs_elemental_mastery = 0;
}

// shaman_spell_t::schedule_execute ========================================

void shaman_spell_t::schedule_execute()
{
  spell_t::schedule_execute();

  if( time_to_execute > 0 )
  {
    shaman_t* p = player -> cast_shaman();
    if( p -> main_hand_attack ) p -> main_hand_attack -> cancel();
    if( p ->  off_hand_attack ) p ->  off_hand_attack -> cancel();
  }
}

// shaman_spell_t::assess_damage ============================================

void shaman_spell_t::assess_damage( double amount, 
				    int8_t dmg_type )
{
  shaman_t* p = player -> cast_shaman();

  spell_t::assess_damage( amount, dmg_type );

  if( dmg_type == DMG_DIRECT    &&
      school   == SCHOOL_NATURE && 
      p -> buffs_nature_vulnerability_charges > 0 )
  {
    p -> buffs_nature_vulnerability_charges--;
    if( p -> buffs_nature_vulnerability_charges == 0 )
    {
      event_t::early( p -> expirations_nature_vulnerability );
    }
  }

  if( ! proc && num_ticks == 0 && p -> buffs_lightning_charges > 0 )
  {
    if( rand_t::roll( p -> talents.static_shock * 0.02 ) )
    {
      p -> buffs_lightning_charges--;
      p -> active_lightning_charge -> execute();
    }
  }
}

// =========================================================================
// Shaman Spells
// =========================================================================

// Chain Lightning Spell ===================================================

struct chain_lightning_t : public shaman_spell_t
{
  int8_t maelstrom;
  stats_t* lightning_overload_stats;

  chain_lightning_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "chain_lightning", player, SCHOOL_NATURE, TREE_ELEMENTAL ), maelstrom(0), lightning_overload_stats(0)
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "rank",      OPT_INT8, &rank_index },
      { "maelstrom", OPT_INT8, &maelstrom  },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 8, 973, 1111, 0, 0.26 },
      { 74, 7, 806,  920, 0, 0.26 },
      { 70, 6, 734,  838, 0,  760 },
      { 63, 5, 603,  687, 0,  650 },
      { 56, 4, 493,  551, 0,  550 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );

    base_execute_time  = 2.0; 
    may_crit           = true;
    cooldown           = 6.0;
    direct_power_mod   = 0.7143;

    base_cost          = rank -> cost;
    cooldown          -= p -> talents.storm_earth_and_fire * 0.5;
    base_execute_time -= p -> talents.lightning_mastery * 0.1;
    base_cost         *= 1.0 - p -> talents.convection * 0.02;
    base_multiplier   *= 1.0 + p -> talents.concussion * 0.01;
    base_hit          += p -> talents.elemental_precision * 0.01;
    base_crit         += p -> talents.call_of_thunder * 0.05;
    base_crit         += p -> talents.tidal_mastery * 0.01;
    base_crit_bonus   *= 1.0 + p -> talents.elemental_fury * 0.20;

    lightning_overload_stats = p -> get_stats( "lightning_overload" );
    lightning_overload_stats -> school = SCHOOL_NATURE;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();
    event_t::early( player -> cast_shaman() -> expirations_maelstrom_weapon );
    if( result_is_hit() )
    {
      trigger_lightning_overload( this, lightning_overload_stats );
    }
  }

  virtual double execute_time()
  {
    double t = shaman_spell_t::execute_time();
    shaman_t* p = player -> cast_shaman();
    if( p -> buffs_maelstrom_weapon )
    {
      if( p -> buffs_maelstrom_weapon == 5 )
      {
	t = 0;
      }
      else
      {
	t *= ( 1.0 - p -> buffs_maelstrom_weapon * 0.20 );
      }
    }
    return t;
  }

  virtual bool ready()
  {
    if( ! shaman_spell_t::ready() )
      return false;

    if( maelstrom > 0 && 
        maelstrom > player -> cast_shaman() -> buffs_maelstrom_weapon )
      return false;

    return true;
  }
};

// Lightning Bolt Spell =====================================================

struct lightning_bolt_t : public shaman_spell_t
{
  int8_t maelstrom;
  stats_t* lightning_overload_stats;

  lightning_bolt_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "lightning_bolt", player, SCHOOL_NATURE, TREE_ELEMENTAL ), maelstrom(0), lightning_overload_stats(0)
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "rank",      OPT_INT8, &rank_index },
      { "maelstrom", OPT_INT8, &maelstrom  },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 79, 14, 715, 815, 0, 0.10 },
      { 73, 13, 595, 679, 0, 0.10 },
      { 67, 12, 563, 643, 0, 330  },
      { 62, 11, 495, 565, 0, 300  },
      { 56, 10, 419, 467, 0, 265  },
      { 50,  9, 347, 389, 0, 230  },
      { 44,  8, 282, 316, 0, 195  },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );

    base_execute_time  = 2.5; 
    may_crit           = true;
    direct_power_mod   = ( base_execute_time / 3.5 );
    base_cost          = rank -> cost;

    base_execute_time -= p -> talents.lightning_mastery * 0.1;
    base_cost         *= 1.0 - p -> talents.convection * 0.02;
    base_multiplier   *= 1.0 + p -> talents.concussion * 0.01;
    base_hit          += p -> talents.elemental_precision * 0.01;
    base_crit         += p -> talents.call_of_thunder * 0.05;
    base_crit         += p -> talents.tidal_mastery * 0.01;
    base_crit_bonus   *= 1.0 + p -> talents.elemental_fury * 0.20;

    if( p -> gear.tier6_4pc ) base_multiplier *= 1.05;
    if( p -> glyphs.lightning_bolt ) base_cost *= 0.90;

    lightning_overload_stats = p -> get_stats( "lightning_overload" );
    lightning_overload_stats -> school = SCHOOL_NATURE;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();
    event_t::early( player -> cast_shaman() -> expirations_maelstrom_weapon );
    if( result_is_hit() )
    {
      trigger_ashtongue_talisman( this );
      trigger_lightning_overload( this, lightning_overload_stats );
      trigger_tier5_4pc( this );
    }
  }

  virtual double execute_time()
  {
    double t = shaman_spell_t::execute_time();
    shaman_t* p = player -> cast_shaman();
    if( p -> buffs_maelstrom_weapon )
    {
      if( p -> buffs_maelstrom_weapon == 5 )
      {
	t = 0;
      }
      else
      {
	t *= ( 1.0 - p -> buffs_maelstrom_weapon * 0.20 );
      }
    }
    return t;
  }

  virtual bool ready()
  {
    if( ! shaman_spell_t::ready() )
      return false;

    if( maelstrom > 0 && 
	maelstrom > player -> cast_shaman() -> buffs_maelstrom_weapon )
      return false;

    return true;
  }
};

// Lava Burst Spell =========================================================

struct lava_burst_t : public shaman_spell_t
{
  int8_t flame_shock;
  int8_t max_ticks_consumed;
  int8_t maelstrom;

  lava_burst_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "lava_burst", player, SCHOOL_FIRE, TREE_ELEMENTAL ), flame_shock(0), max_ticks_consumed(0), maelstrom(0)
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "rank",               OPT_INT8, &rank_index         },
      { "flame_shock",        OPT_INT8, &flame_shock        },
      { "max_ticks_consumed", OPT_INT8, &max_ticks_consumed },
      { "maelstrom",          OPT_INT8, &maelstrom          },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 2, 1082, 1378, 0, 0.10 },
      { 75, 1,  920, 1172, 0, 0.10 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );

    base_execute_time  = 2.0; 
    direct_power_mod   = base_execute_time / 3.5;
    may_crit           = true;
    cooldown           = 8.0;

    base_cost          = rank -> cost;
    base_cost         *= 1.0 - p -> talents.convection * 0.02;
    base_execute_time -= p -> talents.lightning_mastery * 0.1;
    base_multiplier   *= 1.0 + p -> talents.concussion * 0.01;
    base_multiplier   *= 1.0 + p -> talents.call_of_flame * 0.02;
    base_hit          += p -> talents.elemental_precision * 0.01;
    base_crit_bonus   *= 1.0 + p -> talents.elemental_fury * 0.20;
    base_crit_bonus   *= 1.0 + p -> talents.lava_flows * 0.06;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();
    event_t::early( player -> cast_shaman() -> expirations_maelstrom_weapon );
    if( result_is_hit() )
    {
      shaman_t* p = player -> cast_shaman();
      if( p -> active_flame_shock )
      {
	p -> active_flame_shock -> cancel();
	p -> active_flame_shock = 0;
      }
    }
  }

  virtual double execute_time()
  {
    double t = shaman_spell_t::execute_time();
    shaman_t* p = player -> cast_shaman();
    if( p -> buffs_maelstrom_weapon )
    {
      if( p -> buffs_maelstrom_weapon == 5 )
      {
	t = 0;
      }
      else
      {
	t *= ( 1.0 - p -> buffs_maelstrom_weapon * 0.20 );
      }
    }
    return t;
  }

  virtual void player_buff()
  {
    shaman_spell_t::player_buff();
    shaman_t* p = player -> cast_shaman();
    if( p -> active_flame_shock ) player_crit += 1.0;
  }

  virtual bool ready()
  {
    if( ! shaman_spell_t::ready() )
      return false;

    if( flame_shock )
    {
      shaman_t* p = player -> cast_shaman();

      if( ! p -> active_flame_shock ) 
	return false;

      double fs_time_remaining = p -> active_flame_shock -> duration_ready - ( sim -> current_time + execute_time() );

      if( fs_time_remaining < 0 )
	return false;

      if( max_ticks_consumed > 0 )
      {
	int8_t fs_tick_remaining = (int8_t) floor( fs_time_remaining / 2.0 );

	if( fs_tick_remaining > max_ticks_consumed )
	  return false;
      }
    }

    if( maelstrom > 0 && 
	maelstrom > player -> cast_shaman() -> buffs_maelstrom_weapon )
      return false;

    return true;
  }
};

// Elemental Mastery Spell ==================================================

struct elemental_mastery_t : public shaman_spell_t
{
  elemental_mastery_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "elemental_mastery", player, SCHOOL_NATURE, TREE_ELEMENTAL )
  {
    shaman_t* p = player -> cast_shaman();
    assert( p -> talents.elemental_mastery );
    cooldown = 180.0;
    trigger_gcd = 0;  
    if( ! options_str.empty() )
    {
      // This will prevent Elemental Mastery from being called before the desired "free spell" is ready to be cast.
      cooldown_group = options_str;
      duration_group = options_str;
    }
  }
   
  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs elemental_mastery", player -> name() );
    update_ready();
    shaman_t* p = player -> cast_shaman();
    p -> aura_gain( "Elemental Mastery" );
    p -> buffs_elemental_mastery = 1;
  }
};

// Natures Swiftness Spell ==================================================

struct shamans_swiftness_t : public shaman_spell_t
{
  shamans_swiftness_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "natures_swiftness", player, SCHOOL_NATURE, TREE_RESTORATION )
  {
    shaman_t* p = player -> cast_shaman();
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
    if( sim -> log ) report_t::log( sim, "%s performs natures_swiftness", player -> name() );
    update_ready();
    shaman_t* p = player -> cast_shaman();
    p -> aura_gain( "Natures Swiftness" );
    p -> buffs_natures_swiftness = 1;
  }
};

// Earth Shock Spell =======================================================

struct earth_shock_t : public shaman_spell_t
{
  int8_t sswait;

  earth_shock_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "earth_shock", player, SCHOOL_NATURE, TREE_ELEMENTAL ), sswait(0)
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "rank",   OPT_INT8, &rank_index },
      { "sswait", OPT_INT8, &sswait     },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 79, 10, 849, 895, 0, 0.18 },
      { 74,  9, 723, 761, 0, 0.18 },
      { 69,  8, 658, 692, 0, 535  },
      { 60,  7, 517, 545, 0, 450  },
      { 48,  6, 359, 381, 0, 345  },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 0; 
    direct_power_mod  = 0.41;
    may_crit          = true;
    cooldown          = 6.0;
    cooldown_group    = "shock";
      
    base_cost       = rank -> cost;
    base_cost       *= 1.0 - p -> talents.convection * 0.02;
    base_cost       *= 1.0 - p -> talents.mental_quickness * 0.02;
    cooldown        -= ( p -> talents.reverberation * 0.2 );
    base_multiplier *= 1.0 + p -> talents.concussion * 0.01;
    base_hit        += p -> talents.elemental_precision * 0.01;
    base_crit_bonus *= 1.0 + p -> talents.elemental_fury * 0.20;

    if( p -> glyphs.earth_shock ) trigger_gcd -= 1.0;
  }

  virtual double cost()
  {
    shaman_t* p = player -> cast_shaman();
    double c = shaman_spell_t::cost();
    if( p -> talents.shamanistic_focus ) c *= 0.55;
    return c;
  }

  virtual void consume_resource()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_spell_t::consume_resource();
    p -> buffs_shamanistic_focus = 0;
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();

    if( ! shaman_spell_t::ready() )
      return false;

    if( sswait && p -> buffs_nature_vulnerability <= 0 )
      return false;

    return true;
  }
};

// Frost Shock Spell =======================================================

struct frost_shock_t : public shaman_spell_t
{
  frost_shock_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "frost_shock", player, SCHOOL_FROST, TREE_ELEMENTAL )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 78, 7, 802, 848, 0, 0.18 },
      { 73, 6, 681, 719, 0, 0.18 },
      { 68, 5, 640, 676, 0, 525  },
      { 58, 4, 486, 514, 0, 430  },
      { 46, 3, 333, 353, 0, 325  },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 0; 
    direct_power_mod  = 0.41;
    may_crit          = true;
    cooldown          = 6.0;
    cooldown_group    = "shock";
      
    base_cost        = rank -> cost;
    base_cost       *= 1.0 - p -> talents.convection * 0.02;
    base_cost       *= 1.0 - p -> talents.mental_quickness * 0.02;
    cooldown        -= ( p -> talents.reverberation * 0.2 );
    base_multiplier *= 1.0 + p -> talents.concussion * 0.01;
    base_hit        += p -> talents.elemental_precision * 0.01;
    base_crit_bonus *= 1.0 + p -> talents.elemental_fury * 0.20;
  }

  virtual double cost()
  {
    shaman_t* p = player -> cast_shaman();
    double c = shaman_spell_t::cost();
    if( p -> talents.shamanistic_focus ) c *= 0.55;
    return c;
  }

  virtual void consume_resource()
  {
    shaman_spell_t::consume_resource();
    shaman_t* p = player -> cast_shaman();
    p -> buffs_shamanistic_focus = 0;
  }
};

// Flame Shock Spell =======================================================

struct flame_shock_t : public shaman_spell_t
{
  flame_shock_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "flame_shock", player, SCHOOL_FIRE, TREE_ELEMENTAL )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 9, 500, 500, 93, 0.17 },
      { 75, 8, 425, 425, 71, 0.17 },
      { 70, 7, 377, 377, 70, 500  },
      { 60, 6, 309, 309, 57, 450  },
      { 52, 5, 230, 230, 43, 345  },
      { 40, 4, 152, 152, 28, 250  },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 0; 
    base_tick_time    = 2.0;
    num_ticks         = 6;
    direct_power_mod  = 0.21;
    tick_power_mod    = 0.39 / num_ticks;
    may_crit          = true;
    cooldown          = 6.0;
    cooldown_group    = "shock";

    if ( p -> glyphs.flame_shock ) num_ticks++;
      
    base_cost        = rank -> cost;
    base_cost       *= 1.0 - p -> talents.convection * 0.02;
    base_cost       *= 1.0 - p -> talents.mental_quickness * 0.02;
    base_tick_dmg    = rank -> tick * ( 1.0 + p -> talents.storm_earth_and_fire * 0.10 );
    tick_power_mod  *= ( 1.0 + p -> talents.storm_earth_and_fire * 0.10 );
    cooldown        -= ( p -> talents.reverberation * 0.2 );
    base_multiplier *= 1.0 + p -> talents.concussion * 0.01;
    base_hit        += p -> talents.elemental_precision * 0.01;
    base_crit_bonus *= 1.0 + p -> talents.elemental_fury * 0.20;
  }

  virtual double cost()
  {
    shaman_t* p = player -> cast_shaman();
    double c = shaman_spell_t::cost();
    if( p -> talents.shamanistic_focus ) c *= 0.55;
    return c;
  }

  virtual void consume_resource()
  {
    shaman_spell_t::consume_resource();
    shaman_t* p = player -> cast_shaman();
    p -> buffs_shamanistic_focus = 0;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();
    if( result_is_hit() )
    {
      player -> cast_shaman() -> active_flame_shock = this;
    }
  }

  virtual void last_tick() 
  {
    shaman_spell_t::last_tick(); 
    player -> cast_shaman() -> active_flame_shock = 0;
  }
};

// Searing Totem Spell =======================================================

struct searing_totem_t : public shaman_spell_t
{
  searing_totem_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "searing_totem", player, SCHOOL_FIRE, TREE_ELEMENTAL )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 69, 7, 58, 58, 0, 205 },
      { 60, 6, 47, 47, 0, 170 },
      { 50, 5, 39, 39, 0, 145 },
      { 40, 4, 30, 30, 0, 110 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );

    base_execute_time = 0; 
    base_tick_time    = 2.0;
    direct_power_mod  = 0.08;
    num_ticks         = 30;
    may_crit          = true;
    duration_group    = "fire_totem";
    trigger_gcd       = 1.0;
      
    base_cost        = rank -> cost;
    base_cost       *= 1.0 - p -> talents.totemic_focus * 0.05;
    base_cost       *= 1.0 - p -> talents.mental_quickness * 0.02;
    base_multiplier *= 1.0 + p -> talents.call_of_flame * 0.05;
    base_hit        += p -> talents.elemental_precision * 0.01;
    base_crit_bonus *= 1.0 + p -> talents.elemental_fury * 0.20;
  }

  // Odd things to handle:
  // (1) Execute is guaranteed.
  // (2) Each "tick" is like an "execute".
  // (3) No hit/miss events are triggered.

  virtual void execute() 
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    schedule_tick();
    update_ready();
    direct_dmg = 0;
    update_stats( DMG_DIRECT );
    player -> action_finish( this );
  }

  virtual void tick() 
  {
    if( sim -> debug ) report_t::log( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );
    may_resist = false;
    target_debuff( DMG_DIRECT );
    calculate_result();
    may_resist = true;
    if( result_is_hit() )
    {
      calculate_direct_damage();
      if( direct_dmg > 0 )
      {
	tick_dmg = direct_dmg;
	assess_damage( tick_dmg, DMG_OVER_TIME );
      }
    }
    else
    {
      if( sim -> log ) report_t::log( sim, "%s avoids %s (%s)", sim -> target -> name(), name(), util_t::result_type_string( result ) );
    }
    update_stats( DMG_OVER_TIME );
  }
};

// Totem of Wrath Spell =====================================================

struct totem_of_wrath_t : public shaman_spell_t
{
  int8_t haste;
  double bonus;

  totem_of_wrath_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "totem_of_wrath", player, SCHOOL_NATURE, TREE_ELEMENTAL ), haste(0), bonus(1)
  {
    shaman_t* p = player -> cast_shaman();
    assert( p -> talents.totem_of_wrath );

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    base_cost      = 400;
    base_cost     *= 1.0 - p -> talents.totemic_focus * 0.05;
    base_cost     *= 1.0 - p -> talents.mental_quickness * 0.02;
    duration_group = "fire_totem";
    trigger_gcd    = 1.0;

    if( p -> glyphs.totem_of_wrath ) haste = 1;

    bonus = ( p -> level <= 60 ? 120 :
	      p -> level <= 70 ? 140 : 280 );
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      totem_of_wrath_t* totem;

      expiration_t( sim_t* sim, player_t* player, totem_of_wrath_t* t ) : event_t( sim, player ), totem(t)
      {
	name = "Totem of Wrath Expiration";
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  p -> aura_gain( "Totem of Wrath" );
	  p -> buffs.totem_of_wrath       = totem -> bonus;
	  p -> buffs.totem_of_wrath_haste = totem -> haste;
	}
	sim -> add_event( this, 300.0 );
      }
      virtual void execute()
      {
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  // Make sure it hasn't already been overriden by a more powerful totem.
	  if( totem -> bonus < p -> buffs.totem_of_wrath ||
	      totem -> haste < p -> buffs.totem_of_wrath_haste )
	    continue;

	  p -> aura_loss( "Totem of Wrath" );

	  p -> buffs.totem_of_wrath = 0;
	  p -> buffs.totem_of_wrath_haste = 0;
	}
      }
    };

    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    player -> action_finish( this );
    new expiration_t( sim, player, this );
  }

  virtual bool ready()
  {
    if( ! shaman_spell_t::ready() )
      return false;

    return( player -> buffs.totem_of_wrath       < bonus ||
	    player -> buffs.totem_of_wrath_haste < haste );
  }
};

// Flametongue Totem Spell ====================================================

struct flametongue_totem_t : public shaman_spell_t
{
  double bonus;

  flametongue_totem_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "flametongue_totem", player, SCHOOL_FIRE, TREE_ENHANCEMENT ), bonus(0)
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 8,  0,  0, 144, 0.11 },
      { 76, 7,  0,  0, 122, 0.11 },
      { 72, 6,  0,  0, 106, 0.11 },
      { 67, 5, 12, 12,  73, 325  },
      { 58, 4, 15, 15,  62, 275  },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
      
    base_cost        = rank -> cost;
    base_cost       *= 1.0 - p -> talents.totemic_focus * 0.05;
    base_cost       *= 1.0 - p -> talents.mental_quickness * 0.02;
    duration_group   = "fire_totem";
    trigger_gcd      = 1.0;

    base_multiplier *= 1.0 + p -> talents.enhancing_totems * 0.05;
    bonus = rank -> tick * base_multiplier;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      flametongue_totem_t* totem;

      expiration_t( sim_t* sim, player_t* player, flametongue_totem_t* t ) : event_t( sim, player ), totem(t)
      {
        name = "Flametongue Totem Expiration";
        for( player_t* p = sim -> player_list; p; p = p -> next )
        {
	  p -> aura_gain( "Flametongue Totem" );
	  p -> buffs.flametongue_totem = t -> bonus;
        }
        sim -> add_event( this, 120.0 );
      }

      virtual void execute()
      {
        for( player_t* p = sim -> player_list; p; p = p -> next )
        {
	  p -> aura_loss( "Flametongue Totem" );
	  p -> buffs.flametongue_totem = 0;
        }
      }
    };

    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    player -> action_finish( this );
    new expiration_t( sim, player, this );
  }

  virtual bool ready()
  {
    if( ! shaman_spell_t::ready() )
      return false;

    return( player -> buffs.flametongue_totem == 0 );
  }
};

// Windfury Totem Spell =====================================================

struct windfury_totem_t : public shaman_spell_t
{
  double bonus;

  windfury_totem_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "windfury_totem", player, SCHOOL_NATURE, TREE_ENHANCEMENT )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    base_cost      = 275;
    base_cost     *= 1.0 - p -> talents.totemic_focus * 0.05;
    base_cost     *= 1.0 - p -> talents.mental_quickness * 0.02;
    duration_group = "air_totem";
    trigger_gcd    = 1.0;

    bonus = 0.16 + p -> talents.improved_windfury_totem * 0.02;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      windfury_totem_t* totem;

      expiration_t( sim_t* sim, player_t* player, windfury_totem_t* t ) : event_t( sim, player ), totem(t)
      {
	name = "Windfury Totem Expiration";
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  p -> aura_gain( "Windfury Totem" );
	  p -> buffs.windfury_totem = t -> bonus;
	}
	sim -> add_event( this, 120.0 );
      }
      virtual void execute()
      {
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  // Make sure it hasn't already been overriden by a more powerful totem.
	  if( totem -> bonus < p -> buffs.windfury_totem )
	    continue;

	  p -> aura_loss( "Windfury Totem" );
	  p -> buffs.windfury_totem = 0;
	}
      }
    };

    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    player -> action_finish( this );
    new expiration_t( sim, player, this );
  }

  virtual bool ready()
  {
    if( ! shaman_spell_t::ready() )
      return false;

    return( player -> buffs.windfury_totem < bonus );
  }
};

// Flametongue Weapon Spell ===================================================

struct flametongue_weapon_t : public shaman_spell_t
{
  double bonus_power;
  int8_t main, off;

  flametongue_weapon_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "flametongue_weapon", player, SCHOOL_NATURE, TREE_ENHANCEMENT ), bonus_power(0), main(0), off(0)
  {
    shaman_t* p = player -> cast_shaman();
    
    std::string weapon_str;

    option_t options[] =
    {
      { "weapon", OPT_STRING, &weapon_str },
      { NULL }
    };
    parse_options( options, options_str );

    if( weapon_str.empty() ) 
    {
      main = off = 1;
    }
    else if( weapon_str == "main" )
    {
      main = 1;
    }
    else if( weapon_str == "off" )
    {
      off = 1;
    }
    else if( weapon_str == "both" )
    {
      main = 1;
      off = 1;
    }
    else
    {
      fprintf( sim -> output_file, "flametongue_weapon: weapon option must be one of main/off/both\n" );
      assert(0);
    }
    trigger_gcd = 0;

    bonus_power = ( ( p -> level <  64 ) ?  45 : 
		    ( p -> level <  71 ) ?  96 : 
		    ( p -> level <  76 ) ? 157 : 
		    ( p -> level <  80 ) ? 186 : 211 );

    bonus_power *= 1.0 + p -> talents.elemental_weapons * 0.10;
  }

  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );

    if( main ) 
    {
      player -> main_hand_weapon.buff = FLAMETONGUE;
      player -> main_hand_weapon.buff_bonus = bonus_power;
    }
    if( off )
    {
      player -> off_hand_weapon.buff = FLAMETONGUE;
      player -> off_hand_weapon.buff_bonus = bonus_power;
    }
  };

  virtual bool ready()
  {
    return( main && player -> main_hand_weapon.buff != FLAMETONGUE ||
	     off && player ->  off_hand_weapon.buff != FLAMETONGUE );
  }
};

// Windfury Weapon Spell ====================================================

struct windfury_weapon_t : public shaman_spell_t
{
  double bonus_power;
  int8_t main, off;

  windfury_weapon_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "windfury_weapon", player, SCHOOL_NATURE, TREE_ENHANCEMENT ), bonus_power(0), main(0), off(0)
  {
    shaman_t* p = player -> cast_shaman();

    std::string weapon_str;
    option_t options[] =
    {
      { "weapon", OPT_STRING, &weapon_str },
      { NULL }
    };
    parse_options( options, options_str );

    if( weapon_str.empty() ) 
    {
      main = off = 1;
    }
    else if( weapon_str == "main" )
    {
      main = 1;
    }
    else if( weapon_str == "off" )
    {
      off = 1;
    }
    else if( weapon_str == "both" )
    {
      main = 1;
      off = 1;
    }
    else
    {
      fprintf( sim -> output_file, "windfury_weapon: weapon option must be one of main/off/both\n" );
      assert(0);
    }
    trigger_gcd = 0;

    bonus_power = ( ( p -> level < 68 ) ? 333  : 
		    ( p -> level < 72 ) ? 445  :
	            ( p -> level < 76 ) ? 835  : 1250 );

    if( p -> glyphs.windfury_weapon ) bonus_power *= 1.40;
  }

  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );

    if( main ) 
    {
      player -> main_hand_weapon.buff = WINDFURY;
      player -> main_hand_weapon.buff_bonus = bonus_power;
    }
    if( off ) 
    {
      player -> off_hand_weapon.buff = WINDFURY;
      player -> off_hand_weapon.buff_bonus = bonus_power;
    }
  };

  virtual bool ready()
  {
    return( main && player -> main_hand_weapon.buff != WINDFURY ||
	     off && player ->  off_hand_weapon.buff != WINDFURY );
  }
};

// Strength of Earth Totem Spell ==============================================

struct strength_of_earth_totem_t : public shaman_spell_t
{
  double attr_bonus;
  int8_t crit_bonus;

  strength_of_earth_totem_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "strength_of_earth_totem", player, SCHOOL_NATURE, TREE_ENHANCEMENT )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 65, 5, 0, 0, 86, 300 },
      { 60, 4, 0, 0, 77, 275 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
      
    base_cost      = rank -> cost;
    base_cost     *= 1.0 - p -> talents.totemic_focus * 0.05;
    base_cost     *= 1.0 - p -> talents.mental_quickness * 0.02;
    duration_group = "earth_totem";
    trigger_gcd    = 1.0;

    attr_bonus = rank -> tick;
    attr_bonus *= 1.0 + p -> talents.enhancing_totems * 0.05;

    crit_bonus = p -> glyphs.strength_of_earth ? 1 : 0;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      strength_of_earth_totem_t* totem;

      expiration_t( sim_t* sim, player_t* player, strength_of_earth_totem_t* t ) : event_t( sim, player ), totem(t)
      {
	name = "Strength of Earth Totem Expiration";
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  p -> aura_gain( "Strength of Earth Totem" );

	  double delta = t -> attr_bonus - p -> buffs.strength_of_earth;

	  p -> attribute[ ATTR_STRENGTH ] += delta;
	  p -> attribute[ ATTR_AGILITY  ] += delta;

	  p -> buffs.strength_of_earth = t -> attr_bonus;
	  p -> buffs.strength_of_earth_crit = t -> crit_bonus;
	}
	sim -> add_event( this, 120.0 );
      }
      virtual void execute()
      {
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  // Make sure it hasn't already been overriden by a more powerful totem.
	  if( totem -> attr_bonus < p -> buffs.strength_of_earth )
	    continue;

	  p -> aura_loss( "Strength of Earth Totem" );

	  p -> attribute[ ATTR_STRENGTH ] -= totem -> attr_bonus;
	  p -> attribute[ ATTR_AGILITY  ] -= totem -> attr_bonus;

	  p -> buffs.strength_of_earth      = 0;
	  p -> buffs.strength_of_earth_crit = 0;
	}
      }
    };

    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    player -> action_finish( this );
    new expiration_t( sim, player, this );
  }

  virtual bool ready()
  {
    if( ! shaman_spell_t::ready() )
      return false;

    return( player -> buffs.strength_of_earth < attr_bonus );
  }
};

// Wrath of Air Totem Spell =================================================

struct wrath_of_air_totem_t : public shaman_spell_t
{
  wrath_of_air_totem_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "wrath_of_air_totem", player, SCHOOL_NATURE, TREE_ENHANCEMENT )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    base_cost      = 320;
    base_cost     *= 1.0 - p -> talents.totemic_focus * 0.05;
    base_cost     *= 1.0 - p -> talents.mental_quickness * 0.02;
    duration_group = "air_totem";
    trigger_gcd    = 1.0;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
      {
	name = "Wrath of Air Totem Expiration";
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  p -> aura_gain( "Wrath of Air Totem" );
	  p -> buffs.wrath_of_air = 1;
	}
	sim -> add_event( this, 120.0 );
      }
      virtual void execute()
      {
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  p -> aura_loss( "Wrath of Air Totem" );
	  p -> buffs.wrath_of_air = 0;
	}
      }
    };

    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    player -> action_finish( this );
    new expiration_t( sim, player );
  }

  virtual bool ready()
  {
    if( ! shaman_spell_t::ready() )
      return false;

    return( player -> buffs.wrath_of_air == 0 );
  }
};

// Mana Tide Totem Spell ==================================================

struct mana_tide_totem_t : public shaman_spell_t
{
  mana_tide_totem_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "mana_tide_totem", player, SCHOOL_NATURE, TREE_RESTORATION )
  {
    shaman_t* p = player -> cast_shaman();
    assert( p -> talents.mana_tide_totem );

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );

    harmful        = false;
    base_tick_time = 3.0; 
    num_ticks      = 4;
    cooldown       = 300.0;
    duration_group = "water_totem";
    base_cost      = 320;
    base_cost      *= 1.0 - p -> talents.totemic_focus * 0.05;
    base_cost      *= 1.0 - p -> talents.mental_quickness * 0.02;
    trigger_gcd     = 1.0;
  }

  virtual void execute() 
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    schedule_tick();
    update_ready();
    player -> action_finish( this );
  }

  virtual void tick() 
  {
    if( sim -> debug ) report_t::log( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );

    double pct = 0.06;
    if( player -> cast_shaman() -> glyphs.mana_tide ) pct += 0.01;

    for( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if( p -> party == player -> party )
      {
	p -> resource_gain( RESOURCE_MANA, p -> resource_max[ RESOURCE_MANA ] * pct, p -> gains.mana_tide );
      }
    }
  }

  virtual bool ready()
  {
    if( ! shaman_spell_t::ready() )
      return false;

    return( player -> resource_current[ RESOURCE_MANA ] < ( 0.75 * player -> resource_max[ RESOURCE_MANA ] ) );
  }
};

// Mana Spring Totem Spell ================================================

struct mana_spring_totem_t : public shaman_spell_t
{
  mana_spring_totem_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "mana_spring_totem", player, SCHOOL_NATURE, TREE_RESTORATION )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );

    harmful         = false;
    base_tick_time  = 2.0; 
    num_ticks       = 150;
    duration_group  = "water_totem";
    base_cost       = 120;
    base_cost      *= 1.0 - p -> talents.totemic_focus * 0.05;
    base_cost      *= 1.0 - p -> talents.mental_quickness * 0.02;
    base_multiplier = 1.0 + p -> talents.restorative_totems * 0.05;
    trigger_gcd     = 1.0;
  }

  virtual void execute() 
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    schedule_tick();
    update_ready();
    player -> action_finish( this );
  }

  virtual void tick() 
  {
    if( sim -> debug ) report_t::log( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );

    for( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if( p -> party == player -> party )
      {
	p -> resource_gain( RESOURCE_MANA, base_multiplier * 30.0, p -> gains.mana_spring );
      }
    }
  }

  virtual bool ready()
  {
    if( ! shaman_spell_t::ready() )
      return false;

    return( player -> resource_current[ RESOURCE_MANA ] < ( 0.95 * player -> resource_max[ RESOURCE_MANA ] ) );
  }
};

// Bloodlust Spell ===========================================================

struct bloodlust_t : public shaman_spell_t
{
  int8_t target_pct;

  bloodlust_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "bloodlust", player, SCHOOL_NATURE, TREE_ENHANCEMENT ), target_pct(0)
  {
    option_t options[] =
    {
      { "target_pct", OPT_INT8, &target_pct },
      { NULL }
    };
    parse_options( options, options_str );
      
    harmful   = false;
    base_cost = ( 0.26 * player -> resource_base[ RESOURCE_MANA ] );
    cooldown  = 600;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
      {
	name = "Bloodlust Expiration";
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  if( sim -> cooldown_ready( p -> cooldowns.bloodlust ) )
	  {
	    p -> aura_gain( "Bloodlust" );
	    p -> buffs.bloodlust = 1;
	    p -> cooldowns.bloodlust = sim -> current_time + 300;
	  }
	}
	sim -> add_event( this, 40.0 );
      }
      virtual void execute()
      {
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  p -> aura_loss( "Bloodlust" );
	  p -> buffs.bloodlust = 0;
	}
      }
    };

    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    player -> action_finish( this );
    new expiration_t( sim, player );
  }

  virtual bool ready()
  {
    if( ! shaman_spell_t::ready() )
      return false;

    if( player -> buffs.bloodlust )
      return false;

    if( ! sim -> cooldown_ready( player -> cooldowns.bloodlust ) )
      return false;

    target_t* t = sim -> target;

    if( target_pct == 0 )
      return true;

    if( t -> initial_health <= 0 )
      return false;

    return( ( t -> current_health / t -> initial_health ) < ( target_pct / 100.0 ) );
  }
};

// Shamanisitc Rage Spell ===========================================================

struct shamanistic_rage_t : public shaman_spell_t
{
  shamanistic_rage_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "shamanistic_rage", player, SCHOOL_NATURE, TREE_ENHANCEMENT )
  {
    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
     
    cooldown = 120;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
      {
	name = "Shamanistic Rage Expiration";
	shaman_t* p = player -> cast_shaman();
	p -> aura_gain( "Shamanistic Rage" );
	p -> buffs_shamanistic_rage = 1;
	sim -> add_event( this, 15.0 );
      }
      virtual void execute()
      {
	shaman_t* p = player -> cast_shaman();
	p -> aura_loss( "Shamanistic Rage" );
	p -> buffs_shamanistic_rage = 0;
      }
    };

    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    update_ready();
    player -> action_finish( this );
    new expiration_t( sim, player );
  }

  virtual bool ready()
  {
    if( ! shaman_spell_t::ready() )
      return false;

    return( player -> resource_current[ RESOURCE_MANA ] < ( 0.90 * player -> resource_max[ RESOURCE_MANA ] ) );
  }
};

// Lightning Shield Spell =====================================================

struct lightning_shield_t : public shaman_spell_t
{
  struct lightning_charge_t : public shaman_spell_t
  {
    lightning_charge_t( player_t* player, double base_dmg ) : 
      shaman_spell_t( "lightning_shield", player, SCHOOL_NATURE, TREE_ENHANCEMENT )
    {
      // Use the same name "lightning_shield" to make sure the cost of refreshing the shield is included with the procs.
    
      shaman_t* p = player -> cast_shaman();

      trigger_gcd      = 0;
      background       = true;
      direct_power_mod = 0.33;

      base_direct_dmg  = base_dmg;
      base_hit        += p -> talents.elemental_precision * 0.01;
      base_multiplier *= 1.0 + p -> talents.improved_shields * 0.05;
      base_crit_bonus *= 1.0 + p -> talents.elemental_fury * 0.20;

      if( p -> glyphs.lightning_shield ) base_multiplier *= 1.20;
    }
  };

  lightning_shield_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "lightning_shield", player, SCHOOL_NATURE, TREE_ENHANCEMENT )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 5, 380, 380, 0, 0 },
      { 75, 5, 325, 325, 0, 0 },
      { 70, 5, 287, 287, 0, 0 },
      { 63, 4, 232, 232, 0, 0 },
      { 56, 3, 198, 198, 0, 0 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_cost = 0;

    if( ! p -> active_lightning_charge )
    {
      p -> active_lightning_charge = new lightning_charge_t( p, ( rank -> dd_min + rank -> dd_max ) / 2.0 ) ;
    }
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_lightning_charges = 3 + 2 * p -> talents.static_shock;
    consume_resource();
    update_ready();
    direct_dmg = 0;
    p -> action_finish( this );
  }

  virtual bool ready()
  {
    if( ! shaman_spell_t::ready() )
      return false;

    return( player -> cast_shaman() -> buffs_lightning_charges == 0 );
  }
};

// Spirit Wolf Spell ==========================================================

struct spirit_wolf_spell_t : public shaman_spell_t
{
  struct spirit_wolf_expiration_t : public event_t
  {
    spirit_wolf_expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      sim -> add_event( this, 45.0 );
    }
    virtual void execute()
    {
      player -> dismiss_pet( "spirit_wolf" );
    }
  };

  int8_t target_pct;

  spirit_wolf_spell_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "spirit_wolf", player, SCHOOL_NATURE, TREE_ENHANCEMENT ), target_pct(0)
  {
    shaman_t* p = player -> cast_shaman();
    assert( p -> talents.feral_spirit );

    option_t options[] =
    {
      { "trigger", OPT_INT8, &target_pct },
      { NULL }
    };
    parse_options( options, options_str );

    cooldown   = 180.0;
    base_cost  = p -> resource_max[ RESOURCE_MANA ] * 0.12;
  }

  virtual void execute() 
  {
    consume_resource();
    update_ready();
    player -> summon_pet( "spirit_wolf" );
    player -> action_finish( this );
    new spirit_wolf_expiration_t( sim, player );
  }

  virtual bool ready()
  {
    if( ! shaman_spell_t::ready() )
      return false;

    target_t* t = sim -> target;

    if( target_pct == 0 )
      return true;

    if( t -> initial_health <= 0 )
      return false;

    return( ( t -> current_health / t -> initial_health ) < ( target_pct / 100.0 ) );
  }
};

// ==========================================================================
// Shaman Character Definition
// ==========================================================================

// shaman_t::create_action  =================================================

action_t* shaman_t::create_action( const std::string& name,
				   const std::string& options_str )
{
  if( name == "auto_attack"             ) return new              auto_attack_t( this, options_str );
  if( name == "bloodlust"               ) return new                bloodlust_t( this, options_str );
  if( name == "chain_lightning"         ) return new          chain_lightning_t( this, options_str );
  if( name == "earth_shock"             ) return new              earth_shock_t( this, options_str );
  if( name == "elemental_mastery"       ) return new        elemental_mastery_t( this, options_str );
  if( name == "flame_shock"             ) return new              flame_shock_t( this, options_str );
  if( name == "flametongue_totem"       ) return new        flametongue_totem_t( this, options_str );
  if( name == "flametongue_weapon"      ) return new       flametongue_weapon_t( this, options_str );
  if( name == "frost_shock"             ) return new              frost_shock_t( this, options_str );
  if( name == "lava_burst"              ) return new               lava_burst_t( this, options_str );
  if( name == "lava_lash"               ) return new                lava_lash_t( this, options_str );
  if( name == "lightning_bolt"          ) return new           lightning_bolt_t( this, options_str );
  if( name == "lightning_shield"        ) return new         lightning_shield_t( this, options_str );
  if( name == "mana_spring_totem"       ) return new        mana_spring_totem_t( this, options_str );
  if( name == "mana_tide_totem"         ) return new          mana_tide_totem_t( this, options_str );
  if( name == "natures_swiftness"       ) return new        shamans_swiftness_t( this, options_str );
  if( name == "searing_totem"           ) return new            searing_totem_t( this, options_str );
  if( name == "shamanistic_rage"        ) return new         shamanistic_rage_t( this, options_str );
  if( name == "spirit_wolf"             ) return new        spirit_wolf_spell_t( this, options_str );
  if( name == "stormstrike"             ) return new              stormstrike_t( this, options_str );
  if( name == "strength_of_earth_totem" ) return new  strength_of_earth_totem_t( this, options_str );
//if( name == "thunderstorm"            ) return new             thunderstorm_t( this, options_str );
  if( name == "totem_of_wrath"          ) return new           totem_of_wrath_t( this, options_str );
  if( name == "windfury_totem"          ) return new           windfury_totem_t( this, options_str );
  if( name == "windfury_weapon"         ) return new          windfury_weapon_t( this, options_str );
  if( name == "wrath_of_air_totem"      ) return new       wrath_of_air_totem_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// shaman_t::create_pet ======================================================

pet_t* shaman_t::create_pet( const std::string& pet_name )
{
  if( pet_name == "spirit_wolf" ) return new spirit_wolf_pet_t( sim, this, pet_name );

  return 0;
}

// shaman_t::init_base ========================================================

void shaman_t::init_base()
{
  attribute_base[ ATTR_STRENGTH  ] = 125;
  attribute_base[ ATTR_AGILITY   ] =  69;
  attribute_base[ ATTR_STAMINA   ] = 135;
  attribute_base[ ATTR_INTELLECT ] = 123;
  attribute_base[ ATTR_SPIRIT    ] = 140;

  base_spell_crit = 0.0225;
  initial_spell_crit_per_intellect = rating_t::interpolate( level, 0.01/60.0, 0.01/80.0, 0.01/166.6 );
  initial_spell_power_per_intellect = talents.natures_blessing * 0.10;

  base_attack_power = 120;
  base_attack_crit  = 0.0167;
  initial_attack_power_per_strength = 1.0;
  initial_attack_power_per_agility  = 1.0;
  initial_attack_crit_per_agility = rating_t::interpolate( level, 0.01/25.0, 0.01/40.0, 0.01/83.3 );

  resource_base[ RESOURCE_HEALTH ] = 3185;
  resource_base[ RESOURCE_MANA   ] = rating_t::interpolate( level, 1415, 2680, 4396 );

  health_per_stamina = 10;
  mana_per_intellect = 15;

  mana_per_intellect *= 1.0 + talents.ancestral_knowledge * 0.01;

  mp5_per_intellect = talents.unrelenting_storm * 0.02;

  if( gear.tier6_2pc )
  {
    // Simply assume the totems are out all the time.

    gear.spell_power_enchant[ SCHOOL_MAX ] += 45;
    gear.crit_rating_enchant               += 35;
    gear.mp5_enchant                       += 15;
  }
}

// shaman_t::reset ===========================================================

void shaman_t::reset()
{
  player_t::reset();

  // Totems
  fire_totem  = 0;
  air_totem   = 0;
  water_totem = 0;
  earth_totem = 0;;

  // Active
  active_flame_shock = 0;

  // Buffs
  buffs_elemental_devastation           = 0;
  buffs_elemental_focus                 = 0;
  buffs_elemental_mastery               = 0;
  buffs_flurry                          = 0;
  buffs_lightning_charges               = 0;
  buffs_maelstrom_weapon                = 0;
  buffs_nature_vulnerability         = 0;
  buffs_nature_vulnerability_charges = 0;
  buffs_natures_swiftness               = 0;
  buffs_shamanistic_focus               = 0;
  buffs_shamanistic_rage                = 0;

  // Cooldowns
  cooldowns_windfury_weapon = 0;

  // Expirations
  expirations_elemental_devastation    = 0;
  expirations_elemental_oath           = 0;
  expirations_maelstrom_weapon         = 0;
  expirations_nature_vulnerability  = 0;
  expirations_unleashed_rage           = 0;
}

// shaman_t::composite_attack_power ==========================================

double shaman_t::composite_attack_power()
{
  double ap = player_t::composite_attack_power();

  if( talents.mental_dexterity )
  {
    ap += composite_attack_power_multiplier() * intellect() * talents.mental_dexterity / 3.0;
  }

  return ap;
}

// shaman_t::composite_spell_power ==========================================

double shaman_t::composite_spell_power( int8_t school )
{
  double sp = player_t::composite_spell_power( school );

  if( talents.mental_quickness )
  {
    sp += composite_attack_power() * talents.mental_quickness * 0.10;
  }

  if( main_hand_weapon.buff == FLAMETONGUE )
  {
    sp += main_hand_weapon.buff_bonus;
  }
  if( off_hand_weapon.buff == FLAMETONGUE )
  {
    sp += off_hand_weapon.buff_bonus;
  }

  return sp;
}

// shaman_t::parse_talents ===============================================

bool shaman_t::parse_talents( const std::string& talent_string,
			      int                encoding )
{
  if( encoding == ENCODING_BLIZZARD )
  {
    if( talent_string.size() != 77 ) return false;

    talent_translation_t translation[] =
    {
      // Elemental
      {  1,  &( talents.convection                ) },
      {  2,  &( talents.concussion                ) },
      {  3,  &( talents.call_of_flame             ) },
      {  5,  &( talents.elemental_devastation     ) },
      {  6,  &( talents.reverberation             ) },
      {  7,  &( talents.elemental_focus           ) },
      {  8,  &( talents.call_of_thunder           ) },
      { 12,  &( talents.elemental_fury            ) },
      { 13,  &( talents.unrelenting_storm         ) },
      { 14,  &( talents.elemental_precision       ) },
      { 15,  &( talents.lightning_mastery         ) },
      { 16,  &( talents.elemental_mastery         ) },
      { 18,  &( talents.elemental_oath            ) },
      { 19,  &( talents.lightning_overload        ) },
      { 21,  &( talents.totem_of_wrath            ) },
      { 22,  &( talents.lava_flows                ) },
      { 23,  &( talents.storm_earth_and_fire      ) },
      { 24,  &( talents.thunderstorm              ) },
      // Enhancement
      { 25,  &( talents.enhancing_totems          ) },
      { 27,  &( talents.ancestral_knowledge       ) },
      { 29,  &( talents.thundering_strikes        ) },
      { 31,  &( talents.improved_shields          ) },
      { 32,  &( talents.mental_dexterity          ) },
      { 33,  &( talents.shamanistic_focus         ) },
      { 35,  &( talents.flurry                    ) },
      { 37,  &( talents.improved_windfury_totem   ) },
      { 38,  &( talents.spirit_weapons            ) },
      { 39,  &( talents.elemental_weapons         ) },
      { 40,  &( talents.mental_quickness          ) },
      { 41,  &( talents.weapon_mastery            ) },
      { 42,  &( talents.dual_wield_specialization ) },
      { 43,  &( talents.dual_wield                ) },
      { 44,  &( talents.stormstrike               ) },
      { 45,  &( talents.unleashed_rage            ) },
      { 46,  &( talents.improved_stormstrike      ) },
      { 47,  &( talents.static_shock              ) },
      { 48,  &( talents.shamanistic_rage          ) },
      { 50,  &( talents.maelstrom_weapon          ) },
      { 51,  &( talents.feral_spirit              ) },
      // Restoration
      { 53,  &( talents.totemic_focus             ) },
      { 61,  &( talents.restorative_totems        ) },
      { 62,  &( talents.tidal_mastery             ) },
      { 64,  &( talents.natures_swiftness         ) },
      { 68,  &( talents.mana_tide_totem           ) },
      { 70,  &( talents.blessing_of_the_eternals  ) },
      { 72,  &( talents.natures_blessing          ) },
      { 0, NULL }
    };
    player_t::parse_talents( translation, talent_string );
  } 
  else if( encoding == ENCODING_MMO )
  {
    if( talent_string.size() != 78 ) return false;

    talent_translation_t translation[] =
    {
      // Elemental
      {  1,  &( talents.convection                ) },
      {  2,  &( talents.concussion                ) },
      {  3,  &( talents.call_of_flame             ) },
      {  5,  &( talents.elemental_devastation     ) },
      {  6,  &( talents.reverberation             ) },
      {  7,  &( talents.elemental_focus           ) },
      {  8,  &( talents.elemental_fury            ) },
      { 12,  &( talents.call_of_thunder           ) },
      { 13,  &( talents.unrelenting_storm         ) },
      { 14,  &( talents.elemental_precision       ) },
      { 15,  &( talents.lightning_mastery         ) },
      { 16,  &( talents.elemental_mastery         ) },
      { 18,  &( talents.elemental_oath            ) },
      { 19,  &( talents.lightning_overload        ) },
      { 21,  &( talents.totem_of_wrath            ) },
      { 22,  &( talents.lava_flows                ) },
      { 23,  &( talents.storm_earth_and_fire      ) },
      { 24,  &( talents.thunderstorm              ) },
      // Restoration
      { 26,  &( talents.totemic_focus             ) },
      { 34,  &( talents.restorative_totems        ) },
      { 35,  &( talents.tidal_mastery             ) },
      { 37,  &( talents.natures_swiftness         ) },
      { 69,  &( talents.mana_tide_totem           ) },
      { 43,  &( talents.blessing_of_the_eternals  ) },
      { 45,  &( talents.natures_blessing          ) },
      // Enhancement
      { 51,  &( talents.enhancing_totems          ) },
      { 53,  &( talents.ancestral_knowledge       ) },
      { 55,  &( talents.thundering_strikes        ) },
      { 57,  &( talents.improved_shields          ) },
      { 58,  &( talents.elemental_weapons         ) },
      { 59,  &( talents.shamanistic_focus         ) },
      { 61,  &( talents.flurry                    ) },
      { 63,  &( talents.improved_windfury_totem   ) },
      { 64,  &( talents.spirit_weapons            ) },
      { 65,  &( talents.mental_dexterity          ) },
      { 66,  &( talents.unleashed_rage            ) },
      { 67,  &( talents.weapon_mastery            ) },
      { 68,  &( talents.dual_wield_specialization ) },
      { 69,  &( talents.dual_wield                ) },
      { 70,  &( talents.stormstrike               ) },
      { 71,  &( talents.mental_quickness          ) },
      { 72,  &( talents.lava_lash                 ) },
      { 73,  &( talents.improved_stormstrike      ) },
      { 74,  &( talents.static_shock              ) },
      { 75,  &( talents.shamanistic_rage          ) },
      { 77,  &( talents.maelstrom_weapon          ) },
      { 78,  &( talents.feral_spirit              ) },
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

// shaman_t::parse_option  ==============================================

bool shaman_t::parse_option( const std::string& name,
			     const std::string& value )
{
  option_t options[] =
  {
    { "ancestral_knowledge",       OPT_INT8,  &( talents.ancestral_knowledge       ) },
    { "blessing_of_the_eternals",  OPT_INT8,  &( talents.blessing_of_the_eternals  ) },
    { "call_of_flame",             OPT_INT8,  &( talents.call_of_flame             ) },
    { "call_of_thunder",           OPT_INT8,  &( talents.call_of_thunder           ) },
    { "concussion",                OPT_INT8,  &( talents.concussion                ) },
    { "convection",                OPT_INT8,  &( talents.convection                ) },
    { "dual_wield",                OPT_INT8,  &( talents.dual_wield                ) },
    { "dual_wield_specialization", OPT_INT8,  &( talents.dual_wield_specialization ) },
    { "elemental_devastation",     OPT_INT8,  &( talents.elemental_devastation     ) },
    { "elemental_focus",           OPT_INT8,  &( talents.elemental_focus           ) },
    { "elemental_fury",            OPT_INT8,  &( talents.elemental_fury            ) },
    { "elemental_mastery",         OPT_INT8,  &( talents.elemental_mastery         ) },
    { "elemental_oath",            OPT_INT8,  &( talents.elemental_oath            ) },
    { "elemental_precision",       OPT_INT8,  &( talents.elemental_precision       ) },
    { "elemental_weapons",         OPT_INT8,  &( talents.elemental_weapons         ) },
    { "enhancing_totems",          OPT_INT8,  &( talents.enhancing_totems          ) },
    { "feral_spirit",              OPT_INT8,  &( talents.feral_spirit              ) },
    { "flurry",                    OPT_INT8,  &( talents.flurry                    ) },
    { "improved_shields",          OPT_INT8,  &( talents.improved_shields          ) },
    { "improved_stormstrike",      OPT_INT8,  &( talents.improved_stormstrike      ) },
    { "improved_windfury_totem",   OPT_INT8,  &( talents.improved_windfury_totem   ) },
    { "lava_flows",                OPT_INT8,  &( talents.lava_flows                ) },
    { "lava_lash",                 OPT_INT8,  &( talents.lava_lash                 ) },
    { "lightning_mastery",         OPT_INT8,  &( talents.lightning_mastery         ) },
    { "lightning_overload",        OPT_INT8,  &( talents.lightning_overload        ) },
    { "maelstrom_weapon",          OPT_INT8,  &( talents.maelstrom_weapon          ) },
    { "mana_tide_totem",           OPT_INT8,  &( talents.mana_tide_totem           ) },
    { "mental_dexterity",          OPT_INT8,  &( talents.mental_dexterity          ) },
    { "mental_quickness",          OPT_INT8,  &( talents.mental_quickness          ) },
    { "natures_blessing",          OPT_INT8,  &( talents.natures_blessing          ) },
    { "natures_swiftness",         OPT_INT8,  &( talents.natures_swiftness         ) },
    { "restorative_totems",        OPT_INT8,  &( talents.restorative_totems        ) },
    { "reverberation",             OPT_INT8,  &( talents.reverberation             ) },
    { "shamanistic_focus",         OPT_INT8,  &( talents.shamanistic_focus         ) },
    { "shamanistic_rage",          OPT_INT8,  &( talents.shamanistic_rage          ) },
    { "spirit_weapons",            OPT_INT8,  &( talents.spirit_weapons            ) },
    { "static_shock",              OPT_INT8,  &( talents.static_shock              ) },
    { "stormstrike",               OPT_INT8,  &( talents.stormstrike               ) },
    { "storm_earth_and_fire",      OPT_INT8,  &( talents.storm_earth_and_fire      ) },
    { "thundering_strikes",        OPT_INT8,  &( talents.thundering_strikes        ) },
    { "thunderstorm",              OPT_INT8,  &( talents.thunderstorm              ) },
    { "tidal_mastery",             OPT_INT8,  &( talents.tidal_mastery             ) },
    { "totem_of_wrath",            OPT_INT8,  &( talents.totem_of_wrath            ) },
    { "totemic_focus",             OPT_INT8,  &( talents.totemic_focus             ) },
    { "unrelenting_storm",         OPT_INT8,  &( talents.unrelenting_storm         ) },
    { "unleashed_rage",            OPT_INT8,  &( talents.unleashed_rage            ) },
    { "weapon_mastery",            OPT_INT8,  &( talents.weapon_mastery            ) },
    // Glyphs
    { "glyph_blue_promises",       OPT_INT8,  &( glyphs.blue_promises              ) },
    { "glyph_earth_shock",         OPT_INT8,  &( glyphs.earth_shock                ) },
    { "glyph_flametongue_weapon",  OPT_INT8,  &( glyphs.flametongue_weapon         ) },
    { "glyph_lightning_bolt",      OPT_INT8,  &( glyphs.lightning_bolt             ) },
    { "glyph_lightning_shield",    OPT_INT8,  &( glyphs.lightning_shield           ) },
    { "glyph_mana_tide",           OPT_INT8,  &( glyphs.mana_tide                  ) },
    { "glyph_stormstrike",         OPT_INT8,  &( glyphs.stormstrike                ) },
    { "glyph_strength_of_earth",   OPT_INT8,  &( glyphs.strength_of_earth          ) },
    { "glyph_totem_of_wrath",      OPT_INT8,  &( glyphs.totem_of_wrath             ) },
    { "glyph_windfury_weapon",     OPT_INT8,  &( glyphs.windfury_weapon            ) },
    { "glyph_flame_shock",         OPT_INT8,  &( glyphs.flame_shock                ) },
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

// player_t::create_shaman  =================================================

player_t* player_t::create_shaman( sim_t*       sim, 
				   std::string& name ) 
{
  return new shaman_t( sim, name );
}
