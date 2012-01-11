// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

// ==========================================================================
// TODO
// ==========================================================================
// Does flametongue's AP coefficient scale with level? Our values are at 85.
// Fire Nova for 4.1.0 works wrong, for now; flares only on single target,
// fix this later.
// ==========================================================================
// BUGS
// ==========================================================================
// Cataclysm:
//   Searing Totem is double dipping to Totem of Wrath
//   Searing Totem's base damage range is wrong in spell data
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Shaman
// ==========================================================================

enum totem_type { TOTEM_NONE=0, TOTEM_AIR, TOTEM_EARTH, TOTEM_FIRE, TOTEM_WATER, TOTEM_MAX };

enum imbue_type_t { IMBUE_NONE=0, FLAMETONGUE_IMBUE, WINDFURY_IMBUE };

struct shaman_targetdata_t : public targetdata_t
{
  dot_t* dots_flame_shock;
  dot_t* dots_searing_flames;

  buff_t* debuffs_searing_flames;

  shaman_targetdata_t( player_t* source, player_t* target );
};

void register_shaman_targetdata( sim_t* sim )
{
  player_type t = SHAMAN;
  typedef shaman_targetdata_t type;

  REGISTER_DOT( flame_shock );
  REGISTER_DOT( searing_flames );

  REGISTER_DEBUFF( searing_flames );
}

struct shaman_t : public player_t
{
  // Options
  timespan_t wf_delay;
  timespan_t wf_delay_stddev;
  timespan_t uf_expiration_delay;
  timespan_t uf_expiration_delay_stddev;

  // Active
  action_t* active_lightning_charge;
  action_t* active_searing_flames_dot;
  
  // Cached actions
  action_t* action_flame_shock;
  action_t* action_improved_lava_lash;

  // Pets
  pet_t* pet_spirit_wolf;
  pet_t* pet_fire_elemental;
  pet_t* pet_earth_elemental;

  // Totems
  action_t* totems[ TOTEM_MAX ];

  // Buffs
  buff_t* buffs_earth_elemental;
  buff_t* buffs_elemental_devastation;
  buff_t* buffs_elemental_focus;
  buff_t* buffs_elemental_mastery;
  buff_t* buffs_elemental_mastery_insta;
  buff_t* buffs_fire_elemental;
  buff_t* buffs_flurry;
  buff_t* buffs_lava_surge;
  buff_t* buffs_lightning_shield;
  buff_t* buffs_maelstrom_weapon;
  buff_t* buffs_natures_swiftness;
  buff_t* buffs_shamanistic_rage;
  buff_t* buffs_spiritwalkers_grace;
  buff_t* buffs_stormfire;
  buff_t* buffs_stormstrike;
  buff_t* buffs_tier13_2pc_caster;
  buff_t* buffs_tier13_4pc_caster;
  buff_t* buffs_tier13_4pc_healer;
  buff_t* buffs_unleash_flame;
  buff_t* buffs_unleash_wind;
  buff_t* buffs_water_shield;

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
  cooldown_t* cooldowns_fire_elemental_totem;
  cooldown_t* cooldowns_lava_burst;
  cooldown_t* cooldowns_shock;
  cooldown_t* cooldowns_strike;
  cooldown_t* cooldowns_t12_2pc_caster;
  cooldown_t* cooldowns_windfury_weapon;

  // Gains
  gain_t* gains_primal_wisdom;
  gain_t* gains_rolling_thunder;
  gain_t* gains_telluric_currents;
  gain_t* gains_thunderstorm;
  gain_t* gains_water_shield;

  // Procs
  proc_t* procs_elemental_overload;
  proc_t* procs_ft_icd;
  proc_t* procs_lava_surge;
  proc_t* procs_maelstrom_weapon;
  proc_t* procs_rolling_thunder;
  proc_t* procs_static_shock;
  proc_t* procs_swings_clipped_mh;
  proc_t* procs_swings_clipped_oh;
  proc_t* procs_wasted_ls;
  proc_t* procs_wasted_mw;
  proc_t* procs_windfury;

  proc_t* procs_fulmination[7];
  proc_t* procs_maelstrom_weapon_used[6];

  // Random Number Generators
  rng_t* rng_elemental_overload;
  rng_t* rng_lava_surge;
  rng_t* rng_primal_wisdom;
  rng_t* rng_rolling_thunder;
  rng_t* rng_searing_flames;
  rng_t* rng_static_shock;
  rng_t* rng_t12_2pc_caster;
  rng_t* rng_windfury_weapon;
  rng_t* rng_windfury_delay;

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
  attack_t* windfury_mh;
  attack_t* windfury_oh;
  spell_t*  flametongue_mh;
  spell_t*  flametongue_oh;

  // Glyphs
  glyph_t* glyph_feral_spirit;
  glyph_t* glyph_fire_elemental_totem;
  glyph_t* glyph_flame_shock;
  glyph_t* glyph_flametongue_weapon;
  glyph_t* glyph_lava_burst;
  glyph_t* glyph_lava_lash;
  glyph_t* glyph_lightning_bolt;
  glyph_t* glyph_stormstrike;
  glyph_t* glyph_unleashed_lightning;
  glyph_t* glyph_water_shield;
  glyph_t* glyph_windfury_weapon;

  glyph_t* glyph_chain_lightning;
  glyph_t* glyph_shocking;
  glyph_t* glyph_thunder;

  glyph_t* glyph_thunderstorm;

  shaman_t( sim_t* sim, const std::string& name, race_type r = RACE_NONE ) : player_t( sim, SHAMAN, name, r ),
    wf_delay( timespan_t::from_seconds(0.95) ), wf_delay_stddev( timespan_t::from_seconds(0.25) ),
    uf_expiration_delay( timespan_t::from_seconds(0.3) ), uf_expiration_delay_stddev( timespan_t::from_seconds(0.05) )
  {
    if ( race == RACE_NONE ) race = RACE_TAUREN;

    tree_type[ SHAMAN_ELEMENTAL   ] = TREE_ELEMENTAL;
    tree_type[ SHAMAN_ENHANCEMENT ] = TREE_ENHANCEMENT;
    tree_type[ SHAMAN_RESTORATION ] = TREE_RESTORATION;

    // Active
    active_lightning_charge   = 0;
    active_searing_flames_dot = 0;
    
    action_flame_shock = 0;
    action_improved_lava_lash = 0;

    // Pets
    pet_spirit_wolf     = 0;
    pet_fire_elemental  = 0;
    pet_earth_elemental = 0;

    // Totem tracking
    for ( int i = 0; i < TOTEM_MAX; i++ ) totems[ i ] = 0;

    // Cooldowns
    cooldowns_elemental_mastery    = get_cooldown( "elemental_mastery"    );
    cooldowns_fire_elemental_totem = get_cooldown( "fire_elemental_totem" );
    cooldowns_lava_burst           = get_cooldown( "lava_burst"           );
    cooldowns_shock                = get_cooldown( "shock"                );
    cooldowns_strike               = get_cooldown( "strike"               );
    cooldowns_t12_2pc_caster       = get_cooldown( "t12_2pc_caster"       );
    cooldowns_windfury_weapon      = get_cooldown( "windfury_weapon"      );

    // Weapon Enchants
    windfury_mh    = 0;
    windfury_oh    = 0;
    flametongue_mh = 0;
    flametongue_oh = 0;

    create_talents();
    create_glyphs();
    create_options();
  }

  // Character Definition
  virtual targetdata_t* new_targetdata( player_t* source, player_t* target ) {return new shaman_targetdata_t( source, target );}
  virtual void      init_talents();
  virtual void      init_spells();
  virtual void      init_base();
  virtual void      init_scaling();
  virtual void      init_buffs();
  virtual void      init_values();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      moving();
  virtual double    composite_attack_crit() const;
  virtual double    composite_attack_hit() const;
  virtual double    composite_spell_hit() const;
  virtual double    composite_spell_crit() const;
  virtual double    composite_spell_power( const school_type school ) const;
  virtual double    composite_spell_power_multiplier() const;
  virtual double    composite_player_multiplier( const school_type school, action_t* a = NULL ) const;
  virtual double    matching_gear_multiplier( const attribute_type attr ) const;
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() const { return RESOURCE_MANA; }
  virtual int       primary_role() const;
  virtual void      combat_begin();

  // Event Tracking
  virtual void regen( timespan_t periodicity );
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// ==========================================================================
// Shaman Attack
// ==========================================================================

struct shaman_attack_t : public attack_t
{
  bool windfury;
  bool flametongue;

  /* Old style construction, spell data will not be accessed */
  shaman_attack_t( const char* n, player_t* player, school_type s, int t, bool special ) :
    attack_t( n, player, RESOURCE_MANA, s, t, special ), windfury( true ), flametongue( true ) { }

  /* Class spell data based construction, spell name in s_name */
  shaman_attack_t( const char* n, const char* s_name, player_t* player ) :
    attack_t( n, s_name, player, TREE_NONE, true ), windfury( true ), flametongue( true )
  {
    may_crit    = true;
  }

  /* Spell data based construction, spell id in spell_id */
  shaman_attack_t( const char* n, uint32_t spell_id, player_t* player ) :
    attack_t( n, spell_id, player, TREE_NONE, true ), windfury( true ), flametongue( true )
  {
    may_crit    = true;
  }

  virtual void execute();
  virtual double cost() const;
  virtual void player_buff();
  virtual double cost_reduction() const;
  virtual void consume_resource();
};

// ==========================================================================
// Shaman Spell
// ==========================================================================

struct shaman_spell_t : public spell_t
{
  double   base_cost_reduction;
  bool     maelstrom;
  bool     overload;
  bool     is_totem;

  /* Old style construction, spell data will not be accessed */
  shaman_spell_t( const char* n, player_t* p, const std::string& options_str, const school_type s, int t ) :
    spell_t( n, p, RESOURCE_MANA, s, t ),
    base_cost_reduction( 0 ), maelstrom( false ), overload( false ), is_totem( false )
  {
    parse_options( NULL, options_str );
  }

  /* Class spell data based construction, spell name in s_name */
  shaman_spell_t( const char* n, const char* s_name, player_t* p, const std::string& options_str = std::string() ) :
    spell_t( n, s_name, p ), base_cost_reduction( 0.0 ), maelstrom( false ), overload( false ), is_totem( false )
  {
    parse_options( NULL, options_str );

    may_crit = true;
  }

  /* Spell data based construction, spell id in spell_id */
  shaman_spell_t( const char* n, uint32_t spell_id, player_t* p, const std::string& options_str = std::string() ) :
    spell_t( n, spell_id, p ), base_cost_reduction( 0.0 ), maelstrom( false ), overload( false ), is_totem( false )
  {
    parse_options( NULL, options_str );

    may_crit = true;
  }

  virtual bool   is_direct_damage() const { return base_dd_min > 0 && base_dd_max > 0; }
  virtual bool   is_periodic_damage() const { return base_td > 0; };
  virtual double cost() const;
  virtual double cost_reduction() const;
  virtual void   consume_resource();
  virtual timespan_t execute_time() const;
  virtual void   execute();
  virtual void   player_buff();
  virtual double haste() const;
  virtual void   schedule_execute();
  virtual bool   usable_moving()
  {
    shaman_t* p = player -> cast_shaman();

    if ( p -> buffs_spiritwalkers_grace -> check() || execute_time() == timespan_t::zero )
      return true;

    return spell_t::usable_moving();
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

      // Wolves have a base multiplier of 1.49835 approximately, and there are
      // two wolves. Verified using paper doll damage range values on a
      // level 85 enhancement shaman, with and without Glyph
      base_multiplier *= 1.49835 * 2.0;
    }

    virtual void execute()
    {
      shaman_t* o = player -> cast_pet() -> owner -> cast_shaman();

      attack_t::execute();

      // Two independent chances to proc it since we model 2 wolf pets as 1 ..
      if ( result_is_hit() )
      {
        if ( sim -> roll( o -> sets -> set( SET_T13_4PC_MELEE ) -> effect1().percent() ) )
        {
          int   mwstack = o -> buffs_maelstrom_weapon -> check();
          if ( o -> buffs_maelstrom_weapon -> trigger( 1, -1, 1.0 ) )
          {
            if ( mwstack == o -> buffs_maelstrom_weapon -> max_stack )
              o -> procs_wasted_mw -> occur();

            o -> procs_maelstrom_weapon -> occur();
          }
        }

        if ( sim -> roll( o -> sets -> set( SET_T13_4PC_MELEE ) -> effect1().percent() ) )
        {
          int   mwstack = o -> buffs_maelstrom_weapon -> check();
          if ( o -> buffs_maelstrom_weapon -> trigger( 1, -1, 1.0 ) )
          {
            if ( mwstack == o -> buffs_maelstrom_weapon -> max_stack )
              o -> procs_wasted_mw -> occur();

            o -> procs_maelstrom_weapon -> occur();
          }
        }
      }
    }
  };

  melee_t* melee;

  spirit_wolf_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "spirit_wolf" ), melee( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 556; // Level 85 Values, approximated
    main_hand_weapon.max_dmg    = 835;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds(1.5);
  }

  virtual void init_base()
  {
    pet_t::init_base();

    // New approximated pet values at 85, roughly the same ratio of str/agi as per the old ones
    // At 85, the wolf has a base attack power of 932
    attribute_base[ ATTR_STRENGTH  ] = 407;
    attribute_base[ ATTR_AGILITY   ] = 138;
    attribute_base[ ATTR_STAMINA   ] = 361;
    attribute_base[ ATTR_INTELLECT ] = 90; // Pet has 90 spell damage :)
    attribute_base[ ATTR_SPIRIT    ] = 109;

    base_attack_power = -20;
    initial_attack_power_per_strength = 2.0;

    melee = new melee_t( this );
  }

  virtual double composite_attack_power() const
  {
    shaman_t* o         = owner -> cast_shaman();
    double ap           = pet_t::composite_attack_power();
    // In game testing has ap_per_owner without glyph as ~33%, and with glyph, ~65%
    double ap_per_owner = 0.329 + o -> glyph_feral_spirit -> ok() * 0.320;

    return ap + ap_per_owner * o -> composite_attack_power_multiplier() * o -> composite_attack_power();
  }

  virtual void summon( timespan_t duration=timespan_t::zero )
  {
    pet_t::summon( duration );
    melee -> execute(); // Kick-off repeating attack
  }

  virtual double composite_attribute_multiplier( int attr ) const
  {
    return attribute_multiplier[ attr ];
  }

  virtual double strength() const
  {
    return attribute[ ATTR_STRENGTH ];
  }

  virtual double agility() const
  {
    return attribute[ ATTR_AGILITY ];
  }

  virtual double composite_attack_power_multiplier() const
  {
    return attack_power_multiplier;
  }

  virtual double composite_player_multiplier( const school_type, action_t* ) const
  {
    return 1.0;
  }
};

struct earth_elemental_pet_t : public pet_t
{
  double owner_sp;

  struct travel_t : public action_t
  {
    travel_t( player_t* player ) : action_t( ACTION_OTHER, "travel", player ) {}
    virtual void execute() { player -> distance = 1; }
    virtual timespan_t execute_time() const { return timespan_t::from_seconds( player -> distance / 10.0 ); }
    virtual bool ready() { return ( player -> distance > 1 ); }
    virtual bool usable_moving() { return true; }
  };

  struct auto_attack_t : public attack_t
  {
    auto_attack_t( player_t* player ) :
      attack_t( "auto_attack", player, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_NONE, false )
    {
      pet_t* p = player -> cast_pet();

      assert( p -> main_hand_weapon.type != WEAPON_NONE );
      p -> main_hand_attack = new melee_t( player );

      trigger_gcd = timespan_t::zero;
    }

    virtual void execute()
    {
      pet_t* p = player -> cast_pet();
      shaman_t* o = ( player -> cast_pet() ) -> owner -> cast_shaman();

      p -> main_hand_attack -> schedule_execute();

      o -> buffs_earth_elemental -> up();
    }

    virtual bool ready()
    {
      pet_t* p = player -> cast_pet();
      if ( p -> is_moving() ) return false;
      return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
    }
  };

  struct melee_t : public attack_t
  {
    melee_t( player_t* player ) :
      attack_t( "earth_melee", player, RESOURCE_NONE, SCHOOL_PHYSICAL, TREE_NONE, false )
    {
      may_crit          = true;
      background        = true;
      repeating         = true;
      weapon            = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      weapon_power_mod  = 0.098475 / base_execute_time.total_seconds();

      base_attack_power_multiplier = 0;
    }

    virtual double swing_haste() const
    {
      return 1.0;
    }

    virtual double    available() const { return sim -> max_time.total_seconds(); }

    // Earth elemental scales purely with spell power
    virtual double total_attack_power() const
    {
      return player -> composite_spell_power( SCHOOL_MAX );
    }

    // Melee swings have a ~3% crit rate on boss level mobs
    virtual double crit_chance( int /* delta_level */ ) const
    {
      return 0.03;
    }
  };

  earth_elemental_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "earth_elemental", true /*GUARDIAN*/ ), owner_sp( 0.0 )
  {
    stamina_per_owner   = 1.0;

    // Approximated from lvl 85 earth elemental
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 370; // Level 85 Values, approximated
    main_hand_weapon.max_dmg    = 409;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds(2.0);
  }

  virtual void init_base()
  {
    pet_t::init_base();

    resource_base[ RESOURCE_HEALTH ] = 8000; // Approximated from lvl85 earth elemental in game
    resource_base[ RESOURCE_MANA   ] = 0; //

    health_per_stamina = 13.75; // See above
    mana_per_intellect = 0;

    // Simple as it gets, travel to target, kick off melee
    action_list_str = "travel/auto_attack,moving=0";
  }

  virtual int primary_resource() const { return RESOURCE_MANA; }

  virtual void regen( timespan_t /* periodicity */ ) { }

  virtual void summon( timespan_t /* duration */ )
  {
    shaman_t* o = owner -> cast_shaman();

    pet_t::summon();
    owner_sp = owner -> composite_spell_power( SCHOOL_MAX ) * owner -> composite_spell_power_multiplier();

    o -> buffs_earth_elemental -> trigger();
  }

  virtual void demise()
  {
    shaman_t* o = owner -> cast_shaman();

    pet_t::demise();

    o -> buffs_earth_elemental -> expire();
  }


  virtual double composite_spell_power( const school_type ) const
  {
    return owner_sp;
  }

  virtual double composite_attack_power() const
  {
    return 0.0;
  }

  virtual double composite_attack_hit() const
  {
    return owner -> composite_spell_hit();
  }

  virtual double composite_attack_expertise() const
  {
    return owner -> composite_spell_hit() * 26.0 / 17.0;
  }


  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "travel"      ) return new travel_t( this );
    if ( name == "auto_attack" ) return new auto_attack_t ( this );

    return pet_t::create_action( name, options_str );
  }

  virtual double composite_player_multiplier( const school_type /* school */, action_t* /* a */ ) const
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
//   multiplier for Intellect and pure Spell Power. Current numbers are from
//   Cataclysm beta, level 85 shaman.
// - Fire Elemental base damage values are from Cataclysm, level 85
// - Fire Elemental now has a proper action list, using Fire Blast and Fire Nova
//   with random cooldowns to do damage to the target. In addition, both
//   Fire Nova and Fire Blast cause the Fire Elemental to reset the swing timer,
//   and an on-going Fire Nova cast will also clip a swing.
// TODO:
// - Analysis of in-game Fire Elemental Fire Nova and Fire Blast ability cooldown
//   distributions would help us make a more realistic model. The current values
//   of 5-20 seconds are not very well researched.
// ==========================================================================

struct fire_elemental_pet_t : public pet_t
{
  struct travel_t : public action_t
  {
    travel_t( player_t* player ) : action_t( ACTION_OTHER, "travel", player ) {}
    virtual void execute() { player -> distance = 1; }
    virtual timespan_t execute_time() const { return timespan_t::from_seconds( player -> distance / 10.0 ); }
    virtual bool ready() { return ( player -> distance > 1 ); }
    virtual timespan_t gcd() const { return timespan_t::zero; }
    virtual bool usable_moving() { return true; }
  };

  struct fire_elemental_spell_t : public spell_t
  {
    double int_multiplier;
    double sp_multiplier;

    fire_elemental_spell_t( player_t* player, const char* n ) :
      spell_t( n, player, RESOURCE_MANA, SCHOOL_FIRE ),
      int_multiplier( 0.85 ), sp_multiplier ( 0.53419 )
    {
      // Apparently, fire elemental spell crit damage bonus is 100% now.
      crit_bonus_multiplier = 2.0;
    }

    virtual double total_spell_power() const
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

    virtual void execute()
    {
      shaman_t* o = ( player -> cast_pet() ) -> owner -> cast_shaman();

      spell_t::execute();

      o -> buffs_fire_elemental -> up();

      // Fire Nova and Fire Blast both seem to reset the swing time of the elemental, or
      // at least incur some sort of delay penalty to melee swings, so push back the
      // auto attack an execute time
      if ( ! background && player -> main_hand_attack && player -> main_hand_attack -> execute_event )
      {
        timespan_t time_to_next_hit = player -> main_hand_attack -> execute_time();
        player -> main_hand_attack -> execute_event -> reschedule( time_to_next_hit );
      }
    }

  };

  struct fire_shield_t : public fire_elemental_spell_t
  {
    fire_shield_t( player_t* player ) :
      fire_elemental_spell_t( player, "fire_shield" )
    {
      aoe                       = -1;
      background                = true;
      repeating                 = true;
      may_crit                  = true;
      base_execute_time         = timespan_t::from_seconds(3.0);
      base_dd_min = base_dd_max = 89;
      direct_power_mod          = player -> dbc.spell( 13376 ) -> effect1().coeff();
    }

    virtual void execute()
    {
      if ( player -> distance <= 11.0 )
        fire_elemental_spell_t::execute();
      else // Out of range, just re-schedule it
        schedule_execute();
    }
  };

  struct fire_nova_t : public fire_elemental_spell_t
  {
    fire_nova_t( player_t* player ) :
      fire_elemental_spell_t( player, "fire_nova" )
    {
      fire_elemental_pet_t* fe = dynamic_cast< fire_elemental_pet_t* >( player );

      aoe                  = -1;
      may_crit             = true;
      direct_power_mod     = player -> dbc.spell( 12470 ) -> effect1().coeff();
      cooldown -> duration = timespan_t::from_seconds(fe -> rng_ability_cooldown -> range( 30.0, 60.0 ));

      // 207 = 80
      base_cost            = player -> level * 2.750;
      // For now, model the cast time increase as well, see below
      base_execute_time    = player -> dbc.spell( 12470 ) -> cast_time( player -> level );

      base_dd_min          = 583;
      base_dd_max          = 663;
    }

    virtual void execute()
    {
      fire_elemental_pet_t* fe = dynamic_cast< fire_elemental_pet_t* >( player );
      // Randomize next cooldown duration here
      cooldown -> duration = timespan_t::from_seconds(fe -> rng_ability_cooldown -> range( 30.0, 60.0 ));

      fire_elemental_spell_t::execute();
    }
  };

  struct fire_blast_t : public fire_elemental_spell_t
  {
    fire_blast_t( player_t* player ) :
      fire_elemental_spell_t( player, "fire_blast" )
    {
      fire_elemental_pet_t* fe = dynamic_cast< fire_elemental_pet_t* >( player );

      may_crit             = true;
      base_cost            = ( player -> level ) * 3.554;
      base_execute_time    = timespan_t::zero;
      direct_power_mod     = player -> dbc.spell( 57984 ) -> effect1().coeff();
      cooldown -> duration = timespan_t::from_seconds(fe -> rng_ability_cooldown -> range( 5.0, 7.0 ));

      base_dd_min        = 276;
      base_dd_max        = 321;
    }

    virtual bool usable_moving()
    {
      return true;
    }

    virtual void execute()
    {
      fire_elemental_pet_t* fe = dynamic_cast< fire_elemental_pet_t* >( player );
      // Randomize next cooldown duration here
      cooldown -> duration = timespan_t::from_seconds(fe -> rng_ability_cooldown -> range( 5.0, 7.0 ));

      fire_elemental_spell_t::execute();
    }
  };

  struct fire_melee_t : public attack_t
  {
    double int_multiplier;
    double sp_multiplier;

    fire_melee_t( player_t* player ) :
      attack_t( "fire_melee", player, RESOURCE_NONE, SCHOOL_FIRE ),
      int_multiplier( 0.9647 ), sp_multiplier ( 0.6457 )
    {
      may_crit                     = true;
      background                   = true;
      repeating                    = true;
      trigger_gcd                  = timespan_t::zero;
      base_cost                    = 0;
      direct_power_mod             = 1.0;
      base_spell_power_multiplier  = 1.0;
      base_attack_power_multiplier = 0.0;
      // 166% crit damage bonus
      crit_bonus_multiplier        = 1.66;
      // Fire elemental has approx 1.5% crit
      base_crit                    = 0.015;
    }

    virtual double total_spell_power() const
    {
      double sp  = 0.0;

      pet_t* pet = player -> cast_pet();

      sp += pet -> intellect() * int_multiplier;
      sp += pet -> composite_spell_power( school ) * sp_multiplier;

      return floor( sp );
    }

    virtual void execute()
    {
      // If we're casting Fire Nova, we should clip a swing
      if ( time_to_execute > timespan_t::zero && player -> executing )
        schedule_execute();
      else
        attack_t::execute();
    }
  };

  struct auto_attack_t : public attack_t
  {
    auto_attack_t( player_t* player ) :
      attack_t( "auto_attack", player, RESOURCE_NONE, SCHOOL_FIRE, TREE_NONE, false )
    {
      player -> main_hand_attack = new fire_melee_t( player );
      player -> main_hand_attack -> weapon = &( player -> main_hand_weapon );
      player -> main_hand_attack -> base_execute_time = player -> main_hand_weapon.swing_time;

      trigger_gcd = timespan_t::zero;
    }

    virtual void execute()
    {
      player -> main_hand_attack -> schedule_execute();
    }

    virtual bool ready()
    {
      if ( player  -> is_moving() ) return false;
      return ( player -> main_hand_attack -> execute_event == 0 ); // not swinging
    }
  };

  rng_t*      rng_ability_cooldown;
  cooldown_t* cooldown_fire_nova;
  cooldown_t* cooldown_fire_blast;
  spell_t*    fire_shield;
  double      owner_int;
  double      owner_sp;

  fire_elemental_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "fire_elemental", true /*GUARDIAN*/ ), owner_int( 0.0 ), owner_sp( 0.0 )
  {
    intellect_per_owner         = 1.0;
    stamina_per_owner           = 1.0;
  }

  virtual void init_base()
  {
    pet_t::init_base();

    resource_base[ RESOURCE_HEALTH ] = 4643; // Approximated from lvl83 fire elem with naked shaman
    resource_base[ RESOURCE_MANA   ] = 8508; //

    health_per_stamina               = 7.5; // See above
    mana_per_intellect               = 4.5;

    main_hand_weapon.type            = WEAPON_BEAST;
    main_hand_weapon.min_dmg         = 427; // Level 85 Values, approximated
    main_hand_weapon.max_dmg         = 461;
    main_hand_weapon.damage          = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time      = timespan_t::from_seconds(2.0);

    rng_ability_cooldown             = get_rng( "fire_elemental_ability_cooldown" );
    rng_ability_cooldown -> average_range = 0;

    cooldown_fire_nova               = get_cooldown( "fire_nova" );
    cooldown_fire_blast              = get_cooldown( "fire_blast" );

    // Run menacingly to the target, start meleeing and acting erratically
    action_list_str                  = "travel/auto_attack/fire_nova/fire_blast";

    fire_shield                      = new fire_shield_t( this );
  }

  virtual int primary_resource() const { return RESOURCE_MANA; }

  virtual void regen( timespan_t periodicity )
  {
    if ( ! recent_cast() )
    {
      resource_gain( RESOURCE_MANA, 10.66 * periodicity.total_seconds() ); // FIXME! Does regen scale with gear???
    }
  }

  // Snapshot int, spell power from player
  virtual void summon( timespan_t duration )
  {
    shaman_t* o = owner -> cast_shaman();

    pet_t::summon();

    owner_int = owner -> intellect();
    owner_sp  = ( owner -> composite_spell_power( SCHOOL_FIRE ) - owner -> spell_power_per_intellect * owner_int ) * owner -> composite_spell_power_multiplier();

    fire_shield -> num_ticks = ( int ) ( duration / fire_shield -> base_execute_time );
    fire_shield -> execute();

    cooldown_fire_nova -> start();
    cooldown_fire_blast -> start();

    o -> buffs_fire_elemental -> trigger();
  }

  virtual void demise()
  {
    shaman_t* o = owner -> cast_shaman();

    pet_t::demise();

    o -> buffs_fire_elemental -> expire();
  }

  virtual double intellect() const
  {
    return owner_int;
  }

  virtual double composite_spell_power( const school_type ) const
  {
    return owner_sp;
  }

  virtual double composite_attack_power() const
  {
    return 0.0;
  }

  virtual double composite_attack_hit() const
  {
    return owner -> composite_spell_hit();
  }

  virtual double composite_attack_expertise() const
  {
    return owner -> composite_spell_hit() * 26.0 / 17.0;
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "travel"      ) return new travel_t     ( this );
    if ( name == "fire_nova"   ) return new fire_nova_t  ( this );
    if ( name == "fire_blast"  ) return new fire_blast_t ( this );
    if ( name == "auto_attack" ) return new auto_attack_t ( this );

    return pet_t::create_action( name, options_str );
  }

  virtual double composite_player_multiplier( const school_type /* school */, action_t* /* a */ ) const
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

// ==========================================================================
// Shaman Ability Triggers
// ==========================================================================

// trigger_flametongue_weapon ===============================================

static void trigger_flametongue_weapon( attack_t* a )
{
  shaman_t* p = a -> player -> cast_shaman();
  spell_t* ft = 0;
  double m_ft = a -> weapon -> swing_time.total_seconds() / 4.0;
  double m_coeff = 0.1253;

  if ( a -> weapon -> slot == SLOT_MAIN_HAND )
    ft = p -> flametongue_mh;
  else
    ft = p -> flametongue_oh;

  // Let's try a new formula for flametongue, based on EJ and such, but with proper damage ranges.
  // Player based scaling is based on max damage in flametongue weapon tooltip
  ft -> base_dd_min      = m_ft * p -> dbc.effect_min( p -> dbc.spell( 8024 ) -> effect2().id() , p -> level ) / 25.0;
  ft -> base_dd_max      = ft -> base_dd_min;
  ft -> direct_power_mod = 1.0;
  // New Flametongue mechanics, as per http://elitistjerks.com/f79/t110302-enhsim_cataclysm/p6/#post1839628
  if ( p -> primary_tree() == TREE_ENHANCEMENT )
  {
    ft -> base_spell_power_multiplier = 0;
    ft -> base_attack_power_multiplier = m_coeff * m_ft;
  }
  else
  {
    ft -> base_spell_power_multiplier = m_coeff * m_ft;
    ft -> base_attack_power_multiplier = 0;
  }

  ft -> execute();
}

// trigger_windfury_weapon ==================================================

struct windfury_delay_event_t : public event_t
{
  attack_t* wf;
  timespan_t delay;

  windfury_delay_event_t( sim_t* sim, player_t* p, attack_t* wf, timespan_t delay ) :
    event_t( sim, p ), wf( wf ), delay( delay )
  {
    name = "windfury_delay_event";
    sim -> add_event( this, delay );
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();

    p -> procs_windfury -> occur();
    wf -> execute();
    wf -> execute();
    wf -> execute();
  }
};

static bool trigger_windfury_weapon( attack_t* a )
{
  shaman_t* p = a -> player -> cast_shaman();
  attack_t* wf = 0;

  if ( a -> weapon -> slot == SLOT_MAIN_HAND )
    wf = p -> windfury_mh;
  else
    wf = p -> windfury_oh;

  if ( p -> cooldowns_windfury_weapon -> remains() > timespan_t::zero ) return false;

  if ( p -> rng_windfury_weapon -> roll( wf -> proc_chance() ) )
  {
    p -> cooldowns_windfury_weapon -> start( p -> rng_windfury_delay -> gauss( timespan_t::from_seconds(3.0), timespan_t::from_seconds(0.3) ) );

    // Delay windfury by some time, up to about a second
    new ( p -> sim ) windfury_delay_event_t( p -> sim, p, wf, p -> rng_windfury_delay -> gauss( p -> wf_delay, p -> wf_delay_stddev ) );
    return true;
  }
  return false;
}

// trigger_rolling_thunder ==================================================

static bool trigger_rolling_thunder ( spell_t* s )
{
  shaman_t* p = s -> player -> cast_shaman();

  if ( ! p -> buffs_lightning_shield -> check() )
    return false;

  if ( p -> rng_rolling_thunder -> roll( p -> talent_rolling_thunder -> proc_chance() ) )
  {
    p -> resource_gain( RESOURCE_MANA,
                        p -> dbc.spell( 88765 ) -> effect1().percent() * p -> resource_max[ RESOURCE_MANA ],
                        p -> gains_rolling_thunder );

    if ( p -> buffs_lightning_shield -> check() == p -> buffs_lightning_shield -> max_stack )
      p -> procs_wasted_ls -> occur();

    p -> buffs_lightning_shield -> trigger();
    p -> procs_rolling_thunder  -> occur();
    return true;
  }

  return false;
}

// trigger_static_shock =====================================================

static bool trigger_static_shock ( attack_t* a )
{
  shaman_t* p = a -> player -> cast_shaman();

  if ( ! p -> buffs_lightning_shield -> stack() )
    return false;

  if ( p -> rng_static_shock -> roll( p -> talent_static_shock -> proc_chance() ) )
  {
    p -> active_lightning_charge -> execute();
    p -> procs_static_shock -> occur();
    return true;
  }
  return false;
}
  
// trigger_improved_lava_lash ===============================================

static bool trigger_improved_lava_lash( attack_t* a )
{
  struct improved_lava_lash_t : public shaman_spell_t
  {
    rng_t* imp_ll_rng;
    cooldown_t* imp_ll_fs_cd;
    stats_t* fs_dummy_stat;
    
    improved_lava_lash_t( player_t* p ) :
      shaman_spell_t( "improved_lava_lash", p, "", SCHOOL_FIRE, TREE_ENHANCEMENT ),
      imp_ll_rng( 0 ), imp_ll_fs_cd( 0 )
    {
      aoe = 4;
      may_miss = may_crit = false;
      proc = true;
      callbacks = false;
      background = true;
      
      imp_ll_rng = sim -> get_rng( "improved_ll" );
      imp_ll_rng -> average_range = false;
      imp_ll_fs_cd = player -> get_cooldown( "improved_ll_fs_cooldown" );
      imp_ll_fs_cd -> duration = timespan_t::zero;
      fs_dummy_stat = player -> get_stats( "flame_shock_dummy" );
    }
    
    // Exclude targets with your flame shock on
    size_t available_targets( std::vector< player_t* >& tl ) const
    {
      for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
      {
        if ( sim -> actor_list[ i ] -> sleeping )
          continue;
        
        if ( ! sim -> actor_list[ i ] -> is_enemy() )
          continue;
        
        if ( sim -> actor_list[ i ] == target )
          continue;
          
        shaman_targetdata_t* td = targetdata_t::get( player, sim -> actor_list[ i ] ) -> cast_shaman();
        
        if ( td -> dots_flame_shock -> ticking )
          continue;

        tl.push_back( sim -> actor_list[ i ] );
      }
      
      return tl.size();
    }

    std::vector< player_t* > target_list() const
    {
      std::vector< player_t* > t;
      
      size_t total_targets = available_targets( t );
      
      // Reduce targets to aoe amount by removing random entries from the 
      // target list until it's at aoe amount
      while ( total_targets > static_cast< size_t >( aoe ) )
      {
        t.erase( t.begin() + static_cast< size_t >( imp_ll_rng -> range( 0, total_targets ) ) );
        total_targets--;
      }
      
      return t;
    }
    
    // Impact on any target triggers a flame shock; for now, cache the 
    // relevant parts of the spell and return them after execute has finished
    void impact( player_t* t, int, double )
    {
      if ( sim -> debug )
        log_t::output( sim, "%s spreads Flame Shock (off of %s) on %s", 
                      player -> name(), 
                      target -> name(),
                      t -> name() );
      
      shaman_t* p = player -> cast_shaman();
      
      double dd_min = p -> action_flame_shock -> base_dd_min,
             dd_max = p -> action_flame_shock -> base_dd_max,
             coeff = p -> action_flame_shock -> direct_power_mod,
             real_base_cost = p -> action_flame_shock -> base_cost;
      player_t* original_target = p -> action_flame_shock -> target;
      cooldown_t* original_cd = p -> action_flame_shock -> cooldown;
      stats_t* original_stats = p -> action_flame_shock -> stats;
      
      p -> action_flame_shock -> base_dd_min = 0;
      p -> action_flame_shock -> base_dd_max = 0;
      p -> action_flame_shock -> direct_power_mod = 0;
      p -> action_flame_shock -> background = true;
      p -> action_flame_shock -> callbacks = false;
      p -> action_flame_shock -> proc = true;
      p -> action_flame_shock -> may_crit = false;
      p -> action_flame_shock -> may_miss = false;
      p -> action_flame_shock -> base_cost = 0;
      p -> action_flame_shock -> target = t;
      p -> action_flame_shock -> cooldown = imp_ll_fs_cd;
      p -> action_flame_shock -> stats = fs_dummy_stat;
      
      p -> action_flame_shock -> execute();
      
      p -> action_flame_shock -> base_dd_min = dd_min;
      p -> action_flame_shock -> base_dd_max = dd_max;
      p -> action_flame_shock -> direct_power_mod = coeff;
      p -> action_flame_shock -> background = false;
      p -> action_flame_shock -> callbacks = true;
      p -> action_flame_shock -> proc = false;
      p -> action_flame_shock -> may_crit = true;
      p -> action_flame_shock -> may_miss = true;
      p -> action_flame_shock -> base_cost = real_base_cost;
      p -> action_flame_shock -> target = original_target;
      p -> action_flame_shock -> cooldown = original_cd;
      p -> action_flame_shock -> stats = original_stats;
      
      // Hide the Flame Shock dummy stat and improved_lava_lash from reports
      fs_dummy_stat -> num_executes = 0;
      stats -> num_executes = 0;
    }
  };
  
  // Do not spread the love when there is only poor Fluffy Pillow against you
  if ( a -> sim -> num_enemies == 1 )
    return false;
  
  shaman_targetdata_t* t = a -> targetdata() -> cast_shaman();
  
  if ( ! t -> dots_flame_shock -> ticking )
    return false;
  
  shaman_t* p = a -> player -> cast_shaman();
  
  if ( p -> talent_improved_lava_lash -> rank() == 0 )
    return false;

  if ( ! p -> action_flame_shock )
  {
    p -> action_flame_shock = p -> find_action( "flame_shock" );
    assert( p -> action_flame_shock );
  }
  
  if ( ! p -> action_improved_lava_lash )
  {
    p -> action_improved_lava_lash = new improved_lava_lash_t( p );
    p -> action_improved_lava_lash -> init();
  }
  
  // Splash from the action's target
  p -> action_improved_lava_lash -> target = a -> target;
  p -> action_improved_lava_lash -> execute();
  
  return true;
}

// ==========================================================================
// Shaman Secondary Spells / Attacks
// ==========================================================================

struct lava_burst_overload_t : public shaman_spell_t
{
  lava_burst_overload_t( player_t* player, bool dtr=false ) :
    shaman_spell_t( "lava_burst_overload", 77451, player )
  {
    shaman_t* p          = player -> cast_shaman();
    overload             = true;
    background           = true;

    // Shamanism, NOTE NOTE NOTE, elemental overloaded abilities use
    // a _DIFFERENT_ effect in shamanism, that has 75% of the shamanism
    // "real" effect
    direct_power_mod  += p -> spec_shamanism -> effect_base_value( 2 ) / 100.0;

    base_multiplier     *= 1.0 +
      p -> talent_concussion -> mod_additive( P_GENERIC ) +
      p -> talent_call_of_flame -> effect2().percent() +
      p -> glyph_lava_burst -> mod_additive( P_GENERIC );

    crit_bonus_multiplier *= 1.0 +
      p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE ) +
      p -> talent_lava_flows -> mod_additive( P_CRIT_DAMAGE );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new lava_burst_overload_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void player_buff()
  {
    shaman_spell_t::player_buff();

    shaman_targetdata_t* td = targetdata() -> cast_shaman();

    if ( td -> dots_flame_shock -> ticking )
      player_crit += 1.0;
  }
};

struct lightning_bolt_overload_t : public shaman_spell_t
{
  lightning_bolt_overload_t( player_t* player, bool dtr=false ) :
    shaman_spell_t( "lightning_bolt_overload", 45284, player )
  {
    shaman_t* p          = player -> cast_shaman();
    overload             = true;
    background           = true;

    // Shamanism, NOTE NOTE NOTE, elemental overloaded abilities use
    // a _DIFFERENT_ effect in shamanism, that has 75% of the shamanism
    // "real" effect
    direct_power_mod  += p -> spec_shamanism -> effect_base_value( 2 ) / 100.0;

    // Elemental fury
    crit_bonus_multiplier *= 1.0 + p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );

    base_multiplier     *= 1.0 +
      p -> talent_concussion -> mod_additive( P_GENERIC );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new lightning_bolt_overload_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      trigger_rolling_thunder( this );
    }
  }
};

struct chain_lightning_overload_t : public shaman_spell_t
{
  int glyph_targets;

  chain_lightning_overload_t( player_t* player, bool dtr=false ) :
    shaman_spell_t( "chain_lightning_overload", 45297, player ), glyph_targets( 0 )
  {
    shaman_t* p          = player -> cast_shaman();
    overload             = true;
    background           = true;

    // Shamanism, NOTE NOTE NOTE, elemental overloaded abilities use
    // a _DIFFERENT_ effect in shamanism, that has 75% of the shamanism
    // "real" effect
    direct_power_mod  += p -> spec_shamanism -> effect_base_value( 2 ) / 100.0;

    // Elemental fury
    crit_bonus_multiplier *= 1.0 + p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );

    base_multiplier     *= 1.0 +
      p -> talent_concussion -> mod_additive( P_GENERIC ) +
      p -> glyph_chain_lightning -> mod_additive( P_GENERIC );

    glyph_targets        = ( int ) p -> glyph_chain_lightning -> mod_additive( P_TARGET );

    base_add_multiplier = 0.7;
    aoe = ( 2 + glyph_targets );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new chain_lightning_overload_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    if ( result_is_hit() )
      trigger_rolling_thunder( this );
  }
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
    hasted_ticks     = false;
    may_crit         = false;

    // Override spell data values, as they seem wrong, instead
    // make searing totem calculate the damage portion of the
    // spell, without using any tick power modifiers or
    // player based multipliers here
    tick_power_mod   = 0.0;
  }

  // Don't double dip
  virtual void target_debuff( player_t* /* t */, int /* dmg_type */ ) { }

  virtual double total_td_multiplier() const
  {
    shaman_targetdata_t* td = targetdata() -> cast_shaman();
    return td -> debuffs_searing_flames -> stack();
  }

  virtual void last_tick( dot_t* d )
  {
    shaman_targetdata_t* td = targetdata() -> cast_shaman();
    shaman_spell_t::last_tick( d );
    td -> debuffs_searing_flames -> expire();
  }
};

struct lightning_charge_t : public shaman_spell_t
{
  int consume_threshold;

  lightning_charge_t( player_t* player, const std::string& n, bool dtr=false ) :
    shaman_spell_t( n.c_str(), 26364, player ), consume_threshold( 0 )
  {
    // Use the same name "lightning_shield" to make sure the cost of refreshing the shield is included with the procs.
    shaman_t* p      = player -> cast_shaman();
    background       = true;
    may_crit         = true;
    base_multiplier *= 1.0 +
      p -> talent_improved_shields -> mod_additive( P_GENERIC );

    crit_bonus_multiplier *= 1.0 + p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );

    consume_threshold = ( int ) p -> talent_fulmination -> base_value();

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new lightning_charge_t( p, n, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void player_buff()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_spell_t::player_buff();

    if ( consume_threshold > 0 )
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
    proc                 = true;

    crit_bonus_multiplier *= 1.0 +
      p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );

    // Don't cooldown here, unleash elements ability will handle it
    cooldown -> duration = timespan_t::zero;
  }

  virtual void execute()
  {
    // Figure out weapons
    shaman_t* p = player -> cast_shaman();

    if ( p -> main_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
    {
      shaman_spell_t::execute();
    }

    if ( p -> off_hand_weapon.type != WEAPON_NONE &&
         p -> off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
    {
      shaman_spell_t::execute();
    }
  }
};

struct flametongue_weapon_spell_t : public shaman_spell_t
{
  flametongue_weapon_spell_t( player_t* player, const std::string& n ) :
    shaman_spell_t( n.c_str(), 8024, player )
  {
    may_crit           = true;
    background         = true;
    proc               = true;

    init();
  }
};

struct windfury_weapon_attack_t : public shaman_attack_t
{
  windfury_weapon_attack_t( player_t* player, const std::string& n, weapon_t* w ) :
    shaman_attack_t( n.c_str(), 33757, player )
  {
    shaman_t* p      = player -> cast_shaman();
    weapon           = w;
    school           = SCHOOL_PHYSICAL;
    stats -> school  = SCHOOL_PHYSICAL;
    background       = true;
    callbacks        = false; // Windfury does not proc any On-Equip procs, apparently
    base_multiplier *= 1.0 + p -> talent_elemental_weapons -> effect3().percent();

    init();
  }

  virtual void player_buff()
  {
    shaman_attack_t::player_buff();
    player_attack_power += weapon -> buff_value;
  }

  virtual double proc_chance() const
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
    background            = true;
    windfury              = false;
    may_dodge = may_parry = false;

    // Don't cooldown here, unleash elements will handle it
    cooldown -> duration = timespan_t::zero;
  }

  virtual void execute()
  {
    // Figure out weapons
    shaman_t* p = player -> cast_shaman();

    if ( p -> main_hand_weapon.buff_type == WINDFURY_IMBUE  )
    {
      weapon = &( p -> main_hand_weapon );
      shaman_attack_t::execute();
    }

    if ( p -> off_hand_weapon.type != WEAPON_NONE &&
         p -> off_hand_weapon.buff_type == WINDFURY_IMBUE )
    {
      weapon = &( p -> off_hand_weapon );
      shaman_attack_t::execute();
    }
  }
};

struct stormstrike_attack_t : public shaman_attack_t
{
  stormstrike_attack_t( player_t* player, const char* n, uint32_t id, weapon_t* w ) :
    shaman_attack_t( n, id, player )
  {
    shaman_t* p          = player -> cast_shaman();

    background           = true;
    may_miss             = false;
    may_dodge            = false;
    may_parry            = false;
    weapon               = w;

    weapon_multiplier   *= 1.0 + p -> talent_focused_strikes -> mod_additive( P_GENERIC );
    base_multiplier     *= 1.0 +
      p -> sets -> set( SET_T11_2PC_MELEE ) -> mod_additive( P_GENERIC );
  }
};

// ==========================================================================
// Shaman Attack
// ==========================================================================

// shaman_attack_t::execute =================================================

void shaman_attack_t::execute()
{
  shaman_t* p = player -> cast_shaman();

  attack_t::execute();

  p -> buffs_spiritwalkers_grace -> up();

  if ( result_is_hit() && ! proc )
  {
    int   mwstack = p -> buffs_maelstrom_weapon -> check();
    double chance = weapon -> proc_chance_on_swing(
      util_t::talent_rank( p -> talent_maelstrom_weapon -> rank(), 3, 3.0, 6.0, 10.0 ) );

    if ( p -> buffs_maelstrom_weapon -> trigger( 1, -1, chance ) )
    {
      if ( mwstack == p -> buffs_maelstrom_weapon -> max_stack )
        p -> procs_wasted_mw -> occur();

      p -> procs_maelstrom_weapon -> occur();
    }

    if ( windfury && weapon -> buff_type == WINDFURY_IMBUE )
      trigger_windfury_weapon( this );

    if ( flametongue && weapon -> buff_type == FLAMETONGUE_IMBUE )
      trigger_flametongue_weapon( this );

    if ( p -> rng_primal_wisdom -> roll( p -> spec_primal_wisdom -> proc_chance() ) )
    {
      double amount = p -> dbc.spell( 63375 ) -> effect1().percent() * p -> resource_base[ RESOURCE_MANA ];
      p -> resource_gain( RESOURCE_MANA, amount, p -> gains_primal_wisdom );
    }
  }
}

void shaman_attack_t::consume_resource()
{
  attack_t::consume_resource();

  shaman_t* p = player -> cast_shaman();

  if ( result == RESULT_CRIT )
    p -> buffs_flurry -> trigger( p -> buffs_flurry -> initial_stacks() );
}

// shaman_attack_t::player_buff =============================================

void shaman_attack_t::player_buff()
{
  attack_t::player_buff();
  shaman_t* p = player -> cast_shaman();

  if ( school == SCHOOL_FIRE && p -> buffs_stormfire -> up() )
    player_multiplier *= 1.0 + p -> buffs_stormfire -> base_value();
}


// shaman_attack_t::cost_reduction ==========================================

double shaman_attack_t::cost_reduction() const
{
  shaman_t* p = player -> cast_shaman();
  double   cr = 0.0;

  if ( p -> buffs_shamanistic_rage -> up() )
    cr += p -> buffs_shamanistic_rage -> mod_additive( P_RESOURCE_COST );

  return cr;
}

// shaman_attack_t::cost ====================================================

double shaman_attack_t::cost() const
{
  double    c = attack_t::cost();
  c *= 1.0 + cost_reduction();
  if ( c < 0 ) c = 0.0;
  return c;
}

// Melee Attack =============================================================

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
    trigger_gcd = timespan_t::zero;
    base_cost   = 0;

    if ( p -> dual_wield() ) base_hit -= 0.19;
  }

  virtual double swing_haste() const
  {
    shaman_t* p = player -> cast_shaman();
    double h = attack_t::swing_haste();
    if ( p -> buffs_flurry -> up() )
      h *= 1.0 / ( 1.0 + p -> buffs_flurry -> base_value() );

    if ( p -> buffs_unleash_wind -> up() )
      h *= 1.0 / ( 1.0 + ( p -> buffs_unleash_wind -> base_value( E_APPLY_AURA, A_319 ) ) );

    return h;
  }

  virtual timespan_t execute_time() const
  {
    timespan_t t = shaman_attack_t::execute_time();
    if ( ! player -> in_combat )
    {
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t/2, timespan_t::from_seconds(0.2) ) : t/2 ) : timespan_t::from_seconds(0.01);
    }
    return t;
  }

  void execute()
  {
    shaman_t* p = player -> cast_shaman();

    if ( time_to_execute > timespan_t::zero && p -> executing )
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
    // Clipped swings do not eat unleash wind buffs
    if ( time_to_execute > timespan_t::zero && ! p -> executing )
      p -> buffs_unleash_wind -> decrement();
  }
};

// Auto Attack ==============================================================

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
      if ( p -> primary_tree() != TREE_ENHANCEMENT ) return;
      p -> off_hand_attack = new melee_t( "melee_off_hand", player, sync_weapons );
      p -> off_hand_attack -> weapon = &( p -> off_hand_weapon );
      p -> off_hand_attack -> base_execute_time = p -> off_hand_weapon.swing_time;
    }

    trigger_gcd = timespan_t::zero;
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
    if ( p -> is_moving() ) return false;
    return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Lava Lash Attack =========================================================

struct lava_lash_t : public shaman_attack_t
{
  int    wf_cd_only;
  double flametongue_bonus,
         sf_bonus;

  lava_lash_t( player_t* player, const std::string& options_str ) :
    shaman_attack_t( "lava_lash", "Lava Lash", player ),
    wf_cd_only( 0 ), flametongue_bonus( 0 ), sf_bonus( 0 )
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

    if ( weapon -> type == WEAPON_NONE )
      background        = true; // Do not allow execution.

    // Lava lash's flametongue bonus is in the weird dummy script effect
    flametongue_bonus   = base_value( E_DUMMY ) / 100.0;

    // Searing flames bonus to base weapon damage
    sf_bonus            = p -> talent_improved_lava_lash -> effect2().percent() +
                          p -> sets -> set( SET_T12_2PC_MELEE ) -> base_value() / 100.0;
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_attack_t::execute();

    if ( result_is_hit() )
    {
      shaman_targetdata_t* td = targetdata() -> cast_shaman();
      td -> debuffs_searing_flames -> expire();
      if ( p -> active_searing_flames_dot )
        p -> active_searing_flames_dot -> dot() -> cancel();

      trigger_static_shock( this );
      if ( td -> dots_flame_shock -> ticking )
        trigger_improved_lava_lash( this );
    }
  }

  virtual void player_buff()
  {
    shaman_t* p = player -> cast_shaman();

    shaman_attack_t::player_buff();

    // Lava lash damage calculation from: http://elitistjerks.com/f79/t110302-enhsim_cataclysm/p11/#post1935780

    // First group, Searing Flames + Flametongue bonus
    shaman_targetdata_t* td = targetdata() -> cast_shaman();
    player_multiplier *= 1.0 + td -> debuffs_searing_flames -> check() * sf_bonus +
                               flametongue_bonus;

    // Second group, Improved Lava Lash + Glyph of Lava Lash + T11 2Piece bonus
    player_multiplier *= 1.0 + p -> glyph_lava_lash -> mod_additive( P_GENERIC ) +
                               p -> talent_improved_lava_lash -> effect1().percent() +
                               p -> sets -> set( SET_T11_2PC_MELEE ) -> mod_additive( P_GENERIC );
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();

    if ( wf_cd_only )
      if ( p -> cooldowns_windfury_weapon -> remains() > timespan_t::zero )
        return false;

    return shaman_attack_t::ready();
  }
};

// Primal Strike Attack =====================================================

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
    cooldown -> duration = p -> dbc.spell( id ) -> cooldown();

    base_multiplier     += p -> talent_focused_strikes -> mod_additive( P_GENERIC );
  }

  virtual void execute()
  {
    shaman_attack_t::execute();
    if ( result_is_hit() )
      trigger_static_shock( this );
  }
};

// Stormstrike Attack =======================================================

struct stormstrike_t : public shaman_attack_t
{
  stormstrike_attack_t * stormstrike_mh;
  stormstrike_attack_t * stormstrike_oh;

  stormstrike_t( player_t* player, const std::string& options_str ) :
    shaman_attack_t( "stormstrike", "Stormstrike", player ),
    stormstrike_mh( 0 ), stormstrike_oh( 0 )
  {
    shaman_t* p = player -> cast_shaman();
    check_talent( p -> talent_stormstrike -> rank() );

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    weapon               = &( p -> main_hand_weapon );
    weapon_multiplier    = 0.0;
    may_crit             = false;
    cooldown             = p -> cooldowns_strike;
    cooldown -> duration = p -> dbc.spell( id ) -> cooldown();

    // Actual damaging attacks are done by stormstrike_attack_t
    stormstrike_mh = new stormstrike_attack_t( player, "stormstrike_mh", effect_trigger_spell( 2 ), &( p -> main_hand_weapon ) );
    add_child( stormstrike_mh );

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      stormstrike_oh = new stormstrike_attack_t( player, "stormstrike_oh", effect_trigger_spell( 3 ), &( p -> off_hand_weapon ) );
      add_child( stormstrike_oh );
    }
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();

    // Bypass shaman-specific attack based procs and co for this spell, the relevant ones
    // are handled by stormstrike_attack_t
    attack_t::execute();

    if ( result_is_hit() )
    {
      p -> buffs_stormstrike -> trigger();
      if ( p -> set_bonus.tier12_4pc_melee() )
        p -> buffs_stormfire -> trigger();
      stormstrike_mh -> execute();
      if ( stormstrike_oh ) stormstrike_oh -> execute();

      bool shock = trigger_static_shock( this );
      if ( !shock && stormstrike_oh ) trigger_static_shock( this );
    }
  }
};

// ==========================================================================
// Shaman Spell
// ==========================================================================

// shaman_spell_t::haste ====================================================

double shaman_spell_t::haste() const
{
  shaman_t* p = player -> cast_shaman();
  double h = spell_t::haste();
  if ( p -> buffs_elemental_mastery -> up() )
    h *= 1.0 / ( 1.0 + p -> buffs_elemental_mastery -> base_value( E_APPLY_AURA, A_MOD_CASTING_SPEED_NOT_STACK ) );

  if ( p -> buffs_tier13_4pc_healer -> up() )
    h *= 1.0 / ( 1.0 + p -> buffs_tier13_4pc_healer -> effect1().percent() );

  return h;
}

// shaman_spell_t::cost_reduction ===========================================

double shaman_spell_t::cost_reduction() const
{
  shaman_t* p = player -> cast_shaman();
  double   cr = base_cost_reduction;

  if ( p -> buffs_shamanistic_rage -> up() )
    cr += p -> buffs_shamanistic_rage -> mod_additive( P_RESOURCE_COST );

  if ( harmful && callbacks && ! proc && p -> buffs_elemental_focus -> up() )
    cr += p -> buffs_elemental_focus -> mod_additive( P_RESOURCE_COST );

  if ( base_execute_time == timespan_t::zero )
    cr += p -> spec_mental_quickness -> mod_additive( P_RESOURCE_COST );

  return cr;
}

// shaman_spell_t::cost =====================================================

double shaman_spell_t::cost() const
{
  double    c = spell_t::cost();

  c *= 1.0 + cost_reduction();

  if ( c < 0 ) c = 0;
  return c;
}

// shaman_spell_t::consume_resource =========================================

void shaman_spell_t::consume_resource()
{
  spell_t::consume_resource();
  shaman_t* p = player -> cast_shaman();
  if ( harmful && callbacks && ! proc && resource_consumed > 0 && p -> buffs_elemental_focus -> up() )
    p -> buffs_elemental_focus -> decrement();
}

// shaman_spell_t::execute_time =============================================

timespan_t shaman_spell_t::execute_time() const
{
  shaman_t* p = player -> cast_shaman();
  if ( p -> buffs_natures_swiftness -> up() && school == SCHOOL_NATURE && base_execute_time < timespan_t::from_seconds(10.0) )
    return timespan_t::from_seconds(1.0 + p -> buffs_natures_swiftness -> base_value());

  return spell_t::execute_time();
}

// shaman_spell_t::player_buff ==============================================

void shaman_spell_t::player_buff()
{
  shaman_t* p = player -> cast_shaman();
  spell_t::player_buff();

  // Can do this here, as elemental will not have access to
  // stormstrike, and all nature spells will benefit from this for enhancement
  if ( school == SCHOOL_NATURE && p -> buffs_stormstrike -> up() )
  {
    player_crit +=
      p -> buffs_stormstrike -> base_value( E_APPLY_AURA, A_308 ) +
      p -> glyph_stormstrike -> mod_additive( P_EFFECT_1 ) / 100.0;
  }

  // Apply Tier12 4 piece enhancement bonus as multiplicative for now
  if ( ! is_totem && school == SCHOOL_FIRE && p -> buffs_stormfire -> up() )
    player_multiplier *= 1.0 + p -> buffs_stormfire -> base_value();
}

// shaman_spell_t::execute ==================================================

void shaman_spell_t::execute()
{
  shaman_t* p = player -> cast_shaman();
  spell_t::execute();

  // Triggers wont happen for procs or totems
  if ( proc || ! callbacks )
    return;

  if ( result_is_hit() )
  {
    if ( result == RESULT_CRIT )
    {
      if ( is_direct_damage() )
      {
        if ( p -> talent_elemental_devastation -> rank() )
          p -> buffs_elemental_devastation -> trigger();

        // Overloads dont trigger elemental focus
        if ( ! overload && p -> talent_elemental_focus -> rank() )
          p -> buffs_elemental_focus -> trigger( p -> buffs_elemental_focus -> initial_stacks() );
      }
    }

    if ( school == SCHOOL_FIRE )
      p -> buffs_unleash_flame -> expire();
  }

  // Record maelstrom weapon stack usage
  if ( maelstrom && p -> primary_tree() == TREE_ENHANCEMENT )
  {
    p -> procs_maelstrom_weapon_used[ p -> buffs_maelstrom_weapon -> check() ] -> occur();
  }

  // Shamans have specialized swing timer reset system, where every cast time spell
  // resets the swing timers, _IF_ the spell is not maelstromable, or the maelstrom
  // weapon stack is zero.
  if ( execute_time() > timespan_t::zero )
  {
    if ( ! maelstrom || p -> buffs_maelstrom_weapon -> check() == 0 )
    {
      if ( sim -> debug )
      {
        log_t::output( sim, "Resetting swing timers for '%s', maelstrom=%d, stacks=%d",
                       name_str.c_str(), maelstrom, p -> buffs_maelstrom_weapon -> check() );
      }

      timespan_t time_to_next_hit;

      // Non-maelstromable spell finishes casting, reset swing timers
      if ( player -> main_hand_attack && player -> main_hand_attack -> execute_event )
      {
        time_to_next_hit = player -> main_hand_attack -> execute_time();
        player -> main_hand_attack -> execute_event -> reschedule( time_to_next_hit );
      }

      // Offhand
      if ( player -> off_hand_attack && player -> off_hand_attack -> execute_event )
      {
        time_to_next_hit = player -> off_hand_attack -> execute_time();
        player -> off_hand_attack -> execute_event -> reschedule( time_to_next_hit );
      }
    }
  }
}

// shaman_spell_t::schedule_execute =========================================

void shaman_spell_t::schedule_execute()
{
  if ( sim -> log )
  {
    log_t::output( sim, "%s schedules execute for %s", player -> name(), name() );
  }

  time_to_execute = execute_time();

  execute_event = new ( sim ) action_execute_event_t( sim, this, time_to_execute );

  if ( ! background )
  {
    player -> executing = this;
    player -> gcd_ready = sim -> current_time + gcd();
    if ( player -> action_queued && sim -> strict_gcd_queue )
    {
      player -> gcd_ready -= sim -> queue_gcd_reduction;
    }
  }
}

// ==========================================================================
// Shaman Spells
// ==========================================================================

// Bloodlust Spell ==========================================================

struct bloodlust_t : public shaman_spell_t
{
  bloodlust_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "bloodlust", "Bloodlust", player, options_str )
  {
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

    if (  player -> buffs.bloodlust -> cooldown -> remains() > timespan_t::zero )
      return false;

    return shaman_spell_t::ready();
  }
};

// Chain Lightning Spell ====================================================

struct chain_lightning_t : public shaman_spell_t
{
  int      glyph_targets;
  chain_lightning_overload_t* overload;

  chain_lightning_t( player_t* player, const std::string& options_str, bool dtr = false ) :
    shaman_spell_t( "chain_lightning", "Chain Lightning", player, options_str ),
    glyph_targets( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    maelstrom          = true;
    direct_power_mod += p -> spec_shamanism -> effect_base_value( 1 ) / 100.0;
    base_execute_time += timespan_t::from_millis(p -> spec_shamanism -> effect_base_value( 3 ));
    crit_bonus_multiplier *= 1.0 + p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );
    cooldown -> duration += timespan_t::from_seconds(p -> spec_elemental_fury -> mod_additive( P_COOLDOWN ));

    base_multiplier     *= 1.0 +
      p -> talent_concussion -> mod_additive( P_GENERIC ) +
      p -> glyph_chain_lightning -> mod_additive( P_GENERIC );

    base_cost_reduction += p -> talent_convection -> mod_additive( P_RESOURCE_COST );

    glyph_targets        = ( int ) p -> glyph_chain_lightning -> mod_additive( P_TARGET );

    overload             = new chain_lightning_overload_t( player );

    base_add_multiplier = 0.7;
    aoe = ( 2 + glyph_targets );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new chain_lightning_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void player_buff()
  {
    shaman_t* p = player -> cast_shaman();

    shaman_spell_t::player_buff();

    if ( p -> buffs_maelstrom_weapon -> up() )
      player_multiplier *= 1.0 + p -> sets -> set( SET_T13_2PC_MELEE ) -> effect1().percent();
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_spell_t::execute();

    p -> buffs_maelstrom_weapon        -> expire();
    p -> buffs_elemental_mastery_insta -> expire();

    p -> cooldowns_elemental_mastery -> ready += timespan_t::from_millis(p -> talent_feedback -> base_value());
  }
  
  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    shaman_spell_t::impact( t, impact_result, travel_dmg );
    
    if ( result_is_hit( impact_result ) )
    {
      shaman_t* p = player -> cast_shaman();

      trigger_rolling_thunder( this );
      
      double overload_chance = p -> composite_mastery() * p -> mastery_elemental_overload -> base_value( E_APPLY_AURA, A_DUMMY, 0 ) / 3.0;
      
      if ( overload_chance && p -> rng_elemental_overload -> roll( overload_chance ) )
      {
        overload -> execute();
        if ( p -> set_bonus.tier13_4pc_caster() )
          p -> buffs_tier13_4pc_caster -> trigger();
      }
    }
  }

  virtual timespan_t execute_time() const
  {
    timespan_t t    = shaman_spell_t::execute_time();
    shaman_t* p = player -> cast_shaman();

    if ( p -> buffs_elemental_mastery_insta -> up() )
      t *= 1.0 + p -> buffs_elemental_mastery_insta -> base_value();

    t *= 1.0 + p -> buffs_maelstrom_weapon -> stack() * p -> buffs_maelstrom_weapon -> mod_additive( P_CAST_TIME );

    return t;
  }

  double cost_reduction() const
  {
    shaman_t* p = player -> cast_shaman();
    double   cr = shaman_spell_t::cost_reduction();

    cr += p -> buffs_maelstrom_weapon -> stack() * p -> buffs_maelstrom_weapon -> mod_additive( P_RESOURCE_COST );

    return cr;
  }
};

// Elemental Mastery Spell ==================================================

struct elemental_mastery_t : public shaman_spell_t
{
  elemental_mastery_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "elemental_mastery", "Elemental Mastery", player, options_str )
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
    if ( p -> set_bonus.tier13_2pc_caster() )
      p -> buffs_tier13_2pc_caster     -> trigger();
  }
};

// Fire Nova Spell ==========================================================
  
struct fire_nova_explosion_t : public shaman_spell_t
{
  player_t* emit_target;
  double m_additive;
  
  fire_nova_explosion_t( player_t* player ) :
    shaman_spell_t( "fire_nova_explosion", 8349, player ), 
    emit_target( 0 ), m_additive( 0.0 )
  {
    shaman_t* p = player -> cast_shaman();
    m_additive =  p -> talent_call_of_flame -> effect1().percent();
    aoe = -1;
    background = true;
    callbacks = false;
    
    crit_bonus_multiplier *= 1.0 + p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );
  }

  void player_buff()
  {
    shaman_spell_t::player_buff();
    
    shaman_t* p = player -> cast_shaman();
    
    if ( p -> buffs_unleash_flame -> up() )
      base_multiplier = 1.0 + m_additive + p -> buffs_unleash_flame -> mod_additive( P_GENERIC );
    else
      base_multiplier = 1.0 + m_additive;
  }
  
  // Fire nova does not damage the main target.
  size_t available_targets( std::vector< player_t* >& tl ) const
  {
    for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
    {
      if ( ! sim -> actor_list[ i ] -> sleeping &&
           sim -> actor_list[ i ] -> is_enemy() && 
           sim -> actor_list[ i ] != emit_target )
        tl.push_back( sim -> actor_list[ i ] );
    }
    
    return tl.size();
  }
  
  void reset()
  {
    shaman_spell_t::reset();
    
    emit_target = 0;
  }
};

struct fire_nova_t : public shaman_spell_t
{
  fire_nova_explosion_t* explosion;

  fire_nova_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "fire_nova", "Fire Nova", player, options_str ), 
    explosion( 0 )
  {
    aoe       = -1;
    may_crit  = false;
    may_miss  = false;
    callbacks = false;
    explosion = new fire_nova_explosion_t( player );
    
    add_child( explosion );
  }

  virtual bool ready()
  {
    shaman_targetdata_t* td = targetdata() -> cast_shaman();

    if ( ! td -> dots_flame_shock -> ticking )
      return false;

    return shaman_spell_t::ready();
  }
  
  void impact( player_t* t, int, double )
  {
    explosion -> emit_target = t;
    explosion -> execute();
  }
  
  // Fire nova is emitted on all targets with a flame shock from us .. so
  std::vector< player_t* > target_list() const
  {
    std::vector< player_t* > t;
    
    for ( player_t* e = sim -> target_list; e; e = e -> next )
    {
      if ( e -> sleeping || ! e -> is_enemy() )
        continue;
      
      shaman_targetdata_t* td = targetdata_t::get( player, e ) -> cast_shaman();
      if ( td -> dots_flame_shock -> ticking )
        t.push_back( e );
    }
    
    return t;
  }
};
  
// Earthquake Spell =========================================================
  
struct earthquake_rumble_t : public shaman_spell_t
{
  earthquake_rumble_t( player_t* player ) :
    shaman_spell_t( "earthquake_rumble", 77478, player )
  {
    harmful = true;
    aoe = -1;
    background = true;
    school = SCHOOL_PHYSICAL;
    stats -> school = SCHOOL_PHYSICAL;
    callbacks = false;
  }
  
  double total_spell_power() const
  {
    return player -> composite_spell_power( SCHOOL_NATURE ) * player -> composite_spell_power_multiplier();
  }
};

struct earthquake_t : public shaman_spell_t
{
  earthquake_rumble_t* quake;
  
  earthquake_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "earthquake", "Earthquake", player, options_str ),
    quake( 0 )
  {
    base_td = 0;
    base_dd_min = base_dd_max = direct_power_mod = 0;
    harmful = true;
    may_miss = false;
    may_miss = may_crit = may_dodge = may_parry = false;
    num_ticks = ( int ) duration().total_seconds();
    base_tick_time = timespan_t::from_seconds(1.0);
    hasted_ticks = false;

    quake = new earthquake_rumble_t( player );
    
    add_child( quake );
  }
  
  void tick( dot_t* d )
  {
    shaman_spell_t::tick( d );
    
    quake -> execute();
  }
};

// Lava Burst Spell =========================================================

struct lava_burst_t : public shaman_spell_t
{
  lava_burst_overload_t* overload;
  double                m_additive;

  lava_burst_t( player_t* player, const std::string& options_str, bool dtr=false ) :
    shaman_spell_t( "lava_burst", "Lava Burst", player, options_str ),
    m_additive( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    // Shamanism
    direct_power_mod += p -> spec_shamanism -> effect_base_value( 1 ) / 100.0;
    base_execute_time   += timespan_t::from_millis(p -> spec_shamanism -> effect_base_value( 3 ));
    base_cost_reduction += p -> talent_convection -> mod_additive( P_RESOURCE_COST );
    m_additive          +=
      p -> talent_concussion -> mod_additive( P_GENERIC ) +
      p -> talent_call_of_flame -> effect2().percent() +
      p -> glyph_lava_burst -> mod_additive( P_GENERIC );

    crit_bonus_multiplier *= 1.0 +
      p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE ) +
      p -> talent_lava_flows -> mod_additive( P_CRIT_DAMAGE );

    overload            = new lava_burst_overload_t( player );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new lava_burst_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_spell_t::execute();
    p -> buffs_elemental_mastery_insta -> expire();
    if ( p -> buffs_lava_surge -> check() )
      p -> buffs_lava_surge -> expire();
  }

  virtual timespan_t execute_time() const
  {
    timespan_t t = shaman_spell_t::execute_time();
    shaman_t* p = player -> cast_shaman();

    if ( p -> buffs_elemental_mastery_insta -> up() )
      t *= 1.0 + p -> buffs_elemental_mastery_insta -> mod_additive( P_CAST_TIME );

    if ( p -> buffs_lava_surge -> up() )
      return timespan_t::zero;

    return t;
  }

  virtual void player_buff()
  {
    shaman_spell_t::player_buff();

    shaman_t* p = player -> cast_shaman();

    shaman_targetdata_t* td = targetdata() -> cast_shaman();
    if ( td -> dots_flame_shock -> ticking )
      player_crit += 1.0;

    if ( p -> buffs_unleash_flame -> up() )
      base_multiplier = 1.0 + m_additive + p -> buffs_unleash_flame -> mod_additive( P_GENERIC );
    else
      base_multiplier = 1.0 + m_additive;
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    shaman_t* p = player -> cast_shaman();

    spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      double overload_chance = p -> composite_mastery() * p -> mastery_elemental_overload -> base_value( E_APPLY_AURA, A_DUMMY, 0 );

      if ( overload_chance && p -> rng_elemental_overload -> roll( overload_chance ) )
      {
        overload -> execute();
        if ( p -> set_bonus.tier13_4pc_caster() )
          p -> buffs_tier13_4pc_caster -> trigger();
      }
    }
  }
};

// Lightning Bolt Spell =====================================================

struct lightning_bolt_t : public shaman_spell_t
{
  lightning_bolt_overload_t* overload;

  lightning_bolt_t( player_t* player, const std::string& options_str, bool dtr=false ) :
    shaman_spell_t( "lightning_bolt", "Lightning Bolt", player, options_str )
  {
    shaman_t* p = player -> cast_shaman();

    maelstrom          = true;
    // Shamanism
    direct_power_mod += p -> spec_shamanism -> effect_base_value( 1 ) / 100.0;
    base_execute_time += timespan_t::from_millis(p -> spec_shamanism -> effect_base_value( 3 ));
    // Elemental fury
    crit_bonus_multiplier *= 1.0 + p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );

    // Tier11 4pc Melee bonus
    base_crit            = p -> sets -> set( SET_T11_4PC_MELEE ) -> mod_additive( P_CRIT );

    base_multiplier     *= 1.0 +
      p -> talent_concussion -> mod_additive( P_GENERIC ) +
      p -> glyph_lightning_bolt -> mod_additive( P_GENERIC );

    base_cost_reduction +=
      p -> talent_convection -> mod_additive( P_RESOURCE_COST );

    overload            = new lightning_bolt_overload_t( player );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new lightning_bolt_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void player_buff()
  {
    shaman_t* p = player -> cast_shaman();

    shaman_spell_t::player_buff();

    if ( p -> buffs_maelstrom_weapon -> up() )
      player_multiplier *= 1.0 + p -> sets -> set( SET_T13_2PC_MELEE ) -> effect1().percent();
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    shaman_spell_t::execute();

    p -> buffs_maelstrom_weapon        -> expire();
    p -> buffs_elemental_mastery_insta -> expire();

    p -> cooldowns_elemental_mastery -> ready += timespan_t::from_millis(p -> talent_feedback -> base_value());
    if ( p -> rng_t12_2pc_caster -> roll( p -> sets -> set( SET_T12_2PC_CASTER ) -> proc_chance() ) )
    {
      p -> cooldowns_fire_elemental_totem -> ready -= timespan_t::from_seconds(p -> sets -> set( SET_T12_2PC_CASTER ) -> effect1().base_value());
    }
  }

  virtual timespan_t execute_time() const
  {
    timespan_t t = shaman_spell_t::execute_time();
    shaman_t* p = player -> cast_shaman();
    if ( p -> buffs_elemental_mastery_insta -> up() )
      t *= 1.0 + p -> buffs_elemental_mastery_insta -> mod_additive( P_CAST_TIME );

    // Tier11 4pc Caster Bonus (apply to raw base cast time for now, until verified)
    t *= 1.0 + p -> sets -> set( SET_T11_4PC_CASTER ) -> mod_additive( P_CAST_TIME );

    t *= 1.0 + p -> buffs_maelstrom_weapon -> stack() * p -> buffs_maelstrom_weapon -> mod_additive( P_CAST_TIME );
    return t;
  }

  virtual double cost_reduction() const
  {
    shaman_t* p = player -> cast_shaman();
    double   cr = shaman_spell_t::cost_reduction();

    cr += p -> buffs_maelstrom_weapon -> stack() * p -> buffs_maelstrom_weapon -> mod_additive( P_RESOURCE_COST );

    return cr;
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    shaman_t* p = player -> cast_shaman();

    spell_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      trigger_rolling_thunder( this );

      double overload_chance = p -> composite_mastery() * p -> mastery_elemental_overload -> base_value( E_APPLY_AURA, A_DUMMY, 0 );

      if ( overload_chance && p -> rng_elemental_overload -> roll( overload_chance ) )
      {
        overload -> execute();
        if ( p -> set_bonus.tier13_4pc_caster() )
          p -> buffs_tier13_4pc_caster -> trigger();
      }

      if ( p -> talent_telluric_currents -> rank() )
      {
        p -> resource_gain( RESOURCE_MANA,
                            travel_dmg * p -> talent_telluric_currents -> base_value() / 100.0,
                            p -> gains_telluric_currents );
      }
    }
  }

  virtual bool usable_moving()
  {
    shaman_t* p = player -> cast_shaman();

    if ( p -> glyph_unleashed_lightning -> ok() )
      return true;

    return shaman_spell_t::usable_moving();
  }
};

// Natures Swiftness Spell ==================================================

struct shamans_swiftness_t : public shaman_spell_t
{
  cooldown_t* sub_cooldown;
  dot_t*      sub_dot;

  shamans_swiftness_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "natures_swiftness", "Nature's Swiftness", player, options_str ),
    sub_cooldown( 0 ), sub_dot( 0 )
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
      if ( sub_cooldown -> remains() > timespan_t::zero )
        return false;

    if ( sub_dot )
      if ( sub_dot -> remains() > timespan_t::zero )
        return false;

    return shaman_spell_t::ready();
  }
};

// Shamanisitc Rage Spell ===================================================

struct shamanistic_rage_t : public shaman_spell_t
{
  shamanistic_rage_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "shamanistic_rage", "Shamanistic Rage", player, options_str )
  {
    shaman_t* p = player -> cast_shaman();
    check_talent( p -> talent_shamanistic_rage -> rank() );

    harmful = false;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    shaman_t* p = player -> cast_shaman();

    p -> buffs_shamanistic_rage -> trigger();
  }

  virtual bool ready()
  {
    if ( ! shaman_spell_t::ready() )
      return false;

    return( player -> resource_current[ RESOURCE_MANA ] < ( 0.10 * player -> resource_max[ RESOURCE_MANA ] ) );
  }
};

// Spirit Wolf Spell ========================================================

struct spirit_wolf_spell_t : public shaman_spell_t
{
  spirit_wolf_spell_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "spirit_wolf", "Feral Spirit", player, options_str )
  {
    shaman_t* p = player -> cast_shaman();
    check_talent( p -> talent_feral_spirit -> rank() );

    harmful = false;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    shaman_t* p = player -> cast_shaman();

    p -> pet_spirit_wolf -> summon( duration() );
  }
};

// Thunderstorm Spell =======================================================

struct thunderstorm_t : public shaman_spell_t
{
  double bonus;

  thunderstorm_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "thunderstorm", "Thunderstorm", player, options_str ), bonus( 0 )
  {
    shaman_t* p = player -> cast_shaman();
    check_spec( TREE_ELEMENTAL );

    cooldown -> duration += timespan_t::from_seconds(p -> glyph_thunder -> mod_additive( P_COOLDOWN ));
    bonus                 =
      p -> dbc.spell( id ) -> effect2().percent() +
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

// Unleash Elements Spell ===================================================

struct unleash_elements_t : public shaman_spell_t
{
  unleash_wind_t*   wind;
  unleash_flame_t* flame;

  unleash_elements_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "unleash_elements", "Unleash Elements", player, options_str )
  {
    may_crit    = false;
    may_miss    = false;

    wind        = new unleash_wind_t( player );
    flame       = new unleash_flame_t( player );
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();
    spell_t::execute();

    wind  -> execute();
    flame -> execute();

    // You get the buffs, regardless of hit/miss
    if ( p -> main_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
      p -> buffs_unleash_flame -> trigger();
    else if ( p -> main_hand_weapon.buff_type == WINDFURY_IMBUE )
      p -> buffs_unleash_wind -> trigger( p -> buffs_unleash_wind -> initial_stacks() );

    if ( p -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( p -> off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
        p -> buffs_unleash_flame -> trigger();
      else if ( p -> off_hand_weapon.buff_type == WINDFURY_IMBUE )
        p -> buffs_unleash_wind -> trigger( p -> buffs_unleash_wind -> initial_stacks() );
    }
  }
};

// ==========================================================================
// Shaman Shock Spells
// ==========================================================================

// Earth Shock Spell ========================================================

struct earth_shock_t : public shaman_spell_t
{
  struct lightning_charge_delay_t : public event_t
  {
    buff_t* buff;
    int consume_stacks;
    int consume_threshold;

    lightning_charge_delay_t( sim_t* sim, player_t* p, buff_t* b, int consume, int consume_threshold ) :
      event_t( sim, p ), buff( b ), consume_stacks( consume ), consume_threshold( consume_threshold )
    {
      name = "lightning_charge_delay_t";
      sim -> add_event( this, timespan_t::from_seconds(0.001) );
    }

    void execute()
    {
      if ( ( buff -> check() - consume_threshold ) > 0 )
        buff -> decrement( consume_stacks );
    }
  };

  int consume_threshold;

  earth_shock_t( player_t* player, const std::string& options_str, bool dtr=false ) :
    shaman_spell_t( "earth_shock", "Earth Shock", player, options_str ), consume_threshold( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    base_dd_multiplier   *= 1.0 +
      p -> talent_concussion -> mod_additive( P_GENERIC );

    base_cost_reduction  += p -> talent_convection -> mod_additive( P_RESOURCE_COST );
    crit_bonus_multiplier *= 1.0 + p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );

    cooldown = p -> cooldowns_shock;
    cooldown -> duration = p -> dbc.spell( id ) -> cooldown() +
      timespan_t::from_seconds(p -> talent_reverberation -> mod_additive( P_COOLDOWN ));

    if ( p -> glyph_shocking -> ok() )
    {
      trigger_gcd         = timespan_t::from_seconds(1.0);
      min_gcd             = timespan_t::from_seconds(1.0);
    }

    consume_threshold     = ( int ) p -> talent_fulmination -> base_value();

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new earth_shock_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
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
      if ( consuming_stacks > 0 && ! is_dtr_action )
      {
        p -> active_lightning_charge -> execute();
        if ( ! is_dtr_action ) new ( p -> sim ) lightning_charge_delay_t( p -> sim, p, p -> buffs_lightning_shield, consuming_stacks, consume_threshold );
        //if ( ! is_dtr_action ) p -> buffs_lightning_shield -> decrement( consuming_stacks );
        p -> procs_fulmination[ consuming_stacks ] -> occur();
      }
    }
  }
};

// Flame Shock Spell ========================================================

struct flame_shock_t : public shaman_spell_t
{
  double m_dd_additive,
         m_td_additive;

  flame_shock_t( player_t* player, const std::string& options_str, bool dtr=false ) :
    shaman_spell_t( "flame_shock", "Flame Shock", player, options_str ), m_dd_additive( 0 ), m_td_additive( 0 )
  {
    shaman_t* p = player -> cast_shaman();

    tick_may_crit         = true;
    dot_behavior          = DOT_REFRESH;
    m_dd_additive         =
      p -> talent_concussion -> mod_additive( P_GENERIC );

    m_td_additive         =
      p -> talent_concussion -> mod_additive( P_TICK_DAMAGE ) +
      p -> talent_lava_flows -> mod_additive( P_TICK_DAMAGE );

    base_cost_reduction  *= 1.0 + p -> talent_convection -> mod_additive( P_RESOURCE_COST );

    // Tier11 2pc Caster Bonus
    base_crit             = p -> sets -> set( SET_T11_2PC_CASTER ) -> mod_additive( P_CRIT );

    crit_bonus_multiplier *= 1.0 + p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );

    num_ticks = ( int ) floor( ( ( double ) num_ticks ) * ( 1.0 + p -> glyph_flame_shock -> mod_additive( P_DURATION ) ) );

    cooldown              = p -> cooldowns_shock;
    cooldown -> duration = p -> dbc.spell( id ) -> cooldown() +
      timespan_t::from_seconds(p -> talent_reverberation -> mod_additive( P_COOLDOWN ));

    if ( p -> glyph_shocking -> ok() )
    {
      trigger_gcd         = timespan_t::from_seconds(1.0);
      min_gcd             = timespan_t::from_seconds(1.0);
    }

    may_trigger_dtr = false; // Disable the dot ticks procing DTR

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new flame_shock_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void player_buff()
  {
    shaman_spell_t::player_buff();
    shaman_t* p = player -> cast_shaman();

    // Unleash flame is an additive bonus
    if ( p -> buffs_unleash_flame -> up() )
    {
      base_dd_multiplier = 1.0 + m_dd_additive + p -> buffs_unleash_flame -> mod_additive( P_GENERIC );
      base_td_multiplier = 1.0 + m_td_additive + p -> buffs_unleash_flame -> mod_additive( P_GENERIC );
    }
    else
    {
      base_dd_multiplier = 1.0 + m_dd_additive;
      base_td_multiplier = 1.0 + m_td_additive;
    }
  }

  virtual void tick( dot_t* d )
  {
    shaman_spell_t::tick( d );

    shaman_t* p = player -> cast_shaman();
    if ( p -> rng_lava_surge -> roll ( p -> talent_lava_surge -> proc_chance() ) )
    {
      p -> procs_lava_surge -> occur();
      p -> cooldowns_lava_burst -> reset();
      if ( p -> set_bonus.tier12_4pc_caster() )
        p -> buffs_lava_surge -> trigger();
    }
  }
};

// Frost Shock Spell ========================================================

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
      p -> talent_concussion -> mod_additive( P_GENERIC );

    base_cost_reduction  += p -> talent_convection -> mod_additive( P_RESOURCE_COST );

    crit_bonus_multiplier *= 1.0 + p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );

    cooldown              = p -> cooldowns_shock;
    cooldown -> duration = p -> dbc.spell( id ) -> cooldown() +
      timespan_t::from_seconds(p -> talent_reverberation -> mod_additive( P_COOLDOWN ));

    if ( p -> glyph_shocking -> ok() )
    {
      trigger_gcd         = timespan_t::from_seconds(1.0);
      min_gcd             = timespan_t::from_seconds(1.0);
    }
  }
};

// Wind Shear Spell =========================================================

struct wind_shear_t : public shaman_spell_t
{
  wind_shear_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "wind_shear", "Wind Shear", player, options_str )
  {
    shaman_t* p = player -> cast_shaman();

    may_miss = may_resist = may_crit = false;

    cooldown -> duration += timespan_t::from_seconds(p -> talent_reverberation -> mod_additive( P_COOLDOWN ));
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() ) return false;
    return shaman_spell_t::ready();
  }
};

// ==========================================================================
// Shaman Totem Spells
// ==========================================================================

struct shaman_totem_t : public shaman_spell_t
{
  timespan_t totem_duration;
  double totem_bonus;
  totem_type totem;

  shaman_totem_t( const char * name, const char * totem_name, player_t* player, const std::string& options_str, totem_type t ) :
    shaman_spell_t( name, totem_name, player, options_str ), totem_duration( timespan_t::zero ), totem_bonus( 0 ), totem( t )
  {
    shaman_t* p = player -> cast_shaman();

    is_totem             = true;
    harmful              = false;
    hasted_ticks         = false;
    callbacks            = false;
    base_cost_reduction += p -> talent_totemic_focus -> mod_additive( P_RESOURCE_COST );
    totem_duration       = duration() * ( 1.0 + p -> talent_totemic_focus -> mod_additive( P_DURATION ) );
    // Model all totems as ticking "dots" for now, this will cause them to properly
    // "fade", so we can recast them if the fight length is long enough and optimal_raid=0
    num_ticks            = 1;
    base_tick_time       = totem_duration;
  }

  // Simulate a totem "drop", this is a simplified action_t::execute()
  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();

    if ( p -> totems[ totem ] )
    {
      p -> totems[ totem ] -> cancel();
      p -> totems[ totem ] -> dot() -> cancel();
    }

    if ( sim -> log )
      log_t::output( sim, "%s performs %s", player -> name(), name() );

    result = RESULT_HIT; tick_dmg = 0; direct_dmg = 0;

    consume_resource();
    update_ready();
    schedule_travel( target );

    p -> totems[ totem ] = this;

    stats -> add_execute( time_to_execute );
  }

  virtual void last_tick( dot_t* d )
  {
    shaman_spell_t::last_tick( d );

    shaman_t* p = player -> cast_shaman();
    if ( sim -> log )
      log_t::output( sim, "%s destroys %s", player -> name(), p -> totems[ totem ] -> name() );
    p -> totems[ totem ] = 0;
  }

  virtual void tick( dot_t* d )
  {
    if ( sim -> debug )
      log_t::output( sim, "%s ticks (%d of %d)", name(), d -> current_tick, d -> num_ticks );

    player_buff(); // Totems recalculate stats on every "tick"

    if ( aoe == -1 || aoe > 0 )
    {
      std::vector< player_t* > tl = target_list();
      
      for ( size_t t = 0; t < tl.size(); t++ )
      {
        target_debuff( tl[ t ], DMG_OVER_TIME );
        
        calculate_result();
        
        if ( result_is_hit() )
        {
          direct_dmg = calculate_direct_damage( t );
          
          if ( direct_dmg > 0 )
          {
            tick_dmg = direct_dmg;
            direct_dmg = 0;
            assess_damage( tl[ t ], tick_dmg, DMG_OVER_TIME, result );
          }
        }
        else
        {
          if ( sim -> log )
            log_t::output( sim, "%s avoids %s (%s)", target -> name(), name(), util_t::result_type_string( result ) );
        }
        
      }

      stats -> add_tick( d -> time_to_tick );
    }
    else
    {
      target_debuff( target, DMG_DIRECT );
      calculate_result();

      if ( result_is_hit() )
      {
        direct_dmg = calculate_direct_damage();
        
        if ( direct_dmg > 0 )
        {
          tick_dmg = direct_dmg;
          direct_dmg = 0;
          assess_damage( target, tick_dmg, DMG_OVER_TIME, result );
        }
      }
      else
      {
        if ( sim -> log )
          log_t::output( sim, "%s avoids %s (%s)", target -> name(), name(), util_t::result_type_string( result ) );
      }
      
      stats -> add_tick( d -> time_to_tick );
    }
  }

  virtual timespan_t gcd() const
  {
    if ( harmful )
      return shaman_spell_t::gcd();

    return player -> in_combat ? shaman_spell_t::gcd() : timespan_t::zero;
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();
    if ( p -> totems[ totem ] )
      return false;

    return spell_t::ready();
  }

  virtual void reset()
  {
    shaman_spell_t::reset();

    shaman_t* p = player -> cast_shaman();
    if ( p -> totems[ totem ] == this )
      p -> totems[ totem ] = 0;
  }
};

// Earth Elemental Totem Spell ==============================================

struct earth_elemental_totem_t : public shaman_totem_t
{
  earth_elemental_totem_t( player_t* player, const std::string& options_str ) :
    shaman_totem_t( "earth_elemental_totem", "Earth Elemental Totem", player, options_str, TOTEM_EARTH )
  {
    // Skip a pointless cancel call (and debug=1 cancel line)
    dot_behavior = DOT_REFRESH;
  }

  virtual void execute()
  {
    shaman_totem_t::execute();

    shaman_t* p = player -> cast_shaman();

    p -> pet_earth_elemental -> summon();
  }

  virtual void last_tick( dot_t* d )
  {
    shaman_totem_t::last_tick( d );

    shaman_t* p = player -> cast_shaman();

    p -> pet_earth_elemental -> dismiss();
  }

  // Earth Elemental Totem will always override any earth totem you have
  virtual bool ready()
  {
    return shaman_spell_t::ready();
  }
};

// Fire Elemental Totem Spell ===============================================

struct fire_elemental_totem_t : public shaman_totem_t
{
  fire_elemental_totem_t( player_t* player, const std::string& options_str ) :
    shaman_totem_t( "fire_elemental_totem", "Fire Elemental Totem", player, options_str, TOTEM_FIRE )
  {
    shaman_t* p = player -> cast_shaman();

    cooldown -> duration += timespan_t::from_seconds(p -> glyph_fire_elemental_totem -> mod_additive( P_COOLDOWN ));
    // Skip a pointless cancel call (and debug=1 cancel line)
    dot_behavior = DOT_REFRESH;
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

    p -> pet_fire_elemental -> summon();
  }

  virtual void last_tick( dot_t* d )
  {
    shaman_totem_t::last_tick( d );

    shaman_t* p = player -> cast_shaman();

    p -> pet_fire_elemental -> dismiss();
  }

  // Allow Fire Elemental Totem to override any active fire totems
  virtual bool ready()
  {
    return shaman_spell_t::ready();
  }
};

// Flametongue Totem Spell ==================================================

struct flametongue_totem_t : public shaman_totem_t
{
  flametongue_totem_t( player_t* player, const std::string& options_str ) :
    shaman_totem_t( "flametongue_totem", "Flametongue Totem", player, options_str, TOTEM_FIRE )
  {
    shaman_t* p = player -> cast_shaman();

    if ( p -> talent_totemic_wrath -> rank() > 0 )
      totem_bonus        = p -> talent_totemic_wrath -> base_value() / 100.0;
    // XX: Hardcode this based on tooltip information for now, effect is spell id 52109
    else
      totem_bonus        = p -> dbc.spell( 52109 ) -> effect1().percent();
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
    if ( sim -> auras.flametongue_totem -> check() )
      return false;

    return shaman_totem_t::ready();
  }
};

// Magma Totem Spell ========================================================

struct magma_totem_t : public shaman_totem_t
{
  magma_totem_t( player_t* player, const std::string& options_str ) :
    shaman_totem_t( "magma_totem", "Magma Totem", player, options_str, TOTEM_FIRE )
  {
    const spell_data_t* trigger;
    shaman_t*            p = player -> cast_shaman();

    aoe               = -1;
    harmful           = true;
    may_crit          = true;
    // Magma Totem is not a real DoT, but rather a pet that is spawned.

    // Base multiplier x2 in the effect of call of flame, no real way to discern them except to hardcode it with
    // the effect number
    base_multiplier  *= 1.0 + p -> talent_call_of_flame -> effect1().percent();

    crit_bonus_multiplier *= 1.0 + p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );

    // Spell id 8188 does the triggering of magma totem's aura
    base_tick_time    = p -> dbc.spell( 8188 ) -> effect1().period();
    num_ticks         = ( int ) ( totem_duration / base_tick_time );

    // Fill out scaling data
    trigger           = p -> dbc.spell( p -> dbc.spell( 8188 ) -> effect1().trigger_spell_id() );
    // Also kludge totem school to fire for accurate damage
    school            = spell_id_t::get_school_type( trigger -> school_mask() );
    stats -> school   = school;

    base_dd_min       = p -> dbc.effect_min( trigger -> effect1().id(), p -> level );
    base_dd_max       = p -> dbc.effect_max( trigger -> effect1().id(), p -> level );
    direct_power_mod  = trigger -> effect1().coeff();
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();

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
};

// Mana Spring Totem Spell ==================================================

struct mana_spring_totem_t : public shaman_totem_t
{
  mana_spring_totem_t( player_t* player, const std::string& options_str ) :
    shaman_totem_t( "mana_spring_totem", "Mana Spring Totem", player, options_str, TOTEM_WATER )
  {
    // Mana spring effect is at spell id 5677. Get scaling information from there.
    totem_bonus  = player -> dbc.effect_average( player -> dbc.spell( 5677 ) -> effect1().id(), player -> level );
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
    if ( sim -> overrides.mana_spring_totem == 1 ||
         sim -> auras.mana_spring_totem -> current_value >= totem_bonus )
      return false;

    return shaman_totem_t::ready();
  }
};

// Mana Tide Totem Spell ====================================================

struct mana_tide_totem_t : public shaman_totem_t
{
  mana_tide_totem_t( player_t* player, const std::string& options_str ) :
    shaman_totem_t( "mana_tide_totem", "Mana Tide Totem", player, options_str, TOTEM_WATER )
  {
    shaman_t* p = player -> cast_shaman();
    check_talent( p -> talent_mana_tide_totem -> rank() );

    // Mana tide effect bonus is in a separate spell, we dont need other info
    // from there anymore, as mana tide does not pulse anymore
    totem_bonus  = p -> dbc.spell( 16191 ) -> effect1().percent();
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

// Searing Totem Spell ======================================================

struct searing_totem_t : public shaman_totem_t
{
  searing_totem_t( player_t* player, const std::string& options_str ) :
    shaman_totem_t( "searing_totem", "Searing Totem", player, options_str, TOTEM_FIRE )
  {
    shaman_t* p = player -> cast_shaman();

    harmful   = true;
    may_crit  = true;

    // Base multiplier has to be applied like this, because the talent has two identical effects
    base_multiplier     *= 1.0 + p -> talent_call_of_flame -> effect1().percent();

    crit_bonus_multiplier *= 1.0 + p -> spec_elemental_fury -> mod_additive( P_CRIT_DAMAGE );

    // Scaling information is found in Searing Bolt (3606)
    // Tooltip and spell data is wrong for 4.0.4, the base_dd_min/max are higher in beta than in
    // spell data
    if ( ! p -> bugs )
    {
      base_dd_min        = p -> dbc.effect_min( p -> dbc.spell( 3606 ) -> effect1().id(), p -> level );
      base_dd_max        = p -> dbc.effect_max( p -> dbc.spell( 3606 ) -> effect1().id(), p -> level );
    }
    else
    {
      base_dd_min        = 92;
      base_dd_max        = 120;
    }
    direct_power_mod     = p -> dbc.spell( 3606 ) -> effect1().coeff();
    // Note, searing totem tick time should come from the searing totem's casting time (1.50 sec),
    // except it's in-game cast time is ~1.6sec
    // base_tick_time       = p -> player_data.spell_cast_time( 3606, p -> level );
    base_tick_time       = timespan_t::from_seconds(1.6);
    travel_speed         = 0; // TODO: Searing bolt has a real travel time, however modeling it is another issue entirely
    range                = p -> dbc.spell( 3606 ) -> max_range();
    num_ticks            = ( int ) ( totem_duration / base_tick_time );
    // Also kludge totem school to fire
    school               = spell_id_t::get_school_type( p -> dbc.spell( 3606 ) -> school_mask() );
    stats -> school      = SCHOOL_FIRE;
    p -> active_searing_flames_dot = new searing_flames_t( p );
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
  }

  virtual double total_spell_power() const
  {
    if ( ! player -> bugs )
      return shaman_totem_t::total_spell_power();
    // Searing totem is double dipping into Wrathful Totems,
    else
    {
      shaman_t* p = player -> cast_shaman();
      double sp   = shaman_totem_t::total_spell_power();
      sp         *= 1.0 + p -> talent_totemic_wrath -> base_value() / 100.0;

      return floor( sp );
    }
  }

  virtual void tick( dot_t* d )
  {
    shaman_t* p = player -> cast_shaman();
    shaman_targetdata_t* td = targetdata() -> cast_shaman();
    shaman_totem_t::tick( d );
    if ( result_is_hit() && td -> debuffs_searing_flames -> trigger() )
    {
      double new_base_td = tick_dmg;
      // Searing flame dot treats all hits as.. HITS
      if ( result == RESULT_CRIT )
        new_base_td /= 1.0 + total_crit_bonus();

      p -> active_searing_flames_dot -> base_td = new_base_td / p -> active_searing_flames_dot -> num_ticks;
      p -> active_searing_flames_dot -> execute();
    }
  }
};

// Strength of Earth Totem Spell ============================================

struct strength_of_earth_totem_t : public shaman_totem_t
{
  strength_of_earth_totem_t( player_t* player, const std::string& options_str ) :
    shaman_totem_t( "strength_of_earth_totem", "Strength of Earth Totem", player, options_str, TOTEM_EARTH )
  {
    // We can use either A_MOD_STAT effect, as they both apply the same amount of stat
    totem_bonus  = player -> dbc.effect_average( player -> dbc.spell( 8076 ) -> effect1().id(), player -> level );
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
    if ( sim -> overrides.strength_of_earth == 1 || sim -> auras.strength_of_earth -> check () )
      return false;

    return shaman_totem_t::ready();
  }
};

// Windfury Totem Spell =====================================================

struct windfury_totem_t : public shaman_totem_t
{
  windfury_totem_t( player_t* player, const std::string& options_str ) :
    shaman_totem_t( "windfury_totem", "Windfury Totem", player, options_str, TOTEM_AIR )
  {
    totem_bonus  = player -> dbc.spell( 8515 ) -> effect1().percent();
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
    if ( sim -> overrides.windfury_totem == 1 || sim -> auras.windfury_totem -> check() )
      return false;

    return shaman_totem_t::ready();
  }
};

// Wrath of Air Totem Spell =================================================

struct wrath_of_air_totem_t : public shaman_totem_t
{
  wrath_of_air_totem_t( player_t* player, const std::string& options_str ) :
    shaman_totem_t( "wrath_of_air_totem", "Wrath of Air Totem", player, options_str, TOTEM_AIR )
  {
    totem_bonus  = player -> dbc.spell( 2895 ) -> effect1().percent();
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
    if ( sim -> overrides.wrath_of_air == 1 || sim -> auras.wrath_of_air -> check() )
      return false;

    return shaman_totem_t::ready();
  }
};

// ==========================================================================
// Shaman Weapon Imbues
// ==========================================================================

// Flametongue Weapon Spell =================================================

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
    bonus_power  = p -> dbc.effect_average( p -> dbc.spell( 10400 ) -> effect2().id(), p -> level );
    bonus_power /= 100.0;
    bonus_power *= 1.0 + p -> talent_elemental_weapons -> effect1().percent();
    harmful      = false;
    may_miss     = false;

    if ( main )
      p -> flametongue_mh = new flametongue_weapon_spell_t( p, "flametongue_mh" );

    if ( off )
      p -> flametongue_oh = new flametongue_weapon_spell_t( p, "flametongue_oh" );
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

  virtual timespan_t gcd() const
  {
    return player -> in_combat ? shaman_spell_t::gcd() : timespan_t::zero;
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

    bonus_power  = p -> dbc.effect_average( p -> dbc.spell( id ) -> effect2().id(), p -> level );
    harmful      = false;
    may_miss     = false;

    if ( main )
      p -> windfury_mh = new windfury_weapon_attack_t( p, "windfury_mh", &( p -> main_hand_weapon ) );

    if ( off )
      p -> windfury_oh = new windfury_weapon_attack_t( p, "windfury_oh", &( p -> off_hand_weapon ) );
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

  virtual timespan_t gcd() const
  {
    return player -> in_combat ? shaman_spell_t::gcd() : timespan_t::zero;
  }
};

// ==========================================================================
// Shaman Shields
// ==========================================================================

// Lightning Shield Spell ===================================================

struct lightning_shield_t : public shaman_spell_t
{
  lightning_shield_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "lightning_shield", "Lightning Shield", player )
  {
    shaman_t* p = player -> cast_shaman();
    harmful     = false;

    p -> active_lightning_charge = new lightning_charge_t( p, p -> primary_tree() == TREE_ELEMENTAL ? "fulmination" : "lightning_shield" );

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    shaman_t* p = player -> cast_shaman();
    p -> buffs_water_shield     -> expire();
    p -> buffs_lightning_shield -> trigger( initial_stacks() );
  }

  virtual bool ready()
  {
    shaman_t* p = player -> cast_shaman();

    if ( p -> buffs_lightning_shield -> check() )
      return false;

    return shaman_spell_t::ready();
  }

  virtual timespan_t gcd() const
  {
    return player -> in_combat ? shaman_spell_t::gcd() : timespan_t::zero;
  }
};

// Water Shield Spell =======================================================

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
    bonus        = p -> dbc.effect_average( p -> dbc.spell( id ) -> effect2().id(), p -> level );
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

  virtual timespan_t gcd() const
  {
    return player -> in_combat ? shaman_spell_t::gcd() : timespan_t::zero;
  }
};

struct spiritwalkers_grace_t : public shaman_spell_t
{
  spiritwalkers_grace_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "spiritwalkers_grace", "Spiritwalker's Grace", player )
  {
    may_miss  = may_crit = harmful = callbacks = false;

    option_t options[] =
    {
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );
    
    // Extend buff duration with t13 4pc healer
    shaman_t* p = player -> cast_shaman();
    p -> buffs_spiritwalkers_grace -> buff_duration = p -> buffs_spiritwalkers_grace -> duration() + 
                                                      p -> sets -> set( SET_T13_4PC_HEAL ) -> effect1().time_value();
  }

  virtual void execute()
  {
    shaman_t* p = player -> cast_shaman();

    shaman_spell_t::execute();

    p -> buffs_spiritwalkers_grace -> trigger();
    if ( p -> set_bonus.tier13_4pc_heal() )
      p -> buffs_tier13_4pc_healer -> trigger();
  }
};

// ==========================================================================
// Shaman Passive Buffs
// ==========================================================================

struct elemental_devastation_t : public buff_t
{
  elemental_devastation_t( player_t*   p,
                           uint32_t    id,
                           const char* n ) :
    buff_t( p, id, n )
  {
    // Duration has to be parsed out from the triggered spell
    const spell_data_t* trigger = p -> dbc.spell( p -> dbc.spell( id ) -> effect1().trigger_spell_id() );
    buff_duration = trigger -> duration();

    // And fix atomic, as it's a triggered spell, but not really .. sigh
    s_single = s_effects[ 0 ];
  }

  virtual double base_value( effect_type_t type, effect_subtype_t sub_type, int misc_value, int misc_value2 ) const
  {
    return buff_t::base_value( type, sub_type, misc_value, misc_value2 ) / 100.0;
  }
};

struct lightning_shield_buff_t : public buff_t
{
  lightning_shield_buff_t( player_t*   p,
                           uint32_t    id,
                           const char* n ) :
    buff_t( p, id, n, 1.0, timespan_t::min, false )
  {
    shaman_t* s = player -> cast_shaman();

    // This requires rolling thunder checking for max stack
    if ( s -> talent_rolling_thunder -> rank() > 0 )
      max_stack = ( int ) s -> talent_rolling_thunder -> base_value( E_APPLY_AURA, A_PROC_TRIGGER_SPELL );

    // Reinit because of max_stack change
    init_buff_shared();
  }
};

struct searing_flames_buff_t : public buff_t
{
  searing_flames_buff_t( actor_pair_t pair,
                         uint32_t    id,
                         const char* n ) :
    buff_t( pair, id, n, 1.0, timespan_t::min, true ) // Quiet buff, dont show in report
  {
    // The default chance is in the script dummy effect base value
    default_chance     = initial_source -> dbc.spell( id ) -> effect1().percent();

    // Various other things are specified in the actual debuff placed on the target
    buff_duration      = initial_source -> dbc.spell( 77661 ) -> duration();
    max_stack          = initial_source -> dbc.spell( 77661 ) -> max_stacks();

    // Reinit because of max_stack change
    init_buff_shared();
  }
};

struct unleash_elements_buff_t : public buff_t
{
  double bonus;

  unleash_elements_buff_t( player_t*   p,
                           uint32_t    id,
                           const char* n ) :
    buff_t( p, id, n, 1.0, timespan_t::min, false ), bonus( 0.0 )
  {
    shaman_t* s = player -> cast_shaman();
    bonus       = s -> talent_elemental_weapons -> effect2().percent();
  }

  virtual double mod_additive( property_type_t p_type ) const
  {
    return buff_t::mod_additive( p_type ) * ( 1.0 + bonus );
  }

  virtual double base_value( effect_type_t type, effect_subtype_t sub_type, int misc_value, int misc_value2 ) const
  {
    return buff_t::base_value( type, sub_type, misc_value, misc_value2 ) * ( 1.0 + bonus );
  }
};

struct unleash_flame_buff_t : public unleash_elements_buff_t
{
  event_t* expiration_delay;

  unleash_flame_buff_t( player_t* p );

  void reset();
  void expire();
};

struct unleash_flame_expiration_delay_t : public event_t
{
  unleash_flame_buff_t* buff;

  unleash_flame_expiration_delay_t( sim_t* sim, player_t* p, unleash_flame_buff_t* b ) :
    event_t( sim, p ), buff( b )
  {
    shaman_t* s = player -> cast_shaman();
    name = "unleash_flame_expiration_delay";
    sim -> add_event( this, sim -> gauss( s -> uf_expiration_delay, s -> uf_expiration_delay_stddev ) );
  }

  virtual void execute()
  {
    // Call real expire after a delay
    buff -> unleash_elements_buff_t::expire();
    buff -> expiration_delay = 0;
  }
};

unleash_flame_buff_t::unleash_flame_buff_t( player_t* p ) :
  unleash_elements_buff_t( p, 73683, "unleash_flame" ), expiration_delay( 0 )
{
}

void
unleash_flame_buff_t::reset()
{
  unleash_elements_buff_t::reset();
  event_t::cancel( expiration_delay );
}

void
unleash_flame_buff_t::expire()
{
  // Active player's Unleash Flame buff has a short aura expiration delay, which allows
  // "Double Casting" with a single buff
  if ( ! player -> sleeping )
  {
    if ( current_stack <= 0 ) return;
    if ( expiration_delay ) return;
    expiration_delay = new ( sim ) unleash_flame_expiration_delay_t( sim, player, this );
  }
  // If the actor is sleeping, make sure the existing buff behavior works (i.e., a call to
  // expire) and additionally, make _absolutely sure_ that any pending expiration delay
  // is canceled
  else
  {
    unleash_elements_buff_t::expire();
    event_t::cancel( expiration_delay );
  }
}

} // ANONYMOUS NAMESPACE ===================================================

// ==========================================================================
// Shaman Character Definition
// ==========================================================================

// shaman_t::create_options =================================================

void shaman_t::create_options()
{
  player_t::create_options();

  option_t shaman_options[] =
  {
    { "wf_delay",                   OPT_TIMESPAN,     &( wf_delay                   ) },
    { "wf_delay_stddev",            OPT_TIMESPAN,     &( wf_delay_stddev            ) },
    { "uf_expiration_delay",        OPT_TIMESPAN,     &( uf_expiration_delay        ) },
    { "uf_expiration_delay_stddev", OPT_TIMESPAN,     &( uf_expiration_delay_stddev ) },
    { NULL,                         OPT_UNKNOWN, NULL                            }
  };

  option_t::copy( options, shaman_options );

}

// shaman_t::create_action  =================================================

action_t* shaman_t::create_action( const std::string& name,
                                   const std::string& options_str )
{
  if ( name == "auto_attack"             ) return new              auto_attack_t( this, options_str );
  if ( name == "bloodlust"               ) return new                bloodlust_t( this, options_str );
  if ( name == "chain_lightning"         ) return new          chain_lightning_t( this, options_str );
  if ( name == "earth_elemental_totem"   ) return new    earth_elemental_totem_t( this, options_str );
  if ( name == "earth_shock"             ) return new              earth_shock_t( this, options_str );
  if ( name == "earthquake"              ) return new               earthquake_t( this, options_str );
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
  if ( name == "spiritwalkers_grace"     ) return new      spiritwalkers_grace_t( this, options_str );
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

// shaman_t::create_pet =====================================================

pet_t* shaman_t::create_pet( const std::string& pet_name,
                             const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "spirit_wolf"     ) return new spirit_wolf_pet_t    ( sim, this );
  if ( pet_name == "fire_elemental"  ) return new fire_elemental_pet_t ( sim, this );
  if ( pet_name == "earth_elemental" ) return new earth_elemental_pet_t( sim, this );

  return 0;
}

// shaman_t::create_pets ====================================================

void shaman_t::create_pets()
{
  pet_spirit_wolf     = create_pet( "spirit_wolf"     );
  pet_fire_elemental  = create_pet( "fire_elemental"  );
  pet_earth_elemental = create_pet( "earth_elemental" );
}

// shaman_t::init_talents ===================================================

void shaman_t::init_talents()
{
  // Talents

  // Elemental
  talent_acuity                   = find_talent( "Acuity" );
  talent_call_of_flame            = find_talent( "Call of Flame" );
  talent_concussion               = find_talent( "Concussion" );
  talent_convection               = find_talent( "Convection" );
  talent_elemental_focus          = find_talent( "Elemental Focus" );
  talent_elemental_mastery        = find_talent( "Elemental Mastery" );
  talent_elemental_oath           = find_talent( "Elemental Oath" );
  talent_elemental_precision      = find_talent( "Elemental Precision" );
  talent_feedback                 = find_talent( "Feedback" );
  talent_fulmination              = find_talent( "Fulmination" );
  talent_improved_fire_nova       = 0;
  talent_lava_flows               = find_talent( "Lava Flows" );
  talent_lava_surge               = find_talent( "Lava Surge" );
  talent_reverberation            = find_talent( "Reverberation" );
  talent_rolling_thunder          = find_talent( "Rolling Thunder" );
  talent_totemic_wrath            = find_talent( "Totemic Wrath" );

  // Enhancement
  talent_elemental_devastation    = find_talent( "Elemental Devastation" );
  talent_elemental_weapons        = find_talent( "Elemental Weapons" );
  talent_feral_spirit             = find_talent( "Feral Spirit" );
  talent_flurry                   = find_talent( "Flurry" );
  talent_focused_strikes          = find_talent( "Focused Strikes" );
  talent_frozen_power             = find_talent( "Frozen Power" );
  talent_improved_lava_lash       = find_talent( "Improved Lava Lash" );
  talent_improved_shields         = find_talent( "Improved Shields" );
  talent_maelstrom_weapon         = find_talent( "Maelstrom Weapon" );
  talent_searing_flames           = find_talent( "Searing Flames" );
  talent_shamanistic_rage         = find_talent( "Shamanistic Rage" );
  talent_static_shock             = find_talent( "Static Shock" );
  talent_stormstrike              = find_talent( "Stormstrike" );
  talent_toughness                = find_talent( "Toughness" );
  talent_unleashed_rage           = find_talent( "Unleashed Rage" );

  // Restoration
  talent_mana_tide_totem          = find_talent( "Mana Tide Totem" );
  talent_natures_swiftness        = find_talent( "Nature's Swiftness" );
  talent_telluric_currents        = find_talent( "Telluric Currents" );
  talent_totemic_focus            = find_talent( "Totemic Focus" );

  player_t::init_talents();
}

// shaman_t::init_spells ====================================================

void shaman_t::init_spells()
{
  // New set bonus system
  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //   C2P     C4P     M2P     M4P    T2P    T4P     H2P     H4P
    {  90503,  90505,  90501,  90502,     0,     0,      0,      0 }, // Tier11
    {  99204,  99206,  99209,  99213,     0,     0,  99190,  99195 }, // Tier12
    { 105780, 105816, 105866, 105872,     0,     0, 105764, 105876 }, // Tier13
    {      0,      0,      0,      0,     0,     0,      0,      0 },
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
  glyph_feral_spirit          = find_glyph( "Glyph of Feral Spirit" );
  glyph_fire_elemental_totem  = find_glyph( "Glyph of Fire Elemental Totem" );
  glyph_flame_shock           = find_glyph( "Glyph of Flame Shock" );
  glyph_flametongue_weapon    = find_glyph( "Glyph of Flametongue Weapon" );
  glyph_lava_burst            = find_glyph( "Glyph of Lava Burst" );
  glyph_lava_lash             = find_glyph( "Glyph of Lava Lash" );
  glyph_lightning_bolt        = find_glyph( "Glyph of Lightning Bolt" );
  glyph_stormstrike           = find_glyph( "Glyph of Stormstrike" );
  glyph_unleashed_lightning = find_glyph( "Glyph of Unleashed Lightning" );
  glyph_water_shield          = find_glyph( "Glyph of Water Shield" );
  glyph_windfury_weapon       = find_glyph( "Glyph of Windfury Weapon" );

  glyph_chain_lightning       = find_glyph( "Glyph of Chain Lightning" );
  glyph_shocking              = find_glyph( "Glyph of Shocking" );
  glyph_thunder               = find_glyph( "Glyph of Thunder" );

  glyph_thunderstorm          = find_glyph( "Glyph of Thunderstorm" );

  sets                        = new set_bonus_array_t( this, set_bonuses );
}

// shaman_t::init_base ======================================================

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

  mana_per_intellect = 15;

  base_spell_crit  += talent_acuity -> base_value();
  base_attack_crit += talent_acuity -> base_value();

  distance = ( primary_tree() == TREE_ENHANCEMENT ) ? 3 : 30;
  default_distance = distance;

  diminished_kfactor    = 0.009880;
  diminished_dodge_capi = 0.006870;
  diminished_parry_capi = 0.006870;
}

// shaman_t::init_scaling ===================================================

void shaman_t::init_scaling()
{
  player_t::init_scaling();

  if ( primary_tree() == TREE_ENHANCEMENT )
  {
    scales_with[ STAT_WEAPON_OFFHAND_DPS    ] = 1;
    scales_with[ STAT_WEAPON_OFFHAND_SPEED  ] = sim -> weapon_speed_scale_factors;
    scales_with[ STAT_HIT_RATING2           ] = 1;
    scales_with[ STAT_SPIRIT                ] = 0;
    scales_with[ STAT_SPELL_POWER           ] = 0;
    scales_with[ STAT_INTELLECT             ] = 0;
  }

  // Elemental Precision treats Spirit like Spell Hit Rating, no need to calculte for Enha though
  if ( talent_elemental_precision -> rank() && sim -> scaling -> scale_stat == STAT_SPIRIT )
  {
    double v = sim -> scaling -> scale_value;
    if ( ! sim -> scaling -> positive_scale_delta )
    {
      invert_scaling = 1;
      attribute_initial[ ATTR_SPIRIT ] -= v * 2;
    }
  }
}

// shaman_t::init_buffs =====================================================

void shaman_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, rng_type=RNG_CYCLIC, activated=true )

  buffs_earth_elemental         = new buff_t                 ( this, "earth_elemental", 1 );
  buffs_elemental_devastation   = new elemental_devastation_t( this, talent_elemental_devastation -> spell_id(),               "elemental_devastation" ); buffs_elemental_devastation -> activated = false;
  buffs_elemental_focus         = new buff_t                 ( this, talent_elemental_focus -> effect_trigger_spell( 1 ),      "elemental_focus"       ); buffs_elemental_focus -> activated = false;
  // For now, elemental mastery will need 2 buffs, 1 to trigger the insta cast, and a second for the haste/damage buff
  buffs_elemental_mastery_insta = new buff_t                 ( this, talent_elemental_mastery -> spell_id(),                   "elemental_mastery_instant", 1.0, timespan_t::min, true );
  // Note the chance override, as the spell itself does not have a proc chance
  buffs_elemental_mastery       = new buff_t                 ( this, talent_elemental_mastery -> effect_trigger_spell( 2 ),    "elemental_mastery",         1.0 );
  buffs_fire_elemental          = new buff_t                 ( this, "fire_elemental", 1 );
  buffs_flurry                  = new buff_t                 ( this, talent_flurry -> effect_trigger_spell( 1 ),               "flurry",                    talent_flurry -> proc_chance() ); buffs_flurry -> activated = false;
  // TBD how this is handled for reals
  buffs_lava_surge              = new buff_t                 ( this, 77762,                                                    "lava_surge",                1.0, timespan_t::min, true ); buffs_lava_surge -> activated = false;
  buffs_lightning_shield        = new lightning_shield_buff_t( this, dbc.class_ability_id( type, "Lightning Shield" ),         "lightning_shield"      );
  buffs_maelstrom_weapon        = new buff_t                 ( this, talent_maelstrom_weapon -> effect_trigger_spell( 1 ),     "maelstrom_weapon"      ); buffs_maelstrom_weapon -> activated = false;
  buffs_natures_swiftness       = new buff_t                 ( this, talent_natures_swiftness -> spell_id(),                   "natures_swiftness"     );
  buffs_shamanistic_rage        = new buff_t                 ( this, talent_shamanistic_rage -> spell_id(),                    "shamanistic_rage"      );
  buffs_spiritwalkers_grace     = new buff_t                 ( this, 79206,                                                    "spiritwalkers_grace",       1.0, timespan_t::zero );
  // Enhancement T12 4Piece Bonus
  buffs_stormfire               = new buff_t                 ( this, 99212,                                                    "stormfire"             );
  buffs_stormstrike             = new buff_t                 ( this, talent_stormstrike -> spell_id(),                         "stormstrike"           );
  //buffs_unleash_flame           = new unleash_elements_buff_t( this, 73683,                                                    "unleash_flame" );
  buffs_unleash_flame           = new unleash_flame_buff_t   ( this );
  buffs_unleash_wind            = new unleash_elements_buff_t( this, 73681,                                                    "unleash_wind"          );
  buffs_water_shield            = new buff_t                 ( this, dbc.class_ability_id( type, "Water Shield" ),             "water_shield"          );

  // Elemental Tier13 set bonuses
  buffs_tier13_2pc_caster       = new stat_buff_t            ( this, 105779, "tier13_2pc_caster", STAT_MASTERY_RATING, dbc.spell( 105779 ) -> effect1().base_value() );
  buffs_tier13_4pc_caster       = new stat_buff_t            ( this, 105821, "tier13_4pc_caster", STAT_HASTE_RATING,   dbc.spell( 105821 ) -> effect1().base_value() );
  buffs_tier13_4pc_healer       = new buff_t                 ( this, 105877, "tier13_4pc_healer" );
}

// shaman_t::init_values ====================================================

void shaman_t::init_values()
{
  player_t::init_values();

  if ( set_bonus.pvp_2pc_caster() )
    attribute_initial[ ATTR_INTELLECT ] += 70;

  if ( set_bonus.pvp_4pc_caster() )
    attribute_initial[ ATTR_INTELLECT ] += 90;

  if ( set_bonus.pvp_2pc_heal() )
    attribute_initial[ ATTR_INTELLECT ] += 70;

  if ( set_bonus.pvp_4pc_heal() )
    attribute_initial[ ATTR_INTELLECT ] += 90;

  if ( set_bonus.pvp_2pc_melee() )
    attribute_initial[ ATTR_AGILITY ]   += 70;

  if ( set_bonus.pvp_4pc_melee() )
    attribute_initial[ ATTR_AGILITY ]   += 90;
}

// shaman_t::init_gains =====================================================

void shaman_t::init_gains()
{
  player_t::init_gains();

  gains_primal_wisdom        = get_gain( "primal_wisdom"     );
  gains_rolling_thunder      = get_gain( "rolling_thunder"   );
  gains_telluric_currents    = get_gain( "telluric_currents" );
  gains_thunderstorm         = get_gain( "thunderstorm"      );
  gains_water_shield         = get_gain( "water_shield"      );
}

// shaman_t::init_procs =====================================================

void shaman_t::init_procs()
{
  player_t::init_procs();

  procs_elemental_overload = get_proc( "elemental_overload"      );
  procs_ft_icd             = get_proc( "flametongue_icd"         );
  procs_lava_surge         = get_proc( "lava_surge"              );
  procs_maelstrom_weapon   = get_proc( "maelstrom_weapon"        );
  procs_static_shock       = get_proc( "static_shock"            );
  procs_swings_clipped_mh  = get_proc( "swings_clipped_mh"       );
  procs_swings_clipped_oh  = get_proc( "swings_clipped_oh"       );
  procs_rolling_thunder    = get_proc( "rolling_thunder"         );
  procs_wasted_ls          = get_proc( "wasted_lightning_shield" );
  procs_wasted_mw          = get_proc( "wasted_maelstrom_weapon" );
  procs_windfury           = get_proc( "windfury"                );

  for ( int i = 0; i < 7; i++ )
  {
    procs_fulmination[ i ] = get_proc( "fulmination_" + util_t::to_string( i ) );
  }

  for ( int i = 0; i < 6; i++ )
  {
    procs_maelstrom_weapon_used[ i ] = get_proc( "maelstrom_weapon_stack_" + util_t::to_string( i ) );
  }
}

// shaman_t::init_rng =======================================================

void shaman_t::init_rng()
{
  player_t::init_rng();
  rng_elemental_overload   = get_rng( "elemental_overload"   );
  rng_lava_surge           = get_rng( "lava_surge"           );
  rng_primal_wisdom        = get_rng( "primal_wisdom"        );
  rng_rolling_thunder      = get_rng( "rolling_thunder"      );
  rng_searing_flames       = get_rng( "searing_flames"       );
  rng_static_shock         = get_rng( "static_shock"         );
  rng_t12_2pc_caster       = get_rng( "t12_2pc_caster"       );
  rng_windfury_delay       = get_rng( "windfury_delay"       );
  rng_windfury_weapon      = get_rng( "windfury_weapon"      );
}

// shaman_t::init_actions ===================================================

void shaman_t::init_actions()
{
  if ( primary_tree() == TREE_ENHANCEMENT && main_hand_weapon.type == WEAPON_NONE )
  {
    if ( ! quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  // Restoration isn't supported atm
  if ( primary_tree() == TREE_RESTORATION && primary_role() == ROLE_HEAL )
  {
    if ( ! quiet )
      sim -> errorf( "Restoration Shaman support for player %s is not currently supported.", name() );

    quiet = true;
    return;
  }

  bool has_power_torrent      = false;
  bool has_dmc_volcano        = false;
  bool has_lightweave         = false;
  bool has_fiery_quintessence = false;
  bool has_will_of_unbinding  = false;
  bool has_bottled_wishes     = false;

  // Detect some stuff so we can figure out how much int should be used to summon FE
  for ( int i = 0; i < SLOT_MAX; i++ )
  {
    if ( util_t::str_compare_ci( items[ i ].name(), "darkmoon_card_volcano" ) )
      has_dmc_volcano = true;
    else if ( util_t::str_compare_ci( items[ i ].name(), "fiery_quintessence" ) )
      has_fiery_quintessence = true;
    else if ( util_t::str_compare_ci( items[ i ].name(), "will_of_unbinding" ) )
      has_will_of_unbinding = true;
    else if ( util_t::str_compare_ci( items[ i ].name(), "bottled_wishes" ) )
      has_bottled_wishes = true;
    else if ( util_t::str_compare_ci( items[ i ].encoded_enchant_str, "power_torrent" ) )
      has_power_torrent = true;
    else if ( util_t::str_compare_ci( items[ i ].encoded_enchant_str, "lightweave_embroidery" ) )
      has_lightweave = true;
  }

  if ( action_list_str.empty() )
  {
    if ( primary_tree() == TREE_ENHANCEMENT )
    {
      action_list_str  = "flask,type=winds/food,type=seafood_magnifique_feast";

      action_list_str +="/windfury_weapon,weapon=main";
      if ( off_hand_weapon.type != WEAPON_NONE )
        action_list_str += "/flametongue_weapon,weapon=off";
      action_list_str += "/strength_of_earth_totem/windfury_totem/mana_spring_totem/lightning_shield";
      action_list_str += "/tolvir_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=40";
      action_list_str += "/snapshot_stats";
      action_list_str += "/auto_attack";
      action_list_str += "/wind_shear";
      action_list_str += "/bloodlust,health_percentage<=25/bloodlust,if=target.time_to_die<=60";
      int num_items = ( int ) items.size();
      for ( int i=0; i < num_items; i++ )
      {
        if ( items[ i ].use.active() )
        {
          action_list_str += "/use_item,name=";
          action_list_str += items[ i ].name();
        }
      }
      action_list_str += init_use_profession_actions();
      action_list_str += init_use_racial_actions();

      if ( level <= 80 )
        action_list_str += "/fire_elemental_totem,if=!ticking";

      action_list_str += "/searing_totem";
      if ( talent_stormstrike -> rank() ) action_list_str += "/stormstrike";
      action_list_str += "/lava_lash";
      action_list_str += "/lightning_bolt,if=buff.maelstrom_weapon.react=5|(set_bonus.tier13_4pc_melee=1&buff.maelstrom_weapon.react>=4&pet.spirit_wolf.active)";
      if ( level > 80 )
      {
        action_list_str += "/unleash_elements";
      }
      action_list_str += "/flame_shock,if=!ticking|buff.unleash_flame.up";
      action_list_str += "/earth_shock";
      if ( talent_feral_spirit -> rank() ) action_list_str += "/spirit_wolf";
      action_list_str += "/earth_elemental_totem";
      action_list_str += "/fire_nova,if=target.adds>1";
      action_list_str += "/spiritwalkers_grace,moving=1";
      action_list_str += "/lightning_bolt,if=buff.maelstrom_weapon.react>1";
    }
    else
    {
      int sp_threshold = 0;
      if ( set_bonus.tier12_2pc_caster() )
        sp_threshold = ( ( has_power_torrent || has_lightweave ) ? ( ( has_dmc_volcano ) ? 1600 : 500 ) : 0 ) + (has_will_of_unbinding?1:0) * 700;
      else
        sp_threshold = (( has_power_torrent || has_lightweave )?1:0) * 500 + (has_dmc_volcano?1:0) * 1600 + (has_will_of_unbinding?1:0) * 700;

      action_list_str  = "flask,type=draconic_mind/food,type=seafood_magnifique_feast";

      action_list_str += "/flametongue_weapon,weapon=main/lightning_shield";
      action_list_str += "/mana_spring_totem/wrath_of_air_totem";
      action_list_str += "/snapshot_stats";
      action_list_str += "/volcanic_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=40";

      action_list_str += "/wind_shear";
      action_list_str += "/bloodlust,health_percentage<=25/bloodlust,if=target.time_to_die<=60";
      int num_items = ( int ) items.size();
      for ( int i=0; i < num_items; i++ )
      {
        if ( ! items[ i ].use.active() ) continue;

        // Fiery Quintessence / Bottled Wishes are aligned to fire elemental and
        // only used when the required temporary int threshold is exceeded
        if ( util_t::str_compare_ci( items[ i ].name(), "fiery_quintessence" ) ||
             util_t::str_compare_ci( items[ i ].name(), "bottled_wishes" ) )
        {
          action_list_str += "/use_item,name=" +
                             std::string( items[ i ].name() ) +
                             ",if=(cooldown.fire_elemental_totem.remains" +
                             ( ( set_bonus.tier12_2pc_caster() ) ? "<25" : "=0" ) +
                             "&temporary_bonus.spell_power>=" +
                             util_t::to_string( sp_threshold ) +
                             ")|set_bonus.tier12_2pc_caster=0";
        }
        else
        {
          action_list_str += "/use_item,name=";
          action_list_str += items[ i ].name();
        }
      }
      action_list_str += init_use_profession_actions();
      action_list_str += init_use_racial_actions();

      if ( talent_elemental_mastery -> rank() )
      {
        if ( level > 80 )
          action_list_str += "/elemental_mastery";
        else
        {
          action_list_str += "/elemental_mastery,time_to_die<=17";
          action_list_str += "/elemental_mastery,if=!buff.bloodlust.react";
        }
      }

      if ( set_bonus.tier13_4pc_heal() )
        action_list_str += "/spiritwalkers_grace,if=!buff.bloodlust.react|target.time_to_die<=25";
      
      if ( ! glyph_unleashed_lightning -> ok() )
        action_list_str += "/unleash_elements,moving=1";
      action_list_str += "/flame_shock,if=!ticking|ticks_remain<2|((buff.bloodlust.react|buff.elemental_mastery.up)&ticks_remain<3)";
      if ( level >= 75 ) action_list_str += "/lava_burst,if=dot.flame_shock.remains>cast_time";
      if ( talent_fulmination -> rank() )
      {
        action_list_str += "/earth_shock,if=buff.lightning_shield.react=9";
        action_list_str += "/earth_shock,if=buff.lightning_shield.react>6&dot.flame_shock.remains>cooldown&dot.flame_shock.remains<cooldown+action.flame_shock.tick_time";
      }

      // Fire elemental summoning logic
      // 1) Make sure we fully use some commonly used temporary int bonuses of the profile, calculating the following
      // additively as a sum of temporary intellect buffs on the profile during iteration
      //    - Power Torrent or Lightweave Embroidery is up (either being up is counted as a temporary int buff of 500)
      //    - DMC: Volcano (temporary int buff of 1600)
      //    - Will of Unbinding (all versions are a temporary int buff of 700 int)
      // 2) If the profile hase Fiery Quintessence, align Fire Elemental summoning always with FQ (and require an
      //    additional 1100 temporary int)
      // 3) If the profile hase Bottled Wishes, align Fire Elemental summoning always with BW (and require an
      //    additional 2290 temporary spell power)
      // 4) If the profile uses pre-potting, make sure we try to use the FE during potion, so that
      //    all the other temporary int/sp buffs are also up
      sp_threshold += (has_fiery_quintessence?1:0) * 1149;
      sp_threshold += (has_bottled_wishes?1:0) * 2290;
      if ( set_bonus.tier12_2pc_caster() )
      {
        if ( sp_threshold > 0 )
        {
          if ( use_pre_potion )
          {
            action_list_str += "/fire_elemental_totem,if=buff.volcanic_potion.up&temporary_bonus.spell_power>=" +
                               util_t::to_string( sp_threshold + 1200 ) + "&(!ticking|remains<25)";
            action_list_str += "/fire_elemental_totem,if=!buff.volcanic_potion.up&temporary_bonus.spell_power>=" +
                               util_t::to_string( sp_threshold ) + "&(!ticking|remains<25)";
          }
          else
            action_list_str += "/fire_elemental_totem,if=temporary_bonus.spell_power>=" +
                               util_t::to_string( sp_threshold ) + "&(!ticking|remains<25)";
        }
        else
          action_list_str += "/fire_elemental_totem,if=!ticking";
      }
      else
      {
        if ( sp_threshold > 0 )
        {
          if ( use_pre_potion )
          {
            action_list_str += "/fire_elemental_totem,if=!ticking&buff.volcanic_potion.up&temporary_bonus.spell_power>=" +
                               util_t::to_string( sp_threshold + 1200 );
            action_list_str += "/fire_elemental_totem,if=!ticking&!buff.volcanic_potion.up&temporary_bonus.spell_power>=" +
                               util_t::to_string( sp_threshold );
          }
          else
          {
            action_list_str += "/fire_elemental_totem,if=!ticking&temporary_bonus.spell_power>=" +
                               util_t::to_string( sp_threshold );
          }
        }
        else
          action_list_str += "/fire_elemental_totem,if=!ticking";
      }

      action_list_str += "/earth_elemental_totem,if=!ticking";
      action_list_str += "/searing_totem";

      if ( glyph_unleashed_lightning -> ok() )
      {
        action_list_str += "/spiritwalkers_grace,moving=1,if=cooldown.lava_burst.remains=0&set_bonus.tier12_4pc_caster=0";
      }
      else
        action_list_str += "/spiritwalkers_grace,moving=1";
      
      action_list_str += "/chain_lightning,if=target.adds>2";
      if ( ! ( set_bonus.tier11_4pc_caster() || level > 80 ) )
        action_list_str += "/chain_lightning,if=(!buff.bloodlust.react&(mana_pct-target.health_pct)>5)|target.adds>1";
      action_list_str += "/lightning_bolt";
      if ( primary_tree() == TREE_ELEMENTAL ) action_list_str += "/thunderstorm";
    }

    action_list_default = 1;
  }

  player_t::init_actions();
}

// shaman_t::moving =========================================================

void shaman_t::moving()
{
  // Spiritwalker's Grace complicates things, as you can cast it while casting
  // anything. So, to model that, if a raid move event comes, we need to check
  // if we can trigger Spiritwalker's Grace. If so, conditionally execute it, to
  // allow the currently executing cast to finish.
  if ( level == 85 )
  {
    action_t* swg = find_action( "spiritwalkers_grace" );

    // We need to bypass swg -> ready() check here, so whip up a special
    // readiness check that only checks for player skill, cooldown and resource
    // availability
    if ( swg &&
         executing &&
         sim -> roll( skill ) &&
         swg -> cooldown -> remains() == timespan_t::zero &&
         resource_available( swg -> resource, swg -> cost() ) )
    {
      // Elemental has to do some additional checking here
      if ( primary_tree() == TREE_ELEMENTAL )
      {
        // Elemental executes SWG mid-cast during a movement event, if
        // 1) The profile does not have Glyph of Unleashed Lightning and is executing
        //    a Lighting Bolt
        // 2) The profile does not have Tier12 4PC set bonus and is executing a
        //    Lava Burst
        // 3) The profile is casting Chain Lightning
        if ( ( ! glyph_unleashed_lightning -> ok() && executing -> id == 403 ) ||
             ( ! set_bonus.tier12_4pc_caster() && executing -> id == 51505 ) ||
             executing -> id == 421 )
        {
          if ( sim -> log )
            log_t::output( sim, "spiritwalkers_grace during spell cast, next cast (%s) should finish",
                           executing -> name_str.c_str() );
          swg -> execute();
        }
      }
      // Other trees blindly execute Spiritwalker's Grace if there's a spell cast
      // executing during movement event
      else
      {
        swg -> execute();
        if ( sim -> log )
          log_t::output( sim, "spiritwalkers_grace during spell cast, next cast (%s) should finish",
                         executing -> name_str.c_str() );
      }
    }
    else
    {
      interrupt();
    }

    if ( main_hand_attack ) main_hand_attack -> cancel();
    if (  off_hand_attack )  off_hand_attack -> cancel();
  }
  else
  {
    halt();
  }
}

// shaman_t::matching_gear_multiplier =======================================

double shaman_t::matching_gear_multiplier( const attribute_type attr ) const
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

// shaman_t::composite_spell_hit ============================================
double shaman_t::composite_attack_hit() const
{
  double hit = player_t::composite_attack_hit();

  hit += spec_dual_wield -> base_value( E_APPLY_AURA, A_MOD_HIT_CHANCE );

  return hit;
}

// shaman_t::composite_spell_hit ============================================

double shaman_t::composite_spell_hit() const
{
  double hit = player_t::composite_spell_hit();

  hit += ( talent_elemental_precision -> base_value( E_APPLY_AURA, A_MOD_RATING_FROM_STAT ) *
    ( spirit() - attribute_base[ ATTR_SPIRIT ] ) ) / rating.spell_hit;

  return hit;
}

// shaman_t::composite_attack_crit ==========================================

double shaman_t::composite_attack_crit() const
{
  double crit = player_t::composite_attack_crit();

  if ( buffs_elemental_devastation -> up() )
    crit += buffs_elemental_devastation -> base_value();

  return crit;
}

// shaman_t::composite_spell_power ==========================================

double shaman_t::composite_spell_power( const school_type school ) const
{
  double sp = 0;

  if ( primary_tree() == TREE_ENHANCEMENT )
    sp = composite_attack_power_multiplier() * composite_attack_power() * spec_mental_quickness -> base_value( E_APPLY_AURA, A_366 ) / 100.0;
  else
  {
    sp = player_t::composite_spell_power( school );
  }

  return sp;
}

// shaman_t::composite_spell_power_multiplier ===============================

double shaman_t::composite_spell_power_multiplier() const
{
  if ( primary_tree() == TREE_ENHANCEMENT )
    return 1.0;

  return player_t::composite_spell_power_multiplier();
}

// shaman_t::composite_player_multiplier ====================================

double shaman_t::composite_player_multiplier( const school_type school, action_t* a ) const
{
  double m = player_t::composite_player_multiplier( school, a );

  if ( school == SCHOOL_FIRE || school == SCHOOL_FROST || school == SCHOOL_NATURE )
  {
    m *= 1.0 + composite_mastery() * mastery_enhanced_elements -> base_value( E_APPLY_AURA, A_DUMMY );

    if ( buffs_elemental_mastery -> up() )
      m *= 1.0 + buffs_elemental_mastery -> base_value( E_APPLY_AURA, A_MOD_DAMAGE_PERCENT_DONE );
  }

  if ( school != SCHOOL_PHYSICAL && school != SCHOOL_BLEED )
  {
    if ( main_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
      m *= 1.0 + main_hand_weapon.buff_value;
    else if ( off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
      m *= 1.0 + off_hand_weapon.buff_value;

    if ( sim -> auras.elemental_oath -> up() && buffs_elemental_focus -> up() )
    {
      // Elemental oath self buff is type 0 sub type 0
      m *= 1.0 + talent_elemental_oath -> base_value( E_NONE, A_NONE ) / 100.0;
    }
  }

  m *= 1.0 + talent_elemental_precision -> base_value( E_APPLY_AURA, A_MOD_DAMAGE_PERCENT_DONE );

  return m;
}

double shaman_t::composite_spell_crit() const
{
  double v = player_t::composite_spell_crit();

  if ( main_hand_weapon.buff_type == FLAMETONGUE_IMBUE || off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
    v += glyph_flametongue_weapon -> mod_additive( P_EFFECT_3 ) / 100.0;

  return v;
}


// shaman_t::regen  =========================================================

void shaman_t::regen( timespan_t periodicity )
{
  mana_regen_while_casting = ( primary_tree() == TREE_RESTORATION ) ? 0.50 : 0.0;

  player_t::regen( periodicity );

  if ( buffs_water_shield -> up() )
  {
    double water_shield_regen = periodicity.total_seconds() * buffs_water_shield -> base_value() / 5.0;

    resource_gain( RESOURCE_MANA, water_shield_regen, gains_water_shield );
  }
}

// shaman_t::combat_begin ===================================================

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

// shaman_t::decode_set =====================================================

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

  if ( strstr( s, "erupting_volcanic" ) )
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

    if ( is_caster ) return SET_T12_CASTER;
    if ( is_melee  ) return SET_T12_MELEE;
  }

  if ( strstr( s, "spiritwalkers" ) )
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

    bool is_heal  = ( strstr( s, "faceguard"      ) ||
                      strstr( s, "mantle"         ) ||
                      strstr( s, "tunic"          ) ||
                      strstr( s, "legwraps"       ) ||
                      strstr( s, "handwraps"      ) );

    if ( is_caster ) return SET_T13_CASTER;
    if ( is_melee  ) return SET_T13_MELEE;
    if ( is_heal   ) return SET_T13_HEAL;
  }

  if ( strstr( s, "_gladiators_linked_"   ) )     return SET_PVP_MELEE;
  if ( strstr( s, "_gladiators_mail_"     ) )     return SET_PVP_CASTER;
  if ( strstr( s, "_gladiators_ringmail_" ) )     return SET_PVP_MELEE;

  return SET_NONE;
}

// shaman_t::primary_role ===================================================

int shaman_t::primary_role() const
{
  if ( player_t::primary_role() == ROLE_HEAL )
    return ROLE_HEAL;

  if ( primary_tree() == TREE_RESTORATION )
  {
    if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_SPELL )
      return ROLE_SPELL;

    return ROLE_SPELL;
  }

  else if ( primary_tree() == TREE_ENHANCEMENT )
    return ROLE_HYBRID;

  else if ( primary_tree() == TREE_ELEMENTAL )
    return ROLE_SPELL;

  return ROLE_NONE;
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
  sim -> auras.elemental_oath    = new aura_t( sim, "elemental_oath",    1, timespan_t::zero );
  sim -> auras.flametongue_totem = new aura_t( sim, "flametongue_totem", 1, timespan_t::from_seconds(300.0) );
  sim -> auras.mana_spring_totem = new aura_t( sim, "mana_spring_totem", 1, timespan_t::from_seconds(300.0) );
  sim -> auras.strength_of_earth = new aura_t( sim, "strength_of_earth", 1, timespan_t::from_seconds(300.0) );
  sim -> auras.unleashed_rage    = new aura_t( sim, "unleashed_rage",    1, timespan_t::zero );
  sim -> auras.windfury_totem    = new aura_t( sim, "windfury_totem",    1, timespan_t::from_seconds(300.0) );
  sim -> auras.wrath_of_air      = new aura_t( sim, "wrath_of_air",      1, timespan_t::from_seconds(300.0) );

  for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    p -> buffs.bloodlust  = new buff_t( p, "bloodlust", 1, timespan_t::from_seconds(40.0) );
    p -> buffs.exhaustion = new buff_t( p, "exhaustion", 1, timespan_t::from_seconds(600.0), timespan_t::zero, 1.0, true );
    p -> buffs.mana_tide  = new buff_t( p, 16190, "mana_tide" );
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
    sim -> auras.mana_spring_totem -> override( 1, sim -> dbc.effect_average( sim -> dbc.spell( 5677 ) -> effect1().id(), sim -> max_player_level ) );
  if ( sim -> overrides.strength_of_earth )
    sim -> auras.strength_of_earth -> override( 1, sim -> dbc.effect_average( sim -> dbc.spell( 8076 ) -> effect1().id(), sim -> max_player_level ) );
}

shaman_targetdata_t::shaman_targetdata_t( player_t* source, player_t* target )
  : targetdata_t( source, target )
{
  shaman_t* p = source -> cast_shaman();
  debuffs_searing_flames          = add_aura( new searing_flames_buff_t  ( this, p -> talent_searing_flames -> spell_id(),                      "searing_flames"        ) );
}
