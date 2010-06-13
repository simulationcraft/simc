// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================
#include "simulationcraft.h"


// ==========================================================================
// Warlock
// ==========================================================================

enum stone_type_t { STONE_NONE=0, SPELL_STONE, FIRE_STONE };

struct warlock_pet_t;

struct warlock_t : public player_t
{
  // Active
  warlock_pet_t* active_pet;
  action_t*      active_corruption;
  action_t*      active_curse;
  action_t*      active_curse_of_agony;
  action_t*      active_curse_of_doom;
  action_t*      active_drain_life;
  action_t*      active_drain_soul;
  action_t*      active_immolate;
  action_t*      active_pandemic;
  action_t*      active_shadowflame;
  action_t*      active_unstable_affliction;

  std::queue<double> decimation_queue;

  buff_t* buffs_backdraft;
  buff_t* buffs_decimation;
  buff_t* buffs_demonic_empowerment;
  buff_t* buffs_demonic_frenzy;
  buff_t* buffs_empowered_imp;
  buff_t* buffs_eradication;
  buff_t* buffs_fel_armor;
  buff_t* buffs_haunted;
  buff_t* buffs_life_tap_glyph;
  buff_t* buffs_shadow_embrace;
  buff_t* buffs_metamorphosis;
  buff_t* buffs_molten_core;
  buff_t* buffs_pyroclasm;
  buff_t* buffs_shadow_trance;
  buff_t* buffs_tier7_2pc_caster;
  buff_t* buffs_tier7_4pc_caster;
  buff_t* buffs_tier10_4pc_caster;

  // Gains
  gain_t* gains_dark_pact;
  gain_t* gains_fel_armor;
  gain_t* gains_felhunter;
  gain_t* gains_life_tap;
  gain_t* gains_soul_leech;

  // Procs
  proc_t* procs_dark_pact;
  proc_t* procs_life_tap;

  // Random Number Generators
  rng_t* rng_soul_leech;
  rng_t* rng_improved_soul_leech;
  rng_t* rng_everlasting_affliction;

  // Custom Parameters
  std::string summon_pet_str;

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
    int  demonic_empowerment;
    int  demonic_knowledge;
    int  demonic_pact;
    int  demonic_power;
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
    int  shadowfury;
    int  shadow_mastery;
    int  siphon_life;
    int  soul_leech;
    int  soul_link;
    int  soul_siphon;
    int  summon_felguard;
    int  suppression;
    int  unholy_power;
    int  unstable_affliction;

    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
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
    int quick_decay;
    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  int hasted_corruption;

  warlock_t( sim_t* sim, const std::string& name, int race_type = RACE_NONE ) : player_t( sim, WARLOCK, name, race_type )
  {
    distance = 30;

    active_pet                 = 0;
    active_corruption          = 0;
    active_curse               = 0;
    active_curse_of_agony      = 0;
    active_curse_of_doom       = 0;
    active_drain_life          = 0;
    active_drain_soul          = 0;
    active_immolate            = 0;
    active_pandemic            = 0;
    active_shadowflame         = 0;
    active_unstable_affliction = 0;

    hasted_corruption = -1;
  }

  // Character Definition
  virtual void      init_glyphs();
  virtual void      init_race();
  virtual void      init_base();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      reset();
  virtual std::vector<talent_translation_t>& get_talent_list();
  virtual std::vector<option_t>& get_options();
  virtual dot_t*    get_dot( const std::string& name );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name );
  virtual void      create_pets();
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST { return RESOURCE_MANA; }
  virtual int       primary_role() SC_CONST     { return ROLE_SPELL; }
  virtual int       primary_tree() SC_CONST;
  virtual double    composite_spell_power( int school ) SC_CONST;

  // Event Tracking
  virtual void regen( double periodicity );
  virtual action_expr_t* create_expression( action_t*, const std::string& name );

  // Utilities
  int affliction_effects()
  {
    int effects = 0;
    if ( active_curse                    ) effects++;
    if ( active_corruption               ) effects++;
    if ( active_drain_life               ) effects++;
    if ( active_drain_soul               ) effects++;
    if ( active_unstable_affliction      ) effects++;
    if ( buffs_haunted        -> check() ) effects++;
    if ( buffs_shadow_embrace -> check() ) effects++;
    return effects;
  }
  int active_dots()
  {
    int dots = 0;
    if ( active_curse_of_agony           ) dots++;
    if ( active_curse_of_doom            ) dots++;
    if ( active_corruption               ) dots++;
    if ( active_drain_life               ) dots++;
    if ( active_drain_soul               ) dots++;
    if ( active_immolate                 ) dots++;
    if ( active_shadowflame              ) dots++;
    if ( active_unstable_affliction      ) dots++;
    return dots;
  }
};

// ==========================================================================
// Curse of Elements
// ==========================================================================

struct coe_debuff_t : public debuff_t
{
  coe_debuff_t( sim_t* s ) : debuff_t( s, "curse_of_elements", 1, 300.0 ) {}
  virtual void expire()
  {
    if( player ) 
    {
      player -> cast_warlock() -> active_curse = 0;
      player = 0;
    }
    debuff_t::expire();
  }
};

// ==========================================================================
// Warlock Pet
// ==========================================================================

enum pet_type_t { PET_NONE=0,     PET_FELGUARD, PET_FELHUNTER, PET_IMP,
                  PET_VOIDWALKER, PET_SUCCUBUS, PET_INFERNAL,  PET_DOOMGUARD
                };

struct warlock_pet_t : public pet_t
{
  int       pet_type;
  double    damage_modifier;
  attack_t* melee;

  warlock_pet_t( sim_t* sim, player_t* owner, const std::string& pet_name, int pt ) :
      pet_t( sim, owner, pet_name ), pet_type( pt ), 
      damage_modifier( 1.0 ), melee( 0 )
  {
  }

  virtual bool ooc_buffs() { return true; }

  virtual void init_base()
  {
    pet_t::init_base();

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

    base_attack_crit = 0.0328;
    base_spell_crit  = 0.0092;
    
    initial_attack_power_per_strength = 2.0;
    initial_attack_crit_per_agility   = 0.01 / 52.0;
    initial_spell_crit_per_intellect  = owner -> initial_spell_crit_per_intellect;

    health_per_stamina = 10;
    mana_per_intellect = 10.8;
    mp5_per_intellect  = 2.0 / 3.0;

    base_mp5 = -55;
  }

  virtual void schedule_ready( double delta_time=0,
                               bool   waiting=false )
  {
    if ( melee && ! melee -> execute_event )
    {
      melee -> schedule_execute();
    }

    pet_t::schedule_ready( delta_time, waiting );
  }

  virtual void summon( double duration=0 )
  {
    warlock_t* o = owner -> cast_warlock();
    pet_t::summon( duration );
    o -> active_pet = this;
  }

  virtual void dismiss()
  {
    warlock_t* o = owner -> cast_warlock();
    pet_t::dismiss();
    o -> active_pet = 0;
  }

  virtual void interrupt()
  {
    pet_t::interrupt();
    if ( melee ) melee -> cancel();
  }

  void adjust_base_modifiers( action_t* a )
  {
    warlock_t* o = owner -> cast_warlock();

    // Orc Command Racial
    if ( o -> race == RACE_ORC )
    {
      a -> base_multiplier  *= 1.05;
    }

    if ( pet_type != PET_INFERNAL && pet_type != PET_DOOMGUARD )
    {
      a -> base_crit        += o -> talents.demonic_tactics * 0.02;
      a -> base_multiplier  *= 1.0 + o -> talents.unholy_power * 0.04;
    }

    if ( pet_type == PET_FELGUARD )
    {
      a -> base_multiplier *= 1.0 + o -> talents.master_demonologist * 0.01;
    }
    else if ( pet_type == PET_IMP )
    {
      if ( a -> school == SCHOOL_FIRE )
      {
        a -> base_multiplier *= 1.0 + o -> talents.master_demonologist * 0.01;
        a -> base_crit       +=       o -> talents.master_demonologist * 0.01;
      }
    }
    else if ( pet_type == PET_SUCCUBUS )
    {
      if ( a -> school == SCHOOL_SHADOW )
      {
        a -> base_multiplier *= 1.0 + o -> talents.master_demonologist * 0.01;
        a -> base_crit       +=       o -> talents.master_demonologist * 0.01;
      }
    }

    if ( o -> set_bonus.tier9_2pc_caster() )
    {
      a -> base_crit += 0.10;
    }
  }


  void adjust_player_modifiers( action_t* a )
  {
    warlock_t* o = owner -> cast_warlock();
    if ( o -> talents.improved_demonic_tactics )
    {
      a -> player_crit += o -> composite_spell_crit() * o -> talents.improved_demonic_tactics * 0.10;
    }
  }

  virtual double composite_attack_hit() SC_CONST
  {
    return owner -> composite_spell_hit();
  }

  virtual double composite_attack_expertise() SC_CONST
  {
    return owner -> composite_spell_hit() * 26.0 / 17.0;
  }

  virtual void register_callbacks();

  virtual int primary_resource() SC_CONST { return RESOURCE_MANA; }
};

// ==========================================================================
// Warlock Pet Melee
// ==========================================================================

struct warlock_pet_melee_t : public attack_t
{
  warlock_pet_melee_t( player_t* player, const char* name ) :
      attack_t( name, player, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_NONE, false )
  {
    warlock_pet_t* p = ( warlock_pet_t* ) player -> cast_pet();

    weapon = &( p -> main_hand_weapon );
    base_execute_time = weapon -> swing_time;
    base_dd_min = base_dd_max = 1;
    may_crit    = true;
    background  = true;
    repeating   = true;

    p -> adjust_base_modifiers( this );

    base_multiplier *= p -> damage_modifier;
  }

  virtual void player_buff()
  {
    warlock_pet_t* p = ( warlock_pet_t* ) player -> cast_pet();
    warlock_t* o = p -> owner -> cast_warlock();
    attack_t::player_buff();
    if ( o -> buffs_tier10_4pc_caster -> up() )
    {
      player_multiplier *= 1.10;
    }
    if ( p -> pet_type != PET_INFERNAL )
    {
      player_attack_power += 0.57 * o -> composite_spell_power( SCHOOL_MAX );
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
      attack_t( n, player, r, s, TREE_NONE, true )
  {
    special    = true;
    may_crit   = true;
    warlock_pet_t* p = ( warlock_pet_t* ) player -> cast_pet();
    p -> adjust_base_modifiers( this );
  }

  virtual void execute()
  {
    attack_t::execute();
  }

  virtual void player_buff()
  {
    warlock_pet_t* p = ( warlock_pet_t* ) player -> cast_pet();
    warlock_t* o = p -> owner -> cast_warlock();
    attack_t::player_buff();
    player_attack_power += 0.57 * o -> composite_spell_power( SCHOOL_MAX );
    if ( o -> buffs_tier10_4pc_caster -> up() )
    {
      player_multiplier *= 1.10;
    }
    p -> adjust_player_modifiers( this );
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
    warlock_pet_t* p = ( warlock_pet_t* ) player -> cast_pet();
    p -> adjust_base_modifiers( this );
  }

  virtual void player_buff()
  {
    warlock_pet_t* p = ( warlock_pet_t* ) player -> cast_pet();
    warlock_t* o = p -> owner -> cast_warlock();
    spell_t::player_buff();
    if ( o -> buffs_tier10_4pc_caster -> up() )
    {
      player_multiplier *= 1.10;
    }
    player_spell_power += 0.15 * o -> composite_spell_power( SCHOOL_MAX );
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
      imp_pet_t* p = ( imp_pet_t* ) player -> cast_pet();
      warlock_t* o = p -> owner -> cast_warlock();

      static rank_t ranks[] =
      {
        { 78, 8, 203, 227, 0, 180 },
        { 68, 8, 110, 124, 0, 145 },
        { 58, 7,  83,  93, 0, 115 },
        { 0, 0, 0, 0, 0, 0 }
      };
      init_rank( ranks, 47964 );
      // improved_imp does not influence +SP from player
      base_dd_min*=1.0 +  o -> talents.improved_imp  * 0.10;
      base_dd_max*=1.0 +  o -> talents.improved_imp  * 0.10;

      base_execute_time = 2.5;
      direct_power_mod  = ( base_execute_time / 3.5 ); // was 2.0/3.5
      may_crit          = true;

      min_gcd           = 1.0;

      base_execute_time -= 0.25 * ( o -> talents.improved_fire_bolt + o -> talents.demonic_power );

      base_multiplier *= 1.0 + ( o -> talents.empowered_imp * 0.10 + // o -> talents.improved_imp moved up
                                 o -> glyphs.imp            * 0.20 );

      base_crit_bonus_multiplier *= 1.0 + o -> talents.ruin * 0.20;
    }
    virtual void execute();
    virtual void player_buff()
    {
      warlock_t* o = player -> cast_pet() -> owner -> cast_warlock();
      warlock_pet_spell_t::player_buff();
      if ( o -> buffs_demonic_empowerment -> up() ) player_crit += 0.20;
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
    mana_per_intellect = 4.8;
    mp5_per_intellect  = 5.0 / 6.0;

    base_mp5 = -257;
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "fire_bolt" ) return new fire_bolt_t( this );

    return player_t::create_action( name, options_str );
  }
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
      felguard_pet_t* p = ( felguard_pet_t* ) player -> cast_pet();

      static rank_t ranks[] =
      {
        { 76, 4, 124, 124, 0, 0.1 },
        { 68, 3,  78,  78, 0, 0.1 },
        { 0, 0, 0, 0, 0, 0 }
      };
      init_rank( ranks, 47994 );

      weapon   = &( p -> main_hand_weapon );
      cooldown -> duration = 6.0;
    }
    virtual void player_buff()
    {
      felguard_pet_t* p = ( felguard_pet_t* ) player -> cast_pet();
      warlock_t*      o = p -> owner -> cast_warlock();

      warlock_pet_attack_t::player_buff();
      player_attack_power *= 1.0 + o -> buffs_demonic_frenzy -> stack() * ( 0.05 + o -> talents.demonic_brutality * 0.01 );
      if ( o -> glyphs.felguard ) player_attack_power *= 1.20;
    }
  };

  struct melee_t : public warlock_pet_melee_t
  {
    melee_t( player_t* player ) :
        warlock_pet_melee_t( player, "felguard_melee" )
    {}
    virtual double execute_time() SC_CONST
    {
      warlock_t* o = player -> cast_pet() -> owner -> cast_warlock();
      double t = attack_t::execute_time();
      if ( o -> buffs_demonic_empowerment -> up() ) t *= 1.0 / 1.20;
      return t;
    }

    virtual void player_buff()
    {
      felguard_pet_t* p = ( felguard_pet_t* ) player -> cast_pet();
      warlock_t*      o = p -> owner -> cast_warlock();

      warlock_pet_melee_t::player_buff();
      player_attack_power *= 1.0 + o -> buffs_demonic_frenzy -> stack() * ( 0.05 + o -> talents.demonic_brutality * 0.01 );

      if ( o -> glyphs.felguard ) player_attack_power *= 1.20;
    }
    virtual void assess_damage( double amount, int dmg_type )
    {
      warlock_t*  o = player -> cast_pet() -> owner -> cast_warlock();
      attack_t::assess_damage( amount, dmg_type );
      o -> buffs_demonic_frenzy -> trigger();
    }
  };

  felguard_pet_t( sim_t* sim, player_t* owner ) :
      warlock_pet_t( sim, owner, "felguard", PET_FELGUARD )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 412.5;
    main_hand_weapon.max_dmg    = 412.5;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = 2.0;

    damage_modifier = 1.05;

    action_list_str = "cleave/wait_until_ready";
  }
  virtual void init_base()
  {
    warlock_pet_t::init_base();

    resource_base[ RESOURCE_HEALTH ] = 1627;
    resource_base[ RESOURCE_MANA   ] = 3331;

    base_attack_power = -20;

    melee = new melee_t( this );
  }
  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "cleave" ) return new cleave_t( this );

    return player_t::create_action( name, options_str );
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
      felhunter_pet_t* p = ( felhunter_pet_t* ) player -> cast_pet();
      warlock_t*       o = p -> owner -> cast_warlock();

      static rank_t ranks[] =
      {
        { 74, 5, 118, 118, 0, 0.03 },
        { 66, 4, 101, 101, 0, 0.03 },
        { 0, 0, 0, 0, 0, 0 }
      };
      init_rank( ranks, 54053 );

      base_spell_power_multiplier = 1.0;
      base_attack_power_multiplier = 0.0;

      direct_power_mod  = ( 1.5 / 3.5 ); // Converting from Pet Attack Power into Pet Spell Power.
      may_crit          = true;

      cooldown -> duration = 6.0;

      cooldown -> duration -= 2.0 * o -> talents.improved_felhunter;
      base_multiplier *= 1.0 + ( o -> talents.shadow_mastery * 0.03 );

      base_crit_bonus = 0.5;
      base_crit_bonus_multiplier = 2.0;
    }
      
    virtual void execute()
    {
      felhunter_pet_t* p = ( felhunter_pet_t* ) player -> cast_pet();
      warlock_t*       o = p -> owner -> cast_warlock();

      warlock_pet_attack_t::execute();

      if ( result_is_hit() && o -> talents.improved_felhunter )
      {
        double amount = p -> resource_max[ RESOURCE_MANA ] * o -> talents.improved_felhunter * 0.08;

        p -> resource_gain( RESOURCE_MANA, amount );
      }
    }

    virtual void player_buff()
    {
      felhunter_pet_t* p = ( felhunter_pet_t* ) player -> cast_pet();
      warlock_t*      o = p -> owner -> cast_warlock();
      warlock_pet_attack_t::player_buff();
      player_spell_power += 0.15 * player -> cast_pet() -> owner -> composite_spell_power( SCHOOL_MAX );
      player_multiplier *= 1.0 + o -> active_dots() * 0.15;
    }
  };

  felhunter_pet_t( sim_t* sim, player_t* owner ) :
      warlock_pet_t( sim, owner, "felhunter", PET_FELHUNTER )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 309.6;
    main_hand_weapon.max_dmg    = 309.6;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = 2.0;

//    damage_modifier = 0.8;

    action_list_str = "shadow_bite/wait_until_ready";
  }
  virtual void init_base()
  {
    warlock_pet_t::init_base();

    base_attack_power = -20;

    resource_base[ RESOURCE_HEALTH ] = 4788;
    resource_base[ RESOURCE_MANA   ] = 1559;

    attribute_base[ ATTR_STRENGTH  ] = 314;
    attribute_base[ ATTR_AGILITY   ] = 90;
    attribute_base[ ATTR_STAMINA   ] = 328;
    attribute_base[ ATTR_INTELLECT ] = 150;
    attribute_base[ ATTR_SPIRIT    ] = 209;

    base_attack_power = -20;
    initial_attack_power_per_strength = 2.0;

    health_per_stamina = 9.5;
    mana_per_intellect = 11.55;
    mp5_per_intellect  = 8.0 / 324.0;

    base_mp5 = 11.22;

    base_attack_crit = 0.0327;
 
    melee = new warlock_pet_melee_t( this, "felhunter_melee" );
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "shadow_bite" ) return new shadow_bite_t( this );

    return player_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Pet Succubus
// ==========================================================================

struct succubus_pet_t : public warlock_pet_t
{
  struct lash_of_pain_t : public warlock_pet_spell_t
  {
    lash_of_pain_t( player_t* player ) :
        warlock_pet_spell_t( "lash_of_pain", player, RESOURCE_MANA, SCHOOL_SHADOW )
    {
      static rank_t ranks[] =
      {
        { 80, 9, 237, 237, 0, 250 },
        { 74, 8, 193, 193, 0, 220 },
        { 68, 7, 123, 123, 0, 190 },
        { 60, 6,  99,  99, 0, 160 },
        { 0, 0, 0, 0, 0, 0 }
      };
      init_rank( ranks, 47992 );

      direct_power_mod  = ( 1.5 / 3.5 );
      may_crit          = true;

      succubus_pet_t* p = ( succubus_pet_t* ) player -> cast_pet();
      warlock_t*      o = p -> owner -> cast_warlock();

      cooldown -> duration  = 12.0;
      cooldown -> duration -= 3.0 * ( o -> talents.improved_lash_of_pain + o -> talents.demonic_power );
    }
  };

  succubus_pet_t( sim_t* sim, player_t* owner ) :
      warlock_pet_t( sim, owner, "succubus", PET_SUCCUBUS )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 412.5;
    main_hand_weapon.max_dmg    = 412.5;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = 2.0;

    damage_modifier = 1.05;

    action_list_str = "lash_of_pain/wait_until_ready";
  }
  virtual void init_base()
  {
    warlock_pet_t::init_base();

    base_attack_power = -20;

    resource_base[ RESOURCE_HEALTH ] = 1468;
    resource_base[ RESOURCE_MANA   ] = 1559;

    melee = new warlock_pet_melee_t( this, "succubus_melee" );
  }
  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "lash_of_pain" ) return new lash_of_pain_t( this );

    return player_t::create_action( name, options_str );
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
      base_dd_min = base_dd_max = 40;
      direct_power_mod  = 0.2;
      background        = true;
      repeating         = true;
    }

    virtual double execute_time() SC_CONST
    {
      // immolation is an aura that ticks every 2 seconds
      return 2.0;
    }

    virtual void player_buff()
    {
      // immolation uses the master's spell power, not the infernal's
      warlock_pet_t* p = ( warlock_pet_t* ) player -> cast_pet();
      warlock_t* o = p -> owner -> cast_warlock();
      warlock_pet_spell_t::player_buff();
      player_spell_power = o -> composite_spell_power( school );
    }
  };

  infernal_immolation_t* immolation;

  infernal_pet_t( sim_t* sim, player_t* owner ) :
      warlock_pet_t( sim, owner, "infernal", PET_INFERNAL )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 412.5;
    main_hand_weapon.max_dmg    = 412.5;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
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
  virtual void schedule_ready( double delta_time=0,
                               bool   waiting=false )
  {
    melee -> schedule_execute();
    immolation -> schedule_execute();
  }
  virtual double composite_attack_hit() SC_CONST { return 0; }
  virtual double composite_spell_hit()  SC_CONST { return 0; }
};

// ==========================================================================
// Pet Doomguard
// ==========================================================================

struct doomguard_pet_t : public warlock_pet_t
{
  doomguard_pet_t( sim_t* sim, player_t* owner ) :
      warlock_pet_t( sim, owner, "doomguard", PET_DOOMGUARD )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 412.5;
    main_hand_weapon.max_dmg    = 412.5;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = 2.0;

    damage_modifier = 1.98;
  }

  virtual void init_base()
  {
    warlock_pet_t::init_base();

    resource_base[ RESOURCE_HEALTH ] = 18000;
    resource_base[ RESOURCE_MANA   ] = 3000;

    base_attack_power = -20;

    melee = new warlock_pet_melee_t( this, "doomguard_melee" );
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
  // 3-state variable where -1 implies "dont-care"
  int metamorphosis;
  int backdraft;
  int molten_core;
  int pyroclasm;

  warlock_spell_t( const char* n, player_t* player, int s, int t ) :
      spell_t( n, player, RESOURCE_MANA, s, t ),
      metamorphosis( -1 ), backdraft( -1 ), molten_core( -1 ), pyroclasm( -1 )
  {
    warlock_t* p = player -> cast_warlock();
    base_hit += p -> talents.suppression * 0.01;
  }

  // Overridden Methods
  virtual double haste() SC_CONST;
  virtual void   player_buff();
  virtual void   target_debuff( int dmg_type );
  virtual void   execute();
  virtual void   tick();
  virtual void   parse_options( option_t*, const std::string& );
  virtual bool   ready();
};

// trigger_soul_leech =======================================================

static void trigger_soul_leech( spell_t* s )
{
  warlock_t* p = s -> player -> cast_warlock();

  if ( p -> talents.soul_leech )
  {
    if ( p -> rng_soul_leech -> roll( 0.10 * p -> talents.soul_leech ) )
    {
      p -> resource_gain( RESOURCE_HEALTH, s -> direct_dmg * 0.20 );

      if ( p -> talents.improved_soul_leech )
      {
        double amount = p -> resource_max[ RESOURCE_MANA ] * p -> talents.improved_soul_leech * 0.01;

        p -> resource_gain( RESOURCE_MANA, amount, p -> gains_soul_leech );
        if ( p -> talents.mana_feed ) p -> active_pet -> resource_gain( RESOURCE_MANA, amount );

        if ( p -> rng_improved_soul_leech -> roll( 0.5 * p -> talents.improved_soul_leech ) )
        {
          p -> trigger_replenishment();
        }
      }
    }
  }
}


// trigger_molten_core =====================================================

static void trigger_molten_core( spell_t* s )
{
  if ( s -> school != SCHOOL_SHADOW ) return;
  warlock_t* p = s -> player -> cast_warlock();
  p -> buffs_molten_core -> trigger(3);
}

// queue_decimation =======================================================

static void queue_decimation( warlock_spell_t* s )
{
  warlock_t* p = s -> player -> cast_warlock();
  if ( s -> time_to_travel )
  {
    p -> decimation_queue.push( s -> sim -> current_time + s -> time_to_travel );
  }
}

// trigger_decimation =====================================================

static void trigger_decimation( warlock_spell_t* s,
                                int result )
{
  warlock_t* p = s -> player -> cast_warlock();
  p -> decimation_queue.pop();
  if ( ( result !=  RESULT_HIT ) && ( result != RESULT_CRIT ) ) return;
  if ( s -> sim -> target -> health_percentage() > 35  ) return;
  p -> buffs_decimation -> trigger();
}


// trigger_deaths_embrace ===================================================

static int trigger_deaths_embrace( spell_t* s )
{
  warlock_t* p = s -> player -> cast_warlock();

  if ( ! p -> talents.deaths_embrace ) return 0;

  if ( s -> sim -> target -> health_percentage() < 35 )
  {
    return p -> talents.deaths_embrace * 4;
  }

  return 0;
}

// trigger_everlasting_affliction ===========================================

static void trigger_everlasting_affliction( spell_t* s )
{
  warlock_t* p = s -> player -> cast_warlock();

  if ( ! p -> talents.everlasting_affliction ) return;

  if ( ! p -> active_corruption ) return;

  if ( p -> rng_everlasting_affliction -> roll( p -> talents.everlasting_affliction * 0.20 ) )
  {
    p -> active_corruption -> snapshot_haste = p -> composite_spell_haste();
    p -> active_corruption -> refresh_duration();
  }
}

// ==========================================================================
// Warlock Spell
// ==========================================================================

// warlock_spell_t::haste ===================================================

double warlock_spell_t::haste() SC_CONST
{
  warlock_t* p = player -> cast_warlock();
  double h = spell_t::haste();
  if ( p -> buffs_eradication -> up() )
  {
    double ranks[] = { 0.0, 0.06, 0.12, 0.20 };
    assert( p -> talents.eradication <= 3 );
    h *= 1.0 / ( 1.0 + ranks[ p -> talents.eradication ] );
  }
  if ( tree == TREE_DESTRUCTION && p -> buffs_backdraft -> up() )
  {
    h *= 1.0 - p -> talents.backdraft * 0.10;
  }
  return h;
}

// warlock_spell_t::player_buff =============================================

void warlock_spell_t::player_buff()
{
  warlock_t* p = player -> cast_warlock();

  spell_t::player_buff();

  if ( p -> buffs_metamorphosis -> up() )
  {
    player_multiplier *= 1.20;
  }

  player_multiplier *= 1.0 + ( p -> talents.demonic_pact * 0.02 );

  if ( p -> buffs_tier10_4pc_caster -> up() )
  {
    player_multiplier *= 1.10;
  }

  if ( p -> talents.malediction ) player_multiplier *= 1.0 + p -> talents.malediction * 0.01;

  if ( school == SCHOOL_SHADOW )
  {
    player_multiplier *= 1.0 + trigger_deaths_embrace( this ) * 0.01;

    if ( p -> buffs_pyroclasm -> up() ) player_multiplier *= 1.0 + p -> talents.pyroclasm * 0.02;
  }
  else if ( school == SCHOOL_FIRE )
  {
    if ( p -> buffs_pyroclasm   -> up() ) player_multiplier *= 1.0 + p -> talents.pyroclasm * 0.02;
  }

  if ( p -> active_pet )
  {
    if ( p -> talents.master_demonologist )
    {
      if ( p -> active_pet -> pet_type == PET_FELGUARD )
      {
        player_multiplier *= 1.0 + p -> talents.master_demonologist * 0.01;
      }
      else if ( p -> active_pet -> pet_type == PET_IMP )
      {
        if ( school == SCHOOL_FIRE )
        {
          player_multiplier *= 1.0 + p -> talents.master_demonologist * 0.01;
          player_crit       +=       p -> talents.master_demonologist * 0.01;
        }
      }
      else if ( p -> active_pet -> pet_type == PET_SUCCUBUS )
      {
        if ( school == SCHOOL_SHADOW )
        {
          player_multiplier *= 1.0 + p -> talents.master_demonologist * 0.01;
          player_crit       +=       p -> talents.master_demonologist * 0.01;
        }
      }
    }

    if ( may_crit )
    {
      if ( p -> buffs_empowered_imp -> up() )
      {
        player_crit += 1.00;
      }
    }
  }
}

// warlock_spell_t::target_debuff ============================================

void warlock_spell_t::target_debuff( int dmg_type )
{
  warlock_t* p = player -> cast_warlock();

  spell_t::target_debuff( dmg_type );

  double stone_bonus = 0;

  if ( dmg_type == DMG_OVER_TIME )
  {
    if ( school == SCHOOL_SHADOW )
    {
      if ( p -> talents.shadow_embrace )
      {
        target_multiplier *= 1.0 + p -> buffs_shadow_embrace -> stack() * p -> talents.shadow_embrace * 0.01;
      }
      if ( p -> buffs_haunted -> up() )
      {
        target_multiplier *= 1.20 + ( p -> glyphs.haunt ? 0.03 : 0.00 );
      }
    }

    if ( p -> main_hand_weapon.buff_type == SPELL_STONE )
    {
      stone_bonus = 0.01;
    }
  }
  else
  {
    if ( p -> main_hand_weapon.buff_type == FIRE_STONE )
    {
      stone_bonus = 0.01;
    }
  }
  if ( stone_bonus > 0 )
  {
    // HACK: Fire/spell stone is additive with Emberstorm and Shadow Mastery
    if ( school == SCHOOL_FIRE )
    {
      target_multiplier /= 1 + p -> talents.emberstorm * 0.03;
      target_multiplier *= 1 + p -> talents.emberstorm * 0.03 + stone_bonus;
    }
    else if ( school == SCHOOL_SHADOW )
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

  if ( time_to_execute > 0 && tree == TREE_DESTRUCTION )
  {
    p -> buffs_backdraft -> decrement();
  }

  spell_t::execute();

  if ( may_crit ) p -> buffs_empowered_imp -> expire();
}

// warlock_spell_t::tick =====================================================

void warlock_spell_t::tick()
{
  spell_t::tick();
}

// warlock_spell_t::parse_options =============================================

void warlock_spell_t::parse_options( option_t*          options,
                                     const std::string& options_str )
{
  option_t base_options[] =
  {
    { "metamorphosis",  OPT_INT,  &metamorphosis  },
    { "backdraft",      OPT_BOOL, &backdraft      },
    { "molten_core",    OPT_BOOL, &molten_core    },
    { "pyroclasm",      OPT_BOOL, &pyroclasm      },
    { NULL, OPT_UNKNOWN, NULL }
  };
  std::vector<option_t> merged_options;
  spell_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

// warlock_spell_t::ready =====================================================

bool warlock_spell_t::ready()
{
  warlock_t* p = player -> cast_warlock();

  if ( ! spell_t::ready() )
    return false;

  if ( metamorphosis == 0 )
    if (  p -> buffs_metamorphosis -> check() )
      return false;

  if ( metamorphosis == 1 )
    if (  ! p -> buffs_metamorphosis -> check() )
      return false;

  if ( backdraft == 0 )
    if ( p -> buffs_backdraft -> check() )
      return false;

  if ( backdraft == 1 )
    if ( ! p -> buffs_backdraft -> check() )
      return false;

  if ( molten_core == 1 )
  {
    if ( ! p -> buffs_molten_core -> may_react() )
      return false;

    if ( ! p -> buffs_molten_core -> up() )
      return false;

    if ( p -> buffs_molten_core -> remains_lt( execute_time() ) )
      return false;
  }

  if ( pyroclasm == 1 )
  {
    if ( ! p -> buffs_pyroclasm -> may_react() )
      return false;

    if ( p -> buffs_pyroclasm -> remains_gt( execute_time() ) )
      return false;
  }

  return true;
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
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 67, 3, 0, 0, 0, 260 },
      { 56, 2, 0, 0, 0, 200 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 47865 );

    base_cost *= 1.0 - p -> talents.suppression * 0.02;

    if ( p -> talents.amplify_curse ) trigger_gcd = 1.0;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    if ( result_is_hit() )
    {
      target_t*  t = sim -> target;
      warlock_t* p = player -> cast_warlock();
      t -> debuffs.curse_of_elements -> expire();
      t -> debuffs.curse_of_elements -> trigger( 1, 13 );
      t -> debuffs.curse_of_elements -> player = p;
      p -> active_curse = this;
    }
  }

  virtual bool ready()
  {
    target_t*  t = sim -> target;
    warlock_t* p = player -> cast_warlock();

    if ( p -> active_curse )
      return false;

    if ( t -> debuffs.curse_of_elements -> check() )
      return false;

    if ( t -> debuffs.earth_and_moon -> current_value >= 13 )
      return false;

    return warlock_spell_t::ready();
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
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 79, 9, 0, 0, 145, 0.10 },
      { 73, 8, 0, 0, 120, 0.10 },
      { 67, 7, 0, 0, 113, 265  },
      { 58, 6, 0, 0,  87, 215  },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 47864 );

    base_execute_time = 0;
    base_tick_time    = 2.0;
    num_ticks         = 12;
    tick_power_mod    = base_tick_time / 15.0;
    tick_power_mod    = 1.20 / num_ticks;  // Nerf Bat!

    base_cost       *= 1.0 - p -> talents.suppression * 0.02;
    base_multiplier *= 1.0 + ( p -> talents.shadow_mastery          * 0.03 +
                               p -> talents.contagion               * 0.01 +
                               p -> talents.improved_curse_of_agony * 0.05 );

    if ( p -> talents.amplify_curse ) trigger_gcd = 1.0;

    if ( p -> glyphs.curse_of_agony )
    {
      num_ticks += 2;
      // after patch 3.0.8, the added ticks are double the base damage
      base_td_init = ( base_td_init * 12 + base_td_init * 4 ) / 14;
    }

    observer = &( p -> active_curse_of_agony );
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();
    if ( result_is_hit() ) p -> active_curse = this;
  }

  virtual void last_tick()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::last_tick();
    p -> active_curse = 0;
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();
    if ( p -> active_curse )
      return false;
    return warlock_spell_t::ready();
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
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 3, 0, 0, 7300, 0.15 },
      { 70, 2, 0, 0, 4200, 380  },
      { 60, 1, 0, 0, 3200, 300  },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 47867 );

    base_execute_time = 0;
    base_tick_time    = 60.0;
    num_ticks         = 1;
    tick_power_mod    = 2.0;
    base_cost        *= 1.0 - p -> talents.suppression * 0.02;
    base_multiplier  *= 1.0 + ( p -> talents.shadow_mastery          * 0.03);

    if ( p -> talents.amplify_curse ) trigger_gcd = 1.0;

    observer = &( p -> active_curse_of_doom );
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();
    if ( result_is_hit() ) p -> active_curse = this;
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
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();
    if ( p -> active_curse )
      return false;
    return warlock_spell_t::ready();
  }
};

// Shadow Bolt Spell ===========================================================

struct shadow_bolt_t : public warlock_spell_t
{
  int shadow_trance;
  int isb_benefit;
  int isb_trigger;

  shadow_bolt_t( player_t* player, const std::string& options_str ) :
      warlock_spell_t( "shadow_bolt", player, SCHOOL_SHADOW, TREE_DESTRUCTION ),
      shadow_trance( 0 ), isb_benefit( 0 ), isb_trigger( 0 )
  {
    warlock_t* p = player -> cast_warlock();

    travel_speed = 21.0; // set before options to allow override
    option_t options[] =
    {
      { "shadow_trance", OPT_BOOL, &shadow_trance },
      { "isb_benefit",   OPT_BOOL, &isb_benefit   },
      { "isb_trigger",   OPT_BOOL, &isb_trigger   },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 79, 13, 690, 770, 0, 0.17 },
      { 74, 12, 596, 664, 0, 0.17 },
      { 69, 11, 541, 603, 0, 420  },
      { 60, 10, 482, 538, 0, 380  },
      { 60,  9, 455, 507, 0, 370  },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 47809 );

    base_execute_time = 3.0;
    may_crit          = true;
    direct_power_mod  = base_execute_time / 3.5;

    base_cost  *= 1.0 - ( util_t::talent_rank( p -> talents.cataclysm, 3, 0.04, 0.07, 0.10 ) + p -> glyphs.shadow_bolt * 0.10 );

    base_execute_time -=  p -> talents.bane * 0.1;

    base_multiplier *= 1.0 + ( p -> talents.shadow_mastery       * 0.03 +
                               p -> set_bonus.tier6_4pc_caster() * 0.06 +
                               p -> talents.improved_shadow_bolt * 0.02 );

    base_crit += ( p -> talents.devastation           * 0.05 +
                               p -> set_bonus.tier8_4pc_caster()  * 0.05 +
                   p -> set_bonus.tier10_2pc_caster() * 0.05 );

    direct_power_mod  *= 1.0 + p -> talents.shadow_and_flame * 0.04;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.ruin * 0.20;
  }

  virtual double execute_time() SC_CONST
  {
    warlock_t* p = player -> cast_warlock();
    if ( p -> buffs_shadow_trance -> up() ) return 0;
    return warlock_spell_t::execute_time();
  }

  virtual void schedule_execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::schedule_execute();
    p -> buffs_shadow_trance -> expire();
  }

  virtual void execute()
  {
    target_t*  t = sim -> target;
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();
    if ( result_is_hit() )
    {
      p -> buffs_shadow_embrace -> trigger();
      t -> debuffs.improved_shadow_bolt -> trigger( 1, 1.0, p -> talents.improved_shadow_bolt / 5.0 );
      trigger_soul_leech( this );
      trigger_everlasting_affliction( this );
    }
    p -> buffs_tier7_2pc_caster -> expire();
  }

  virtual void schedule_travel()
  {
    warlock_spell_t::schedule_travel();
    queue_decimation( this );
  }

  virtual void travel( int    travel_result,
                       double travel_dmg )
  {
    warlock_spell_t::travel( travel_result, travel_dmg );
    trigger_decimation( this, travel_result );
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::player_buff();
    if ( p -> buffs_tier7_2pc_caster -> up() ) player_crit += 0.10;
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if ( ! warlock_spell_t::ready() )
      return false;

    if ( shadow_trance )
      if ( ! p -> buffs_shadow_trance -> check() )
        return false;

    if ( isb_benefit )
      if ( ! sim -> target -> debuffs.improved_shadow_bolt -> check() )
        return false;

    if ( isb_trigger )
      if ( sim -> target -> debuffs.improved_shadow_bolt -> check() )
        return false;

    return true;
  }
};

// Chaos Bolt Spell ===========================================================

struct chaos_bolt_t : public warlock_spell_t
{

  chaos_bolt_t( player_t* player, const std::string& options_str ) :
      warlock_spell_t( "chaos_bolt", player, SCHOOL_FIRE, TREE_DESTRUCTION )
  {
    warlock_t* p = player -> cast_warlock();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );


    rank_t ranks[] =
    {
      { 80, 4, 1429, 1813, 0, 0.07 },
      { 75, 3, 1217, 1545, 0, 0.07 },
      { 70, 2, 1077, 1367, 0, 0.07 },
      { 60, 1,  837, 1061, 0, 0.09 },
      { 0, 0, 0, 0, 0, 0 }
    };

    init_rank( ranks, 59172 );

    base_execute_time = 2.5;
    direct_power_mod  = base_execute_time / 3.5;
    may_crit          = true;
    may_resist        = false;

    base_cost *= 1.0 - util_t::talent_rank( p -> talents.cataclysm, 3, 0.04, 0.07, 0.10 );

    base_execute_time -=  p -> talents.bane * 0.1;
    base_multiplier   *= 1.0 + p -> talents.emberstorm  * 0.03;
    base_crit         += p -> talents.devastation * 0.05;
    direct_power_mod  *= 1.0 + p -> talents.shadow_and_flame * 0.04;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.ruin * 0.20;

    cooldown -> duration = 12.0 - ( p -> glyphs.chaos_bolt * 2.0 );
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    if ( result_is_hit() )
    {
      trigger_soul_leech( this );
    }
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::player_buff();
    if ( p -> active_immolate )
    {
      player_multiplier *= 1 + 0.02 * p -> talents.fire_and_brimstone;
    }
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
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 78, 6, 790, 790, 0, 0.23 },
      { 73, 5, 670, 670, 0, 0.23 },
      { 68, 4, 519, 519, 0, 600  },
      { 58, 3, 400, 400, 0, 480  },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 47860 );

    base_execute_time = 0;
    may_crit          = true;
    binary            = true;
    direct_power_mod  = ( 1.5 / 3.5 ) / 2.0;

    base_cost       *= 1.0 -  p -> talents.suppression * 0.02;
    base_multiplier *= 1.0 + p -> talents.shadow_mastery * 0.03;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.ruin * 0.20;

    cooldown -> duration = 120;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    if ( result_is_hit() )
    {
      player -> resource_gain( RESOURCE_HEALTH, direct_dmg );
    }
  }

};

// Shadow Burn Spell ===========================================================

struct shadow_burn_t : public warlock_spell_t
{
  shadow_burn_t( player_t* player, const std::string& options_str ) :
      warlock_spell_t( "shadow_burn", player, SCHOOL_SHADOW, TREE_DESTRUCTION )
  {
    warlock_t* p = player -> cast_warlock();
    check_talent( p -> talents.shadow_burn );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 10, 775, 865, 0, 0.20 },
      { 75,  9, 662, 738, 0, 0.20 },
      { 70,  8, 597, 665, 0, 0.20 },
      { 63,  7, 518, 578, 0, 0.27 },
      { 56,  6, 450, 502, 0, 0.27 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 47827 );

    may_crit = true;
    direct_power_mod = ( 1.5 / 3.5 );
    base_cost *= 1.0 - util_t::talent_rank( p -> talents.cataclysm, 3, 0.04, 0.07, 0.10 );
    base_multiplier *= 1.0 + p -> talents.shadow_mastery * 0.03;
    base_crit_bonus_multiplier *= 1.0 + p -> talents.ruin * 0.20;
    cooldown -> duration = 15;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    if ( result_is_hit() )
    {
      trigger_soul_leech( this );
    }
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::player_buff();
    if ( p -> glyphs.shadow_burn )
    {
      if ( sim -> target -> health_percentage() < 35 )
      {
        player_crit += 0.20;
      }
    }
  }
};

// Shadowfury Spell ===========================================================

struct shadowfury_t : public warlock_spell_t
{
  double cast_gcd;

  shadowfury_t( player_t* player, const std::string& options_str ) :
      warlock_spell_t( "shadowfury", player, SCHOOL_FIRE, TREE_DESTRUCTION ), cast_gcd( -1 )
  {
    warlock_t* p = player -> cast_warlock();
    check_talent( p -> talents.shadowfury );

    option_t options[] =
    {
      { "cast_gcd",    OPT_FLT,  &cast_gcd    },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    rank_t ranks[] =
    {
      { 80, 4, 968, 1152, 0, 0.27 },
      { 75, 3, 822,  978, 0, 0.27 },
      { 70, 2, 612,  728, 0, 0.27 },
      { 60, 1, 459,  547, 0, 0.37 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 47847 );

    may_crit = true;
    base_execute_time = 0;
    direct_power_mod  = ( 1.5/3.5 );
    cooldown -> duration = 20;

    // estimate - measured at ~0.6sec, but lag in there too, plus you need to mouse-click
    trigger_gcd = ( cast_gcd >= 0 ) ? cast_gcd : 0.5;

    base_cost *= 1.0 - util_t::talent_rank( p -> talents.cataclysm, 3, 0.04, 0.07, 0.10 );

    base_multiplier   *= 1.0 + p -> talents.emberstorm  * 0.03;
    base_crit         += p -> talents.devastation * 0.05;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.ruin * 0.20;
  }
};


// Corruption Spell ===========================================================

struct corruption_t : public warlock_spell_t
{
  int tier10_4pc;
  bool tier10_4pc_effect;
  corruption_t( player_t* player, const std::string& options_str ) :
      warlock_spell_t( "corruption", player, SCHOOL_SHADOW, TREE_AFFLICTION ), tier10_4pc( 0 ), tier10_4pc_effect( false )
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { "tier10_4pc",    OPT_BOOL,  &tier10_4pc   },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 77, 10, 0, 0, 180, 0.14 },
      { 71,  9, 0, 0, 165, 0.14 },
      { 65,  8, 0, 0, 150, 370  },
      { 60,  7, 0, 0, 137, 340  },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 47813 );

    base_execute_time = 0.0;
    base_tick_time    = 3.0;
    num_ticks         = 6;
    tick_power_mod    = base_tick_time / 15.0;

    base_cost       *= 1.0 - p -> talents.suppression * 0.02;
    base_multiplier *= 1.0 + ( p -> talents.shadow_mastery       * 0.03 +
                               p -> talents.contagion            * 0.01 +
                               p -> talents.improved_corruption  * 0.02 +
                               p -> set_bonus.tier9_4pc_caster() * 0.10 +
                               ( ( p -> talents.siphon_life ) ?  0.05 : 0 ) );
    tick_power_mod  += p -> talents.empowered_corruption * 0.02;
    tick_power_mod  += p -> talents.everlasting_affliction * 0.01;

    if ( p -> talents.pandemic )
    {
      base_crit_bonus_multiplier = 2;
      tick_may_crit = true;
      base_crit += p -> talents.malediction           * 0.03 +
                   p -> set_bonus.tier10_2pc_caster() * 0.05;
    }

    observer = &( p -> active_corruption );
  }

  virtual void execute()
  {
    base_td = base_td_init;
    warlock_spell_t::execute();
    if ( result_is_hit() ) {
      warlock_t* p = player -> cast_warlock();
      tier10_4pc_effect = p -> buffs_tier10_4pc_caster -> up();
      num_ticks = 6;
    }
  }

  virtual int scale_ticks_with_haste() SC_CONST
  {
    warlock_t* p = player -> cast_warlock();

    if ( p -> hasted_corruption == 0 )
      return 0;
  
    return ( ( p -> hasted_corruption > 0 ) || p -> glyphs.quick_decay );
  }

  virtual void tick()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::tick();
    p -> buffs_eradication -> trigger();
    p -> buffs_shadow_trance -> trigger( 1, 1.0, p -> talents.nightfall * 0.02 );
    p -> buffs_shadow_trance -> trigger( 1, 1.0, p -> glyphs.corruption * 0.04 );
    p -> buffs_tier7_2pc_caster -> trigger();
    if ( p -> set_bonus.tier6_2pc_caster() ) p -> resource_gain( RESOURCE_HEALTH, 70 );
    trigger_molten_core( this );
  }

  virtual bool ready()
  {
    if ( tier10_4pc )
    {
      warlock_t* p = player -> cast_warlock();
      if ( ! tier10_4pc_effect && p -> buffs_tier10_4pc_caster -> up() )
      {
        recast = true;
        bool ret = warlock_spell_t::ready();
        recast = false;
        return ret;
      }
    }
    return warlock_spell_t::ready();
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
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 78, 9, 0, 0, 133, 0.17 },
      { 69, 8, 0, 0, 108, 425  },
      { 62, 7, 0, 0,  87, 355  },
      { 54, 6, 0, 0,  71, 300  },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 47857 );

    base_execute_time = 0;
    base_tick_time    = 1.0;
    num_ticks         = 5;
    channeled         = true;
    binary            = true;
    tick_power_mod    = ( base_tick_time / 3.5 ) / 2.0;

    base_cost       *= 1.0 - p -> talents.suppression * 0.02;
    base_multiplier *= 1.0 + p -> talents.shadow_mastery * 0.03;

    observer = &( p -> active_drain_life );
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    if ( result_is_hit() )
    {
      trigger_everlasting_affliction( this );
    }
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();

    warlock_spell_t::player_buff();

    double min_multiplier[] = { 0, 0.03, 0.06 };
    double max_multiplier[] = { 0, 0.09, 0.18 };

    assert( p -> talents.soul_siphon >= 0 &&
            p -> talents.soul_siphon <= 2 );

    double min = min_multiplier[ p -> talents.soul_siphon ];
    double max = max_multiplier[ p -> talents.soul_siphon ];

    double multiplier = p -> affliction_effects() * min;

    if ( multiplier > max ) multiplier = max;

    player_multiplier *= 1.0 + multiplier;
  }

  virtual void tick()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::tick();
    p -> buffs_shadow_trance -> trigger( 1, 1.0, p -> talents.nightfall * 0.02 );
  }
};

// Drain Soul Spell ===========================================================

struct drain_soul_t : public warlock_spell_t
{
  int interrupt;
  int health_multiplier;

  drain_soul_t( player_t* player, const std::string& options_str ) :
      warlock_spell_t( "drain_soul", player, SCHOOL_SHADOW, TREE_AFFLICTION ), interrupt( 0 ), health_multiplier( 0 )
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { "interrupt",  OPT_BOOL, &interrupt },
      { "target_pct", OPT_DEPRECATED, ( void* ) "health_percentage<" },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 77, 6, 0, 0, 142, 0.14 },
      { 67, 5, 0, 0, 124, 360  },
      { 52, 4, 0, 0,  91, 290  },
      { 0, 0, 0, 0, 0, 0 }
    };
    rank_t* rank = init_rank( ranks, 47855 );

    base_execute_time = 0;
    base_tick_time    = 3.0;
    num_ticks         = 5;
    channeled         = true;
    binary            = true;
    tick_power_mod    = ( base_tick_time / 3.5 ) / 2.0;

    base_cost        *= 1.0 - p -> talents.suppression * 0.02;
    base_multiplier  *= 1.0 + p -> talents.shadow_mastery * 0.03;

    health_multiplier = ( rank -> level >= 6 ) ? 1 : 0;

    observer = &( p -> active_drain_soul );
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    if ( result_is_hit() )
    {
      trigger_everlasting_affliction( this );
    }
  }

  virtual void tick()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::tick();
    if ( interrupt && ( current_tick != num_ticks ) )
    {
      // If any spell ahead of DS in the action list is "ready", then cancel the DS channel
      for ( action_t* action = p -> action_list; action != this; action = action -> next )
      {
        if ( action -> background ) continue;
        if ( action -> ready() )
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

    double min_multiplier[] = { 0, 0.03, 0.06 };
    double max_multiplier[] = { 0, 0.09, 0.18 };

    assert( p -> talents.soul_siphon >= 0 &&
            p -> talents.soul_siphon <= 2 );

    double min = min_multiplier[ p -> talents.soul_siphon ];
    double max = max_multiplier[ p -> talents.soul_siphon ];

    double multiplier = p -> affliction_effects() * min;

    if ( multiplier > max ) multiplier = max;

    player_multiplier *= 1.0 + multiplier;

    // FIXME! Hack! Deaths Embrace is additive with Drain Soul "execute".
    // Perhaps it is time to add notion of "execute" into action_t class.

    int de_bonus = trigger_deaths_embrace( this );
    if ( de_bonus ) player_multiplier /= 1.0 + de_bonus * 0.01;

    if ( health_multiplier )
    {
      if ( sim -> target -> health_percentage() < 25 )
      {
        player_multiplier *= 4.0 + de_bonus * 0.01;
      }
    }
  }

};

// Unstable Affliction Spell ======================================================

struct unstable_affliction_t : public warlock_spell_t
{
  unstable_affliction_t( player_t* player, const std::string& options_str ) :
      warlock_spell_t( "unstable_affliction", player, SCHOOL_SHADOW, TREE_AFFLICTION )
  {
    warlock_t* p = player -> cast_warlock();
    check_talent( p -> talents.unstable_affliction );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 5,  0, 0, 230, 0.15 },
      { 75, 4,  0, 0, 197, 0.15 },
      { 70, 3,  0, 0, 175, 400  },
      { 60, 2,  0, 0, 155, 315  },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 47843 );

    base_execute_time = 1.5;
    base_tick_time    = 3.0;
    num_ticks         = 5;
    tick_power_mod    = base_tick_time / 15.0;

    base_cost        *= 1.0 - p -> talents.suppression * 0.02;
    base_multiplier  *= 1.0 + ( p -> talents.shadow_mastery * 0.03 +
                                p -> set_bonus.tier8_2pc_caster() * 0.20 + // FIXME! assuming additive
                                p -> set_bonus.tier9_4pc_caster() * 0.10 +
                                ( ( p -> talents.siphon_life ) ? 0.05 : 0 ) );

    tick_power_mod   += p -> talents.everlasting_affliction * 0.01;

    dot = p -> get_dot( "immo_ua" );

    if ( p -> talents.pandemic )
    {
      base_crit_bonus_multiplier = 2;
      tick_may_crit = true;
      base_crit += p -> talents.malediction * 0.03;
    }

    if ( p -> glyphs.unstable_affliction )
    {
      base_execute_time = 1.3;
      //trigger_gcd     = 1.3; latest research seems to imply it does not affect the gcd
    }

    observer = &( p -> active_unstable_affliction );
  }
  virtual void tick()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::tick();
    if ( p -> set_bonus.tier10_4pc_caster() && tick_dmg > 0 )
    {
      p -> buffs_tier10_4pc_caster -> trigger();
    }
  }
};

// Haunt Spell ==============================================================

struct haunt_t : public warlock_spell_t
{
  int debuff;

  haunt_t( player_t* player, const std::string& options_str ) :
      warlock_spell_t( "haunt", player, SCHOOL_SHADOW, TREE_AFFLICTION ), debuff( 0 )
  {
    warlock_t* p = player -> cast_warlock();
    check_talent( p -> talents.haunt );

    option_t options[] =
    {
      { "debuff", OPT_BOOL, &debuff     },
      { "only_for_debuff", OPT_DEPRECATED, ( void* ) "debuff" },
      { NULL, OPT_UNKNOWN, NULL }
    };

    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 4, 645, 753, 0, 0.12 },
      { 75, 3, 550, 642, 0, 0.12 },
      { 70, 2, 487, 569, 0, 0.12 },
      { 60, 1, 405, 473, 0, 0.12 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 59164 );

    may_crit = true;
    base_execute_time = 1.5;
    direct_power_mod = base_execute_time / 3.5;
    cooldown -> duration = 8.0;

    if ( p -> talents.pandemic ) base_crit_bonus_multiplier = 2;

    base_cost        *= 1.0 - p -> talents.suppression * 0.02;
    base_multiplier  *= 1.0 + p -> talents.shadow_mastery * 0.03;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    if ( result_is_hit() )
    {
      warlock_t* p = player -> cast_warlock();
      p -> buffs_haunted -> trigger();
      p -> buffs_shadow_embrace -> trigger();
      trigger_everlasting_affliction( this );
    }
  }

  virtual void player_buff()
  {
    warlock_spell_t::player_buff();
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if ( debuff )
      if ( p -> buffs_haunted -> remains_gt( execute_time() ) )
        return false;
    
    return warlock_spell_t::ready();
  }
};

// Immolate Spell =============================================================

struct immolate_t : public warlock_spell_t
{
  cooldown_t* conflagrate_cooldown;
  double      conflagrate_lag;

  immolate_t( player_t* player, const std::string& options_str ) :
      warlock_spell_t( "immolate", player, SCHOOL_FIRE, TREE_DESTRUCTION ),
      conflagrate_cooldown(0), conflagrate_lag(0.5)
  {
    warlock_t* p = player -> cast_warlock();

    option_t options[] =
    {
      { "conflagrate_lag", OPT_FLT, &conflagrate_lag },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 11, 460, 460, 157, 0.17 },
      { 75, 10, 370, 370, 139, 0.17 },
      { 69,  9, 327, 327, 123, 445  },
      { 60,  8, 279, 279, 102, 380  },
      { 60,  7, 258, 258,  97, 370  },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 47811 );

    base_execute_time = 2.0;
    may_crit          = true;
    base_tick_time    = 3.0;
    num_ticks         = 5;
    direct_power_mod  = 0.20;
    tick_power_mod    = 0.20;
    tick_may_crit = true;

    base_cost *= 1.0 - util_t::talent_rank( p -> talents.cataclysm, 3, 0.04, 0.07, 0.10 );

    base_execute_time -= p -> talents.bane * 0.1;
    base_crit         += p -> talents.devastation * 0.05;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.ruin * 0.20;

    base_dd_multiplier *= 1.0 + ( p -> talents.emberstorm           * 0.03 +
                                  p -> set_bonus.tier8_2pc_caster() * 0.10 +
                                  p -> set_bonus.tier9_4pc_caster() * 0.10 +
                                  p -> talents.improved_immolate    * 0.10 );

    base_td_multiplier *= 1.0 + ( p -> talents.improved_immolate    * 0.10 +
                                  p -> glyphs.immolate              * 0.10 +
                                  p -> talents.aftermath            * 0.03 +
                                  p -> talents.emberstorm           * 0.03 +
                                  p -> set_bonus.tier8_2pc_caster() * 0.10 +
                                  p -> set_bonus.tier9_4pc_caster() * 0.10 );

    num_ticks += p -> talents.molten_core;

    dot = p -> get_dot( "immo_ua" );

    conflagrate_cooldown = p -> get_cooldown( "conflagrate" );

    observer = &( p -> active_immolate );
  }

  virtual void execute()
  {
    base_td = base_td_init;
    warlock_spell_t::execute();
    if ( conflagrate_lag > 0 )
    {
      double travel_ready = sim -> current_time + conflagrate_lag;
      if ( travel_ready > conflagrate_cooldown -> ready )
      {
	conflagrate_cooldown -> ready = travel_ready;
      }
    }
  }

  virtual void tick()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::tick();
    if ( p -> set_bonus.tier10_4pc_caster() && tick_dmg > 0 )
    {
      p -> buffs_tier10_4pc_caster -> trigger();
    }
    p -> buffs_tier7_2pc_caster -> trigger(); 
    if ( p -> set_bonus.tier6_2pc_caster() ) p -> resource_gain( RESOURCE_HEALTH, 70 );
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
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 2, 615, 671, 128, 0.25 },
      { 75, 1, 520, 568, 108, 0.25 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 61290 );

    base_execute_time = 0;
    may_crit          = true;
    base_tick_time    = 2.0;
    num_ticks         = 4;
    direct_power_mod  = 0.106667;
    tick_power_mod    = 0.066667 ;

    cooldown -> duration = 15.0;

    base_cost *= 1.0 - util_t::talent_rank( p -> talents.cataclysm, 3, 0.04, 0.07, 0.10 );
    base_crit += p -> talents.devastation * 0.05;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.ruin * 0.20;

    base_dd_multiplier *= 1.0 + ( p -> talents.shadow_mastery * 0.03 );
    base_td_multiplier *= 1.0 + ( p -> talents.emberstorm * 0.03 );

    observer = &( p -> active_shadowflame );
  }

  virtual void execute()
  {
    base_td = base_td_init;
    // DD is shadow damage, DoT is fire damage
    school = SCHOOL_SHADOW;
    warlock_spell_t::execute();
    if ( result_is_hit() )
    {
      // DD was shadow, now DoT is fire, so reset school
      school = SCHOOL_FIRE;
    }
  }
};

// Conflagrate Spell =========================================================

struct conflagrate_t : public warlock_spell_t
{
  int ticks_lost;

  action_t* dot_spell;
  double immolate_multiplier;
  double shadowflame_multiplier;

  conflagrate_t( player_t* player, const std::string& options_str ) :
    warlock_spell_t( "conflagrate", player, SCHOOL_FIRE, TREE_DESTRUCTION ), 
    ticks_lost( 0 ), dot_spell( 0 ), immolate_multiplier( 1.0 ), shadowflame_multiplier( 1.0 )
  {
    warlock_t* p = player -> cast_warlock();
    check_talent( p -> talents.conflagrate );

    option_t options[] =
    {
      { "ticks_lost", OPT_INT, &ticks_lost },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 40, 1, 0, 0, 0, 0.16 },
      {  0, 0, 0, 0, 0, 0    }
    };
    init_rank( ranks, 17962 );

    may_crit = true;
    base_execute_time = 0;
    direct_power_mod  = ( 1.5/3.5 );

    cooldown -> duration = 10;

    base_cost *= 1.0 - util_t::talent_rank( p -> talents.cataclysm, 3, 0.04, 0.07, 0.10 );

    base_crit += p -> talents.devastation * 0.05 + p -> talents.fire_and_brimstone * 0.05 ;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.ruin * 0.20;

    immolate_multiplier *= 1.0 + ( p -> talents.aftermath            * 0.03 +
                                   p -> talents.improved_immolate    * 0.10 +
                                   p -> glyphs.immolate              * 0.10 +
                                   p -> talents.emberstorm           * 0.03 +
                                   p -> set_bonus.tier9_4pc_caster() * 0.10 +
                                   p -> set_bonus.tier8_2pc_caster() * 0.10 );

    shadowflame_multiplier *=  1.0 + p -> talents.emberstorm * 0.03;

    base_tick_time = 1.0;
    num_ticks      = 3;
    tick_may_crit  = true;
  }

  virtual double calculate_tick_damage()
  {
    warlock_spell_t::calculate_tick_damage();
    warlock_t* p = player -> cast_warlock();

    if ( dot_spell == p -> active_immolate )
    {
      tick_dmg *= immolate_multiplier;
    }
    else
    {
      tick_dmg *= shadowflame_multiplier;
    }

    tick_dmg /= 3;

    // 3.3.2 buff
    tick_dmg *= 2.0;

    return tick_dmg;
  }

  virtual double calculate_direct_damage()
  {
    warlock_spell_t::calculate_direct_damage();
    warlock_t* p = player -> cast_warlock();
    if ( dot_spell == p -> active_immolate )
    {
      direct_dmg *= immolate_multiplier;
    }
    else
    {
      direct_dmg *= shadowflame_multiplier;
    }
    return direct_dmg;
  }

  virtual double total_spell_power() SC_CONST
  {
    return dot_spell -> total_spell_power();
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();

    if      (   p -> active_immolate && ! p -> active_shadowflame ) dot_spell = p -> active_immolate;
    else if ( ! p -> active_immolate &&   p -> active_shadowflame ) dot_spell = p -> active_shadowflame;
    else if ( sim -> rng -> roll( 0.50 ) )                          dot_spell = p -> active_immolate;
    else                                                            dot_spell = p -> active_shadowflame;

    int tick_contribution = 0;
    if( dot_spell == p -> active_immolate )
    {
      tick_contribution = 3;
    }
    else
    {
      tick_contribution = 4;
    }

    base_dd_min      = dot_spell -> base_td_init   * tick_contribution;
    base_dd_max      = dot_spell -> base_td_init   * tick_contribution;
    direct_power_mod = dot_spell -> tick_power_mod * tick_contribution;

    base_td        = dot_spell -> base_td_init;
    tick_power_mod = dot_spell -> tick_power_mod;

    warlock_spell_t::execute();

    if ( result_is_hit() )
    {
      if ( result == RESULT_CRIT ) p -> buffs_pyroclasm -> trigger();
      trigger_soul_leech( this );
      p -> buffs_backdraft -> trigger( 3 );
      if ( ! p -> glyphs.conflagrate )
      {
        dot_spell -> cancel();
      }
    }
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    // If there is neither an active immolate nor shadowflame, then conflag is not ready
    if ( ! ( p -> active_immolate || p -> active_shadowflame ) )
      return false;

    if ( ticks_lost > 0 )
    {
      // The "ticks_lost" checking is a human decision and should not have any RNG.
      // The priority will always be to preserve the Immolate spell if both are up.
      int ticks_remaining = 0;

      if ( p -> active_immolate )
      {
        ticks_remaining = ( p -> active_immolate -> num_ticks -
                            p -> active_immolate -> current_tick );
      }
      else
      {
        ticks_remaining = ( p -> active_shadowflame -> num_ticks -
                            p -> active_shadowflame -> current_tick );
      }

      if ( ticks_remaining > ticks_lost )
        return false;
    }

    return warlock_spell_t::ready();
  }
};

// Incinerate Spell =========================================================

struct incinerate_t : public warlock_spell_t
{
  double immolate_bonus;

  incinerate_t( player_t* player, const std::string& options_str ) :
      warlock_spell_t( "incinerate", player, SCHOOL_FIRE, TREE_DESTRUCTION ), immolate_bonus( 0 )
  {
    warlock_t* p = player -> cast_warlock();

    travel_speed = 21.0; // set before options to allow override

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 4, 582, 676, 0, 0.14 },
      { 74, 3, 485, 563, 0, 0.14 },
      { 70, 2, 429, 497, 0, 300  },
      { 64, 1, 357, 413, 0, 256  },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 47838 );

    base_execute_time  = 2.5;
    may_crit           = true;
    direct_power_mod   = ( 2.5/3.5 );

    base_cost *= 1.0 - util_t::talent_rank( p -> talents.cataclysm, 3, 0.04, 0.07, 0.10 );

    base_execute_time -= p -> talents.emberstorm * 0.05;

    base_multiplier *= 1.0 + ( p -> talents.emberstorm           * 0.03 +
                               p -> set_bonus.tier6_4pc_caster() * 0.06 +
                               p -> glyphs.incinerate            * 0.05 );

    base_crit += ( p -> talents.devastation           * 0.05 +
                               p -> set_bonus.tier8_4pc_caster()  * 0.05 +
                   p -> set_bonus.tier10_2pc_caster() * 0.05 );

    direct_power_mod  *= 1.0 + p -> talents.shadow_and_flame * 0.04;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.ruin * 0.20;

    immolate_bonus = util_t::ability_rank( p -> level,  157,80,  130,74,  120,70,  108,0 );
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    base_dd_adder = p -> active_immolate ? immolate_bonus : 0;
    warlock_spell_t::execute();
    if ( result_is_hit() )
    {
      trigger_soul_leech( this );
    }
    p -> buffs_tier7_2pc_caster -> expire();
  }

  virtual void schedule_travel()
  {
    warlock_spell_t::schedule_travel();
    queue_decimation( this );
  }

  virtual void travel( int    travel_result,
                       double travel_dmg )
  {
    warlock_spell_t::travel( travel_result, travel_dmg );
    trigger_decimation( this, travel_result );
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::player_buff();
    if ( p -> buffs_tier7_2pc_caster -> up() ) player_crit += 0.10;
    if ( p -> buffs_molten_core -> up() ) {
      player_multiplier *= 1 + p -> talents.molten_core * 0.06;
      p -> buffs_molten_core -> decrement();
    }
    if ( p -> active_immolate )
    {
      player_multiplier *= 1 + 0.02 * p -> talents.fire_and_brimstone;
    }
  }

  double haste() SC_CONST
  {
    warlock_t* p = player -> cast_warlock();
    double h = warlock_spell_t::haste();
    if ( p -> buffs_molten_core -> up() )
    {
      h *= 1.0 - p -> talents.molten_core * 0.10;
    }
    return h;
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
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 79, 10, 343, 405, 0, 0.08 },
      { 74,  9, 295, 349, 0, 0.08 },
      { 70,  8, 270, 320, 0, 205  },
      { 65,  7, 243, 287, 0, 191  },
      { 58,  6, 204, 240, 0, 168  },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 47815 );

    base_execute_time = 1.5;
    may_crit          = true;
    direct_power_mod  = base_execute_time / 3.5;

    base_cost *= 1.0 - util_t::talent_rank( p -> talents.cataclysm, 3, 0.04, 0.07, 0.10 );

    base_multiplier *= 1.0 + p -> talents.emberstorm  * 0.03;
    base_crit       += p -> talents.devastation * 0.05;
    base_crit       += p -> talents.improved_searing_pain * 0.03
                       + ( ( p -> talents.improved_searing_pain ) ? 0.01 : 0 );

    base_crit_bonus_multiplier *= 1.0 + ( p -> talents.ruin * 0.20 +
                                          p -> glyphs.searing_pain * 0.20 );
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    warlock_t* p = player -> cast_warlock();
    if ( result_is_hit() )
    {
      if ( result == RESULT_CRIT ) p -> buffs_pyroclasm -> trigger();
      trigger_soul_leech( this );
    }
  }
};

// Soul Fire Spell ============================================================

struct soul_fire_t : public warlock_spell_t
{
  int decimation;

  soul_fire_t( player_t* player, const std::string& options_str ) :
      warlock_spell_t( "soul_fire", player, SCHOOL_FIRE, TREE_DESTRUCTION ), decimation( 0 )
  {
    warlock_t* p = player -> cast_warlock();

    travel_speed = 21.0;

    option_t options[] =
    {
      { "decimation", OPT_BOOL, &decimation },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 6, 1323, 1657, 0, 0.09 },
      { 75, 5, 1137, 1423, 0, 0.09 },
      { 70, 4, 1003, 1257, 0, 455  },
      { 64, 3,  839, 1051, 0, 390  },
      { 56, 2,  703,  881, 0, 335  },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 47825 );

    base_execute_time = 6.0;
    may_crit          = true;
    direct_power_mod  = 1.15;

    base_cost *= 1.0 - util_t::talent_rank( p -> talents.cataclysm, 3, 0.04, 0.07, 0.10 );

    base_execute_time -= p -> talents.bane * 0.4;
    base_multiplier   *= 1.0 + p -> talents.emberstorm  * 0.03;
    base_crit         += p -> talents.devastation           * 0.05 +
                         p -> set_bonus.tier10_2pc_caster() * 0.05;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.ruin * 0.20;
  }

  virtual void execute()
  {
    warlock_spell_t::execute();
    if ( result_is_hit() )
    {
      trigger_soul_leech( this );
    }
  }

  virtual double execute_time() SC_CONST
  {
    warlock_t* p = player -> cast_warlock();
    double t = warlock_spell_t::execute_time();
    if ( p -> buffs_decimation -> up() )
    {
      t *= 1.0 - p -> talents.decimation * 0.20;
      assert( t > 0 );
    }
    return t;
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if ( decimation )
    {
      if ( ! p -> buffs_decimation -> check() )
        return false;
    }

    return warlock_spell_t::ready();
  }

  virtual void schedule_travel()
  {
    warlock_spell_t::schedule_travel();

    queue_decimation( this );
  }

  virtual void travel( int    travel_result,
                       double travel_dmg )
  {
    warlock_spell_t::travel( travel_result, travel_dmg );

    trigger_decimation( this, travel_result );
  }

  virtual void player_buff()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::player_buff();
    if ( p -> buffs_molten_core -> up() )
    {
      player_crit += p -> talents.molten_core * 0.05;
      player_multiplier *= 1 + p -> talents.molten_core * 0.06;
      p -> buffs_molten_core -> decrement();
    }
  }
};

// Life Tap Spell ===========================================================

struct life_tap_t : public warlock_spell_t
{
  int    buff_refresh;
  int    inferno;
  int    prevent_overflow;
  double trigger;
  double max_mana_pct;
  double base_tap;

  life_tap_t( player_t* player, const std::string& options_str ) :
      warlock_spell_t( "life_tap", player, SCHOOL_SHADOW, TREE_AFFLICTION ),
      buff_refresh(0), inferno(0), prevent_overflow(0), trigger(0), max_mana_pct(0), base_tap(0)
  {
    id = 57946;

    option_t options[] =
    {
      { "buff_refresh",     OPT_BOOL, &buff_refresh     },
      { "inferno",          OPT_BOOL, &inferno          },
      { "mana_percentage<", OPT_FLT,  &max_mana_pct     },
      { "prevent_overflow", OPT_BOOL, &prevent_overflow },
      { "trigger",          OPT_FLT,  &trigger          },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful = false;

    // FIXME!!! Need to test new base mana at levels below 80
    base_tap = util_t::ability_rank( player -> level,  2000,80,  710,68,  500,0 );

    direct_power_mod = 0.5;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> procs_life_tap -> occur();
    double mana = ( base_tap + ( direct_power_mod * p -> composite_spell_power( SCHOOL_SHADOW ) ) );
    p -> resource_loss( RESOURCE_HEALTH, mana );
    mana *= ( 1.0 + p -> talents.improved_life_tap * 0.10 );
    p -> resource_gain( RESOURCE_MANA, mana, p -> gains_life_tap );
    if ( p -> talents.mana_feed ) p -> active_pet -> resource_gain( RESOURCE_MANA, mana );
    p -> buffs_tier7_4pc_caster -> trigger();
	p -> buffs_life_tap_glyph -> trigger();
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if (  max_mana_pct > 0 ) 
      if( ( 100.0 * p -> resource_current[ RESOURCE_MANA ] / p -> resource_max[ RESOURCE_MANA ] ) > max_mana_pct )
        return false;

    if ( buff_refresh )
    {
      if( ! p -> set_bonus.tier7_4pc_caster() && ! p -> glyphs.life_tap )
        return false;

      if ( p -> buffs_tier7_4pc_caster -> check() || p -> buffs_life_tap_glyph -> check() )
        return false;
    }

    if ( prevent_overflow )
    {
      double max_current = ( p -> resource_max[ RESOURCE_MANA ] - ( base_tap + 3.0 * p -> spirit() ) * ( 1.0 + p -> talents.improved_life_tap * 0.10 ) );

      if ( p -> resource_current[ RESOURCE_MANA ] > max_current )
        return false;
    }

    if ( trigger > 0 )
      if ( p -> resource_current[ RESOURCE_MANA ] > trigger )
        return false;

    // skip if infernal out and over 80% mana?
    if ( inferno )
    {
      if ( p -> active_pet == 0 ||
           p -> active_pet -> pet_type != PET_INFERNAL )
      {
        if  ( p -> resource_current[ RESOURCE_MANA ] >
              p -> resource_base   [ RESOURCE_MANA ] * 0.80 )
          return false;
      }
    }

    return warlock_spell_t::ready();
  }
};

// Dark Pact Spell =========================================================

struct dark_pact_t : public warlock_spell_t
{
  int    buff_refresh;
  double trigger;
  double max_mana_pct;
  double base_tap;

  dark_pact_t( player_t* player, const std::string& options_str ) :
    warlock_spell_t( "dark_pact", player, SCHOOL_SHADOW, TREE_AFFLICTION ),
    buff_refresh(0), trigger(0), max_mana_pct(0)
  {
    warlock_t* p = player -> cast_warlock();
    check_talent( p -> talents.dark_pact );

    option_t options[] =
    {
      { "buff_refresh",     OPT_BOOL, &buff_refresh     },
      { "mana_percentage<", OPT_FLT,  &max_mana_pct     },
      { "trigger",          OPT_FLT,  &trigger          },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 5, 1200, 1200, 0, 0 },
      { 70, 3,  700,  700, 0, 0 },
      { 60, 2,  545,  545, 0, 0 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 59092 );

    harmful = false;

    base_execute_time = 0.0;
    direct_power_mod  = 0.96;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> procs_dark_pact -> occur();
    p -> buffs_life_tap_glyph -> trigger();
    double mana = base_dd_max + ( direct_power_mod * p -> composite_spell_power( SCHOOL_SHADOW ) );
    p -> resource_gain( RESOURCE_MANA, mana, p -> gains_dark_pact );
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if (  max_mana_pct > 0 ) 
      if( ( 100.0 * p -> resource_current[ RESOURCE_MANA ] / p -> resource_max[ RESOURCE_MANA ] ) > max_mana_pct )
        return false;

    if ( buff_refresh )
    {
      if( ! p -> glyphs.life_tap )
        return false;

      if ( p -> buffs_life_tap_glyph -> check() )
        return false;
    }

    if ( trigger > 0 )
      if ( p -> resource_current[ RESOURCE_MANA ] > trigger )
        return false;

    return warlock_spell_t::ready();
  }
};

// Fel Armor Spell ==========================================================

struct fel_armor_t : public warlock_spell_t
{
  double bonus_spell_power;

  fel_armor_t( player_t* player, const std::string& options_str ) :
      warlock_spell_t( "fel_armor", player, SCHOOL_SHADOW, TREE_DEMONOLOGY ), bonus_spell_power( 0 )
  {
    warlock_t* p = player -> cast_warlock();

    harmful = false;
    trigger_gcd = 0;
    bonus_spell_power = util_t::ability_rank( p -> level,  180.0,78,  150.0,73,  100.0,67,  50.0,62,  0.0,0 );
    bonus_spell_power *= 1.0 + p -> talents.demonic_aegis * 0.10;

    // Model the passive health tick.....
    base_tick_time = 5.0;
    num_ticks      = 1;

    id = 47893;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();

    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );

    p -> buffs_fel_armor -> start( 1, bonus_spell_power );
    p -> spell_power_per_spirit += 0.30 * ( 1.0 + p -> talents.demonic_aegis * 0.10 );

    schedule_tick();
  }

  virtual void tick()
  {
    warlock_t* p = player -> cast_warlock();
    current_tick = 0; // ticks indefinitely
    p -> resource_gain( RESOURCE_HEALTH, 0, p -> gains_fel_armor, this );
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();
    return ! p -> buffs_fel_armor -> check();
  }
};

// Summon Pet Spell ==========================================================

struct summon_pet_t : public warlock_spell_t
{
  std::string pet_name;

  summon_pet_t( player_t* player, const std::string& options_str ) :
      warlock_spell_t( "summon_pet", player, SCHOOL_SHADOW, TREE_DEMONOLOGY )
  {
    warlock_t* p = player -> cast_warlock();
    pet_name = ( options_str.size() > 0 ) ? options_str : p -> summon_pet_str;
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
    if ( p -> active_pet ) return false;
    if ( sim -> current_time > 10.0 ) return false;
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
      { "target_pct",     OPT_DEPRECATED, ( void* ) "health_percentage<" },
      { "time_remaining", OPT_DEPRECATED, ( void* ) "time_to_die<"       },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( max_health_percentage == 0 &&
         max_time_to_die       == 0 )
    {
      max_time_to_die = 60;
    }

    static rank_t ranks[] =
    {
      { 50, 1, 200, 200, 0, 0.80 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 1122 );

    may_crit = true;
    base_execute_time = 1.5;
    direct_power_mod = 1;
    cooldown -> duration = 20 * 60;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    warlock_spell_t::execute();
    if ( p -> active_pet ) p -> active_pet -> dismiss();
    p -> summon_pet( "infernal", 60.0 );
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
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    warlock_t* p   = player -> cast_warlock();
    base_cost      = 0.64 * p -> resource_base[ RESOURCE_MANA ];
    base_td_init   = 481;
    base_tick_time = 1.0;
    num_ticks      = 15;
    tick_power_mod = 0.143;
    cooldown -> duration = 30;
    metamorphosis = 1;

    id = 50589;
  }

  virtual double tick_time() SC_CONST
  {
    return base_tick_time * haste();
  }

  virtual void tick()
  {
    warlock_t* p = player -> cast_warlock();
    if ( p -> buffs_metamorphosis -> check() )
      warlock_spell_t::tick();
    else
      current_tick = num_ticks;
  }
};

// Metamorphosis Spell =======================================================

struct metamorphosis_t : public warlock_spell_t
{
  metamorphosis_t( player_t* player, const std::string& options_str ) :
      warlock_spell_t( "metamorphosis", player, SCHOOL_SHADOW, TREE_DEMONOLOGY )
  {
    warlock_t* p = player -> cast_warlock();
    check_talent( p -> talents.metamorphosis );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful = false;
    base_cost   = 0;
    trigger_gcd = 0;

    cooldown -> duration = 180 * ( 1.0 - p -> talents.nemesis * 0.1 );

    id = 59672;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    p -> buffs_metamorphosis -> start();
  }
};

// Demonic Empowerment Spell ================================================

struct demonic_empowerment_t : public warlock_spell_t
{
  int demonic_frenzy;

  demonic_empowerment_t( player_t* player, const std::string& options_str ) :
      warlock_spell_t( "demonic_empowerment", player, SCHOOL_SHADOW, TREE_DEMONOLOGY ), demonic_frenzy( 0 )
  {
    warlock_t* p = player -> cast_warlock();
    check_talent( p -> talents.demonic_empowerment );

    option_t options[] =
    {
      { "demonic_frenzy", OPT_INT, &demonic_frenzy },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful = false;
    base_cost = player -> resource_base[ RESOURCE_MANA ] * 0.06;
    cooldown -> duration  = 60 * ( 1.0 - p -> talents.nemesis * 0.1 );
    trigger_gcd = 0;

    id = 47193;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    if( p -> active_pet -> pet_type == PET_FELGUARD )
    {
      p -> buffs_demonic_empowerment -> duration = 15.0;
      p -> buffs_demonic_empowerment -> trigger();
    }
    else if( p -> active_pet -> pet_type == PET_IMP )
    {
      p -> buffs_demonic_empowerment -> duration = 30.0;
      p -> buffs_demonic_empowerment -> trigger();
    }
    else assert( false );
  }

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if ( ! p -> active_pet )
      return false;

    if( p -> active_pet -> pet_type == PET_FELGUARD )
    {
      if ( demonic_frenzy )
        if ( p -> buffs_demonic_frenzy -> current_stack < demonic_frenzy ) 
          return false;
    }
    else if( p -> active_pet -> pet_type == PET_IMP )
    {
    }
    else return false;

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

    bonus_crit = ( int ) util_t::ability_rank( p -> level,  49,80,  42,74,  35,66,  28,0 );

    bonus_crit = ( int ) ( bonus_crit * ( 1.0 + p -> talents.master_conjuror * 1.50 ) );

    id = 55158;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    player -> main_hand_weapon.buff_type = FIRE_STONE;
    player -> spell_crit += bonus_crit / player -> rating.spell_crit;
  };

  virtual bool ready()
  {
    return( player -> main_hand_weapon.buff_type != FIRE_STONE );
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

    bonus_haste = ( int ) ( bonus_haste * ( 1.0 + p -> talents.master_conjuror * 1.50 ) );

    id = 55194;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    player -> main_hand_weapon.buff_type = SPELL_STONE;
    player -> haste_rating += bonus_haste;
    player -> recalculate_haste();
  };

  virtual bool ready()
  {
    return( player -> main_hand_weapon.buff_type != SPELL_STONE );
  }
};

// Wait For Decimation =========================================================

struct wait_for_decimation_t : public action_t
{
  double time;

  wait_for_decimation_t( player_t* player, const std::string& options_str ) :
      action_t( ACTION_OTHER, "wait_for_decimation", player ), time( 0.25 )
  {
    warlock_t* p = player -> cast_warlock();
    check_talent( p -> talents.decimation );

    option_t options[] =
    {
      { "time", OPT_FLT, &time },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
  }

  virtual void execute()
  {
    warlock_t* p = player -> cast_warlock();

    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );

    trigger_gcd = p -> decimation_queue.front() - sim -> current_time;

    assert( trigger_gcd > 0 && trigger_gcd <= time );
  };

  virtual bool ready()
  {
    warlock_t* p = player -> cast_warlock();

    if ( ! p->talents.decimation ) return false;

    if ( sim -> target -> health_percentage() <= 35 )
      if ( ! p -> buffs_decimation -> check() )
        if ( ! p -> decimation_queue.empty() )
          if ( ( p -> decimation_queue.front() - sim -> current_time ) <= time )
            return true;

    return false;
  }
};

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Warlock Pet Events
// ==========================================================================

// demonic_pact_callback ===================================================

struct demonic_pact_callback_t : public action_callback_t
{
  double cooldown_ready;

  demonic_pact_callback_t( player_t* p ) : action_callback_t( p -> sim, p ), cooldown_ready(0) {}

  virtual void reset() { action_callback_t::reset(); cooldown_ready=0; }

  virtual void trigger( action_t* a )
  {
    if ( sim -> current_time < cooldown_ready ) return;
    cooldown_ready = sim -> current_time + 20;

    warlock_t* o = listener -> cast_pet() -> owner -> cast_warlock();

    // HACK ALERT!!! Remove "double-dip" during crit scale factor generation.
    if ( sim -> scaling -> scale_stat == STAT_CRIT_RATING && o -> talents.improved_demonic_tactics )
    {
      if ( ! sim -> rng -> roll( a -> player_crit / ( a -> player_crit + sim -> scaling -> scale_value * o -> talents.improved_demonic_tactics * 0.10 / o -> rating.spell_crit ) ) ) 
	return;
    }

    // HACK ALERT!!! Remove "double-dip" during int scale factor generation.
    if ( sim -> scaling -> scale_stat == STAT_INTELLECT && o -> talents.improved_demonic_tactics )
    {
      if ( ! sim -> rng -> roll( a -> player_crit / ( a -> player_crit + sim -> scaling -> scale_value * o -> spell_crit_per_intellect * o -> talents.improved_demonic_tactics * 0.10 ) ) ) 
	return;
    }

    // HACK ALERT!!!  To prevent spell power contributions from ToW/FT/IDS/DP buffs, we fiddle with player type
    o -> type = PLAYER_GUARDIAN;
    double buff = o -> composite_spell_power( SCHOOL_MAX ) * 0.10;
    o -> type = WARLOCK;

    for( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if( p -> sleeping ) continue;

      p -> buffs.demonic_pact -> trigger( 1, buff );

      // HACK ALERT!!! Remove "double-dip" during spell power scale factor generation.
      if ( p != o && sim -> scaling -> scale_stat == STAT_SPELL_POWER )
      {
        p -> buffs.demonic_pact -> current_value -= sim -> scaling -> scale_value * 0.10;
      }
	  // HACK ALERT!!! Remove "double-dip" during spirit scale factor generation.
      if ( p != o && sim -> scaling -> scale_stat == STAT_SPIRIT )
      {
        p -> buffs.demonic_pact -> current_value -= sim -> scaling -> scale_value * o -> spell_power_per_spirit * 0.10;
		if ( o -> buffs_life_tap_glyph -> up() ) p -> buffs.demonic_pact -> current_value -= sim -> scaling -> scale_value * 0.20 * 0.10;
      }
	  // HACK ALERT!!! Remove "double-dip" during intellect scale factor generation.
      if ( p != o && sim -> scaling -> scale_stat == STAT_INTELLECT )
      {
        p -> buffs.demonic_pact -> current_value -= sim -> scaling -> scale_value * o -> talents.demonic_knowledge * 0.04 * 0.10;
      }
	  // HACK ALERT!!! Remove "double-dip" during stamina scale factor generation.
      if ( p != o && sim -> scaling -> scale_stat == STAT_STAMINA )
      {
        p -> buffs.demonic_pact -> current_value -= sim -> scaling -> scale_value * o -> talents.demonic_knowledge * 0.04 * 0.10;
      }
    }
  }
};

// warlock_pet_t::register_callbacks ========================================

void warlock_pet_t::register_callbacks()
{
  warlock_t* o = owner -> cast_warlock();

  pet_t::register_callbacks();

  if ( o -> talents.demonic_pact )
  {
    action_callback_t* cb = new demonic_pact_callback_t( this );

    register_attack_result_callback( RESULT_CRIT_MASK, cb );
    register_spell_result_callback ( RESULT_CRIT_MASK, cb );
  }
}

// imp_pet_t::fire_bolt_t::execute ==========================================

void imp_pet_t::fire_bolt_t::execute()
{
  warlock_pet_spell_t::execute();

  if ( result == RESULT_CRIT )
  {
    warlock_t* p = player -> cast_pet() -> owner -> cast_warlock();
    p -> buffs_empowered_imp -> trigger();
  }
}

// ==========================================================================
// Warlock Character Definition
// ==========================================================================

// warlock_t::composite_spell_power =========================================

double warlock_t::composite_spell_power( int school ) SC_CONST
{
  double sp = player_t::composite_spell_power( school );

  sp += buffs_fel_armor -> value();

  if ( buffs_life_tap_glyph -> up() )
  {
	  sp += player_t::spirit() * 0.20;
  }

  if ( active_pet && talents.demonic_knowledge )
  {
    if ( active_pet -> pet_type != PET_INFERNAL &&
         active_pet -> pet_type != PET_DOOMGUARD )
    {
      sp += ( active_pet -> stamina() +
              active_pet -> intellect() ) * talents.demonic_knowledge * 0.04;
    }
  }

  return sp;
}

// warlock_t::create_action =================================================

action_t* warlock_t::create_action( const std::string& name,
                                    const std::string& options_str )
{
  if ( name == "chaos_bolt"          ) return new          chaos_bolt_t( this, options_str );
  if ( name == "conflagrate"         ) return new         conflagrate_t( this, options_str );
  if ( name == "corruption"          ) return new          corruption_t( this, options_str );
  if ( name == "curse_of_agony"      ) return new      curse_of_agony_t( this, options_str );
  if ( name == "curse_of_doom"       ) return new       curse_of_doom_t( this, options_str );
  if ( name == "curse_of_elements"   ) return new   curse_of_elements_t( this, options_str );
  if ( name == "dark_pact"           ) return new           dark_pact_t( this, options_str );
  if ( name == "death_coil"          ) return new          death_coil_t( this, options_str );
  if ( name == "demonic_empowerment" ) return new demonic_empowerment_t( this, options_str );
  if ( name == "drain_life"          ) return new          drain_life_t( this, options_str );
  if ( name == "drain_soul"          ) return new          drain_soul_t( this, options_str );
  if ( name == "fel_armor"           ) return new           fel_armor_t( this, options_str );
  if ( name == "fire_stone"          ) return new          fire_stone_t( this, options_str );
  if ( name == "spell_stone"         ) return new         spell_stone_t( this, options_str );
  if ( name == "haunt"               ) return new               haunt_t( this, options_str );
  if ( name == "immolate"            ) return new            immolate_t( this, options_str );
  if ( name == "immolation"          ) return new          immolation_t( this, options_str );
  if ( name == "shadowflame"         ) return new         shadowflame_t( this, options_str );
  if ( name == "incinerate"          ) return new          incinerate_t( this, options_str );
  if ( name == "inferno"             ) return new             inferno_t( this, options_str );
  if ( name == "life_tap"            ) return new            life_tap_t( this, options_str );
  if ( name == "metamorphosis"       ) return new       metamorphosis_t( this, options_str );
  if ( name == "shadow_bolt"         ) return new         shadow_bolt_t( this, options_str );
  if ( name == "shadow_burn"         ) return new         shadow_burn_t( this, options_str );
  if ( name == "shadowfury"          ) return new          shadowfury_t( this, options_str );
  if ( name == "searing_pain"        ) return new        searing_pain_t( this, options_str );
  if ( name == "soul_fire"           ) return new           soul_fire_t( this, options_str );
  if ( name == "summon_pet"          ) return new          summon_pet_t( this, options_str );
  if ( name == "unstable_affliction" ) return new unstable_affliction_t( this, options_str );
  if ( name == "wait_for_decimation" ) return new wait_for_decimation_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// warlock_t::create_pet =====================================================

pet_t* warlock_t::create_pet( const std::string& pet_name )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "felguard"  ) return new  felguard_pet_t( sim, this );
  if ( pet_name == "felhunter" ) return new felhunter_pet_t( sim, this );
  if ( pet_name == "imp"       ) return new       imp_pet_t( sim, this );
  if ( pet_name == "succubus"  ) return new  succubus_pet_t( sim, this );
  if ( pet_name == "infernal"  ) return new  infernal_pet_t( sim, this );
  if ( pet_name == "doomguard" ) return new doomguard_pet_t( sim, this );

  return 0;
}

// warlock_t::create_pets ====================================================

void warlock_t::create_pets()
{
  create_pet( "felguard"  );
  create_pet( "felhunter" );
  create_pet( "imp"       );
  create_pet( "succubus"  );
  create_pet( "infernal"  );
  create_pet( "doomguard" );
}

// warlock_t::init_glyphs =====================================================

void warlock_t::init_glyphs()
{
  memset( ( void* ) &glyphs, 0x0, sizeof( glyphs_t ) );

  std::vector<std::string> glyph_names;
  int num_glyphs = util_t::string_split( glyph_names, glyphs_str, ",/" );

  for ( int i=0; i < num_glyphs; i++ )
  {
    std::string& n = glyph_names[ i ];

    if     ( n == "chaos_bolt"          ) glyphs.chaos_bolt = 1;
    else if ( n == "conflagrate"         ) glyphs.conflagrate = 1;
    else if ( n == "corruption"          ) glyphs.corruption = 1;
    else if ( n == "curse_of_agony"      ) glyphs.curse_of_agony = 1;
    else if ( n == "felguard"            ) glyphs.felguard = 1;
    else if ( n == "felhunter"           ) glyphs.felhunter = 1;
    else if ( n == "haunt"               ) glyphs.haunt = 1;
    else if ( n == "immolate"            ) glyphs.immolate = 1;
    else if ( n == "imp"                 ) glyphs.imp = 1;
    else if ( n == "incinerate"          ) glyphs.incinerate = 1;
    else if ( n == "life_tap"            ) glyphs.life_tap = 1;
    else if ( n == "metamorphosis"       ) glyphs.metamorphosis = 1;
    else if ( n == "searing_pain"        ) glyphs.searing_pain = 1;
    else if ( n == "shadow_bolt"         ) glyphs.shadow_bolt = 1;
    else if ( n == "shadow_burn"         ) glyphs.shadow_burn = 1;
    else if ( n == "siphon_life"         ) glyphs.siphon_life = 1;
    else if ( n == "unstable_affliction" ) glyphs.unstable_affliction = 1;
    else if ( n == "quick_decay"         ) glyphs.quick_decay = 1;
    // minor glyphs, to prevent 'not-found' warning
    else if ( n == "curse_of_exhaustion" ) ;
    else if ( n == "curse_of_exhausion" )  ; // It's mis-spelt on the armory.
    else if ( n == "drain_soul" )          ;
    else if ( n == "enslave_demon" )       ;
    else if ( n == "healthstone" )         ;
    else if ( n == "howl_of_terror")       ;
    else if ( n == "kilrogg" )             ;
    else if ( n == "shadowflame" )         ;
    else if ( n == "soul_link" )           ;
    else if ( n == "souls" )               ;
    else if ( n == "soulstone" )           ;
    else if ( n == "unending_breath" )     ;
    else if ( n == "voidwalker" )          ;
    else if ( ! sim -> parent ) 
    {
      sim -> errorf( "Player %s has unrecognized glyph %s\n", name(), n.c_str() );
    }
  }
}

// warlock_t::init_race ======================================================

void warlock_t::init_race()
{
  race = util_t::parse_race_type( race_str );
  switch ( race )
  {
  case RACE_HUMAN:
  case RACE_GNOME:
  case RACE_UNDEAD:
  case RACE_ORC:
  case RACE_BLOOD_ELF:
    break;
  default:
    race = RACE_UNDEAD;
    race_str = util_t::race_type_string( race );
  }

  player_t::init_race();
}

// warlock_t::init_base ======================================================

void warlock_t::init_base()
{
  attribute_base[ ATTR_STRENGTH  ] = rating_t::get_attribute_base( sim, level, WARLOCK, race, BASE_STAT_STRENGTH );
  attribute_base[ ATTR_AGILITY   ] = rating_t::get_attribute_base( sim, level, WARLOCK, race, BASE_STAT_AGILITY );
  attribute_base[ ATTR_STAMINA   ] = rating_t::get_attribute_base( sim, level, WARLOCK, race, BASE_STAT_STAMINA );
  attribute_base[ ATTR_INTELLECT ] = rating_t::get_attribute_base( sim, level, WARLOCK, race, BASE_STAT_INTELLECT );
  attribute_base[ ATTR_SPIRIT    ] = rating_t::get_attribute_base( sim, level, WARLOCK, race, BASE_STAT_SPIRIT );
  resource_base[ RESOURCE_HEALTH ] = rating_t::get_attribute_base( sim, level, WARLOCK, race, BASE_STAT_HEALTH );
  resource_base[ RESOURCE_MANA   ] = rating_t::get_attribute_base( sim, level, WARLOCK, race, BASE_STAT_MANA );
  base_spell_crit                  = rating_t::get_attribute_base( sim, level, WARLOCK, race, BASE_STAT_SPELL_CRIT );
  base_attack_crit                 = rating_t::get_attribute_base( sim, level, WARLOCK, race, BASE_STAT_MELEE_CRIT );
  initial_spell_crit_per_intellect = rating_t::get_attribute_base( sim, level, WARLOCK, race, BASE_STAT_SPELL_CRIT_PER_INT );
  initial_attack_crit_per_agility  = rating_t::get_attribute_base( sim, level, WARLOCK, race, BASE_STAT_MELEE_CRIT_PER_AGI );

  attribute_multiplier_initial[ ATTR_STAMINA ] *= 1.0 + talents.demonic_embrace * 0.03
                                                  + ( ( talents.demonic_embrace ) ? 0.01 : 0 );

  base_spell_crit+= talents.demonic_tactics * 0.02 + talents.backlash * 0.01;

  base_attack_power = -10;
  initial_attack_power_per_strength = 1.0;

  health_per_stamina = 10;
  mana_per_intellect = 15;

  mana_per_intellect *= 1.0 + ( talents.fel_intellect + talents.fel_vitality ) * 0.01;
  health_per_stamina *= 1.0 + ( talents.fel_stamina   + talents.fel_vitality ) * 0.01;
}

// warlock_t::init_scaling ===================================================

void warlock_t::init_scaling()
{
  player_t::init_scaling();

  if ( talents.demonic_knowledge )
    scales_with[ STAT_STAMINA ] = 1;
  else
    scales_with[ STAT_STAMINA ] = -1; // show 0.00
}

// warlock_t::init_buffs =====================================================

void warlock_t::init_buffs()
{
  player_t::init_buffs();

  buffs_backdraft           = new buff_t( this, "backdraft",           3, 15.0, 0.0, talents.backdraft );
  buffs_decimation          = new buff_t( this, "decimation",          1,  8.0, 0.0, talents.decimation );
  buffs_demonic_empowerment = new buff_t( this, "demonic_empowerment", 1 );
  buffs_demonic_frenzy      = new buff_t( this, "demonic_frenzy",     10, 10.0 );
  buffs_empowered_imp       = new buff_t( this, "empowered_imp",       1,  8.0, 0.0, talents.empowered_imp / 3.0 );
  buffs_eradication         = new buff_t( this, "eradication",         1, 10.0, 0.0, talents.eradication ? 0.06 : 0.00 );
  buffs_fel_armor           = new buff_t( this, "fel_armor"     );
  buffs_haunted             = new buff_t( this, "haunted",             1, 12.0, 0.0, talents.haunt );
  buffs_life_tap_glyph      = new buff_t( this, "life_tap_glyph",      1, 40.0, 0.0, glyphs.life_tap );
  buffs_metamorphosis       = new buff_t( this, "metamorphosis",       1, 30.0 + glyphs.metamorphosis * 6.0, 0.0, talents.metamorphosis );
  buffs_molten_core         = new buff_t( this, "molten_core",         3, 15.0, 0.0, talents.molten_core * 0.04 );
  buffs_pyroclasm           = new buff_t( this, "pyroclasm",           1, 10.0, 0.0, talents.pyroclasm );
  buffs_shadow_embrace      = new buff_t( this, "shadow_embrace",      3, 12.0, 0.0, talents.shadow_embrace );
  buffs_shadow_trance       = new buff_t( this, "shadow_trance",       1,  0.0, 0.0, talents.nightfall );
  buffs_tier10_4pc_caster   = new buff_t( this, "tier10_4pc_caster",   1, 10.0, 0.0, 0.15 ); // Fix-Me: Might need to add an ICD.

  buffs_tier7_2pc_caster = new      buff_t( this, "tier7_2pc_caster",                   1, 10.0, 0.0, set_bonus.tier7_2pc_caster() * 0.15 );
  buffs_tier7_4pc_caster = new stat_buff_t( this, "tier7_4pc_caster", STAT_SPIRIT, 300, 1, 10.0, 0.0, set_bonus.tier7_4pc_caster() );
}

// warlock_t::init_gains =====================================================

void warlock_t::init_gains()
{
  player_t::init_gains();

  gains_dark_pact  = get_gain( "dark_pact"  );
  gains_fel_armor  = get_gain( "fel_armor"  );
  gains_felhunter  = get_gain( "felhunter"  );
  gains_life_tap   = get_gain( "life_tap"   );
  gains_soul_leech = get_gain( "soul_leech" );
}

// warlock_t::init_procs =====================================================

void warlock_t::init_procs()
{
  player_t::init_procs();

  procs_dark_pact = get_proc( "dark_pact" );
  procs_life_tap  = get_proc( "life_tap"  );
}

// warlock_t::init_rng =======================================================

void warlock_t::init_rng()
{
  player_t::init_rng();

  rng_soul_leech             = get_rng( "soul_leech"             );
  rng_improved_soul_leech    = get_rng( "improved_soul_leech"    );
  rng_everlasting_affliction = get_rng( "everlasting_affliction" );
}

// warlock_t::init_actions ===================================================

void warlock_t::init_actions()
{
  if ( action_list_str.empty() )
  {
    std::string tap_str = ( talents.dark_pact ) ? "dark_pact" : "life_tap";
    action_list_str += "flask,type=frost_wyrm/food,type=fish_feast";
    action_list_str += ( talents.master_conjuror || talents.haunt ) ? "/spell_stone" : "/fire_stone";
    action_list_str += "/fel_armor/summon_pet";

    // Choose Pet
    if ( summon_pet_str.empty() )
    {
      if ( talents.summon_felguard )
        summon_pet_str = "felguard";
      else if ( talents.empowered_imp || talents.improved_imp )
        summon_pet_str = "imp";
      else if ( talents.improved_felhunter )
        summon_pet_str = "felhunter";
      else
        summon_pet_str = "succubus";
    }
    action_list_str += "," + summon_pet_str;
    action_list_str += "/snapshot_stats";

    // Refresh Life Tap Buff
    if( set_bonus.tier7_4pc_caster() || glyphs.life_tap )
    {
      action_list_str+="/" + tap_str + ",buff_refresh=1";
    }

    // Trigger 5% spellcrit debuff
    if ( talents.improved_shadow_bolt) action_list_str += "/shadow_bolt,isb_trigger=1";

    // Usable Item
    int num_items = ( int ) items.size();
    for ( int i=0; i < num_items; i++ )
    {
      if ( items[ i ].use.active() )
      {
        action_list_str += "/use_item,name=";
        action_list_str += items[ i ].name();
      }
    }

    // Race Skills
    if ( race == RACE_ORC )
    {
      action_list_str += "/blood_fury";
    }
    else if ( race == RACE_BLOOD_ELF )
    {
      action_list_str += "/arcane_torrent";
    }

    // Choose Potion
    if ( talents.haunt )
    {
    	action_list_str += "/wild_magic_potion,if=!in_combat";
    	action_list_str += "/speed_potion,if=buff.bloodlust.react";
    }
    else
    {
    	action_list_str += "/wild_magic_potion,if=(buff.bloodlust.react)|(!in_combat)";
    }

    // Affliction 41+_xx_xx
    if ( talents.haunt || talents.unstable_affliction )
    {
      if ( talents.haunt ) action_list_str += "/haunt,if=(buff.haunted.remains<3)|(dot.corruption.remains<4)";
      action_list_str += "/corruption,if=!ticking";
      if ( talents.unstable_affliction ) action_list_str += "/unstable_affliction,time_to_die>=5,if=(dot.unstable_affliction.remains<cast_time)";
      if ( talents.haunt ) action_list_str += "/curse_of_agony,time_to_die>=20,if=!ticking";

      if ( talents.soul_siphon ) action_list_str += "/drain_soul,health_percentage<=25,interrupt=1";
    }

    // Destruction 00_13_58
    else if ( talents.chaos_bolt )
    {
      if ( talents.conflagrate ) action_list_str += "/conflagrate";
      action_list_str += "/immolate,time_to_die>=3,if=(dot.immolate.remains<cast_time)";
      action_list_str += "/chaos_bolt";
      action_list_str += "/curse_of_doom,time_to_die>=70";
      action_list_str += "/curse_of_agony,moving=1,if=(!ticking)&!(dot.curse_of_doom.remains>0)";
    }

    // Demonology 00_56_15
    else if ( talents.metamorphosis )
    {
      if ( talents.demonic_empowerment ) action_list_str += "/demonic_empowerment";

      action_list_str += "/metamorphosis";
      action_list_str += "/immolate,time_to_die>=4,if=(dot.immolate.remains<cast_time)";
      action_list_str += "/immolation,if=(buff.tier10_4pc_caster.react)|(buff.metamorphosis.remains<15)";
      action_list_str += "/curse_of_doom,time_to_die>=70";
      if ( talents.decimation ) action_list_str += "/soul_fire,if=(buff.decimation.react)&(buff.molten_core.react)";
      action_list_str += "/corruption,time_to_die>=8,if=!ticking";
      if ( talents.decimation ) action_list_str += "/soul_fire,if=buff.decimation.react";
      action_list_str += "/incinerate,if=buff.molten_core.react";
		  // Set Mana Buffer pre 35% with or without Glyph of Life Tap
		  if( set_bonus.tier7_4pc_caster() || glyphs.life_tap )
			  {
			  action_list_str += "/life_tap,trigger=12000,health_percentage>=35,if=buff.metamorphosis.down";
			  }
		  else
			  {
			  action_list_str += "/life_tap,trigger=19000,health_percentage>=35,if=buff.metamorphosis.down";
			  }
	  action_list_str += "/curse_of_agony,moving=1,if=(!ticking)&!(dot.curse_of_doom.remains>0)";
    }

    // Hybrid 00_41_30
    else if ( talents.summon_felguard && talents.emberstorm && talents.decimation )
    {
      if ( talents.demonic_empowerment ) action_list_str += "/demonic_empowerment";
      action_list_str += "/corruption,time_to_die>=20,if=!ticking";
      action_list_str += "/immolate,if=(dot.immolate.remains<cast_time)";
      action_list_str += "/soul_fire,if=buff.decimation.react";
      action_list_str += "/curse_of_agony,time_to_die>=20,if=!ticking";
    }

    // Hybrid 00_40_31
    else if ( talents.decimation && talents.conflagrate )
    {
      if ( talents.demonic_empowerment ) action_list_str += "/demonic_empowerment";
      action_list_str += "/corruption,time_to_die>=20,if=!ticking";
      action_list_str += "/immolate,if=(dot.immolate.remains<cast_time)";
      action_list_str += "/conflagrate";
      action_list_str += "/soul_fire,if=buff.decimation.react";
      action_list_str += "/curse_of_agony,time_to_die>=20,if=!ticking";
    }

    else // generic
    {
      action_list_str += "/corruption,if=!ticking/curse_of_agony,time_to_die>20,if=!ticking/immolate,if=(dot.immolate.remains<cast_time)";
      if ( sim->debug ) log_t::output( sim, "Using generic action string for %s.", name() );
    }

    // Main Nuke
    action_list_str += talents.emberstorm ? "/incinerate" : "/shadow_bolt";

    // instants to use when moving if possible
    if ( talents.shadow_burn ) action_list_str += "/shadow_burn,moving=1";
    if ( talents.shadowfury  ) action_list_str += "/shadowfury,moving=1";

    action_list_str += "/" + tap_str; // to use when no mana or nothing else is possible

    action_list_default = 1;
  }

  player_t::init_actions();
}


// warlock_t::reset ==========================================================

void warlock_t::reset()
{
  player_t::reset();

  // Active
  active_pet                 = 0;
  active_corruption          = 0;
  active_curse               = 0;
  active_curse_of_agony      = 0;
  active_curse_of_doom       = 0;
  active_drain_life          = 0;
  active_drain_soul          = 0;
  active_immolate            = 0;
  active_shadowflame         = 0;
  active_unstable_affliction = 0;

  while ( ! decimation_queue.empty() ) decimation_queue.pop();
}

// warlock_t::regen ==========================================================

void warlock_t::regen( double periodicity )
{
  player_t::regen( periodicity );
}

// warlock_t::create_expression =================================================

action_expr_t* warlock_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "decimation_queue" )
  {
    struct decimation_queue_expr_t : public action_expr_t
    {
      decimation_queue_expr_t( action_t* a ) : action_expr_t( a, "decimation_queue" ) { result_type = TOK_NUM; }
      virtual int evaluate() { result_num = action -> player -> cast_warlock() -> decimation_queue.size(); return TOK_NUM; }
    };
    return new decimation_queue_expr_t( a );
  }

  return player_t::create_expression( a, name_str );
}

// warlock_t::primary_tree =================================================

int warlock_t::primary_tree() SC_CONST
{
  if ( talents.unstable_affliction ) return TREE_AFFLICTION;
  if ( talents.decimation          ) return TREE_DEMONOLOGY;
  if ( talents.shadow_and_flame    ) return TREE_DESTRUCTION;

  return TALENT_TREE_MAX;
}

// warlock_t::get_dot ======================================================

dot_t* warlock_t::get_dot( const std::string& name )
{
  if ( name == "immolate"            ) return player_t::get_dot( "immo_ua" );
  if ( name == "unstable_affliction" ) return player_t::get_dot( "immo_ua" );

  return player_t::get_dot( name );
}

// warlock_t::get_talent_trees =============================================

std::vector<talent_translation_t>& warlock_t::get_talent_list()
{
  if(talent_list.empty())
  {
          talent_translation_t translation_table[][MAX_TALENT_TREES] =
        {
    { {  1, 2, &( talents.improved_curse_of_agony ), 0, 0}, {  1, 2, NULL                                 , 0, 0}, {  1, 5, &( talents.improved_shadow_bolt  ), 0, 0} },
    { {  2, 3, &( talents.suppression             ), 0, 0}, {  2, 3, &( talents.improved_imp             ), 0, 0}, {  2, 5, &( talents.bane                  ), 0, 0} },
    { {  3, 5, &( talents.improved_corruption     ), 0, 0}, {  3, 3, &( talents.demonic_embrace          ), 0, 0}, {  3, 2, &( talents.aftermath             ), 1, 0} },
    { {  4, 2, NULL                                , 1, 0}, {  4, 2, NULL                                 , 1, 0}, {  4, 3, NULL                              , 1, 0} },
    { {  5, 2, &( talents.improved_drain_soul     ), 1, 0}, {  5, 2, NULL                                 , 1, 0}, {  5, 3, &( talents.cataclysm             ), 1, 0} },
    { {  6, 2, &( talents.improved_life_tap       ), 1, 0}, {  6, 3, &( talents.demonic_brutality        ), 1, 0}, {  6, 2, &( talents.demonic_power         ), 2, 0} },
    { {  7, 2, &( talents.soul_siphon             ), 1, 0}, {  7, 3, &( talents.fel_vitality             ), 1, 0}, {  7, 1, &( talents.shadow_burn           ), 2, 0} },
    { {  8, 2, NULL                                , 2, 0}, {  8, 3, NULL                                 , 2, 0}, {  8, 5, &( talents.ruin                  ), 2, 0} },
    { {  9, 3, NULL                                , 2, 0}, {  9, 1, NULL                                 , 2, 0}, {  9, 2, NULL                              , 3, 0} },
    { { 10, 1, &( talents.amplify_curse           ), 2, 0}, { 10, 1, &( talents.fel_domination           ), 2, 0}, { 10, 2, &( talents.destructive_reach     ), 3, 0} },
    { { 11, 2, NULL                                , 3, 0}, { 11, 3, &( talents.demonic_aegis            ), 3, 0}, { 11, 3, &( talents.improved_searing_pain ), 3, 0} },
    { { 12, 2, &( talents.nightfall               ), 3, 0}, { 12, 5, &( talents.unholy_power             ), 3, 9}, { 12, 3, &( talents.backlash              ), 4, 9} },
    { { 13, 3, &( talents.empowered_corruption    ), 3, 0}, { 13, 2, &( talents.master_summoner          ), 3,10}, { 13, 3, &( talents.improved_immolate     ), 4, 0} },
    { { 14, 5, &( talents.shadow_embrace          ), 4, 0}, { 14, 1, &( talents.mana_feed                ), 4,12}, { 14, 1, &( talents.devastation           ), 4, 8} },
    { { 15, 1, &( talents.siphon_life             ), 4, 0}, { 15, 2, &( talents.master_conjuror          ), 4, 0}, { 15, 3, NULL                              , 5, 0} },
    { { 16, 1, NULL                                , 4,10}, { 16, 5, &( talents.master_demonologist      ), 4,12}, { 16, 5, &( talents.emberstorm            ), 5, 0} },
          { { 17, 2, &( talents.improved_felhunter      ), 5, 0}, { 17, 3, &( talents.molten_core              ), 5, 0}, { 17, 1, &( talents.conflagrate           ), 6,13} },
    { { 18, 5, &( talents.shadow_mastery          ), 5,15}, { 18, 3, NULL                                 , 5, 0}, { 18, 3, &( talents.soul_leech            ), 6, 0} },
    { { 19, 3, &( talents.eradication             ), 6, 0}, { 19, 1, &( talents.demonic_empowerment      ), 6,16}, { 19, 3, &( talents.pyroclasm             ), 6, 0} },
    { { 20, 5, &( talents.contagion               ), 6, 0}, { 20, 3, &( talents.demonic_knowledge        ), 6, 0}, { 20, 5, &( talents.shadow_and_flame      ), 7, 0} },
    { { 21, 1, &( talents.dark_pact               ), 6, 0}, { 21, 5, &( talents.demonic_tactics          ), 6, 0}, { 21, 2, &( talents.improved_soul_leech   ), 7,18} },
    { { 22, 2, NULL                                , 7, 0}, { 22, 2, &( talents.decimation               ), 7, 0}, { 22, 3, &( talents.backdraft             ), 8,17} },
    { { 23, 3, &( talents.malediction             ), 7, 0}, { 23, 3, &( talents.improved_demonic_tactics ), 7,21}, { 23, 1, &( talents.shadowfury            ), 8, 0} },
    { { 24, 3, &( talents.deaths_embrace          ), 8, 0}, { 24, 1, &( talents.summon_felguard          ), 8, 0}, { 24, 3, &( talents.empowered_imp         ), 8, 0} },
    { { 25, 1, &( talents.unstable_affliction     ), 8,20}, { 25, 3, &( talents.nemesis                  ), 8, 0}, { 25, 5, &( talents.fire_and_brimstone    ), 9, 0} },
    { { 26, 1, &( talents.pandemic                ), 8,25}, { 26, 5, &( talents.demonic_pact             ), 9, 0}, { 26, 1, &( talents.chaos_bolt            ), 10, 0} },
    { { 27, 5, &( talents.everlasting_affliction  ), 9, 0}, { 27, 1, &( talents.metamorphosis            ), 10, 0}, {  0, 0, NULL                               } },
    { { 28, 1, &( talents.haunt                   ),10, 0}, {  0, 0, NULL                                  }, {  0, 0, NULL                               } },
    { {  0, 0, NULL                                 }, {  0, 0, NULL                                  }, {  0, 0, NULL                               } }
  };

    util_t::translate_talent_trees( talent_list, translation_table, sizeof( translation_table) );
  }
  return talent_list;
}

// warlock_t::get_options ==================================================

std::vector<option_t>& warlock_t::get_options()
{
  if ( options.empty() )
  {
    player_t::get_options();

    option_t warlock_options[] =
    {
      // @option_doc loc=player/warlock/talents title="Talents"
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
      // @option_doc loc=player/warlock/misc title="Misc"
      { "summon_pet",               OPT_STRING, &( summon_pet_str                 ) },
      { "hasted_corruption",        OPT_INT,  &( hasted_corruption                ) },
      { NULL, OPT_UNKNOWN, NULL }
    };

    option_t::copy( options, warlock_options );
  }

  return options;
}

// warlock_t::decode_set ===================================================

int warlock_t::decode_set( item_t& item )
{
  if ( item.slot != SLOT_HEAD      &&
       item.slot != SLOT_SHOULDERS &&
       item.slot != SLOT_CHEST     &&
       item.slot != SLOT_HANDS     &&
       item.slot != SLOT_LEGS      )
  {
    return SET_NONE;
  }

  const char* s = item.name();

  if ( strstr( s, "plagueheart"  ) ) return SET_T7_CASTER;
  if ( strstr( s, "deathbringer" ) ) return SET_T8_CASTER;
  if ( strstr( s, "kelthuzads"   ) ) return SET_T9_CASTER;
  if ( strstr( s, "guldans"      ) ) return SET_T9_CASTER;
  if ( strstr( s, "dark_coven"   ) ) return SET_T10_CASTER;

  return SET_NONE;
}

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_warlock ================================================

player_t* player_t::create_warlock( sim_t* sim, const std::string& name, int race_type )
{
  return new warlock_t( sim, name, race_type );
}

// player_t::warlock_init ===================================================

void player_t::warlock_init( sim_t* sim )
{
  for( player_t* p = sim -> player_list; p; p = p -> next )
  {
    p -> buffs.demonic_pact = new buff_t( p, "demonic_pact", 1, 45.0 );
  }

  target_t* t = sim -> target;
  t -> debuffs.improved_shadow_bolt = new     debuff_t( sim, "improved_shadow_bolt", 1, 30.0 );
  t -> debuffs.curse_of_elements    = new coe_debuff_t( sim );
}

// player_t::warlock_combat_begin ===========================================

void player_t::warlock_combat_begin( sim_t* sim )
{
  target_t* t = sim -> target;
  if ( sim -> overrides.improved_shadow_bolt ) t -> debuffs.improved_shadow_bolt -> override();
  if ( sim -> overrides.curse_of_elements    ) t -> debuffs.curse_of_elements    -> override( 1, 13 );
}

