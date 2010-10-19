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

const char *rune_symbols = "!bfu!!";

#define RUNE_TYPE_MASK     3
#define RUNE_SLOT_MAX      6

#define RUNIC_POWER_REFUND  0.9

#define GET_BLOOD_RUNE_COUNT(x)  (((x&0x01) + ((x&0x02)>>1))    )
#define GET_FROST_RUNE_COUNT(x)  (((x&0x08) + ((x&0x10)>>1)) >>3)
#define GET_UNHOLY_RUNE_COUNT(x) (((x&0x40) + ((x&0x80)>>1)) >>6)

enum rune_state { STATE_DEPLETED, STATE_REGENERATING, STATE_FULL };

struct dk_rune_t
{
  int        type;
  rune_state state;
  double     cooldown_time;
  dk_rune_t* paired_rune;

  dk_rune_t() : type( RUNE_TYPE_NONE ), state( STATE_FULL ), cooldown_time( 0.0 ), paired_rune( NULL ) {}

  bool is_death() SC_CONST { return ( type & RUNE_TYPE_DEATH ) != 0; }
  bool is_ready( double t ) SC_CONST { return t >= cooldown_time; }
  int  get_type() SC_CONST { return type & RUNE_TYPE_MASK; }

  rune_state get_rune_state( sim_t* sim )
  {
    if ( sim -> current_time >= cooldown_time )
      return STATE_FULL;
    else
      return STATE_DEPLETED;
  }

  void consume( sim_t* sim, player_t* p, bool convert )
  {
    assert ( get_rune_state( sim ) == STATE_FULL );

    
    type = ( type & RUNE_TYPE_MASK ) | ( ( type << 1 ) & RUNE_TYPE_WASDEATH ) | ( convert ? RUNE_TYPE_DEATH : 0 );
    double cooldown = 10.0 / ( 1 + p -> composite_attack_haste() );

    /* FIX ME: This doesn't work
    death_knight_t* o = p -> cast_death_knight();
    if ( o -> buffs_blood_presence -> check() && o -> talents.improved_blood_presence -> rank() )
    {
      cooldown /= 1.0 + ( o -> talents.improved_blood_presence -> effect_base_value( 3 ) / 100.0 );  
    }
    if ( o -> buffs_unholy_presence -> check() )
    {
      cooldown /= 1.0 + 0.10;
    }
    // FIXME: Does this increase the refresh rate of all runes or just newly used runes under the buff
    if ( o -> buffs_runic_corruption -> check() )
    {
      cooldown /= 1.0 + ( o -> talents.runic_corruption -> effect_base_value( 1 ) / 100.0 );
    }
    */

    if ( paired_rune->get_rune_state( sim ) == STATE_FULL )
    {
      cooldown_time = sim->current_time + cooldown;
    }
    else
    {
      cooldown_time = paired_rune->cooldown_time + cooldown;
    }
  }

  void fill_rune( player_t* p )
  {
    cooldown_time = 0.0;
    double cooldown = 10.0 / ( 1 + p -> composite_attack_haste() );
    /* FIX ME: This doesn't work
    death_knight_t* o = p -> cast_death_knight();
    if ( o -> buffs_blood_presence -> check() && o -> talents.improved_blood_presence -> rank() )
    {
      cooldown /= 1.0 + ( o -> talents.improved_blood_presence -> effect_base_value( 3 ) / 100.0 );  
    }
    if ( o -> buffs_unholy_presence -> check() )
    {
      cooldown /= 1.0 + 0.10;
    }
    // FIXME: Does this increase the refresh rate of all runes or just newly used runes under the buff
    if ( o -> buffs_runic_corruption -> check() )
    {
      cooldown /= 1.0 + ( o -> talents.runic_corruption -> effect_base_value( 1 ) / 100.0 );
    }
    */
    paired_rune -> cooldown_time = std::min( p -> sim->current_time + cooldown, paired_rune-> cooldown_time );
  }

  void reset()
  {
    cooldown_time = 0.0;
    type = type & RUNE_TYPE_MASK;
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

  // Pets and Guardians
  bloodworms_pet_t*          active_bloodworms;
  dancing_rune_weapon_pet_t* active_dancing_rune_weapon;
  ghoul_pet_t*               active_ghoul;

  // Buffs
  buff_t* buffs_blood_presence;
  buff_t* buffs_blood_swarm;
  buff_t* buffs_bloodworms;
  buff_t* buffs_bone_shield;    
  buff_t* buffs_dancing_rune_weapon;
  buff_t* buffs_dark_transformation;
  buff_t* buffs_frost_presence;
  buff_t* buffs_killing_machine;
  buff_t* buffs_pillar_of_frost;
  buff_t* buffs_rime;
  buff_t* buffs_rune_of_razorice;
  buff_t* buffs_rune_of_the_fallen_crusader;
  buff_t* buffs_runic_corruption;
  buff_t* buffs_scent_of_blood;
  buff_t* buffs_shadow_infusion;
  buff_t* buffs_tier10_4pc_melee;
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

    // Major
    int hungering_cold;

    // Minor
    int horn_of_winter;

    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  // Mastery
  struct mastery_t
  {
    passive_spell_t* blightcaller;
    passive_spell_t* frozen_heart;

    mastery_t() { memset( ( void* ) this, 0x0, sizeof( mastery_t ) ); }
  };
  mastery_t mastery;

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

  // Procs
  proc_t* procs_glyph_of_disease;
  proc_t* procs_sudden_doom;

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
      }

    }
    void reset() { for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) slot[i].reset();                           }
  };
  runes_t _runes;

  // Special
  spell_t* sudden_doom;

  // Spells
  struct spells_t
  {
    active_spell_t* blood_boil;
    active_spell_t* blood_plague;
    active_spell_t* blood_strike;
    active_spell_t* blood_tap;
    active_spell_t* death_coil;
    active_spell_t* death_strike;
    active_spell_t* empower_rune_weapon;
    active_spell_t* festering_strike;
    active_spell_t* frost_fever;
    active_spell_t* frost_strike;
    active_spell_t* heart_strike;
    active_spell_t* horn_of_winter;
    active_spell_t* icy_touch;
    active_spell_t* mind_freeze;
    active_spell_t* necrotic_strike;
    active_spell_t* obliterate;
    active_spell_t* outbreak;
    active_spell_t* plague_strike;
    active_spell_t* pestilence;
    active_spell_t* raise_dead;
    active_spell_t* scourge_strike; 
    active_spell_t* unholy_frenzy;

    spells_t() { memset( ( void* ) this, 0x0, sizeof( spells_t ) ); }
  };
  spells_t spells;

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
    talent_t* hungering_cold;
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
    talent_t* contagion;
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

  // Up-Times
  uptime_t* uptimes_blood_plague;
  uptime_t* uptimes_frost_fever;

  bool plate_specialization;

  death_knight_t( sim_t* sim, const std::string& name, race_type r = RACE_NONE ) :
    player_t( sim, DEATH_KNIGHT, name, r )
  {
    tree_type[ DEATH_KNIGHT_BLOOD  ] = TREE_BLOOD;
    tree_type[ DEATH_KNIGHT_FROST  ] = TREE_FROST;
    tree_type[ DEATH_KNIGHT_UNHOLY ] = TREE_UNHOLY;

    // Active
    active_presence            = 0;
    active_blood_caked_blade   = NULL;
    active_unholy_blight       = NULL;

    // DoTs
    dots_blood_plague  = get_dot( "blood_plague" );
    dots_frost_fever   = get_dot( "frost_fever" );

    // Pets and Guardians
    active_bloodworms          = NULL;
    active_dancing_rune_weapon = NULL;
    active_ghoul               = NULL;

    sudden_doom         = NULL;
    blood_plague        = NULL;
    frost_fever         = NULL;

    cooldowns_howling_blast = get_cooldown( "howling_blast" );
  }

  // Character Definition
  virtual void      init();
  virtual void      init_talents();
  virtual void      init_spells();
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
  virtual void      init_values();
  virtual double    composite_attack_haste() SC_CONST;
  virtual double    composite_attack_power() SC_CONST;
  virtual double    composite_attribute_multiplier( int attr ) SC_CONST;
  virtual double    matching_gear_multiplier( const stat_type attr ) SC_CONST;
  virtual double    composite_spell_hit() SC_CONST;
  virtual double    composite_tank_parry() SC_CONST;
  virtual double    composite_player_multiplier( const school_type school ) SC_CONST;
  virtual void      interrupt();
  virtual void      regen( double periodicity );
  virtual void      reset();
  virtual int       target_swing();
  virtual void      combat_begin();
  virtual std::vector<talent_translation_t>& get_talent_list();
  virtual std::vector<option_t>& get_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual action_expr_t* create_expression( action_t*, const std::string& name );
  virtual pet_t*    create_pet( const std::string& name );
  virtual void      create_pets();
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST { return RESOURCE_RUNIC; }
  virtual void      trigger_runic_empowerment();

  // Death Knight Specific helperfunctions
  int  diseases()
  {
    int disease_count = 0;

    if ( dots_blood_plague -> ticking() )
      disease_count++;

    if ( dots_frost_fever -> ticking() )
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

    if ( dots_drw_blood_plague -> ticking() )
      drw_disease_count++;

    if ( dots_drw_frost_fever -> ticking() )
      drw_disease_count++;

    return drw_disease_count;
  }

  struct drw_blood_boil_t : public spell_t
  {
    drw_blood_boil_t( player_t* player ) :
      spell_t( "blood_boil", player, RESOURCE_NONE, SCHOOL_SHADOW, TREE_BLOOD )
    {
      pet_t* p = player -> cast_pet();
      death_knight_t* o = p -> owner -> cast_death_knight();
      background  = true;
      trigger_gcd = 0;
      aoe = true;
      may_crit = true;

      base_execute_time = 0;
      base_spell_power_multiplier  = 0;
      base_attack_power_multiplier = 1;

      direct_power_mod  = 0.06;
      base_multiplier *= 0.5;
      base_dd_min = 180;
      base_dd_max = 220;

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

      background        = true;
      base_td           = 36;
      trigger_gcd       = 0;
      base_tick_time    = 3.0;
      num_ticks         = 5 + o -> talents.epidemic -> effect_base_value( 1 ) / 1000 / 3;
      base_attack_power_multiplier = 1;
      tick_power_mod    = 0.055 * 1.15;
      base_spell_power_multiplier  = 0;
      base_multiplier  *= 0.50;
      may_miss          = false;

      if ( o -> race == RACE_ORC )
      {
        base_multiplier *= 1.05;
      }

      reset();
    }
    virtual bool ready() { return false; }

  };

  struct drw_blood_strike_t : public attack_t
  {
    bool proc_sd;
    bool roll_sd;
    drw_blood_strike_t( player_t* player ) :
      attack_t( "blood_strike", player, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_BLOOD, true )
    {
      background  = true;
      trigger_gcd = 0;
      may_crit = true;

      weapon = &( player -> main_hand_weapon );
      normalize_weapon_speed = true;
      weapon_multiplier     *= 0.4;

      pet_t* p = player -> cast_pet();
      death_knight_t* o = p -> owner -> cast_death_knight();
      base_multiplier *= 0.50; // DRW malus
      base_dd_min = base_dd_max = 764;

      if ( o -> race == RACE_ORC )
      {
        base_multiplier *= 1.05;
      }

      reset();
    }

    virtual void target_debuff( int dmg_type )
    {
      dancing_rune_weapon_pet_t* p = ( dancing_rune_weapon_pet_t* ) player;
      attack_t::target_debuff( dmg_type );
      target_multiplier *= 1 + p -> drw_diseases() * 0.125;
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
      background  = true;
      trigger_gcd = 0;
      may_crit = true;

      base_execute_time = 0;
      base_spell_power_multiplier  = 0;
      base_attack_power_multiplier = 1;

      base_dd_multiplier *= 1 + ( o -> talents.morbidity -> effect_base_value( 1 ) / 100.0 +
                                  0.15 * o -> glyphs.death_coil );
      direct_power_mod  = 0.15;
      base_multiplier *= 0.50;
      base_dd_min = base_dd_max = 443;

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
      background  = true;
      trigger_gcd = 0;
      may_crit = true;

      weapon = &( player -> main_hand_weapon );
      normalize_weapon_speed = true;
      weapon_multiplier     *= 0.75;

      pet_t* p = player -> cast_pet();
      death_knight_t* o = p -> owner -> cast_death_knight();
      base_crit += o -> talents.improved_death_strike -> effect_base_value( 2 ) / 100.0;
      base_multiplier *= 1 + o -> talents.improved_death_strike -> effect_base_value( 1 ) / 100.0;
      base_multiplier *= 0.50; // DRW malus
      base_dd_min = base_dd_max = 297;

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

      background        = true;
      trigger_gcd       = 0;
      base_td           = 29;
      base_tick_time    = 3.0;
      num_ticks         = 5 + o -> talents.epidemic -> effect_base_value( 1 ) / 1000 / 3;
      base_multiplier  *= 1.0 + o -> glyphs.icy_touch * 0.2;
      base_attack_power_multiplier = 1;
      tick_power_mod    = 0.055 * 1.15;
      base_spell_power_multiplier  = 0;
      base_multiplier  *= 0.50;

      may_miss          = false;

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
      attack_t( "heart_strike", player, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_BLOOD, true )
    {
      background  = true;
      trigger_gcd = 0;
      may_crit = true;

      weapon = &( player -> main_hand_weapon );
      normalize_weapon_speed = true;
      weapon_multiplier     *= 0.5;

      pet_t* p = player -> cast_pet();
      death_knight_t* o = p -> owner -> cast_death_knight();
      base_multiplier *= 1 + o -> set_bonus.tier10_2pc_melee() * 0.07;
      base_multiplier *= 0.50; // DRW malus
      base_dd_min = base_dd_max = 736;
      if ( o -> race == RACE_ORC )
      {
        base_multiplier *= 1.05;
      }
 
      reset();
    }

    void target_debuff( int dmg_type )
    {
      dancing_rune_weapon_pet_t* p = ( dancing_rune_weapon_pet_t* ) player;
      attack_t::target_debuff( dmg_type );
      target_multiplier *= 1 + p -> drw_diseases() * 0.1;
    }

    virtual bool ready() { return false; }
  };

  struct drw_icy_touch_t : public spell_t
  {
    drw_icy_touch_t( player_t* player ) :
      spell_t( "icy_touch", player, RESOURCE_NONE, SCHOOL_FROST, TREE_BLOOD )
    {
      pet_t* p = player -> cast_pet();
      death_knight_t* o = p -> owner -> cast_death_knight();
      background  = true;
      trigger_gcd = 0;
      may_crit = true;

      base_execute_time = 0;
      base_spell_power_multiplier  = 0;
      base_attack_power_multiplier = 1;

      direct_power_mod  = 0.1;
      base_multiplier *= 0.5;
      base_dd_min = 227;
      base_dd_max = 245;
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

  struct drw_obliterate_t : public attack_t
  {
    drw_obliterate_t( player_t* player ) :
      attack_t( "obliterate", player, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_BLOOD, true )
    {
      background  = true;
      trigger_gcd = 0;
      may_crit = true;

      weapon = &( player -> main_hand_weapon );
      normalize_weapon_speed = true;
      weapon_multiplier     *= 0.8;

      pet_t* p = player -> cast_pet();
      death_knight_t* o = p -> owner -> cast_death_knight();
      if ( o -> glyphs.obliterate ) weapon_multiplier *= 1.0;
      base_multiplier *= 1.0 + o -> set_bonus.tier10_2pc_melee() * 0.1;
      base_multiplier *= 0.50; // DRW malus
      base_dd_min = base_dd_max = 584;

      if ( p -> owner -> race == RACE_ORC )
      {
        base_multiplier *= 1.05;
      }
    }

    virtual void target_debuff( int dmg_type )
    {
      dancing_rune_weapon_pet_t* p = ( dancing_rune_weapon_pet_t* ) player;
      attack_t::target_debuff( dmg_type );
      target_multiplier *= 1 + p -> drw_diseases() * 0.1;
    }

    virtual void execute()
    {
      dancing_rune_weapon_pet_t* p = ( dancing_rune_weapon_pet_t* ) player;
      attack_t::execute();

      if ( result_is_hit() )
      {
        p -> dots_drw_blood_plague -> reset();
        p -> dots_drw_frost_fever -> reset();
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
      background  = true;
      trigger_gcd = 0;
      may_crit = true;

      weapon = &( player -> main_hand_weapon );
      normalize_weapon_speed = true;
      weapon_multiplier     *= 0.5;

      pet_t* p = player -> cast_pet();
      base_multiplier *= 0.50; // DRW malus
      base_dd_min = base_dd_max = 378;

      if ( p -> owner -> race == RACE_ORC )
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
      weapon = &( p -> owner -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      base_dd_min = 2;
      base_dd_max = 322;
      may_crit = true;
      background = true;
      repeating = true;
      weapon_power_mod *= 2.0; //Attack power scaling is unaffected by the DRW 50% penalty.
      base_multiplier *= 0.5;

      if ( p -> owner -> race == RACE_ORC )
      {
        base_multiplier *= 1.05;
      }
    }
  };

  double snapshot_spell_crit, snapshot_attack_crit, haste_snapshot;
  spell_t*  drw_blood_boil;
  spell_t*  drw_blood_plague;
  attack_t* drw_blood_strike;
  spell_t*  drw_death_coil;  
  attack_t* drw_death_strike;
  spell_t*  drw_frost_fever;
  attack_t* drw_heart_strike;
  spell_t*  drw_icy_touch;
  attack_t* drw_obliterate;
  spell_t*  drw_pestilence;
  attack_t* drw_plague_strike;
  attack_t* drw_melee;

  dancing_rune_weapon_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "dancing_rune_weapon", true ),
    snapshot_spell_crit( 0.0 ), snapshot_attack_crit( 0.0 ),
    haste_snapshot( 1.0 ), drw_blood_boil( 0 ), drw_blood_plague( 0 ),
    drw_blood_strike( 0 ), drw_death_coil( 0 ), drw_death_strike( 0 ),
    drw_frost_fever( 0 ), drw_heart_strike( 0 ), drw_icy_touch( 0 ),
    drw_obliterate( 0 ), drw_pestilence( 0 ), drw_plague_strike( 0 ),
    drw_melee( 0 )
  {
    dots_drw_blood_plague  = get_dot( "blood_plague" );
    dots_drw_frost_fever   = get_dot( "frost_fever" );

    main_hand_weapon.type       = WEAPON_SWORD_2H;
    main_hand_weapon.min_dmg    = 685;
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
    drw_blood_strike  = new drw_blood_strike_t ( this );
    drw_death_strike  = new drw_death_strike_t ( this );
    drw_frost_fever   = new drw_frost_fever_t  ( this );
    drw_heart_strike  = new drw_heart_strike_t ( this );
    drw_icy_touch     = new drw_icy_touch_t    ( this );
    drw_obliterate    = new drw_obliterate_t   ( this );
    drw_pestilence    = new drw_pestilence_t   ( this );
    drw_plague_strike = new drw_plague_strike_t( this );
    drw_melee         = new drw_melee_t        ( this );
  }
    
  virtual double composite_attack_crit() SC_CONST        { return snapshot_attack_crit; }
  virtual double composite_attack_haste() SC_CONST       { return haste_snapshot; }
  virtual double composite_attack_power() SC_CONST       { return attack_power; }
  virtual double composite_attack_penetration() SC_CONST { return attack_penetration; }
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
    attack_penetration   = o -> composite_attack_penetration();
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
  ghoul_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "ghoul" )
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
      attack_t( "ghoul_pet_attack", player, RESOURCE_ENERGY, SCHOOL_PHYSICAL )
    {
      weapon = &( player -> main_hand_weapon );
      may_crit = true;
      weapon_power_mod *= 0.84;
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
      direct_power_mod  = 0;
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

      id = 91776;
      parse_data( p -> player_data );

      base_dd_min      = base_dd_max = 0.1;
      direct_power_mod = 0;
    }
  };

  struct ghoul_pet_sweeping_claws_t : public ghoul_pet_attack_t
  {
    ghoul_pet_sweeping_claws_t( player_t* player ) :
      ghoul_pet_attack_t( "sweeping_claws", player )
    {
      ghoul_pet_t* p = ( ghoul_pet_t* ) player -> cast_pet();      

      id = 91778;
      parse_data( p -> player_data );

      base_dd_min      = base_dd_max = 0.1;
      direct_power_mod = 0;
    }
    
    virtual void assess_damage( double amount, int dmg_type )
    {
      ghoul_pet_attack_t::assess_damage( amount, dmg_type );

      for ( int i=0; i < sim -> target -> adds_nearby && i < 2; i ++ )
      {
        ghoul_pet_attack_t::additional_damage( amount, dmg_type );
      }
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
    initial_attack_power_per_strength = 1.0;
    initial_attack_power_per_agility  = 1.0;

    initial_attack_crit_per_agility = rating_t::interpolate( level, 0.01/25.0, 0.01/40.0, 0.01/83.3 );

    resource_base[ RESOURCE_ENERGY ] = 100;
    energy_regen_per_second  = 10;
  }

  virtual bool ooc_buffs()
  {
    death_knight_t* o = owner -> cast_death_knight();
    if ( o -> passives.master_of_ghouls -> ok() )
      return true;

    return false;
  }

  virtual double strength() SC_CONST
  {
    death_knight_t* o = owner -> cast_death_knight();
    double a = attribute[ ATTR_STRENGTH ];
    if ( o -> passives.master_of_ghouls )
      a += std::max( sim -> auras.strength_of_earth -> value(),
           std::max( sim -> auras.horn_of_winter    -> value(),
                     sim -> auras.battle_shout      -> value() ) );

    double strength_scaling = 0.7; // % of str ghould gets from the DK
    strength_scaling += o -> glyphs.raise_dead * .4; // But the glyph is additive!
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

  virtual void player_buff()
  {
    death_knight_t* o = owner -> cast_death_knight();
    if ( o -> buffs_shadow_infusion -> check() )
    {
      // player_multiplier *= o -> buffs_shadow_infusion -> stack() * 0.10;
    }
  }

  virtual int primary_resource() SC_CONST { return RESOURCE_ENERGY; }

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

  death_knight_attack_t( const char* n, player_t* player, const school_type s=SCHOOL_PHYSICAL, int t=TREE_NONE, bool special=true ) :
    attack_t( n, player, RESOURCE_RUNIC, s, t ),
    requires_weapon( true ),
    cost_blood( 0 ),cost_frost( 0 ),cost_unholy( 0 ),convert_runes( 0 ),
    additive_factors( false )
  {
    death_knight_t* p = player -> cast_death_knight();
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;
    may_crit   = true;
    may_glance = false;

    base_crit += p -> talents.ebon_plaguebringer -> rank() * 0.01;
  }

  virtual void   reset();
    
  virtual void   consume_resource();
  virtual double cost() SC_CONST;
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

  death_knight_spell_t( const char* n, player_t* player, const school_type s, int t ) :
    spell_t( n, player, RESOURCE_RUNIC, s, t ),
    cost_blood( 0 ),cost_frost( 0 ),cost_unholy( 0 ),convert_runes( false )
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

    base_crit += p -> talents.ebon_plaguebringer -> rank() * 0.01;
  }

  virtual void   reset();
  
  virtual void   consume_resource();
  virtual double cost() SC_CONST;
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

static int count_death_runes( death_knight_t* p, bool inactive )
{
  // Getting death rune count is a bit more complicated as it depends
  // on talents which runetype can be converted to death runes
  double t = p -> sim -> current_time;
  int count = 0;
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
  {
    dk_rune_t& r = p -> _runes.slot[i];
    if ( ( inactive || r.is_ready( t ) ) && r.is_death() )
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
      p -> _runes.slot[i].consume( p->sim, player, convert_runes );

      if ( p -> sim -> log )
        log_t::output( p -> sim, "%s consumes rune #%d, type %d", p -> name(), i, consumed_type );
    }
  }

  if ( p -> sim -> log )
  {
    std::string rune_str;
    for ( int j = 0; j < RUNE_SLOT_MAX; ++j )
    {
      char rune_letter = rune_symbols[p -> _runes.slot[j].get_type()];
      if ( p -> _runes.slot[j].is_death() )
        rune_letter = 'd';

      if ( p -> _runes.slot[j].is_ready( p -> sim -> current_time ) )
        rune_letter = toupper( rune_letter );
      rune_str += rune_letter;
    }
    log_t::output( p -> sim, "%s runes: %s", p -> name(), rune_str.c_str() );
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

// Trigger Brittle Bones ====================================================

static void trigger_brittle_bones( spell_t* s )
{
  death_knight_t* p = s -> player -> cast_death_knight();
  if ( ! p -> talents.brittle_bones -> rank() )
    return;

  target_t* t = s -> sim -> target;

  // Don't set duration if it's set to 0
  if ( t -> debuffs.brittle_bones -> buff_duration > 0 )
  {
    t -> debuffs.brittle_bones -> buff_duration = s -> num_ticks * s -> base_tick_time;
  }
  double bb_value = p -> talents.brittle_bones -> effect_base_value( 2 ) / 100.0;

  if ( bb_value >= t -> debuffs.brittle_bones -> current_value )
  {
    t -> debuffs.brittle_bones -> trigger( 1, bb_value );
  }
}

// Trigger Ebon Plaguebringer ===============================================

static void trigger_ebon_plaguebringer( action_t* a )
{
  if ( a -> sim -> overrides.ebon_plaguebringer ) return;
  death_knight_t* p = a -> player -> cast_death_knight();
  if ( ! p -> talents.ebon_plaguebringer -> rank() ) return;

  double disease_duration = a -> dot -> ready - a -> sim -> current_time;
  if ( a -> sim -> target -> debuffs.ebon_plaguebringer -> remains_lt( disease_duration ) )
  {
    double value = util_t::talent_rank( p -> talents.ebon_plaguebringer -> rank(), 3, 4, 9, 13 );
    a -> sim -> target -> debuffs.ebon_plaguebringer -> buff_duration = disease_duration;
    a -> sim -> target -> debuffs.ebon_plaguebringer -> trigger( 1, value );
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
    unholy_blight_t( player_t* player ) :
      death_knight_spell_t( "unholy_blight", player, SCHOOL_SHADOW, TREE_UNHOLY )
    {
      death_knight_t* p = player -> cast_death_knight();

      id = 49194;
      parse_data( p -> player_data );

      base_tick_time    = 2.0;
      num_ticks         = 5;
      background        = true;
      proc              = true;
      may_crit          = false;
      may_resist        = false;
      may_miss          = false;
      scale_with_haste  = false;

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

  if ( ! p -> active_unholy_blight )
    p -> active_unholy_blight = new unholy_blight_t( p );

  double unholy_blight_dmg = death_coil_dmg * 0.10;
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
    if ( p -> buffs_frost_presence -> check() )
      gain *= 1 + sim -> sim_data.effect_base_value( 50384, E_APPLY_AURA, A_ADD_FLAT_MODIFIER );
    if ( p -> talents.improved_frost_presence -> rank() )
      gain *= 1.0 + p -> talents.improved_frost_presence -> effect_base_value( 2 ) / 100.0;

    if ( result_is_hit() )
      p -> resource_gain( resource, gain, p -> gains_rune_abilities );
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

// death_knight_attack_t::cost ==============================================

double death_knight_attack_t::cost() SC_CONST
{
  double c = attack_t::cost() / 10.0; // Runic Power Costs are stored as * 10
  return c;
}

// death_knight_attack_t::execute() =========================================

void death_knight_attack_t::execute()
{
  death_knight_t* p = player -> cast_death_knight();

  attack_t::execute();

  if ( result_is_hit() )
  {
    p -> buffs_bloodworms -> trigger();
  }
}

// death_knight_attack_t::player_buff() =====================================

void death_knight_attack_t::player_buff()
{
  death_knight_t* p = player -> cast_death_knight();

  attack_t::player_buff();

  if ( weapon )
  {
    if ( p -> talents.nerves_of_cold_steel -> rank() )
    {
      if ( weapon -> slot == SLOT_OFF_HAND )
        player_multiplier *= 1.0 + p -> talents.nerves_of_cold_steel -> effect_base_value( 2 ) / 100.0;
      if ( weapon -> group() == WEAPON_1H )
        player_hit += p -> talents.nerves_of_cold_steel -> effect_base_value( 1 ) / 100.0;
    }
  }

  // 3.3 Scourge Strike: Shadow damage is calcuted with some buffs
  // being additive Desolation, Bone Shield, Black Ice, Blood
  // Presence.  We divide away the multiplier as if they weren't
  // additive then multiply by the additive multiplier to get the
  // correct value since composite_player_multiplier will already have
  // factored these in.
  if ( additive_factors )
  {
    std::vector<double> sum_factors;

    sum_factors.push_back( p -> buffs_blood_presence -> value() );
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
    player_multiplier *= 1.0 + p -> mastery.frozen_heart -> effect_base_value( 2 ) / 10000.0;
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

// death_knight_attack_t::target_debuff =====================================

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

// death_knight_spell_t::reset() ============================================

void death_knight_spell_t::reset()
{
  for ( int i = 0; i < RUNE_SLOT_MAX; ++i ) use[i] = false;
  action_t::reset();
}

// death_knight_spell_t::cost ===============================================

double death_knight_spell_t::cost() SC_CONST
{
  double c = spell_t::cost() / 10.0; // Runic Power Costs are stored as * 10
  return c;
}

// death_knight_spell_t::consume_resource() =================================

void death_knight_spell_t::consume_resource()
{
  death_knight_t* p = player -> cast_death_knight();
  double gain = -cost();
  if ( gain > 0 )
  {
    if ( result_is_hit() ) p -> resource_gain( resource, gain, p -> gains_rune_abilities );
  }
  else
  {
    spell_t::consume_resource();
  }

  if ( result_is_hit() )
    consume_runes( player, use, convert_runes == 0 ? false : sim->roll( convert_runes ) == 1 );
}

// death_knight_spell_t::player_buff() ======================================

void death_knight_spell_t::player_buff()
{
  death_knight_t* p = player -> cast_death_knight();

  spell_t::player_buff();

  // TODO: merge this with death_knight_attack_t::player_buff to avoid
  // redundancy and errors when adding abilities.

  player_multiplier *= 1.0 + p -> buffs_tier10_4pc_melee -> value();

  if ( p -> mastery.frozen_heart -> ok() && school == SCHOOL_FROST )
  {
    player_multiplier *= 1.0 + p -> mastery.frozen_heart -> effect_base_value( 2 ) / 10000.0;
  }

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

// death_knight_spell_t::target_debuff() ====================================

void death_knight_spell_t::target_debuff( int dmg_type )
{
  spell_t::target_debuff( dmg_type );
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
      trigger_blood_caked_blade( this );
      if ( p -> buffs_scent_of_blood -> up() )
      {
        p -> resource_gain( resource, 10, p -> gains_scent_of_blood );
        p -> buffs_scent_of_blood -> decrement();
      }
      // KM: 1/2/3 PPM proc, only auto-attacks
      // TODO: Confirm PPM
      double chance = weapon -> proc_chance_on_swing( p -> talents.killing_machine -> rank() );
      p -> buffs_killing_machine -> trigger( 1, 1.0, chance );

      if ( p -> rng_might_of_the_frozen_wastes -> roll( p -> talents.might_of_the_frozen_wastes -> proc_chance() ) )
      {
        p -> resource_gain( resource, sim -> sim_data.effect_base_value( 81331, E_ENERGIZE, A_NONE ), p -> gains_might_of_the_frozen_wastes );
      }
    }
  }
};

// Auto Attack ==============================================================

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
    init_rank( ranks, 48721 );

    parse_data( p -> player_data );

    cost_blood = 1;
    aoe = true;

    base_execute_time  = 0;
    direct_power_mod   = 0.06;
    player_multiplier *= 1.0 + p -> talents.crimson_scourge -> effect_base_value( 1 ) / 100.0;
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    base_dd_adder = ( p -> diseases() ? 95 : 0 );
    direct_power_mod  = 0.06 + ( p -> diseases() ? 0.035 : 0 );
    death_knight_spell_t::execute();
    if ( p -> buffs_dancing_rune_weapon -> check() )
      p -> active_dancing_rune_weapon -> drw_blood_boil -> execute();
  }
};

// Blood Plague =============================================================

struct blood_plague_t : public death_knight_spell_t
{
  blood_plague_t( player_t* player ) :
    death_knight_spell_t( "blood_plague", player, SCHOOL_SHADOW, TREE_UNHOLY )
  {
    death_knight_t* p = player -> cast_death_knight();

    id = p -> spells.blood_plague -> spell_id();
    effect_nr = 0;
    parse_data(p->player_data);

    trigger_gcd       = 0;
    base_cost         = 0;
    base_execute_time = 0;
    base_tick_time    = 3.0;
    num_ticks         = 5 + p -> talents.epidemic -> effect_base_value( 1 ) / 1000 / 3;
    base_attack_power_multiplier *= 0.055 * 1.15;
    base_multiplier  *= 1.0 + p -> talents.ebon_plaguebringer -> rank() * 0.15;
    tick_power_mod    = 1;
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
    num_ticks = 5 + p -> talents.epidemic -> effect_base_value( 1 ) / 1000 / 3;
    if ( ! sim -> overrides.blood_plague )
    {
      t -> debuffs.blood_plague -> buff_duration = 3.0 * num_ticks;
      t -> debuffs.blood_plague -> trigger();
    }
    trigger_ebon_plaguebringer( this );
  }

  virtual void extend_duration( int extra_ticks )
  {
    death_knight_spell_t::extend_duration( extra_ticks );
    target_t* t = sim -> target;
    if ( ! sim -> overrides.blood_plague && t -> debuffs.blood_plague -> remains_lt( dot -> ready ) )
    {
      t -> debuffs.blood_plague -> buff_duration = dot -> ready - sim -> current_time;
      t -> debuffs.blood_plague -> trigger();
    }
    trigger_ebon_plaguebringer( this );
  }

  virtual void player_buff()
  {
    death_knight_spell_t::player_buff();
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> mastery.blightcaller -> ok() )
    {
      player_multiplier *= 1.0 + p -> mastery.blightcaller -> effect_base_value( 2 ) / 1000.0 * p -> composite_mastery();
    }
  }

  virtual void refresh_duration()
  {
    death_knight_spell_t::refresh_duration();
    target_t* t = sim -> target;
    if ( ! sim -> overrides.blood_plague && t -> debuffs.blood_plague -> remains_lt( dot -> ready ) )
    {
      t -> debuffs.blood_plague -> buff_duration = dot -> ready - sim -> current_time;
      t -> debuffs.blood_plague -> trigger();
    }
    trigger_ebon_plaguebringer( this );
  }

  virtual bool ready() { return false; }
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
      { 80,  6, 764, 764, 0, -10 },
      { 74,  5, 625, 625, 0, -10 },
      { 69,  4, 411, 411, 0, -10 },
      { 64,  3, 347, 347, 0, -10 },
      { 59,  2, 295, 295, 0, -10 },
      { 55,  1, 260, 260, 0, -10 },
      { 0,   0,   0,   0, 0,   0 }
    };
    init_rank( ranks, 49930 );
    parse_data( p -> player_data );

    cost_blood = 1;
    if ( p -> passives.blood_of_the_north -> ok() || p -> passives.reaping -> ok() )
      convert_runes = 1.0;

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier *= 0.4;
  }

  virtual void target_debuff( int dmg_type )
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::target_debuff( dmg_type );
    target_multiplier *= 1 + p -> diseases() * 0.125;
  }

  virtual void consume_resource() { }
  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    weapon = &( p -> main_hand_weapon );
    death_knight_attack_t::execute();
    death_knight_attack_t::consume_resource();

    if ( p -> buffs_dancing_rune_weapon -> check() )
      p -> active_dancing_rune_weapon -> drw_blood_strike -> execute();

    if ( result_is_hit() )
    {
        if ( p -> buffs_dancing_rune_weapon -> check() ) p -> active_dancing_rune_weapon -> drw_blood_strike -> execute();
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
    death_knight_t* p = player -> cast_death_knight();
    parse_options( options, options_str );

    id = 45529;
    parse_data( p -> player_data );

    if ( p -> talents.improved_blood_tap -> rank() )
      cooldown -> duration += p -> talents.improved_blood_tap -> effect_base_value( 1 ) / 1000.0;
    
    harmful = false;
  }

  void execute()
  {
    death_knight_spell_t::execute();

    death_knight_t* p = player -> cast_death_knight();
    double t = p -> sim -> current_time;

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
      if ( r.get_type() == RUNE_TYPE_BLOOD && ! r.is_death() && ! r.is_ready( t ) )
      {
        r.fill_rune( p );
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
        if ( r.get_type() == RUNE_TYPE_BLOOD && r.is_death() && ! r.is_ready( t ) )
        {
          r.fill_rune( p );
          rune_was_refreshed = true;
          break;
        }
      }
    }

    // Now find a ready blood rune and turn it into a death rune.
    for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    {
      dk_rune_t& r = p -> _runes.slot[i];
      if ( r.get_type() == RUNE_TYPE_BLOOD && ! r.is_death() && r.is_ready( t ) )
      {
        r.type = ( r.type & RUNE_TYPE_MASK ) | RUNE_TYPE_DEATH;
        break;
      }
    }

    // Rune tap gives 10 RP.
    p -> resource_gain( RESOURCE_RUNIC, 10, p -> gains_rune_abilities );
  }
};

// Bone Shield ==============================================================

struct bone_shield_t : public death_knight_spell_t
{
  bone_shield_t( player_t* player, const std::string& options_str ) :
    death_knight_spell_t( "bone_shield", player, SCHOOL_SHADOW, TREE_UNHOLY )
  {
    death_knight_t* p = player -> cast_death_knight();
    check_talent( p -> talents.bone_shield -> rank() );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    id = 49222;
    parse_data( p -> player_data );

    cost_unholy = 1;
    harmful     = false;
  }

  virtual double gcd() SC_CONST { return player -> in_combat ? death_knight_spell_t::gcd() : 0; }

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
    p -> resource_gain( RESOURCE_RUNIC, 10, p -> gains_rune_abilities );
  }
};

// Dancing Rune Weapon ======================================================

struct dancing_rune_weapon_t : public death_knight_spell_t
{
  dancing_rune_weapon_t( player_t* player, const std::string& options_str ) :
    death_knight_spell_t( "dancing_rune_weapon", player, SCHOOL_NONE, TREE_BLOOD )
  {
    death_knight_t* p = player -> cast_death_knight();
    check_talent( p -> talents.dancing_rune_weapon -> rank() );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    id = 49028;
    parse_data( p -> player_data );
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_spell_t::execute();
    p -> buffs_dancing_rune_weapon -> trigger();
    p -> summon_pet( "dancing_rune_weapon", 12.0 );
  }
};

// Dark Transformation ======================================================

struct dark_transformation_t : public death_knight_spell_t
{
  dark_transformation_t( player_t* player, const std::string& options_str ) :
    death_knight_spell_t( "dark_transformation", player, SCHOOL_NONE, TREE_FROST )
  {
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    id = 63560;
    parse_data( p -> player_data );

    cost_unholy = 1;
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_spell_t::execute();

    p -> buffs_dark_transformation -> trigger();
  }

  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();

    if ( p -> buffs_shadow_infusion -> stack() != 5 )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Death Coil ===============================================================

struct death_coil_t : public death_knight_spell_t
{
  death_coil_t( player_t* player, const std::string& options_str ) :
    death_knight_spell_t( "death_coil", player, SCHOOL_SHADOW, TREE_UNHOLY )
  {
    death_knight_t* p = player -> cast_death_knight();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    id = p -> spells.death_coil -> spell_id();
    parse_data( p -> player_data );

    direct_power_mod    = 0.15;
    base_dd_multiplier *= 1 + ( p -> talents.morbidity -> effect_base_value( 1 ) / 100.0 +
                                0.15 * p -> glyphs.death_coil );
    if ( p -> talents.runic_corruption -> rank() )
      base_cost += effect_base_value( 2 );
  }

  void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_spell_t::execute();

    if ( p -> buffs_dancing_rune_weapon -> check() )
      p -> active_dancing_rune_weapon -> drw_death_coil -> execute();

    if ( result_is_hit() )
      trigger_unholy_blight( this, direct_dmg );

    if ( p -> sim -> roll( p -> constants.runic_empowerment_proc_chance ) )
      p -> trigger_runic_empowerment();
    
    p -> buffs_shadow_infusion -> trigger();
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
      { 80,  5, 297, 297, 0, -15 },
      { 75,  4, 250, 250, 0, -15 },
      { 70,  3, 165, 165, 0, -15 },
      { 63,  2, 130, 130, 0, -15 },
      { 56,  1, 112, 112, 0, -15 },
      {  0,  0,   0,   0, 0,   0 }
    };
    init_rank( ranks, 49924 );

    cost_frost  = 1;
    cost_unholy = 1;
    if ( p -> passives.blood_rites -> ok() )
      convert_runes = 1.0;

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier *= 0.75;

    base_crit += p -> talents.improved_death_strike -> effect_base_value( 2 ) / 100.0;
    base_multiplier *= 1 + p -> talents.improved_death_strike -> effect_base_value( 1 ) / 100.0;
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
      if ( p -> resource_current[ RESOURCE_RUNIC ] >= 25 )
      {
        player_multiplier *= 1.25;
      }
      else
      {
        player_multiplier *= 1 + p -> resource_current[ RESOURCE_RUNIC ] * 0.01;
      }
    }
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

    id = 47568;
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_spell_t::execute();

    for ( int i = 0; i < RUNE_SLOT_MAX; ++i )
    {
      dk_rune_t& r = p -> _runes.slot[i];
      r.fill_rune( p );
    }
    p -> resource_gain( RESOURCE_RUNIC, 25, p -> gains_rune_abilities );
  }
};

// Festering Strike =========================================================

struct festering_strike_t : public death_knight_attack_t
{
  festering_strike_t( player_t* player, const std::string& options_str  ) :
    death_knight_attack_t( "festering_strike", player, SCHOOL_PHYSICAL, TREE_UNHOLY )
  {
    death_knight_t* p = player -> cast_death_knight();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    id = p -> spells.festering_strike -> spell_id();
    effect_nr = 0;
    parse_data(p->player_data);

    cost_frost = 1;
    cost_blood = 1;
    if ( p -> passives.reaping -> ok() )
      convert_runes = 1.0;

    base_multiplier *= 1.0 + p -> talents.rage_of_rivendare -> effect_base_value( 1 ) / 100.0;

    weapon = &( p -> main_hand_weapon );
    normalize_weapon_speed = true;
    weapon_multiplier     *= 0.50;
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
      if (  p -> dots_blood_plague -> ticking() )
	p -> dots_blood_plague -> action -> extend_duration( 2 );
      if (  p -> dots_frost_fever -> ticking() )
	p -> dots_frost_fever -> action -> extend_duration( 2 );
    }
  }
};

// Frost Fever ==============================================================

struct frost_fever_t : public death_knight_spell_t
{
  frost_fever_t( player_t* player ) :
    death_knight_spell_t( "frost_fever", player, SCHOOL_FROST, TREE_FROST )
  {
    death_knight_t* p = player -> cast_death_knight();

    id = p-> spells.blood_plague -> spell_id();
    effect_nr = 0;
    parse_data(p->player_data);

    trigger_gcd       = 0;
    base_cost         = 0;
    base_execute_time = 0;
    base_tick_time    = 3.0;
    num_ticks = 5 + p -> talents.epidemic -> effect_base_value( 1 ) / 1000 / 3;
    base_attack_power_multiplier *= 0.055 * 1.15;
    base_multiplier  *= 1.0 + p -> glyphs.icy_touch * 0.2 + p -> talents.ebon_plaguebringer -> rank() * 0.15;
    tick_power_mod    = 1;

    may_miss          = false;
    cooldown -> duration = 0.0;

    reset();
  }

  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_spell_t::execute();

    target_t* t = sim -> target;
    added_ticks = 0;
    num_ticks = 5 + p -> talents.epidemic -> effect_base_value( 1 ) / 1000 / 3;
    if ( ! sim -> overrides.frost_fever )
    {
      t -> debuffs.frost_fever -> buff_duration = 3.0 * num_ticks;
      t -> debuffs.frost_fever -> trigger();
    }
    trigger_brittle_bones( this );
    trigger_ebon_plaguebringer( this );
  }

  virtual void extend_duration( int extra_ticks )
  {
    death_knight_spell_t::extend_duration( extra_ticks );
    target_t* t = sim -> target;
    if ( ! sim -> overrides.frost_fever && t -> debuffs.frost_fever -> remains_lt( dot -> ready ) )
    {
      t -> debuffs.frost_fever -> buff_duration = dot -> ready - sim -> current_time;
      t -> debuffs.frost_fever -> trigger();
    }
    trigger_brittle_bones( this );
    trigger_ebon_plaguebringer( this );
  }

  virtual void refresh_duration()
  {
    death_knight_spell_t::refresh_duration();
    target_t* t = sim -> target;
    if ( ! sim -> overrides.frost_fever && t -> debuffs.frost_fever -> remains_lt( dot -> ready ) )
    {
      t -> debuffs.frost_fever -> buff_duration = dot -> ready - sim -> current_time;
      t -> debuffs.frost_fever -> trigger();
    }
    trigger_brittle_bones( this );
    trigger_ebon_plaguebringer( this );
  }

  virtual void player_buff()
  {
    death_knight_spell_t::player_buff();
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> mastery.blightcaller -> ok() )
    {
      player_multiplier *= 1.0 + p -> mastery.blightcaller -> effect_base_value( 2 ) / 1000.0 * p -> composite_mastery();
    }
  }

  virtual bool ready()     { return false; }
};

// Frost Strike =============================================================

struct frost_strike_t : public death_knight_attack_t
{
  frost_strike_t( player_t* player, const std::string& options_str  ) :
    death_knight_attack_t( "frost_strike", player, SCHOOL_FROST, TREE_FROST )
  {
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    id = 49143;
    parse_data( p -> player_data );

    base_cost -= p -> glyphs.frost_strike * 80;
  }

  virtual void consume_resource() { }
  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    weapon = &( p -> main_hand_weapon );
    death_knight_attack_t::execute();
    death_knight_attack_t::consume_resource();

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( p -> rng_threat_of_thassarian -> roll ( p -> talents.threat_of_thassarian -> effect_base_value( 1 ) / 100.0 ) )
      {
        weapon = &( p -> off_hand_weapon );
        death_knight_attack_t::execute();
      }
    }
    p -> buffs_killing_machine -> expire();
    if ( p -> sim -> roll( p -> constants.runic_empowerment_proc_chance ) )
      p -> trigger_runic_empowerment();
  }

  virtual void player_buff()
  {
    death_knight_attack_t::player_buff();
    death_knight_t* p = player -> cast_death_knight();

    player_crit += p -> buffs_killing_machine -> value();

    if ( sim -> target -> health_percentage() < 35 )
      player_multiplier *= 1.0 + p -> talents.merciless_combat -> effect_base_value( 1 ) / 100.0;
  }
};

// Heart Strike =============================================================

struct heart_strike_t : public death_knight_attack_t
{
  heart_strike_t( player_t* player, const std::string& options_str  ) :
    death_knight_attack_t( "heart_strike", player, SCHOOL_PHYSICAL, TREE_BLOOD )
  {
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    cost_blood = 1;

    id = 55050;
    parse_data( p -> player_data );

    base_multiplier *= 1 + p -> set_bonus.tier10_2pc_melee() * 0.07;
  }

  virtual void assess_damage( double amount, int dmg_type )
  {
    death_knight_attack_t::assess_damage( amount, dmg_type );

    for ( int i=0; i < sim -> target -> adds_nearby; i ++ )
    {
      amount *= 0.75;
      death_knight_attack_t::additional_damage( amount, dmg_type );
    }
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

  void target_debuff( int dmg_type )
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::target_debuff( dmg_type );
    target_multiplier *= 1 + p -> diseases() * effect_base_value( 3 ) / 100.0;
  }
};

// Horn of Winter============================================================

struct horn_of_winter_t : public death_knight_spell_t
{
  double bonus;

  horn_of_winter_t( player_t* player, const std::string& options_str ) :
    death_knight_spell_t( "horn_of_winter", player, SCHOOL_NONE, TREE_FROST )
  {
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    cooldown -> duration       = 20;
    trigger_gcd    = 1.0;
    bonus = util_t::ability_rank( p -> level, 155,75, 86,65 );
    id = 57623;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    update_ready();
    death_knight_t* p = player -> cast_death_knight();
    if ( ! sim -> overrides.horn_of_winter )
      sim -> auras.horn_of_winter -> buff_duration = 120 + p -> glyphs.horn_of_winter * 60;
    sim -> auras.horn_of_winter -> trigger( 1, bonus );

    player -> resource_gain( RESOURCE_RUNIC, 10, p -> gains_horn_of_winter );
  }

  virtual double gcd() SC_CONST { return player -> in_combat ? death_knight_spell_t::gcd() : 0; }
};

// Howling Blast ============================================================

struct howling_blast_t : public death_knight_spell_t
{
  howling_blast_t( player_t* player, const std::string& options_str ) :
    death_knight_spell_t( "howling_blast", player, SCHOOL_FROST, TREE_FROST )
  {
    death_knight_t* p = player -> cast_death_knight();
    check_talent( p -> talents.howling_blast -> rank() );

    option_t options[] =
    {
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
    init_rank( ranks, 49184 );

    parse_data( p -> player_data );

    direct_power_mod  = 0.4;
    cost_unholy       = 1;
    cost_frost        = 1;
    aoe               = true;
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
        if ( ! p -> frost_fever )
          p -> frost_fever = new frost_fever_t( p );

        p -> frost_fever -> execute();
      }
      if ( p -> talents.chill_of_the_grave -> rank() )
        p -> resource_gain( RESOURCE_RUNIC, p -> talents.chill_of_the_grave -> effect_base_value( 1 ) / 10.0, p -> gains_chill_of_the_grave );
    }
    p -> buffs_rime -> expire();
  }

  virtual void player_buff()
  {
    death_knight_spell_t::player_buff();
    death_knight_t* p = player -> cast_death_knight();

    if ( sim -> target -> health_percentage() < 35 )
      player_multiplier *= 1.0 + p -> talents.merciless_combat -> effect_base_value( 1 ) / 100.0;
  }

  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();

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
    return death_knight_spell_t::ready();
  }
};

// Icy Touch ================================================================

struct icy_touch_t : public death_knight_spell_t
{
  icy_touch_t( player_t* player, const std::string& options_str ) :
    death_knight_spell_t( "icy_touch", player, SCHOOL_FROST, TREE_FROST )
  {
    death_knight_t* p = player -> cast_death_knight();
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    id = p -> spells.icy_touch -> spell_id();
    effect_nr = 0;
    parse_data(p->player_data);

    cost_frost = 1;

    base_execute_time = 0;
    direct_power_mod  = 0.1;
    cooldown -> duration          = 0.0;
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
        p -> resource_gain( RESOURCE_RUNIC, p -> talents.chill_of_the_grave -> effect_base_value( 1 ) / 10.0, p -> gains_chill_of_the_grave );

      if ( ! p -> frost_fever )
        p -> frost_fever = new frost_fever_t( p );
      p -> frost_fever -> execute();
    }
    p -> buffs_killing_machine -> expire();
    p -> buffs_rime -> expire();
  }

  virtual void player_buff()
  {
    death_knight_spell_t::player_buff();
    death_knight_t* p = player -> cast_death_knight();

    player_crit += p -> buffs_killing_machine -> value();

    if ( sim -> target -> health_percentage() < 35 )
      player_multiplier *= 1.0 + p -> talents.merciless_combat -> effect_base_value( 1 ) / 100.0;
  }

  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();

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
    return death_knight_spell_t::ready();
  }
};

// Mind Freeze ==============================================================
struct mind_freeze_t : public death_knight_spell_t
{
  mind_freeze_t( player_t* player, const std::string& options_str ) :
      death_knight_spell_t( "pummel",  player, SCHOOL_PHYSICAL, TREE_FURY )
  {
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    id = 47528;
    parse_data( p -> player_data );

    if ( p -> talents.endless_winter -> rank() )
      base_cost += p -> talents.endless_winter -> effect_base_value( 1 ) / 10.0;

    may_miss = may_resist = may_glance = may_block = may_dodge = may_crit = false;
  }

  virtual bool ready()
  {
    if ( ! sim -> target -> debuffs.casting -> check() ) return false;
    return death_knight_spell_t::ready();
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
      { 79,  4, 584, 584, 0, -15 },
      { 73,  3, 477, 477, 0, -15 },
      { 67,  2, 305, 305, 0, -15 },
      { 61,  1, 248, 248, 0, -15 },
      {  0,  0,   0,   0, 0,   0 }
    };
    init_rank( ranks, 51425 );

    cost_frost = 1;
    cost_unholy = 1;
    if ( p -> passives.blood_rites -> ok() )
      convert_runes = 1.0;
  
    weapon = &( p -> main_hand_weapon );
    weapon_multiplier = 0.8;
    normalize_weapon_speed = true;

    // (0.8+0.2)/0.8 = 1.25
    if ( p -> glyphs.obliterate )
      weapon_multiplier = 1.0;

    base_multiplier *= 1.0 + p -> talents.annihilation -> effect_base_value( 1 ) / 100.0
                            + p -> set_bonus.tier10_2pc_melee() * 0.10;
  }

  virtual void target_debuff( int dmg_type )
  {
    death_knight_t* p = player -> cast_death_knight();
    death_knight_attack_t::target_debuff( dmg_type );
    target_multiplier *= 1 + p -> diseases() * 0.125;
  }

  virtual void player_buff()
  {
    death_knight_attack_t::player_buff();
    death_knight_t* p = player -> cast_death_knight();

    if ( sim -> target -> health_percentage() < 35 )
      player_multiplier *= 1.0 + p -> talents.merciless_combat -> effect_base_value( 1 ) / 100.0;
  }

  virtual void consume_resource() { }
  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    weapon = &( p -> main_hand_weapon );
    death_knight_attack_t::execute();
    death_knight_attack_t::consume_resource();

    if ( p -> buffs_dancing_rune_weapon -> check() )
      p -> active_dancing_rune_weapon -> drw_obliterate -> execute();


    if ( result_is_hit() )
    {
      if ( p -> talents.chill_of_the_grave -> rank() )
        p -> resource_gain( RESOURCE_RUNIC, p -> talents.chill_of_the_grave -> effect_base_value( 1 ) / 10.0, p -> gains_chill_of_the_grave );

      if ( p -> buffs_rime -> trigger() )
      {
        p -> cooldowns_howling_blast -> reset();
        update_ready();
      }
    }
         
    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( p -> rng_threat_of_thassarian -> roll ( p -> talents.threat_of_thassarian -> effect_base_value( 1 ) / 100.0 ) )
      {
        weapon = &( p -> off_hand_weapon );
        death_knight_attack_t::execute();
        if ( result_is_hit() && p -> buffs_rime -> trigger() )
        {
          p -> cooldowns_howling_blast -> reset();
          update_ready();
        }
      }
    }
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
    may_crit = false;
    parse_options( options, options_str );

    id = 50842;
    parse_data( p -> player_data );

    cost_blood = 1;

    if ( p -> passives.blood_of_the_north -> ok() || p -> passives.reaping -> ok() )
      convert_runes = 1.0;
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> buffs_dancing_rune_weapon -> check() )
      p -> active_dancing_rune_weapon -> drw_pestilence -> execute();

    p -> resource_gain( RESOURCE_RUNIC, 10, p -> gains_rune_abilities );
  }
};

// Pillar of Frost ==========================================================
struct pillar_of_frost_t : public death_knight_spell_t
{
  pillar_of_frost_t( player_t* player, const std::string& options_str ) :
    death_knight_spell_t( "pillar_of_frost", player, SCHOOL_NONE, TREE_FROST )
  {
    death_knight_t* p = player -> cast_death_knight();
    check_talent( p -> talents.pillar_of_frost -> rank() );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );    

    id = 51271;
    parse_data( p -> player_data );

    cost_frost = 1;
  }

  virtual void execute()
  {
    death_knight_spell_t::execute();
    death_knight_t* p = player -> cast_death_knight();
    p -> buffs_pillar_of_frost -> trigger( 1, effect_base_value( 1 ) / 100.0 );
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

    id = p -> spells.plague_strike -> spell_id();
    parse_data(p->player_data);

    cost_unholy = 1;

    base_multiplier *= 1.0 + p -> talents.rage_of_rivendare -> effect_base_value( 1 ) / 100.0;
  }

  virtual void consume_resource() { }
  virtual void execute()
  {
    death_knight_t* p = player -> cast_death_knight();
    weapon = &( p -> main_hand_weapon );
    death_knight_attack_t::execute();
    death_knight_attack_t::consume_resource();

    if ( p -> buffs_dancing_rune_weapon -> check() )
      p -> active_dancing_rune_weapon -> drw_plague_strike -> execute();

    if ( result_is_hit() )
    {
      if ( p -> dots_blood_plague -> ticking() )
        p -> buffs_blood_swarm -> trigger();

      if ( ! p -> blood_plague )
        p -> blood_plague = new blood_plague_t( p );
      p -> blood_plague -> execute();
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
      case PRESENCE_BLOOD:  p -> buffs_blood_presence  -> trigger(); break;
      case PRESENCE_FROST:
        {
          double fp_value = sim -> sim_data.effect_base_value( 48266, E_APPLY_AURA, A_MOD_DAMAGE_PERCENT_DONE );
          if ( p -> talents.improved_frost_presence -> rank() )
            fp_value += p -> talents.improved_frost_presence -> effect_base_value( 1 );
          p -> buffs_frost_presence  -> trigger( 1, fp_value / 100.0 );
        }
        break;
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
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    id = 46584;
    parse_data( p -> player_data );

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

  virtual bool ready()
  {
    death_knight_t* p = player -> cast_death_knight();
    if ( p -> active_ghoul )
      return false;

    return death_knight_spell_t::ready();
  }
};

// Scourge Strike ===========================================================
struct scourge_strike_t : public death_knight_attack_t
{
  attack_t* scourge_strike_shadow;
  struct scourge_strike_shadow_t : public death_knight_attack_t
  {
    scourge_strike_shadow_t( player_t* player ) : death_knight_attack_t( "scourge_strike_shadow", player, SCHOOL_SHADOW, TREE_UNHOLY )
    {
      weapon = &( player -> main_hand_weapon );
      may_miss = may_parry = may_dodge = false;
      may_crit    = false;
      proc        = true;
      background  = true;
      trigger_gcd = 0;

      // Only blizzard knows, but for the shadowpart in 3.3 the follwing
      // +x% buffs are ADDITIVE (which this flag controls)
      // Bone Shield 2%, Desolation 5%, Blood Presence 15%, Black Ice 10%
      additive_factors = true;

      weapon_multiplier = 0;

      base_attack_power_multiplier = 0;
      base_dd_min = base_dd_max    = 0.1;
    }

    virtual void target_debuff( int dmg_type )
    {
      death_knight_t* p = player -> cast_death_knight();
      death_knight_attack_t::target_debuff( dmg_type );
      target_multiplier *= p -> diseases() * effect_base_value( 3 ) / 100.0;
    }
  };

  scourge_strike_t( player_t* player, const std::string& options_str  ) :
    death_knight_attack_t( "scourge_strike", player, SCHOOL_PHYSICAL, TREE_UNHOLY )
  {
    death_knight_t* p = player -> cast_death_knight();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    id = p -> spells.scourge_strike -> spell_id();
    parse_data( p -> player_data );
        
    scourge_strike_shadow = new scourge_strike_shadow_t( player );

    cost_unholy = 1;

    base_multiplier *= 1.0 + p -> set_bonus.tier10_2pc_melee() * 0.1
                           + p -> talents.rage_of_rivendare -> effect_base_value( 1 ) / 100.0;
  }

  void execute()
  {
    death_knight_attack_t::execute();
    if ( result_is_hit() )
    {
      death_knight_t* p = player -> cast_death_knight();
      scourge_strike_shadow -> base_dd_adder = direct_dmg;
      scourge_strike_shadow -> execute();

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
    check_talent( p -> talents.summon_gargoyle -> rank() );

    id = 49206;
  }

  virtual void execute()
  {
    consume_resource();
    update_ready();
    player -> summon_pet( "gargoyle", 30.0 );
  }
};

// Unholy Frenzy ============================================================
struct unholy_frenzy_t : public spell_t
{
  player_t* unholy_frenzy_target;

  unholy_frenzy_t( player_t* player, const std::string& options_str ) :
    spell_t( "unholy_frenzy", player, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_UNHOLY ), unholy_frenzy_target( 0 )
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

    id = 49016;
    parse_data( p -> player_data );

    if ( target_str.empty() )
    {
      unholy_frenzy_target = p;
    }
    else
    {
      unholy_frenzy_target = sim -> find_player( target_str );
      assert ( unholy_frenzy_target != 0 );
    }
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
//if ( name == "blood_presence"           ) return new blood_presence_t           ( this, options_str );
  if ( name == "blood_strike"             ) return new blood_strike_t             ( this, options_str );
  if ( name == "blood_tap"                ) return new blood_tap_t                ( this, options_str );
  if ( name == "dancing_rune_weapon"      ) return new dancing_rune_weapon_t      ( this, options_str );
  if ( name == "heart_strike"             ) return new heart_strike_t             ( this, options_str );
  if ( name == "pestilence"               ) return new pestilence_t               ( this, options_str );

  // Frost Actions
  if ( name == "empower_rune_weapon"      ) return new empower_rune_weapon_t      ( this, options_str );
//if ( name == "frost_presence"           ) return new frost_presence_t           ( this, options_str );
  if ( name == "frost_strike"             ) return new frost_strike_t             ( this, options_str );
  if ( name == "horn_of_winter"           ) return new horn_of_winter_t           ( this, options_str );
  if ( name == "howling_blast"            ) return new howling_blast_t            ( this, options_str );
//if ( name == "hungering_cold"           ) return new hungering_cold_t           ( this, options_str );
  if ( name == "icy_touch"                ) return new icy_touch_t                ( this, options_str );
  if ( name == "mind_freeze"              ) return new mind_freeze_t              ( this, options_str );
  if ( name == "obliterate"               ) return new obliterate_t               ( this, options_str );
  if ( name == "pillar_of_frost"          ) return new pillar_of_frost_t          ( this, options_str );
//if ( name == "rune_strike"              ) return new rune_strike_t              ( this, options_str );

  // Unholy Actions
//if ( name == "army_of_the_dead"         ) return new army_of_the_dead_t         ( this, options_str );
  if ( name == "bone_shield"              ) return new bone_shield_t              ( this, options_str );
  if ( name == "dark_transformation"      ) return new dark_transformation_t      ( this, options_str );
//if ( name == "death_and_decay"          ) return new death_and_decay_t          ( this, options_str );
  if ( name == "death_coil"               ) return new death_coil_t               ( this, options_str );
  if ( name == "death_strike"             ) return new death_strike_t             ( this, options_str );
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

// death_knight_t::composite_attack_haste() =================================

double death_knight_t::composite_attack_haste() SC_CONST
{
  double haste = player_t::composite_attack_haste();
  haste *= 1.0 / ( 1.0 + buffs_unholy_presence -> value() );

  if ( passives.icy_talons -> ok() )
    haste *= 1.0 / ( 1.0 + passives.icy_talons -> effect_base_value( 1 ) / 100.0 );
  if ( talents.improved_icy_talons -> rank() )
    haste *= 1.0 / ( 1.0 + talents.improved_icy_talons ->effect_base_value( 3 ) / 100.0 );
  if ( active_presence == PRESENCE_UNHOLY && talents.improved_unholy_presence -> rank() )
    haste *= 1.0 + talents.improved_unholy_presence -> effect_base_value( 2 );

  return haste;
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
}

// death_knight_t::init_race ================================================

void death_knight_t::init_race()
{
  race = util_t::parse_race_type( race_str );
  switch ( race )
  {
    case RACE_BLOOD_ELF: 
    case RACE_DWARF:
    case RACE_DRAENEI:
    case RACE_GOBLIN:
    case RACE_GNOME:
    case RACE_HUMAN:
    case RACE_NIGHT_ELF:
    case RACE_ORC:
    case RACE_TAUREN:
    case RACE_TROLL:
    case RACE_UNDEAD:
    case RACE_WORGEN:  
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
  rng_blood_caked_blade          = get_rng( "blood_caked_blade"          );
  rng_might_of_the_frozen_wastes = get_rng( "might_of_the_frozen_wastes" );
  rng_threat_of_thassarian       = get_rng( "threat_of_thassarian"       );
}

// death_knight_t::init_base ================================================

void death_knight_t::init_base()
{
  player_t::init_base();

  double str_mult = ( talents.abominations_might -> effect_base_value( 2 ) / 100.0 +
                      talents.endless_winter -> rank() * 0.02 );

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

  resource_base[ RESOURCE_RUNIC ] = 100;
  if ( talents.runic_power_mastery -> rank() )
    resource_base[ RESOURCE_RUNIC ] += talents.runic_power_mastery -> effect_base_value( 1 ) / 10.0;

  base_gcd = 1.5;
}

// death_knight_t::init_talents =============================================

void death_knight_t::init_talents()
{
  // Blood
  talents.abominations_might          = new talent_t( this, "abominations_might", "Abomination's Might" );
  talents.bladed_armor                = new talent_t( this, "bladed_armor", "Bladed Armor" );
  talents.blood_caked_blade           = new talent_t( this, "blood_caked_blade", "Blood-Caked Blade" );
  talents.blood_parasite              = new talent_t( this, "blood_parasite", "Blood Parasite" );
  talents.bone_shield                 = new talent_t( this, "bone_shield", "Bone Shield" );
  talents.butchery                    = new talent_t( this, "butchery", "Butchery" );
  talents.crimson_scourge             = new talent_t( this, "crimson_scourge", "Crimson Scourge" );
  talents.dancing_rune_weapon         = new talent_t( this, "dancing_rune_weapon", "Dancing Rune Weapon" );
  talents.improved_blood_presence     = new talent_t( this, "improved_blood_presence", "Improved Blood Presence" );
  talents.improved_blood_tap          = new talent_t( this, "improved_blood_tap", "Improved Blood Tap" );
  talents.improved_death_strike       = new talent_t( this, "improved_death_strike", "Improved Death Strike" );
  talents.scent_of_blood              = new talent_t( this, "scent_of_blood", "Scent of Blood" );

  // Frost
  talents.annihilation                = new talent_t( this, "annihilation", "Annihilation" );
  talents.brittle_bones               = new talent_t( this, "brittle_bones", "Brittle Bones" );
  talents.chill_of_the_grave          = new talent_t( this, "chill_of_the_grave", "Chill of the Grave" );
  talents.endless_winter              = new talent_t( this, "endless_winter", "Endless Winter" );
  talents.howling_blast               = new talent_t( this, "howling_blast", "Howling Blast" );
  talents.hungering_cold              = new talent_t( this, "hungering_cold", "Hungering Cold" );
  talents.improved_frost_presence     = new talent_t( this, "improved_frost_presence", "Improved Frost Presence" );
  talents.improved_icy_talons         = new talent_t( this, "improved_icy_talons", "Improved Icy Talons" );
  talents.killing_machine             = new talent_t( this, "killing_machine", "Killing Machine" );
  talents.merciless_combat            = new talent_t( this, "merciless_combat", "Merciless Combat" );
  talents.might_of_the_frozen_wastes  = new talent_t( this, "might_of_the_frozen_wastes", "Might of the Frozen Wastes" );
  talents.nerves_of_cold_steel        = new talent_t( this, "nerves_of_cold_steel", "Nerves of Cold Steel" );
  talents.pillar_of_frost             = new talent_t( this, "pillar_of_frost", "Pillar of Frost" );
  talents.rime                        = new talent_t( this, "rime", "Rime" );
  talents.runic_power_mastery         = new talent_t( this, "runic_power_mastery", "Runic Power Mastery" );
  talents.threat_of_thassarian        = new talent_t( this, "threat_of_thassarian", "Threat of Thassarian" );
  talents.toughness                   = new talent_t( this, "toughness", "Toughness" );
    
  // Unholy    
  talents.contagion                   = new talent_t( this, "contagion", "Contagion" );
  talents.dark_transformation         = new talent_t( this, "dark_transformation", "Dark Transformation" );
  talents.ebon_plaguebringer          = new talent_t( this, "ebon_plaguebringer", "Ebon Plaguebringer" );
  talents.epidemic                    = new talent_t( this, "epidemic", "Epidemic" );
  talents.improved_unholy_presence    = new talent_t( this, "improved_unholy_presence", "Improved Unholy Presence" );
  talents.morbidity                   = new talent_t( this, "morbidity", "Morbidity" );
  talents.rage_of_rivendare           = new talent_t( this, "rage_of_rivendare", "Rage of Rivendare" );
  talents.runic_corruption            = new talent_t( this, "runic_corruption", "Runic Corruption" );
  talents.shadow_infusion             = new talent_t( this, "shadow_infusion", "Shadow Infusion" );
  talents.sudden_doom                 = new talent_t( this, "sudden_doom", "Sudden Doom" );
  talents.summon_gargoyle             = new talent_t( this, "summon_gargoyle", "Summon Gargoyle" );
  talents.unholy_blight               = new talent_t( this, "unholy_blight", "Unholy Blight" );
  talents.unholy_frenzy               = new talent_t( this, "unholy_frenzy", "Unholy Frenzy" );
  talents.virulence                   = new talent_t( this, "virulence", "Virulence" );

  player_t::init_talents();
}

// death_knight_t::init_spells ==============================================

void death_knight_t::init_spells()
{
  player_t::init_spells();

  // Mastery
  mastery.blightcaller               = new passive_spell_t( this, "blightcaller", "Blightcaller", DEATH_KNIGHT_UNHOLY );
  mastery.frozen_heart               = new passive_spell_t( this, "frozen_heart", "Frozen Heart", DEATH_KNIGHT_FROST );

  // Passives
  passives.blood_of_the_north        = new passive_spell_t( this, "blood_of_the_north", "Blood of the North", DEATH_KNIGHT_FROST );
  passives.blood_rites               = new passive_spell_t( this, "blood_rites", "Blood Rites", DEATH_KNIGHT_BLOOD );
  passives.icy_talons                = new passive_spell_t( this, "icy_talons", "Icy Talons", DEATH_KNIGHT_FROST );
  passives.master_of_ghouls          = new passive_spell_t( this, "master_of_ghouls", "Master of Ghouls", DEATH_KNIGHT_UNHOLY );
  passives.reaping                   = new passive_spell_t( this, "reaping", "Reaping", DEATH_KNIGHT_UNHOLY );
  passives.runic_empowerment         = new passive_spell_t( this, "runic_empowerment", 81229 );
  passives.unholy_might              = new passive_spell_t( this, "unholy_might", "Unholy Might", DEATH_KNIGHT_UNHOLY );
  passives.veteran_of_the_third_war  = new passive_spell_t( this, "veteran_of_the_third_war", "Veteran of the Third War", DEATH_KNIGHT_BLOOD );
  
  // Spells
  spells.blood_boil                  = new active_spell_t( this, "blood_boil", "Blood Boil", DEATH_KNIGHT_BLOOD );
  spells.blood_plague                = new active_spell_t( this, "blood_plague", "Blood Plague", DEATH_KNIGHT_UNHOLY );
  spells.blood_strike                = new active_spell_t( this, "blood_strike", "Blood Strike", DEATH_KNIGHT_BLOOD );
  spells.blood_tap                   = new active_spell_t( this, "blood_tap", "Blood Tap", DEATH_KNIGHT_BLOOD );
  spells.death_coil                  = new active_spell_t( this, "death_coil", "Death Coil", DEATH_KNIGHT_UNHOLY );
  spells.death_strike                = new active_spell_t( this, "death_strike", "Death Strike", DEATH_KNIGHT_BLOOD );
  spells.empower_rune_weapon         = new active_spell_t( this, "empower_rune_weapon", "Empower Rune Weapon", DEATH_KNIGHT_FROST );
  spells.festering_strike            = new active_spell_t( this, "festering_strike", "Festering Strke", DEATH_KNIGHT_FROST );
  spells.frost_fever                 = new active_spell_t( this, "frost_fever", "Frost Fever", DEATH_KNIGHT_FROST );
  spells.frost_strike                = new active_spell_t( this, "frost_strike", "Frost Strike", DEATH_KNIGHT_FROST );
  spells.heart_strike                = new active_spell_t( this, "heart_strike", "Heart Strike", DEATH_KNIGHT_BLOOD );
  spells.horn_of_winter              = new active_spell_t( this, "horn_of_winter", "Horn of Winter", DEATH_KNIGHT_FROST );
  spells.icy_touch                   = new active_spell_t( this, "icy_touch", "Icy Touch", DEATH_KNIGHT_FROST );
  spells.mind_freeze                 = new active_spell_t( this, "mind_freeze", "Mind Freeze", DEATH_KNIGHT_FROST );
  spells.necrotic_strike             = new active_spell_t( this, "necrotic_strike", "Necrotic Strike", DEATH_KNIGHT_UNHOLY );
  spells.obliterate                  = new active_spell_t( this, "obliterate", "Obliterate", DEATH_KNIGHT_FROST );
  spells.outbreak                    = new active_spell_t( this, "outbreak", "Outbreak", DEATH_KNIGHT_UNHOLY );
  spells.pestilence                  = new active_spell_t( this, "pestilence", "Pestilence", DEATH_KNIGHT_UNHOLY );
  spells.plague_strike               = new active_spell_t( this, "plague_strike", "Plague Strike", DEATH_KNIGHT_UNHOLY );
  spells.raise_dead                  = new active_spell_t( this, "raise_dead", "Raise Dead", DEATH_KNIGHT_UNHOLY );
  spells.scourge_strike              = new active_spell_t( this, "scourge_strike", "Scourge Strke", DEATH_KNIGHT_UNHOLY );
  spells.unholy_frenzy               = new active_spell_t( this, "unholy_frenzy", "Unholy Frenzy", DEATH_KNIGHT_UNHOLY );
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
    action_list_str  = "flask,type=endless_rage";
    action_list_str += "/food,type=dragonfin_filet";
    action_list_str += "/presence,choose=blood";
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
      action_list_str += "/speed_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
      action_list_str += "/auto_attack";
      if ( talents.bone_shield -> rank() )
        action_list_str += "/bone_shield,if=!buff.bone_shield.up";
      action_list_str += "/raise_dead,time>=10";
      if ( talents.epidemic -> rank() < 2 )
      {
        action_list_str += "/icy_touch,if=dot.frost_fever.remains<=0.1&unholy_cooldown<=2";
        action_list_str += "/plague_strike,if=dot.blood_plague.remains<=0.1&dot.frost_fever.remains>=13";
      }
      else
      {
        action_list_str += "/icy_touch,if=dot.frost_fever.remains<=2";
        action_list_str += "/plague_strike,if=dot.blood_plague.remains<=2";
      }
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
      action_list_str += "/speed_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
      action_list_str += "/auto_attack";
      if ( talents.pillar_of_frost -> rank() )
        action_list_str += "/pillar_of_frost,if=cooldown.blood_tap.remains>=58"; //Forces Pillar of Frost to only be used in combination with Blood Tap

      action_list_str += "/raise_dead,time>=5";
      action_list_str += "/icy_touch,if=dot.frost_fever.remains<=2";
      action_list_str += "/plague_strike,if=dot.blood_plague.remains<=2";
      if ( talents.howling_blast -> rank() )
        action_list_str += "/howling_blast,if=buff.rime.react";
      action_list_str += "/obliterate";
      // If you are using Pillar of Frost, only use Blood Tap after you have consumed Death runes in your rotation.
      // If not, use Blood Tap to produce an extra Obliterate once a minute.
      if ( talents.pillar_of_frost -> rank() )
      {
        action_list_str += "/blood_tap,time>=10,if=blood=0&frost=0&unholy=0&inactive_death=0";
      }
      else
      {
        action_list_str += "/blood_tap,if=blood=1&inactive_death=1";
      }
      action_list_str += "/blood_strike,if=blood=2&death<=2";
      action_list_str += "/blood_strike,if=blood=1&death<=1";
      action_list_str += "/empower_rune_weapon,if=blood=0&unholy=0&death=0";
      if ( spells.frost_strike )
        action_list_str += "/frost_strike";
      if ( talents.howling_blast -> rank() )
        action_list_str += "/howling_blast,if=buff.rime.react";
      break;
    case TREE_UNHOLY:
      action_list_str += "/raise_dead";
      if ( talents.bladed_armor -> rank() > 0 )
      {
        action_list_str += "/indestructible_potion,if=!in_combat";
        action_list_str += "/speed_potion,if=buff.bloodlust.react|target.time_to_die<=60";
      }
      else
      {
        action_list_str += "/speed_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=60";
      }
      action_list_str += "/auto_attack";
      action_list_str += "/icy_touch,if=dot.frost_fever.remains<=2";
      action_list_str += "/plague_strike,if=dot.blood_plague.remains<=2";
      if ( passives.reaping )
      {
        // 0 Death: Always BS
        // 1 Death: Only BS with 2 blood runes (because 1 blood = death)
        // 2 Death: With reaping we only use BS to convert
        action_list_str += "/blood_tap,if=frost=0&unholy=0&blood=1&inactive_death=1";
        action_list_str += "/blood_strike,if=death=0";
        action_list_str += "/blood_strike,if=death=1&blood=2";
        action_list_str += "/scourge_strike";
      }
      else
      {
        action_list_str += "/scourge_strike";
        action_list_str += "/blood_strike";
      }
      if ( talents.summon_gargoyle -> rank() )
      {
        action_list_str += "/summon_gargoyle,time<=60";
        action_list_str += "/summon_gargoyle,if=buff.bloodlust.react";
      }
      action_list_str += "/empower_rune_weapon,if=blood=0&frost=0&unholy=0";
      action_list_str += "/death_coil";
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

  // Rune of the Fallen Crusader =======================================
  struct fallen_crusader_callback_t : public action_callback_t
  {
    int slot;
    buff_t* buff;

    fallen_crusader_callback_t( player_t* p, int s, buff_t* b ) : action_callback_t( p -> sim, p ), slot( s ), buff( b ) {}

    virtual void trigger( action_t* a )
    {
      weapon_t* w = a -> weapon;
      if ( ! w ) return;
      if ( w -> slot != slot ) return;

      // RotFC is 2 PPM.
      double PPM        = 2.0;
      double chance     = w -> proc_chance_on_swing( PPM );

      buff -> trigger( 1, 0.15, chance );
    }
  };

  // Rune of the Razorice =======================================

  // Damage Proc
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

    razorice_callback_t( player_t* p, int s, buff_t* b ) : action_callback_t( p -> sim, p ), slot( s ), buff( b ), razorice_damage_proc( 0 )
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
      buff -> trigger( 1, 0.02, 1.0 );

      razorice_damage_proc -> base_dd_min = w -> min_dmg;
      razorice_damage_proc -> base_dd_max = w -> max_dmg;
      razorice_damage_proc -> execute();
    }
  };

  buffs_rune_of_razorice = new buff_t( this, "rune_of_razorice", 5,  20.0 );

  buffs_rune_of_the_fallen_crusader = new buff_t( this, "rune_of_the_fallen_crusader", 1, 15.0 );
  if ( mh_enchant == "rune_of_the_fallen_crusader" )
  {
    register_attack_result_callback( RESULT_HIT_MASK, new fallen_crusader_callback_t( this, SLOT_MAIN_HAND, buffs_rune_of_the_fallen_crusader ) );
  }
  else if ( mh_enchant == "rune_of_razorice" )
  {
    register_attack_result_callback( RESULT_HIT_MASK, new razorice_callback_t( this, SLOT_MAIN_HAND, buffs_rune_of_razorice ) );
  }
  if ( oh_enchant == "rune_of_the_fallen_crusader" )
  {
    register_attack_result_callback( RESULT_HIT_MASK, new fallen_crusader_callback_t( this, SLOT_OFF_HAND, buffs_rune_of_the_fallen_crusader ) );
  }
  else if ( oh_enchant == "rune_of_razorice" )
  {
    register_attack_result_callback( RESULT_HIT_MASK, new razorice_callback_t( this, SLOT_OFF_HAND, buffs_rune_of_razorice ) );
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
    else if ( n == "hungering_cold"  ) glyphs.hungering_cold = 1;
    else if ( n == "icy_touch"       ) glyphs.icy_touch = 1;
    else if ( n == "obliterate"      ) glyphs.obliterate = 1;
    else if ( n == "raise_dead"      ) glyphs.raise_dead = 1;
    else if ( n == "rune_strike"     ) glyphs.rune_strike = 1;
    else if ( n == "scourge_strike"  ) glyphs.scourge_strike = 1;
    // To prevent warnings
    else if ( n == "anit_magic_shield"   ) ;
    else if ( n == "blood_boil"          ) ;
    else if ( n == "blood_tap"           ) ;
    else if ( n == "bone_shield"         ) ;
    else if ( n == "chians_of_ice"       ) ;
    else if ( n == "dancing_rune_weapon" ) ;
    else if ( n == "death_grip"          ) ;
    else if ( n == "deaths_embrace"      ) ;
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
    scales_with[ STAT_WEAPON_OFFHAND_SPEED  ] = 1;
  }
}

// death_knight_t::init_buffs ===============================================

void death_knight_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( sim, name, max_stack, duration, cooldown, proc_chance, quiet )
  buffs_blood_presence      = new buff_t( this, "blood_presence" );
  buffs_blood_swarm         = new buff_t( this, "blood_swarm",                                        1,  10.0,  0.0, talents.crimson_scourge -> rank() * 0.50 );   
  buffs_bone_shield         = new buff_t( this, "bone_shield",                                        3, 300.0 );          
  buffs_dancing_rune_weapon = new buff_t( this, "dancing_rune_weapon",                                1,  12.0,  0.0, 1.0, true );
  buffs_dark_transformation = new buff_t( this, "dark_transformation",                                1,  30.0 );
  buffs_frost_presence      = new buff_t( this, "frost_presence" );
  buffs_killing_machine     = new buff_t( this, "killing_machine",                                    1,  30.0,  0.0, 0.0 ); // PPM based!
  buffs_pillar_of_frost     = new buff_t( this, "pillar_of_frost",                                    1,  20.0 );
  buffs_rime                = new buff_t( this, "rime",                                               1,  30.0,  0.0, talents.rime -> proc_chance() );
  buffs_runic_corruption    = new buff_t( this, "runic_corruption",                                   1,   3.0,  0.0, talents.runic_corruption -> effect_base_value( 1 ) / 100.0 );
  buffs_scent_of_blood      = new buff_t( this, "scent_of_blood",      talents.scent_of_blood -> rank(),  20.0,  0.0, talents.scent_of_blood -> proc_chance() );
  buffs_shadow_infusion      = new buff_t( this, "shadow_infusion",                                    5,  30.0,  0.0, talents.shadow_infusion -> proc_chance() );
  buffs_tier10_4pc_melee    = new buff_t( this, "tier10_4pc_melee",                                   1,  15.0,  0.0, set_bonus.tier10_4pc_melee() );  
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

  procs_sudden_doom = get_proc( "sudden_doom" );
}

// death_knight_t::init_resources ===========================================

void death_knight_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resource_current[ RESOURCE_RUNIC ] = 0;
}

// death_knight_t::init_uptimes =============================================

void death_knight_t::init_uptimes()
{
  player_t::init_uptimes();

  uptimes_blood_plague       = get_uptime( "blood_plague" );
  uptimes_frost_fever        = get_uptime( "frost_fever" );
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

// death_knight_t::target_swing =============================================

int death_knight_t::target_swing()
{
  int result = player_t::target_swing();

  buffs_scent_of_blood -> trigger();

  return result;
}

// death_knight_t::composite_attack_power ===================================

double death_knight_t::composite_attack_power() SC_CONST
{
  double ap = player_t::composite_attack_power();
  if ( talents.bladed_armor -> rank() )
  {
    ap += composite_armor() / talents.bladed_armor -> effect_base_value( 1 );
  }
  return ap;
}

// death_knight_t::composite_attribute_multiplier ===========================

double death_knight_t::composite_attribute_multiplier( int attr ) SC_CONST
{
  double m = player_t::composite_attribute_multiplier( attr );

  if ( attr == ATTR_STRENGTH )
  {
    m *= 1.0 + buffs_rune_of_the_fallen_crusader -> value();
    if ( talents.brittle_bones -> rank() )
    {
      m *= 1.0 + talents.brittle_bones -> effect_base_value( 2 ) / 100.0;
    }
    if ( buffs_pillar_of_frost -> check() )
    {
      m *= 1.0 + buffs_pillar_of_frost -> value();
    }
    if ( passives.unholy_might -> ok() )
    {
      m *= 1.0 + passives.unholy_might -> effect_base_value( 1 ) / 100.0;
    }
  }

  return m;
}

// death_knight_t::matching_gear_multiplier ==================================

double death_knight_t::matching_gear_multiplier( const stat_type attr ) SC_CONST
{
  switch ( primary_tree() )
  {
  case TREE_UNHOLY:
  case TREE_FROST:
    if ( attr == STAT_STRENGTH )
      return 0.05;
    break;
  case TREE_BLOOD:
    if ( attr == STAT_STAMINA )
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
  if ( talents.virulence -> rank() )
    hit += talents.virulence -> effect_base_value( 1 ) / 100.0;
  return hit;
}

// death_knight_t::composite_tank_parry =====================================

double death_knight_t::composite_tank_parry() SC_CONST
{
  double parry = player_t::composite_tank_parry();

  if ( buffs_dancing_rune_weapon -> check() )
    parry += 0.20;

  return parry;
}

// death_knight_t::composite_player_multiplier ==============================

double death_knight_t::composite_player_multiplier( const school_type school ) SC_CONST
{
  double m = player_t::composite_player_multiplier( school );
  // Factor flat multipliers here so they effect procs, grenades, etc.
  m *= 1.0 + buffs_blood_presence -> value();
  m *= 1.0 + buffs_bone_shield -> value();
  return m;
}

// death_knight_t::interrupt ================================================

void death_knight_t::interrupt()
{
  player_t::interrupt();

  if ( main_hand_attack ) main_hand_attack -> cancel();
  if (  off_hand_attack )  off_hand_attack -> cancel();
}

// death_knight_t::regen ====================================================

void death_knight_t::regen( double periodicity )
{
  player_t::regen( periodicity );

  if ( talents.butchery -> rank() )
    resource_gain( RESOURCE_RUNIC, ( talents.butchery -> effect_base_value( 1 ) / 100.0 / periodicity ), gains_butchery );

  uptimes_blood_plague -> update( dots_blood_plague -> ticking() );
  uptimes_frost_fever  -> update( dots_frost_fever -> ticking() );
}

// death_knight_t::get_translation_list =====================================

std::vector<talent_translation_t>& death_knight_t::get_talent_list()
{
  talent_list.clear();
  return talent_list;
}

// death_knight_t::get_options ==============================================

std::vector<option_t>& death_knight_t::get_options()
{
  if ( options.empty() )
  {
    player_t::get_options();

    option_t death_knight_options[] =
    {
      { "plate_specialization", OPT_BOOL,   &( plate_specialization     ) },
      { "unholy_frenzy_target", OPT_STRING, &( unholy_frenzy_target_str ) },
      // @option_doc loc=player/death_knight/talents title="Talents"
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

void death_knight_t::trigger_runic_empowerment()
{
  if ( talents.runic_corruption -> rank() )
  {
    buffs_runic_corruption -> trigger();
    return;
  }

  double now = sim -> current_time;

  bool fully_depleted_runes[RUNE_SLOT_MAX / 2];
  for ( int i = 0; i < RUNE_SLOT_MAX / 2; ++i )
  {
    fully_depleted_runes[i] = false;
    if ( !_runes.slot[i].is_ready( now ) && !_runes.slot[i+1].is_ready( now ) )
      fully_depleted_runes[i] = true;
  }

  // Use a fair algorithm to pick whichever rune to regen.
  int rune_to_regen = -1;
  for ( int i = 0; i < RUNE_SLOT_MAX / 2; ++i )
  {
    if ( sim -> roll( 1.0 / static_cast<double>( i + 1 ) ) )
    {
      rune_to_regen = i;
    }
  }

  if ( rune_to_regen >= 0 )
  {
    if ( _runes.slot[rune_to_regen].cooldown_time < _runes.slot[rune_to_regen + 1].cooldown_time )
    {
      ++rune_to_regen;
    }
    _runes.slot[rune_to_regen].fill_rune( this );
    if ( sim -> log )
      log_t::output( sim, "runic empowerment regen'd rune %d", rune_to_regen );
  }
}

// player_t implementations =================================================

player_t* player_t::create_death_knight( sim_t* sim, const std::string& name, race_type r )
{
  sim -> errorf( "Deathknight Module isn't available at the moment." );

  // return new death_knight_t( sim, name, r );
  return NULL;
}

// player_t::death_knight_init ==============================================

void player_t::death_knight_init( sim_t* sim )
{
  sim -> auras.abominations_might  = new aura_t( sim, "abominations_might",  1,   0.0 );
  sim -> auras.horn_of_winter      = new aura_t( sim, "horn_of_winter",      1, 120.0 );
  sim -> auras.improved_icy_talons = new aura_t( sim, "improved_icy_talons", 1,   0.0 );

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    p -> buffs.unholy_frenzy = new buff_t( p, "unholy_frenzy", 1, 30.0 );
  }

  target_t* t = sim -> target;
  t -> debuffs.blood_plague       = new debuff_t( sim, "blood_plague",       1, 15.0 );
  t -> debuffs.brittle_bones      = new debuff_t( sim, "brittle_bones",      1, 15.0 );
  t -> debuffs.frost_fever        = new debuff_t( sim, "frost_fever",        1, 15.0 );
  t -> debuffs.ebon_plaguebringer = new debuff_t( sim, "ebon_plaguebringer", 1, 15.0 );
}

// player_t::death_knight_combat_begin ======================================

void player_t::death_knight_combat_begin( sim_t* sim )
{
  if ( sim -> overrides.abominations_might  ) sim -> auras.abominations_might  -> override( 1, 0.10 );
  if ( sim -> overrides.horn_of_winter      )
    sim -> auras.horn_of_winter      -> override( 1, sim -> sim_data.effect_min( 57330, ( sim -> P403 ) ? 85 : 80, E_APPLY_AURA, A_MOD_STAT ) );

  if ( sim -> overrides.improved_icy_talons ) sim -> auras.improved_icy_talons -> override( 1, 0.10 );

  target_t* t = sim -> target;
  if ( sim -> overrides.blood_plague       ) t -> debuffs.blood_plague       -> override();
  if ( sim -> overrides.brittle_bones      ) t -> debuffs.brittle_bones      -> override( 1, 0.04 );
  if ( sim -> overrides.frost_fever        ) t -> debuffs.frost_fever        -> override();
  if ( sim -> overrides.ebon_plaguebringer ) t -> debuffs.ebon_plaguebringer -> override( 1,   8 );
}
