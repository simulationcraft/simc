// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simcraft.h"

// ==========================================================================
// Warlock
// ==========================================================================

struct warlock_pet_t;

struct warlock_t : public player_t
{
  // Active
  warlock_pet_t* active_pet;
  action_t*      active_corruption;
  action_t*      active_curse;
  action_t*      active_immolate;
  action_t*      active_shadowflame;
  action_t*      active_pandemic;
  int            active_dots;
  int            affliction_effects;

  // Buffs
  struct _buffs_t
  {
    int    backdraft;
    int    decimation;
    int    demon_armor;
    int    demonic_empathy;
    int    empowered_imp;
    double eradication;
    double fel_armor;
    int    flame_shadow;
    int    haunted;
    double life_tap_glyph;
    int    metamorphosis;
    double molten_core;
    int    pet_sacrifice;
    double pyroclasm;
    int    shadow_embrace;
    int    shadow_flame;
    int    shadow_trance;
    double shadow_vulnerability;
    int    shadow_vulnerability_charges;

    void reset() { memset( (void*) this, 0x00, sizeof( _buffs_t ) ); }
    _buffs_t() { reset(); }
  };
  _buffs_t _buffs;

  // Cooldowns
  struct _cooldowns_t
  {
    double eradication;

    void reset() { memset( (void*) this, 0x00, sizeof( _cooldowns_t ) ); }
    _cooldowns_t() { reset(); }
  };
  _cooldowns_t _cooldowns;

  // Expirations
  struct _expirations_t
  {
    event_t* backdraft;
    event_t* decimation;
    event_t* demonic_empathy;
    event_t* empowered_imp;
    event_t* eradication;
    event_t* flame_shadow;
    event_t* haunted;
    event_t* infernal;
    event_t* life_tap_glyph;
    event_t* molten_core;
    event_t* pyroclasm;
    event_t* shadow_embrace;
    event_t* shadow_flame;
    event_t* shadow_vulnerability;

    void reset() { memset( (void*) this, 0x00, sizeof( _expirations_t ) ); }
    _expirations_t() { reset(); }
  };
  _expirations_t _expirations;

  // Gains
  gain_t* gains_dark_pact;
  gain_t* gains_fel_armor;
  gain_t* gains_felhunter;
  gain_t* gains_life_tap;
  gain_t* gains_sacrifice;
  gain_t* gains_soul_leech;

  // Procs
  proc_t* procs_dark_pact;
  proc_t* procs_life_tap;
  proc_t* procs_shadow_trance;

  // Up-Times
  uptime_t* uptimes_backdraft;
  uptime_t* uptimes_demonic_empathy;
  uptime_t* uptimes_demonic_pact;
  uptime_t* uptimes_demonic_soul;
  uptime_t* uptimes_empowered_imp;
  uptime_t* uptimes_eradication;
  uptime_t* uptimes_molten_core;
  uptime_t* uptimes_pyroclasm;
  uptime_t* uptimes_shadow_flame;
  uptime_t* uptimes_flame_shadow;
  uptime_t* uptimes_shadow_trance;
  uptime_t* uptimes_shadow_vulnerability;
  uptime_t* uptimes_spirits_of_the_damned;

  struct talents_t
  {
    int  aftermath;
    int  amplify_curse;
    int  backdraft;
    int  backlash;
    int  bane;
    int  cataclysm;
    int  chaos_bolt;
    int  conflagrate;
    int  contagion;
    int  dark_pact;
    int  deaths_embrace;
    int  decimation;
    int  demonic_aegis;
    int  demonic_brutality;
    int  demonic_embrace;
    int  demonic_empathy;
    int  demonic_empowerment;
    int  demonic_knowledge;
    int  demonic_pact;
    int  demonic_power;
    int  demonic_sacrifice;
    int  demonic_tactics;
    int  destructive_reach;
    int  devastation;
    int  emberstorm;
    int  empowered_corruption;
    int  empowered_imp;
    int  eradication;
    int  everlasting_affliction;
    int  fel_domination;
    int  fel_intellect;
    int  fel_stamina;
    int  fel_synergy;
    int  fel_vitality;
    int  fire_and_brimstone;
    int  haunt;
    int  improved_corruption;
    int  improved_curse_of_agony;
    int  improved_demonic_tactics;
    int  improved_drain_soul;
    int  improved_felhunter;
    int  improved_fire_bolt;
    int  improved_immolate;
    int  improved_imp;
    int  improved_lash_of_pain;
    int  improved_life_tap;
    int  improved_searing_pain;
    int  improved_shadow_bolt;
    int  improved_soul_leech;
    int  improved_succubus;
    int  improved_voidwalker;
    int  malediction;
    int  mana_feed;
    int  master_conjuror;
    int  master_demonologist;
    int  master_summoner;
    int  metamorphosis;
    int  molten_core;
    int  nemesis;
    int  nightfall;
    int  pandemic;
    int  pyroclasm;
    int  ruin;
    int  shadow_and_flame;
    int  shadow_burn;
    int  shadow_embrace;
    int  shadow_mastery;
    int  siphon_life;
    int  soul_leech;
    int  soul_link;
    int  soul_siphon;
    int  summon_felguard;
    int  suppression;
    int  unholy_power;
    int  unstable_affliction;
        
    talents_t() { memset( (void*) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int chaos_bolt;
    int conflagrate;
    int corruption;
    int curse_of_agony;
    int felguard;
    int felhunter;
    int haunt;
    int immolate;
    int imp;
    int incinerate;
    int life_tap;
    int metamorphosis;
    int searing_pain;
    int shadow_bolt;
    int shadow_burn;
    int siphon_life;
    int unstable_affliction;
    glyphs_t() { memset( (void*) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  warlock_t( sim_t* sim, std::string& name ) : player_t( sim, WARLOCK, name ) 
  {
    // Active
    active_pet         = 0;
    active_corruption  = 0;
    active_curse       = 0;
    active_immolate    = 0;
    active_shadowflame = 0;
    active_pandemic    = 0;
    active_dots        = 0;
    affliction_effects = 0;

    // Gains
    gains_dark_pact  = get_gain( "dark_pact"  );
    gains_fel_armor  = get_gain( "fel_armor"  );
    gains_felhunter  = get_gain( "felhunter"  );
    gains_life_tap   = get_gain( "life_tap"   );
    gains_sacrifice  = get_gain( "sacrifice"  );
    gains_soul_leech = get_gain( "soul_leech" );

    // Procs
    procs_dark_pact     = get_proc( "dark_pact" );
    procs_life_tap      = get_proc( "life_tap" );
    procs_shadow_trance = get_proc( "shadow_trance" );

    // Up-Times
    uptimes_backdraft             = get_uptime( "backdraft"             );
    uptimes_demonic_empathy       = get_uptime( "demonic_empathy"       );
    uptimes_demonic_pact          = get_uptime( "demonic_pact"          );
    uptimes_demonic_soul          = get_uptime( "demonic_soul"          );
    uptimes_empowered_imp         = get_uptime( "empowered_imp"         );
    uptimes_eradication           = get_uptime( "eradication"           );
    uptimes_flame_shadow          = get_uptime( "flame_shadow"          );
    uptimes_molten_core           = get_uptime( "molten_core"           );
    uptimes_pyroclasm             = get_uptime( "pyroclasm"             );
    uptimes_shadow_flame          = get_uptime( "shadow_flame"          );
    uptimes_shadow_trance         = get_uptime( "shadow_trance"         );
    uptimes_shadow_vulnerability  = get_uptime( "shadow_vulnerability"  );
    uptimes_spirits_of_the_damned = get_uptime( "spirits_of_the_damned" );
  }

  // Character Definition
  virtual void      init_base();
  virtual void      reset();
  virtual bool      get_talent_trees( std::vector<int*>& affliction, std::vector<int*>& demonology, std::vector<int*>& destruction );
  virtual bool      parse_option ( const std::string& name, const std::string& value );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name );
  virtual int       primary_resource() { return RESOURCE_MANA; }
  virtual double    composite_spell_power( int school );

  // Event Tracking
  virtual void regen( double periodicity );
};

// ==========================================================================
// Warlock Pet
// ==========================================================================

enum pet_type_t { PET_NONE=0, PET_FELGUARD, PET_FELHUNTER, PET_IMP,
                  PET_VOIDWALKER, PET_SUCCUBUS, PET_INFERNAL, PET_DOOMGUARD };

struct warlock_pet_t : public pet_t
{
  int    pet_type;
  double damage_modifier;

  struct _buffs_t
  {
    int demonic_empathy;
    int demonic_empowerment;
    int demonic_frenzy;

    void reset() { memset( (void*) this, 0x00, sizeof( _buffs_t ) ); }
    _buffs_t() { reset(); }
  };
  _buffs_t _buffs;

  struct _expirations_t
  {
    event_t* demonic_empathy;
    event_t* demonic_pact;

    void reset() { memset( (void*) this, 0x00, sizeof( _expirations_t ) ); }
    _expirations_t() { reset(); }
  };
  _expirations_t _expirations;

  warlock_pet_t( sim_t* sim, player_t* owner, const std::string& pet_name, int pt ) :
    pet_t( sim, owner, pet_name ), pet_type(pt), damage_modifier(1.0)
  {
  }

  virtual void init_base()
  {
    warlock_t* o = owner -> cast_warlock();

    attribute_base[ ATTR_STRENGTH  ] = 314;
    attribute_base[ ATTR_AGILITY   ] =  90;
    attribute_base[ ATTR_STAMINA   ] = 328;
    attribute_base[ ATTR_INTELLECT ] = 150;
    attribute_base[ ATTR_SPIRIT    ] = 209;

    attribute_multiplier_initial[ ATTR_STAMINA   ] *= 1.0 + ( o -> talents.fel_stamina  * 0.03 +
                                                              o -> talents.fel_vitality * 0.05 );

    attribute_multiplier_initial[ ATTR_INTELLECT ] *= 1.0 + ( o -> talents.fel_intellect * 0.03 +
                                                              o -> talents.fel_vitality  * 0.05 );

    if( sim -> patch.before( 3, 1, 0 ) )
    {
        stamina_per_owner += o -> talents.fel_synergy * 0.05;
      intellect_per_owner += o -> talents.fel_synergy * 0.05;
    }

    initial_attack_power_per_strength = 2.0;
    initial_attack_crit_per_agility   = rating_t::interpolate( level, 0.01/16.0, 0.01/24.9, 0.01/52.1 );

    health_per_stamina = 10;
    mana_per_intellect = 15;
  }

  virtual void reset()
  {
    pet_t::reset();

    _buffs.reset();
    _expirations.reset();
  }

  virtual void summon()
  {
    warlock_t* o = owner -> cast_warlock();
    pet_t::summon();
    o -> active_pet = this;

    _buffs.reset();
    _expirations.reset();
  }

  void adjust_base_modifiers( action_t* a )
  {
    warlock_t* o = owner -> cast_warlock();

    if ( pet_type != PET_INFERNAL && pet_type != PET_DOOMGUARD )
    {
      a -> base_crit        += o -> talents.demonic_tactics * 0.02;
      a -> base_multiplier  *= 1.0 + o -> talents.unholy_power * 0.04;
    }

    if( pet_type == PET_FELGUARD )
    {
      a -> base_multiplier *= 1.0 + o -> talents.master_demonologist * 0.01;
    }
    else if( pet_type == PET_IMP )
    {
      if( a -> school == SCHOOL_FIRE ) 
      {
        a -> base_multiplier *= 1.0 + o -> talents.master_demonologist * 0.01;
        a -> base_crit       +=       o -> talents.master_demonologist * 0.01;
      }
    }
    else if( pet_type == PET_SUCCUBUS )
    {
      if( a -> school == SCHOOL_SHADOW ) 
      {
        a -> base_multiplier *= 1.0 + o -> talents.master_demonologist * 0.01;
        a -> base_crit       +=       o -> talents.master_demonologist * 0.01;
      }
    }
  }

  void adjust_player_modifiers( action_t* a )
  {
    warlock_t* o = owner -> cast_warlock();
    if( o -> talents.improved_demonic_tactics )
    {
      a -> player_crit += o -> composite_spell_crit() * o -> talents.improved_demonic_tactics * 0.10;
    }
  }

  virtual void attack_hit_event( attack_t* a );
  virtual void  spell_hit_event( spell_t* s );
};

// ==========================================================================
// Warlock Pet Melee
// ==========================================================================

struct warlock_pet_melee_t : public attack_t
{
  warlock_pet_melee_t( player_t* player, const char* name ) : 
    attack_t( name, player, RESOURCE_NONE, SCHOOL_PHYSICAL )
  {
    warlock_pet_t* p = (warlock_pet_t*) player -> cast_pet();

    weapon = &( p -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;
    base_direct_dmg   = 1;
    background        = true;
    repeating         = true;

    p -> adjust_base_modifiers( this );

    base_multiplier *= p -> damage_modifier;
  }

  virtual void player_buff()
  {
    warlock_pet_t* p = (warlock_pet_t*) player -> cast_pet();
    warlock_t* o = p -> owner -> cast_warlock();
    attack_t::player_buff();
    if( p -> pet_type != PET_INFERNAL )
    {
      player_power += 0.57 * o -> composite_spell_power( SCHOOL_MAX );
      p -> adjust_player_modifiers( this );
    }
  }
};

// ==========================================================================
// Warlock Pet Attack
// ==========================================================================

struct warlock_pet_attack_t : public attack_t
{
  warlock_pet_attack_t( const char* n, player_t* player, int r=RESOURCE_MANA, int s=SCHOOL_PHYSICAL ) : 
    attack_t( n, player, r, s )
  {
    background = true;
    repeating  = true;
    warlock_pet_t* p = (warlock_pet_t*) player -> cast_pet();
    p -> adjust_base_modifiers( this );
  }

  virtual void execute()
  {
    warlock_pet_t* p = (warlock_pet_t*) player -> cast_pet();
    if( p -> _buffs.demonic_empathy > 0 )
    {
      p -> _buffs.demonic_empathy--;
      if( p -> _buffs.demonic_empathy == 0 ) event_t::early( p -> _expirations.demonic_empathy );
    }
    attack_t::execute();
  }

  virtual void player_buff()
  {
    warlock_pet_t* p = (warlock_pet_t*) player -> cast_pet();
    warlock_t* o = p -> owner -> cast_warlock();
    attack_t::player_buff();
    player_power += 0.57 * player -> cast_pet() -> owner -> composite_spell_power( SCHOOL_MAX );
    if( p -> _buffs.demonic_empathy ) player_multiplier *= 1.0 + o -> talents.demonic_empathy * 0.01;
    p -> adjust_player_modifiers( this );
  }

  virtual double execute_time()
  {
    return cooldown;
  }

};

// ==========================================================================
// Warlock Pet Spell
// ==========================================================================

struct warlock_pet_spell_t : public spell_t
{
  warlock_pet_spell_t( const char* n, player_t* player, int r=RESOURCE_MANA, int s=SCHOOL_SHADOW ) : 
    spell_t( n, player, r, s )
  {
    warlock_pet_t* p = (warlock_pet_t*) player -> cast_pet();
    p -> adjust_base_modifiers( this );
  }

  virtual void execute()
  {
    warlock_pet_t* p = (warlock_pet_t*) player -> cast_pet();
    if( p -> _buffs.demonic_empathy > 0 )
    {
      p -> _buffs.demonic_empathy--;
      if( p -> _buffs.demonic_empathy == 0 ) event_t::early( p -> _expirations.demonic_empathy );
    }
    spell_t::execute();
  }

  virtual void player_buff()
  {
    warlock_pet_t* p = (warlock_pet_t*) player -> cast_pet();
    warlock_t* o = p -> owner -> cast_warlock();
    spell_t::player_buff();
    player_power += 0.15 * player -> cast_pet() -> owner -> composite_spell_power( SCHOOL_MAX );
    if( p -> _buffs.demonic_empathy ) player_multiplier *= 1.0 + o -> talents.demonic_empathy * 0.01;
    p -> adjust_player_modifiers( this );
  }
};

// ==========================================================================
// Pet Imp
// ==========================================================================

struct imp_pet_t : public warlock_pet_t
{
  struct fire_bolt_t : public warlock_pet_spell_t
  {
    fire_bolt_t( player_t* player ):
      warlock_pet_spell_t( "fire_bolt", player, RESOURCE_MANA, SCHOOL_FIRE ) 
    {
      imp_pet_t* p = (imp_pet_t*) player -> cast_pet();
      warlock_t* o = p -> owner -> cast_warlock();

      static rank_t ranks[] =
      {
        { 78, 8, 203, 227, 0, 180 },
        { 68, 8, 110, 124, 0, 145 },
        { 58, 7,  83,  93, 0, 115 },
        { 0,  0 }
      };
      init_rank( ranks );

      base_cost         = 0; // FIXME! When pets no longer have infinite mana......
      base_execute_time = 2.5;
      direct_power_mod  = ( 2.0 / 3.5 );
      may_crit          = true;

      base_execute_time -= 0.25 * ( o -> talents.improved_fire_bolt + o -> talents.demonic_power );

      base_multiplier *= 1.0 + ( o -> talents.improved_imp  * 0.10 +
                                 o -> talents.empowered_imp * 0.05 +
                                 o -> glyphs.imp * ( ( sim -> patch.before( 3, 1, 0 ) ) ? 0.10 : 0.20 ) );
    }
    virtual void player_buff()
    {
      imp_pet_t* p = (imp_pet_t*) player -> cast_pet();
      warlock_pet_spell_t::player_buff();
      if( p -> _buffs.demonic_empowerment ) player_crit += 0.20;
    }
  };

  imp_pet_t( sim_t* sim, player_t* owner ) :
    warlock_pet_t( sim, owner, "imp", PET_IMP )
  {
    action_list_str = "fire_bolt";
  }

  virtual void init_base()
  {
    warlock_pet_t::init_base();

    attribute_base[ ATTR_STRENGTH  ] = 297;
    attribute_base[ ATTR_AGILITY   ] =  79;
    attribute_base[ ATTR_STAMINA   ] = 118;
    attribute_base[ ATTR_INTELLECT ] = 369;
    attribute_base[ ATTR_SPIRIT    ] = 367;

    resource_base[ RESOURCE_HEALTH ] = 4011;
    resource_base[ RESOURCE_MANA   ] = 1175;

    health_per_stamina = 2;
    mana_per_intellect = 5;
  }

  virtual void summon()
  {
    warlock_pet_t::summon();
    schedule_ready();
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if( name == "fire_bolt" ) return new fire_bolt_t( this );

    return player_t::create_action( name, options_str );
  }

  virtual void spell_hit_event( spell_t* s );
};

// ==========================================================================
// Pet Felguard
// ==========================================================================

struct felguard_pet_t : public warlock_pet_t
{
  struct cleave_t : public warlock_pet_attack_t
  {
    cleave_t( player_t* player ) : 
      warlock_pet_attack_t( "cleave", player, RESOURCE_MANA, SCHOOL_PHYSICAL )
    {
      felguard_pet_t* p = (felguard_pet_t*) player -> cast_pet();
      weapon   = &( p -> main_hand_weapon );
      cooldown = 6.0;
      base_direct_dmg = util_t::ability_rank( p -> level,  124.0,76,  78.0,68,  50.0,0 );
    }
    virtual void player_buff()
    {
      felguard_pet_t* p = (felguard_pet_t*) player -> cast_pet();
      warlock_t*      o = p -> owner -> cast_warlock();

      warlock_pet_attack_t::player_buff();
      player_power *= 1.0 + p -> _buffs.demonic_frenzy * ( 0.05 + o -> talents.demonic_brutality * 0.01 );
      if( o -> glyphs.felguard ) player_power *= 1.20;
    }
  };

  struct melee_t : public warlock_pet_melee_t
  {
    melee_t( player_t* player ) : 
      warlock_pet_melee_t( player, "felguard_melee" )
    {
    }
    virtual double execute_time()
    {
      felguard_pet_t* p = (felguard_pet_t*) player -> cast_pet();
      double t = attack_t::execute_time();
      if( p -> _buffs.demonic_empowerment ) t *= 1.0 / 1.20;
      return t;
    }

    virtual void player_buff()
    {
      felguard_pet_t* p = (felguard_pet_t*) player -> cast_pet();
      warlock_t*      o = p -> owner -> cast_warlock();

      warlock_pet_melee_t::player_buff();
      player_power *= 1.0 + p -> _buffs.demonic_frenzy * ( 0.05 + o -> talents.demonic_brutality * 0.01 );

      if( o -> glyphs.felguard ) player_power *= 1.20;
    }
    virtual void assess_damage( double amount, int dmg_type )
    {
      felguard_pet_t* p = (felguard_pet_t*) player -> cast_pet();

      attack_t::assess_damage( amount, dmg_type );
      if( p -> _buffs.demonic_frenzy < 10 ) p -> _buffs.demonic_frenzy++;
    }    
  };

  cleave_t* cleave;
  melee_t*  melee;

  felguard_pet_t( sim_t* sim, player_t* owner ) :
    warlock_pet_t( sim, owner, "felguard", PET_FELGUARD ), melee(0)
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.damage     = 412.5;
    main_hand_weapon.swing_time = 2.0;

    damage_modifier = 1.05;
  }
  virtual void init_base()
  {
    warlock_pet_t::init_base();

    resource_base[ RESOURCE_HEALTH ] = 1627;
    resource_base[ RESOURCE_MANA   ] = 3331;

    base_attack_power = -20;

    melee  = new  melee_t( this );
    cleave = new cleave_t( this );
  }
  virtual void summon()
  {
    warlock_pet_t::summon();
    melee  -> schedule_execute(); 
    cleave -> schedule_execute();
  }
};

// ==========================================================================
// Pet Felhunter
// ==========================================================================

struct felhunter_pet_t : public warlock_pet_t
{
  // TODO: Need to add fel intelligence on the warlock while felhunter is out
  // This is +48 int / + 64 spi at rank 5, plus 5%/10% if talented in the affliction tree
  // These do NOT stack with Prayer of Spirit, or with Arcane Intellect/Arcane Brilliance
  struct shadow_bite_t : public warlock_pet_attack_t
  {
    shadow_bite_t( player_t* player ) : 
      warlock_pet_attack_t( "shadow_bite", player, RESOURCE_MANA, SCHOOL_SHADOW )
    {
      felhunter_pet_t* p = (felhunter_pet_t*) player -> cast_pet();
      cooldown = 6.0;
      base_direct_dmg = util_t::ability_rank( p -> level,  118.0,74,  101.0,66,  88.0,0 );
    }
    virtual void player_buff()
    {
      felhunter_pet_t* p = (felhunter_pet_t*) player -> cast_pet();
      warlock_t*      o = p -> owner -> cast_warlock();
      warlock_pet_attack_t::player_buff();
      player_multiplier *= 1.0 + o -> active_dots * 0.05;
      if( o -> glyphs.felhunter ) player_crit += 0.06;
    }
  };

  warlock_pet_melee_t* melee;
  shadow_bite_t* shadow_bite;

  felhunter_pet_t( sim_t* sim, player_t* owner ) :
    warlock_pet_t( sim, owner, "felhunter", PET_FELHUNTER ), melee(0)
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.damage     = 412.5;
    main_hand_weapon.swing_time = 2.0;

    damage_modifier = 0.8;
  }
  virtual void init_base()
  {
    warlock_pet_t::init_base();

    base_attack_power = -20;

    resource_base[ RESOURCE_HEALTH ] = 1468;
    resource_base[ RESOURCE_MANA   ] = 1559;

    melee       = new warlock_pet_melee_t( this, "felhunter_melee" );
    shadow_bite = new       shadow_bite_t( this );
  }
  virtual void summon()
  {
    warlock_pet_t::summon();
    melee       -> schedule_execute();
    shadow_bite -> schedule_execute();
  }
};

// ==========================================================================
// Pet Succubus
// ==========================================================================

struct succubus_pet_t : public warlock_pet_t
{
  struct lash_of_pain_t : public warlock_pet_attack_t
  {
    lash_of_pain_t( player_t* player ) : 
      warlock_pet_attack_t( "lash_of_pain", player, RESOURCE_MANA, SCHOOL_SHADOW )
    {
      succubus_pet_t* p = (succubus_pet_t*) player -> cast_pet();
      warlock_t*      o = p -> owner -> cast_warlock();

      cooldown   = 12.0;
      base_direct_dmg = util_t::ability_rank( p -> level,  237.0,80,  193.0,74,  123.0,68,  99.0,0 );
      cooldown  -= 3.0 * ( o -> talents.improved_lash_of_pain + o -> talents.demonic_power );
    }
  };

  warlock_pet_melee_t* melee;
  lash_of_pain_t*      lash_of_pain;

  succubus_pet_t( sim_t* sim, player_t* owner ) :
    warlock_pet_t( sim, owner, "succubus", PET_SUCCUBUS ), melee(0)
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.damage     = 412.5;
    main_hand_weapon.swing_time = 2.0;

    damage_modifier = 1.05;
  }
  virtual void init_base()
  {
    warlock_pet_t::init_base();

    base_attack_power = -20;

    resource_base[ RESOURCE_HEALTH ] = 1468;
    resource_base[ RESOURCE_MANA   ] = 1559;

    melee        = new warlock_pet_melee_t( this, "succubus_melee" );
    lash_of_pain = new      lash_of_pain_t( this );
  }
  virtual void summon()
  {
    warlock_pet_t::summon();
    melee        -> schedule_execute();
    lash_of_pain -> schedule_execute();
  }
};

// ==========================================================================
// Pet Infernal
// ==========================================================================

struct infernal_pet_t : public warlock_pet_t
{
  struct infernal_immolation_t : public warlock_pet_spell_t
  {
    infernal_immolation_t( player_t* player ) : 
      warlock_pet_spell_t( "immolation", player, RESOURCE_NONE, SCHOOL_FIRE )
    {
      base_direct_dmg   = 40;
      direct_power_mod  = 0.2;
      background        = true;
      repeating         = true;
    }

    virtual double execute_time()
    {
      // immolation is an aura that ticks every 2 seconds
      return 2.0;
    }

    virtual void player_buff()
    {
      // immolation uses the master's spell power, not the infernal's
      warlock_pet_t* p = (warlock_pet_t*) player -> cast_pet();
      warlock_t* o = p -> owner -> cast_warlock();
      spell_t::player_buff();
      player_power += o -> composite_spell_power( school );
    }
  };

  warlock_pet_melee_t*   melee;
  infernal_immolation_t* immolation;

  infernal_pet_t( sim_t* sim, player_t* owner ) :
    warlock_pet_t( sim, owner, "infernal", PET_INFERNAL ), melee(0)
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.damage     = 412.5;
    main_hand_weapon.swing_time = 2.0;

    damage_modifier = 3.20;
  }
  virtual void init_base()
  {
    warlock_pet_t::init_base();

    attribute_base[ ATTR_STRENGTH  ] = 331;
    attribute_base[ ATTR_AGILITY   ] = 113;
    attribute_base[ ATTR_STAMINA   ] = 361;
    attribute_base[ ATTR_INTELLECT ] =  65;
    attribute_base[ ATTR_SPIRIT    ] = 109;

    resource_base[ RESOURCE_HEALTH ] = 20300;
    resource_base[ RESOURCE_MANA   ] = 0;

    base_attack_power = -20;

    melee      = new   warlock_pet_melee_t( this, "infernal_melee" );
    immolation = new infernal_immolation_t( this );
  }
  virtual void reset()
  {
    pet_t::reset();
  }
  virtual void summon()
  {
    warlock_pet_t::summon();
    melee      -> schedule_execute();
    immolation -> schedule_execute();
  }
  virtual double composite_attack_hit() { return 0; }
  virtual double composite_spell_hit()  { return 0; }
};

// ==========================================================================
// Pet Doomguard
// ==========================================================================

struct doomguard_pet_t : public warlock_pet_t
{
  warlock_pet_melee_t* melee;

  doomguard_pet_t( sim_t* sim, player_t* owner ) :
    warlock_pet_t( sim, owner, "doomguard", PET_DOOMGUARD ), melee(0)
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.damage     = 412.5;
    main_hand_weapon.swing_time = 2.0;

    damage_modifier = ( sim -> patch.before( 3, 1, 0 ) ) ? 2.978 : 1.98;
  }

  virtual void init_base()
  {
    warlock_pet_t::init_base();

    resource_base[ RESOURCE_HEALTH ] = 18000;
    resource_base[ RESOURCE_MANA   ] = 3000;

    base_attack_power = -20;

    melee = new warlock_pet_melee_t( this, "doomguard_melee" );
  }
  virtual void reset()
  {
    pet_t::reset();
  }
  virtual void summon()
  {
    warlock_pet_t::summon();
    melee -> schedule_execute();
    schedule_ready();
  }
  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    return player_t::create_action( name, options_str );
  }
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// ==========================================================================
// Warlock Spell
// ==========================================================================

struct warlock_spell_t : public spell_t
{
  bool decimation_trigger, decimation_consumer;

  warlock_spell_t( const char* n, player_t* player, int s, int t ) : 
    spell_t( n, player, RESOURCE_MANA, s, t ), decimation_trigger(false), decimation_consumer(false)
  {
  }

  // Overridden Methods
  virtual double haste();
  virtual double execute_time();
  virtual double gcd();
  virtual void   player_buff();
  virtual void   target_debuff( int dmg_type );
  virtual void   execute();
  virtual void   tick();
};

// trigger_tier7_2pc =======================================================

static void trigger_tier7_2pc( spell_t* s )
{
  warlock_t* p = s -> player -> cast_warlock();

  if(   p ->  gear.tier7_2pc &&
      ! p -> buffs.tier7_2pc )
  {
    p -> buffs.tier7_2pc = s -> sim -> roll( 0.15 );

    if( p -> buffs.tier7_2pc ) p -> aura_gain( "Demonic Soul" );
  }
}

// trigger_tier5_4pc ========================================================

static void trigger_tier5_4pc( spell_t*  s,
                               action_t* dot_spell )
{
  warlock_t* p = s -> player -> cast_warlock();

  if( p -> gear.tier5_4pc )
  {
    if( dot_spell )
    {
      dot_spell -> base_tick_dmg *= 1.10;
    }
  }
}

// trigger_tier4_2pc ========================================================

static void trigger_tier4_2pc( spell_t* s )
{
  struct expiration_t : public event_t
  {
    int school;
    expiration_t( sim_t* sim, warlock_t* p, spell_t* s ) : event_t( sim, p ), school( s -> school )
    {
      name = "Tier4 2-Piece Buff Expiration";
      if( school == SCHOOL_SHADOW )
      {
        p -> aura_gain( "flame_shadow" );
        p -> _buffs.flame_shadow = 1;
      }
      else
      {
        p -> aura_gain( "shadow_flame" );
        p -> _buffs.shadow_flame = 1;
      }
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      warlock_t* p = player -> cast_warlock();
      if( school == SCHOOL_SHADOW )
      {
        p -> aura_loss( "flame_shadow" );
        p -> _buffs.flame_shadow = 0;
        p -> _expirations.flame_shadow = 0;
      }
      else
      {
        p -> aura_loss( "shadow_flame" );
        p -> _buffs.shadow_flame = 0;
        p -> _expirations.shadow_flame = 0;
      }
    }
  };
  
  warlock_t* p = s -> player -> cast_warlock();

  if( p -> gear.tier4_2pc )
  {
    if( s -> sim -> roll( 0.05 ) )
    {
      event_t*& e = ( s -> school == SCHOOL_SHADOW ) ? p -> _expirations.shadow_flame : p -> _expirations.flame_shadow;

      if( e )
      {
        e -> reschedule( 15.0 );
      }
      else
      {
        e = new ( s -> sim ) expiration_t( s -> sim, p, s );
      }
    }
  }
}

// decrement_shadow_vulnerability ===========================================

static void decrement_shadow_vulnerability( warlock_t* p )
{
  if( p -> _buffs.shadow_vulnerability_charges > 0 )
  {
    p -> _buffs.shadow_vulnerability_charges--;
    if( p -> _buffs.shadow_vulnerability_charges == 0 )
    {
      event_t::early( p -> _expirations.shadow_vulnerability );
    }
  }
}

// trigger_improved_shadow_bolt ===========================================
static void trigger_improved_shadow_bolt( spell_t* s )
{
  warlock_t* p = s -> player -> cast_warlock();

  if( ! p -> talents.improved_shadow_bolt ) return;

  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, warlock_t* p ) : event_t( sim )
    {
      if( sim -> log ) report_t::log( sim, "%s gains Improved Shadow Bolt %d", sim -> target -> name(), sim -> target -> debuffs.improved_shadow_bolt );
      name = "Improved Shadow Bolt Expiration";
      sim -> add_event( this, 30.0 );
      sim -> target -> debuffs.improved_shadow_bolt = p -> talents.improved_shadow_bolt;
    }
    virtual void execute()
    {
      if( sim -> log ) report_t::log( sim, "%s loses Improved Shadow Bolt", sim -> target -> name() );
      sim -> target -> debuffs.improved_shadow_bolt = 0;
      sim -> target -> expirations.improved_shadow_bolt = 0;
    }
  };
    
  target_t* t = s -> sim -> target;

  event_t*& e = t -> expirations.improved_shadow_bolt;
        
  if( e )
  {
    e -> reschedule( 30.0 );
  }
  else
  {
    e = new ( s -> sim ) expiration_t( s -> sim, p );
  }
}

// trigger_shadow_vulnerability =============================================

static void trigger_shadow_vulnerability( spell_t* s )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, warlock_t* p ) : event_t( sim, p )
    {
      name = "Shadow Vulnerability Expiration";
      p -> aura_gain( "Shadow Vulnerability" );
      p -> _buffs.shadow_vulnerability = sim -> current_time;
      sim -> add_event( this, 12.0 );
    }
    virtual void execute()
    {
      warlock_t* p = player -> cast_warlock();
      p -> aura_loss( "Shadow Vulnerability" );
      p -> _buffs.shadow_vulnerability = 0;
      p -> _buffs.shadow_vulnerability_charges = 0;
      p -> _expirations.shadow_vulnerability = 0;
    }
  };
  
  warlock_t* p = s -> player -> cast_warlock();

  if( p -> talents.improved_shadow_bolt && s -> sim -> patch.before( 3, 1, 0 ) )
  {
    p -> _buffs.shadow_vulnerability_charges = 4;

    event_t*& e = p -> _expirations.shadow_vulnerability;

    if( e )
    {
      e -> reschedule( 12.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim, p );
    }
  }
}

// stack_shadow_embrace =====================================================

static void stack_shadow_embrace( spell_t* s )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, warlock_t* p ) : event_t( sim, p )
    {
      name = "Shadow Embrace Expiration";
      p -> aura_gain( "Shadow Embrace" );
      if( sim -> patch.before( 3, 1, 0 ) )
        sim -> target -> debuffs.affliction_effects++;
      else
        p -> affliction_effects++;
      sim -> add_event( this, 12.0 );
    }
    virtual void execute()
    {
      warlock_t* p = player -> cast_warlock();
      p -> aura_loss( "Shadow Embrace" );
      p -> _buffs.shadow_embrace = 0;
      p -> _expirations.shadow_embrace = 0;
      if( sim -> patch.before( 3, 1, 0 ) )
        sim -> target -> debuffs.affliction_effects--;
      else
        p -> affliction_effects--;
    }
  };
  
  warlock_t* p = s -> player -> cast_warlock();

  if( p -> talents.shadow_embrace )
  {
    if( p -> _buffs.shadow_embrace < 2 ) p -> _buffs.shadow_embrace++;

    event_t*& e = p -> _expirations.shadow_embrace;

    if( e )
    {
      e -> reschedule( 12.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim, p );
    }
  }
}

// trigger_nightfall ========================================================

static void trigger_nightfall( spell_t* s )
{
  warlock_t* p = s -> player -> cast_warlock();

  if( p -> talents.nightfall && ! p -> _buffs.shadow_trance )
  {
    if( s -> sim -> roll( 0.02 * p -> talents.nightfall ) )
    {
      p -> procs_shadow_trance -> occur();
      p -> aura_gain( "Shadow Trance" );
      p -> _buffs.shadow_trance = 1;
    }
  }
}

// trigger_corruption_glyph =================================================

static void trigger_corruption_glyph( spell_t* s )
{
  warlock_t* p = s -> player -> cast_warlock();

  if( p -> glyphs.corruption && ! p -> _buffs.shadow_trance )
  {
    if( s -> sim -> roll( 0.04 ) )
    {
      p -> procs_shadow_trance -> occur();
      p -> aura_gain( "Shadow Trance" );
      p -> _buffs.shadow_trance = 1;
    }
  }
}

// trigger_soul_leech =======================================================

static void trigger_soul_leech( spell_t* s )
{
  warlock_t* p = s -> player -> cast_warlock();

  if( p -> talents.soul_leech )
  {
    if( s -> sim -> roll( 0.10 * p -> talents.soul_leech ) )
    {
      p -> resource_gain( RESOURCE_HEALTH, s -> direct_dmg * 0.20 );

      if( p -> talents.improved_soul_leech )
      {
        double amount = p -> resource_max[ RESOURCE_MANA ] * p -> talents.improved_soul_leech * 0.01;

        p -> resource_gain( RESOURCE_MANA, amount, p -> gains_soul_leech );
      }
    }
  }
}

// trigger_backdraft ========================================================

static void trigger_backdraft( spell_t* s )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, warlock_t* p ) : event_t( sim, p )
    {
      name = "Backdraft Expiration";
      p -> aura_gain( "Backdraft" );
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      warlock_t* p = player -> cast_warlock();
      p -> aura_loss( "Backdraft" );
      p -> _buffs.backdraft = 0;
      p -> _expirations.backdraft = 0;
    }
  };
  
  warlock_t* p = s -> player -> cast_warlock();

  if( p -> talents.backdraft )
  {
    p -> _buffs.backdraft = 3;

    event_t*& e = p -> _expirations.backdraft;

    if( e )
    {
      e -> reschedule( 15.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim, p );
    }
  }
}

// trigger_haunt ========================================================

static void trigger_haunted( spell_t* s )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, warlock_t* p ) : event_t( sim, p )
    {
      name = "Haunt Expiration";
      p -> aura_gain( "Haunted" );
      p -> _buffs.haunted = 1;
      if( sim -> patch.before( 3, 1, 0 ) )
        sim -> target -> debuffs.affliction_effects++;
      else
        p -> affliction_effects++;
      sim -> add_event( this, 12.0 );
    }
    virtual void execute()
    {
      warlock_t* p = player -> cast_warlock();
      p -> aura_loss( "Haunted" );
      p -> _buffs.haunted = 0;
      if( sim -> patch.before( 3, 1, 0 ) )
        sim -> target -> debuffs.affliction_effects--;
      else
        p -> affliction_effects--;
      p -> _expirations.haunted = 0;
    }
  };
  
  warlock_t* p = s -> player -> cast_warlock();
  event_t*&  e = p -> _expirations.haunted;

  if( e )
  {
    e -> reschedule( 12.0 );
  }
  else
  {
    e = new ( s -> sim ) expiration_t( s -> sim, p );
  }
}

// trigger_molten_core =====================================================

static void trigger_molten_core( spell_t* s )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, warlock_t* p ) : event_t( sim, p )
    {
      name = "Molten Core Expiration";
      p -> aura_gain( "Molten Core" );
      p -> _buffs.molten_core = sim -> current_time;
      sim -> add_event( this, 12.0 );
    }
    virtual void execute()
    {
      warlock_t* p = player -> cast_warlock();
      p -> aura_loss( "Molten Core" );
      p -> _buffs.molten_core = 0;
      p -> _expirations.molten_core = 0;
    }
  };

  if( s -> school != SCHOOL_SHADOW ) return;
  
  warlock_t* p = s -> player -> cast_warlock();

  if( ! p -> talents.molten_core ) return;

  if( s -> sim -> roll( p -> talents.molten_core * 0.05 ) )
  {
    event_t*&  e = p -> _expirations.molten_core;

    if( e )
    {
      e -> reschedule( 12.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim, p );
    }
  }
}

// trigger_decimation =====================================================

static void trigger_decimation( warlock_spell_t* s )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, warlock_t* p ) : event_t( sim, p )
    {
      name = "Decimation Expiration";
      p -> aura_gain( "Decimation" );
      //Assumes 1.5 second travel time on triggering spells
      sim -> add_event( this, 11.5 );
    }
    virtual void execute()
    {
      warlock_t* p = player -> cast_warlock();
      p -> aura_loss( "Decimation" );
      if( p -> _buffs.decimation == 2 )
        p -> _buffs.decimation = 1;
      else if( p -> _buffs.decimation != 1 )
        p -> _buffs.decimation = 0;
      p -> _expirations.decimation = 0;
    }
  };
  
  warlock_t* p = s -> player -> cast_warlock();

  if( ( ! p -> talents.decimation ) || ( s -> sim -> target -> health_percentage() > 35 ) ) return;

  if( s -> decimation_trigger && s -> result_is_hit() )
  {
    event_t*&  e = p -> _expirations.decimation;
    if( e )
    {
      //Assumes 1.5 second travel time on triggering spells
      e -> reschedule( 11.5 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim, p );
    }
  }

  switch( p -> _buffs.decimation )
  {
  case 0:
    // No decimation buff, no trigger in air
    if( s -> decimation_trigger && s -> result_is_hit() )
      p -> _buffs.decimation = 1;
    break;
  case 1:
    // No decimation buff, trigger in air
    if( s -> decimation_trigger )
      p -> _buffs.decimation = ( s -> result_is_hit() ) ? 2 : 3;
    break;
  case 2:
    // Decimation buff, trigger in air
    if( s -> decimation_consumer )
      p -> _buffs.decimation = 1;
    else if( ! ( s -> decimation_trigger && s -> result_is_hit() ) )
      p -> _buffs.decimation = 3;
    break;
  case 3:
    // Decimation buff, no trigger in air
    if( s -> decimation_consumer )
      p -> _buffs.decimation = 0;
    else if( s -> decimation_trigger && s -> result_is_hit() )
      p -> _buffs.decimation = 2;
    break;
  }
}

// trigger_eradication =====================================================

static void trigger_eradication( spell_t* s )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, warlock_t* p ) : event_t( sim, p )
    {
      name = "Eradication Expiration";
      p -> aura_gain( "Eradication" );
      if ( sim -> patch.before( 3, 1, 0 ) )
      {
        p -> _buffs.eradication = 1;
        sim -> add_event( this, 12.0 );
      }
      else
      {
        p -> _buffs.eradication = 1;
        sim -> add_event( this, 10.0 );
      }
    }
    virtual void execute()
    {
      warlock_t* p = player -> cast_warlock();
      p -> aura_loss( "Eradication" );
      p -> _buffs.eradication = 0;
      p -> _expirations.eradication = 0;
    }
  };
  
  warlock_t* p = s -> player -> cast_warlock();

  if( ! p -> talents.eradication ) return;

  if( s -> sim -> patch.before( 3, 1, 0 ) )
  {
    if( s -> sim -> cooldown_ready( p -> _cooldowns.eradication ) )
    {
      if( s -> sim -> roll( 0.01 + p -> talents.eradication * 0.03 ) )
      {
        p -> _cooldowns.eradication = s -> sim -> current_time + 30;
        new ( s -> sim ) expiration_t( s -> sim, p );
      }
    }
  }
  else
  {
    if( s -> sim -> roll( 0.06 ) )
    {
      event_t*&  e = p -> _expirations.eradication;
      if( e )
      {
        e -> reschedule( 10.0 );
      }
      else
      {
        e = new ( s -> sim ) expiration_t( s -> sim, p );
      }
    }
  }
}

// trigger_deaths_embrace ===================================================

static int trigger_deaths_embrace( spell_t* s )
{
  warlock_t* p = s -> player -> cast_warlock();

  if( ! p -> talents.deaths_embrace ) return 0;

  if( s -> sim -> target -> health_percentage() < 35 )
  {
    return p -> talents.deaths_embrace * 4;
  }
  
  return 0;
}

// trigger_everlasting_affliction ===========================================

static void trigger_everlasting_affliction( spell_t* s )
{
  warlock_t* p = s -> player -> cast_warlock();

  if( ! p -> talents.everlasting_affliction ) return;

  if( ! p -> active_corruption ) return;

  if( s -> sim -> roll( p -> talents.everlasting_affliction * 0.20 ) )
  {
    p -> active_corruption -> refresh_duration();
  }
}

// trigger_pandemic ========================================================

static void trigger_pandemic( spell_t* s )
{
  warlock_t* p = s -> player -> cast_warlock();

  if( s -> sim -> patch.after( 3, 1, 0 ) || ! p -> talents.pandemic ) return;

  struct pandemic_t : public spell_t
  {
    pandemic_t( player_t* player ) : 
      spell_t( "pandemic", player, RESOURCE_MANA, SCHOOL_SHADOW, TREE_AFFLICTION )
    {
      background = true;
      proc = true;
      trigger_gcd = 0;
      base_hit += player -> cast_warlock() -> talents.suppression * 0.01;
    }
    virtual void target_debuff( int dmg_type ) 
    {
      spell_t::target_debuff( dmg_type );
      target_multiplier = 1.0;
    }
    virtual void player_buff()
    {
      spell_t::player_buff();
      player_multiplier = 1.0;
    }
  };

  // Currently, Pandemic takes player crit into account.

  if( s -> sim -> roll( s -> player_crit ) )
  {
    if( ! p -> active_pandemic ) p -> active_pandemic = new pandemic_t( p );

    p -> active_pandemic -> base_direct_dmg = s -> tick_dmg * p -> talents.pandemic / 3.0;
    p -> active_pandemic -> execute();
  }
}

// trigger_pyroclasm =====================================================

static void trigger_pyroclasm( spell_t* s )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, warlock_t* p ) : event_t( sim, p )
    {
      name = "Pyroclasm Expiration";
      p -> aura_gain( "Pyroclasm" );
      p -> _buffs.pyroclasm = 1;
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      warlock_t* p = player -> cast_warlock();
      p -> aura_loss( "Pyroclasm" );
      p -> _buffs.pyroclasm = 0;
      p -> _expirations.pyroclasm = 0;
    }
  };
  
  warlock_t* p = s -> player -> cast_warlock();

  if( ! p -> talents.pyroclasm ) return;

  event_t*&  e = p -> _expirations.pyroclasm;

  if( e )
  {
    e -> reschedule( 10.0 );
  }
  else
  {
    e = new ( s -> sim ) expiration_t( s -> sim, p );
  }
}

// trigger_empowered_imp ====================================================

static void trigger_empowered_imp( spell_t* s )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, warlock_t* o ) : event_t( sim, o )
    {
      name = "Empowered Imp Expiration";
      o -> aura_gain( "Empowered Imp" );
      o -> _buffs.empowered_imp = 1;
      sim -> add_event( this, 8.0 );
    }
    virtual void execute()
    {
      warlock_t* o = player -> cast_warlock();
      o -> aura_loss( "Empowered Imp" );
      o -> _buffs.empowered_imp = 0;
      o -> _expirations.empowered_imp = 0;
    }
  };
  
  imp_pet_t* p = (imp_pet_t*) s -> player -> cast_pet();
  warlock_t* o = p -> owner -> cast_warlock();

  if( ! o -> talents.empowered_imp ) return;

  if( s -> sim -> roll( o -> talents.empowered_imp / 3.0 ) )
  {
    event_t*& e = o -> _expirations.empowered_imp;

    if( e )
    {
      e -> reschedule( 8.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim, o );
    }
  }
}

// trigger_demonic_empathy_on_owner ========================================

static void trigger_demonic_empathy_on_owner( action_t* a )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, warlock_t* o ) : event_t( sim, o )
    {
      name = "Demonic Empathy Expiration";
      o -> aura_gain( "Demonic Empathy" );
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      warlock_t* o = player -> cast_warlock();
      o -> aura_loss( "Demonic Empathy" );
      o -> _buffs.demonic_empathy = 0;
      o -> _expirations.demonic_empathy = 0;
    }
  };
  
  warlock_pet_t* p = (warlock_pet_t*) a -> player -> cast_pet();
  warlock_t* o = p -> owner -> cast_warlock();

  if( ! o -> talents.demonic_empathy ) return;

  // demonic empathy only triggers on spells and abilities, not melee
  if( a -> resource != RESOURCE_MANA ) return;

  o -> _buffs.demonic_empathy = 3;

  event_t*& e = o -> _expirations.demonic_empathy;

  if( e )
  {
    e -> reschedule( 15.0 );
  }
  else
  {
    e = new ( a -> sim ) expiration_t( a -> sim, o );
  }
}

// trigger_demonic_empathy_on_pet ==========================================

static void trigger_demonic_empathy_on_pet( spell_t* s )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, warlock_pet_t* p ) : event_t( sim, p )
    {
      name = "Demonic Empathy Expiration";
      p -> aura_gain( "Demonic Empathy" );
      sim -> add_event( this, 15.0 );
    }
    virtual void execute()
    {
      warlock_pet_t* p = (warlock_pet_t*) player -> cast_pet();
      p -> aura_loss( "Demonic Empathy" );
      p -> _buffs.demonic_empathy = 0;
      p -> _expirations.demonic_empathy = 0;
    }
  };
  
  warlock_t* o = s -> player -> cast_warlock();
  warlock_pet_t* p = o -> active_pet;

  if( ! o -> talents.demonic_empathy ) return;

  p -> _buffs.demonic_empathy = 3;

  event_t*& e = p -> _expirations.demonic_empathy;

  if( e )
  {
    e -> reschedule( 15.0 );
  }
  else
  {
    e = new ( s -> sim ) expiration_t( s -> sim, p );
  }
}

// trigger_demonic_pact ====================================================

static void trigger_demonic_pact( action_t* a )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, warlock_pet_t* pet, double buff ) : event_t( sim, pet )
    {
      name = "Demonic Pact Expiration";
      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
        p -> aura_gain( "Demonic Pact" );
        p -> buffs.demonic_pact = buff; 
        p -> buffs.demonic_pact_pet = pet;
      }
      sim -> add_event( this, 12.0 );
    }
    virtual void execute()
    {
      warlock_pet_t* pet = (warlock_pet_t*) player -> cast_pet();
      for( player_t* p = sim -> player_list; p; p = p -> next )
      {
        p -> aura_loss( "Demonic Pact" );
        p -> buffs.demonic_pact = 0;    
        p -> buffs.demonic_pact_pet = 0;
      }
      pet -> _expirations.demonic_pact = 0;
    }
  };
  
  warlock_pet_t* p = (warlock_pet_t*) a -> player -> cast_pet();
  warlock_t* o = p -> owner -> cast_warlock();

  if( ! o -> talents.demonic_pact ) return;

  double buff = 0.10 * ( o -> composite_spell_power( SCHOOL_MAX )
                     - ( o -> spell_power_per_spirit * o -> spirit() ) ); // Does not currently include spell power from spirit

  if( buff < p -> buffs.demonic_pact ) return;

  if( p -> buffs.demonic_pact_pet ) 
  {
    if( p -> buffs.demonic_pact == buff &&
        p -> buffs.demonic_pact_pet == p )
    {
      // If the SAME pet is putting up the SAME buff, then just let it reschedule the one in place.
    }
    else
    {
      event_t::cancel( ( (warlock_pet_t*) p -> buffs.demonic_pact_pet ) -> _expirations.demonic_pact );
    }
  }

  event_t*& e = p -> _expirations.demonic_pact;

  if( e )
  {
    e -> reschedule( 12.0 );
  }
  else
  {
    e = new ( a -> sim ) expiration_t( a -> sim, p, buff );
  }
}

// trigger_tier7_4pc ===============================================

static void trigger_tier7_4pc( spell_t* s )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Spirits of the Damned Expiration";
      player -> aura_gain( "Spirits of the Damned" );
      player -> attribute[ ATTR_SPIRIT ] += 300;
      player -> buffs.tier7_4pc = 1;
      sim -> add_event( this, 10.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Spirits of the Damned" );
      player -> attribute[ ATTR_SPIRIT ] -= 300;
      player -> expirations.tier7_4pc = 0;
      player -> buffs.tier7_4pc = 0;
    }
  };

  player_t* p = s -> player;

  if( p -> gear.tier7_4pc )
  {
    p -> procs.tier7_4pc -> occur();

    event_t*& e = p -> expirations.tier7_4pc;

    if( e )
    {
      e -> reschedule( 10.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim, p );
    }
  }
}

// trigger_life_tap_glyph ===============================================

static void trigger_life_tap_glyph( spell_t* s )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, warlock_t* p ) : event_t( sim, p )
    {
      p = player -> cast_warlock();
      name = "Life Tap Glyph Expiration";
      p -> aura_gain( "Life Tap Glyph" );
      p -> _buffs.life_tap_glyph = 1;
      sim -> add_event( this, 20.0 );
    }
    virtual void execute()
    {
      warlock_t* p = player -> cast_warlock();
      p -> aura_loss( "Life Tap Glyph" );
      p -> _expirations.life_tap_glyph = 0;
      p -> _buffs.life_tap_glyph = 0;
    }
  };

  warlock_t* p = s -> player -> cast_warlock();

  if( p -> glyphs.life_tap )
  {
    event_t*& e = p -> _expirations.life_tap_glyph;

    if( e )
    {
      e -> reschedule( 20.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim, p );
    }
  }
}

// trigger_ashtongue_talisman ===============================================

static void trigger_ashtongue_talisman( spell_t* s )
{
  struct expiration_t : public event_t
  {
    expiration_t( sim_t* sim, player_t* p ) : event_t( sim, p )
    {
      name = "Ashtongue Talisman Expiration";
      player -> aura_gain( "Ashtongue Talisman" );
      player -> spell_power[ SCHOOL_MAX ] += 220;
      sim -> add_event( this, 5.0 );
    }
    virtual void execute()
    {
      player -> aura_loss( "Ashtongue Talisman" );
      player -> spell_power[ SCHOOL_MAX ] -= 220;
      player -> expirations.ashtongue_talisman = 0;
    }
  };

  player_t* p = s -> player;

  if( p -> gear.ashtongue_talisman && s -> sim -> roll( 0.20 ) )
  {
    p -> procs.ashtongue_talisman -> occur();

    event_t*& e = p -> expirations.ashtongue_talisman;

    if( e )
    {
      e -> reschedule( 5.0 );
    }
    else
    {
      e = new ( s -> sim ) expiration_t( s -> sim, p );
    }
  }
}

// ==========================================================================
// Warlock Spell
// ==========================================================================

// warlock_spell_t::haste ===================================================

double warlock_spell_t::haste()
{
  warlock_t* p = player -> cast_warlock();
  double h = spell_t::haste();
  if( p -> _buffs.eradication )
  {
    if( sim -> patch.before( 3, 1, 0 ) )
    {
      h *= ( 1.0 / 1.20 );
    }
    else
    {
      double mod = 1 + p -> talents.eradication * 0.06;
      if( p -> talents.eradication == 3 ) mod += 0.02;
      h *= ( 1.0 / mod );
    }   
  }
  return h;
}

// warlock_spell_t::execute_time ============================================

double warlock_spell_t::execute_time()
{
  warlock_t* p = player -> cast_warlock();
  double t = spell_t::execute_time();
  if( t > 0 )
  {
    p -> uptimes_backdraft   -> update( p -> _buffs.backdraft   != 0 );
    p -> uptimes_eradication -> update( p -> _buffs.eradication != 0 );
    if( p -> _buffs.backdraft && tree == TREE_DESTRUCTION ) t *= 1.0 - p -> talents.backdraft * 0.10;
  }
  return t;
}

// warlock_spell_t::gcd ============================================

double warlock_spell_t::gcd() {
  double t = spell_t::gcd();
  warlock_t* p = player -> cast_warlock();
  if( p -> _buffs.backdraft && tree == TREE_DESTRUCTION ) t *= 1.0 - p -> talents.backdraft * 0.10;
  return t;
}

// warlock_spell_t::player_buff =============================================

void warlock_spell_t::player_buff()
{
  warlock_t* p = player -> cast_warlock();

  spell_t::player_buff();

  if( p -> _buffs.metamorphosis ) player_multiplier *= 1.20;

  if( p -> talents.malediction ) player_multiplier *= 1.0 + p -> talents.malediction * 0.01;

  if( school == SCHOOL_SHADOW )
  {
    player_multiplier *= 1.0 + trigger_deaths_embrace( this ) * 0.01;

    if( p -> _buffs.pet_sacrifice == PET_FELGUARD ) player_multiplier *= 1.0 + 0.07;
    if( p -> _buffs.pet_sacrifice == PET_SUCCUBUS ) player_multiplier *= 1.0 + 0.10;

    if( p -> _buffs.flame_shadow ) player_power += 135;
    p -> uptimes_flame_shadow -> update( p -> _buffs.flame_shadow != 0 );

    if( p -> _buffs.pyroclasm ) player_multiplier *= 1.0 + p -> talents.pyroclasm * 0.02;
    p -> uptimes_pyroclasm -> update( p -> _buffs.pyroclasm != 0 );
  }
  else if( school == SCHOOL_FIRE )
  {
    if( p -> _buffs.pet_sacrifice == PET_FELGUARD ) player_multiplier *= 1.0 + 0.07;
    if( p -> _buffs.pet_sacrifice == PET_IMP      ) player_multiplier *= 1.0 + 0.10;

    if( p -> _buffs.shadow_flame ) player_power += 135;
    p -> uptimes_shadow_flame -> update( p -> _buffs.shadow_flame != 0 );

    if( p -> _buffs.molten_core ) player_multiplier *= 1.10;
    p -> uptimes_molten_core -> update( p -> _buffs.molten_core != 0 );

    if( p -> _buffs.pyroclasm ) player_multiplier *= 1.0 + p -> talents.pyroclasm * 0.02;
    p -> uptimes_pyroclasm -> update( p -> _buffs.pyroclasm != 0 );
  }

  p -> uptimes_spirits_of_the_damned -> update( p -> buffs.tier7_4pc != 0 );

  if( p -> active_pet )
  {
    if( p -> talents.master_demonologist )
    {
      if( p -> active_pet -> pet_type == PET_FELGUARD ) 
      {
        player_multiplier *= 1.0 + p -> talents.master_demonologist * 0.01;
      }
      else if( p -> active_pet -> pet_type == PET_IMP ) 
      {
        if( school == SCHOOL_FIRE )
        {
          player_multiplier *= 1.0 + p -> talents.master_demonologist * 0.01;
          player_crit       +=       p -> talents.master_demonologist * 0.01;
        }
      }
      else if( p -> active_pet -> pet_type == PET_SUCCUBUS ) 
      {
        if( school == SCHOOL_SHADOW )
        {
          player_multiplier *= 1.0 + p -> talents.master_demonologist * 0.01;
          player_crit       +=       p -> talents.master_demonologist * 0.01;
        }
      }
    }

    if( p -> talents.demonic_tactics )
    {
      player_crit += p -> talents.demonic_tactics * 0.02;
    }

    if( p -> _buffs.demonic_empathy ) player_multiplier *= 1.0 + p -> talents.demonic_empathy * 0.01;
    p -> uptimes_demonic_empathy -> update( p -> _buffs.demonic_empathy != 0 );

    if( may_crit )
    {
      if( p -> _buffs.empowered_imp ) player_crit += 0.20;
      p -> uptimes_empowered_imp -> update( p -> _buffs.empowered_imp != 0 );
    }

    if( p -> talents.demonic_pact )
    {
      p -> uptimes_demonic_pact -> update( p -> buffs.demonic_pact != 0 );
    }
  }
}

// warlock_spell_t::target_debuff ============================================

void warlock_spell_t::target_debuff( int dmg_type )
{
  warlock_t* p = player -> cast_warlock();

  spell_t::target_debuff( dmg_type );

  double stone_bonus = 0;

  if( dmg_type == DMG_OVER_TIME )
  {
    if( p -> _buffs.shadow_embrace && ( school == SCHOOL_SHADOW || sim -> patch.before( 3, 1, 0 ) ) )
    {
      target_multiplier *= 1.0 + p -> _buffs.shadow_embrace * p -> talents.shadow_embrace * 0.01;
    }
    if( p -> _buffs.haunted && ( school == SCHOOL_SHADOW || sim -> patch.before( 3, 1, 0 ) ) )
    {
      target_multiplier *= ( p -> glyphs.haunt ) ? 1.23 : 1.20;
    }

    if( p -> main_hand_weapon.buff == SPELL_STONE ) 
    {
      stone_bonus = 0.01;
      if( sim -> patch.after( 3, 1, 0 ) && p -> talents.master_conjuror > 0 )
        stone_bonus = p -> talents.master_conjuror * 0.02;
    }
  }
  else
  {
    if( p -> main_hand_weapon.buff == FIRE_STONE ) 
    {
      stone_bonus = 0.01;
      if( sim -> patch.after( 3, 1, 0 ) && p -> talents.master_conjuror > 0 )
        stone_bonus = p -> talents.master_conjuror * 0.02;
    }
  }
  if( stone_bonus > 0 )
  {
    // HACK: Fire/spell stone is additive with Emberstorm and Shadow Mastery
    if( school == SCHOOL_FIRE )
    {
      target_multiplier /= 1 + p -> talents.emberstorm * 0.03;
      target_multiplier *= 1 + p -> talents.emberstorm * 0.03 + stone_bonus;
    }
    else if( school == SCHOOL_SHADOW )
    {
      target_multiplier /= 1 + p -> talents.shadow_mastery * 0.03;
      target_multiplier *= 1 + p -> talents.shadow_mastery * 0.03 + stone_bonus;
    }
    else
    {
      target_multiplier *= 1 + stone_bonus;
    }
  }
}

// warlock_spell_t::execute ==================================================

void warlock_spell_t::execute()
{
  warlock_t* p = player -> cast_warlock();

  if( time_to_execute > 0 && tree == TREE_DESTRUCTION )
  {
    if( p -> _buffs.backdraft > 0 ) p -> _buffs.backdraft--;
  }
  if( p -> _buffs.demonic_empathy > 0 )
  {
    p -> _buffs.demonic_empathy--;
    if( p -> _buffs.demonic_empathy == 0 ) event_t::early( p -> _expirations.demonic_empathy );
  }

  spell_t::execute();

  if( result_is_hit() )
  {
    trigger_tier4_2pc( this );
    trigger_molten_core( this );

    if( result == RESULT_CRIT )
    {
      trigger_demonic_empathy_on_pet( this );
    }
  }

  if( may_crit && p -> _expirations.empowered_imp ) 
  {
    event_t::early( p -> _expirations.empowered_imp );
  }

  trigger_decimation( this );
}

// warlock_spell_t::tick =====================================================

void warlock_spell_t::tick()
{
  spell_t::tick();
  trigger_molten_core( this );
  trigger_tier7_2pc( this );
}

// Curse of Elements Spell ===================================================

struct curse_of_elements_t : public warlock_spell_t
{
  curse_of_elements_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "curse_of_elements", player, SCHOOL_SHADOW, TREE_AFFLICTION )
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 67, 3, 0, 0, 0, 260 },
      { 56, 2, 0, 0, 0, 200 },
      { 0,  0 }
    };
    init_rank( ranks );
    
    harmful = false;
    base_execute_time = 0; 
    base_cost        *= 1.0 - p -> talents.suppression * 0.02;
    base_hit         +=       p -> talents.suppression * 0.01;

    if( p -> talents.amplify_curse ) trigger_gcd = 1.0;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, warlock_t* p ) : event_t( sim, p )
      {
        name = "Cure of Elements Expiration";
        target_t* t = sim -> target;
        if( sim -> patch.before( 3, 1, 0 ) )
        {
          t -> debuffs.affliction_effects++;
          t -> debuffs.curse_of_elements = 10 + p -> talents.malediction;
        }
        else
        {
          p -> affliction_effects++;
          t -> debuffs.curse_of_elements = 13;
        }
        sim -> add_event( this, 300.0 );
      }
      virtual void execute()
      {
        warlock_t* p = player -> cast_warlock();
        target_t*  t = sim -> target;
        p -> active_curse = 0;
        t -> debuffs.curse_of_elements = 0;
        t -> expirations.curse_of_elements = 0;
        if( sim -> patch.before( 3, 1, 0 ) )
          t -> debuffs.affliction_effects--;
        else
          p -> affliction_effects--;
      }
    };

    warlock_spell_t::execute(); 

    if( result_is_hit() )
    {
      warlock_t* p = player -> cast_warlock();
      target_t* t = sim -> target;
      event_t::early( t -> expirations.curse_of_elements );
      t -> expirations.curse_of_elements = new ( sim ) expiration_t( sim, p );
      p -> active_curse = this;
    }
  }
  
  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if( p -> active_curse != 0 )
      return false;

    if( ! warlock_spell_t::ready() ) 
      return false;

    target_t* t = sim -> target;

    if( sim -> patch.before( 3, 1, 0 ) )
      return std::max( t -> debuffs.curse_of_elements, t -> debuffs.earth_and_moon ) < 10 + p -> talents.malediction;
    else
      return std::max( t -> debuffs.curse_of_elements, t -> debuffs.earth_and_moon ) < 13;
  }
};

// Curse of Agony Spell ===========================================================

struct curse_of_agony_t : public warlock_spell_t
{
  curse_of_agony_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "curse_of_agony", player, SCHOOL_SHADOW, TREE_AFFLICTION )
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 79, 9, 0, 0, 145, 0.10 },
      { 73, 8, 0, 0, 120, 0.10 },
      { 67, 7, 0, 0, 113, 265  },
      { 58, 6, 0, 0,  87, 215  },
      { 0, 0 }
    };
    init_rank( ranks );
      
    base_execute_time = 0; 
    base_tick_time    = 2.0; 
    num_ticks         = 12;
    tick_power_mod    = base_tick_time / 15.0;
    tick_power_mod    = 1.20 / num_ticks;  // Nerf Bat!

    base_cost       *= 1.0 - p -> talents.suppression * 0.02;
    base_hit        +=       p -> talents.suppression * 0.01;
    base_multiplier *= 1.0 + ( p -> talents.shadow_mastery          * 0.03 +
                               p -> talents.contagion               * 0.01 +
                               p -> talents.improved_curse_of_agony * 0.05 );

    if( p -> talents.amplify_curse ) trigger_gcd = 1.0;

    if( p -> glyphs.curse_of_agony )
    {
      num_ticks += 2;
      // after patch 3.0.8, the added ticks are double the base damage
      if( sim -> patch.after(3, 0, 8) ) base_td_init = ( base_td_init * 12 + base_td_init * 4 ) / 14;
    }
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();
    if( result_is_hit() )
    {
      p -> active_dots++;
      p -> active_curse = this;
      if( sim -> patch.before( 3, 1, 0 ) )
        sim -> target -> debuffs.affliction_effects++;
      else
        p -> affliction_effects++;
    }
  }

  virtual void last_tick()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::last_tick(); 
    p -> active_dots--;
    p -> active_curse = 0;
    if( sim -> patch.before( 3, 1, 0 ) )
        sim -> target -> debuffs.affliction_effects--;
      else
        p -> affliction_effects--;
  }
  
  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if( p -> active_curse != 0 )
      return false;

    if( ! warlock_spell_t::ready() ) 
      return false;

    return true;
  }
};

// Curse of Doom Spell ===========================================================

struct curse_of_doom_t : public warlock_spell_t
{
  curse_of_doom_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "curse_of_doom", player, SCHOOL_SHADOW, TREE_AFFLICTION )
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 3, 0, 0, 7300, 0.15 },
      { 70, 2, 0, 0, 4200, 380  },
      { 60, 1, 0, 0, 3200, 300  },
      { 0, 0 }
    };
    init_rank( ranks );
      
    base_execute_time = 0; 
    base_tick_time    = 60.0; 
    num_ticks         = 1;
    tick_power_mod    = 2.0;
    base_cost        *= 1.0 - p -> talents.suppression * 0.02;
    base_hit         +=       p -> talents.suppression * 0.01;

    if( p -> talents.amplify_curse ) trigger_gcd = 1.0;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();
    if( result_is_hit() )
    {
      p -> active_curse = this;
      p -> active_dots++;
      if( sim -> patch.before( 3, 1, 0 ) )
        sim -> target -> debuffs.affliction_effects++;
      else
        p -> affliction_effects++;
    }
  }

  virtual void assess_damage( double amount, 
                              int    dmg_type )
  {
    warlock_spell_t::assess_damage( amount, DMG_DIRECT );
  }

  virtual void last_tick()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::last_tick(); 
    p -> active_curse = 0;
    p -> active_dots--;
    if( sim -> patch.before( 3, 1, 0 ) )
        sim -> target -> debuffs.affliction_effects--;
      else
        p -> affliction_effects--;
  }
  
  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if( p -> active_curse != 0 )
      return false;

    if( ! warlock_spell_t::ready() ) 
      return false;

    return true;
  }
};

// Shadow Bolt Spell ===========================================================

struct shadow_bolt_t : public warlock_spell_t
{
  int shadow_trance;
  int backdraft;
  int isb_benefit;
  int isb_trigger;

  shadow_bolt_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "shadow_bolt", player, SCHOOL_SHADOW, TREE_DESTRUCTION ), 
    shadow_trance(0), backdraft(0), isb_benefit(0), isb_trigger(0)
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { "shadow_trance", OPT_INT, &shadow_trance },
      { "backdraft",     OPT_INT, &backdraft     },
      { "isb_benefit",   OPT_INT, &isb_benefit   },
      { "isb_trigger",   OPT_INT, &isb_trigger   },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 79, 13, 690, 770, 0, 0.17 },
      { 74, 12, 596, 664, 0, 0.17 },
      { 69, 11, 541, 603, 0, 420  },
      { 60, 10, 482, 538, 0, 380  },
      { 60,  9, 455, 507, 0, 370  },
      { 0, 0 }
    };
    init_rank( ranks );
    
    base_execute_time = 3.0; 
    may_crit          = true;
    direct_power_mod  = base_execute_time / 3.5;
    
    if( sim -> patch.before( 3, 1, 0 ) )
    {
      base_hit        += p -> talents.cataclysm * 0.01;
      base_cost       *= 1.0 - ( p -> talents.cataclysm  * 0.01 +
                                 p -> glyphs.shadow_bolt * 0.10 );
    }
    else
    {
      base_hit        += p -> talents.suppression * 0.01;
      base_cost       *= 1.0 - ( p -> talents.cataclysm  * 0.03 +
                             ( ( p -> talents.cataclysm ) ? 0.01 : 0 ) +
                                 p -> glyphs.shadow_bolt * 0.10 );
    }
    base_execute_time -=  p -> talents.bane * 0.1;
    base_multiplier   *= 1.0 + ( p -> talents.shadow_mastery       * 0.03 +
                                 p -> gear.tier6_4pc               * 0.06 +
                                 p -> talents.improved_shadow_bolt * ( ( sim -> patch.after( 3, 1, 0 ) ) ? 0.01 : 0 ) );
    base_crit         += p -> talents.devastation * 0.05;
    if( p -> gear.tier8_4pc )
      base_crit       += 0.05;
    base_crit         += p -> talents.backlash * 0.01;
    direct_power_mod  *= 1.0 + p -> talents.shadow_and_flame * 0.04;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.ruin * 0.20;

    decimation_trigger = true;
  }

  virtual double execute_time()
  {
    warlock_t* p = player -> cast_warlock();
    p -> uptimes_shadow_trance -> update( p -> _buffs.shadow_trance != 0 );
    if( p -> _buffs.shadow_trance ) return 0;
    return warlock_spell_t::execute_time();
  }
  
  virtual void schedule_execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::schedule_execute(); 
    if( p -> _buffs.shadow_trance )
    {
      p -> aura_loss( "Shadow Trance" );
      p -> _buffs.shadow_trance = 0;
    }
  }
  
  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute(); 
    if( result_is_hit() )
    {
      stack_shadow_embrace( this );
      trigger_soul_leech( this );
      trigger_tier5_4pc( this, p -> active_corruption );
      if( sim -> patch.before( 3, 1, 0 ) )
      {
        decrement_shadow_vulnerability( p );
        if( result == RESULT_CRIT ) trigger_shadow_vulnerability( this );
      }
      else
      {
        trigger_improved_shadow_bolt( this );
      }
    }
    if( p -> buffs.tier7_2pc )
    {
      p -> aura_loss( "Demonic Soul" );
      p -> buffs.tier7_2pc = 0;
    }
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();

    warlock_spell_t::player_buff();

    if( sim -> patch.before( 3, 1, 0 ) )
    {
      if( p -> _buffs.shadow_vulnerability ) player_multiplier *= 1.0 + p -> talents.improved_shadow_bolt * 0.02;
      p -> uptimes_shadow_vulnerability -> update( p -> _buffs.shadow_vulnerability != 0 );
    }

    if( p -> buffs.tier7_2pc ) player_crit += 0.10;
    p -> uptimes_demonic_soul -> update( p -> buffs.tier7_2pc != 0 );
  }
  
  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if( ! warlock_spell_t::ready() )
      return false;

    if( shadow_trance )
      if( ! p -> _buffs.shadow_trance )
        return false;

    if( backdraft )
      if( ! p -> _buffs.backdraft )
        return false;

    if( isb_benefit )
      if( ! sim -> time_to_think( p -> _buffs.shadow_vulnerability ) )
        return false;

    if( isb_trigger )
      if( sim -> time_to_think( p -> _buffs.shadow_vulnerability ) )
        return false;

    return true;
  }
};

// Chaos Bolt Spell ===========================================================

struct chaos_bolt_t : public warlock_spell_t
{
  int backdraft;
  int molten_core;

  chaos_bolt_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "chaos_bolt", player, SCHOOL_FIRE, TREE_DESTRUCTION ), backdraft(0), molten_core(0)
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { "backdraft",   OPT_INT, &backdraft   },
      { "molten_core", OPT_INT, &molten_core },
      { NULL }
    };
    parse_options( options, options_str );
      
    rank_t ranks[] =
    {
      { 80, 4, 1429, 1813, 0, 0.07 },
      { 75, 3, 1217, 1545, 0, 0.07 },
      { 70, 2, 1077, 1367, 0, 0.07 },
      { 60, 1,  837, 1061, 0, 0.09 },
      { 0, 0 }
    };
    
    if( sim -> P309 )
    {
      ranks[0].dd_min = 1243;
      ranks[0].dd_max = 1577;
      ranks[1].dd_min = 882;
      ranks[1].dd_max = 1120;
      ranks[2].dd_min = 781;
      ranks[2].dd_max = 991;
      ranks[3].dd_min = 607;
      ranks[3].dd_max = 769;
    }

    init_rank( ranks );
    
    base_execute_time = 2.5; 
    direct_power_mod  = base_execute_time / 3.5; 
    cooldown          = 12.0;
    if ( p -> glyphs.chaos_bolt ) cooldown -= 2;
    may_crit          = true;
    may_resist        = false;
    
    if( sim -> patch.before( 3, 1, 0 ) )
    {
      base_hit        += p -> talents.cataclysm * 0.01;
      base_cost       *= 1.0 - p -> talents.cataclysm * 0.01;
    }
    else
    {
      base_hit        += p -> talents.suppression * 0.01;
      base_cost       *= 1.0 - ( p -> talents.cataclysm * 0.03
                             + ( ( p -> talents.cataclysm ) ? 0.01 : 0 ) );
    }
    base_execute_time -=  p -> talents.bane * 0.1;
    base_multiplier   *= 1.0 + p -> talents.emberstorm  * 0.03;
    base_crit         += p -> talents.devastation * 0.05;
    base_crit         += p -> talents.backlash * 0.01;
    direct_power_mod  *= 1.0 + p -> talents.shadow_and_flame * 0.04;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.ruin * 0.20;
  }

  virtual void execute()
  {
    warlock_spell_t::execute(); 
    if( result_is_hit() )
    {
      trigger_soul_leech( this );
    }
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if( ! warlock_spell_t::ready() )
      return false;

    if( backdraft )
      if( ! p -> _buffs.backdraft )
        return false;

    if( molten_core )
    {
      if( ! sim -> time_to_think( p -> _buffs.molten_core ) )
        return false;

      if( sim -> current_time + execute_time() > p -> _expirations.molten_core -> occurs() )
        return false;
    }
          
    return true;
  }
};

// Death Coil Spell ===========================================================

struct death_coil_t : public warlock_spell_t
{
  death_coil_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "death_coil", player, SCHOOL_SHADOW, TREE_AFFLICTION )
  {
    warlock_t* p = player -> cast_warlock();
    
    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 78, 6, 790, 790, 0, 0.23 },
      { 73, 5, 670, 670, 0, 0.23 },
      { 68, 4, 519, 519, 0, 600  },
      { 58, 3, 400, 400, 0, 480  },
      { 0, 0 }
    };
    init_rank( ranks );
      
    base_execute_time = 0; 
    may_crit          = true; 
    binary            = true; 
    cooldown          = 120;
    direct_power_mod  = ( 1.5 / 3.5 ) / 2.0; 
      
    base_cost       *= 1.0 -  p -> talents.suppression * 0.02;
    base_multiplier *= 1.0 + p -> talents.shadow_mastery * 0.03;
    base_hit        += p -> talents.suppression * 0.01;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.ruin * 0.20;
  }

  virtual void execute() 
  {
    warlock_spell_t::execute();
    if( result_is_hit() )
    {
      player -> resource_gain( RESOURCE_HEALTH, direct_dmg );
      decrement_shadow_vulnerability( player -> cast_warlock() );
    }
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::player_buff();
    if( p -> _buffs.shadow_vulnerability ) player_multiplier *= 1.0 + p -> talents.improved_shadow_bolt * 0.02;
    p -> uptimes_shadow_vulnerability -> update( p -> _buffs.shadow_vulnerability != 0 );
  }
};

// Shadow Burn Spell ===========================================================

struct shadow_burn_t : public warlock_spell_t
{
  shadow_burn_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "shadow_burn", player, SCHOOL_SHADOW, TREE_DESTRUCTION )
  {
    warlock_t* p = player -> cast_warlock();

    assert( p -> talents.shadow_burn );

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 10, 775, 865, 0, 0.20 },
      { 75,  9, 662, 738, 0, 0.20 },
      { 70,  8, 597, 665, 0, 0.20 },
      { 63,  7, 518, 578, 0, 0.27 },
      { 56,  6, 450, 502, 0, 0.27 },
      { 0, 0 }
    };
    init_rank( ranks );
      
    may_crit         = true; 
    cooldown         = 15;
    direct_power_mod = ( 1.5 / 3.5 ); 
      
    if( sim -> patch.before( 3, 1, 0 ) )
    {
      base_hit        += p -> talents.cataclysm * 0.01;
      base_cost       *= 1.0 - p -> talents.cataclysm * 0.01;
    }
    else
    {
      base_hit        += p -> talents.suppression * 0.01;
      base_cost       *= 1.0 - ( p -> talents.cataclysm * 0.03
                             + ( ( p -> talents.cataclysm ) ? 0.01 : 0 ) );
    }
    base_multiplier *= 1.0 + p -> talents.shadow_mastery * 0.03;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.ruin * 0.20;
   }

  virtual void execute()
  {
    warlock_spell_t::execute(); 
    if( result_is_hit() )
    {
      trigger_soul_leech( this );
      decrement_shadow_vulnerability( player -> cast_warlock() );
    }
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::player_buff();
    if( p -> _buffs.shadow_vulnerability ) player_multiplier *= 1.0 + p -> talents.improved_shadow_bolt * 0.02;
    p -> uptimes_shadow_vulnerability -> update( p -> _buffs.shadow_vulnerability != 0 );
    if( p -> glyphs.shadow_burn )
    {
      if( sim -> target -> health_percentage() < 35 )
      {
        player_crit += 0.20;
      }
    }
  }
};

// Corruption Spell ===========================================================

struct corruption_t : public warlock_spell_t
{
  corruption_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "corruption", player, SCHOOL_SHADOW, TREE_AFFLICTION )
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 77, 10, 0, 0, 180, 0.14 },
      { 71,  9, 0, 0, 165, 0.14 },
      { 65,  8, 0, 0, 150, 370  },
      { 60,  7, 0, 0, 137, 340  },
      { 0, 0 }
    };
    init_rank( ranks );

    base_execute_time = 0.0; 
    base_tick_time    = 3.0; 
    num_ticks         = 6;
    tick_power_mod    = base_tick_time / 15.0;

    base_cost       *= 1.0 - p -> talents.suppression * 0.02;
    base_hit        +=  p -> talents.suppression * 0.01;
    base_multiplier *= 1.0 + ( p -> talents.shadow_mastery      * 0.03 +
                               p -> talents.contagion           * 0.01 +
                               p -> talents.improved_corruption * 0.02 +
                           ( ( p -> talents.siphon_life && sim -> patch.after( 3, 1, 0 ) ) ? 0.05 : 0 ) );
    tick_power_mod  += p -> talents.empowered_corruption * 0.02;
    tick_power_mod  += p -> talents.everlasting_affliction * 0.01;

    if( sim -> patch.after( 3, 1, 0 ) && p -> talents.pandemic )
    {
      base_crit_bonus_multiplier = 2;
      tick_may_crit = true;
      base_crit += p -> talents.malediction * 0.03;
    }

    if( p -> gear.tier4_4pc ) num_ticks++;

    observer = &( p -> active_corruption );

  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    base_tick_dmg = base_td_init;
    warlock_spell_t::execute();
    if( result_is_hit() )
    {
      p -> active_dots++;
      if( sim -> patch.before( 3, 1, 0 ) )
        sim -> target -> debuffs.affliction_effects++;
      else
        p -> affliction_effects++;
    }
  }

  virtual void tick()
  {
    warlock_spell_t::tick(); 
    trigger_pandemic( this );
    trigger_eradication( this );
    trigger_nightfall( this );
    trigger_corruption_glyph( this );
    trigger_ashtongue_talisman( this );
    if( player -> gear.tier6_2pc ) player -> resource_gain( RESOURCE_HEALTH, 70 );
  }

  virtual void last_tick()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::last_tick(); 
    p -> active_dots--;
    if( sim -> patch.before( 3, 1, 0 ) )
        sim -> target -> debuffs.affliction_effects--;
      else
        p -> affliction_effects--;
  }
};

// Drain Life Spell ===========================================================

struct drain_life_t : public warlock_spell_t
{
  drain_life_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "drain_life", player, SCHOOL_SHADOW, TREE_AFFLICTION )
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 78, 9, 0, 0, 133, 0.17 },
      { 69, 8, 0, 0, 108, 425  },
      { 62, 7, 0, 0,  87, 355  },
      { 54, 6, 0, 0,  71, 300  },
      { 0, 0 }
    };
    init_rank( ranks );
    
    base_execute_time = 0; 
    base_tick_time    = 1.0; 
    num_ticks         = 5; 
    channeled         = true; 
    binary            = true;
    tick_power_mod    = ( base_tick_time / 3.5 ) / 2.0; 

    base_cost       *= 1.0 - p -> talents.suppression * 0.02;
    base_hit        +=       p -> talents.suppression * 0.01;
    base_multiplier *= 1.0 + p -> talents.shadow_mastery * 0.03;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();
    if( result_is_hit() )
    {
      trigger_everlasting_affliction( this );
      p -> active_dots++;
      if( sim -> patch.before( 3, 1, 0 ) )
        sim -> target -> debuffs.affliction_effects++;
      else
        p -> affliction_effects++;
    }
  }

  virtual void last_tick()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::last_tick(); 
    p -> active_dots--;
    if( sim -> patch.before( 3, 1, 0 ) )
        sim -> target -> debuffs.affliction_effects--;
      else
        p -> affliction_effects--;
  }

  virtual void player_buff()
  {
    warlock_spell_t::player_buff();

    warlock_t* p = player -> cast_warlock();
    target_t*  t = sim -> target;

    double min_multiplier[] = { 0, 0.02, 0.04 };
    double max_multiplier[] = { 0, 0.24, 0.60 };

    if( sim -> patch.after( 3, 1, 0 ) )
    {
      min_multiplier[1] = 0.03;
      min_multiplier[2] = 0.06;
      max_multiplier[1] = 0.09;
      max_multiplier[2] = 0.18;
    }

    assert( p -> talents.soul_siphon >= 0 &&
            p -> talents.soul_siphon <= 2 );

    double min = min_multiplier[ p -> talents.soul_siphon ];
    double max = max_multiplier[ p -> talents.soul_siphon ];

    double multiplier;
    if( sim -> patch.before( 3, 1, 0 ) )
      multiplier = t -> debuffs.affliction_effects * min;
    else
      multiplier = p -> affliction_effects * min;

    if( multiplier > max ) multiplier = max;

    player_multiplier *= 1.0 + multiplier;    
  }

  virtual void tick()
  {
    warlock_spell_t::tick(); 
    trigger_nightfall( this );
  }
};

// Drain Soul Spell ===========================================================

struct drain_soul_t : public warlock_spell_t
{
  int interrupt;
  int health_multiplier;

  drain_soul_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "drain_soul", player, SCHOOL_SHADOW, TREE_AFFLICTION ), interrupt(0), health_multiplier(0)
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { "interrupt", OPT_INT, &interrupt },
      { "target_pct", OPT_DEPRECATED, (void*) "health_percentage<" },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 77, 6, 0, 0, 142, 0.14 },
      { 67, 5, 0, 0, 124, 360  },
      { 52, 4, 0, 0,  91, 290  },
      { 0, 0 }
    };
    rank_t* rank = init_rank( ranks );
    
    base_execute_time = 0; 
    base_tick_time    = 3.0; 
    num_ticks         = 5; 
    channeled         = true; 
    binary            = true;
    tick_power_mod    = ( base_tick_time / 3.5 ) / 2.0; 

    base_cost        *= 1.0 - p -> talents.suppression * 0.02;
    base_hit         += p -> talents.suppression * 0.01;
    base_multiplier  *= 1.0 + p -> talents.shadow_mastery * 0.03;

    health_multiplier = ( rank -> level >= 6 ) ? 1 : 0;
  }

  virtual void execute()
  {
    if ( ticking ) return;

    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();

    if( result_is_hit() )
    {
      p -> active_dots++;
      if( sim -> patch.before( 3, 1, 0 ) )
        sim -> target -> debuffs.affliction_effects++;
      else
        p -> affliction_effects++;
    }
  }

  virtual void last_tick()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::last_tick(); 
    p -> active_dots--;
    if( sim -> patch.before( 3, 1, 0 ) )
        sim -> target -> debuffs.affliction_effects--;
      else
        p -> affliction_effects--;
  }

  virtual void tick()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::tick();
    if( interrupt && ( current_tick != num_ticks ) )
    {
      // If any spell ahead of DS in the action list is "ready", then cancel the DS channel
      for( action_t* action = p -> action_list; action != this; action = action -> next )
      {
        if( action -> ready() )
        {
          current_tick = num_ticks;
          break;
        }
      }
    }
  }

  virtual void player_buff()
  {
    warlock_spell_t::player_buff();

    warlock_t* p = player -> cast_warlock();
    target_t*  t = sim -> target;

    double min_multiplier[] = { 0, 0.02, 0.04 };
    double max_multiplier[] = { 0, 0.24, 0.60 };

    if( sim -> patch.after( 3, 1, 0 ) )
    {
      min_multiplier[1] = 0.03;
      min_multiplier[2] = 0.06;
      max_multiplier[1] = 0.09;
      max_multiplier[2] = 0.18;
    }

    assert( p -> talents.soul_siphon >= 0 &&
            p -> talents.soul_siphon <= 2 );

    double min = min_multiplier[ p -> talents.soul_siphon ];
    double max = max_multiplier[ p -> talents.soul_siphon ];

    double multiplier;
    if( sim -> patch.before( 3, 1, 0 ) )
      multiplier = t -> debuffs.affliction_effects * min;
    else
      multiplier = p -> affliction_effects * min;

    if( multiplier > max ) multiplier = max;

    player_multiplier *= 1.0 + multiplier;

    // FIXME! Hack! Deaths Embrace is additive with Drain Soul "execute".
    // Perhaps it is time to add notion of "execute" into action_t class.

    int de_bonus = trigger_deaths_embrace( this );
    if( de_bonus ) player_multiplier /= 1.0 + de_bonus * 0.01;

    if( health_multiplier )
    {
      if( sim -> target -> health_percentage() < 25 )
      {
        player_multiplier *= 4.0 + de_bonus * 0.01;
      }
    }
  }

  virtual void target_debuff( int dmg_type )
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::target_debuff( dmg_type );
    if( p -> _buffs.haunted && sim -> patch.before( 3, 1, 0 ) )
    {
      // Haunt did not affect Drain Soul prior to 3.1
      target_multiplier /= 1.20;
    }
  }
};

// Siphon Life Spell ===========================================================

struct siphon_life_t : public warlock_spell_t
{
  siphon_life_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "siphon_life", player, SCHOOL_SHADOW, TREE_AFFLICTION )
  {
    warlock_t* p = player -> cast_warlock();

    assert( p -> talents.siphon_life );

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 8, 0, 0, 81, 0.16 },
      { 75, 7, 0, 0, 70, 0.16 },
      { 70, 6, 0, 0, 63, 410  },
      { 63, 5, 0, 0, 52, 350  },
      { 58, 4, 0, 0, 45, 310  },
      { 0, 0 }
    };
    init_rank( ranks );
     
    base_execute_time = 0; 
    base_tick_time    = 3.0; 
    num_ticks         = 10; 
    binary            = true;
    tick_power_mod    = ( base_tick_time / 15.0 ) / 2.0; 

    base_cost       *= 1.0 - p -> talents.suppression * 0.02;
    base_hit        +=       p -> talents.suppression * 0.01;
    base_multiplier *= 1.0 + ( p -> talents.shadow_mastery * 0.03 +
                               p -> glyphs.siphon_life     * 0.20 );                    
    tick_power_mod  += p -> talents.everlasting_affliction * 0.01;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();
    if( result_is_hit() )
    {
      p -> active_dots++;
      if( sim -> patch.before( 3, 1, 0 ) )
        sim -> target -> debuffs.affliction_effects++;
      else
        p -> affliction_effects++;
    }
  }

  virtual void last_tick()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::last_tick(); 
    p -> active_dots--;
    if( sim -> patch.before( 3, 1, 0 ) )
      sim -> target -> debuffs.affliction_effects--;
    else
      p -> affliction_effects--;
  }

  virtual void tick()
  {
    warlock_spell_t::tick(); 
    player -> resource_gain( RESOURCE_HEALTH, tick_dmg );
  }

  virtual bool ready()
  {
    if( sim -> patch.before( 3, 1, 0 ) )
      return warlock_spell_t::ready();
    else
      return false;
  }
};

// Unstable Affliction Spell ======================================================

struct unstable_affliction_t : public warlock_spell_t
{
  unstable_affliction_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "unstable_affliction", player, SCHOOL_SHADOW, TREE_AFFLICTION )
  {
    warlock_t* p = player -> cast_warlock();

    assert( p -> talents.unstable_affliction );
    
    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 5,  0, 0, 230, 0.15 },
      { 75, 4,  0, 0, 197, 0.15 },
      { 70, 3,  0, 0, 175, 400  },
      { 60, 2,  0, 0, 155, 315  },
      { 0, 0 }
    };
    init_rank( ranks );
    
    base_execute_time = 1.5; 
    base_tick_time    = 3.0;
    num_ticks         = 5;
    tick_power_mod    = base_tick_time / 15.0; 
    
    base_cost        *= 1.0 - p -> talents.suppression * 0.02;
    base_hit         +=       p -> talents.suppression * 0.01;
    base_multiplier  *= 1.0 + p -> talents.shadow_mastery * 0.03
                            + p -> gear.tier8_2pc         * 0.20 + //FIXME assuming additive
                          ( ( p -> talents.siphon_life && sim -> patch.after( 3, 1, 0 ) ) ? 0.05 : 0 );

    tick_power_mod   += p -> talents.everlasting_affliction * 0.01;

    if( sim -> patch.after( 3, 1, 0 ) ) duration_group = "immolate";

    if( sim -> patch.after( 3, 1, 0 ) && p -> talents.pandemic )
    {
      base_crit_bonus_multiplier = 2;
      tick_may_crit = true;
      base_crit += p -> talents.malediction * 0.03;
    }

    if( p -> glyphs.unstable_affliction )
    {
      base_execute_time = 1.3;
      //trigger_gcd     = 1.3; latest research seems to imply it does not affect the gcd
    }
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();
    if( result_is_hit() )
    {
      p -> active_dots++;;
      if( sim -> patch.before( 3, 1, 0 ) )
        sim -> target -> debuffs.affliction_effects++;
      else
        p -> affliction_effects++;
    }
  }

  virtual void tick()
  {
    warlock_spell_t::tick(); 
    trigger_pandemic( this );
  }

  virtual void last_tick()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::last_tick(); 
    p -> active_dots--;;
    if( sim -> patch.before( 3, 1, 0 ) )
      sim -> target -> debuffs.affliction_effects--;
    else
      p -> affliction_effects--;
  }
};

// Haunt Spell ==============================================================

struct haunt_t : public warlock_spell_t
{
  int debuff;

  haunt_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "haunt", player, SCHOOL_SHADOW, TREE_AFFLICTION ), debuff(0)
  {
    warlock_t* p = player -> cast_warlock();
    assert( p -> talents.haunt );

    option_t options[] =
    {
      { "debuff", OPT_INT, &debuff     },
      { "only_for_debuff", OPT_DEPRECATED, (void*) "debuff" },
      { NULL }
    };

    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 4, 645, 753, 0, 0.12 },
      { 75, 3, 550, 642, 0, 0.12 },
      { 70, 2, 487, 569, 0, 0.12 },
      { 60, 1, 405, 473, 0, 0.12 },
      { 0, 0 }
    };
    init_rank( ranks );
    
    base_execute_time = 1.5; 
    direct_power_mod  = base_execute_time / 3.5; 
    cooldown          = 8.0;
    may_crit          = true;
      
    base_cost        *= 1.0 - p -> talents.suppression * 0.02;
    base_hit         +=       p -> talents.suppression * 0.01;
    base_multiplier  *= 1.0 + p -> talents.shadow_mastery * 0.03;
  }

  virtual void execute()
  {
    warlock_spell_t::execute(); 
    if( result_is_hit() )
    {
      trigger_haunted( this );
      trigger_everlasting_affliction( this );
      stack_shadow_embrace( this );
      decrement_shadow_vulnerability( player -> cast_warlock() );
    }
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::player_buff();
    if( p -> _buffs.shadow_vulnerability ) player_multiplier *= 1.0 + p -> talents.improved_shadow_bolt * 0.02;
    p -> uptimes_shadow_vulnerability -> update( p -> _buffs.shadow_vulnerability != 0 );
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if( ! warlock_spell_t::ready() )
      return false;

    if( debuff && p -> _expirations.haunted )
      if( ( sim -> current_time + execute_time() ) < p -> _expirations.haunted -> occurs() )
        return false;

    return true;
  }
};

// Immolate Spell =============================================================

struct immolate_t : public warlock_spell_t
{
  immolate_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "immolate", player, SCHOOL_FIRE, TREE_DESTRUCTION )
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { "target_pct", OPT_DEPRECATED, (void*) "health_percentage>" },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 11, 460, 460, 157, 0.17 },
      { 75, 10, 370, 370, 139, 0.17 },
      { 69,  9, 327, 327, 123, 445  },
      { 60,  8, 279, 279, 102, 380  },
      { 60,  7, 258, 258,  97, 370  },
      { 0, 0 }
    };
    init_rank( ranks );

    base_execute_time = 2.0; 
    may_crit          = true;
    base_tick_time    = 3.0; 
    num_ticks         = 5;
    direct_power_mod  = 0.20; 
    tick_power_mod    = 0.20;

    if( sim -> patch.before( 3, 1, 0 ) )
    {
      base_hit        += p -> talents.cataclysm * 0.01;
      base_cost       *= 1.0 - p -> talents.cataclysm * 0.01;
    }
    else
    {
      base_hit        += p -> talents.suppression * 0.01;
      base_cost       *= 1.0 - ( p -> talents.cataclysm * 0.03
                             + ( ( p -> talents.cataclysm ) ? 0.01 : 0 ) );
    }
    base_execute_time -= p -> talents.bane * 0.1;
    base_crit         += p -> talents.devastation * 0.05;
    base_crit         += p -> talents.backlash * 0.01;
    direct_power_mod  += p -> talents.fire_and_brimstone * 0.01;
    tick_power_mod    += p -> talents.fire_and_brimstone * 0.02 / num_ticks;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.ruin * 0.20;

    if( sim -> patch.before( 3, 1, 0 ) )
    {
      base_dd_multiplier *= 1.0 + ( p -> talents.emberstorm        * 0.03 +
                                    p -> talents.improved_immolate * 0.10 -
                                    p -> glyphs.immolate           * 0.10 );

      base_td_multiplier *= 1.0 + ( p -> talents.emberstorm        * 0.03 +
                                    p -> talents.improved_immolate * 0.10 +
                                    p -> glyphs.immolate           * 0.20 );
    }
    else
    {
      base_dd_multiplier *= 1.0 + ( p -> talents.emberstorm        * 0.03 +
                                    p -> talents.improved_immolate * 0.10 +
                                    p -> gear.tier8_2pc            * 0.10 ); //FIXME assuming additive

      base_td_multiplier *= 1.0 + ( p -> talents.emberstorm        * 0.03 +
                                    p -> talents.improved_immolate * 0.10 +
                                    p -> glyphs.immolate           * 0.10 +
                                    p -> talents.aftermath         * 0.03 +
                                    p -> gear.tier8_2pc            * 0.10 ); //FIXME assuming additive
    }


    if( p -> gear.tier4_4pc ) num_ticks++;

    observer = &( p -> active_immolate );
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    base_tick_dmg = base_td_init;
    warlock_spell_t::execute();
    if( result_is_hit() )
    {
      p -> active_dots++;
    }
  }

  virtual void tick()
  {
    warlock_spell_t::tick(); 
    if( player -> gear.tier6_2pc ) player -> resource_gain( RESOURCE_HEALTH, 35 );
  }

  virtual void last_tick()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::last_tick(); 
    p -> active_dots--;
  }
};

// Shadowflame Spell =============================================================

struct shadowflame_t : public warlock_spell_t
{
  shadowflame_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "shadowflame", player, SCHOOL_SHADOW, TREE_DESTRUCTION )
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 2, 615, 671, 128, 0.25 },
      { 75, 1, 520, 568, 108, 0.25 },
      { 0, 0 }
    };
    init_rank( ranks );

    base_execute_time = 0; 
    may_crit          = true;
    base_tick_time    = 2.0; 
    num_ticks         = 4;
    direct_power_mod  = 0.1429; 
    tick_power_mod    = 0.28;
    cooldown          = 15.0;

    if( sim -> patch.before( 3, 1, 0 ) )
    {
      base_hit        += p -> talents.cataclysm * 0.01;
      base_cost       *= 1.0 - p -> talents.cataclysm * 0.01;
    }
    else
    {
      base_hit        += p -> talents.suppression * 0.01;
      base_cost       *= 1.0 - ( p -> talents.cataclysm * 0.03
                             + ( ( p -> talents.cataclysm ) ? 0.01 : 0 ) );
    }
    base_crit += p -> talents.devastation * 0.05;
    base_crit += p -> talents.backlash * 0.01;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.ruin * 0.20;

    base_dd_multiplier *= 1.0 + ( p -> talents.shadow_mastery * 0.03 );

    base_td_multiplier *= 1.0 + ( p -> talents.emberstorm * 0.03 );

    observer = &( p -> active_shadowflame );
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    base_tick_dmg = base_td_init;
    // DD is shadow damage, DoT is fire damage
    school = SCHOOL_SHADOW;
    warlock_spell_t::execute();
    if( result_is_hit() )
    {
      // DD was shadow, now DoT is fire, so reset school
      school = SCHOOL_FIRE;
      p -> active_dots++;
    }
  }

  virtual void tick()
  {
    warlock_spell_t::tick(); 
  }

  virtual void last_tick()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::last_tick(); 
    p -> active_dots--;
  }
};

// Conflagrate Spell =========================================================

struct conflagrate_t : public warlock_spell_t
{
  int  no_backdraft;
  int  ticks_lost;
  bool cancel_dot;

  action_t** dot_spell;

  conflagrate_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "conflagrate", player, SCHOOL_FIRE, TREE_DESTRUCTION ), no_backdraft(0), ticks_lost(0), cancel_dot(true)
  {
    warlock_t* p = player -> cast_warlock();

    assert( p -> talents.conflagrate );

    option_t options[] =
    {
      { "no_backdraft", OPT_INT, &no_backdraft },
      { "ticks_lost",   OPT_INT, &ticks_lost   },
      { NULL }
    };
    parse_options( options, options_str );
 
    if( sim -> patch.before( 3, 1, 0 ) )
    {
      static rank_t ranks[] =
      {
        { 80, 8, 766, 954, 0, 0.12 },
        { 75, 7, 650, 810, 0, 0.12 },
        { 70, 6, 579, 721, 0, 305  },
        { 65, 5, 512, 638, 0, 280  },
        { 60, 4, 447, 557, 0, 255  },
        { 0, 0 }
      };
      init_rank( ranks );
    }
    else
    {
      static rank_t ranks[] =
      {
        { 40, 1, 0, 0, 0, 0.16 },
        { 0, 0 }
      };
      init_rank( ranks );
    }

    base_execute_time = 0; 
    may_crit          = true;
    direct_power_mod  = (1.5/3.5);
    cooldown          = 10;

    if( sim -> patch.before( 3, 1, 0 ) )
    {
      base_hit        += p -> talents.cataclysm * 0.01;
      base_cost       *= 1.0 - p -> talents.cataclysm * 0.01;
    }
    else
    {
      base_hit        += p -> talents.suppression * 0.01;
      base_cost       *= 1.0 - ( p -> talents.cataclysm * 0.03
                             + ( ( p -> talents.cataclysm ) ? 0.01 : 0 ) );
      base_multiplier *= 1.0 + p -> talents.aftermath         * 0.03
                             + p -> talents.improved_immolate * 0.10
                             + p -> glyphs.immolate           * 0.10
                             + p -> gear.tier8_2pc            * 0.10; //FIXME assuming additive
          
    }
    base_multiplier  *= 1.0 + p -> talents.emberstorm * 0.03;
    base_crit        += p -> talents.devastation * 0.05;
    base_crit        += p -> talents.backlash * 0.01;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.ruin * 0.20;

    cancel_dot = ( sim -> patch.before( 3, 0, 8 ) || ! p -> glyphs.conflagrate );

    if( p -> glyphs.conflagrate && sim -> patch.before( 3, 0, 8 ) ) base_cost *= 0.80;
  }
    
  virtual double calculate_direct_damage()
  {
    warlock_t* p = player -> cast_warlock();
    if( p -> active_immolate && ! p -> active_shadowflame )
    {
      dot_spell = &( p -> active_immolate );
    }
    else if( ! p -> active_immolate && p -> active_shadowflame )
    {
      dot_spell = &( p -> active_shadowflame );
    }
    else if( sim -> roll( 0.50 ) )
    {
      dot_spell = &( p -> active_immolate );
    }
    else
    {
      dot_spell = &( p -> active_shadowflame );
    }
    if( sim -> patch.after( 3, 1, 0 ) )
    {
      base_dd_min = base_dd_max = ( (*dot_spell) -> base_td_init   )
                                * ( (*dot_spell) -> num_ticks      );
      direct_power_mod          = ( (*dot_spell) -> tick_power_mod )
                                * ( (*dot_spell) -> num_ticks      );
      player_power = (*dot_spell) -> player_power;
    }
    return warlock_spell_t::calculate_direct_damage();
  }

  virtual void execute()
  {
    warlock_spell_t::execute(); 
    if( result_is_hit() )
    {
      if ( result == RESULT_CRIT ) trigger_pyroclasm( this );
      trigger_soul_leech( this );
      trigger_backdraft( this );

      if( cancel_dot )
      {
        (*dot_spell) -> cancel();
        *dot_spell = 0;
      }
    }
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::player_buff(); 
    if( p -> talents.fire_and_brimstone &&
        p -> active_immolate )
    {
      int ticks_remaining = ( p -> active_immolate -> num_ticks - 
                              p -> active_immolate -> current_tick );
      
      if( ticks_remaining <= 2 )
      {
        player_crit += p -> talents.fire_and_brimstone * 0.05;
      }
    }
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if( ! spell_t::ready() ) 
      return false;

    if( ! p -> active_immolate && ! p -> active_shadowflame )
      return false;

    if( ticks_lost > 0 )
    {
      // The "ticks_lost" checking is a human decision and should not have any RNG.
      // The priority will always be to preserve the Immolate spell if both are up.

      int ticks_remaining = 0;

      if( p -> active_immolate )
      {
        ticks_remaining = ( p -> active_immolate -> num_ticks - 
                            p -> active_immolate -> current_tick );
      } 
      else 
      {
        ticks_remaining = ( p -> active_shadowflame -> num_ticks - 
                            p -> active_shadowflame -> current_tick );
      }
      return( ticks_remaining <= ticks_lost );
    }

    return true;
  }
};

// Incinerate Spell =========================================================

struct incinerate_t : public warlock_spell_t
{
  int    backdraft;
  int    molten_core;
  double immolate_bonus;

  incinerate_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "incinerate", player, SCHOOL_FIRE, TREE_DESTRUCTION ), backdraft(0), molten_core(0), immolate_bonus(0)
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { "backdraft",   OPT_INT, &backdraft   },
      { "molten_core", OPT_INT, &molten_core },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 4, 582, 676, 0, 0.14 },
      { 74, 3, 485, 563, 0, 0.14 },
      { 70, 2, 429, 497, 0, 300  },
      { 64, 1, 357, 413, 0, 256  },
      { 0, 0 }
    };
    init_rank( ranks );
     
    base_execute_time  = 2.5; 
    may_crit           = true;
    direct_power_mod   = (2.5/3.5); 

    if( sim -> patch.before( 3, 1, 0 ) )
    {
      base_hit        += p -> talents.cataclysm * 0.01;
      base_cost       *= 1.0 - p -> talents.cataclysm * 0.01;
    }
    else
    {
      base_hit        += p -> talents.suppression * 0.01;
      base_cost       *= 1.0 - ( p -> talents.cataclysm * 0.03
                             + ( ( p -> talents.cataclysm ) ? 0.01 : 0 ) );
    }
    base_execute_time -= p -> talents.emberstorm * 0.05;
    base_multiplier   *= 1.0 + ( p -> talents.emberstorm  * 0.03 +
                                 p -> gear.tier6_4pc      * 0.06 +
                                 p -> glyphs.incinerate   * 0.05 );
    base_crit         += p -> talents.devastation * 0.05;
    base_crit         += p -> talents.backlash * 0.01;
    if( p -> gear.tier8_4pc )
      base_crit       += 0.05;
    direct_power_mod  *= 1.0 + p -> talents.shadow_and_flame * 0.04;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.ruin * 0.20;

    immolate_bonus = util_t::ability_rank( p -> level,  157,80,  130,74,  120,70,  108,0 );

    decimation_trigger = true;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    base_direct_dmg = ( base_dd_min + base_dd_max ) / 2.0;
    if( p -> active_immolate ) base_direct_dmg += immolate_bonus;
    warlock_spell_t::execute();
    if( result_is_hit() )
    {
      trigger_soul_leech( this );
      trigger_tier5_4pc( this, p -> active_immolate );
    }
    if( p -> buffs.tier7_2pc )
    {
      p -> aura_loss( "Demonic Soul" );
      p -> buffs.tier7_2pc = 0;
    }
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::player_buff();
    if( p -> buffs.tier7_2pc ) player_crit += 0.10;
    p -> uptimes_demonic_soul -> update( p -> buffs.tier7_2pc != 0 );
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if( ! warlock_spell_t::ready() )
      return false;

    if( backdraft )
      if( ! p -> _buffs.backdraft )
        return false;

    if( molten_core )
    {
      if( ! sim -> time_to_think( p -> _buffs.molten_core ) )
        return false;

      if( sim -> current_time + execute_time() > p -> _expirations.molten_core -> occurs() )
        return false;
    }
          
    return true;
  }
};

// Searing Pain Spell =========================================================

struct searing_pain_t : public warlock_spell_t
{
  searing_pain_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "searing_pain", player, SCHOOL_FIRE, TREE_DESTRUCTION )
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 79, 10, 343, 405, 0, 0.08 },
      { 74,  9, 295, 349, 0, 0.08 },
      { 70,  8, 270, 320, 0, 205  },
      { 65,  7, 243, 287, 0, 191  },
      { 58,  6, 204, 240, 0, 168  },
      { 0, 0 }
    };
    init_rank( ranks );
     
    base_execute_time = 1.5; 
    may_crit          = true;
    direct_power_mod  = base_execute_time / 3.5; 

    if( sim -> patch.before( 3, 1, 0 ) )
    {
      base_hit        += p -> talents.cataclysm * 0.01;
      base_cost       *= 1.0 - p -> talents.cataclysm * 0.01;
    }
    else
    {
      base_hit        += p -> talents.suppression * 0.01;
      base_cost       *= 1.0 - ( p -> talents.cataclysm * 0.03
                             + ( ( p -> talents.cataclysm ) ? 0.01 : 0 ) );
    }
    base_multiplier *= 1.0 + p -> talents.emberstorm  * 0.03;
    base_crit       += p -> talents.devastation * 0.05;
    base_crit       += p -> talents.backlash * 0.01;
    base_crit       += p -> talents.improved_searing_pain * 0.03
                    + ( ( p -> talents.improved_searing_pain ) ? 0.01 : 0 );

    base_crit_bonus_multiplier *= 1.0 + ( p -> talents.ruin * 0.20 +
                                          p -> glyphs.searing_pain * 0.20 );
  }

  virtual void execute()
  {
    warlock_spell_t::execute(); 
    if( result_is_hit() )
    {
      if ( result == RESULT_CRIT ) trigger_pyroclasm( this );
      trigger_soul_leech( this );
    }
  }
};

// Soul Fire Spell ============================================================

struct soul_fire_t : public warlock_spell_t
{
  int backdraft;
  int decimation;
  int noweave;

  soul_fire_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "soul_fire", player, SCHOOL_FIRE, TREE_DESTRUCTION ), backdraft(0), decimation(0), noweave(0)
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { "backdraft",  OPT_INT, &backdraft  },
      { "decimation", OPT_INT, &decimation },
      { "noweave"   , OPT_INT, &noweave    },
      { NULL }
    };
    parse_options( options, options_str );
      
    static rank_t ranks[] =
    {
      { 80, 6, 1323, 1657, 0, 0.09 },
      { 75, 5, 1137, 1423, 0, 0.09 },
      { 70, 4, 1003, 1257, 0, 455  },
      { 64, 3,  839, 1051, 0, 390  },
      { 56, 2,  703,  881, 0, 335  },
      { 0, 0 }
    };
    init_rank( ranks );
     
    base_execute_time = 6.0; 
    may_crit          = true; 
    direct_power_mod  = 1.15; 

    if( sim -> patch.before( 3, 1, 0 ) )
    {
      base_hit        += p -> talents.cataclysm * 0.01;
      base_cost       *= 1.0 - p -> talents.cataclysm * 0.01;
    }
    else
    {
      base_hit        += p -> talents.suppression * 0.01;
      base_cost       *= 1.0 - ( p -> talents.cataclysm * 0.03
                             + ( ( p -> talents.cataclysm ) ? 0.01 : 0 ) );
    }
    base_execute_time -= p -> talents.bane * 0.4;
    base_multiplier   *= 1.0 + p -> talents.emberstorm  * 0.03;
    base_crit         += p -> talents.devastation * 0.05;
    base_crit         += p -> talents.backlash * 0.01;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.ruin * 0.20;

    decimation_consumer = true;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();
    if( noweave ) p -> _buffs.decimation = 0;
    if( result_is_hit() )
    {
      trigger_soul_leech( this );
    }
  }

  virtual double execute_time()
  {
    warlock_t* p = player -> cast_warlock();
    double t = warlock_spell_t::execute_time();
    if( p -> _buffs.decimation >= 2 )
    {
      if( p -> talents.decimation == 1 )
        t *= 0.65;
      else
        t *= 0.40;
    }
    return t;
  }
    
  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if( ! warlock_spell_t::ready() )
      return false;

    if( backdraft )
      if( ! p -> _buffs.backdraft )
        return false;

    if( decimation )
      if( ! ( p -> _buffs.decimation >= 2 ) )
        return false;
          
    return true;
  }
};

// Life Tap Spell ===========================================================

struct life_tap_t : public warlock_spell_t
{
  int    trigger;
  int    inferno;
  int    glyph;
  int    tier7_4pc;
  double base_tap;

  life_tap_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "life_tap", player, SCHOOL_SHADOW, TREE_AFFLICTION ), trigger(1000), inferno(0), glyph(0), tier7_4pc(0), base_tap(0)
  {
    option_t options[] =
    {
      { "trigger",   OPT_INT, &trigger   },
      { "inferno",   OPT_INT, &inferno   },
      { "glyph",     OPT_INT, &glyph     },
      { "tier7_4pc", OPT_INT, &tier7_4pc },
      { NULL }
    };
    parse_options( options, options_str );

    harmful = false;

    base_tap = util_t::ability_rank( player -> level,  1490,80,  710,68,  500,0 );
  }

  virtual void execute() 
  {
    warlock_t* p = player -> cast_warlock();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    p -> procs_life_tap -> occur();
    double mana = base_tap + 3.0 * p -> spirit();
    p -> resource_loss( RESOURCE_HEALTH, mana );
    p -> resource_gain( RESOURCE_MANA, mana * ( 1.0 + p -> talents.improved_life_tap * 0.10 ), p -> gains_life_tap );
    trigger_life_tap_glyph( this );
    trigger_tier7_4pc( this );
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if( inferno )
    {
      if( p -> active_pet == 0 ||
          p -> active_pet -> pet_type != PET_INFERNAL )
      {
        return( p -> resource_current[ RESOURCE_MANA ] < 
                p -> resource_base   [ RESOURCE_MANA ] * 0.80 );
      }
    }

    if( tier7_4pc )
      return( ! p -> buffs.tier7_4pc );

    if( glyph )
      return( ! p -> _buffs.life_tap_glyph );

    return( p -> resource_current[ RESOURCE_MANA ] < trigger );
  }
};

// Dark Pact Spell =========================================================

struct dark_pact_t : public warlock_spell_t
{
  dark_pact_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "dark_pact", player, SCHOOL_SHADOW, TREE_AFFLICTION )
  {
    warlock_t* p = player -> cast_warlock();
    assert( p -> talents.dark_pact );

    static rank_t ranks[] =
    {
      { 80, 5, 1200, 1200, 0, 0 },
      { 70, 3,  700,  700, 0, 0 },
      { 60, 2,  545,  545, 0, 0 },
        { 0, 0 }
    };
    init_rank( ranks );

    harmful = false;

    base_execute_time = 0.0; 
    direct_power_mod  = 0.96;
  }

  virtual void execute() 
  {
    warlock_t* p = player -> cast_warlock();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    p -> procs_dark_pact -> occur();
    player_buff();
    double mana = ( base_dd_max + ( direct_power_mod * p -> composite_spell_power( SCHOOL_SHADOW ) ) ) * base_multiplier * player_multiplier;
    p -> resource_gain( RESOURCE_MANA, mana, p -> gains_dark_pact );
  }

  virtual bool ready()
  {
    return( player -> resource_current[ RESOURCE_MANA ] < 1000 );
  }
};

// Fel Armor Spell ==========================================================

struct fel_armor_t : public warlock_spell_t
{
  double bonus_spell_power;

  fel_armor_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "fel_armor", player, SCHOOL_SHADOW, TREE_DEMONOLOGY ), bonus_spell_power(0)
  {
    warlock_t* p = player -> cast_warlock();
    harmful = false;
    trigger_gcd = 0;
    bonus_spell_power = util_t::ability_rank( p -> level,  180.0,78,  150.0,73,  100.0,67,  50.0,62,  0.0,0 );
    bonus_spell_power *= 1.0 + p -> talents.demonic_aegis * 0.10;

    // Model the passive health tick.....
    base_tick_time = 5.0; 
    num_ticks      = 1;
    heal           = true;
  }

  virtual void execute() 
  {
    warlock_t* p = player -> cast_warlock();

    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );

    p -> _buffs.fel_armor = bonus_spell_power;
    p -> _buffs.demon_armor = 0;
    p -> spell_power_per_spirit += 0.30 * ( 1.0 + p -> talents.demonic_aegis * 0.10 );

    schedule_tick();
  }

  virtual void tick()
  {
    warlock_t* p = player -> cast_warlock();
    current_tick = 0; // ticks indefinitely
    p -> resource_gain( RESOURCE_HEALTH, 0, p -> gains_fel_armor );
    p -> action_heal( this, 0 );
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();
    return p -> _buffs.fel_armor == 0;
  }
};

// Sacrifice Pet Spell =======================================================

struct sacrifice_pet_t : public warlock_spell_t
{
  sacrifice_pet_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "sacrifice_pet", player, SCHOOL_SHADOW, TREE_DEMONOLOGY )
  {
    warlock_t* p = player -> cast_warlock();
    assert( p -> talents.demonic_sacrifice );
    harmful = false;
    trigger_gcd = 0;
  }

  virtual void execute() 
  {
    warlock_t* p = player -> cast_warlock();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    p -> _buffs.pet_sacrifice = p -> active_pet -> pet_type;
    p -> active_pet -> dismiss();
    p -> active_pet = 0;
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();
    return p -> active_pet != 0;
  }
};

// Summon Pet Spell ==========================================================

struct summon_pet_t : public warlock_spell_t
{
  std::string pet_name;

  summon_pet_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "summon_pet", player, SCHOOL_SHADOW, TREE_DEMONOLOGY )
  {
    pet_name = options_str;
    harmful = false;
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    player -> summon_pet( pet_name.c_str() );
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();
    if( p -> active_pet ) return false;
    if( p -> _buffs.pet_sacrifice ) return false;
    if( sim -> current_time > 10.0 ) return false;
    return true;
  }
};

// Inferno Spell ==========================================================

struct inferno_t : public warlock_spell_t
{
  inferno_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "inferno", player, SCHOOL_FIRE, TREE_DEMONOLOGY )
  {
    option_t options[] =
    {
      { "target_pct",     OPT_DEPRECATED, (void*) "health_percentage<" },
      { "time_remaining", OPT_DEPRECATED, (void*) "time_to_die<"       },
      { NULL }
    };
    parse_options( options, options_str );

    if( max_health_percentage == 0 && 
	max_time_to_die       == 0 )
    {
      max_time_to_die = 60;
    }

    static rank_t ranks[] =
    {
      { 50, 1, 200, 200, 0, 0.80 },
      { 0, 0 }
    };
    init_rank( ranks );
    
    cooldown          = 20 * 60;
    base_execute_time = 1.5; 
    may_crit          = true;
    direct_power_mod  = 1;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();

    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, warlock_t* p ) : event_t( sim, p )
      {
        name = "Infernal Expiration";
        if( p -> active_pet )
        {
          p -> active_pet -> dismiss();
          p -> active_pet = 0;
        }
        p -> summon_pet( "infernal" );
        sim -> add_event( this, 60.0 );
      }
      virtual void execute()
      {
        warlock_t* p = player -> cast_warlock();
        p -> dismiss_pet( "infernal" );
        p -> _expirations.infernal = 0;
        p -> active_pet = 0;
      }
    };
  
    warlock_t* p = player -> cast_warlock();
    event_t*&  e = p -> _expirations.infernal;

    if( !e )
    {
      e = new ( sim ) expiration_t( sim, p );
    }
  }
};

// Immolation Spell =======================================================

struct immolation_t : public warlock_spell_t
{
  immolation_t( player_t* player, const std::string& options_str ) :  
    warlock_spell_t( "immolation", player, SCHOOL_FIRE, TREE_DEMONOLOGY )
  {
    option_t options[] =
    {
      { NULL }
    };
    parse_options( options, options_str );

    warlock_t* p   = player -> cast_warlock();
    base_cost      = 0.64 * p -> resource_base[ RESOURCE_MANA ];
    base_hit      += p -> talents.suppression * 0.01;
    base_td_init   = 481;
    base_tick_time = 1.0; 
    num_ticks      = 15;
    tick_power_mod = 0.143;
    cooldown       = 30;
  }
  
  virtual double tick_time()
  {
    return base_tick_time * haste();
  }

  virtual void tick()
  {
    warlock_t* p = player -> cast_warlock();
    if( p -> _buffs.metamorphosis )
      warlock_spell_t::tick();
    else
      current_tick = num_ticks;
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if( ! warlock_spell_t::ready() )
      return false;

    if( ! p -> _buffs.metamorphosis )
      return false;

    return true;
  }
};

// Metamorphosis Spell =======================================================

struct metamorphosis_t : public warlock_spell_t
{
  metamorphosis_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "metamorphosis", player, SCHOOL_SHADOW, TREE_DEMONOLOGY )
  {
    warlock_t* p = player -> cast_warlock();
    assert( p -> talents.metamorphosis );

    option_t options[] =
    {
      { "target_pct", OPT_DEPRECATED, (void*) "health_percentage<" },
      { NULL }
    };
    parse_options( options, options_str );
      
    harmful = false;
    base_cost   = 0;
    trigger_gcd = 0;
    cooldown    = 180 * ( 1.0 - p -> talents.nemesis * 0.1 );
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, player_t* player ) : event_t( sim, player )
      {
        name = "Metamorphosis Expiration";
        warlock_t* p = player -> cast_warlock();
        p -> aura_gain( "Metamorphosis" );
        p -> _buffs.metamorphosis = 1;
        sim -> add_event( this, 30.0 + p -> glyphs.metamorphosis * 6.0 );
      }
      virtual void execute()
      {
        warlock_t* p = player -> cast_warlock();
        p -> aura_loss( "Metamorphosis" );
        p -> _buffs.metamorphosis = 0;
      }
    };

    warlock_t* p = player -> cast_warlock();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    p -> action_finish( this );
    new ( sim ) expiration_t( sim, p );
  }
};

// Demonic Empowerment Spell ================================================

struct demonic_empowerment_t : public warlock_spell_t
{
  int demonic_frenzy;
  demonic_empowerment_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "demonic_empowerment", player, SCHOOL_SHADOW, TREE_DEMONOLOGY )
  {
    warlock_t* p = player -> cast_warlock();
    assert( p -> talents.demonic_empowerment );

    option_t options[] =
    {
      { "target_pct",     OPT_DEPRECATED, (void*) "health_percentage<" },
      { "demonic_frenzy", OPT_INT,        &demonic_frenzy              },
      { NULL }
    };
    parse_options( options, options_str );
      
    harmful = false;
    base_cost = player -> resource_base[ RESOURCE_MANA ] * 0.06;
    cooldown  = 60 * ( 1.0 - p -> talents.nemesis * 0.1 );
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    struct expiration_t : public event_t
    {
      expiration_t( sim_t* sim, warlock_pet_t* pet ) : event_t( sim, pet )
      {
        name = "Demonic Empowerment Expiration";
        pet -> aura_gain( "Demonic Empowerment" );
        pet -> _buffs.demonic_empowerment = 1;
        double duration = ( pet -> pet_type == PET_FELGUARD ? 15.0 :
                            pet -> pet_type == PET_IMP      ? 30.0 : 0.01 );
        sim -> add_event( this, duration );
      }
      virtual void execute()
      {
        warlock_pet_t* pet = (warlock_pet_t*) player -> cast_pet();
        pet -> aura_loss( "Demonic Empowerment" );
        pet -> _buffs.demonic_empowerment = 0;
      }
    };

    warlock_t* p = player -> cast_warlock();
    if( sim -> log ) report_t::log( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    p -> action_finish( this );
    new ( sim ) expiration_t( sim, p -> active_pet );
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if( ! p -> active_pet )
      return false;

    if( demonic_frenzy )
    {
      if( p -> active_pet -> pet_type == PET_FELGUARD )
      {
        felguard_pet_t* f = (felguard_pet_t*) p -> active_pet;
        if( f -> _buffs.demonic_frenzy < demonic_frenzy ) return false;
      }
      else
      { 
        return false;
      }
    }

    return warlock_spell_t::ready();
  }
};

// Fire Stone Spell ===========================================================

struct fire_stone_t : public warlock_spell_t
{
  int bonus_crit;

  fire_stone_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "fire_stone", player, SCHOOL_FIRE, TREE_DEMONOLOGY )
  {
    warlock_t* p = player -> cast_warlock();
    trigger_gcd = 0;

    harmful = false;

    bonus_crit = (int) util_t::ability_rank( p -> level,  49,80,  42,74,  35,66,  28,0 );

    if( sim -> patch.before( 3, 1, 0 ) )
      bonus_crit = (int) ( bonus_crit * ( 1.0 + p -> talents.master_conjuror * 0.15 ) );
    else
      bonus_crit = (int) ( bonus_crit * ( 1.0 + p -> talents.master_conjuror * 1.50 ) );
  }

  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );

    player -> main_hand_weapon.buff = FIRE_STONE;
    player -> spell_crit += bonus_crit / player -> rating.spell_crit;
  };

  virtual bool ready()
  {
    return( player -> main_hand_weapon.buff != FIRE_STONE );
  }
};

// Spell Stone Spell ===========================================================

struct spell_stone_t : public warlock_spell_t
{
  int bonus_haste;

  spell_stone_t( player_t* player, const std::string& options_str ) : 
    warlock_spell_t( "spell_stone", player, SCHOOL_FIRE, TREE_DEMONOLOGY )
  {
    warlock_t* p = player -> cast_warlock();
    trigger_gcd = 0;

    harmful = false;

    bonus_haste = util_t::ability_rank( p -> level,  60,80,  50,72,  40,66,  30,0 );
    
    bonus_haste = (int) ( bonus_haste * ( 1.0 + p -> talents.master_conjuror * 1.50 ) );
  }

  virtual void execute()
  {
    if( sim -> log ) report_t::log( sim, "%s performs %s", player -> name(), name() );

    player -> main_hand_weapon.buff = SPELL_STONE;
    player -> haste_rating += bonus_haste;
    player -> recalculate_haste();
  };

  virtual bool ready()
  {
    return( player -> main_hand_weapon.buff != SPELL_STONE );
  }
};

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Warlock Pet Events
// ==========================================================================

// warlock_pet_t::attack_hit_event ==========================================

void warlock_pet_t::attack_hit_event( attack_t* a )
{
  player_t::attack_hit_event( a );

  if( a -> result == RESULT_CRIT )
  {
    trigger_demonic_pact( a );
    trigger_demonic_empathy_on_owner( a );
  }
}

// warlock_pet_t::spell_hit_event ===========================================

void warlock_pet_t::spell_hit_event( spell_t* s )
{
  player_t::spell_hit_event( s );

  if( s -> result == RESULT_CRIT )
  {
    trigger_demonic_pact( s );
    trigger_demonic_empathy_on_owner( s );
  }
}

// imp_pet_t::spell_hit_event ===============================================

void imp_pet_t::spell_hit_event( spell_t* s )
{
  warlock_pet_t::spell_hit_event( s );

  if( s -> result == RESULT_CRIT )
  {
    trigger_empowered_imp( s );
  }
}

// ==========================================================================
// Warlock Character Definition
// ==========================================================================

// warlock_t::composite_spell_power =========================================

double warlock_t::composite_spell_power( int school )
{
  double sp = player_t::composite_spell_power( school );

  sp += _buffs.fel_armor;

  if( _buffs.life_tap_glyph )
  {
    sp += spirit() * 0.2;
  }

  if( active_pet && talents.demonic_knowledge && active_pet -> pet_type != PET_INFERNAL
                                              && active_pet -> pet_type != PET_DOOMGUARD )
  {
    sp += ( active_pet -> stamina() + 
            active_pet -> intellect() ) * talents.demonic_knowledge * 0.04;
  }

  return sp;
}

// warlock_t::create_action =================================================

action_t* warlock_t::create_action( const std::string& name,
                                    const std::string& options_str )
{
  if( name == "chaos_bolt"          ) return new          chaos_bolt_t( this, options_str );
  if( name == "conflagrate"         ) return new         conflagrate_t( this, options_str );
  if( name == "corruption"          ) return new          corruption_t( this, options_str );
  if( name == "curse_of_agony"      ) return new      curse_of_agony_t( this, options_str );
  if( name == "curse_of_doom"       ) return new       curse_of_doom_t( this, options_str );
  if( name == "curse_of_elements"   ) return new   curse_of_elements_t( this, options_str );
  if( name == "dark_pact"           ) return new           dark_pact_t( this, options_str );
  if( name == "death_coil"          ) return new          death_coil_t( this, options_str );
  if( name == "demonic_empowerment" ) return new demonic_empowerment_t( this, options_str );
  if( name == "drain_life"          ) return new          drain_life_t( this, options_str );
  if( name == "drain_soul"          ) return new          drain_soul_t( this, options_str );
  if( name == "fel_armor"           ) return new           fel_armor_t( this, options_str );
  if( name == "fire_stone"          ) return new          fire_stone_t( this, options_str );
  if( name == "spell_stone"         ) return new         spell_stone_t( this, options_str );
  if( name == "haunt"               ) return new               haunt_t( this, options_str );
  if( name == "immolate"            ) return new            immolate_t( this, options_str );
  if( name == "immolation"          ) return new          immolation_t( this, options_str );
  if( name == "shadowflame"         ) return new         shadowflame_t( this, options_str );
  if( name == "incinerate"          ) return new          incinerate_t( this, options_str );
  if( name == "inferno"             ) return new             inferno_t( this, options_str );
  if( name == "life_tap"            ) return new            life_tap_t( this, options_str );
  if( name == "metamorphosis"       ) return new       metamorphosis_t( this, options_str );
  if( name == "sacrifice_pet"       ) return new       sacrifice_pet_t( this, options_str );
  if( name == "shadow_bolt"         ) return new         shadow_bolt_t( this, options_str );
  if( name == "shadow_burn"         ) return new         shadow_burn_t( this, options_str );
  if( name == "searing_pain"        ) return new        searing_pain_t( this, options_str );
  if( name == "soul_fire"           ) return new           soul_fire_t( this, options_str );
  if( name == "siphon_life"         ) return new         siphon_life_t( this, options_str );
  if( name == "summon_pet"          ) return new          summon_pet_t( this, options_str );
  if( name == "unstable_affliction" ) return new unstable_affliction_t( this, options_str );

  return 0;
}

// warlock_t::create_pet =====================================================

pet_t* warlock_t::create_pet( const std::string& pet_name )
{
  pet_t* p = find_pet( pet_name );

  if( p ) return p;

  if( pet_name == "felguard"  ) return new  felguard_pet_t( sim, this );
  if( pet_name == "felhunter" ) return new felhunter_pet_t( sim, this );
  if( pet_name == "imp"       ) return new       imp_pet_t( sim, this );
  if( pet_name == "succubus"  ) return new  succubus_pet_t( sim, this );
  if( pet_name == "infernal"  ) return new  infernal_pet_t( sim, this );
  if( pet_name == "doomguard" ) return new doomguard_pet_t( sim, this );

  return 0;
}

// warlock_t::init_base ======================================================

void warlock_t::init_base()
{
  attribute_base[ ATTR_STRENGTH  ] =  54;
  attribute_base[ ATTR_AGILITY   ] =  55;
  attribute_base[ ATTR_STAMINA   ] =  78;
  attribute_base[ ATTR_INTELLECT ] = 130;
  attribute_base[ ATTR_SPIRIT    ] = 142;

  if( sim -> patch.before( 3, 1, 0 ) )
  {
    attribute_multiplier_initial[ ATTR_STAMINA ] *= 1.0 + talents.demonic_embrace * 0.02;
  }
  else
  {
    attribute_multiplier_initial[ ATTR_STAMINA ] *= 1.0 + talents.demonic_embrace * 0.03
                                                  + ( ( talents.demonic_embrace ) ? 0.01 : 0 );
  }

  base_spell_crit = 0.0170;
  initial_spell_crit_per_intellect = rating_t::interpolate( level, 0.01/60.0, 0.01/80.0, 0.01/166.6 );

  base_attack_power = -10;
  base_attack_crit  = 0.02;
  initial_attack_power_per_strength = 1.0;
  initial_attack_crit_per_agility = rating_t::interpolate( level, 0.01/16.0, 0.01/24.9, 0.01/52.1 );

  // FIXME! Make this level-specific.
  resource_base[ RESOURCE_HEALTH ] = 3200;
  resource_base[ RESOURCE_MANA   ] = rating_t::interpolate( level, 1383, 2620, 3863 );

  health_per_stamina = 10;
  mana_per_intellect = 15;

  mana_per_intellect *= 1.0 + ( talents.fel_intellect + talents.fel_vitality ) * 0.01;
  health_per_stamina *= 1.0 + ( talents.fel_stamina   + talents.fel_vitality ) * 0.01;
}

// warlock_t::reset ==========================================================

void warlock_t::reset()
{
  player_t::reset();

  // Active
  active_pet         = 0;
  active_corruption  = 0;
  active_curse       = 0;
  active_immolate    = 0;
  active_shadowflame = 0;
  active_dots        = 0;
  affliction_effects = 0;

  _buffs.reset();
  _cooldowns.reset();
  _expirations.reset();
}

// warlock_t::regen ==========================================================

void warlock_t::regen( double periodicity )
{
  player_t::regen( periodicity );

  if( _buffs.pet_sacrifice == PET_FELHUNTER )
  {
    double felhunter_regen = periodicity * resource_max[ RESOURCE_MANA ] * 0.03 / 4.0;

    resource_gain( RESOURCE_MANA, felhunter_regen, gains_felhunter );
  }
}

// warlock_t::get_talent_trees =============================================

bool warlock_t::get_talent_trees( std::vector<int*>& affliction,
                                  std::vector<int*>& demonology,
                                  std::vector<int*>& destruction )
{
  if( sim -> patch.before(3, 1, 0) )
  {
    talent_translation_t translation[][3] =
    {
      { {  1, &( talents.improved_curse_of_agony ) }, {  1, NULL                                  }, {  1, &( talents.improved_shadow_bolt  ) } },
      { {  2, &( talents.suppression             ) }, {  2, &( talents.improved_imp             ) }, {  2, &( talents.bane                  ) } },
      { {  3, &( talents.improved_corruption     ) }, {  3, &( talents.demonic_embrace          ) }, {  3, NULL                               } },
      { {  4, NULL                                 }, {  4, NULL                                  }, {  4, NULL                               } },
      { {  5, &( talents.improved_drain_soul     ) }, {  5, &( talents.demonic_brutality        ) }, {  5, &( talents.cataclysm             ) } },
      { {  6, &( talents.improved_life_tap       ) }, {  6, &( talents.fel_vitality             ) }, {  6, &( talents.demonic_power         ) } },
      { {  7, &( talents.soul_siphon             ) }, {  7, NULL                                  }, {  7, &( talents.shadow_burn           ) } },
      { {  8, NULL                                 }, {  8, NULL                                  }, {  8, &( talents.ruin                  ) } },
      { {  9, NULL                                 }, {  9, &( talents.fel_domination           ) }, {  9, NULL                               } },
      { { 10, &( talents.amplify_curse           ) }, { 10, &( talents.demonic_aegis            ) }, { 10, &( talents.destructive_reach     ) } },
      { { 11, NULL                                 }, { 11, &( talents.unholy_power             ) }, { 11, &( talents.improved_searing_pain ) } },
      { { 12, &( talents.nightfall               ) }, { 12, &( talents.master_summoner          ) }, { 12, NULL                               } },
      { { 13, &( talents.empowered_corruption    ) }, { 13, &( talents.demonic_sacrifice        ) }, { 13, &( talents.improved_immolate     ) } },
      { { 14, &( talents.shadow_embrace          ) }, { 14, &( talents.master_conjuror          ) }, { 14, &( talents.devastation           ) } },
      { { 15, &( talents.siphon_life             ) }, { 15, &( talents.mana_feed                ) }, { 15, NULL                               } },
      { { 16, NULL                                 }, { 16, &( talents.master_demonologist      ) }, { 16, &( talents.emberstorm            ) } },
      { { 17, &( talents.improved_felhunter      ) }, { 17, NULL                                  }, { 17, &( talents.conflagrate           ) } },
      { { 18, &( talents.shadow_mastery          ) }, { 18, NULL                                  }, { 18, &( talents.soul_leech            ) } },
      { { 19, &( talents.eradication             ) }, { 19, &( talents.demonic_empowerment      ) }, { 19, &( talents.backlash              ) } },
      { { 20, &( talents.contagion               ) }, { 20, &( talents.demonic_knowledge        ) }, { 20, &( talents.shadow_and_flame      ) } },
      { { 21, &( talents.dark_pact               ) }, { 21, &( talents.demonic_tactics          ) }, { 21, &( talents.improved_soul_leech   ) } },
      { { 22, NULL                                 }, { 22, &( talents.fel_synergy              ) }, { 22, &( talents.backdraft             ) } },
      { { 23, &( talents.malediction             ) }, { 23, &( talents.improved_demonic_tactics ) }, { 23, NULL                               } },
      { { 24, &( talents.deaths_embrace          ) }, { 24, &( talents.summon_felguard          ) }, { 24, &( talents.empowered_imp         ) } },
      { { 25, &( talents.unstable_affliction     ) }, { 25, &( talents.demonic_empathy          ) }, { 25, &( talents.fire_and_brimstone    ) } },
      { { 26, &( talents.pandemic                ) }, { 26, &( talents.demonic_pact             ) }, { 26, &( talents.chaos_bolt            ) } },
      { { 27, &( talents.everlasting_affliction  ) }, { 27, &( talents.metamorphosis            ) }, {  0, NULL                               } },
      { { 28, &( talents.haunt                   ) }, {  0, NULL                                  }, {  0, NULL                               } },
      { {  0, NULL                                 }, {  0, NULL                                  }, {  0, NULL                               } }
    };
    return player_t::get_talent_trees( affliction, demonology, destruction, translation );
  }
  else
  {
    talent_translation_t translation[][3] =
    {
      { {  1, &( talents.improved_curse_of_agony ) }, {  1, NULL                                  }, {  1, &( talents.improved_shadow_bolt  ) } },
      { {  2, &( talents.suppression             ) }, {  2, &( talents.improved_imp             ) }, {  2, &( talents.bane                  ) } },
      { {  3, &( talents.improved_corruption     ) }, {  3, &( talents.demonic_embrace          ) }, {  3, &( talents.aftermath             ) } },
      { {  4, NULL                                 }, {  4, NULL                                  }, {  4, NULL                               } },
      { {  5, &( talents.improved_drain_soul     ) }, {  5, NULL                                  }, {  5, &( talents.cataclysm             ) } },
      { {  6, &( talents.improved_life_tap       ) }, {  6, &( talents.demonic_brutality        ) }, {  6, &( talents.demonic_power         ) } },
      { {  7, &( talents.soul_siphon             ) }, {  7, &( talents.fel_vitality             ) }, {  7, &( talents.shadow_burn           ) } },
      { {  8, NULL                                 }, {  8, NULL                                  }, {  8, &( talents.ruin                  ) } },
      { {  9, NULL                                 }, {  9, NULL                                  }, {  9, NULL                               } },
      { { 10, &( talents.amplify_curse           ) }, { 10, &( talents.fel_domination           ) }, { 10, &( talents.destructive_reach     ) } },
      { { 11, NULL                                 }, { 11, &( talents.demonic_aegis            ) }, { 11, &( talents.improved_searing_pain ) } },
      { { 12, &( talents.nightfall               ) }, { 12, &( talents.unholy_power             ) }, { 12, &( talents.backlash              ) } },
      { { 13, &( talents.empowered_corruption    ) }, { 13, &( talents.master_summoner          ) }, { 13, &( talents.improved_immolate     ) } },
      { { 14, &( talents.shadow_embrace          ) }, { 14, &( talents.mana_feed                ) }, { 14, &( talents.devastation           ) } },
      { { 15, &( talents.siphon_life             ) }, { 15, &( talents.master_conjuror          ) }, { 15, NULL                               } },
      { { 16, NULL                                 }, { 16, &( talents.master_demonologist      ) }, { 16, &( talents.emberstorm            ) } },
      { { 17, &( talents.improved_felhunter      ) }, { 17, &( talents.molten_core              ) }, { 17, &( talents.conflagrate           ) } },
      { { 18, &( talents.shadow_mastery          ) }, { 18, NULL                                  }, { 18, &( talents.soul_leech            ) } },
      { { 19, &( talents.eradication             ) }, { 19, &( talents.demonic_empowerment      ) }, { 19, &( talents.pyroclasm             ) } },
      { { 20, &( talents.contagion               ) }, { 20, &( talents.demonic_knowledge        ) }, { 20, &( talents.shadow_and_flame      ) } },
      { { 21, &( talents.dark_pact               ) }, { 21, &( talents.demonic_tactics          ) }, { 21, &( talents.improved_soul_leech   ) } },
      { { 22, NULL                                 }, { 22, &( talents.decimation               ) }, { 22, &( talents.backdraft             ) } },
      { { 23, &( talents.malediction             ) }, { 23, &( talents.improved_demonic_tactics ) }, { 23, NULL                               } },
      { { 24, &( talents.deaths_embrace          ) }, { 24, &( talents.summon_felguard          ) }, { 24, &( talents.empowered_imp         ) } },
      { { 25, &( talents.unstable_affliction     ) }, { 25, &( talents.nemesis                  ) }, { 25, &( talents.fire_and_brimstone    ) } },
      { { 26, &( talents.pandemic                ) }, { 26, &( talents.demonic_pact             ) }, { 26, &( talents.chaos_bolt            ) } },
      { { 27, &( talents.everlasting_affliction  ) }, { 27, &( talents.metamorphosis            ) }, {  0, NULL                               } },
      { { 28, &( talents.haunt                   ) }, {  0, NULL                                  }, {  0, NULL                               } },
      { {  0, NULL                                 }, {  0, NULL                                  }, {  0, NULL                               } }
    };
    return player_t::get_talent_trees( affliction, demonology, destruction, translation );
  }
}

// warlock_t::parse_option =================================================

bool warlock_t::parse_option( const std::string& name,
                              const std::string& value )
{
  option_t options[] =
  {
    { "aftermath",                OPT_INT,  &( talents.aftermath                ) },
    { "amplify_curse",            OPT_INT,  &( talents.amplify_curse            ) },
    { "backdraft",                OPT_INT,  &( talents.backdraft                ) },
    { "backlash",                 OPT_INT,  &( talents.backlash                 ) },
    { "bane",                     OPT_INT,  &( talents.bane                     ) },
    { "cataclysm",                OPT_INT,  &( talents.cataclysm                ) },
    { "chaos_bolt",               OPT_INT,  &( talents.chaos_bolt               ) },
    { "conflagrate",              OPT_INT,  &( talents.conflagrate              ) },
    { "contagion",                OPT_INT,  &( talents.contagion                ) },
    { "dark_pact",                OPT_INT,  &( talents.dark_pact                ) },
    { "deaths_embrace",           OPT_INT,  &( talents.deaths_embrace           ) },
    { "decimation",               OPT_INT,  &( talents.decimation               ) },
    { "demonic_aegis",            OPT_INT,  &( talents.demonic_aegis            ) },
    { "demonic_brutality",        OPT_INT,  &( talents.demonic_brutality        ) },
    { "demonic_embrace",          OPT_INT,  &( talents.demonic_embrace          ) },
    { "demonic_empowerment",      OPT_INT,  &( talents.demonic_empowerment      ) },
    { "demonic_knowledge",        OPT_INT,  &( talents.demonic_knowledge        ) },
    { "demonic_pact",             OPT_INT,  &( talents.demonic_pact             ) },
    { "demonic_power",            OPT_INT,  &( talents.demonic_power            ) },
    { "demonic_sacrifice",        OPT_INT,  &( talents.demonic_sacrifice        ) },
    { "demonic_tactics",          OPT_INT,  &( talents.demonic_tactics          ) },
    { "destructive_reach",        OPT_INT,  &( talents.destructive_reach        ) },
    { "devastation",              OPT_INT,  &( talents.devastation              ) },
    { "emberstorm",               OPT_INT,  &( talents.emberstorm               ) },
    { "empowered_corruption",     OPT_INT,  &( talents.empowered_corruption     ) },
    { "empowered_imp",            OPT_INT,  &( talents.empowered_imp            ) },
    { "eradication",              OPT_INT,  &( talents.eradication              ) },
    { "everlasting_affliction",   OPT_INT,  &( talents.everlasting_affliction   ) },
    { "fel_domination",           OPT_INT,  &( talents.fel_domination           ) },
    { "fel_intellect",            OPT_INT,  &( talents.fel_intellect            ) },
    { "fel_stamina",              OPT_INT,  &( talents.fel_stamina              ) },
    { "fel_synergy",              OPT_INT,  &( talents.fel_synergy              ) },
    { "fel_vitality",             OPT_INT,  &( talents.fel_vitality             ) },
    { "fire_and_brimstone",       OPT_INT,  &( talents.fire_and_brimstone       ) },
    { "haunt",                    OPT_INT,  &( talents.haunt                    ) },
    { "improved_corruption",      OPT_INT,  &( talents.improved_corruption      ) },
    { "improved_curse_of_agony",  OPT_INT,  &( talents.improved_curse_of_agony  ) },
    { "improved_demonic_tactics", OPT_INT,  &( talents.improved_demonic_tactics ) },
    { "improved_drain_soul",      OPT_INT,  &( talents.improved_drain_soul      ) },
    { "improved_felhunter",       OPT_INT,  &( talents.improved_felhunter       ) },
    { "improved_fire_bolt",       OPT_INT,  &( talents.improved_fire_bolt       ) },
    { "improved_immolate",        OPT_INT,  &( talents.improved_immolate        ) },
    { "improved_imp",             OPT_INT,  &( talents.improved_imp             ) },
    { "improved_lash_of_pain",    OPT_INT,  &( talents.improved_lash_of_pain    ) },
    { "improved_life_tap",        OPT_INT,  &( talents.improved_life_tap        ) },
    { "improved_searing_pain",    OPT_INT,  &( talents.improved_searing_pain    ) },
    { "improved_shadow_bolt",     OPT_INT,  &( talents.improved_shadow_bolt     ) },
    { "improved_soul_leech",      OPT_INT,  &( talents.improved_soul_leech      ) },
    { "improved_succubus",        OPT_INT,  &( talents.improved_succubus        ) },
    { "improved_voidwalker",      OPT_INT,  &( talents.improved_voidwalker      ) },
    { "malediction",              OPT_INT,  &( talents.malediction              ) },
    { "mana_feed",                OPT_INT,  &( talents.mana_feed                ) },
    { "master_conjuror",          OPT_INT,  &( talents.master_conjuror          ) },
    { "master_demonologist",      OPT_INT,  &( talents.master_demonologist      ) },
    { "master_summoner",          OPT_INT,  &( talents.master_summoner          ) },
    { "metamorphosis",            OPT_INT,  &( talents.metamorphosis            ) },
    { "molten_core",              OPT_INT,  &( talents.molten_core              ) },
    { "nemesis",                  OPT_INT,  &( talents.nemesis                  ) },
    { "nightfall",                OPT_INT,  &( talents.nightfall                ) },
    { "pandemic",                 OPT_INT,  &( talents.pandemic                 ) },
    { "pyroclasm",                OPT_INT,  &( talents.pyroclasm                ) },
    { "ruin",                     OPT_INT,  &( talents.ruin                     ) },
    { "shadow_and_flame",         OPT_INT,  &( talents.shadow_and_flame         ) },
    { "shadow_burn",              OPT_INT,  &( talents.shadow_burn              ) },
    { "shadow_mastery",           OPT_INT,  &( talents.shadow_mastery           ) },
    { "siphon_life",              OPT_INT,  &( talents.siphon_life              ) },
    { "soul_leech",               OPT_INT,  &( talents.soul_leech               ) },
    { "soul_link",                OPT_INT,  &( talents.soul_link                ) },
    { "soul_siphon",              OPT_INT,  &( talents.soul_siphon              ) },
    { "summon_felguard",          OPT_INT,  &( talents.summon_felguard          ) },
    { "suppression",              OPT_INT,  &( talents.suppression              ) },
    { "unholy_power",             OPT_INT,  &( talents.unholy_power             ) },
    { "unstable_affliction",      OPT_INT,  &( talents.unstable_affliction      ) },
    // Glyphs
    { "glyph_chaos_bolt",          OPT_INT,  &( glyphs.chaos_bolt               ) },
    { "glyph_conflagrate",         OPT_INT,  &( glyphs.conflagrate              ) },
    { "glyph_corruption",          OPT_INT,  &( glyphs.corruption               ) },
    { "glyph_curse_of_agony",      OPT_INT,  &( glyphs.curse_of_agony           ) },
    { "glyph_felguard",            OPT_INT,  &( glyphs.felguard                 ) },
    { "glyph_felhunter",           OPT_INT,  &( glyphs.felhunter                ) },
    { "glyph_haunt",               OPT_INT,  &( glyphs.haunt                    ) },
    { "glyph_immolate",            OPT_INT,  &( glyphs.immolate                 ) },
    { "glyph_imp",                 OPT_INT,  &( glyphs.imp                      ) },
    { "glyph_incinerate",          OPT_INT,  &( glyphs.incinerate               ) },
    { "glyph_life_tap",            OPT_INT,  &( glyphs.life_tap                 ) },
    { "glyph_metamorphosis",       OPT_INT,  &( glyphs.metamorphosis            ) },
    { "glyph_searing_pain",        OPT_INT,  &( glyphs.searing_pain             ) },
    { "glyph_shadow_bolt",         OPT_INT,  &( glyphs.shadow_bolt              ) },
    { "glyph_shadow_burn",         OPT_INT,  &( glyphs.shadow_burn              ) },
    { "glyph_siphon_life",         OPT_INT,  &( glyphs.siphon_life              ) },
    { "glyph_unstable_affliction", OPT_INT,  &( glyphs.unstable_affliction      ) },
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

// player_t::create_warlock ================================================

player_t* player_t::create_warlock( sim_t*       sim, 
                                    std::string& name ) 
{
  warlock_t* p = new warlock_t( sim, name );

  new  felguard_pet_t( sim, p );
  new felhunter_pet_t( sim, p );
  new       imp_pet_t( sim, p );
  new  succubus_pet_t( sim, p );
  new  infernal_pet_t( sim, p );
  new doomguard_pet_t( sim, p );

  return p;
}


