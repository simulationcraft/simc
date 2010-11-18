// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

// ==========================================================================
// TODO
// ==========================================================================
// Spirit Walker's Grace Ability
// Validate lvl85 values
// Searing Flames affected by player crit?
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

  // New Buffs
  new_buff_t* buffs_flurry;
  new_buff_t* buffs_stormstrike;
  new_buff_t* buffs_shamanistic_rage;
  new_buff_t* buffs_elemental_focus;
  new_buff_t* buffs_elemental_devastation;
  new_buff_t* buffs_elemental_rage;
  new_buff_t* buffs_maelstrom_weapon;
  new_buff_t* buffs_elemental_mastery_insta;
  new_buff_t* buffs_elemental_mastery;
  new_buff_t* buffs_lightning_shield;
  new_buff_t* buffs_maelstrom_power;
  new_buff_t* buffs_natures_swiftness;
  new_buff_t* buffs_searing_flames;
  new_buff_t* buffs_unleash_flame;
  new_buff_t* buffs_unleash_wind;
  new_buff_t* buffs_water_shield;
  
  // Tree specialization passives
  passive_spell_t* spec_elemental_fury;
  passive_spell_t* spec_shamanism;
  passive_spell_t* spec_mental_quickness;
  passive_spell_t* spec_dual_wield;
  passive_spell_t* spec_primal_wisdom;
  
  // Masteries
  mastery_t* mastery_elemental_overload;
  mastery_t* mastery_enhanced_elements;
  
  // Armor specializations
  passive_spell_t* mail_specialization;

  // Cooldowns
  cooldown_t* cooldowns_elemental_mastery;
  cooldown_t* cooldowns_flametongue_weapon;
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
  proc_t* procs_maelstrom_weapon;
  proc_t* procs_rolling_thunder;
  proc_t* procs_static_shock;
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
  
  // Glyphs
  glyph_t* glyph_feral_spirit;
  glyph_t* glyph_fire_elemental_totem;
  glyph_t* glyph_flame_shock;
  glyph_t* glyph_flametongue_weapon;
  glyph_t* glyph_lava_burst;
  glyph_t* glyph_lava_lash;
  glyph_t* glyph_lightning_bolt;
  glyph_t* glyph_stormstrike;
  glyph_t* glyph_water_shield;
  glyph_t* glyph_windfury_weapon;
  
  glyph_t* glyph_chain_lightning;
  glyph_t* glyph_shocking;
  glyph_t* glyph_thunder;
  
  glyph_t* glyph_thunderstorm;
  
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
    cooldowns_elemental_mastery = get_cooldown( "elemental_mastery"  );
    cooldowns_flametongue_weapon= get_cooldown( "flametongue_weapon" );
    cooldowns_lava_burst        = get_cooldown( "lava_burst"         );
    cooldowns_shock             = get_cooldown( "shock"              );
    cooldowns_strike            = get_cooldown( "strike"             );
    cooldowns_windfury_weapon   = get_cooldown( "windfury_weapon"    );

    // Weapon Enchants
    windfury_weapon_attack   = 0;
    flametongue_weapon_spell = 0;
  }

  // Character Definition
  virtual void      init_talents();
  virtual void      init_spells();
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
  virtual double    composite_spell_power( const school_type school ) SC_CONST;
  virtual double    composite_player_multiplier( const school_type school ) SC_CONST;
  virtual double    matching_gear_multiplier( const attribute_type attr ) SC_CONST;
  virtual std::vector<talent_translation_t>& get_talent_list();
  virtual std::vector<option_t>& get_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name );
  virtual void      create_pets();
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST { return RESOURCE_MANA; }
  virtual int       primary_role() SC_CONST     { return talent_stormstrike -> rank() ? ROLE_HYBRID : ROLE_SPELL; }
  virtual void      combat_begin();
  // virtual int       normalize_by() SC_CONST { return ( primary_tree() == TREE_ENHANCEMENT ) ? STAT_AGILITY : STAT_INTELLECT; }

  // Event Tracking
  virtual void regen( double periodicity );
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// ==========================================================================
// Shaman Attack
// ==========================================================================

struct shaman_attack_t : public attack_t
{
  /* Old style construction, spell data will not be accessed */
  shaman_attack_t( const char* n, player_t* player, school_type s, int t, bool special ) :
    attack_t( n, player, RESOURCE_MANA, s, t, special )
  {
    shaman_t* p = player -> cast_shaman();
    base_hit   += p -> spec_dual_wield -> base_value( E_APPLY_AURA, A_MOD_HIT_CHANCE );
  }
  
  /* Class spell data based construction, spell name in s_name */
  shaman_attack_t( const char* n, const char* s_name, player_t* player ) :
    attack_t( n, s_name, player, TREE_NONE, true ) 
  { 
    shaman_t* p = player -> cast_shaman();
    base_hit   += p -> spec_dual_wield -> base_value( E_APPLY_AURA, A_MOD_HIT_CHANCE );
    may_crit    = true;
  }
  
  /* Spell data based construction, spell id in spell_id */
  shaman_attack_t( const char* n, uint32_t spell_id, player_t* player ) :
    attack_t( n, spell_id, player, TREE_NONE, true )
  { 
    shaman_t* p = player -> cast_shaman();
    base_hit   += p -> spec_dual_wield -> base_value( E_APPLY_AURA, A_MOD_HIT_CHANCE );
    may_crit    = true;
  }

  virtual void execute();
  virtual double cost() SC_CONST;
  virtual void player_buff();
  virtual void assess_damage( double amount, int dmg_type );
  virtual double cost_reduction() SC_CONST;
  virtual void consume_resource();
};

// ==========================================================================
// Shaman Spell
// ==========================================================================

struct shaman_spell_t : public spell_t
{
  double                    base_cost_reduction;

  /* Old style construction, spell data will not be accessed */
  shaman_spell_t( const char* n, player_t* p, const school_type s, int t ) :
    spell_t( n, p, RESOURCE_MANA, s, t ), base_cost_reduction( 0 ) { }
  
  /* Class spell data based construction, spell name in s_name */
  shaman_spell_t( const char* n, const char* s_name, player_t* p ) :
    spell_t( n, s_name, p ), base_cost_reduction( 0.0 )
  {
    may_crit = true;
  }

  /* Spell data based construction, spell id in spell_id */
  shaman_spell_t( const char* n, uint32_t spell_id, player_t* p ) :
    spell_t( n, spell_id, p ), base_cost_reduction( 0.0 )
  {
    may_crit = true;
  }

  virtual bool   is_direct_damage() SC_CONST { return base_dd_min > 0 && base_dd_max > 0; }
  virtual bool   is_periodic_damage() SC_CONST { return base_td > 0; };
  virtual bool   is_instant() SC_CONST;
  virtual double cost() SC_CONST;
  virtual double cost_reduction() SC_CONST;
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
    double ap_per_owner = 0.3;
    
    ap_per_owner += o -> glyph_feral_spirit -> base_value( E_APPLY_AURA, A_DUMMY ) / 100.0;
    
    return ap + ap_per_owner * o -> composite_attack_power_multiplier() * o -> composite_attack_power();
  }
  virtual void summon( double duration=0 )
  {
    pet_t::summon( duration );
    melee -> execute(); // Kick-off repeating attack
  }
};

// ==========================================================================
// Pet Fire Elemental
// New modeling for fire elemental, based on cata beta tests
// - Fire elemental does not gain any kind of attack-power based bonuses,
//   it scales purely on spellpower
// - Fire elemental takes a snapshot of owner's Intellect and pure Spell Power
// - Fire elemental gets 7.5 health and 4.5 mana per point of owner's Stamina 
//   and Intellect, respectively
// - DBC Data files gave coefficients for spells it does, melee coefficient 
//   is based on empirical testing
// - Fire elemental gains Spell Power (to power abilities) with a different 
//   multiplier for Intellect and pure Spell Power, currently estimated to 
//   ~ 0.8003 and ~ 0.501, respectively. Melee also uses a slightly different
//   pure Spell Power multiplier of ~ 0.5355 to better represent the melee
//   damage ranges.
// TODO:
// - Figure out better base values, current ones are very much estimated, 
//   based on player's level
// - Elemental action priority list is a weird one, current implementation is 
//   close, w/ regards to averages, but could get some work
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

  struct fire_elemental_spell_t : public spell_t
  {
    double int_multiplier;
    double sp_multiplier;
    
    fire_elemental_spell_t( player_t* player, const char* n ) :
      spell_t( n, player, RESOURCE_MANA, SCHOOL_FIRE ),
      int_multiplier( ( player -> sim -> P403 ) ? 0.8003 : 0.805 ), 
      sp_multiplier ( ( player -> sim -> P403 ) ? 0.501  : 0.475 )
    {
      
    }
    
    virtual double total_spell_power() SC_CONST
    {
      double sp  = 0.0;
      pet_t* pet = player -> cast_pet();
      
      sp += pet -> intellect() * int_multiplier;
      sp += pet -> composite_spell_power( school ) * sp_multiplier;
      return floor( sp );
    }
    
    virtual void player_buff() 
    {
      spell_t::player_buff();
      // Fire elemental has approx 3% crit
      player_crit = 0.03;
    }
    
    virtual void target_debuff( int dmg_type ) { }
  };

  struct fire_shield_t : public fire_elemental_spell_t
  {
    fire_shield_t( player_t* player ) :
      fire_elemental_spell_t( player, "fire_shield" )
    {
      aoe                       = true;
      background                = true;
      repeating                 = true;
      may_crit                  = true;
      base_execute_time         = 3.0;
      base_dd_min = base_dd_max = ( player -> level + 3 ) * 1.2;
      direct_power_mod          = player -> player_data.effect_coeff( 13376, E_SCHOOL_DAMAGE );
    };
    
    virtual double total_multiplier() SC_CONST
    { 
      return ( player -> distance > 1 ) ? 0.0 : spell_t::total_multiplier();
    }
  };

  struct fire_nova_t : public fire_elemental_spell_t
  {
    fire_nova_t( player_t* player ) :
      fire_elemental_spell_t( player, "fire_nova" )
    {
      aoe                  = true;
      may_crit             = true;
      direct_power_mod     = player -> player_data.effect_coeff( 12470, E_SCHOOL_DAMAGE );
      
      // 207 = 80
      base_cost            = player -> level * 2.750;
      // For now, model the cast time increase as well, see below
      base_execute_time    = player -> player_data.spell_cast_time( 12470, player -> level );
      
      // Fire elemental works very differently in cataclysm w/ regards to damage
      if ( player -> sim -> P403 )
      {
        base_dd_min        = ( player -> level ) * 6.7;
        base_dd_max        = ( player -> level ) * 7.7;
      }
      else
      {
        base_dd_min        = ( player -> level ) * 15.288;
        base_dd_max        = ( player -> level ) * 17.300;
      }
    };
  };

  struct fire_blast_t : public fire_elemental_spell_t
  {
    fire_blast_t( player_t* player ) :
      fire_elemental_spell_t( player, "fire_blast" )
    {
      may_crit             = true;
      base_cost            = ( player -> level ) * 3.554;
      base_execute_time    = 0;
      direct_power_mod     = player -> player_data.effect_coeff( 57984, E_SCHOOL_DAMAGE );
      
      if ( player -> sim -> P403 )
      {
        base_dd_min        = ( player -> level ) * 3.169;
        base_dd_max        = ( player -> level ) * 3.687;
      }
      else
      {
        base_dd_min        = ( player -> level ) * 10.800;
        base_dd_max        = ( player -> level ) * 12.637;
      }
    };
  };

  struct fire_melee_t : public attack_t
  {
    double int_multiplier;
    double sp_multiplier;
    
    fire_melee_t( player_t* player ) :
      attack_t( "fire_melee", player, RESOURCE_NONE, SCHOOL_FIRE ), 
      int_multiplier( ( player -> sim -> P403 ) ? 0.8003 : 0.805 ), 
      sp_multiplier ( ( player -> sim -> P403 ) ? 0.5355 : 0.509 )
    {
      may_crit                     = true;
      base_execute_time            = 0.0;
      direct_power_mod             = 1.135;
      base_spell_power_multiplier  = 1.0;
      base_attack_power_multiplier = 0.0;
      base_execute_time            = 2.0;
      
      if ( player -> sim -> P403 )
      {
        base_dd_min                = ( player -> level ) * 4.916;
        base_dd_max                = ( player -> level ) * 5.301;
      }
      else
      {
        base_dd_min                = ( player -> level ) * 4.663;
        base_dd_max                = ( player -> level ) * 5.050;
      }
    }
    
    virtual double total_spell_power() SC_CONST
    {
      double sp  = 0.0;
      
      pet_t* pet = player -> cast_pet();
      
      sp += pet -> intellect() * int_multiplier;
      sp += pet -> composite_spell_power( school ) * sp_multiplier;
      
      return floor( sp );
    }
  };

  spell_t* fire_shield;
  double   owner_int;
  double   owner_sp;

  fire_elemental_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "fire_elemental", true /*GUARDIAN*/ ), owner_int( 0.0 ), owner_sp( 0.0 )
  {
    intellect_per_owner = 1.0;
    stamina_per_owner   = 1.0;
  }

  virtual void init_base()
  {
    pet_t::init_base();

    resource_base[ RESOURCE_HEALTH ] = 4643; // Approximated from lvl83 fire elem with naked shaman
    resource_base[ RESOURCE_MANA   ] = 8508; // 

    health_per_stamina = 7.5; // See above
    mana_per_intellect = 4.5;

    // Modeling melee as a foreground action since a loose model is Nova-Blast-Melee-repeat.
    // The actual actions are not really so deterministic, but if you look at the entire spawn time,
    // you will see that there is a 1-to-1-to-1 distribution (provided there is sufficient mana).
    action_list_str = "travel/sequence,name=attack:fire_melee:fire_nova:fire_blast/restart_sequence,name=attack,moving=0";
    
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

  // Snapshot int, spell power from player
  virtual void summon( double duration )
  {
    pet_t::summon();
    fire_shield -> execute();
    
    owner_int = owner -> intellect();
    owner_sp  = ( owner -> composite_spell_power( SCHOOL_FIRE ) - owner -> spell_power_per_intellect * owner_int ) * owner -> composite_spell_power_multiplier();
  }
  
  virtual double intellect() SC_CONST
  {
    return owner_int;
  }

  virtual double composite_spell_power( const school_type ) SC_CONST
  {
    return owner_sp;
  }

  virtual double composite_attack_power() SC_CONST
  {
    return 0.0;
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
  
  virtual double composite_player_multiplier( const school_type school ) SC_CONST
  {
    double m = 1.0;
    
    // 10/14 21:18:26.413  SPELL_AURA_APPLIED,0x0000000000000000,nil,0x80000000,0xF1303C4E0000E927,"Greater Fire Elemental",0x2111,73828,"Strength of Wrynn",0x1,BUFF
    if ( buffs.hellscreams_warsong -> check() || buffs.strength_of_wrynn -> check() )
    {
      // ICC buff.
      m *= 1.3;
    }
    
    return m;
  }
};

// =========================================================================
// Shaman Ability Triggers
// =========================================================================

// trigger_elemental_overload ===============================================
/*
static void trigger_elemental_overload( spell_t* s,
                                        stats_t* elemental_overload_stats )
{
  shaman_t* p = s -> player -> cast_shaman();

  if ( p -> primary_tree() != TREE_ELEMENTAL )
    return;

  if ( s -> proc ) 
    return;

  double overload_chance = p -> composite_mastery() * p -> mastery_elemental_overload -> base_value( E_APPLY_AURA, A_DUMMY, 0 ) / 10000.0;
  
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
*/
// trigger_flametongue_weapon ===============================================

static void trigger_flametongue_weapon( attack_t* a )
{
  shaman_t* p = a -> player -> cast_shaman();
  double m_ft = a -> weapon -> swing_time / 4.0;

  if ( p -> cooldowns_flametongue_weapon -> remains() > 0 ) return;

  // Let's try a new formula for flametongue, based on EJ and such, but with proper damage ranges.
  // Player based scaling is based on max damage in flametongue weapon tooltip
  p -> flametongue_weapon_spell -> base_dd_min      = m_ft * p -> player_data.effect_min( 8024, p -> level, E_DUMMY ) / 25.0;
  p -> flametongue_weapon_spell -> base_dd_max      = p -> flametongue_weapon_spell -> base_dd_min;
  p -> flametongue_weapon_spell -> direct_power_mod = m_ft * 0.15244;
  
  // Add a very slight cooldown to flametongue weapon to prevent overly good results
  // when using FT/FT combo and synced auto-attack. This should model in-game results 
  // decently as well. See EJ Shaman forum for more information.
  p -> cooldowns_flametongue_weapon -> start( 0.15 );

  p -> flametongue_weapon_spell -> execute();
}

// trigger_windfury_weapon ================================================

static void trigger_windfury_weapon( attack_t* a )
{
  shaman_t* p = a -> player -> cast_shaman();

  if ( p -> cooldowns_windfury_weapon -> remains() > 0 ) return;

  if ( p -> rng_windfury_weapon -> roll( p -> windfury_weapon_attack -> proc_chance() ) )
  {
    p -> cooldowns_windfury_weapon -> start( 3.0 );

    p -> procs_windfury -> occur();
    p -> windfury_weapon_attack -> weapon = a -> weapon;
    p -> windfury_weapon_attack -> execute();
    p -> windfury_weapon_attack -> execute();
  }
}

// trigger_rolling_thunder ==============================================

static void trigger_rolling_thunder ( spell_t* s )
{
  shaman_t* p = s -> player -> cast_shaman();
  
  if ( ! p -> buffs_lightning_shield -> check() )
    return;
  
  if ( p -> rng_rolling_thunder -> roll( p -> talent_rolling_thunder -> proc_chance() ) )
  {
    p -> resource_gain( RESOURCE_MANA, 
                        p -> player_data.effect_base_value( 88765, E_ENERGIZE_PCT ) * p -> resource_max[ RESOURCE_MANA ],
                        p -> gains_rolling_thunder );
    p -> buffs_lightning_shield -> trigger( 1 );
    p -> procs_rolling_thunder  -> occur();
  }
}

// trigger_static_shock =============================================

static void trigger_static_shock ( attack_t* a )
{
  shaman_t* p = a -> player -> cast_shaman();

  if ( ! p -> talent_static_shock -> rank() )
    return;

  if ( ! p -> buffs_lightning_shield -> stack() )
    return;
  
  double chance = 
    p -> talent_static_shock -> proc_chance() + 
    p -> sets -> set( SET_T9_2PC_MELEE ) -> mod_additive( P_PROC_CHANCE );

  if ( p -> rng_static_shock -> roll( chance ) )
  {
    p -> active_lightning_charge -> execute();
    p -> procs_static_shock -> occur();
  }
}

// =========================================================================
// Shaman Secondary Spells / Attacks
// =========================================================================

struct lava_burst_overload_t : public shaman_spell_t
{
  lava_burst_overload_t( player_t* player ) :
    shaman_spell_t( "lava_burst_overload", 77451, player )
  {
    shaman_t* p          = player -> cast_shaman();
    proc                 = true;
    background           = true;
    
    // Shamanism, NOTE NOTE NOTE, elemental overloaded abilities use 
    // a _DIFFERENT_ effect in shamanism, that has 75% of the shamanism
    // "real" effect
    direct_power_mod  += p -> spec_shamanism -> effect_base_value( 2 ) / 100.0;
    
    base_multiplier     *= 1.0 +
      p -> talent_concussion -> mod_additive( P_GENERIC ) +
      p -> talent_call_of_flame -> effect_base_value( 2 ) / 100.0;
            
    base_crit_bonus_multiplier *= 1.0 + 
      p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE ) +
      p -> talent_lava_flows -> mod_additive( P_CRIT_DAMAGE ) +
      p -> sets -> set( SET_T7_4PC_CASTER ) -> mod_additive( P_CRIT_DAMAGE );
  }

  virtual void player_buff()
  {
    shaman_spell_t::player_buff();
    
    shaman_t* p = player -> cast_shaman();
    
    if ( p -> active_flame_shock ) 
      player_crit += 1.0;
  }
};

struct lightning_bolt_overload_t : public shaman_spell_t
{
  lightning_bolt_overload_t( player_t* player ) :
    shaman_spell_t( "lightning_bolt_overload", 45284, player )
  {
    shaman_t* p          = player -> cast_shaman();
    proc                 = true;
    background           = true;
    
    // Shamanism, NOTE NOTE NOTE, elemental overloaded abilities use 
    // a _DIFFERENT_ effect in shamanism, that has 75% of the shamanism
    // "real" effect
    direct_power_mod    += p -> spec_shamanism -> effect_base_value( 2 ) / 100.0;
      
    // Elemental fury
    base_crit_bonus_multiplier *= 1.0 + p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );

    base_multiplier     *= 1.0 +
      p -> talent_concussion -> mod_additive( P_GENERIC ) +
      p -> sets -> set( SET_T6_4PC_CASTER ) -> mod_additive( P_GENERIC ) +
      p -> glyph_lightning_bolt -> mod_additive( P_GENERIC );
  }
};

struct chain_lightning_overload_t : public shaman_spell_t
{
  int glyph_targets;
  
  chain_lightning_overload_t( player_t* player ) :
    shaman_spell_t( "chain_lightning_overload", 45297, player ), glyph_targets( 0 )
  {
    shaman_t* p          = player -> cast_shaman();
    proc                 = true;
    background           = true;
    
    // Shamanism, NOTE NOTE NOTE, elemental overloaded abilities use 
    // a _DIFFERENT_ effect in shamanism, that has 75% of the shamanism
    // "real" effect
    direct_power_mod  += p -> spec_shamanism -> effect_base_value( 2 ) / 100.0;
      
    // Elemental fury
    base_crit_bonus_multiplier *= 1.0 + p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );

    base_multiplier     *= 1.0 +
      p -> talent_concussion -> mod_additive( P_GENERIC ) +
      p -> glyph_chain_lightning -> mod_additive( P_GENERIC );

    glyph_targets        = (int) p -> glyph_chain_lightning -> mod_additive( P_TARGET );
  }
  
  virtual void assess_damage( double amount,
                               int    dmg_type )
  {
    shaman_spell_t::assess_damage( amount, dmg_type );

    for ( int i=0; i < target -> adds_nearby && i < ( 2 + glyph_targets ); i ++ )
    {
      amount *= 0.70;
      shaman_spell_t::additional_damage( amount, dmg_type );
    }
  }
};

struct lightning_bolt_dot_t : public shaman_spell_t
{
  lightning_bolt_dot_t( player_t* player ) : 
    shaman_spell_t( "tier8_4pc_elemental", player, SCHOOL_NATURE, TREE_ELEMENTAL )
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

struct searing_flames_t : public shaman_spell_t
{
  searing_flames_t( player_t* player ) : 
    shaman_spell_t( "searing_flames", 77661, player )
  {
    background       = true;
    may_miss         = false;    
    tick_may_crit    = true;  
    proc             = true;      
    dot_behavior     = DOT_REFRESH;
    scale_with_haste = false;
    
    // Override spell data values, as they seem wrong, instead
    // make searing totem calculate the damage portion of the
    // spell, without using any tick power modifiers or 
    // player based multipliers here
    tick_power_mod   = 0.0;
  }
  

  // Don't double dip
  virtual void target_debuff( int dmg_type ) { }
  virtual void player_buff() 
  {
    // Make searing flames have some sort of base crit
    player_crit = 0.03;
  }
  
  virtual double total_td_multiplier() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();
    return p -> buffs_searing_flames -> stack();
  }

  virtual void last_tick()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_spell_t::last_tick();
    p -> buffs_searing_flames -> expire();
  }
};

struct lightning_charge_t : public shaman_spell_t
{
  int consume_threshold;
  
  lightning_charge_t( player_t* player ) :
    shaman_spell_t( "lightning_shield", 26364, player ), consume_threshold( 0 )
  {
    // Use the same name "lightning_shield" to make sure the cost of refreshing the shield is included with the procs.
    shaman_t* p      = player -> cast_shaman();
    background       = true;
    may_crit         = true;
    base_multiplier *= 1.0 + 
      p -> talent_improved_shields -> mod_additive( P_GENERIC ) +
      p -> sets -> set( SET_T7_2PC_MELEE ) -> mod_additive( P_GENERIC );

    base_crit_bonus_multiplier *= 1.0 + p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );
      
    consume_threshold = ( int ) p -> talent_fulmination -> base_value();
  }

  virtual void player_buff()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_spell_t::player_buff();

    if( consume_threshold > 0 )
    {
      // Don't use stack() here so we don't count the occurence twice
      // together with trigger_fulmination()
      int consuming_stack =  p -> buffs_lightning_shield -> check() - consume_threshold;
      if ( consuming_stack > 0 )
        player_multiplier *= consuming_stack;
    }
  }
};

struct unleash_flame_t : public shaman_spell_t
{
  unleash_flame_t( player_t* player ) : 
    shaman_spell_t( "unleash_flame", 73683, player )
  {
    shaman_t* p = player -> cast_shaman();

    background           = true;
    may_miss             = false;
    proc                 = true;
    
    base_crit_bonus_multiplier *= 1.0 + 
      p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );

    // Don't cooldown here, unleash elements ability will handle it
    cooldown -> duration = 0.0;
  }

  virtual void execute()
  {
    // Figure out weapons
    shaman_t* p = player -> cast_shaman();
    
    if ( p -> main_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
    {
      shaman_spell_t::execute();
      p -> buffs_unleash_flame -> trigger();
    }
    
    if ( p -> off_hand_weapon.type != WEAPON_NONE && 
         p -> off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
    {
      shaman_spell_t::execute();
      p -> buffs_unleash_flame -> trigger();
    }
  }
};

struct flametongue_weapon_spell_t : public shaman_spell_t
{
  flametongue_weapon_spell_t( player_t* player ) :
    shaman_spell_t( "flametongue", 8024, player )
  {
    may_crit           = true;
    background         = true;
    proc               = true;

    reset();
  }
};

struct windfury_weapon_attack_t : public shaman_attack_t
{
  windfury_weapon_attack_t( player_t* player ) :
    shaman_attack_t( "windfury", 33757, player )
  {
    shaman_t* p      = player -> cast_shaman();
    school           = SCHOOL_PHYSICAL;
    stats -> school  = SCHOOL_PHYSICAL;
    background       = true;
    base_multiplier *= 1.0 + p -> talent_elemental_weapons -> effect_base_value( 3 ) / 100.0;
    
    reset();
  }
  
  virtual void player_buff()
  {
    shaman_attack_t::player_buff();
    player_attack_power += weapon -> buff_value;
  }
  
  virtual double proc_chance() SC_CONST
  {
    shaman_t* p      = player -> cast_shaman();
    
    return spell_id_t::proc_chance() + p -> glyph_windfury_weapon -> mod_additive( P_PROC_CHANCE );
  }
};

struct unleash_wind_t : public shaman_attack_t
{
  unleash_wind_t( player_t* player ) : 
    shaman_attack_t( "unleash_wind", 73681, player )
  {
    background           = true;
    may_miss             = false;
    proc                 = true;
    
    // Don't cooldown here, unleash elements will handle it
    cooldown -> duration = 0.0;
  }

  virtual void execute()
  {
    // Figure out weapons
    shaman_t* p = player -> cast_shaman();
    
    if ( p -> main_hand_weapon.buff_type == WINDFURY_IMBUE  )
    {
      weapon = &( p -> main_hand_weapon );
      shaman_attack_t::execute();
      p -> buffs_unleash_wind -> trigger();
    }
    
    if ( p -> off_hand_weapon.type != WEAPON_NONE && 
         p -> off_hand_weapon.buff_type == WINDFURY_IMBUE )
    {
      weapon = &( p -> off_hand_weapon );
      shaman_attack_t::execute();
      p -> buffs_unleash_wind -> trigger();
    }
  }
};

struct stormstrike_attack_t : public shaman_attack_t
{
  stormstrike_attack_t( player_t* player ) : 
    shaman_attack_t( "stormstrike", 32175, player )
  {
    shaman_t* p          = player -> cast_shaman();

    background           = true;
    may_miss             = false;
    
    weapon_multiplier   *= 1.0 + p -> talent_focused_strikes -> mod_additive( P_GENERIC );
    base_multiplier     *= 1.0 +
      p -> sets -> set( SET_T8_2PC_MELEE )  -> mod_additive( P_GENERIC ) +
      p -> sets -> set( SET_T11_2PC_MELEE ) -> mod_additive( P_GENERIC );
  }

  virtual void execute()
  {
    // Figure out weapons
    shaman_t* p = player -> cast_shaman();
    
    weapon = p -> main_hand_attack -> weapon;
    shaman_attack_t::execute();
    
    trigger_static_shock( this );

    if ( p -> off_hand_weapon.type != WEAPON_NONE && p -> off_hand_attack )
    {
      weapon = p -> off_hand_attack -> weapon;
      shaman_attack_t::execute();

      trigger_static_shock( this );
    }
  }
};

// =========================================================================
// Shaman Attack
// =========================================================================

// shaman_attack_t::execute ================================================

void shaman_attack_t::execute()
{
  shaman_t* p = player -> cast_shaman();

  attack_t::execute();

  if ( result_is_hit() && ! proc )
  {
    if ( p -> buffs_maelstrom_weapon -> trigger( 1, -1, 
      weapon -> proc_chance_on_swing( p -> talent_maelstrom_weapon -> rank() * 2.0 ) ) )
      p -> procs_maelstrom_weapon -> occur();

    if ( weapon -> buff_type == WINDFURY_IMBUE )
      trigger_windfury_weapon( this );

    if ( weapon -> buff_type == FLAMETONGUE_IMBUE )
      trigger_flametongue_weapon( this );

    if ( p -> rng_primal_wisdom -> roll( p -> spec_primal_wisdom -> proc_chance() ) )
      p -> resource_gain( RESOURCE_MANA, 
      p -> player_data.effect_base_value( 63375, E_ENERGIZE ) * p -> resource_base[ RESOURCE_MANA ], 
      p -> gains_primal_wisdom );
  }
}

void shaman_attack_t::consume_resource()
{
  attack_t::consume_resource();
  
  shaman_t* p = player -> cast_shaman();

  if ( result == RESULT_CRIT )
    p -> buffs_flurry -> trigger();
}

// shaman_attack_t::player_buff ============================================

void shaman_attack_t::player_buff()
{
  attack_t::player_buff();
  shaman_t* p = player -> cast_shaman();
  
  if ( p -> buffs_elemental_devastation -> up() )
    player_crit += p -> buffs_elemental_devastation -> base_value() / 100.0;
  
  if ( school == SCHOOL_FIRE || school == SCHOOL_FROST || school == SCHOOL_NATURE )
    player_multiplier *= 1.0 + p -> talent_elemental_precision -> base_value( E_APPLY_AURA, A_MOD_DAMAGE_PERCENT_DONE );

  if ( p -> buffs_elemental_rage -> up() )
    player_multiplier *= 1.0 + p -> buffs_elemental_rage -> base_value();
}

// shaman_attack_t::assess_damage ==========================================

void shaman_attack_t::assess_damage( double amount,
                                     int    dmg_type )
{
  attack_t::assess_damage( amount, dmg_type );
}

// shaman_attack_t::cost_reduction ====================================

double shaman_attack_t::cost_reduction() SC_CONST
{
  shaman_t* p = player -> cast_shaman();
  double   cr = 0.0;
  
  if ( p -> buffs_shamanistic_rage -> up() )
    cr += p -> buffs_shamanistic_rage -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER );

  return cr;
}

// shaman_attack_t::cost ==========================================

double shaman_attack_t::cost() SC_CONST
{
  double    c = attack_t::cost();
  c *= 1.0 + cost_reduction();
  if ( c < 0 ) c = 0.0;  
  return c;
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

  virtual double swing_haste() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();
    double h = attack_t::swing_haste();
    if ( p -> buffs_flurry -> up() )
      h *= 1.0 / ( 1.0 + ( p -> buffs_flurry -> base_value() + p -> sets -> set( SET_T7_4PC_MELEE ) -> mod_additive( P_UNKNOWN_1 ) / 100.0 ) );
    
    if ( p -> buffs_unleash_wind -> up() )
      h *= 1.0 / ( 1.0 + ( p -> buffs_unleash_wind -> base_value( E_APPLY_AURA, A_319 ) ) );
    
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
      // Kludge of the century to avoid manifest anger eating flurry procs
      if ( name_str != "manifest_anger" )
        p -> buffs_flurry -> decrement();
      shaman_attack_t::execute();
    }
  }

  void schedule_execute()
  {
    shaman_attack_t::schedule_execute();
    shaman_t* p = player -> cast_shaman();
    p -> buffs_unleash_wind -> decrement();
  }
};

// Auto Attack =============================================================

struct auto_attack_t : public shaman_attack_t
{
  int sync_weapons;

  auto_attack_t( player_t* player, const std::string& options_str ) :
      shaman_attack_t( "auto_attack", player, SCHOOL_PHYSICAL, TREE_NONE, false ),
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
  int    wf_cd_only;
  double flametongue_bonus,
         sf_bonus,
         i_weapon_multiplier;

  lava_lash_t( player_t* player, const std::string& options_str ) :
    shaman_attack_t( "lava_lash", "Lava Lash", player ), 
    wf_cd_only( 0 ), flametongue_bonus( 0 ), sf_bonus( 0 ),
    i_weapon_multiplier( 0 )
  {
    shaman_t* p = player -> cast_shaman();
    check_spec( TREE_ENHANCEMENT );

    option_t options[] =
    {
      { "wf_cd_only", OPT_INT, &wf_cd_only  },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    weapon              = &( player -> off_hand_weapon );
    i_weapon_multiplier = weapon_multiplier;

    if ( weapon -> type == WEAPON_NONE ) 
      background        = true; // Do not allow execution.
    
    // Lava lash's flametongue bonus is in the weird dummy script effect
    flametongue_bonus   = base_value( E_DUMMY ) / 100.0;

    // Searing flames bonus to base weapon damage
    sf_bonus            = p -> talent_improved_lava_lash -> base_value( E_APPLY_AURA, A_DUMMY ) / 100.0;
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_attack_t::execute();
    trigger_static_shock( this );

    if ( result_is_hit() )
    {
      p -> buffs_searing_flames -> expire();
      if ( p -> active_searing_flames_dot ) 
        p -> active_searing_flames_dot -> cancel();
    }
  }

  virtual void player_buff()
  {
    shaman_t* p = player -> cast_shaman();

    shaman_attack_t::player_buff();
    
    // Searing Flames most likely boosts base weapon multiplier
    weapon_multiplier = i_weapon_multiplier + sf_bonus * p -> buffs_searing_flames -> stack();
    
    // Additive bonuses Glyph and Improved Lava Lash, put tier bonuses here as well for now, 
    // as they are additive in spell data
    player_multiplier *= 1.0 + 
          p -> glyph_lava_lash -> mod_additive( P_GENERIC ) + 
          p -> talent_improved_lava_lash -> mod_additive( P_GENERIC ) +
          p -> sets -> set( SET_T11_2PC_MELEE ) -> mod_additive( P_GENERIC ) +
          p -> sets -> set( SET_T8_2PC_MELEE ) -> mod_additive( P_GENERIC );

    // Flametongue is multiplicative
    player_multiplier *= 1.0 + ( weapon -> buff_type == FLAMETONGUE_IMBUE ) * flametongue_bonus;
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

// Primal Strike Attack =======================================================

struct primal_strike_t : public shaman_attack_t
{
  primal_strike_t( player_t* player, const std::string& options_str ) :
    shaman_attack_t( "primal_strike", "Primal Strike", player )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    weapon               = &( p -> main_hand_weapon );
    cooldown             = p -> cooldowns_strike;
    cooldown -> duration = p -> player_data.spell_cooldown( spell_id() );

    base_multiplier     += p -> talent_focused_strikes -> mod_additive( P_GENERIC );
  }

  virtual void execute()
  {
    shaman_attack_t::execute();
    trigger_static_shock( this );
  }
};

// Stormstrike Attack =======================================================

struct stormstrike_t : public shaman_attack_t
{
  stormstrike_attack_t * stormstrike;
  
  stormstrike_t( player_t* player, const std::string& options_str ) :
    shaman_attack_t( "stormstrike", "Stormstrike", player )
  {
    shaman_t* p = player -> cast_shaman();
    check_talent( p -> talent_stormstrike -> rank() );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    may_crit             = false;
    cooldown             = p -> cooldowns_strike;
    cooldown -> duration = p -> player_data.spell_cooldown( spell_id() );

    // Actual damaging attacks are done by stormstrike_attack_t
    stormstrike = new stormstrike_attack_t( player );
  }
  
  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    
    calculate_result();
    consume_resource();
    
    if ( result_is_hit() )
    {
      p -> buffs_stormstrike -> trigger();
      stormstrike -> execute();
    }
    
    update_ready();
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
  // XXX: Is there any point for checking instant cast speed here?
  if ( ! is_instant() && p -> buffs_elemental_mastery -> up() )
  {
    h *= 1.0 / ( 1.0 + p -> buffs_elemental_mastery -> base_value( E_APPLY_AURA, A_MOD_CASTING_SPEED_NOT_STACK ) );
  }
  return h;
}

bool shaman_spell_t::is_instant() SC_CONST
{
  if ( base_execute_time == 0 )
    return true;
  
  shaman_t* p = player -> cast_shaman();

  if ( p -> buffs_elemental_mastery_insta -> check() )
    return true;
    
  return false;
}

// shaman_spell_t::cost_reduction ===========================================
double shaman_spell_t::cost_reduction() SC_CONST
{
  shaman_t* p = player -> cast_shaman();
  double   cr = base_cost_reduction;

  if ( p -> buffs_shamanistic_rage -> up() )
    cr += p -> buffs_shamanistic_rage -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER );
    
  // XXX: School mask for the spell is in effect 2, should we use it?
  if ( harmful && ! pseudo_pet && ! proc && p -> buffs_elemental_focus -> up() )
    cr += p -> buffs_elemental_focus -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER );

  if ( is_instant() && p -> primary_tree() == TREE_ENHANCEMENT )
    cr += p -> spec_mental_quickness -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER );

  return cr;
}

// shaman_spell_t::cost ====================================================

double shaman_spell_t::cost() SC_CONST
{
  double    c = spell_t::cost();
  
  c *= 1.0 + cost_reduction();
  
  if ( c < 0 ) c = 0;
  return c;
}

// shaman_spell_t::consume_resource ========================================

void shaman_spell_t::consume_resource()
{
  spell_t::consume_resource();
  shaman_t* p = player -> cast_shaman();
  if ( harmful && ! pseudo_pet && ! proc && resource_consumed > 0 && p -> buffs_elemental_focus -> up() )
  {
    p -> buffs_elemental_focus -> decrement();
  }
}

// shaman_spell_t::execute_time ============================================

double shaman_spell_t::execute_time() SC_CONST
{
  shaman_t* p = player -> cast_shaman();
  if ( p -> buffs_natures_swiftness -> up() && school == SCHOOL_NATURE && base_execute_time < 10.0 ) 
    return 1.0 + p -> buffs_natures_swiftness -> base_value();

  return spell_t::execute_time();
}

// shaman_spell_t::player_buff =============================================

void shaman_spell_t::player_buff()
{
  shaman_t* p = player -> cast_shaman();
  spell_t::player_buff();

  if ( p -> main_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
    player_crit += p -> glyph_flametongue_weapon -> mod_additive( P_EFFECT_3 ) / 100.0;
  
  if ( p -> off_hand_weapon.type != WEAPON_NONE && p -> off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
    player_crit += p -> glyph_flametongue_weapon -> mod_additive( P_EFFECT_3 ) / 100.0;
  
  // Can do this here, as elemental will not have access to 
  // stormstrike, and all nature spells will benefit from this for enhancement
  if ( school == SCHOOL_NATURE && p -> buffs_stormstrike -> up() )
  {
    player_crit += 
      p -> buffs_stormstrike -> base_value() / 100.0 +
      p -> glyph_stormstrike -> mod_additive( P_EFFECT_1 ) / 100.0;
  }
    
  if ( sim -> auras.elemental_oath -> up() && p -> buffs_elemental_focus -> up() )
  {
    // Elemental oath self buff is type 0 sub type 0
    player_multiplier *= 1.0 + p -> talent_elemental_oath -> base_value( E_NONE, A_NONE ) / 100.0;
  }
  
  if ( p -> buffs_elemental_mastery -> up() && ( school == SCHOOL_FIRE || school == SCHOOL_FROST || school == SCHOOL_NATURE ) )
    player_multiplier *= 1.0 + p -> buffs_elemental_mastery -> base_value( E_APPLY_AURA, A_MOD_DAMAGE_PERCENT_DONE );
    
  if ( school == SCHOOL_FIRE || school == SCHOOL_FROST || school == SCHOOL_NATURE )
    player_multiplier *= 1.0 + p -> talent_elemental_precision -> base_value( E_APPLY_AURA, A_MOD_DAMAGE_PERCENT_DONE );
    
  if ( p -> buffs_elemental_rage -> up() )
    player_multiplier *= 1.0 + p -> buffs_elemental_rage -> base_value();
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
      if ( ! proc && ! pseudo_pet && is_direct_damage() )
      {
        if ( p -> talent_elemental_devastation -> rank() )
          p -> buffs_elemental_devastation -> trigger();

        if ( p -> talent_elemental_focus -> rank() && 
          ( school == SCHOOL_FIRE || school == SCHOOL_NATURE || school == SCHOOL_FROST ) )
          p -> buffs_elemental_focus       -> trigger();
      }
    }
    
    if ( ! proc && ! pseudo_pet && school == SCHOOL_FIRE )
      p -> buffs_unleash_flame -> expire();
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
    shaman_spell_t( "bloodlust", "Bloodlust", player )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    
    harmful = false;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();
    
    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if ( p -> sleeping || p -> buffs.exhaustion -> check() ) 
        continue;
      p -> buffs.bloodlust -> trigger();
      p -> buffs.exhaustion -> trigger();
    }
  }

  virtual bool ready()
  {
    if ( player -> buffs.exhaustion -> check() )
      return false;

    if (  player -> buffs.bloodlust -> cooldown -> remains() > 0 )
      return false;

    return shaman_spell_t::ready();
  }
};

// Chain Lightning Spell ===================================================

struct chain_lightning_t : public shaman_spell_t
{
  int      clearcasting,
           conserve,
           maelstrom,
           glyph_targets;
  double   max_lvb_cd;
  // stats_t* elemental_overload_stats;
  chain_lightning_overload_t* overload;
  
  chain_lightning_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "chain_lightning", "Chain Lightning", player ),
    clearcasting( 0 ), conserve( 0 ), maelstrom( 0 ), glyph_targets( 0 ), 
    max_lvb_cd( 0 ) /*, elemental_overload_stats( 0 ) */
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "clearcasting", OPT_INT,     &clearcasting },
      { "conserve",     OPT_INT,     &conserve     },
      { "lvb_cd<",      OPT_FLT,     &max_lvb_cd   },
      { "maelstrom",    OPT_INT,     &maelstrom    },
      { NULL,           OPT_UNKNOWN, NULL          }
    };
    parse_options( options, options_str );

    direct_power_mod  += p -> spec_shamanism -> effect_base_value( 1 ) / 100.0;
    base_execute_time += p -> spec_shamanism -> effect_base_value( 3 ) / 1000.0;
    base_crit_bonus_multiplier *= 1.0 + p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );

    base_multiplier     *= 1.0 +
      p -> talent_concussion -> mod_additive( P_GENERIC ) +
      p -> glyph_chain_lightning -> mod_additive( P_GENERIC );

    base_cost_reduction += p -> talent_convection -> mod_additive( P_RESOURCE_COST );

    // elemental_overload_stats = p -> get_stats( "elemental_overload" ); // Testing needed to see if this still suffers from the jump penalty
    // elemental_overload_stats -> school = SCHOOL_NATURE;    
    glyph_targets        = (int) p -> glyph_chain_lightning -> mod_additive( P_TARGET );
      
    overload             = new chain_lightning_overload_t( player );
   }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_spell_t::execute();

    p -> buffs_maelstrom_weapon        -> expire();
    p -> buffs_elemental_mastery_insta -> expire();
    
    // Note, T10 elem 2PC bonus differs from the norm, the reduction is a positive number in spell data
    p -> cooldowns_elemental_mastery -> ready -= p -> sets -> set( SET_T10_2PC_CASTER ) -> base_value( E_APPLY_AURA, A_DUMMY ) / 1000.0;
    p -> cooldowns_elemental_mastery -> ready += p -> talent_feedback -> base_value() / 1000.0;

    if ( result_is_hit() )
    {
      trigger_rolling_thunder( this );

      double overload_chance = p -> composite_mastery() * p -> mastery_elemental_overload -> base_value( E_APPLY_AURA, A_DUMMY, 0 );

      if ( overload_chance && p -> rng_elemental_overload -> roll( overload_chance ) )
        overload -> execute();
    }
  }

  virtual double execute_time() SC_CONST
  {
    double t    = shaman_spell_t::execute_time();
    shaman_t* p = player -> cast_shaman();
    
    if ( p -> buffs_elemental_mastery_insta -> up() )
      t        *= 1.0 + p -> buffs_elemental_mastery_insta -> base_value();

    // misc_value of 10 is cast time reduction for A_ADD_PCT_MODIFIER
    t          *= 1.0 + p -> buffs_maelstrom_weapon -> stack() * p -> buffs_maelstrom_weapon -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER, 10 );
    
    return t;
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();

    if ( clearcasting > 0 && ! p -> buffs_elemental_focus -> check() )
      return false;

    if ( conserve > 0 )
    {
      double mana_pct = 100.0 * p -> resource_current[ RESOURCE_MANA ] / p -> resource_max [ RESOURCE_MANA ];
      if ( ( mana_pct - 5.0 ) < target -> health_percentage() )
        return false;
    }

    if ( maelstrom > 0 )
      if ( maelstrom > p -> buffs_maelstrom_weapon -> check() )
      return false;

    if ( max_lvb_cd > 0  )
      if ( p -> cooldowns_lava_burst -> remains() > ( max_lvb_cd * haste() ) )
	    return false;

    return shaman_spell_t::ready();
  }

  double cost_reduction() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();
    double   cr = shaman_spell_t::cost_reduction();
    
    // misc_value of 14 is mana cost reduction for A_ADD_PCT_MODIFIER
    cr         += p -> buffs_maelstrom_weapon -> stack() * p -> buffs_maelstrom_weapon -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER, 14);

    return cr;
  }

  virtual void assess_damage( double amount,
                               int    dmg_type )
  {
    shaman_spell_t::assess_damage( amount, dmg_type );

    for ( int i=0; i < target -> adds_nearby && i < ( 2 + glyph_targets ); i ++ )
    {
      amount *= 0.70;
      shaman_spell_t::additional_damage( amount, dmg_type );
    }
  }

  virtual bool is_instant() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();

    if ( p -> buffs_maelstrom_weapon -> check() == p -> buffs_maelstrom_weapon -> max_stack )
      return true;
    
    return shaman_spell_t::is_instant();
  }
};

// Elemental Mastery Spell ==================================================

struct elemental_mastery_t : public shaman_spell_t
{
  elemental_mastery_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "elemental_mastery", "Elemental Mastery", player )
  {
    shaman_t* p = player -> cast_shaman();
    check_talent( p -> talent_elemental_mastery -> rank() );
    
    harmful  = false;
    may_crit = false;
    may_miss = false;
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_spell_t::execute();
    
    // Hack cooldowns for elemental mastery, as they are now tracked in both
    // in this spell, and in the two buffs we use for it.
    p -> buffs_elemental_mastery_insta -> cooldown -> reset();
    p -> buffs_elemental_mastery       -> cooldown -> reset();
    
    p -> buffs_elemental_mastery_insta -> trigger();
    p -> buffs_elemental_mastery       -> trigger();
  }
};

// Fire Nova Spell =======================================================

struct fire_nova_t : public shaman_spell_t
{
  double m_additive;
  
  fire_nova_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "fire_nova", "Fire Nova", player ), m_additive( 0.0 )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    aoe                   = true;

    m_additive            =  
      p -> talent_improved_fire_nova -> mod_additive( P_GENERIC ) +
      p -> talent_call_of_flame      -> effect_base_value( 1 ) / 100.0;
    cooldown -> duration += p -> talent_improved_fire_nova -> mod_additive( P_COOLDOWN );

    base_crit_bonus_multiplier *= 1.0 + p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );
    
    // Scaling information is from another spell (8349)
    base_dd_min           = p -> player_data.effect_min( 8349, p -> level, E_SCHOOL_DAMAGE );
    base_dd_max           = p -> player_data.effect_max( 8349, p -> level, E_SCHOOL_DAMAGE );
    direct_power_mod      = p -> player_data.effect_coeff( 8349, E_SCHOOL_DAMAGE );
  }
  
  virtual void player_buff()
  {
    shaman_spell_t::player_buff();
    
    shaman_t* p = player -> cast_shaman();
    
    if ( p -> buffs_unleash_flame -> up() )
    {
      base_multiplier = 1.0 + m_additive +
        p -> buffs_unleash_flame -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER );
    }
    else
      base_multiplier = 1.0 + m_additive;
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();

    if ( ! p -> active_fire_totem )
      return false;
      
    // Searing totem cannot proc fire nova
    if ( p -> active_fire_totem -> spell_id() == 3599 )
      return false;

    return shaman_spell_t::ready();
  }
};

// Lava Burst Spell =========================================================

struct lava_burst_t : public shaman_spell_t
{
  lava_burst_overload_t* overload;
  int                   flame_shock;
  // stats_t*              elemental_overload_stats;
  double                m_additive;

  lava_burst_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "lava_burst", "Lava Burst", player ),
      flame_shock( 0 ), /* elemental_overload_stats( 0 ), */ m_additive( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "flame_shock", OPT_BOOL,    &flame_shock },
      { NULL,          OPT_UNKNOWN, NULL         }
    };
    parse_options( options, options_str );

    // Shamanism
    direct_power_mod    += p -> spec_shamanism -> effect_base_value( 1 ) / 100.0;
    base_execute_time   += p -> spec_shamanism -> effect_base_value( 3 ) / 1000.0;
    base_cost_reduction += p -> talent_convection -> mod_additive( P_RESOURCE_COST );
    m_additive          += 
      p -> talent_concussion -> mod_additive( P_GENERIC ) +
      p -> talent_call_of_flame -> effect_base_value( 2 ) / 100.0 + 
      p -> glyph_lava_burst -> mod_additive( P_GENERIC );
      
    base_crit_bonus_multiplier *= 1.0 + 
      p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE ) +
      p -> talent_lava_flows -> mod_additive( P_CRIT_DAMAGE ) +
      p -> sets -> set( SET_T7_4PC_CASTER ) -> mod_additive( P_CRIT_DAMAGE );

    // There is a real spell for this, could move it to a secondary effect
    if ( p -> set_bonus.tier9_4pc_caster() )
    {
      num_ticks         = 3;
      base_tick_time    = 2.0;
      tick_may_crit     = false;
      tick_power_mod    = 0.0;
      scale_with_haste  = false;
    }

    // elemental_overload_stats = p -> get_stats( "elemental_overload" );
    // elemental_overload_stats -> school = SCHOOL_FIRE;
    overload            = new lava_burst_overload_t( player );
  }

  virtual double total_td_multiplier() SC_CONST
  {
    return 1.0; // Don't double-dip with tier9_4pc
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_spell_t::execute();
    p -> buffs_elemental_mastery_insta -> expire();

    if ( result_is_hit() )
    {
      double overload_chance = p -> composite_mastery() * p -> mastery_elemental_overload -> base_value( E_APPLY_AURA, A_DUMMY, 0 );

      if ( overload_chance && p -> rng_elemental_overload -> roll( overload_chance ) )
        overload -> execute();

      if ( p -> set_bonus.tier9_4pc_caster() )
        base_td = direct_dmg * 0.1 / num_ticks;

      if ( p -> set_bonus.tier10_4pc_caster() && p -> active_flame_shock )
      {
        double nticks = p -> sets -> set( SET_T10_4PC_CASTER ) -> base_value( E_APPLY_AURA, A_DUMMY ) / p -> active_flame_shock -> base_tick_time;
        nticks *= 1.0 / p -> active_flame_shock -> snapshot_haste;
        p -> active_flame_shock -> extend_duration( int( floor( nticks + 0.5 ) ) );
      }
    }
  }

  virtual double execute_time() SC_CONST
  {
    double t = shaman_spell_t::execute_time();
    shaman_t* p = player -> cast_shaman();
    
    if ( p -> buffs_elemental_mastery_insta -> up() )
      t *= 1.0 + p -> buffs_elemental_mastery_insta -> base_value();
      
    return t;
  }

  virtual void player_buff()
  {
    shaman_spell_t::player_buff();
    
    shaman_t* p = player -> cast_shaman();
    
    if ( p -> active_flame_shock ) 
      player_crit += 1.0;
      
    if ( p -> buffs_unleash_flame -> up() )
    {
      base_multiplier = 1.0 + m_additive +
        p -> buffs_unleash_flame -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER );
    }
    else
      base_multiplier = 1.0 + m_additive;
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
};

// Lightning Bolt Spell =====================================================

struct lightning_bolt_t : public shaman_spell_t
{
  int      maelstrom;
  lightning_bolt_overload_t* overload;
  // stats_t* elemental_overload_stats;

  lightning_bolt_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "lightning_bolt", "Lightning Bolt", player ),
    maelstrom( 0 ) /*,  elemental_overload_stats( 0 ) */
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "maelstrom", OPT_INT,  &maelstrom  },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    // Tier11 4pc Caster Bonus (apply to raw base cast time for now, until verified)
    base_execute_time *= 1.0 + p -> sets -> set( SET_T11_4PC_CASTER ) -> mod_additive( P_CAST_TIME );
    // Shamanism
    direct_power_mod  += p -> spec_shamanism -> effect_base_value( 1 ) / 100.0;
    base_execute_time += p -> spec_shamanism -> effect_base_value( 3 ) / 1000.0;
    // Elemental fury
    base_crit_bonus_multiplier *= 1.0 + p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );

    // Tier11 4pc Melee bonus
    base_crit            = p -> sets -> set( SET_T11_4PC_MELEE ) -> mod_additive( P_CRIT );

    base_multiplier     *= 1.0 +
      p -> talent_concussion -> mod_additive( P_GENERIC ) +
      p -> sets -> set( SET_T6_4PC_CASTER ) -> mod_additive( P_GENERIC ) +
      p -> glyph_lightning_bolt -> mod_additive( P_GENERIC );

    base_cost_reduction += 
      p -> talent_convection -> mod_additive( P_RESOURCE_COST ) +
      p -> sets -> set( SET_T7_2PC_CASTER ) -> mod_additive( P_RESOURCE_COST );

    // elemental_overload_stats = p -> get_stats( "elemental_overload" );
    // elemental_overload_stats -> school = SCHOOL_NATURE;   

    overload            = new lightning_bolt_overload_t( player );

    // Tier8 bonus
    p -> active_lightning_bolt_dot = new lightning_bolt_dot_t( p );
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_spell_t::execute();
    
    p -> buffs_maelstrom_weapon        -> expire();
    p -> buffs_elemental_mastery_insta -> expire();

    // Note, T10 elem 2PC bonus differs from the norm, the reduction is a positive number in spell data
    p -> cooldowns_elemental_mastery -> ready -= p -> sets -> set( SET_T10_2PC_CASTER ) -> base_value( E_APPLY_AURA, A_DUMMY ) / 1000.0;
    p -> cooldowns_elemental_mastery -> ready += p -> talent_feedback -> base_value() / 1000.0;
    
    if ( result_is_hit() )
    {
      trigger_rolling_thunder( this );

      double overload_chance = p -> composite_mastery() * p -> mastery_elemental_overload -> base_value( E_APPLY_AURA, A_DUMMY, 0 );

      if ( overload_chance && p -> rng_elemental_overload -> roll( overload_chance ) )
        overload -> execute();

      if ( p -> talent_telluric_currents -> rank() )
      {
        p -> resource_gain( RESOURCE_MANA,
          direct_dmg * p -> talent_telluric_currents -> base_value() / 100.0, 
          p -> gains_telluric_currents );
      }

      if ( result == RESULT_CRIT && p -> set_bonus.tier8_4pc_caster() )
      {
        double dmg = direct_dmg * 0.08;

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
    }
  }
  
  virtual double execute_time() SC_CONST
  {
    double t = shaman_spell_t::execute_time();
    shaman_t* p = player -> cast_shaman();
    if ( p -> buffs_elemental_mastery_insta -> up() )
      t *= 1.0 + p -> buffs_elemental_mastery_insta -> base_value();

    t *= 1.0 + p -> buffs_maelstrom_weapon -> stack() * p -> buffs_maelstrom_weapon -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER, 10 );
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

    return true;
  }
  
  virtual double cost_reduction() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();
    double   cr = shaman_spell_t::cost_reduction();
    
    cr += p -> buffs_maelstrom_weapon -> stack() * p -> buffs_maelstrom_weapon -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER, 10 );

    return cr;
  }
  
  virtual bool is_instant() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();

    if ( p -> buffs_maelstrom_weapon -> check() == p -> buffs_maelstrom_weapon -> max_stack )
      return true;
    
    return shaman_spell_t::is_instant();
  }
};

// Natures Swiftness Spell ==================================================

struct shamans_swiftness_t : public shaman_spell_t
{
  cooldown_t* sub_cooldown;
  dot_t*      sub_dot;

  shamans_swiftness_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "natures_swiftness", "Nature's Swiftness", player ),
    sub_cooldown(0), sub_dot(0)
  {
    shaman_t* p = player -> cast_shaman();
    check_talent( p -> talent_natures_swiftness -> rank() );
    
    if ( ! options_str.empty() )
    {
      // This will prevent Natures Swiftness from being called before the desired "fast spell" is ready to be cast.
      sub_cooldown = p -> get_cooldown( options_str );
      sub_dot      = p -> get_dot     ( options_str );
    }
    
    harmful = false;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();
    shaman_t* p = player -> cast_shaman();
    p -> buffs_natures_swiftness -> trigger();
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
    shaman_spell_t( "shamanistic_rage", "Shamanistic Rage", player ),
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

    harmful = false;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    shaman_t* p = player -> cast_shaman();

    p -> buffs_shamanistic_rage -> trigger();
    
    if ( p -> set_bonus.tier10_2pc_melee() )
      p -> buffs_elemental_rage -> trigger();
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
    shaman_spell_t( "spirit_wolf", "Feral Spirit", player )
  {
    shaman_t* p = player -> cast_shaman();
    check_talent( p -> talent_feral_spirit -> rank() );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();
    
    player -> summon_pet( "spirit_wolf", duration() );
  }
};

// Thunderstorm Spell ==========================================================

struct thunderstorm_t : public shaman_spell_t
{
  double bonus;
  
  thunderstorm_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "thunderstorm", "Thunderstorm", player ), bonus( 0 )
  {
    shaman_t* p = player -> cast_shaman();
    check_spec( TREE_ELEMENTAL );

    cooldown -> duration += p -> glyph_thunder -> mod_additive( P_COOLDOWN );
    bonus                 = 
      p -> player_data.effect_base_value( id, E_ENERGIZE_PCT ) +
      p -> glyph_thunderstorm -> mod_additive( P_EFFECT_2 ) / 100.0;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    shaman_t* p = player -> cast_shaman();

    p -> resource_gain( resource, p -> resource_max[ resource ] * bonus, p -> gains_thunderstorm );
  }

  virtual bool ready()
  {
    if ( ! shaman_spell_t::ready() )
      return false;

    return player -> resource_current[ RESOURCE_MANA ] < ( 0.92 * player -> resource_max[ RESOURCE_MANA ] );
  }
};

// Unleash Elements Spell ==========================================================

struct unleash_elements_t : public shaman_spell_t
{
  unleash_wind_t*   wind;
  unleash_flame_t* flame;
  
  unleash_elements_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "unleash_elements", "Unleash Elements", player )
  {
    may_crit    = false;
    
    wind        = new unleash_wind_t( player );
    flame       = new unleash_flame_t( player );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();
    
    if ( result_is_hit() )
    {
      wind  -> execute();
      flame -> execute();
    }
  }
};

// =========================================================================
// Shaman Shock Spells
// =========================================================================

// Earth Shock Spell =======================================================

struct earth_shock_t : public shaman_spell_t
{
  int consume_threshold;
  
  earth_shock_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "earth_shock", "Earth Shock", player ), consume_threshold( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    
    base_dd_multiplier   *= 1.0 + 
      p -> talent_concussion -> mod_additive( P_GENERIC ) +
      p -> sets -> set( SET_T9_4PC_MELEE ) -> mod_additive( P_GENERIC );
                                  
    base_cost_reduction  += p -> talent_convection -> mod_additive( P_RESOURCE_COST );
    base_crit_bonus_multiplier *= 1.0 + p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );

    cooldown = p -> cooldowns_shock;
    cooldown -> duration  = p -> player_data.spell_cooldown( spell_id() ) + 
      p -> talent_reverberation -> mod_additive( P_COOLDOWN );
    
    if ( p -> glyph_shocking -> ok() )
    {
      trigger_gcd         = 1.0;
      min_gcd             = 1.0;
    }
    
    consume_threshold     = ( int ) p -> talent_fulmination -> base_value();
  }
  
  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_spell_t::execute();

    if ( consume_threshold == 0 )
      return;

    if ( result_is_hit() )
    {
      int consuming_stacks = p -> buffs_lightning_shield -> stack() - consume_threshold;
      if ( consuming_stacks > 0 ) 
      {
        p -> active_lightning_charge -> execute();
        p -> buffs_lightning_shield  -> decrement( consuming_stacks );
      }
    }
  }
};

// Flame Shock Spell =======================================================

struct flame_shock_t : public shaman_spell_t
{
  double m_dd_additive,
         m_td_additive;
  
  flame_shock_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "flame_shock", "Flame Shock", player ), m_dd_additive( 0 ), m_td_additive( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    tick_may_crit         = true;

    m_dd_additive         = 
      p -> talent_concussion -> mod_additive( P_GENERIC ) +
      p -> sets -> set( SET_T9_4PC_MELEE ) -> mod_additive( P_GENERIC );
                                  
    m_td_additive         = 
      p -> talent_concussion -> mod_additive( P_TICK_DAMAGE ) + 
      p -> talent_lava_flows -> mod_additive( P_TICK_DAMAGE ) +
      p -> sets -> set( SET_T9_4PC_MELEE ) -> mod_additive( P_TICK_DAMAGE ) +
      p -> sets -> set( SET_T8_2PC_CASTER ) -> mod_additive( P_TICK_DAMAGE );
    
    base_cost_reduction  *= 1.0 + p -> talent_convection -> mod_additive( P_RESOURCE_COST );
    
    // Tier11 2pc Caster Bonus
    base_crit             = p -> sets -> set( SET_T11_2PC_CASTER ) -> mod_additive( P_CRIT );
    
    base_crit_bonus_multiplier *= 1.0 + p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );

    // XX: For now, apply the tier9 2p bonus first, then do the glyph duration increase
    double n              = num_ticks;
    n                    += p -> sets -> set( SET_T9_2PC_CASTER ) -> mod_additive( P_DURATION ) / base_tick_time;
    n                    *= 1.0 + p -> glyph_flame_shock -> mod_additive( P_DURATION );
    num_ticks             = (int) n;

    cooldown              = p -> cooldowns_shock;
    cooldown -> duration  = p -> player_data.spell_cooldown( spell_id() ) + 
      p -> talent_reverberation -> mod_additive( P_COOLDOWN );

    if ( p -> glyph_shocking -> ok() )
    {
      trigger_gcd         = 1.0;
      min_gcd             = 1.0;
    }

    observer              = &( p -> active_flame_shock );
  }

  virtual void execute()
  {
    added_ticks = 0;
    
    shaman_spell_t::execute();
  }
  
  virtual void player_buff()
  {
    shaman_spell_t::player_buff();
    shaman_t* p = player -> cast_shaman();
    
    // Unleash flame is an additive bonus
    if ( p -> buffs_unleash_flame -> up() )
    {
      base_dd_multiplier = 1.0 + m_dd_additive + 
        p -> buffs_unleash_flame -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER );
      base_td_multiplier = 1.0 + m_td_additive +
        p -> buffs_unleash_flame -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER );
    }
    else
    {
      base_dd_multiplier = 1.0 + m_dd_additive;
      base_td_multiplier = 1.0 + m_td_additive;
    }
  }
  
  virtual void tick()
  {
    shaman_spell_t::tick();
    
    shaman_t* p = player -> cast_shaman();
    if ( p -> rng_lava_surge -> roll ( p -> talent_lava_surge -> proc_chance() ) )
    {
      p -> procs_lava_surge -> occur();
      p -> cooldowns_lava_burst -> reset();
    }
  }
};

// Frost Shock Spell =======================================================

struct frost_shock_t : public shaman_spell_t
{
  frost_shock_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "frost_shock", "Frost Shock", player )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_multiplier   *= 1.0 + 
      p -> talent_concussion -> mod_additive( P_GENERIC ) +
      p -> sets -> set( SET_T9_4PC_MELEE ) -> mod_additive( P_GENERIC );

    base_cost_reduction  += p -> talent_convection -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER, 14 );

    base_crit_bonus_multiplier *= 1.0 + p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );

    cooldown              = p -> cooldowns_shock;
    cooldown -> duration  = p -> player_data.spell_cooldown( spell_id() ) + 
      p -> talent_reverberation -> mod_additive( P_COOLDOWN );

    if ( p -> glyph_shocking -> ok() )
    {
      trigger_gcd         = 1.0;
      min_gcd             = 1.0;
    }
  }
};

// Wind Shear Spell ========================================================

struct wind_shear_t : public shaman_spell_t
{
  wind_shear_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "wind_shear", "Wind Shear", player )
  {
    shaman_t* p = player -> cast_shaman();
    
    cooldown -> duration += p -> talent_reverberation -> mod_additive( P_COOLDOWN );
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() ) return false;
    return shaman_spell_t::ready();
  }
};

// =========================================================================
// Shaman Totem Spells
// =========================================================================

struct shaman_totem_t : public shaman_spell_t
{
  double totem_duration;
  double totem_bonus;
  
  shaman_totem_t( const char * name, const char * totem_name, player_t* player, const std::string& options_str ) :
    shaman_spell_t( name, totem_name, player ), totem_duration( 0 ), totem_bonus( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful              = false;
    scale_with_haste     = false;
    base_cost_reduction += p -> talent_totemic_focus -> mod_additive( P_RESOURCE_COST );
    totem_duration       = p -> player_data.spell_duration( spell_id() ) * 
      ( 1.0 + p -> talent_totemic_focus -> mod_additive( P_DURATION ) );
  }
    
  virtual void execute()
  {
    if ( sim -> log ) 
      log_t::output( sim, "%s performs %s", player -> name(), name() );

    consume_resource();
    update_ready();

    if ( num_ticks )
      schedule_tick();
  }
  
  virtual void tick()
  {
    if ( sim -> debug ) 
      log_t::output( sim, "%s ticks (%d of %d)", name(), current_tick, num_ticks );

    target_debuff( DMG_DIRECT );
    calculate_result();

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
      if ( sim -> log ) 
        log_t::output( sim, "%s avoids %s (%s)", target -> name(), name(), util_t::result_type_string( result ) );
    }
    
    update_stats( DMG_OVER_TIME );
  }

  virtual double gcd() SC_CONST 
  { 
    if ( harmful )
      return shaman_spell_t::gcd();
    
    return player -> in_combat ? shaman_spell_t::gcd() : 0; 
  }
};

// Fire Elemental Totem Spell =================================================

struct fire_elemental_totem_t : public shaman_totem_t
{
  fire_elemental_totem_t( player_t* player, const std::string& options_str ) :
    shaman_totem_t( "fire_elemental_totem", "Fire Elemental Totem", player, options_str )
  {
    shaman_t* p = player -> cast_shaman();

    num_ticks            = 1; 
    number_ticks         = num_ticks;
    base_tick_time       = totem_duration;
    
    cooldown -> duration += p -> glyph_fire_elemental_totem -> mod_additive( P_COOLDOWN );
    
    observer             = &( p -> active_fire_totem );
  }
  
  virtual void schedule_execute()
  {
    shaman_t* p = player -> cast_shaman();

    if ( p -> active_fire_totem )
      p -> active_fire_totem -> cancel();
      
    action_t::schedule_execute();
  }
  
  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();

    shaman_totem_t::execute();

    if ( p -> talent_totemic_wrath -> rank() )
    {
      if ( sim -> overrides.flametongue_totem == 0 )
      {
        sim -> auras.flametongue_totem -> buff_duration = totem_duration;
        sim -> auras.flametongue_totem -> trigger( 1, p -> talent_totemic_wrath -> base_value() / 100.0 );
      }
    }

    p -> summon_pet( "fire_elemental" );
  }
  
  virtual void tick() { }

  virtual void last_tick()
  {
    shaman_spell_t::last_tick();
    player -> dismiss_pet( "fire_elemental" );
  }
};

// Flametongue Totem Spell ====================================================

struct flametongue_totem_t : public shaman_totem_t
{
  flametongue_totem_t( player_t* player, const std::string& options_str ) :
    shaman_totem_t( "flametongue_totem", "Flametongue Totem", player, options_str )
  {
    shaman_t* p = player -> cast_shaman();

    if ( p -> talent_totemic_wrath -> rank() > 0 )
      totem_bonus        = p -> talent_totemic_wrath -> base_value() / 100.0;
    // XX: Hardcode this based on tooltip information for now, effect is spell id 52109
    else
      totem_bonus        = p -> player_data.effect_base_value( 52109, E_APPLY_AREA_AURA_RAID, A_317 );

    observer             = &( p -> active_fire_totem );
  }

  virtual void schedule_execute()
  {
    shaman_t* p = player -> cast_shaman();

    if ( p -> active_fire_totem )
      p -> active_fire_totem -> cancel();
      
    action_t::schedule_execute();
  }
  
  virtual void execute()
  {
    shaman_totem_t::execute();

    if ( sim -> overrides.flametongue_totem == 0 )
    {
      sim -> auras.flametongue_totem -> buff_duration = totem_duration;
      sim -> auras.flametongue_totem -> trigger( 1, totem_bonus );
    }
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
};

// Magma Totem Spell =======================================================

struct magma_totem_t : public shaman_totem_t
{
  magma_totem_t( player_t* player, const std::string& options_str ) :
    shaman_totem_t( "magma_totem", "Magma Totem", player, options_str )
  {
    uint32_t trig_spell_id = 0;
    shaman_t*            p = player -> cast_shaman();

    aoe               = true;
    harmful           = true;
    may_crit          = true;
    // Magma Totem is not a real DoT, but rather a pet that is spawned.
    pseudo_pet        = true;
    
    // Base multiplier x2 in the effect of call of flame, no real way to discern them except to hardcode it with
    // the effect number
    base_multiplier  *= 1.0 + p -> talent_call_of_flame -> effect_base_value( 1 ) / 100.0;
    
    base_crit_bonus_multiplier *= 1.0 + p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );
    
    // Spell id 8188 does the triggering of magma totem's aura
    base_tick_time    = p -> player_data.effect_period( 8188, E_APPLY_AURA, A_PERIODIC_TRIGGER_SPELL );
    num_ticks         = (int) ( totem_duration / base_tick_time );
    number_ticks      = num_ticks;
    
    // Fill out scaling data
    trig_spell_id     = p -> player_data.effect_trigger_spell_id( 8188, E_APPLY_AURA, A_PERIODIC_TRIGGER_SPELL );
    // Also kludge totem school to fire for accurate damage
    school            = spell_id_t::get_school_type( p -> player_data.spell_school_mask( trig_spell_id ) );
    stats -> school   = school;
    
    base_dd_min       = p -> player_data.effect_min( trig_spell_id, p -> level, E_SCHOOL_DAMAGE );
    base_dd_max       = p -> player_data.effect_max( trig_spell_id, p -> level, E_SCHOOL_DAMAGE );
    direct_power_mod  = p -> player_data.effect_coeff( trig_spell_id, E_SCHOOL_DAMAGE );

    observer          = &( p -> active_fire_totem );
  }

  virtual void schedule_execute()
  {
    shaman_t* p = player -> cast_shaman();

    if ( p -> active_fire_totem )
      p -> active_fire_totem -> cancel();
      
    action_t::schedule_execute();
  }
  
  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    
    // Add a direct damage result as the "drop event" of the totem
    direct_dmg = 0;
    result     = RESULT_HIT;
    update_stats( DMG_DIRECT );
    
    if ( p -> talent_totemic_wrath -> rank() )
    {
      if ( sim -> overrides.flametongue_totem == 0 )
      {
        sim -> auras.flametongue_totem -> buff_duration = totem_duration;
        sim -> auras.flametongue_totem -> trigger( 1, p -> talent_totemic_wrath -> base_value() / 100.0 );
      }
    }
      
    player_buff();
    shaman_totem_t::execute();
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();
    if ( p -> active_fire_totem )
      return false;
    return shaman_spell_t::ready();
  }
};

// Mana Spring Totem Spell ================================================

struct mana_spring_totem_t : public shaman_totem_t
{
  mana_spring_totem_t( player_t* player, const std::string& options_str ) :
    shaman_totem_t( "mana_spring_totem", "Mana Spring Totem", player, options_str )
  {
    shaman_t* p = player -> cast_shaman();

    // Mana spring effect is at spell id 5677. Get scaling information from there.
    totem_bonus  = p -> player_data.effect_min( 5677, p -> level, E_APPLY_AREA_AURA_RAID, A_MOD_POWER_REGEN );
  }

  virtual void execute()
  {
    shaman_totem_t::execute();
    
    if ( sim -> overrides.mana_spring_totem == 0 )
    {
      sim -> auras.mana_spring_totem -> buff_duration = totem_duration;
      sim -> auras.mana_spring_totem -> trigger( 1, totem_bonus );
    }
  }

  virtual bool ready()
  {
    if ( sim -> auras.mana_spring_totem -> current_value >= totem_bonus )
      return false;

    return shaman_spell_t::ready();
  }
};

// Mana Tide Totem Spell ==================================================

struct mana_tide_totem_t : public shaman_totem_t
{
  mana_tide_totem_t( player_t* player, const std::string& options_str ) :
    shaman_totem_t( "mana_tide_totem", "Mana Tide Totem", player, options_str )
  {
    shaman_t* p = player -> cast_shaman();
    check_talent( p -> talent_mana_tide_totem -> rank() );

    // Mana tide effect bonus is in a separate spell, we dont need other info
    // from there anymore, as mana tide does not pulse anymore
    totem_bonus  = p -> player_data.effect_base_value( spell_id(), E_APPLY_AREA_AURA_PARTY, A_MOD_TOTAL_STAT_PERCENTAGE );
  }

  virtual void execute()
  {
    shaman_totem_t::execute();
    
    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if ( p -> party == player -> party )
      {
        // Change buff duration based on totem duration
        p -> buffs.mana_tide -> buff_duration = totem_duration;
        p -> buffs.mana_tide -> trigger( 1, totem_bonus );
      }
    }

    update_ready();
  }

  virtual bool ready()
  {
    if ( ! shaman_spell_t::ready() )
      return false;

    return ( player -> resource_current[ RESOURCE_MANA ] < ( 0.75 * player -> resource_max[ RESOURCE_MANA ] ) );
  }
};

// Searing Totem Spell =======================================================

struct searing_totem_t : public shaman_totem_t
{
  searing_totem_t( player_t* player, const std::string& options_str ) :
    shaman_totem_t( "searing_totem", "Searing Totem", player, options_str )
  {
    shaman_t* p = player -> cast_shaman();

    harmful              = true;
    pseudo_pet           = true;
    may_crit             = true;
    
    // Base multiplier has to be applied like this, because the talent has two identical effects
    base_multiplier     *= 1.0 + p -> talent_call_of_flame -> effect_base_value( 1 ) / 100.0;
    
    base_crit_bonus_multiplier *= 1.0 + p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );
    
    // Scaling information is found in Searing Bolt (3606)
    base_dd_min          = p -> player_data.effect_min( 3606, p -> level, E_SCHOOL_DAMAGE );
    base_dd_max          = p -> player_data.effect_max( 3606, p -> level, E_SCHOOL_DAMAGE );
    direct_power_mod     = p -> player_data.effect_coeff( 3606, E_SCHOOL_DAMAGE );
    // Note, searing totem tick time should come from the searing totem's casting time (1.50 sec), 
    // except it's in-game cast time is ~1.6sec
    // base_tick_time       = p -> player_data.spell_cast_time( 3606, p -> level );
    base_tick_time       = 1.6;
    travel_speed         = p -> player_data.spell_missile_speed( 3606 );
    range                = p -> player_data.spell_max_range( 3606 );
    num_ticks            = (int) ( totem_duration / base_tick_time );
    number_ticks         = num_ticks;
    // Also kludge totem school to fire
    school               = spell_id_t::get_school_type( p -> player_data.spell_school_mask( 3606 ) );
    stats -> school      = SCHOOL_FIRE;

    observer             = &( p -> active_fire_totem );
    
    p -> active_searing_flames_dot = new searing_flames_t( p );
  }

  virtual void schedule_execute()
  {
    shaman_t* p = player -> cast_shaman();

    if ( p -> active_fire_totem )
      p -> active_fire_totem -> cancel();
      
    action_t::schedule_execute();
  }
  
  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    
    // Add a direct damage result as the "drop event" of the totem
    direct_dmg = 0;
    result     = RESULT_HIT;
    update_stats( DMG_DIRECT );

    if ( p -> talent_totemic_wrath -> rank() )
    {
      if ( sim -> overrides.flametongue_totem == 0 )
      {
        sim -> auras.flametongue_totem -> buff_duration = totem_duration;
        sim -> auras.flametongue_totem -> trigger( 1, p -> talent_totemic_wrath -> base_value() / 100.0 );
      }
    }
      
    player_buff();
    shaman_totem_t::execute();
  }

  virtual void tick()
  {
    shaman_t* p = player -> cast_shaman();
    
    shaman_totem_t::tick();
    
    if ( ! p -> talent_searing_flames -> rank() )
      return;
    
    if ( result_is_hit() && p -> buffs_searing_flames -> trigger() )
    {
      double new_base_td = tick_dmg;
      // Searing flame dot treats all hits as.. HITS
      if ( result == RESULT_CRIT )
        new_base_td /= 1.0 + total_crit_bonus();

      p -> active_searing_flames_dot -> base_td = new_base_td / p -> active_searing_flames_dot -> num_ticks;
      p -> active_searing_flames_dot -> execute();
    }
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();

    if ( p -> active_fire_totem )
      return false;

    return shaman_spell_t::ready();
  }
};

// Strength of Earth Totem Spell ==============================================

struct strength_of_earth_totem_t : public shaman_totem_t
{
  strength_of_earth_totem_t( player_t* player, const std::string& options_str ) :
    shaman_totem_t( "strength_of_earth_totem", "Strength of Earth Totem", player, options_str )
  {
    shaman_t* p = player -> cast_shaman();

    // We can use either A_MOD_STAT effect, as they both apply the same amount of stat
    totem_bonus  = p -> player_data.effect_min( 8076, p -> level, E_APPLY_AREA_AURA_RAID, A_MOD_STAT );
  }

  virtual void execute()
  {
    shaman_totem_t::execute();

    if ( sim -> overrides.strength_of_earth == 0 )
    {
      sim -> auras.strength_of_earth -> buff_duration = totem_duration;
      sim -> auras.strength_of_earth -> trigger( 1, totem_bonus );
    }
  }

  virtual bool ready()
  {
    if ( sim -> auras.strength_of_earth -> current_value >= totem_bonus )
      return false;

    return shaman_spell_t::ready();
  }
};

// Windfury Totem Spell =====================================================

struct windfury_totem_t : public shaman_totem_t
{
  windfury_totem_t( player_t* player, const std::string& options_str ) :
    shaman_totem_t( "windfury_totem", "Windfury Totem", player, options_str )
  {
    shaman_t* p = player -> cast_shaman();
    
    totem_bonus  = p -> player_data.effect_base_value( 8515, E_APPLY_AREA_AURA_RAID, A_319 );
  }

  virtual void execute()
  {
    shaman_totem_t::execute();

    if ( sim -> overrides.windfury_totem == 0 )
    {
      sim -> auras.windfury_totem -> buff_duration = totem_duration;
      sim -> auras.windfury_totem -> trigger( 1, totem_bonus );
    }
  }

  virtual bool ready()
  {
    if( sim -> auras.windfury_totem -> current_value >= totem_bonus )
      return false;

    return shaman_spell_t::ready();
  }
};

// Wrath of Air Totem Spell =================================================

struct wrath_of_air_totem_t : public shaman_totem_t
{
  wrath_of_air_totem_t( player_t* player, const std::string& options_str ) :
    shaman_totem_t( "wrath_of_air_totem", "Wrath of Air Totem", player, options_str )
  {
    shaman_t* p = player -> cast_shaman();

    totem_bonus  = p -> player_data.effect_base_value( 2895, E_APPLY_AREA_AURA_RAID, A_MOD_CASTING_SPEED_NOT_STACK );
  }

  virtual void execute()
  {
    shaman_totem_t::execute();

    if ( sim -> overrides.wrath_of_air == 0 )
    {
      sim -> auras.wrath_of_air -> buff_duration = totem_duration;
      sim -> auras.wrath_of_air -> trigger( 1, totem_bonus );
    }
  }

  virtual bool ready()
  {
    if ( sim -> auras.wrath_of_air -> check() )
      return false;

    return shaman_spell_t::ready();
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
    shaman_spell_t( "flametongue_weapon", "Flametongue Weapon", player ), 
    bonus_power( 0 ), main( 0 ), off( 0 )
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
    
    // Spell damage scaling is defined in "Flametongue Weapon (Passive), id 10400"
    bonus_power  = p -> player_data.effect_min( 10400, p -> level, E_APPLY_AURA, A_MOD_DAMAGE_DONE );
    bonus_power *= 1.0 + p -> talent_elemental_weapons -> effect_base_value( 1 ) / 100.0;
    harmful      = false;
    may_miss     = false;
    
    p -> flametongue_weapon_spell = new flametongue_weapon_spell_t( p );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

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
  
  virtual double gcd() SC_CONST 
  { 
    return player -> in_combat ? shaman_spell_t::gcd() : 0; 
  }
};

// Windfury Weapon Spell ====================================================

struct windfury_weapon_t : public shaman_spell_t
{
  double bonus_power;
  int    main, off;

  windfury_weapon_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "windfury_weapon", "Windfury Weapon", player ), 
    bonus_power( 0 ), main( 0 ), off( 0 )
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

    bonus_power  = p -> player_data.effect_min( spell_id(), p -> level, E_DUMMY );
    harmful      = false;
    may_miss     = false;
    
    p -> windfury_weapon_attack = new windfury_weapon_attack_t( p );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

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
  
  virtual double gcd() SC_CONST 
  { 
    return player -> in_combat ? shaman_spell_t::gcd() : 0; 
  }
};

// =========================================================================
// Shaman Shields
// =========================================================================

// Lightning Shield Spell =====================================================

struct lightning_shield_t : public shaman_spell_t
{
  lightning_shield_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "lightning_shield", "Lightning Shield", player )
  {
    shaman_t* p = player -> cast_shaman();
    harmful     = false;

    p -> active_lightning_charge = new lightning_charge_t( p ) ;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    shaman_t* p = player -> cast_shaman();
    p -> buffs_water_shield     -> expire();
    p -> buffs_lightning_shield -> trigger();
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();

    if ( p -> buffs_lightning_shield -> check() )
      return false;

    return shaman_spell_t::ready();
  }
  
  virtual double gcd() SC_CONST 
  { 
    return player -> in_combat ? shaman_spell_t::gcd() : 0; 
  }
};

// Water Shield Spell =========================================================

struct water_shield_t : public shaman_spell_t
{
  double bonus;
  
  water_shield_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "water_shield", "Water Shield", player ), bonus( 0.0 )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    
    shaman_t* p  = player -> cast_shaman();
    harmful      = false;
    bonus        = p -> player_data.effect_min( spell_id(), p -> level, E_APPLY_AURA, A_MOD_POWER_REGEN );
    bonus       *= 1.0 +
      p -> talent_improved_shields -> mod_additive( P_GENERIC ) +
      p -> glyph_water_shield -> mod_additive( P_EFFECT_2 ) / 100.0;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();
    
    shaman_t* p = player -> cast_shaman();
    p -> buffs_lightning_shield -> expire();
    p -> buffs_water_shield -> trigger( initial_stacks(), bonus );
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();
    if ( p -> buffs_water_shield -> check() )
      return false;
    return shaman_spell_t::ready();
  }
  
  virtual double gcd() SC_CONST 
  { 
    return player -> in_combat ? shaman_spell_t::gcd() : 0; 
  }
};

// ==========================================================================
// Shaman Passive Buffs
// ==========================================================================

struct maelstrom_weapon_t : public new_buff_t
{
  maelstrom_weapon_t( player_t *         p,
                      const char* n,
                      uint32_t           id ) :
    new_buff_t( p, n, id ) { }

  virtual bool trigger( int, double, double chance )
  {
    bool      can_increase = current_stack <  max_stack;
    bool            result = false;
    shaman_t*            p = player -> cast_shaman();

    // This probably should not be a straight up +20% proc chance
    chance                += p -> sets -> set( SET_T8_4PC_MELEE ) -> mod_additive( P_PROC_FREQUENCY );

    result = new_buff_t::trigger( default_stack_charge, -1, chance );

    if ( result && p -> set_bonus.tier10_4pc_melee() && ( can_increase && current_stack == max_stack ) )
      p -> buffs_maelstrom_power -> trigger();
    
    return result;
  }
};

struct elemental_devastation_t : public new_buff_t
{
  elemental_devastation_t( player_t *         p,
                           const char*        n,
                           uint32_t           id ) :
    new_buff_t( p, n, id ) 
  { 
    if ( ! p -> player_data.spell_exists( id ) )
      return;

    // Duration has to be parsed out from the triggered spell
    uint32_t trigger = p -> player_data.effect_trigger_spell_id( id, E_APPLY_AURA, A_PROC_TRIGGER_SPELL_WITH_VALUE );
    buff_duration = p -> player_data.spell_duration( trigger );
    
    // And fix atomic, as it's a triggered spell, but not really .. sigh
    single = e_data[ 0 ];
  }
};

struct lightning_shield_buff_t : public new_buff_t
{
  lightning_shield_buff_t( player_t *         p,
                           const char*        n,
                           uint32_t           id ) :
    new_buff_t( p, n, id ) 
  { 
    shaman_t* s = player -> cast_shaman();

    if ( ! p -> player_data.spell_exists( id ) )
      return;
      
    // This requires rolling thunder checking for max stack
    if ( s -> talent_rolling_thunder -> rank() > 0 )
      max_stack = (int) s -> talent_rolling_thunder -> base_value( E_APPLY_AURA, A_PROC_TRIGGER_SPELL );
      
    _init_buff_t();
  }
};

struct searing_flames_buff_t : public new_buff_t
{
  searing_flames_buff_t( player_t *         p,
                         const char*        n,
                         uint32_t           id ) :
    new_buff_t( p, n, id, 1.0, true ) // Quiet buff, dont show in report
  {
    if ( ! p -> player_data.spell_exists( id ) )
      return;

    // The default chance is in the script dummy effect base value
    default_chance     = p -> player_data.effect_base_value( id, E_APPLY_AURA ) / 100.0;

    // Various other things are specified in the actual debuff placed on the target
    buff_duration      = p -> player_data.spell_duration( 77661 );
    max_stack          = p -> player_data.spell_max_stacks( 77661 );

    _init_buff_t();
  }
};

struct unleash_elements_buff_t : public new_buff_t
{
  double bonus;
  unleash_elements_buff_t( player_t *         p,
                           const char*        n,
                           uint32_t           id ) :
    new_buff_t( p, n, id, 1.0, false ), bonus( 0.0 )
  {
    if ( ! p -> player_data.spell_exists( id ) )
      return;

    shaman_t* s = player -> cast_shaman();
    bonus       = s -> talent_elemental_weapons -> effect_base_value( 2 ) / 100.0;

    _init_buff_t();
  }
  
  virtual double base_value( effect_type_t type, effect_subtype_t sub_type, int misc_value, int misc_value2 ) SC_CONST
  {
    return new_buff_t::base_value( type, sub_type, misc_value, misc_value2 ) * ( 1.0 + bonus );
  }
};

struct maelstrom_power_t : public new_buff_t
{
  maelstrom_power_t( player_t *         p,
                     const char*        n,
                     uint32_t           id ) :
    new_buff_t( p, n, id, 1.0, false )
  {
    if ( ! p -> player_data.spell_exists( id ) )
      return;

    // Proc chance is in the base spell, 70832
    default_chance = p -> player_data.effect_base_value( 70832, E_APPLY_AURA, A_DUMMY ) / 100.0;

    _init_buff_t();
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
  if ( name == "unleash_elements"        ) return new         unleash_elements_t( this, options_str );
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

// shaman_t::init_talents ======================================================

void shaman_t::init_talents()
{
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

  player_t::init_talents(); 
}

// shaman_t::init_spells ======================================================

void shaman_t::init_spells()
{
  // New set bonus system
  uint32_t set_bonuses[N_TIER][N_TIER_BONUS] = {
    //  C2P    C4P    M2P    M4P    T2P    T4P
    { 38443, 38436, 38429, 38432,     0,     0 }, // Tier6
    { 60164, 60165, 60168, 60169,     0,     0 }, // Tier7
    { 64925, 64928, 64916, 64917,     0,     0 }, // Tier8
    { 67227, 67228, 67220, 67221,     0,     0 }, // Tier9
    { 70811, 70817, 70830, 70832,     0,     0 }, // Tier10
    { 90503, 90505, 90501, 90502,     0,     0 }, // Tier11
    {     0,     0,     0,     0,     0,     0 },
  };

  player_t::init_spells();

  // Tree Specialization
  spec_elemental_fury         = new passive_spell_t( this, "elemental_fury",     60188 );
  spec_shamanism              = new passive_spell_t( this, "shamanism",          62099 );

  spec_mental_quickness       = new passive_spell_t( this, "mental_quickness",   30814 );
  spec_dual_wield             = new passive_spell_t( this, "dual_wield",         86629 );
  spec_primal_wisdom          = new passive_spell_t( this, "primal_wisdom",      51522 );

  mail_specialization         = new passive_spell_t( this, "mail_specialization", 86529 );
  
  // Masteries
  mastery_elemental_overload  = new mastery_t( this, "elemental_overload", 77222, TREE_ELEMENTAL );
  mastery_enhanced_elements   = new mastery_t( this, "enhanced_elements",  77223, TREE_ENHANCEMENT );
  
  // Glyphs
  glyph_feral_spirit          = new glyph_t( this, "Glyph of Feral Spirit" );
  glyph_fire_elemental_totem  = new glyph_t( this, "Glyph of Fire Elemental Totem" );
  glyph_flame_shock           = new glyph_t( this, "Glyph of Flame Shock" );
  glyph_flametongue_weapon    = new glyph_t( this, "Glyph of Flametongue Weapon" );
  glyph_lava_burst            = new glyph_t( this, "Glyph of Lava Burst" );
  glyph_lava_lash             = new glyph_t( this, "Glyph of Lava Lash" );
  glyph_lightning_bolt        = new glyph_t( this, "Glyph of Lightning Bolt" );
  glyph_stormstrike           = new glyph_t( this, "Glyph of Stormstrike" );
  glyph_water_shield          = new glyph_t( this, "Glyph of Water Shield" );
  glyph_windfury_weapon       = new glyph_t( this, "Glyph of Windfury Weapon" );
  
  glyph_chain_lightning       = new glyph_t( this, "Glyph of Chain Lightning" );
  glyph_shocking              = new glyph_t( this, "Glyph of Shocking" );
  glyph_thunder               = new glyph_t( this, "Glyph of Thunder" );
  
  glyph_thunderstorm          = new glyph_t( this, "Glyph of Thunderstorm" );
  
  sets                        = new set_bonus_array_t( this, set_bonuses );
}

// shaman_t::init_glyphs ======================================================

void shaman_t::init_glyphs()
{
  std::vector<std::string> glyph_names;
  int num_glyphs = util_t::string_split( glyph_names, glyphs_str, ",/" );

  for ( int i=0; i < num_glyphs; i++ )
  {
    std::string& n = glyph_names[ i ];

    if      ( n == "chain_lightning"      ) glyph_chain_lightning -> enable();
    else if ( n == "feral_spirit"         ) glyph_feral_spirit -> enable();
    else if ( n == "fire_elemental_totem" ) glyph_fire_elemental_totem -> enable();
    else if ( n == "flame_shock"          ) glyph_flame_shock -> enable();
    else if ( n == "flametongue_weapon"   ) glyph_flametongue_weapon -> enable();
    else if ( n == "lava_burst"           ) glyph_lava_burst -> enable();
    else if ( n == "lava_lash"            ) glyph_lava_lash -> enable();
    else if ( n == "lightning_bolt"       ) glyph_lightning_bolt -> enable();
    else if ( n == "shocking"             ) glyph_shocking -> enable();
    else if ( n == "stormstrike"          ) glyph_stormstrike -> enable();
    else if ( n == "thunder"              ) glyph_thunder -> enable();
    else if ( n == "thunderstorm"         ) glyph_thunderstorm -> enable();
    else if ( n == "water_shield"         ) glyph_water_shield -> enable();
    else if ( n == "windfury_weapon"      ) glyph_windfury_weapon -> enable();
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

  attribute_multiplier_initial[ ATTR_STAMINA   ] *= 1.0 + 
    talent_toughness -> base_value( E_APPLY_AURA, A_MOD_TOTAL_STAT_PERCENTAGE );
  base_attack_expertise = talent_unleashed_rage -> base_value( E_APPLY_AURA, A_MOD_EXPERTISE );

  base_attack_power = ( level * 2 ) - 30;
  initial_attack_power_per_strength = 1.0;
  initial_attack_power_per_agility  = 2.0;
  initial_spell_power_per_intellect = 1.0;

  health_per_stamina = 10;
  mana_per_intellect = 15;

  base_spell_crit  += talent_acuity -> base_value();
  base_attack_crit += talent_acuity -> base_value();

  if ( set_bonus.tier6_2pc_caster() )
  {
    // Simply assume the totems are out all the time.

    enchant.spell_power += 45;
    enchant.crit_rating += 35;
    enchant.mp5         += 15;
  }

  distance = ( primary_tree() == TREE_ENHANCEMENT ) ? 3 : 30;

  diminished_kfactor    = 0.009880;
  diminished_dodge_capi = 0.006870;
  diminished_parry_capi = 0.006870;
}

// shaman_t::init_scaling ====================================================

void shaman_t::init_scaling()
{
  player_t::init_scaling();

  if ( primary_tree() == TREE_ENHANCEMENT )
  {
    scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = 1;
    scales_with[ STAT_WEAPON_OFFHAND_SPEED  ] = sim -> weapon_speed_scale_factors;
  }

  // Elemental Precision treats Spirit like Spell Hit Rating
  if( talent_elemental_precision -> rank() && sim -> scaling -> scale_stat == STAT_SPIRIT )
  {
    double v = sim -> scaling -> scale_value;
    invert_spirit_scaling = 1;
    attribute_initial[ ATTR_SPIRIT ] -= v * 2;
  }

}

// shaman_t::init_buffs ======================================================

void shaman_t::init_buffs()
{
  player_t::init_buffs();

  buffs_flurry                  = new new_buff_t             ( this, "flurry",                 talent_flurry               -> effect_trigger_spell( 1 ) );
  buffs_stormstrike             = new new_buff_t             ( this, "stormstrike",            talent_stormstrike          -> spell_id() );
  buffs_shamanistic_rage        = new new_buff_t             ( this, "shamanistic_rage",       talent_shamanistic_rage     -> spell_id() );
  buffs_elemental_focus         = new new_buff_t             ( this, "elemental_focus",        talent_elemental_focus      -> effect_trigger_spell( 1 ) );
  buffs_natures_swiftness       = new new_buff_t             ( this, "natures_swiftness",      talent_natures_swiftness    -> spell_id() );
  buffs_water_shield            = new new_buff_t             ( this, "water_shield",           player_data.find_class_spell( type, "Water Shield" ) );
  // For now, elemental mastery will need 2 buffs, 1 to trigger the insta cast, and a second for the haste/damage buff
  buffs_elemental_mastery_insta = new new_buff_t             ( this, "elemental_mastery",      talent_elemental_mastery    -> spell_id() );
  // Note the chance override, as the spell itself does not have a proc chance
  buffs_elemental_mastery       = new new_buff_t             ( this, "elemental_mastery_buff", talent_elemental_mastery    -> effect_trigger_spell( 2 ), 1.0 );
  
  buffs_elemental_devastation   = new elemental_devastation_t( this, "elemental_devastation",  talent_elemental_devastation -> spell_id() );
  buffs_maelstrom_weapon        = new maelstrom_weapon_t     ( this, "maelstrom_weapon",       talent_maelstrom_weapon      -> effect_trigger_spell( 1 ) );  
  buffs_lightning_shield        = new lightning_shield_buff_t( this, "lightning_shield",       player_data.find_class_spell( type, "Lightning Shield" ) );
  buffs_unleash_wind            = new unleash_elements_buff_t( this, "unleash_wind",           73681 );
  buffs_unleash_flame           = new unleash_elements_buff_t( this, "unleash_flame",          73683 );
  // Searing flames needs heavy trickery to get things correct
  buffs_searing_flames          = new searing_flames_buff_t  ( this, "searing_flames",         talent_searing_flames        -> spell_id() );
  
  // Set bonuses
  buffs_elemental_rage          = new new_buff_t             ( this, "elemental_rage",         70829 ); // Enhancement Tier10 2PC
  buffs_maelstrom_power         = new maelstrom_power_t      ( this, "maelstrom_power",        70831 ); // Enhancement Tier10 4PC
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
  procs_maelstrom_weapon   = get_proc( "maelstrom_weapon"   );
  procs_static_shock       = get_proc( "static_shock"       );
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
      if ( level > 80 )
        action_list_str  = "flask,type=winds/food,type=seafood_magnifique_feast";
      else
        action_list_str  = "flask,type=endless_rage/food,type=fish_feast";
        
      action_list_str +="/windfury_weapon,weapon=main/flametongue_weapon,weapon=off";
      action_list_str += "/strength_of_earth_totem/windfury_totem/mana_spring_totem/lightning_shield";
      if ( talent_feral_spirit -> rank() ) action_list_str += "/spirit_wolf";
      if ( level > 80 )
        action_list_str += "/tolvir_potion,if=!in_combat";
      else
        action_list_str += "/speed_potion,if=!in_combat";
      action_list_str += "/snapshot_stats";
      action_list_str += "/auto_attack";
      action_list_str += "/wind_shear";
      action_list_str += "/bloodlust,health_percentage<=25/bloodlust,time_to_die<=60";
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

      if ( level > 80 )
        action_list_str += "/tolvir_potion,if=buff.bloodlust.react";
      else
        action_list_str += "/speed_potion,if=buff.bloodlust.react";
        
      action_list_str += "/lightning_bolt,if=buff.maelstrom_weapon.stack=5&buff.maelstrom_weapon.react";
      
      if ( set_bonus.tier10_4pc_melee() )
      {
        action_list_str += "/fire_elemental_totem,if=buff.maelstrom_power.react,time_to_die>=125";
        action_list_str += "/fire_elemental_totem,time_to_die<=124";
      }
      else
        action_list_str += "/fire_elemental_totem";
        
      action_list_str += "/searing_totem";
      action_list_str += "/lava_lash";
      if ( level > 80 ) action_list_str += "/unleash_elements";
      action_list_str += "/flame_shock,if=!ticking";
      action_list_str += "/earth_shock";
      if ( talent_stormstrike -> rank() ) action_list_str += "/stormstrike";
      action_list_str += "/fire_nova";
      if ( level < 81 )
      {
        if ( talent_shamanistic_rage -> rank() ) action_list_str += "/shamanistic_rage,tier10_2pc_melee=1";
        if ( talent_shamanistic_rage -> rank() ) action_list_str += "/shamanistic_rage";
      }
    }
    else
    {
      if ( level > 80 )
        action_list_str  = "flask,type=draconic_mind/food,type=seafood_magnifique_feast";
      else
        action_list_str  = "flask,type=frost_wyrm/food,type=fish_feast";
        
      action_list_str += "/flametongue_weapon,weapon=main/lightning_shield";
      action_list_str += "/mana_spring_totem/wrath_of_air_totem";
      action_list_str += "/snapshot_stats";
      
      if ( level > 80 )
        action_list_str += "/volcanic_potion,if=!in_combat";
      else
        action_list_str += "/speed_potion,if=!in_combat";
      
      action_list_str += "/wind_shear";
      action_list_str += "/bloodlust,health_percentage<=25/bloodlust,time_to_die<=60";
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
      
      if ( level > 80 )
        action_list_str += "/volcanic_potion,if=buff.bloodlust.react";
      else
      {
        if ( set_bonus.tier9_2pc_caster() || set_bonus.tier10_2pc_caster() )
          action_list_str += "/wild_magic_potion,if=buff.bloodlust.react";
        else
          action_list_str += "/speed_potion";
      }
      
      if ( talent_elemental_mastery -> rank() )
      {
        if ( level > 80 )
          action_list_str += "/elemental_mastery,time_to_die<=10";
        else
        {
          action_list_str += "/elemental_mastery,time_to_die<=17";
          action_list_str += "/elemental_mastery,if=!buff.bloodlust.react";
        }
      }
      
      if ( talent_fulmination -> rank() )
        action_list_str += "/earth_shock,if=buff.lightning_shield.stack=9";
      action_list_str += "/flame_shock,if=!ticking";
      // Unleash elements for elemental is a downgrade in dps ...
      //if ( level >= 81 )
      //  action_list_str += "/unleash_elements";
      if ( level >= 75 ) action_list_str += "/lava_burst,if=(dot.flame_shock.remains-cast_time)>=0.05";
      action_list_str += "/fire_elemental_totem";
      action_list_str += "/searing_totem";
      action_list_str += "/chain_lightning,if=target.adds>2";
      if ( ! ( set_bonus.tier9_4pc_caster() || set_bonus.tier10_2pc_caster() || set_bonus.tier11_4pc_caster() || level > 80 ))
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

// shaman_t::matching_gear_multiplier =============================================

double shaman_t::matching_gear_multiplier( const attribute_type attr ) SC_CONST
{
  if ( primary_tree() == TREE_ENHANCEMENT )
  {
    if ( attr == ATTR_AGILITY )
      return mail_specialization -> base_value() / 100.0;
  }
  else
  {
    if ( attr == ATTR_INTELLECT )
      return mail_specialization -> base_value() / 100.0;
  }

  return 0.0;
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

  hit += ( talent_elemental_precision -> base_value( E_APPLY_AURA, A_MOD_RATING_FROM_STAT ) * 
    ( spirit() - attribute_base[ ATTR_SPIRIT ] ) ) / rating.spell_hit;

  return hit;
}

// shaman_t::composite_attack_power_multiplier ==========================================

double shaman_t::composite_attack_power_multiplier() SC_CONST
{
  double multiplier = player_t::composite_attack_power_multiplier();

  if ( buffs_maelstrom_power -> up() )
    multiplier *= 1.0 + buffs_maelstrom_power -> base_value();

  return multiplier;
}

// shaman_t::composite_spell_power ==========================================

double shaman_t::composite_spell_power( const school_type school ) SC_CONST
{
  double sp = player_t::composite_spell_power( school );

  if ( primary_tree() == TREE_ENHANCEMENT )
    sp += composite_attack_power_multiplier() * composite_attack_power() * spec_mental_quickness -> base_value( E_APPLY_AURA, A_MOD_SPELL_DAMAGE_OF_ATTACK_POWER );

  if ( main_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
    sp += main_hand_weapon.buff_value;

  if ( off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
    sp += off_hand_weapon.buff_value;

  return sp;
}

// shaman_t::composite_player_multiplier ====================================

double shaman_t::composite_player_multiplier( const school_type school ) SC_CONST
{
  double m = player_t::composite_player_multiplier( school );
  
  if ( school == SCHOOL_FIRE || school == SCHOOL_FROST || school == SCHOOL_NATURE )
    m *= 1.0 + composite_mastery() * mastery_enhanced_elements -> base_value( E_APPLY_AURA, A_DUMMY );
 
  return m; 
}


// shaman_t::regen  =======================================================

void shaman_t::regen( double periodicity )
{
  mana_regen_while_casting = ( primary_tree() == TREE_RESTORATION ) ? 0.50 : 0.0;

  player_t::regen( periodicity );

  if ( buffs_water_shield -> up() )
  {
    double water_shield_regen = periodicity * buffs_water_shield -> base_value() / 5.0;

    resource_gain( RESOURCE_MANA, water_shield_regen, gains_water_shield );
  }
}

// shaman_t::combat_begin =================================================

void shaman_t::combat_begin()
{
  player_t::combat_begin();

  if ( talent_elemental_oath -> rank() ) sim -> auras.elemental_oath -> trigger();

  double ur = talent_unleashed_rage -> base_value( E_APPLY_AREA_AURA_RAID, A_MOD_ATTACK_POWER_PCT );

  if ( ur != 0 && ur >= sim -> auras.unleashed_rage -> current_value )
  {
    sim -> auras.unleashed_rage -> trigger( 1, ur );
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
  if ( strstr( s, "raging_elements" ) )
  {
    bool is_caster = ( strstr( s, "headpiece"     ) ||
                       strstr( s, "shoulderwraps" ) ||
                       strstr( s, "hauberk"       ) ||
                       strstr( s, "kilt"          ) ||
                       strstr( s, "gloves"        ) );

    bool is_melee = ( strstr( s, "helmet"         ) ||
                      strstr( s, "spaulders"      ) ||
                      strstr( s, "cuirass"        ) ||
                      strstr( s, "legguards"      ) ||
                      strstr( s, "grips"          ) );

    if ( is_caster ) return SET_T11_CASTER;
    if ( is_melee  ) return SET_T11_MELEE;
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
    p -> buffs.bloodlust  = new buff_t( p, "bloodlust", 1, 40.0 );
    p -> buffs.exhaustion = new buff_t( p, "exhaustion", 1, 600.0 );
    p -> buffs.mana_tide  = new buff_t( p, "mana_tide", 16190 );
  }
}

// player_t::shaman_combat_begin ============================================

void player_t::shaman_combat_begin( sim_t* sim )
{
  if ( sim -> overrides.elemental_oath    ) sim -> auras.elemental_oath    -> override();
  if ( sim -> overrides.flametongue_totem ) sim -> auras.flametongue_totem -> override( 1, 0.10 );
  if ( sim -> overrides.unleashed_rage    ) sim -> auras.unleashed_rage    -> override( 1, 0.10 );
  if ( sim -> overrides.windfury_totem    ) sim -> auras.windfury_totem    -> override( 1, 0.10 );
  if ( sim -> overrides.wrath_of_air      ) sim -> auras.wrath_of_air      -> override( 1, 0.05 );
  
  if ( sim -> overrides.mana_spring_totem ) 
    sim -> auras.mana_spring_totem -> override( 1, sim -> sim_data.effect_min( 5677, ( sim -> P403 ) ? 85 : 80, E_APPLY_AREA_AURA_RAID, A_MOD_POWER_REGEN ) );
  if ( sim -> overrides.strength_of_earth ) 
    sim -> auras.strength_of_earth -> override( 1, sim -> sim_data.effect_min( 8076, ( sim -> P403 ) ? 85 : 80, E_APPLY_AREA_AURA_RAID, A_MOD_STAT ) );
}
