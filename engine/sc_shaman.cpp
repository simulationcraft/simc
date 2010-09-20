// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

// ==========================================================================
// TODO
// ==========================================================================
// Spirit Walker's Grace Ability
// Unleash Elements Ability
// Ability/Spell Rank scaling
// Maelstrom's mana reduction (what is it?)
// Mail Specialization -> 5% intel or agi bonus (waiting on sim changes to check armor type)
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
  action_t* active_fire_totem;
  action_t* active_flame_shock;
  action_t* active_lightning_charge;
  action_t* active_lightning_bolt_dot;
  action_t* active_searing_flames_dot;

  // Buffs
  buff_t* buffs_elemental_devastation;
  buff_t* buffs_elemental_focus;
  buff_t* buffs_elemental_mastery;
  buff_t* buffs_flurry;
  buff_t* buffs_lightning_shield;
  buff_t* buffs_maelstrom_weapon;
  buff_t* buffs_natures_swiftness;
  buff_t* buffs_searing_flames;
  buff_t* buffs_shamanistic_rage;
  buff_t* buffs_stormstrike;
  buff_t* buffs_tier10_2pc_melee;
  buff_t* buffs_tier10_4pc_melee;
  buff_t* buffs_water_shield;

  // Cooldowns
  cooldown_t* cooldowns_elemental_mastery;
  cooldown_t* cooldowns_lava_burst;
  cooldown_t* cooldowns_shock;
  cooldown_t* cooldowns_strike;
  cooldown_t* cooldowns_windfury_weapon;

  // Gains
  gain_t* gains_primal_wisdom;
  gain_t* gains_rolling_thunder;
  gain_t* gains_telluric_currents;
  gain_t* gains_thunderstorm;
  gain_t* gains_water_shield;

  // Procs
  proc_t* procs_elemental_overload;
  proc_t* procs_lava_surge;
  proc_t* procs_rolling_thunder;
  proc_t* procs_windfury;

  // Random Number Generators
  rng_t* rng_elemental_overload;
  rng_t* rng_lava_surge;
  rng_t* rng_primal_wisdom;
  rng_t* rng_rolling_thunder;
  rng_t* rng_searing_flames;
  rng_t* rng_static_shock;
  rng_t* rng_windfury_weapon;

  // Talents

  // Elemental
  talent_t* talent_acuity;
  talent_t* talent_call_of_flame;
  talent_t* talent_concussion;
  talent_t* talent_convection;
  talent_t* talent_elemental_focus;
  talent_t* talent_elemental_mastery;
  talent_t* talent_elemental_oath;
  talent_t* talent_elemental_precision;
  talent_t* talent_feedback;
  talent_t* talent_fulmination;
  talent_t* talent_lava_flows;
  talent_t* talent_lava_surge;
  talent_t* talent_reverberation;
  talent_t* talent_rolling_thunder;
  talent_t* talent_totemic_wrath;

  // Enhancement
  talent_t* talent_elemental_devastation;
  talent_t* talent_elemental_weapons;
  talent_t* talent_feral_spirit;
  talent_t* talent_flurry;
  talent_t* talent_focused_strikes;  
  talent_t* talent_frozen_power;
  talent_t* talent_improved_fire_nova;
  talent_t* talent_improved_lava_lash;
  talent_t* talent_improved_shields;
  talent_t* talent_maelstrom_weapon;
  talent_t* talent_searing_flames;
  talent_t* talent_shamanistic_rage;
  talent_t* talent_static_shock;
  talent_t* talent_stormstrike;
  talent_t* talent_toughness;
  talent_t* talent_unleashed_rage;

  // Restoration
  talent_t* talent_mana_tide_totem;
  talent_t* talent_natures_swiftness;
  talent_t* talent_telluric_currents;
  talent_t* talent_totemic_focus;

  // Weapon Enchants
  attack_t* windfury_weapon_attack;
  spell_t*  flametongue_weapon_spell;

  struct glyphs_t
  {
    // Prime
    int feral_spirit;
    int fire_elemental_totem;
    int flame_shock;
    int flametongue_weapon;
    int lava_burst;
    int lava_lash;
    int lightning_bolt;
    int stormstrike;
    int water_shield;
    int windfury_weapon;

    // Major
    int chain_lightning;
    int shocking;
    int thunder;

    // Minor
    int thunderstorm;
   
    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  shaman_t( sim_t* sim, const std::string& name, race_type r = RACE_NONE ) : player_t( sim, SHAMAN, name, r )
  {
    tree_type[ SHAMAN_ELEMENTAL   ] = TREE_ELEMENTAL;
    tree_type[ SHAMAN_ENHANCEMENT ] = TREE_ENHANCEMENT;
    tree_type[ SHAMAN_RESTORATION ] = TREE_RESTORATION;

    // Active
    active_flame_shock        = 0;
    active_lightning_charge   = 0;
    active_lightning_bolt_dot = 0;
    active_searing_flames_dot = 0;

    // Cooldowns
    cooldowns_elemental_mastery = get_cooldown( "elemental_mastery" );
    cooldowns_lava_burst        = get_cooldown( "lava_burst"        );
    cooldowns_shock             = get_cooldown( "shock"             );
    cooldowns_strike            = get_cooldown( "strike"            );
    cooldowns_windfury_weapon   = get_cooldown( "windfury_weapon"   );

    // Talents

    // Elemental
    talent_acuity                   = new talent_t( this, "acuity", "Acuity" ); 
    talent_call_of_flame            = new talent_t( this, "call_of_flame", "Call of Flame" );
    talent_concussion               = new talent_t( this, "concussion", "Concussion" );
    talent_convection               = new talent_t( this, "convection", "Convection" );
    talent_elemental_focus          = new talent_t( this, "elemental_focus", "Elemental Focus" );
    talent_elemental_mastery        = new talent_t( this, "elemental_mastery", "Elemental Mastery" );
    talent_elemental_oath           = new talent_t( this, "elemental_oath", "Elemental Oath" );
    talent_elemental_precision      = new talent_t( this, "elemental_precision", "Elemental Precision" );
    talent_feedback                 = new talent_t( this, "feedback", "Feedback" );
    talent_fulmination              = new talent_t( this, "fulmination", "Fulmination" );
    talent_improved_fire_nova       = new talent_t( this, "improved_fire_nova", "Improved Fire Nova" );
    talent_lava_flows               = new talent_t( this, "lava_flows", "Lava Flows" );
    talent_lava_surge               = new talent_t( this, "lava_surge", "Lava Surge" );
    talent_reverberation            = new talent_t( this, "reverberation", "Reverberation" );
    talent_rolling_thunder          = new talent_t( this, "rolling_thunder", "Rolling Thunder" );
    talent_totemic_wrath            = new talent_t( this, "totemic_wrath", "Totemic Wrath" );

    // Enhancement
    talent_elemental_devastation    = new talent_t( this, "elemental_devastation", "Elemental Devastation" );
    talent_elemental_weapons        = new talent_t( this, "elemental_weapons", "Elemental Weapons" );
    talent_feral_spirit             = new talent_t( this, "feral_spirit", "Feral Spirit" );
    talent_flurry                   = new talent_t( this, "flurry", "Flurry" );
    talent_focused_strikes          = new talent_t( this, "focused_strikes", "Focused Strikes" );
    talent_frozen_power             = new talent_t( this, "frozen_power", "Frozen Power" );
    talent_improved_lava_lash       = new talent_t( this, "improved_lava_lash", "Improved Lava Lash" );
    talent_improved_shields         = new talent_t( this, "improved_shields", "Improved Shields" );    
    talent_maelstrom_weapon         = new talent_t( this, "maelstrom_weapon", "Maelstrom Weapon" );
    talent_searing_flames           = new talent_t( this, "searing_flames", "Searing Flames" );
    talent_shamanistic_rage         = new talent_t( this, "shamanistic_rage", "Shamanistic Rage" );
    talent_static_shock             = new talent_t( this, "static_shock", "Static Shock" );
    talent_stormstrike              = new talent_t( this, "stormstrike", "Stormstrike" );
    talent_toughness                = new talent_t( this, "toughness", "Toughness" );
    talent_unleashed_rage           = new talent_t( this, "unleashed_rage", "Unleashed Rage" );

    // Restoration
    talent_mana_tide_totem          = new talent_t( this, "mana_tide_totem", "Mana Tide Totem" );
    talent_natures_swiftness        = new talent_t( this, "natures_swiftness", "Nature's Swiftness" );
    talent_telluric_currents        = new talent_t( this, "telluric_currents", "Telluric Currents" );
    talent_totemic_focus            = new talent_t( this, "totemic_focus", "Totemic Focus" );

    // Weapon Enchants
    windfury_weapon_attack   = 0;
    flametongue_weapon_spell = 0;
  }

  // Character Definition
  virtual void      init_rating();
  virtual void      init_glyphs();
  virtual void      init_race();
  virtual void      init_base();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      interrupt();
  virtual void      clear_debuffs();
  virtual double    composite_attack_power() SC_CONST;
  virtual double    composite_attack_power_multiplier() SC_CONST;
  virtual double    composite_spell_hit() SC_CONST;
  virtual double    composite_spell_power( int school ) SC_CONST;
  virtual std::vector<talent_translation_t>& get_talent_list();
  virtual std::vector<option_t>& get_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name );
  virtual void      create_pets();
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST { return RESOURCE_MANA; }
  virtual int       primary_role() SC_CONST     { return talent_stormstrike -> rank() ? ROLE_HYBRID : ROLE_SPELL; }
  virtual talent_tree_type       primary_tree() SC_CONST;
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
    if ( p -> primary_tree() == TREE_ENHANCEMENT ) base_hit += 0.06;
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

// trigger_elemental_overload ===============================================

static void trigger_elemental_overload( spell_t* s,
                                        stats_t* elemental_overload_stats )
{
  shaman_t* p = s -> player -> cast_shaman();

  if ( p -> primary_tree() != TREE_ELEMENTAL ) return;

  if ( s -> proc ) return;

  double overload_chance = p -> composite_mastery() * 0.02;
  
  if ( p -> rng_elemental_overload -> roll( overload_chance ) )
  {
    p -> procs_elemental_overload -> occur();

    double   cost             = s -> base_cost;
    double   multiplier       = s -> base_multiplier;
    double   direct_power_mod = s -> direct_power_mod;
    stats_t* stats            = s -> stats;

    s -> proc                 = true;
    s -> pseudo_pet           = true; // Prevent Honor Among Thieves
    s -> base_cost            = 0;
    s -> base_multiplier     /= 1.0 / 0.75;
    s -> direct_power_mod    += 0.20; // Reapplied here because Shamanism isn't affected by the *0.20.
    s -> stats                = elemental_overload_stats;

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
      base_multiplier *= 1.0 + p -> talent_elemental_weapons -> rank() * 0.20;
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

  if ( p -> talent_maelstrom_weapon -> rank() == 0 ) return;

  double chance = a -> weapon -> proc_chance_on_swing( p -> talent_maelstrom_weapon -> rank() * 2.0 );
  bool can_increase = p -> buffs_maelstrom_weapon -> check() <  p -> buffs_maelstrom_weapon -> max_stack;

  if ( p -> set_bonus.tier8_4pc_melee() ) chance *= 1.20;

  p -> buffs_maelstrom_weapon -> trigger( 1, 1, chance );

  if ( p -> set_bonus.tier10_4pc_melee() &&
       ( can_increase && p -> buffs_maelstrom_weapon -> stack() == p -> buffs_maelstrom_weapon -> max_stack ) )
    p -> buffs_tier10_4pc_melee -> trigger();
}

// trigger_fulmination ==============================================

static void trigger_fulmination ( spell_t* s )
{
  shaman_t* p = s -> player -> cast_shaman();
  
  if ( p -> talent_fulmination -> rank() == 0 ) return;

  int consuming_stacks = p -> buffs_lightning_shield -> stack() - 3;
  if ( consuming_stacks <= 0 ) return;

  p -> active_lightning_charge -> execute();
  p -> buffs_lightning_shield  -> decrement( consuming_stacks );
}

// trigger_primal_wisdom =============================================

static void trigger_primal_wisdom ( attack_t* a )
{
  shaman_t* p = a -> player -> cast_shaman();

  if ( p -> rng_primal_wisdom -> roll( p -> primary_tree() == TREE_ENHANCEMENT * 0.40 ) )
  {
    p -> resource_gain( RESOURCE_MANA, 0.05 * p -> resource_base[ RESOURCE_MANA ], p -> gains_primal_wisdom );
  }
}

// trigger_rolling_thunder ==============================================

static void trigger_rolling_thunder ( spell_t* s )
{
  shaman_t* p = s -> player -> cast_shaman();
  
  if ( p -> rng_rolling_thunder -> roll( p -> talent_rolling_thunder -> rank() * 0.30 ) && p -> buffs_lightning_shield -> check() )
  {
    p -> resource_gain( RESOURCE_MANA, 0.02 * p -> resource_max[ RESOURCE_MANA ], p -> gains_rolling_thunder );
    p -> buffs_lightning_shield -> trigger();
    p -> procs_rolling_thunder  -> occur();
  }
}

// trigger_searing_flames ===============================================

static void trigger_searing_flames( spell_t* s )
{
  struct searing_flames_dot_t : public shaman_spell_t
  {
    searing_flames_dot_t( player_t* player ) : 
      shaman_spell_t( "searing_flames", player, SCHOOL_FIRE, TREE_ENHANCEMENT )
    {
      trigger_gcd     = 0;
      background      = true;
      may_miss        = false;      
      proc            = true;      
      base_tick_time  = 3.0;
      num_ticks       = 5;
      dot_behavior    = DOT_REFRESH;
    }
    void player_buff() {}
    void target_debuff( int dmg_type ) {}

    virtual void last_tick()
    {
      shaman_t* p = player -> cast_shaman();
      shaman_spell_t::last_tick();
      p -> buffs_searing_flames -> expire();
    }
  };

  shaman_t* p = s -> player -> cast_shaman();
  
  if ( ! p -> rng_searing_flames -> roll ( p -> talent_searing_flames -> rank() / 3.0 ) ) return;

  p -> buffs_searing_flames -> trigger();

  if ( ! p -> active_searing_flames_dot ) 
    p -> active_searing_flames_dot = new searing_flames_dot_t ( p );

  // Searing flame dot treats all hits as.. HITS
  double new_base_td = s -> direct_dmg /= 1.0 + s -> total_crit_bonus();

  p -> active_searing_flames_dot -> base_td = new_base_td / 5.0 * p -> buffs_searing_flames -> stack();
  p -> active_searing_flames_dot -> execute();
}

// trigger_static_shock =============================================

static void trigger_static_shock ( attack_t* a )
{
  shaman_t* p = a -> player -> cast_shaman();

  if ( p -> talent_static_shock -> rank() && p -> buffs_lightning_shield -> stack() )
  {
    double chance = p -> talent_static_shock -> rank() * 0.15 + p -> set_bonus.tier9_2pc_melee() * 0.03;
    if ( p -> rng_static_shock -> roll( chance ) )
      p -> active_lightning_charge -> execute();
  }
}

// trigger_telluric_currents =============================================

static void trigger_telluric_currents ( spell_t* s )
{
  shaman_t* p = s -> player -> cast_shaman();

  if ( p -> talent_telluric_currents -> rank() )
  {
    double mana_gain = s -> direct_dmg * ( 0.20 * p -> talent_telluric_currents -> rank() );
    p -> resource_gain( RESOURCE_MANA, mana_gain, p -> gains_telluric_currents );
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

    stack_maelstrom_weapon( this );
    trigger_flametongue_weapon( this );
    trigger_primal_wisdom( this );
    trigger_windfury_weapon( this );    
  }
}

// shaman_attack_t::player_buff ============================================

void shaman_attack_t::player_buff()
{
  attack_t::player_buff();
  shaman_t* p = player -> cast_shaman();
  if ( p -> buffs_elemental_devastation -> up() )
  {
    player_crit += p -> talent_elemental_devastation -> rank() * 0.03;
  }

  if ( p -> buffs_tier10_2pc_melee -> up() )
    player_multiplier *= 1.12;
}

// shaman_attack_t::assess_damage ==========================================

void shaman_attack_t::assess_damage( double amount,
                                     int    dmg_type )
{
  attack_t::assess_damage( amount, dmg_type );
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
      h *= 1.0 / ( 1.0 + 0.05 * ( p -> talent_flurry -> rank() + 1 + p -> set_bonus.tier7_4pc_melee() ) );
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
      if( p -> primary_tree() != TREE_ENHANCEMENT ) return;
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

    if ( p -> primary_tree() != TREE_ENHANCEMENT ) return;
    weapon = &( player -> off_hand_weapon );
    if ( weapon -> type == WEAPON_NONE ) background = true; // Do not allow execution.
    
    may_crit    = true;
    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.04;
    cooldown -> duration = 10;
    weapon_multiplier *= 1.5;

    if ( p -> set_bonus.tier8_2pc_melee() ) base_multiplier *= 1.0 + 0.20;
   
    id = 60103;
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_attack_t::execute();
    trigger_static_shock( this );
    p -> buffs_searing_flames -> expire();
    if ( p -> active_searing_flames_dot ) 
      p -> active_searing_flames_dot -> cancel();
  }

  virtual void player_buff()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_attack_t::player_buff();
    if ( weapon -> buff_type == FLAMETONGUE_IMBUE )
    {
      player_multiplier *= 1.40;
    }
    player_multiplier *= 1 + ( p -> talent_improved_lava_lash -> rank() * 0.15 ) +
                             ( p -> glyphs.lava_lash                  * 0.20 ) +
                             ( p -> talent_improved_lava_lash -> rank() * 0.10 * p -> buffs_searing_flames -> stack() );
    }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();

    if ( wf_cd_only )
      if ( p -> cooldowns_windfury_weapon -> remains() > 0 )
        return false;

    return shaman_attack_t::ready();
  }
    
  double cost() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();
    double c = attack_t::cost();
    if ( p -> buffs_shamanistic_rage -> up() )
      return 0;
    return c;
  }
};

// Primal Strike Attack =======================================================

struct primal_strike_t : public shaman_attack_t
{
  primal_strike_t( player_t* player, const std::string& options_str ) :
      shaman_attack_t( "primal_strike", player, SCHOOL_PHYSICAL, TREE_ENHANCEMENT )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 85,  5, 178, 178, 0, .08 },
      { 80,  1, 160, 160, 0, .08 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 73899 );

    may_crit  = true;

    cooldown = p -> cooldowns_strike;
    cooldown -> duration = 8.0;

    base_multiplier *= 1 + p -> talent_focused_strikes -> rank() * 0.15;;
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();

    attack_t::consume_resource();
    update_ready();

    weapon = &( p -> main_hand_weapon );
    shaman_attack_t::execute();

    trigger_static_shock( this );
  }

  virtual void consume_resource() {}
    
  double cost() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();
    double c = attack_t::cost();
    if ( c == 0 ) return 0;
    if ( p -> buffs_shamanistic_rage -> up() )
      return 0;
    return c;
  }
};

// Stormstrike Attack =======================================================

struct stormstrike_t : public shaman_attack_t
{
  stormstrike_t( player_t* player, const std::string& options_str ) :
      shaman_attack_t( "stormstrike", player, SCHOOL_PHYSICAL, TREE_ENHANCEMENT )
  {
    shaman_t* p = player -> cast_shaman();
    check_talent( p -> talent_stormstrike -> rank() );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    may_crit    = true;
    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.08;
   
    weapon_multiplier *= 1.25;
    base_multiplier   *= 1 + p -> talent_focused_strikes -> rank() * 0.15;

    if ( p -> set_bonus.tier8_2pc_melee() ) base_multiplier *= 1.0 + 0.20;

    cooldown = p -> cooldowns_strike;
    cooldown -> duration = 8.0;

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
      p -> buffs_stormstrike -> trigger();

      if ( p -> off_hand_weapon.type != WEAPON_NONE )
      {
        weapon = &( p -> off_hand_weapon );
        shaman_attack_t::execute();
      }
    }

    trigger_static_shock( this );
  }

  virtual void consume_resource() {}

  double cost() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();
    double c = attack_t::cost();
    if ( p -> buffs_shamanistic_rage -> up() )
      return 0;
    return c;
  }
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
    h *= 1.0 / 1.20;
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
    player_multiplier *= 1.0 + p -> talent_elemental_oath -> rank() * 0.05;
  }
  if ( p -> buffs_elemental_mastery -> up() && ( school == SCHOOL_FIRE || school == SCHOOL_FROST || school == SCHOOL_NATURE ) )
    player_multiplier *= 1.15;
  if ( school == SCHOOL_FIRE || school == SCHOOL_FROST || school == SCHOOL_NATURE )
    player_multiplier *= 1 + p -> talent_elemental_precision -> rank() * 0.01;
  if ( p -> primary_tree() == TREE_ENHANCEMENT && ( school == SCHOOL_FIRE || school == SCHOOL_FROST || school == SCHOOL_NATURE ) )
    player_multiplier *= 1 + ( p -> composite_mastery() * 0.025 );
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
  spell_t::schedule_execute();
}

// shaman_spell_t::assess_damage ============================================

void shaman_spell_t::assess_damage( double amount,
                                    int    dmg_type )
{
  spell_t::assess_damage( amount, dmg_type );
}

// =========================================================================
// Shaman Spells
// =========================================================================

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
    base_cost = p -> resource_base[ RESOURCE_MANA ] * 0.26;
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

// Chain Lightning Spell ===================================================

struct chain_lightning_t : public shaman_spell_t
{
  int      clearcasting;
  int      conserve;
  double   max_lvb_cd;
  int      maelstrom;
  stats_t* elemental_overload_stats;

  chain_lightning_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "chain_lightning", player, SCHOOL_NATURE, TREE_ELEMENTAL ),
      clearcasting( 0 ), conserve( 0 ), max_lvb_cd( 0 ), maelstrom( 0 ), elemental_overload_stats( 0 )
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
      { 85, 9, 1020, 1164, 0, 0.26 },
      { 80, 8,  973, 1111, 0, 0.26 },
      { 74, 7,  806,  920, 0, 0.26 },
      { 70, 6,  734,  838, 0,  760 },
      { 63, 5,  603,  687, 0,  650 },
      { 56, 4,  493,  551, 0,  550 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 49271 );

    base_execute_time    = 2.0;
    may_crit             = true;
    direct_power_mod     = ( base_execute_time / 3.5 );
    direct_power_mod    += p -> primary_tree() == TREE_ELEMENTAL * 0.20;
    base_execute_time   -= p -> primary_tree() == TREE_ELEMENTAL * 0.5;
    base_cost_reduction += p -> talent_convection -> rank() * 0.05;
    base_multiplier     *= 1.0 + p -> talent_concussion -> rank() * 0.02;

    if ( p -> glyphs.chain_lightning ) base_multiplier *= 0.90;

    base_crit_bonus_multiplier *= 1.0 + p -> primary_tree() == TREE_ELEMENTAL;

    cooldown -> duration  = 3.0;

    elemental_overload_stats = p -> get_stats( "elemental_overload" ); // Testing needed to see if this still suffers from the jump penalty
    elemental_overload_stats -> school = SCHOOL_NATURE;    
   }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_spell_t::execute();
    p -> buffs_maelstrom_weapon -> expire();
    p -> buffs_elemental_mastery -> current_value = 0;
    if ( p -> set_bonus.tier10_2pc_caster() )
      p -> cooldowns_elemental_mastery -> ready -= 2.0;
    p -> cooldowns_elemental_mastery -> ready -= p -> talent_feedback -> rank() * 1.0;
    if ( result_is_hit() )
    {
      trigger_elemental_overload( this, elemental_overload_stats  );
      trigger_rolling_thunder( this );
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

  virtual void player_buff()
  {
    shaman_spell_t::player_buff();
    shaman_t* p = player -> cast_shaman();
    if ( p -> buffs_stormstrike -> up() ) player_crit += 0.25;
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();

    if ( clearcasting > 0 && ! p -> buffs_elemental_focus -> check() )
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

  double cost() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();
    double c = spell_t::cost();
    if ( c == 0 ) return 0;
    if ( p -> buffs_shamanistic_rage -> up() )
      return 0;
    double cr = cost_reduction();
    if ( p -> buffs_elemental_focus -> check() ) cr += 0.40;
    c *= 1.0 - cr;
    if ( c < 0 ) c = 0;
    return c;
  }

  virtual void assess_damage( double amount,
                               int    dmg_type )
  {
    shaman_spell_t::assess_damage( amount, dmg_type );
    shaman_t* p = player -> cast_shaman();

    for ( int i=0; i < sim -> target -> adds_nearby && i < ( 2 + p -> glyphs.chain_lightning * 2 ); i ++ )
    {
      amount *= 0.70;
      shaman_spell_t::additional_damage( amount, dmg_type );
    }
  }
};

// Elemental Mastery Spell ==================================================

struct elemental_mastery_t : public shaman_spell_t
{
  elemental_mastery_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "elemental_mastery", player, SCHOOL_NATURE, TREE_ELEMENTAL )
  {
    shaman_t* p = player -> cast_shaman();
    check_talent( p -> talent_elemental_mastery -> rank() );
    trigger_gcd = 0;
    cooldown -> duration = 180.0;

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
      { 85, 10, 486, 542, 0, 0.22 }, 
      { 80,  9, 893, 997, 0, 0.22 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 61657 );

    aoe = true;
    pseudo_pet = true;
    may_crit = true;
    base_execute_time = 0;
    direct_power_mod  = 0.8 / 3.5;
    cooldown -> duration = 10.0;

    base_multiplier *= 1.0 + ( p -> talent_improved_fire_nova -> rank() * 0.10 +
			                         p -> talent_call_of_flame      -> rank() * 0.10 );

    base_crit_bonus_multiplier *= 1.0 + p -> primary_tree() == TREE_ELEMENTAL;

    cooldown -> duration -= p -> talent_improved_fire_nova -> rank() * 2.0;
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();

    if ( ! p -> active_fire_totem )
      return false;

    if ( p -> active_fire_totem -> name_str == "searing_totem" )
      return false;

    return shaman_spell_t::ready();
  }

  // Fire Nova doesn't benefit from Elemental Focus
  double cost() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();
    double c = spell_t::cost();
    if ( c == 0 ) return 0;
    if ( p -> buffs_shamanistic_rage -> up() )
      return 0;
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

// Lava Burst Spell =========================================================

struct lava_burst_t : public shaman_spell_t
{
  int      flame_shock;
  stats_t* elemental_overload_stats;

  lava_burst_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "lava_burst", player, SCHOOL_FIRE, TREE_ELEMENTAL ),
      flame_shock( 0 ), elemental_overload_stats( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "flame_shock",        OPT_BOOL, &flame_shock        },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 85, 3, 1267, 1615, 0, 0.10 },
      { 80, 2, 1136, 1448, 0, 0.10 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 51505 );

    may_crit             = true;
    base_execute_time    = 2.0;
    direct_power_mod     = base_execute_time / 3.5;
    direct_power_mod    += p -> glyphs.lava_burst ? 0.10 : 0.00;

    base_execute_time   -= p -> primary_tree() == TREE_ELEMENTAL * 0.50;
    base_cost_reduction += p -> talent_convection -> rank() * 0.05;
    base_multiplier     *= 1.0 + p -> talent_concussion    -> rank() * 0.03 + p -> talent_call_of_flame -> rank() * 0.05;
    direct_power_mod    += p -> primary_tree() == TREE_ELEMENTAL * 0.20;

    base_crit_bonus_multiplier *= 1.0 + ( util_t::talent_rank( p -> talent_lava_flows -> rank(), 3, 0.06, 0.12, 0.24 ) +
                                          ( p -> primary_tree() == TREE_ELEMENTAL * 1 ) + 
                                          ( p -> set_bonus.tier7_4pc_caster() ? 0.10 : 0.00 ) );

    cooldown -> duration = 8.0;

    if ( p -> set_bonus.tier9_4pc_caster() )
    {
      num_ticks      = 3;
      base_tick_time = 2.0;
      tick_may_crit  = false;
      tick_power_mod = 0.0;
    }

    elemental_overload_stats = p -> get_stats( "elemental_overload" );
    elemental_overload_stats -> school = SCHOOL_FIRE;
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
      trigger_elemental_overload( this, elemental_overload_stats );
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
    if ( p -> buffs_elemental_mastery -> value()      ) return 0;
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
    shaman_t* p = player -> cast_shaman();

    if ( flame_shock )
    {
      if ( ! p -> active_flame_shock )
        return false;

      double lvb_finish = sim -> current_time + execute_time();
      double fs_finish  = p -> active_flame_shock -> dot -> ready;
      if ( lvb_finish > fs_finish )
        return false;
    }

    return true;
  }
  
  double cost() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();
    double c = spell_t::cost();
    if ( c == 0 ) return 0;
    if ( p -> buffs_shamanistic_rage -> up() )
      return 0;
    double cr = cost_reduction();
    if ( p -> buffs_elemental_focus -> check() ) cr += 0.40;
    c *= 1.0 - cr;
    if ( c < 0 ) c = 0;
    return c;
  }
};

// Lightning Bolt Spell =====================================================

struct lightning_bolt_t : public shaman_spell_t
{
  int      maelstrom;
  stats_t* elemental_overload_stats;

  lightning_bolt_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "lightning_bolt", player, SCHOOL_NATURE, TREE_ELEMENTAL ),
      maelstrom( 0 ), elemental_overload_stats( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "maelstrom", OPT_INT,  &maelstrom  },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 85, 16, 719, 821, 0, 0.06 },
      { 80, 15, 645, 735, 0, 0.06 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 403 );

    base_execute_time    = 2.5;
    may_crit             = true;
    direct_power_mod     = ( base_execute_time / 3.5 );
    base_execute_time   -= p -> primary_tree() == TREE_ELEMENTAL    * 0.5;
    base_cost_reduction += p -> talent_convection          -> rank()  * 0.05;
    base_multiplier     *= 1.0 + p -> talent_concussion    -> rank()  * 0.02  + ( p -> glyphs.lightning_bolt != 0 ? 0.04 : 0.0 );
    direct_power_mod    += p -> primary_tree() == TREE_ELEMENTAL    * 0.20;

    base_crit_bonus_multiplier *= 1.0 + p -> primary_tree() == TREE_ELEMENTAL;

    if ( p -> set_bonus.tier6_4pc_caster() ) base_multiplier     *= 1.05;
    if ( p -> set_bonus.tier7_2pc_caster() ) base_cost_reduction += 0.05;

    elemental_overload_stats = p -> get_stats( "elemental_overload" );
    elemental_overload_stats -> school = SCHOOL_NATURE;   
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
    p -> cooldowns_elemental_mastery -> ready -= p -> talent_feedback -> rank() * 1.0;
    if ( result_is_hit() )
    {
      trigger_elemental_overload( this, elemental_overload_stats );
      trigger_rolling_thunder( this );
      trigger_telluric_currents( this );
      if ( result == RESULT_CRIT )
      {
        trigger_tier8_4pc_elemental( this );
      }      
    }
  }

  virtual double execute_time() SC_CONST
  {
    double t = shaman_spell_t::execute_time();
    shaman_t* p = player -> cast_shaman();
    if ( p -> buffs_elemental_mastery -> value()      ) return 0;
    if ( p -> buffs_maelstrom_weapon  -> stack() == 5 ) return 0;
    t *= 1.0 - p -> buffs_maelstrom_weapon -> stack() * 0.20;
    return t;
  }
  
  virtual void player_buff()
  {
    shaman_spell_t::player_buff();
    shaman_t* p = player -> cast_shaman();
    if ( p -> buffs_stormstrike -> up() ) player_crit += 0.25;
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();

    if ( ! shaman_spell_t::ready() )
      return false;

    if ( maelstrom > 0 )
      if( maelstrom > p -> buffs_maelstrom_weapon -> current_stack )
        return false;

    return true;
  }
  
  double cost() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();
    double c = spell_t::cost();
    if ( c == 0 ) return 0;
    if ( p -> buffs_shamanistic_rage -> up() )
      return 0;
    double cr = cost_reduction();
    if ( p -> buffs_elemental_focus -> check() ) cr += 0.40;
    c *= 1.0 - cr;
    if ( c < 0 ) c = 0;
    return c;
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
    check_talent( p -> talent_natures_swiftness -> rank() );
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

// Shamanisitc Rage Spell ===========================================================

struct shamanistic_rage_t : public shaman_spell_t
{
  bool tier10_2pc;

  shamanistic_rage_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "shamanistic_rage", player, SCHOOL_NATURE, TREE_ENHANCEMENT ),
      tier10_2pc( false )
  {
    shaman_t* p = player -> cast_shaman();
    check_talent( p -> talent_shamanistic_rage -> rank() );

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

// Spirit Wolf Spell ==========================================================

struct spirit_wolf_spell_t : public shaman_spell_t
{
  spirit_wolf_spell_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "spirit_wolf", player, SCHOOL_NATURE, TREE_ENHANCEMENT )
  {
    shaman_t* p = player -> cast_shaman();
    check_talent( p -> talent_feral_spirit -> rank() );

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
  
  double cost() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();
    double c = spell_t::cost();
    if ( c == 0 ) return 0;
    if ( p -> buffs_shamanistic_rage -> up() )
      return 0;
    double cr = cost_reduction();
    c *= 1.0 - cr;
    if ( c < 0 ) c = 0;
    return c;
  }
};

// Thunderstorm Spell ==========================================================

struct thunderstorm_t : public shaman_spell_t
{
  thunderstorm_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "thunderstorm", player, SCHOOL_NATURE, TREE_ELEMENTAL )
  {
    shaman_t* p = player -> cast_shaman();
    if ( p -> primary_tree() != TREE_ELEMENTAL ) return;
    cooldown -> duration = p -> glyphs.thunder ? 0.10 :45.0;
    id = 51490;
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

// =========================================================================
// Shaman Shock Spells
// =========================================================================

// Earth Shock Spell =======================================================

struct earth_shock_t : public shaman_spell_t
{
  earth_shock_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "earth_shock", player, SCHOOL_NATURE, TREE_ELEMENTAL )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    static rank_t ranks[] =
    {
      { 85, 12, 907, 955, 0, 0.18 },
      { 80, 11, 812, 856, 0, 0.18 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 8042 );

    may_crit          = true;
    base_execute_time = 0;
    direct_power_mod  = 0.41;
    base_multiplier  *= 1.0 + p -> talent_concussion    -> rank() * 0.02 + p -> set_bonus.tier9_4pc_melee() * 0.25;

    base_cost_reduction        += p -> talent_convection -> rank() * 0.05;
    base_crit_bonus_multiplier *= 1.0 + p -> primary_tree() == TREE_ELEMENTAL;

    cooldown = p -> cooldowns_shock;
    cooldown -> duration  = 6.0;
    cooldown -> duration -= p -> talent_reverberation -> rank()  * 0.5;

    if ( p -> glyphs.shocking )
    {
      trigger_gcd = 1.0;
      min_gcd     = 1.0;
    }
  }
  virtual void execute()
  {
    // shaman_t* p = player -> cast_shaman();
    shaman_spell_t::execute();

    if ( result_is_hit() )
    {
      trigger_fulmination( this );
    }
  }

  virtual void player_buff()
  {
    shaman_spell_t::player_buff();
    shaman_t* p = player -> cast_shaman();
    if ( p -> buffs_stormstrike -> up() ) player_crit += 0.25;
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();

    if ( ! shaman_spell_t::ready() )
      return false;

    return true;
  }

  double cost() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();
    double c = spell_t::cost();
    if ( c == 0 ) return 0;
    if ( p -> buffs_shamanistic_rage -> up() )
      return 0;
    double cr = cost_reduction();
    if ( p -> buffs_elemental_focus -> check() ) cr += 0.40;
    c *= 1.0 - cr;
    if ( c < 0 ) c = 0;
    return c;
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
      { 85, 9, 531, 531, 142, 0.17 }, 
      { 80, 8, 476, 476, 128, 0.17 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 8050 );

    base_execute_time = 0;
    base_tick_time    = 3.0;
    num_ticks         = 6;
    may_crit          = true;
    direct_power_mod  = 0.5*1.5/3.5;
    tick_power_mod    = 0.100;

    base_dd_multiplier *= 1.0 + ( p -> talent_concussion -> rank()   * 0.02 +
                                  p -> set_bonus.tier9_4pc_melee() * 0.25 );

    base_td_multiplier *= 1.0 + ( p -> talent_concussion -> rank()   * 0.02 +
                                  p -> set_bonus.tier9_4pc_melee() * 0.25 +
                                  util_t::talent_rank( p -> talent_lava_flows -> rank(), 3, 0.20 ) +
                                  p -> set_bonus.tier8_2pc_caster() * 0.2 );
    
    base_cost_reduction  += p -> talent_convection -> rank() * 0.05;
    
    base_crit_bonus_multiplier *= 1.0 + ( p -> primary_tree() == TREE_ELEMENTAL );
    if ( p -> glyphs.flame_shock ) num_ticks = int( num_ticks * 1.50 );

    cooldown = p -> cooldowns_shock;
    cooldown -> duration  = 6.0;
    cooldown -> duration -= ( p -> talent_reverberation -> rank() * 0.5 );

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
  }



  virtual void tick()
  {
    shaman_spell_t::tick();
    shaman_t* p = player -> cast_shaman();
    if ( p -> rng_lava_surge -> roll ( p -> talent_lava_surge -> rank() * 0.10 ) )
    {
      p -> procs_lava_surge -> occur();
      p -> cooldowns_lava_burst -> reset();
    }

  }

  double cost() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();
    double c = spell_t::cost();
    if ( c == 0 ) return 0;
    if ( p -> buffs_shamanistic_rage -> up() )
      return 0;
    double cr = cost_reduction();
    if ( p -> buffs_elemental_focus -> check() ) cr += 0.40;
    c *= 1.0 - cr;
    if ( c < 0 ) c = 0;
    return c;
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
      { 85, 7, 848, 896, 0, 0.18 },
      { 80, 6, 761, 803, 0, 0.18 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 8056 );

    may_crit = true;
    base_execute_time = 0;
    direct_power_mod  = 0.41;

    base_multiplier  *= 1.0 + ( p -> talent_concussion -> rank()   * 0.02 +
                                p -> set_bonus.tier9_4pc_melee() * 0.25 );

    base_cost_reduction  += p -> talent_convection -> rank() * 0.05;

    base_crit_bonus_multiplier *= 1.0 + p -> primary_tree() == TREE_ELEMENTAL;

    cooldown = p -> cooldowns_shock;
    cooldown -> duration  = 6.0;
    cooldown -> duration -= p -> talent_reverberation -> rank() * 0.5;

    if ( p -> glyphs.shocking )
    {
      trigger_gcd = 1.0;
      min_gcd     = 1.0;
    }
  }

  double cost() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();
    double c = spell_t::cost();
    if ( c == 0 ) return 0;
    if ( p -> buffs_shamanistic_rage -> up() )
      return 0;
    double cr = cost_reduction();
    if ( p -> buffs_elemental_focus -> check() ) cr += 0.40;
    c *= 1.0 - cr;
    if ( c < 0 ) c = 0;
    return c;
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
    base_cost = player -> resource_base[ RESOURCE_MANA ] * 0.08;
    base_spell_power_multiplier = 0;
    cooldown -> duration = 6.0 - ( p -> talent_reverberation -> rank() * 0.5 );
    id = 57994;
  }

  virtual bool ready()
  {
    if ( ! sim -> target -> debuffs.casting -> check() ) return false;
    return shaman_spell_t::ready();
  }

  // does not benefit from elemental focus
  double cost() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();
    double c = spell_t::cost();
    if ( c == 0 ) return 0;
    if ( p -> buffs_shamanistic_rage -> up() )
      return 0;
    double cr = cost_reduction();
    c *= 1.0 - cr;
    if ( c < 0 ) c = 0;
    return c;
  }
  
  // doesn't consume elemental focus charges
  void consume_resource()
  {
    spell_t::consume_resource();
  }

};

// =========================================================================
// Shaman Totem Spells
// =========================================================================

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
    base_cost_reduction += p -> talent_totemic_focus -> rank() * 0.10;
    num_ticks = 1;
    base_tick_time = ( 1 + p -> talent_totemic_focus -> rank() * 0.20 ) * 120.0 + 0.1;
    cooldown -> duration = p -> glyphs.fire_elemental_totem ? 300 : 600;
    observer = &( p -> active_fire_totem );
    id = 2894;
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    if ( p -> active_fire_totem ) p -> active_fire_totem -> cancel();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    if ( p -> talent_totemic_wrath -> rank() )
      sim -> auras.flametongue_totem -> trigger( 1, .10 );
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

  // doesn't benefit from elemental focus charges
  double cost() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();
    double c = spell_t::cost();
    if ( c == 0 ) return 0;
    if ( p -> buffs_shamanistic_rage -> up() )
      return 0;
    double cr = cost_reduction();
    c *= 1.0 - cr;
    if ( c < 0 ) c = 0;
    return c;
  }
    
  // doesn't consume elemental focus charges
  void consume_resource()
  {
    spell_t::consume_resource();
  }
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

    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.11;
    trigger_gcd = 1.0;

    base_cost_reduction += p -> talent_totemic_focus -> rank() * 0.10;

    bonus = ( p -> talent_totemic_wrath -> rank() ) ? 0.10 : 0.06;

    observer = &( p -> active_fire_totem );

    id = 8227;
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    sim -> auras.flametongue_totem -> duration = 300 * ( 1 + p -> talent_totemic_focus -> rank() * 0.20 );
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
    
  // does not benefit from elemental focus or shamanistic rage
  double cost() SC_CONST
  {
    double c = spell_t::cost();
    if ( c == 0 ) return 0;
    double cr = cost_reduction();
    c *= 1.0 - cr;
    if ( c < 0 ) c = 0;
    return c;
  }
  
  // doesn't consume elemental focus charges
  void consume_resource()
  {
    spell_t::consume_resource();
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
      { 85,  8, 200, 200, 0, 0.18 },
      { 78,  7, 371, 371, 0, 0.18 },
      { 73,  6, 314, 314, 0, 0.18 },
      { 65,  5, 180, 180, 0, 0.18 },
      { 56,  4, 131, 131, 0, 0.18 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 8190 );

    aoe = true;
    base_execute_time = 0;
    base_tick_time    = 2.0;
    direct_power_mod  = 0.75 * base_tick_time / 15.0;
    num_ticks         = int ( ( 1 + p -> talent_totemic_focus -> rank() * 0.20 ) * 10.0 );
    may_crit          = true;
    trigger_gcd       = 1.0;
    base_multiplier  *= 1.0 + p -> talent_call_of_flame -> rank() * 0.10;

    base_cost_reduction += p -> talent_totemic_focus -> rank() * 0.10;

    base_crit_bonus_multiplier *= 1.0 + p -> primary_tree() == TREE_ELEMENTAL;

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
    shaman_t* p = player -> cast_shaman();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    if ( p -> talent_totemic_wrath -> rank() )
      sim -> auras.flametongue_totem -> trigger( 1, .10 );
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
    
  double cost() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();
    double c = spell_t::cost();
    if ( c == 0 ) return 0;
    if ( p -> buffs_shamanistic_rage -> up() )
      return 0;
    double cr = cost_reduction();
    c *= 1.0 - cr;
    if ( c < 0 ) c = 0;
    return c;
  }
  
  // doesn't consume elemental focus charges
  void consume_resource()
  {
    spell_t::consume_resource();
  }
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
    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.04;
    trigger_gcd = 1.0;

    base_cost_reduction += p -> talent_totemic_focus -> rank() * 0.10;

    regen = util_t::ability_rank( p -> level,  828.0,85, 91.0,80,  82.0,76,  73.0,71,  41.0,65,  31.0,0 );

    id = 5675;
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    sim -> auras.mana_spring_totem -> duration = 300 * ( 1 + p -> talent_totemic_focus -> rank() * 0.20 );
    sim -> auras.mana_spring_totem -> trigger( 1, regen );
  }

  virtual bool ready()
  {
    if ( sim -> auras.mana_spring_totem -> current_value >= regen )
      return false;

    return shaman_spell_t::ready();
  }

  virtual double gcd() SC_CONST { return player -> in_combat ? shaman_spell_t::gcd() : 0; }

  // doesn't benefit from elemental focus
  double cost() SC_CONST
  {
    double c = spell_t::cost();
    if ( c == 0 ) return 0;
    double cr = cost_reduction();
    c *= 1.0 - cr;
    if ( c < 0 ) c = 0;
    return c;
  }
  
  // doesn't consume elemental focus charges
  void consume_resource()
  {
    spell_t::consume_resource();
  }
};

// Mana Tide Totem Spell ==================================================

struct mana_tide_totem_t : public shaman_spell_t
{
  mana_tide_totem_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "mana_tide_totem", player, SCHOOL_NATURE, TREE_RESTORATION )
  {
    shaman_t* p = player -> cast_shaman();
    check_talent( p -> talent_mana_tide_totem -> rank() );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful        = false;
    base_tick_time = 3.0;
    num_ticks      = 4 + ( p -> talent_totemic_focus -> rank() ); // Each point adds a tick
    base_cost      = 320;
    trigger_gcd    = 1.0;

    cooldown -> duration = 300.0;

    dot = p -> get_dot( "water_totem" );

    base_cost_reduction += p -> talent_totemic_focus -> rank() * 0.10;

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
      { 85, 11, 82, 110, 0, 0.07 },
      { 80, 10, 90, 120, 0, 0.07 },
      { 75,  9, 77, 103, 0, 0.07 },
      { 71,  8, 68,  92, 0, 0.07 },
      { 69,  7, 56,  74, 0, 0.07 },
      { 60,  6, 40,  54, 0, 0.09 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 3599 );

    base_execute_time = 0;
    base_tick_time    = 1.64;
    direct_power_mod  = base_tick_time / 15.0;
    num_ticks         = int ( ( 1 + p -> talent_totemic_focus -> rank() * 0.20 ) * 24 );
    may_crit          = true;
    trigger_gcd       = 1.0;
    base_multiplier  *= 1.0 + p -> talent_call_of_flame -> rank() * 0.10;

    base_cost_reduction += p -> talent_totemic_focus -> rank()    * 0.10;

    base_crit_bonus_multiplier *= 1.0 + p -> primary_tree() == TREE_ELEMENTAL;

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
    shaman_t* p = player -> cast_shaman();

    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    if ( p -> talent_totemic_wrath -> rank() )
      sim -> auras.flametongue_totem -> trigger( 1, .10 );
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
    //shaman_t* p = player -> cast_shaman();

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
        trigger_searing_flames( this );
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

  // doesn't benefit from elemental focus
  double cost() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();
    double c = spell_t::cost();
    if ( c == 0 ) return 0;
    if ( p -> buffs_shamanistic_rage -> up() )
      return 0;
    double cr = cost_reduction();
    c *= 1.0 - cr;
    if ( c < 0 ) c = 0;
    return c;
  }

  // doesn't consume elemental focus charges
  void consume_resource()
  {
    spell_t::consume_resource();
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

    base_cost    = p -> resource_base[ RESOURCE_MANA ] * 0.10;
    trigger_gcd  = 1.0;

    base_cost_reduction += p -> talent_totemic_focus -> rank() * 0.10;

    bonus  = util_t::ability_rank( p -> level,  1395,85, 155,80, 115,75, 86,65,  77,0 );
   
    id = 8075;
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    sim -> auras.strength_of_earth -> duration = 300 * ( 1 + p -> talent_totemic_focus -> rank() * 0.20 );
    sim -> auras.strength_of_earth -> trigger( 1, bonus );
  }

  virtual bool ready()
  {
    if ( sim -> auras.strength_of_earth -> current_value >= bonus )
      return false;

    return shaman_spell_t::ready();
  }

  virtual double gcd() SC_CONST { return player -> in_combat ? shaman_spell_t::gcd() : 0; }

  // doesn't benefit from elemental focus or shamanistic rage
  double cost() SC_CONST
  {
    double c = spell_t::cost();
    if ( c == 0 ) return 0;
    double cr = cost_reduction();
    c *= 1.0 - cr;
    if ( c < 0 ) c = 0;
    return c;
  }

  // doesn't consume elemental focus charges
  void consume_resource()
  {
    spell_t::consume_resource();
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

    base_cost    = p -> resource_base[ RESOURCE_MANA ] * 0.11;
    trigger_gcd  = 1.0;

    base_cost_reduction += p -> talent_totemic_focus -> rank() * 0.10;

    bonus = 0.20;

    id = 8512;
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    sim -> auras.windfury_totem -> duration = 300 * ( 1 + p -> talent_totemic_focus -> rank() * 0.20 );
    sim -> auras.windfury_totem -> trigger( 1, bonus );
  }

  virtual bool ready()
  {
    if( sim -> auras.windfury_totem -> current_value >= bonus )
      return false;

    return shaman_spell_t::ready();
  }

  virtual double gcd() SC_CONST { return player -> in_combat ? shaman_spell_t::gcd() : 0; }

  // doesn't benefit from elemental focus or shamanistic rage
  double cost() SC_CONST
  {
    double c = spell_t::cost();
    if ( c == 0 ) return 0;
    double cr = cost_reduction();
    c *= 1.0 - cr;
    if ( c < 0 ) c = 0;
    return c;
  }

  // doesn't consume elemental focus charges
  void consume_resource()
  {
    spell_t::consume_resource();
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
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_cost   = p -> resource_base[ RESOURCE_MANA ] * 0.11;
    trigger_gcd = 1.0;

    base_cost_reduction += p -> talent_totemic_focus -> rank() * 0.10;

    id = 3738;
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    sim -> auras.wrath_of_air -> duration = 300 * ( 1 + p -> talent_totemic_focus -> rank() * 0.20 );
    sim -> auras.wrath_of_air -> trigger();
  }

  virtual bool ready()
  {
    if ( sim -> auras.wrath_of_air -> check() )
      return false;

    return shaman_spell_t::ready();
  }

  virtual double gcd() SC_CONST { return player -> in_combat ? shaman_spell_t::gcd() : 0; }

  // doesn't benefit from elemental focus or shamanistic rage
  double cost() SC_CONST
  {
    double c = spell_t::cost();
    if ( c == 0 ) return 0;
    double cr = cost_reduction();
    c *= 1.0 - cr;
    if ( c < 0 ) c = 0;
    return c;
  }

  // doesn't consume elemental focus charges
  void consume_resource()
  {
    spell_t::consume_resource();
  }
};

// =========================================================================
// Shaman Weapon Imbues
// =========================================================================

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

    bonus_power = util_t::ability_rank( p -> level,  750.0,85, 211.0,80,  186.0,77,  157.0,72,  96.0,65,  45.0,0  );

    bonus_power *= 1.0 + p -> talent_elemental_weapons -> rank() * 0.20;

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

    bonus_power = util_t::ability_rank( p -> level,  4430.0,85, 1250.0,80,  1090.0,76,  835.0,71,  445.0,0  );

    id = 8232;
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

// =========================================================================
// Shaman Shields
// =========================================================================

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

      base_multiplier *= 1.0 + p -> talent_improved_shields -> rank() * 0.05 + ( p -> set_bonus.tier7_2pc_melee() ? 0.10 : 0.00 );

      base_crit_bonus_multiplier *= 1.0 + p -> primary_tree() == TREE_ELEMENTAL;
    }

    virtual void player_buff()
    {
      shaman_t* p = player -> cast_shaman();
      shaman_spell_t::player_buff();

      if( p -> talent_fulmination -> rank() )
      {
        // Don't use stack() here so we don't count the occurence twice
        // together with trigger_fulmination()
        int consuming_stack =  p -> buffs_lightning_shield -> current_stack - 3;
        if ( consuming_stack > 0 )
          player_multiplier *= consuming_stack;
      }

      if ( p -> buffs_stormstrike -> up() ) player_crit += 0.25;
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
    init_rank( ranks, 324 );

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
    p -> buffs_lightning_shield -> trigger( 3 );
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

    shaman_t* p = player -> cast_shaman();

    static rank_t ranks[] =
    {
      { 76, 9, 0, 0, 0, 100 },
      { 69, 8, 0, 0, 0,  50 },
      { 62, 7, 0, 0, 0,  43 },
      { 55, 6, 0, 0, 0,  38 },
      { 0, 0, 0, 0, 0, 0 }
    };
    init_rank( ranks, 52127 );

    trigger_gcd = 0;

    if ( p -> glyphs.water_shield ) base_cost *= 1.5;
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

} // ANONYMOUS NAMESPACE ===================================================

// ==========================================================================
// Shaman Character Definition
// ==========================================================================

// shaman_t::clear_debuffs  =================================================

void shaman_t::clear_debuffs()
{
  player_t::clear_debuffs();
  buffs_searing_flames -> expire();
}
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
  if ( name == "magma_totem"             ) return new              magma_totem_t( this, options_str );
  if ( name == "mana_spring_totem"       ) return new        mana_spring_totem_t( this, options_str );
  if ( name == "mana_tide_totem"         ) return new          mana_tide_totem_t( this, options_str );
  if ( name == "natures_swiftness"       ) return new        shamans_swiftness_t( this, options_str );  
  if ( name == "primal_strike"           ) return new            primal_strike_t( this, options_str );
  if ( name == "searing_totem"           ) return new            searing_totem_t( this, options_str );
  if ( name == "shamanistic_rage"        ) return new         shamanistic_rage_t( this, options_str );
  if ( name == "spirit_wolf"             ) return new        spirit_wolf_spell_t( this, options_str );
  if ( name == "stormstrike"             ) return new              stormstrike_t( this, options_str );
  if ( name == "strength_of_earth_totem" ) return new  strength_of_earth_totem_t( this, options_str );
  if ( name == "thunderstorm"            ) return new             thunderstorm_t( this, options_str );
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
    else if ( n == "feral_spirit"         ) glyphs.feral_spirit = 1;
    else if ( n == "fire_elemental_totem" ) glyphs.fire_elemental_totem = 1;
    else if ( n == "flame_shock"          ) glyphs.flame_shock = 1;
    else if ( n == "flametongue_weapon"   ) glyphs.flametongue_weapon = 1;
    else if ( n == "lava_burst"                 ) glyphs.lava_burst = 1;
    else if ( n == "lava_lash"            ) glyphs.lava_lash = 1;
    else if ( n == "lightning_bolt"       ) glyphs.lightning_bolt = 1;
    else if ( n == "shocking"             ) glyphs.shocking = 1;
    else if ( n == "stormstrike"          ) glyphs.stormstrike = 1;
    else if ( n == "thunder"              ) glyphs.thunder = 1;
    else if ( n == "thunderstorm"         ) glyphs.thunderstorm = 1;
    else if ( n == "water_shield"         ) glyphs.water_shield = 1;
    else if ( n == "windfury_weapon"      ) glyphs.windfury_weapon = 1;
    // To prevent warnings....
    else if ( n == "artic_wolf"           ) ;
    else if ( n == "astral_recall"        ) ;
    else if ( n == "chain_heal"           ) ;
    else if ( n == "earth_shield"         ) ;
    else if ( n == "earthliving_weapon"   ) ;
    else if ( n == "elemental_mastery"    ) ;
    else if ( n == "fire_nova"            ) ;
    else if ( n == "frost_shock"          ) ;
    else if ( n == "ghost_wolf"           ) ;
    else if ( n == "grounding_totem"      ) ;
    else if ( n == "healing_stream_totem" ) ;
    else if ( n == "healing_wave"         ) ;
    else if ( n == "hex"                  ) ;
    else if ( n == "lightning_shield"     ) ;
    else if ( n == "renewed_life"         ) ;
    else if ( n == "riptide"              ) ;
    else if ( n == "shamanistic_rage"     ) ;
    else if ( n == "stoneclaw_totem"      ) ;
    else if ( n == "totemic_recall"       ) ;
    else if ( n == "water_breathing"      ) ;
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
  case RACE_DWARF:
  case RACE_GOBLIN:
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
  player_t::init_base();

  attribute_multiplier_initial[ ATTR_STAMINA   ] *= 1.0 + talent_toughness -> rank() * 0.02;
  base_attack_expertise = talent_unleashed_rage -> rank() * 0.04;

  base_attack_power = ( level * 2 ) - 20;
  initial_attack_power_per_strength = 1.0;
  initial_attack_power_per_agility  = 2.0;

  health_per_stamina = 10;
  mana_per_intellect = 15;

  base_spell_crit  += talent_acuity -> rank() * 0.01;
  base_attack_crit += talent_acuity -> rank() * 0.01;

  if ( set_bonus.tier6_2pc_caster() )
  {
    // Simply assume the totems are out all the time.

    enchant.spell_power += 45;
    enchant.crit_rating += 35;
    enchant.mp5         += 15;
  }

  distance = ( primary_tree() == TREE_ENHANCEMENT ) ? 3 : 30;
}

// shaman_t::init_scaling ====================================================

void shaman_t::init_scaling()
{
  player_t::init_scaling();

  if ( primary_tree() == TREE_ENHANCEMENT )
  {
    scales_with[ STAT_SPIRIT ] = 0;
    scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = 1;
    scales_with[ STAT_WEAPON_OFFHAND_SPEED  ] = 1;
  }
}

// shaman_t::init_buffs ======================================================

void shaman_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( sim, player, name, max_stack, duration, cooldown, proc_chance, quiet )

  buffs_elemental_devastation = new buff_t( this, "elemental_devastation", 1,  10.0, 0.0, talent_elemental_devastation -> rank() );
  buffs_elemental_focus       = new buff_t( this, "elemental_focus",       2,  15.0, 0.0, talent_elemental_focus       -> rank() );
  buffs_elemental_mastery     = new buff_t( this, "elemental_mastery",     1,  15.0, 0.0, talent_elemental_mastery     -> rank() );
  buffs_flurry                = new buff_t( this, "flurry",                3,  15.0, 0.0, talent_flurry                -> rank() );
  buffs_lightning_shield      = new buff_t( this, "lightning_shield",      9 );
  buffs_maelstrom_weapon      = new buff_t( this, "maelstrom_weapon",      5,  30.0 );
  buffs_natures_swiftness     = new buff_t( this, "natures_swiftness" );
  buffs_searing_flames        = new buff_t( this, "searing_flames",        5,   0.0, 0.0, talent_searing_flames   -> rank()      , true ); // Only to track stack count, won't show up in reports
  buffs_shamanistic_rage      = new buff_t( this, "shamanistic_rage",      1,  15.0, 0.0, talent_shamanistic_rage -> rank()      );
  buffs_stormstrike           = new buff_t( this, "stormstrike",           1,  15.0 );
  buffs_tier10_2pc_melee      = new buff_t( this, "tier10_2pc_melee",      1,  15.0, 0.0, set_bonus.tier10_2pc_melee()  );
  buffs_tier10_4pc_melee      = new buff_t( this, "tier10_4pc_melee",      1,  10.0, 0.0, 0.15                          ); //FIX ME - assuming no icd on this
  // buffs_totem_of_wrath_glyph  = new buff_t( this, "totem_of_wrath_glyph",  1, 300.0, 0.0, glyphs.totem_of_wrath );
  buffs_water_shield          = new buff_t( this, "water_shield",          1, 600.0 );
}

// shaman_t::init_gains ======================================================

void shaman_t::init_gains()
{
  player_t::init_gains();

  gains_primal_wisdom        = get_gain( "primal_wisdom"     );
  gains_rolling_thunder      = get_gain( "rolling_thunder"   );
  gains_telluric_currents    = get_gain( "telluric_currents" );
  gains_thunderstorm         = get_gain( "thunderstorm"      );
  gains_water_shield         = get_gain( "water_shield"      );
}

// shaman_t::init_procs ======================================================

void shaman_t::init_procs()
{
  player_t::init_procs();

  procs_elemental_overload = get_proc( "elemental_overload" );  
  procs_lava_surge         = get_proc( "lava_surge"         );
  procs_rolling_thunder    = get_proc( "rolling_thunder"    );
  procs_windfury           = get_proc( "windfury"           );
}

// shaman_t::init_rng ========================================================

void shaman_t::init_rng()
{
  player_t::init_rng();
  rng_elemental_overload   = get_rng( "elemental_overload"   );
  rng_lava_surge           = get_rng( "lava_surge"           );
  rng_primal_wisdom        = get_rng( "primal_wisdom"        );
  rng_rolling_thunder      = get_rng( "rolling_thunder"      );
  rng_searing_flames       = get_rng( "searing_flames"       );
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
      action_list_str += "/strength_of_earth_totem/windfury_totem/mana_spring_totem/lightning_shield";
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
      if ( talent_feral_spirit -> rank() ) action_list_str += "/spirit_wolf";
      action_list_str += "/speed_potion";
      action_list_str += "/lava_burst,if=buff.maelstrom_weapon.stack=5&buff.maelstrom_weapon.react&(dot.flame_shock.remains-cast_time)>=0";
      action_list_str += "/lightning_bolt,if=buff.maelstrom_weapon.stack=5&buff.maelstrom_weapon.react";
      if ( talent_stormstrike -> rank() ) action_list_str += "/stormstrike";
      action_list_str += "/flame_shock,if=!ticking";
      action_list_str += "/earth_shock/searing_totem";
      if ( primary_tree() == TREE_ENHANCEMENT ) action_list_str += "/lava_lash";
      if ( talent_shamanistic_rage -> rank() ) action_list_str += "/shamanistic_rage,tier10_2pc_melee=1";
      if ( talent_shamanistic_rage -> rank() ) action_list_str += "/shamanistic_rage";
    }
    else
    {
      action_list_str  = "flask,type=frost_wyrm/food,type=fish_feast/flametongue_weapon,weapon=main/water_shield";
      action_list_str += "/mana_spring_totem/wrath_of_air_totem";
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
      if ( talent_elemental_mastery -> rank() )
      {
        action_list_str += "/elemental_mastery,time_to_die<=17";
        action_list_str += "/elemental_mastery,if=!buff.bloodlust.react";
      }
      action_list_str += "/flame_shock,if=!ticking";
      if ( level >= 75 ) action_list_str += "/lava_burst,if=(dot.flame_shock.remains-cast_time)>=0";
	    action_list_str += "/fire_elemental_totem";
	    action_list_str += "/searing_totem";
      action_list_str += "/chain_lightning,if=target.adds>1";
      if ( ! ( set_bonus.tier9_4pc_caster() || set_bonus.tier10_2pc_caster() ) )
        action_list_str += "/chain_lightning,if=(!buff.bloodlust.react&(mana_pct-target.health_pct)>5)|target.adds>1";
      action_list_str += "/lightning_bolt";
      if ( primary_tree() == TREE_ELEMENTAL ) action_list_str += "/thunderstorm";
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

  return ap;
}

// shaman_t::composite_spell_hit ==========================================

double shaman_t::composite_spell_hit() SC_CONST
{
  double hit = player_t::composite_spell_hit();

  hit += ( talent_elemental_precision -> rank() / 3 ) * ( spirit() - attribute_base[ ATTR_SPIRIT] ) / rating.spell_hit;

  return hit;
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

  if ( primary_tree() == TREE_ENHANCEMENT )
  {
    double ap = composite_attack_power_multiplier() * composite_attack_power();

    sp += ap * 0.06;
  }

  if ( main_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
  {
    sp += main_hand_weapon.buff_value;
  }
  if ( off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
  {
    sp += off_hand_weapon.buff_value;
  }

  // Glyph unchanged so far
  // sp += buffs_totem_of_wrath_glyph -> value();

  return sp;
}

// shaman_t::regen  =======================================================

void shaman_t::regen( double periodicity )
{
  mana_regen_while_casting = ( primary_tree() == TREE_RESTORATION ) ? 0.50 : 0.0;

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

  if ( talent_elemental_oath -> rank() ) sim -> auras.elemental_oath -> trigger();

  int ur = util_t::talent_rank( talent_unleashed_rage -> rank(), 2, 5, 10 );

  if ( ur >= sim -> auras.unleashed_rage -> current_value )
  {
    sim -> auras.unleashed_rage -> trigger( 1, ur );
    
  }
}

// shaman_t::primary_tree =================================================

talent_tree_type shaman_t::primary_tree() SC_CONST
{
  if ( level > 10 && level <= 69 )
  {
	  if ( talent_tab_points[ SHAMAN_ELEMENTAL   ] > 0 ) return TREE_ELEMENTAL;
    if ( talent_tab_points[ SHAMAN_ENHANCEMENT ] > 0 ) return TREE_ENHANCEMENT;
	  if ( talent_tab_points[ SHAMAN_RESTORATION ] > 0 ) return TREE_RESTORATION;
    return TREE_NONE;
  }
  else
  {
    if ( talent_tab_points[ SHAMAN_ELEMENTAL ] >= talent_tab_points[ SHAMAN_ENHANCEMENT ] ) 
    {
      if ( talent_tab_points[ SHAMAN_ELEMENTAL ] >= talent_tab_points[ SHAMAN_RESTORATION ] )
      {
        return TREE_ELEMENTAL;
      }
      else
      {
        return TREE_RESTORATION;
      }
    }
    else if ( talent_tab_points[ SHAMAN_RESTORATION ] >= talent_tab_points[ SHAMAN_ENHANCEMENT ] )
    {
      return TREE_RESTORATION;
    }
    else
    {
      return TREE_ENHANCEMENT;
    }
  }
}

// shaman_t::get_talent_trees ==============================================

std::vector<talent_translation_t>& shaman_t::get_talent_list()
{
  talent_list.clear();
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

player_t* player_t::create_shaman( sim_t* sim, const std::string& name, race_type r )
{
  return new shaman_t( sim, name, r );
}

// player_t::shaman_init ====================================================

void player_t::shaman_init( sim_t* sim )
{
  sim -> auras.elemental_oath    = new aura_t( sim, "elemental_oath",    1, 0.0 );
  sim -> auras.flametongue_totem = new aura_t( sim, "flametongue_totem", 1, 300.0 );
  sim -> auras.mana_spring_totem = new aura_t( sim, "mana_spring_totem", 1, 300.0 );
  sim -> auras.strength_of_earth = new aura_t( sim, "strength_of_earth", 1, 300.0 );
  sim -> auras.unleashed_rage    = new aura_t( sim, "unleashed_rage",    1, 0.0 );
  sim -> auras.windfury_totem    = new aura_t( sim, "windfury_totem",    1, 300.0 );
  sim -> auras.wrath_of_air      = new aura_t( sim, "wrath_of_air",      1, 300.0 );

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    p -> buffs.bloodlust = new buff_t( p, "bloodlust", 1, 40.0, 600.0 );
  }
}

// player_t::shaman_combat_begin ============================================

void player_t::shaman_combat_begin( sim_t* sim )
{
  if ( sim -> overrides.elemental_oath    ) sim -> auras.elemental_oath    -> override();
  if ( sim -> overrides.flametongue_totem ) sim -> auras.flametongue_totem -> override( 1, 0.10 );
  if ( sim -> overrides.mana_spring_totem ) sim -> auras.mana_spring_totem -> override( 1, 828.0 );
  if ( sim -> overrides.strength_of_earth ) sim -> auras.strength_of_earth -> override( 1, 1395.0 );
  if ( sim -> overrides.unleashed_rage    ) sim -> auras.unleashed_rage    -> override( 1, 10.0 );
  if ( sim -> overrides.windfury_totem    ) sim -> auras.windfury_totem    -> override( 1, 0.20 );
  if ( sim -> overrides.wrath_of_air      ) sim -> auras.wrath_of_air      -> override( 1, 0.20 );
}
