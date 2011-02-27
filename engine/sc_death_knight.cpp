// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

struct army_ghoul_pet_t;
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

const char *rune_symbols = "!bfu!!";

#define RUNE_TYPE_MASK     3
#define RUNE_SLOT_MAX      6

#define RUNIC_POWER_REFUND  0.9

// These macros simplify using the result of count_runes(), which
// returns a number of the form 0x000AABBCC where AA is the number of
// Unholy runes, BB is the number of Frost runes, and CC is the number
// of Blood runes.
#define GET_BLOOD_RUNE_COUNT(x)  ((x >>  0) & 0xff)
#define GET_FROST_RUNE_COUNT(x)  ((x >>  8) & 0xff)
#define GET_UNHOLY_RUNE_COUNT(x) ((x >> 16) & 0xff)

enum rune_state { STATE_DEPLETED, STATE_REGENERATING, STATE_FULL };

struct dk_rune_t
{
  int        type;
  rune_state state;
  double     value;   // 0.0 to 1.0, with 1.0 being full
  int        slot_number;
  bool       permanent_death_rune;
  dk_rune_t* paired_rune;

  dk_rune_t() : type( RUNE_TYPE_NONE ), state( STATE_FULL ), value( 0.0 ), permanent_death_rune( false ), paired_rune( NULL ) {}

  bool is_death() SC_CONST { return ( type & RUNE_TYPE_DEATH ) != 0; }
  bool is_ready() SC_CONST { return state == STATE_FULL; }
  bool is_depleted() SC_CONST { return state == STATE_DEPLETED; }
  bool is_regenerating() SC_CONST { return state == STATE_REGENERATING; }
  int  get_type() SC_CONST { return type & RUNE_TYPE_MASK; }

  void regen_rune( player_t* p, double periodicity );

  void make_permanent_death_rune()
  {
    permanent_death_rune = true;
    type |= RUNE_TYPE_DEATH;
  }

  void consume( bool convert )
  {
    assert ( value >= 1.0 );
    if ( permanent_death_rune )
    {
      type |= RUNE_TYPE_DEATH;
    }
    else
    {
      type = ( type & RUNE_TYPE_MASK ) | ( ( type << 1 ) & RUNE_TYPE_WASDEATH ) | ( convert ? RUNE_TYPE_DEATH : 0 );
    }
    value = 0.0;
    state = STATE_DEPLETED;
  }

  void fill_rune()
  {
    value = 1.0;
    state = STATE_FULL;
  }

  void reset()
  {
    value = 1.0;
    state = STATE_FULL;
    type = type & RUNE_TYPE_MASK;
    if ( permanent_death_rune )
    {
      type |= RUNE_TYPE_DEATH;
    }
  }
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
  action_t* active_unholy_blight;

  // Armor Spec
  passive_spell_t* plate_specialization;

  // Buffs
  buff_t* buffs_blood_presence;
  buff_t* buffs_bloodworms;
  buff_t* buffs_bone_shield;
  buff_t* buffs_crimson_scourge;
  buff_t* buffs_dancing_rune_weapon;
  buff_t* buffs_dark_transformation;
  buff_t* buffs_ebon_plaguebringer; // Used for disease tracking
  buff_t* buffs_frost_presence;
  buff_t* buffs_killing_machine;
  buff_t* buffs_pillar_of_frost;
  buff_t* buffs_rime;
  buff_t* buffs_rune_of_cinderglacier;
  buff_t* buffs_rune_of_razorice;
  buff_t* buffs_rune_of_the_fallen_crusader;
  buff_t* buffs_runic_corruption;
  buff_t* buffs_scent_of_blood;
  buff_t* buffs_shadow_infusion;
  buff_t* buffs_sudden_doom;
  buff_t* buffs_tier10_4pc_melee;
  buff_t* buffs_tier11_4pc_melee;
  buff_t* buffs_unholy_presence;

  // Constants
  struct constants_t
  {
    double runic_empowerment_proc_chance;

    constants_t() { memset( ( void* ) this, 0x0, sizeof( constants_t ) ); }
  };
  constants_t constants;

  // Cooldowns
  cooldown_t* cooldowns_howling_blast;

  // Diseases
  spell_t* blood_plague;
  spell_t* frost_fever;

  // DoTs
  dot_t* dots_blood_plague;
  dot_t* dots_death_and_decay;
  dot_t* dots_frost_fever;

  // Gains
  gain_t* gains_butchery;
  gain_t* gains_chill_of_the_grave;
  gain_t* gains_horn_of_winter;
  gain_t* gains_might_of_the_frozen_wastes;
  gain_t* gains_power_refund;
  gain_t* gains_rune_abilities;
  gain_t* gains_scent_of_blood;

  // Glyphs
  struct glyphs_t
  {
    // Prime
    int death_and_decay;
    int death_coil;
    int death_strike;
    int frost_strike;
    int heart_strike;
    int howling_blast;
    int icy_touch;
    int obliterate;
    int raise_dead;
    int rune_strike;
    int scourge_strike;

    // Minor
    int horn_of_winter;

    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  // Mastery
  struct dk_mastery_t
  {
    mastery_t* dreadblade;
    mastery_t* frozen_heart;

    dk_mastery_t() { memset( ( void* ) this, 0x0, sizeof( dk_mastery_t ) ); }
  };
  dk_mastery_t mastery;

  // Options
  std::string unholy_frenzy_target_str;

  // Passive abilities
  struct passives_t
  {
    passive_spell_t* blood_of_the_north;
    passive_spell_t* blood_rites;
    passive_spell_t* icy_talons;
    passive_spell_t* master_of_ghouls;
    passive_spell_t* reaping;
    passive_spell_t* runic_empowerment;
    passive_spell_t* unholy_might;
    passive_spell_t* veteran_of_the_third_war;

    passives_t() { memset( ( void* ) this, 0x0, sizeof( passives_t ) ); }
  };
  passives_t passives;

  // Pets and Guardians
  army_ghoul_pet_t*          active_army_ghoul;
  bloodworms_pet_t*          active_bloodworms;
  dancing_rune_weapon_pet_t* active_dancing_rune_weapon;
  ghoul_pet_t*               active_ghoul;

  // Procs
  proc_t* proc_runic_empowerment;
  proc_t* proc_runic_empowerment_wasted;

  // RNGs
  rng_t* rng_blood_caked_blade;
  rng_t* rng_might_of_the_frozen_wastes;
  rng_t* rng_threat_of_thassarian;

  // Runes
  struct runes_t
  {
    dk_rune_t slot[RUNE_SLOT_MAX];

    runes_t()
    {
      for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
      {
        slot[i].type = RUNE_TYPE_BLOOD + ( i >> 1 );
        slot[i].paired_rune = &( slot[i ^ 1] ); // xor!
        slot[i].slot_number = i;
      }

    }
    void reset() { for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) slot[i].reset(); }
  };
  runes_t _runes;

  // Talents
  struct talents_t
  {
    // Blood
    talent_t* abominations_might;
    talent_t* bladed_armor;
    talent_t* blood_caked_blade;
    talent_t* blood_parasite;
    talent_t* bone_shield;
    talent_t* butchery;
    talent_t* crimson_scourge;
    talent_t* dancing_rune_weapon;
    talent_t* improved_blood_presence;
    talent_t* improved_blood_tap;
    talent_t* improved_death_strike;
    talent_t* scent_of_blood;

    // Frost
    talent_t* annihilation;
    talent_t* brittle_bones;
    talent_t* chill_of_the_grave;
    talent_t* endless_winter;
    talent_t* howling_blast;
    talent_t* improved_frost_presence;
    talent_t* improved_icy_talons;
    talent_t* killing_machine;
    talent_t* merciless_combat;
    talent_t* might_of_the_frozen_wastes;
    talent_t* nerves_of_cold_steel;
    talent_t* pillar_of_frost;
    talent_t* rime;
    talent_t* runic_power_mastery;
    talent_t* threat_of_thassarian;
    talent_t* toughness;

    // Unholy
    talent_t* dark_transformation;
    talent_t* ebon_plaguebringer;
    talent_t* epidemic;
    talent_t* improved_unholy_presence;
    talent_t* morbidity;
    talent_t* rage_of_rivendare;
    talent_t* runic_corruption;
    talent_t* shadow_infusion;
    talent_t* sudden_doom;
    talent_t* summon_gargoyle;
    talent_t* unholy_blight;
    talent_t* unholy_frenzy;
    talent_t* virulence;

    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  death_knight_t( sim_t* sim, const std::string& name, race_type r = RACE_NONE ) :
    player_t( sim, DEATH_KNIGHT, name, r )
  {
    if ( race == RACE_NONE ) race = RACE_NIGHT_ELF;

    tree_type[ DEATH_KNIGHT_BLOOD  ] = TREE_BLOOD;
    tree_type[ DEATH_KNIGHT_FROST  ] = TREE_FROST;
    tree_type[ DEATH_KNIGHT_UNHOLY ] = TREE_UNHOLY;

    // Active
    active_presence            = 0;
    active_blood_caked_blade   = NULL;
    active_unholy_blight       = NULL;

    // DoTs
    dots_blood_plague    = get_dot( "blood_plague" );
    dots_death_and_decay = get_dot( "death_and_decay" );
    dots_frost_fever     = get_dot( "frost_fever" );

    // Pets and Guardians
    active_army_ghoul          = NULL;
    active_bloodworms          = NULL;
    active_dancing_rune_weapon = NULL;
    active_ghoul               = NULL;

    blood_plague = NULL;
    frost_fever  = NULL;

    cooldowns_howling_blast = get_cooldown( "howling_blast" );

    create_talents();
    create_glyphs();
    create_options();
  }

  // Character Definition
  virtual void      init();
  virtual void      init_talents();
  virtual void      init_spells();
  virtual void      init_actions();
  virtual void      init_enchant();
  virtual void      init_rng();
  virtual void      init_base();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_glyphs();
  virtual void      init_resources( bool force );
  virtual void      init_values();
  double composite_pet_attack_crit();
  virtual double    composite_attack_haste() SC_CONST;
  virtual double    composite_attack_hit() SC_CONST;
  virtual double    composite_attack_power() SC_CONST;
  virtual double    composite_attribute_multiplier( int attr ) SC_CONST;
  virtual double    matching_gear_multiplier( const attribute_type attr ) SC_CONST;
  virtual double    composite_spell_hit() SC_CONST;
  virtual double    composite_tank_parry() SC_CONST;
  virtual double    composite_player_multiplier( const school_type school ) SC_CONST;
  virtual double    composite_tank_crit( const school_type school ) SC_CONST;
  virtual void      regen( double periodicity );
  virtual void      reset();
  virtual double      assess_damage( double amount, const school_type school, int    dmg_type, int result, action_t* a );
  virtual void      combat_begin();
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual action_expr_t* create_expression( action_t*, const std::string& name );
  virtual pet_t*    create_pet( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST { return RESOURCE_RUNIC; }
  virtual int       primary_role() SC_CONST;
  virtual void      trigger_runic_empowerment();

  int diseases()
  {
    int disease_count = 0;
    if ( buffs_ebon_plaguebringer -> check() ) disease_count++;
    if ( dots_blood_plague -> ticking ) disease_count++;
    if ( dots_frost_fever  -> ticking ) disease_count++;
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

static void log_rune_status( death_knight_t* p )
{
  std::string rune_str;
  for ( int j = 0; j < RUNE_SLOT_MAX; ++j )
  {
    char rune_letter = rune_symbols[p -> _runes.slot[j].get_type()];
    if ( p -> _runes.slot[j].is_death() )
      rune_letter = 'd';

    if ( p -> _runes.slot[j].is_ready() )
      rune_letter = toupper( rune_letter );
    rune_str += rune_letter;
  }
  log_t::output( p -> sim, "%s runes: %s", p -> name(), rune_str.c_str() );
}

void dk_rune_t::regen_rune( player_t* p, double periodicity )
{
  // Full runes don't regen.
  if ( state == STATE_FULL ) return;
  // If the other rune is already regening, we don't.
  if ( state == STATE_DEPLETED && paired_rune -> state == STATE_REGENERATING ) return;

  death_knight_t* o = p -> cast_death_knight();
  // Base rune regen rate is 10 seconds; we want the per-second regen
  // rate, so divide by 10.0.  Haste is a multiplier (so 30% haste
  // means composite_attack_haste is 1/1.3), so we invert it.  Haste
  // linearly scales regen rate -- 100% haste means a rune regens in 5
  // seconds, etc.
  double runes_per_second = 1.0 / 10.0 / p -> composite_attack_haste();

  if ( o -> buffs_blood_presence -> check() && o -> talents.improved_blood_presence -> rank() )
  {
    runes_per_second *= 1.0 + ( o -> talents.improved_blood_presence -> effect_base_value( 3 ) / 100.0 );
  }
  // Unholy Presence's 10% (or, talented, 15%) increase is factored in elsewhere as melee haste.
  if ( o -> buffs_runic_corruption -> check() )
  {
    runes_per_second *= 1.0 + ( o -> talents.runic_corruption -> effect_base_value( 1 ) / 100.0 );
  }

  // Chances are, we will overflow by a small amount.  Toss extra
  // overflow into our paired rune if it is regenerating or depleted.
  value += periodicity * runes_per_second;
  double overflow = 0.0;
  if ( value > 1.0 )
  {
    overflow = value - 1.0;
    value = 1.0;
  }

  if ( value >= 1.0 )
    state = STATE_FULL;
  else
    state = STATE_REGENERATING;

  if ( overflow > 0.0 && (paired_rune -> state == STATE_REGENERATING || paired_rune -> state == STATE_DEPLETED) )
  {
    paired_rune -> value = std::min( 1.0, paired_rune -> value + overflow );
    if ( paired_rune -> value >= 1.0 )
      paired_rune -> state = STATE_FULL;
    else
      paired_rune -> state = STATE_REGENERATING;
  }

  if ( p -> sim -> debug )
    log_t::output( p -> sim, "rune %d regen rate %.3f with haste %.2f percent", slot_number, runes_per_second, 100.0 / p -> composite_attack_haste() );

  if ( state == STATE_FULL )
  {
    if ( p -> sim -> log )
    {
      log_rune_status( o );
    }
    if ( is_death() )
      o -> buffs_tier11_4pc_melee -> trigger();

    if ( p -> sim -> debug )
      log_t::output( p -> sim, "rune %d regens to full", slot_number );
  }
}

// ==========================================================================
// Guardians
// ==========================================================================

// ==========================================================================
// Army of the Dead Ghoul
// ==========================================================================

// Army of the Dead ghouls are basically a copy of the pet ghoul, but with a 50% damage penalty, but you get 8 of them
struct army_ghoul_pet_t : public pet_t
{
  double snapshot_haste, snapshot_hit, snapshot_crit, snapshot_expertise;

  army_ghoul_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "army_of_the_dead_ghoul_8" ),
    snapshot_haste( 0 ), snapshot_hit( 0 ), snapshot_crit( 0 ), snapshot_expertise( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 100; // FIXME only level 80 value
    main_hand_weapon.max_dmg    = 100; // FIXME only level 80 value
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = 2.0;

    action_list_str = "auto_attack/claw";
  }

  struct army_ghoul_pet_attack_t : public attack_t
  {
    army_ghoul_pet_attack_t( const char* n, player_t* player, const resource_type r=RESOURCE_ENERGY, const school_type s=SCHOOL_PHYSICAL ) :
      attack_t( n, player, r, SCHOOL_PHYSICAL )
    {
      weapon = &( player -> main_hand_weapon );
      may_crit = true;
      weapon_power_mod *= 0.84; // What is this based off?
      base_multiplier *= 4.0; // 50% of damage x 8 ghouls
    }
  };

  struct army_ghoul_pet_melee_t : public army_ghoul_pet_attack_t
  {
    army_ghoul_pet_melee_t( player_t* player ) :
      army_ghoul_pet_attack_t( "melee", player, RESOURCE_NONE, SCHOOL_PHYSICAL )
    {
      base_execute_time = weapon -> swing_time;
      base_dd_min       = base_dd_max = 1;
      background        = true;
      repeating         = true;
    }
  };

  struct army_ghoul_pet_auto_attack_t : public army_ghoul_pet_attack_t
  {
    army_ghoul_pet_auto_attack_t( player_t* player ) :
      army_ghoul_pet_attack_t( "auto_attack", player )
    {
      army_ghoul_pet_t* p = ( army_ghoul_pet_t* ) player -> cast_pet();

      weapon = &( p -> main_hand_weapon );
      p -> main_hand_attack = new army_ghoul_pet_melee_t( player );
      trigger_gcd = 0;
    }

    virtual void execute()
    {
      army_ghoul_pet_t* p = ( army_ghoul_pet_t* ) player -> cast_pet();
      p -> main_hand_attack -> schedule_execute();
    }

    virtual bool ready()
    {
      army_ghoul_pet_t* p = ( army_ghoul_pet_t* ) player -> cast_pet();
      return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
    }
  };

  struct army_ghoul_pet_claw_t : public army_ghoul_pet_attack_t
  {
    army_ghoul_pet_claw_t( player_t* player ) :
      army_ghoul_pet_attack_t( "claw", player )
    {
      army_ghoul_pet_t* p = ( army_ghoul_pet_t* ) player -> cast_pet();
      death_knight_t* o = p -> owner -> cast_death_knight();

      id = 91776;
      parse_data( o -> player_data );
    }
  };

  virtual void init_base()
  {
    // FIXME: Level 80/85 values did they change?
    attribute_base[ ATTR_STRENGTH  ] = 331;
    attribute_base[ ATTR_AGILITY   ] = 856;
    attribute_base[ ATTR_STAMINA   ] = 361;
    attribute_base[ ATTR_INTELLECT ] = 65;
    attribute_base[ ATTR_SPIRIT    ] = 109;

    base_attack_power = -20;
    initial_attack_power_per_strength = 2.0;
    initial_attack_power_per_agility  = 0.0;

    // Ghouls don't appear to gain any crit from agi, they may also just have none
    // initial_attack_crit_per_agility = rating_t::interpolate( level, 0.01/25.0, 0.01/40.0, 0.01/83.3 );

    resource_base[ RESOURCE_ENERGY ] = 100;
    base_energy_regen_per_second  = 10;
  }

  virtual double strength() SC_CONST
  {
    double a = attribute[ ATTR_STRENGTH ];
    a *= composite_attribute_multiplier( ATTR_STRENGTH );
    return a;
  }

  virtual void summon( double duration=0 )
  {
    death_knight_t* o = owner -> cast_death_knight();
    pet_t::summon( duration );
    snapshot_haste     = o -> composite_attack_haste();
    snapshot_hit       = o -> composite_attack_hit();
    snapshot_crit      = o -> composite_pet_attack_crit();
    snapshot_expertise = o -> composite_attack_expertise();
    o -> active_army_ghoul = this;
  }

  virtual void dismiss()
  {
    death_knight_t* o = owner -> cast_death_knight();
    pet_t::dismiss();
    o -> active_army_ghoul = 0;
  }

  virtual double composite_attack_expertise() SC_CONST
  {
    // Divide hit by 0.25; expertise is stored not as a percent or
    // rating but as the effective "expertise" amount, which is 0.25%
    // antidodge/antiparry per point.  Since hit is a percent, we need
    // to divide by 0.25 to get an expertise value of the equivalentb
    // percents, as percent-equivalent is apparently how the game
    // functions.
    return std::max( snapshot_hit / 0.25, snapshot_expertise ); // Hit gains equal to expertise
  }

  virtual double composite_attack_haste() SC_CONST
  {
    // Ghouls receive 100% of their master's haste.
    // http://elitistjerks.com/f72/t42606-pet_discussion_garg_aotd_ghoul/
    return snapshot_haste;
  }

  virtual double composite_attack_hit() SC_CONST
  {
    return snapshot_hit;
  }

  virtual double composite_attack_crit() SC_CONST
  {
    return snapshot_crit;
  }

  virtual int primary_resource() SC_CONST
  {
    return RESOURCE_ENERGY;
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "auto_attack"    ) return new  army_ghoul_pet_auto_attack_t( this );
    if ( name == "claw"           ) return new         army_ghoul_pet_claw_t( this );

    return pet_t::create_action( name, options_str );
  }
};

// ==========================================================================
// Bloodworms
// ==========================================================================
struct bloodworms_pet_t : public pet_t
{
  // FIXME: Level 80/85 values
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
  };

  melee_t* melee;

  bloodworms_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "bloodworms", true /*guardian*/ )
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
  dot_t* dots_drw_blood_plague;
  dot_t* dots_drw_frost_fever;

  int drw_diseases()
  {
    int drw_disease_count = 0;
    if ( dots_drw_blood_plague -> ticking ) drw_disease_count++;
    if ( dots_drw_frost_fever  -> ticking ) drw_disease_count++;
    return drw_disease_count;
  }

  struct drw_blood_boil_t : public spell_t
  {
    drw_blood_boil_t( player_t* player ) :
      spell_t( "blood_boil", player, RESOURCE_NONE, SCHOOL_SHADOW, TREE_BLOOD )
    {
      pet_t* p = player -> cast_pet();
      death_knight_t* o = p -> owner -> cast_death_knight();

      id = 48721;
      parse_data( o -> player_data );

      background       = true;
      trigger_gcd      = 0;
      aoe              = -1;
      may_crit         = true;
      direct_power_mod = 0.06;
      base_multiplier *= 0.50; // DRW penalty

      if ( o -> race == RACE_ORC )
      {
        base_multiplier *= 1.05;
      }
    }

    virtual void execute()
    {
      dancing_rune_weapon_pet_t* p = ( dancing_rune_weapon_pet_t* ) player;
      base_dd_adder = ( p -> drw_diseases() ? 95 : 0 );
      direct_power_mod  = 0.06 + ( p -> drw_diseases() ? 0.035 : 0 );
      spell_t::execute();
    }

    virtual bool ready() { return false; }
  };

  struct drw_blood_plague_t : public spell_t
  {
    drw_blood_plague_t( player_t* player ) :
      spell_t( "blood_plague", player, RESOURCE_NONE, SCHOOL_SHADOW, TREE_UNHOLY )
    {
      pet_t* p = player -> cast_pet();
      death_knight_t* o = p -> owner -> cast_death_knight();

      id = 59879;
      parse_data( o -> player_data );

      background         = true;
      base_tick_time     = 3.0;
      num_ticks          = 7 + util_t::talent_rank( o -> talents.epidemic -> effect_base_value( 1 ), 3, 1, 3, 4 );
      direct_power_mod  *= 0.055 * 1.15;
      may_miss           = false;
      hasted_ticks       = false;
      base_multiplier   *= 0.50; // DRW penalty

      if ( o -> race == RACE_ORC )
      {
        base_multiplier *= 1.05;
      }

      reset();
    }

    virtual bool ready() { return false; }

  };

  struct drw_death_coil_t : public spell_t
  {
    drw_death_coil_t( player_t* player ) :
      spell_t( "death_coil", player, RESOURCE_NONE, SCHOOL_SHADOW, TREE_BLOOD )
    {
      pet_t* p = player -> cast_pet();
      death_knight_t* o = p -> owner -> cast_death_knight();

      id = 47541;
      parse_data( o -> player_data );

      background  = true;
      trigger_gcd = 0;
      direct_power_mod = 0.3 * 0.85; // FIX-ME: From Feb 9th Hotfix. Test to confirm value.
      base_dd_min      = o -> player_data.effect_min( id, p -> level, E_DUMMY, A_NONE );
      base_dd_max      = o -> player_data.effect_max( id, p -> level, E_DUMMY, A_NONE );
      base_multiplier *= 1 + o -> glyphs.death_coil * 0.15;
      base_multiplier *= 0.50; // DRW Penalty
      if ( o -> set_bonus.tier11_2pc_melee() )
        base_crit     += 0.05;

      if ( o -> race == RACE_ORC )
      {
        base_multiplier *= 1.05;
      }
    }
    virtual bool ready() { return false; }
  };

  struct drw_death_strike_t : public attack_t
  {
    drw_death_strike_t( player_t* player ) :
      attack_t( "death_strike", player, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_BLOOD, true )
    {
      pet_t* p = player -> cast_pet();
      death_knight_t* o = p -> owner -> cast_death_knight();

      id = 49998;
      parse_data( o -> player_data );

      background  = true;
      trigger_gcd = 0;
      base_crit       +=     o -> talents.improved_death_strike -> effect_base_value( 2 ) / 100.0;
      base_multiplier *= 1 + o -> talents.improved_death_strike -> effect_base_value( 1 ) / 100.0;
      base_multiplier *= 0.50; // DRW penalty

      if ( o -> race == RACE_ORC )
      {
        base_multiplier *= 1.05;
      }
    }

    virtual bool ready() { return false; }
  };

  struct drw_frost_fever_t : public spell_t
  {
    drw_frost_fever_t( player_t* player ) :
      spell_t( "frost_fever", player, RESOURCE_NONE, SCHOOL_FROST, TREE_FROST )
    {
      pet_t* p = player -> cast_pet();
      death_knight_t* o = p -> owner -> cast_death_knight();

      id = 59921;
      parse_data( o -> player_data );

      background        = true;
      trigger_gcd       = 0;
      base_tick_time    = 3.0;
      hasted_ticks      = false;
      may_miss          = false;
      num_ticks         = 7 + util_t::talent_rank( o -> talents.epidemic -> effect_base_value( 1 ), 3, 1, 3, 4 );
      direct_power_mod *= 0.055 * 1.15;
      base_multiplier  *= 1.0 + o -> glyphs.icy_touch * 0.2;
      base_multiplier  *= 0.50; // DRW Penalty

      if ( o -> race == RACE_ORC )
      {
        base_multiplier *= 1.05;
      }
    }

    virtual bool ready()     { return false; }
  };

  struct drw_heart_strike_t : public attack_t
  {
    drw_heart_strike_t( player_t* player ) :
      attack_t( "heart_strike", player, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_BLOOD )
    {
      pet_t* p = player -> cast_pet();
      death_knight_t* o = p -> owner -> cast_death_knight();

      id = 55050;
      parse_data( p -> player_data );

      background          = true;
      aoe                 = 2;
      base_add_multiplier = 0.75;
      trigger_gcd         = 0;
      base_multiplier    *= 1 + o -> set_bonus.tier10_2pc_melee() * 0.07
                              + o -> glyphs.heart_strike          * 0.30;
      base_multiplier    *= 0.50; // DRW penalty

      if ( o -> race == RACE_ORC )
      {
        base_multiplier *= 1.05;
      }

      reset();
    }

    void target_debuff( player_t* t, int dmg_type )
    {
      dancing_rune_weapon_pet_t* p = ( dancing_rune_weapon_pet_t* ) player;
      attack_t::target_debuff( t, dmg_type );

      target_multiplier *= 1 + p -> drw_diseases() * 0.1;
    }

    virtual bool ready() { return false; }
  };

  struct drw_icy_touch_t : public spell_t
  {
    drw_icy_touch_t( player_t* player ) :
      spell_t( "icy_touch", player, RESOURCE_NONE, SCHOOL_FROST, TREE_FROST )
    {
      pet_t* p = player -> cast_pet();
      death_knight_t* o = p -> owner -> cast_death_knight();

      id = 45477;
      parse_data( p -> player_data );

      background       = true;
      trigger_gcd      = 0;
      direct_power_mod = 0.2;
      base_multiplier *= 0.5; // DRW Penalty

      if ( o -> race == RACE_ORC )
      {
        base_multiplier *= 1.05;
      }
    }

    virtual void execute()
    {
      dancing_rune_weapon_pet_t* p = ( dancing_rune_weapon_pet_t* ) player;

      spell_t::execute();

      if ( result_is_hit() )
      {
        if ( ! p -> drw_frost_fever )
          p -> drw_frost_fever = new drw_frost_fever_t( p );
        p -> drw_frost_fever -> execute();
      }
    }

    virtual bool ready() { return false; }
  };

  struct drw_pestilence_t : public spell_t
  {
    drw_pestilence_t( player_t* player ) :
      spell_t( "pestilence", player, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_BLOOD )
    {
      trigger_gcd = 0;
    }

    virtual bool ready() { return false; }
  };

  struct drw_plague_strike_t : public attack_t
  {
    drw_plague_strike_t( player_t* player ) :
      attack_t( "plague_strike", player, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_BLOOD, true )
    {
      pet_t* p = player -> cast_pet();
      death_knight_t* o = p -> owner -> cast_death_knight();

      id = 45462;
      parse_data( o -> player_data );

      background       = true;
      trigger_gcd      = 0;
      may_crit         = true;
      base_multiplier *= 0.50; // DRW penalty

      if ( o -> race == RACE_ORC )
      {
        base_multiplier *= 1.05;
      }
    }

    virtual void execute()
    {
      dancing_rune_weapon_pet_t* p = ( dancing_rune_weapon_pet_t* ) player;

      attack_t::execute();

      if ( result_is_hit() )
      {
        if ( ! p -> drw_blood_plague )
          p -> drw_blood_plague = new drw_blood_plague_t( p );
        p -> drw_blood_plague -> execute();
      }
    }

    virtual bool ready() { return false; }
  };

  struct drw_melee_t : public attack_t
  {
    drw_melee_t( player_t* player ) :
      attack_t( "drw_melee", player, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_NONE, false )
    {
      pet_t* p = player -> cast_pet();
      weapon            = &( p -> owner -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      base_dd_min       = 2; // FIXME: Should these be set?
      base_dd_max       = 322;
      may_crit          = true;
      background        = true;
      repeating         = true;
      weapon_power_mod *= 2.0; //Attack power scaling is unaffected by the DRW 50% penalty.
      base_multiplier  *= 0.5; // DRW Penalty

      if ( p -> owner -> race == RACE_ORC )
      {
        base_multiplier *= 1.05;
      }
    }
  };

  double snapshot_spell_crit, snapshot_attack_crit, haste_snapshot;
  spell_t*  drw_blood_boil;
  spell_t*  drw_blood_plague;
  spell_t*  drw_death_coil;
  attack_t* drw_death_strike;
  spell_t*  drw_frost_fever;
  attack_t* drw_heart_strike;
  spell_t*  drw_icy_touch;
  spell_t*  drw_pestilence;
  attack_t* drw_plague_strike;
  attack_t* drw_melee;

  dancing_rune_weapon_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "dancing_rune_weapon", true ),
    dots_drw_blood_plague( 0 ), dots_drw_frost_fever( 0 ),
    snapshot_spell_crit( 0.0 ), snapshot_attack_crit( 0.0 ),
    haste_snapshot( 1.0 ), drw_blood_boil( 0 ), drw_blood_plague( 0 ),
    drw_death_coil( 0 ), drw_death_strike( 0 ), drw_frost_fever( 0 ),
    drw_heart_strike( 0 ), drw_icy_touch( 0 ), drw_pestilence( 0 ),
    drw_plague_strike( 0 ), drw_melee( 0 )
  {
    dots_drw_blood_plague  = get_dot( "blood_plague" );
    dots_drw_frost_fever   = get_dot( "frost_fever" );

    main_hand_weapon.type       = WEAPON_SWORD_2H;
    main_hand_weapon.min_dmg    = 685; // FIXME: Should these be hardcoded?
    main_hand_weapon.max_dmg    = 975;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = 3.3;
  }

  virtual void init_base()
  {
    // Everything stays at zero.
    // DRW uses a snapshot of the DKs stats when summoned.
    drw_blood_boil    = new drw_blood_boil_t   ( this );
    drw_blood_plague  = new drw_blood_plague_t ( this );
    drw_death_coil    = new drw_death_coil_t   ( this );
    drw_death_strike  = new drw_death_strike_t ( this );
    drw_frost_fever   = new drw_frost_fever_t  ( this );
    drw_heart_strike  = new drw_heart_strike_t ( this );
    drw_icy_touch     = new drw_icy_touch_t    ( this );
    drw_pestilence    = new drw_pestilence_t   ( this );
    drw_plague_strike = new drw_plague_strike_t( this );
    drw_melee         = new drw_melee_t        ( this );
  }

  virtual double composite_attack_crit() SC_CONST        { return snapshot_attack_crit; }
  virtual double composite_attack_haste() SC_CONST       { return haste_snapshot; }
  virtual double composite_attack_power() SC_CONST       { return attack_power; }
  virtual double composite_spell_crit() SC_CONST         { return snapshot_spell_crit;  }
  virtual double composite_player_multiplier( const school_type school ) SC_CONST
  {
    double m = player_t::composite_player_multiplier( school );
    return m;
  }

  virtual void summon( double duration=0 )
  {
    death_knight_t* o = owner -> cast_death_knight();
    pet_t::summon( duration );
    o -> active_dancing_rune_weapon = this;
    snapshot_spell_crit  = o -> composite_spell_crit();
    snapshot_attack_crit = o -> composite_attack_crit();
    haste_snapshot       = o -> composite_attack_haste();
    attack_power         = o -> composite_attack_power() * o -> composite_attack_power_multiplier();
    drw_melee -> schedule_execute();
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
  // FIXME: Did any of these stats change?
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

      base_dd_min                  = 130;
      base_dd_max                  = 150;
      direct_power_mod             = 0.40;
      base_execute_time            = 2.0;
      base_spell_power_multiplier  = 0;
      base_attack_power_multiplier = 1;

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
  virtual double composite_spell_haste() SC_CONST { return haste_snapshot; }
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
  double snapshot_crit, snapshot_haste, snapshot_hit, snapshot_strength;

  ghoul_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "ghoul" ),
    snapshot_crit( 0 ), snapshot_haste( 0 ), snapshot_hit( 0 ), snapshot_strength( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 100; // FIXME only level 80 value
    main_hand_weapon.max_dmg    = 100; // FIXME only level 80 value
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = 2.0;

    action_list_str = "auto_attack/sweeping_claws/claw";
  }

  struct ghoul_pet_attack_t : public attack_t
  {
    ghoul_pet_attack_t( const char* n, player_t* player, const resource_type r=RESOURCE_ENERGY, const school_type s=SCHOOL_PHYSICAL ) :
      attack_t( n, player, RESOURCE_ENERGY, SCHOOL_PHYSICAL )
    {
      weapon = &( player -> main_hand_weapon );
      may_crit = true;
      weapon_power_mod *= 0.84; // What is this based off?
    }

    virtual void player_buff()
    {
      attack_t::player_buff();
      ghoul_pet_t* p = ( ghoul_pet_t* ) player -> cast_pet();
      death_knight_t* o = p -> owner -> cast_death_knight();
      if ( o -> buffs_shadow_infusion -> check() )
      {
        player_multiplier *= 1.0 + o -> buffs_shadow_infusion -> stack() * ( 0.06 );
      }
      if ( o -> buffs_dark_transformation -> check() )
      {
        player_multiplier *= 1.6;
      }
      if ( o -> buffs_frost_presence -> check() )
      {
        player_multiplier *= 1.0 + o -> buffs_frost_presence -> value();
      }
    }
  };

  struct ghoul_pet_melee_t : public ghoul_pet_attack_t
  {
    ghoul_pet_melee_t( player_t* player ) :
      ghoul_pet_attack_t( "melee", player, RESOURCE_NONE, SCHOOL_PHYSICAL )
    {
      base_execute_time = weapon -> swing_time;
      base_dd_min       = base_dd_max = 1;
      background        = true;
      repeating         = true;
    }
  };

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
      death_knight_t* o = p -> owner -> cast_death_knight();

      id = 91776;
      parse_data( o -> player_data );
    }

  };

  struct ghoul_pet_sweeping_claws_t : public ghoul_pet_attack_t
  {
    ghoul_pet_sweeping_claws_t( player_t* player ) :
      ghoul_pet_attack_t( "sweeping_claws", player )
    {
      ghoul_pet_t* p = ( ghoul_pet_t* ) player -> cast_pet();
      death_knight_t* o = p -> owner -> cast_death_knight();

      id = 91778;
      aoe = 2;
      parse_data( o -> player_data );
    }

    virtual bool ready()
    {
      ghoul_pet_t* p = ( ghoul_pet_t* ) player -> cast_pet();
      death_knight_t* o = p -> owner -> cast_death_knight();

      if ( ! o -> buffs_dark_transformation -> check() )
        return false;

      return ghoul_pet_attack_t::ready();
    }
  };

  virtual void init_base()
  {
    // FIXME: Level 80/85 values did they change?
    attribute_base[ ATTR_STRENGTH  ] = 331;
    attribute_base[ ATTR_AGILITY   ] = 856;
    attribute_base[ ATTR_STAMINA   ] = 361;
    attribute_base[ ATTR_INTELLECT ] = 65;
    attribute_base[ ATTR_SPIRIT    ] = 109;

    base_attack_power = -20;
    initial_attack_power_per_strength = 2.0;
    initial_attack_power_per_agility  = 1.0;

    initial_attack_crit_per_agility = rating_t::interpolate( level, 0.01/25.0, 0.01/40.0, 0.01/83.3 );

    resource_base[ RESOURCE_ENERGY ] = 100;
    base_energy_regen_per_second  = 10;
  }

  virtual double strength() SC_CONST
  {
    death_knight_t* o = owner -> cast_death_knight();
    double a = attribute[ ATTR_STRENGTH ];
    double strength_scaling = 1.0; // % of str ghould gets from the DK
    strength_scaling += o -> glyphs.raise_dead * .4; // But the glyph is additive!

    // Perma Ghouls are updated constantly
    if ( o -> passives.master_of_ghouls -> ok() )
    {
      a += o -> strength() * strength_scaling;
    }
    else
    {
      a += snapshot_strength * strength_scaling;
    }
    a *= composite_attribute_multiplier( ATTR_STRENGTH );
    return a;
  }

  virtual void summon( double duration=0 )
  {
    death_knight_t* o = owner -> cast_death_knight();
    pet_t::summon( duration );
    // Pets don't seem to inherit their master's crit at the moment.
    snapshot_crit     = o -> composite_pet_attack_crit();
    snapshot_haste    = o -> composite_attack_haste();
    snapshot_hit      = o -> composite_attack_hit();
    snapshot_strength = o -> strength();
    o -> active_ghoul = this;
  }

  virtual void dismiss()
  {
    death_knight_t* o = owner -> cast_death_knight();
    pet_t::dismiss();
    o -> active_ghoul = 0;
  }


  virtual double composite_attack_crit() SC_CONST
  {
    // Ghouls recieve 100% of their master's crit
    death_knight_t* o = owner -> cast_death_knight();

    // Perma Ghouls are updated constantly
    if ( o -> passives.master_of_ghouls -> ok() )
    {
      return o -> composite_pet_attack_crit();
    }
    else
    {
      return snapshot_crit;
    }
  }

  virtual double composite_attack_expertise() SC_CONST
  {
    death_knight_t* o = owner -> cast_death_knight();

    // Perma Ghouls are updated constantly
    if ( o -> passives.master_of_ghouls -> ok() )
    {
      // See comment above about expertise and pets.
      return std::max( o -> composite_attack_hit() / 0.25, o -> composite_attack_expertise() );
    }
    else
    {
      return snapshot_hit;
    }
  }

  virtual double composite_attack_haste() SC_CONST
  {
    // Ghouls receive 100% of their master's haste.
    // http://elitistjerks.com/f72/t42606-pet_discussion_garg_aotd_ghoul/
    death_knight_t* o = owner -> cast_death_knight();

    // Perma Ghouls are updated constantly
    if ( o -> passives.master_of_ghouls -> ok() )
    {
      return o -> composite_attack_haste();
    }
    else
    {
      return snapshot_haste;
    }
  }

  virtual double composite_attack_hit() SC_CONST
  {
    // Hit is rounded down, 7.99% hit is 7%
    death_knight_t* o = owner -> cast_death_knight();

    // Perma Ghouls are updated constantly
    if ( o -> passives.master_of_ghouls -> ok() )
    {
      return o -> composite_attack_hit();
    }
    else
    {
      return snapshot_hit;
    }
  }

  virtual int primary_resource() SC_CONST
  {
    return RESOURCE_ENERGY;
  }

  virtual void regen( double periodicity )
  {
    periodicity *= 1.0 + composite_attack_haste();

    player_t::regen( periodicity );
  }

  virtual action_t* create_action( const std::string& name, const std::string& options_str )
  {
    if ( name == "auto_attack"    ) return new    ghoul_pet_auto_attack_t( this );
    if ( name == "claw"           ) return new           ghoul_pet_claw_t( this );
    if ( name == "sweeping_claws" ) return new ghoul_pet_sweeping_claws_t( this );

    return pet_t::create_action( name, options_str );
  }
};

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
  bool   additive_factors;

  death_knight_attack_t( const char* n, death_knight_t* p, bool special = false ) :
    attack_t( n, p, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_NONE, special ),
    requires_weapon( true ),
    cost_blood( 0 ),cost_frost( 0 ),cost_unholy( 0 ),convert_runes( 0 ),
    additive_factors( false )
  {
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;
    may_crit   = true;
    may_glance = false;
  }

  death_knight_attack_t( const char* n, uint32_t id, death_knight_t* p ) :
    attack_t( n, id, p, 0, true ),
    requires_weapon( true ),
    cost_blood( 0 ),cost_frost( 0 ),cost_unholy( 0 ),convert_runes( 0 ),
    additive_factors( false )
  {
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;
    may_crit   = true;
    may_glance = false;
  }

  virtual void   reset();

  virtual void   consume_resource();
  virtual void   execute();
  virtual void   player_buff();
  virtual double calculate_weapon_damage();
  virtual void   target_debuff( player_t* t, int dmg_type );
  virtual bool   ready();
  virtual double swing_haste() SC_CONST;
};

// ==========================================================================
// Death Knight Spell
// ==========================================================================

void extract_rune_cost( spell_id_t* spell, int* cost_blood, int* cost_frost, int* cost_unholy )
{
  // Rune costs appear to be in binary: 0a0b0c where 'c' is whether the ability
  // costs a blood rune, 'b' is whether it costs an unholy rune, and 'a'
  // is whether it costs a frost rune.
  
  uint32_t spell_id = spell -> spell_id();
  if ( spell_id == 0 ) return;

  uint32_t rune_cost = spell -> rune_cost();
  *cost_blood  =        rune_cost & 0x1;
  *cost_unholy = ( rune_cost >> 2 ) & 0x1;
  *cost_frost  = ( rune_cost >> 4 ) & 0x1;
}

struct death_knight_spell_t : public spell_t
{
  int    cost_blood;
  int    cost_frost;
  int    cost_unholy;
  double convert_runes;
  bool   use[RUNE_SLOT_MAX];

  death_knight_spell_t( const char* n, death_knight_t* p, int r=RESOURCE_NONE, const school_type s=SCHOOL_PHYSICAL, int t=TREE_NONE ) :
    spell_t( n, p, r, s, t ),
    cost_blood( 0 ),cost_frost( 0 ),cost_unholy( 0 ),convert_runes( false )
  {
    _init_dk_spell();
  }

  death_knight_spell_t( const char* n, uint32_t id, death_knight_t* p ) :
    spell_t( n, id, p ),
    cost_blood( 0 ),cost_frost( 0 ),cost_unholy( 0 ),convert_runes( false )
  {
    _init_dk_spell();
  }

  void _init_dk_spell()
  {
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
  }

  virtual void   reset();

  virtual void   consume_resource();
  virtual void   execute();
  virtual void   player_buff();
  virtual void   target_debuff( player_t* t, int dmg_type );
  virtual bool   ready();
};

// ==========================================================================
// Local Utility Functions
// ==========================================================================

// Count Runes ==============================================================

static int count_runes( player_t* player )
{
  death_knight_t* p = player -> cast_death_knight();
  int count_by_type[RUNE_SLOT_MAX / 2] = { 0, 0, 0 }; // blood, frost, unholy

  for ( int i = 0; i < RUNE_SLOT_MAX / 2; ++i )
    count_by_type[i] = ( ( int )p -> _runes.slot[2 * i].is_ready() +
                         ( int )p -> _runes.slot[2 * i + 1].is_ready() );

  return count_by_type[0] + ( count_by_type[1] << 8 ) + ( count_by_type[2] << 16 );
}

// Count Death Runes ========================================================

static int count_death_runes( death_knight_t* p, bool inactive )
{
  // Getting death rune count is a bit more complicated as it depends
  // on talents which runetype can be converted to death runes
  int count = 0;
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
  {
    dk_rune_t& r = p -> _runes.slot[i];
    if ( ( inactive || r.is_ready() ) && r.is_death() )
      ++count;
  }
  return count;
}

// Consume Runes ============================================================

static void consume_runes( player_t* player, const bool* use, bool convert_runes = false )
{
  death_knight_t* p = player -> cast_death_knight();

  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
  {
    if ( use[i] )
    {
      // Show the consumed type of the rune
      // Not the type it is after consumption
      int consumed_type = p -> _runes.slot[i].type;
      p -> _runes.slot[i].consume( convert_runes );

      if ( p -> sim -> log )
        log_t::output( p -> sim, "%s consumes rune #%d, type %d", p -> name(), i, consumed_type );
    }
  }

  if ( p -> sim -> log )
  {
    log_rune_status( p );
  }

  if ( count_runes( p ) == 0 )
    p -> buffs_tier10_4pc_melee -> trigger( 1, 0.03 );
}

// Group Runes ==============================================================

static bool group_runes ( player_t* player, int blood, int frost, int unholy, bool* group )
{
  death_knight_t* p = player -> cast_death_knight();
  int cost[]  = { blood + frost + unholy, blood, frost, unholy };
  bool use[RUNE_SLOT_MAX] = { false };

  // Selecting available non-death runes to satisfy cost
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
  {
    dk_rune_t& r = p -> _runes.slot[i];
    if ( r.is_ready() && ! r.is_death() && cost[r.get_type()] > 0 )
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
    if ( r.is_ready() && r.is_death() )
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

// ==========================================================================
// Triggers
// ==========================================================================

// Trigger Blood Caked Blade ================================================

static void trigger_blood_caked_blade( action_t* a )
{
  death_knight_t* p = a -> player -> cast_death_knight();

  if ( ! p -> talents.blood_caked_blade -> rank() )
    return;

  double chance = p -> talents.blood_caked_blade -> rank() * 0.10;

  if ( p -> rng_blood_caked_blade -> roll( chance ) )
  {
    struct bcb_t : public death_knight_attack_t
    {
      bcb_t( death_knight_t* player ) :
        death_knight_attack_t( "blood_caked_blade", player -> talents.blood_caked_blade -> spell_id(), player )
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

      virtual void target_debuff( player_t* t, int dmg_type )
      {
        death_knight_attack_t::target_debuff( t, dmg_type );
        death_knight_t* p = player -> cast_death_knight();

        target_multiplier *= 0.25 + p -> diseases() * 0.125;
      }
    };

    if ( ! p -> active_blood_caked_blade ) p -> active_blood_caked_blade = new bcb_t( p );
    p -> active_blood_caked_blade -> weapon = a -> weapon;
    p -> active_blood_caked_blade -> execute();
  }
}

// Trigger Brittle Bones ====================================================

static void trigger_brittle_bones( spell_t* s, player_t* t )
{
  death_knight_t* p = s -> player -> cast_death_knight();
  if ( ! p -> talents.brittle_bones -> rank() )
    return;

  double bb_value = p -> talents.brittle_bones -> effect_base_value( 2 ) / 100.0;

  if ( bb_value >= t -> debuffs.brittle_bones -> current_value )
  {
    t -> debuffs.brittle_bones -> trigger( 1, bb_value );
    t -> debuffs.brittle_bones -> source = p;
  }
}

// Trigger Ebon Plaguebringer ===============================================

static void trigger_ebon_plaguebringer( action_t* a )
{
  death_knight_t* p = a -> player -> cast_death_knight();
  if ( ! p -> talents.ebon_plaguebringer -> rank() )
    return;

  // Each DK gets their own ebon plaguebringer debuff, but we only track one a time, so fake it
  p -> buffs_ebon_plaguebringer -> trigger();

  if ( a -> sim -> overrides.ebon_plaguebringer )
    return;

  double duration = 21.0 + util_t::talent_rank( p -> talents.epidemic -> effect_base_value( 1 ), 3, 3, 9, 12 ); 
  if ( a -> target -> debuffs.ebon_plaguebringer -> remains_lt( duration ) )
  {
    a -> target -> debuffs.ebon_plaguebringer -> buff_duration = duration;
    a -> target -> debuffs.ebon_plaguebringer -> trigger( 1, 8.0 );
    a -> target -> debuffs.ebon_plaguebringer -> source = p;
  }
}

// Trigger Unholy Blight ====================================================

static void trigger_unholy_blight( action_t* a, double death_coil_dmg )
{
  death_knight_t* p = a -> player -> cast_death_knight();

  if ( ! p -> talents.unholy_blight -> rank() )
    return;

  struct unholy_blight_t : public death_knight_spell_t
  {
    unholy_blight_t( death_knight_t* player ) :
      death_knight_spell_t( "unholy_blight", player -> talents.unholy_blight -> spell_id(), player )
    {
      base_tick_time = 1.0;
      num_ticks      = 10;
      background     = true;
      proc           = true;
      may_crit       = false;
      may_resist     = false;
      may_miss       = false;
      hasted_ticks   = false;

      reset();
    }

    void target_debuff( player_t* t, int dmg_type )
    {
      // no debuff effect
    }

    void player_buff()
    {
      // no buffs
    }
  };

  if ( ! p -> active_unholy_blight )
    p -> active_unholy_blight = new unholy_blight_t( p );

  double unholy_blight_dmg = death_coil_dmg * p -> talents.unholy_blight -> effect_base_value( 1 ) / 100.0;

  dot_t* dot = p -> active_unholy_blight -> dot;

  if ( dot -> ticking )
  {
    unholy_blight_dmg += p -> active_unholy_blight -> base_td * dot -> ticks();

    p -> active_unholy_blight -> cancel();
  }
  p -> active_unholy_blight -> base_td = unholy_blight_dmg / p -> active_unholy_blight -> num_ticks;
  p -> active_unholy_blight -> execute();
}

// ==========================================================================
// Death Knight Attack Methods
// ==========================================================================

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
  if ( rp_gain > 0 )
  {
    if ( result_is_hit() )
    {
      double real_rp_gain = rp_gain;
      if ( p -> buffs_frost_presence -> check() )
      {
        real_rp_gain *= 1.0 + sim -> sim_data.effect_base_value( 48266, E_APPLY_AURA, A_329 ) / 100.0;
      }
      if ( p -> talents.improved_frost_presence -> rank() && ! p -> buffs_frost_presence -> check() )
      {
        real_rp_gain *= 1.0 + p -> talents.improved_frost_presence -> effect_base_value( 1 ) / 100.0;
      }
      p -> resource_gain( RESOURCE_RUNIC, real_rp_gain, p -> gains_rune_abilities );
    }
  }
  else
  {
    attack_t::consume_resource();
  }

  if ( result_is_hit() )
    consume_runes( player, use, convert_runes == 0 ? false : sim->roll( convert_runes ) == 1 );
  else
    refund_power( this );
}

// death_knight_attack_t::execute() =========================================

void death_knight_attack_t::execute()
{
  death_knight_t* p = player -> cast_death_knight();

  attack_t::execute();

  if ( result_is_hit() )
  {
    p -> buffs_bloodworms -> trigger();
    if ( school == SCHOOL_FROST || school == SCHOOL_SHADOW )
      if ( ! proc )
        p -> buffs_rune_of_cinderglacier -> decrement();
  }
}

// death_knight_attack_t::calculate_weapon_damage() =========================

double death_knight_attack_t::calculate_weapon_damage()
{
  double dmg = attack_t::calculate_weapon_damage();

  death_knight_t* p = player -> cast_death_knight();

  if ( weapon -> slot == SLOT_OFF_HAND && p -> talents.nerves_of_cold_steel -> rank() )
  {
    dmg *= 1.0 + p -> talents.nerves_of_cold_steel -> effect_base_value( 2 ) / 100.0;
  }

  return dmg;
}

// death_knight_attack_t::player_buff() =====================================

void death_knight_attack_t::player_buff()
{
  death_knight_t* p = player -> cast_death_knight();

  attack_t::player_buff();
  
  if ( ( school == SCHOOL_FROST || school == SCHOOL_SHADOW ) )
    if ( ! proc )
      player_multiplier *= 1.0 + p -> buffs_rune_of_cinderglacier -> value();

  if ( p -> main_hand_attack -> weapon -> group() == WEAPON_2H )
    player_multiplier *= 1.0 + p -> talents.might_of_the_frozen_wastes -> effect_base_value( 3 ) / 100.0;

  // 3.3 Scourge Strike: Shadow damage is calcuted with some buffs
  // being additive Desolation, Bone Shield, Black Ice, Blood
  // Presence.  We divide away the multiplier as if they weren't
  // additive then multiply by the additive multiplier to get the
  // correct value since composite_player_multiplier will already have
  // factored these in.
  if ( additive_factors )
  {
    std::vector<double> sum_factors;

    sum_factors.push_back( p -> buffs_frost_presence -> value() );
    sum_factors.push_back( p -> buffs_bone_shield -> value() );

    double sum = 0;
    double divisor = 1.0;
    for ( size_t i = 0; i < sum_factors.size(); ++i )
    {
      sum += sum_factors[i];
      divisor *= 1.0 + sum_factors[i];
    }
    player_multiplier = player_multiplier * ( 1.0 + sum ) / divisor;
  }

  player_multiplier *= 1.0 + p -> buffs_tier10_4pc_melee -> value();

  if ( p -> mastery.frozen_heart -> ok() && school == SCHOOL_FROST )
  {
    player_multiplier *= 1.0 + p -> mastery.frozen_heart -> base_value( E_APPLY_AURA, A_DUMMY ) * p -> composite_mastery();
  }
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

  return group_runes( p, cost_blood, cost_frost, cost_unholy, use );
}

// death_knight_attack_t::swing_haste() =====================================

double death_knight_attack_t::swing_haste() SC_CONST
{
    double haste = attack_t::swing_haste();
    death_knight_t* p = player -> cast_death_knight();

  if ( p -> passives.icy_talons -> ok() )
    haste *= 1.0 / ( 1.0 + p -> passives.icy_talons -> effect_base_value( 1 ) / 100.0 );

  if ( p -> talents.improved_icy_talons -> rank() )
    haste *= 1.0 / ( 1.0 + p -> talents.improved_icy_talons -> effect_base_value( 3 ) / 100.0 );

    return haste;
}

// death_knight_attack_t::target_debuff =====================================

void death_knight_attack_t::target_debuff( player_t* t, int dmg_type )
{
  attack_t::target_debuff( t, dmg_type );
  death_knight_t* p = player -> cast_death_knight();

  if ( dmg_type == SCHOOL_FROST  )
  {
    target_multiplier *= 1.0 + p -> buffs_rune_of_razorice -> value();
  }
}

// ==========================================================================
// Death Knight Spell Methods
// ==========================================================================

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
  if ( rp_gain > 0 )
  {
    if ( result_is_hit() )
    {
      double real_rp_gain = rp_gain;
      if ( p -> buffs_frost_presence -> check() )
      {
        real_rp_gain *= 1.0 + sim -> sim_data.effect_base_value( 48266, E_APPLY_AURA, A_329 ) / 100.0;
      }
      if ( p -> talents.improved_frost_presence -> rank() && ! p -> buffs_frost_presence -> check() )
      {
        real_rp_gain *= 1.0 + p -> talents.improved_frost_presence -> effect_base_value( 1 ) / 100.0;
      }
      p -> resource_gain( RESOURCE_RUNIC, real_rp_gain, p -> gains_rune_abilities );
    }
  }
  else
  {
    spell_t::consume_resource();
  }

  if ( result_is_hit() )
    consume_runes( player, use, convert_runes == 0 ? false : sim->roll( convert_runes ) == 1 );
}

void death_knight_spell_t::execute()
{
  spell_t::execute();

  if ( result_is_hit() )
  {
    if ( school == SCHOOL_FROST || school == SCHOOL_SHADOW )
    {
      death_knight_t* p = player -> cast_death_knight();
      p -> buffs_rune_of_cinderglacier -> decrement();
    }
  }
}

// death_knight_spell_t::player_buff() ======================================

void death_knight_spell_t::player_buff()
{
  death_knight_t* p = player -> cast_death_knight();

  spell_t::player_buff();

  player_multiplier *= 1.0 + p -> buffs_tier10_4pc_melee -> value();

  if ( p -> mastery.frozen_heart -> ok() && school == SCHOOL_FROST )
  {
    player_multiplier *= 1.0 + p -> mastery.frozen_heart -> base_value( E_APPLY_AURA, A_DUMMY ) * p -> composite_mastery();
  }

  if ( ( school == SCHOOL_FROST || school == SCHOOL_SHADOW ) )
    player_multiplier *= 1.0 + p -> buffs_rune_of_cinderglacier -> value();

  if ( sim -> debug )
    log_t::output( sim, "death_knight_spell_t::player_buff: %s hit=%.2f crit=%.2f power=%.2f penetration=%.0f, p_mult=%.0f",
                   name(), player_hit, player_crit, player_spell_power, player_penetration, player_multiplier );
}

// death_knight_spell_t::ready() ============================================

bool death_knight_spell_t::ready()
{
  if ( ! spell_t::ready() )
    return false;

  if ( ! player -> in_combat && ! harmful )
    return group_runes( player, 0, 0, 0, use );
  else
    return group_runes( player, cost_blood, cost_frost, cost_unholy, use );
}

// death_knight_spell_t::target_debuff ====================================

void death_knight_spell_t::target_debuff( player_t* t, int dmg_type )
{
  spell_t::target_debuff( t, dmg_type );
  death_knight_t* p = player -> cast_death_knight();

  if ( dmg_type == SCHOOL_FROST  )
  {
    target_multiplier *= 1.0 + p -> buffs_rune_of_razorice -> value();
  }
}

// ==========================================================================
// Death Knight Attacks
// ==========================================================================

// Melee Attack =============================================================

struct melee_t : public death_knight_attack_t
{
  int sync_weapons;
  melee_t( const char* name, death_knight_t* player, int sw ) :
    death_knight_attack_t( name, player ), sync_weapons( sw )
  {
    death_knight_t* p = player -> cast_death_knight();

    may_glance      = true;
    background      = true;
    repeating       = true;
    trigger_gcd     = 0;
    base_cost       = 0;

    if ( p -> dual_wield() )
      base_hit -= 0.19;
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
      if ( weapon -> slot == SLOT_MAIN_HAND )
      {
        p -> buffs_sudden_doom -> trigger( 1, -1, weapon -> proc_chance_on_swing( p -> talents.sudden_doom -> rank() ) );
        // TODO: Confirm PPM for ranks 1/2 http://elitistjerks.com/f72/t110296-frost_dps_|_cataclysm_4_0_3_nothing_lose/p9/#post1869431
        double chance = weapon -> proc_chance_on_swing( util_t::talent_rank( p -> talents.killing_machine -> rank(), 3, 1, 3, 5 ) );
        p -> buffs_killing_machine -> trigger( 1, 1.0, chance );
      }

      if ( p -> dots_blood_plague && p -> dots_blood_plague -> ticking )
        p -> buffs_crimson_scourge -> trigger();
      
      trigger_blood_caked_blade( this );
      if ( p -> buffs_scent_of_blood -> up() )
      {
        p -> resource_gain( RESOURCE_RUNIC, 10, p -> gains_scent_of_blood );
        p -> buffs_scent_of_blood -> decrement();
      }

      if ( p -> rng_might_of_the_frozen_wastes -> roll( p -> talents.might_of_the_frozen_wastes -> proc_chance() ) )
      {
        p -> resource_gain( RESOURCE_RUNIC, sim -> sim_data.effect_base_value( 81331, E_ENERGIZE, A_NONE ) / 10.0, p -> gains_might_of_the_frozen_wastes );
      }
    }
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public death_knight_attack_t
{
  int sync_weapons;

  auto_attack_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_attack_t( "auto_attack", player ), sync_weapons( 0 )
  {
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { "sync_weapons", OPT_BOOL, &sync_weapons },
      { NULL, OPT_UNKNOWN, NULL }
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
    if ( p -> buffs.moving -> check() )
      return false;
    return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// ==========================================================================
// Death Knight Abilities
// ==========================================================================

// Army of the Dead =========================================================

struct army_of_the_dead_t : public death_knight_spell_t
{
  army_of_the_dead_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "army_of_the_dead", 42650, player )
  {
    parse_options( NULL, options_str );

    harmful     = false;
    channeled   = true;
    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    if ( ! p -> in_combat )
    {
      // Pre-casting it before the fight
      int saved_ticks = num_ticks;
      num_ticks = 0;
      channeled = false;
      death_knight_spell_t::execute();
      channeled = true;
      num_ticks = saved_ticks;
      // Because of the new rune regen system in 4.0, it only makes
      // sense to cast ghouls 7-10s before a fight begins so you don't
      // waste rune regen and enter the fight depleted.  So, the time
      // you get for ghouls is 4-6 seconds less.
      player -> summon_pet( "army_of_the_dead_ghoul_8", 35.0 );
    }
    else
    {
      death_knight_spell_t::execute();
      player -> summon_pet( "army_of_the_dead_ghoul_8", 40.0 );
    }
  }
  virtual void consume_resource()
  {
    if ( ! player -> in_combat ) death_knight_spell_t::consume_resource();
  }

  virtual double gcd() SC_CONST
  {
    return player -> in_combat ? death_knight_spell_t::gcd() : 0;
  }
  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> active_army_ghoul )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Blood Boil ===============================================================

struct blood_boil_t : public death_knight_spell_t
{
  blood_boil_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "blood_boil", 48721, player )
  {
    death_knight_t* p = player -> cast_death_knight();

    parse_options( NULL, options_str );

    aoe                = -1;
    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
    direct_power_mod   = 0.08;
    player_multiplier *= 1.0 + p -> talents.crimson_scourge -> mod_additive( P_GENERIC );
  }

  virtual double cost() SC_CONST
  {
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> buffs_crimson_scourge -> up() )
      return 0;
    
    return death_knight_spell_t::cost();
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    base_dd_adder = ( p -> diseases() ? 95 : 0 );
    direct_power_mod  = 0.08 + ( p -> diseases() ? 0.035 : 0 );
    death_knight_spell_t::execute();
    if ( p -> buffs_dancing_rune_weapon -> check() )
      p -> active_dancing_rune_weapon -> drw_blood_boil -> execute();
  }
};

// Blood Plague =============================================================

struct blood_plague_t : public death_knight_spell_t
{
  blood_plague_t( death_knight_t* player ) :
    death_knight_spell_t( "blood_plague", 59879, player )
  {
    death_knight_t* p = player -> cast_death_knight();

    base_td          = 1.0;
    base_tick_time   = 3.0;
    tick_may_crit    = true;
    background       = true;
    num_ticks        = 7 + util_t::talent_rank( p -> talents.epidemic -> effect_base_value( 1 ), 3, 1, 3, 4 );
    tick_power_mod   = 0.055 * 1.15;
    dot_behavior     = DOT_REFRESH;
    base_multiplier *= 1.0 + p -> talents.ebon_plaguebringer -> effect_base_value( 1 ) / 100.0;
    base_multiplier  *= 1.0 + p -> talents.virulence -> mod_additive( P_TICK_DAMAGE );
    may_miss         = false;
    may_crit         = false;
    hasted_ticks     = false;
    reset(); // Not a real action
  }
};

// Blood Strike =============================================================

struct blood_strike_offhand_t : public death_knight_attack_t
{
  blood_strike_offhand_t( death_knight_t* player ) :
    death_knight_attack_t( "blood_strike_offhand", 45902, player )
  {
    death_knight_t* p = player -> cast_death_knight();
    
    background = true;
    weapon     = &( p -> off_hand_weapon );
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::target_debuff( t, dmg_type );

    target_multiplier *= 1 + p -> diseases() * effect_base_value( 3 ) / 100.0;
  }
};

struct blood_strike_t : public death_knight_attack_t
{
  attack_t* oh_attack;

  blood_strike_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_attack_t( "blood_strike", 45902, player ), oh_attack( 0 )
  {
    death_knight_t* p = player -> cast_death_knight();

    parse_options( NULL, options_str );

    weapon = &( p -> main_hand_weapon );

    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );

    if ( p -> passives.blood_of_the_north -> ok() || p -> passives.reaping -> ok() )
      convert_runes = 1.0;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      oh_attack = new blood_strike_offhand_t( p );
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::execute();

    if ( oh_attack )
    {
      if ( p -> rng_threat_of_thassarian -> roll ( p -> talents.threat_of_thassarian -> effect_base_value( 1 ) / 100.0 ) )
      {
        oh_attack -> execute();
      }
    }
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::target_debuff( t, dmg_type );

    target_multiplier *= 1 + p -> diseases() * effect_base_value( 3 ) / 100.0;
  }
};

// Blood Tap ================================================================

struct blood_tap_t : public death_knight_spell_t
{
  blood_tap_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "blood_tap", 45529, player )
  {
    death_knight_t* p = player -> cast_death_knight();

    parse_options( NULL, options_str );
    
    base_cost = 0.0; // Cost is stored as 6 in the DBC for some odd reason
    harmful   = false;
    if ( p -> talents.improved_blood_tap -> rank() )
      cooldown -> duration += p -> talents.improved_blood_tap -> mod_additive( P_COOLDOWN );
  }

  void execute()
  {
    death_knight_spell_t::execute();

    death_knight_t* p = player -> cast_death_knight();

    // Blood tap has some odd behavior.  One of the oddest is that, if
    // you have a death rune on cooldown and a full blood rune, using
    // it will take the death rune off cooldown and turn the blood
    // rune into a death rune!  This allows for a nice sequence for
    // Unholy Death Knights: "IT PS BS SS tap SS" which replaces a
    // blood strike with a scourge strike.

    // Find a non-death blood rune and refresh it.
    bool rune_was_refreshed = false;
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    {
      dk_rune_t& r = p -> _runes.slot[i];
      if ( r.get_type() == RUNE_TYPE_BLOOD && ! r.is_death() && ! r.is_ready() )
      {
        r.fill_rune();
        rune_was_refreshed = true;
        break;
      }
    }
    // Couldn't find a non-death blood rune needing refreshed?
    // Instead, refresh a death rune that is a blood rune.
    if ( ! rune_was_refreshed )
    {
      for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
      {
        dk_rune_t& r = p -> _runes.slot[i];
        if ( r.get_type() == RUNE_TYPE_BLOOD && r.is_death() && ! r.is_ready() )
        {
          r.fill_rune();
          rune_was_refreshed = true;
          break;
        }
      }
    }

    // Now find a ready blood rune and turn it into a death rune.
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    {
      dk_rune_t& r = p -> _runes.slot[i];
      if ( r.get_type() == RUNE_TYPE_BLOOD && ! r.is_death() && r.is_ready() )
      {
        r.type = ( r.type & RUNE_TYPE_MASK ) | RUNE_TYPE_DEATH;
        break;
      }
    }
  }
};

// Bone Shield ==============================================================

struct bone_shield_t : public death_knight_spell_t
{
  bone_shield_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "bone_shield", 49222, player )
  {
    death_knight_t* p = player -> cast_death_knight();
    check_talent( p -> talents.bone_shield -> rank() );

    parse_options( NULL, options_str );

    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
    harmful = false;
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
    if ( ! p -> in_combat )
    {
      // Pre-casting it before the fight, perfect timing would be so
      // that the used rune is ready when it is needed in the
      // rotation.  Assume we casted Bone Shield somewhere between
      // 8-16s before we start fighting.  The cost in this case is
      // zero and we don't cause any cooldown.
      double pre_cast = p -> sim -> range( 8.0, 16.0 );

      cooldown -> duration -= pre_cast;
      p -> buffs_bone_shield -> buff_duration -= pre_cast;

      p -> buffs_bone_shield -> trigger( 1, p -> talents.bone_shield -> effect_base_value( 2 ) / 100.0 );
      death_knight_spell_t::execute();

      cooldown -> duration += pre_cast;
      p -> buffs_bone_shield -> buff_duration += pre_cast;
    }
    else
    {
      p -> buffs_bone_shield -> trigger( 1, p -> talents.bone_shield -> effect_base_value( 2 ) / 100.0 );
      death_knight_spell_t::execute();
    }
  }

  virtual double gcd() SC_CONST
  {
    return player -> in_combat ? death_knight_spell_t::gcd() : 0;
  }
};

// Dancing Rune Weapon ======================================================

struct dancing_rune_weapon_t : public death_knight_spell_t
{
  dancing_rune_weapon_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "dancing_rune_weapon", 49028, player )
  {
    death_knight_t* p = player -> cast_death_knight();
    check_talent( p -> talents.dancing_rune_weapon -> rank() );

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_spell_t::execute();
    p -> buffs_dancing_rune_weapon -> trigger();
    p -> summon_pet( "dancing_rune_weapon", p -> player_data.spell_duration( id ) );
  }
};

// Dark Transformation ======================================================

struct dark_transformation_t : public death_knight_spell_t
{
  dark_transformation_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "dark_transformation", 63560, player )
  {
    death_knight_t* p = player -> cast_death_knight();

    check_talent( p -> talents.dark_transformation -> rank() );

    parse_options( NULL, options_str );

    harmful = false;
    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
    base_cost = 0; // DBC has a value of 9 of an unknown resource
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_spell_t::execute();

    p -> buffs_dark_transformation -> trigger();
    p -> buffs_shadow_infusion -> expire();
  }

  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();

    if ( p -> buffs_shadow_infusion -> check() != 5 )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Death and Decay ==========================================================

struct death_and_decay_t : public death_knight_spell_t
{
  death_and_decay_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "death_and_decay", 43265, player )
  {
    death_knight_t* p = player -> cast_death_knight();

    parse_options( NULL, options_str );

    aoe              = -1;
    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
    tick_power_mod   = 0.064;
    base_dd_min      = p -> player_data.effect_min( id, p -> level, E_PERSISTENT_AREA_AURA, A_PERIODIC_DUMMY );
    base_dd_max      = p -> player_data.effect_max( id, p -> level, E_PERSISTENT_AREA_AURA, A_PERIODIC_DUMMY );
    base_tick_time   = 1.0;
    num_ticks        = 11;
    tick_zero        = true;
    hasted_ticks     = false;
    if ( p -> glyphs.death_and_decay )
      num_ticks     += 5;
    if ( p -> talents.morbidity -> rank() )
      base_multiplier *= 1.0 + p -> talents.morbidity -> mod_additive( P_EFFECT_1 );
  }
};

// Death Coil ===============================================================

struct death_coil_t : public death_knight_spell_t
{
  death_coil_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "death_coil", 47541, player )
  {
    death_knight_t* p = player -> cast_death_knight();

    parse_options( NULL, options_str );

    direct_power_mod = 0.3 * 0.85; // FIX-ME: From Feb 9th Hotfix. Test to confirm value.
    base_dd_min      = p -> player_data.effect_min( id, p -> level, E_DUMMY, A_NONE );
    base_dd_max      = p -> player_data.effect_max( id, p -> level, E_DUMMY, A_NONE );
    base_multiplier *= 1 + p -> talents.morbidity -> mod_additive( P_GENERIC )
                       + p -> glyphs.death_coil * 0.15;
    if ( p -> set_bonus.tier11_2pc_melee() )
      base_crit        += 0.05;

    if ( p -> talents.runic_corruption -> rank() )
      base_cost += p -> talents.runic_corruption -> mod_additive( P_RESOURCE_COST ) / 10.0;
  }

  virtual double cost() SC_CONST
  {
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> buffs_sudden_doom -> check() ) return 0;
    return death_knight_spell_t::cost();
  }

  void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_spell_t::execute();
    p -> buffs_sudden_doom -> decrement();

    if ( p -> buffs_dancing_rune_weapon -> check() )
      p -> active_dancing_rune_weapon -> drw_death_coil -> execute();

    if ( result_is_hit() )
    {
      trigger_unholy_blight( this, direct_dmg );
      p -> trigger_runic_empowerment(); // FIX ME: Confirm this needs to hit to trigger
    }

    if ( ! p -> buffs_dark_transformation -> check() )
      p -> buffs_shadow_infusion -> trigger(); // Doesn't stack while your ghoul is empowered
  }
};

// Death Strike =============================================================

struct death_strike_t : public death_knight_attack_t
{
  death_strike_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_attack_t( "death_strike", 49998, player )
  {
    death_knight_t* p = player -> cast_death_knight();

    parse_options( NULL, options_str );

    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
    if ( p -> passives.blood_rites -> ok() )
      convert_runes = 1.0;

    base_crit       +=     p -> talents.improved_death_strike -> mod_additive( P_CRIT );
    base_multiplier *= 1 + p -> talents.improved_death_strike -> mod_additive( P_GENERIC );
  }

  virtual void consume_resource() { }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    weapon = &( p -> main_hand_weapon );
    death_knight_attack_t::execute();
    death_knight_attack_t::consume_resource();

    if ( p -> buffs_dancing_rune_weapon -> check() )
      p -> active_dancing_rune_weapon -> drw_death_strike -> execute();

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( p -> rng_threat_of_thassarian -> roll ( p -> talents.threat_of_thassarian -> effect_base_value( 1 ) / 100.0 ) )
      {
        weapon = &( p -> off_hand_weapon );
        death_knight_attack_t::execute();
      }
    }
  }

  virtual void player_buff()
  {
    death_knight_attack_t::player_buff();
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> glyphs.death_strike )
    {
      player_multiplier *= 1 + p -> resource_current[ RESOURCE_RUNIC ] / 5 * 0.02;
    }
  }
};

// Empower Rune Weapon ======================================================

struct empower_rune_weapon_t : public death_knight_spell_t
{
  empower_rune_weapon_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "empower_rune_weapon", 47568, player )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_spell_t::execute();

    for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    {
      dk_rune_t& r = p -> _runes.slot[i];
      r.fill_rune();
    }
  }
};

// Festering Strike =========================================================

struct festering_strike_t : public death_knight_attack_t
{
  festering_strike_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_attack_t( "festering_strike", 85948, player )
  {
    death_knight_t* p = player -> cast_death_knight();

    parse_options( NULL, options_str );

    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
    if ( p -> passives.reaping -> ok() )
    {
      convert_runes = 1.0;
    }

    base_multiplier *= 1.0 + p -> talents.rage_of_rivendare -> mod_additive( P_GENERIC );
  }

  virtual void consume_resource() { }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    weapon = &( p -> main_hand_weapon );
    death_knight_attack_t::execute();
    death_knight_attack_t::consume_resource();

    if ( result_is_hit() )
    {
      if ( p -> dots_blood_plague -> ticking ) p -> dots_blood_plague -> action -> extend_duration( 2 );
      if ( p -> dots_frost_fever  -> ticking ) p -> dots_frost_fever  -> action -> extend_duration( 2 );
      if ( p -> dots_blood_plague -> ticking || p -> dots_frost_fever -> ticking )
        trigger_ebon_plaguebringer( this );
    }
  }
};

// Frost Fever ==============================================================

struct frost_fever_t : public death_knight_spell_t
{
  frost_fever_t( death_knight_t* player ) :
    death_knight_spell_t( "frost_fever", 59921, player )
  {
    death_knight_t* p = player -> cast_death_knight();

    base_td           = 1.0;
    base_tick_time    = 3.0;
    hasted_ticks      = false;
    may_miss          = false;
    may_crit          = false;
    background        = true;
    tick_may_crit     = true;
    dot_behavior      = DOT_REFRESH;
    base_crit_bonus_multiplier /= 2.0;  // current bug, FF crits for 150% instead of 200%, which means half the bonus multiplier.
    num_ticks         = 7 + util_t::talent_rank( p -> talents.epidemic -> effect_base_value( 1 ), 3, 1, 3, 4 );
    tick_power_mod    = 0.055 * 1.15;
    base_multiplier  *= 1.0 + p -> glyphs.icy_touch * 0.2
                        + p -> talents.ebon_plaguebringer -> effect_base_value( 1 ) / 100.0;
    base_multiplier  *= 1.0 + p -> talents.virulence -> effect_base_value( 1 ) / 100.0;
    reset(); // Not a real action
  }

  virtual void travel( player_t* t, int travel_result, double travel_dmg )
  {
    death_knight_spell_t::travel( t, travel_result, travel_dmg );

    if ( result_is_hit( travel_result ) )
      trigger_brittle_bones( this, t );
  }

  virtual void last_tick()
  {
    death_knight_spell_t::last_tick();

    if ( target -> debuffs.brittle_bones -> check()
         && target -> debuffs.brittle_bones -> source
         && target -> debuffs.brittle_bones -> source == player )
      target -> debuffs.brittle_bones -> expire();
  }
};

// Frost Strike =============================================================

struct frost_strike_offhand_t : public death_knight_attack_t
{
  frost_strike_offhand_t( death_knight_t* player ) :
    death_knight_attack_t( "frost_strike_offhand", 66196, player )
  {
    death_knight_t* p = player -> cast_death_knight();

    background = true;
    weapon     = &( p -> off_hand_weapon );
    // FIXME: Base Value is halfed in the DBC, should it be?

    if ( p -> set_bonus.tier11_2pc_melee() )
      base_crit += 0.05;
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::execute();
   
    if( result_is_hit() )
      p -> trigger_runic_empowerment(); // FIX ME: Does DW get 2 chances to proc this?
  }

  virtual void player_buff()
  {
    death_knight_attack_t::player_buff();
    death_knight_t* p = player -> cast_death_knight();

    player_crit += p -> buffs_killing_machine -> value();
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    death_knight_attack_t::target_debuff( t, dmg_type);

    death_knight_t* p = player -> cast_death_knight();

    if ( t -> health_percentage() < 35 )
     player_multiplier *= 1.0 + p -> talents.merciless_combat -> effect_base_value( 1 ) / 100.0;
  }
};

struct frost_strike_t : public death_knight_attack_t
{
  attack_t* oh_attack;

  frost_strike_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_attack_t( "frost_strike", 49143, player ), oh_attack( 0 )
  {
    death_knight_t* p = player -> cast_death_knight();
    check_spec( TREE_FROST );

    parse_options( NULL, options_str );

    weapon     = &( p -> main_hand_weapon );
    base_cost -= p -> glyphs.frost_strike * 8;
    if ( p -> set_bonus.tier11_2pc_melee() )
      base_crit += 0.05;

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      oh_attack = new frost_strike_offhand_t( p );
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();    
    death_knight_attack_t::execute();
   
    if( result_is_hit() )
      p -> trigger_runic_empowerment(); // FIX ME: Does DW get 2 chances to proc this?

    if ( oh_attack )
    {
      if ( p -> rng_threat_of_thassarian -> roll ( p -> talents.threat_of_thassarian -> effect_base_value( 1 ) / 100.0 ) )
      {
        oh_attack -> execute();
      }
    }
    p -> buffs_killing_machine -> expire();
  }

  virtual void player_buff()
  {
    death_knight_attack_t::player_buff();
    death_knight_t* p = player -> cast_death_knight();

    player_crit += p -> buffs_killing_machine -> value();
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    death_knight_attack_t::target_debuff( t, dmg_type);

    death_knight_t* p = player -> cast_death_knight();

    if ( t -> health_percentage() < 35 )
     player_multiplier *= 1.0 + p -> talents.merciless_combat -> effect_base_value( 1 ) / 100.0;
  }
};

// Heart Strike =============================================================

struct heart_strike_t : public death_knight_attack_t
{
  heart_strike_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_attack_t( "heart_strike", 55050, player )
  {
    death_knight_t* p = player -> cast_death_knight();
    check_spec( TREE_BLOOD );

    parse_options( NULL, options_str );

    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );

    base_multiplier *= 1 + p -> set_bonus.tier10_2pc_melee() * 0.07
                       + p -> glyphs.heart_strike            * 0.30;
    
    aoe = 2;                   
    base_add_multiplier *= 0.75;
  }

  void execute()
  {
    death_knight_attack_t::execute();
    death_knight_t* p = player -> cast_death_knight();

    if ( result_is_hit() )
    {
      if ( p -> buffs_dancing_rune_weapon -> check() )
      {
        p -> active_dancing_rune_weapon -> drw_heart_strike -> execute();
      }
    }
  }

  void target_debuff( player_t* t, int dmg_type )
  {
    death_knight_t* p = player -> cast_death_knight();

    death_knight_attack_t::target_debuff( t, dmg_type );

    target_multiplier *= 1 + p -> diseases() * effect_base_value( 3 ) / 100.0;
  }
};

// Horn of Winter============================================================

struct horn_of_winter_t : public death_knight_spell_t
{
  double bonus;

  horn_of_winter_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "horn_of_winter", 57330, player ), bonus( 0 )
  {
    death_knight_t* p = player -> cast_death_knight();

    parse_options( NULL, options_str );

    harmful = false;
    bonus   = p -> player_data.effect_min( id, p -> level, E_APPLY_AURA, A_MOD_STAT );
  }

  virtual void execute()
  {
    if ( sim -> log )
      log_t::output( sim, "%s performs %s", player -> name(), name() );

    update_ready();

    death_knight_t* p = player -> cast_death_knight();
    if ( ! sim -> overrides.horn_of_winter )
    {
      sim -> auras.horn_of_winter -> buff_duration = 120.0 + p -> glyphs.horn_of_winter * 60.0;
      sim -> auras.horn_of_winter -> trigger( 1, bonus );
    }

    player -> resource_gain( RESOURCE_RUNIC, 10, p -> gains_horn_of_winter );
  }

  virtual double gcd() SC_CONST
  {
    return player -> in_combat ? death_knight_spell_t::gcd() : 0;
  }
};

// Howling Blast ============================================================

struct howling_blast_t : public death_knight_spell_t
{
  howling_blast_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "howling_blast", 49184, player )
  {
    death_knight_t* p = player -> cast_death_knight();
    check_talent( p -> talents.howling_blast -> rank() );

    parse_options( NULL, options_str );

    extract_rune_cost( this , &cost_blood, &cost_frost, &cost_unholy );
    aoe               = 1; // Change to -1 once we support meteor split effect
    direct_power_mod  = 0.4;

    if ( ! p -> frost_fever )
      p -> frost_fever = new frost_fever_t( p );
  }

  virtual double cost() SC_CONST
  {
    // Rime also prevents getting RP because there are no runes used!
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> buffs_rime -> check() )
      return 0;
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
        if ( p -> frost_fever )
          p -> frost_fever -> execute();
      }
      if ( p -> talents.chill_of_the_grave -> rank() )
      {
        p -> resource_gain( RESOURCE_RUNIC, p -> talents.chill_of_the_grave -> effect_base_value( 1 ) / 10.0, p -> gains_chill_of_the_grave );
      }
    }
    p -> buffs_rime -> expire();
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    death_knight_spell_t::target_debuff( t, dmg_type);

    death_knight_t* p = player -> cast_death_knight();

    if ( t -> health_percentage() < 35 )
     player_multiplier *= 1.0 + p -> talents.merciless_combat -> effect_base_value( 1 ) / 100.0;
  }

  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();

    if ( p -> buffs_rime -> check() )
    {
      // If Rime is up, runes are no restriction.
      cost_frost  = 0;
      bool rime_ready = death_knight_spell_t::ready();
      cost_frost  = 1;
      return rime_ready;
    }
    return death_knight_spell_t::ready();
  }
};

// Icy Touch ================================================================

struct icy_touch_t : public death_knight_spell_t
{
  icy_touch_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "icy_touch", 45477, player )
  {
    parse_options( NULL, options_str );

    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
    direct_power_mod = 0.2;

    death_knight_t* p = player -> cast_death_knight();

    if ( ! p -> frost_fever )
      p -> frost_fever = new frost_fever_t( p );
  }

  virtual double cost() SC_CONST
  {
    // Rime also prevents getting RP because there are no runes used!
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> buffs_rime -> check() )
      return 0;
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

    if ( p -> buffs_dancing_rune_weapon -> check() )
      p -> active_dancing_rune_weapon -> drw_icy_touch -> execute();

    if ( result_is_hit() )
    {
      if ( p -> talents.chill_of_the_grave -> rank() )
      {
        p -> resource_gain( RESOURCE_RUNIC, p -> talents.chill_of_the_grave -> effect_base_value( 1 ) / 10.0, p -> gains_chill_of_the_grave );
      }
      if ( p -> frost_fever )
        p -> frost_fever -> execute();
      trigger_ebon_plaguebringer( this );
    }
    p -> buffs_killing_machine -> expire();
    p -> buffs_rime -> expire();
  }

  virtual void player_buff()
  {
    death_knight_spell_t::player_buff();
    death_knight_t* p = player -> cast_death_knight();

    player_crit += p -> buffs_killing_machine -> value();
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    death_knight_spell_t::target_debuff( t, dmg_type );

    death_knight_t* p = player -> cast_death_knight();

    if ( t -> health_percentage() < 35 )
     player_multiplier *= 1.0 + p -> talents.merciless_combat -> effect_base_value( 1 ) / 100.0;
  }

  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();

    if ( p -> buffs_rime -> check() )
    {
      // If Rime is up, runes are no restriction.
      cost_frost  = 0;
      bool rime_ready = death_knight_spell_t::ready();
      cost_frost  = 1;
      return rime_ready;
    }
    return death_knight_spell_t::ready();
  }
};

// Mind Freeze ==============================================================

struct mind_freeze_t : public death_knight_spell_t
{
  mind_freeze_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "mind_freeze", 47528, player )
  {
    death_knight_t* p = player -> cast_death_knight();

    parse_options( NULL, options_str );

    if ( p -> talents.endless_winter -> rank() )
      base_cost += p -> talents.endless_winter -> mod_additive( P_RESOURCE_COST ) / 10.0;

    may_miss = may_resist = may_glance = may_block = may_dodge = may_parry = may_crit = false;
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Necrotic Strike ==========================================================

struct necrotic_strike_t : public death_knight_attack_t
{
  necrotic_strike_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_attack_t( "necrotic_strike", 73975, player )
  {
    parse_options( NULL, options_str );

    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
  }
};

// Obliterate ===============================================================

struct obliterate_offhand_t : public death_knight_attack_t
{
  obliterate_offhand_t( death_knight_t* player ) :
    death_knight_attack_t( "obliterate_offhand", 66198, player )
  {
    death_knight_t* p = player -> cast_death_knight();

    background = true;
    weapon     = &( p -> off_hand_weapon );
    // FIXME: Base Value is halfed in the DBC, should it be?

    if ( p -> glyphs.obliterate )
      weapon_multiplier *= 1.2;

    base_multiplier *= 1.0 + p -> talents.annihilation -> mod_additive( P_GENERIC )
                       + p -> set_bonus.tier10_2pc_melee() * 0.10;
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::execute();

    if ( result_is_hit() )
    {
      if ( p -> buffs_rime -> trigger() )
      {
        p -> cooldowns_howling_blast -> reset();
        update_ready();
      }
    }
  }

  virtual void player_buff()
  {
    death_knight_attack_t::player_buff();
    death_knight_t* p = player -> cast_death_knight();

    player_crit += p -> buffs_killing_machine -> value();
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::target_debuff( t, dmg_type );

    target_multiplier *= 1 + p -> diseases() * 0.125;

    if ( t -> health_percentage() < 35 )
      player_multiplier *= 1.0 + p -> talents.merciless_combat -> effect_base_value( 1 ) / 100.0;
  }
};

struct obliterate_t : public death_knight_attack_t
{
  attack_t* oh_attack;

  obliterate_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_attack_t( "obliterate", 49020, player ), oh_attack( 0 )
  {
    death_knight_t* p = player -> cast_death_knight();

    parse_options( NULL, options_str );

    weapon = &( p -> main_hand_weapon );

    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
    if ( p -> passives.blood_rites -> ok() )
      convert_runes = 1.0;

    if ( p -> glyphs.obliterate )
      weapon_multiplier *= 1.2;

    base_multiplier *= 1.0 + p -> talents.annihilation -> mod_additive( P_GENERIC )
                       + p -> set_bonus.tier10_2pc_melee() * 0.10;
    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      oh_attack = new obliterate_offhand_t( p );
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::execute();

    if ( result_is_hit() )
    {
      if ( p -> talents.chill_of_the_grave -> rank() )
      {
        p -> resource_gain( RESOURCE_RUNIC, p -> talents.chill_of_the_grave -> effect_base_value( 1 ) / 10.0, p -> gains_chill_of_the_grave );
      }
      if ( p -> buffs_rime -> trigger() )
      {
        p -> cooldowns_howling_blast -> reset();
        update_ready();
      }
    }

    if ( oh_attack )
    {
      if ( p -> rng_threat_of_thassarian -> roll ( p -> talents.threat_of_thassarian -> effect_base_value( 1 ) / 100.0 ) )
      {
        oh_attack -> execute();
      }
    }

    p -> buffs_killing_machine -> expire();
  }

  virtual void player_buff()
  {
    death_knight_attack_t::player_buff();
    death_knight_t* p = player -> cast_death_knight();

    player_crit += p -> buffs_killing_machine -> value();
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::target_debuff( t, dmg_type );

    target_multiplier *= 1 + p -> diseases() * 0.125;

    if ( t -> health_percentage() < 35 )
      player_multiplier *= 1.0 + p -> talents.merciless_combat -> effect_base_value( 1 ) / 100.0;
  }
};

// Outbreak =================================================================

struct outbreak_t : public death_knight_spell_t
{
  outbreak_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "outbreak", 77575, player )
  {
    parse_options( NULL, options_str );

    may_crit   = false;

    death_knight_t* p = player -> cast_death_knight();

    if ( ! p -> blood_plague )
      p -> blood_plague = new blood_plague_t( p );

    if ( ! p -> frost_fever )
      p -> frost_fever = new frost_fever_t( p );
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();
    death_knight_t* p = player -> cast_death_knight();

    if ( result_is_hit() )
    {
      if ( p -> blood_plague )
        p -> blood_plague -> execute();

      if ( p -> frost_fever )
        p -> frost_fever -> execute();
    }
  }
};

// Pestilence ===============================================================

struct pestilence_t : public death_knight_spell_t
{
  pestilence_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "pestilence", 50842, player )
  {
    death_knight_t* p = player -> cast_death_knight();
    
    parse_options( NULL, options_str );

    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );

    if ( p -> passives.blood_of_the_north -> ok() || p -> passives.reaping -> ok() )
      convert_runes = 1.0;
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> buffs_dancing_rune_weapon -> check() )
      p -> active_dancing_rune_weapon -> drw_pestilence -> execute();
  }
};

// Pillar of Frost ==========================================================

struct pillar_of_frost_t : public death_knight_spell_t
{
  pillar_of_frost_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "pillar_of_frost", 51271, player )
  {
    death_knight_t* p = player -> cast_death_knight();
    check_talent( p -> talents.pillar_of_frost -> rank() );

    parse_options( NULL, options_str );

    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
    harmful = false;
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();
    death_knight_t* p = player -> cast_death_knight();
    p -> buffs_pillar_of_frost -> trigger( 1, p -> talents.pillar_of_frost -> effect_base_value( 1 ) / 100.0 );
  }
};

// Plague Strike ============================================================

struct plague_strike_offhand_t : public death_knight_attack_t
{
  plague_strike_offhand_t( death_knight_t* player ) :
    death_knight_attack_t( "plague_strike_offhand", 66216, player )
  {
    death_knight_t* p = player -> cast_death_knight();
    
    background = true;
    weapon     = &( p -> off_hand_weapon );
    // FIXME: Base Value is halfed in the DBC, should it be?

    base_multiplier *= 1.0 + p -> talents.rage_of_rivendare -> effect_base_value( 1 ) / 100.0;
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();    
    death_knight_attack_t::execute();

    if ( result_is_hit() )
    {
      if ( ! p -> blood_plague ) p -> blood_plague = new blood_plague_t( p );

      p -> blood_plague -> execute();
    }
  }
};

struct plague_strike_t : public death_knight_attack_t
{
  attack_t* oh_attack;

  plague_strike_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_attack_t( "plague_strike", 45462, player ), oh_attack( 0 )
  {
    death_knight_t* p = player -> cast_death_knight();
    
    parse_options( NULL, options_str );

    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );
    weapon = &( p -> main_hand_weapon );
    base_multiplier *= 1.0 + p -> talents.rage_of_rivendare -> effect_base_value( 1 ) / 100.0;
    if ( p -> off_hand_weapon.type != WEAPON_NONE )
      oh_attack = new plague_strike_offhand_t( p );
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();    
    death_knight_attack_t::execute();

    if ( p -> buffs_dancing_rune_weapon -> check() )
    {
      p -> active_dancing_rune_weapon -> drw_plague_strike -> execute();
    }
    if ( result_is_hit() )
    {
      if ( ! p -> blood_plague ) p -> blood_plague = new blood_plague_t( p );

      p -> blood_plague -> execute();
      trigger_ebon_plaguebringer( this );
    }

    if ( oh_attack )
    {
      if ( p -> rng_threat_of_thassarian -> roll ( p -> talents.threat_of_thassarian -> effect_base_value( 1 ) / 100.0 ) )
      {
        oh_attack -> execute();
      }
    }
  }
};

// Presence =================================================================

struct presence_t : public death_knight_spell_t
{
  int switch_to_presence;
  presence_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "presence", 0, player )
  {
    std::string presence_str;
    option_t options[] =
    {
      { "choose",  OPT_STRING, &presence_str },
      { NULL,     OPT_UNKNOWN, NULL          }
    };
    parse_options( options, options_str );

    if ( ! presence_str.empty() )
    {
      if ( presence_str == "blood" || presence_str == "bp" )
      {
        switch_to_presence = PRESENCE_BLOOD;
      }
      else if ( presence_str == "frost" || presence_str == "fp" )
      {
        switch_to_presence = PRESENCE_FROST;
      }
      else if ( presence_str == "unholy" || presence_str == "up" )
      {
        switch_to_presence = PRESENCE_UNHOLY;
      }
    }
    else
    {
      // Default to Frost Presence
      switch_to_presence = PRESENCE_FROST;
    }

    trigger_gcd = 0;
    cooldown -> duration = 1.0;
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
    case PRESENCE_BLOOD:
      p -> buffs_blood_presence  -> trigger();
      p -> armor_multiplier += p -> buffs_blood_presence -> effect_base_value( 1 ) / 100.0;
    break;
    case PRESENCE_FROST:
    {
      double fp_value = sim -> sim_data.effect_base_value( 48266, E_APPLY_AURA, A_MOD_DAMAGE_PERCENT_DONE );
      if ( p -> talents.improved_frost_presence -> rank() )
        fp_value += p -> talents.improved_frost_presence -> effect_base_value( 2 ) / 100.0 ;
      p -> buffs_frost_presence -> trigger( 1, fp_value );
    }
    break;
    case PRESENCE_UNHOLY:
      p -> buffs_unholy_presence -> trigger( 1, 0.10 + p -> talents.improved_unholy_presence -> effect_base_value( 2 ) / 100.0 );
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
  raise_dead_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "raise_dead", 46584, player )
  {
    death_knight_t* p = player -> cast_death_knight();

    parse_options( NULL, options_str );

    if ( p -> passives.master_of_ghouls -> ok() )
      cooldown -> duration += p -> passives.master_of_ghouls -> effect_base_value( 1 ) / 1000.0;
    harmful = false;
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    consume_resource();
    update_ready();
    player -> summon_pet( "ghoul", p -> passives.master_of_ghouls -> ok() ? 0.0 : 60.0 );
  }

  virtual double gcd() SC_CONST
  {
    return player -> in_combat ? death_knight_spell_t::gcd() : 0;
  }

  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> active_ghoul )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Rune Strike ==============================================================

struct rune_strike_t : public death_knight_attack_t
{
  rune_strike_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_attack_t( "rune_strike", 56815, player )
  {
    death_knight_t* p = player -> cast_death_knight();
    
    parse_options( NULL, options_str );

    base_crit       += p -> glyphs.rune_strike * 0.10;
    direct_power_mod = 0.15;
    may_dodge = may_block = may_parry = false;
  }

  virtual void consume_resource() { }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    weapon = &( p -> main_hand_weapon );
    death_knight_attack_t::execute();
    death_knight_attack_t::consume_resource();
    
    if ( result_is_hit() )
    {
      p -> trigger_runic_empowerment();  // FIX ME: Does DW get 2 chances to proc this?
    }

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( p -> rng_threat_of_thassarian -> roll ( p -> talents.threat_of_thassarian -> effect_base_value( 1 ) / 100.0 ) )
      {
        weapon = &( p -> off_hand_weapon );
        death_knight_attack_t::execute();
      }
    }
  }

  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();
    if ( ! p -> buffs_blood_presence -> check() )
      return false;

    return death_knight_attack_t::ready();
  }
};

// Scourge Strike ===========================================================

struct scourge_strike_t : public death_knight_attack_t
{
  spell_t* scourge_strike_shadow;

  struct scourge_strike_shadow_t : public death_knight_spell_t
  {
    scourge_strike_shadow_t( death_knight_t* player ) :
      death_knight_spell_t( "scourge_strike_shadow", player, RESOURCE_NONE, SCHOOL_SHADOW )
    {
      death_knight_t* p = player -> cast_death_knight();
      check_spec( TREE_UNHOLY );

      id = 70890;  // not in inc files?  hmm
      weapon = &( player -> main_hand_weapon );
      may_miss = may_parry = may_dodge = false;
      may_crit          = false;
      proc              = true;
      background        = true;
      trigger_gcd       = 0;
      weapon_multiplier = 0;
      base_multiplier  *= 1.0 + p -> glyphs.scourge_strike * 0.3;
    }

    virtual void target_debuff( player_t* t, int dmg_type )
    {
      // Shadow portion doesn't double dips in debuffs, other than EP/E&M/CoE below
      // death_knight_spell_t::target_debuff( t, dmg_type );

      death_knight_t* p = player -> cast_death_knight();

      target_multiplier = p -> diseases() * 0.18;

      if ( t -> debuffs.earth_and_moon -> up() || t -> debuffs.ebon_plaguebringer -> up() || t -> debuffs.curse_of_elements -> up() )
        target_multiplier *= 1.08;
    }

  };

  scourge_strike_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_attack_t( "scourge_strike", 55090, player ),
    scourge_strike_shadow( 0 )
  {
    death_knight_t* p = player -> cast_death_knight();

    parse_options( NULL, options_str );

    scourge_strike_shadow = new scourge_strike_shadow_t( player );
    extract_rune_cost( this, &cost_blood, &cost_frost, &cost_unholy );

    base_multiplier *= 1.0 + p -> set_bonus.tier10_2pc_melee() * 0.1
                       + p -> talents.rage_of_rivendare -> mod_additive( P_GENERIC );
  }

  void execute()
  {
    death_knight_attack_t::execute();
    if ( result_is_hit() )
    {
      // We divide out our composite_player_multiplier here because we
      // don't want to double dip; in particular, 3% damage from ret
      // paladins, arcane mages, and beastmaster hunters do not affect
      // scourge_strike_shadow.
      double modified_dd = direct_dmg / player -> player_t::composite_player_multiplier( SCHOOL_SHADOW );
      scourge_strike_shadow -> base_dd_max = scourge_strike_shadow -> base_dd_min = modified_dd;
      scourge_strike_shadow -> execute();
    }
  }
};

// Summon Gargoyle ==========================================================

struct summon_gargoyle_t : public death_knight_spell_t
{
  summon_gargoyle_t( death_knight_t* player, const std::string& options_str ) :
    death_knight_spell_t( "summon_gargoyle", 49206, player )
  {
    death_knight_t* p = player -> cast_death_knight();
    check_talent( p -> talents.summon_gargoyle -> rank() );
    rp_gain = 0.0;  // For some reason, the inc file thinks we gain RP for this spell
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();
    // Examining logs show gargoyls take 4.5-5.5 seconds before they
    // can begin casting, so rather than the tooltip's 30s duration,
    // let's use 25s.  This still probably overestimates and assumes
    // no meleeing, which gargoyles sometimes choose to do.
    player -> summon_pet( "gargoyle", 25.0 );
  }
};

// Unholy Frenzy ============================================================

struct unholy_frenzy_t : public spell_t
{
  player_t* unholy_frenzy_target;

  unholy_frenzy_t( player_t* player, const std::string& options_str ) :
    spell_t( "unholy_frenzy", 49016, player ), unholy_frenzy_target( 0 )
  {
    death_knight_t* p = player -> cast_death_knight();
    check_talent( p -> talents.unholy_frenzy -> rank() );

    std::string target_str = p -> unholy_frenzy_target_str;
    option_t options[] =
    {
      { "target", OPT_STRING, &target_str },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( target_str.empty() )
    {
      unholy_frenzy_target = p;
    }
    else
    {
      unholy_frenzy_target = sim -> find_player( target_str );
      assert ( unholy_frenzy_target != 0 );
    }

    harmful = false;
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(),name() );
    consume_resource();
    update_ready();
    unholy_frenzy_target -> buffs.unholy_frenzy -> trigger( 1 );
  }

};

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Death Knight Character Definition
// ==========================================================================

// death_knight_t::create_action  ===========================================

action_t* death_knight_t::create_action( const std::string& name, const std::string& options_str )
{
  // General Actions
  if ( name == "auto_attack"              ) return new auto_attack_t              ( this, options_str );
  if ( name == "presence"                 ) return new presence_t                 ( this, options_str );

  // Blood Actions
  if ( name == "blood_boil"               ) return new blood_boil_t               ( this, options_str );
  if ( name == "blood_strike"             ) return new blood_strike_t             ( this, options_str );
  if ( name == "blood_tap"                ) return new blood_tap_t                ( this, options_str );
  if ( name == "dancing_rune_weapon"      ) return new dancing_rune_weapon_t      ( this, options_str );
  if ( name == "heart_strike"             ) return new heart_strike_t             ( this, options_str );
  if ( name == "pestilence"               ) return new pestilence_t               ( this, options_str );

  // Frost Actions
  if ( name == "empower_rune_weapon"      ) return new empower_rune_weapon_t      ( this, options_str );
  if ( name == "frost_strike"             ) return new frost_strike_t             ( this, options_str );
  if ( name == "horn_of_winter"           ) return new horn_of_winter_t           ( this, options_str );
  if ( name == "howling_blast"            ) return new howling_blast_t            ( this, options_str );
  if ( name == "icy_touch"                ) return new icy_touch_t                ( this, options_str );
  if ( name == "mind_freeze"              ) return new mind_freeze_t              ( this, options_str );
  if ( name == "obliterate"               ) return new obliterate_t               ( this, options_str );
  if ( name == "pillar_of_frost"          ) return new pillar_of_frost_t          ( this, options_str );
  if ( name == "rune_strike"              ) return new rune_strike_t              ( this, options_str );

  // Unholy Actions
  if ( name == "army_of_the_dead"         ) return new army_of_the_dead_t         ( this, options_str );
  if ( name == "bone_shield"              ) return new bone_shield_t              ( this, options_str );
  if ( name == "dark_transformation"      ) return new dark_transformation_t      ( this, options_str );
  if ( name == "death_and_decay"          ) return new death_and_decay_t          ( this, options_str );
  if ( name == "death_coil"               ) return new death_coil_t               ( this, options_str );
  if ( name == "death_strike"             ) return new death_strike_t             ( this, options_str );
  if ( name == "festering_strike"         ) return new festering_strike_t         ( this, options_str );
  if ( name == "outbreak"                 ) return new outbreak_t                 ( this, options_str );
  if ( name == "plague_strike"            ) return new plague_strike_t            ( this, options_str );
  if ( name == "raise_dead"               ) return new raise_dead_t               ( this, options_str );
  if ( name == "scourge_strike"           ) return new scourge_strike_t           ( this, options_str );
  if ( name == "summon_gargoyle"          ) return new summon_gargoyle_t          ( this, options_str );
  if ( name == "unholy_frenzy"            ) return new unholy_frenzy_t            ( this, options_str );

  return player_t::create_action( name, options_str );
}

// death_knight_t::create_expression ========================================

action_expr_t* death_knight_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "blood" )
  {
    struct blood_expr_t : public action_expr_t
    {
      blood_expr_t( action_t* a ) : action_expr_t( a, "blood" ) { result_type = TOK_NUM; }
      virtual int evaluate()
      {
        result_num = GET_BLOOD_RUNE_COUNT( count_runes( action -> player -> cast_death_knight() ) ); return TOK_NUM;
      }
    };
    return new blood_expr_t( a );
  }

  if ( name_str == "frost" )
  {
    struct frost_expr_t : public action_expr_t
    {
      frost_expr_t( action_t* a ) : action_expr_t( a, "frost" ) { result_type = TOK_NUM; }
      virtual int evaluate()
      {
        result_num = GET_FROST_RUNE_COUNT( count_runes( action -> player -> cast_death_knight() ) ); return TOK_NUM;
      }
    };
    return new frost_expr_t( a );
  }

  if ( name_str == "unholy" )
  {
    struct unholy_expr_t : public action_expr_t
    {
      unholy_expr_t( action_t* a ) : action_expr_t( a, "unholy" ) { result_type = TOK_NUM; }
      virtual int evaluate()
      {
        result_num = GET_UNHOLY_RUNE_COUNT( count_runes( action -> player -> cast_death_knight() ) ); return TOK_NUM;
      }
    };
    return new unholy_expr_t( a );
  }

  if ( name_str == "death" || name_str == "inactive_death" )
  {
    struct death_expr_t : public action_expr_t
    {
      std::string name;
      death_expr_t( action_t* a, const std::string name_in ) : action_expr_t( a, name_in ), name( name_in ) { result_type = TOK_NUM; }
      virtual int evaluate()
      {
        result_num = count_death_runes( action -> player -> cast_death_knight(), name == "inactive_death" ); return TOK_NUM;
      }
    };
    return new death_expr_t( a, name_str );
  }

  return player_t::create_expression( a, name_str );
}

// death_knight_t::create_pets ==============================================

void death_knight_t::create_pets()
{
  create_pet( "army_of_the_dead_ghoul_8" );
  create_pet( "bloodworms" );
  create_pet( "dancing_rune_weapon" );
  create_pet( "gargoyle" );
  create_pet( "ghoul" );
}

// death_knight_t::create_pet ===============================================

pet_t* death_knight_t::create_pet( const std::string& pet_name,
				   const std::string& pet_type )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "army_of_the_dead_ghoul_8" ) return new army_ghoul_pet_t          ( sim, this );
  if ( pet_name == "bloodworms"               ) return new bloodworms_pet_t          ( sim, this );
  if ( pet_name == "dancing_rune_weapon"      ) return new dancing_rune_weapon_pet_t ( sim, this );
  if ( pet_name == "gargoyle"                 ) return new gargoyle_pet_t            ( sim, this );
  if ( pet_name == "ghoul"                    ) return new ghoul_pet_t               ( sim, this );

  return 0;
}

// death_knight_t::composite_attack_haste() =================================

double death_knight_t::composite_attack_haste() SC_CONST
{
  double haste = player_t::composite_attack_haste();
  haste *= 1.0 / ( 1.0 + buffs_unholy_presence -> value() );

  return haste;
}

// death_knight_t::composite_attack_hit() ===================================

double death_knight_t::composite_attack_hit() SC_CONST
{
  double hit = player_t::composite_attack_hit();

  // Factor in the hit from NoCS here so it shows up in the report, to match the character sheet
  if ( main_hand_weapon.group() == WEAPON_1H || off_hand_weapon.group() == WEAPON_1H )
  {
    if ( talents.nerves_of_cold_steel -> rank() )
    {
      hit += talents.nerves_of_cold_steel -> effect_base_value( 1 ) / 100.0;
    }
  }

  return hit;
}
// death_knight_t::init =====================================================

void death_knight_t::init()
{
  player_t::init();

  if ( pet_t* ghoul = find_pet( "ghoul" ) )
  {
    if ( passives.master_of_ghouls )
    {
      ghoul -> type = PLAYER_PET;
    }
    else
    {
      ghoul -> type = PLAYER_GUARDIAN;
    }
  }

  if ( ptr && passives.blood_of_the_north -> ok() )
  {
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    {
      if ( _runes.slot[i].type == RUNE_TYPE_BLOOD )
      {
	_runes.slot[i].make_permanent_death_rune();
      }
    }
  }
}

void death_knight_t::init_rng()
{
  player_t::init_rng();
  rng_blood_caked_blade          = get_rng( "blood_caked_blade"          );
  rng_might_of_the_frozen_wastes = get_rng( "might_of_the_frozen_wastes" );
  rng_threat_of_thassarian       = get_rng( "threat_of_thassarian"       );
}

// death_knight_t::init_base ================================================

void death_knight_t::init_base()
{
  player_t::init_base();

  double str_mult = ( talents.abominations_might -> effect_base_value( 2 ) / 100.0 +
                      talents.brittle_bones -> effect_base_value( 2 ) / 100.0 +
                      passives.unholy_might -> effect_base_value( 1 ) / 100.0 );

  attribute_multiplier_initial[ ATTR_STRENGTH ] *= 1.0 + str_mult;

  if ( passives.veteran_of_the_third_war -> ok() )
  {
    attribute_multiplier_initial[ ATTR_STAMINA ]  *= 1.0 + passives.veteran_of_the_third_war -> effect_base_value( 1 ) / 100.0;
    base_attack_expertise = passives.veteran_of_the_third_war -> effect_base_value( 2 ) / 100.0;
  }

  if ( talents.toughness -> rank() )
    initial_armor_multiplier *= 1.0 + talents.toughness -> effect_base_value( 1 ) / 100.0;

  // For some reason, my buffless, naked Death Knight Human with
  // 180str (3% bonus) has 583 AP.  No talent or buff can explain this
  // discrepency, but it is also present on other Death Knights I have
  // checked.  TODO: investigate this further.
  base_attack_power = 220;

  initial_attack_power_per_strength = 2.0;

  if ( primary_tree() == TREE_BLOOD )
    vengeance_enabled = true;

  resource_base[ RESOURCE_RUNIC ] = 100;
  if ( talents.runic_power_mastery -> rank() )
    resource_base[ RESOURCE_RUNIC ] += talents.runic_power_mastery -> effect_base_value( 1 ) / 10.0;

  base_gcd = 1.5;

  diminished_kfactor    = 0.009560;
  diminished_dodge_capi = 0.01523660;
  diminished_parry_capi = 0.01523660;
}

// death_knight_t::init_talents =============================================

void death_knight_t::init_talents()
{
  // Blood
  talents.abominations_might          = find_talent( "Abomination's Might" );
  talents.bladed_armor                = find_talent( "Bladed Armor" );
  talents.blood_caked_blade           = find_talent( "Blood-Caked Blade" );
  talents.blood_parasite              = find_talent( "Blood Parasite" );
  talents.bone_shield                 = find_talent( "Bone Shield" );
  talents.toughness                   = find_talent( "Toughness" );
  talents.butchery                    = find_talent( "Butchery" );
  talents.crimson_scourge             = find_talent( "Crimson Scourge" );
  talents.dancing_rune_weapon         = find_talent( "Dancing Rune Weapon" );
  talents.improved_blood_presence     = find_talent( "Improved Blood Presence" );
  talents.improved_blood_tap          = find_talent( "Improved Blood Tap" );
  talents.improved_death_strike       = find_talent( "Improved Death Strike" );
  talents.scent_of_blood              = find_talent( "Scent of Blood" );

  // Frost
  talents.annihilation                = find_talent( "Annihilation" );
  talents.brittle_bones               = find_talent( "Brittle Bones" );
  talents.chill_of_the_grave          = find_talent( "Chill of the Grave" );
  talents.endless_winter              = find_talent( "Endless Winter" );
  talents.howling_blast               = find_talent( "Howling Blast" );
  talents.improved_frost_presence     = find_talent( "Improved Frost Presence" );
  talents.improved_icy_talons         = find_talent( "Improved Icy Talons" );
  talents.killing_machine             = find_talent( "Killing Machine" );
  talents.merciless_combat            = find_talent( "Merciless Combat" );
  talents.might_of_the_frozen_wastes  = find_talent( "Might of the Frozen Wastes" );
  talents.nerves_of_cold_steel        = find_talent( "Nerves of Cold Steel" );
  talents.pillar_of_frost             = find_talent( "Pillar of Frost" );
  talents.rime                        = find_talent( "Rime" );
  talents.runic_power_mastery         = find_talent( "Runic Power Mastery" );
  talents.threat_of_thassarian        = find_talent( "Threat of Thassarian" );

  // Unholy
  talents.dark_transformation         = find_talent( "Dark Transformation" );
  talents.ebon_plaguebringer          = find_talent( "Ebon Plaguebringer" );
  talents.epidemic                    = find_talent( "Epidemic" );
  talents.improved_unholy_presence    = find_talent( "Improved Unholy Presence" );
  talents.morbidity                   = find_talent( "Morbidity" );
  talents.rage_of_rivendare           = find_talent( "Rage of Rivendare" );
  talents.runic_corruption            = find_talent( "Runic Corruption" );
  talents.shadow_infusion             = find_talent( "Shadow Infusion" );
  talents.sudden_doom                 = find_talent( "Sudden Doom" );
  talents.summon_gargoyle             = find_talent( "Summon Gargoyle" );
  talents.unholy_blight               = find_talent( "Unholy Blight" );
  talents.unholy_frenzy               = find_talent( "Unholy Frenzy" );
  talents.virulence                   = find_talent( "Virulence" );

  player_t::init_talents();
}

// death_knight_t::init_spells ==============================================

void death_knight_t::init_spells()
{
  player_t::init_spells();

  // Armor Bonus
  plate_specialization               = new passive_spell_t( this, "plate_specialization", 86524 );

  // Mastery
  mastery.dreadblade                 = new mastery_t( this, "dreadblade", 77515, TREE_UNHOLY );
  mastery.frozen_heart               = new mastery_t( this, "frozen_heart", 77514, TREE_FROST );

  // Passives
  passives.blood_of_the_north        = new passive_spell_t( this, "blood_of_the_north", "Blood of the North" );
  passives.blood_rites               = new passive_spell_t( this, "blood_rites", "Blood Rites" );
  passives.icy_talons                = new passive_spell_t( this, "icy_talons", "Icy Talons" );
  passives.master_of_ghouls          = new passive_spell_t( this, "master_of_ghouls", "Master of Ghouls" );
  passives.reaping                   = new passive_spell_t( this, "reaping", 56835 );
  passives.runic_empowerment         = new passive_spell_t( this, "runic_empowerment", 81229 );
  passives.unholy_might              = new passive_spell_t( this, "unholy_might", "Unholy Might" );
  passives.veteran_of_the_third_war  = new passive_spell_t( this, "veteran_of_the_third_war", "Veteran of the Third War" );

  // Tier Bonuses
  static uint32_t set_bonuses[N_TIER][N_TIER_BONUS] = 
  {
    //  C2P    C4P    M2P    M4P    T2P    T4P    H2P    H4P
    {     0,     0, 70655, 70656, 70650, 70652,     0,     0 }, // Tier10
    {     0,     0, 90457, 90459, 90454, 90456,     0,     0 }, // Tier11
    {     0,     0,     0,     0,     0,     0,     0,     0 },
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// death_knight_t::init_actions =============================================

void death_knight_t::init_actions()
{
  if ( main_hand_weapon.type == WEAPON_NONE )
  {
    sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  if ( action_list_str.empty() )
  {
    action_list_str  = "flask,type=titanic_strength";
    action_list_str += "/food,type=beer_basted_crocolisk";
    if ( ( primary_tree() == TREE_FROST && main_hand_weapon.group() == WEAPON_2H )
         || primary_tree() == TREE_UNHOLY )
    {
      action_list_str += "/presence,choose=unholy"; // 2H Frost/Unholy
    }
    else
    {
      action_list_str += "/presence,choose=frost"; // 1H Frost/Blood
    }
    action_list_str +="/army_of_the_dead";
    action_list_str += "/snapshot_stats";
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

    // Use slightly into the fight, when debuffs are up
    switch ( race )
    {
    case RACE_DWARF:     if ( talents.bladed_armor -> rank() > 0 ) action_list_str += "/stoneform,time>=10"; break;
    case RACE_ORC:       action_list_str += "/blood_fury,time>=10";     break;
    case RACE_TROLL:     action_list_str += "/berserking,time>=10";     break;
    case RACE_BLOOD_ELF: action_list_str += "/arcane_torrent,time>=10"; break;
    default: break;
    }

    switch ( primary_tree() )
    {
    case TREE_BLOOD:
      action_list_str += "/golemblood_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
      action_list_str += "/auto_attack";
      if ( talents.bone_shield -> rank() )
        action_list_str += "/bone_shield,if=!buff.bone_shield.up";
      action_list_str += "/raise_dead,time>=10";
      action_list_str += "/icy_touch,if=dot.frost_fever.remains<=2";
      action_list_str += "/plague_strike,if=dot.blood_plague.remains<=2";
      action_list_str += "/heart_strike";
      action_list_str += "/death_strike";
      if ( talents.dancing_rune_weapon -> rank() )
      {
        action_list_str += "/dancing_rune_weapon,time<=150,if=dot.frost_fever.remains<=5|dot.blood_plague.remains<=5";
        action_list_str += "/dancing_rune_weapon,if=(dot.frost_fever.remains<=5|dot.blood_plague.remains<=5)&buff.bloodlust.react";
      }
      action_list_str += "/empower_rune_weapon,if=blood=0&unholy=0&frost=0";
      action_list_str += "/death_coil";
      break;
    case TREE_FROST:
      action_list_str += "/golemblood_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
      action_list_str += "/auto_attack";
      if ( talents.pillar_of_frost -> rank() )
        action_list_str += "/pillar_of_frost";
      action_list_str += "/raise_dead,time>=5";
      // Priority Taken from Frost DK OP
      // Diseases
      if ( level > 81 )
        action_list_str += "/outbreak,if=dot.frost_fever.remains<=2|dot.blood_plague.remains<=2";
      action_list_str += "/howling_blast,if=dot.frost_fever.remains<=2";
      action_list_str += "/plague_strike,if=dot.blood_plague.remains<=2";
      //  Ob if both Frost/Unholy pairs and/or both Death runes are up, or if KM is procced
      action_list_str += "/obliterate,if=frost=2&unholy=2";
      action_list_str += "/obliterate,if=death=2";
      action_list_str += "/obliterate,if=buff.killing_machine.react"; // All 3 are seperated for Sample Sequence
      // BS if both blood are up
      action_list_str += "/blood_tap";
      action_list_str += "/blood_strike,if=blood=2";
      // FS if capped; using 30 less than cap was a DPS gain in all cases
      action_list_str +="/frost_strike,if=runic_power>=90";
      // Rime
      if ( talents.howling_blast -> rank() && talents.rime -> rank() )
        action_list_str += "/howling_blast,if=buff.rime.react"; 
      // OB -> BS -> FS
      action_list_str += "/obliterate";
      action_list_str += "/blood_strike";
      action_list_str += "/frost_strike";
      // Other
      action_list_str += "/empower_rune_weapon";
      action_list_str += "/horn_of_winter";
      break;
    case TREE_UNHOLY:
      action_list_str += "/raise_dead";
      action_list_str += "/golemblood_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
      action_list_str += "/auto_attack";
      if ( talents.unholy_frenzy -> rank() )
        action_list_str += "/unholy_frenzy,if=!buff.bloodlust.react|target.time_to_die<=45";
      if ( level > 81 )
        action_list_str += "/outbreak,if=dot.frost_fever.remains<=2|dot.blood_plague.remains<=2";
      action_list_str += "/icy_touch,if=dot.frost_fever.remains<3";
      action_list_str += "/plague_strike,if=dot.blood_plague.remains<3";
      if ( talents.dark_transformation -> rank() )
        action_list_str += "/dark_transformation";
      if ( talents.summon_gargoyle -> rank() )
      {
        action_list_str += "/summon_gargoyle,time<=60";
        action_list_str += "/summon_gargoyle,if=buff.bloodlust.react";
        action_list_str += "/summon_gargoyle,if=buff.unholy_frenzy.react";
      }
      action_list_str += "/death_and_decay,if=death=4";
      action_list_str += "/death_and_decay,if=unholy=2";
      action_list_str += "/scourge_strike,if=death=4";
      action_list_str += "/scourge_strike,if=unholy=2";
      action_list_str += "/festering_strike,if=blood=2&frost=2";
      action_list_str += "/death_coil,if=runic_power>90";
      if ( talents.sudden_doom -> rank() )
        action_list_str += "/death_coil,if=buff.sudden_doom.react";
      action_list_str += "/death_and_decay";
      action_list_str += "/scourge_strike";
      action_list_str += "/festering_strike";
      action_list_str += "/death_coil";
      action_list_str += "/blood_tap,if=unholy=0&inactive_death=1";
      action_list_str += "/empower_rune_weapon,if=unholy=0";
      action_list_str += "/horn_of_winter";
      break;
    default: break;
    }
    action_list_default = 1;
  }

  player_t::init_actions();
}

// death_knight_t::init_enchant =============================================

void death_knight_t::init_enchant()
{
  player_t::init_enchant();

  std::string& mh_enchant = items[ SLOT_MAIN_HAND ].encoded_enchant_str;
  std::string& oh_enchant = items[ SLOT_OFF_HAND  ].encoded_enchant_str;

  // Rune of Cinderglacier ==================================================
  struct cinderglacier_callback_t : public action_callback_t
  {
    int slot;
    buff_t* buff;

    cinderglacier_callback_t( player_t* p, int s, buff_t* b ) : action_callback_t( p -> sim, p ), slot( s ), buff( b ) {}

    virtual void trigger( action_t* a, void* call_data )
    {
      weapon_t* w = a -> weapon;
      if ( ! w || w -> slot != slot ) return;
      
      // FIX ME: What is the proc rate? For now assuming the same as FC
      buff -> trigger( 2, 0.2, w -> proc_chance_on_swing( 2.0 ) );

      // FIX ME: This should roll the benefit when casting DND, it does not
    }
  };

  // Rune of the Fallen Crusader ============================================
  struct fallen_crusader_callback_t : public action_callback_t
  {
    int slot;
    buff_t* buff;

    fallen_crusader_callback_t( player_t* p, int s, buff_t* b ) : action_callback_t( p -> sim, p ), slot( s ), buff( b ) {}

    virtual void trigger( action_t* a, void* call_data )
    {
      weapon_t* w = a -> weapon;
      if ( ! w ) return;
      if ( w -> slot != slot ) return;

      // RotFC is 2 PPM.
      buff -> trigger( 1, 0.15, w -> proc_chance_on_swing( 2.0 ) );
    }
  };

  // Rune of the Razorice ===================================================

  // Damage Proc
  struct razorice_spell_t : public death_knight_spell_t
  {
    razorice_spell_t( death_knight_t* player ) : death_knight_spell_t( "razorice", 50401, player )
    {
      may_miss    = false;
      may_crit    = false;
      may_resist  = true;
      background  = true;
      proc        = true;
      trigger_gcd = 0;
      reset();
    }
  };

  struct razorice_callback_t : public action_callback_t
  {
    int slot;
    buff_t* buff;
    spell_t* razorice_damage_proc;

    razorice_callback_t( death_knight_t* p, int s, buff_t* b ) : action_callback_t( p -> sim, p ), slot( s ), buff( b ), razorice_damage_proc( 0 )
    {
      razorice_damage_proc = new razorice_spell_t( p );
    }

    virtual void trigger( action_t* a, void* call_data )
    {
      weapon_t* w = a -> weapon;
      if ( ! w ) return;
      if ( w -> slot != slot ) return;

      // http://elitistjerks.com/f72/t64830-dw_builds_3_2_revenge_offhand/p28/#post1332820
      // double PPM        = 2.0;
      // double swing_time = a -> time_to_execute;
      // double chance     = w -> proc_chance_on_swing( PPM, swing_time );
      buff -> trigger( 1, 0.02, 1.0 );

      razorice_damage_proc -> base_dd_min = w -> min_dmg;
      razorice_damage_proc -> base_dd_max = w -> max_dmg;
      razorice_damage_proc -> execute();
    }
  };

  buffs_rune_of_cinderglacier       = new buff_t( this, "rune_of_cinderglacier",       2, 30.0 );
  buffs_rune_of_razorice            = new buff_t( this, "rune_of_razorice",            5, 20.0 );
  buffs_rune_of_the_fallen_crusader = new buff_t( this, "rune_of_the_fallen_crusader", 1, 15.0 );

  if ( mh_enchant == "rune_of_the_fallen_crusader" )
  {
    register_attack_callback( RESULT_HIT_MASK, new fallen_crusader_callback_t( this, SLOT_MAIN_HAND, buffs_rune_of_the_fallen_crusader ) );
  }
  else if ( mh_enchant == "rune_of_razorice" )
  {
    register_attack_callback( RESULT_HIT_MASK, new razorice_callback_t( this, SLOT_MAIN_HAND, buffs_rune_of_razorice ) );
  }
  else if ( mh_enchant == "rune_of_cinderglacier" )
  {
    register_attack_callback( RESULT_HIT_MASK, new cinderglacier_callback_t( this, SLOT_MAIN_HAND, buffs_rune_of_cinderglacier ) );
  }

  if ( oh_enchant == "rune_of_the_fallen_crusader" )
  {
    register_attack_callback( RESULT_HIT_MASK, new fallen_crusader_callback_t( this, SLOT_OFF_HAND, buffs_rune_of_the_fallen_crusader ) );
  }
  else if ( oh_enchant == "rune_of_razorice" )
  {
    register_attack_callback( RESULT_HIT_MASK, new razorice_callback_t( this, SLOT_OFF_HAND, buffs_rune_of_razorice ) );
  }
  else if ( oh_enchant == "rune_of_cinderglacier" )
  {
    register_attack_callback( RESULT_HIT_MASK, new cinderglacier_callback_t( this, SLOT_OFF_HAND, buffs_rune_of_cinderglacier ) );
  }
}

// death_knight_t::init_glyphs ==============================================

void death_knight_t::init_glyphs()
{
  memset( ( void* ) &glyphs, 0x0, sizeof( glyphs_t ) );

  std::vector<std::string> glyph_names;
  int num_glyphs = util_t::string_split( glyph_names, glyphs_str, ",/" );

  for ( int i=0; i < num_glyphs; i++ )
  {
    std::string& n = glyph_names[ i ];

    if      ( n == "death_and_decay" ) glyphs.death_and_decay = 1;
    else if ( n == "death_coil"      ) glyphs.death_coil = 1;
    else if ( n == "death_strike"    ) glyphs.death_strike = 1;
    else if ( n == "frost_strike"    ) glyphs.frost_strike = 1;
    else if ( n == "heart_strike"    ) glyphs.heart_strike = 1;
    else if ( n == "horn_of_winter"  ) glyphs.horn_of_winter = 1;
    else if ( n == "howling_blast"   ) glyphs.howling_blast = 1;
    else if ( n == "icy_touch"       ) glyphs.icy_touch = 1;
    else if ( n == "obliterate"      ) glyphs.obliterate = 1;
    else if ( n == "raise_dead"      ) glyphs.raise_dead = 1;
    else if ( n == "rune_strike"     ) glyphs.rune_strike = 1;
    else if ( n == "scourge_strike"  ) glyphs.scourge_strike = 1;
    // To prevent warnings
    else if ( n == "antimagic_shell"     ) ;
    else if ( n == "blood_boil"          ) ;
    else if ( n == "blood_tap"           ) ;
    else if ( n == "bone_shield"         ) ;
    else if ( n == "chains_of_ice"       ) ;
    else if ( n == "dancing_rune_weapon" ) ;
    else if ( n == "death_grip"          ) ;
    else if ( n == "deaths_embrace"      ) ;
    else if ( n == "hungering_cold"      ) ;
    else if ( n == "path_of_frost"       ) ;
    else if ( n == "pestilence"          ) ;
    else if ( n == "pilar_of_frost"      ) ;
    else if ( n == "raise_ally"          ) ;
    else if ( n == "resilient_grip"      ) ;
    else if ( n == "rune_tap"            ) ;
    else if ( n == "strangulate"         );
    else if ( n == "vampiric_blood"      );
    else if ( ! sim -> parent )
    {
      sim -> errorf( "Player %s has unrecognized glyph %s\n", name(), n.c_str() );
    }
  }
}

// death_knight_t::init_scaling =============================================

void death_knight_t::init_scaling()
{
  player_t::init_scaling();

  if ( talents.bladed_armor -> rank() > 0 )
    scales_with[ STAT_ARMOR ] = 1;

  if ( off_hand_weapon.type != WEAPON_NONE )
  {
    scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = 1;
  }
}

// death_knight_t::init_buffs ===============================================

void death_knight_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( sim, name, max_stack, duration, cooldown, proc_chance, quiet )
  buffs_blood_presence      = new buff_t( this, 48263, "blood_presence" );
  buffs_bone_shield         = new buff_t( this, "bone_shield",                                        4, 300.0 );
  buffs_crimson_scourge     = new buff_t( this, 81141, "crimson_scourge", talents.crimson_scourge -> proc_chance() );
  buffs_dancing_rune_weapon = new buff_t( this, "dancing_rune_weapon",                                1,  12.0,  0.0, 1.0, true );
  buffs_dark_transformation = new buff_t( this, "dark_transformation",                                1,  30.0 );
  buffs_ebon_plaguebringer  = new buff_t( this, "ebon_plaguebringer_track",	
                                          1, 21.0 + util_t::talent_rank(talents.epidemic -> effect_base_value( 1 ), 3, 3, 9, 12 ), 0.0, 1.0, true );
  buffs_frost_presence      = new buff_t( this, "frost_presence" );
  buffs_killing_machine     = new buff_t( this, "killing_machine",                                    1,  30.0,  0.0, 0.0 ); // PPM based!
  buffs_pillar_of_frost     = new buff_t( this, "pillar_of_frost",                                    1,  20.0 );
  buffs_rime                = new buff_t( this, "rime",                                               1,  30.0,  0.0, talents.rime -> proc_chance() );
  buffs_runic_corruption    = new buff_t( this, "runic_corruption",                                   1,   3.0,  0.0, talents.runic_corruption -> effect_base_value( 1 ) / 100.0 );
  buffs_scent_of_blood      = new buff_t( this, "scent_of_blood",      talents.scent_of_blood -> rank(),  20.0,  0.0, talents.scent_of_blood -> proc_chance() );
  buffs_shadow_infusion     = new buff_t( this, "shadow_infusion",                                    5,  30.0,  0.0, talents.shadow_infusion -> proc_chance() );
  buffs_sudden_doom         = new buff_t( this, "sudden_doom",                                        1,  10.0,  0.0, 1.0 );
  buffs_tier10_4pc_melee    = new buff_t( this, "tier10_4pc_melee",                                   1,  15.0,  0.0, set_bonus.tier10_4pc_melee() );
  buffs_tier11_4pc_melee    = new buff_t( this, "tier11_4pc_melee",                                   3,  30.0,  0.0, set_bonus.tier11_4pc_melee() );
  buffs_unholy_presence     = new buff_t( this, "unholy_presence" );

  struct bloodworms_buff_t : public buff_t
  {
    bloodworms_buff_t( death_knight_t* p ) :
      buff_t( p, "bloodworms", 1, 19.99, 0.0, p -> talents.blood_parasite -> rank() * 0.05 ) {}
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

// death_knight_t::init_gains ===============================================

void death_knight_t::init_gains()
{
  player_t::init_gains();

  gains_butchery                   = get_gain( "butchery"                   );
  gains_chill_of_the_grave         = get_gain( "chill_of_the_grave"         );
  gains_horn_of_winter             = get_gain( "horn_of_winter"             );
  gains_might_of_the_frozen_wastes = get_gain( "might_of_the_frozen_wastes" );
  gains_power_refund               = get_gain( "power_refund"               );
  gains_rune_abilities             = get_gain( "rune_abilities"             );
  gains_scent_of_blood             = get_gain( "scent_of_blood"             );
}

// death_knight_t::init_procs ===============================================

void death_knight_t::init_procs()
{
  player_t::init_procs();

  proc_runic_empowerment        = get_proc( "runic_empowerment"        );
  proc_runic_empowerment_wasted = get_proc( "runic_empowerment_wasted" );
}

// death_knight_t::init_resources ===========================================

void death_knight_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resource_current[ RESOURCE_RUNIC ] = 0;
}

// death_knight_t::init_values ==============================================

void death_knight_t::init_values()
{
  player_t::init_values();

  constants.runic_empowerment_proc_chance = passives.runic_empowerment -> proc_chance();
}

// death_knight_t::reset ====================================================

void death_knight_t::reset()
{
  player_t::reset();

  // Active
  active_presence = 0;

  // Pets and Guardians
  active_army_ghoul          = NULL;
  active_bloodworms          = NULL;
  active_dancing_rune_weapon = NULL;
  active_ghoul               = NULL;

  _runes.reset();
}

// death_knight_t::combat_begin =============================================

void death_knight_t::combat_begin()
{
  player_t::combat_begin();

  double am_value = talents.abominations_might -> effect_base_value( 1 ) / 100.0;
  if ( am_value > 0 && am_value >= sim -> auras.abominations_might -> current_value )
    sim -> auras.abominations_might -> trigger( 1, am_value );
}

// death_knight_t::asses_damage =============================================

double death_knight_t::assess_damage( double            amount,
                                      const school_type school,
                                      int               dmg_type,
                                      int               result,
                                      action_t*         action )
{
  if ( buffs_blood_presence -> check() )
  {
    amount *= 1.0 - player_data.effect_base_value( player_data.spell_effect_id( 61261, 1 ) ) / 100.0;
  }

  if ( result != RESULT_MISS )
  {
    buffs_scent_of_blood -> trigger();
  }

  return player_t::assess_damage( amount, school, dmg_type, result, action );
}


// death_knight_t::composite_pet_attack_crit ===============================

double death_knight_t::composite_pet_attack_crit()
{
  // XXX: this is what I observe.  My ghoul absolutely doesn't get my
  // crit rating.  He crits on average ~10% of the time across
  // multiple nights of raiding and thousands of hits.  I would expect
  // 35-40% but get solidly 9-10% after lvl 83 crit suppression.
  // Needs more testing to confirm.
  return 0.14;
}

// death_knight_t::composite_attack_power ===================================

double death_knight_t::composite_attack_power() SC_CONST
{
  double ap = player_t::composite_attack_power();
  if ( talents.bladed_armor -> rank() )
  {
    ap += composite_armor() / talents.bladed_armor -> effect_base_value( 1 );
  }
  if ( buffs_tier11_4pc_melee -> check() )
    ap *= 1.0 + buffs_tier11_4pc_melee -> stack() / 100.0;

  return ap;
}

// death_knight_t::composite_attribute_multiplier ===========================

double death_knight_t::composite_attribute_multiplier( int attr ) SC_CONST
{
  double m = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_STRENGTH )
  {
    m *= 1.0 + buffs_rune_of_the_fallen_crusader -> value();
    m *= 1.0 + buffs_pillar_of_frost -> value();
  }

  if ( attr == ATTR_STAMINA )
    if ( buffs_blood_presence -> check() )
      m *= 1.0 + buffs_blood_presence -> effect_base_value( 3 ) / 100.0;

  return m;
}

// death_knight_t::matching_gear_multiplier ==================================

double death_knight_t::matching_gear_multiplier( const attribute_type attr ) SC_CONST
{
  switch ( primary_tree() )
  {
  case TREE_UNHOLY:
  case TREE_FROST:
    if ( attr == ATTR_STRENGTH )
      return plate_specialization -> base_value() / 100.0;
    break;
  case TREE_BLOOD:
    if ( attr == ATTR_STAMINA )
      return plate_specialization -> base_value() / 100.0;
    break;
  default:
    break;
  }

  return 0.0;
}

// death_knight_t::composite_spell_hit ======================================

double death_knight_t::composite_spell_hit() SC_CONST
{
  double hit = player_t::composite_spell_hit();
  hit += .09; // Not in Runic Empowerment's data yet
  return hit;
}

// death_knight_t::composite_tank_parry =====================================

double death_knight_t::composite_tank_parry() SC_CONST
{
  double parry = player_t::composite_tank_parry();

  if ( buffs_dancing_rune_weapon -> up() )
    parry += 0.20;

  return parry;
}

// death_knight_t::composite_player_multiplier ==============================

double death_knight_t::composite_player_multiplier( const school_type school ) SC_CONST
{
  double m = player_t::composite_player_multiplier( school );
  // Factor flat multipliers here so they effect procs, grenades, etc.
  m *= 1.0 + buffs_frost_presence -> value();
  m *= 1.0 + buffs_bone_shield -> value();

  if ( school == SCHOOL_SHADOW )
    m *= 1.0 + mastery.dreadblade -> base_value( E_APPLY_AURA, A_DUMMY ) * composite_mastery();

  return m;
}

// death_knight_t::composite_tank_crit ==========================================

double death_knight_t::composite_tank_crit( const school_type school ) SC_CONST
{
  double c = player_t::composite_tank_crit( school );

  if ( school == SCHOOL_PHYSICAL && talents.improved_blood_presence -> rank() )
    c += talents.improved_blood_presence -> effect_base_value( 2 ) / 100.0;

  return c;
}

// death_knight_t::primary_role ====================================================

int death_knight_t::primary_role() SC_CONST
{
  if ( player_t::primary_role() == ROLE_TANK )
    return ROLE_TANK;

  if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_ATTACK )
    return ROLE_ATTACK;

  if ( primary_tree() == TREE_BLOOD )
    return ROLE_TANK;

  return ROLE_ATTACK;
}

// death_knight_t::regen ====================================================

void death_knight_t::regen( double periodicity )
{
  player_t::regen( periodicity );

  if ( talents.butchery -> rank() )
    resource_gain( RESOURCE_RUNIC, ( talents.butchery -> effect_base_value( 2 ) / 10.0 / 5.0 * periodicity ), gains_butchery );

  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
  {
    _runes.slot[i].regen_rune( this, periodicity );
  }
}

// death_knight_t::create_options ===========================================

void death_knight_t::create_options()
{
  player_t::create_options();

  option_t death_knight_options[] =
  {
    { "unholy_frenzy_target", OPT_STRING, &( unholy_frenzy_target_str ) },
    { NULL, OPT_UNKNOWN, NULL }
  };

  option_t::copy( options, death_knight_options );
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

  if ( strstr( s, "scourgelord" ) )
  {
    if ( is_melee ) return SET_T10_MELEE;
    if ( is_tank  ) return SET_T10_TANK;
  }
  if ( strstr( s, "magma_plated" ) )
  {
    bool is_melee = ( strstr( s, "helmet"        ) ||
                      strstr( s, "pauldrons"     ) ||
                      strstr( s, "battleplate"   ) ||
                      strstr( s, "legplates"     ) ||
                      strstr( s, "gauntlets"     ) );

    bool is_tank = ( strstr( s, "faceguard"      ) ||
                     strstr( s, "shoulderguards" ) ||
                     strstr( s, "chestguard"     ) ||
                     strstr( s, "legguards"      ) ||
                     strstr( s, "handguards"     ) );

    if ( is_melee ) return SET_T11_MELEE;
    if ( is_tank  ) return SET_T11_TANK;
  }

  return SET_NONE;
}

// death_knight_t::trigger_runic_empowerment ================================

void death_knight_t::trigger_runic_empowerment()
{
  if ( ! sim -> roll( constants.runic_empowerment_proc_chance ) )
    return;

  if ( talents.runic_corruption -> rank() )
  {
    if ( buffs_runic_corruption -> up() )
    {
      buffs_runic_corruption -> extend_duration( this, 3 );
    }
    else
    {
      buffs_runic_corruption -> trigger();
    }
    return;
  }

  int depleted_runes[RUNE_SLOT_MAX];
  int num_depleted=0;

  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    if ( _runes.slot[i].is_depleted() )
      depleted_runes[ num_depleted++ ] = i;

  if ( num_depleted > 0 )
  {
    int rune_to_regen = depleted_runes[ (int) ( sim -> rng -> real() * num_depleted * 0.9999 ) ];
    _runes.slot[rune_to_regen].fill_rune();
    if ( sim -> log ) log_t::output( sim, "runic empowerment regen'd rune %d", rune_to_regen );
    proc_runic_empowerment -> occur();
  }
  else
  {
    // If there were no available runes to refresh
    proc_runic_empowerment_wasted -> occur();
  }
}

// player_t implementations =================================================

player_t* player_t::create_death_knight( sim_t* sim, const std::string& name, race_type r )
{
  return new death_knight_t( sim, name, r );
}

// player_t::death_knight_init ==============================================

void player_t::death_knight_init( sim_t* sim )
{
  sim -> auras.abominations_might  = new aura_t( sim, "abominations_might",  1,   0.0 );
  sim -> auras.horn_of_winter      = new aura_t( sim, "horn_of_winter",      1, 120.0 );
  sim -> auras.improved_icy_talons = new aura_t( sim, "improved_icy_talons", 1,   0.0 );

  for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    p -> buffs.unholy_frenzy        = new   buff_t( p, "unholy_frenzy",      1, 30.0 );
    p -> debuffs.brittle_bones      = new debuff_t( p, "brittle_bones",      1 );
    p -> debuffs.ebon_plaguebringer = new debuff_t( p, "ebon_plague",        1, 15.0 );
    p -> debuffs.scarlet_fever      = new debuff_t( p, "scarlet_fever",      1, 21.0 );
  }
}

// player_t::death_knight_combat_begin ======================================

void player_t::death_knight_combat_begin( sim_t* sim )
{
  if ( sim -> overrides.abominations_might  ) sim -> auras.abominations_might  -> override( 1, 0.10 );
  if ( sim -> overrides.horn_of_winter      )
    sim -> auras.horn_of_winter      -> override( 1, sim -> sim_data.effect_min( 57330, sim -> max_player_level, E_APPLY_AURA, A_MOD_STAT ) );

  if ( sim -> overrides.improved_icy_talons ) sim -> auras.improved_icy_talons -> override( 1, 0.10 );

  for ( target_t* t = sim -> target_list; t; t = t -> next )
  {
    if ( sim -> overrides.brittle_bones      ) t -> debuffs.brittle_bones      -> override( 1, 0.04 );
    if ( sim -> overrides.ebon_plaguebringer ) t -> debuffs.ebon_plaguebringer -> override( 1,  8.0 );
    if ( sim -> overrides.scarlet_fever      ) t -> debuffs.scarlet_fever      -> override();
  }
}
