// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Shaman
// ==========================================================================

enum element_type_t { ELEMENT_NONE=0, ELEMENT_AIR, ELEMENT_EARTH, ELEMENT_FIRE, ELEMENT_WATER, ELEMENT_MAX };

enum imbue_type_t { IMBUE_NONE=0, FLAMETONGUE_IMBUE, WINDFURY_IMBUE };

struct shaman_t : public player_t
{
  // Active
  action_t* active_flame_shock;
  action_t* active_lightning_charge;
  action_t* active_lightning_bolt_dot;
  action_t* active_fire_totem;

  // Buffs
  buff_t* buffs_avalanche;
  buff_t* buffs_dueling;
  buff_t* buffs_electrifying_wind;
  buff_t* buffs_elemental_devastation;
  buff_t* buffs_elemental_focus;
  buff_t* buffs_elemental_mastery;
  buff_t* buffs_flurry;
  buff_t* buffs_indomitability;
  buff_t* buffs_lightning_shield;
  buff_t* buffs_maelstrom_weapon;
  buff_t* buffs_nature_vulnerability;
  buff_t* buffs_natures_swiftness;
  buff_t* buffs_quaking_earth;
  buff_t* buffs_shamanistic_rage;
  buff_t* buffs_shattered_ice;
  buff_t* buffs_stonebreaker;
  buff_t* buffs_tier10_2pc_melee;
  buff_t* buffs_tier10_4pc_melee;
  buff_t* buffs_totem_of_wrath_glyph;
  buff_t* buffs_tundra;
  buff_t* buffs_water_shield;

  // Cooldowns
  cooldown_t* cooldowns_elemental_mastery;
  cooldown_t* cooldowns_lava_burst;
  cooldown_t* cooldowns_windfury_weapon;

  // Gains
  gain_t* gains_improved_stormstrike;
  gain_t* gains_shamanistic_rage;
  gain_t* gains_thunderstorm;
  gain_t* gains_water_shield;

  // Procs
  proc_t* procs_lightning_overload;
  proc_t* procs_windfury;

  // Up-Times
  uptime_t* uptimes_tundra;

  // Random Number Generators
  rng_t* rng_improved_stormstrike;
  rng_t* rng_lightning_overload;
  rng_t* rng_static_shock;
  rng_t* rng_windfury_weapon;

  // Weapon Enchants
  attack_t* windfury_weapon_attack;
  spell_t*  flametongue_weapon_spell;

  struct talents_t
  {
    int  ancestral_knowledge;
    int  blessing_of_the_eternals;
    int  booming_echoes;
    int  call_of_flame;
    int  call_of_thunder;
    int  concussion;
    int  convection;
    int  dual_wield;
    int  dual_wield_specialization;
    int  earth_shield;
    int  elemental_devastation;
    int  elemental_focus;
    int  elemental_fury;
    int  elemental_mastery;
    int  elemental_oath;
    int  elemental_precision;
    int  elemental_weapons;
    int  enhancing_totems;
    int  feral_spirit;
    int  flurry;
    int  frozen_power;
    int  improved_fire_nova;
    int  improved_shields;
    int  improved_stormstrike;
    int  improved_windfury_totem;
    int  lava_flows;
    int  lava_lash;
    int  lightning_mastery;
    int  lightning_overload;
    int  maelstrom_weapon;
    int  mana_tide_totem;
    int  mental_dexterity;
    int  mental_quickness;
    int  natures_swiftness;
    int  restorative_totems;
    int  reverberation;
    int  shamanism;
    int  shamanistic_focus;
    int  shamanistic_rage;
    int  spirit_weapons;
    int  static_shock;
    int  stormstrike;
    int  storm_earth_and_fire;
    int  toughness;
    int  thundering_strikes;
    int  thunderstorm;
    int  tidal_mastery;
    int  totem_of_wrath;
    int  totemic_focus;
    int  unrelenting_storm;
    int  unleashed_rage;
    int  weapon_mastery;

    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  struct glyphs_t
  {
    int chain_lightning;
    int elemental_mastery;
    int feral_spirit;
    int fire_elemental_totem;
    int fire_nova;
    int flame_shock;
    int flametongue_weapon;
    int lava;
    int lava_lash;
    int lightning_bolt;
    int lightning_shield;
    int mana_tide;
    int shocking;
    int stormstrike;
    int totem_of_wrath;
    int thunderstorm;
    int windfury_weapon;
    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  struct totems_t
  {
    int dueling;
    int hex;
    int indomitability;
    int dancing_flame;
    int electrifying_wind;
    int quaking_earth;
    int splintering;
    int stonebreaker;
    int thunderfall;
    int avalanche;
    int shattered_ice;
    int tundra;

    totems_t() { memset( ( void* ) this, 0x0, sizeof( totems_t ) ); }
  };
  totems_t totems;

  shaman_t( sim_t* sim, const std::string& name, int race_type = RACE_NONE ) : player_t( sim, SHAMAN, name, race_type )
  {
    // Active
    active_flame_shock        = 0;
    active_lightning_charge   = 0;
    active_lightning_bolt_dot = 0;

    // Cooldowns
    cooldowns_elemental_mastery = get_cooldown( "elemental_mastery" );
    cooldowns_lava_burst        = get_cooldown( "lava_burst"        );
    cooldowns_windfury_weapon   = get_cooldown( "windfury_weapon"   );

    // Weapon Enchants
    windfury_weapon_attack   = 0;
    flametongue_weapon_spell = 0;
  }

  // Character Definition
  virtual void      init_rating();
  virtual void      init_glyphs();
  virtual void      init_race();
  virtual void      init_base();
  virtual void      init_items();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      interrupt();
  virtual double    composite_attack_power() SC_CONST;
  virtual double    composite_attack_power_multiplier() SC_CONST;
  virtual double    composite_spell_power( int school ) SC_CONST;
  virtual std::vector<talent_translation_t>& get_talent_list();
  virtual std::vector<option_t>& get_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name );
  virtual void      create_pets();
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST { return RESOURCE_MANA; }
  virtual int       primary_role() SC_CONST     { return talents.dual_wield ? ROLE_HYBRID : ROLE_SPELL; }
  virtual int       primary_tree() SC_CONST;
  virtual void      combat_begin();

  // Event Tracking
  virtual void regen( double periodicity );
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// ==========================================================================
// Shaman Attack
// ==========================================================================

struct shaman_attack_t : public attack_t
{
  shaman_attack_t( const char* n, player_t* player, int s=SCHOOL_PHYSICAL, int t=TREE_NONE, bool special=true ) :
      attack_t( n, player, RESOURCE_MANA, s, t, special )
  {
    base_dd_min = base_dd_max = 1;
    shaman_t* p = player -> cast_shaman();
    base_multiplier *= 1.0 + p -> talents.weapon_mastery * 0.1/3;
    if ( p -> dual_wield() ) base_hit += p -> talents.dual_wield_specialization * 0.02;
  }

  virtual void execute();
  virtual void player_buff();
  virtual void assess_damage( double amount, int dmg_type );
};

// ==========================================================================
// Shaman Spell
// ==========================================================================

struct shaman_spell_t : public spell_t
{
  double base_cost_reduction;

  shaman_spell_t( const char* n, player_t* p, int s, int t ) :
      spell_t( n, p, RESOURCE_MANA, s, t ), base_cost_reduction( 0 )
  {

  }

  virtual double cost() SC_CONST;
  virtual double cost_reduction() SC_CONST { return base_cost_reduction; }
  virtual void   consume_resource();
  virtual double execute_time() SC_CONST;
  virtual void   execute();
  virtual void   player_buff();
  virtual void   schedule_execute();
  virtual void   assess_damage( double amount, int dmg_type );
  virtual double haste() SC_CONST;
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
      base_dd_min = base_dd_max = 1;
      background = true;
      repeating = true;
      may_crit = true;

      pet_t* p = player -> cast_pet();

      // Orc Command Racial
      if ( p -> owner -> race == RACE_ORC )
      {
        base_multiplier *= 1.05;
      }

      // There are actually two wolves.....
      base_multiplier *= 2.0;
    }
  };

  melee_t* melee;

  spirit_wolf_pet_t( sim_t* sim, player_t* owner ) :
      pet_t( sim, owner, "spirit_wolf" ), melee( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 310;
    main_hand_weapon.max_dmg    = 310;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = 1.5;
  }
  virtual void init_base()
  {
    pet_t::init_base();

    attribute_base[ ATTR_STRENGTH  ] = 331;
    attribute_base[ ATTR_AGILITY   ] = 113;
    attribute_base[ ATTR_STAMINA   ] = 361;
    attribute_base[ ATTR_INTELLECT ] = 65;
    attribute_base[ ATTR_SPIRIT    ] = 109;

    base_attack_power = -20;
    initial_attack_power_per_strength = 2.0;

    melee = new melee_t( this );
  }
  virtual double composite_attack_power() SC_CONST
  {
    shaman_t* o = owner -> cast_shaman();
    double ap = pet_t::composite_attack_power();
    ap += ( o -> glyphs.feral_spirit ? 0.61 : 0.31 ) * o -> composite_attack_power_multiplier() * o -> composite_attack_power();
    return ap;
  }
  virtual void summon( double duration=0 )
  {
    pet_t::summon( duration );
    melee -> execute(); // Kick-off repeating attack
  }
};

// ==========================================================================
// Pet Fire Elemental
// ==========================================================================

struct fire_elemental_pet_t : public pet_t
{
  struct travel_t : public action_t
  {
    travel_t( player_t* player ) : action_t( ACTION_OTHER, "travel", player ) {}
    virtual void execute() { player -> distance = 1; }
    virtual double execute_time() SC_CONST { return ( player -> distance / 10.0 ); }
    virtual bool ready() { return ( player -> distance > 1 ); }
  };

  struct fire_shield_t : public spell_t
  {
    fire_shield_t( player_t* player ) :
      spell_t( "fire_shield", player, RESOURCE_MANA, SCHOOL_FIRE )
    {
      aoe = true;
      background = true;
      repeating = true;
      may_crit = true;
      base_execute_time = 3.0;
      base_dd_min = base_dd_max = 96;
      direct_power_mod = 0.015;

      id = 12470;
    };
    virtual double total_multiplier() SC_CONST { return ( player -> distance > 1 ) ? 0.0 : spell_t::total_multiplier(); }
  };

  struct fire_nova_t : public spell_t
  {
    fire_nova_t( player_t* player ) :
      spell_t( "fire_nova", player, RESOURCE_MANA, SCHOOL_FIRE )
    {
      aoe = true;
      may_crit = true;
      base_cost = 207;
      base_execute_time = 2.5; // Really 2.0sec, but there appears to be a 0.5sec lag following.
      base_dd_min = 960;
      base_dd_max = 1000;
      direct_power_mod = 0.50;
    };
  };

  struct fire_blast_t : public spell_t
  {
    fire_blast_t( player_t* player ) :
      spell_t( "fire_blast", player, RESOURCE_MANA, SCHOOL_FIRE )
    {
      may_crit = true;
      base_cost = 276;
      base_execute_time = 0;
      base_dd_min = 750;
      base_dd_max = 800;
      direct_power_mod = 0.20;
      cooldown -> duration = 4.0;

      id = 57984;
    };
  };

  struct fire_melee_t : public attack_t
  {
    fire_melee_t( player_t* player ) :
      attack_t( "fire_melee", player, RESOURCE_NONE, SCHOOL_FIRE )
    {
      may_crit = true;
      base_dd_min = base_dd_max = 180;
      base_execute_time = 2.0;
      direct_power_mod = 1.0;
      base_spell_power_multiplier = 0.60;
      base_attack_power_multiplier = base_execute_time / 14;
    }
  };

  spell_t* fire_shield;

  fire_elemental_pet_t( sim_t* sim, player_t* owner ) : pet_t( sim, owner, "fire_elemental", true /*GUARDIAN*/ ) {}

  virtual void init_base()
  {
    pet_t::init_base();

    resource_base[ RESOURCE_HEALTH ] = 0; // FIXME!!
    resource_base[ RESOURCE_MANA   ] = 4000;

    health_per_stamina = 10;
    mana_per_intellect = 15;

    // Modeling melee as a foreground action since a loose model is Nova-Blast-Melee-repeat.
    // The actual actions are not really so deterministic, but if you look at the entire spawn time,
    // you will see that there is a 1-to-1-to-1 distribution (provided there is sufficient mana).

    action_list_str = "travel/sequence,name=attack:fire_nova:fire_blast:fire_melee/restart_sequence,name=attack,moving=0";

    fire_shield = new fire_shield_t( this );
  }

  virtual int primary_resource() SC_CONST { return RESOURCE_MANA; }

  virtual void regen( double periodicity )
  {
    if ( ! recent_cast() )
    {
      resource_gain( RESOURCE_MANA, 10.66 * periodicity ); // FIXME! Does regen scale with gear???
    }
  }

  virtual void summon( double duration=0 )
  {
    pet_t::summon();
    fire_shield -> execute();
  }

  virtual double composite_attack_power() SC_CONST
  {
    return owner -> composite_attack_power_multiplier() * owner -> composite_attack_power();
  }

  virtual double composite_spell_power( int school ) SC_CONST
  {
    return owner -> composite_spell_power_multiplier() * owner -> composite_spell_power( school );
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "travel"      ) return new travel_t     ( this );
    if ( name == "fire_nova"   ) return new fire_nova_t  ( this );
    if ( name == "fire_blast"  ) return new fire_blast_t ( this );
    if ( name == "fire_melee"  ) return new fire_melee_t ( this );

    return pet_t::create_action( name, options_str );
  }
};

// trigger_flametongue_weapon ===============================================

static void trigger_flametongue_weapon( attack_t* a )
{
  struct flametongue_weapon_spell_t : public shaman_spell_t
  {
    flametongue_weapon_spell_t( player_t* player ) :
        shaman_spell_t( "flametongue", player, SCHOOL_FIRE, TREE_ENHANCEMENT )
    {
      shaman_t* p = player -> cast_shaman();

      background   = true;
      proc         = true;
      may_crit     = true;
      trigger_gcd  = 0;
      base_hit    += p -> talents.elemental_precision * 0.01;

      reset();
    }
  };

  if ( a -> weapon &&
       a -> weapon -> buff_type == FLAMETONGUE_IMBUE )
  {
    shaman_t* p = a -> player -> cast_shaman();

    if ( ! p -> flametongue_weapon_spell )
    {
      p -> flametongue_weapon_spell = new flametongue_weapon_spell_t( p );
    }

    double fire_dmg = util_t::ability_rank( p -> level,  70.0,80,  70.0,77,  60.0,72,  35.0,65,  30.0,0 );

    fire_dmg *= a -> weapon -> swing_time;

    p -> flametongue_weapon_spell -> base_dd_min = fire_dmg;
    p -> flametongue_weapon_spell -> base_dd_max = fire_dmg;
    p -> flametongue_weapon_spell -> direct_power_mod = 0.03811 * a -> weapon -> swing_time;
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
      may_crit    = true;
      background  = true;
      trigger_gcd = 0;
      base_multiplier *= 1.0 + p -> talents.elemental_weapons * 0.133333;
      if ( p -> totems.splintering ) base_attack_power += 212;
      reset();
    }
    virtual void player_buff()
    {
      shaman_attack_t::player_buff();
      player_attack_power += weapon -> buff_value;
    }
  };

  shaman_t* p = a -> player -> cast_shaman();

  if ( a -> proc ) return;
  if ( a -> weapon == 0 ) return;
  if ( a -> weapon -> buff_type != WINDFURY_IMBUE ) return;

  if ( p -> cooldowns_windfury_weapon -> remains() > 0 ) return;

  if ( p -> rng_windfury_weapon -> roll( 0.20 + ( p -> glyphs.windfury_weapon ? 0.02 : 0 ) ) )
  {
    if ( ! p -> windfury_weapon_attack )
    {
      p -> windfury_weapon_attack = new windfury_weapon_attack_t( p );
    }
    p -> cooldowns_windfury_weapon -> start( 3.0 );

    p -> procs_windfury -> occur();
    p -> windfury_weapon_attack -> weapon = a -> weapon;
    p -> windfury_weapon_attack -> execute();
    p -> windfury_weapon_attack -> execute();
  }
}

// stack_maelstrom_weapon ==================================================

static void stack_maelstrom_weapon( attack_t* a )
{
  shaman_t* p = a -> player -> cast_shaman();

  if ( p -> talents.maelstrom_weapon == 0 ) return;

  double chance = a -> weapon -> proc_chance_on_swing( p -> talents.maelstrom_weapon * 2.0 );
  bool can_increase = p -> buffs_maelstrom_weapon -> check() <  p -> buffs_maelstrom_weapon -> max_stack;

  if ( p -> set_bonus.tier8_4pc_melee() ) chance *= 1.20;

  p -> buffs_maelstrom_weapon -> trigger( 1, 1, chance );

  if ( p -> set_bonus.tier10_4pc_melee() &&
       ( can_increase && p -> buffs_maelstrom_weapon -> stack() == p -> buffs_maelstrom_weapon -> max_stack ) )
    p -> buffs_tier10_4pc_melee -> trigger();
}

// trigger_improved_stormstrike =============================================

static void trigger_improved_stormstrike( attack_t* a )
{
  shaman_t* p = a -> player -> cast_shaman();

  if ( p -> rng_improved_stormstrike -> roll( p -> talents.improved_stormstrike / 2.0 ) )
  {
    p -> resource_gain( RESOURCE_MANA, 0.20 * p -> resource_base[ RESOURCE_MANA ], p -> gains_improved_stormstrike );
  }
}

// trigger_tier8_4pc_elemental ===============================================

static void trigger_tier8_4pc_elemental( spell_t* s )
{
  shaman_t* p = s -> player -> cast_shaman();

  if ( ! p -> set_bonus.tier8_4pc_caster() ) return;

  struct lightning_bolt_dot_t : public shaman_spell_t
  {
    lightning_bolt_dot_t( player_t* player ) : shaman_spell_t( "tier8_4pc_elemental", player, SCHOOL_NATURE, TREE_ELEMENTAL )
    {
      may_miss        = false;
      background      = true;
      proc            = true;
      trigger_gcd     = 0;
      base_cost       = 0;
      base_multiplier = 1.0;
      tick_power_mod  = 0;
      base_tick_time  = 2.0;
      num_ticks       = 2;
    }
    void player_buff() {}
    void target_debuff( int dmg_type ) {}
  };

  double dmg = s -> direct_dmg * 0.08;

  if ( ! p -> active_lightning_bolt_dot ) p -> active_lightning_bolt_dot = new lightning_bolt_dot_t( p );

  if ( p -> active_lightning_bolt_dot -> ticking )
  {
    int num_ticks = p -> active_lightning_bolt_dot -> num_ticks;
    int remaining_ticks = num_ticks - p -> active_lightning_bolt_dot -> current_tick;

    dmg += p -> active_lightning_bolt_dot -> base_td * remaining_ticks;

    p -> active_lightning_bolt_dot -> cancel();
  }

  p -> active_lightning_bolt_dot -> base_td = dmg / p -> active_lightning_bolt_dot -> num_ticks;
  p -> active_lightning_bolt_dot -> schedule_tick();
}

// trigger_lightning_overload ===============================================

static void trigger_lightning_overload( spell_t* s,
                                        stats_t* lightning_overload_stats,
                                        double   lightning_overload_chance )
{
  shaman_t* p = s -> player -> cast_shaman();

  if ( lightning_overload_chance == 0 ) return;

  if ( s -> proc ) return;

  if ( p -> rng_lightning_overload -> roll( lightning_overload_chance ) )
  {
    p -> procs_lightning_overload -> occur();

    double   cost             = s -> base_cost;
    double   multiplier       = s -> base_multiplier;
    double   direct_power_mod = s -> direct_power_mod;
    stats_t* stats            = s -> stats;

    s -> proc                 = true;
    s -> pseudo_pet           = true; // Prevent Honor Among Thieves
    s -> base_cost            = 0;
    s -> base_multiplier     /= 2.0;
    s -> direct_power_mod    += p -> talents.shamanism * 0.04; // Reapplied here because Shamanism isn't affected by the *0.5.
    s -> stats                = lightning_overload_stats;

    s -> time_to_execute      = 0;
    s -> execute();

    s -> proc                 = false;
    s -> pseudo_pet           = false;
    s -> base_cost            = cost;
    s -> base_multiplier      = multiplier;
    s -> direct_power_mod     = direct_power_mod;
    s -> stats                = stats;
  }
}

// =========================================================================
// Shaman Attack
// =========================================================================

// shaman_attack_t::execute ================================================

void shaman_attack_t::execute()
{
  shaman_t* p = player -> cast_shaman();

  attack_t::execute();

  if ( result_is_hit() )
  {
    if ( result == RESULT_CRIT )
    {
      p -> buffs_flurry -> trigger( 3 );
    }
    if ( p -> buffs_shamanistic_rage -> up() )
    {
      double mana = player_attack_power * 0.15;
      p -> resource_gain( RESOURCE_MANA, mana, p -> gains_shamanistic_rage );
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
  if ( p -> buffs_elemental_devastation -> up() )
  {
    player_crit += p -> talents.elemental_devastation * 0.03;
  }

  if ( p -> buffs_tier10_2pc_melee -> up() )
    player_multiplier *= 1.12;
}

// shaman_attack_t::assess_damage ==========================================

void shaman_attack_t::assess_damage( double amount,
                                     int    dmg_type )
{
  shaman_t* p = player -> cast_shaman();

  attack_t::assess_damage( amount, dmg_type );

  if ( num_ticks == 0 && p -> talents.static_shock && p -> buffs_lightning_shield -> stack() )
  {
    double chance = p -> talents.static_shock * 0.02 + p -> set_bonus.tier9_2pc_melee() * 0.03;
    if ( p -> rng_static_shock -> roll( chance ) )
    {
      p -> buffs_lightning_shield -> decrement();
      p -> active_lightning_charge -> execute();
    }
  }
}

// Melee Attack ============================================================

struct melee_t : public shaman_attack_t
{
  int sync_weapons;

  melee_t( const char* name, player_t* player, int sw ) :
    shaman_attack_t( name, player, SCHOOL_PHYSICAL, TREE_NONE, false ), sync_weapons( sw )
  {
    shaman_t* p = player -> cast_shaman();

    may_crit    = true;
    background  = true;
    repeating   = true;
    trigger_gcd = 0;
    base_cost   = 0;

    if ( p -> dual_wield() ) base_hit -= 0.19;
  }

  virtual double haste() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();
    double h = shaman_attack_t::haste();
    if ( p -> buffs_flurry -> up() )
    {
      h *= 1.0 / ( 1.0 + 0.05 * ( p -> talents.flurry + 1 + p -> set_bonus.tier7_4pc_melee() ) );
    }
    return h;
  }

  virtual double execute_time() SC_CONST
  {
    double t = shaman_attack_t::execute_time();
    if ( ! player -> in_combat )
    {
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t/2, 0.2 ) : t/2 ) : 0.01;
    }
    return t;
  }

  void execute()
  {
    shaman_t* p = player -> cast_shaman();

    if ( time_to_execute > 0 && p -> executing )      
    {
      if ( sim -> debug ) log_t::output( sim, "Executing '%s' during melee (%s).", p -> executing -> name(), util_t::slot_type_string( weapon -> slot ) );
      schedule_execute();
    }
    else
    {
      shaman_attack_t::execute();
    }
  }

  void schedule_execute()
  {
    shaman_attack_t::schedule_execute();
    shaman_t* p = player -> cast_shaman();
    p -> buffs_flurry -> decrement();
  }
};

// Auto Attack =============================================================

struct auto_attack_t : public shaman_attack_t
{
  int sync_weapons;

  auto_attack_t( player_t* player, const std::string& options_str ) :
      shaman_attack_t( "auto_attack", player ),
      sync_weapons( 0 )
  {
    shaman_t* p = player -> cast_shaman();

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
      check_talent( p -> talents.dual_wield );
      p -> off_hand_attack = new melee_t( "melee_off_hand", player, sync_weapons );
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
    }

    trigger_gcd = 0;
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    p -> main_hand_attack -> schedule_execute();
    if ( p -> off_hand_attack )
    {
      p -> off_hand_attack -> schedule_execute();
    }
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();
    if ( p -> buffs.moving -> check() ) return false;
    return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Lava Lash Attack =========================================================

struct lava_lash_t : public shaman_attack_t
{
  int wf_cd_only;

  lava_lash_t( player_t* player, const std::string& options_str ) :
      shaman_attack_t( "lava_lash", player, SCHOOL_FIRE, TREE_ENHANCEMENT ), wf_cd_only( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "wf_cd_only", OPT_INT, &wf_cd_only  },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    weapon = &( player -> off_hand_weapon );
    if ( weapon -> type == WEAPON_NONE ) background = true; // Do not allow execution.

    base_dd_min = base_dd_max = 1;
    may_crit    = true;
    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.04;
    cooldown -> duration = 6;
    if ( p -> set_bonus.tier8_2pc_melee() ) base_multiplier *= 1.0 + 0.20;

    id = 60103;
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_attack_t::execute();
    p -> buffs_indomitability -> trigger();
    p -> buffs_quaking_earth -> trigger();
  }

  virtual void player_buff()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_attack_t::player_buff();
    if ( weapon -> buff_type == FLAMETONGUE_IMBUE )
    {
      player_multiplier *= 1.25 + p -> glyphs.lava_lash * 0.10;
    }
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();

    if ( wf_cd_only )
      if ( p -> cooldowns_windfury_weapon -> remains() > 0 )
        return false;

    return shaman_attack_t::ready();
  }
};

// Stormstrike Attack =======================================================

struct stormstrike_t : public shaman_attack_t
{
  stormstrike_t( player_t* player, const std::string& options_str ) :
      shaman_attack_t( "stormstrike", player, SCHOOL_PHYSICAL, TREE_ENHANCEMENT )
  {
    shaman_t* p = player -> cast_shaman();

    may_crit  = true;
    base_cost = p -> resource_base[ RESOURCE_MANA ] * 0.08;
    cooldown -> duration = 8.0;

    if ( p -> set_bonus.tier8_2pc_melee() ) base_multiplier *= 1.0 + 0.20;

    if ( p -> totems.dancing_flame )
    {
      base_dd_max += 155;
      base_dd_min += 155;
    }

    id = 17364;
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();

    attack_t::consume_resource();
    update_ready();

    weapon = &( p -> main_hand_weapon );
    shaman_attack_t::execute();

    if ( result_is_hit() )
    {
      p -> buffs_nature_vulnerability -> trigger( 4 );

      if ( p -> off_hand_weapon.type != WEAPON_NONE )
      {
        weapon = &( p -> off_hand_weapon );
        shaman_attack_t::execute();
      }
    }

    trigger_improved_stormstrike( this );
    p -> buffs_dueling -> trigger();
    p -> buffs_avalanche -> trigger();
  }

  virtual void consume_resource() {}
};

// =========================================================================
// Shaman Spell
// =========================================================================

// shaman_spell_t::haste ====================================================

double shaman_spell_t::haste() SC_CONST
{
  shaman_t* p = player -> cast_shaman();
  double h = spell_t::haste();
  if ( p -> buffs_elemental_mastery -> up() )
  {
    h *= 1.0 / 1.15;
  }
  return h;
}


// shaman_spell_t::cost ====================================================

double shaman_spell_t::cost() SC_CONST
{
  shaman_t* p = player -> cast_shaman();
  double c = spell_t::cost();
  if ( c == 0 ) return 0;
  double cr = cost_reduction();
  if ( p -> buffs_elemental_focus -> check() ) cr += 0.40;
  c *= 1.0 - cr;
  if ( c < 0 ) c = 0;
  return c;
}

// shaman_spell_t::consume_resource ========================================

void shaman_spell_t::consume_resource()
{
  spell_t::consume_resource();
  shaman_t* p = player -> cast_shaman();
  if ( resource_consumed > 0 && p -> buffs_elemental_focus -> up() )
  {
    p -> buffs_elemental_focus -> decrement();
  }
}

// shaman_spell_t::execute_time ============================================

double shaman_spell_t::execute_time() SC_CONST
{
  shaman_t* p = player -> cast_shaman();
  if ( p -> buffs_natures_swiftness -> up() ) return 0;
  return spell_t::execute_time();
}

// shaman_spell_t::player_buff =============================================

void shaman_spell_t::player_buff()
{
  shaman_t* p = player -> cast_shaman();
  spell_t::player_buff();
  if ( p -> buffs_nature_vulnerability -> up() && school == SCHOOL_NATURE )
  {
    player_multiplier *= 1.0 + ( p -> glyphs.stormstrike ? 0.28 : 0.20 );
  }
  if ( p -> glyphs.flametongue_weapon )
  {
    if ( p -> main_hand_weapon.buff_type == FLAMETONGUE_IMBUE ||
         p ->  off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
    {
      player_crit += 0.02;
    }
  }
  if ( sim -> auras.elemental_oath -> up() && p -> buffs_elemental_focus -> up() )
  {
    player_multiplier *= 1.0 + p -> talents.elemental_oath * 0.05;
  }
  if ( p -> buffs_tier10_2pc_melee -> up() )
    player_multiplier *= 1.12;
}

// shaman_spell_t::execute =================================================

void shaman_spell_t::execute()
{
  shaman_t* p = player -> cast_shaman();
  spell_t::execute();
  if ( result_is_hit() )
  {
    if ( result == RESULT_CRIT )
    {
      if ( ! proc && ! pseudo_pet )
      {
        p -> buffs_elemental_devastation -> trigger();
        p -> buffs_elemental_focus -> trigger( 2 );
      }
    }
  }
}

// shaman_spell_t::schedule_execute ========================================

void shaman_spell_t::schedule_execute()
{
  /* shaman_t* p = player -> cast_shaman(); */

  spell_t::schedule_execute();

  /*if ( time_to_execute > 0 && ! p -> buffs_maelstrom_weapon -> check() )
  {
    if ( p -> main_hand_attack ) p -> main_hand_attack -> cancel();
    if ( p ->  off_hand_attack ) p ->  off_hand_attack -> cancel();
  }*/
}

// shaman_spell_t::assess_damage ============================================

void shaman_spell_t::assess_damage( double amount,
                                    int    dmg_type )
{
  shaman_t* p = player -> cast_shaman();

  spell_t::assess_damage( amount, dmg_type );

  if ( dmg_type == DMG_DIRECT && school == SCHOOL_NATURE )
  {
    p -> buffs_nature_vulnerability -> decrement();
  }
}

// =========================================================================
// Shaman Spells
// =========================================================================

// Chain Lightning Spell ===================================================

struct chain_lightning_t : public shaman_spell_t
{
  int      clearcasting;
  int      conserve;
  double   max_lvb_cd;
  int      maelstrom;
  stats_t* lightning_overload_stats;
  double   lightning_overload_chance;

  chain_lightning_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "chain_lightning", player, SCHOOL_NATURE, TREE_ELEMENTAL ),
      clearcasting( 0 ), conserve( 0 ), max_lvb_cd( 0 ), maelstrom( 0 ), lightning_overload_stats( 0 ), lightning_overload_chance( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "clearcasting", OPT_INT, &clearcasting },
      { "conserve", OPT_INT, &conserve    },
      { "lvb_cd<",   OPT_FLT, &max_lvb_cd },
      { "maelstrom", OPT_INT, &maelstrom  },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 8, 973, 1111, 0, 0.26 },
      { 74, 7, 806,  920, 0, 0.26 },
      { 70, 6, 734,  838, 0,  760 },
      { 63, 5, 603,  687, 0,  650 },
      { 56, 4, 493,  551, 0,  550 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 49271 );

    base_execute_time    = 2.0;
    may_crit             = true;
    direct_power_mod     = ( base_execute_time / 3.5 );
    direct_power_mod    += p -> talents.shamanism * 0.04;
    base_execute_time   -= p -> talents.lightning_mastery * 0.1;
    base_cost_reduction += p -> talents.convection * 0.02;
    base_multiplier     *= 1.0 + p -> talents.concussion * 0.01;
    base_hit            += p -> talents.elemental_precision * 0.01;
    base_crit           += p -> talents.call_of_thunder * 0.05;
    base_crit           += p -> talents.tidal_mastery * 0.01;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.elemental_fury * 0.20;

    cooldown -> duration  = 6.0;
    cooldown -> duration -= util_t::talent_rank( p -> talents.storm_earth_and_fire, 3, 0.75, 1.5, 2.5 );

    if ( p -> totems.hex ) base_spell_power += 165;

    lightning_overload_stats = p -> get_stats( "lightning_overload" );
    lightning_overload_stats -> school = SCHOOL_NATURE;

    lightning_overload_chance = util_t::talent_rank( p -> talents.lightning_overload, 3, 0.11, 0.22, 0.33 ) / 3.0;
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_spell_t::execute();
    p -> buffs_maelstrom_weapon -> expire();
    p -> buffs_elemental_mastery -> current_value = 0;
    if ( p -> set_bonus.tier10_2pc_caster() )
    {
      p -> cooldowns_elemental_mastery -> ready -= 2.0;
    }
    if ( result_is_hit() )
    {
      trigger_lightning_overload( this, lightning_overload_stats, lightning_overload_chance );
    }
  }

  virtual double execute_time() SC_CONST
  {
    double t = shaman_spell_t::execute_time();
    shaman_t* p = player -> cast_shaman();
    if ( p -> buffs_elemental_mastery -> value() ) return 0;
    if ( p -> buffs_maelstrom_weapon -> stack() == 5 ) return 0;
    t *= 1.0 - p -> buffs_maelstrom_weapon -> stack() * 0.20;
    return t;
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();

    if ( clearcasting > 0 )
      if ( ! p -> buffs_elemental_focus -> check() )
	    return false;

    if ( conserve > 0 )
    {
      double mana_pct = 100.0 * p -> resource_current[ RESOURCE_MANA ] / p -> resource_max [ RESOURCE_MANA ];
      if ( ( mana_pct - 5.0 ) < sim -> target -> health_percentage() )
	    return false;
    }

    if ( maelstrom > 0 )
      if ( maelstrom > p -> buffs_maelstrom_weapon -> current_stack )
	    return false;

    if ( max_lvb_cd > 0  )
      if ( p -> cooldowns_lava_burst -> remains() > ( max_lvb_cd * haste() ) )
	    return false;

    return shaman_spell_t::ready();
  }

  virtual void assess_damage( double amount,
                               int    dmg_type )
  {
    shaman_spell_t::assess_damage( amount, dmg_type );
    shaman_t* p = player -> cast_shaman();

    for ( int i=0; i < sim -> target -> adds_nearby && i < ( 2 + p -> glyphs.chain_lightning ); i ++ )
    {
      amount *= 0.70;
      shaman_spell_t::additional_damage( amount, dmg_type );
    }
  }
};

// Lightning Bolt Spell =====================================================

struct lightning_bolt_t : public shaman_spell_t
{
  int      maelstrom;
  int      ss_wait;
  stats_t* lightning_overload_stats;
  double   lightning_overload_chance;

  lightning_bolt_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "lightning_bolt", player, SCHOOL_NATURE, TREE_ELEMENTAL ),
      maelstrom( 0 ), ss_wait( 0 ), lightning_overload_stats( 0 ), lightning_overload_chance( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "maelstrom", OPT_INT,  &maelstrom  },
      { "ss_wait",   OPT_BOOL, &ss_wait    },
      { NULL, OPT_UNKNOWN, NULL }
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
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 49238 );

    base_execute_time    = 2.5;
    may_crit             = true;
    direct_power_mod     = ( base_execute_time / 3.5 );
    base_execute_time   -= p -> talents.lightning_mastery * 0.1;
    base_cost_reduction += p -> talents.convection * 0.02;
    base_multiplier     *= 1.0 + p -> talents.concussion * 0.01 + ( p -> glyphs.lightning_bolt != 0 ? 0.04 : 0.0 );
    base_hit            += p -> talents.elemental_precision * 0.01;
    base_crit           += p -> talents.call_of_thunder * 0.05;
    base_crit           += p -> talents.tidal_mastery * 0.01;
    direct_power_mod    += p -> talents.shamanism * 0.04;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.elemental_fury * 0.20;

    if ( p -> set_bonus.tier6_4pc_caster() ) base_multiplier     *= 1.05;
    if ( p -> set_bonus.tier7_2pc_caster() ) base_cost_reduction += 0.05;

    if ( p -> totems.hex             ) base_spell_power += 165;

    lightning_overload_stats = p -> get_stats( "lightning_overload" );
    lightning_overload_stats -> school = SCHOOL_NATURE;

    lightning_overload_chance = util_t::talent_rank( p -> talents.lightning_overload, 3, 0.11, 0.22, 0.33 );
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_spell_t::execute();
    p -> buffs_maelstrom_weapon -> expire();
    p -> buffs_elemental_mastery -> current_value = 0;
    if ( p -> set_bonus.tier10_2pc_caster() )
    {
      p -> cooldowns_elemental_mastery -> ready -= 2.0;
    }
    if ( result_is_hit() )
    {
      trigger_lightning_overload( this, lightning_overload_stats, lightning_overload_chance );
      if ( result == RESULT_CRIT )
      {
        trigger_tier8_4pc_elemental( this );
      }
    }
    p -> buffs_electrifying_wind -> trigger();
  }

  virtual double execute_time() SC_CONST
  {
    double t = shaman_spell_t::execute_time();
    shaman_t* p = player -> cast_shaman();
    if ( p -> buffs_elemental_mastery -> value() ) return 0;
    if ( p -> buffs_maelstrom_weapon -> stack() == 5 ) return 0;
    t *= 1.0 - p -> buffs_maelstrom_weapon -> stack() * 0.20;
    return t;
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();

    if ( ! shaman_spell_t::ready() )
      return false;

    if ( maelstrom > 0 )
      if( maelstrom > p -> buffs_maelstrom_weapon -> current_stack )
        return false;

    if ( ss_wait && ! p -> buffs_nature_vulnerability -> check() )
      return false;

    return true;
  }
};

// Lava Burst Spell =========================================================

struct lava_burst_t : public shaman_spell_t
{
  int maelstrom;
  int flame_shock;

  lava_burst_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "lava_burst", player, SCHOOL_FIRE, TREE_ELEMENTAL ),
      maelstrom( 0 ), flame_shock( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "maelstrom",          OPT_INT,  &maelstrom          },
      { "flame_shock",        OPT_BOOL, &flame_shock        },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 2, 1192, 1518, 0, 0.10 },
      { 75, 1, 1012, 1290, 0, 0.10 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 60043 );

    may_crit             = true;
    base_execute_time    = 2.0;
    direct_power_mod     = base_execute_time / 3.5;
    direct_power_mod    += p -> glyphs.lava ? 0.10 : 0.00;

    base_cost_reduction += p -> talents.convection * 0.02;
    base_execute_time   -= p -> talents.lightning_mastery * 0.1;
    base_multiplier     *= 1.0 + p -> talents.concussion * 0.01 + p -> talents.call_of_flame * 0.02;
    base_hit            += p -> talents.elemental_precision * 0.01;
    direct_power_mod    += p -> talents.shamanism * 0.05;

    base_crit_bonus_multiplier *= 1.0 + ( util_t::talent_rank( p -> talents.lava_flows,     3, 0.06, 0.12, 0.24 ) +
                                          util_t::talent_rank( p -> talents.elemental_fury, 5, 0.20 ) +
                                          ( p -> set_bonus.tier7_4pc_caster() ? 0.10 : 0.00 ) );

    cooldown -> duration = 8.0;

    if ( p -> set_bonus.tier9_4pc_caster() )
    {
      num_ticks      = 3;
      base_tick_time = 2.0;
      tick_may_crit  = false;
      tick_power_mod = 0.0;
    }

    if ( p -> totems.thunderfall )
    {
      base_dd_min += 215;
      base_dd_max += 215;
    }
  }

  virtual double total_td_multiplier() SC_CONST
  {
    return 1.0; // Don't double-dip with tier9_4pc
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_spell_t::execute();
    p -> buffs_elemental_mastery -> current_value = 0;

    if ( result_is_hit() )
    {
      if ( p -> set_bonus.tier9_4pc_caster() )
        base_td = direct_dmg * 0.1 / num_ticks;

      if ( p -> set_bonus.tier10_4pc_caster() && p -> active_flame_shock )
        p -> active_flame_shock -> extend_duration( 2 );
    }
  }

  virtual double execute_time() SC_CONST
  {
    double t = shaman_spell_t::execute_time();
    shaman_t* p = player -> cast_shaman();
    if ( p -> buffs_elemental_mastery -> value() ) return 0;
    return t;
  }

  virtual void player_buff()
  {
    shaman_spell_t::player_buff();
    shaman_t* p = player -> cast_shaman();
    if ( p -> active_flame_shock ) player_crit += 1.0;
  }

  virtual bool ready()
  {
    if ( ! shaman_spell_t::ready() )
      return false;

    if ( flame_shock )
    {
      shaman_t* p = player -> cast_shaman();

      if ( ! p -> active_flame_shock )
        return false;

      double lvb_finish = sim -> current_time + execute_time();
      double fs_finish  = p -> active_flame_shock -> dot -> ready;

      if ( lvb_finish > fs_finish )
        return false;
    }

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
    check_talent( p -> talents.elemental_mastery );
    trigger_gcd = 0;
    cooldown -> duration = p -> glyphs.elemental_mastery ? 150.0 : 180.0;

    id = 16166;
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_elemental_mastery -> trigger();
    update_ready();
  }
};

// Natures Swiftness Spell ==================================================

struct shamans_swiftness_t : public shaman_spell_t
{
  cooldown_t* sub_cooldown;
  dot_t*      sub_dot;

  shamans_swiftness_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "natures_swiftness", player, SCHOOL_NATURE, TREE_RESTORATION ),
    sub_cooldown(0), sub_dot(0)
  {
    shaman_t* p = player -> cast_shaman();
    check_talent( p -> talents.natures_swiftness );
    trigger_gcd = 0;
    cooldown -> duration = 120.0;
    if ( ! options_str.empty() )
    {
      // This will prevent Natures Swiftness from being called before the desired "fast spell" is ready to be cast.
      sub_cooldown = p -> get_cooldown( options_str );
      sub_dot      = p -> get_dot     ( options_str );
    }

    id = 16188;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs natures_swiftness", player -> name() );
    shaman_t* p = player -> cast_shaman();
    p -> buffs_natures_swiftness -> trigger();
    update_ready();
  }

  virtual bool ready()
  {
    if ( sub_cooldown )
      if ( sub_cooldown -> remains() > 0 )
	return false;

    if ( sub_dot )
      if ( sub_dot -> remains() > 0 )
	return false;

    return shaman_spell_t::ready();
  }
};

// Earth Shock Spell =======================================================

struct earth_shock_t : public shaman_spell_t
{
  int ss_wait;

  earth_shock_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "earth_shock", player, SCHOOL_NATURE, TREE_ELEMENTAL ), ss_wait( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "ss_wait", OPT_BOOL, &ss_wait },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 79, 10, 849, 895, 0, 0.18 },
      { 74,  9, 723, 761, 0, 0.18 },
      { 69,  8, 658, 692, 0, 535  },
      { 60,  7, 517, 545, 0, 450  },
      { 48,  6, 359, 381, 0, 345  },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 49231 );

    may_crit          = true;
    base_execute_time = 0;
    direct_power_mod  = 0.41;
    base_multiplier  *= 1.0 + p -> talents.concussion * 0.01 + p -> set_bonus.tier9_4pc_melee() * 0.25;
    base_hit         += p -> talents.elemental_precision * 0.01;

    base_cost_reduction  += ( p -> talents.convection        * 0.02 +
                              p -> talents.mental_quickness  * 0.02 +
                              p -> talents.shamanistic_focus * 0.45 );

    base_crit_bonus_multiplier *= 1.0 + p -> talents.elemental_fury * 0.20;

    cooldown = p -> get_cooldown( "shock" );
    cooldown -> duration  = 6.0;
    cooldown -> duration -= p -> talents.reverberation  * 0.2;

    if ( p -> glyphs.shocking )
    {
      trigger_gcd = 1.0;
      min_gcd     = 1.0;
    }
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_spell_t::execute();
    p -> buffs_stonebreaker -> trigger();
    p -> buffs_tundra       -> trigger();
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();

    if ( ! shaman_spell_t::ready() )
      return false;

    if ( ss_wait && ! p -> buffs_nature_vulnerability -> check() )
      return false;

    return true;
  }
};

// Fire Nova Spell =======================================================

struct fire_nova_t : public shaman_spell_t
{
  fire_nova_t( player_t* player, const std::string& options_str ) :
               shaman_spell_t( "fire_nova", player, SCHOOL_FIRE, TREE_ELEMENTAL )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 9, 893, 997, 0, 0.22 },
      { 75, 8, 755, 843, 0, 0.22 },
      { 70, 7, 727, 813, 0, 0.22 },
      { 61, 6, 518, 578, 0, 0.22 },
      { 52, 5, 396, 442, 0, 0.22 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 61657 );

    aoe = true;
    pseudo_pet = true;
    may_crit = true;
    base_execute_time = 0;
    direct_power_mod  = 0.8 / 3.5;
    cooldown -> duration = 10.0;

    base_multiplier *= 1.0 + ( p -> talents.improved_fire_nova * 0.10 +
			       p -> talents.call_of_flame      * 0.05 );

    base_hit += p -> talents.elemental_precision * 0.01;

    base_crit_bonus_multiplier *= 1.0 + p -> talents.elemental_fury * 0.20;

    if ( p -> glyphs.fire_nova ) cooldown -> duration -= 3.0;
    cooldown -> duration -= p -> talents.improved_fire_nova * 2.0;
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();

    if ( ! p -> active_fire_totem )
      return false;

    return shaman_spell_t::ready();
  }

  // Fire Nova doesn't benefit from Elemental Focus
  double cost() SC_CONST
  {
    double c = spell_t::cost();
    if ( c == 0 ) return 0;
    double cr = cost_reduction();
    c *= 1.0 - cr;
    if ( c < 0 ) c = 0;
    return c;
  }

  // Fire Nova doesn't consume Elemental Focus charges.
  void consume_resource()
  {
    spell_t::consume_resource();
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
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 78, 7, 802, 848, 0, 0.18 },
      { 73, 6, 681, 719, 0, 0.18 },
      { 68, 5, 640, 676, 0, 525  },
      { 58, 4, 486, 514, 0, 430  },
      { 46, 3, 333, 353, 0, 325  },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 49236 );

    may_crit = true;
    base_execute_time = 0;
    direct_power_mod  = 0.41;

    base_multiplier  *= 1.0 + ( p -> talents.concussion          * 0.01 +
                                p -> talents.booming_echoes      * 0.10 +
                                p -> set_bonus.tier9_4pc_melee() * 0.25 );

    base_hit += p -> talents.elemental_precision * 0.01;

    base_cost_reduction  += ( p -> talents.convection        * 0.02 +
                              p -> talents.mental_quickness  * 0.02 +
                              p -> talents.shamanistic_focus * 0.45 );

    base_crit_bonus_multiplier *= 1.0 + p -> talents.elemental_fury * 0.20;

    cooldown = p -> get_cooldown( "shock" );
    cooldown -> duration  = 6.0;
    cooldown -> duration -= ( p -> talents.reverberation  * 0.2 +
			      p -> talents.booming_echoes * 1.0 );

    if ( p -> glyphs.shocking )
    {
      trigger_gcd = 1.0;
      min_gcd     = 1.0;
    }
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_spell_t::execute();
    p -> buffs_stonebreaker -> trigger();
    p -> buffs_tundra       -> trigger();
  }
};

// Flame Shock Spell =======================================================

struct flame_shock_t : public shaman_spell_t
{
  double td_multiplier;
  flame_shock_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "flame_shock", player, SCHOOL_FIRE, TREE_ELEMENTAL )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 9, 500, 500, 139, 0.17 },
      { 75, 8, 425, 425, 119, 0.17 },
      { 70, 7, 377, 377, 105, 500  },
      { 60, 6, 309, 309,  86, 450  },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 49233 );

    base_execute_time = 0;
    base_tick_time    = 3.0;
    num_ticks         = 6;

    may_crit          = true;
    direct_power_mod  = 0.5*1.5/3.5;
    tick_power_mod    = 0.100;

    base_hit += p -> talents.elemental_precision * 0.01;

    base_dd_multiplier *= 1.0 + ( p -> talents.concussion       * 0.01 +
                                  p -> talents.booming_echoes   * 0.10 +
                                  p -> set_bonus.tier9_4pc_melee() * 0.25 );

    base_td_multiplier *= 1.0 + ( p -> talents.concussion       * 0.01 +
                                  p -> set_bonus.tier9_4pc_melee() * 0.25 +
                                  util_t::talent_rank( p -> talents.storm_earth_and_fire, 3, 0.20 ) +
                                  p -> set_bonus.tier8_2pc_caster() * 0.2 );


    base_cost_reduction  += ( p -> talents.convection        * 0.02 +
                              p -> talents.mental_quickness  * 0.02 +
                              p -> talents.shamanistic_focus * 0.45 );

    base_crit_bonus_multiplier *= 1.0 + ( p -> talents.elemental_fury * 0.20 +
					  p -> glyphs.flame_shock     * 0.60 );

    cooldown = p -> get_cooldown( "shock" );
    cooldown -> duration  = 6.0;
    cooldown -> duration -= ( p -> talents.reverberation  * 0.2 +
			      p -> talents.booming_echoes * 1.0 );

    if ( p -> glyphs.flame_shock ) tick_may_crit = true;

    // T8 2pc not yet changed
    if ( p -> set_bonus.tier8_2pc_caster() ) tick_may_crit = true;

    tick_may_crit = true;

    if ( p -> glyphs.shocking )
    {
      trigger_gcd = 1.0;
      min_gcd     = 1.0;
    }

    observer = &( p -> active_flame_shock );
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();

    num_ticks = ( p -> set_bonus.tier9_2pc_caster() ) ? 9 : 6;
    added_ticks = 0;
    shaman_spell_t::execute();
    p -> buffs_stonebreaker -> trigger();
    p -> buffs_tundra       -> trigger();
  }

  virtual int scale_ticks_with_haste() SC_CONST
  {
    return 1;
  }
};

// Wind Shear Spell ========================================================

struct wind_shear_t : public shaman_spell_t
{
  wind_shear_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "wind_shear", player, SCHOOL_NATURE, TREE_ELEMENTAL )
  {
    shaman_t* p = player -> cast_shaman();
    trigger_gcd = 0;
    base_dd_min = base_dd_max = 1;
    may_miss = may_resist = false;
    base_cost = player -> resource_base[ RESOURCE_MANA ] * 0.09;
    base_spell_power_multiplier = 0;
    cooldown -> duration = 6.0 - ( p -> talents.reverberation * 0.2 );

    id = 57994;
  }

  virtual bool ready()
  {
    if ( ! sim -> target -> debuffs.casting -> check() ) return false;
    return shaman_spell_t::ready();
  }
};

// =========================================================================
// Shaman Totems
// =========================================================================

// Searing Totem Spell =======================================================

struct searing_totem_t : public shaman_spell_t
{
  searing_totem_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "searing_totem", player, SCHOOL_FIRE, TREE_ELEMENTAL )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 10, 90, 120, 0, 0.07 },
      { 75,  9, 77, 103, 0, 0.07 },
      { 71,  8, 68,  92, 0, 0.07 },
      { 69,  7, 56,  74, 0, 0.07 },
      { 60,  6, 40,  54, 0, 0.09 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 58704 );

    base_execute_time = 0;
    base_tick_time    = 2.5;
    direct_power_mod  = base_tick_time / 15.0;
    num_ticks         = 24;
    may_crit          = true;
    trigger_gcd       = 1.0;
    base_multiplier  *= 1.0 + p -> talents.call_of_flame * 0.05;
    base_hit         += p -> talents.elemental_precision * 0.01;

    base_cost_reduction += ( p -> talents.totemic_focus    * 0.05 +
                             p -> talents.mental_quickness * 0.02 );

    base_crit_bonus_multiplier *= 1.0 + p -> talents.elemental_fury * 0.20;

    // Searing Totem is not a real DoT, but rather a pet that is spawned.
    pseudo_pet = true;

    observer = &( p -> active_fire_totem );
  }

  // Odd things to handle:
  // (1) Execute is guaranteed.
  // (2) Each "tick" is like an "execute".
  // (3) No hit/miss events are triggered.

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    player_buff();
    schedule_tick();
    update_ready();
    direct_dmg = 0;
    update_stats( DMG_DIRECT );
    *observer = this;
  }

  virtual void tick()
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );
    may_resist = false;
    target_debuff( DMG_DIRECT );
    calculate_result();
    may_resist = true;
    if ( result_is_hit() )
    {
      calculate_direct_damage();
      if ( direct_dmg > 0 )
      {
        tick_dmg = direct_dmg;
        assess_damage( tick_dmg, DMG_OVER_TIME );
      }
    }
    else
    {
      if ( sim -> log ) log_t::output( sim, "%s avoids %s (%s)", sim -> target -> name(), name(), util_t::result_type_string( result ) );
    }
    update_stats( DMG_OVER_TIME );
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();
    if ( p -> active_fire_totem )
      return false;
    return shaman_spell_t::ready();
  }
};

// Magma Totem Spell =======================================================

struct magma_totem_t : public shaman_spell_t
{
  magma_totem_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "magma_totem", player, SCHOOL_FIRE, TREE_ELEMENTAL )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 78,  7, 371, 371, 0, 0.27 },
      { 73,  6, 314, 314, 0, 0.27 },
      { 65,  5, 180, 180, 0, 0.27 },
      { 56,  4, 131, 131, 0, 0.27 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 58734 );

    aoe = true;
    base_execute_time = 0;
    base_tick_time    = 2.0;
    direct_power_mod  = 0.75 * base_tick_time / 15.0;
    num_ticks         = 10;
    may_crit          = true;
    trigger_gcd       = 1.0;
    base_multiplier  *= 1.0 + p -> talents.call_of_flame * 0.05;
    base_hit         += p -> talents.elemental_precision * 0.01;

    base_cost_reduction += ( p -> talents.totemic_focus    * 0.05 +
                             p -> talents.mental_quickness * 0.02 );

    base_crit_bonus_multiplier *= 1.0 + p -> talents.elemental_fury * 0.20;

    // Magma Totem is not a real DoT, but rather a pet that is spawned.
    pseudo_pet = true;

    observer = &( p -> active_fire_totem );
  }

  // Odd things to handle:
  // (1) Execute is guaranteed.
  // (2) Each "tick" is like an "execute".
  // (3) No hit/miss events are triggered.

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    player_buff();
    schedule_tick();
    update_ready();
    direct_dmg = 0;
    update_stats( DMG_DIRECT );
    *observer = this;
  }

  virtual void tick()
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );
    may_resist = false;
    target_debuff( DMG_DIRECT );
    calculate_result();
    may_resist = true;
    if ( result_is_hit() )
    {
      calculate_direct_damage();
      if ( direct_dmg > 0 )
      {
        tick_dmg = direct_dmg;
        assess_damage( tick_dmg, DMG_OVER_TIME );
      }
    }
    else
    {
      if ( sim -> log ) log_t::output( sim, "%s avoids %s (%s)", sim -> target -> name(), name(), util_t::result_type_string( result ) );
    }
    update_stats( DMG_OVER_TIME );
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();
    if ( p -> active_fire_totem )
      return false;
    return shaman_spell_t::ready();
  }
};

// Totem of Wrath Spell =====================================================

struct totem_of_wrath_t : public shaman_spell_t
{
  double bonus_spell_power;

  totem_of_wrath_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "totem_of_wrath", player, SCHOOL_NATURE, TREE_ELEMENTAL ), bonus_spell_power( 0 )
  {
    shaman_t* p = player -> cast_shaman();
    check_talent( p -> talents.totem_of_wrath );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost   = 400;
    trigger_gcd = 1.0;

    base_cost_reduction += ( p -> talents.totemic_focus    * 0.05 +
                             p -> talents.mental_quickness * 0.02 );

    bonus_spell_power = util_t::ability_rank( p -> level,  280.0,80,  140.0,70,  120.0,0 );

    observer = &( p -> active_fire_totem );

    id = 57722;
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    consume_resource();
    update_ready();
    sim -> auras.totem_of_wrath -> trigger( 1, bonus_spell_power );
    sim -> target -> debuffs.totem_of_wrath -> trigger();
    p -> buffs_totem_of_wrath_glyph -> trigger( 1, 0.30 * bonus_spell_power );
    *observer = this;
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();

    if ( ! shaman_spell_t::ready() )
      return false;

    if ( p -> glyphs.totem_of_wrath && ! p -> buffs_totem_of_wrath_glyph -> check() )
      return true;

    return ! sim -> auras.totem_of_wrath -> check();
  }

  virtual double gcd() SC_CONST { return player -> in_combat ? shaman_spell_t::gcd() : 0; }
};

// Flametongue Totem Spell ====================================================

struct flametongue_totem_t : public shaman_spell_t
{
  double bonus;

  flametongue_totem_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "flametongue_totem", player, SCHOOL_FIRE, TREE_ENHANCEMENT ), bonus( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 8,  0,  0, 144, 0.11 },
      { 76, 7,  0,  0, 122, 0.11 },
      { 72, 6,  0,  0, 106, 0.11 },
      { 67, 5, 12, 12,  73, 325  },
      { 58, 4, 15, 15,  62, 275  },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks );

    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.11;
    trigger_gcd = 1.0;

    base_cost_reduction += ( p -> talents.totemic_focus    * 0.05 +
                             p -> talents.mental_quickness * 0.02 );

    bonus = util_t::ability_rank( p -> level,  144,80,  122,76,  106,72,  73,67,  62,0 );
    bonus *= 1 + p -> talents.enhancing_totems * 0.05;

    observer = &( p -> active_fire_totem );

    id = 58656;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    sim -> auras.flametongue_totem -> trigger( 1, bonus );
    *observer = this;
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();

    if ( p -> active_fire_totem )
      return false;

    if( sim -> auras.flametongue_totem -> check() )
      return false;

    return shaman_spell_t::ready();
  }

  virtual double gcd() SC_CONST { return player -> in_combat ? shaman_spell_t::gcd() : 0; }
};

// Fire Elemental Totem Spell =================================================

struct fire_elemental_totem_t : public shaman_spell_t
{
  fire_elemental_totem_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "fire_elemental_totem", player, SCHOOL_FIRE, TREE_ELEMENTAL )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.23;
    trigger_gcd = 1.0;

    base_cost_reduction += ( p -> talents.totemic_focus    * 0.05 +
                             p -> talents.mental_quickness * 0.02 );

    num_ticks = 1;
    base_tick_time = 120.1;

    cooldown -> duration = p -> glyphs.fire_elemental_totem ? 300 : 600;

    observer = &( p -> active_fire_totem );

    id = 2894;
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    if ( p -> active_fire_totem ) p -> active_fire_totem -> cancel();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    schedule_tick();
    update_ready();
    p -> summon_pet( "fire_elemental" );
    *observer = this;
  }

  virtual void last_tick()
  {
    shaman_spell_t::last_tick();
    player -> dismiss_pet( "fire_elemental" );
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
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost    = 275;
    trigger_gcd  = 1.0;

    base_cost_reduction += ( p -> talents.totemic_focus    * 0.05 +
                             p -> talents.mental_quickness * 0.02 );

    bonus = 0.16 + p -> talents.improved_windfury_totem * 0.02;

    id = 8512;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    sim -> auras.windfury_totem -> trigger( 1, bonus );
  }

  virtual bool ready()
  {
    if( sim -> auras.windfury_totem -> current_value >= bonus )
      return false;

    return shaman_spell_t::ready();
  }

  virtual double gcd() SC_CONST { return player -> in_combat ? shaman_spell_t::gcd() : 0; }
};

// Flametongue Weapon Spell ===================================================

struct flametongue_weapon_t : public shaman_spell_t
{
  double bonus_power;
  int    main, off;

  flametongue_weapon_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "flametongue_weapon", player, SCHOOL_FIRE, TREE_ENHANCEMENT ), bonus_power( 0 ), main( 0 ), off( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    std::string weapon_str;

    option_t options[] =
    {
      { "weapon", OPT_STRING, &weapon_str },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( weapon_str.empty() )
    {
      main = off = 1;
    }
    else if ( weapon_str == "main" )
    {
      main = 1;
    }
    else if ( weapon_str == "off" )
    {
      off = 1;
    }
    else if ( weapon_str == "both" )
    {
      main = 1;
      off = 1;
    }
    else
    {
      sim -> errorf( "Player %s: flametongue_weapon: weapon option must be one of main/off/both\n", p -> name() );
      assert( 0 );
    }
    trigger_gcd = 0;

    bonus_power = util_t::ability_rank( p -> level,  211.0,80,  186.0,77,  157.0,72,  96.0,65,  45.0,0  );

    bonus_power *= 1.0 + p -> talents.elemental_weapons * 0.10;

    id = 58790;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    if ( main )
    {
      player -> main_hand_weapon.buff_type  = FLAMETONGUE_IMBUE;
      player -> main_hand_weapon.buff_value = bonus_power;
    }
    if ( off )
    {
      player -> off_hand_weapon.buff_type  = FLAMETONGUE_IMBUE;
      player -> off_hand_weapon.buff_value = bonus_power;
    }
  };

  virtual bool ready()
  {
    if ( main && ( player -> main_hand_weapon.buff_type != FLAMETONGUE_IMBUE ) )
      return true;

    if ( off && ( player -> off_hand_weapon.buff_type != FLAMETONGUE_IMBUE ) )
      return true;

    return false;
  }
};

// Windfury Weapon Spell ====================================================

struct windfury_weapon_t : public shaman_spell_t
{
  double bonus_power;
  int    main, off;

  windfury_weapon_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "windfury_weapon", player, SCHOOL_NATURE, TREE_ENHANCEMENT ), bonus_power( 0 ), main( 0 ), off( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    std::string weapon_str;
    option_t options[] =
    {
      { "weapon", OPT_STRING, &weapon_str },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    if ( weapon_str.empty() )
    {
      main = off = 1;
    }
    else if ( weapon_str == "main" )
    {
      main = 1;
    }
    else if ( weapon_str == "off" )
    {
      off = 1;
    }
    else if ( weapon_str == "both" )
    {
      main = 1;
      off = 1;
    }
    else
    {
      sim -> errorf( "Player %s: windfury_weapon: weapon option must be one of main/off/both\n", p -> name() );
      sim -> cancel();
    }
    trigger_gcd = 0;

    bonus_power = util_t::ability_rank( p -> level,  1250.0,80,  1090.0,76,  835.0,71,  445.0,0  );

    id = 58804;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    if ( main )
    {
      player -> main_hand_weapon.buff_type  = WINDFURY_IMBUE;
      player -> main_hand_weapon.buff_value = bonus_power;
    }
    if ( off )
    {
      player -> off_hand_weapon.buff_type  = WINDFURY_IMBUE;
      player -> off_hand_weapon.buff_value = bonus_power;
    }
  };

  virtual bool ready()
  {
    if ( main && ( player -> main_hand_weapon.buff_type != WINDFURY_IMBUE ) )
      return true;

    if ( off && ( player -> off_hand_weapon.buff_type != WINDFURY_IMBUE ) )
      return true;

    return false;
  }
};

// Strength of Earth Totem Spell ==============================================

struct strength_of_earth_totem_t : public shaman_spell_t
{
  double bonus;

  strength_of_earth_totem_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "strength_of_earth_totem", player, SCHOOL_NATURE, TREE_ENHANCEMENT ), bonus( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    base_cost    = util_t::ability_rank( p -> level,  300,65,  275,0 );
    trigger_gcd  = 1.0;

    base_cost_reduction += ( p -> talents.totemic_focus    * 0.05 +
                             p -> talents.mental_quickness * 0.02 );

    bonus  = util_t::ability_rank( p -> level,  155,80, 115,75, 86,65,  77,0 );
    bonus *= 1.0 + p -> talents.enhancing_totems * 0.05;

    id = 58643;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    sim -> auras.strength_of_earth -> trigger( 1, bonus );
  }

  virtual bool ready()
  {
    if ( sim -> auras.strength_of_earth -> current_value >= bonus )
      return false;

    return shaman_spell_t::ready();
  }

  virtual double gcd() SC_CONST { return player -> in_combat ? shaman_spell_t::gcd() : 0; }
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
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost   = 320;
    trigger_gcd = 1.0;

    base_cost_reduction += ( p -> talents.totemic_focus    * 0.05 +
                             p -> talents.mental_quickness * 0.02 );

    id = 3738;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    sim -> auras.wrath_of_air -> trigger();
  }

  virtual bool ready()
  {
    if ( sim -> auras.wrath_of_air -> check() )
      return false;

    return shaman_spell_t::ready();
  }

  virtual double gcd() SC_CONST { return player -> in_combat ? shaman_spell_t::gcd() : 0; }
};

// Mana Tide Totem Spell ==================================================

struct mana_tide_totem_t : public shaman_spell_t
{
  mana_tide_totem_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "mana_tide_totem", player, SCHOOL_NATURE, TREE_RESTORATION )
  {
    shaman_t* p = player -> cast_shaman();
    check_talent( p -> talents.mana_tide_totem );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful        = false;
    base_tick_time = 3.0;
    num_ticks      = 4;
    base_cost      = 320;
    trigger_gcd     = 1.0;

    cooldown -> duration = 300.0;

    dot = p -> get_dot( "water_totem" );

    base_cost_reduction += ( p -> talents.totemic_focus    * 0.05 +
                             p -> talents.mental_quickness * 0.02 );

    id = 16190;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    schedule_tick();
    update_ready();
  }

  virtual void tick()
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );

    double pct = 0.06;
    if ( player -> cast_shaman() -> glyphs.mana_tide ) pct += 0.01;

    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if ( p -> party == player -> party )
      {
        p -> resource_gain( RESOURCE_MANA, p -> resource_max[ RESOURCE_MANA ] * pct, p -> gains.mana_tide );
      }
    }
  }

  virtual bool ready()
  {
    if ( ! shaman_spell_t::ready() )
      return false;

    return( player -> resource_current[ RESOURCE_MANA ] < ( 0.75 * player -> resource_max[ RESOURCE_MANA ] ) );
  }

  virtual double gcd() SC_CONST { return player -> in_combat ? shaman_spell_t::gcd() : 0; }
};

// Mana Spring Totem Spell ================================================

struct mana_spring_totem_t : public shaman_spell_t
{
  double regen;

  mana_spring_totem_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "mana_spring_totem", player, SCHOOL_NATURE, TREE_RESTORATION ), regen( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful     = false;
    base_cost   = 120;
    trigger_gcd = 1.0;

    base_cost_reduction += ( p -> talents.totemic_focus    * 0.05 +
                             p -> talents.mental_quickness * 0.02 );

    regen = util_t::ability_rank( p -> level,  91.0,80,  82.0,76,  73.0,71,  41.0,65,  31.0,0 );

    regen *= 1.0 + util_t::talent_rank( p -> talents.restorative_totems, 3, 0.07, 0.12, 0.20 );

    id = 58774;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    sim -> auras.mana_spring_totem -> trigger( 1, regen );
  }

  virtual bool ready()
  {
    if ( sim -> auras.mana_spring_totem -> current_value >= regen )
      return false;

    return shaman_spell_t::ready();
  }

  virtual double gcd() SC_CONST { return player -> in_combat ? shaman_spell_t::gcd() : 0; }
};

// Bloodlust Spell ===========================================================

struct bloodlust_t : public shaman_spell_t
{
  bloodlust_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "bloodlust", player, SCHOOL_NATURE, TREE_ENHANCEMENT )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful = false;
    base_cost = ( 0.26 * player -> resource_base[ RESOURCE_MANA ] );
    base_cost_reduction += p -> talents.mental_quickness * 0.02;
    cooldown -> duration = 300;

    id = 2825;
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if ( p -> sleeping ) continue;
      p -> buffs.bloodlust -> trigger();
    }
  }

  virtual bool ready()
  {
    if ( player -> buffs.bloodlust -> check() )
      return false;

    if (  sim -> current_time < player -> buffs.bloodlust -> cooldown_ready )
      return false;

    return shaman_spell_t::ready();
  }
};

// Shamanisitc Rage Spell ===========================================================

struct shamanistic_rage_t : public shaman_spell_t
{
  bool tier10_2pc;

  shamanistic_rage_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "shamanistic_rage", player, SCHOOL_NATURE, TREE_ENHANCEMENT ),
      tier10_2pc( false )
  {
    option_t options[] =
    {
      { "tier10_2pc_melee", OPT_BOOL, &tier10_2pc },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    cooldown -> duration = 60;

    id = 30823;
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    update_ready();
    p -> buffs_shamanistic_rage -> trigger();
    p -> buffs_tier10_2pc_melee -> trigger();
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();

    if ( ! shaman_spell_t::ready() )
      return false;

    if ( tier10_2pc )
      return p -> set_bonus.tier10_2pc_melee() != 0;

    return( player -> resource_current[ RESOURCE_MANA ] < ( 0.10 * player -> resource_max[ RESOURCE_MANA ] ) );
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

      base_dd_min = base_dd_max = base_dmg;

      trigger_gcd      = 0;
      background       = true;
      direct_power_mod = 0.33;

      base_hit        += p -> talents.elemental_precision * 0.01;
      base_multiplier *= 1.0 + p -> talents.improved_shields * 0.05 + ( p -> set_bonus.tier7_2pc_melee() ? 0.10 : 0.00 );

      base_crit_bonus_multiplier *= 1.0 + p -> talents.elemental_fury * 0.20;

      if ( p -> glyphs.lightning_shield ) base_multiplier *= 1.20;

      // Lightning Shield may actually be modeled as a pet given that it cannot proc Honor Among Thieves
      pseudo_pet = true;
    }
  };

  lightning_shield_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "lightning_shield", player, SCHOOL_NATURE, TREE_ENHANCEMENT )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 80, 7, 380, 380, 0, 0 },
      { 75, 6, 325, 325, 0, 0 },
      { 70, 5, 287, 287, 0, 0 },
      { 63, 4, 232, 232, 0, 0 },
      { 56, 3, 198, 198, 0, 0 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 49281 );

    if ( ! p -> active_lightning_charge )
    {
      p -> active_lightning_charge = new lightning_charge_t( p, ( base_dd_min + base_dd_max ) / 2.0 ) ;
    }
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_water_shield -> expire();
    p -> buffs_lightning_shield -> trigger( 3 + 2 * p -> talents.static_shock );
    consume_resource();
    update_ready();
    direct_dmg = 0;
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();
    if ( p -> buffs_lightning_shield -> check() )
      return false;
    return shaman_spell_t::ready();
  }
};

// Water Shield Spell =========================================================

struct water_shield_t : public shaman_spell_t
{
  water_shield_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "water_shield", player, SCHOOL_NATURE, TREE_ENHANCEMENT )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 76, 9, 0, 0, 0, 100 },
      { 69, 8, 0, 0, 0,  50 },
      { 62, 7, 0, 0, 0,  43 },
      { 55, 6, 0, 0, 0,  38 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 57960 );

    trigger_gcd = 0;
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
    p -> buffs_lightning_shield -> expire();
    p -> buffs_water_shield -> trigger( 1, base_cost );
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();
    if ( p -> buffs_water_shield -> check() )
      return false;
    return shaman_spell_t::ready();
  }
};

// Thunderstorm Spell ==========================================================

struct thunderstorm_t : public shaman_spell_t
{
  thunderstorm_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "thunderstorm", player, SCHOOL_NATURE, TREE_ELEMENTAL )
  {
    shaman_t* p = player -> cast_shaman();
    check_talent( p -> talents.thunderstorm );
    cooldown -> duration = 45.0;

    id = 59159;
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    update_ready();
    double mana_pct = p -> glyphs.thunderstorm ? 0.10 : 0.08;
    p -> resource_gain( RESOURCE_MANA, p -> resource_max[ RESOURCE_MANA ] * mana_pct, p -> gains_thunderstorm );
  }

  virtual bool ready()
  {
    if ( ! shaman_spell_t::ready() )
      return false;

    return player -> resource_current[ RESOURCE_MANA ] < ( 0.92 * player -> resource_max[ RESOURCE_MANA ] );
  }
};

// Spirit Wolf Spell ==========================================================

struct spirit_wolf_spell_t : public shaman_spell_t
{
  spirit_wolf_spell_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "spirit_wolf", player, SCHOOL_NATURE, TREE_ENHANCEMENT )
  {
    shaman_t* p = player -> cast_shaman();
    check_talent( p -> talents.feral_spirit );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    trigger_gcd = 0;
    base_cost  = p -> resource_max[ RESOURCE_MANA ] * 0.12;
    cooldown -> duration = 180.0;

    id = 51533;
  }

  virtual void execute()
  {
    consume_resource();
    update_ready();
    player -> summon_pet( "spirit_wolf", 45.0 );
  }

};

} // ANONYMOUS NAMESPACE ===================================================

// ==========================================================================
// Shaman Character Definition
// ==========================================================================

// shaman_t::create_action  =================================================

action_t* shaman_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  if ( name == "auto_attack"             ) return new              auto_attack_t( this, options_str );
  if ( name == "bloodlust"               ) return new                bloodlust_t( this, options_str );
  if ( name == "chain_lightning"         ) return new          chain_lightning_t( this, options_str );
  if ( name == "earth_shock"             ) return new              earth_shock_t( this, options_str );
  if ( name == "elemental_mastery"       ) return new        elemental_mastery_t( this, options_str );
  if ( name == "fire_elemental_totem"    ) return new     fire_elemental_totem_t( this, options_str );
  if ( name == "fire_nova"               ) return new                fire_nova_t( this, options_str );
  if ( name == "flame_shock"             ) return new              flame_shock_t( this, options_str );
  if ( name == "flametongue_totem"       ) return new        flametongue_totem_t( this, options_str );
  if ( name == "flametongue_weapon"      ) return new       flametongue_weapon_t( this, options_str );
  if ( name == "frost_shock"             ) return new              frost_shock_t( this, options_str );
  if ( name == "lava_burst"              ) return new               lava_burst_t( this, options_str );
  if ( name == "lava_lash"               ) return new                lava_lash_t( this, options_str );
  if ( name == "lightning_bolt"          ) return new           lightning_bolt_t( this, options_str );
  if ( name == "lightning_shield"        ) return new         lightning_shield_t( this, options_str );
  if ( name == "mana_spring_totem"       ) return new        mana_spring_totem_t( this, options_str );
  if ( name == "mana_tide_totem"         ) return new          mana_tide_totem_t( this, options_str );
  if ( name == "natures_swiftness"       ) return new        shamans_swiftness_t( this, options_str );
  if ( name == "searing_totem"           ) return new            searing_totem_t( this, options_str );
  if ( name == "magma_totem"             ) return new              magma_totem_t( this, options_str );
  if ( name == "shamanistic_rage"        ) return new         shamanistic_rage_t( this, options_str );
  if ( name == "spirit_wolf"             ) return new        spirit_wolf_spell_t( this, options_str );
  if ( name == "stormstrike"             ) return new              stormstrike_t( this, options_str );
  if ( name == "strength_of_earth_totem" ) return new  strength_of_earth_totem_t( this, options_str );
  if ( name == "thunderstorm"            ) return new             thunderstorm_t( this, options_str );
  if ( name == "totem_of_wrath"          ) return new           totem_of_wrath_t( this, options_str );
  if ( name == "water_shield"            ) return new             water_shield_t( this, options_str );
  if ( name == "wind_shear"              ) return new               wind_shear_t( this, options_str );
  if ( name == "windfury_totem"          ) return new           windfury_totem_t( this, options_str );
  if ( name == "windfury_weapon"         ) return new          windfury_weapon_t( this, options_str );
  if ( name == "wrath_of_air_totem"      ) return new       wrath_of_air_totem_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// shaman_t::create_pet ======================================================

pet_t* shaman_t::create_pet( const std::string& pet_name )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "spirit_wolf"    ) return new spirit_wolf_pet_t   ( sim, this );
  if ( pet_name == "fire_elemental" ) return new fire_elemental_pet_t( sim, this );

  return 0;
}

// shaman_t::create_pets =====================================================

void shaman_t::create_pets()
{
  create_pet( "spirit_wolf" );
  create_pet( "fire_elemental" );
}

// shaman_t::init_rating ======================================================

void shaman_t::init_rating()
{
  player_t::init_rating();

  rating.attack_haste *= 1.0 / 1.30;
}

// shaman_t::init_glyphs ======================================================

void shaman_t::init_glyphs()
{
  memset( ( void* ) &glyphs, 0x0, sizeof( glyphs_t ) );

  std::vector<std::string> glyph_names;
  int num_glyphs = util_t::string_split( glyph_names, glyphs_str, ",/" );

  for ( int i=0; i < num_glyphs; i++ )
  {
    std::string& n = glyph_names[ i ];

    if      ( n == "chain_lightning"      ) glyphs.chain_lightning = 1;
    else if ( n == "elemental_mastery"    ) glyphs.elemental_mastery = 1;
    else if ( n == "feral_spirit"         ) glyphs.feral_spirit = 1;
    else if ( n == "fire_elemental_totem" ) glyphs.fire_elemental_totem = 1;
    else if ( n == "fire_nova"            ) glyphs.fire_nova = 1;
    else if ( n == "fire_nova_totem"      ) glyphs.fire_nova = 1;
    else if ( n == "flame_shock"          ) glyphs.flame_shock = 1;
    else if ( n == "flametongue_weapon"   ) glyphs.flametongue_weapon = 1;
    else if ( n == "lava"                 ) glyphs.lava = 1;
    else if ( n == "lava_lash"            ) glyphs.lava_lash = 1;
    else if ( n == "lightning_bolt"       ) glyphs.lightning_bolt = 1;
    else if ( n == "lightning_shield"     ) glyphs.lightning_shield = 1;
    else if ( n == "mana_tide"            ) glyphs.mana_tide = 1;
    else if ( n == "shocking"             ) glyphs.shocking = 1;
    else if ( n == "stormstrike"          ) glyphs.stormstrike = 1;
    else if ( n == "thunderstorm"         ) glyphs.thunderstorm = 1;
    else if ( n == "totem_of_wrath"       ) glyphs.totem_of_wrath = 1;
    else if ( n == "windfury_weapon"      ) glyphs.windfury_weapon = 1;
    // To prevent warnings....
    else if ( n == "astral_recall"        ) ;
    else if ( n == "chain_heal"           ) ;
    else if ( n == "earth_shield"         ) ;
    else if ( n == "ghost_wolf"           ) ;
    else if ( n == "healing_stream_totem" ) ;
    else if ( n == "healing_wave"         ) ;
    else if ( n == "lesser_healing_wave"  ) ;
    else if ( n == "renewed_life"         ) ;
    else if ( n == "riptide"              ) ;
    else if ( n == "stoneclaw_totem"      ) ;
    else if ( n == "water_breathing"      ) ;
    else if ( n == "water_mastery"        ) ;
    else if ( n == "water_shield"         ) ;
    else if ( n == "water_walking"        ) ;
    else if ( ! sim -> parent ) 
    {
      sim -> errorf( "Player %s has unrecognized glyph %s\n", name(), n.c_str() );
    }
  }
}

// shaman_t::init_race ======================================================

void shaman_t::init_race()
{
  race = util_t::parse_race_type( race_str );
  switch ( race )
  {
  case RACE_DRAENEI:
  case RACE_TAUREN:
  case RACE_ORC:
  case RACE_TROLL:
    break;
  default:
    race = RACE_ORC;
    race_str = util_t::race_type_string( race );
  }

  player_t::init_race();
}

// shaman_t::init_base ========================================================

void shaman_t::init_base()
{
  attribute_base[ ATTR_STRENGTH  ] = rating_t::get_attribute_base( sim, level, SHAMAN, race, BASE_STAT_STRENGTH );
  attribute_base[ ATTR_AGILITY   ] = rating_t::get_attribute_base( sim, level, SHAMAN, race, BASE_STAT_AGILITY );
  attribute_base[ ATTR_STAMINA   ] = rating_t::get_attribute_base( sim, level, SHAMAN, race, BASE_STAT_STAMINA );
  attribute_base[ ATTR_INTELLECT ] = rating_t::get_attribute_base( sim, level, SHAMAN, race, BASE_STAT_INTELLECT );
  attribute_base[ ATTR_SPIRIT    ] = rating_t::get_attribute_base( sim, level, SHAMAN, race, BASE_STAT_SPIRIT );
  resource_base[ RESOURCE_HEALTH ] = rating_t::get_attribute_base( sim, level, SHAMAN, race, BASE_STAT_HEALTH );
  resource_base[ RESOURCE_MANA   ] = rating_t::get_attribute_base( sim, level, SHAMAN, race, BASE_STAT_MANA );
  base_spell_crit                  = rating_t::get_attribute_base( sim, level, SHAMAN, race, BASE_STAT_SPELL_CRIT );
  base_attack_crit                 = rating_t::get_attribute_base( sim, level, SHAMAN, race, BASE_STAT_MELEE_CRIT );
  initial_spell_crit_per_intellect = rating_t::get_attribute_base( sim, level, SHAMAN, race, BASE_STAT_SPELL_CRIT_PER_INT );
  initial_attack_crit_per_agility  = rating_t::get_attribute_base( sim, level, SHAMAN, race, BASE_STAT_MELEE_CRIT_PER_AGI );

  attribute_multiplier_initial[ ATTR_INTELLECT ] *= 1.0 + talents.ancestral_knowledge * 0.02;
  attribute_multiplier_initial[ ATTR_STAMINA   ] *= 1.0 + talents.toughness           * 0.02;
  base_attack_expertise = talents.unleashed_rage * 0.03;

  base_attack_power = ( level * 2 ) - 20;
  initial_attack_power_per_strength = 1.0;
  initial_attack_power_per_agility  = 1.0;

  health_per_stamina = 10;
  mana_per_intellect = 15;

  mp5_per_intellect = util_t::talent_rank( talents.unrelenting_storm, 3, 0.04 );

  base_spell_crit  += talents.blessing_of_the_eternals * 0.02;
  base_spell_crit  += talents.thundering_strikes * 0.01;
  base_attack_crit += talents.thundering_strikes * 0.01;

  if ( set_bonus.tier6_2pc_caster() )
  {
    // Simply assume the totems are out all the time.

    enchant.spell_power += 45;
    enchant.crit_rating += 35;
    enchant.mp5         += 15;
  }

  distance = ( primary_tree() == TREE_ENHANCEMENT ) ? 3 : 30;
}

// shaman_t::init_items =====================================================

void shaman_t::init_items()
{
  player_t::init_items();

  std::string& totem = items[ SLOT_RANGED ].encoded_name_str;

  if ( totem.empty() ) return;

  if      ( totem == "stonebreakers_totem"            ) totems.stonebreaker = 1;
  else if ( totem == "totem_of_dueling"               ) totems.dueling = 1;
  else if ( totem == "totem_of_electrifying_wind"     ) totems.electrifying_wind = 1;
  else if ( totem == "totem_of_hex"                   ) totems.hex = 1;
  else if ( totem == "totem_of_indomitability"        ) totems.indomitability = 1;
  else if ( totem == "totem_of_splintering"           ) totems.splintering = 1;
  else if ( totem == "totem_of_quaking_earth"         ) totems.quaking_earth = 1;
  else if ( totem == "totem_of_the_dancing_flame"     ) totems.dancing_flame = 1;
  else if ( totem == "totem_of_the_tundra"            ) totems.tundra = 1;
  else if ( totem == "thunderfall_totem"              ) totems.thunderfall = 1;
  else if ( totem == "totem_of_the_avalanche"         ) totems.avalanche = 1;
  else if ( totem == "bizuris_totem_of_shattered_ice" ) totems.shattered_ice = 1;
  else if ( totem.find( "totem_of_indomitability" ) != std::string::npos )
  {
    totems.indomitability = 1;
  }
  // To prevent warnings...
  else if ( totem == "steamcallers_totem"      ) ;
  else if ( totem == "totem_of_calming_tides"  ) ;
  else if ( totem == "totem_of_forest_growth"  ) ;
  else if ( totem == "totem_of_healing_rains"  ) ;
  else if ( totem == "totem_of_the_surging_sea" ) ;
  else if ( totem == "deadly_gladiators_totem_of_the_third_wind" ) ;
  else
  {
    sim -> errorf( "Player %s has unknown totem %s", name(), totem.c_str() );
  }
}

// shaman_t::init_scaling ====================================================

void shaman_t::init_scaling()
{
  player_t::init_scaling();

  scales_with[ STAT_SPIRIT ] = 0;

  if ( talents.dual_wield )
  {
    scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = 1;
    scales_with[ STAT_WEAPON_OFFHAND_SPEED  ] = 1;
  }
}

// shaman_t::init_buffs ======================================================

void shaman_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( sim, player, name, max_stack, duration, cooldown, proc_chance, quiet )

  buffs_elemental_devastation = new buff_t( this, "elemental_devastation", 1,  10.0, 0.0, talents.elemental_devastation );
  buffs_elemental_focus       = new buff_t( this, "elemental_focus",       2,  15.0, 0.0, talents.elemental_focus       );
  buffs_elemental_mastery     = new buff_t( this, "elemental_mastery",     1,  15.0, 0.0, talents.elemental_mastery     );
  buffs_flurry                = new buff_t( this, "flurry",                3,   0.0, 0.0, talents.flurry                );
  buffs_lightning_shield      = new buff_t( this, "lightning_shield",      3 + 2 * talents.static_shock );
  buffs_maelstrom_weapon      = new buff_t( this, "maelstrom_weapon",      5,  30.0 );
  buffs_nature_vulnerability  = new buff_t( this, "nature_vulnerability",  4,  12.0 );
  buffs_natures_swiftness     = new buff_t( this, "natures_swiftness" );
  buffs_shamanistic_rage      = new buff_t( this, "shamanistic_rage",      1,  15.0, 0.0, talents.shamanistic_rage      );
  buffs_tier10_2pc_melee      = new buff_t( this, "tier10_2pc_melee",      1,  15.0, 0.0, set_bonus.tier10_2pc_melee() );
  buffs_tier10_4pc_melee      = new buff_t( this, "tier10_4pc_melee",      1,  10.0, 0.0, 0.15 ); //FIX ME - assuming no icd on this
  buffs_totem_of_wrath_glyph  = new buff_t( this, "totem_of_wrath_glyph",  1, 300.0, 0.0, glyphs.totem_of_wrath );
  buffs_water_shield          = new buff_t( this, "water_shield",          1, 600.0 );


  // stat_buff_t( sim, player, name, stat, amount, max_stack, duration, cooldown, proc_chance, quiet )

  buffs_avalanche         = new stat_buff_t( this, "avalanche",         STAT_ATTACK_POWER, 146, 3, 15.0,     0, totems.avalanche );
  buffs_dueling           = new stat_buff_t( this, "dueling",           STAT_HASTE_RATING,  60, 1,  6.0, 10.01, totems.dueling );
  buffs_electrifying_wind = new stat_buff_t( this, "electrifying_wind", STAT_HASTE_RATING, 200, 1, 12.0,  6.01, totems.electrifying_wind * 0.70 );
  buffs_indomitability    = new stat_buff_t( this, "indomitability",    STAT_ATTACK_POWER, 120, 1, 10.0, 10.01, totems.indomitability );
  buffs_quaking_earth     = new stat_buff_t( this, "quaking_earth",     STAT_ATTACK_POWER, 400, 1, 18.0,  9.01, totems.quaking_earth * 0.80 );
  buffs_shattered_ice     = new stat_buff_t( this, "shattered_ice",     STAT_HASTE_RATING,  44, 5, 30.0,     0, totems.shattered_ice );
  buffs_stonebreaker      = new stat_buff_t( this, "stonebreaker",      STAT_ATTACK_POWER, 110, 1, 10.0, 10.01, totems.stonebreaker );
  buffs_tundra            = new stat_buff_t( this, "tundra",            STAT_ATTACK_POWER,  94, 1, 10.0, 10.01, totems.tundra );
}

// shaman_t::init_gains ======================================================

void shaman_t::init_gains()
{
  player_t::init_gains();

  gains_improved_stormstrike = get_gain( "improved_stormstrike" );
  gains_shamanistic_rage     = get_gain( "shamanistic_rage"     );
  gains_thunderstorm         = get_gain( "thunderstorm"         );
  gains_water_shield         = get_gain( "water_shield"         );
}

// shaman_t::init_procs ======================================================

void shaman_t::init_procs()
{
  player_t::init_procs();

  procs_lightning_overload = get_proc( "lightning_overload" );
  procs_windfury           = get_proc( "windfury"           );
}

// shaman_t::init_rng ========================================================

void shaman_t::init_rng()
{
  player_t::init_rng();

  rng_improved_stormstrike = get_rng( "improved_stormstrike" );
  rng_lightning_overload   = get_rng( "lightning_overload"   );
  rng_static_shock         = get_rng( "static_shock"         );
  rng_windfury_weapon      = get_rng( "windfury_weapon"      );
}

// shaman_t::init_actions =====================================================

void shaman_t::init_actions()
{
  if ( primary_tree() == TREE_ENHANCEMENT && main_hand_weapon.type == WEAPON_NONE )
  {
    sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  if ( action_list_str.empty() )
  {
    if ( primary_tree() == TREE_ENHANCEMENT )
    {
      action_list_str  = "flask,type=endless_rage/food,type=fish_feast/windfury_weapon,weapon=main/flametongue_weapon,weapon=off";
      action_list_str += "/strength_of_earth_totem/windfury_totem";
      action_list_str += "/auto_attack";
      action_list_str += "/snapshot_stats";
      action_list_str += "/wind_shear";
      action_list_str += "/bloodlust,time_to_die<=60";
      int num_items = ( int ) items.size();
      for ( int i=0; i < num_items; i++ )
      {
        if ( items[ i ].use.active() )
        {
          action_list_str += "/use_item,name=";
          action_list_str += items[ i ].name();
        }
      }
      if ( race == RACE_ORC )
      {
        action_list_str += "/blood_fury";
      }
      else if ( race == RACE_TROLL )
      {
        action_list_str += "/berserking";
      }

      action_list_str += "/fire_elemental_totem";
      if ( talents.feral_spirit ) action_list_str += "/spirit_wolf";
      action_list_str += "/speed_potion";
      action_list_str += "/lightning_bolt,if=buff.maelstrom_weapon.stack=5&buff.maelstrom_weapon.react";

      if ( talents.stormstrike ) action_list_str += "/stormstrike";
      action_list_str += "/flame_shock,if=!ticking";
      action_list_str += "/earth_shock/magma_totem/fire_nova/lightning_shield";
      if ( talents.lava_lash ) action_list_str += "/lava_lash";
      if ( talents.shamanistic_rage ) action_list_str += "/shamanistic_rage,tier10_2pc_melee=1";
      if ( talents.shamanistic_rage ) action_list_str += "/shamanistic_rage";
    }
    else
    {
      action_list_str  = "flask,type=frost_wyrm/food,type=fish_feast/flametongue_weapon,weapon=main/water_shield";
      action_list_str += "/mana_spring_totem/wrath_of_air_totem";
      if ( talents.totem_of_wrath ) action_list_str += "/totem_of_wrath";
      action_list_str += "/snapshot_stats";
      action_list_str += "/wind_shear";
      action_list_str += "/bloodlust,time_to_die<=59";
      int num_items = ( int ) items.size();
      for ( int i=0; i < num_items; i++ )
      {
        if ( items[ i ].use.active() )
        {
          action_list_str += "/use_item,name=";
          action_list_str += items[ i ].name();
        }
      }
      if ( race == RACE_ORC )
      {
        action_list_str += "/blood_fury";
      }
      else if ( race == RACE_TROLL )
      {
        action_list_str += "/berserking";
      }
      if ( set_bonus.tier9_2pc_caster() || set_bonus.tier10_2pc_caster() )
      {
          action_list_str += "/wild_magic_potion,if=buff.bloodlust.react";
      }
      else
      {
          action_list_str += "/speed_potion";
      }
      if ( talents.elemental_mastery )
      {
          action_list_str += "/elemental_mastery,time_to_die<=17";
          action_list_str += "/elemental_mastery,if=!buff.bloodlust.react";
      }
      action_list_str += "/flame_shock,if=!ticking";
      if ( level >= 75 ) action_list_str += "/lava_burst,if=(dot.flame_shock.remains-cast_time)>=0";
      if ( ! talents.totem_of_wrath )
      {
	    action_list_str += "/fire_elemental_totem";
	    action_list_str += "/searing_totem";
      }
      action_list_str += "/fire_nova,if=target.adds>3";
      action_list_str += "/chain_lightning,if=target.adds>1";
      action_list_str += "/fire_nova,if=target.adds>2";
      if ( ! ( set_bonus.tier9_4pc_caster() || set_bonus.tier10_2pc_caster() ) )
          action_list_str += "/chain_lightning,if=(!buff.bloodlust.react&(mana_pct-target.health_pct)>5)";
      action_list_str += "/lightning_bolt";
      action_list_str += "/fire_nova,moving=1";
      if ( talents.thunderstorm ) action_list_str += "/thunderstorm";
    }

    action_list_default = 1;
  }

  player_t::init_actions();
}

// shaman_t::interrupt =======================================================

void shaman_t::interrupt()
{
  player_t::interrupt();

  if ( main_hand_attack ) main_hand_attack -> cancel();
  if (  off_hand_attack )  off_hand_attack -> cancel();
}

// shaman_t::composite_attack_power ==========================================

double shaman_t::composite_attack_power() SC_CONST
{
  double ap = player_t::composite_attack_power();

  if ( talents.mental_dexterity )
  {
    ap += intellect() * talents.mental_dexterity / 3.0;
  }

  return ap;
}

// shaman_t::composite_attack_power_multiplier ==========================================

double shaman_t::composite_attack_power_multiplier() SC_CONST
{
  double multiplier = player_t::composite_attack_power_multiplier();

  if ( buffs_tier10_4pc_melee -> up() )
  {
    multiplier *= 1.20;
  }

  return multiplier;
}

// shaman_t::composite_spell_power ==========================================

double shaman_t::composite_spell_power( int school ) SC_CONST
{
  double sp = player_t::composite_spell_power( school );

  if ( talents.mental_quickness )
  {
    double ap = composite_attack_power_multiplier() * composite_attack_power();

    sp += ap * 0.30 * talents.mental_quickness / 3.0;
  }

  if ( main_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
  {
    sp += main_hand_weapon.buff_value;
  }
  if ( off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
  {
    sp += off_hand_weapon.buff_value;
  }

  sp += buffs_totem_of_wrath_glyph -> value();

  return sp;
}

// shaman_t::regen  =======================================================

void shaman_t::regen( double periodicity )
{
  player_t::regen( periodicity );

  if ( buffs_water_shield -> up() )
  {
    double water_shield_regen = periodicity * buffs_water_shield -> value() / 5.0;

    resource_gain( RESOURCE_MANA, water_shield_regen, gains_water_shield );
  }
}

// shaman_t::combat_begin =================================================

void shaman_t::combat_begin()
{
  player_t::combat_begin();

  if ( talents.elemental_oath ) sim -> auras.elemental_oath -> trigger();

  int ur = util_t::talent_rank( talents.unleashed_rage, 3, 4, 7, 10 );

  if ( ur >= sim -> auras.unleashed_rage -> current_value )
  {
    sim -> auras.unleashed_rage -> trigger( 1, ur );
  }
}

// shaman_t::primary_tree =================================================

int shaman_t::primary_tree() SC_CONST
{
  if ( talents.dual_wield ) return TREE_ENHANCEMENT;
  if ( talents.earth_shield || talents.mana_tide_totem ) return TREE_RESTORATION;
  return TREE_ELEMENTAL;
}

// shaman_t::get_talent_trees ==============================================

std::vector<talent_translation_t>& shaman_t::get_talent_list()
{
  if(talent_list.empty())
  {
          talent_translation_t translation_table[][MAX_TALENT_TREES] =
        {
    { {  1, 5, &( talents.convection            ) }, {  1, 3, &( talents.enhancing_totems          ) }, {  1, 0, NULL                                  } },
    { {  2, 5, &( talents.concussion            ) }, {  2, 0, NULL                                   }, {  2, 5, &( talents.totemic_focus            ) } },
    { {  3, 3, &( talents.call_of_flame         ) }, {  3, 5, &( talents.ancestral_knowledge       ) }, {  3, 0, NULL                                  } },
    { {  4, 0, NULL                               }, {  4, 0, NULL                                   }, {  4, 0, NULL                                  } },
    { {  5, 3, &( talents.elemental_devastation ) }, {  5, 5, &( talents.thundering_strikes        ) }, {  5, 0, NULL                                  } },
    { {  6, 5, &( talents.reverberation         ) }, {  6, 0, NULL                                   }, {  6, 0, NULL                                  } },
    { {  7, 1, &( talents.elemental_focus       ) }, {  7, 3, &( talents.improved_shields          ) }, {  7, 0, NULL                                  } },
    { {  8, 5, &( talents.elemental_fury        ) }, {  8, 3, &( talents.elemental_weapons         ) }, {  8, 0, NULL                                  } },
    { {  9, 0, &( talents.improved_fire_nova    ) }, {  9, 1, &( talents.shamanistic_focus         ) }, {  9, 0, NULL                                  } },
    { { 10, 0, NULL                               }, { 10, 0, NULL                                   }, { 10, 3, &( talents.restorative_totems       ) } },
    { { 11, 0, NULL                               }, { 11, 5, &( talents.flurry                    ) }, { 11, 5, &( talents.tidal_mastery            ) } },
    { { 12, 1, &( talents.call_of_thunder       ) }, { 12, 0, NULL                                   }, { 12, 0, NULL                                  } },
    { { 13, 3, &( talents.unrelenting_storm     ) }, { 13, 2, &( talents.improved_windfury_totem   ) }, { 13, 1, &( talents.natures_swiftness        ) } },
    { { 14, 3, &( talents.elemental_precision   ) }, { 14, 1, &( talents.spirit_weapons            ) }, { 14, 0, NULL                                  } },
    { { 15, 5, &( talents.lightning_mastery     ) }, { 15, 3, &( talents.mental_dexterity          ) }, { 15, 0, NULL                                  } },
    { { 16, 1, &( talents.elemental_mastery     ) }, { 16, 3, &( talents.unleashed_rage            ) }, { 16, 0, NULL                                  } },
    { { 17, 3, &( talents.storm_earth_and_fire  ) }, { 17, 3, &( talents.weapon_mastery            ) }, { 17, 1, &( talents.mana_tide_totem          ) } },
    { { 18, 2, &( talents.booming_echoes        ) }, { 18, 2, &( talents.frozen_power              ) }, { 18, 0, NULL                                  } },
    { { 19, 2, &( talents.elemental_oath        ) }, { 19, 3, &( talents.dual_wield_specialization ) }, { 19, 2, &( talents.blessing_of_the_eternals ) } },
    { { 20, 3, &( talents.lightning_overload    ) }, { 20, 1, &( talents.dual_wield                ) }, { 20, 0, NULL                                  } },
    { { 21, 0, NULL                               }, { 21, 1, &( talents.stormstrike               ) }, { 21, 0, NULL                                  } },
    { { 22, 1, &( talents.totem_of_wrath        ) }, { 22, 3, &( talents.static_shock              ) }, { 22, 0, NULL                                  } },
    { { 23, 3, &( talents.lava_flows            ) }, { 23, 1, &( talents.lava_lash                 ) }, { 23, 1, &( talents.earth_shield             ) } },
    { { 24, 5, &( talents.shamanism             ) }, { 24, 2, &( talents.improved_stormstrike      ) }, { 24, 0, NULL                                  } },
    { { 25, 1, &( talents.thunderstorm          ) }, { 25, 3, &( talents.mental_quickness          ) }, { 25, 0, NULL                                  } },
    { {  0, 0, NULL                               }, { 26, 1, &( talents.shamanistic_rage          ) }, { 26, 0, NULL                                  } },
    { {  0, 0, NULL                               }, { 27, 0, NULL                                   }, {  0, 0, NULL                                  } },
    { {  0, 0, NULL                               }, { 28, 5, &( talents.maelstrom_weapon          ) }, {  0, 0, NULL                                  } },
    { {  0, 0, NULL                               }, { 29, 1, &( talents.feral_spirit              ) }, {  0, 0, NULL                                  } },
    { {  0, 0, NULL                               }, {  0, 0, NULL                                   }, {  0, 0, NULL                                  } }
  };

    util_t::translate_talent_trees( talent_list, translation_table, sizeof( translation_table) );
  }
  return talent_list;
}

// shaman_t::get_options ================================================

std::vector<option_t>& shaman_t::get_options()
{
  if ( options.empty() )
  {
    player_t::get_options();

    option_t shaman_options[] =
    {
      // @option_doc loc=player/shaman/talents title="Talents"
      { "ancestral_knowledge",       OPT_INT,  &( talents.ancestral_knowledge       ) },
      { "blessing_of_the_eternals",  OPT_INT,  &( talents.blessing_of_the_eternals  ) },
      { "booming_echoes",            OPT_INT,  &( talents.booming_echoes            ) },
      { "call_of_flame",             OPT_INT,  &( talents.call_of_flame             ) },
      { "call_of_thunder",           OPT_INT,  &( talents.call_of_thunder           ) },
      { "concussion",                OPT_INT,  &( talents.concussion                ) },
      { "convection",                OPT_INT,  &( talents.convection                ) },
      { "dual_wield",                OPT_INT,  &( talents.dual_wield                ) },
      { "dual_wield_specialization", OPT_INT,  &( talents.dual_wield_specialization ) },
      { "elemental_devastation",     OPT_INT,  &( talents.elemental_devastation     ) },
      { "elemental_focus",           OPT_INT,  &( talents.elemental_focus           ) },
      { "elemental_fury",            OPT_INT,  &( talents.elemental_fury            ) },
      { "elemental_mastery",         OPT_INT,  &( talents.elemental_mastery         ) },
      { "elemental_oath",            OPT_INT,  &( talents.elemental_oath            ) },
      { "elemental_precision",       OPT_INT,  &( talents.elemental_precision       ) },
      { "elemental_weapons",         OPT_INT,  &( talents.elemental_weapons         ) },
      { "enhancing_totems",          OPT_INT,  &( talents.enhancing_totems          ) },
      { "feral_spirit",              OPT_INT,  &( talents.feral_spirit              ) },
      { "flurry",                    OPT_INT,  &( talents.flurry                    ) },
      { "frozen_power",              OPT_INT,  &( talents.frozen_power              ) },
      { "improved_fire_nova",        OPT_INT,  &( talents.improved_fire_nova        ) },
      { "improved_fire_nova_totem",  OPT_INT,  &( talents.improved_fire_nova        ) },
      { "improved_shields",          OPT_INT,  &( talents.improved_shields          ) },
      { "improved_stormstrike",      OPT_INT,  &( talents.improved_stormstrike      ) },
      { "improved_windfury_totem",   OPT_INT,  &( talents.improved_windfury_totem   ) },
      { "lava_flows",                OPT_INT,  &( talents.lava_flows                ) },
      { "lava_lash",                 OPT_INT,  &( talents.lava_lash                 ) },
      { "lightning_mastery",         OPT_INT,  &( talents.lightning_mastery         ) },
      { "lightning_overload",        OPT_INT,  &( talents.lightning_overload        ) },
      { "maelstrom_weapon",          OPT_INT,  &( talents.maelstrom_weapon          ) },
      { "mana_tide_totem",           OPT_INT,  &( talents.mana_tide_totem           ) },
      { "mental_dexterity",          OPT_INT,  &( talents.mental_dexterity          ) },
      { "mental_quickness",          OPT_INT,  &( talents.mental_quickness          ) },
      { "natures_swiftness",         OPT_INT,  &( talents.natures_swiftness         ) },
      { "restorative_totems",        OPT_INT,  &( talents.restorative_totems        ) },
      { "reverberation",             OPT_INT,  &( talents.reverberation             ) },
      { "shamanism",                 OPT_INT,  &( talents.shamanism                 ) },
      { "shamanistic_focus",         OPT_INT,  &( talents.shamanistic_focus         ) },
      { "shamanistic_rage",          OPT_INT,  &( talents.shamanistic_rage          ) },
      { "spirit_weapons",            OPT_INT,  &( talents.spirit_weapons            ) },
      { "static_shock",              OPT_INT,  &( talents.static_shock              ) },
      { "stormstrike",               OPT_INT,  &( talents.stormstrike               ) },
      { "storm_earth_and_fire",      OPT_INT,  &( talents.storm_earth_and_fire      ) },
      { "toughness",                 OPT_INT,  &( talents.toughness                 ) },
      { "thundering_strikes",        OPT_INT,  &( talents.thundering_strikes        ) },
      { "thunderstorm",              OPT_INT,  &( talents.thunderstorm              ) },
      { "tidal_mastery",             OPT_INT,  &( talents.tidal_mastery             ) },
      { "totem_of_wrath",            OPT_INT,  &( talents.totem_of_wrath            ) },
      { "totemic_focus",             OPT_INT,  &( talents.totemic_focus             ) },
      { "unrelenting_storm",         OPT_INT,  &( talents.unrelenting_storm         ) },
      { "unleashed_rage",            OPT_INT,  &( talents.unleashed_rage            ) },
      { "weapon_mastery",            OPT_INT,  &( talents.weapon_mastery            ) },
      // @option_doc loc=player/druid/misc title="Misc"
 	    { "totem",                     OPT_STRING, &( items[ SLOT_RANGED ].options_str ) },
      { NULL, OPT_UNKNOWN, NULL }
    };

    option_t::copy( options, shaman_options );
  }

  return options;
}

// shaman_t::decode_set ====================================================

int shaman_t::decode_set( item_t& item )
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

  bool is_caster = ( strstr( s, "helm"     ) ||
                     strstr( s, "shoulderpads" ) ||
                     strstr( s, "hauberk"      ) ||
                   ( strstr( s, "kilt"         ) && !strstr( s, "warkilt" ) ) ||
                     strstr( s, "gloves"       ) );

  bool is_melee = ( strstr( s, "faceguard"      ) ||
                    strstr( s, "shoulderguards" ) ||
                    strstr( s, "chestguard"     ) ||
                    strstr( s, "warkilt"        ) ||
                    strstr( s, "grips"          ) );

  if ( strstr( s, "earthshatter" ) )
  {
    if ( is_melee  ) return SET_T7_MELEE;
    if ( is_caster ) return SET_T7_CASTER;
  }
  if ( strstr( s, "worldbreaker" ) )
  {
    if ( is_melee  ) return SET_T8_MELEE;
    if ( is_caster ) return SET_T8_CASTER;
  }
  if ( strstr( s, "nobundos" ) ||
       strstr( s, "thralls"  ) )
  {
    if ( is_melee  ) return SET_T9_MELEE;
    if ( is_caster ) return SET_T9_CASTER;
  }
  if ( strstr( s, "frost_witch" ) )
  {
    if ( is_caster ) return SET_T10_CASTER;
    if ( is_melee  ) return SET_T10_MELEE;
  }

  return SET_NONE;
}

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_shaman  =================================================

player_t* player_t::create_shaman( sim_t* sim, const std::string& name, int race_type )
{
  return new shaman_t( sim, name, race_type );
}

// player_t::shaman_init ====================================================

void player_t::shaman_init( sim_t* sim )
{
  sim -> auras.elemental_oath    = new aura_t( sim, "elemental_oath",    1, 0.0 );
  sim -> auras.flametongue_totem = new aura_t( sim, "flametongue_totem", 1, 300.0 );
  sim -> auras.mana_spring_totem = new aura_t( sim, "mana_spring_totem", 1, 300.0 );
  sim -> auras.strength_of_earth = new aura_t( sim, "strength_of_earth", 1, 300.0 );
  sim -> auras.totem_of_wrath    = new aura_t( sim, "totem_of_wrath",    1, 300.0 );
  sim -> auras.unleashed_rage    = new aura_t( sim, "unleashed_rage",    1, 0.0 );
  sim -> auras.windfury_totem    = new aura_t( sim, "windfury_totem",    1, 300.0 );
  sim -> auras.wrath_of_air      = new aura_t( sim, "wrath_of_air",      1, 300.0 );

  sim -> target -> debuffs.totem_of_wrath = new debuff_t( sim, "totem_of_wrath_debuff" );

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    p -> buffs.bloodlust = new buff_t( p, "bloodlust", 1, 40.0, 600.0 );
  }
}

// player_t::shaman_combat_begin ============================================

void player_t::shaman_combat_begin( sim_t* sim )
{
  if ( sim -> overrides.totem_of_wrath )
  {
    sim -> auras.totem_of_wrath -> override( 1, 280.0 );
    sim -> target -> debuffs.totem_of_wrath -> override();
  }
  if ( sim -> overrides.mana_spring_totem ) sim -> auras.mana_spring_totem -> override( 1, 91.0 * 1.2 );
  if ( sim -> overrides.flametongue_totem ) sim -> auras.flametongue_totem -> override( 1, 144 * 1.15 );
  if ( sim -> overrides.   windfury_totem ) sim -> auras.   windfury_totem -> override( 1, 0.20 );
  if ( sim -> overrides.strength_of_earth ) sim -> auras.strength_of_earth -> override( 1, 155 * 1.15 );
  if ( sim -> overrides.wrath_of_air      ) sim -> auras.wrath_of_air      -> override( 1, 0.20 );
  if ( sim -> overrides.elemental_oath    ) sim -> auras.elemental_oath    -> override();
  if ( sim -> overrides.unleashed_rage    ) sim -> auras.unleashed_rage    -> override( 1, 10 );
}

