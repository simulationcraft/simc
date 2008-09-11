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
  int8_t buffs_natures_swiftness;
  int8_t buffs_shamanistic_focus;
  int8_t buffs_shamanistic_rage;

  // Expirations
  event_t* expirations_elemental_devastation;
  event_t* expirations_maelstrom_weapon;
  event_t* expirations_windfury_weapon;

  // Gains
  gain_t* gains_shamanistic_rage;

  // Procs
  proc_t* procs_lightning_overload;
  
  // Up-Times
  uptime_t* uptimes_elemental_devastation;
  uptime_t* uptimes_elemental_oath;
  uptime_t* uptimes_flurry;
  
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
    int8_t  improved_weapon_totems;
    int8_t  improved_windfury_totem;
    int8_t  lava_flows;
    int8_t  lightning_mastery;
    int8_t  lightning_overload;
    int8_t  maelstrom_weapon;
    int8_t  mana_tide_totem;
    int8_t  mental_dexterity;
    int8_t  mental_quickness;
    int8_t  natures_blessing;
    int8_t  natures_guidance;
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
    int8_t  tidal_mastery;
    int8_t  totem_of_wrath;
    int8_t  totemic_focus;
    int8_t  unrelenting_storm;
    int8_t  unleashed_rage;
    int8_t  weapon_mastery;
    int8_t  weapon_specialization;
    
    talents_t() { memset( (void*) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int8_t earth_shock;
    int8_t flametongue_weapon;
    int8_t lightning_bolt;
    int8_t lightning_shield;
    int8_t mana_tide;
    int8_t stormstrike;
    int8_t strength_of_earth;
    int8_t totem_of_wrath;
    int8_t windfury_weapon;
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
    buffs_elemental_devastation = 0;
    buffs_elemental_focus       = 0;
    buffs_elemental_mastery     = 0;
    buffs_flurry                = 0;
    buffs_lightning_charges     = 0;
    buffs_maelstrom_weapon      = 0;
    buffs_natures_swiftness     = 0;
    buffs_shamanistic_focus     = 0;
    buffs_shamanistic_rage      = 0;

    // Expirations
    expirations_elemental_devastation = 0;
    expirations_maelstrom_weapon      = 0;
    expirations_windfury_weapon       = 0;
  
    // Gains
    gains_shamanistic_rage = get_gain( "shamanistic_rage" );

    // Procs
    procs_lightning_overload = get_proc( "lightning_overload" );

    // Up-Times
    uptimes_elemental_devastation = get_uptime( "elemental_devastation" );
    uptimes_elemental_oath        = get_uptime( "elemental_oath"        );
    uptimes_flurry                = get_uptime( "flurry"                );
  
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
  virtual double    composite_attack_hit();
  virtual double    composite_attack_crit();
  virtual double    composite_spell_power( int8_t school );
  virtual double    composite_spell_hit();
  virtual double    composite_spell_crit();
  virtual void      parse_talents( const std::string& talent_string );
  virtual bool      parse_option( const std::string& name, const std::string& value );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual int       primary_resource() { return RESOURCE_MANA; }

  // Event Tracking
  virtual void regen( double periodicity );
  virtual void attack_hit_event   ( attack_t* );
  virtual void attack_damage_event( attack_t*, double amount, int8_t dmg_type );
  virtual void spell_start_event ( spell_t* );
  virtual void spell_hit_event   ( spell_t* );
  virtual void spell_damage_event( spell_t*, double amount, int8_t dmg_type );
};

// ==========================================================================
// Shaman Attack
// ==========================================================================

struct shaman_attack_t : public attack_t
{
  shaman_attack_t( const char* n, player_t* player, int8_t s=SCHOOL_PHYSICAL, int8_t t=TREE_NONE ) : 
    attack_t( n, player, RESOURCE_MANA, s, t ) 
  {
    base_dd = 1;
    shaman_t* p = player -> cast_shaman();
    base_multiplier *= 1.0 + p -> talents.weapon_specialization * 0.02;
  }

  virtual void execute();
  virtual void player_buff();
};

// ==========================================================================
// Shaman Spell
// ==========================================================================

struct shaman_spell_t : public spell_t
{
  shaman_spell_t( const char* n, player_t* p, int8_t s, int8_t t ) : 
    spell_t( n, p, RESOURCE_MANA, s, t ) {}

  virtual double cost();
  virtual void   consume_resource();
  virtual double execute_time();
  virtual void   player_buff();
  virtual void   schedule_execute();

  // Passthru Methods
  virtual void execute()     { spell_t::execute();      }
  virtual void last_tick()   { spell_t::last_tick();    }
  virtual bool ready()       { return spell_t::ready(); }
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// trigger_flametongue_weapon ===============================================

static void trigger_flametongue_weapon( attack_t* a )
{
  struct flametongue_weapon_spell_t : public shaman_spell_t
  {
    double fire_dmg;

    flametongue_weapon_spell_t( player_t* player ) :
      shaman_spell_t( "flametongue", player, SCHOOL_FIRE, TREE_ENHANCEMENT ), fire_dmg(0)
    {
      shaman_t* p = player -> cast_shaman();

      background       = true;
      proc             = true;
      may_crit         = true;
      trigger_gcd      = 0;
      dd_power_mod     = 0.10;
      base_multiplier *= 1.0 + p -> talents.elemental_weapons * 0.05;
      base_hit        += p -> talents.elemental_precision * ( sim_t::WotLK ? 0.01 : 0.02 );

      fire_dmg  = ( p -> level < 64 ) ? 28 : 
	          ( p -> level < 71 ) ? 35 : 
	          ( p -> level < 80 ) ? 55 : 75;

      reset();
    }
    virtual void get_base_damage()
    {
      base_dd = fire_dmg * weapon -> swing_time;
    }
  };

  if( a -> weapon &&
      a -> weapon -> buff == FLAMETONGUE_WEAPON )
  {
    shaman_t* p = a -> player -> cast_shaman();

    if( ! p -> flametongue_weapon_spell )
    {
      p -> flametongue_weapon_spell = new flametongue_weapon_spell_t( p );
    }

    p -> flametongue_weapon_spell -> weapon = a -> weapon;
    p -> flametongue_weapon_spell -> execute();
  }
}

// trigger_windfury_weapon ================================================

static void trigger_windfury_weapon( attack_t* a )
{
  struct windfury_weapon_expiration_t : public event_t
  {
    windfury_weapon_expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
    {
      name = "Windfury Weapon Expiration";
      sim -> add_event( this, 3.0 );
    }
    virtual void execute()
    {
      player -> cast_shaman() -> expirations_windfury_weapon = 0;
    }
  };

  struct windfury_weapon_attack_t : public shaman_attack_t
  {
    double bonus_power;
    
    windfury_weapon_attack_t( player_t* player ) :
      shaman_attack_t( "windfury", player ), bonus_power(0)
    {
      shaman_t* p = player -> cast_shaman();
      may_glance  = false;
      background  = true;
      trigger_gcd = 0;
      base_multiplier *= 1.0 + p -> talents.elemental_weapons * 0.133333;

      bonus_power = ( p -> level <= 60 ) ? 333  : 
	            ( p -> level <= 68 ) ? 445  :
	            ( p -> level <= 72 ) ? 835  :
	            ( p -> level <= 76 ) ? 1090 : 1250;

      if( p -> glyphs.windfury_weapon ) bonus_power *= 1.40;

      reset();
    }
    virtual void player_buff()
    {
      shaman_attack_t::player_buff();
      player_power += bonus_power;
    }
  };

  shaman_t* p = a -> player -> cast_shaman();

  if(   a -> weapon                            &&
        a -> weapon -> buff == WINDFURY_WEAPON &&
      ! p -> expirations_windfury_weapon       &&
        rand_t::roll( 0.20 )                     )
  {
    if( ! p -> windfury_weapon_attack )
    {
      p -> windfury_weapon_attack = new windfury_weapon_attack_t( p );
    }
    p -> expirations_windfury_weapon = new windfury_weapon_expiration_t( a -> sim, p );

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
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      shaman_t* p = player -> cast_shaman();
      p -> aura_loss( "Maelstrom Weapon" );
      p -> buffs_maelstrom_weapon = 0;
      p -> expirations_maelstrom_weapon = 0;
    }
  };

  if( p -> buffs_maelstrom_weapon < 5 ) 
  {
    p -> buffs_maelstrom_weapon++;

    if( a -> sim -> log ) report_t::log( a -> sim, "%s gains Maelstrom Weapon %d", p -> name(), p -> buffs_maelstrom_weapon );
  }

  event_t*& e = p -> expirations_maelstrom_weapon;
    
  if( e )
  {
    e -> reschedule( 15.0 );
  }
  else
  {
    e = new maelstrom_weapon_expiration_t( a -> sim, p );
  }
}

// trigger_unleashed_rage =================================================

static void trigger_unleashed_rage( attack_t* a )
{
  int8_t talents_ur = a -> player -> cast_shaman() -> talents.unleashed_rage;
  int8_t   buffs_ur = a -> player ->                    buffs.unleashed_rage;
  
  if( talents_ur == 0 || ( talents_ur < buffs_ur ) )
    return;

  struct unleashed_rage_expiration_t : public event_t
  {
    unleashed_rage_expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
    {
      name = "Unleashed Rage Expiration";
      player -> aura_gain( "Unleashed Rage" );
      player -> attack_power_multiplier *= 1.0 + player -> buffs.unleashed_rage * 0.02;
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Unleashed Rage" );
      player -> attack_power_multiplier /= 1.0 + player -> buffs.unleashed_rage * 0.02;
      player -> buffs.unleashed_rage = 0;
      player -> expirations.unleashed_rage = 0;
    }
  };

  for( player_t* p = a -> sim -> player_list; p; p = p -> next )
  {
    if( sim_t::WotLK || p -> party == a -> player -> party )
    {
      event_t*& e = p -> expirations.unleashed_rage;
    
      if( e )
      {
	if( talents_ur > buffs_ur )
	{
	  p -> attack_power_multiplier *= ( 1.0 + talents_ur * 0.02 ) / ( 1.0 + buffs_ur * 0.02 );
	  p -> buffs.unleashed_rage = talents_ur;
	}
	e -> reschedule( 10.0 );
      }
      else
      {
	p -> buffs.unleashed_rage = talents_ur;
	e = new unleashed_rage_expiration_t( a -> sim, p );
      }
    }
  }
}

// trigger_nature_vulnerability =============================================

static void trigger_nature_vulnerability( attack_t* a )
{
  struct nature_vulnerability_expiration_t : public event_t
  {
    nature_vulnerability_expiration_t( sim_t* sim ) : event_t( sim )
    {
      name = "Nature Vulnerability Expiration";
      if( sim -> log ) report_t::log( sim, "%s gains Nature Vulnerability", sim -> target -> name() );
      sim -> add_event( this, 12.0 );
    }
    virtual void execute()
    {
      if( sim -> log ) report_t::log( sim, "%s loses Nature Vulnerability", sim -> target -> name() );
      target_t* t = sim -> target;
      t -> debuffs.nature_vulnerability = 0;
      t -> debuffs.nature_vulnerability_charges = 0;
      t -> expirations.nature_vulnerability = 0;
    }
  };

  shaman_t* p = a -> player -> cast_shaman();
  target_t* t = a -> sim -> target;
  event_t*& e = t -> expirations.nature_vulnerability;

  t -> debuffs.nature_vulnerability_charges = 2 + p -> talents.improved_stormstrike;
  t -> debuffs.nature_vulnerability = p -> glyphs.stormstrike ? 28 : 20;
   
  if( e )
  {
    e -> reschedule( 12.0 );
  }
  else
  {
    e = new nature_vulnerability_expiration_t( a -> sim );
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

    double   cost       = s -> base_cost;
    double   multiplier = s -> base_multiplier;
    stats_t* stats      = s -> stats;

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
  if( s -> player -> cast_shaman() -> talents.elemental_oath == 0 ) return;

  if( s -> proc ) return;

  struct elemental_oath_expiration_t : public event_t
  {
    elemental_oath_expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
    {
      name = "Elemental Oath Expiration";
      player -> aura_gain( "Maelstrom Weapon" );
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Maelstrom Weapon" );
      player -> buffs.elemental_oath = 0;
      player -> expirations.elemental_oath = 0;
    }
  };

  for( player_t* p = s -> sim -> player_list; p; p = p -> next )
  {
    if( p -> buffs.elemental_oath < 6 )
    {
      // Assume talent has two ranks.
      p -> buffs.elemental_oath += 2;

      if( s -> sim -> log ) report_t::log( s -> sim, "%s gains Elemental Oath %d", p -> name(), p -> buffs.elemental_oath );
    }

    event_t*& e = p -> expirations.elemental_oath;
    
    if( e )
    {
      e -> reschedule( 15.0 );
    }
    else
    {
      e = new elemental_oath_expiration_t( s -> sim, p );
    }
  }
}

} // ANONYMOUS NAMESPACE ===================================================

// =========================================================================
// Shaman Attack
// =========================================================================

// shaman_attack_t::execute ================================================

void shaman_attack_t::execute()
{
  attack_t::execute();
  if( result_is_hit() )
  {
    shaman_t* p = player -> cast_shaman();
    if( p -> buffs_shamanistic_rage ) 
    {
      p -> resource_gain( RESOURCE_MANA, player_power * 0.30, p -> gains_shamanistic_rage );
    }
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
      t *= 1.0 / ( 1.0 + 0.05 * ( p -> talents.flurry + 1 ) );
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

  virtual void execute()
  {
    shaman_attack_t::execute();
    if( result_is_hit() )
    {
      if( ! sim_t::WotLK )
      {
	enchant_t::trigger_flametongue_totem( this );
	enchant_t::trigger_windfury_totem( this );
      }
    }
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
    base_multiplier *= 1.0 + p -> talents.weapon_specialization * 0.02;
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
    mh_dd = dd;
    mh_result = result;

    if( result_is_hit() )
    {
      trigger_nature_vulnerability( this );

      if( player -> off_hand_weapon.type != WEAPON_NONE )
      {
	weapon = &( player -> off_hand_weapon );
	shaman_attack_t::execute();
	oh_dd = dd;
	oh_result = result;
      }
    }

    // This is probably bad form.....  Perhaps better to merge results and call update_stats().
    stats -> add( mh_dd + oh_dd, DMG_DIRECT, mh_result, time_to_execute );

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
  p -> uptimes_elemental_oath -> update( p -> buffs.elemental_oath == 6 );
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
  p -> buffs_elemental_mastery = 0;
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
  spell_t::player_buff();
  shaman_t* p = player -> cast_shaman();
  if( p -> buffs_elemental_mastery )
  {
    player_crit += 1.0;
  }
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
    dd_power_mod       = 0.7143;
    may_crit           = true;
    cooldown           = 6.0;
    base_cost          = rank -> cost;

    base_execute_time -= p -> talents.lightning_mastery * 0.1;
    base_cost         *= 1.0 - p -> talents.convection * ( sim_t::WotLK ? 0.04 : 0.02 );
    base_multiplier   *= 1.0 + p -> talents.concussion * 0.01;
    base_hit          += p -> talents.elemental_precision * ( sim_t::WotLK ? 0.01 : 0.02 );
    base_crit         += p -> talents.call_of_thunder * 0.01;
    base_crit         += p -> talents.tidal_mastery * 0.01;
    if( p -> talents.elemental_fury ) base_crit_bonus *= 2.0;

    lightning_overload_stats = p -> get_stats( "lightning_overload" );
    lightning_overload_stats -> school = SCHOOL_NATURE;
    lightning_overload_stats -> adjust_for_lost_time = false;
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
      t *= ( 1.0 - p -> buffs_maelstrom_weapon * p -> talents.maelstrom_weapon * 0.04001 );
      if( t < 0 ) t = 0;
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
    dd_power_mod       = sim_t::WotLK ? ( 2.5 / 3.5 ) : 0.794;
    may_crit           = true;
    base_cost          = rank -> cost;

    base_execute_time -= p -> talents.lightning_mastery * 0.1;
    base_cost         *= 1.0 - p -> talents.convection * ( sim_t::WotLK ? 0.04 : 0.02 );
    base_multiplier   *= 1.0 + p -> talents.concussion * 0.01;
    base_hit          += p -> talents.elemental_precision * ( sim_t::WotLK ? 0.01 : 0.02 );
    base_crit         += p -> talents.call_of_thunder * 0.01;
    base_crit         += p -> talents.tidal_mastery * 0.01;
    if( p -> talents.elemental_fury ) base_crit_bonus *= 2.0;
    dd_power_mod      *= 1.0 + p -> talents.storm_earth_and_fire * 0.02; 

    if( p -> gear.tier6_4pc ) base_multiplier *= 1.05;
    if( p -> glyphs.lightning_bolt ) base_cost *= 0.90;

    lightning_overload_stats = p -> get_stats( "lightning_overload" );
    lightning_overload_stats -> school = SCHOOL_NATURE;
    lightning_overload_stats -> adjust_for_lost_time = false;
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
      t *= ( 1.0 - p -> buffs_maelstrom_weapon * p -> talents.maelstrom_weapon * 0.04001 );
      if( t < 0 ) t = 0;
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
      { 74, 1, 888, 1132, 0, 655 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );

    base_execute_time  = 2.0; 
    dd_power_mod       = ( 2.0 / 3.5 );
    may_crit           = true;
    cooldown           = 8.0;

    base_cost          = rank -> cost;
    base_cost         *= 1.0 - p -> talents.convection * ( sim_t::WotLK ? 0.04 : 0.02 );
    base_multiplier   *= 1.0 + p -> talents.concussion * 0.01;
    base_multiplier   *= 1.0 + p -> talents.call_of_flame * 0.01;
    base_hit          += p -> talents.elemental_precision * ( sim_t::WotLK ? 0.01 : 0.02 );
    if( p -> talents.elemental_fury ) base_crit_bonus *= 2.0;
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
      t *= ( 1.0 - p -> buffs_maelstrom_weapon * p -> talents.maelstrom_weapon * 0.04001 );
      if( t < 0 ) t = 0;
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
    dd_power_mod      = 0.41;
    may_crit          = true;
    cooldown          = 6.0;
    cooldown_group    = "shock";
      
    base_cost       = rank -> cost;
    base_cost       *= 1.0 - p -> talents.convection * ( sim_t::WotLK ? 0.04 : 0.02 );
    base_cost       *= 1.0 - p -> talents.mental_quickness * 0.02;
    cooldown        -= ( p -> talents.reverberation * 0.2 );
    base_multiplier *= 1.0 + p -> talents.concussion * 0.01;
    base_hit        += p -> talents.elemental_precision * ( sim_t::WotLK ? 0.01 : 0.02 );
    if( p -> talents.elemental_fury ) base_crit_bonus *= 2.0;

    if( p -> glyphs.earth_shock ) trigger_gcd -= 1.0;
  }

  virtual double cost()
  {
    shaman_t* p = player -> cast_shaman();

    double c = shaman_spell_t::cost();

    if( sim_t::WotLK )
    {
      if( p -> talents.shamanistic_focus ) c *= 0.55;
    }
    else
    {
      if( p -> buffs_shamanistic_focus ) c *= 0.40;
    }

    return c;
  }

  virtual void consume_resource()
  {
    shaman_spell_t::consume_resource();
    shaman_t* p = player -> cast_shaman();
    p -> buffs_shamanistic_focus = 0;
  }

  virtual bool ready()
  {
    if( ! shaman_spell_t::ready() )
      return false;

    if( sswait && sim -> target -> debuffs.nature_vulnerability <= 0 )
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
    dd_power_mod      = 0.41;
    may_crit          = true;
    cooldown          = 6.0;
    cooldown_group    = "shock";
      
    base_cost        = rank -> cost;
    base_cost       *= 1.0 - p -> talents.convection * ( sim_t::WotLK ? 0.04 : 0.02 );
    base_cost       *= 1.0 - p -> talents.mental_quickness * 0.02;
    cooldown        -= ( p -> talents.reverberation * 0.2 );
    base_multiplier *= 1.0 + p -> talents.concussion * 0.01;
    base_hit        += p -> talents.elemental_precision * ( sim_t::WotLK ? 0.01 : 0.02 );
    if( p -> talents.elemental_fury ) base_crit_bonus *= 2.0;
  }

  virtual double cost()
  {
    shaman_t* p = player -> cast_shaman();

    double c = shaman_spell_t::cost();

    if( sim_t::WotLK )
    {
      if( p -> talents.shamanistic_focus ) c *= 0.55;
    }
    else
    {
      if( p -> buffs_shamanistic_focus ) c *= 0.40;
    }

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
      { 80, 9, 500, 500, 556, 0.17 },
      { 75, 8, 425, 425, 476, 0.17 },
      { 70, 7, 377, 377, 420, 500  },
      { 60, 6, 309, 309, 344, 450  },
      { 52, 5, 230, 230, 256, 345  },
      { 40, 4, 152, 152, 168, 250  },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    base_execute_time = 0; 
    dd_power_mod      = 0.21;
    base_duration     = 12.0;
    num_ticks         = 6;
    dot_power_mod     = 0.39;
    may_crit          = true;
    cooldown          = 6.0;
    cooldown_group    = "shock";
      
    base_cost        = rank -> cost;
    base_cost       *= 1.0 - p -> talents.convection * ( sim_t::WotLK ? 0.04 : 0.02 );
    base_cost       *= 1.0 - p -> talents.mental_quickness * 0.02;
    base_dot         = rank -> dot * ( 1.0 + p -> talents.storm_earth_and_fire * 0.10 );
    dot_power_mod   *= ( 1.0 + p -> talents.storm_earth_and_fire * 0.20 );
    cooldown        -= ( p -> talents.reverberation * 0.2 );
    base_multiplier *= 1.0 + p -> talents.concussion * 0.01;
    base_hit        += p -> talents.elemental_precision * ( sim_t::WotLK ? 0.01 : 0.02 );
    if( p -> talents.elemental_fury ) base_crit_bonus *= 2.0;
  }

  virtual double cost()
  {
    shaman_t* p = player -> cast_shaman();

    double c = shaman_spell_t::cost();

    if( sim_t::WotLK )
    {
      if( p -> talents.shamanistic_focus ) c *= 0.55;
    }
    else
    {
      if( p -> buffs_shamanistic_focus ) c *= 0.40;
    }

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
    base_duration     = 60.0;
    dd_power_mod      = 0.08;
    num_ticks         = 30;
    may_crit          = true;
    duration_group    = "fire_totem";
    trigger_gcd       = 1.0;
      
    base_cost        = rank -> cost;
    base_cost       *= 1.0 - p -> talents.totemic_focus * 0.05;
    base_cost       *= 1.0 - p -> talents.mental_quickness * 0.02;
    base_multiplier *= 1.0 + p -> talents.call_of_flame * 0.05;
    if( p -> talents.elemental_fury ) base_crit_bonus *= 2.0;
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
    dd = 0;
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
      calculate_damage();
      adjust_dd_for_result();
      if( dd > 0 )
      {
	dot_tick = dd;
	assess_damage( dot_tick, DMG_OVER_TIME );
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

    if( p -> glyphs.totem_of_wrath ) haste = 1;

    if( sim_t::WotLK )
    {
      bonus = ( p -> level <= 60 ? 120 :
		p -> level <= 70 ? 140 : 160 );
    }
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
	  if( sim_t::WotLK || p -> party == player -> party )
	  {
	    p -> aura_gain( "Totem of Wrath" );
	    p -> buffs.totem_of_wrath       = totem -> bonus;
	    p -> buffs.totem_of_wrath_haste = totem -> haste;
	  }
	}
	sim -> add_event( this, 120.0 );
      }
      virtual void execute()
      {
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  if( sim_t::WotLK || p -> party == player -> party )
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
      
    base_cost       = rank -> cost;
    base_cost       *= 1.0 - p -> talents.totemic_focus * 0.05;
    base_cost       *= 1.0 - p -> talents.mental_quickness * 0.02;
    duration_group   = "fire_totem";
    trigger_gcd      = 1.0;

    if( sim_t::WotLK )
    {
      base_multiplier *= 1.0 + p -> talents.enhancing_totems * 0.05;
      bonus = rank -> dot * base_multiplier;
    }
    else
    {
      base_multiplier *= 1.0 + p -> talents.improved_weapon_totems * 0.06;
      base_multiplier *= 1.0 + p -> talents.call_of_flame * 0.05;
      bonus = rank -> dd_max * base_multiplier;
    }
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
	  if( sim_t::WotLK || p -> party == player -> party )
	  {
	    p -> aura_gain( "Flametongue Totem" );
	    p -> buffs.flametongue_totem = t -> bonus;

	    if( ! sim_t::WotLK && p -> main_hand_weapon.buff == WEAPON_BUFF_NONE )
	    {
	      p -> main_hand_weapon.buff = FLAMETONGUE_TOTEM;
	    }
	  }
	}
	sim -> add_event( this, 120.0 );
      }
      virtual void execute()
      {
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  if( sim_t::WotLK || p -> party == player -> party )
	  {
	    p -> aura_loss( "Flametongue Totem" );
	    p -> buffs.flametongue_totem = 0;

	    if( ! sim_t::WotLK && p -> main_hand_weapon.buff == FLAMETONGUE_TOTEM )
	    {
	      p -> main_hand_weapon.buff = WEAPON_BUFF_NONE;
	    }
	  }
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

    if( sim_t::WotLK )
    {
      bonus = 0.16 + p -> talents.improved_windfury_totem * 0.02;
    }
    else
    {
      bonus = 375.0 + p -> talents.improved_weapon_totems * 0.15;
    }
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
	  if( sim_t::WotLK || p -> party == player -> party )
	  {
	    p -> aura_gain( "Windfury Totem" );
	    p -> buffs.windfury_totem = t -> bonus;

	    if( ! sim_t::WotLK )
	    {
	      if( p -> main_hand_weapon.buff == WEAPON_BUFF_NONE )
	      {
		p -> main_hand_weapon.buff = WINDFURY_TOTEM;
	      }
	    }
	  }
	}
	sim -> add_event( this, 120.0 );
      }
      virtual void execute()
      {
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  if( sim_t::WotLK || p -> party == player -> party )
	  {
	    // Make sure it hasn't already been overriden by a more powerful totem.
	    if( totem -> bonus < p -> buffs.windfury_totem )
	      continue;

	    p -> aura_loss( "Windfury Totem" );
	    p -> buffs.windfury_totem = 0;

	    if( ! sim_t::WotLK )
	    {
	      if( p -> main_hand_weapon.buff == WINDFURY_TOTEM )
	      {
		p -> main_hand_weapon.buff = WEAPON_BUFF_NONE;
	      }
	    }
	  }
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
  double bonus;
  int8_t main, off;

  flametongue_weapon_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "flametongue_weapon", player, SCHOOL_NATURE, TREE_ENHANCEMENT ), bonus(0), main(0), off(0)
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

    bonus = ( p -> level <  64 ) ? 30 : 
            ( p -> level <  71 ) ? 52 : 
            ( p -> level <  80 ) ? 74 : 96;

    bonus *= 1.0 + p -> talents.lava_flows * 0.15;
  }

  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );

    if( main && player -> main_hand_weapon.buff != FLAMETONGUE_WEAPON ) 
    {
      player -> main_hand_weapon.buff = FLAMETONGUE_WEAPON;
      if( sim_t::WotLK ) player -> spell_power[ SCHOOL_MAX ] += bonus;
    }
    if( off && player -> off_hand_weapon.buff != FLAMETONGUE_WEAPON )
    {
      player -> off_hand_weapon.buff = FLAMETONGUE_WEAPON;
      if( sim_t::WotLK ) player -> spell_power[ SCHOOL_MAX ] += bonus;
    }
  };

  virtual bool ready()
  {
    return( main && player -> main_hand_weapon.buff != FLAMETONGUE_WEAPON ||
	     off && player ->  off_hand_weapon.buff != FLAMETONGUE_WEAPON );
  }
};

// Windfury Weapon Spell ====================================================

struct windfury_weapon_t : public shaman_spell_t
{
  int8_t main, off;

  windfury_weapon_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "windfury_weapon", player, SCHOOL_NATURE, TREE_ENHANCEMENT ), main(0), off(0)
  {
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
  }

  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );
    if( main ) player -> main_hand_weapon.buff = WINDFURY_WEAPON;
    if( off  ) player ->  off_hand_weapon.buff = WINDFURY_WEAPON;
  };

  virtual bool ready()
  {
    return( main && player -> main_hand_weapon.buff != WINDFURY_WEAPON ||
	     off && player ->  off_hand_weapon.buff != WINDFURY_WEAPON );
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

    attr_bonus = rank -> dot;
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
	  if( sim_t::WotLK || p -> party == player -> party )
	  {
	    p -> aura_gain( "Strength of Earth Totem" );

	    double delta = t -> attr_bonus - p -> buffs.strength_of_earth;

	    p -> attribute[ ATTR_STRENGTH ] += delta;
	    p -> buffs.strength_of_earth = t -> attr_bonus;

	    if( sim_t::WotLK )
	    {
	      p -> attribute[ ATTR_AGILITY ] += delta;
	      p -> buffs.grace_of_air = t -> crit_bonus;
	    }
	  }
	}
	sim -> add_event( this, 120.0 );
      }
      virtual void execute()
      {
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  if( sim_t::WotLK || p -> party == player -> party )
	  {
	    // Make sure it hasn't already been overriden by a more powerful totem.
	    if( totem -> attr_bonus < p -> buffs.strength_of_earth )
	      continue;

	    p -> aura_loss( "Strength of Earth Totem" );

	    p -> buffs.strength_of_earth = 0;
	    p -> attribute[ ATTR_STRENGTH ] -= totem -> attr_bonus;

	    if( sim_t::WotLK )
	    {
	      p -> buffs.grace_of_air = 0;
	      p -> attribute[ ATTR_AGILITY ] -= totem -> attr_bonus;
	    }
	  }
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

// Grace of Air Totem Spell =================================================

struct grace_of_air_totem_t : public shaman_spell_t
{
  double attr_bonus;

  grace_of_air_totem_t( player_t* player, const std::string& options_str ) : 
    shaman_spell_t( "grace_of_air_totem", player, SCHOOL_NATURE, TREE_ENHANCEMENT )
  {
    assert( ! sim_t::WotLK );

    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "rank", OPT_INT8, &rank_index },
      { NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 60, 4, 0, 0, 77, 310 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
      
    base_cost      = rank -> cost;
    base_cost     *= 1.0 - p -> talents.totemic_focus * 0.05;
    base_cost     *= 1.0 - p -> talents.mental_quickness * 0.02;
    duration_group = "air_totem";
    trigger_gcd    = 1.0;

    attr_bonus = rank -> dot;
    attr_bonus *= 1.0 + p -> talents.enhancing_totems * 0.05;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      grace_of_air_totem_t* totem;

      expiration_t( sim_t* sim, player_t* player, grace_of_air_totem_t* t ) : event_t( sim, player ), totem(t)
      {
	name = "Grace of Air Totem Expiration";
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  if( p -> party == player -> party )
	  {
	    p -> aura_gain( "Grace of Air Totem" );
	    double delta = t -> attr_bonus - p -> buffs.grace_of_air;
	    p -> attribute[ ATTR_AGILITY ] += delta;
	    p -> buffs.grace_of_air = t -> attr_bonus;
	  }
	}
	sim -> add_event( this, 120.0 );
      }
      virtual void execute()
      {
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  if( p -> party == player -> party )
	  {
	    // Make sure it hasn't already been overriden by a more powerful totem.
	    if( totem -> attr_bonus < p -> buffs.grace_of_air )
	      continue;

	    p -> aura_loss( "Wrath of Air Totem" );
	    p -> attribute[ ATTR_AGILITY ] -= totem -> attr_bonus;
	    p -> buffs.grace_of_air = 0;
	  }
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

    return( player -> buffs.grace_of_air < attr_bonus );
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
      double bonus;
      expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player ), bonus(0)
      {
	name = "Wrath of Air Totem Expiration";
	bonus = 101;
	if( player -> gear.tier4_2pc ) bonus += 20;
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  if( p -> party == player -> party )
	  {
	    assert( p -> buffs.wrath_of_air == 0 );
	    p -> buffs.wrath_of_air = 1;
	    p -> aura_gain( "Wrath of Air Totem" );
	    if( ! sim_t::WotLK ) p -> spell_power[ SCHOOL_MAX ] += bonus;
	  }
	}
	sim -> add_event( this, 120.0 );
      }
      virtual void execute()
      {
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  if( p -> party == player -> party )
	  {
	    p -> buffs.wrath_of_air = 0;
	    p -> aura_loss( "Wrath of Air Totem" );
	    if( ! sim_t::WotLK ) p -> spell_power[ SCHOOL_MAX ] -= bonus;
	  }
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
    base_duration  = 12.0; 
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
    base_duration   = 120.0; 
    num_ticks       = 60;
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
	p -> resource_gain( RESOURCE_MANA, base_multiplier * 20.0, p -> gains.mana_spring );
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
    base_cost = sim_t::WotLK ? ( 0.26 * player -> resource_base[ RESOURCE_MANA ] ): 750;
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
	  if( sim_t::WotLK )
	  {
	    if( sim -> cooldown_ready( p -> cooldowns.bloodlust ) )
	    {
	      p -> aura_gain( "Bloodlust" );
	      p -> buffs.bloodlust = 1;
	      p -> cooldowns.bloodlust = sim -> current_time + 300;
	    }
	  }
	  else
	  {
	    if( p -> party == player -> party )
	    {
	      p -> aura_gain( "Bloodlust" );
	      p -> buffs.bloodlust = 1;
	    }
	  }
	}
	sim -> add_event( this, 40.0 );
      }
      virtual void execute()
      {
	for( player_t* p = sim -> player_list; p; p = p -> next )
	{
	  if( sim_t::WotLK || p -> party == player -> party )
	  {
	    p -> aura_loss( "Bloodlust" );
	    p -> buffs.bloodlust = 0;
	  }
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

      trigger_gcd  = 0;
      background   = true;
      dd_power_mod = 0.33;

      base_dd          = base_dmg;
      base_hit        += p -> talents.elemental_precision * ( sim_t::WotLK ? 0.01 : 0.02 );
      base_multiplier *= 1.0 + p -> talents.improved_shields * 0.05;

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
      { 80, 5, 380, 380, 0, 375 },
      { 75, 5, 325, 325, 0, 350 },
      { 70, 5, 287, 287, 0, 325 },
      { 63, 4, 232, 232, 0, 300 },
      { 56, 3, 198, 198, 0, 275 },
      { 0, 0 }
    };
    player -> init_mana_costs( ranks );
    rank = choose_rank( ranks );
    
    if( sim_t::WotLK )
    {
      base_cost = 0;
    }
    else
    {
      base_cost  = rank -> cost;
      base_cost *= 1.0 - p -> talents.mental_quickness * 0.02;
    }

    if( ! p -> active_lightning_charge )
    {
      p -> active_lightning_charge = new lightning_charge_t( p, ( rank -> dd_min + rank -> dd_max ) / 2.0 ) ;
    }
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_lightning_charges = 3 + p -> talents.static_shock;
    consume_resource();
    update_ready();
    dd = 0;
    stats_t::last_execute = stats;
    p -> action_finish( this );
  }

  virtual bool ready()
  {
    if( ! shaman_spell_t::ready() )
      return false;

    return( player -> cast_shaman() -> buffs_lightning_charges == 0 );
  }
};

// ============================================================================
// Shaman Event Tracking
// ============================================================================

// shaman_t::attack_hit_event =================================================

void shaman_t::attack_hit_event( attack_t* a )
{
  player_t::attack_hit_event( a );

  if( a -> result == RESULT_CRIT )
  {
    trigger_unleashed_rage( a );
    stack_maelstrom_weapon( a );
    shaman_t* p = a -> player -> cast_shaman();
    if( p -> talents.flurry ) p -> buffs_flurry = 3;
    if( ! sim_t::WotLK && p -> talents.shamanistic_focus ) buffs_shamanistic_focus = 1;
  }
  trigger_flametongue_weapon( a );
  trigger_windfury_weapon( a );
}

// shaman_t::attack_damage_event ==============================================

void shaman_t::attack_damage_event( attack_t* a,
				    double   amount,
				    int8_t   dmg_type )
{
  player_t::attack_damage_event( a, amount, dmg_type );

  if( ! a -> proc && ( dmg_type == DMG_DIRECT ) && ( buffs_lightning_charges > 0 ) )
  {
    if( rand_t::roll( talents.static_shock * 0.02 ) )
    {
      buffs_lightning_charges--;
      active_lightning_charge -> execute();
    }
  }
}

// shaman_t::spell_start_event ================================================

void shaman_t::spell_start_event( spell_t* s )
{
  player_t::spell_start_event( s );

}

// shaman_t::spell_hit_event ==================================================

void shaman_t::spell_hit_event( spell_t* s )
{
  player_t::spell_hit_event( s );

  if( s -> result == RESULT_CRIT )
  {
    trigger_elemental_devastation( s );
    trigger_elemental_oath( s );

    buffs_elemental_focus = 2;

    if( gear.tier4_4pc )
    {
      buffs.tier4_4pc = rand_t::roll( 0.11 ) ;
    }
  }
}

// shaman_t::spell_damage_event ==============================================

void shaman_t::spell_damage_event( spell_t* s,
				   double   amount,
				   int8_t   dmg_type )
{
  player_t::spell_damage_event( s, amount, dmg_type );

  if( ! s -> proc && ( dmg_type == DMG_DIRECT ) && ( buffs_lightning_charges > 0 ) )
  {
    if( rand_t::roll( talents.static_shock * 0.02 ) )
    {
      buffs_lightning_charges--;
      active_lightning_charge -> execute();
    }
  }
}

// ==========================================================================
// Shaman Character Definition
// ==========================================================================

// shaman_t::create_action  =================================================

action_t* shaman_t::create_action( const std::string& name,
				   const std::string& options )
{
  if( name == "auto_attack"             ) return new              auto_attack_t( this, options );
  if( name == "bloodlust"               ) return new                bloodlust_t( this, options );
  if( name == "chain_lightning"         ) return new          chain_lightning_t( this, options );
  if( name == "earth_shock"             ) return new              earth_shock_t( this, options );
  if( name == "elemental_mastery"       ) return new        elemental_mastery_t( this, options );
  if( name == "flame_shock"             ) return new              flame_shock_t( this, options );
  if( name == "flametongue_totem"       ) return new        flametongue_totem_t( this, options );
  if( name == "flametongue_weapon"      ) return new       flametongue_weapon_t( this, options );
  if( name == "frost_shock"             ) return new              frost_shock_t( this, options );
  if( name == "grace_of_air_totem"      ) return new       grace_of_air_totem_t( this, options );
  if( name == "lava_burst"              ) return new               lava_burst_t( this, options );
  if( name == "lightning_bolt"          ) return new           lightning_bolt_t( this, options );
  if( name == "lightning_shield"        ) return new         lightning_shield_t( this, options );
  if( name == "mana_spring_totem"       ) return new        mana_spring_totem_t( this, options );
  if( name == "mana_tide_totem"         ) return new          mana_tide_totem_t( this, options );
  if( name == "natures_swiftness"       ) return new        shamans_swiftness_t( this, options );
  if( name == "searing_totem"           ) return new            searing_totem_t( this, options );
  if( name == "shamanistic_rage"        ) return new         shamanistic_rage_t( this, options );
  if( name == "stormstrike"             ) return new              stormstrike_t( this, options );
  if( name == "strength_of_earth_totem" ) return new  strength_of_earth_totem_t( this, options );
  if( name == "totem_of_wrath"          ) return new           totem_of_wrath_t( this, options );
  if( name == "windfury_totem"          ) return new           windfury_totem_t( this, options );
  if( name == "windfury_weapon"         ) return new          windfury_weapon_t( this, options );
  if( name == "wrath_of_air_totem"      ) return new       wrath_of_air_totem_t( this, options );

  return 0;
}

// shaman_t::init_base ========================================================

void shaman_t::init_base()
{
  attribute_base[ ATTR_STRENGTH  ] = 105;
  attribute_base[ ATTR_AGILITY   ] =  60;
  attribute_base[ ATTR_STAMINA   ] = 115;
  attribute_base[ ATTR_INTELLECT ] = 105;
  attribute_base[ ATTR_SPIRIT    ] = 120;

  base_spell_crit = 0.0225;
  initial_spell_crit_per_intellect = rating_t::interpolate( level, 0.01/60.0, 0.01/80.0, 0.01/166.6 );
  initial_spell_power_per_intellect = talents.natures_blessing * 0.10;

  base_attack_power = 120;
  base_attack_crit  = 0.0167;
  initial_attack_power_per_strength = 2.0;
  initial_attack_crit_per_agility = rating_t::interpolate( level, 0.01/25.0, 0.01/40.0, 0.01/83.3 );

  resource_base[ RESOURCE_HEALTH ] = 3185;
  resource_base[ RESOURCE_MANA   ] = rating_t::interpolate( level, 1415, 2680, 4396 );

  health_per_stamina = 10;
  mana_per_intellect = 15;

  mana_per_intellect *= 1.0 + talents.ancestral_knowledge * 0.01;

  if( gear.tier6_2pc )
  {
    // Simply assume the totems are out all the time.

    gear.spell_power_enchant[ SCHOOL_MAX ] += 45;
    gear.spell_crit_rating_enchant         += 35;
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
  buffs_elemental_devastation = 0;
  buffs_elemental_focus       = 0;
  buffs_elemental_mastery     = 0;
  buffs_flurry                = 0;
  buffs_lightning_charges     = 0;
  buffs_maelstrom_weapon      = 0;
  buffs_natures_swiftness     = 0;
  buffs_shamanistic_focus     = 0;
  buffs_shamanistic_rage      = 0;

  // Expirations
  expirations_elemental_devastation = 0;
  expirations_maelstrom_weapon      = 0;
  expirations_windfury_weapon       = 0;
}

// shaman_t::composite_attack_power ==========================================

double shaman_t::composite_attack_power()
{
  double ap = player_t::composite_attack_power();

  if( talents.mental_dexterity )
  {
    ap += intellect() * talents.mental_dexterity * 0.33333;
  }

  return ap;
}

// shaman_t::composite_attack_hit ===========================================

double shaman_t::composite_attack_hit()
{
  double hit = player_t::composite_attack_hit();

  if( talents.natures_guidance )
  {
    hit += talents.natures_guidance * 0.01;
  }
  if( dual_wield() ) 
  {
    hit += talents.dual_wield_specialization * 0.02;
  }

  return hit;
}

// shaman_t::composite_attack_crit ==========================================

double shaman_t::composite_attack_crit()
{
  double crit = player_t::composite_attack_crit();

  if( talents.thundering_strikes )
  {
    crit += talents.thundering_strikes * 0.01;
  }

  return crit;
}

// shaman_t::composite_spell_power ==========================================

double shaman_t::composite_spell_power( int8_t school )
{
  double sp = player_t::composite_spell_power( school );

  if( talents.mental_quickness )
  {
    sp += composite_attack_power() * talents.mental_quickness * 0.10;
  }

  return sp;
}

// shaman_t::composite_spell_hit =============================================

double shaman_t::composite_spell_hit()
{
  double hit = player_t::composite_spell_hit();

  if( talents.natures_guidance )
  {
    hit += talents.natures_guidance * 0.01;
  }

  return hit;
}

// shaman_t::composite_spell_crit ============================================

double shaman_t::composite_spell_crit()
{
  double crit = player_t::composite_spell_crit();

  if( talents.blessing_of_the_eternals )
  {
    crit += talents.blessing_of_the_eternals * 0.02;
  }
  if( glyphs.flametongue_weapon )
  {
    if( main_hand_weapon.buff == FLAMETONGUE_WEAPON ||
	 off_hand_weapon.buff == FLAMETONGUE_WEAPON ) 
    {
      crit += 0.02;
    }
  }

  return crit;
}

// shaman_t::regen  ==========================================================

void shaman_t::regen( double periodicity )
{
  double spirit_regen = periodicity * spirit_regen_per_second();

  if( buffs.innervate )
  {
    spirit_regen *= 4.0;
  }
  else if( recent_cast() )
  {
    spirit_regen = 0;
  }

  double mp5_regen = periodicity * ( mp5 + intellect() * talents.unrelenting_storm * 0.02 ) / 5.0;

  resource_gain( RESOURCE_MANA, spirit_regen, gains.spirit_regen );
  resource_gain( RESOURCE_MANA,    mp5_regen, gains.mp5_regen    );

  if( buffs.replenishment )
  {
    double replenishment_regen = periodicity * resource_max[ RESOURCE_MANA ] * 0.005 / 1.0;

    resource_gain( RESOURCE_MANA, replenishment_regen, gains.replenishment );
  }

  if( buffs.water_elemental_regen )
  {
    double water_elemental_regen = periodicity * resource_max[ RESOURCE_MANA ] * 0.006 / 5.0;

    resource_gain( RESOURCE_MANA, water_elemental_regen, gains.water_elemental_regen );
  }
}

// shaman_t::parse_talents ===============================================

void shaman_t::parse_talents( const std::string& talent_string )
{
  if( talent_string.size() == 61 )
  {
    talent_translation_t translation[] =
    {
      {  1,  &( talents.convection                ) },
      {  2,  &( talents.concussion                ) },
      {  5,  &( talents.call_of_flame             ) },
      {  6,  &( talents.elemental_focus           ) },
      {  7,  &( talents.reverberation             ) },
      {  8,  &( talents.call_of_thunder           ) },
      { 13,  &( talents.elemental_fury            ) },
      { 14,  &( talents.unrelenting_storm         ) },
      { 15,  &( talents.elemental_precision       ) },
      { 16,  &( talents.lightning_mastery         ) },
      { 17,  &( talents.elemental_mastery         ) },
      { 19,  &( talents.lightning_overload        ) },
      { 20,  &( talents.totem_of_wrath            ) },
      { 21,  &( talents.ancestral_knowledge       ) },
      { 24,  &( talents.thundering_strikes        ) },
      { 26,  &( talents.improved_shields          ) },
      { 27,  &( talents.enhancing_totems          ) },
      { 28,  &( talents.shamanistic_focus         ) },
      { 30,  &( talents.flurry                    ) },
      { 32,  &( talents.improved_weapon_totems    ) },
      { 33,  &( talents.spirit_weapons            ) },
      { 34,  &( talents.elemental_weapons         ) },
      { 35,  &( talents.mental_quickness          ) },
      { 36,  &( talents.weapon_mastery            ) },
      { 37,  &( talents.dual_wield_specialization ) },
      { 38,  &( talents.dual_wield                ) },
      { 39,  &( talents.stormstrike               ) },
      { 40,  &( talents.unleashed_rage            ) },
      { 41,  &( talents.shamanistic_rage          ) },
      { 46,  &( talents.totemic_focus             ) },
      { 47,  &( talents.natures_guidance          ) },
      { 51,  &( talents.restorative_totems        ) },
      { 52,  &( talents.tidal_mastery             ) },
      { 54,  &( talents.natures_swiftness         ) },
      { 57,  &( talents.mana_tide_totem           ) },
      { 59,  &( talents.natures_blessing          ) },
      { 0, NULL }
    };
    player_t::parse_talents( translation, talent_string );
  } 
  else if( talent_string.size() == 77 )
  {
    talent_translation_t translation[] =
    {
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
      { 22,  &( talents.lava_flows                 ) },
      { 23,  &( talents.storm_earth_and_fire      ) },
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
  else
  {
    fprintf( sim -> output_file, "Malformed Shaman talent string.  Number encoding should have length 61 for Burning Crusade or 77 for Wrath of the Lich King.\n" );
    assert( 0 );
  }
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
    { "improved_weapon_totems",    OPT_INT8,  &( talents.improved_weapon_totems    ) },
    { "improved_windfury_totem",   OPT_INT8,  &( talents.improved_windfury_totem   ) },
    { "lava_flows",                OPT_INT8,  &( talents.lava_flows                ) },
    { "lightning_mastery",         OPT_INT8,  &( talents.lightning_mastery         ) },
    { "lightning_overload",        OPT_INT8,  &( talents.lightning_overload        ) },
    { "maelstrom_weapon",          OPT_INT8,  &( talents.maelstrom_weapon          ) },
    { "mana_tide_totem",           OPT_INT8,  &( talents.mana_tide_totem           ) },
    { "mental_dexterity",          OPT_INT8,  &( talents.mental_dexterity          ) },
    { "mental_quickness",          OPT_INT8,  &( talents.mental_quickness          ) },
    { "natures_blessing",          OPT_INT8,  &( talents.natures_blessing          ) },
    { "natures_guidance",          OPT_INT8,  &( talents.natures_guidance          ) },
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
    { "tidal_mastery",             OPT_INT8,  &( talents.tidal_mastery             ) },
    { "totem_of_wrath",            OPT_INT8,  &( talents.totem_of_wrath            ) },
    { "totemic_focus",             OPT_INT8,  &( talents.totemic_focus             ) },
    { "unrelenting_storm",         OPT_INT8,  &( talents.unrelenting_storm         ) },
    { "unleashed_rage",            OPT_INT8,  &( talents.unleashed_rage            ) },
    { "weapon_mastery",            OPT_INT8,  &( talents.weapon_mastery            ) },
    { "weapon_specialization",     OPT_INT8,  &( talents.weapon_specialization     ) },
    // Glyphs
    { "glyph_earth_shock",         OPT_INT8,  &( glyphs.earth_shock                ) },
    { "glyph_flametongue_weapon",  OPT_INT8,  &( glyphs.flametongue_weapon         ) },
    { "glyph_lightning_bolt",      OPT_INT8,  &( glyphs.lightning_bolt             ) },
    { "glyph_lightning_shield",    OPT_INT8,  &( glyphs.lightning_shield           ) },
    { "glyph_mana_tide",           OPT_INT8,  &( glyphs.mana_tide                  ) },
    { "glyph_stormstrike",         OPT_INT8,  &( glyphs.stormstrike                ) },
    { "glyph_strength_of_earth",   OPT_INT8,  &( glyphs.strength_of_earth          ) },
    { "glyph_totem_of_wrath",      OPT_INT8,  &( glyphs.totem_of_wrath             ) },
    { "glyph_windfury_weapon",     OPT_INT8,  &( glyphs.windfury_weapon            ) },
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
