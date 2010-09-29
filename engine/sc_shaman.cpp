// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

// ==========================================================================
// TODO
// ==========================================================================
// Spirit Walker's Grace Ability
// Mail Specialization -> use correct spell data information instead of hardcoding
// Elemental Precision does not benefit from spirit enchants or gems (nightmare tear, all stats chest), this is a bug.
// Migrate rest of the trigger_foo into a separate secondary effect, where applicable
// Validate lvl85 values
// Searing Totem, Searing Flames affected by player crit?
// Elemental talent validation
// Verify Flametongue Weapon damage ranges
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Shaman
// ==========================================================================

enum element_type_t { ELEMENT_NONE=0, ELEMENT_AIR, ELEMENT_EARTH, ELEMENT_FIRE, ELEMENT_WATER, ELEMENT_MAX };

enum imbue_type_t { IMBUE_NONE=0, FLAMETONGUE_IMBUE, WINDFURY_IMBUE };

struct new_buff_t : public buff_t
{
  int                       default_stack_charge;
  const spelleffect_data_t* e_data[MAX_EFFECTS];
  const spelleffect_data_t* single;
  
  new_buff_t( player_t*, const std::string&, uint32_t, 
    double override_chance = 0.0, bool quiet = false, bool reverse = false, int rng_type = RNG_CYCLIC );

  virtual bool   trigger( int stacks = 1, double value = -1.0, double chance = -1.0 );
  virtual double base_value( effect_subtype_t stype = A_NONE, effect_type_t type = E_APPLY_AURA, int misc_value = 0 );
};

// ==========================================================================
// Generic Passive Buff system
// ==========================================================================

new_buff_t::new_buff_t( player_t*          p, 
                        const std::string& n,
                        uint32_t           id,
                        double             override_chance,
                        bool               quiet,
                        bool               reverse,
                        int                rng_type ) :
  buff_t( p, n, 1, 0, 0, override_chance, quiet, reverse, rng_type, id ),
  default_stack_charge( 1 ), single( 0 )
{
  uint32_t effect_id;
  int        effects = 0;
  
  memset(e_data, 0, sizeof(e_data));
  
  // Find some stuff for the buff, generically
  if ( id > 0 && player -> player_data.spell_exists( id ) )
  {
    max_stack            = player -> player_data.spell_max_stacks( id );
    duration             = player -> player_data.spell_duration( id );
    cooldown             = player -> player_data.spell_cooldown( id );
    default_stack_charge = player -> player_data.spell_initial_stacks( id );

    if ( override_chance == 0 )
    {
      default_chance     = player -> player_data.spell_proc_chance( id );
      
      if ( default_chance == 0.0 )
        default_chance   = 1.0;
    }

    // If there's no max stack, set max stack to proc charges
    if ( max_stack == 0 )
      max_stack          = player -> player_data.spell_initial_stacks( id );

    // A duration of < 0 in blizzard speak is infinite -> change to 0
    if ( duration < 0 )
      duration           = 0.0;

    // Some spells seem to have a max_stack but no initial stacks, so set 
    // initial stacks to 1 if it is 0 in spell data
    if ( default_stack_charge == 0 ) 
      default_stack_charge = 1;

    // Some spells do not have a max stack, but we need to at least have 
    // a single application for a buff. Set the max stack (if 0) to the 
    // initial stack value
    if ( max_stack == 0 )
      max_stack            = default_stack_charge;

    // Map effect data for this buff
    for ( int i = 1; i <= MAX_EFFECTS; i++ ) 
    {
      if ( ! ( effect_id = player -> player_data.spell_effect_id( id, i ) ) )
        continue;
      
      e_data[ i - 1 ] = player -> player_data.m_effects_index[ effect_id ];

      // Trigger spells will not be used in anything for now
      if ( e_data[ i - 1 ] -> trigger_spell > 0 )
        continue;

      effects++;
    }
    
    // Optimize if there's only a single effect to use
    if ( effects < 2 )
    {
      for ( int i = 0; i < MAX_EFFECTS; i++ ) 
      {
        if ( ! e_data[ i ] || e_data[ i ] -> trigger_spell > 0 )
          continue;
        
        single = e_data[ i ];
        break;
      }
    }
    
    if ( sim -> debug )
    {
      log_t::output( sim, "Initializing Shaman New Buff %s id=%d max_stack=%d default_charges=%d cooldown=%.2f duration=%.2f default_chance=%.2f atomic=%s",
        name_str.c_str(), id, max_stack, default_stack_charge, cooldown, duration, default_chance,
        single ? "true" : "false");
    }
  }
  
  init();
}

bool new_buff_t::trigger( int stacks, double value, double chance)
{
  // For buffs going up, use the default_stack_charge as the amount
  if ( current_stack == 0 )
    return buff_t::trigger( default_stack_charge, value, chance );
  else
    return buff_t::trigger( stacks, value, chance );
}

double new_buff_t::base_value( effect_subtype_t stype, effect_type_t type, int misc_value )
{
  // Atomic shortcut for singular buffs, which should be quite a few
  if ( single ) return single -> base_value;
  
  for ( int i = 0; i < MAX_EFFECTS; i++ )
  {
    if ( ! e_data[ i ] )
      continue;
   
    // log_t::output( sim, "effect=%d id=%d type=%d sub_type=%d base_value=%d",
    //  e_data[ i ] -> index, e_data[ i ] -> id, e_data[ i ] -> type, e_data[ i ] -> subtype, e_data[ i ] -> base_value );
      
    if ( e_data[ i ] -> type == type && 
      ( ! stype || e_data[ i ] -> subtype == stype ) && 
      ( ! misc_value || e_data[ i ] -> misc_value == misc_value ) )
      return e_data[ i ] -> base_value;
  }
  
  return 0.0;
}

struct shaman_t : public player_t
{
  // Active
  action_t* active_fire_totem;
  action_t* active_flame_shock;
  action_t* active_lightning_charge;
  action_t* active_lightning_bolt_dot;
  action_t* active_searing_flames_dot;

  // Buffs
  buff_t* buffs_tier10_2pc_melee;
  buff_t* buffs_tier10_4pc_melee;
  
  // New Buffs
  new_buff_t* buffs_flurry;
  new_buff_t* buffs_stormstrike;
  new_buff_t* buffs_shamanistic_rage;
  new_buff_t* buffs_elemental_focus;
  new_buff_t* buffs_elemental_devastation;
  new_buff_t* buffs_maelstrom_weapon;
  new_buff_t* buffs_elemental_mastery_insta;
  new_buff_t* buffs_elemental_mastery;
  new_buff_t* buffs_lightning_shield;
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
  passive_spell_t* mastery_elemental_overload;
  passive_spell_t* mastery_enhanced_elements;

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
  
  bool mail_specialization;

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

    // Tree Specialization
    spec_elemental_fury             = new passive_spell_t( this, "elemental_fury",     60188 );
    spec_shamanism                  = new passive_spell_t( this, "shamanism",          62099 );

    spec_mental_quickness           = new passive_spell_t( this, "mental_quickness",   30814 );
    spec_dual_wield                 = new passive_spell_t( this, "dual_wield",         86629 );
    spec_primal_wisdom              = new passive_spell_t( this, "primal_wisdom",      51522 );

    // Masteries
    mastery_elemental_overload      = new passive_spell_t( this, "elemental_overload", 77222 );
    mastery_enhanced_elements       = new passive_spell_t( this, "enhanced_elements",  77223 );

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
    
    // Mail spec enabled by default
    mail_specialization      = true;
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
  virtual void      interrupt();
  virtual void      clear_debuffs();
  virtual double    composite_attribute_multiplier( int attr ) SC_CONST;
  virtual double    composite_attack_power() SC_CONST;
  virtual double    composite_attack_power_multiplier() SC_CONST;
  virtual double    composite_spell_hit() SC_CONST;
  virtual double    composite_spell_power( const school_type school ) SC_CONST;
  virtual std::vector<talent_translation_t>& get_talent_list();
  virtual std::vector<option_t>& get_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name );
  virtual void      create_pets();
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() SC_CONST { return RESOURCE_MANA; }
  virtual int       primary_role() SC_CONST     { return talent_stormstrike -> rank() ? ROLE_HYBRID : ROLE_SPELL; }
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
  uint32_t id;
  
  shaman_attack_t( const char* n, player_t* player, school_type s, int t, bool special ) :
    attack_t( n, player, RESOURCE_MANA, s, t, special ), id( 0 )
  {
    shaman_t* p = player -> cast_shaman();
    if ( p -> primary_tree() == TREE_ENHANCEMENT ) 
      base_hit += p -> spec_dual_wield -> base_value( E_APPLY_AURA, A_MOD_HIT_CHANCE ) / 100.0;
  }
  
  shaman_attack_t( const char* n, player_t* player, uint32_t id ) :
    attack_t( n, player, RESOURCE_MANA, SCHOOL_PHYSICAL, 0 ), id( id )
  {
    const spelleffect_data_t * e;
    uint32_t effect_id;
    shaman_t* p = player -> cast_shaman();

    assert( p -> player_data.spell_exists( id ) );

    special              = true;
    may_crit             = true;
    
    school               = spell_id_t::get_school_type( p -> player_data.spell_school_mask( id ) );
    stats -> school      = school;
    resource             = p -> player_data.spell_power_type( id );

    trigger_gcd          = p -> player_data.spell_gcd( id );
    base_execute_time    = p -> player_data.spell_cast_time( id, p -> level );
    cooldown -> duration = p -> player_data.spell_cooldown( id );

    // Enhancement melee hit specialization is combined into the dual wield talent
    if ( p -> primary_tree() == TREE_ENHANCEMENT )
    {
      base_hit          += p -> spec_dual_wield -> base_value( E_APPLY_AURA, A_MOD_HIT_CHANCE );
    }

    if ( resource == RESOURCE_MANA )
      base_cost          = (int) ( p -> resource_base[ RESOURCE_MANA ] * p -> player_data.spell_cost( id ) );
    else
      base_cost          = p -> player_data.spell_cost( id );
      
    // Do simplistic parsing of stuff, based on effect type/subtype
    for ( int i = 1; i <= MAX_EFFECTS; i++ ) 
    {
      if ( ! ( effect_id = p -> player_data.spell_effect_id( id, i ) ) )
        continue;
      
      e = p -> player_data.m_effects_index[ effect_id ];

      // Trigger spells will not be used in anything for now
      if ( e -> trigger_spell > 0 )
        continue;
        
      switch ( e -> type )
      {
        case E_WEAPON_PERCENT_DAMAGE:
          weapon_multiplier = e -> base_value / 100.0;
          break;
        // XXX: p -> type is wrong here (tho it should work), but get_player_type( int ) is not defined outside sc_data_access.cpp
        case E_WEAPON_DAMAGE:
          base_dd_min      = p -> player_data.effect_min( e -> id, p -> type, p -> level );
          base_dd_max      = p -> player_data.effect_max( e -> id, p -> type, p -> level );
          direct_power_mod = p -> player_data.effect_coeff( e -> id );
          break;
      }
    }
    
    log_debug();
  }

  virtual void execute();
  virtual double cost() SC_CONST;
  virtual void player_buff();
  virtual void assess_damage( double amount, int dmg_type );
  virtual double cost_reduction() SC_CONST;
  virtual void   log_debug() SC_CONST 
  {
    if ( sim -> debug )
    {
      log_t::output( sim, "Initializing Shaman Attack %s id=%d school=%d power=%d cost=%.2f gcd=%.2f base_cast=%.2f cooldown=%.2f min_dd=%.2f max_dd=%.2f coeff_dd=%.5f weapon_multiplier=%.2f",
        name_str.c_str(), id, school, resource, base_cost, trigger_gcd, base_execute_time, cooldown -> duration,
        base_dd_min, base_dd_max, direct_power_mod, weapon_multiplier );
    }
  }
};

// ==========================================================================
// Shaman Spell
// ==========================================================================

struct shaman_spell_t : public spell_t
{
  uint32_t                  id;
  double                    base_cost_reduction;

  shaman_spell_t( const char* n, player_t* p, const school_type s, int t ) :
    spell_t( n, p, RESOURCE_MANA, s, t ), base_cost_reduction( 0 )
  {

  }

  shaman_spell_t( const char* n, player_t* p, uint32_t id ) :
    spell_t( n, p, RESOURCE_MANA, SCHOOL_NONE, 0 ), id ( id ), base_cost_reduction( 0.0 )
  {
    const spelleffect_data_t * e;
    uint32_t effect_id;
    
    assert( p -> player_data.spell_exists( id ) );
    
    may_crit             = true;
    
    school               = spell_id_t::get_school_type( p -> player_data.spell_school_mask( id ) );
    stats -> school      = school;
    resource             = p -> player_data.spell_power_type( id );
    
    trigger_gcd          = p -> player_data.spell_gcd( id );
    base_execute_time    = p -> player_data.spell_cast_time( id, p -> level );
    cooldown -> duration = p -> player_data.spell_cooldown( id );
    
    if ( resource == RESOURCE_MANA )
      base_cost = (int) ( p -> resource_base[ RESOURCE_MANA ] * p -> player_data.spell_cost( id ) );
    else
      base_cost = p -> player_data.spell_cost( id );
      
    // Do simplistic parsing of stuff, based on effect type/subtype
    for ( int i = 1; i <= MAX_EFFECTS; i++ ) 
    {
      if ( ! ( effect_id = p -> player_data.spell_effect_id( id, i ) ) )
        continue;
      
      e = p -> player_data.m_effects_index[ effect_id ];

      // Trigger spells will not be used in anything for now
      if ( e -> trigger_spell > 0 )
        continue;
        
      switch ( e -> type )
      {
        // XXX: p -> type is wrong here (tho it should work), but get_player_type( int ) is not defined outside sc_data_access.cpp
        case E_SCHOOL_DAMAGE:
          base_dd_min      = p -> player_data.effect_min( e -> id, p -> type, p -> level );
          base_dd_max      = p -> player_data.effect_max( e -> id, p -> type, p -> level );
          direct_power_mod = p -> player_data.effect_coeff( e -> id );
          break;
        case E_APPLY_AURA:
          switch ( e -> subtype )
          {
            case A_PERIODIC_DAMAGE:
              base_td        = floor( p -> player_data.effect_min( e -> id, p -> type, p -> level ) );
              base_tick_time = p -> player_data.effect_period( e -> id );
              tick_power_mod = p -> player_data.effect_coeff( e -> id );
              num_ticks      = (int) ( p -> player_data.spell_duration( id ) / base_tick_time );
              break;
          }
          break;
      }
    }
    
    log_debug();
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
  virtual void   log_debug() SC_CONST 
  {
    if ( sim -> debug )
    {
      log_t::output( sim, "Initializing Shaman Spell %s id=%d school=%d power=%d cost=%.2f gcd=%.2f base_cast=%.2f cooldown=%.2f min_dd=%.2f max_dd=%.2f coeff_dd=%.5f td=%.2f n_ticks=%d amplitude=%.2f td_coeff=%.5f",
        name_str.c_str(), id, school, resource, base_cost, trigger_gcd, base_execute_time, cooldown -> duration,
        base_dd_min, base_dd_max, direct_power_mod, 
        base_td, num_ticks, base_tick_time, tick_power_mod );
    }
  }
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
    
    if ( o -> glyphs.feral_spirit )
      ap_per_owner += player_data.effect_base_value( 63271, E_APPLY_AURA, A_DUMMY ) / 100.0;
    
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
// - Make sure melee crits are 200%
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

  struct fire_shield_t : public spell_t
  {
    double int_multiplier;
    double sp_multiplier;
    
    fire_shield_t( player_t* player ) :
      spell_t( "fire_shield", player, RESOURCE_MANA, SCHOOL_FIRE ), 
      int_multiplier( 0.8003 ), sp_multiplier( 0.501 )
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
    
    virtual double total_spell_power() SC_CONST
    {
      double sp  = 0.0;
      pet_t* pet = player -> cast_pet();
      
      sp += pet ->  intellect() * int_multiplier;
      sp += pet -> composite_spell_power( school ) * sp_multiplier;
      return sp;
    }
  };

  struct fire_nova_t : public spell_t
  {
    double int_multiplier;
    double sp_multiplier;
    
    fire_nova_t( player_t* player ) :
      spell_t( "fire_nova", player, RESOURCE_MANA, SCHOOL_FIRE ), 
      int_multiplier( 0.8003 ), sp_multiplier( 0.501 )
    {
      aoe                  = true;
      may_crit             = true;
      direct_power_mod     = player -> player_data.effect_coeff( 12470, E_SCHOOL_DAMAGE );
      
      // 207 = 80
      base_cost            = player -> level * 2.750;
      // For now, model the cast time increase as well, see below
      base_execute_time    = player -> player_data.spell_cast_time( 12470, player -> level );
      
      // Set base_dd_min, base_dd_max on empirical results 
      // against a dummy, approximately 6.7 / 7.7 per level
      base_dd_min          = ( player -> level ) * 6.7;
      base_dd_max          = ( player -> level ) * 7.7;
    };
    
    virtual double total_spell_power() SC_CONST
    {
      double sp  = 0.0;
      pet_t* pet = player -> cast_pet();
      
      sp += pet ->  intellect() * int_multiplier;
      sp += pet -> composite_spell_power( school ) * sp_multiplier;
      return sp;
    }
  };

  struct fire_blast_t : public spell_t
  {
    double int_multiplier;
    double sp_multiplier;
    
    fire_blast_t( player_t* player ) :
      spell_t( "fire_blast", player, RESOURCE_MANA, SCHOOL_FIRE ), 
      int_multiplier( 0.8003 ), sp_multiplier( 0.501 )
    {
      may_crit             = true;
      base_cost            = ( player -> level ) * 3.554;
      base_execute_time    = 0;
      base_dd_min          = ( player -> level ) * 3.169;
      base_dd_max          = ( player -> level ) * 3.687;
      direct_power_mod     = player -> player_data.effect_coeff( 57984, E_SCHOOL_DAMAGE );
    };
    
    virtual double total_spell_power() SC_CONST
    {
      double sp  = 0.0;
      pet_t* pet = player -> cast_pet();
      
      sp += pet ->  intellect() * int_multiplier;
      sp += pet -> composite_spell_power( school ) * sp_multiplier;
      return sp;
    }
  };

  struct fire_melee_t : public attack_t
  {
    double int_multiplier;
    double sp_multiplier;
    
    fire_melee_t( player_t* player ) :
      attack_t( "fire_melee", player, RESOURCE_NONE, SCHOOL_FIRE ), 
      int_multiplier( 0.8003 ), sp_multiplier( 0.5355 )
    {
      may_crit                     = true;
      base_dd_min                  = ( player -> level ) * 4.916;
      base_dd_max                  = ( player -> level ) * 5.301;
      base_execute_time            = 0.0;
      direct_power_mod             = 1.135;
      base_spell_power_multiplier  = 1.0;
      base_attack_power_multiplier = 0.0;
      base_execute_time            = 2.0;
    }
    
    virtual double total_spell_power() SC_CONST
    {
      double sp  = 0.0;
      
      pet_t* pet = player -> cast_pet();
      
      sp += pet -> intellect() * int_multiplier;
      sp += pet -> composite_spell_power( school ) * sp_multiplier;
      
      return sp;
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
    owner_sp  = ( owner -> composite_spell_power( SCHOOL_FIRE ) - owner -> spell_power_per_intellect * owner -> intellect() ) * owner -> composite_spell_power_multiplier();
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
};

// =========================================================================
// Shaman Secondary Spells / Attacks
// =========================================================================

struct searing_flames_t : public shaman_spell_t
{
  searing_flames_t( player_t* player ) : 
    shaman_spell_t( "searing_flames", player, 77661 )
  {
    background       = true;
    may_miss         = false;      
    may_crit         = false;
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
  virtual void player_buff() { }
  
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
  lightning_charge_t( player_t* player ) :
      shaman_spell_t( "lightning_shield", player, 26364 )
  {
    // Use the same name "lightning_shield" to make sure the cost of refreshing the shield is included with the procs.
    shaman_t* p      = player -> cast_shaman();
    background       = true;
    may_crit         = true;
    base_multiplier *= 1.0 + 
      p -> talent_improved_shields -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER ) + 
      p -> set_bonus.tier7_2pc_melee() * 0.10;

    if ( p -> primary_tree() == TREE_ELEMENTAL )
      base_crit_bonus_multiplier += p -> spec_elemental_fury -> base_value();
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
  }
};

struct unleash_flame_t : public shaman_spell_t
{
  unleash_flame_t( player_t* player ) : 
    shaman_spell_t( "unleash_flame", player, 73683 )
  {
    background           = true;
    may_miss             = false;
    proc                 = true;
    
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
    shaman_spell_t( "flametongue", player, 8024 )
  {
    may_crit           = true;
    background         = true;
    proc               = true;

    reset();
  }
};

struct windfury_weapon_attack_t : public shaman_attack_t
{
  double glyph_bonus;
  
  windfury_weapon_attack_t( player_t* player ) :
    shaman_attack_t( "windfury", player, 33757 ), glyph_bonus( 0.0 )
  {
    shaman_t* p      = player -> cast_shaman();
    may_crit         = true;
    background       = true;
    base_multiplier *= 1.0 + p -> talent_elemental_weapons -> effect_base_value( 2 ) / 100.0;
    if ( p -> glyphs.windfury_weapon )
      glyph_bonus    = p -> player_data.effect_base_value( 55445, E_APPLY_AURA, A_ADD_FLAT_MODIFIER ) / 100.0;
    
    reset();
  }
  
  virtual void player_buff()
  {
    shaman_attack_t::player_buff();
    player_attack_power += weapon -> buff_value;
  }
  
  virtual double proc_chance() SC_CONST
  {
    return pp -> player_data.spell_proc_chance( id ) + glyph_bonus;
  }
};

struct unleash_wind_t : public shaman_attack_t
{
  unleash_wind_t( player_t* player ) : 
    shaman_attack_t( "unleash_wind", player, 73681 )
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
    shaman_attack_t( "stormstrike", player, 32175 )
  {
    shaman_t* p          = player -> cast_shaman();

    background           = true;
    may_miss             = false;
    
    weapon_multiplier   *= 1.0 + p -> talent_focused_strikes -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER );
    
    if ( p -> set_bonus.tier8_2pc_melee() ) 
      base_multiplier   *= 1.0 + 0.20;
  }

  virtual void execute()
  {
    // Figure out weapons
    shaman_t* p = player -> cast_shaman();
    
    weapon = p -> main_hand_attack -> weapon;
    shaman_attack_t::execute();
    
    if ( p -> off_hand_weapon.type != WEAPON_NONE && p -> off_hand_attack )
    {
      weapon = p -> off_hand_attack -> weapon;
      shaman_attack_t::execute();
    }
  }
};

// =========================================================================
// Shaman Spell Triggers
// =========================================================================

// trigger_elemental_overload ===============================================

static void trigger_elemental_overload( spell_t* s,
                                        stats_t* elemental_overload_stats )
{
  shaman_t* p = s -> player -> cast_shaman();

  if ( p -> primary_tree() != TREE_ELEMENTAL ) return;

  if ( s -> proc ) return;

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

// trigger_flametongue_weapon ===============================================

static void trigger_flametongue_weapon( attack_t* a )
{
  if ( a -> proc ) return;
  if ( a -> weapon == 0 ) return;
  if ( a -> weapon -> buff_type != FLAMETONGUE_IMBUE ) return;

  shaman_t* p = a -> player -> cast_shaman();

  // Let's try a new formula for flametongue, based on EJ and such, but with proper damage ranges
  // Scaling data comes from main spell from a dummy server-side effect. Note the weird / 77 and / 25 divisors
  // which are picked out from tooltip data. The base damage values match the tooltip for the player level.
  p -> flametongue_weapon_spell -> base_dd_min      = a -> weapon -> swing_time / 4.0 * 
    ( p -> player_data.effect_min( p -> flametongue_weapon_spell -> spell_id() , p -> level, E_DUMMY ) / 77 );
  p -> flametongue_weapon_spell -> base_dd_max      = a -> weapon -> swing_time / 4.0 * 
    ( p -> player_data.effect_max( p -> flametongue_weapon_spell -> spell_id(), p -> level, E_DUMMY ) / 25 );
  p -> flametongue_weapon_spell -> direct_power_mod = 0.03811 * a -> weapon -> swing_time;
  
  p -> flametongue_weapon_spell -> execute();
}

// trigger_windfury_weapon ================================================

static void trigger_windfury_weapon( attack_t* a )
{
  if ( a -> proc ) return;
  if ( a -> weapon == 0 ) return;
  if ( a -> weapon -> buff_type != WINDFURY_IMBUE ) return;

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
    p -> resource_gain( RESOURCE_MANA, 0.02 * p -> resource_max[ RESOURCE_MANA ], p -> gains_rolling_thunder );
    p -> buffs_lightning_shield -> trigger( 1 );
    p -> procs_rolling_thunder  -> occur();
  }
}

// trigger_static_shock =============================================

static void trigger_static_shock ( attack_t* a )
{
  shaman_t* p = a -> player -> cast_shaman();

  if ( p -> talent_static_shock -> rank() && p -> buffs_lightning_shield -> stack() )
  {
    double chance = p -> talent_static_shock -> proc_chance() + p -> set_bonus.tier9_2pc_melee() * 0.03;
    if ( p -> rng_static_shock -> roll( chance ) )
      p -> active_lightning_charge -> execute();
  }
}

// trigger_telluric_currents =============================================

static void trigger_telluric_currents ( spell_t* s )
{
  shaman_t* p = s -> player -> cast_shaman();

  if ( ! p -> talent_telluric_currents -> rank() )
    return;

  double mana_gain = s -> direct_dmg * p -> talent_telluric_currents -> base_value() / 100.0;
  p -> resource_gain( RESOURCE_MANA, mana_gain, p -> gains_telluric_currents );
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
      p -> buffs_flurry -> trigger();
    }

    // Prevent unleash wind from stacking maelstrom, as it doesnt do that
    if ( ! proc )
      p -> buffs_maelstrom_weapon -> trigger( 1, -1, weapon -> proc_chance_on_swing( p -> talent_maelstrom_weapon -> rank() * 2.0 ) );

    trigger_flametongue_weapon( this );
    trigger_windfury_weapon( this );    

    if ( p -> rng_primal_wisdom -> roll( p -> spec_primal_wisdom -> proc_chance() ) )
    {
      p -> resource_gain( RESOURCE_MANA, 0.05 * p -> resource_base[ RESOURCE_MANA ], p -> gains_primal_wisdom );
    }
  }
}

// shaman_attack_t::player_buff ============================================

void shaman_attack_t::player_buff()
{
  attack_t::player_buff();
  shaman_t* p = player -> cast_shaman();
  
  if ( p -> buffs_elemental_devastation -> up() )
  {
    player_crit += p -> buffs_elemental_devastation -> base_value() / 100.0;
  }

  if ( p -> buffs_tier10_2pc_melee -> up() )
    player_multiplier *= 1.12;
    
  if ( p -> primary_tree() == TREE_ENHANCEMENT && ( school == SCHOOL_FIRE || school == SCHOOL_FROST || school == SCHOOL_NATURE ) )
    player_multiplier *= 1.0 + ( p -> composite_mastery() * p -> mastery_enhanced_elements -> base_value( E_APPLY_AURA, A_DUMMY ) / 10000.0 );
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
    cr += p -> buffs_shamanistic_rage -> base_value( A_ADD_PCT_MODIFIER ) / 100.0;

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

  virtual double haste() SC_CONST
  {
    shaman_t* p = player -> cast_shaman();
    double h = shaman_attack_t::haste();
    if ( p -> buffs_flurry -> up() )
    {
      h *= 1.0 / ( 1.0 + ( p -> buffs_flurry -> base_value() / 100.0 + 0.05 * p -> set_bonus.tier7_4pc_melee() ) );
    }
    
    if ( p -> buffs_unleash_wind -> up() )
    {
      h *= 1.0 / ( 1.0 + ( p -> buffs_unleash_wind -> base_value( A_319 ) / 100.0 ) );
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
  double flametongue_bonus;
  double glyph_bonus;

  lava_lash_t( player_t* player, const std::string& options_str ) :
    shaman_attack_t( "lava_lash", player, 60103 ), 
    wf_cd_only( 0 ), flametongue_bonus( 0 ), glyph_bonus( 0 )
  {
    shaman_t* p = player -> cast_shaman();
    check_spec( TREE_ENHANCEMENT );

    option_t options[] =
    {
      { "wf_cd_only", OPT_INT, &wf_cd_only  },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    // XX: Could use SPELL_ATTR_REQ_OFFHAND_WEAPON ?
    weapon             = &( player -> off_hand_weapon );
    if ( weapon -> type == WEAPON_NONE ) 
      background       = true; // Do not allow execution.
    
    if ( p -> set_bonus.tier8_2pc_melee() ) 
      base_multiplier *= 1.0 + 0.20;
      
    // Lava lash's flametongue bonus is in the weird dummy script effect
    flametongue_bonus  = p -> player_data.effect_base_value( id, E_DUMMY ) / 100.0;

    // Glyph of Lava Lash
    glyph_bonus        = p -> player_data.effect_base_value( 55444, E_APPLY_AURA, A_ADD_PCT_MODIFIER );
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

    player_multiplier *= 1.0 + 
      ( ( weapon -> buff_type == FLAMETONGUE_IMBUE ) * flametongue_bonus +
          p -> talent_improved_lava_lash -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER ) +
          p -> glyphs.lava_lash * glyph_bonus + 
        ( p -> talent_improved_lava_lash -> base_value( E_APPLY_AURA, A_DUMMY ) / 100.0 ) * p -> buffs_searing_flames -> stack() );
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
    shaman_attack_t( "primal_strike", player, 73899 )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    weapon               = &( p -> main_hand_weapon );
    cooldown             = p -> cooldowns_strike;
    cooldown -> duration = p -> player_data.spell_cooldown( id );

    base_multiplier     += p -> talent_focused_strikes -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER );
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
    shaman_attack_t( "stormstrike", player, 17364 )
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
    cooldown -> duration = p -> player_data.spell_cooldown( id );

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

    trigger_static_shock( this );
    
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
    h *= 1.0 / ( 1.0 + p -> buffs_elemental_mastery -> base_value( A_MOD_CASTING_SPEED_NOT_STACK ) / 100.0 );
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
    cr += p -> buffs_shamanistic_rage ->base_value( A_ADD_PCT_MODIFIER ) / 100.0;
    
  // XXX: School mask for the spell is in effect 2, should we use it?
  if ( harmful && ! pseudo_pet && ! proc && p -> buffs_elemental_focus -> up() )
    cr += p -> buffs_elemental_focus -> base_value( A_ADD_PCT_MODIFIER ) / 100.0;

  // XXX: Does maelstrom 5stack abilities count as instants?
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
    return 1.0 + p -> buffs_natures_swiftness -> base_value() / 100.0;

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
      player_crit += p -> player_data.effect_base_value( 55451, E_APPLY_AURA, A_ADD_FLAT_MODIFIER ) / 100.0;
    }
  }
  
  // Can do this here, as elemental will not have access to 
  // stormstrike
  if ( school == SCHOOL_NATURE && p -> buffs_stormstrike -> up() )
    player_crit += p -> buffs_stormstrike -> base_value() / 100.0;
    
  if ( sim -> auras.elemental_oath -> up() && p -> buffs_elemental_focus -> up() )
  {
    // Elemental oath self buff is type 0 sub type 0
    player_multiplier *= 1.0 + p -> talent_elemental_oath -> base_value( E_NONE, A_NONE ) / 100.0;
  }
  
  if ( p -> buffs_elemental_mastery -> up() && ( school == SCHOOL_FIRE || school == SCHOOL_FROST || school == SCHOOL_NATURE ) )
    player_multiplier *= 1.0 + p -> buffs_elemental_mastery -> base_value( A_MOD_DAMAGE_PERCENT_DONE ) / 100.0;
    
  if ( school == SCHOOL_FIRE || school == SCHOOL_FROST || school == SCHOOL_NATURE )
    player_multiplier *= 1.0 + p -> talent_elemental_precision -> base_value( E_APPLY_AURA, A_MOD_DAMAGE_PERCENT_DONE );
    
  // Removed school check here, all our spells are nature/fire/frost so there's no need to do it, though
  // this now applies to magma/searing as well, possibly
  if ( harmful && p -> primary_tree() == TREE_ENHANCEMENT )
    player_multiplier *= 1.0 + p -> composite_mastery() * p -> mastery_enhanced_elements -> base_value( E_APPLY_AURA, A_DUMMY ) / 10000.0;
    
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
      shaman_spell_t( "bloodlust", player, 2825 )
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
      if ( p -> sleeping ) 
        continue;
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
  int      glyph_targets;
  
  chain_lightning_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "chain_lightning", player, 421 ),
      clearcasting( 0 ), conserve( 0 ), max_lvb_cd( 0 ), maelstrom( 0 ), elemental_overload_stats( 0 ),
      glyph_targets( 0 )
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

    // Elemental specialization passives
    if ( p -> primary_tree() == TREE_ELEMENTAL )
    {
      // Shamanism
      direct_power_mod  += p -> spec_shamanism -> effect_base_value( 1 ) / 100.0;
      base_execute_time += p -> spec_shamanism -> effect_base_value( 3 ) / 1000.0;
      
      // Elemental fury
      base_crit_bonus_multiplier += p -> spec_elemental_fury -> base_value();
    }

    base_multiplier     *= 1.0 +
      ( p -> talent_concussion -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER ) +
        p -> glyphs.chain_lightning * p -> player_data.effect_base_value( 55449, E_APPLY_AURA, A_ADD_PCT_MODIFIER ) );

    base_cost_reduction += p -> talent_convection -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER );

    elemental_overload_stats = p -> get_stats( "elemental_overload" ); // Testing needed to see if this still suffers from the jump penalty
    elemental_overload_stats -> school = SCHOOL_NATURE;    
    if ( p -> glyphs.chain_lightning )
      glyph_targets = p -> player_data.effect_base_value( 55449, E_APPLY_AURA, A_ADD_FLAT_MODIFIER );
   }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_spell_t::execute();

    p -> buffs_maelstrom_weapon        -> expire();
    p -> buffs_elemental_mastery_insta -> expire();
    if ( p -> set_bonus.tier10_2pc_caster() )
    {
      p -> cooldowns_elemental_mastery -> ready -= 2.0;
    }
    p -> cooldowns_elemental_mastery   -> ready += p -> talent_feedback -> base_value() / 1000.0;

    if ( result_is_hit() )
    {
      trigger_elemental_overload( this, elemental_overload_stats );
      trigger_rolling_thunder( this );
    }
  }

  virtual double execute_time() SC_CONST
  {
    double t    = shaman_spell_t::execute_time();
    shaman_t* p = player -> cast_shaman();
    
    if ( p -> buffs_elemental_mastery_insta -> up() )
      t        *= 1.0 + p -> buffs_elemental_mastery_insta -> base_value() / 100.0;

    // misc_value of 10 is cast time reduction for A_ADD_PCT_MODIFIER
    t          *= 1.0 + p -> buffs_maelstrom_weapon -> stack() * p -> buffs_maelstrom_weapon -> base_value( A_ADD_PCT_MODIFIER, E_APPLY_AURA, 10 ) / 100.0;
    
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
      if ( ( mana_pct - 5.0 ) < sim -> target -> health_percentage() )
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
    cr         += p -> buffs_maelstrom_weapon -> stack() * p -> buffs_maelstrom_weapon -> base_value( A_ADD_PCT_MODIFIER, E_APPLY_AURA, 14) / 100.0;

    return cr;
  }

  virtual void assess_damage( double amount,
                               int    dmg_type )
  {
    shaman_spell_t::assess_damage( amount, dmg_type );

    // XXX: Get additional targets from effect
    for ( int i=0; i < sim -> target -> adds_nearby && i < ( 2 + glyph_targets ); i ++ )
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
    shaman_spell_t( "elemental_mastery", player, 16166 )
  {
    shaman_t* p = player -> cast_shaman();
    check_talent( p -> talent_elemental_mastery -> rank() );
    
    harmful = false;
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_spell_t::execute();
    
    p -> buffs_elemental_mastery_insta -> trigger();
    p -> buffs_elemental_mastery       -> trigger();
  }
};

// Fire Nova Spell =======================================================

struct fire_nova_t : public shaman_spell_t
{
  fire_nova_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "fire_nova", player, 1535 )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    aoe                   = true;
    pseudo_pet            = true;

    base_multiplier      *= 1.0 + 
      ( p -> talent_improved_fire_nova -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER ) +
        p -> talent_call_of_flame      -> effect_base_value( 1 ) / 100.0 );
    cooldown -> duration += p -> talent_improved_fire_nova -> base_value( E_APPLY_AURA, A_ADD_FLAT_MODIFIER ) / 1000.0;

    base_crit_bonus_multiplier *= 1.0 + ( p -> primary_tree() == TREE_ELEMENTAL );
    
    // Scaling information is from another spell (8349)
    // XXX: In-game tooltip says 674 damage at 83, but scaling has a range of damage
    uint32_t eid          = p -> player_data.spell_effect_id( 8349, E_SCHOOL_DAMAGE );
    base_dd_min           = p -> player_data.effect_min( eid, p -> player_data.spell_scaling_class( 8349 ), p -> level );
    base_dd_max           = p -> player_data.effect_max( eid, p -> player_data.spell_scaling_class( 8349 ), p -> level );
    direct_power_mod      = p -> player_data.effect_coeff( eid );
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();

    if ( ! p -> active_fire_totem )
      return false;
      
    // Searing totem cannot proc fire nova
    if ( p -> active_fire_totem -> id == 3599 )
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
      shaman_spell_t( "lava_burst", player, 51505 ),
      flame_shock( 0 ), elemental_overload_stats( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "flame_shock", OPT_BOOL,    &flame_shock },
      { NULL,          OPT_UNKNOWN, NULL         }
    };
    parse_options( options, options_str );

    // Elemental specialization passives
    if ( p -> primary_tree() == TREE_ELEMENTAL )
    {
      // Shamanism
      direct_power_mod  += p -> spec_shamanism -> effect_base_value( 1 ) / 100.0;
      base_execute_time += p -> spec_shamanism -> effect_base_value( 3 ) / 1000.0;
    }
    
    base_cost_reduction += p -> talent_convection -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER );
    
    base_multiplier     *= 1.0 + 
      ( p -> talent_concussion -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER ) +
        p -> talent_call_of_flame -> effect_base_value( 2 ) / 100.0 + 
        p -> glyphs.lava_burst * p -> player_data.effect_base_value( 55454, E_APPLY_AURA, A_ADD_PCT_MODIFIER) );
      
    base_crit_bonus_multiplier *= 1.0 + 
      ( ( p -> primary_tree() == TREE_ELEMENTAL ) * p -> spec_elemental_fury -> base_value() +
          p -> talent_lava_flows -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER, 24 ) +
          p -> set_bonus.tier7_4pc_caster() * 0.10 );

    // XXX: Item effects
    if ( p -> set_bonus.tier9_4pc_caster() )
    {
      num_ticks         = 3;
      base_tick_time    = 2.0;
      tick_may_crit     = false;
      tick_power_mod    = 0.0;
      scale_with_haste  = false;
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
    p -> buffs_elemental_mastery_insta -> expire();

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
    
    if ( p -> buffs_elemental_mastery_insta -> up() )
      t *= 1.0 + p -> buffs_elemental_mastery_insta -> base_value() / 100.0;
      
    return t;
  }

  virtual void player_buff()
  {
    shaman_spell_t::player_buff();
    
    shaman_t* p = player -> cast_shaman();
    
    if ( p -> active_flame_shock ) 
      player_crit += 1.0;
      
    // Apply unleash flame here, however i'm not 100% certain it is the right place, as it's an additive bonus
    if ( p -> buffs_unleash_flame -> up() )
      player_multiplier *= 1.0 + p -> buffs_unleash_flame -> base_value( A_ADD_PCT_MODIFIER ) / 100.0;
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
  stats_t* elemental_overload_stats;

  lightning_bolt_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "lightning_bolt", player, 403 ),
    maelstrom( 0 ), elemental_overload_stats( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { "maelstrom", OPT_INT,  &maelstrom  },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    // Elemental specialization passives
    if ( p -> primary_tree() == TREE_ELEMENTAL )
    {
      // Shamanism
      direct_power_mod  += p -> spec_shamanism -> effect_base_value( 1 ) / 100.0;
      base_execute_time += p -> spec_shamanism -> effect_base_value( 3 ) / 1000.0;
      
      // Elemental fury
      base_crit_bonus_multiplier += p -> spec_elemental_fury -> base_value();
    }

    base_multiplier     *= 1.0 +
      ( p -> talent_concussion -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER ) +
        p -> glyphs.lightning_bolt * p -> player_data.effect_base_value( 55453, E_APPLY_AURA, A_ADD_PCT_MODIFIER ) +
        p -> set_bonus.tier6_4pc_caster() * 0.05 );

    base_cost_reduction += 
      p -> talent_convection -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER ) -
      p -> set_bonus.tier7_2pc_caster() * 0.05;

    elemental_overload_stats = p -> get_stats( "elemental_overload" );
    elemental_overload_stats -> school = SCHOOL_NATURE;   
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_spell_t::execute();
    
    p -> buffs_maelstrom_weapon        -> expire();
    p -> buffs_elemental_mastery_insta -> expire();
    
    if ( p -> set_bonus.tier10_2pc_caster() )
    {
      p -> cooldowns_elemental_mastery -> ready -= 2.0;
    }
    p -> cooldowns_elemental_mastery   -> ready += p -> talent_feedback -> base_value() / 1000.0;
    
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
    if ( p -> buffs_elemental_mastery_insta -> up() )
    {
      t *= 1.0 + p -> buffs_elemental_mastery_insta -> base_value() / 100.0;
    }

    t *= 1.0 + p -> buffs_maelstrom_weapon -> stack() * p -> buffs_maelstrom_weapon -> base_value( A_ADD_PCT_MODIFIER, E_APPLY_AURA, 10 ) / 100.0;
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
    
    cr += p -> buffs_maelstrom_weapon -> stack() * p -> buffs_maelstrom_weapon -> base_value( A_ADD_PCT_MODIFIER, E_APPLY_AURA, 10 ) / 100.0;

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
    shaman_spell_t( "natures_swiftness", player, 16188 ),
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
    shaman_spell_t( "shamanistic_rage", player, 30823 ),
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
    shaman_spell_t( "spirit_wolf", player, 51533 )
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
    
    player -> summon_pet( "spirit_wolf", 30.0 );
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
  double bonus;
  
  thunderstorm_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "thunderstorm", player, 51490 ), bonus( 0 )
  {
    shaman_t* p = player -> cast_shaman();
    check_spec(  TREE_ELEMENTAL );

    cooldown -> duration += p -> glyphs.thunder * p -> player_data.effect_base_value( 63270, E_APPLY_AURA, A_ADD_FLAT_MODIFIER ) / 1000.0;
    bonus                 = p -> player_data.effect_base_value( id, E_ENERGIZE_PCT );
    if ( p -> glyphs.thunderstorm )
      bonus              += p -> player_data.effect_base_value( 62132, E_APPLY_AURA, A_ADD_FLAT_MODIFIER ) / 100.0;
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
    shaman_spell_t( "unleash_elements", player, 73680 )
  {
    may_crit = false;
    
    wind = new unleash_wind_t( player );
    flame = new unleash_flame_t( player );
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
  earth_shock_t( player_t* player, const std::string& options_str ) :
      shaman_spell_t( "earth_shock", player, 8042 )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    
    // XX: Misc value 0 for direct damage
    base_dd_multiplier   *= 1.0 + 
      ( p -> talent_concussion -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER ) / 100.0 +
        p -> set_bonus.tier9_4pc_melee() * 0.25 );
                                  
    base_cost_reduction  += p -> talent_convection -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER, 14 ) / 100.0;
    
    if ( p -> primary_tree() == TREE_ELEMENTAL )
      base_crit_bonus_multiplier += p -> spec_elemental_fury -> base_value();

    cooldown = p -> cooldowns_shock;
    cooldown -> duration  = 6.0 + p -> talent_reverberation -> base_value() / 1000.0;

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

    if ( p -> talent_fulmination -> rank() == 0 ) 
      return;

    if ( result_is_hit() )
    {
      int consuming_stacks = p -> buffs_lightning_shield -> stack() - 3;
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
  flame_shock_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "flame_shock", player, 8050 )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    tick_may_crit         = true;
    may_crit              = true;

    base_dd_multiplier   *= 1.0 + 
      ( p -> talent_concussion -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER ) / 100.0 +
        p -> set_bonus.tier9_4pc_melee() * 0.25 );
                                  
    // XX: Misc value 22 for periodic damage
    base_td_multiplier   *= 1.0 + 
      ( p -> talent_concussion -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER, 22 ) / 100.0 +
        p -> talent_lava_flows -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER, 22 ) / 100.0 +
        p -> set_bonus.tier9_4pc_melee() * 0.25 +
        p -> set_bonus.tier8_2pc_caster() * 0.2 );
    
    base_cost_reduction  += p -> talent_convection -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER, 14 ) / 100.0;
    
    if ( p -> primary_tree() == TREE_ELEMENTAL )
      base_crit_bonus_multiplier += p -> spec_elemental_fury -> base_value();

    // XX: For now, apply the tier9 2p bonus first, then do the glyph duration increase
    double n              = num_ticks;
    n                    += ( p -> set_bonus.tier9_2pc_caster() * ( 9 / base_tick_time ) );
    if ( p -> glyphs.flame_shock )
      n                  *= 1.0 + p -> player_data.effect_base_value( 55447, E_APPLY_AURA, A_ADD_PCT_MODIFIER );
    num_ticks             = (int) n;

    cooldown              = p -> cooldowns_shock;
    cooldown -> duration  = 6.0 + p -> talent_reverberation -> base_value() / 1000.0;

    if ( p -> glyphs.shocking )
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
    
    // To get the unleash flame out, recalculate player_buff()
    player_buff();
  }
  
  virtual void player_buff()
  {
    shaman_spell_t::player_buff();
    shaman_t* p = player -> cast_shaman();
    
    // Apply unleash flame here, however i'm not 100% certain it is the right place, as it's an additive bonus
    if ( p -> buffs_unleash_flame -> up() )
      player_multiplier *= 1.0 + p -> buffs_unleash_flame -> base_value( A_ADD_PCT_MODIFIER ) / 100.0;
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
    shaman_spell_t( "frost_shock", player, 8056 )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_dd_multiplier   *= 1.0 + 
      ( p -> talent_concussion -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER ) / 100.0 +
        p -> set_bonus.tier9_4pc_melee() * 0.25 );

    base_cost_reduction  += p -> talent_convection -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER, 14 ) / 100.0;

    if ( p -> primary_tree() == TREE_ELEMENTAL )
      base_crit_bonus_multiplier += p -> spec_elemental_fury -> base_value();

    cooldown              = p -> cooldowns_shock;
    cooldown -> duration  = 6.0 + p -> talent_reverberation -> base_value() / 1000.0;

    if ( p -> glyphs.shocking )
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
      shaman_spell_t( "wind_shear", player, 57994 )
  {
    shaman_t* p = player -> cast_shaman();
    
    cooldown -> duration += p -> talent_reverberation -> base_value() / 1000.0;
  }

  virtual bool ready()
  {
    if ( ! sim -> target -> debuffs.casting -> check() ) return false;
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
  
  shaman_totem_t( player_t* player, const std::string& options_str, const char * name, uint32_t totem_id ) :
    shaman_spell_t( name, player, totem_id ), totem_duration( 0 ), totem_bonus( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful              = false;
    scale_with_haste     = false;
    base_cost_reduction += p -> talent_totemic_focus -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER, 14 );
    totem_duration       = p -> player_data.spell_duration( id ) * 
      ( 1.0 + p -> talent_totemic_focus -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER, 1 ) );
  }
    
  virtual void execute()
  {
    if ( sim -> log ) 
      log_t::output( sim, "%s performs %s", player -> name(), name() );

    consume_resource();
    update_ready();

    if ( num_ticks )
      schedule_tick();
    
    if ( observer )
      *observer = this;
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
        log_t::output( sim, "%s avoids %s (%s)", sim -> target -> name(), name(), util_t::result_type_string( result ) );
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
    shaman_totem_t( player, options_str, "fire_elemental_totem", 2894 )
  {
    shaman_t* p = player -> cast_shaman();

    num_ticks            = 1; 
    number_ticks         = num_ticks;
    base_tick_time       = totem_duration;
    
    // XX: Hope someone makes a glyph_t to rip the data out of easily
    if ( p -> glyphs.fire_elemental_totem )
    {
      cooldown -> duration += p -> player_data.effect_base_value( 55455, E_APPLY_AURA, A_ADD_FLAT_MODIFIER ) / 1000.0;
    }
    
    observer             = &( p -> active_fire_totem );
  }
  
  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();

    if ( p -> active_fire_totem ) 
      p -> active_fire_totem -> cancel();

    shaman_totem_t::execute();

    if ( p -> talent_totemic_wrath -> rank() )
    {
      if ( sim -> overrides.flametongue_totem == 0 )
        sim -> auras.flametongue_totem -> duration = totem_duration;
      sim -> auras.flametongue_totem -> trigger( 1, p -> talent_totemic_wrath -> base_value() / 100.0 );
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
    shaman_totem_t( player, options_str, "flametongue_totem", 8227 )
  {
    shaman_t* p = player -> cast_shaman();

    if ( p -> talent_totemic_wrath -> rank() > 0 )
      totem_bonus        = p -> talent_totemic_wrath -> base_value() / 100.0;
    // XX: Hardcode this based on tooltip information for now, effect is spell id 52109
    else
    {
      totem_bonus        = p -> player_data.effect_base_value( 52109, E_APPLY_AREA_AURA_RAID, A_317 ) / 100.0;
    }

    observer             = &( p -> active_fire_totem );
  }

  virtual void execute()
  {
    shaman_totem_t::execute();

    if ( sim -> overrides.flametongue_totem == 0 )
      sim -> auras.flametongue_totem -> duration = totem_duration;

    sim -> auras.flametongue_totem -> trigger( 1, totem_bonus );
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
    shaman_totem_t( player, options_str, "magma_totem", 8190 )
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
    
    if ( p -> primary_tree() == TREE_ELEMENTAL )
      base_crit_bonus_multiplier += p -> spec_elemental_fury -> base_value();
    
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
        sim -> auras.flametongue_totem -> duration = totem_duration;
      sim -> auras.flametongue_totem -> trigger( 1, p -> talent_totemic_wrath -> base_value() / 100.0 );
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
    shaman_totem_t( player, options_str, "mana_spring_totem", 5675 )
  {
    shaman_t* p = player -> cast_shaman();

    // Mana spring effect is at spell id 5677. Get scaling information from there.
    totem_bonus  = p -> player_data.effect_min( 5677, p -> level, E_APPLY_AREA_AURA_RAID, A_MOD_POWER_REGEN );
  }

  virtual void execute()
  {
    shaman_totem_t::execute();
    
    if ( sim -> overrides.mana_spring_totem == 0 )
      sim -> auras.mana_spring_totem -> duration = totem_duration;
    sim -> auras.mana_spring_totem -> trigger( 1, totem_bonus );
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
    shaman_totem_t( player, options_str, "mana_tide_totem", 16190 )
  {
    shaman_t* p = player -> cast_shaman();
    check_talent( p -> talent_mana_tide_totem -> rank() );

    // Mana tide effect bonus is in a separate spell, we dont need other info
    // from there anymore, as mana tide does not pulse anymore
    totem_bonus  = p -> player_data.effect_base_value( id, E_APPLY_AREA_AURA_PARTY, A_MOD_TOTAL_STAT_PERCENTAGE );
  }

  virtual void execute()
  {
    shaman_totem_t::execute();
    
    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if ( p -> party == player -> party )
      {
        // Change buff duration based on totem duration
        p -> buffs.mana_tide -> duration = totem_duration;
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
    shaman_totem_t( player, options_str, "searing_totem", 3599 )
  {
    shaman_t* p = player -> cast_shaman();

    harmful              = true;
    pseudo_pet           = true;
    may_crit             = true;
    
    // Base multiplier has to be applied like this, because the talent has two identical effects
    base_multiplier     *= 1.0 + p -> talent_call_of_flame -> effect_base_value( 2 ) / 100.0;
    
    if ( p -> primary_tree() == TREE_ELEMENTAL )
      base_crit_bonus_multiplier += p -> spec_elemental_fury -> base_value();
    
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
    
    log_debug();
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
        sim -> auras.flametongue_totem -> duration = totem_duration;
      sim -> auras.flametongue_totem -> trigger( 1, p -> talent_totemic_wrath -> base_value() / 100.0 );
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
      if ( ! p -> active_searing_flames_dot ) 
        p -> active_searing_flames_dot = new searing_flames_t ( p );
        
      double new_base_td = tick_dmg;
      // Searing flame dot treats all hits as.. HITS
      if ( result == RESULT_CRIT )
      {
        new_base_td /= 1.0 + total_crit_bonus();
      }

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
    shaman_totem_t( player, options_str, "strength_of_earth_totem", 8075 )
  {
    shaman_t* p = player -> cast_shaman();

    // We can use either A_MOD_STAT effect, as they both apply the same amount of stat
    totem_bonus  = p -> player_data.effect_min( 8076, p -> level, E_APPLY_AREA_AURA_RAID, A_MOD_STAT );
  }

  virtual void execute()
  {
    shaman_totem_t::execute();

    if ( sim -> overrides.strength_of_earth == 0 )
      sim -> auras.strength_of_earth -> duration = totem_duration;
    sim -> auras.strength_of_earth -> trigger( 1, totem_bonus );
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
    shaman_totem_t( player, options_str, "windfury_totem", 8512 )
  {
    shaman_t* p = player -> cast_shaman();
    
    totem_bonus  = p -> player_data.effect_base_value( 8515, E_APPLY_AREA_AURA_RAID, A_319 );
  }

  virtual void execute()
  {
    shaman_totem_t::execute();

    if ( sim -> overrides.windfury_totem == 0 )
      sim -> auras.windfury_totem -> duration = totem_duration;
    sim -> auras.windfury_totem -> trigger( 1, totem_bonus );
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
    shaman_totem_t( player, options_str, "wrath_of_air_totem", 3738 )
  {
    shaman_t* p = player -> cast_shaman();

    totem_bonus  = p -> player_data.effect_base_value( 2895, E_APPLY_AREA_AURA_RAID, A_MOD_CASTING_SPEED_NOT_STACK );
  }

  virtual void execute()
  {
    shaman_totem_t::execute();

    if ( sim -> overrides.wrath_of_air == 0 )
      sim -> auras.wrath_of_air -> duration = totem_duration;
    sim -> auras.wrath_of_air -> trigger( 1, totem_bonus );
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
    shaman_spell_t( "flametongue_weapon", player, 8024 ), 
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
};

// Windfury Weapon Spell ====================================================

struct windfury_weapon_t : public shaman_spell_t
{
  double bonus_power;
  int    main, off;

  windfury_weapon_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "windfury_weapon", player, 8232 ), 
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

    bonus_power  = p -> player_data.effect_min( id, p -> level, E_DUMMY );
    harmful      = false;
    
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
};

// =========================================================================
// Shaman Shields
// =========================================================================

// Lightning Shield Spell =====================================================

struct lightning_shield_t : public shaman_spell_t
{
  lightning_shield_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "lightning_shield", player, 324 )
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
};

// Water Shield Spell =========================================================

struct water_shield_t : public shaman_spell_t
{
  double bonus;
  
  water_shield_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "water_shield", player, 52127 ), bonus( 0.0 )
  {
    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    
    shaman_t* p  = player -> cast_shaman();
    harmful      = false;
    bonus        = p -> player_data.effect_min( id, p -> level, E_APPLY_AURA, A_MOD_POWER_REGEN );
    bonus       *= 1.0 +
      ( p -> talent_improved_shields -> base_value( E_APPLY_AURA, A_ADD_PCT_MODIFIER ) +
        p -> glyphs.water_shield * p -> player_data.effect_base_value( 55436, E_APPLY_AURA, A_ADD_PCT_MODIFIER ) );
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
};

// ==========================================================================
// Shaman Passive Buffs
// ==========================================================================

struct maelstrom_weapon_t : public new_buff_t
{
  maelstrom_weapon_t( player_t *         p,
                      const std::string& n,
                      uint32_t           id ) :
    new_buff_t( p, n, id ) { }

  virtual bool trigger( int, double, double chance )
  {
    bool      can_increase = current_stack <  max_stack;
    bool            result = false;
    shaman_t*            p = player -> cast_shaman();

    if ( p -> set_bonus.tier8_4pc_melee() ) chance *= 1.20;

    result = new_buff_t::trigger( default_stack_charge, -1, chance );

    if ( result && p -> set_bonus.tier10_4pc_melee() && ( can_increase && current_stack == max_stack ) )
      p -> buffs_tier10_4pc_melee -> trigger();
    
    return result;
  }
};

struct elemental_devastation_t : public new_buff_t
{
  elemental_devastation_t( player_t *         p,
                           const std::string& n,
                           uint32_t           id ) :
    new_buff_t( p, n, id ) 
  { 
    if ( ! p -> player_data.spell_exists( id ) )
      return;

    // Duration has to be parsed out from the triggered spell
    uint32_t trigger = p -> player_data.effect_trigger_spell_id( id, E_APPLY_AURA, A_PROC_TRIGGER_SPELL_WITH_VALUE );
    duration = p -> player_data.spell_duration( trigger );
    
    // And fix atomic, as it's a triggered spell, but not really .. sigh
    single = e_data[ 0 ];
  }
};

struct lightning_shield_buff_t : public new_buff_t
{
  lightning_shield_buff_t( player_t *         p,
                           const std::string& n,
                           uint32_t           id ) :
    new_buff_t( p, n, id ) 
  { 
    shaman_t* s = player -> cast_shaman();

    if ( ! p -> player_data.spell_exists( id ) )
      return;
      
    // This requires rolling thunder checking for max stack
    if ( s -> talent_rolling_thunder -> rank() > 0 )
      max_stack = s -> talent_rolling_thunder -> base_value( E_APPLY_AURA, A_PROC_TRIGGER_SPELL );
      
    init();
  }
};

struct searing_flames_buff_t : public new_buff_t
{
  searing_flames_buff_t( player_t *         p,
                         const std::string& n,
                         uint32_t           id ) :
    new_buff_t( p, n, id, 1.0, true ) // Quiet buff, dont show in report
  {
    if ( ! p -> player_data.spell_exists( id ) )
      return;

    // The default chance is in the script dummy effect base value
    default_chance     = p -> player_data.effect_base_value( id, E_APPLY_AURA ) / 100.0;

    // Various other things are specified in the actual debuff placed on the target
    duration           = p -> player_data.spell_duration( 77661 );
    max_stack          = p -> player_data.spell_max_stacks( 77661 );

    init();
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
    else if ( n == "lava_burst"           ) glyphs.lava_burst = 1;
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

  attribute_multiplier_initial[ ATTR_STAMINA   ] *= 1.0 + 
    talent_toughness -> base_value( E_APPLY_AURA, A_MOD_TOTAL_STAT_PERCENTAGE );
  base_attack_expertise = talent_unleashed_rage -> base_value( E_APPLY_AURA, A_MOD_EXPERTISE );

  // XXX: Has base attaack power changed?
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
}

// shaman_t::init_scaling ====================================================

void shaman_t::init_scaling()
{
  player_t::init_scaling();

  if ( primary_tree() == TREE_ENHANCEMENT )
  {
    scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = 1;
    scales_with[ STAT_WEAPON_OFFHAND_SPEED  ] = 1;
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
  buffs_unleash_wind            = new new_buff_t             ( this, "unleash_wind",           73681 );
  buffs_unleash_flame           = new new_buff_t             ( this, "unleash_flame",          73683 );

  // For now, elemental mastery will need 2 buffs, 1 to trigger the insta cast, and a second for the haste/damage buff
  buffs_elemental_mastery_insta = new new_buff_t             ( this, "elemental_mastery",      talent_elemental_mastery    -> spell_id() );
  // Note the chance override, as the spell itself does not have a proc chance
  buffs_elemental_mastery       = new new_buff_t             ( this, "elemental_mastery_buff", talent_elemental_mastery    -> effect_trigger_spell( 2 ), 1.0 );
  
  buffs_elemental_devastation   = new elemental_devastation_t( this, "elemental_devastation",  talent_elemental_devastation -> spell_id() );
  buffs_maelstrom_weapon        = new maelstrom_weapon_t     ( this, "maelstrom_weapon",       talent_maelstrom_weapon      -> effect_trigger_spell( 1 ) );  
  buffs_lightning_shield        = new lightning_shield_buff_t( this, "lightning_shield",       player_data.find_class_spell( type, "Lightning Shield" ) );
  
  // Searing flames needs heavy trickery to get things correct
  buffs_searing_flames          = new searing_flames_buff_t  ( this, "searing_flames",         talent_searing_flames        -> spell_id() );
  
  buffs_tier10_2pc_melee      = new buff_t( this, "tier10_2pc_melee",      1,  15.0, 0.0, set_bonus.tier10_2pc_melee()  );
  buffs_tier10_4pc_melee      = new buff_t( this, "tier10_4pc_melee",      1,  10.0, 0.0, 0.15                          ); //FIX ME - assuming no icd on this
  // buffs_totem_of_wrath_glyph  = new buff_t( this, "totem_of_wrath_glyph",  1, 300.0, 0.0, glyphs.totem_of_wrath );
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
      action_list_str  = "flask,type=frost_wyrm/food,type=fish_feast/flametongue_weapon,weapon=main/lightning_shield";
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
      if ( talent_fulmination -> rank() )
        action_list_str += "/earth_shock,if=buff.lightning_shield.stack=9";
      action_list_str += "/flame_shock,if=!ticking";
      if ( level >= 81 )
        action_list_str += "/unleash_elements";
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

// shaman_t::composite_attribute_multiplier ===============================

double shaman_t::composite_attribute_multiplier( int attr ) SC_CONST
{
  double m = player_t::composite_attribute_multiplier( attr );

  if ( ( attr == STAT_INTELLECT ) && mail_specialization && primary_tree() == TREE_ELEMENTAL )
    m *= 1.05;
  else if ( ( attr == STAT_AGILITY ) && mail_specialization && primary_tree() == TREE_ENHANCEMENT )
    m *= 1.05;

  return m;
}

// shaman_t::composite_spell_hit ==========================================

double shaman_t::composite_spell_hit() SC_CONST
{
  double hit = player_t::composite_spell_hit();
  double s_spirit = 0.0;
  
  s_spirit += gear.attribute[ ATTR_SPIRIT ] * composite_attribute_multiplier( ATTR_SPIRIT );

  hit += ( talent_elemental_precision -> base_value( E_APPLY_AURA, A_MOD_RATING_FROM_STAT ) * s_spirit ) / rating.spell_hit;

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

double shaman_t::composite_spell_power( const school_type school ) SC_CONST
{
  double sp = player_t::composite_spell_power( school );

  if ( primary_tree() == TREE_ENHANCEMENT )
  {
    double ap = composite_attack_power_multiplier() * composite_attack_power();
    sp += ap * spec_mental_quickness -> base_value( E_APPLY_AURA, A_MOD_SPELL_DAMAGE_OF_ATTACK_POWER );
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
    double water_shield_regen = periodicity * buffs_water_shield -> base_value() / 5.0;

    resource_gain( RESOURCE_MANA, water_shield_regen, gains_water_shield );
  }
}

// shaman_t::combat_begin =================================================

void shaman_t::combat_begin()
{
  player_t::combat_begin();

  if ( talent_elemental_oath -> rank() ) sim -> auras.elemental_oath -> trigger();

  int ur = util_t::talent_rank( talent_unleashed_rage -> rank(), 2, 5, 10 );

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
      { "mail_specialization", OPT_BOOL, &mail_specialization },
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
    p -> buffs.bloodlust = new buff_t( p, "bloodlust", 1, 40.0, 600.0 );
    p -> buffs.mana_tide = new new_buff_t( p, "mana_tide", 16190 );
  }
}

// player_t::shaman_combat_begin ============================================

void player_t::shaman_combat_begin( sim_t* sim )
{
  if ( sim -> overrides.elemental_oath    ) sim -> auras.elemental_oath    -> override();
  if ( sim -> overrides.flametongue_totem ) sim -> auras.flametongue_totem -> override( 1, 0.10 );
  if ( sim -> overrides.mana_spring_totem ) sim -> auras.mana_spring_totem -> override( 1, 326.0 );
  if ( sim -> overrides.strength_of_earth ) sim -> auras.strength_of_earth -> override( 1, 549.0 );
  if ( sim -> overrides.unleashed_rage    ) sim -> auras.unleashed_rage    -> override( 1, 10.0 );
  if ( sim -> overrides.windfury_totem    ) sim -> auras.windfury_totem    -> override( 1, 0.10 );
  if ( sim -> overrides.wrath_of_air      ) sim -> auras.wrath_of_air      -> override( 1, 0.20 );
}
