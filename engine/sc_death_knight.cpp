// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

struct bloodworms_pet_t;
struct dancing_rune_weapon_pet_t;
struct ghoul_pet_t;

// ==========================================================================
// Death Knight Runes
// ==========================================================================

enum rune_type
{
  RUNE_TYPE_NONE=0, RUNE_TYPE_BLOOD, RUNE_TYPE_FROST, RUNE_TYPE_UNHOLY, RUNE_TYPE_DEATH, RUNE_TYPE_WASDEATH=8
};

#define RUNE_TYPE_MASK     3
#define RUNE_SLOT_MAX      6

#define RUNE_GRACE_PERIOD   2.0
#define RUNIC_POWER_REFUND  0.9

#define GET_BLOOD_RUNE_COUNT(x)  (((x&0x01) + ((x&0x02)>>1))    )
#define GET_FROST_RUNE_COUNT(x)  (((x&0x08) + ((x&0x10)>>1)) >>3)
#define GET_UNHOLY_RUNE_COUNT(x) (((x&0x40) + ((x&0x80)>>1)) >>6)

struct dk_rune_t
{
  double cooldown_ready;
  int    type;

  dk_rune_t() : cooldown_ready( 0 ), type( RUNE_TYPE_NONE ) {}

  bool is_death(                     ) SC_CONST { return ( type & RUNE_TYPE_DEATH ) != 0;  }
  bool is_ready( double current_time ) SC_CONST { return cooldown_ready <= current_time; }
  int  get_type(                     ) SC_CONST { return type & RUNE_TYPE_MASK;          }

  void consume( double current_time, double cooldown, bool convert )
  {
    assert ( current_time >= cooldown_ready );

    cooldown_ready = ( current_time <= ( cooldown_ready + RUNE_GRACE_PERIOD ) ? cooldown_ready : current_time ) + cooldown;
    type = ( type & RUNE_TYPE_MASK ) | ( ( type << 1 ) & RUNE_TYPE_WASDEATH ) | ( convert ? RUNE_TYPE_DEATH : 0 ) ;
  }

  void refund()   { cooldown_ready = 0; type  = ( type & RUNE_TYPE_MASK ) | ( ( type >> 1 ) & RUNE_TYPE_DEATH ); }
  void reset()    { cooldown_ready = -RUNE_GRACE_PERIOD - 1; type  = type & RUNE_TYPE_MASK;                               }
};

// ==========================================================================
// Death Knight
// ==========================================================================

enum death_knight_presence { PRESENCE_BLOOD=1, PRESENCE_FROST, PRESENCE_UNHOLY=4 };

struct death_knight_t : public player_t
{
  // Active
  int       active_presence;
  action_t* active_blood_caked_blade;
  action_t* active_necrosis;
  action_t* active_unholy_blight;
  action_t* active_wandering_plague;

  // Pets and Guardians
  bloodworms_pet_t*          active_bloodworms;
  dancing_rune_weapon_pet_t* active_dancing_rune_weapon;
  ghoul_pet_t*               active_ghoul;

  // Buffs
  buff_t* buffs_bloodworms;
  buff_t* buffs_bloody_vengeance;
  buff_t* buffs_bone_shield;
  buff_t* buffs_dancing_rune_weapon;
  buff_t* buffs_desolation;
  buff_t* buffs_deathchill;
  buff_t* buffs_icy_talons;
  buff_t* buffs_killing_machine;
  buff_t* buffs_rime;
  buff_t* buffs_scent_of_blood;
  buff_t* buffs_sigil_hanged_man;
  buff_t* buffs_sigil_virulence;
  buff_t* buffs_tier10_4pc_melee;
  buff_t* buffs_tier9_2pc_melee;
  buff_t* buffs_rune_of_the_fallen_crusader;
  buff_t* buffs_rune_of_razorice;
  buff_t* buffs_unbreakable_armor;
  
  // DoTs
  dot_t* dots_blood_plague;
  dot_t* dots_frost_fever;
  
  // Presences
  buff_t* buffs_blood_presence;
  buff_t* buffs_frost_presence;
  buff_t* buffs_unholy_presence;

  // Gains
  gain_t* gains_butchery;
  gain_t* gains_chill_of_the_grave;
  gain_t* gains_dirge;
  gain_t* gains_glyph_icy_touch;
  gain_t* gains_horn_of_winter;
  gain_t* gains_power_refund;
  gain_t* gains_rune_abilities;
  gain_t* gains_scent_of_blood;

  // Procs
  proc_t* procs_abominations_might;
  proc_t* procs_sudden_doom;

  // RNGs
  rng_t* rng_annihilation;
  rng_t* rng_blood_caked_blade;
  rng_t* rng_threat_of_thassarian;
  rng_t* rng_wandering_plague;

  // Up-Times
  uptime_t* uptimes_blood_plague;
  uptime_t* uptimes_frost_fever;

  // Cooldowns
  cooldown_t* cooldowns_howling_blast;

  // Auto-Attack
  attack_t* main_hand_attack;
  attack_t*  off_hand_attack;

  // Diseases
  spell_t* blood_plague;
  spell_t* frost_fever;

  // Special
  spell_t* sudden_doom;

  // Options
  std::string hysteria_target_str;

  // Talents
  struct talents_t
  {
    int abominations_might;
    int acclimation;
    int annihilation;
    int anti_magic_zone;
    int anticipation;
    int black_ice;
    int blade_barrier;
    int bladed_armor;
    int blood_caked_blade;
    int blood_gorged;
    int blood_of_the_north;
    int bloodworms;
    int bloody_strikes;
    int bloody_vengeance;
    int bone_shield;
    int butchery;
    int chilblains;
    int chill_of_the_grave;
    int corpse_explosion;
    int crypt_fever;
    int dancing_rune_weapon;
    int dark_conviction;
    int death_rune_mastery;
    int deathchill;
    int desecration;
    int desolation;
    int dirge;
    int ebon_plaguebringer;
    int endless_winter;
    int epidemic;
    int frigid_deathplate;
    int frost_strike;
    int ghoul_frenzy;
    int glacier_rot;
    int guile_of_gorefiend;
    int heart_strike;
    int howling_blast;
    int hungering_cold;
    int hysteria;
    int icy_reach;
    int icy_talons;
    int improved_blood_presence;
    int improved_death_strike;
    int improved_frost_presence;
    int improved_icy_talons;
    int improved_icy_touch;
    int improved_rune_tap;
    int improved_unholy_presence;
    int impurity;
    int killing_machine;
    int lichborne;
    int magic_supression;
    int mark_of_blood;
    int master_of_ghouls;
    int merciless_combat;
    int might_of_mograine;
    int morbidity;
    int necrosis;
    int nerves_of_cold_steel;
    int night_of_the_dead;
    int on_a_pale_horse;
    int outbreak;
    int rage_of_rivendare;
    int ravenous_dead;
    int reaping;
    int rime;
    int rune_tap;
    int runic_power_mastery;
    int scent_of_blood;
    int scourge_strike;
    int spell_deflection;
    int subversion;
    int sudden_doom;
    int summon_gargoyle;
    int threat_of_thassarian;
    int toughness;
    int tundra_stalker;
    int two_handed_weapon_specialization;
    int unbreakable_armor;
    int unholy_blight;
    int unholy_command;
    int vampiric_blood;
    int vendetta;
    int veteran_of_the_third_war;
    int vicious_strikes;
    int virulence;
    int wandering_plague;
    int will_of_the_necropolis;

    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  // Glyphs
  struct glyphs_t
  {
    int anti_magic_shell;
    int blood_strike;
    int blood_tap;
    int bone_shield;
    int chains_of_ice;
    int corpse_explosion;
    int dancing_rune_weapon;
    int dark_command;
    int dark_death;
    int death_and_decay;
    int death_grip;
    int death_strike;
    int deaths_embrace;
    int disease;
    int frost_strike;
    int heart_strike;
    int horn_of_winter;
    int howling_blast;
    int hungering_cold;
    int icebound_fortitude;
    int icy_touch;
    int obliterate;
    int pestilence;
    int plague_strike;
    int raise_dead;
    int rune_strike;
    int rune_tap;
    int scourge_strike;
    int strangulate;
    int the_ghoul;
    int unbreakable_armor;
    int unholy_blight;
    int vampiric_blood;

    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;
  
  // Sigils
  struct sigils_t
  {
    // PVP
    int deadly_strife;
    int furious_strife;
    int hateful_strife;
    int relentless_strife;
    int savage_strife;

    // PVE
    int arthritic_binding;
    int awareness;
    int dark_rider;
    int deflection;
    int frozen_conscience;
    int hanged_man;
    int haunted_dreams;
    int insolence;
    int unfaltering_knight;
    int vengeful_heart;
    int virulence;
    int wild_buck;

    sigils_t() { memset( ( void* ) this, 0x0, sizeof( sigils_t ) ); }
  };
  sigils_t sigils;

  struct runes_t
  {
    dk_rune_t slot[RUNE_SLOT_MAX];

    runes_t()    { for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) slot[i].type = RUNE_TYPE_BLOOD + ( i >> 1 ); }
    void reset() { for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) slot[i].reset();                           }
  };
  runes_t _runes;

  death_knight_t( sim_t* sim, const std::string& name, int race_type = RACE_NONE ) :
      player_t( sim, DEATH_KNIGHT, name, race_type )
  {
    // Active
    active_presence            = 0;
    active_blood_caked_blade   = NULL;
    active_necrosis            = NULL;
    active_wandering_plague    = NULL;
    active_unholy_blight       = NULL;
    
    // DoTs
    dots_blood_plague  = get_dot( "blood_plague" );
    dots_frost_fever   = get_dot( "frost_fever" );

    // Pets and Guardians
    active_bloodworms          = NULL;
    active_dancing_rune_weapon = NULL;
    active_ghoul               = NULL;

    // Auto-Attack
    main_hand_attack    = NULL;
    off_hand_attack     = NULL;

    sudden_doom         = NULL;
    blood_plague        = NULL;
    frost_fever         = NULL;

    cooldowns_howling_blast = get_cooldown( "howling_blast" );
  }

  // Character Definition
  virtual void      init();
  virtual void      init_actions();
  virtual void      init_enchant();
  virtual void      init_race();
  virtual void      init_rng();
  virtual void      init_base();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_gains();
  virtual void      init_glyphs();
  virtual void      init_procs();
  virtual void      init_resources( bool force );
  virtual void      init_uptimes();
  virtual void      init_items();
  virtual void      init_rating();
  virtual double    composite_armor() SC_CONST;
  virtual double    composite_attack_haste() SC_CONST;
  virtual double    composite_attack_power() SC_CONST;
  virtual double    composite_attribute_multiplier( int attr ) SC_CONST;
  virtual double    composite_spell_hit() SC_CONST;
  virtual void      regen( double periodicity );
  virtual void      reset();
  virtual int       target_swing();
  virtual void      combat_begin();
  virtual std::vector<talent_translation_t>& get_talent_list();
  virtual std::vector<option_t>& get_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet( const std::string& name );
  virtual void      create_pets();
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST { return RESOURCE_RUNIC; }
  virtual int       primary_role() SC_CONST     { return ROLE_ATTACK; }
  virtual int       primary_tree() SC_CONST;
  
  // Death Knight Specific helperfunctions
  int  diseases()
  {
    int disease_count = 0;
    
    if ( dots_blood_plague -> ticking() )
      disease_count++;
    
    if ( dots_frost_fever -> ticking() )
      disease_count++;
  
    if ( disease_count && talents.crypt_fever )
      disease_count++;
    
    return disease_count;
  }  
  void reset_gcd()
  {
    for ( action_t* a=action_list; a; a = a -> next )
    {
      if ( a -> trigger_gcd != 0 ) a -> trigger_gcd = base_gcd;
    }
  }
};

// ==========================================================================
// Guardians
// ==========================================================================

// ==========================================================================
// Bloodworms
// ==========================================================================
struct bloodworms_pet_t : public pet_t
{
  int spawn_count;

  struct melee_t : public attack_t
  {
    melee_t( player_t* player ) :
        attack_t( "bloodworm_melee", player, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_NONE, false )
    {
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      base_dd_min = base_dd_max = 1;
      may_crit    = true;
      background  = true;
      repeating   = true;

      pet_t* p = player -> cast_pet();

      // Orc Command Racial
      if ( p -> owner -> race == RACE_ORC )
      {
        base_multiplier *= 1.05;
      }
    }
    virtual void player_buff()
    {
      bloodworms_pet_t* p = ( bloodworms_pet_t* ) player;
      attack_t::player_buff();
      player_multiplier *= p -> spawn_count;
    }
  };

  melee_t* melee;

  bloodworms_pet_t( sim_t* sim, player_t* owner ) :
      pet_t( sim, owner, "bloodworms", true /*guardian*/ ), spawn_count( 3 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 20;
    main_hand_weapon.max_dmg    = 20;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = 2.0;
  }
  virtual void init_base()
  {
    pet_t::init_base();

    // Stolen from Priest's Shadowfiend
    attribute_base[ ATTR_STRENGTH  ] = 145;
    attribute_base[ ATTR_AGILITY   ] =  38;
    attribute_base[ ATTR_STAMINA   ] = 190;
    attribute_base[ ATTR_INTELLECT ] = 133;

    health_per_stamina = 7.5;
    mana_per_intellect = 5;

    melee = new melee_t( this );
  }
  virtual void summon( double duration=0 )
  {
    death_knight_t* o = owner -> cast_death_knight();
    pet_t::summon( duration );
    o -> active_bloodworms = this;
    melee -> schedule_execute();
  }
  virtual void dismiss()
  {
    death_knight_t* o = owner -> cast_death_knight();
    pet_t::dismiss();
    o -> active_bloodworms = 0;
  }

  virtual int primary_resource() SC_CONST { return RESOURCE_MANA; }
};

// ==========================================================================
// Dancing Rune Weapon
// ==========================================================================

struct dancing_rune_weapon_pet_t : public pet_t
{
  struct drw_heart_strike_t : public attack_t
  {
    drw_heart_strike_t( pet_t* player ) : 
    attack_t( "heart_strike", player, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_BLOOD, true )
    {
      background  = true;
      trigger_gcd = 0;

      weapon = &( player -> owner -> main_hand_weapon );
      normalize_weapon_speed = true;
      weapon_multiplier     *= 0.5;
  
      death_knight_t* o = player -> owner -> cast_death_knight();
      base_crit += o -> talents.subversion * 0.03;
      base_crit_bonus_multiplier *= 1.0 * o -> talents.might_of_mograine * 0.15;
      base_multiplier *= 1 + o -> talents.bloody_strikes * 0.15;
      base_multiplier *= 1 + o -> set_bonus.tier10_2pc_melee() * 0.07;
      base_multiplier *= 0.50; // DRW malus
      base_dd_min = base_dd_max = 0.01;
    }
    virtual bool ready() { return false; }
  };
  struct drw_death_strike_t : public attack_t
  {
    drw_death_strike_t( pet_t* player ) : 
    attack_t( "death_strike", player, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_BLOOD, true )
    {
      background  = true;
      trigger_gcd = 0;

      weapon = &( player -> owner -> main_hand_weapon );
      normalize_weapon_speed = true;
      weapon_multiplier     *= 0.75;
  
      death_knight_t* o = player -> owner -> cast_death_knight();
      base_crit += o -> talents.improved_death_strike * 0.03;
      base_crit_bonus_multiplier *= 1.0 + o -> talents.might_of_mograine * 0.15;
      base_multiplier *= 1 + o -> talents.improved_death_strike * 0.15;
      base_multiplier *= 0.50; // DRW malus
      base_dd_min = base_dd_max = 0.01;
    }
    virtual bool ready() { return false; }
  };

  double snapshot_spell_crit, snapshot_attack_crit;
  attack_t* drw_heart_strike;
  attack_t* drw_death_strike;

  dancing_rune_weapon_pet_t( sim_t* sim, player_t* owner ) :
      pet_t( sim, owner, "dancing_rune_weapon", true ),
      snapshot_spell_crit( 0.0 ), snapshot_attack_crit( 0.0 ), drw_heart_strike( 0 ),
      drw_death_strike( 0 )
      
  {
  }

  virtual void init_base()
  {
    // Everything stays at zero.
    // DRW uses a snapshot of the DKs stats when summoned.
    drw_heart_strike = new drw_heart_strike_t( this );
    drw_death_strike = new drw_death_strike_t( this );
  }

  virtual double composite_spell_crit() SC_CONST         { return snapshot_spell_crit;  }
  virtual double composite_attack_crit() SC_CONST        { return snapshot_attack_crit; }
  virtual double composite_attack_power() SC_CONST       { return attack_power; }
  virtual double composite_attack_penetration() SC_CONST { return attack_penetration; }

  virtual void summon( double duration=0 )
  {
    death_knight_t* o = owner -> cast_death_knight();
    pet_t::summon( duration );
    o -> active_dancing_rune_weapon = this;
    snapshot_spell_crit  = o -> composite_spell_crit();
    snapshot_attack_crit = o -> composite_attack_crit();
    attack_power         = o -> composite_attack_power() * o -> composite_attack_power_multiplier();
  }
  virtual void dismiss()
  {
    death_knight_t* o = owner -> cast_death_knight();
    pet_t::dismiss();
    o -> active_dancing_rune_weapon = 0;
  }
};

// ==========================================================================
// Gargoyle
// ==========================================================================

struct gargoyle_pet_t : public pet_t
{
  struct gargoyle_strike_t : public spell_t
  {
    gargoyle_strike_t( player_t* player ) :
        spell_t( "gargoyle_strike", player, RESOURCE_NONE, SCHOOL_NATURE )
    {
      // FIX ME!
      // Resist (can be partial)? Scaling?
      background  = true;
      repeating   = true;
      may_crit    = false;      

      base_dd_min = 130;
      base_dd_max = 150;
      base_spell_power_multiplier  = 0;
      base_attack_power_multiplier = 1;
      direct_power_mod             = 0.40;

      base_execute_time = 2.0;
    }
  };

  gargoyle_strike_t* gargoyle_strike;
  double haste_snapshot, power_snapshot;

  gargoyle_pet_t( sim_t* sim, player_t* owner ) :
      pet_t( sim, owner, "gargoyle", true ), gargoyle_strike( 0 ), haste_snapshot( 1.0 ),
      power_snapshot( 0.0 )
  {
  }

  virtual void init_base()
  {
    // FIX ME!
    attribute_base[ ATTR_STRENGTH  ] = 0;
    attribute_base[ ATTR_AGILITY   ] = 0;
    attribute_base[ ATTR_STAMINA   ] = 0;
    attribute_base[ ATTR_INTELLECT ] = 0;
    attribute_base[ ATTR_SPIRIT    ] = 0;

    gargoyle_strike = new gargoyle_strike_t( this );
  }
  virtual double haste() { return haste_snapshot; }
  virtual double composite_attack_power() SC_CONST { return power_snapshot; }

  virtual void summon( double duration=0 )
  {
    pet_t::summon( duration );
    // Haste etc. are taken at the time of summoning
    death_knight_t* o = owner -> cast_death_knight();
    haste_snapshot = o -> composite_attack_haste();
    power_snapshot = o -> composite_attack_power() * o -> composite_attack_power_multiplier();
    gargoyle_strike -> schedule_execute(); // Kick-off repeating attack
  }
};

// ==========================================================================
// Pet Ghoul
// ==========================================================================

struct ghoul_pet_t : public pet_t
{
  attack_t* main_hand_attack;
  ghoul_pet_t( sim_t* sim, player_t* owner ) :
      pet_t( sim, owner, "ghoul" ), main_hand_attack( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 100; // FIXME only level 80 value
    main_hand_weapon.max_dmg    = 100; // FIXME only level 80 value
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = 2.0;
    
    action_list_str = "auto_attack/claw";
  }

  virtual void init_base()
  {
    attribute_base[ ATTR_STRENGTH  ] = 331;
    attribute_base[ ATTR_AGILITY   ] = 856;
    attribute_base[ ATTR_STAMINA   ] = 361;
    attribute_base[ ATTR_INTELLECT ] = 65;
    attribute_base[ ATTR_SPIRIT    ] = 109;

    base_attack_power = -20;
    initial_attack_power_per_strength = 1.0;
    initial_attack_power_per_agility  = 1.0;

    initial_attack_crit_per_agility = rating_t::interpolate( level, 0.01/25.0, 0.01/40.0, 0.01/83.3 );

    resource_base[ RESOURCE_ENERGY ] = 100;
    energy_regen_per_second  = 10;
  }

  virtual bool ooc_buffs() {
    death_knight_t* o = owner -> cast_death_knight();
    if ( o -> talents.master_of_ghouls )
      return true;

    return false;
  }

  virtual double strength() SC_CONST
  {
    death_knight_t* o = owner -> cast_death_knight();
    double a = attribute[ ATTR_STRENGTH ];
    if ( o -> talents.master_of_ghouls )
      a += std::max( sim -> auras.strength_of_earth -> value(),
                     sim -> auras.horn_of_winter -> value() );
    
    double strength_scaling = 0.7; // % of str ghould gets from the DK
    strength_scaling *= 1.0 + o -> talents.ravenous_dead * .2; // The talent affects the 70% => 70% * 1.6 => 112%
    strength_scaling += o -> glyphs.the_ghoul * .4; // But the glyph is additive!
    a += o -> strength() *  strength_scaling;
    
    a *= composite_attribute_multiplier( ATTR_STRENGTH );
    return floor( a );
  }

  virtual void summon( double duration=0 )
  {
    death_knight_t* o = owner -> cast_death_knight();
    pet_t::summon( duration );
    o -> active_ghoul = this;
  }

  virtual void dismiss()
  {
    death_knight_t* o = owner -> cast_death_knight();
    pet_t::dismiss();
    o -> active_ghoul = 0;
  }

  virtual double composite_attack_haste() SC_CONST
  {
    // Ghouls receive 100% of their master's haste.
    // http://elitistjerks.com/f72/t42606-pet_discussion_garg_aotd_ghoul/
    death_knight_t* o = owner -> cast_death_knight();
    return o -> composite_attack_haste();
  }

  virtual int primary_resource() SC_CONST { return RESOURCE_ENERGY; }
  
  virtual action_t* create_action( const std::string& name, const std::string& options_str );  
};

struct ghoul_pet_attack_t : public attack_t
{
  ghoul_pet_attack_t( const char* n, player_t* player, int r=RESOURCE_ENERGY, int sc=SCHOOL_PHYSICAL, bool special=true ) :
      attack_t( n, player, r, sc, TREE_UNHOLY, special )
  {
    weapon = &( player -> main_hand_weapon );
    may_crit = true;
    weapon_power_mod *= 0.84;
  }  
};

// Ghoul Pet Melee ===========================================================

struct ghoul_pet_melee_t : public ghoul_pet_attack_t
{
  ghoul_pet_melee_t( player_t* player ) :
      ghoul_pet_attack_t( "melee", player, RESOURCE_NONE, SCHOOL_PHYSICAL, false )
  {
    base_execute_time = weapon -> swing_time;
    base_dd_min       = base_dd_max = 1;
    background        = true;
    repeating         = true;
    direct_power_mod  = 0;
  }
};

// Ghoul Auto Attack =========================================================

struct ghoul_pet_auto_attack_t : public ghoul_pet_attack_t
{
  ghoul_pet_auto_attack_t( player_t* player ) :
      ghoul_pet_attack_t( "auto_attack", player )
  {
    ghoul_pet_t* p = ( ghoul_pet_t* ) player -> cast_pet();

    weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack = new ghoul_pet_melee_t( player );
    trigger_gcd = 0;
  }
  virtual void execute()
  {
    ghoul_pet_t* p = ( ghoul_pet_t* ) player -> cast_pet();
    p -> main_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    ghoul_pet_t* p = ( ghoul_pet_t* ) player -> cast_pet();
    return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

struct ghoul_pet_claw_t : public ghoul_pet_attack_t
{
  ghoul_pet_claw_t( player_t* player ) :
      ghoul_pet_attack_t( "claw", player )
  {
    ghoul_pet_t* p = ( ghoul_pet_t* ) player -> cast_pet();

    weapon            = &( p -> main_hand_weapon );
    base_dd_min       = base_dd_max = 0.1;
    base_cost         = 40;
    weapon_multiplier = 1.5;
    direct_power_mod  = 0;
  }
};

// ghoul_pet_t::create_action ==============================================

action_t* ghoul_pet_t::create_action( const std::string& name,
                                       const std::string& options_str )
{
  if ( name == "auto_attack" ) return new ghoul_pet_auto_attack_t( this );
  if ( name == "claw"        ) return new        ghoul_pet_claw_t( this );

  return pet_t::create_action( name, options_str );
}

namespace   // ANONYMOUS NAMESPACE ==========================================
{

// ==========================================================================
// Death Knight Attack
// ==========================================================================

struct death_knight_attack_t : public attack_t
{
  bool   requires_weapon;
  int    cost_blood;
  int    cost_frost;
  int    cost_unholy;
  double convert_runes;
  bool   use[RUNE_SLOT_MAX];
  int    min_blood, max_blood, exact_blood;
  int    min_frost, max_frost, exact_frost;
  int    min_unholy, max_unholy, exact_unholy;
  int    min_death, max_death, exact_death;
  int    min_runic, max_runic, exact_runic;
  int    min_blood_plague, max_blood_plague;
  int    min_frost_fever, max_frost_fever;
  int    min_desolation, max_desolation;
  bool   additive_factors;

  death_knight_attack_t( const char* n, player_t* player, int s=SCHOOL_PHYSICAL, int t=TREE_NONE, bool special=true ) :
      attack_t( n, player, RESOURCE_RUNIC, s, t ),
      requires_weapon( true ),
      cost_blood( 0 ),cost_frost( 0 ),cost_unholy( 0 ),convert_runes( 0 ),
      min_blood( -1 ), max_blood( -1 ), exact_blood( -1 ),
      min_frost( -1 ), max_frost( -1 ), exact_frost( -1 ),
      min_unholy( -1 ), max_unholy( -1 ), exact_unholy( -1 ),
      min_death( -1 ), max_death( -1 ), exact_death( -1 ),
      min_runic( -1 ), max_runic( -1 ), exact_runic( -1 ),
      min_blood_plague( -1 ), max_blood_plague( -1 ),
      min_frost_fever( -1 ), max_frost_fever( -1 ),
      min_desolation( -1 ), max_desolation( -1 ),
      additive_factors( false )
  {
    death_knight_t* p = player -> cast_death_knight();
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;
    may_crit   = true;
    may_glance = false;

    base_crit += p -> talents.dark_conviction * 0.01;
    base_crit += p -> talents.ebon_plaguebringer * 0.01;
  }

  virtual void   parse_options( option_t*, const std::string& options_str );
  virtual void   reset();

  virtual void   consume_resource();
  virtual void   execute();
  virtual void   player_buff();
  virtual void   target_debuff( int school );
  virtual bool   ready();
};

// ==========================================================================
// Death Knight Spell
// ==========================================================================

struct death_knight_spell_t : public spell_t
{
  int    cost_blood;
  int    cost_frost;
  int    cost_unholy;
  double convert_runes;
  bool   use[RUNE_SLOT_MAX];
  int    min_blood, max_blood, exact_blood;
  int    min_frost, max_frost, exact_frost;
  int    min_unholy, max_unholy, exact_unholy;
  int    min_death, max_death, exact_death;
  int    min_runic, max_runic, exact_runic;
  int    min_blood_plague, max_blood_plague;
  int    min_frost_fever, max_frost_fever;
  int    min_desolation, max_desolation;

  death_knight_spell_t( const char* n, player_t* player, int s, int t ) :
      spell_t( n, player, RESOURCE_RUNIC, s, t ),
      cost_blood( 0 ),cost_frost( 0 ),cost_unholy( 0 ),convert_runes( false ),
      min_blood( -1 ), max_blood( -1 ), exact_blood( -1 ),
      min_frost( -1 ), max_frost( -1 ), exact_frost( -1 ),
      min_unholy( -1 ), max_unholy( -1 ), exact_unholy( -1 ),
      min_death( -1 ), max_death( -1 ), exact_death( -1 ),
      min_runic( -1 ), max_runic( -1 ), exact_runic( -1 ),
      min_blood_plague( -1 ), max_blood_plague( -1 ),
      min_frost_fever( -1 ), max_frost_fever( -1 ),
      min_desolation( -1 ), max_desolation( -1 )
  {
    death_knight_t* p = player -> cast_death_knight();
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;
    may_crit = true;
    // The Chaotic Skyflare Diamonds plays oddly for death knight
    // spells.  For a death coil where base damage is 1747, we would
    // expect the crit bonus to be 1747, then multiply by the CSD, for
    // a total of 3599 damage.  In reality, we get 3653.  It would
    // appear the CSD bonus is applied to overall damage AND the boost
    // from "Runic Focus", resulting in 1.03 * (1747 + 1747 * 1.03).
    // I have no explanation other than having verified it at many
    // different attack power levels and it is consistent within one
    // point of damage.
    base_crit_bonus_multiplier = 2.0;
    base_spell_power_multiplier = 0;
    base_attack_power_multiplier = 1;

    base_crit += p -> talents.dark_conviction * 0.01;
    base_crit += p -> talents.ebon_plaguebringer * 0.01;
  }

  virtual void   parse_options( option_t*, const std::string& options_str );
  virtual void   reset();

  virtual void   consume_resource();
  virtual void   execute();
  virtual void   player_buff();
  virtual void   target_debuff( int school );
  virtual bool   ready();
};

// ==========================================================================
// Local Utility Functions
// ==========================================================================

// Count Runes ==============================================================
static int count_runes( player_t* player )
{
  death_knight_t* p = player -> cast_death_knight();
  double t = p -> sim -> current_time;
  int count = 0;

  // Storing ready runes by type in 2 bit blocks with 0 value bit separator (11 bits total)
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    if ( p -> _runes.slot[i].is_ready( t ) )
      count |= 1 << ( i + ( i >> 1 ) );

  // Adding bits in each two-bit block (0xDB = 011 011 011)
  // NOTE: Use GET_XXX_RUNE_COUNT macros to process return value
  return ( ( count & ( count << 1 ) ) | ( count >> 1 ) ) & 0xDB;
}

// Count Death Runes ========================================================
static int count_death_runes( death_knight_t* p )
{
  // Getting death rune count is a bit more complicated as it depends 
  // on talents which runetype can be converted to death runes
  double t = p -> sim -> current_time;
  int count = 0;
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
  {
    dk_rune_t& r = p -> _runes.slot[i];
    if ( r.is_ready( t ) && r.is_death() )
      ++count;
  }
  return count;
}


// Consume Runes ============================================================
static void consume_runes( player_t* player, const bool* use, bool convert_runes = false )
{
  death_knight_t* p = player -> cast_death_knight();
  double t = p -> sim -> current_time;

  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    if ( use[i] )
    {
      if ( p -> sim -> log ) 
        log_t::output( p -> sim, "%s consumes rune #%d, type %d", p -> name(), i, p -> _runes.slot[i].type );

      p -> _runes.slot[i].consume( t, 10.0, convert_runes );
    }

  if ( count_runes( p ) == 0 )
    p -> buffs_tier10_4pc_melee -> trigger( 1, 0.03 );
}

// Group Runes ==============================================================
static bool group_runes ( player_t* player, int blood, int frost, int unholy, bool* group )
{
  death_knight_t* p = player -> cast_death_knight();
  double t = p -> sim -> current_time;
  int cost[]  = { blood + frost + unholy, blood, frost, unholy };
  bool use[RUNE_SLOT_MAX] = { false };

  // Selecting available non-death runes to satisfy cost
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
  {
    dk_rune_t& r = p -> _runes.slot[i];
    if ( r.is_ready( t ) && ! r.is_death() && cost[r.get_type()] > 0 )
    {
      --cost[r.get_type()];
      --cost[0];
      use[i] = true;
    }
  }

  // Selecting available death runes to satisfy remaining cost
  for ( int i = RUNE_SLOT_MAX; cost[0] > 0 && i--; )
  {
    dk_rune_t& r = p -> _runes.slot[i];
    if ( r.is_ready( t ) && r.is_death() )
    {
      --cost[0];
      use[i] = true;
    }
  }

  if ( cost[0] > 0 ) return false;

  // Storing rune slots selected
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) group[i] = use[i];

  return true;
}

// Refund Power =============================================================
static void refund_power( action_t* a )
{
  death_knight_t* p = a -> player -> cast_death_knight();

  if ( a -> resource_consumed > 0 )
    p -> resource_gain( RESOURCE_RUNIC, a -> resource_consumed * RUNIC_POWER_REFUND, p -> gains_power_refund );
}
// Refund Runes =============================================================
static void refund_runes( player_t* player, const bool* use )
{
  death_knight_t* p = player -> cast_death_knight();
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    if ( use[i] ) p -> _runes.slot[i].refund();
}


// ==========================================================================
// Triggers
// ==========================================================================

// Trigger Abominations Might ===============================================
static void trigger_abominations_might( action_t* a, double base_chance )
{
  death_knight_t* dk = a -> player -> cast_death_knight();

  a -> sim -> auras.abominations_might -> trigger( 1, 1.0, base_chance * dk -> talents.abominations_might );
}

// Trigger Blood Caked Blade ================================================
static void trigger_blood_caked_blade( action_t* a )
{
  death_knight_t* p = a -> player -> cast_death_knight();

  if ( ! p -> talents.blood_caked_blade )
    return;

  double chance = 0;
  chance = p -> talents.blood_caked_blade * 0.10;
  if ( p -> rng_blood_caked_blade->roll( chance ) )
  {
    struct bcb_t : public death_knight_attack_t
    {
      bcb_t( player_t* player ) :
          death_knight_attack_t( "blood_caked_blade", player, SCHOOL_PHYSICAL, TREE_UNHOLY )
      {
        may_crit       = false;
        background     = true;
        proc           = true;
        trigger_gcd    = false;
        base_dd_min = base_dd_max = 0.01;
        weapon = &( player -> main_hand_weapon );
        normalize_weapon_speed = false;
        reset();
      }
      virtual void target_debuff( int dmg_type )
      {
        death_knight_attack_t::target_debuff( dmg_type );
        death_knight_t* p = player -> cast_death_knight();
        target_multiplier *= 0.25 + p -> diseases() * 0.125;
      }
    };

    if ( ! p -> active_blood_caked_blade ) p -> active_blood_caked_blade = new bcb_t( p );
    p -> active_blood_caked_blade -> weapon =  a -> weapon;
    p -> active_blood_caked_blade -> execute();
  }
}

// Trigger Crypt Fever ======================================================
static void trigger_crypt_fever( action_t* a )
{
  if ( a -> sim -> overrides.crypt_fever ) return;

  death_knight_t* p = a -> player -> cast_death_knight();
  if ( ! p -> talents.crypt_fever ) return;

  double disease_duration = a -> dot -> ready - a -> sim -> current_time;
  if ( a -> sim -> target -> debuffs.crypt_fever -> remains_lt( disease_duration ) )
  {
    double value = util_t::talent_rank( p -> talents.crypt_fever, 3, 10 );
    a -> sim -> target -> debuffs.crypt_fever -> duration = disease_duration;
    a -> sim -> target -> debuffs.crypt_fever -> trigger( 1, value );
  }
}  

// Trigger Ebon Plaguebringer ===============================================
static void trigger_ebon_plaguebringer( action_t* a )
{
  if ( a -> sim -> overrides.ebon_plaguebringer ) return;
  death_knight_t* p = a -> player -> cast_death_knight();
  if ( ! p -> talents.ebon_plaguebringer ) return;

  double disease_duration = a -> dot -> ready - a -> sim -> current_time;
  if ( a -> sim -> target -> debuffs.crypt_fever -> remains_lt( disease_duration ) )
  {
    double value = util_t::talent_rank( p -> talents.ebon_plaguebringer, 3, 4, 9, 13 );
    a -> sim -> target -> debuffs.ebon_plaguebringer -> duration = disease_duration;
    a -> sim -> target -> debuffs.ebon_plaguebringer -> trigger( 1, value );
  }
}

// Trigger Icy Talons =======================================================
static void trigger_icy_talons( action_t* a )
{
  death_knight_t* p = a -> player -> cast_death_knight();
  if ( ! p -> talents.icy_talons ) return;

  p -> buffs_icy_talons -> trigger( 1, p -> talents.icy_talons * 0.04 );

  if ( p -> talents.improved_icy_talons )
    a -> sim -> auras.improved_icy_talons -> trigger();
}

// Trigger Necrosis =========================================================
static void trigger_necrosis( action_t* a )
{
  death_knight_t* p = a -> player -> cast_death_knight();

  if ( ! p -> talents.necrosis )
    return;

  struct necrosis_t : public death_knight_spell_t
  {
    necrosis_t( player_t* player ) :
        death_knight_spell_t( "necrosis", player, SCHOOL_SHADOW, TREE_UNHOLY )
    {
      may_crit       = false;
      may_miss       = false;
      background     = true;
      proc           = true;
      trigger_gcd    = false;
      base_dd_max = base_dd_max = 0.01;

      reset();
    }
    virtual double total_multiplier() SC_CONST { return 1.0; }
  };

  if ( ! p -> active_necrosis ) p -> active_necrosis = new necrosis_t( p );

  p -> active_necrosis -> base_dd_adder = a -> direct_dmg * 0.04 * p -> talents.necrosis;
  p -> active_necrosis -> execute();
}

// Trigger Sudden Doom ======================================================
static void trigger_sudden_doom( action_t* a )
{
  death_knight_t* p = a -> player -> cast_death_knight();

  if ( ! p -> sim -> roll( p -> talents.sudden_doom * 0.05 ) ) return;

  p -> procs_sudden_doom -> occur();
  // TODO: Intialize me
  //p -> sudden_doom -> execute();
}

// Trigger Wandering Plague =================================================
static void trigger_wandering_plague( action_t* a, double disease_damage )
{
  death_knight_t* p = a -> player -> cast_death_knight();

  if ( ! p -> talents.wandering_plague )
    return;

  struct wandering_plague_t : public death_knight_spell_t
  {
    wandering_plague_t( player_t* player ) :
        death_knight_spell_t( "wandering_plague", player, SCHOOL_SHADOW, TREE_UNHOLY )
    {
      may_crit       = false;
      may_miss       = false;
      background     = true;
      proc           = true;
      trigger_gcd    = false;
      may_resist     = false;
      cooldown = player -> get_cooldown( "wandering_plague" );
      cooldown -> duration       = 1.0;
      reset();
    }
    void target_debuff( int dmg_type )
    {
      // no debuff effect
    }
    void player_buff()
    {
      // no buffs
    }
  };
  if ( ! p -> active_wandering_plague ) p -> active_wandering_plague = new wandering_plague_t( p );

  double chance = p -> composite_attack_crit();

  if ( p -> rng_wandering_plague -> roll( chance ) && p -> active_wandering_plague -> ready() )
  {
    double dmg = disease_damage * p -> talents.wandering_plague / 3.0;

    p -> active_wandering_plague -> base_dd_min = dmg;
    p -> active_wandering_plague -> base_dd_max = dmg;
    p -> active_wandering_plague -> execute();
  }
}


// Trigger Unholy Blight ====================================================
static void trigger_unholy_blight( action_t* a, double death_coil_dmg )
{
  death_knight_t* p = a -> player -> cast_death_knight();

  if ( ! p -> talents.unholy_blight )
    return;

  struct unholy_blight_t : public death_knight_spell_t
  {
    unholy_blight_t( player_t* player ) :
        death_knight_spell_t( "unholy_blight", player, SCHOOL_SHADOW, TREE_UNHOLY )
    {
      base_tick_time = 2.0;
      num_ticks = 5;
      trigger_gcd = 0;
      background     = true;
      proc           = true;
      may_crit       = false;
      may_resist     = false;
      may_miss       = false;
      
      death_knight_t* p = player -> cast_death_knight();
      base_multiplier *= 1.0 + p -> glyphs.unholy_blight * 0.4;
      reset();
    }

    void target_debuff( int dmg_type )
    {
      // no debuff effect
    }

    void player_buff()
    {
      // no buffs
    }
  };

  if ( ! p -> active_unholy_blight ) p -> active_unholy_blight = new unholy_blight_t( p );

  double unholy_blight_dmg = death_coil_dmg * ( p -> sim -> P330 ? 0.1 : 0.2 );
  if ( p -> active_unholy_blight -> ticking )
  {
    int remaining_ticks = p -> active_unholy_blight -> num_ticks - p -> active_unholy_blight -> current_tick;
    unholy_blight_dmg += p -> active_unholy_blight -> base_td * remaining_ticks;
    p -> active_unholy_blight -> cancel();
  }
  p -> active_unholy_blight -> base_td = unholy_blight_dmg / p -> active_unholy_blight -> num_ticks;
  p -> active_unholy_blight -> execute();
}

// ==========================================================================
// Death Knight Attack Methods
// ==========================================================================

// death_knight_attack_t::parse_options =====================================

void death_knight_attack_t::parse_options( option_t* options, const std::string& options_str )
{
  option_t base_options[] =
  {
    { "blood",         OPT_INT,  &exact_blood },
    { "blood>",        OPT_INT,  &min_blood },
    { "blood<",        OPT_INT,  &max_blood },
    { "frost",         OPT_INT,  &exact_frost },
    { "frost>",        OPT_INT,  &min_frost },
    { "frost<",        OPT_INT,  &max_frost },
    { "unholy",        OPT_INT,  &exact_unholy },
    { "unholy>",       OPT_INT,  &min_unholy },
    { "unholy<",       OPT_INT,  &max_unholy },
    { "death",         OPT_INT,  &exact_death },
    { "death>",        OPT_INT,  &min_death },
    { "death<",        OPT_INT,  &max_death },
    { "runic",         OPT_INT,  &exact_runic },
    { "runic>",        OPT_INT,  &min_runic },
    { "runic<",        OPT_INT,  &max_runic },
    { "blood_plague>", OPT_INT,  &min_blood_plague },
    { "blood_plague<", OPT_INT,  &max_blood_plague },
    { "frost_fever>",  OPT_INT,  &min_frost_fever },
    { "frost_fever<",  OPT_INT,  &max_frost_fever },
    { "desolation>",   OPT_INT,  &min_desolation },
    { "desolation<",   OPT_INT,  &max_desolation },
    { NULL, OPT_UNKNOWN, NULL }
  };
  std::vector<option_t> merged_options;
  attack_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

// death_knight_attack_t::reset() ===========================================

void death_knight_attack_t::reset()
{
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;
  action_t::reset();
}

// death_knight_attack_t::consume_resource() ================================

void death_knight_attack_t::consume_resource()
{
  death_knight_t* p = player -> cast_death_knight();
  double gain = -cost();
  if ( gain > 0 )
  {
    // calculate_result() gets called before consume_resourcs()
    if ( result_is_hit() ) p -> resource_gain( resource, gain, p -> gains_rune_abilities );
  }
  else 
    attack_t::consume_resource();

  consume_runes( player, use, convert_runes == 0 ? false : sim->roll( convert_runes ) == 1 );
}

// death_knight_attack_t::execute() =========================================

void death_knight_attack_t::execute()
{
  death_knight_t* p = player -> cast_death_knight();

  attack_t::execute();

  if ( result_is_hit() )
  {
    p -> buffs_bloodworms -> trigger();

    if ( result == RESULT_CRIT )
      p -> buffs_bloody_vengeance -> trigger( 1, p -> talents.bloody_vengeance * 0.01);
  }
  else
  {
    refund_power( this );
    refund_runes( p, use );
  }
}

// death_knight_attack_t::player_buff() =====================================

void death_knight_attack_t::player_buff()
{
  death_knight_t* p = player -> cast_death_knight();

  attack_t::player_buff();

  if ( special )
  {
    // Rivendare is spells and abilities... so everything *except* raw melee ( = special)
    // Things not affected: SS shadow damage in 3.3, wandering plague
    if ( ! proc && sim -> target -> debuffs.blood_plague -> up() )
      player_multiplier *= 1.0 + p -> talents.rage_of_rivendare * 0.02;
      
    // Fix Me! Does the same apply as for RoR?
    if ( ! proc && sim -> target -> debuffs.frost_fever -> up() )
      player_multiplier *= 1.0 + p -> talents.tundra_stalker * 0.03;

    // Annihilation only applies to abilities
    player_crit += p -> talents.annihilation * 0.01;
  }

  if ( p -> talents.blood_gorged )
  {
    player_penetration += p -> talents.blood_gorged * 0.02;

    if ( p -> resource_current[ RESOURCE_HEALTH ] >=
         p -> resource_max    [ RESOURCE_HEALTH ] * 0.75 )
    {
      player_multiplier *= 1 + p -> talents.blood_gorged * 0.02;
    }
  }

  if ( weapon )
  {
    // http://www.wowhead.com/?spell=50138
    // +1/2/3% melee hit, ONLY one-handers
    // Does not apply to spells!
    if ( weapon -> slot == SLOT_OFF_HAND )
      player_multiplier *= 1.0 + p -> talents.nerves_of_cold_steel * 0.05;

    if ( weapon -> group() == WEAPON_1H )
    {
      player_hit += p -> talents.nerves_of_cold_steel  * 0.01;
    }
    else if ( weapon -> group() == WEAPON_2H )
    {
      player_multiplier *= 1.0 + p -> talents.two_handed_weapon_specialization * 0.02;
    }
  }
  
  // 3.3 Scourge Strike: Shadow damage is calcuted with some buffs being additive
  // Desolation, Bone Shield, Black Ice, Blood Presence
  if ( additive_factors )
  {
    double sum_factors = 0.0;
    
    sum_factors += p -> buffs_blood_presence -> value();
    sum_factors += p -> buffs_bone_shield -> value();
    sum_factors += p -> buffs_desolation -> value();
    if ( school == SCHOOL_FROST || school == SCHOOL_SHADOW )
      sum_factors += p -> talents.black_ice * 0.02;

    player_multiplier *= 1.0 + sum_factors;
  }
  else
  {
    player_multiplier *= 1.0 + p -> buffs_blood_presence -> value();
    player_multiplier *= 1.0 + p -> buffs_bone_shield -> value();
    player_multiplier *= 1.0 +  p -> buffs_desolation -> value();
    if ( school == SCHOOL_FROST || school == SCHOOL_SHADOW )
      player_multiplier *= 1.0 + p -> talents.black_ice * 0.02;
  }

  if ( school == SCHOOL_PHYSICAL )
  {
    player_multiplier *= 1.0 + p -> buffs_bloody_vengeance -> value();
  }
  
  player_multiplier *= 1.0 + p -> buffs_tier10_4pc_melee -> value();
}

// death_knight_attack_t::ready() ===========================================

bool death_knight_attack_t::ready()
{
  death_knight_t* p = player -> cast_death_knight();

  if ( ! attack_t::ready() )
    return false;

  if ( requires_weapon )
    if ( ! weapon || weapon->group() == WEAPON_RANGED )
      return false;

  if ( exact_runic > 0 )
    if ( p -> resource_current[ RESOURCE_RUNIC ] != exact_runic )
      return false;

  if ( min_runic > 0 )
    if ( p -> resource_current[ RESOURCE_RUNIC ] < min_runic )
      return false;

  if ( max_runic > 0 )
    if ( p -> resource_current[ RESOURCE_RUNIC ] > max_runic )
      return false;

  double ct = sim -> current_time;

  if ( min_blood_plague > 0 )
    if ( ! p -> dots_blood_plague -> ticking() || ( ( p -> dots_blood_plague -> ready - ct ) < min_blood_plague ) )
      return false;

  if ( max_blood_plague > 0 )
    if ( p -> dots_blood_plague -> ticking() && ( ( p -> dots_blood_plague -> ready - ct ) > max_blood_plague ) )
      return false;

  if ( min_frost_fever > 0 )
    if ( ! p -> dots_frost_fever -> ticking() || ( ( p -> dots_frost_fever -> ready - ct ) < min_frost_fever ) )
      return false;

  if ( max_frost_fever > 0 )
    if ( p -> dots_frost_fever -> ticking() && ( ( p -> dots_frost_fever -> ready - ct ) > max_frost_fever ) )
      return false;

  if ( min_desolation > 0 )
    if ( ! p -> buffs_desolation || ( p -> buffs_desolation -> remains_lt( min_desolation ) ) )
      return false;

  if ( max_desolation > 0 )
    if ( p -> buffs_desolation && ( p -> buffs_desolation -> remains_gt( max_desolation ) ) )
      return false;

  int count = count_runes( p );
  int c = GET_BLOOD_RUNE_COUNT( count );
  if ( ( exact_blood != -1 && c != exact_blood ) ||
       ( min_blood != -1 && c < min_blood ) ||
       ( max_blood != -1 && c > max_blood ) ) return false;

  c = GET_FROST_RUNE_COUNT( count );
  if ( ( exact_frost != -1 && c != exact_frost ) ||
       ( min_frost != -1 && c < min_frost ) ||
       ( max_frost != -1 && c > max_frost ) ) return false;

  c = GET_UNHOLY_RUNE_COUNT( count );
  if ( ( exact_unholy != -1 && c != exact_unholy ) ||
       ( min_unholy != -1 && c < min_unholy ) ||
       ( max_unholy != -1 && c > max_unholy ) ) return false;

  c = count_death_runes( p );
  if ( ( exact_death != -1 && c != exact_death ) ||
       ( min_death != -1 && c < min_death ) ||
       ( max_death != -1 && c > max_death ) ) return false;

  return group_runes( p, cost_blood, cost_frost, cost_unholy, use );
}

void death_knight_attack_t::target_debuff( int dmg_type )
{
  attack_t::target_debuff( dmg_type );
  death_knight_t* p = player -> cast_death_knight();
  if ( dmg_type == SCHOOL_FROST  )
  {
    target_multiplier *= 1.0 + p -> buffs_rune_of_razorice -> value();
  }
}

// ==========================================================================
// Death Knight Spell Methods
// ==========================================================================

// death_knight_spell_t::parse_options() ====================================

void death_knight_spell_t::parse_options( option_t* options, const std::string& options_str )
{
  option_t base_options[] =
  {
    { "blood",         OPT_INT,  &exact_blood },
    { "blood>",        OPT_INT,  &min_blood },
    { "blood<",        OPT_INT,  &max_blood },
    { "frost",         OPT_INT,  &exact_frost },
    { "frost>",        OPT_INT,  &min_frost },
    { "frost<",        OPT_INT,  &max_frost },
    { "unholy",        OPT_INT,  &exact_unholy },
    { "unholy>",       OPT_INT,  &min_unholy },
    { "unholy<",       OPT_INT,  &max_unholy },
    { "death",         OPT_INT,  &exact_death },
    { "death>",        OPT_INT,  &min_death },
    { "death<",        OPT_INT,  &max_death },
    { "runic",         OPT_INT,  &exact_runic },
    { "runic>",        OPT_INT,  &min_runic },
    { "runic<",        OPT_INT,  &max_runic },
    { "blood_plague>", OPT_INT,  &min_blood_plague },
    { "blood_plague<", OPT_INT,  &max_blood_plague },
    { "frost_fever>",  OPT_INT,  &min_frost_fever },
    { "frost_fever<",  OPT_INT,  &max_frost_fever },
    { "desolation>",   OPT_INT,  &min_desolation },
    { "desolation<",   OPT_INT,  &max_desolation },
    { NULL, OPT_UNKNOWN, NULL }
  };
  std::vector<option_t> merged_options;
  spell_t::parse_options( merge_options( merged_options, options, base_options ), options_str );
}

// death_knight_spell_t::reset() ============================================

void death_knight_spell_t::reset()
{
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;
  action_t::reset();
}

// death_knight_spell_t::consume_resource() =================================

void death_knight_spell_t::consume_resource()
{
  death_knight_t* p = player -> cast_death_knight();
  double gain = -cost();
  if ( gain > 0 )
  {
    // calculate_result() gets called before consume_resourcs()
    if ( result_is_hit() ) p -> resource_gain( resource, gain, p -> gains_rune_abilities );
  }
  else 
    spell_t::consume_resource();
  
  consume_runes( player, use, convert_runes == 0 ? false : sim->roll( convert_runes ) == 1 );
}

// death_knight_spell_t::xecute() ==========================================

void death_knight_spell_t::execute()
{
  death_knight_t* p = player -> cast_death_knight();

  spell_t::execute();

  if ( result_is_hit() )
  {
    if ( result == RESULT_CRIT )
    {
      p -> buffs_bloody_vengeance -> trigger();
    }
  }
  else
  {
    refund_power( this );
    refund_runes( p, use );
  }
}

// death_knight_spell_t::player_buff() ======================================

void death_knight_spell_t::player_buff()
{
  death_knight_t* p = player -> cast_death_knight();

  spell_t::player_buff();

  // TODO: merge this with death_knight_attack_t::player_buff to avoid
  // redundancy and errors when adding abilities.

  // Rivendare is spells and abilities... so everything *except* raw melee ( = special)
  // Things not affected: SS shadow damage in 3.3, wandering plague
  if ( ! proc && sim -> target -> debuffs.blood_plague -> up() )
    player_multiplier *= 1.0 + p -> talents.rage_of_rivendare * 0.02;
    
  // Fix Me! Does the same apply as for RoR?
  if ( ! proc && sim -> target -> debuffs.frost_fever -> up() )
    player_multiplier *= 1.0 + p -> talents.tundra_stalker * 0.03;

  if ( school == SCHOOL_PHYSICAL )
  {
    player_multiplier *= 1.0 + p -> talents.bloody_vengeance * 0.01 * p -> buffs_bloody_vengeance -> stack();
  }

  player_multiplier *= 1.0 + p -> buffs_blood_presence -> value();
  player_multiplier *= 1.0 + p -> buffs_bone_shield -> value();
  player_multiplier *= 1.0 +  p -> buffs_desolation -> value();
  if ( school == SCHOOL_FROST || school == SCHOOL_SHADOW )
    player_multiplier *= 1.0 + p -> talents.black_ice * 0.02;

  player_multiplier *= 1.0 + p -> talents.tundra_stalker * 0.03;
  player_multiplier *= 1.0 + p -> buffs_tier10_4pc_melee -> value();
  
  if ( sim -> debug )
    log_t::output( sim, "death_knight_spell_t::player_buff: %s hit=%.2f crit=%.2f power=%.2f penetration=%.0f, p_mult=%.0f",
                   name(), player_hit, player_crit, player_spell_power, player_penetration, player_multiplier );
}

// death_knight_spell_t::ready() ============================================

bool death_knight_spell_t::ready()
{
  death_knight_t* p = player -> cast_death_knight();

  if ( ! spell_t::ready() )
    return false;

  if ( exact_runic > 0 )
    if ( p -> resource_current[ RESOURCE_RUNIC ] != exact_runic )
      return false;

  if ( min_runic > 0 )
    if ( p -> resource_current[ RESOURCE_RUNIC ] < min_runic )
      return false;

  if ( max_runic > 0 )
    if ( p -> resource_current[ RESOURCE_RUNIC ] > max_runic )
      return false;

  double ct = sim -> current_time;

  if ( min_blood_plague > 0 )
    if ( ! p -> dots_blood_plague -> ticking() || ( ( p -> dots_blood_plague -> ready - ct ) < min_blood_plague ) )
      return false;

  if ( max_blood_plague > 0 )
    if ( p -> dots_blood_plague -> ticking() && ( ( p -> dots_blood_plague -> ready - ct ) > max_blood_plague ) )
      return false;

  if ( min_frost_fever > 0 )
    if ( ! p -> dots_frost_fever -> ticking() || ( ( p -> dots_frost_fever -> ready - ct ) < min_frost_fever ) )
      return false;

  if ( max_frost_fever > 0 )
    if ( p -> dots_frost_fever -> ticking() && ( ( p -> dots_frost_fever -> ready - ct ) > max_frost_fever ) )
      return false;

  if ( min_desolation > 0 )
    if ( ! p -> buffs_desolation || ( p -> buffs_desolation -> remains_lt( min_desolation ) ) )
      return false;

  if ( max_desolation > 0 )
    if ( p -> buffs_desolation && ( p -> buffs_desolation -> remains_gt( max_desolation ) ) )
      return false;

  int count = count_runes( p );
  int c = GET_BLOOD_RUNE_COUNT( count );
  if ( ( exact_blood != -1 && c != exact_blood ) ||
       ( min_blood != -1 && c < min_blood ) ||
       ( max_blood != -1 && c > max_blood ) ) return false;

  c = GET_FROST_RUNE_COUNT( count );
  if ( ( exact_frost != -1 && c != exact_frost ) ||
       ( min_frost != -1 && c < min_frost ) ||
       ( max_frost != -1 && c > max_frost ) ) return false;

  c = GET_UNHOLY_RUNE_COUNT( count );
  if ( ( exact_unholy != -1 && c != exact_unholy ) ||
       ( min_unholy != -1 && c < min_unholy ) ||
       ( max_unholy != -1 && c > max_unholy ) ) return false;

  c = count_death_runes( p );
  if ( ( exact_death != -1 && c != exact_death ) ||
       ( min_death != -1 && c < min_death ) ||
       ( max_death != -1 && c > max_death ) ) return false;

  if ( ! player -> in_combat && ! harmful )
    return group_runes( player, 0, 0, 0, use );
  else
    return group_runes( player, cost_blood, cost_frost, cost_unholy, use );
}

void death_knight_spell_t::target_debuff( int dmg_type )
{
  spell_t::target_debuff( dmg_type );
  death_knight_t* p = player -> cast_death_knight();
  if ( dmg_type == SCHOOL_FROST  )
  {
    target_multiplier *= 1.0 + p -> buffs_rune_of_razorice -> value();
  }
}


// =========================================================================
// Death Knight Attacks
// =========================================================================

// Melee Attack ============================================================

struct melee_t : public death_knight_attack_t
{
  int sync_weapons;
  melee_t( const char* name, player_t* player, int sw ) :
    death_knight_attack_t( name, player, SCHOOL_PHYSICAL, TREE_NONE, false ), sync_weapons( sw )
  {
    death_knight_t* p = player -> cast_death_knight();

    base_dd_min = base_dd_max = 1;
    may_glance      = true;
    background      = true;
    repeating       = true;
    trigger_gcd     = 0;
    base_cost       = 0;

    if ( p -> dual_wield() ) base_hit -= 0.19;
  }

  virtual double execute_time() SC_CONST
  {
    double t = death_knight_attack_t::execute_time();
    if ( ! player -> in_combat ) 
    {
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t/2, 0.2 ) : t/2 ) : 0.01;
    }
    return t;
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();

    death_knight_attack_t::execute();

    if ( result_is_hit() )
    {
      trigger_necrosis( this );
      trigger_blood_caked_blade( this );
      if ( p -> buffs_scent_of_blood -> up() )
      {
        p -> resource_gain( resource, 5, p -> gains_scent_of_blood );
        p -> buffs_scent_of_blood -> decrement();
      }
      // KM: 1/2/3/4/5 PPM proc, only auto-attacks
      double chance = weapon -> proc_chance_on_swing( p -> talents.killing_machine );
      p -> buffs_killing_machine -> trigger( 1, 1, chance );
    }
  }
};

// Auto Attack =============================================================

struct auto_attack_t : public death_knight_attack_t
{
  int sync_weapons;

  auto_attack_t( player_t* player, const std::string& options_str ) :
      death_knight_attack_t( "auto_attack", player ), sync_weapons( 0 )
  {
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { "sync_weapons", OPT_BOOL, &sync_weapons }
    };
    parse_options( options, options_str );

    assert( p -> main_hand_weapon.type != WEAPON_NONE );

    p -> main_hand_attack = new melee_t( "melee_main_hand", player, sync_weapons );
    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      p -> off_hand_attack = new melee_t( "melee_off_hand", player, sync_weapons );
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
    }

    trigger_gcd = 0;
  }
  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    p -> main_hand_attack -> schedule_execute();
    if ( p -> off_hand_attack ) 
    {
      p -> off_hand_attack -> schedule_execute();
    }
  }

  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> buffs.moving -> check() ) return false;
    return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// ==========================================================================
// Death Knight Abilities
// ==========================================================================

// Blood Boil ===============================================================
struct blood_boil_t : public death_knight_spell_t
{
  blood_boil_t( player_t* player, const std::string& options_str, bool sudden_doom = false ) :
      death_knight_spell_t( "blood_boil", player, SCHOOL_SHADOW, TREE_BLOOD )
  {
    death_knight_t* p = player -> cast_death_knight();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 78, 4, 180, 220, 0, 0 },
      { 72, 3, 149, 181, 0, 0 },
      { 66, 2, 116, 140, 0, 0 },
      { 58, 1, 89, 107, 0, 0 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    cost_blood = 1;

    base_execute_time = 0;
    cooldown -> duration          = 0.0;    
    base_crit_bonus_multiplier *= 1.0 + p -> talents.might_of_mograine * 0.15;

    direct_power_mod  = 0.06;
  }
  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    base_dd_adder = ( p -> diseases() ? 95 : 0 );
    direct_power_mod  = 0.06 + ( p -> diseases() ? 0.035 : 0 );
    death_knight_spell_t::execute();
  }

};

// Blood Plague =============================================================
struct blood_plague_t : public death_knight_spell_t
{
  blood_plague_t( player_t* player ) :
      death_knight_spell_t( "blood_plague", player, SCHOOL_SHADOW, TREE_UNHOLY )
  {
    death_knight_t* p = player -> cast_death_knight();

    static rank_t ranks[] =
    {
      { 55, 1, 0, 0, 36, 0 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    trigger_gcd       = 0;
    base_cost         = 0;
    base_execute_time = 0;
    base_tick_time    = 3.0;
    num_ticks         = 5 + p -> talents.epidemic;
    base_attack_power_multiplier *= 0.055 * 1.15 * ( 1 + 0.04 * p -> talents.impurity );
    tick_power_mod    = 1;

    tick_may_crit     = ( p -> set_bonus.tier9_4pc_melee() != 0 && ! p -> sim -> P330 );
    may_miss          = false;
    cooldown -> duration          = 0.0;
    
    reset();
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_spell_t::execute();

    target_t* t = sim -> target;
    added_ticks = 0;
    num_ticks = 5 + p -> talents.epidemic;
    if ( ! sim -> overrides.blood_plague ) 
    {
      t -> debuffs.blood_plague -> duration = 3.0 * num_ticks;
      t -> debuffs.blood_plague -> trigger();
    }
    trigger_crypt_fever( this );
    trigger_ebon_plaguebringer( this );
  }

  virtual void extend_duration( int extra_ticks )
  {
    death_knight_spell_t::extend_duration( extra_ticks );
    target_t* t = sim -> target;
    if ( ! sim -> overrides.blood_plague && t -> debuffs.blood_plague -> remains_lt( dot -> ready ) )
    {
      t -> debuffs.blood_plague -> duration = dot -> ready - sim -> current_time;
      t -> debuffs.blood_plague -> trigger();
    }
    trigger_crypt_fever( this );
    trigger_ebon_plaguebringer( this );    
  }

  virtual void refresh_duration()
  {
    death_knight_spell_t::refresh_duration();
    target_t* t = sim -> target;
    if ( ! sim -> overrides.blood_plague && t -> debuffs.blood_plague -> remains_lt( dot -> ready ) )
    {
      t -> debuffs.blood_plague -> duration = dot -> ready - sim -> current_time;
      t -> debuffs.blood_plague -> trigger();
    }
    trigger_crypt_fever( this );
    trigger_ebon_plaguebringer( this );    
  }
  
  virtual void tick()
  {
    death_knight_spell_t::tick();
    trigger_wandering_plague( this, tick_dmg );
  }

  virtual bool ready() { return false; }

  virtual void target_debuff( int dmg_type )
  {
    death_knight_spell_t::target_debuff( dmg_type );
    target_multiplier *= 1 + sim -> target -> debuffs.crypt_fever -> value() * 0.01;
  }
};

// Blood Strike =============================================================
struct blood_strike_t : public death_knight_attack_t
{
  blood_strike_t( player_t* player, const std::string& options_str  ) :
      death_knight_attack_t( "blood_strike", player, SCHOOL_PHYSICAL, TREE_BLOOD )
  {
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80,  6, 305.6, 305.6, 0, -10 },
      { 74,  5, 250,   250,   0, -10 },
      { 69,  4, 164.4, 164.4, 0, -10 },
      { 64,  3, 138.8, 138.8, 0, -10 },
      { 59,  2, 118,   118,   0, -10 },
      { 55,  1, 104,   104,   0, -10 },
      { 0,   0, 0,     0,     0, 0 }
    };
    init_rank( ranks );

    cost_blood = 1;

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier *= 0.4;

    base_crit += p -> talents.subversion * 0.03;
    base_crit_bonus_multiplier *= 1.0 + p -> talents.might_of_mograine * 0.15
                                      + p -> talents.guile_of_gorefiend * 0.15
                                      + p -> talents.blood_of_the_north / 3.0;

    base_multiplier *= 1 + p -> talents.bloody_strikes * 0.15;
    
    convert_runes = p -> talents.blood_of_the_north / 3.0
                  + p -> talents.reaping / 3.0;
  }

  virtual void target_debuff( int dmg_type )
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::target_debuff( dmg_type );
    target_multiplier *= 1 + p -> diseases() * 0.125 * ( 1.0 + p -> set_bonus.tier8_4pc_melee() * .2 );
  }

  virtual void consume_resource() { }
  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    weapon = &( p -> main_hand_weapon );
    may_miss = may_dodge = may_parry = true;
    death_knight_attack_t::execute();
    death_knight_attack_t::consume_resource();

    if ( result_is_hit() )
    {
      p -> buffs_tier9_2pc_melee -> trigger();
      trigger_abominations_might( this, 0.25 );
      trigger_sudden_doom( this );
      p -> buffs_desolation -> trigger( 1, p -> talents.desolation * 0.01 );
    }
    // 30/60/100% to also hit with OH
    double chance = util_t::talent_rank( p -> talents.threat_of_thassarian, 3, 0.30, 0.60, 1.0 );
    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      if ( p -> rng_threat_of_thassarian -> roll ( chance ) )
      {
        group_runes( p, 0, 0, 0, use );
        may_miss = may_dodge = may_parry = false;
        weapon = &( p -> off_hand_weapon );
        death_knight_attack_t::execute();
      }
  }
};

// Blood Tap ================================================================
struct blood_tap_t : public death_knight_spell_t
{
  blood_tap_t( player_t* player, const std::string& options_str ) :
      death_knight_spell_t( "blood_tap", player, SCHOOL_NONE, TREE_BLOOD )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    cooldown -> duration    = 60.0;
    trigger_gcd = 0;
    base_cost   = 0;
  }

  void execute()
  {
    death_knight_spell_t::execute();

    death_knight_t* p = player -> cast_death_knight();
    double t = p -> sim -> current_time;

    // If you have a non-death blood rune, it will convert and activate that one.
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    {
      dk_rune_t& r = p -> _runes.slot[i];
      if ( r.get_type() == RUNE_TYPE_BLOOD && ! r.is_death() )
      {
        r.type = ( r.type & RUNE_TYPE_MASK ) | RUNE_TYPE_DEATH;
        r.cooldown_ready = 0;
        return;
      }
    }
    // If all your blood runes were already death runes, it will activate one.
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    {
      dk_rune_t& r = p -> _runes.slot[i];
      if ( r.get_type() == RUNE_TYPE_BLOOD && ! r.is_ready( t ) )
      {
        r.cooldown_ready = 0;
        return;
      }
    }
  }
};

// Bone Shield ==============================================================
struct bone_shield_t : public death_knight_spell_t
{
  bone_shield_t( player_t* player, const std::string& options_str ) :
      death_knight_spell_t( "bone_shield", player, SCHOOL_SHADOW, TREE_UNHOLY )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    
    death_knight_t* p = player -> cast_death_knight();
    
    check_talent( p -> talents.bone_shield );

    cost_unholy = 1;
    harmful     = false;
    base_cost   = -10;
    
    cooldown -> duration    = 60.0;
  }
  virtual double cost() SC_CONST 
  { 
    if ( player -> in_combat )
      return death_knight_spell_t::cost();
        
    return 0;
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> in_combat )
    {
      // Pre-casting it before the fight, perfect timing would be so that the
      // used rune is ready when it is needed in the rotation.
      // Assume we casted Bone Shield somewhere between 8-16s before we start fighting
      double pre_cast = p -> sim -> range( 8.0, 16.0 );

      cooldown -> duration -= pre_cast;
      p -> buffs_bone_shield -> duration -= pre_cast;

      p -> buffs_bone_shield -> trigger( 1, 0.02 );
      death_knight_spell_t::execute();

      cooldown -> duration = 60;
      p -> buffs_bone_shield -> duration = 60;
    }
    else
    {
      p -> buffs_bone_shield -> trigger( 1, 0.02 );
      death_knight_spell_t::execute();
    }
  }
};

// Dancing Rune Weapon ======================================================
struct dancing_rune_weapon_t : public death_knight_spell_t
{
  dancing_rune_weapon_t( player_t* player, const std::string& options_str ) :
      death_knight_spell_t( "dancing_rune_weapon", player, SCHOOL_NONE, TREE_BLOOD )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    base_cost   = 60.0;
    cooldown -> duration    = 90.0;
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_spell_t::execute();
    p -> buffs_dancing_rune_weapon -> trigger();
    p -> summon_pet( "dancing_rune_weapon", 12.0 + 5.0 * p -> glyphs.dancing_rune_weapon );
  }

  virtual bool ready()
  {
    if ( ! death_knight_spell_t::ready() )
      return false;

    death_knight_t* p = player -> cast_death_knight();
    return p -> talents.dancing_rune_weapon != 0;
  }
};

// Death Coil ===============================================================
struct death_coil_t : public death_knight_spell_t
{
  death_coil_t( player_t* player, const std::string& options_str, bool sudden_doom = false ) :
      death_knight_spell_t( "death_coil", player, SCHOOL_SHADOW, TREE_UNHOLY )
  {
    death_knight_t* p = player -> cast_death_knight();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 5, 443, 443, 0, 40 },
      { 76, 4, 381, 381, 0, 40 },
      { 68, 3, 275, 275, 0, 40 },
      { 62, 2, 208, 208, 0, 40 },
      { 55, 1, 184, 184, 0, 40 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    base_execute_time = 0;
    cooldown -> duration          = 0.0;
    direct_power_mod  = 0.15 * ( 1 + 0.04 * p -> talents.impurity );
    base_dd_multiplier *= 1 + ( 0.05 * p -> talents.morbidity +
                                0.15 * p -> glyphs.dark_death );
    base_crit += p -> set_bonus.tier8_2pc_melee() * 0.08;
    base_dd_adder = 380 * p -> sigils.vengeful_heart;

                               
    if ( sudden_doom )
    {
      proc = true;
      base_cost = 0;
      trigger_gcd = 0;
    }
  }

  void execute()
  {
    death_knight_spell_t::execute();
    if ( result_is_hit() ) trigger_unholy_blight( this, direct_dmg );
  }

  bool ready()
  {
    if ( proc ) return false;

    return death_knight_spell_t::ready();
  }
};

// Death Strike =============================================================
struct death_strike_t : public death_knight_attack_t
{
  death_strike_t( player_t* player, const std::string& options_str  ) :
      death_knight_attack_t( "death_strike", player, SCHOOL_PHYSICAL, TREE_UNHOLY )
  {
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80,  5, 222.75, 222.75, 0, -15 },
      { 75,  4, 150,    150,    0, -15 },
      { 70,  3, 123.75, 123.75, 0, -15 },
      { 63,  2, 97.5,   97.5,   0, -15 },
      { 56,  1, 84,     84,     0, -15 },
      { 0,   0, 0,      0,      0,  0 }
    };
    init_rank( ranks );

    cost_frost = 1;
    cost_unholy = 1;

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier *= 0.75;

    base_dd_adder = p -> sigils.awareness * 315;
    base_crit += p -> talents.improved_death_strike * 0.03;
    base_crit_bonus_multiplier *= 1.0 + p -> talents.might_of_mograine * 0.15;
    base_multiplier *= 1 + p -> talents.improved_death_strike * 0.15;
    convert_runes = p -> talents.death_rune_mastery / 3.0;
  }

  virtual void consume_resource() { }
  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    weapon = &( p -> main_hand_weapon );
    may_miss = may_dodge = may_parry = true;
    death_knight_attack_t::execute();
    death_knight_attack_t::consume_resource();

    if ( p -> buffs_dancing_rune_weapon -> check() )
       p -> active_dancing_rune_weapon -> drw_death_strike -> execute();

    if ( result_is_hit() )
    {
      p -> buffs_sigil_virulence -> trigger();

      if ( p -> talents.dirge )
        p -> resource_gain( RESOURCE_RUNIC, 2.5 * p -> talents.dirge, p -> gains_dirge );

      trigger_abominations_might( this, 0.5 );
    }
    // 30/60/100% to also hit with OH
    double chance = util_t::talent_rank( p -> talents.threat_of_thassarian, 3, 0.30, 0.60, 1.0 );
    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      if ( p -> rng_threat_of_thassarian -> roll ( chance ) )
      {
        group_runes( p, 0, 0, 0, use );
        may_miss = may_dodge = may_parry = false;
        weapon = &( p -> off_hand_weapon );
        death_knight_attack_t::execute();
      }
  }
};

// Deathchill ======================================================
struct deathchill_t : public death_knight_spell_t
{
  deathchill_t( player_t* player, const std::string& options_str ) :
  death_knight_spell_t( "deathchill", player, SCHOOL_NONE, TREE_FROST )
  {
    death_knight_t* p = player -> cast_death_knight();
    
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    
    check_talent( p -> talents.deathchill );
    
    trigger_gcd = 0;
    cooldown -> duration = 120;
  }
  
  void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_deathchill -> trigger( 1, 1 );
    update_ready();
  }
};

// Empower Rune Weapon ======================================================
struct empower_rune_weapon_t : public death_knight_spell_t
{
  empower_rune_weapon_t( player_t* player, const std::string& options_str ) :
      death_knight_spell_t( "empower_rune_weapon", player, SCHOOL_NONE, TREE_FROST )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
    cooldown -> duration = 300;
  }

  void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_spell_t::execute();

    for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    {
      dk_rune_t& r = p -> _runes.slot[i];
      r.cooldown_ready = 0;
    }
    p -> resource_gain( RESOURCE_RUNIC, 25, p -> gains_rune_abilities );
  }
};

// Frost Fever ==============================================================
struct frost_fever_t : public death_knight_spell_t
{
  frost_fever_t( player_t* player ) :
      death_knight_spell_t( "frost_fever", player, SCHOOL_FROST, TREE_FROST )
  {
    death_knight_t* p = player -> cast_death_knight();

    static rank_t ranks[] =
    {
      { 55, 1, 0, 0, 29, 0 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    trigger_gcd       = 0;
    base_cost         = 0;
    base_execute_time = 0;
    base_tick_time    = 3.0;
    num_ticks         = 5 + p -> talents.epidemic;
    base_attack_power_multiplier *= 0.055 * 1.15 * ( 1 + 0.04 * p -> talents.impurity );
    base_multiplier  *= 1.0 + ( ( p -> sim -> P330 && p -> glyphs.icy_touch ) ? 0.2 : 0.0 );
    tick_power_mod    = 1;

    tick_may_crit     = p -> set_bonus.tier9_4pc_melee() != 0;
    may_miss          = false;
    cooldown -> duration          = 0.0;

    reset();
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_spell_t::execute();

    target_t* t = sim -> target;
    added_ticks = 0;
    num_ticks = 5 + p -> talents.epidemic;
    if ( ! sim -> overrides.frost_fever ) 
    {
      t -> debuffs.frost_fever -> duration = 3.0 * num_ticks;
      t -> debuffs.frost_fever -> trigger();
    }
    trigger_crypt_fever( this );
    trigger_ebon_plaguebringer( this );
  }

  virtual void extend_duration( int extra_ticks )
  {
    death_knight_spell_t::extend_duration( extra_ticks );
    target_t* t = sim -> target;
    if ( ! sim -> overrides.frost_fever && t -> debuffs.frost_fever -> remains_lt( dot -> ready ) )
    {
      t -> debuffs.frost_fever -> duration = dot -> ready - sim -> current_time;
      t -> debuffs.frost_fever -> trigger();
    }
    trigger_crypt_fever( this );
    trigger_ebon_plaguebringer( this );    
  }

  virtual void refresh_duration()
  {
    death_knight_spell_t::refresh_duration();
    target_t* t = sim -> target;
    if ( ! sim -> overrides.frost_fever && t -> debuffs.frost_fever -> remains_lt( dot -> ready ) )
    {
      t -> debuffs.frost_fever -> duration = dot -> ready - sim -> current_time;
      t -> debuffs.frost_fever -> trigger();
    }
    trigger_crypt_fever( this );
    trigger_ebon_plaguebringer( this );    
  }
  virtual void tick()
  {
    death_knight_spell_t::tick();
    trigger_wandering_plague( this, tick_dmg );
  }

  virtual void target_debuff( int dmg_type )
  {
    death_knight_spell_t::target_debuff( dmg_type );
    target_multiplier *= 1 + sim -> target -> debuffs.crypt_fever -> value() * 0.01;
  }

  virtual bool ready()     { return false; }
};

// Frost Strike =============================================================
struct frost_strike_t : public death_knight_attack_t
{
  int killing_machine;
  frost_strike_t( player_t* player, const std::string& options_str  ) :
      death_knight_attack_t( "frost_strike", player, SCHOOL_FROST, TREE_FROST ), killing_machine( 0 )
  {
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { "killing_machine", OPT_INT, &killing_machine },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    check_talent( p -> talents.frost_strike );

    static rank_t ranks[] =
    {
      { 80,  6, 137.5, 137.5, 0, 40 },
      { 75,  5, 110.6, 110.6, 0, 40 },
      { 70,  4,  78.1,  78.1, 0, 40 },
      { 65,  3,  63.5,  73.5, 0, 40 },
      { 60,  2,  56.7,  56.7, 0, 40 },
      { 55,  1,  47.9,  47.9, 0, 40 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier     *= 0.55;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.guile_of_gorefiend * 0.15;
    base_multiplier *= 1.0 + p -> talents.blood_of_the_north / 3.0;
    base_crit += p -> set_bonus.tier8_2pc_melee() * 0.08;
    base_cost -= p -> glyphs.frost_strike * 8;
    base_dd_adder = 113 * p -> sigils.vengeful_heart;
  }
  
  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();
    if ( killing_machine && ! p -> buffs_killing_machine -> check() )
      return false;
      
    return death_knight_attack_t::ready();
  }

  virtual void consume_resource() { }
  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    weapon = &( p -> main_hand_weapon );
    may_miss = may_dodge = may_parry = true;
    death_knight_attack_t::execute();
    death_knight_attack_t::consume_resource();

    double chance = util_t::talent_rank( p -> talents.threat_of_thassarian, 3, 0.30, 0.60, 1.0 );
    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      if ( p -> rng_threat_of_thassarian -> roll ( chance ) )
      {
        weapon = &( p -> off_hand_weapon );
        may_miss = may_dodge = may_parry = false;
        death_knight_attack_t::execute();
      }
    p -> buffs_killing_machine -> expire();
    p -> buffs_deathchill -> expire();
  }
  
  virtual void player_buff()
  {
    death_knight_attack_t::player_buff();
    death_knight_t* p = player -> cast_death_knight();
    
    player_crit += p -> buffs_killing_machine -> value();
    
    player_crit += p -> buffs_deathchill -> value();
    
    if ( p -> diseases() ) player_multiplier *= 1 + 0.20 * ( p -> talents.glacier_rot / 3.0 );

    if ( sim -> target -> health_percentage() < 35 )
      player_multiplier *= 1 + 0.04 * p -> talents.merciless_combat;
  }
};

// Heart Strike =============================================================
struct heart_strike_t : public death_knight_attack_t
{
  heart_strike_t( player_t* player, const std::string& options_str  ) :
      death_knight_attack_t( "heart_strike", player, SCHOOL_PHYSICAL, TREE_BLOOD )
  {
    death_knight_t* p = player -> cast_death_knight();
    check_talent( p -> talents.heart_strike );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80,  6, 368,   368,   0, -10 },
      { 74,  5, 300.5, 300.5, 0, -10 },
      { 69,  4, 197.5, 197.5, 0, -10 },
      { 64,  3, 167,   167,   0, -10 },
      { 59,  2, 142,   142,   0, -10 },
      { 55,  1, 125,   125,   0, -10 },
      { 0,   0, 0,     0,     0,  0 }
    };
    init_rank( ranks );

    cost_blood = 1;

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier     *= 0.5;

    base_crit += p -> talents.subversion * 0.03;
    base_crit_bonus_multiplier *= 1.0 * p -> talents.might_of_mograine * 0.15;
    base_multiplier *= 1 + p -> talents.bloody_strikes * 0.15;
    base_multiplier *= 1 + p -> set_bonus.tier10_2pc_melee() * 0.07;
    check_talent( p -> talents.heart_strike );
  }

  void target_debuff( int dmg_type )
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::target_debuff( dmg_type );
    target_multiplier *= 1 + p -> diseases() * 0.1 * ( 1.0 + p -> set_bonus.tier8_4pc_melee() * .2 );
  }

  void execute()
  {
    death_knight_attack_t::execute();
    death_knight_t* p = player -> cast_death_knight();

    if ( p -> buffs_dancing_rune_weapon -> check() )
       p -> active_dancing_rune_weapon -> drw_heart_strike -> execute();

    if ( result_is_hit() )
    {
      p -> buffs_tier9_2pc_melee -> trigger();
      trigger_abominations_might( this, 0.25 );
      trigger_sudden_doom( this );
    }
  }
};

// Horn of Winter============================================================
struct horn_of_winter_t : public death_knight_spell_t
{
  double bonus;
  int rebuff_early;

  horn_of_winter_t( player_t* player, const std::string& options_str ) :
      death_knight_spell_t( "horn_of_winter", player, SCHOOL_NONE, TREE_FROST ),
      rebuff_early( 0 )
  {
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { "rebuff_early", OPT_INT, &rebuff_early },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    cooldown -> duration       = 20;
    trigger_gcd    = 1.0;
    bonus = util_t::ability_rank( p -> level, 155,75, 86,65 );
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    update_ready();
    death_knight_t* p = player -> cast_death_knight();
    if ( ! sim -> overrides.horn_of_winter )
      sim -> auras.horn_of_winter -> duration = 120 + p -> glyphs.horn_of_winter * 60;
    sim -> auras.horn_of_winter -> trigger( 1, bonus );

    player -> resource_gain( RESOURCE_RUNIC, 10, p -> gains_horn_of_winter );
  }

  virtual bool ready()
  {
    if ( ! death_knight_spell_t::ready() )
      return false;

    // In case they're using it for the runic power
    if ( rebuff_early )
    {
      return true;
    }

    return sim -> auras.strength_of_earth -> current_value < bonus;
  }

  virtual double gcd() SC_CONST { return player -> in_combat ? death_knight_spell_t::gcd() : 0; }
};

// Howling Blast ============================================================
struct howling_blast_t : public death_knight_spell_t
{
  int killing_machine, rime;
  howling_blast_t( player_t* player, const std::string& options_str ) :
      death_knight_spell_t( "howling_blast", player, SCHOOL_FROST, TREE_FROST ), killing_machine( 0 ), rime( 0 )
  {
    
    death_knight_t* p = player -> cast_death_knight();
    check_talent( p -> talents.howling_blast );
    option_t options[] =
    {
      { "killing_machine", OPT_INT, &killing_machine },
      { "rime",            OPT_INT, &rime            },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 4, 527, 562, 0, -15 },
      { 75, 3, 441, 479, 0, -15 },
      { 70, 2, 324, 352, 0, -15 },
      { 60, 1, 198, 214, 0, -15 },
      {  0, 0,   0,   0, 0,   0 }
    };
    init_rank( ranks );

    base_execute_time = 0;
    cooldown -> duration          = 8.0;
    direct_power_mod  = 0.2;
    
    base_crit_bonus_multiplier *= 1.0 + p -> talents.guile_of_gorefiend * 0.15;
   
    cost_unholy = 1;
    cost_frost  = 1;
  }
  
  virtual double cost() SC_CONST
  {
    // Rime also prevents getting RP because there are no runes used!
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> buffs_rime -> check() ) return 0;
    return death_knight_spell_t::cost();
  } 
  
  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> buffs_rime -> up() )
    {
      // Don't use any runes if Rime is up
      group_runes( p, 0, 0, 0, use );
    }
    death_knight_spell_t::execute();

    if ( result_is_hit() )
    {
      if ( p -> glyphs.howling_blast )
      {
        if ( ! p -> frost_fever )
          p -> frost_fever = new frost_fever_t( p );

        p -> frost_fever -> execute();
      }
      p -> resource_gain( RESOURCE_RUNIC, 2.5 * p -> talents.chill_of_the_grave, p -> gains_chill_of_the_grave );
    }
    p -> buffs_killing_machine -> expire();
    p -> buffs_deathchill -> expire();
    p -> buffs_rime -> expire();
  }

  virtual void player_buff()
  {
    death_knight_spell_t::player_buff();
    death_knight_t* p = player -> cast_death_knight();

    player_crit += p -> buffs_killing_machine -> value();

    player_crit += p -> buffs_deathchill -> value();

    if ( p -> diseases() ) player_multiplier *= 1 + 0.20 * ( p -> talents.glacier_rot / 3.0 );

    if ( sim -> target -> health_percentage() < 35 )
      player_multiplier *= 1 + 0.04 * p -> talents.merciless_combat;
  }
  
  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();

    if ( killing_machine && ! p -> buffs_killing_machine -> check() )
      return false;

    if ( p -> buffs_rime -> check() )
    {
      // If Rime is up, runes are no restriction.
      cost_unholy = 0;
      cost_frost  = 0;
      bool rime_ready = death_knight_spell_t::ready();
      cost_unholy = 1;
      cost_frost  = 1;
      return rime_ready;
    }
    else if ( rime )
    {
      return false;
    }
    return death_knight_spell_t::ready();
  }
};

// Hysteria =================================================================
struct hysteria_t : public action_t
{
  player_t* hysteria_target;

  hysteria_t( player_t* player, const std::string& options_str ) :
      action_t( ACTION_OTHER, "hysteria", player, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_BLOOD ), hysteria_target( 0 )
  {
    death_knight_t* p = player -> cast_death_knight();
    check_talent( p -> talents.hysteria );

    std::string target_str = p -> hysteria_target_str;
    option_t options[] =
    {
      { "target", OPT_STRING, &target_str },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    
    check_talent( p -> talents.hysteria );
    if ( target_str.empty() )
    {
      hysteria_target = p;
    }
    else
    {
      hysteria_target = sim -> find_player( target_str );
      assert ( hysteria_target != 0 );
    }

    trigger_gcd = 0;
    cooldown -> duration = 180;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(),name() );
    consume_resource();
    update_ready();
    hysteria_target -> buffs.hysteria -> trigger( 1 );
  }

};

// Icy Touch ================================================================
struct icy_touch_t : public death_knight_spell_t
{
  int killing_machine;
  icy_touch_t( player_t* player, const std::string& options_str ) :
      death_knight_spell_t( "icy_touch", player, SCHOOL_FROST, TREE_FROST ), killing_machine( 0 )
  {
    death_knight_t* p = player -> cast_death_knight();
    option_t options[] =
    {
      { "killing_machine", OPT_INT, &killing_machine },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 78, 5, 227, 245, 0, -10 },
      { 73, 4, 187, 203, 0, -10 },
      { 67, 3, 161, 173, 0, -10 },
      { 61, 2, 144, 156, 0, -10 },
      { 55, 1, 127, 137, 0, -10 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    cost_frost = 1;

    base_execute_time = 0;
    direct_power_mod  = 0.1 * ( 1 + 0.04 * p -> talents.impurity );
    cooldown -> duration          = 0.0;
    
    base_crit += p -> talents.rime * 0.05;
  }

  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();
    if ( killing_machine && ! p -> buffs_killing_machine -> check() )
      return false;
      
    return death_knight_spell_t::ready();
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();

    death_knight_spell_t::execute();
    if ( result_is_hit() )
    {
      if ( ! p -> sim -> P330 && p -> glyphs.icy_touch )
        p -> resource_gain( RESOURCE_RUNIC, 10, p -> gains_glyph_icy_touch );

      p -> resource_gain( RESOURCE_RUNIC, 2.5 * p -> talents.chill_of_the_grave, p -> gains_chill_of_the_grave );

      if ( ! p -> frost_fever )
        p -> frost_fever = new frost_fever_t( p );
      p -> frost_fever -> execute();
      
      trigger_icy_talons( this );
    }
    p -> buffs_killing_machine -> expire();
    p -> buffs_deathchill -> expire();
  }
  
  virtual void player_buff()
  {
    death_knight_spell_t::player_buff();
    death_knight_t* p = player -> cast_death_knight();
    
    player_crit += p -> buffs_killing_machine -> value();
    
    player_crit += p -> buffs_deathchill -> value();
    
    if ( p -> diseases() ) 
      player_multiplier *= 1 + 0.20 * ( p -> talents.glacier_rot / 3.0 );
    
    if ( sim -> target -> health_percentage() < 35 )
      player_multiplier *= 1 + 0.04 * p -> talents.merciless_combat;
  }
};

// Obliterate ===============================================================
struct obliterate_t : public death_knight_attack_t
{
  obliterate_t( player_t* player, const std::string& options_str  ) :
      death_knight_attack_t( "obliterate", player, SCHOOL_PHYSICAL, TREE_FROST )
  {
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 79,  4, 467.2, 467.2, 0, -15 },
      { 73,  3, 381.6, 381.6, 0, -15 },
      { 67,  2, 244,   244,   0, -15 },
      { 61,  1, 198.4, 198.4, 0, -15 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    cost_frost = 1;
    cost_unholy = 1;

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier     *= 0.8;
    
    base_multiplier *= 1.0 + p -> set_bonus.tier10_2pc_melee() * 0.1;
    base_multiplier *= 1.0 + p -> glyphs.obliterate * 0.2;
    base_dd_adder = p -> sigils.awareness * 336;
    base_crit += p -> talents.subversion * 0.03;
    base_crit += p -> talents.rime * 0.05;
    base_crit_bonus_multiplier *= 1.0 + p -> talents.guile_of_gorefiend * 0.15;
    convert_runes = p -> talents.death_rune_mastery / 3;
  }

  virtual void target_debuff( int dmg_type )
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::target_debuff( dmg_type );
    target_multiplier *= 1 + p -> diseases() * 0.125 * ( 1.0 + p -> set_bonus.tier8_4pc_melee() * .2 );
  }

  virtual void player_buff()
  {
    death_knight_attack_t::player_buff();
    death_knight_t* p = player -> cast_death_knight();
    
    player_crit += p -> buffs_deathchill -> value();

    if ( sim -> target -> health_percentage() < 35 )
      player_multiplier *= 1 + 0.04 * p -> talents.merciless_combat;
  }

  virtual void consume_resource() { }
  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    weapon = &( p -> main_hand_weapon );
    may_miss = may_dodge = may_parry = true;
    death_knight_attack_t::execute();
    death_knight_attack_t::consume_resource();

    if ( result_is_hit() )
    {
      trigger_abominations_might( this, 0.5 );

      double chance = p -> talents.annihilation / 3.0;
      // 33/66/100% chance to NOT consume the diseases
      if ( ! p -> rng_annihilation -> roll( chance ) )
      {
        p -> dots_blood_plague -> reset();
        p -> dots_frost_fever -> reset();
      }
      p -> resource_gain( RESOURCE_RUNIC, 2.5 * p -> talents.chill_of_the_grave, p -> gains_chill_of_the_grave );
      p -> buffs_sigil_virulence -> trigger();

      if ( p -> buffs_rime -> trigger() ) 
      {
        p -> cooldowns_howling_blast -> reset();
        update_ready();
      }
    }
    
    // 30/60/100% to also hit with OH
    double chance = util_t::talent_rank( p -> talents.threat_of_thassarian, 3, 0.30, 0.60, 1.0 );
    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      if ( p -> rng_threat_of_thassarian -> roll ( chance ) )
      {
        group_runes( p, 0, 0, 0, use );
        may_miss = may_dodge = may_parry = false;
        weapon = &( p -> off_hand_weapon );
        death_knight_attack_t::execute();
        if ( p -> buffs_rime -> trigger() ) 
        {
          p -> cooldowns_howling_blast -> reset();
          update_ready();
        }
      }

    p -> buffs_deathchill -> expire();
  }
};

// Pestilence ===============================================================
struct pestilence_t : public death_knight_spell_t
{
  pestilence_t( player_t* player, const std::string& options_str ) :
      death_knight_spell_t( "pestilence", player, SCHOOL_PHYSICAL, TREE_UNHOLY )
  {
    death_knight_t* p = player -> cast_death_knight();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    
    base_cost     = -10;
    cost_blood    = 1;
    convert_runes = p -> talents.blood_of_the_north / 3.0
                  + p -> talents.reaping / 3.0;
  }
  
  virtual void execute()
  {
    death_knight_spell_t::execute();
    death_knight_t* p = player -> cast_death_knight();
    if ( result_is_hit() && p -> glyphs.disease )
    {
      if ( p -> dots_blood_plague -> ticking() )
      {
        if ( sim -> P330 ) 
          p -> dots_blood_plague -> action -> player_buff();
        
        p -> dots_blood_plague -> action -> refresh_duration();
      }
      if ( p -> dots_frost_fever -> ticking() )
      {
        if ( sim -> P330 ) 
          p -> dots_frost_fever -> action -> player_buff();
        
        p -> dots_frost_fever -> action -> refresh_duration();
      }
    }
  }
};

// Plague Strike ============================================================
struct plague_strike_t : public death_knight_attack_t
{
  plague_strike_t( player_t* player, const std::string& options_str  ) :
      death_knight_attack_t( "plague_strike", player, SCHOOL_PHYSICAL, TREE_UNHOLY )
  {
    death_knight_t* p = player -> cast_death_knight();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80,  6, 189,  189,  0, -10 },
      { 75,  5, 157,  157,  0, -10 },
      { 70,  4, 108,  108,  0, -10 },
      { 65,  3, 89,   89,   0, -10 },
      { 60,  2, 75.5, 75.5, 0, -10 },
      { 55,  1, 62.5, 62.5, 0, -10 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    cost_unholy = 1;

    base_crit += p -> talents.vicious_strikes * 0.03;
    base_crit_bonus_multiplier *= 1.0 + ( p -> talents.vicious_strikes * 0.15 );
    base_multiplier *= 1.0 + ( p -> talents.outbreak * 0.10 );

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier     *= 0.50;
  }

  virtual void consume_resource() { }
  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    weapon = &( p -> main_hand_weapon );
    may_miss = may_dodge = may_parry = true;
    death_knight_attack_t::execute();
    death_knight_attack_t::consume_resource();

    if ( result_is_hit() )
    {
      // 30/60/100% to also hit with OH
      double chance = util_t::talent_rank( p -> talents.threat_of_thassarian, 3, 0.30, 0.60, 1.0 );
      if ( p -> off_hand_weapon.type != WEAPON_NONE )
        if ( p -> rng_threat_of_thassarian -> roll ( chance ) )
        {
          group_runes( p, 0, 0, 0, use );
          may_miss = may_dodge = may_parry = false;
          weapon = &( p -> off_hand_weapon );
          death_knight_attack_t::execute();
        }

      if ( p -> talents.dirge )
      {
        p -> resource_gain( RESOURCE_RUNIC, 2.5 * p -> talents.dirge, p -> gains_dirge );
      }

      if ( ! p -> blood_plague )
        p -> blood_plague = new blood_plague_t( p );
      p -> blood_plague -> execute();
    }
  }
};

// Presence =================================================================
struct presence_t : public death_knight_spell_t
{
  int switch_to_presence;
  presence_t( player_t* player, const std::string& options_str ) :
      death_knight_spell_t( "presence", player, SCHOOL_PHYSICAL, TREE_UNHOLY )
  {
    std::string presence_str;
    option_t options[] =
    {
      { "choose",  OPT_STRING, &presence_str },
      { NULL,     OPT_UNKNOWN, NULL          }
    };
    parse_options( options, options_str );

    cost_unholy = 0;
    cost_blood  = 0;
    cost_frost  = 0;
    if ( ! presence_str.empty() )
    {
      if ( presence_str == "blood" || presence_str == "bp" )
      {
        switch_to_presence = PRESENCE_BLOOD;
        cost_blood  = 1;
      }
      else if ( presence_str == "frost" || presence_str == "fp" )
      {
        switch_to_presence = PRESENCE_FROST;
        cost_frost  = 1;
      }
      else if ( presence_str == "unholy" || presence_str == "up" )
      {
        switch_to_presence = PRESENCE_UNHOLY;
        cost_unholy = 1;
      }
    }
    else
    {
      // Default to Blood Presence
      switch_to_presence = PRESENCE_BLOOD;
      cost_blood  = 1;
    }

    base_cost   = 0;
    trigger_gcd = 0;
    cooldown -> duration    = 1.0;
    harmful     = false;
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();
    death_knight_t* p = player -> cast_death_knight();
    // If not in combat, just do it without consuming runes.
    if ( ! p -> in_combat )
      group_runes( p, 0, 0, 0, use );
    
    p -> base_gcd = 1.50;
    
    switch ( p -> active_presence )
    {
      case PRESENCE_BLOOD:  p -> buffs_blood_presence  -> expire(); break;
      case PRESENCE_FROST:  p -> buffs_frost_presence  -> expire(); break;
      case PRESENCE_UNHOLY: p -> buffs_unholy_presence -> expire(); break;
    }
    p -> active_presence = switch_to_presence;

    switch ( p -> active_presence )
    {
      case PRESENCE_BLOOD:  p -> buffs_blood_presence  -> trigger( 1, 0.15 ); break;
      case PRESENCE_FROST:  p -> buffs_frost_presence  -> trigger( 1, 0.60 ); break;
      case PRESENCE_UNHOLY:
        p -> buffs_unholy_presence -> trigger( 1, 0.15 );
        p -> base_gcd = 1.0;
        break;

    }
    p -> reset_gcd();
  }

  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> active_presence == switch_to_presence )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Raise Dead ===============================================================
struct raise_dead_t : public death_knight_spell_t
{
  raise_dead_t( player_t* player, const std::string& options_str ) :
      death_knight_spell_t( "raise_dead", player, SCHOOL_NONE, TREE_UNHOLY )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    cooldown -> duration   = 180.0;
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();

    consume_resource();
    update_ready();
    player -> summon_pet( "ghoul", p -> talents.master_of_ghouls ? 0.0 : 60.0 );
  }
  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> active_ghoul ) return false;
    
    return death_knight_spell_t::ready();
  }
};

// Rune Tap =================================================================
struct rune_tap_t : public death_knight_spell_t
{
  rune_tap_t( player_t* player, const std::string& options_str ) :
      death_knight_spell_t( "rune_tap", player, SCHOOL_NONE, TREE_BLOOD )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    cost_blood = 1;

    cooldown -> duration    = 60.0;
    trigger_gcd = 0;
  }

  void execute()
  {
    death_knight_spell_t::execute();

    death_knight_t* p = player -> cast_death_knight();
    double pctOfMaxGained = 0.1;
    pctOfMaxGained += ( p -> talents.improved_rune_tap / 3 ) * 0.1;
    pctOfMaxGained *= 1.0 + ( p -> glyphs.rune_tap * 0.1 );
    p -> resource_gain( RESOURCE_HEALTH, pctOfMaxGained * p -> resource_max[ RESOURCE_HEALTH ] );
  }
};

// Scourge Strike ===========================================================
struct scourge_strike_t : public death_knight_attack_t
{
  attack_t* scourge_strike_shadow;
  scourge_strike_t( player_t* player, const std::string& options_str  ) :
      death_knight_attack_t( "scourge_strike", player, SCHOOL_SHADOW, TREE_UNHOLY )
  {
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 79,  4, 317.5, 317.5, 0, -15 },
      { 73,  3, 259,   259,   0, -15 },
      { 67,  2, 166,   166,   0, -15 },
      { 55,  1, 135,   135,   0, -15 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );
    check_talent( p -> talents.scourge_strike );

    cost_frost = 1;
    cost_unholy = 1;

    base_dd_adder = p -> sigils.awareness * 187;
    base_crit += p -> talents.subversion * 0.03;
    base_crit += p -> talents.vicious_strikes * 0.03;
    base_crit_bonus_multiplier *= 1.0 + ( p -> talents.vicious_strikes * 0.15 );
    base_multiplier *= 1.0 + ( 0.2 * p -> talents.outbreak / 3.0 );
    base_multiplier *= 1.0 + p -> set_bonus.tier10_2pc_melee() * 0.1;

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    if ( ! p -> sim -> P330 )
      weapon_multiplier *= 0.40;
    else
    {
      struct scourge_strike_shadow_t : public death_knight_attack_t
      {
        scourge_strike_shadow_t( player_t* player ) : death_knight_attack_t( "scourge_strike_shadow", player, SCHOOL_SHADOW, TREE_UNHOLY )
        {
          death_knight_t* p = player -> cast_death_knight();

          may_miss    = false;
          proc        = true;
          background  = true;
          trigger_gcd = 0;
          
          // Only blizzard knows, but for the shadowpart in 3.3 the follwing
          // +x% buffs are ADDITIVE (which this flag controls)
          // Bone Shield 2%, Desolation 5%, Blood Presence 15%, Black Ice 10%
          additive_factors = true;
          
          base_attack_power_multiplier = 0;
          base_dd_min = base_dd_max    = 1;

          base_crit += p -> talents.subversion * 0.03;
          base_crit += p -> talents.vicious_strikes * 0.03;
          base_crit_bonus_multiplier *= 1.0 + ( p -> talents.vicious_strikes * 0.15 );
        }

        virtual void target_debuff( int dmg_type )
        {
          // for each of your diseases on your target, you deal an 
          // additional 25% of the Physical damage done as Shadow damage.
          death_knight_t* p = player -> cast_death_knight();
          death_knight_attack_t::target_debuff( dmg_type );

          // FIX ME!! How does 4T8 play with SS in 3.3
          target_multiplier *= p -> diseases() * 0.25 * ( 1.0 + p -> set_bonus.tier8_4pc_melee() * .2 );
        }
      };
      scourge_strike_shadow = new scourge_strike_shadow_t( player );

      stats -> school = school  = SCHOOL_PHYSICAL;
      weapon_multiplier        *= 0.50;
      base_dd_min = base_dd_max = 400;
    }
  }

  void execute()
  {
    death_knight_attack_t::execute();
    if ( result_is_hit() )
    {
      death_knight_t* p = player -> cast_death_knight();
      if ( p -> sim -> P330 )
      {
        scourge_strike_shadow -> base_dd_adder = direct_dmg;
        scourge_strike_shadow -> execute();
      }
      
      p -> buffs_sigil_virulence -> trigger();
      if ( p -> talents.dirge )
      {
        p -> resource_gain( RESOURCE_RUNIC, 2.5 * p -> talents.dirge, p -> gains_dirge );
      }

      if ( p -> glyphs.scourge_strike )
      {
        if (  p -> dots_blood_plague -> ticking() )
          if (  p -> dots_blood_plague -> action -> added_ticks < 3 )
            p -> dots_blood_plague -> action -> extend_duration( 1 );

        if (  p -> dots_frost_fever -> ticking() )
          if (  p -> dots_frost_fever -> action -> added_ticks < 3 )
            p -> dots_frost_fever -> action -> extend_duration( 1 );
      }
    }
  }

  void target_debuff( int dmg_type )
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::target_debuff( dmg_type );
    if ( ! p -> sim -> P330 )
      target_multiplier *= 1 + p -> diseases() * 0.10 * ( 1.0 + p -> set_bonus.tier8_4pc_melee() * .2 );
  }
};

// Summon Gargoyle ==========================================================
struct summon_gargoyle_t : public death_knight_spell_t
{

  summon_gargoyle_t( player_t* player, const std::string& options_str ) :
      death_knight_spell_t( "summon_gargoyle", player, SCHOOL_SHADOW, TREE_UNHOLY )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    base_cost   = 60;
    cooldown -> duration    = 180.0;
    trigger_gcd = 0;
    
    death_knight_t* p = player -> cast_death_knight();
    check_talent( p -> talents.summon_gargoyle );
  }

  virtual void execute()
  {
    consume_resource();
    update_ready();
    player -> summon_pet( "gargoyle", 30.0 );
  }
};


// Unbreakable Armor ========================================================
struct unbreakable_armor_t : public death_knight_spell_t
{
  unbreakable_armor_t( player_t* player, const std::string& options_str ) :
      death_knight_spell_t( "unbreakable_armor", player, SCHOOL_NONE, TREE_FROST )
  {
    death_knight_t* p = player -> cast_death_knight();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    check_talent( p -> talents.unbreakable_armor );

    cost_frost = 1;
    cooldown -> duration = 60.0;
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();

    death_knight_t* p = player -> cast_death_knight();
    p -> buffs_unbreakable_armor -> trigger( 1, 0.25 );
  }
};

} // ANONYMOUS NAMESPACE ===================================================

// ==========================================================================
// Death Knight Character Definition
// ==========================================================================

// death_knight_t::create_action  =================================================

action_t* death_knight_t::create_action( const std::string& name, const std::string& options_str )
{
  // General Actions
  if ( name == "auto_attack"              ) return new auto_attack_t              ( this, options_str );
  if ( name == "presence"                 ) return new presence_t                 ( this, options_str );

  // Blood Actions
  if ( name == "blood_boil"               ) return new blood_boil_t               ( this, options_str );
//if ( name == "blood_presence"           ) return new blood_presence_t           ( this, options_str );
  if ( name == "blood_strike"             ) return new blood_strike_t             ( this, options_str );
  if ( name == "blood_tap"                ) return new blood_tap_t                ( this, options_str );
  if ( name == "dancing_rune_weapon"      ) return new dancing_rune_weapon_t      ( this, options_str );
//if ( name == "dark_command"             ) return new dark_command_t             ( this, options_str );
//if ( name == "death_pact"               ) return new death_pact_t               ( this, options_str );
//if ( name == "forceful_deflection"      ) return new forceful_deflection_t      ( this, options_str );  Passive
  if ( name == "heart_strike"             ) return new heart_strike_t             ( this, options_str );
  if ( name == "hysteria"                 ) return new hysteria_t                 ( this, options_str );
//if ( name == "improved_blood_presence"  ) return new improved_blood_presence_t  ( this, options_str );  Passive
//if ( name == "mark_of_blood"            ) return new mark_of_blood_t            ( this, options_str );
  if ( name == "pestilence"               ) return new pestilence_t               ( this, options_str );
  if ( name == "rune_tap"                 ) return new rune_tap_t                 ( this, options_str );
//if ( name == "strangulate"              ) return new strangulate_t              ( this, options_str );
//if ( name == "vampiric_blood"           ) return new vampiric_blood_t           ( this, options_str );

  // Frost Actions
//if ( name == "chains_of_ice"            ) return new chains_of_ice_t            ( this, options_str );
  if ( name == "empower_rune_weapon"      ) return new empower_rune_weapon_t      ( this, options_str );
  if ( name == "deathchill"               ) return new deathchill_t               ( this, options_str );
//if ( name == "frost_fever"              ) return new frost_fever_t              ( this, options_str );  Passive
//if ( name == "frost_presence"           ) return new frost_presence_t           ( this, options_str );
  if ( name == "frost_strike"             ) return new frost_strike_t             ( this, options_str );
  if ( name == "horn_of_winter"           ) return new horn_of_winter_t           ( this, options_str );
  if ( name == "howling_blast"            ) return new howling_blast_t            ( this, options_str );
//if ( name == "icebound_fortitude"       ) return new icebound_fortitude_t       ( this, options_str );
  if ( name == "icy_touch"                ) return new icy_touch_t                ( this, options_str );
//if ( name == "improved_frost_presence"  ) return new improved_frost_presence_t  ( this, options_str );  Passive
//if ( name == "mind_freeze"              ) return new mind_freeze_t              ( this, options_str );
  if ( name == "obliterate"               ) return new obliterate_t               ( this, options_str );
//if ( name == "path_of_frost"            ) return new path_of_frost_t            ( this, options_str );  Non-Combat
//if ( name == "rune_strike"              ) return new rune_strike_t              ( this, options_str );
//if ( name == "runic_focus"              ) return new runic_focus_t              ( this, options_str );  Passive
  if ( name == "unbreakable_armor"        ) return new unbreakable_armor_t        ( this, options_str );

  // Unholy Actions
//if ( name == "anti_magic_shell"         ) return new anti_magic_shell_t         ( this, options_str );
//if ( name == "anti_magic_zone"          ) return new anti_magic_zone_t          ( this, options_str );
//if ( name == "army_of_the_dead"         ) return new army_of_the_dead_t         ( this, options_str );
//if ( name == "blood_plague"             ) return new blood_plague_t             ( this, options_str );  Passive
  if ( name == "bone_shield"              ) return new bone_shield_t              ( this, options_str );
//if ( name == "corpse_explosion"         ) return new corpse_explosion_t         ( this, options_str );
//if ( name == "death_and_decay"          ) return new death_and_decay_t          ( this, options_str );
  if ( name == "death_coil"               ) return new death_coil_t               ( this, options_str );
//if ( name == "death_gate"               ) return new death_gate_t               ( this, options_str );  Non-Combat
//if ( name == "death_grip"               ) return new death_grip_t               ( this, options_str );
  if ( name == "death_strike"             ) return new death_strike_t             ( this, options_str );
//if ( name == "ghoul_frenzy"             ) return new ghoul_frenzy_t             ( this, options_str );
//if ( name == "improved_unholy_presence" ) return new improved_unholy_presence_t ( this, options_str );  Passive
  if ( name == "plague_strike"            ) return new plague_strike_t            ( this, options_str );
//if ( name == "raise_ally"               ) return new raise_ally_t               ( this, options_str );
  if ( name == "raise_dead"               ) return new raise_dead_t               ( this, options_str );
  if ( name == "scourge_strike"           ) return new scourge_strike_t           ( this, options_str );
  if ( name == "summon_gargoyle"          ) return new summon_gargoyle_t          ( this, options_str );
//if ( name == "unholy_blight"            ) return new unholy_blight_t            ( this, options_str );
//if ( name == "unholy_presence"          ) return new unholy_presence_t          ( this, options_str );

  return player_t::create_action( name, options_str );
}

// death_knight_t::create_pets ==============================================

void death_knight_t::create_pets()
{
  create_pet( "bloodworms" );
  create_pet( "dancing_rune_weapon" );
  create_pet( "gargoyle" );
  create_pet( "ghoul" );
}

// death_knight_t::create_pet ===============================================

pet_t* death_knight_t::create_pet( const std::string& pet_name )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "bloodworms"          ) return new bloodworms_pet_t          ( sim, this );
  if ( pet_name == "dancing_rune_weapon" ) return new dancing_rune_weapon_pet_t ( sim, this );
  if ( pet_name == "gargoyle"            ) return new gargoyle_pet_t            ( sim, this );
  if ( pet_name == "ghoul"               ) return new ghoul_pet_t               ( sim, this );

  return 0;
}

// death_knight_t::composite_armor ===========================================

double death_knight_t::composite_armor() SC_CONST
{
  double armor = player_t::composite_armor();
  armor *= 1.0 + buffs_unbreakable_armor -> value();
  return armor;
}

// death_knight_t::composite_attack_haste() ================================

double death_knight_t::composite_attack_haste() SC_CONST
{
  double haste = player_t::composite_attack_haste();
  haste *= 1.0/ ( 1.0 + buffs_unholy_presence -> value());
  
  if ( talents.improved_icy_talons )
    haste *= 1.0/ ( 1.0 + 0.05 );
  
  // Icy Talons give the DK 20%/20s, Imp.IT makes that buff raidwide, which
  // are two different buff that don't stack (does not stack with WF totem)
  // If you got 16% WF totem, 20% IT but not ITT you will gain 20% haste.
  // I can't make up a case where you would go 5/5 IT but not take IIT.
  double it_haste = buffs_icy_talons -> value();
  if ( it_haste > sim -> auras.windfury_totem -> current_value && ! sim -> auras.improved_icy_talons -> check() )
  {
    haste *= ( 1.0 + sim -> auras.windfury_totem -> current_value );
    haste *= 1.0/ ( 1.0 + it_haste );
  }
    
  return haste;
}

// death_knight_t::init ======================================================

void death_knight_t::init()
{
  player_t::init();

  if ( pet_t* ghoul = find_pet( "ghoul" ) )
  {
    ghoul -> type = PLAYER_GUARDIAN;
  }
}

// death_knight_t::init_race =================================================

void death_knight_t::init_race()
{
  race = util_t::parse_race_type( race_str );
  switch ( race )
  {
  case RACE_HUMAN:
  case RACE_DWARF:
  case RACE_DRAENEI:
  case RACE_NIGHT_ELF:
  case RACE_GNOME:
  case RACE_UNDEAD:
  case RACE_ORC:
  case RACE_TROLL:
  case RACE_TAUREN:
  case RACE_BLOOD_ELF:
    break;
  default:
    race = RACE_NIGHT_ELF;
    race_str = util_t::race_type_string( race );
  }

  player_t::init_race();
}

void death_knight_t::init_rng()
{
  player_t::init_rng();
  rng_annihilation         = get_rng( "annihilation" );
  rng_blood_caked_blade    = get_rng( "blood_caked_blade" );
  rng_threat_of_thassarian = get_rng( "threat_of_thassarian");
  rng_wandering_plague     = get_rng( "wandering_plague" );
}

void death_knight_t::init_base()
{
  attribute_base[ ATTR_STRENGTH  ] = rating_t::get_attribute_base( sim, level, DEATH_KNIGHT, race, BASE_STAT_STRENGTH );
  attribute_base[ ATTR_AGILITY   ] = rating_t::get_attribute_base( sim, level, DEATH_KNIGHT, race, BASE_STAT_AGILITY );
  attribute_base[ ATTR_STAMINA   ] = rating_t::get_attribute_base( sim, level, DEATH_KNIGHT, race, BASE_STAT_STAMINA );
  attribute_base[ ATTR_INTELLECT ] = rating_t::get_attribute_base( sim, level, DEATH_KNIGHT, race, BASE_STAT_INTELLECT );
  attribute_base[ ATTR_SPIRIT    ] = rating_t::get_attribute_base( sim, level, DEATH_KNIGHT, race, BASE_STAT_SPIRIT );
  resource_base[ RESOURCE_HEALTH ] = rating_t::get_attribute_base( sim, level, DEATH_KNIGHT, race, BASE_STAT_HEALTH );
  base_attack_crit                 = rating_t::get_attribute_base( sim, level, DEATH_KNIGHT, race, BASE_STAT_MELEE_CRIT );
  initial_attack_crit_per_agility  = rating_t::get_attribute_base( sim, level, DEATH_KNIGHT, race, BASE_STAT_MELEE_CRIT_PER_AGI );

  attribute_multiplier_initial[ ATTR_STRENGTH ] *= 1.0 + ( talents.veteran_of_the_third_war * 0.02 +
                                                           talents.abominations_might * 0.01 +
                                                           talents.ravenous_dead * 0.01 );
  attribute_multiplier_initial[ ATTR_STAMINA ]  *= 1.0 + talents.veteran_of_the_third_war * 0.02;

  // For some reason, my buffless, naked Death Knight Human with
  // 180str (3% bonus) has 583 AP.  No talent or buff can explain this
  // discrepency, but it is also present on other Death Knights I have
  // checked.  TODO: investigate this further.
  base_attack_power = 220;
  base_attack_expertise = 0.01 * ( talents.veteran_of_the_third_war * 2 +
                                   talents.tundra_stalker +
                                   talents.rage_of_rivendare );

  initial_attack_power_per_strength = 2.0;

  resource_base[ RESOURCE_RUNIC ]  = 100 + talents.runic_power_mastery * 15;

  health_per_stamina = 10;
  base_gcd = 1.5;
}

void death_knight_t::init_actions()
{
  if ( action_list_str.empty() )
  {
    action_list_str  = "flask,type=endless_rage";
    action_list_str += "/food,type=dragonfin_filet";
    action_list_str += "/presence,choose=blood";
    action_list_str += "/snapshot_stats";
    action_list_str += "/auto_attack";
    int num_items = ( int ) items.size();
    for ( int i=0; i < num_items; i++ )
    {
      if ( items[ i ].use.active() )
      {
        action_list_str += "/use_item,name=";
        action_list_str += items[ i ].name();
        action_list_str += ",time>=10";
      }
    }
    if ( race == RACE_DWARF )
    {
      if ( talents.bladed_armor > 0 )
        action_list_str += "/stoneform";
    }
    else if ( race == RACE_ORC )
    {
      action_list_str += "/blood_fury";
    }
    else if ( race == RACE_TROLL )
    {
      action_list_str += "/berserking";
    }
    else if ( race == RACE_BLOOD_ELF )
    {
      action_list_str += "/arcane_torrent";
    }
    // Use slightly into the fight, when debuffs are up
    action_list_str += ",time>=10";

    if ( primary_tree() == TREE_UNHOLY )
    {
      // TODO: Add Empower Rune Weapon
      action_list_str += "/raise_dead";
      action_list_str += "/sequence,name=unholy,wait_on_ready=1";
      action_list_str += ":icy_touch";
      action_list_str += ":plague_strike";
      action_list_str += ":blood_strike";
      action_list_str += ":scourge_strike";
      action_list_str += ":blood_strike";
      action_list_str += "/summon_gargoyle,time<=20";
      action_list_str += "/summon_gargoyle,if=buff.bloodlust.react";
      action_list_str += "/death_coil";
      action_list_str += "/horn_of_winter";
      action_list_str += "/restart_sequence,name=unholy";
    }
    else if ( primary_tree() == TREE_BLOOD )
    {
      // TODO: Add Empower Rune Weapon
      action_list_str += "/hysteria,if=buff.bloodlust.react";
      action_list_str += "/dancing_rune_weapon,if=buff.bloodlust.react";
      action_list_str += "/sequence,name=blood1,wait_on_ready=1";
      action_list_str += ":icy_touch";
      action_list_str += ":plague_strike";
      action_list_str += ":heart_strike";
      action_list_str += ":heart_strike";
      action_list_str += ":death_strike";
      action_list_str += ":blood_strike";
      action_list_str += ":raise_dead,wait_on_ready=0";
      action_list_str += "/death_coil";
      action_list_str += "/sequence,name=blood2,wait_on_ready=1";
      action_list_str += ":death_strike";
      action_list_str += ":heart_strike";
      action_list_str += ":heart_strike";
      action_list_str += ":heart_strike";
      action_list_str += ":heart_strike";
      action_list_str += "/restart_sequence,name=blood1";
      action_list_str += "/restart_sequence,name=blood2";
    }
    else if ( primary_tree() == TREE_FROST )
    {
      /*Rotation:
      IT PS OB BS BS Dump
      OB OB OB Dump
      
      Priority Rotation (most important at the top):
      - Frost Fever
      - Blood Plague
      - Killing Machine + Rime
      - Obliterate
      - Blood Strike
      - Frost Strike
      */
      // UA 'lags' in updating armor, so first ghoul should be a few
      // seconds after it, second ghoud then with bloodlust
      
      if ( talents.unbreakable_armor ) 
        action_list_str += "/unbreakable_armor,time>=10";

      action_list_str += "/raise_dead,time>=15,time<=40"; 
      action_list_str += "/raise_dead,if=buff.bloodlust.react"; 

      if ( glyphs.disease )
      {
        action_list_str += "/icy_touch,frost_fever<=0.1";
        action_list_str += "/plague_strike,if=dot.blood_plague.remains<=0.1";
        action_list_str += "/pestilence,if=dot.blood_plague.remains<=2";
        action_list_str += "/pestilence,if=dot.frost_fever.remains<=2";
      }
      else
      {
        action_list_str += "/icy_touch,if=dot.frost_fever.remains<=2";
        action_list_str += "/plague_strike,if=dot.blood_plague.remains<=2";
      }
      if ( talents.howling_blast )
        action_list_str += "/howling_blast,if=buff.rime.react&buff.killing_machine.react";
      if ( talents.frost_strike ) 
        action_list_str += "/frost_strike,if=buff.killing_machine.react";
      if ( talents.deathchill )
        action_list_str += "/deathchill";
      action_list_str += "/obliterate";
      action_list_str += "/blood_strike,blood=2,death<=2";
      action_list_str += "/blood_strike,blood=1,death<=1";
      if ( talents.frost_strike ) 
        action_list_str += "/frost_strike";
      if ( talents.howling_blast )
        action_list_str += "/howling_blast,if=buff.rime.react";
      action_list_str += "/empower_rune_weapon";
      action_list_str += "/horn_of_winter";
    }
    action_list_default = 1;
  }

  player_t::init_actions();
}

void death_knight_t::init_enchant()
{
  player_t::init_enchant();
  
  std::string& mh_enchant = items[ SLOT_MAIN_HAND ].encoded_enchant_str;
  std::string& oh_enchant = items[ SLOT_OFF_HAND  ].encoded_enchant_str;

  // Rune of the Fallen Crusader =======================================
  struct fallen_crusader_callback_t : public action_callback_t
  {
    int slot;
    buff_t* buff;
  
    fallen_crusader_callback_t( player_t* p, int s, buff_t* b ) : action_callback_t( p -> sim, p ), slot(s), buff(b) {}
  
    virtual void trigger( action_t* a )
    {
      weapon_t* w = a -> weapon;
      if ( ! w ) return;
      if ( w -> slot != slot ) return;
  
      // RotFC is 2 PPM.
      double PPM        = 2.0;
      double swing_time = a -> time_to_execute;
      double chance     = w -> proc_chance_on_swing( PPM, swing_time );
  
      buff -> trigger( 1, 0.15, chance );
    }
  };
  
  // Rune of the Razorice =======================================
  
  // Damage Procc
  struct razorice_spell_t : public death_knight_spell_t
  {
    razorice_spell_t( player_t* player ) : death_knight_spell_t( "razorice", player, SCHOOL_FROST, TREE_FROST )
    {
      may_miss    = false;
      may_crit    = false; // FIXME!!  Can the damage crit?
      may_resist  = true;
      background  = true;
      proc        = true;
      trigger_gcd = 0;
      base_dd_min = base_dd_max = 0.01;
      base_dd_multiplier *= 0.02;
      reset();
    }
  };

  struct razorice_callback_t : public action_callback_t
  {
    int slot;
    buff_t* buff;
    spell_t* razorice_damage_proc;
  
    razorice_callback_t( player_t* p, int s, buff_t* b ) : action_callback_t( p -> sim, p ), slot(s), buff(b), razorice_damage_proc( 0 )
    {
      razorice_damage_proc = new razorice_spell_t( p );
    }
  
    virtual void trigger( action_t* a )
    {
      weapon_t* w = a -> weapon;
      if ( ! w ) return;
      if ( w -> slot != slot ) return;

      // http://elitistjerks.com/f72/t64830-dw_builds_3_2_revenge_offhand/p28/#post1332820
      // double PPM        = 2.0;
      // double swing_time = a -> time_to_execute;
      // double chance     = w -> proc_chance_on_swing( PPM, swing_time );
      buff -> trigger( 1, 0.01, 0.30 );
      
      razorice_damage_proc -> base_dd_min = w -> min_dmg;
      razorice_damage_proc -> base_dd_max = w -> max_dmg;
      razorice_damage_proc -> execute();
    }
  };
  buffs_rune_of_razorice = new buff_t( this, "rune_of_razorice", 10, 20.0);
  buffs_rune_of_the_fallen_crusader = new buff_t( this, "rune_of_the_fallen_crusader", 1, 15.0);
  if ( mh_enchant == "rune_of_the_fallen_crusader" )
  {
    register_attack_result_callback( RESULT_HIT_MASK, new fallen_crusader_callback_t( this, SLOT_MAIN_HAND, buffs_rune_of_the_fallen_crusader ) );
  }
  else if( mh_enchant == "rune_of_razorice" )
  {
    register_attack_result_callback( RESULT_HIT_MASK, new razorice_callback_t( this, SLOT_MAIN_HAND, buffs_rune_of_razorice ) );
  }
  if ( oh_enchant == "rune_of_the_fallen_crusader" )
  {
    register_attack_result_callback( RESULT_HIT_MASK, new fallen_crusader_callback_t( this, SLOT_OFF_HAND, buffs_rune_of_the_fallen_crusader ) );
  }
  else if( oh_enchant == "rune_of_razorice" )
  {
    register_attack_result_callback( RESULT_HIT_MASK, new razorice_callback_t( this, SLOT_OFF_HAND, buffs_rune_of_razorice ) );
  }

}

void death_knight_t::init_glyphs()
{
  memset( ( void* ) &glyphs, 0x0, sizeof( glyphs_t ) );

  std::vector<std::string> glyph_names;
  int num_glyphs = util_t::string_split( glyph_names, glyphs_str, ",/" );

  for ( int i=0; i < num_glyphs; i++ )
  {
    std::string& n = glyph_names[ i ];

    // Major Glyphs
    if      ( n == "anti_magic_shell"    ) glyphs.anti_magic_shell = 1;
    else if ( n == "blood_strike"        ) glyphs.blood_strike = 1;
    else if ( n == "bone_shield"         ) glyphs.bone_shield = 1;
    else if ( n == "chains_of_ice"       ) glyphs.chains_of_ice = 1;
    else if ( n == "dancing_rune_weapon" ) glyphs.dancing_rune_weapon = 1;
    else if ( n == "dark_command"        ) glyphs.dark_command = 1;
    else if ( n == "dark_death"          ) glyphs.dark_death = 1;
    else if ( n == "death_and_decay"     ) glyphs.death_and_decay = 1;
    else if ( n == "death_grip"          ) glyphs.death_grip = 1;
    else if ( n == "death_strike"        ) glyphs.death_strike = 1;
    else if ( n == "disease"             ) glyphs.disease = 1;
    else if ( n == "frost_strike"        ) glyphs.frost_strike = 1;
    else if ( n == "heart_strike"        ) glyphs.heart_strike = 1;
    else if ( n == "howling_blast"       ) glyphs.howling_blast = 1;
    else if ( n == "hungering_cold"      ) glyphs.hungering_cold = 1;
    else if ( n == "icebound_fortitude"  ) glyphs.icebound_fortitude = 1;
    else if ( n == "icy_touch"           ) glyphs.icy_touch = 1;
    else if ( n == "obliterate"          ) glyphs.obliterate = 1;
    else if ( n == "plague_strike"       ) glyphs.plague_strike = 1;
    else if ( n == "rune_strike"         ) glyphs.rune_strike = 1;
    else if ( n == "rune_tap"            ) glyphs.rune_tap = 1;
    else if ( n == "scourge_strike"      ) glyphs.scourge_strike = 1;
    else if ( n == "strangulate"         ) glyphs.strangulate = 1;
    else if ( n == "the_ghoul"           ) glyphs.the_ghoul = 1;
    else if ( n == "unbreakable_armor"   ) glyphs.unbreakable_armor = 1;
    else if ( n == "unholy_blight"       ) glyphs.unholy_blight = 1;
    else if ( n == "vampiric_blood"      ) glyphs.vampiric_blood = 1;

    // Minor Glyphs
    else if ( n == "blood_tap"           ) glyphs.blood_tap = 1;
    else if ( n == "corpse_explosion"    ) glyphs.corpse_explosion = 1;
    else if ( n == "deaths_embrace"      ) glyphs.deaths_embrace = 1;
    else if ( n == "horn_of_winter"      ) glyphs.horn_of_winter = 1;
    else if ( n == "pestilence"          ) glyphs.pestilence = 1;
    else if ( n == "raise_dead"          ) glyphs.raise_dead = 1;

    else if ( ! sim -> parent ) util_t::fprintf( sim -> output_file, "simulationcraft: Player %s has unrecognized glyph %s\n", name(), n.c_str() );
  }
}

void death_knight_t::init_scaling()
{
  player_t::init_scaling();

  if ( talents.bladed_armor > 0 )
      scales_with[ STAT_ARMOR ] = 1;
  
  if ( off_hand_weapon.type != WEAPON_NONE ) {
      scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = 1;
      scales_with[ STAT_WEAPON_OFFHAND_SPEED  ] = 1;
  }
}

void death_knight_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( sim, player, name, max_stack, duration, cooldown, proc_chance, quiet )
  buffs_blood_presence      = new buff_t( this, "blood_presence" );
  buffs_frost_presence      = new buff_t( this, "frost_presence" );
  buffs_unholy_presence     = new buff_t( this, "unholy_presence" );
  buffs_bloody_vengeance    = new buff_t( this, "bloody_vengeance",    3,                                30.0,  0.0, talents.bloody_vengeance );
  buffs_bone_shield         = new buff_t( this, "bone_shield",         4 + glyphs.bone_shield,           60.0,  0.0, talents.bone_shield );
  buffs_dancing_rune_weapon = new buff_t( this, "dancing_rune_weapon", 1, 12 + 5 * glyphs.dancing_rune_weapon,  0.0, 1, true ); 
  buffs_desolation          = new buff_t( this, "desolation",          1,                                20.0,  0.0, talents.desolation );
  buffs_deathchill          = new buff_t( this, "deathchill",          1,                                30.0 );
  buffs_icy_talons          = new buff_t( this, "icy_talons",          1,                                20.0,  0.0, talents.icy_talons );
  buffs_killing_machine     = new buff_t( this, "killing_machine",     1,                                30.0,  0.0, 0 ); // PPM based!
  buffs_rime                = new buff_t( this, "rime",                1,                                30.0,  0.0, talents.rime * 0.05 );
  buffs_scent_of_blood      = new buff_t( this, "scent_of_blood",      talents.scent_of_blood,            0.0, 10.0, talents.scent_of_blood ? 0.15 : 0.00 );
  buffs_tier10_4pc_melee    = new buff_t( this, "tier10_4pc_melee",    1,                                15.0,  0.0, set_bonus.tier10_4pc_melee() );
  buffs_unbreakable_armor   = new buff_t( this, "unbreakable_armor",   1,                                20.0, 60.0, talents.unbreakable_armor );

  // stat_buff_t( sim, player, name, stat, amount, max_stack, duration, cooldown, proc_chance, quiet )
  buffs_sigil_hanged_man   = new stat_buff_t( this, "sigil_of_the_hanged_man", STAT_STRENGTH ,  73, 3, 15.0,   0, sigils.hanged_man );
  buffs_sigil_virulence    = new stat_buff_t( this, "sigil_of_virulence",      STAT_STRENGTH , 200, 1, 20.0,   0, sigils.virulence   * 0.80 );
  buffs_tier9_2pc_melee    = new stat_buff_t( this, "tier9_2pc_melee",         STAT_STRENGTH , 180, 1, 15.0,  45, set_bonus.tier9_2pc_melee() * 0.50 );

  struct bloodworms_buff_t : public buff_t
  {
    bloodworms_buff_t( death_knight_t* p ) :
        buff_t( p, "bloodworms", 1, 19.99, 20.01, p -> talents.bloodworms * 0.03 ) {}
    virtual void start( int stacks, double value )
    {
      buff_t::start( stacks, value );
      player -> summon_pet( "bloodworms" );
    }
    virtual void expire()
    {
      buff_t::expire();
      death_knight_t* p = player -> cast_death_knight();
      if ( p -> active_bloodworms ) p -> active_bloodworms -> dismiss();
    }
  };
  buffs_bloodworms = new bloodworms_buff_t( this );
}

void death_knight_t::init_gains()
{
  player_t::init_gains();

  gains_butchery           = get_gain( "butchery" );
  gains_chill_of_the_grave = get_gain( "chill_of_the_grave" );
  gains_dirge              = get_gain( "dirge" );
  gains_glyph_icy_touch    = get_gain( "glyph_icy_touch" );
  gains_horn_of_winter     = get_gain( "horn_of_winter" );
  gains_power_refund       = get_gain( "power_refund" );
  gains_rune_abilities     = get_gain( "rune_abilities" );
  gains_scent_of_blood     = get_gain( "scent_of_blood" );
}

void death_knight_t::init_procs()
{
  player_t::init_procs();

  procs_abominations_might = get_proc( "abominations_might", sim );
  procs_sudden_doom        = get_proc( "sudden_doom",        sim );
}

void death_knight_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resource_current[ RESOURCE_RUNIC ] = 0;
}

void death_knight_t::init_uptimes()
{
  player_t::init_uptimes();

  uptimes_blood_plague       = get_uptime( "blood_plague" );
  uptimes_frost_fever        = get_uptime( "frost_fever" );
}

void death_knight_t::init_items()
{
  player_t::init_items();

  std::string& sigil = items[ SLOT_RANGED ].encoded_name_str;

  if ( sigil.empty() ) return;

  if      ( sigil == "deadly_gladiators_sigil_of_strife"     ) sigils.deadly_strife = 1;
  else if ( sigil == "furious_gladiators_sigil_of_strife"    ) sigils.furious_strife = 1;
  else if ( sigil == "hateful_gladiators_sigil_of_strife"    ) sigils.hateful_strife = 1;
  else if ( sigil == "relentless_gladiators_sigil_of_strife" ) sigils.relentless_strife = 1;
  else if ( sigil == "savage_gladiators_sigil_of_strife"     ) sigils.savage_strife = 1;
  else if ( sigil == "sigil_of_arthritic_binding"            ) sigils.arthritic_binding = 1;
  else if ( sigil == "sigil_of_awareness"                    ) sigils.awareness = 1;
  else if ( sigil == "sigil_of_deflection"                   ) sigils.deflection = 1;
  else if ( sigil == "sigil_of_haunted_dreams"               ) sigils.haunted_dreams = 1;
  else if ( sigil == "sigil_of_insolence"                    ) sigils.insolence = 1;
  else if ( sigil == "sigil_of_the_dark_rider"               ) sigils.dark_rider = 1;
  else if ( sigil == "sigil_of_the_frozen_conscience"        ) sigils.frozen_conscience = 1;
  else if ( sigil == "sigil_of_the_hanged_man"               ) sigils.hanged_man = 1;
  else if ( sigil == "sigil_of_the_unfaltering_knight"       ) sigils.unfaltering_knight = 1;
  else if ( sigil == "sigil_of_the_vengeful_heart"           ) sigils.vengeful_heart = 1;
  else if ( sigil == "sigil_of_the_wild_buck"                ) sigils.wild_buck = 1;
  else if ( sigil == "sigil_of_virulence"                    ) sigils.virulence = 1;
  else
  {
    log_t::output( sim, "simulationcraft: %s has unknown sigil %s", name(), sigil.c_str() );
  }
}

// death_knight_t::init_rating =======================================================

void death_knight_t::init_rating()
{
  player_t::init_rating();

  rating.attack_haste *= 1.0 / 1.30;
}

void death_knight_t::reset()
{
  player_t::reset();

  // Active
  active_presence = 0;

  // Pets and Guardians
  active_bloodworms          = NULL;
  active_dancing_rune_weapon = NULL;
  active_ghoul               = NULL;

  _runes.reset();
}

void death_knight_t::combat_begin()
{
  player_t::combat_begin();
}

int death_knight_t::target_swing()
{
  int result = player_t::target_swing();

  buffs_scent_of_blood -> trigger( 3 );

  return result;
}

double death_knight_t::composite_attack_power() SC_CONST
{
  double ap = player_t::composite_attack_power();
  if ( talents.bladed_armor )
  {
    ap += talents.bladed_armor * composite_armor() / 180;
  }
  return ap;
}

double death_knight_t::composite_attribute_multiplier( int attr ) SC_CONST
{
  double m = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_STRENGTH )
  {
    m *= 1.0 + buffs_rune_of_the_fallen_crusader -> value();
    if ( buffs_unbreakable_armor -> check() )
      m *= 1.10;
  }

  return m;
}

double death_knight_t::composite_spell_hit() SC_CONST
{
  double hit = player_t::composite_spell_hit();
  hit += 0.01 * talents.virulence;
  return hit;
}

void death_knight_t::regen( double periodicity )
{
  player_t::regen( periodicity );

  if ( talents.butchery ) 
    resource_gain( RESOURCE_RUNIC, ( periodicity / talents.butchery ), gains_butchery );

  uptimes_blood_plague -> update( dots_blood_plague -> ticking() );
  uptimes_frost_fever  -> update( dots_frost_fever -> ticking() );
}

// death_knight_t::primary_tree =================================================

int death_knight_t::primary_tree() SC_CONST
{
  if ( talents.heart_strike   ) return TREE_BLOOD;
  if ( talents.frost_strike   ) return TREE_FROST;
  if ( talents.scourge_strike ) return TREE_UNHOLY;

  return TALENT_TREE_MAX;
}

// death_knight_t::get_translation_list ==============================================

std::vector<talent_translation_t>& death_knight_t::get_talent_list()
{
  if ( talent_list.empty() )
  {
    talent_translation_t translation_table[][MAX_TALENT_TREES] =
    {
      { {  1, 2, &( talents.butchery                         ) }, {  1, 3, &( talents.improved_icy_touch      ) }, {  1, 2, &( talents.vicious_strikes          ) } },
      { {  2, 3, &( talents.subversion                       ) }, {  2, 2, &( talents.runic_power_mastery     ) }, {  2, 3, &( talents.virulence                ) } },
      { {  3, 5, &( talents.blade_barrier                    ) }, {  3, 5, &( talents.toughness               ) }, {  3, 5, &( talents.anticipation             ) } },
      { {  4, 5, &( talents.bladed_armor                     ) }, {  4, 2, &( talents.icy_reach               ) }, {  4, 2, &( talents.epidemic                 ) } },
      { {  5, 3, &( talents.scent_of_blood                   ) }, {  5, 5, &( talents.black_ice               ) }, {  5, 3, &( talents.morbidity                ) } },
      { {  6, 2, &( talents.two_handed_weapon_specialization ) }, {  6, 3, &( talents.nerves_of_cold_steel    ) }, {  6, 2, &( talents.unholy_command           ) } },
      { {  7, 1, &( talents.rune_tap                         ) }, {  7, 5, &( talents.icy_talons              ) }, {  7, 3, &( talents.ravenous_dead            ) } },
      { {  8, 5, &( talents.dark_conviction                  ) }, {  8, 1, &( talents.lichborne               ) }, {  8, 3, &( talents.outbreak                 ) } },
      { {  9, 3, &( talents.death_rune_mastery               ) }, {  9, 3, &( talents.annihilation            ) }, {  9, 5, &( talents.necrosis                 ) } },
      { { 10, 3, &( talents.improved_rune_tap                ) }, { 10, 5, &( talents.killing_machine         ) }, { 10, 1, &( talents.corpse_explosion         ) } },
      { { 11, 3, &( talents.spell_deflection                 ) }, { 11, 2, &( talents.chill_of_the_grave      ) }, { 11, 2, &( talents.on_a_pale_horse          ) } },
      { { 12, 3, &( talents.vendetta                         ) }, { 12, 2, &( talents.endless_winter          ) }, { 12, 3, &( talents.blood_caked_blade        ) } },
      { { 13, 3, &( talents.bloody_strikes                   ) }, { 13, 3, &( talents.frigid_deathplate       ) }, { 13, 2, &( talents.night_of_the_dead        ) } },
      { { 14, 3, &( talents.veteran_of_the_third_war         ) }, { 14, 3, &( talents.glacier_rot             ) }, { 14, 1, &( talents.unholy_blight            ) } },
      { { 15, 1, &( talents.mark_of_blood                    ) }, { 15, 1, &( talents.deathchill              ) }, { 15, 5, &( talents.impurity                 ) } },
      { { 16, 3, &( talents.bloody_vengeance                 ) }, { 16, 1, &( talents.improved_icy_talons     ) }, { 16, 2, &( talents.dirge                    ) } },
      { { 17, 2, &( talents.abominations_might               ) }, { 17, 2, &( talents.merciless_combat        ) }, { 17, 2, &( talents.desecration              ) } },
      { { 18, 3, &( talents.bloodworms                       ) }, { 18, 4, &( talents.rime                    ) }, { 18, 3, &( talents.magic_supression         ) } },
      { { 19, 1, &( talents.hysteria                         ) }, { 19, 3, &( talents.chilblains              ) }, { 19, 3, &( talents.reaping                  ) } },
      { { 20, 2, &( talents.improved_blood_presence          ) }, { 20, 1, &( talents.hungering_cold          ) }, { 20, 1, &( talents.master_of_ghouls         ) } },
      { { 21, 2, &( talents.improved_death_strike            ) }, { 21, 2, &( talents.improved_frost_presence ) }, { 21, 5, &( talents.desolation               ) } },
      { { 22, 3, &( talents.sudden_doom                      ) }, { 22, 3, &( talents.threat_of_thassarian    ) }, { 22, 1, &( talents.anti_magic_zone          ) } },
      { { 23, 1, &( talents.vampiric_blood                   ) }, { 23, 3, &( talents.blood_of_the_north      ) }, { 23, 2, &( talents.improved_unholy_presence ) } },
      { { 24, 3, &( talents.will_of_the_necropolis           ) }, { 24, 1, &( talents.unbreakable_armor       ) }, { 24, 1, &( talents.ghoul_frenzy             ) } },
      { { 25, 1, &( talents.heart_strike                     ) }, { 25, 3, &( talents.acclimation             ) }, { 25, 3, &( talents.crypt_fever              ) } },
      { { 26, 3, &( talents.might_of_mograine                ) }, { 26, 1, &( talents.frost_strike            ) }, { 26, 1, &( talents.bone_shield              ) } },
      { { 27, 5, &( talents.blood_gorged                     ) }, { 27, 3, &( talents.guile_of_gorefiend      ) }, { 27, 3, &( talents.wandering_plague         ) } },
      { { 28, 1, &( talents.dancing_rune_weapon              ) }, { 28, 5, &( talents.tundra_stalker          ) }, { 28, 3, &( talents.ebon_plaguebringer       ) } },
      { {  0, 0, NULL                                          }, { 29, 1, &( talents.howling_blast           ) }, { 29, 1, &( talents.scourge_strike           ) } },
      { {  0, 0, NULL                                          }, {  0, 0, NULL                                 }, { 30, 5, &( talents.rage_of_rivendare        ) } },
      { {  0, 0, NULL                                          }, {  0, 0, NULL                                 }, { 31, 1, &( talents.summon_gargoyle          ) } },
      { {  0, 0, NULL                                          }, {  0, 0, NULL                                 }, {  0, 0, NULL                                  } }
    };

    util_t::translate_talent_trees( talent_list, translation_table, sizeof( translation_table ) );
  }
  return talent_list;
}

// death_knight_t::get_options ================================================

std::vector<option_t>& death_knight_t::get_options()
{
  if ( options.empty() )
  {
    player_t::get_options();

    option_t death_knight_options[] =
    {
      // @option_doc loc=player/deathknight/misc title="Talents"
      { "abominations_might",               OPT_INT, &( talents.abominations_might               ) },
      { "acclimation",                      OPT_INT, &( talents.acclimation                      ) },
      { "annihilation",                     OPT_INT, &( talents.annihilation                     ) },
      { "anti_magic_zone",                  OPT_INT, &( talents.anti_magic_zone                  ) },
      { "anticipation",                     OPT_INT, &( talents.anticipation                     ) },
      { "black_ice",                        OPT_INT, &( talents.black_ice                        ) },
      { "blade_barrier",                    OPT_INT, &( talents.blade_barrier                    ) },
      { "bladed_armor",                     OPT_INT, &( talents.bladed_armor                     ) },
      { "blood_caked_blade",                OPT_INT, &( talents.blood_caked_blade                ) },
      { "blood_gorged",                     OPT_INT, &( talents.blood_gorged                     ) },
      { "blood_of_the_north",               OPT_INT, &( talents.blood_of_the_north               ) },
      { "bloodworms",                       OPT_INT, &( talents.bloodworms                       ) },
      { "bloody_strikes",                   OPT_INT, &( talents.bloody_strikes                   ) },
      { "bloody_vengeance",                 OPT_INT, &( talents.bloody_vengeance                 ) },
      { "bone_shield",                      OPT_INT, &( talents.bone_shield                      ) },
      { "butchery",                         OPT_INT, &( talents.butchery                         ) },
      { "chilblains",                       OPT_INT, &( talents.chilblains                       ) },
      { "chill_of_the_grave",               OPT_INT, &( talents.chill_of_the_grave               ) },
      { "corpse_explosion",                 OPT_INT, &( talents.corpse_explosion                 ) },
      { "crypt_fever",                      OPT_INT, &( talents.crypt_fever                      ) },
      { "dancing_rune_weapon",              OPT_INT, &( talents.dancing_rune_weapon              ) },
      { "dark_conviction",                  OPT_INT, &( talents.dark_conviction                  ) },
      { "death_rune_mastery",               OPT_INT, &( talents.death_rune_mastery               ) },
      { "deathchill",                       OPT_INT, &( talents.deathchill                       ) },
      { "desecration",                      OPT_INT, &( talents.desecration                      ) },
      { "desolation",                       OPT_INT, &( talents.desolation                       ) },
      { "dirge",                            OPT_INT, &( talents.dirge                            ) },
      { "ebon_plaguebringer",               OPT_INT, &( talents.ebon_plaguebringer               ) },
      { "endless_winter",                   OPT_INT, &( talents.endless_winter                   ) },
      { "epidemic",                         OPT_INT, &( talents.epidemic                         ) },
      { "frigid_deathplate",                OPT_INT, &( talents.frigid_deathplate                ) },
      { "frost_strike",                     OPT_INT, &( talents.frost_strike                     ) },
      { "ghoul_frenzy",                     OPT_INT, &( talents.ghoul_frenzy                     ) },
      { "glacier_rot",                      OPT_INT, &( talents.glacier_rot                      ) },
      { "guile_of_gorefiend",               OPT_INT, &( talents.guile_of_gorefiend               ) },
      { "heart_strike",                     OPT_INT, &( talents.heart_strike                     ) },
      { "howling_blast",                    OPT_INT, &( talents.howling_blast                    ) },
      { "hungering_cold",                   OPT_INT, &( talents.hungering_cold                   ) },
      { "hysteria",                         OPT_INT, &( talents.hysteria                         ) },
      { "icy_reach",                        OPT_INT, &( talents.icy_reach                        ) },
      { "icy_talons",                       OPT_INT, &( talents.icy_talons                       ) },
      { "improved_blood_presence",          OPT_INT, &( talents.improved_blood_presence          ) },
      { "improved_death_strike",            OPT_INT, &( talents.improved_death_strike            ) },
      { "improved_frost_presence",          OPT_INT, &( talents.improved_frost_presence          ) },
      { "improved_icy_talons",              OPT_INT, &( talents.improved_icy_talons              ) },
      { "improved_icy_touch",               OPT_INT, &( talents.improved_icy_touch               ) },
      { "improved_rune_tap",                OPT_INT, &( talents.improved_rune_tap                ) },
      { "improved_unholy_presence",         OPT_INT, &( talents.improved_unholy_presence         ) },
      { "impurity",                         OPT_INT, &( talents.impurity                         ) },
      { "killing_machine",                  OPT_INT, &( talents.killing_machine                  ) },
      { "lichborne",                        OPT_INT, &( talents.lichborne                        ) },
      { "magic_supression",                 OPT_INT, &( talents.magic_supression                 ) },
      { "mark_of_blood",                    OPT_INT, &( talents.mark_of_blood                    ) },
      { "master_of_ghouls",                 OPT_INT, &( talents.master_of_ghouls                 ) },
      { "merciless_combat",                 OPT_INT, &( talents.merciless_combat                 ) },
      { "might_of_mograine",                OPT_INT, &( talents.might_of_mograine                ) },
      { "morbidity",                        OPT_INT, &( talents.morbidity                        ) },
      { "necrosis",                         OPT_INT, &( talents.necrosis                         ) },
      { "nerves_of_cold_steel",             OPT_INT, &( talents.nerves_of_cold_steel             ) },
      { "night_of_the_dead",                OPT_INT, &( talents.night_of_the_dead                ) },
      { "on_a_pale_horse",                  OPT_INT, &( talents.on_a_pale_horse                  ) },
      { "outbreak",                         OPT_INT, &( talents.outbreak                         ) },
      { "rage_of_rivendare",                OPT_INT, &( talents.rage_of_rivendare                ) },
      { "ravenous_dead",                    OPT_INT, &( talents.ravenous_dead                    ) },
      { "reaping",                          OPT_INT, &( talents.reaping                          ) },
      { "rime",                             OPT_INT, &( talents.rime                             ) },
      { "rune_tap",                         OPT_INT, &( talents.rune_tap                         ) },
      { "runic_power_mastery",              OPT_INT, &( talents.runic_power_mastery              ) },
      { "scent_of_blood",                   OPT_INT, &( talents.scent_of_blood                   ) },
      { "scourge_strike",                   OPT_INT, &( talents.scourge_strike                   ) },
      { "spell_deflection",                 OPT_INT, &( talents.spell_deflection                 ) },
      { "subversion",                       OPT_INT, &( talents.subversion                       ) },
      { "sudden_doom",                      OPT_INT, &( talents.sudden_doom                      ) },
      { "summon_gargoyle",                  OPT_INT, &( talents.summon_gargoyle                  ) },
      { "threat_of_thassarian",             OPT_INT, &( talents.threat_of_thassarian             ) },
      { "toughness",                        OPT_INT, &( talents.toughness                        ) },
      { "tundra_stalker",                   OPT_INT, &( talents.tundra_stalker                   ) },
      { "two_handed_weapon_specialization", OPT_INT, &( talents.two_handed_weapon_specialization ) },
      { "unbreakable_armor",                OPT_INT, &( talents.unbreakable_armor                ) },
      { "unholy_blight",                    OPT_INT, &( talents.unholy_blight                    ) },
      { "unholy_command",                   OPT_INT, &( talents.unholy_command                   ) },
      { "vampiric_blood",                   OPT_INT, &( talents.vampiric_blood                   ) },
      { "vendetta",                         OPT_INT, &( talents.vendetta                         ) },
      { "veteran_of_the_third_war",         OPT_INT, &( talents.veteran_of_the_third_war         ) },
      { "vicious_strikes",                  OPT_INT, &( talents.vicious_strikes                  ) },
      { "virulence",                        OPT_INT, &( talents.virulence                        ) },
      { "wandering_plague",                 OPT_INT, &( talents.wandering_plague                 ) },
      { "will_of_the_necropolis",           OPT_INT, &( talents.will_of_the_necropolis           ) },
      // @option_doc loc=player/deathknight/misc title="Misc"
      { "hysteria_target",                  OPT_STRING, &( hysteria_target_str                   ) },
      { "sigil",                            OPT_STRING, &( items[ SLOT_RANGED ].options_str      ) },
      { NULL, OPT_UNKNOWN, NULL }
    };

    option_t::copy( options, death_knight_options );
  }

  return options;
}

// death_knight_t::decode_set ===============================================

int death_knight_t::decode_set( item_t& item )
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

  bool is_melee = ( strstr( s, "helmet"         ) ||
                    strstr( s, "shoulderplates" ) ||
                    strstr( s, "battleplate"    ) ||
                    strstr( s, "legplates"      ) ||
                    strstr( s, "gauntlets"      ) );

  bool is_tank = ( strstr( s, "faceguard"  ) ||
                   strstr( s, "pauldrons"  ) ||
                   strstr( s, "chestguard" ) ||
                   strstr( s, "legguards"  ) ||
                   strstr( s, "handguards" ) );

  if ( strstr( s, "scourgeborne" ) )
  {
    if ( is_melee ) return SET_T7_MELEE;
    if ( is_tank  ) return SET_T7_TANK;
  }
  if ( strstr( s, "darkruned" ) )
  {
    if ( is_melee ) return SET_T8_MELEE;
    if ( is_tank  ) return SET_T8_TANK;
  }
  if ( strstr( s, "koltiras"    ) ||
       strstr( s, "thassarians" ) )
  {
    if ( is_melee ) return SET_T9_MELEE;
    if ( is_tank  ) return SET_T9_TANK;
  }
  if ( strstr( s, "scourgelord" ) )
  {
    if ( is_melee ) return SET_T10_MELEE;
    if ( is_tank  ) return SET_T10_TANK;
  }
  
  return SET_NONE;
}

// player_t implementations ============================================

player_t* player_t::create_death_knight( sim_t* sim, const std::string& name, int race_type )
{
  return new death_knight_t( sim, name, race_type );
}

// player_t::death_knight_init ==============================================

void player_t::death_knight_init( sim_t* sim )
{
  sim -> auras.abominations_might  = new aura_t( sim, "abominations_might",  1,  10.0 );
  sim -> auras.horn_of_winter      = new aura_t( sim, "horn_of_winter",      1, 120.0 );
  sim -> auras.improved_icy_talons = new aura_t( sim, "improved_icy_talons", 1,  20.0 );
  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    p -> buffs.hysteria = new buff_t( p, "hysteria", 1, 30.0 );
  }

  target_t* t = sim -> target;
  t -> debuffs.blood_plague       = new debuff_t( sim, "blood_plague",       1, 15.0 );
  t -> debuffs.frost_fever        = new debuff_t( sim, "frost_fever",        1, 15.0 );
  t -> debuffs.crypt_fever        = new debuff_t( sim, "crypt_fever",        1, 15.0 );
  t -> debuffs.ebon_plaguebringer = new debuff_t( sim, "ebon_plaguebringer", 1, 15.0 );
}

// player_t::death_knight_combat_begin ======================================

void player_t::death_knight_combat_begin( sim_t* sim )
{
  if ( sim -> overrides.abominations_might  ) sim -> auras.abominations_might  -> override();
  if ( sim -> overrides.horn_of_winter      ) sim -> auras.horn_of_winter      -> override( 1, 155 );
  if ( sim -> overrides.improved_icy_talons ) sim -> auras.improved_icy_talons -> override( 1, 0.20 );

  target_t* t = sim -> target;
  if ( sim -> overrides.blood_plague       ) t -> debuffs.blood_plague       -> override();
  if ( sim -> overrides.crypt_fever        ) t -> debuffs.crypt_fever        -> override( 1, 30 );
  if ( sim -> overrides.frost_fever        ) t -> debuffs.frost_fever        -> override();
  if ( sim -> overrides.ebon_plaguebringer ) t -> debuffs.ebon_plaguebringer -> override( 1, 13 );
}
