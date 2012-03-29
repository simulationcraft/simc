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

#include <simulationcraft.hpp>

// ==========================================================================
// Shaman
// ==========================================================================

#if SC_SHAMAN == 1

enum totem_type { TOTEM_NONE=0, TOTEM_AIR, TOTEM_EARTH, TOTEM_FIRE, TOTEM_WATER, TOTEM_MAX };

enum imbue_type_t { IMBUE_NONE=0, FLAMETONGUE_IMBUE, WINDFURY_IMBUE };

struct shaman_targetdata_t : public targetdata_t
{
  dot_t* dots_flame_shock;
  dot_t* dots_searing_flames;

  buff_t* debuffs_searing_flames;
  buff_t* debuffs_stormstrike;
  buff_t* debuffs_unleashed_fury_ft;

  shaman_targetdata_t( player_t* source, player_t* target );
};

void register_shaman_targetdata( sim_t* sim )
{
  player_type t = SHAMAN;
  typedef shaman_targetdata_t type;

  REGISTER_DOT( flame_shock );
  REGISTER_DOT( searing_flames );

  REGISTER_DEBUFF( searing_flames );
  REGISTER_DEBUFF( stormstrike );
  REGISTER_DEBUFF( unleashed_fury_ft );
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
  struct
  {
    buff_t* elemental_blast;
    buff_t* elemental_focus;
    buff_t* elemental_mastery;
    buff_t* flurry;
    buff_t* lava_surge;
    buff_t* lightning_shield;
    buff_t* maelstrom_weapon;
    buff_t* shamanistic_rage;
    buff_t* spiritwalkers_grace;
    buff_t* tier13_2pc_caster;
    buff_t* tier13_4pc_caster;
    buff_t* tier13_4pc_healer;
    buff_t* unleash_flame;
    buff_t* unleash_wind;
    buff_t* unleashed_fury_ft;
    buff_t* unleashed_fury_wf;
    buff_t* water_shield;
  } buff;

  // Cooldowns
  struct
  {
    cooldown_t* elemental_mastery;
    cooldown_t* fire_elemental;
    cooldown_t* lava_burst;
    cooldown_t* shock;
    cooldown_t* strike;
    cooldown_t* windfury_weapon;
  } cooldown;

  // Gains
  struct
  {
    gain_t* primal_wisdom;
    gain_t* rolling_thunder;
    gain_t* telluric_currents;
    gain_t* thunderstorm;
    gain_t* water_shield;
  } gain;

  // Tracked Procs
  struct
  {
    proc_t* elemental_overload;
    proc_t* lava_surge;
    proc_t* maelstrom_weapon;
    proc_t* rolling_thunder;
    proc_t* static_shock;
    proc_t* swings_clipped_mh;
    proc_t* swings_clipped_oh;
    proc_t* wasted_ls;
    proc_t* wasted_mw;
    proc_t* windfury;

    proc_t* fulmination[7];
    proc_t* maelstrom_weapon_used[6];
  } proc;

  // Random Number Generators
  struct
  {
    rng_t* elemental_overload;
    rng_t* lava_surge;
    rng_t* primal_wisdom;
    rng_t* rolling_thunder;
    rng_t* searing_flames;
    rng_t* static_shock;
    rng_t* windfury_delay;
    rng_t* windfury_weapon;
  } rng;

  // Class Specializations
  struct
  {
    // Generic
    spell_id_t* mail_specialization;

    // Elemental
    spell_id_t* elemental_focus;
    spell_id_t* elemental_precision;
    spell_id_t* elemental_fury;
    spell_id_t* fulmination;
    spell_id_t* lava_surge;
    spell_id_t* rolling_thunder;
    spell_id_t* shamanism;
    spell_id_t* spiritual_insight;

    // Enhancement
    spell_id_t* dual_wield;
    spell_id_t* flurry;
    spell_id_t* mental_quickness;
    spell_id_t* primal_wisdom;
    spell_id_t* shamanistic_rage;
    spell_id_t* static_shock;
    spell_id_t* maelstrom_weapon;
  } spec;

  // Masteries
  struct
  {
    spell_id_t* elemental_overload;
    spell_id_t* enhanced_elements;
  } mastery;

  // Talents
  struct
  {
    talent_t* call_of_the_elements;
    talent_t* totemic_restoration;

    talent_t* elemental_mastery;
    talent_t* ancestral_swiftness;
    talent_t* echo_of_the_elements;

    talent_t* unleashed_fury;
    talent_t* primal_elementalist;
    talent_t* elemental_blast;
  } talent;

  // Glyphs
  struct
  {
    glyph_t* chain_lightning;
    glyph_t* fire_elemental_totem;
    glyph_t* flame_shock;
    glyph_t* lava_burst;
    glyph_t* lava_lash;
    glyph_t* spiritwalkers_grace;
    glyph_t* telluric_currents;
    glyph_t* thunder;
    glyph_t* thunderstorm;
    glyph_t* unleashed_lightning;
    glyph_t* water_shield;
  } glyph;

  // Weapon Enchants
  attack_t* windfury_mh;
  attack_t* windfury_oh;
  spell_t*  flametongue_mh;
  spell_t*  flametongue_oh;


  shaman_t( sim_t* sim, const std::string& name, race_type r = RACE_NONE ) : player_t( sim, SHAMAN, name, r ),
    wf_delay( timespan_t::from_seconds( 0.95 ) ), wf_delay_stddev( timespan_t::from_seconds( 0.25 ) ),
    uf_expiration_delay( timespan_t::from_seconds( 0.3 ) ), uf_expiration_delay_stddev( timespan_t::from_seconds( 0.05 ) )
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
    cooldown.elemental_mastery    = get_cooldown( "elemental_mastery"    );
    cooldown.fire_elemental       = get_cooldown( "fire_elemental_totem" );
    cooldown.lava_burst           = get_cooldown( "lava_burst"           );
    cooldown.shock                = get_cooldown( "shock"                );
    cooldown.strike               = get_cooldown( "strike"               );
    cooldown.windfury_weapon      = get_cooldown( "windfury_weapon"      );

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
  virtual double    composite_attack_speed() const;
  virtual double    composite_spell_hit() const;
  virtual double    composite_spell_power( const school_type school ) const;
  virtual double    composite_spell_power_multiplier() const;
  virtual double    composite_player_multiplier( const school_type school, action_t* a = NULL ) const;
  virtual double    matching_gear_multiplier( const attribute_type attr ) const;
  virtual void      create_options();
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual int       decode_set( item_t& item );
  virtual resource_type primary_resource() const { return RESOURCE_MANA; }
  virtual int       primary_role() const;
  virtual void      combat_begin();

  // Event Tracking
  virtual void regen( timespan_t periodicity );
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// ==========================================================================
// Shaman Attack
// ==========================================================================

struct shaman_attack_state_t : public action_state_t
{
  shaman_t* actor;
  shaman_targetdata_t* td;

  shaman_attack_state_t( action_t* a, player_t* t ) :
    action_state_t( a, t ),
    actor( a -> player -> cast_shaman() ),
    td( targetdata_t::get( action -> player, target ) -> cast_shaman() ) { }
};

struct shaman_attack_t : public attack_t
{
  shaman_t* actor;
  bool windfury;
  bool flametongue;

  /* Old style construction, spell data will not be accessed */
  shaman_attack_t( const char* n, shaman_t* player, school_type s, int t, bool special ) :
    attack_t( n, player, RESOURCE_MANA, s, t, special ),
    actor( player -> cast_shaman() ), windfury( true ), flametongue( true )
  {
    if ( player -> primary_tree() == TREE_ENHANCEMENT &&
         ( school == SCHOOL_FIRE || school == SCHOOL_NATURE || school == SCHOOL_FROST ) )
      snapshot_flags |= STATE_MASTERY;
  }

  /* Class spell data based construction, spell name in s_name */
  shaman_attack_t( const char* n, const char* s_name, shaman_t* player ) :
    attack_t( n, s_name, player, TREE_NONE, true ),
    actor( player -> cast_shaman() ), windfury( true ), flametongue( true )
  {
    may_crit    = true;
    if ( player -> primary_tree() == TREE_ENHANCEMENT &&
         ( school == SCHOOL_FIRE || school == SCHOOL_NATURE || school == SCHOOL_FROST ) )
      snapshot_flags |= STATE_MASTERY;
  }

  /* Spell data based construction, spell id in spell_id */
  shaman_attack_t( const char* n, uint32_t spell_id, shaman_t* player ) :
    attack_t( n, spell_id, player, TREE_NONE, true ),
    actor( player -> cast_shaman() ), windfury( true ), flametongue( true )
  {
    may_crit    = true;
    if ( player -> primary_tree() == TREE_ENHANCEMENT &&
         ( school == SCHOOL_FIRE || school == SCHOOL_NATURE || school == SCHOOL_FROST ) )
      snapshot_flags |= STATE_MASTERY;
  }

  shaman_t* p() const
  { return static_cast<shaman_t*>( player ); }

  virtual void execute();
  virtual void impact_s( action_state_t* );
  virtual double cost() const;
  virtual double cost_reduction() const;
  virtual action_state_t* new_state();
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
  shaman_t* actor;

  /* Old style construction, spell data will not be accessed */
  shaman_spell_t( const char* n, shaman_t* p, const std::string& options_str, const school_type s, int t ) :
    spell_t( n, p, RESOURCE_MANA, s, t ),
    base_cost_reduction( 0 ), maelstrom( false ), overload( false ), is_totem( false ),
    actor( p -> cast_shaman() )
  {
    parse_options( NULL, options_str );
    if ( actor -> primary_tree() == TREE_ENHANCEMENT &&
         ( school == SCHOOL_FIRE || school == SCHOOL_NATURE || school == SCHOOL_FROST ) )
      snapshot_flags |= STATE_MASTERY;

    crit_bonus_multiplier *= 1.0 + actor -> spec.elemental_fury -> effect1().percent();
  }

  /* Class spell data based construction, spell name in s_name */
  shaman_spell_t( const char* n, const char* s_name, player_t* p, const std::string& options_str = std::string() ) :
    spell_t( n, s_name, p ), base_cost_reduction( 0.0 ), maelstrom( false ), overload( false ), is_totem( false ),
    actor( p -> cast_shaman() )
  {
    parse_options( NULL, options_str );

    may_crit = true;
    if ( actor -> primary_tree() == TREE_ENHANCEMENT &&
         ( school == SCHOOL_FIRE || school == SCHOOL_NATURE || school == SCHOOL_FROST ) )
      snapshot_flags |= STATE_MASTERY;

    crit_bonus_multiplier *= 1.0 + actor -> spec.elemental_fury -> effect1().percent();
  }

  /* Spell data based construction, spell id in spell_id */
  shaman_spell_t( const char* n, uint32_t spell_id, player_t* p, const std::string& options_str = std::string() ) :
    spell_t( n, spell_id, p ), base_cost_reduction( 0.0 ), maelstrom( false ), overload( false ), is_totem( false ),
    actor( p -> cast_shaman() )
  {
    parse_options( NULL, options_str );

    may_crit = true;
    if ( actor -> primary_tree() == TREE_ENHANCEMENT &&
         ( school == SCHOOL_FIRE || school == SCHOOL_NATURE || school == SCHOOL_FROST ) )
      snapshot_flags |= STATE_MASTERY;

    crit_bonus_multiplier *= 1.0 + actor -> spec.elemental_fury -> effect1().percent();
  }

  virtual bool   is_direct_damage() const { return base_dd_min > 0 && base_dd_max > 0; }
  virtual bool   is_periodic_damage() const { return base_td > 0; };
  virtual double cost() const;
  virtual double cost_reduction() const;
  virtual void   consume_resource();
  virtual void   execute();
  virtual void   impact_s( action_state_t* );
  virtual double haste() const;
  virtual void   schedule_execute();
  virtual action_state_t* new_state();
  virtual bool   usable_moving()
  {
    if ( actor -> buff.spiritwalkers_grace -> check() || execute_time() == timespan_t::zero )
      return true;

    return spell_t::usable_moving();
  }
};

struct shaman_spell_state_t : public action_state_t
{
  shaman_spell_t*      spell;
  shaman_t*            actor;
  shaman_targetdata_t* td;

  shaman_spell_state_t( action_t* a, player_t* t ) :
    action_state_t( a, t ),
    spell( dynamic_cast< shaman_spell_t* >( a ) ),
    actor( a -> player -> cast_shaman() ),
    td( targetdata_t::get( action -> player, target ) -> cast_shaman() ) { }

  void snapshot_target_crit();
  void snapshot_haste();
  void snapshot_actor_multiplier( uint32_t );
  void take_state( uint32_t flags )
  {
    action_state_t::take_state( flags );
    td = targetdata_t::get( action -> player, target ) -> cast_shaman();
  }
};

inline void shaman_spell_state_t::snapshot_target_crit()
{
  action_state_t::snapshot_target_crit();

  if ( action -> school == SCHOOL_NATURE && td -> debuffs_stormstrike -> up() )
    target_crit += td -> debuffs_stormstrike -> effect1().percent();
}

inline void shaman_spell_state_t::snapshot_haste()
{
  action_state_t::snapshot_haste();

  if ( actor -> buff.elemental_mastery -> up() )
    haste *= 1.0 / ( 1.0 + actor -> buff.elemental_mastery -> effect1().percent() );
}

inline void shaman_spell_state_t::snapshot_actor_multiplier( uint32_t flags )
{
  action_state_t::snapshot_actor_multiplier( flags );

  if ( spell -> maelstrom && actor -> buff.maelstrom_weapon -> stack() > 0 )
    actor_multiplier *= 1.0 + actor -> sets -> set( SET_T13_2PC_MELEE ) -> effect1().percent();
}

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

      // Two wolves
      base_multiplier  *= 2.0;
      // TODO: Wolves get no weapon speed based bonus to damage?
      weapon_power_mod /= player -> main_hand_weapon.swing_time.total_seconds();
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
          int   mwstack = o -> buff.maelstrom_weapon -> check();
          if ( o -> buff.maelstrom_weapon -> trigger( 1, -1, 1.0 ) )
          {
            if ( mwstack == o -> buff.maelstrom_weapon -> max_stack )
              o -> proc.wasted_mw -> occur();

            o -> proc.maelstrom_weapon -> occur();
          }
        }

        if ( sim -> roll( o -> sets -> set( SET_T13_4PC_MELEE ) -> effect1().percent() ) )
        {
          int   mwstack = o -> buff.maelstrom_weapon -> check();
          if ( o -> buff.maelstrom_weapon -> trigger( 1, -1, 1.0 ) )
          {
            if ( mwstack == o -> buff.maelstrom_weapon -> max_stack )
              o -> proc.wasted_mw -> occur();

            o -> proc.maelstrom_weapon -> occur();
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
    main_hand_weapon.min_dmg    = 655; // Level 85 Values, approximated
    main_hand_weapon.max_dmg    = 934;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.5 );
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
    double ap_per_owner = 0.4944;

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

  virtual double composite_attribute_multiplier( int attr )
  {
    if ( attr == ATTR_STRENGTH || attr == ATTR_AGILITY )
      return 1.0;

    return player_t::composite_attribute_multiplier( attr );
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

      p -> main_hand_attack -> schedule_execute();
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
    virtual double crit_chance( int ) const
    {
      return 0.03;
    }

    virtual double crit_chance( const action_state_t* ) const
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
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
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

  virtual resource_type primary_resource() const { return RESOURCE_MANA; }

  virtual void regen( timespan_t /* periodicity */ ) { }

  virtual void summon( timespan_t /* duration */ )
  {
    pet_t::summon();
    owner_sp = owner -> composite_spell_power( SCHOOL_MAX ) * owner -> composite_spell_power_multiplier();
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

  virtual double composite_attack_expertise( weapon_t* ) const
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
      spell_t::execute();

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
      base_execute_time         = timespan_t::from_seconds( 3.0 );
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
      cooldown -> duration = timespan_t::from_seconds( fe -> rng_ability_cooldown -> range( 30.0, 60.0 ) );

      // 207 = 80
      base_costs[ RESOURCE_MANA ]            = player -> level * 2.750;
      // For now, model the cast time increase as well, see below
      base_execute_time    = player -> dbc.spell( 12470 ) -> cast_time( player -> level );

      base_dd_min          = 583;
      base_dd_max          = 663;
    }

    virtual void execute()
    {
      fire_elemental_pet_t* fe = dynamic_cast< fire_elemental_pet_t* >( player );
      // Randomize next cooldown duration here
      cooldown -> duration = timespan_t::from_seconds( fe -> rng_ability_cooldown -> range( 30.0, 60.0 ) );

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
      base_costs[ RESOURCE_MANA ]            = ( player -> level ) * 3.554;
      base_execute_time    = timespan_t::zero;
      direct_power_mod     = player -> dbc.spell( 57984 ) -> effect1().coeff();
      cooldown -> duration = timespan_t::from_seconds( fe -> rng_ability_cooldown -> range( 5.0, 7.0 ) );

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
      cooldown -> duration = timespan_t::from_seconds( fe -> rng_ability_cooldown -> range( 5.0, 7.0 ) );

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
    main_hand_weapon.swing_time      = timespan_t::from_seconds( 2.0 );

    rng_ability_cooldown             = get_rng( "fire_elemental_ability_cooldown" );
    rng_ability_cooldown -> average_range = 0;

    cooldown_fire_nova               = get_cooldown( "fire_nova" );
    cooldown_fire_blast              = get_cooldown( "fire_blast" );

    // Run menacingly to the target, start meleeing and acting erratically
    action_list_str                  = "travel/auto_attack/fire_nova/fire_blast";

    fire_shield                      = new fire_shield_t( this );
  }

  virtual resource_type primary_resource() const { return RESOURCE_MANA; }

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
    pet_t::summon();

    owner_int = owner -> intellect();
    owner_sp  = ( owner -> composite_spell_power( SCHOOL_FIRE ) - owner -> spell_power_per_intellect * owner_int ) * owner -> composite_spell_power_multiplier();

    fire_shield -> num_ticks = ( int ) ( duration / fire_shield -> base_execute_time );
    fire_shield -> execute();

    cooldown_fire_nova -> start();
    cooldown_fire_blast -> start();
  }

  virtual double composite_attribute( int attr ) const
  {
    if ( attr == ATTR_INTELLECT )
      return owner_int;

    return pet_t::composite_attribute( attr );
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

  virtual double composite_attack_expertise( weapon_t* ) const
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

  if ( a -> weapon -> slot == SLOT_MAIN_HAND )
    p -> flametongue_mh -> execute();
  else
    p -> flametongue_oh -> execute();
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

    p -> proc.windfury -> occur();
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

  if ( p -> cooldown.windfury_weapon -> remains() > timespan_t::zero ) return false;

  if ( p -> rng.windfury_weapon -> roll( wf -> proc_chance() ) )
  {
    p -> cooldown.windfury_weapon -> start( p -> rng.windfury_delay -> gauss( timespan_t::from_seconds( 3.0 ), timespan_t::from_seconds( 0.3 ) ) );

    // Delay windfury by some time, up to about a second
    new ( p -> sim ) windfury_delay_event_t( p -> sim, p, wf, p -> rng.windfury_delay -> gauss( p -> wf_delay, p -> wf_delay_stddev ) );
    return true;
  }
  return false;
}

// trigger_rolling_thunder ==================================================

static bool trigger_rolling_thunder ( spell_t* s )
{
  shaman_t* p = s -> player -> cast_shaman();

  if ( ! p -> buff.lightning_shield -> check() )
    return false;

  if ( p -> rng.rolling_thunder -> roll( p -> spec.rolling_thunder -> proc_chance() ) )
  {
    p -> resource_gain( RESOURCE_MANA,
                        p -> dbc.spell( 88765 ) -> effect1().percent() * p -> resource_max[ RESOURCE_MANA ],
                        p -> gain.rolling_thunder );

    if ( p -> buff.lightning_shield -> check() == p -> buff.lightning_shield -> max_stack )
      p -> proc.wasted_ls -> occur();

    p -> buff.lightning_shield -> trigger();
    p -> proc.rolling_thunder  -> occur();
    return true;
  }

  return false;
}

// trigger_static_shock =====================================================

static bool trigger_static_shock ( attack_t* a )
{
  shaman_t* p = a -> player -> cast_shaman();

  if ( ! p -> buff.lightning_shield -> stack() )
    return false;

  if ( p -> rng.static_shock -> roll( p -> spec.static_shock -> proc_chance() ) )
  {
    p -> active_lightning_charge -> execute();
    p -> proc.static_shock -> occur();
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

    improved_lava_lash_t( shaman_t* p ) :
      shaman_spell_t( "improved_lava_lash", p, "", SCHOOL_FIRE, TREE_ENHANCEMENT ),
      imp_ll_rng( 0 ), imp_ll_fs_cd( 0 )
    {
      aoe = 4;
      may_miss = may_crit = false;
      proc = true;
      callbacks = false;
      background = true;
      stateless = true;

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
    void impact_s( action_state_t* state )
    {
      if ( sim -> debug )
        log_t::output( sim, "%s spreads Flame Shock (off of %s) on %s",
                       player -> name(),
                       target -> name(),
                       state -> target -> name() );

      shaman_t* p = player -> cast_shaman();

      double dd_min = p -> action_flame_shock -> base_dd_min,
             dd_max = p -> action_flame_shock -> base_dd_max,
             coeff = p -> action_flame_shock -> direct_power_mod,
             real_base_cost = p -> action_flame_shock -> base_costs[ p -> action_flame_shock -> current_resource() ];
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
      p -> action_flame_shock -> base_costs[ p -> action_flame_shock -> current_resource() ] = 0;
      p -> action_flame_shock -> target = state -> target;
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
      p -> action_flame_shock -> base_costs[ p -> action_flame_shock -> current_resource() ] = real_base_cost;
      p -> action_flame_shock -> target = original_target;
      p -> action_flame_shock -> cooldown = original_cd;
      p -> action_flame_shock -> stats = original_stats;

      // Hide the Flame Shock dummy stat and improved_lava_lash from reports
      fs_dummy_stat -> num_executes = 0;
      stats -> num_executes = 0;

      release_state( state );
    }
  };

  // Do not spread the love when there is only poor Fluffy Pillow against you
  if ( a -> sim -> num_enemies == 1 )
    return false;

  shaman_targetdata_t* t = a -> targetdata() -> cast_shaman();

  if ( ! t -> dots_flame_shock -> ticking )
    return false;

  shaman_t* p = a -> player -> cast_shaman();

  if ( p -> glyph.lava_lash -> ok() )
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
  struct lava_burst_overload_state_t : public shaman_spell_state_t
  {
    lava_burst_overload_state_t( action_t* a, player_t* t ) :
      shaman_spell_state_t( a, t ) { }

    inline void snapshot_target_crit()
    {
      shaman_spell_state_t::snapshot_target_crit();

      // TODO: Does Glyph of Lava Burst affect overload lava bursts?
      if ( td -> dots_flame_shock -> ticking || actor -> glyph.lava_burst -> ok() )
        target_crit += 1.0;
    }
  };

  lava_burst_overload_t( player_t* player, bool dtr=false ) :
    shaman_spell_t( "lava_burst_overload", 77451, player )
  {
    overload             = true;
    background           = true;
    stateless            = true;

    // TODO: Does Glyph of Lava Burst affect overload lava bursts?
    if ( actor -> glyph.lava_burst -> ok() )
      base_multiplier *= 1.0 + actor -> glyph.lava_burst -> effect1().percent();

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new lava_burst_overload_t( player, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  action_state_t* new_state()
  {
    return new lava_burst_overload_state_t( this, target );
  }
};

struct lightning_bolt_overload_t : public shaman_spell_t
{
  lightning_bolt_overload_t( player_t* player, bool dtr=false ) :
    shaman_spell_t( "lightning_bolt_overload", 45284, player )
  {
    overload             = true;
    background           = true;
    stateless            = true;

    direct_power_mod  += actor -> spec.shamanism -> effectN( 1 ).percent();

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new lightning_bolt_overload_t( player, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  void impact_( action_state_t* state )
  {
    shaman_spell_t::impact_s( state );

    if ( result_is_hit( state -> result ) )
      trigger_rolling_thunder( this );
  }
};

struct chain_lightning_overload_t : public shaman_spell_t
{
  int glyph_targets;

  chain_lightning_overload_t( player_t* player, bool dtr=false ) :
    shaman_spell_t( "chain_lightning_overload", 45297, player ), glyph_targets( 0 )
  {
    overload             = true;
    background           = true;
    stateless            = true;

    direct_power_mod  += actor -> spec.shamanism -> effectN( 2 ).percent();

    aoe = 2;
    if ( actor -> glyph.chain_lightning -> ok() )
    {
      base_multiplier *= 1.0 + actor -> glyph.chain_lightning -> effectN( 2 ).percent();
      aoe             += ( int ) actor -> glyph.chain_lightning -> effectN( 1 ).base_value();
    }

    base_add_multiplier = effect_chain_multiplier( 1 );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new chain_lightning_overload_t( player, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  void impact_s( action_state_t* state )
  {
    shaman_spell_t::impact_s( state );

    if ( result_is_hit( state -> result ) )
      trigger_rolling_thunder( this );
  }
};

struct searing_flames_t : public shaman_spell_t
{
  struct searing_flames_state_t : public shaman_spell_state_t
  {
    searing_flames_state_t( action_t* a, player_t* t ) :
      shaman_spell_state_t( a, t )
    {
    }

    // Target multiplier is the amount of searing flames stacks on it
    inline void snapshot_target_multiplier()
    {
      target_multiplier = td -> debuffs_searing_flames -> stack();
    }
  };

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
    stateless        = true;
    snapshot_flags   = STATE_TARGET_MUL | STATE_CRIT;
    update_flags     = 0;

    // Override spell data values, as they seem wrong, instead
    // make searing totem calculate the damage portion of the
    // spell, without using any tick power modifiers or
    // player based multipliers here
    base_td          = 0.0;
    tick_power_mod   = 0.0;
  }

  action_state_t* new_state()
  {
    return new searing_flames_state_t( this, target );
  }

  void last_tick( dot_t* d )
  {
    shaman_targetdata_t* td = targetdata() -> cast_shaman();
    shaman_spell_t::last_tick( d );
    td -> debuffs_searing_flames -> expire();
  }
};

struct lightning_charge_t : public shaman_spell_t
{
  struct lightning_charge_state_t : public shaman_spell_state_t
  {
    int threshold;

    lightning_charge_state_t( action_t* a, player_t* t ) :
      shaman_spell_state_t( a, t ),
      threshold( ( int ) actor -> spec.fulmination -> base_value() ) { }

    inline void snapshot_actor_multiplier( uint32_t flags )
    {
      shaman_spell_state_t::snapshot_actor_multiplier( flags );

      if ( threshold > 0 )
      {
        // Don't use stack() here so we don't count the occurence twice
        // together with trigger_fulmination()
        int consuming_stack =  actor -> buff.lightning_shield -> check() - threshold;
        if ( consuming_stack > 0 )
          actor_da_multiplier *= consuming_stack;
      }
    }
  };

  lightning_charge_t( player_t* player, const std::string& n, bool dtr=false ) :
    shaman_spell_t( n.c_str(), 26364, player )
  {
    // Use the same name "lightning_shield" to make sure the cost of refreshing the shield is included with the procs.
    background       = true;
    may_crit         = true;
    stateless        = true;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new lightning_charge_t( player, n, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  action_state_t* new_state()
  {
    return new lightning_charge_state_t( this, target );
  }
};

struct unleash_flame_t : public shaman_spell_t
{
  unleash_flame_t( player_t* player ) :
    shaman_spell_t( "unleash_flame", 73683, player )
  {
    background           = true;
    proc                 = true;
    stateless            = true;

    // Don't cooldown here, unleash elements ability will handle it
    cooldown -> duration = timespan_t::zero;
  }

  virtual void execute()
  {
    if ( actor -> main_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
    {
      shaman_spell_t::execute();
    }

    if ( actor -> off_hand_weapon.type != WEAPON_NONE &&
         actor -> off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
    {
      shaman_spell_t::execute();
    }
  }
};

struct flametongue_weapon_spell_t : public shaman_spell_t
{
  flametongue_weapon_spell_t( player_t* player, const std::string& n, weapon_t* w ) :
    shaman_spell_t( n.c_str(), 8024, player )
  {
    may_crit           = true;
    background         = true;
    proc               = true;
    stateless          = true;
    direct_power_mod   = 1.0;

    base_dd_min = w -> swing_time.total_seconds() / 4.0 * player -> dbc.effect_min( effect2().id(), player -> level ) / 25.0;
    base_dd_max = base_dd_min;

    if ( player -> primary_tree() == TREE_ENHANCEMENT )
    {
      snapshot_flags               = STATE_AP;
      base_attack_power_multiplier = w -> swing_time.total_seconds() / 4.0 * 0.1253;
      base_spell_power_multiplier  = 0;
    }
    else
    {
      base_attack_power_multiplier = 0;
      base_spell_power_multiplier  = w -> swing_time.total_seconds() / 4.0 * 0.1253;
    }

    init();
  }
};

struct windfury_weapon_attack_t : public shaman_attack_t
{
  struct windfury_state_t : public shaman_attack_state_t
  {
    windfury_state_t( action_t* a, player_t* t ) :
      shaman_attack_state_t( a, t ) { }

    inline void snapshot_attack_power()
    {
      shaman_attack_state_t::snapshot_attack_power();

      actor_attack_power += action -> weapon -> buff_value;
    }
  };

  windfury_weapon_attack_t( shaman_t* player, const std::string& n, weapon_t* w ) :
    shaman_attack_t( n.c_str(), 33757, player )
  {
    weapon           = w;
    school           = SCHOOL_PHYSICAL;
    stats -> school  = SCHOOL_PHYSICAL;
    background       = true;
    callbacks        = false; // Windfury does not proc any On-Equip procs, apparently
    stateless        = true;

    init();
  }

  action_state_t* new_state()
  {
    return new windfury_state_t( this, target );
  }
};

struct unleash_wind_t : public shaman_attack_t
{
  unleash_wind_t( shaman_t* player ) :
    shaman_attack_t( "unleash_wind", 73681, player )
  {
    background            = true;
    windfury              = false;
    may_dodge = may_parry = false;
    stateless             = true;

    // Don't cooldown here, unleash elements will handle it
    cooldown -> duration = timespan_t::zero;
  }

  void execute()
  {
    // Figure out weapons
    if ( actor -> main_hand_weapon.buff_type == WINDFURY_IMBUE )
    {
      weapon = &( actor -> main_hand_weapon );
      shaman_attack_t::execute();
    }

    if ( actor -> off_hand_weapon.type != WEAPON_NONE &&
         actor -> off_hand_weapon.buff_type == WINDFURY_IMBUE )
    {
      weapon = &( actor -> off_hand_weapon );
      shaman_attack_t::execute();
    }
  }
};

struct stormstrike_attack_t : public shaman_attack_t
{
  stormstrike_attack_t( shaman_t* player, const char* n, uint32_t id, weapon_t* w ) :
    shaman_attack_t( n, id, player )
  {
    background           = true;
    may_miss             = false;
    may_dodge            = false;
    may_parry            = false;
    weapon               = w;
    stateless            = true;
  }
};

// ==========================================================================
// Shaman Attack
// ==========================================================================

action_state_t* shaman_attack_t::new_state()
{
  return new shaman_attack_state_t( this, target );
}

// shaman_attack_t::execute =================================================

void shaman_attack_t::execute()
{
  attack_t::execute();

  actor -> buff.spiritwalkers_grace -> up();
}

void shaman_attack_t::impact_s( action_state_t* state )
{
  attack_t::impact_s( state );

  if ( result_is_hit( state -> result ) && ! proc )
  {
    int   mwstack = actor -> buff.maelstrom_weapon -> check();
    // TODO: Chance is based on Rank 3, i.e., 10 PPM?
    double chance = weapon -> proc_chance_on_swing( 10.0 );

    if ( actor -> buff.maelstrom_weapon -> trigger( 1, -1, chance ) )
    {
      if ( mwstack == actor -> buff.maelstrom_weapon -> max_stack )
        actor -> proc.wasted_mw -> occur();

      actor -> proc.maelstrom_weapon -> occur();
    }

    if ( windfury && weapon -> buff_type == WINDFURY_IMBUE )
      trigger_windfury_weapon( this );

    if ( flametongue && weapon -> buff_type == FLAMETONGUE_IMBUE )
      trigger_flametongue_weapon( this );

    if ( actor -> rng.primal_wisdom -> roll( actor -> spec.primal_wisdom -> proc_chance() ) )
    {
      double amount = actor -> dbc.spell( 63375 ) -> effect1().percent() * actor -> resource_base[ RESOURCE_MANA ];
      actor -> resource_gain( RESOURCE_MANA, amount, actor -> gain.primal_wisdom );
    }
  }
}

// shaman_attack_t::cost_reduction ==========================================

double shaman_attack_t::cost_reduction() const
{
  return actor -> buff.shamanistic_rage -> effectN( 1 ).percent();
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

  melee_t( const char* name, shaman_t* player, int sw ) :
    shaman_attack_t( name, player, SCHOOL_PHYSICAL, TREE_NONE, false ), sync_weapons( sw )
  {
    may_crit    = true;
    background  = true;
    repeating   = true;
    trigger_gcd = timespan_t::zero;
    stateless   = true;

    if ( actor -> dual_wield() ) base_hit -= 0.19;
  }

  virtual timespan_t execute_time() const
  {
    timespan_t t = shaman_attack_t::execute_time();
    if ( ! player -> in_combat )
    {
      return ( weapon -> slot == SLOT_OFF_HAND ) ? ( sync_weapons ? std::min( t/2, timespan_t::from_seconds( 0.2 ) ) : t/2 ) : timespan_t::from_seconds( 0.01 );
    }
    return t;
  }

  void execute()
  {
    if ( time_to_execute > timespan_t::zero && actor -> executing )
    {
      if ( sim -> debug ) log_t::output( sim, "Executing '%s' during melee (%s).", actor -> executing -> name(), util_t::slot_type_string( weapon -> slot ) );
      schedule_execute();
    }
    else
    {
      shaman_attack_t::execute();
    }
  }

  void impact_s( action_state_t* state )
  {
    shaman_attack_t::impact_s( state );

    if ( state -> result == RESULT_CRIT )
      actor -> buff.flurry -> trigger( actor -> buff.flurry -> initial_stacks() );
  }

  void schedule_execute()
  {
    shaman_attack_t::schedule_execute();
    // Clipped swings do not eat unleash wind buffs
    if ( time_to_execute > timespan_t::zero && ! actor -> executing )
      actor -> buff.unleash_wind -> decrement();
  }
};

// Auto Attack ==============================================================

struct auto_attack_t : public shaman_attack_t
{
  int sync_weapons;

  auto_attack_t( shaman_t* player, const std::string& options_str ) :
    shaman_attack_t( "auto_attack", player, SCHOOL_PHYSICAL, TREE_NONE, false ),
    sync_weapons( 0 )
  {
    option_t options[] =
    {
      { "sync_weapons", OPT_BOOL, &sync_weapons },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    assert( actor -> main_hand_weapon.type != WEAPON_NONE );
    actor -> main_hand_attack = new melee_t( "melee_main_hand", player, sync_weapons );
    actor -> main_hand_attack -> weapon = &( actor -> main_hand_weapon );
    actor -> main_hand_attack -> base_execute_time = actor -> main_hand_weapon.swing_time;

    if ( actor -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( ! actor -> dual_wield() ) return;
      actor -> off_hand_attack = new melee_t( "melee_off_hand", player, sync_weapons );
      actor -> off_hand_attack -> weapon = &( actor -> off_hand_weapon );
      actor -> off_hand_attack -> base_execute_time = actor -> off_hand_weapon.swing_time;
    }

    trigger_gcd = timespan_t::zero;
  }

  virtual void execute()
  {
    actor -> main_hand_attack -> schedule_execute();
    if ( actor -> off_hand_attack )
      actor -> off_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    if ( actor -> is_moving() ) return false;
    return ( actor -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Lava Lash Attack =========================================================

struct lava_lash_t : public shaman_attack_t
{
  // Lava Lash multiplier calculation from
  // http://elitistjerks.com/f79/t110302-enhsim_cataclysm/p11/#post1935780
  // MoP: Vastly simplified, most bonuses are gone
  struct lava_lash_state_t : public shaman_attack_state_t
  {
    double ft_bonus;
    double sf_bonus;

    lava_lash_state_t( action_t* a, player_t* p, double ft, double sf ) :
      shaman_attack_state_t( a, p ), ft_bonus( ft ), sf_bonus( sf ) { }

    inline void snapshot_target_multiplier()
    {
      shaman_attack_state_t::snapshot_target_multiplier();

      // Second group, Searing Flames + Flametongue bonus
      target_multiplier *= 1.0 +
                           ( td -> debuffs_searing_flames -> check() * sf_bonus ) +
                           ( action -> weapon -> buff_type == FLAMETONGUE_IMBUE ) * ft_bonus;
    }
  };

  lava_lash_t( shaman_t* player, const std::string& options_str ) :
    shaman_attack_t( "lava_lash", "Lava Lash", player )
  {
    check_spec( TREE_ENHANCEMENT );

    parse_options( NULL, options_str );

    stateless           = true;
    weapon              = &( player -> off_hand_weapon );

    if ( weapon -> type == WEAPON_NONE )
      background        = true; // Do not allow execution.
  }

  action_state_t* new_state()
  {
    return new lava_lash_state_t( this, target, effectN( 2 ).percent(), actor -> dbc.spell( 77657 ) -> effectN( 1 ).percent() );
  }

  void impact_s( action_state_t* state )
  {
    shaman_attack_t::impact_s( state );

    if ( result_is_hit( state -> result ) )
    {
      shaman_targetdata_t* td = targetdata_t::get( actor, state -> target ) -> cast_shaman();
      td -> debuffs_searing_flames -> expire();
      if ( actor -> active_searing_flames_dot )
        actor -> active_searing_flames_dot -> dot() -> cancel();

      trigger_static_shock( this );
      if ( td -> dots_flame_shock -> ticking )
        trigger_improved_lava_lash( this );
    }
  }
};

// Primal Strike Attack =====================================================

struct primal_strike_t : public shaman_attack_t
{
  primal_strike_t( shaman_t* player, const std::string& options_str ) :
    shaman_attack_t( "primal_strike", "Primal Strike", player )
  {
    parse_options( NULL, options_str );

    stateless            = true;
    weapon               = &( actor -> main_hand_weapon );
    cooldown             = actor -> cooldown.strike;
    cooldown -> duration = actor -> dbc.spell( id ) -> cooldown();
  }

  void impact_s( action_state_t* state )
  {
    shaman_attack_t::impact_s( state );
    if ( result_is_hit( state -> result ) )
      trigger_static_shock( this );
  }
};

// Stormstrike Attack =======================================================

struct stormstrike_t : public shaman_attack_t
{
  stormstrike_attack_t * stormstrike_mh;
  stormstrike_attack_t * stormstrike_oh;

  stormstrike_t( shaman_t* player, const std::string& options_str ) :
    shaman_attack_t( "stormstrike", "Stormstrike", player ),
    stormstrike_mh( 0 ), stormstrike_oh( 0 )
  {
    check_spec( TREE_ENHANCEMENT );

    parse_options( NULL, options_str );

    stateless            = true;
    weapon               = &( actor -> main_hand_weapon );
    weapon_multiplier    = 0.0;
    may_crit             = false;
    cooldown             = actor -> cooldown.strike;
    cooldown -> duration = actor -> dbc.spell( id ) -> cooldown();

    // Actual damaging attacks are done by stormstrike_attack_t
    stormstrike_mh = new stormstrike_attack_t( actor, "stormstrike_mh", effect_trigger_spell( 2 ), &( actor -> main_hand_weapon ) );
    add_child( stormstrike_mh );

    if ( actor -> off_hand_weapon.type != WEAPON_NONE )
    {
      stormstrike_oh = new stormstrike_attack_t( actor, "stormstrike_oh", effect_trigger_spell( 3 ), &( actor -> off_hand_weapon ) );
      add_child( stormstrike_oh );
    }
  }

  void impact_s( action_state_t* state )
  {
    // Bypass shaman-specific attack based procs and co for this spell, the relevant ones
    // are handled by stormstrike_attack_t
    attack_t::impact_s( state );

    if ( result_is_hit( state -> result ) )
    {
      shaman_targetdata_t* td = targetdata_t::get( actor, state -> target ) -> cast_shaman();
      td -> debuffs_stormstrike -> trigger();

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

action_state_t* shaman_spell_t::new_state()
{
  return new shaman_spell_state_t( this, target );
}

// shaman_spell_t::haste ====================================================

double shaman_spell_t::haste() const
{
  double h = spell_t::haste();
  if ( actor -> buff.elemental_mastery -> up() )
    h *= 1.0 / ( 1.0 + actor -> buff.elemental_mastery -> effect1().percent() );

  if ( actor -> buff.tier13_4pc_healer -> up() )
    h *= 1.0 / ( 1.0 + actor -> buff.tier13_4pc_healer -> effect1().percent() );

  return h;
}

// shaman_spell_t::cost_reduction ===========================================

double shaman_spell_t::cost_reduction() const
{
  double   cr = base_cost_reduction;

  if ( ( harmful || is_totem ) && actor -> buff.shamanistic_rage -> up() )
    cr += actor -> buff.shamanistic_rage -> effectN( 1 ).percent();

  if ( harmful && callbacks && ! proc && actor -> buff.elemental_focus -> up() )
    cr += actor -> buff.elemental_focus -> effectN( 1 ).percent();

  if ( base_execute_time == timespan_t::zero )
    cr += actor -> spec.mental_quickness -> effectN( 2 ).percent();

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
  if ( harmful && callbacks && ! proc && resource_consumed > 0 && actor -> buff.elemental_focus -> up() )
    actor -> buff.elemental_focus -> decrement();
}

// shaman_spell_t::execute =================================================

void shaman_spell_t::execute()
{
  spell_t::execute();

  if ( ! is_totem && ! proc && school == SCHOOL_FIRE )
    actor -> buff.unleash_flame -> expire();

  // Record maelstrom weapon stack usage
  if ( maelstrom && actor -> primary_tree() == TREE_ENHANCEMENT )
    actor -> proc.maelstrom_weapon_used[ actor -> buff.maelstrom_weapon -> check() ] -> occur();

  // Shamans have specialized swing timer reset system, where every cast time spell
  // resets the swing timers, _IF_ the spell is not maelstromable, or the maelstrom
  // weapon stack is zero.
  if ( execute_time() > timespan_t::zero )
  {
    if ( ! maelstrom || actor -> buff.maelstrom_weapon -> check() == 0 )
    {
      if ( sim -> debug )
      {
        log_t::output( sim, "Resetting swing timers for '%s', maelstrom=%d, stacks=%d",
                       name_str.c_str(), maelstrom, actor -> buff.maelstrom_weapon -> check() );
      }

      timespan_t time_to_next_hit;

      // Non-maelstromable spell finishes casting, reset swing timers
      if ( actor -> main_hand_attack && actor -> main_hand_attack -> execute_event )
      {
        time_to_next_hit = actor -> main_hand_attack -> execute_time();
        actor -> main_hand_attack -> execute_event -> reschedule( time_to_next_hit );
      }

      // Offhand
      if ( actor -> off_hand_attack && actor -> off_hand_attack -> execute_event )
      {
        time_to_next_hit = player -> off_hand_attack -> execute_time();
        actor -> off_hand_attack -> execute_event -> reschedule( time_to_next_hit );
      }
    }
  }
}

// shaman_spell_t::impact ==================================================

void shaman_spell_t::impact_s( action_state_t* state )
{
  spell_t::impact_s( state );

  // Triggers wont happen for procs or totems
  if ( proc || ! callbacks )
    return;

  if ( is_direct_damage() && result_is_hit( state -> result ) && state -> result == RESULT_CRIT )
  {
    // Overloads dont trigger elemental focus
    if ( ! overload && actor -> primary_tree() == TREE_ELEMENTAL )
      actor -> buff.elemental_focus -> trigger( actor -> buff.elemental_focus -> initial_stacks() );
  }
}

// shaman_spell_t::schedule_execute =========================================

void shaman_spell_t::schedule_execute()
{
  if ( sim -> log )
  {
    log_t::output( sim, "%s schedules execute for %s", actor -> name(), name() );
  }

  time_to_execute = execute_time();

  execute_event = new ( sim ) action_execute_event_t( sim, this, time_to_execute );

  if ( ! background )
  {
    actor -> executing = this;
    actor -> gcd_ready = sim -> current_time + gcd();
    if ( actor -> action_queued && sim -> strict_gcd_queue )
    {
      actor -> gcd_ready -= sim -> queue_gcd_reduction;
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
    stateless = true;
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
    if ( actor -> buffs.exhaustion -> check() )
      return false;

    if (  actor -> buffs.bloodlust -> cooldown -> remains() > timespan_t::zero )
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
    maelstrom             = true;
    direct_power_mod     += actor -> spec.shamanism      -> effectN( 2 ).percent();
    base_execute_time    += actor -> spec.shamanism      -> effectN( 3 ).time_value();
    cooldown -> duration += actor -> spec.elemental_fury -> effectN( 2 ).time_value();
    base_multiplier      *= 1.0 + actor -> glyph.chain_lightning   -> effectN( 2 ).percent();
    aoe                   = ( 2 + ( int ) actor -> glyph.chain_lightning -> effectN( 1 ).base_value() );
    base_add_multiplier   = effect_chain_multiplier( 1 );

    overload              = new chain_lightning_overload_t( player );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new chain_lightning_t( actor, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  void impact_s( action_state_t* state )
  {
    shaman_spell_t::impact_s( state );

    actor -> buff.maelstrom_weapon -> expire();

    if ( result_is_hit( state -> result ) )
    {
      trigger_rolling_thunder( this );

      double overload_chance = actor -> composite_mastery() *
                               actor -> mastery.elemental_overload -> effectN( 2 ).base_value() / 10000.0 / 3.0;

      if ( overload_chance && actor -> rng.elemental_overload -> roll( overload_chance ) )
      {
        overload -> execute();
        if ( actor -> set_bonus.tier13_4pc_caster() )
          actor -> buff.tier13_4pc_caster -> trigger();
      }
    }
  }

  virtual timespan_t execute_time() const
  {
    timespan_t t = shaman_spell_t::execute_time();
    t *= 1.0 + actor -> buff.maelstrom_weapon -> stack() * actor -> buff.maelstrom_weapon -> effectN( 1 ).percent();
    return t;
  }

  double cost_reduction() const
  {
    double cr = shaman_spell_t::cost_reduction();
    cr += actor -> buff.maelstrom_weapon -> stack() * actor -> buff.maelstrom_weapon -> effectN( 2 ).percent();
    return cr;
  }
};

// Elemental Mastery Spell ==================================================

struct elemental_mastery_t : public shaman_spell_t
{
  elemental_mastery_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "elemental_mastery", "Elemental Mastery", player, options_str )
  {
    check_talent( actor -> talent.elemental_mastery -> rank() );
    harmful   = false;
    may_crit  = false;
    may_miss  = false;
    stateless = true;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    actor -> buff.elemental_mastery -> trigger();
    if ( actor -> set_bonus.tier13_2pc_caster() )
      actor -> buff.tier13_2pc_caster -> trigger();
  }
};

// Fire Nova Spell ==========================================================

struct fire_nova_explosion_t : public shaman_spell_t
{
  player_t* emit_target;

  fire_nova_explosion_t( player_t* player ) :
    shaman_spell_t( "fire_nova_explosion", 8349, player ),
    emit_target( 0 )
  {
    check_spec( TREE_ENHANCEMENT );
    aoe        = -1;
    background = true;
    callbacks  = false;
    stateless  = true;
  }

  double action_da_multiplier() const
  {
    if ( actor -> buff.unleash_flame -> up() )
      return actor -> buff.unleash_flame -> effectN( 2 ).percent();

    return 0.0;
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
    stateless = true;

    add_child( explosion );
  }

  virtual bool ready()
  {
    shaman_targetdata_t* td = targetdata() -> cast_shaman();

    if ( ! td -> dots_flame_shock -> ticking )
      return false;

    return shaman_spell_t::ready();
  }

  void impact_s( action_state_t* state )
  {
    explosion -> emit_target = state -> target;
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
  struct earthquake_state_t : public shaman_spell_state_t
  {
    earthquake_state_t( action_t* a, player_t* t ) :
      shaman_spell_state_t( a, t ) { }

    inline void snapshot_spell_power()
    {
      actor_spell_power = action -> player -> composite_spell_power( SCHOOL_NATURE );
    }
  };

  earthquake_rumble_t( player_t* player ) :
    shaman_spell_t( "earthquake_rumble", 77478, player )
  {
    harmful = true;
    aoe = -1;
    background = true;
    school = SCHOOL_PHYSICAL;
    stats -> school = SCHOOL_PHYSICAL;
    callbacks = false;
    stateless = true;
    may_resist = false;
  }

  action_state_t* new_state()
  {
    return new earthquake_state_t( this, target );
  }

  double armor() const
  {
    return 0.0;
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
    base_tick_time = timespan_t::from_seconds( 1.0 );
    hasted_ticks = false;
    stateless = true;

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
  struct lava_burst_state_t : public shaman_spell_state_t
  {
    lava_burst_state_t( action_t* a, player_t* t ) :
      shaman_spell_state_t( a, t ) { }

    inline void snapshot_target_crit()
    {
      action_state_t::snapshot_target_crit();

      if ( td -> dots_flame_shock -> ticking )
        target_crit += 1.0;
    }

    inline void snapshot_target_multiplier()
    {
      action_state_t::snapshot_target_multiplier();

      if ( td -> debuffs_unleashed_fury_ft -> up() )
        target_multiplier *= 1.0 + td -> debuffs_unleashed_fury_ft -> effectN( 1 ).percent();
    }
  };

  lava_burst_overload_t* overload;
  double                m_additive;

  lava_burst_t( player_t* player, const std::string& options_str, bool dtr=false ) :
    shaman_spell_t( "lava_burst", "Lava Burst", player, options_str ),
    m_additive( 0 )
  {
    stateless   = true;
    m_additive += actor -> glyph.lava_burst -> effectN( 1 ).percent();
    overload    = new lava_burst_overload_t( player );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new lava_burst_t( actor, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual action_state_t* new_state()
  {
    return new lava_burst_state_t( this, target );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();
    if ( actor -> buff.lava_surge -> check() )
      actor -> buff.lava_surge -> expire();
  }

  virtual timespan_t execute_time() const
  {
    if ( actor -> buff.lava_surge -> up() )
      return timespan_t::zero;

    return shaman_spell_t::execute_time();
  }

  double action_da_multiplier() const
  {
    double m = 1.0 + m_additive;

    if ( actor -> buff.unleash_flame -> up() )
      m += actor -> buff.unleash_flame -> effectN( 2 ).percent();

    return m;
  }

  virtual void impact_s( action_state_t* state )
  {
    shaman_spell_t::impact_s( state );

    if ( result_is_hit( state -> result ) )
    {
      double overload_chance = actor -> composite_mastery() *
                               actor -> mastery.elemental_overload -> effectN( 2 ).base_value() / 10000.0;

      if ( overload_chance && actor -> rng.elemental_overload -> roll( overload_chance ) )
      {
        overload -> execute();
        if ( actor -> set_bonus.tier13_4pc_caster() )
          actor -> buff.tier13_4pc_caster -> trigger();
      }
    }
  }
};

// Lightning Bolt Spell =====================================================

struct lightning_bolt_t : public shaman_spell_t
{
  struct lightning_bolt_state_t : public shaman_spell_state_t
  {
    lightning_bolt_state_t( action_t* a, player_t* t ) :
      shaman_spell_state_t( a, t ) { }

    inline void snapshot_target_multiplier()
    {
      action_state_t::snapshot_target_multiplier();

      if ( td -> debuffs_unleashed_fury_ft -> up() )
        target_multiplier *= 1.0 + td -> debuffs_unleashed_fury_ft -> effectN( 1 ).percent();
    }
  };

  lightning_bolt_overload_t* overload;

  lightning_bolt_t( player_t* player, const std::string& options_str, bool dtr=false ) :
    shaman_spell_t( "lightning_bolt", "Lightning Bolt", player, options_str )
  {
    maelstrom          = true;
    stateless          = true;
    direct_power_mod  += actor -> spec.shamanism -> effectN( 1 ).percent();
    base_multiplier   *= 1.0 + actor -> glyph.telluric_currents -> effectN( 1 ).percent();
    base_execute_time += actor -> spec.shamanism -> effectN( 3 ).time_value();
    base_execute_time *= 1.0 + actor -> glyph.unleashed_lightning -> effectN( 2 ).percent();
    overload           = new lightning_bolt_overload_t( actor );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new lightning_bolt_t( actor, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual action_state_t* new_state()
  {
    return new lightning_bolt_state_t( this, target );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    actor -> buff.maelstrom_weapon -> expire();
  }

  virtual timespan_t execute_time() const
  {
    timespan_t t = shaman_spell_t::execute_time();
    t *= 1.0 + actor -> buff.maelstrom_weapon -> stack() * actor -> buff.maelstrom_weapon -> effectN( 1 ).percent();
    return t;
  }

  virtual double cost_reduction() const
  {
    double   cr = shaman_spell_t::cost_reduction();
    cr += actor -> buff.maelstrom_weapon -> stack() * actor -> buff.maelstrom_weapon -> effectN( 2 ).percent();
    return cr;
  }

  virtual void impact_s( action_state_t* state )
  {
    shaman_spell_t::impact_s( state );

    if ( result_is_hit( state -> result ) )
    {
      trigger_rolling_thunder( this );

      double overload_chance = actor -> composite_mastery() *
                               actor -> mastery.elemental_overload -> effectN( 2 ).base_value() / 10000.0;

      if ( overload_chance && actor -> rng.elemental_overload -> roll( overload_chance ) )
      {
        overload -> execute();
        if ( actor -> set_bonus.tier13_4pc_caster() )
          actor -> buff.tier13_4pc_caster -> trigger();
      }

      if ( actor -> glyph.telluric_currents -> ok() )
      {
        actor -> resource_gain( RESOURCE_MANA,
                                state -> result_amount * actor -> glyph.telluric_currents -> effectN( 2 ).percent(),
                                actor -> gain.telluric_currents );
      }
    }
  }

  virtual bool usable_moving()
  {
    if ( actor -> glyph.unleashed_lightning -> ok() )
      return true;

    return shaman_spell_t::usable_moving();
  }
};

// Elemental Blast Spell ====================================================

struct elemental_blast_t : public shaman_spell_t
{
  elemental_blast_t( player_t* player, const std::string& options_str, bool dtr=false ) :
    shaman_spell_t( "elemental_blast", "Elemental Blast", player, options_str )
  {
    stateless   = true;

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new elemental_blast_t( actor, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact_s( action_state_t* state )
  {
    shaman_spell_t::impact_s( state );

    if ( result_is_hit( state -> result ) )
      actor -> buff.elemental_blast -> trigger();
  }
};


// Shamanisitc Rage Spell ===================================================

struct shamanistic_rage_t : public shaman_spell_t
{
  shamanistic_rage_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "shamanistic_rage", "Shamanistic Rage", player, options_str )
  {
    check_spec( TREE_ENHANCEMENT );

    harmful   = false;
    stateless = true;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    actor -> buff.shamanistic_rage -> trigger();
  }
};

// Spirit Wolf Spell ========================================================

struct spirit_wolf_spell_t : public shaman_spell_t
{
  spirit_wolf_spell_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "spirit_wolf", "Feral Spirit", player, options_str )
  {
    check_spec( TREE_ENHANCEMENT );

    harmful   = false;
    stateless = true;
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    actor -> pet_spirit_wolf -> summon( duration() );
  }
};

// Thunderstorm Spell =======================================================

struct thunderstorm_t : public shaman_spell_t
{
  double bonus;

  thunderstorm_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "thunderstorm", "Thunderstorm", player, options_str ), bonus( 0 )
  {
    check_spec( TREE_ELEMENTAL );

    stateless             = true;
    cooldown -> duration += actor -> glyph.thunder -> effectN( 1 ).time_value();
    bonus                 = effectN( 2 ).percent() +
                            actor -> glyph.thunderstorm -> effectN( 1 ).percent();
  }

  virtual void impact_s( action_state_t* state )
  {
    shaman_spell_t::impact_s( state );

    actor -> resource_gain( effectN( 2 ).resource_gain_type(),
                            actor -> resource_max[ effectN( 2 ).resource_gain_type() ] * bonus,
                            actor -> gain.thunderstorm );
  }
};

// Unleash Elements Spell ===================================================

struct unleash_elements_t : public shaman_spell_t
{
  unleash_wind_t*   wind;
  unleash_flame_t* flame;

  unleash_elements_t( shaman_t* player, const std::string& options_str ) :
    shaman_spell_t( "unleash_elements", "Unleash Elements", player, options_str )
  {
    may_crit    = false;
    may_miss    = false;
    stateless   = true;

    wind        = new unleash_wind_t( player );
    flame       = new unleash_flame_t( player );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    wind  -> execute();
    flame -> execute();

    // You get the buffs, regardless of hit/miss
    if ( actor -> main_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
    {
      actor -> buff.unleash_flame -> trigger();
      if ( actor -> talent.unleashed_fury -> ok() )
        actor -> buff.unleashed_fury_ft -> trigger();
    }
    else if ( actor -> main_hand_weapon.buff_type == WINDFURY_IMBUE )
    {
      actor -> buff.unleash_wind -> trigger( actor -> buff.unleash_wind -> initial_stacks() );
      if ( actor -> talent.unleashed_fury -> ok() )
        actor -> buff.unleashed_fury_wf -> trigger();
    }

    if ( actor -> off_hand_weapon.type != WEAPON_NONE )
    {
      if ( actor -> off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
        actor -> buff.unleash_flame -> trigger();
      else if ( actor -> off_hand_weapon.buff_type == WINDFURY_IMBUE )
        actor -> buff.unleash_wind -> trigger( actor -> buff.unleash_wind -> initial_stacks() );
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
      sim -> add_event( this, timespan_t::from_seconds( 0.001 ) );
    }

    void execute()
    {
      if ( ( buff -> check() - consume_threshold ) > 0 )
        buff -> decrement( consume_stacks );
    }
  };

  int consume_threshold;

  earth_shock_t( player_t* player, const std::string& options_str, bool dtr=false ) :
    shaman_spell_t( "earth_shock", "Earth Shock", player, options_str ),
    consume_threshold( ( int ) actor -> spec.fulmination -> effectN( 1 ).base_value() )
  {
    stateless            = true;
    cooldown             = actor -> cooldown.shock;
    cooldown -> duration = spell_id_t::cooldown();

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new earth_shock_t( actor, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    if ( consume_threshold == 0 )
      return;

    if ( result_is_hit( state -> result ) )
    {
      int consuming_stacks = actor -> buff.lightning_shield -> stack() - consume_threshold;
      if ( consuming_stacks > 0 && ! is_dtr_action )
      {
        actor -> active_lightning_charge -> execute();
        if ( ! is_dtr_action )
          new ( actor -> sim ) lightning_charge_delay_t( actor -> sim, actor, actor -> buff.lightning_shield, consuming_stacks, consume_threshold );
        actor -> proc.fulmination[ consuming_stacks ] -> occur();
      }
    }
  }
};

// Flame Shock Spell ========================================================

struct flame_shock_t : public shaman_spell_t
{
  flame_shock_t( player_t* player, const std::string& options_str, bool dtr=false ) :
    shaman_spell_t( "flame_shock", "Flame Shock", player, options_str )
  {
    may_trigger_dtr       = false; // Disable the dot ticks procing DTR
    tick_may_crit         = true;
    stateless             = true;
    dot_behavior          = DOT_REFRESH;
    num_ticks             = ( int ) floor( ( ( double ) num_ticks ) * ( 1.0 + actor -> glyph.flame_shock -> effectN( 1 ).percent() ) );
    cooldown              = actor -> cooldown.shock;
    cooldown -> duration  = spell_id_t::cooldown();
    base_dd_min          += actor -> glyph.flame_shock -> effectN( 2 ).base_value();

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new flame_shock_t( actor, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  double action_da_multiplier() const
  {
    double m = 1.0;

    if ( actor -> buff.unleash_flame -> up() )
      m += actor -> buff.unleash_flame -> effectN( 2 ).percent();

    return m;
  }

  double action_ta_multiplier() const
  {
    double m = 1.0;

    if ( actor -> buff.unleash_flame -> up() )
      m += actor -> buff.unleash_flame -> effectN( 3 ).percent();

    return m;
  }

  virtual void tick( dot_t* d )
  {
    shaman_spell_t::tick( d );

    if ( actor -> rng.lava_surge -> roll ( actor -> spec.lava_surge -> proc_chance() ) )
    {
      actor -> proc.lava_surge -> occur();
      actor -> cooldown.lava_burst -> reset();
    }
  }
};

// Frost Shock Spell ========================================================

struct frost_shock_t : public shaman_spell_t
{
  frost_shock_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "frost_shock", "Frost Shock", player )
  {
    parse_options( NULL, options_str );

    stateless            = true;
    cooldown             = actor -> cooldown.shock;
    cooldown -> duration = spell_id_t::cooldown();
  }
};

// Wind Shear Spell =========================================================

struct wind_shear_t : public shaman_spell_t
{
  wind_shear_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "wind_shear", "Wind Shear", player, options_str )
  {
    may_miss = may_resist = may_crit = false;
    stateless = true;
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
    is_totem             = true;
    harmful              = false;
    hasted_ticks         = false;
    callbacks            = false;
    totem_duration       = duration();
    // Model all totems as ticking "dots" for now, this will cause them to properly
    // "fade", so we can recast them if the fight length is long enough and optimal_raid=0
    num_ticks            = 1;
    base_tick_time       = totem_duration;
  }

  // Simulate a totem "drop", this is a simplified action_t::execute()
  virtual void execute()
  {
    if ( actor -> totems[ totem ] )
    {
      actor -> totems[ totem ] -> cancel();
      actor -> totems[ totem ] -> dot() -> cancel();
    }

    if ( sim -> log )
      log_t::output( sim, "%s performs %s", actor -> name(), name() );

    result = RESULT_HIT; tick_dmg = 0; direct_dmg = 0;

    consume_resource();
    update_ready();
    schedule_travel( target );

    actor -> totems[ totem ] = this;

    stats -> add_execute( time_to_execute );
  }

  virtual void last_tick( dot_t* d )
  {
    shaman_spell_t::last_tick( d );

    if ( sim -> log )
      log_t::output( sim, "%s destroys %s", actor -> name(), actor -> totems[ totem ] -> name() );
    actor -> totems[ totem ] = 0;
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
        direct_dmg = calculate_direct_damage( 0 );

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
    if ( actor -> totems[ totem ] )
      return false;

    return spell_t::ready();
  }

  virtual void reset()
  {
    shaman_spell_t::reset();

    if ( actor -> totems[ totem ] == this )
      actor -> totems[ totem ] = 0;
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

    actor -> pet_earth_elemental -> summon();
  }

  virtual void last_tick( dot_t* d )
  {
    shaman_totem_t::last_tick( d );

    actor -> pet_earth_elemental -> dismiss();
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
    cooldown -> duration *= 1.0 - actor -> glyph.fire_elemental_totem -> effectN( 1 ).percent();
    totem_duration       *= 1.0 - actor -> glyph.fire_elemental_totem -> effectN( 2 ).percent();
    // Skip a pointless cancel call (and debug=1 cancel line)
    dot_behavior = DOT_REFRESH;
  }

  virtual void execute()
  {
    shaman_totem_t::execute();
    actor -> pet_fire_elemental -> summon();
  }

  virtual void last_tick( dot_t* d )
  {
    shaman_totem_t::last_tick( d );
    actor -> pet_fire_elemental -> dismiss();
  }

  // Allow Fire Elemental Totem to override any active fire totems
  virtual bool ready()
  {
    return shaman_spell_t::ready();
  }
};

// Magma Totem Spell ========================================================

struct magma_totem_t : public shaman_totem_t
{
  magma_totem_t( player_t* player, const std::string& options_str ) :
    shaman_totem_t( "magma_totem", "Magma Totem", player, options_str, TOTEM_FIRE )
  {
    const spell_data_t* trigger;

    aoe               = -1;
    harmful           = true;
    may_crit          = true;
    // Magma Totem is not a real DoT, but rather a pet that is spawned.

    // Spell id 8188 does the triggering of magma totem's aura
    base_tick_time    = actor -> dbc.spell( 8188 ) -> effectN( 1 ).period();
    num_ticks         = ( int ) ( totem_duration / base_tick_time );

    // Fill out scaling data
    trigger           = actor -> dbc.spell( actor -> dbc.spell( 8188 ) -> effectN( 1 ).trigger_spell_id() );
    // Also kludge totem school to fire for accurate damage
    school            = spell_id_t::get_school_type( trigger -> school_mask() );
    stats -> school   = school;

    base_dd_min       = actor -> dbc.effect_min( trigger -> effectN( 1 ).id(), actor -> level );
    base_dd_max       = actor -> dbc.effect_max( trigger -> effectN( 1 ).id(), actor -> level );
    direct_power_mod  = trigger -> effectN( 1 ).coeff();
  }

  virtual void execute()
  {
    player_buff();
    shaman_totem_t::execute();
  }
};

// Mana Tide Totem Spell ====================================================

struct mana_tide_totem_t : public shaman_totem_t
{
  mana_tide_totem_t( player_t* player, const std::string& options_str ) :
    shaman_totem_t( "mana_tide_totem", "Mana Tide Totem", player, options_str, TOTEM_WATER )
  {
    check_spec( TREE_RESTORATION );
    // Mana tide effect bonus is in a separate spell, we dont need other info
    // from there anymore, as mana tide does not pulse anymore
    totem_bonus  = actor -> dbc.spell( 16191 ) -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    shaman_totem_t::execute();

    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      // Change buff duration based on totem duration
      p -> buffs.mana_tide -> buff_duration = totem_duration;
      p -> buffs.mana_tide -> trigger( 1, totem_bonus );
    }

    update_ready();
  }

  virtual bool ready()
  {
    if ( ! shaman_spell_t::ready() )
      return false;

    return ( actor -> resource_current[ RESOURCE_MANA ] < ( 0.75 * actor -> resource_max[ RESOURCE_MANA ] ) );
  }
};

// Searing Totem Spell ======================================================

struct searing_totem_t : public shaman_totem_t
{
  searing_totem_t( player_t* player, const std::string& options_str ) :
    shaman_totem_t( "searing_totem", "Searing Totem", player, options_str, TOTEM_FIRE )
  {
    harmful   = true;
    may_crit  = true;

    // TODO: Make sure Searing Totem scaling information is now correct
    base_dd_min      = actor -> dbc.effect_min( actor -> dbc.spell( 3606 ) -> effectN( 1 ).id(), actor -> level );
    base_dd_max      = actor -> dbc.effect_max( actor -> dbc.spell( 3606 ) -> effectN( 1 ).id(), actor -> level );
    direct_power_mod = actor -> dbc.spell( 3606 ) -> effectN( 1 ).coeff();
    // TODO: Is the shoot time of Searing Totme now 1.5 seconds, like spell data says?
    base_tick_time   = timespan_t::from_seconds( 1.6 );
    travel_speed     = 0; // TODO: Searing bolt has a real travel time, however modeling it is another issue entirely
    range            = actor -> dbc.spell( 3606 ) -> max_range();
    num_ticks        = ( int ) ( totem_duration / base_tick_time );
    // Also kludge totem school to fire
    school           = spell_id_t::get_school_type( actor -> dbc.spell( 3606 ) -> school_mask() );
    stats -> school  = school;
    actor -> active_searing_flames_dot = new searing_flames_t( actor );
  }

  virtual void tick( dot_t* d )
  {
    shaman_targetdata_t* td = targetdata() -> cast_shaman();
    shaman_totem_t::tick( d );
    if ( result_is_hit() && td -> debuffs_searing_flames -> trigger() )
    {
      double new_base_td = tick_dmg;
      // Searing flame dot treats all hits as.. HITS
      if ( result == RESULT_CRIT )
        new_base_td /= 1.0 + total_crit_bonus();

      actor -> active_searing_flames_dot -> base_td = new_base_td / actor -> active_searing_flames_dot -> num_ticks;
      actor -> active_searing_flames_dot -> execute();
    }
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
      sim -> errorf( "Player %s: flametongue_weapon: weapon option must be one of main/off/both\n", actor -> name() );
      assert( 0 );
    }

    // Spell damage scaling is defined in "Flametongue Weapon (Passive), id 10400"
    bonus_power  = actor -> dbc.spell( 10400 ) -> effectN( 2 ).percent();
    harmful      = false;
    may_miss     = false;

    if ( main )
      actor -> flametongue_mh = new flametongue_weapon_spell_t( actor, "flametongue_mh", &( actor -> main_hand_weapon ) );

    if ( off )
      actor -> flametongue_oh = new flametongue_weapon_spell_t( actor, "flametongue_oh", &( actor -> off_hand_weapon ) );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    if ( main )
    {
      actor -> main_hand_weapon.buff_type  = FLAMETONGUE_IMBUE;
      actor -> main_hand_weapon.buff_value = bonus_power;
    }
    if ( off )
    {
      actor -> off_hand_weapon.buff_type  = FLAMETONGUE_IMBUE;
      actor -> off_hand_weapon.buff_value = bonus_power;
    }
  };

  virtual bool ready()
  {
    if ( main && ( actor -> main_hand_weapon.buff_type != FLAMETONGUE_IMBUE ) )
      return true;

    if ( off && ( actor -> off_hand_weapon.buff_type != FLAMETONGUE_IMBUE ) )
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
    shaman_spell_t( "windfury_weapon", "Windfury Weapon", player ),
    bonus_power( 0 ), main( 0 ), off( 0 )
  {
    check_spec( TREE_ENHANCEMENT );
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
      sim -> errorf( "Player %s: windfury_weapon: weapon option must be one of main/off/both\n", actor -> name() );
      sim -> cancel();
    }

    bonus_power  = actor -> dbc.effect_average( actor -> dbc.spell( id ) -> effectN( 1 ).id(), actor -> level );
    harmful      = false;
    may_miss     = false;

    if ( main )
      actor -> windfury_mh = new windfury_weapon_attack_t( actor, "windfury_mh", &( actor -> main_hand_weapon ) );

    if ( off )
      actor -> windfury_oh = new windfury_weapon_attack_t( actor, "windfury_oh", &( actor -> off_hand_weapon ) );
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    if ( main )
    {
      actor -> main_hand_weapon.buff_type  = WINDFURY_IMBUE;
      actor -> main_hand_weapon.buff_value = bonus_power;
    }
    if ( off )
    {
      actor -> off_hand_weapon.buff_type  = WINDFURY_IMBUE;
      actor -> off_hand_weapon.buff_value = bonus_power;
    }
  };

  virtual bool ready()
  {
    if ( main && ( actor -> main_hand_weapon.buff_type != WINDFURY_IMBUE ) )
      return true;

    if ( off && ( actor -> off_hand_weapon.buff_type != WINDFURY_IMBUE ) )
      return true;

    return false;
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
    parse_options( NULL, options_str );

    harmful     = false;

    actor -> active_lightning_charge = new lightning_charge_t( actor, actor -> primary_tree() == TREE_ELEMENTAL ? "fulmination" : "lightning_shield" );

  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    actor -> buff.water_shield     -> expire();
    actor -> buff.lightning_shield -> trigger( initial_stacks() );
  }

  virtual bool ready()
  {
    if ( actor -> buff.lightning_shield -> check() )
      return false;

    return shaman_spell_t::ready();
  }
};

// Water Shield Spell =======================================================

struct water_shield_t : public shaman_spell_t
{
  double bonus;

  water_shield_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "water_shield", "Water Shield", player ), bonus( 0.0 )
  {
    parse_options( NULL, options_str );

    harmful      = false;
    bonus        = actor -> dbc.effect_average( actor -> dbc.spell( id ) -> effectN( 2 ).id(), actor -> level );
    bonus       *= 1.0 + actor -> glyph.water_shield -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    actor -> buff.lightning_shield -> expire();
    actor -> buff.water_shield -> trigger( initial_stacks(), bonus );
  }

  virtual bool ready()
  {
    if ( actor -> buff.water_shield -> check() )
      return false;
    return shaman_spell_t::ready();
  }
};

struct spiritwalkers_grace_t : public shaman_spell_t
{
  spiritwalkers_grace_t( player_t* player, const std::string& options_str ) :
    shaman_spell_t( "spiritwalkers_grace", "Spiritwalker's Grace", player )
  {
    parse_options( NULL, options_str );

    may_miss  = may_crit = harmful = callbacks = false;

    // Extend buff duration with t13 4pc healer
    actor -> buff.spiritwalkers_grace -> buff_duration = actor -> buff.spiritwalkers_grace -> duration() +
                                                         actor -> glyph.spiritwalkers_grace -> effectN( 1 ).time_value() + 
                                                         actor -> sets -> set( SET_T13_4PC_HEAL ) -> effectN( 1 ).time_value();
  }

  virtual void execute()
  {
    shaman_spell_t::execute();

    actor -> buff.spiritwalkers_grace -> trigger();
    if ( actor -> set_bonus.tier13_4pc_heal() )
      actor -> buff.tier13_4pc_healer -> trigger();
  }
};

// ==========================================================================
// Shaman Passive Buffs
// ==========================================================================

struct lightning_shield_buff_t : public buff_t
{
  lightning_shield_buff_t( player_t* p, uint32_t id, const char* n ) :
    buff_t( p, id, n, 1.0, timespan_t::min, false )
  {
    shaman_t* s = player -> cast_shaman();

    if ( s -> primary_tree() == TREE_ELEMENTAL )
      max_stack = ( int ) s -> spec.rolling_thunder -> effectN( 1 ).base_value();

    // Reinit because of max_stack change
    init_buff_shared();
  }
};

struct searing_flames_buff_t : public buff_t
{
  searing_flames_buff_t( actor_pair_t pair, uint32_t id, const char* n ) :
    buff_t( pair, id, n, 1.0, timespan_t::min, true ) // Quiet buff, dont show in report
  {
    default_chance = player -> dbc.spell( 77661 ) -> proc_chance();
    buff_duration  = player -> dbc.spell( 77661 ) -> duration();
    max_stack      = player -> dbc.spell( 77661 ) -> max_stacks();

    // Reinit because of max_stack change
    init_buff_shared();
  }
};

struct unleash_flame_buff_t : public buff_t
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
    buff -> buff_t::expire();
    buff -> expiration_delay = 0;
  }
};

unleash_flame_buff_t::unleash_flame_buff_t( player_t* p ) :
  buff_t( p, 73683, "unleash_flame" ), expiration_delay( 0 )
{
}

void unleash_flame_buff_t::reset()
{
  buff_t::reset();
  event_t::cancel( expiration_delay );
}

void unleash_flame_buff_t::expire()
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
    buff_t::expire();
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
  if ( name == "elemental_blast"         ) return new          elemental_blast_t( this, options_str );
  if ( name == "earth_elemental_totem"   ) return new    earth_elemental_totem_t( this, options_str );
  if ( name == "earth_shock"             ) return new              earth_shock_t( this, options_str );
  if ( name == "earthquake"              ) return new               earthquake_t( this, options_str );
  if ( name == "elemental_mastery"       ) return new        elemental_mastery_t( this, options_str );
  if ( name == "fire_elemental_totem"    ) return new     fire_elemental_totem_t( this, options_str );
  if ( name == "fire_nova"               ) return new                fire_nova_t( this, options_str );
  if ( name == "flame_shock"             ) return new              flame_shock_t( this, options_str );
  if ( name == "flametongue_weapon"      ) return new       flametongue_weapon_t( this, options_str );
  if ( name == "frost_shock"             ) return new              frost_shock_t( this, options_str );
  if ( name == "lava_burst"              ) return new               lava_burst_t( this, options_str );
  if ( name == "lava_lash"               ) return new                lava_lash_t( this, options_str );
  if ( name == "lightning_bolt"          ) return new           lightning_bolt_t( this, options_str );
  if ( name == "lightning_shield"        ) return new         lightning_shield_t( this, options_str );
  if ( name == "magma_totem"             ) return new              magma_totem_t( this, options_str );
  if ( name == "mana_tide_totem"         ) return new          mana_tide_totem_t( this, options_str );
  if ( name == "primal_strike"           ) return new            primal_strike_t( this, options_str );
  if ( name == "searing_totem"           ) return new            searing_totem_t( this, options_str );
  if ( name == "shamanistic_rage"        ) return new         shamanistic_rage_t( this, options_str );
  if ( name == "spirit_wolf"             ) return new        spirit_wolf_spell_t( this, options_str );
  if ( name == "spiritwalkers_grace"     ) return new      spiritwalkers_grace_t( this, options_str );
  if ( name == "stormstrike"             ) return new              stormstrike_t( this, options_str );
  if ( name == "thunderstorm"            ) return new             thunderstorm_t( this, options_str );
  if ( name == "unleash_elements"        ) return new         unleash_elements_t( this, options_str );
  if ( name == "water_shield"            ) return new             water_shield_t( this, options_str );
  if ( name == "wind_shear"              ) return new               wind_shear_t( this, options_str );
  if ( name == "windfury_weapon"         ) return new          windfury_weapon_t( this, options_str );

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
  talent.call_of_the_elements = find_talent( "Call of the Elements" );
  talent.totemic_restoration  = find_talent( "Totemic Restoration"  );

  talent.elemental_mastery    = find_talent( "Elemental Mastery"    );
  talent.ancestral_swiftness  = find_talent( "Ancestral Swiftness"  );
  talent.echo_of_the_elements = find_talent( "Echo of the Elements" );

  talent.unleashed_fury       = find_talent( "Unleashed Fury"       );
  talent.primal_elementalist  = find_talent( "Primal Elementalist"  );
  talent.elemental_blast      = find_talent( "Elemental Blast"      );

  player_t::init_talents();
}

// shaman_t::init_spells ====================================================

void shaman_t::init_spells()
{
  // New set bonus system
  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //   C2P     C4P     M2P     M4P    T2P    T4P     H2P     H4P
    { 105780, 105816, 105866, 105872,     0,     0, 105764, 105876 }, // Tier13
    {      0,      0,      0,      0,     0,     0,      0,      0 },
  };
  sets                               = new set_bonus_array_t( this, set_bonuses );

  // Generic
  spec.mail_specialization = find_specialization_spell( "Mail Specialization", "", ( talent_tree_type ) primary_tree() );

  // Elemental
  spec.elemental_focus     = find_specialization_spell( "Elemental Focus" );
  spec.elemental_fury      = find_specialization_spell( "Elemental Fury" );
  spec.elemental_precision = find_specialization_spell( "Elemental Precision" );
  spec.fulmination         = find_specialization_spell( "Fulmination" );
  spec.lava_surge          = find_specialization_spell( "Lava Surge" );
  spec.rolling_thunder     = find_specialization_spell( "Rolling Thunder" );
  spec.shamanism           = find_specialization_spell( "Shamanism" );
  spec.spiritual_insight   = find_specialization_spell( "Spiritual Insight" );

  // Enhancement
  spec.dual_wield          = find_specialization_spell( "Dual Wield" );
  spec.flurry              = find_specialization_spell( "Flurry" );
  spec.maelstrom_weapon    = find_specialization_spell( "Maelstrom Weapon" );
  spec.mental_quickness    = find_specialization_spell( "Mental Quickness" );
  spec.primal_wisdom       = find_specialization_spell( "Primal Wisdom" );
  spec.shamanistic_rage    = find_specialization_spell( "Shamanistic Rage" );
  spec.static_shock        = find_specialization_spell( "Static Shock" );

  // Masteries
  mastery.elemental_overload         = find_mastery_spell( "Elemental Overload" );
  mastery.enhanced_elements          = find_mastery_spell( "Enhanced Elements" );

  // Glyphs
  glyph.chain_lightning              = find_glyph( "Glyph of Chain Lightning" );
  glyph.fire_elemental_totem         = find_glyph( "Glyph of Fire Elemental Totem" );
  glyph.flame_shock                  = find_glyph( "Glyph of Flame Shock" );
  glyph.lava_burst                   = find_glyph( "Glyph of Lava Burst" );
  glyph.lava_lash                    = find_glyph( "Glyph of Lava Lash" );
  glyph.spiritwalkers_grace          = find_glyph( "Glyph of Spiritwalker's Grace" );
  glyph.telluric_currents            = find_glyph( "Glyph of Telluric Currents" );
  glyph.thunder                      = find_glyph( "Glyph of Thunder" );
  glyph.thunderstorm                 = find_glyph( "Glyph of Thunderstorm" );
  glyph.unleashed_lightning          = find_glyph( "Glyph of Unleashed Lightning" );
  glyph.water_shield                 = find_glyph( "Glyph of Water Shield" );

  player_t::init_spells();

}

// shaman_t::init_base ======================================================

void shaman_t::init_base()
{
  player_t::init_base();

  base_attack_power = ( level * 2 ) - 30;
  initial_attack_power_per_strength = 1.0;
  initial_attack_power_per_agility  = 2.0;
  initial_spell_power_per_intellect = 1.0;

  mana_per_intellect = 15;

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
  if ( spec.elemental_precision -> ok() && sim -> scaling -> scale_stat == STAT_SPIRIT )
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

  // buff_t( player, name, max_stack, duration, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, id, name, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )
  // buff_t( player, name, spellname, chance=-1, cd=-1, quiet=false, reverse=false, activated=true )

  buff.elemental_focus     = new buff_t                 ( this, spec.elemental_focus -> effect_trigger_spell( 1 ), "elemental_focus"           );
  buff.elemental_focus -> activated = false;
  buff.elemental_mastery   = new buff_t                 ( this, talent.elemental_mastery -> spell_id(),                       "elemental_mastery",   1.0 );
  buff.flurry              = new buff_t                 ( this, spec.flurry -> effect_trigger_spell( 1 ),           "flurry",              1.0 );
  buff.flurry -> activated = false;
  // TBD how this is handled for reals
  buff.lava_surge          = new buff_t                 ( this, spec.lava_surge -> spell_id(),                      "lava_surge",          1.0, timespan_t::min, true );
  buff.lava_surge -> activated = false;
  buff.lightning_shield    = new lightning_shield_buff_t( this, dbc.class_ability_id( type, "Lightning Shield" ),             "lightning_shield"         );
  buff.maelstrom_weapon    = new buff_t                 ( this, spec.maelstrom_weapon -> effect_trigger_spell( 1 ), "maelstrom_weapon"         );
  buff.maelstrom_weapon -> activated = false;
  buff.shamanistic_rage    = new buff_t                 ( this, spec.shamanistic_rage -> spell_id(),                "shamanistic_rage"         );
  buff.spiritwalkers_grace = new buff_t                 ( this, dbc.class_ability_id( type, "Spiritwalker's Grace" ),         "spiritwalkers_grace", 1.0, timespan_t::zero );
  buff.unleash_flame       = new unleash_flame_buff_t   ( this );
  buff.unleash_wind        = new buff_t                 ( this, 73681,                                                        "unleash_wind"             );
  buff.water_shield        = new buff_t                 ( this, dbc.class_ability_id( type, "Water Shield" ),                 "water_shield"             );

  // Elemental Tier13 set bonuses
  buff.tier13_2pc_caster   = new stat_buff_t            ( this, 105779, "tier13_2pc_caster", STAT_MASTERY_RATING, dbc.spell( 105779 ) -> effect1().base_value() );
  buff.tier13_4pc_caster   = new stat_buff_t            ( this, 105821, "tier13_4pc_caster", STAT_HASTE_RATING,   dbc.spell( 105821 ) -> effect1().base_value() );
  buff.tier13_4pc_healer   = new buff_t                 ( this, 105877, "tier13_4pc_healer" );

  // MoP stuff
  buff.elemental_blast     = new buff_t                 ( this, 118522, "elemental_blast" );
  buff.unleashed_fury_wf   = new buff_t                 ( this, 118472, "unleashed_fury_wf" );
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

  gain.primal_wisdom        = get_gain( "primal_wisdom"     );
  gain.rolling_thunder      = get_gain( "rolling_thunder"   );
  gain.telluric_currents    = get_gain( "telluric_currents" );
  gain.thunderstorm         = get_gain( "thunderstorm"      );
  gain.water_shield         = get_gain( "water_shield"      );
}

// shaman_t::init_procs =====================================================

void shaman_t::init_procs()
{
  player_t::init_procs();

  proc.elemental_overload = get_proc( "elemental_overload"      );
  proc.lava_surge         = get_proc( "lava_surge"              );
  proc.maelstrom_weapon   = get_proc( "maelstrom_weapon"        );
  proc.static_shock       = get_proc( "static_shock"            );
  proc.swings_clipped_mh  = get_proc( "swings_clipped_mh"       );
  proc.swings_clipped_oh  = get_proc( "swings_clipped_oh"       );
  proc.rolling_thunder    = get_proc( "rolling_thunder"         );
  proc.wasted_ls          = get_proc( "wasted_lightning_shield" );
  proc.wasted_mw          = get_proc( "wasted_maelstrom_weapon" );
  proc.windfury           = get_proc( "windfury"                );

  for ( int i = 0; i < 7; i++ )
  {
    proc.fulmination[ i ] = get_proc( "fulmination_" + util_t::to_string( i ) );
  }

  for ( int i = 0; i < 6; i++ )
  {
    proc.maelstrom_weapon_used[ i ] = get_proc( "maelstrom_weapon_stack_" + util_t::to_string( i ) );
  }
}

// shaman_t::init_rng =======================================================

void shaman_t::init_rng()
{
  player_t::init_rng();
  rng.elemental_overload   = get_rng( "elemental_overload"   );
  rng.lava_surge           = get_rng( "lava_surge"           );
  rng.primal_wisdom        = get_rng( "primal_wisdom"        );
  rng.rolling_thunder      = get_rng( "rolling_thunder"      );
  rng.searing_flames       = get_rng( "searing_flames"       );
  rng.static_shock         = get_rng( "static_shock"         );
  rng.windfury_delay       = get_rng( "windfury_delay"       );
  rng.windfury_weapon      = get_rng( "windfury_weapon"      );
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
      action_list_str += "/stormstrike";
      action_list_str += "/lava_lash";
      action_list_str += "/lightning_bolt,if=buff.maelstrom_weapon.react=5|(set_bonus.tier13_4pc_melee=1&buff.maelstrom_weapon.react>=4&pet.spirit_wolf.active)";
      if ( level > 80 )
      {
        action_list_str += "/unleash_elements";
      }
      action_list_str += "/flame_shock,if=!ticking|buff.unleash_flame.up";
      action_list_str += "/earth_shock";
      action_list_str += "/spirit_wolf";
      action_list_str += "/earth_elemental_totem";
      action_list_str += "/fire_nova,if=target.adds>1";
      action_list_str += "/spiritwalkers_grace,moving=1";
      action_list_str += "/lightning_bolt,if=buff.maelstrom_weapon.react>1";
    }
    else
    {
      int sp_threshold = 0;
      sp_threshold = ( ( has_power_torrent || has_lightweave )?1:0 ) * 500 + ( has_dmc_volcano?1:0 ) * 1600 + ( has_will_of_unbinding?1:0 ) * 700;

      action_list_str  = "flask,type=draconic_mind/food,type=seafood_magnifique_feast";

      action_list_str += "/flametongue_weapon,weapon=main/lightning_shield";
      action_list_str += "/snapshot_stats";
      action_list_str += "/volcanic_potion,if=!in_combat|buff.bloodlust.react|target.time_to_die<=40";

      action_list_str += "/wind_shear";
      action_list_str += "/bloodlust,health_percentage<=25/bloodlust,if=target.time_to_die<=60";
      int num_items = ( int ) items.size();
      for ( int i=0; i < num_items; i++ )
      {
        if ( ! items[ i ].use.active() ) continue;

        int duration = 0;

        // Fiery Quintessence / Bottled Wishes are aligned to fire elemental and
        // only used when the required temporary spell power threshold is exceeded
        if ( util_t::str_compare_ci( items[ i ].name(), "fiery_quintessence" ) )
          duration = 25;
        else if ( util_t::str_compare_ci( items[ i ].name(), "bottled_wishes" ) )
          duration = 15;

        action_list_str += "/use_item,name=" + std::string( items[ i ].name() );

        if ( duration > 0 )
        {
          action_list_str += ",if=(cooldown.fire_elemental_totem.remains=0&temporary_bonus.spell_power>=" +
                             util_t::to_string( sp_threshold ) +
                             ")";
        }
      }
      action_list_str += init_use_profession_actions();
      action_list_str += init_use_racial_actions();

      if ( talent.elemental_mastery -> rank() )
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

      if ( ! glyph.unleashed_lightning -> ok() )
        action_list_str += "/unleash_elements,moving=1";
      action_list_str += "/flame_shock,if=!ticking|ticks_remain<2|((buff.bloodlust.react|buff.elemental_mastery.up)&ticks_remain<3)";
      if ( level >= 75 ) action_list_str += "/lava_burst,if=dot.flame_shock.remains>cast_time";
      if ( spec.fulmination -> ok() )
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
      sp_threshold += ( has_fiery_quintessence?1:0 ) * 1149;
      sp_threshold += ( has_bottled_wishes?1:0 ) * 2290;

      action_list_str += "/earth_elemental_totem,if=!ticking";
      action_list_str += "/searing_totem";

      if ( glyph.unleashed_lightning -> ok() )
      {
        action_list_str += "/spiritwalkers_grace,moving=1,if=cooldown.lava_burst.remains=0";
      }
      else
        action_list_str += "/spiritwalkers_grace,moving=1";

      action_list_str += "/chain_lightning,if=target.adds>2";
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
         resource_available( swg -> current_resource(), swg -> cost() ) )
    {
      // Elemental has to do some additional checking here
      if ( primary_tree() == TREE_ELEMENTAL )
      {
        // Elemental executes SWG mid-cast during a movement event, if
        // 1) The profile does not have Glyph of Unleashed Lightning and is executing
        //    a Lighting Bolt
        // 2) The profile is executing a Lava Burst
        // 3) The profile is casting Chain Lightning
        if ( ( ! glyph.unleashed_lightning -> ok() && executing -> id == 403 ) ||
             ( executing -> id == 51505 ) ||
             ( executing -> id == 421 ) )
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
  if ( attr == ATTR_AGILITY || attr == ATTR_INTELLECT )
    return spec.mail_specialization -> effectN( 1 ).percent();

  return 0.0;
}

// shaman_t::composite_spell_hit ============================================

double shaman_t::composite_spell_hit() const
{
  double hit = player_t::composite_spell_hit();

  hit += ( spec.elemental_precision -> ok() *
    ( spirit() - attribute_base[ ATTR_SPIRIT ] ) ) / rating.spell_hit;

  return hit;
}

// shaman_t::composite_attack_speed =========================================

double shaman_t::composite_attack_speed() const
{
  double speed = player_t::composite_attack_speed();

  if ( buff.flurry -> up() )
    speed *= 1.0 / ( 1.0 + buff.flurry -> effectN( 1 ).percent() );

  if ( buff.unleash_wind -> up() )
    speed *= 1.0 / ( 1.0 + buff.unleash_wind -> effectN( 2 ).percent() );

  return speed;
}

// shaman_t::composite_spell_power ==========================================

double shaman_t::composite_spell_power( const school_type school ) const
{
  double sp = 0;

  if ( primary_tree() == TREE_ENHANCEMENT )
    sp = composite_attack_power_multiplier() * composite_attack_power() * spec.mental_quickness -> effectN( 1 ).percent();
  else
    sp = player_t::composite_spell_power( school );

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

  if ( school != SCHOOL_PHYSICAL && school != SCHOOL_BLEED )
  {
    if ( main_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
      m *= 1.0 + main_hand_weapon.buff_value;
    else if ( off_hand_weapon.buff_type == FLAMETONGUE_IMBUE )
      m *= 1.0 + off_hand_weapon.buff_value;
  }

  return m;
}

// shaman_t::regen  =========================================================

void shaman_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  if ( buff.water_shield -> up() )
  {
    double water_shield_regen = periodicity.total_seconds() * buff.water_shield -> base_value() / 5.0;

    resource_gain( RESOURCE_MANA, water_shield_regen, gain.water_shield );
  }
}

// shaman_t::combat_begin ===================================================

void shaman_t::combat_begin()
{
  player_t::combat_begin();

  // TODO: Trigger Shaman auras based on specialization
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

#endif // SC_SHAMAN

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_shaman  =================================================

player_t* player_t::create_shaman( sim_t* sim, const std::string& name, race_type r )
{
  SC_CREATE_SHAMAN( sim, name, r );
}

// player_t::shaman_init ====================================================

void player_t::shaman_init( sim_t* sim )
{
  sim -> auras.burning_wrath    = new aura_t( sim, "burning_wrath",    1, timespan_t::zero );
  sim -> auras.grace_of_air     = new aura_t( sim, "grace_of_air",     1, timespan_t::zero );

  for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    p -> buffs.bloodlust  = new buff_t( p, "bloodlust", 1, timespan_t::from_seconds( 40.0 ) );
    p -> buffs.exhaustion = new buff_t( p, "exhaustion", 1, timespan_t::from_seconds( 600.0 ), timespan_t::zero, 1.0, true );
    p -> buffs.mana_tide  = new buff_t( p, 16190, "mana_tide" );
  }
}

// player_t::shaman_combat_begin ============================================

void player_t::shaman_combat_begin( sim_t* sim )
{
  if ( sim -> overrides.burning_wrath ) sim -> auras.burning_wrath -> override( 1, 0.10 );
  if ( sim -> overrides.grace_of_air  ) sim -> auras.grace_of_air  -> override( 1, 5.0  );
}

#if SC_SHAMAN == 1

shaman_targetdata_t::shaman_targetdata_t( player_t* source, player_t* target )
  : targetdata_t( source, target )
{
  shaman_t* p = source -> cast_shaman();
  debuffs_searing_flames = add_aura( new searing_flames_buff_t( this, p -> dbc.specialization_ability_id( p -> type, "Searing Flames" ), "searing_flames" ) );
  debuffs_stormstrike    = add_aura( new buff_t( this, p -> dbc.specialization_ability_id( p -> type, "Stormstrike" ), "stormstrike" ) );
  debuffs_unleashed_fury_ft = add_aura( new buff_t( this, 118470, "unleashed_fury_ft" ) );
}

#endif
