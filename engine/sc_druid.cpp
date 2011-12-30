// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.h"

// ==========================================================================
// Druid
// ==========================================================================

struct druid_targetdata_t : public targetdata_t
{
  dot_t* dots_fiery_claws;
  dot_t* dots_insect_swarm;
  dot_t* dots_lacerate;
  dot_t* dots_lifebloom;
  dot_t* dots_moonfire;
  dot_t* dots_rake;
  dot_t* dots_regrowth;
  dot_t* dots_rejuvenation;
  dot_t* dots_rip;
  dot_t* dots_sunfire;
  dot_t* dots_wild_growth;

  buff_t* buffs_lifebloom;
  buff_t* buffs_combo_points;

  double combo_point_rank( double* cp_list ) const
  {
    assert( buffs_combo_points -> check() );
    return cp_list[ buffs_combo_points -> stack() - 1 ];
  }

  double combo_point_rank( double cp1, double cp2, double cp3, double cp4, double cp5 ) const
  {
    double cp_list[] = { cp1, cp2, cp3, cp4, cp5 };
    return combo_point_rank( cp_list );
  }

  bool hot_ticking()
  {
    return dots_regrowth->ticking || dots_rejuvenation->ticking || dots_lifebloom->ticking || dots_wild_growth->ticking;
  }

  druid_targetdata_t( player_t* source, player_t* target )
    : targetdata_t( source, target )
  {
    buffs_combo_points = add_aura( new buff_t( this, "combo_points", 5 ) );

    buffs_lifebloom = add_aura( new buff_t( this, this->source->dbc.class_ability_id( this->source->type, "Lifebloom" ), "lifebloom", 1.0, 0 ) );
    buffs_lifebloom -> buff_duration = 11.0; // Override duration so the bloom works correctly
  }
};

void register_druid_targetdata( sim_t* sim )
{
  player_type t = DRUID;
  typedef druid_targetdata_t type;

  REGISTER_DOT( fiery_claws );
  REGISTER_DOT( insect_swarm );
  REGISTER_DOT( lacerate );
  REGISTER_DOT( lifebloom );
  REGISTER_DOT( moonfire );
  REGISTER_DOT( rake );
  REGISTER_DOT( regrowth );
  REGISTER_DOT( rejuvenation );
  REGISTER_DOT( rip );
  REGISTER_DOT( sunfire );
  REGISTER_DOT( wild_growth );

  REGISTER_BUFF( combo_points );
  REGISTER_BUFF( lifebloom );
}

struct druid_t : public player_t
{
  // Active
  heal_t*   active_efflorescence;
  action_t* active_fury_swipes;
  heal_t*   active_living_seed;
  attack_t* active_tier12_2pc_melee;

  // Pets
  pet_t* pet_burning_treant;
  pet_t* pet_treants;

  // Auto-attacks
  attack_t* cat_melee_attack;
  attack_t* bear_melee_attack;

  double equipped_weapon_dps;

  // Buffs
  buff_t* buffs_barkskin;
  buff_t* buffs_bear_form;
  buff_t* buffs_cat_form;
  buff_t* buffs_eclipse_lunar;
  buff_t* buffs_eclipse_solar;
  buff_t* buffs_enrage;
  buff_t* buffs_frenzied_regeneration;
  buff_t* buffs_glyph_of_innervate;
  buff_t* buffs_harmony;
  buff_t* buffs_lacerate;
  buff_t* buffs_lunar_shower;
  buff_t* buffs_moonkin_form;
  buff_t* buffs_natures_grace;
  buff_t* buffs_natures_swiftness;
  buff_t* buffs_omen_of_clarity;
  buff_t* buffs_pulverize;
  buff_t* buffs_revitalize;
  buff_t* buffs_savage_defense;
  buff_t* buffs_savage_roar;
  buff_t* buffs_shooting_stars;
  buff_t* buffs_stampede_bear;
  buff_t* buffs_stampede_cat;
  buff_t* buffs_stealthed;
  buff_t* buffs_survival_instincts;
  buff_t* buffs_t11_4pc_caster;
  buff_t* buffs_t11_4pc_melee;
  buff_t* buffs_t13_4pc_melee;
  buff_t* buffs_wild_mushroom;
  buff_t* buffs_berserk;
  buff_t* buffs_primal_madness_bear;
  buff_t* buffs_primal_madness_cat;
  buff_t* buffs_tigers_fury;
  buff_t* buffs_tree_of_life;

  // Cooldowns
  cooldown_t* cooldowns_burning_treant;
  cooldown_t* cooldowns_fury_swipes;
  cooldown_t* cooldowns_lotp;
  cooldown_t* cooldowns_mangle_bear;
  cooldown_t* cooldowns_starsurge;

  // Gains
  gain_t* gains_bear_melee;
  gain_t* gains_energy_refund;
  gain_t* gains_enrage;
  gain_t* gains_euphoria;
  gain_t* gains_frenzied_regeneration;
  gain_t* gains_glyph_of_innervate;
  gain_t* gains_glyph_ferocious_bite;
  gain_t* gains_incoming_damage;
  gain_t* gains_lotp_health;
  gain_t* gains_lotp_mana;
  gain_t* gains_moonkin_form;
  gain_t* gains_natural_reaction;
  gain_t* gains_omen_of_clarity;
  gain_t* gains_primal_fury;
  gain_t* gains_primal_madness;
  gain_t* gains_revitalize;
  gain_t* gains_tigers_fury;
  gain_t* gains_heartfire; // T12_2pc_Heal

  // Glyphs
  struct glyphs_t
  {
    glyph_t* berserk;
    glyph_t* bloodletting;
    glyph_t* ferocious_bite;
    glyph_t* focus;
    glyph_t* frenzied_regeneration;
    glyph_t* healing_touch;
    glyph_t* innervate;
    glyph_t* insect_swarm;
    glyph_t* lacerate;
    glyph_t* lifebloom;
    glyph_t* mangle;
    glyph_t* mark_of_the_wild;
    glyph_t* maul;
    glyph_t* monsoon;
    glyph_t* moonfire;
    glyph_t* regrowth;
    glyph_t* rejuvenation;
    glyph_t* rip;
    glyph_t* savage_roar;
    glyph_t* starfall;
    glyph_t* starfire;
    glyph_t* starsurge;
    glyph_t* swiftmend;
    glyph_t* tigers_fury;
    glyph_t* typhoon;
    glyph_t* wild_growth;
    glyph_t* wrath;

    glyphs_t() { memset( ( void* ) this, 0x0, sizeof( glyphs_t ) ); }
  };
  glyphs_t glyphs;

  // Procs
  proc_t* procs_combo_points_wasted;
  proc_t* procs_empowered_touch;
  proc_t* procs_fury_swipes;
  proc_t* procs_parry_haste;
  proc_t* procs_primal_fury;
  proc_t* procs_revitalize;
  proc_t* procs_wrong_eclipse_wrath;
  proc_t* procs_wrong_eclipse_starfire;
  proc_t* procs_unaligned_eclipse_gain;
  proc_t* procs_burning_treant;
  proc_t* procs_munched_tier12_2pc_melee;
  proc_t* procs_rolled_tier12_2pc_melee;

  // Random Number Generation
  rng_t* rng_berserk;
  rng_t* rng_blood_in_the_water;
  rng_t* rng_empowered_touch;
  rng_t* rng_euphoria;
  rng_t* rng_fury_swipes;
  rng_t* rng_primal_fury;
  rng_t* rng_tier12_4pc_melee;
  rng_t* rng_wrath_eclipsegain;
  rng_t* rng_burning_treant;
  rng_t* rng_heartfire; // Tier12_2pc_Heal

  // Spell Data
  struct spells_t
  {
    spell_data_t* aggression;
    spell_data_t* gift_of_nature;
    spell_data_t* meditation;
    spell_data_t* moonfury;
    spell_data_t* primal_madness_cat;
    spell_data_t* razor_claws;
    spell_data_t* savage_defender; // NYI
    spell_data_t* harmony;
    spell_data_t* total_eclipse;
    spell_data_t* vengeance;

    spells_t() { memset( ( void* ) this, 0x0, sizeof( spells_t ) ); }
  };
  spells_t spells;

  // Eclipse Management
  int eclipse_bar_value; // Tracking the current value of the eclipse bar
  int eclipse_wrath_count; // Wrath gains eclipse in 13, 13, 14, 13, 13, 14, 13, 13
  int eclipse_bar_direction; // Tracking the current direction of the eclipse bar

  // Up-Times
  benefit_t* uptimes_energy_cap;
  benefit_t* uptimes_rage_cap;

  // Talents
  struct talents_t
  {
    // Balance
    talent_t* balance_of_power;
    talent_t* earth_and_moon;
    talent_t* euphoria;
    talent_t* dreamstate;
    talent_t* force_of_nature;
    talent_t* fungal_growth;
    talent_t* gale_winds;
    talent_t* genesis;
    talent_t* lunar_shower;
    talent_t* moonglow;
    talent_t* moonkin_form;
    talent_t* natures_grace;
    talent_t* natures_majesty;
    talent_t* owlkin_frenzy;
    talent_t* shooting_stars;
    talent_t* solar_beam;
    talent_t* starfall;
    talent_t* starlight_wrath;
    talent_t* sunfire;
    talent_t* typhoon;

    // Feral
    talent_t* berserk;
    talent_t* blood_in_the_water;
    talent_t* brutal_impact;
    talent_t* endless_carnage;
    talent_t* feral_aggression;
    talent_t* feral_charge;
    talent_t* feral_swiftness;
    talent_t* furor;
    talent_t* fury_swipes;
    talent_t* infected_wounds;
    talent_t* king_of_the_jungle;
    talent_t* leader_of_the_pack;
    talent_t* natural_reaction;
    talent_t* nurturing_instict; // NYI
    talent_t* predatory_strikes;
    talent_t* primal_fury;
    talent_t* primal_madness;
    talent_t* pulverize;
    talent_t* rend_and_tear;
    talent_t* stampede;
    talent_t* survival_instincts;
    talent_t* thick_hide; // How does the +10% and +33%@bear stack etc

    // Resto
    talent_t* blessing_of_the_grove;
    talent_t* efflorescence;
    talent_t* empowered_touch;
    talent_t* fury_of_stormrage; // NYI
    talent_t* gift_of_the_earthmother;
    talent_t* heart_of_the_wild;
    talent_t* improved_rejuvenation;
    talent_t* living_seed;
    talent_t* malfurions_gift;
    talent_t* master_shapeshifter;
    talent_t* natural_shapeshifter;
    talent_t* natures_bounty;
    talent_t* natures_cure; // NYI
    talent_t* natures_swiftness;
    talent_t* natures_ward; // NYI
    talent_t* naturalist;
    talent_t* perseverance;
    talent_t* revitalize;
    talent_t* swift_rejuvenation;
    talent_t* tree_of_life;
    talent_t* wild_growth;

    talents_t() { memset( ( void* ) this, 0x0, sizeof( talents_t ) ); }
  };
  talents_t talents;

  druid_t( sim_t* sim, const std::string& name, race_type r = RACE_NONE ) : player_t( sim, DRUID, name, r )
  {
    if ( race == RACE_NONE ) race = RACE_NIGHT_ELF;

    tree_type[ DRUID_BALANCE     ] = TREE_BALANCE;
    tree_type[ DRUID_FERAL       ] = TREE_FERAL;
    tree_type[ DRUID_RESTORATION ] = TREE_RESTORATION;

    active_efflorescence      = 0;
    active_fury_swipes        = NULL;
    active_living_seed        = 0;
    active_tier12_2pc_melee   = 0;

    pet_burning_treant = 0;
    pet_treants         = 0;

    eclipse_bar_value     = 0;
    eclipse_wrath_count   = 0;
    eclipse_bar_direction = 0;

    cooldowns_burning_treant = get_cooldown( "burning_treant" );
    cooldowns_burning_treant -> duration = 45.0;
    cooldowns_fury_swipes    = get_cooldown( "fury_swipes"    );
    cooldowns_lotp           = get_cooldown( "lotp"           );
    cooldowns_mangle_bear    = get_cooldown( "mangle_bear"    );
    cooldowns_starsurge      = get_cooldown( "starsurge"      );

    cat_melee_attack = 0;
    bear_melee_attack = 0;

    equipped_weapon_dps = 0;

    create_talents();
    create_glyphs();

    distance = ( primary_tree() == TREE_FERAL ) ? 3 : 30;
    default_distance = distance;

    create_options();
  }

  // Character Definition
  virtual targetdata_t* new_targetdata( player_t* source, player_t* target ) {return new druid_targetdata_t( source, target );}
  virtual void      init_talents();
  virtual void      init_spells();
  virtual void      init_base();
  virtual void      init_buffs();
  virtual void      init_values();
  virtual void      init_scaling();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_benefits();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      combat_begin();
  virtual void      reset();
  virtual void      regen( double periodicity );
  virtual double    available() const;
  virtual double    composite_armor_multiplier() const;
  virtual double    composite_attack_power() const;
  virtual double    composite_attack_power_multiplier() const;
  virtual double    composite_attack_crit() const;
  virtual double    composite_player_multiplier( const school_type school, action_t* a = NULL ) const;
  virtual double    composite_spell_hit() const;
  virtual double    composite_spell_crit() const;
  virtual double    composite_attribute_multiplier( int attr ) const;
  virtual double    matching_gear_multiplier( const attribute_type attr ) const;
  virtual double    composite_block_value() const { return 0; }
  virtual double    composite_tank_parry() const { return 0; }
  virtual double    composite_tank_block() const { return 0; }
  virtual double    composite_tank_crit( const school_type school ) const;
  virtual action_expr_t* create_expression( action_t*, const std::string& name );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual int       decode_set( item_t& item );
  virtual int       primary_resource() const;
  virtual int       primary_role() const;
  virtual double    assess_damage( double amount, const school_type school, int dmg_type, int result, action_t* a );
  virtual heal_info_t assess_heal( double amount, const school_type school, int type, int result, action_t* a );
  virtual double    intellect() const;


  void reset_gcd()
  {
    for ( action_t* a=action_list; a; a = a -> next )
    {
      if ( a -> trigger_gcd != 0 ) a -> trigger_gcd = base_gcd;
    }
  }
};

namespace { // ANONYMOUS NAMESPACE ==========================================

// ==========================================================================
// Druid Cat Attack
// ==========================================================================

struct druid_cat_attack_t : public attack_t
{
  int requires_stealth;
  int requires_position;
  bool requires_combo_points;
  int adds_combo_points;

  druid_cat_attack_t( const char* n, player_t* player, const school_type s=SCHOOL_PHYSICAL, int t=TREE_NONE, bool special=true ) :
    attack_t( n, player, RESOURCE_ENERGY, s, t, special ),
    requires_stealth( 0 ),
    requires_position( POSITION_NONE ),
    requires_combo_points( false ),
    adds_combo_points( 0 )
  {
    tick_may_crit = true;
  }

  druid_cat_attack_t( const char* n, uint32_t id, druid_t* p, bool special = true ) :
    attack_t( n, id, p, 0, special ),
    adds_combo_points( 0 )
  {
    adds_combo_points     = ( int ) base_value( E_ADD_COMBO_POINTS );
    may_crit              = true;
    tick_may_crit         = true;
    requires_combo_points = false;
    requires_position     = POSITION_NONE;
    requires_stealth      = false;
  }

  virtual double cost() const;
  virtual void   execute();
  virtual void   consume_resource();
  virtual void   player_buff();
  virtual bool   ready();
};

// ==========================================================================
// Druid Bear Attack
// ==========================================================================

struct druid_bear_attack_t : public attack_t
{
  druid_bear_attack_t( const char* n, player_t* player, const school_type s=SCHOOL_PHYSICAL, int t=TREE_NONE, bool special=true ) :
    attack_t( n, player, RESOURCE_RAGE, s, t, special )
  {}

  druid_bear_attack_t( const char* n, uint32_t id, druid_t* p, bool special = true ) :
    attack_t( n, id, p, 0, special )
  {
    may_crit      = true;
    tick_may_crit = true;
  }

  virtual double cost() const;
  virtual void   execute();
  virtual double haste() const;
  virtual void   consume_resource();
  virtual void   player_buff();
};

// ==========================================================================
// Druid Heal
// ==========================================================================

struct druid_heal_t : public heal_t
{
  double additive_factors;
  bool consume_ooc;

  druid_heal_t( const char* n, druid_t* p, const uint32_t id, int t = TREE_NONE ) :
    heal_t( n, p, id, t ), additive_factors( 0 ), consume_ooc( false )
  {
    dot_behavior      = DOT_REFRESH;
    may_crit          = true;
    tick_may_crit     = true;
    weapon_multiplier = 0;

    if ( p -> primary_tree() == TREE_RESTORATION )
    {
      additive_factors += p -> spells.gift_of_nature -> effect1().percent();
    }
  }

  virtual void   consume_resource();
  virtual double cost() const;
  virtual double cost_reduction() const;
  virtual void   execute();
  virtual double execute_time() const;
  virtual double haste() const;
  virtual void   player_buff();
};

// ==========================================================================
// Druid Spell
// ==========================================================================

struct druid_spell_t : public spell_t
{
  double additive_multiplier;

  druid_spell_t( const char* n, player_t* p, const school_type s, int t ) :
    spell_t( n, p, RESOURCE_MANA, s, t ), additive_multiplier( 0.0 )
  {
  }

  druid_spell_t( const char* n, uint32_t id, druid_t* p ) :
    spell_t( n, id, p, 0 ), additive_multiplier( 0.0 )
  {
    may_crit      = true;
    tick_may_crit = true;
  }

  virtual void   consume_resource();
  virtual double cost() const;
  virtual double cost_reduction() const;
  virtual void   execute();
  virtual double execute_time() const;
  virtual double haste() const;
  virtual void   player_tick();
  virtual void   player_buff();
  virtual void   schedule_execute();
};

// ==========================================================================
// Pet Treants
// ==========================================================================

struct treants_pet_t : public pet_t
{
  struct melee_t : public attack_t
  {
    melee_t( player_t* player ) :
      attack_t( "treant_melee", player )
    {
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      base_dd_min = base_dd_max = 1;
      background = true;
      repeating = true;
      may_crit = true;

      // Model the three Treants as one actor hitting 3x hard
      base_multiplier *= 3.0;
    }
  };

  treants_pet_t( sim_t* sim, player_t* owner, const std::string& pet_name ) :
    pet_t( sim, owner, pet_name )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 580;
    main_hand_weapon.max_dmg    = 580;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = 1.65;
  }

  virtual void init_base()
  {
    pet_t::init_base();

    // At 85 base AP of 932
    attribute_base[ ATTR_STRENGTH  ] = 476;
    attribute_base[ ATTR_AGILITY   ] = 113;
    attribute_base[ ATTR_STAMINA   ] = 361;
    attribute_base[ ATTR_INTELLECT ] = 65;
    attribute_base[ ATTR_SPIRIT    ] = 109;

    base_attack_crit  = .05;
    base_attack_power = -20;
    initial_attack_power_per_strength = 2.0;

    main_hand_attack = new melee_t( this );
  }

  virtual double composite_attack_power() const
  {
    double ap = pet_t::composite_attack_power();
    ap += 0.57 * owner -> composite_spell_power( SCHOOL_MAX ) * owner -> composite_spell_power_multiplier();
    return ap;
  }

  virtual double composite_attack_hit() const
  {
    return owner -> composite_spell_hit();
  }

  virtual double composite_attack_expertise() const
  {
    // Hit scales that if they are hit capped, you're expertise capped.
    return owner -> composite_spell_hit() * 26.0 / 17.0;
  }

  virtual void schedule_ready( double delta_time=0,
                               bool   waiting=false )
  {
    pet_t::schedule_ready( delta_time, waiting );
    if ( ! main_hand_attack -> execute_event ) main_hand_attack -> execute();
  }
};

// ==========================================================================
// Pet Burning Treant Tier 12 set bonus
// ==========================================================================

struct burning_treant_pet_t : public pet_t
{
  double snapshot_crit;

  burning_treant_pet_t( sim_t* sim, player_t* owner ) :
    pet_t( sim, owner, "burning_treant", true /*guardian*/ ),
    snapshot_crit( 0 )
  {
    action_list_str += "/snapshot_stats";
    action_list_str += "/fireseed";
  }

  virtual void summon( double duration=0 )
  {
    pet_t::summon( duration );
    // Guardians use snapshots
    snapshot_crit = owner -> composite_spell_crit();
    if ( owner -> bugs )
    {
      snapshot_crit = 0.00; // Rough guess
    }
  }

  virtual double composite_spell_crit() const
  {
    return snapshot_crit;
  }

  virtual double composite_spell_haste() const
  {
    return 1.0;
  }

  struct fireseed_t : public spell_t
  {
    fireseed_t( burning_treant_pet_t* p ):
      spell_t( "fireseed", 99026, p )
    {
      may_crit          = true;
      trigger_gcd = 1.5;
      if ( p -> owner -> bugs )
      {
        ability_lag = 0.74;
        ability_lag_stddev = 0.62 / 2.0;
      }
    }
  };

  virtual void schedule_ready( double delta_time,
                               bool   waiting  )
  {
    pet_t::schedule_ready( delta_time, waiting );
  }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "fireseed" ) return new fireseed_t( this );

    return pet_t::create_action( name, options_str );
  }
};

// add_combo_point ==========================================================

static void add_combo_point( druid_t* p, druid_targetdata_t* td )
{
  if ( td -> buffs_combo_points -> current_stack == 5 )
  {
    p -> procs_combo_points_wasted -> occur();
    return;
  }

  td -> buffs_combo_points -> trigger();
}

// trigger_earth_and_moon ===================================================

static void trigger_earth_and_moon( spell_t* s )
{
  druid_t* p = s -> player -> cast_druid();

  if ( p -> talents.earth_and_moon -> rank() == 0 ) return;

  s -> target -> debuffs.earth_and_moon -> trigger( 1, 8 );
  s -> target -> debuffs.earth_and_moon -> source = p;
}

// trigger_eclipse_proc =====================================================

static void trigger_eclipse_proc( druid_t* p )
{
  // All extra procs when eclipse pops
  p -> resource_gain( RESOURCE_MANA, p -> resource_max[ RESOURCE_MANA ] * p -> talents.euphoria -> effect3().resource( RESOURCE_MANA ), p -> gains_euphoria );
  p -> buffs_t11_4pc_caster -> trigger( 3 );
  p -> buffs_natures_grace -> cooldown -> reset();
}

// trigger_eclipse_energy_gain ==============================================

static void trigger_eclipse_energy_gain( spell_t* s, int gain )
{
  if ( gain == 0 ) return;

  druid_t* p = s -> player -> cast_druid();

  // Gain will only happen if it is either aligned with the bar direction or
  // the bar direction has not been set yet.
  if ( p -> eclipse_bar_direction == -1 && gain > 0 )
  {
    p -> procs_unaligned_eclipse_gain -> occur();
    return;
  }
  else if ( p -> eclipse_bar_direction ==  1 && gain < 0 )
  {
    p -> procs_unaligned_eclipse_gain -> occur();
    return;
  }

  int old_eclipse_bar_value = p -> eclipse_bar_value;
  p -> eclipse_bar_value += gain;

  if ( p -> eclipse_bar_value <= 0 )
  {
    p -> buffs_eclipse_solar -> expire();

    if ( p -> eclipse_bar_value < -100 )
      p -> eclipse_bar_value = -100;
  }

  if ( p -> eclipse_bar_value >= 0 )
  {
    p -> buffs_eclipse_lunar -> expire();

    if ( p -> eclipse_bar_value > 100 )
      p -> eclipse_bar_value = 100;
  }

  int actual_gain = p -> eclipse_bar_value - old_eclipse_bar_value;
  if ( s -> sim -> log )
  {
    log_t::output( s -> sim, "%s gains %d (%d) %s from %s (%d)",
                   p -> name(), actual_gain, gain,
                   "Eclipse", s -> name(),
                   p -> eclipse_bar_value );
  }

  // Eclipse proc:
  // Procs when you reach 100 and only then, you have to have an
  // actual gain of eclipse energy (bar value)
  if ( actual_gain != 0 )
  {
    if ( p -> eclipse_bar_value == 100 )
    {
      if ( p -> buffs_eclipse_solar -> trigger() )
      {
        trigger_eclipse_proc( p );
        // Solar proc => bar direction changes to -1 (towards Lunar)
        p -> eclipse_bar_direction = -1;
      }
    }
    else if ( p -> eclipse_bar_value == -100 )
    {
      if ( p -> buffs_eclipse_lunar -> trigger() )
      {
        trigger_eclipse_proc( p );
        // Lunar proc => bar direction changes to +1 (towards Solar)
        p -> eclipse_bar_direction = 1;
      }
    }
  }
}

// trigger_eclipse_gain_delay ===============================================

static void trigger_eclipse_gain_delay( spell_t* s, int gain )
{
  struct eclipse_delay_t : public event_t
  {
    spell_t* s;
    int g;

    eclipse_delay_t ( spell_t* spell, int gain ) :
      event_t( spell -> sim, spell -> player ),
      s( spell ), g( gain )
    {
      name = "Eclipse gain delay";
      sim -> add_event( this, sim -> gauss( sim -> default_aura_delay, sim -> default_aura_delay_stddev ) );
    }

    virtual void execute()
    {
      trigger_eclipse_energy_gain( s, g );
    }
  };

  new ( s -> sim ) eclipse_delay_t( s, gain );
}

// trigger_efflorescence ====================================================

static void trigger_efflorescence( heal_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( ! p -> talents.efflorescence -> rank() ) return;

  struct efflorescence_t : public druid_heal_t
  {
    efflorescence_t( druid_t* player ) :
      druid_heal_t( "efflorescence", player, 81269 )
    {
      aoe            = 3;
      background     = true;
      base_tick_time = 1.0;
      hasted_ticks   = true;
      may_crit       = false;
      num_ticks      = 7;
      proc           = true;
      tick_may_crit  = false;

      init();
    }
  };

  if ( ! p -> active_efflorescence ) p -> active_efflorescence = new efflorescence_t( p );

  double heal = a -> direct_dmg * p -> talents.efflorescence -> effect1().percent();
  p -> active_efflorescence -> base_td = heal;
  p -> active_efflorescence -> execute();
}

// trigger_empowered_touch ==================================================

static void trigger_empowered_touch( heal_t* a )
{
  druid_t* p = a -> player -> cast_druid();
  druid_targetdata_t* td = a -> targetdata() -> cast_druid();

  if ( p -> rng_empowered_touch -> roll( p -> talents.empowered_touch -> effect2().percent() ) )
  {
    if ( td -> dots_lifebloom -> ticking )
    {
      td -> dots_lifebloom -> refresh_duration();
      if ( td -> buffs_lifebloom -> check() ) td -> buffs_lifebloom -> refresh();
      p -> procs_empowered_touch -> occur();
    }
  }
}

// trigger_energy_refund ====================================================

static void trigger_energy_refund( druid_cat_attack_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  double energy_restored = a -> resource_consumed * 0.80;

  p -> resource_gain( RESOURCE_ENERGY, energy_restored, p -> gains_energy_refund );
}

// trigger_fury_swipes ======================================================

static void trigger_fury_swipes( action_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( a -> proc ) return;

  if ( ! a -> weapon ) return;

  if ( ! p -> talents.fury_swipes -> rank() )
    return;

  if ( p -> cooldowns_fury_swipes -> remains() > 0 )
    return;

  if ( p -> rng_fury_swipes -> roll( p -> talents.fury_swipes -> proc_chance() ) )
  {
    if ( ! p -> active_fury_swipes )
    {
      struct fury_swipes_t : public druid_cat_attack_t
      {
        fury_swipes_t( druid_t* player ) :
          druid_cat_attack_t( "fury_swipes", 80861, player )
        {
          background  = true;
          proc        = true;
          trigger_gcd = 0;
          init();
        }
      };
      p -> active_fury_swipes = new fury_swipes_t( p );
    }

    p -> active_fury_swipes    -> execute();
    p -> cooldowns_fury_swipes -> start( 3.0 );
    p -> procs_fury_swipes     -> occur();
  }
}

// trigger_infected_wounds ==================================================

static void trigger_infected_wounds( action_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( p -> talents.infected_wounds -> rank() )
  {
    a -> target -> debuffs.infected_wounds -> trigger();
    a -> target -> debuffs.infected_wounds -> source = p;
  }
}

// trigger_living_seed ======================================================

static void trigger_living_seed( heal_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( ! p -> talents.living_seed -> ok() )
    return;

  struct living_seed_t : public druid_heal_t
  {
    living_seed_t( druid_t* player ) :
      druid_heal_t( "living_seed", player, 48504 )
    {
      background = true;
      may_crit   = false;
      proc       = true;

      init();
    }

    virtual void player_buff()
    { } // no double dipping
  };

  if ( ! p -> active_living_seed ) p -> active_living_seed = new living_seed_t( p );

  // Technically this should be a buff on the target, then bloom when they're attacked
  // For simplicity we're going to assume it always heals the target
  double heal = a -> direct_dmg * p -> talents.living_seed -> effect1().percent();
  p -> active_living_seed -> base_dd_min = heal;
  p -> active_living_seed -> base_dd_max = heal;
  p -> active_living_seed -> execute();
};

// trigger_lotp =============================================================

static void trigger_lotp( action_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( ! p -> talents.leader_of_the_pack -> ok() )
    return;

  if ( p -> cooldowns_lotp -> remains() > 0 )
    return;

  // Has to do damage and can't be a proc
  if ( ( a -> direct_dmg <= 0 && a -> tick_dmg <= 0 ) || a -> proc )
    return;

  p -> resource_gain( RESOURCE_HEALTH,
                      p -> resource_max[ RESOURCE_HEALTH ] * p -> dbc.spell( 24932 ) -> effect2().percent(),
                      p -> gains_lotp_health );

  p -> resource_gain( RESOURCE_MANA,
                      p -> resource_max[ RESOURCE_MANA ] * p -> talents.leader_of_the_pack -> effect1().percent(),
                      p -> gains_lotp_mana );

  p -> cooldowns_lotp -> start( 6.0 );
};

// trigger_omen_of_clarity ==================================================

static void trigger_omen_of_clarity( action_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( a -> proc ) return;

  p -> buffs_omen_of_clarity -> trigger();
}

// trigger_primal_fury (Bear) ===============================================

static void trigger_primal_fury( druid_bear_attack_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( ! p -> talents.primal_fury -> rank() )
    return;

  // Has to do damage and can't be a proc
  if ( ( a -> direct_dmg <= 0 && a -> tick_dmg <= 0 ) || a -> proc )
    return;

  const spell_data_t* primal_fury = p -> dbc.spell( p -> talents.primal_fury -> effect1().trigger_spell_id() );

  if ( p -> rng_primal_fury -> roll( primal_fury -> proc_chance() ) )
  {
    p -> resource_gain( RESOURCE_RAGE,
                        p -> dbc.spell( primal_fury -> effect1().trigger_spell_id() ) -> effect1().base_value() / 10,
                        p -> gains_primal_fury );
  }
}

// trigger_primal_fury (Cat) ================================================

static void trigger_primal_fury( druid_cat_attack_t* a )
{
  druid_t* p = a -> player -> cast_druid();
  druid_targetdata_t * td = a -> targetdata() -> cast_druid();

  if ( ! p -> talents.primal_fury -> rank() )
    return;

  if ( ! a -> adds_combo_points )
    return;

  const spell_data_t* blood_frenzy = p -> dbc.spell( p -> talents.primal_fury -> effect2().trigger_spell_id() );

  if ( p -> rng_primal_fury -> roll( blood_frenzy -> proc_chance() ) )
  {
    add_combo_point( p, td );
    p -> procs_primal_fury -> occur();
  }
}

// trigger_rage_gain ========================================================

static void trigger_rage_gain( druid_bear_attack_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  double rage_gain = 16.0;

  p -> resource_gain( RESOURCE_RAGE, rage_gain, p -> gains_bear_melee );
}

// trigger_revitalize =======================================================

static void trigger_revitalize( druid_heal_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( p -> buffs_revitalize -> trigger() )
  {
    p -> procs_revitalize -> occur();
    p -> resource_gain( RESOURCE_MANA,
                        p -> resource_max[ RESOURCE_MANA ] * p -> talents.revitalize -> effect1().percent(),
                        p -> gains_revitalize );
  }
}

// trigger_burning_treant ===================================================

static void trigger_burning_treant( spell_t* s )
{
  druid_t* p = s -> player -> cast_druid();

  if ( p -> set_bonus.tier12_2pc_caster() && ( p -> cooldowns_burning_treant -> remains() == 0 ) )
  {
    if ( p -> rng_burning_treant -> roll( p -> sets -> set( SET_T12_2PC_CASTER ) -> proc_chance() ) )
    {
      p -> procs_burning_treant -> occur();
      p -> pet_burning_treant -> dismiss();
      p -> pet_burning_treant -> summon( p -> dbc.spell( 99035 ) -> duration() - 0.01 );
      p -> cooldowns_burning_treant -> start();
    }
  }
}

// trigger_tier12_2pc_melee =================================================

static void trigger_tier12_2pc_melee( attack_t* s, double dmg )
{
  if ( s -> school != SCHOOL_PHYSICAL ) return;

  druid_t* p = s -> player -> cast_druid();
  druid_targetdata_t* td = s -> targetdata() -> cast_druid();
  sim_t* sim = s -> sim;

  if ( ! p -> set_bonus.tier12_2pc_melee() ) return;

  struct fiery_claws_t : public druid_cat_attack_t
  {
    fiery_claws_t( druid_t* p ) :
      druid_cat_attack_t( "fiery_claws", 99002, p )
    {
      background    = true;
      proc          = true;
      may_resist    = true;
      tick_may_crit = false;
      hasted_ticks  = false;
      dot_behavior  = DOT_REFRESH;
      init();
    }

    virtual void impact( player_t* t, int impact_result, double total_dot_dmg )
    {
      druid_cat_attack_t::impact( t, impact_result, 0 );

      base_td = total_dot_dmg / dot() -> num_ticks;
    }

    virtual double travel_time()
    {
      return sim -> gauss( sim -> aura_delay, 0.25 * sim -> aura_delay );
    }

    virtual void target_debuff( player_t* /* t */, int /* dmg_type */ )
    {
      target_multiplier            = 1.0;
      target_hit                   = 0;
      target_crit                  = 0;
      target_attack_power          = 0;
      target_spell_power           = 0;
      target_penetration           = 0;
      target_dd_adder              = 0;
      if ( sim -> debug )
        log_t::output( sim, "action_t::target_debuff: %s multiplier=%.2f hit=%.2f crit=%.2f attack_power=%.2f spell_power=%.2f penetration=%.0f",
                       name(), target_multiplier, target_hit, target_crit, target_attack_power, target_spell_power, target_penetration );
    }

    virtual double total_td_multiplier() const { return 1.0; }
  };

  double total_dot_dmg = dmg * p -> dbc.spell( 99001 ) -> effect1().percent();

  if ( ! p -> active_tier12_2pc_melee ) p -> active_tier12_2pc_melee = new fiery_claws_t( p );

  dot_t* dot = td -> dots_fiery_claws;

  if ( dot -> ticking )
  {
    total_dot_dmg += p -> active_tier12_2pc_melee -> base_td * dot -> ticks();
  }

  if ( ( p -> dbc.spell( 99002 ) -> duration() + sim -> aura_delay ) < dot -> remains() )
  {
    if ( sim -> log ) log_t::output( sim, "Player %s munches Fiery Claws due to Max Fiery Claws Duration.", p -> name() );
    p -> procs_munched_tier12_2pc_melee -> occur();
    return;
  }

  if ( p -> active_tier12_2pc_melee -> travel_event )
  {
    // There is an SPELL_AURA_APPLIED already in the queue, which will get munched.
    if ( sim -> log ) log_t::output( sim, "Player %s munches previous Fiery Claws due to Aura Delay.", p -> name() );
    p -> procs_munched_tier12_2pc_melee -> occur();
  }

  p -> active_tier12_2pc_melee -> direct_dmg = total_dot_dmg;
  p -> active_tier12_2pc_melee -> result = RESULT_HIT;
  p -> active_tier12_2pc_melee -> schedule_travel( s -> target );

  if ( p -> active_tier12_2pc_melee -> travel_event && dot -> ticking )
  {
    if ( dot -> tick_event -> occurs() < p -> active_tier12_2pc_melee -> travel_event -> occurs() )
    {
      // Fiery Claws will tick before SPELL_AURA_APPLIED occurs, which means that the current Fiery Claws will
      // both tick -and- get rolled into the next Fiery Claws.
      if ( sim -> log ) log_t::output( sim, "Player %s rolls Fiery Claws.", p -> name() );
      p -> procs_rolled_tier12_2pc_melee -> occur();
    }
  }
}

// ==========================================================================
// Druid Cat Attack
// ==========================================================================

// druid_cat_attack_t::cost =================================================

double druid_cat_attack_t::cost() const
{
  double c = attack_t::cost();
  druid_t* p = player -> cast_druid();

  if ( c == 0 )
    return 0;

  if ( harmful &&  p -> buffs_omen_of_clarity -> check() )
    return 0;

  if ( p -> buffs_berserk -> check() )
    c *= 1.0 + p -> talents.berserk -> effect1().percent();

  return c;
}

// druid_cat_attack_t::consume_resource =====================================

void druid_cat_attack_t::consume_resource()
{
  attack_t::consume_resource();
  druid_t* p = player -> cast_druid();

  if ( harmful && p -> buffs_omen_of_clarity -> up() )
  {
    // Treat the savings like a energy gain.
    double amount = attack_t::cost();
    if ( amount > 0 )
    {
      p -> gains_omen_of_clarity -> add( amount );
      p -> gains_omen_of_clarity -> type = RESOURCE_ENERGY;
      p -> buffs_omen_of_clarity -> expire();
    }
  }
}

// druid_cat_attack_t::execute ==============================================

void druid_cat_attack_t::execute()
{
  druid_t* p = player -> cast_druid();
  druid_targetdata_t* td = targetdata() -> cast_druid();

  attack_t::execute();

  if ( result_is_hit() )
  {
    if ( requires_combo_points )
    {
      if ( p -> set_bonus.tier12_4pc_melee() & p -> buffs_berserk -> check() )
      {
        if ( p -> rng_tier12_4pc_melee -> roll( td -> buffs_combo_points -> check() / 5 ) )
          p -> buffs_berserk -> extend_duration( p, p ->sets ->set( SET_T12_4PC_MELEE ) -> base_value() );
      }

      td -> buffs_combo_points -> expire();
    }
    if ( adds_combo_points ) add_combo_point ( p, td );

    if ( result == RESULT_CRIT )
    {
      trigger_lotp( this );
      trigger_primal_fury( this );
    }
  }
  else
  {
    trigger_energy_refund( this );
  }

  if ( harmful ) p -> buffs_stealthed -> expire();
}

// druid_cat_attack_t::player_buff ==========================================

void druid_cat_attack_t::player_buff()
{
  attack_t::player_buff();
}

// druid_cat_attack_t::ready ================================================

bool druid_cat_attack_t::ready()
{
  if ( ! attack_t::ready() )
    return false;

  druid_t*  p = player -> cast_druid();
  druid_targetdata_t* td = targetdata() -> cast_druid();

  if ( ! p -> buffs_cat_form -> check() )
    return false;

  if ( requires_position != POSITION_NONE )
    if ( p -> position != requires_position )
      return false;

  if ( requires_stealth )
    if ( ! p -> buffs_stealthed -> check() )
      return false;

  if ( requires_combo_points && ! td -> buffs_combo_points -> check() )
    return false;

  return true;
}

// Cat Melee Attack =========================================================

struct cat_melee_t : public druid_cat_attack_t
{
  cat_melee_t( player_t* player ) :
    druid_cat_attack_t( "cat_melee", player, SCHOOL_PHYSICAL, TREE_NONE, /*special*/false )
  {
    background  = true;
    repeating   = true;
    may_crit    = true;
    trigger_gcd = 0;
    base_cost   = 0;
  }

  virtual void player_buff()
  {
    druid_cat_attack_t::player_buff();
    druid_t* p = player -> cast_druid();

    player_multiplier *= 1.0 + p -> buffs_savage_roar -> value();
  }

  virtual double execute_time() const
  {
    if ( ! player -> in_combat )
      return 0.01;

    return druid_cat_attack_t::execute_time();
  }

  virtual void execute()
  {
    druid_cat_attack_t::execute();

    if ( result_is_hit() )
    {
      trigger_fury_swipes( this );
      trigger_omen_of_clarity( this );
    }
  }
};

// Claw =====================================================================

struct claw_t : public druid_cat_attack_t
{
  claw_t( druid_t* player, const std::string& options_str ) :
    druid_cat_attack_t( "claw", 1082, player )
  {
    parse_options( NULL, options_str );
  }
};

// Feral Charge (Cat) =======================================================

struct feral_charge_cat_t : public druid_cat_attack_t
{
  double stampede_cost_reduction, stampede_duration;

  feral_charge_cat_t( druid_t* p, const std::string& options_str ) :
    druid_cat_attack_t( "feral_charge_cat", 49376, p ),
    stampede_cost_reduction( 0.0 ), stampede_duration( 0.0 )
  {
    parse_options( NULL, options_str );

    if ( p -> talents.stampede -> rank() )
    {
      uint32_t sid = ( p -> talents.stampede -> rank() == 2 ) ? 81022 : 81021;
      passive_spell_t stampede_spell( p, "stampede_cat_spell", sid );
      stampede_cost_reduction = stampede_spell.mod_additive( P_RESOURCE_COST );
      // 81022 is bugged and says FLAT_MODIFIER not PCT_MODIFIER so adjust it
      if ( sid == 81022 )
      {
        stampede_cost_reduction /= 100.0;
      }
      stampede_cost_reduction = -stampede_cost_reduction;
      stampede_duration = stampede_spell.duration();
    }

    p -> buffs_stampede_cat -> buff_duration = stampede_duration;

    may_miss   = false;
    may_dodge  = false;
    may_parry  = false;
    may_block  = false;
    may_glance = false;
  }

  virtual void execute()
  {
    druid_cat_attack_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buffs_stampede_cat -> trigger( 1, stampede_cost_reduction );
  }

  virtual bool ready()
  {
    bool ranged = ( player -> position == POSITION_RANGED_FRONT ||
                    player -> position == POSITION_RANGED_BACK );

    if ( player -> in_combat && ! ranged )
    {
      return false;
    }

    return druid_cat_attack_t::ready();
  }
};

// Ferocious Bite ===========================================================

struct ferocious_bite_t : public druid_cat_attack_t
{
  double base_dmg_per_point;
  double excess_energy;
  double max_excess_energy;

  ferocious_bite_t( druid_t* p, const std::string& options_str ) :
    druid_cat_attack_t( "ferocious_bite", 22568, p ),
    base_dmg_per_point( 0 ), excess_energy( 0 ), max_excess_energy( 0 )
  {
    parse_options( NULL, options_str );

    base_dmg_per_point    = p -> dbc.effect_bonus( effect_id( 1 ), p -> level );
    base_multiplier      *= 1.0 + p -> talents.feral_aggression -> mod_additive( P_GENERIC );

    max_excess_energy     = effect2().base_value();
    requires_combo_points = true;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata() -> cast_druid();

    base_dd_adder    = base_dmg_per_point * td -> buffs_combo_points -> stack();
    direct_power_mod = 0.109 * td -> buffs_combo_points -> stack();

    // consumes up to 35 additional energy to increase damage by up to 100%.
    // Assume 100/35 = 2.857% per additional energy consumed

    excess_energy = ( p -> resource_current[ RESOURCE_ENERGY ] - druid_cat_attack_t::cost() );

    if ( excess_energy > max_excess_energy )
    {
      excess_energy = max_excess_energy;
    }
    else if ( excess_energy < 0 )
    {
      excess_energy = 0;
    }

    druid_cat_attack_t::execute();

    if ( result_is_hit() )
    {
      if ( p -> glyphs.ferocious_bite -> enabled() )
      {
        double amount = p -> resource_max[ RESOURCE_HEALTH ] *
                        p -> glyphs.ferocious_bite -> effect1().percent() *
                        ( ( int ) ( excess_energy + druid_cat_attack_t::cost() ) / 10 );
        p -> resource_gain( RESOURCE_HEALTH, amount, p -> gains_glyph_ferocious_bite );
      }
    }

    double health_percentage = ( p -> set_bonus.tier13_2pc_melee() ) ? 60.0 : p -> talents.blood_in_the_water -> base_value();
    if ( result_is_hit() && target -> health_percentage() <= health_percentage )
    {
      // Proc chance is not stored in the talent anymore
      if ( td -> dots_rip -> ticking && p -> rng_blood_in_the_water -> roll( p -> talents.blood_in_the_water -> rank() * 0.50 ) )
      {
        td -> dots_rip -> refresh_duration();
      }
    }
  }

  virtual void consume_resource()
  {
    // Ferocious Bite consumes 35+x energy, with 0 <= x <= 35.
    // Consumes the base_cost and handles Omen of Clarity
    druid_cat_attack_t::consume_resource();

    if ( result_is_hit() )
    {
      // Let the additional energy consumption create it's own debug log entries.
      if ( sim -> debug )
        log_t::output( sim, "%s consumes an additional %.1f %s for %s", player -> name(),
                       excess_energy, util_t::resource_type_string( resource ), name() );

      player -> resource_loss( resource, excess_energy );
      stats -> consume_resource( excess_energy );
    }
  }

  virtual void player_buff()
  {
    druid_cat_attack_t::player_buff();
    druid_t* p = player -> cast_druid();

    player_multiplier *= 1.0 + excess_energy / max_excess_energy;

    if ( target -> debuffs.bleeding -> check() )
      player_crit += p -> talents.rend_and_tear -> effect2().percent();
  }
};

// Frenzied Regeneration Buff ===============================================

struct frenzied_regeneration_buff_t : public buff_t
{
  int health_gain;

  frenzied_regeneration_buff_t( druid_t* p ) :
    buff_t( p, 22842, "frenzied_regeneration" ), health_gain( 0 )
  { }

  virtual void start( int stacks, double value )
  {
    druid_t* p = player -> cast_druid();

    // Kick off rage to health 'hot'
    if ( ! p -> glyphs.frenzied_regeneration -> ok() )
    {
      struct frenzied_regeneration_event_t : public event_t
      {
        stats_t* rage_stats;

        frenzied_regeneration_event_t ( druid_t* p ) :
          event_t( p -> sim, p, "frenzied_regeneration_heal" ),
          rage_stats( 0 )
        {
          sim -> add_event( this, 1.0 );
          rage_stats = p -> get_stats( "frenzied_regeneration" );
        }

        virtual void execute()
        {
          druid_t* p = player -> cast_druid();

          if ( p -> buffs_frenzied_regeneration -> check() )
          {
            int rage_consumed = ( int ) ( std::min( p -> resource_current[ RESOURCE_RAGE ], 10.0 ) );
            double health_pct = p -> dbc.spell( 22842 ) -> effect1().percent() / 100;
            double rage_health = rage_consumed * health_pct * p -> resource_max[ RESOURCE_HEALTH ];
            p -> resource_gain( RESOURCE_HEALTH, rage_health, p -> gains_frenzied_regeneration );
            p -> resource_loss( RESOURCE_RAGE, rage_consumed );
            rage_stats -> consume_resource( rage_consumed );
            new ( sim ) frenzied_regeneration_event_t( p );
          }
        }
      };
      new ( sim ) frenzied_regeneration_event_t( p );
    }
    else
    {
      // FIXME: Glyph should increase healing received
    }

    double health_pct = effect2().percent();

    health_gain = ( int ) floor( player -> resource_max[ RESOURCE_HEALTH ] * health_pct );
    p -> stat_gain( STAT_MAX_HEALTH, health_gain );

    // Ability also heals to 30% if not at that amount
    if ( p -> resource_current[ RESOURCE_HEALTH ] < health_gain )
    {
      p -> resource_gain( RESOURCE_HEALTH, health_gain - p -> resource_current[ RESOURCE_HEALTH ], p -> gains_frenzied_regeneration );
    }

    buff_t::start( stacks, value );
  }

  virtual void expire()
  {
    druid_t* p = player -> cast_druid();
    p -> stat_loss( STAT_MAX_HEALTH, health_gain );

    buff_t::expire();
  }
};

// Frenzied Regeneration ====================================================

struct frenzied_regeneration_t : public druid_bear_attack_t
{
  frenzied_regeneration_t( druid_t* p, const std::string& options_str ) :
    druid_bear_attack_t( "frenzied_regeneration", 22842, p )
  {
    parse_options( NULL, options_str );

    harmful = false;

    num_ticks = 0; // No need for this to tick, handled in the buff
  }

  virtual void execute()
  {
    druid_bear_attack_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buffs_frenzied_regeneration -> trigger();
  }
};

// Maim =====================================================================

struct maim_t : public druid_cat_attack_t
{
  double base_dmg_per_point;

  maim_t( druid_t* player, const std::string& options_str ) :
    druid_cat_attack_t( "maim", 22570, player ), base_dmg_per_point( 0 )
  {
    parse_options( NULL, options_str );

    base_dmg_per_point    = player -> dbc.effect_bonus( effect_id( 1 ), player -> level );
    requires_combo_points = true;
  }

  virtual void execute()
  {
    druid_targetdata_t* td = targetdata() -> cast_druid();

    base_dd_adder = base_dmg_per_point * td -> buffs_combo_points -> stack();

    druid_cat_attack_t::execute();
  }
};

// Mangle (Cat) =============================================================

struct mangle_cat_t : public druid_cat_attack_t
{
  int extend_rip;

  mangle_cat_t( druid_t* p, const std::string& options_str ) :
    druid_cat_attack_t( "mangle_cat", 33876, p ),
    extend_rip( 0 )
  {
    option_t options[] =
    {
      { "extend_rip", OPT_BOOL, &extend_rip },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    adds_combo_points = 1; // Not picked up from the DBC
    base_multiplier  *= 1.0 + p -> glyphs.mangle -> mod_additive( P_GENERIC );
  }

  virtual void execute()
  {
    druid_cat_attack_t::execute();
    if ( result_is_hit() )
    {
      druid_t* p = player -> cast_druid();
      druid_targetdata_t* td = targetdata() -> cast_druid();

      if ( p -> glyphs.bloodletting -> enabled() &&
           td -> dots_rip -> ticking  &&
           td -> dots_rip -> added_ticks < 4 )
      {
        // Glyph adds 1/1/2 ticks on execute
        int extra_ticks = ( td -> dots_rip -> added_ticks < 2 ) ? 1 : 2;
        td -> dots_rip -> extend_duration( extra_ticks );
      }
      p -> buffs_t11_4pc_melee -> trigger();
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    druid_cat_attack_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      trigger_infected_wounds( this );
      trigger_tier12_2pc_melee( this, direct_dmg );
      t -> debuffs.mangle -> trigger();
      t -> debuffs.mangle -> source = player;
    }
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata() -> cast_druid();

    if ( extend_rip )
      if ( ! p -> glyphs.bloodletting -> enabled() ||
           ! td -> dots_rip -> ticking ||
           ( td -> dots_rip -> added_ticks == 4 ) )
        return false;

    return druid_cat_attack_t::ready();
  }
};

// Pounce ===================================================================

struct pounce_bleed_t : public druid_cat_attack_t
{
  pounce_bleed_t( druid_t* player ) :
    druid_cat_attack_t( "pounce_bleed", 9007, player )
  {
    background     = true;
    stats          = player -> get_stats( "pounce", this );
    tick_power_mod = 0.03;
  }
};

struct pounce_t : public druid_cat_attack_t
{
  pounce_bleed_t* pounce_bleed;

  pounce_t( druid_t* p, const std::string& options_str ) :
    druid_cat_attack_t( "pounce", 9005, p ), pounce_bleed( 0 )
  {
    parse_options( NULL, options_str );

    requires_stealth = true;
    pounce_bleed     = new pounce_bleed_t( p );
  }

  virtual void execute()
  {
    druid_cat_attack_t::execute();

    if ( result_is_hit() )
      pounce_bleed -> execute();
  }
};

// Rake =====================================================================

struct rake_t : public druid_cat_attack_t
{
  rake_t( druid_t* p, const std::string& options_str ) :
    druid_cat_attack_t( "rake", 1822, p )
  {
    parse_options( NULL, options_str );

    dot_behavior        = DOT_REFRESH;
    direct_power_mod    = 0.147;
    tick_power_mod      = 0.147;
    num_ticks          += p -> talents.endless_carnage -> rank();
    base_td_multiplier *= 1.0 + p -> sets -> set( SET_T11_2PC_MELEE ) -> mod_additive( P_GENERIC );
    base_dd_min = base_dd_max = base_td;
  }
};

// Ravage ===================================================================

struct ravage_t : public druid_cat_attack_t
{
  ravage_t( druid_t* player, const std::string& options_str ) :
    druid_cat_attack_t( "ravage", 6785, player )
  {
    parse_options( NULL, options_str );

    requires_position = POSITION_BACK;
    requires_stealth  = true;
  }

  virtual void execute()
  {
    druid_cat_attack_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buffs_stampede_cat -> decrement();
    p -> buffs_t13_4pc_melee -> decrement();
    requires_stealth = true;
    requires_position = POSITION_BACK;

    if ( result_is_hit() )
      trigger_infected_wounds( this );

    p -> buffs_stampede_cat -> up();
  }

  virtual double cost() const
  {
    druid_t* p = player -> cast_druid();

    double c = druid_cat_attack_t::cost();

    if ( p -> buffs_t13_4pc_melee -> up() )
      return 0;

    c *= 1.0 - p -> buffs_stampede_cat -> value();

    return c;
  }

  virtual void consume_resource()
  {
    attack_t::consume_resource();
    druid_t* p = player -> cast_druid();

    if ( p -> buffs_omen_of_clarity -> up() && ! p -> buffs_stampede_cat -> check() && ! p -> buffs_t13_4pc_melee -> check() )
    {
      // Treat the savings like a energy gain.
      double amount = attack_t::cost();
      if ( amount > 0 )
      {
        p -> gains_omen_of_clarity -> add( amount );
        p -> gains_omen_of_clarity -> type = RESOURCE_ENERGY;
        p -> buffs_omen_of_clarity -> expire();
      }
    }
  }

  virtual void player_buff()
  {
    druid_t* p = player -> cast_druid();

    if ( p -> talents.predatory_strikes -> rank() )
    {
      if ( target -> health_percentage() >= p -> talents.predatory_strikes -> effect2().base_value() )
        player_crit += p -> talents.predatory_strikes -> effect1().percent();
    }

    druid_cat_attack_t::player_buff();
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();

    if ( p -> buffs_stampede_cat -> check() )
    {
      requires_stealth = false;
    }

    if ( p -> buffs_t13_4pc_melee -> check() )
    {
      requires_stealth = false;
      requires_position = POSITION_NONE;
    }

    return druid_cat_attack_t::ready();
  }
};

// Rip ======================================================================

struct rip_t : public druid_cat_attack_t
{
  double base_dmg_per_point;
  double ap_per_point;

  rip_t( druid_t* p, const std::string& options_str ) :
    druid_cat_attack_t( "rip", 1079, p ),
    base_dmg_per_point( 0 ), ap_per_point( 0 )
  {
    parse_options( NULL, options_str );

    base_dmg_per_point    = p -> dbc.effect_bonus( effect_id( 1 ), p -> level );
    ap_per_point          = 0.0207;
    requires_combo_points = true;
    may_crit              = false;
    base_multiplier      *= 1.0 + p -> glyphs.rip -> mod_additive( P_TICK_DAMAGE );

    dot_behavior          = DOT_REFRESH;
  }

  virtual void execute()
  {
    // We have to save these values for refreshes by Blood in the Water, so
    // we simply reset them to zeroes on each execute and check them in ::player_buff.
    base_td        = 0.0;
    tick_power_mod = 0.0;

    druid_cat_attack_t::execute();
  }

  virtual void player_buff()
  {
    druid_targetdata_t* td = targetdata() -> cast_druid();

    if ( base_td == 0.0 && tick_power_mod == 0.0 )
    {
      base_td        = base_td_init + td -> buffs_combo_points -> stack() * base_dmg_per_point;
      tick_power_mod = td -> buffs_combo_points -> stack() * ap_per_point;
    }

    druid_cat_attack_t::player_buff();
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata() -> cast_druid();

    if ( td -> dots_rip -> ticking )
    {
      double b = base_td_init + td -> buffs_combo_points -> stack() * base_dmg_per_point;
      double t = td -> buffs_combo_points -> stack() * ap_per_point;
      double current_value = b + t * p -> composite_attack_power();
      double saved_value = base_td + tick_power_mod * player_attack_power;

      if ( current_value < saved_value )
      {
        // A more powerful spell is active message.
        // Note we're making the assumption that's it is based off the simple sum of: basedmg + tick_mod * AP
        // We know it doesn't involve crit at all due to testing. It's unsure if player_multiplier will be needed or not.
        // Or if it's based off some other set of rules.
        return false;
      }
    }
    return druid_cat_attack_t::ready();
  }
};

// Savage Roar ==============================================================

struct savage_roar_t : public druid_cat_attack_t
{
  double buff_value;

  savage_roar_t( druid_t* p, const std::string& options_str ) :
    druid_cat_attack_t( "savage_roar", 52610, p )
  {
    parse_options( NULL, options_str );

    buff_value            = effect2().percent();
    buff_value           += p -> glyphs.savage_roar -> base_value() / 100.0;
    harmful               = false;
    requires_combo_points = true;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata() -> cast_druid();

    double duration = this -> duration() + 5.0 * td -> buffs_combo_points -> stack();
    duration += p -> talents.endless_carnage -> effect2().seconds();

    // execute clears CP, so has to be after calculation duration
    druid_cat_attack_t::execute();

    p -> buffs_savage_roar -> buff_duration = duration;
    p -> buffs_savage_roar -> trigger( 1, buff_value );
  }
};

// Shred ====================================================================

struct shred_t : public druid_cat_attack_t
{
  int extend_rip;

  shred_t( druid_t* p, const std::string& options_str ) :
    druid_cat_attack_t( "shred", 5221, p ),
    extend_rip( 0 )
  {
    option_t options[] =
    {
      { "extend_rip", OPT_BOOL, &extend_rip },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    requires_position  = POSITION_BACK;
  }

  virtual void execute()
  {
    druid_cat_attack_t::execute();
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata() -> cast_druid();

    if ( result_is_hit() )
    {
      if ( p -> glyphs.bloodletting -> enabled() &&
           td -> dots_rip -> ticking  &&
           td -> dots_rip -> added_ticks < 4 )
      {
        // Glyph adds 1/1/2 ticks on execute
        int extra_ticks = ( td -> dots_rip -> added_ticks < 2 ) ? 1 : 2;
        td -> dots_rip -> extend_duration( extra_ticks );
      }
      trigger_infected_wounds( this );
    }
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    druid_cat_attack_t::impact( t, impact_result, travel_dmg );

    trigger_tier12_2pc_melee( this, direct_dmg );
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    druid_cat_attack_t::target_debuff( t, dmg_type );
    druid_t* p = player -> cast_druid();

    if ( t -> debuffs.mangle -> up() || t -> debuffs.blood_frenzy_bleed -> up() || t -> debuffs.hemorrhage -> up() )
      target_multiplier *= 1.30;

    if ( t -> debuffs.bleeding -> up() )
      target_multiplier *= 1.0 + p -> talents.rend_and_tear -> effect1().percent();
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata() -> cast_druid();

    if ( extend_rip )
      if ( ! p -> glyphs.bloodletting -> enabled() ||
           ! td -> dots_rip -> ticking ||
           ( td -> dots_rip -> added_ticks == 4 ) )
        return false;

    return druid_cat_attack_t::ready();
  }
};

// Skull Bash (Cat) =========================================================

struct skull_bash_cat_t : public druid_cat_attack_t
{
  skull_bash_cat_t( druid_t* p, const std::string& options_str ) :
    druid_cat_attack_t( "skull_bash_cat", 22570, p )
  {
    parse_options( NULL, options_str );

    may_miss = may_resist = may_glance = may_block = may_dodge = may_parry = may_crit = false;

    cooldown -> duration += p -> talents.brutal_impact -> effect3().seconds();
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return druid_cat_attack_t::ready();
  }
};

// Swipe (Cat) ==============================================================

struct swipe_cat_t : public druid_cat_attack_t
{
  swipe_cat_t( druid_t* player, const std::string& options_str ) :
    druid_cat_attack_t( "swipe_cat", 62078, player )
  {
    parse_options( NULL, options_str );

    aoe = -1;
  }
};

// Tigers Fury ==============================================================

struct tigers_fury_t : public druid_cat_attack_t
{
  tigers_fury_t( druid_t* p, const std::string& options_str ) :
    druid_cat_attack_t( "tigers_fury", 5217, p )
  {
    parse_options( NULL, options_str );

    harmful = false;
    cooldown -> duration += p -> glyphs.tigers_fury -> mod_additive( P_COOLDOWN );
  }

  virtual void execute()
  {
    druid_cat_attack_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buffs_tigers_fury -> trigger( 1, effect1().percent() );

    if ( p -> talents.primal_madness -> rank() )
    {
      p -> buffs_primal_madness_cat -> buff_duration = p -> buffs_tigers_fury -> buff_duration;
      p -> buffs_primal_madness_cat -> trigger();
    }

    if ( p -> talents.king_of_the_jungle -> rank() )
    {
      p -> resource_gain( RESOURCE_ENERGY, p -> talents.king_of_the_jungle -> effect2().resource( RESOURCE_ENERGY ), p -> gains_tigers_fury );
    }

    p -> buffs_t13_4pc_melee -> trigger();
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();

    if ( p -> buffs_berserk -> check() )
      return false;

    return druid_cat_attack_t::ready();
  }
};

// ==========================================================================
// Druid Bear Attack
// ==========================================================================

// druid_bear_attack_t::cost ================================================

double druid_bear_attack_t::cost() const
{
  druid_t* p = player -> cast_druid();

  double c = attack_t::cost();

  if ( harmful && p -> buffs_omen_of_clarity -> check() )
    return 0;

  return c;
}

// druid_bear_attack_t::consume_resource ====================================

void druid_bear_attack_t::consume_resource()
{
  attack_t::consume_resource();
  druid_t* p = player -> cast_druid();

  if ( harmful && p -> buffs_omen_of_clarity -> up() )
  {
    // Treat the savings like a rage gain.
    double amount = attack_t::cost();
    if ( amount > 0 )
    {
      p -> gains_omen_of_clarity -> add( amount );
      p -> gains_omen_of_clarity -> type = RESOURCE_RAGE;
      p -> buffs_omen_of_clarity -> expire();
    }
  }
}

// druid_bear_attack_t::execute =============================================

void druid_bear_attack_t::execute()
{
  attack_t::execute();

  if ( result_is_hit() )
  {
    if ( result == RESULT_CRIT )
    {
      trigger_lotp( this );
      trigger_primal_fury( this );
      druid_t* p = player -> cast_druid();
      p -> buffs_savage_defense -> trigger( 1, p -> composite_attack_power() * 0.35 );
    }
  }
}

// druid_bear_attack_t::haste ===============================================

double druid_bear_attack_t::haste() const
{
  double h = attack_t::haste();
  druid_t* p = player -> cast_druid();

  h *= p -> buffs_stampede_bear -> value();

  return h;
}

// druid_bear_attack_t::player_buff =========================================

void druid_bear_attack_t::player_buff()
{
  attack_t::player_buff();
  druid_t* p = player -> cast_druid();

  player_multiplier *= 1.0 + p -> talents.master_shapeshifter -> base_value() * 0.01;

  if ( p -> talents.king_of_the_jungle -> rank() && p -> buffs_enrage -> up() )
  {
    player_multiplier *= 1.0 + p -> talents.king_of_the_jungle -> effect1().percent();
  }

  player_crit += p -> buffs_pulverize -> value();
}

// Bear Melee Attack ========================================================

struct bear_melee_t : public druid_bear_attack_t
{
  bear_melee_t( player_t* player ) :
    druid_bear_attack_t( "bear_melee", player, SCHOOL_PHYSICAL, TREE_NONE, /*special*/false )
  {
    background  = true;
    repeating   = true;
    trigger_gcd = 0;
    base_cost   = 0;
    may_crit    = true;
  }

  virtual double execute_time() const
  {
    if ( ! player -> in_combat )
      return 0.01;

    return druid_bear_attack_t::execute_time();
  }

  virtual void execute()
  {
    druid_bear_attack_t::execute();

    if ( result != RESULT_MISS )
      trigger_rage_gain( this );

    if ( result_is_hit() )
    {
      trigger_fury_swipes( this );
      trigger_omen_of_clarity( this );
    }
  }
};

// Demoralizing Roar ========================================================

struct demoralizing_roar_t : public druid_bear_attack_t
{
  demoralizing_roar_t( druid_t* p, const std::string& options_str ) :
    druid_bear_attack_t( "demoralizing_roar", 99, p )
  {
    parse_options( NULL, options_str );

    may_dodge  = false;
    may_parry  = false;
    may_block  = false;
    may_glance = false;
  }

  virtual void execute()
  {
    druid_bear_attack_t::execute();
    druid_t* p = player -> cast_druid();

    if ( ! p -> sim -> overrides.demoralizing_roar )
    {
      target -> debuffs.demoralizing_roar -> trigger();
      target -> debuffs.demoralizing_roar -> source = p;
    }
  }
};

// Feral Charge (Bear) ======================================================

struct feral_charge_bear_t : public druid_bear_attack_t
{
  double stampede_haste, stampede_duration;

  feral_charge_bear_t( druid_t* p, const std::string& options_str ) :
    druid_bear_attack_t( "feral_charge_bear", 16979, p ),
    stampede_haste( 0.0 ), stampede_duration( 0.0 )
  {
    parse_options( NULL, options_str );

    if ( p -> talents.stampede -> rank() )
    {
      uint32_t sid = ( p -> talents.stampede -> rank() == 2 ) ? 78893 : 78892;
      passive_spell_t stampede_spell( p, "stampede_bear_spell", sid );
      stampede_haste = stampede_spell.base_value( E_APPLY_AURA, A_MOD_HASTE ) / 100.0;
      stampede_duration = stampede_spell.duration();
    }

    stampede_haste = 1.0 / ( 1.0 + stampede_haste );
    p -> buffs_stampede_bear -> buff_duration = stampede_duration;

    may_miss   = false;
    may_dodge  = false;
    may_parry  = false;
    may_block  = false;
    may_glance = false;
  }

  virtual void execute()
  {
    druid_bear_attack_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buffs_stampede_bear -> trigger( 1, stampede_haste );
  }

  virtual bool ready()
  {
    bool ranged = ( player -> position == POSITION_RANGED_FRONT ||
                    player -> position == POSITION_RANGED_BACK );

    if ( player -> in_combat && ! ranged )
      return false;

    return druid_bear_attack_t::ready();
  }
};

// Lacerate =================================================================

struct lacerate_t : public druid_bear_attack_t
{
  cooldown_t* mangle_bear_cooldown;

  lacerate_t( druid_t* p, const std::string& options_str ) :
    druid_bear_attack_t( "lacerate",  33745, p ), mangle_bear_cooldown( 0 )
  {
    parse_options( NULL, options_str );

    direct_power_mod     = 0.0552;
    tick_power_mod       = 0.00369;
    dot_behavior         = DOT_REFRESH;
    base_crit           += p -> glyphs.lacerate -> mod_additive( P_CRIT );
    mangle_bear_cooldown = p -> cooldowns_mangle_bear;
  }

  virtual void execute()
  {
    druid_bear_attack_t::execute();
    druid_t* p = player -> cast_druid();

    if ( result_is_hit() )
    {
      p -> buffs_lacerate -> trigger();
      base_td_multiplier  = p -> buffs_lacerate -> current_stack;
      base_td_multiplier *= 1.0 + p -> sets -> set( SET_T11_2PC_MELEE ) -> mod_additive( P_GENERIC );
    }
  }

  virtual void tick( dot_t* d )
  {
    druid_bear_attack_t::tick( d );
    druid_t* p = player -> cast_druid();

    if ( p -> talents.berserk -> ok() )
    {
      if ( p -> rng_berserk -> roll( 0.30 ) )
        mangle_bear_cooldown -> reset();
    }
  }

  virtual void last_tick( dot_t* d )
  {
    druid_bear_attack_t::last_tick( d );
    druid_t* p = player -> cast_druid();

    p -> buffs_lacerate -> expire();
  }
};

// Mangle (Bear) ============================================================

struct mangle_bear_t : public druid_bear_attack_t
{
  mangle_bear_t( druid_t* p, const std::string& options_str ) :
    druid_bear_attack_t( "mangle_bear", 33878, p )
  {
    parse_options( NULL, options_str );

    base_multiplier *= 1.0 + p -> glyphs.mangle -> mod_additive( P_GENERIC );
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();

    if ( p -> buffs_berserk -> up() )
      aoe = 2;

    druid_bear_attack_t::execute();

    aoe = 0;
    if ( p -> buffs_berserk -> up() )
      cooldown -> reset();
  }

  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    druid_bear_attack_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      target -> debuffs.mangle -> trigger();
      target -> debuffs.mangle -> source = player;
      trigger_infected_wounds( this );
      trigger_tier12_2pc_melee( this, direct_dmg );
    }
  }
};

// Maul =====================================================================

struct maul_t : public druid_bear_attack_t
{
  maul_t( druid_t* p, const std::string& options_str ) :
    druid_bear_attack_t( "maul",  6807, p )
  {
    parse_options( NULL, options_str );

    weapon = &( player -> main_hand_weapon );

    aoe = p -> glyphs.maul -> effect1().base_value();
    base_add_multiplier = p -> glyphs.maul -> effect3().percent();

    // DBC data points to base scaling, tooltip states AP scaling which is correct
    base_dd_min = base_dd_max = 35;
    direct_power_mod = 0.19;
  }

  virtual void execute()
  {
    druid_bear_attack_t::execute();

    if ( result_is_hit() )
    {
      // trigger_omen_of_clarity( this ); //FIX ME is this still true?
      trigger_infected_wounds( this );
    }
  }
  virtual void impact( player_t* t, int impact_result, double travel_dmg )
  {
    druid_bear_attack_t::impact( t, impact_result, travel_dmg );

    trigger_tier12_2pc_melee( this, direct_dmg );
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    druid_bear_attack_t::target_debuff( t, dmg_type );
    druid_t* p = player -> cast_druid();

    if ( t -> debuffs.mangle -> up() || t -> debuffs.blood_frenzy_bleed -> up() || t -> debuffs.hemorrhage -> up() )
      target_multiplier *= 1.30;

    if ( t -> debuffs.bleeding -> up() )
      target_multiplier *= 1.0 + p -> talents.rend_and_tear -> effect1().percent();
  }
};

// Pulverize ================================================================

struct pulverize_t : public druid_bear_attack_t
{
  pulverize_t( druid_t* p, const std::string& options_str ) :
    druid_bear_attack_t( "pulverize", 80313, p )
  {
    check_talent( p -> talents.pulverize -> ok() );

    parse_options( NULL, options_str );

    weapon = &( player -> main_hand_weapon );
    normalize_weapon_speed = false; // Override since bear weapon speed is already normalized
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();

    base_dd_adder = ( effect_average( 3 ) * effect_base_value( 1 ) / 100.0 ) * p -> buffs_lacerate -> stack();

    druid_bear_attack_t::execute();

    if ( result_is_hit() )
    {
      druid_t* p = player -> cast_druid();
      druid_targetdata_t* td = targetdata() -> cast_druid();
      if ( td -> dots_lacerate -> ticking )
      {
        p -> buffs_pulverize -> trigger( 1, p -> buffs_lacerate -> stack() * 0.03 );
        td -> dots_lacerate -> cancel();
        p -> buffs_lacerate -> expire();
      }
    }
  }
};

// Skull Bash (Bear) ========================================================

struct skull_bash_bear_t : public druid_bear_attack_t
{
  skull_bash_bear_t( druid_t* p, const std::string& options_str ) :
    druid_bear_attack_t( "skull_bash_bear", 80964, p )
  {
    parse_options( NULL, options_str );

    may_miss = may_resist = may_glance = may_block = may_dodge = may_parry = may_crit = false;

    cooldown -> duration  += p -> talents.brutal_impact -> effect2().seconds();
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return druid_bear_attack_t::ready();
  }
};

// Swipe (Bear) =============================================================

struct swipe_bear_t : public druid_bear_attack_t
{
  swipe_bear_t( druid_t* p, const std::string& options_str ) :
    druid_bear_attack_t( "swipe_bear", 779, p )
  {
    parse_options( NULL, options_str );

    aoe               = -1;
    direct_power_mod  = extra_coeff();
    weapon            = &( player -> main_hand_weapon );
    weapon_multiplier = 0;
  }
};

// Thrash ===================================================================

struct thrash_t : public druid_bear_attack_t
{
  thrash_t( druid_t* p, const std::string& options_str ) :
    druid_bear_attack_t( "thrash", 77758, p )
  {
    parse_options( NULL, options_str );

    aoe               = -1;
    direct_power_mod  = 0.0982;
    tick_power_mod    = 0.0167;
    weapon            = &( player -> main_hand_weapon );
    weapon_multiplier = 0;
  }
};

// ==========================================================================
// Druid Heal
// ==========================================================================

// druid_spell_t::consume_resource ==========================================

void druid_heal_t::consume_resource()
{
  heal_t::consume_resource();
  druid_t* p = player -> cast_druid();

  if ( consume_ooc && p -> buffs_omen_of_clarity -> up() )
  {
    // Treat the savings like a mana gain.
    double amount = heal_t::cost();
    if ( amount > 0 )
    {
      p -> gains_omen_of_clarity -> add( amount );
      p -> gains_omen_of_clarity -> type = RESOURCE_MANA;
      p -> buffs_omen_of_clarity -> expire();
    }
  }
}

// druid_heal_t::cost =======================================================

double druid_heal_t::cost() const
{
  druid_t* p = player -> cast_druid();

  if ( consume_ooc && p -> buffs_omen_of_clarity -> check() )
    return 0;

  double c = heal_t::cost();

  c *= 1.0 + cost_reduction();

  if ( c < 0 )
    c = 0.0;

  return c;
}

// druid_heal_t::cost_reduction =============================================

double druid_heal_t::cost_reduction() const
{
  druid_t* p = player -> cast_druid();

  double cr = 0.0;

  cr += p -> talents.moonglow -> base_value();

  return cr;
}

// druid_heal_t::execute ====================================================

void druid_heal_t::execute()
{
  druid_t* p = player -> cast_druid();

  heal_t::execute();

  if ( base_execute_time > 0 && p -> buffs_natures_swiftness -> up() )
  {
    p -> buffs_natures_swiftness -> expire();
  }

  if ( direct_dmg > 0 && ! background )
  {
    p -> buffs_harmony -> trigger( 1, p -> spells.harmony -> effect1().coeff() * 0.01 * p -> composite_mastery() );
  }
}

// druid_heal_t::execute_time ===============================================

double druid_heal_t::execute_time() const
{
  druid_t* p = player -> cast_druid();

  if ( p -> buffs_natures_swiftness -> check() )
    return 0;

  return heal_t::execute_time();
}

// druid_heal_t::haste ======================================================

double druid_heal_t::haste() const
{
  double h =  heal_t::haste();
  druid_t* p = player -> cast_druid();

  h *= 1.0 / ( 1.0 +  p -> buffs_natures_grace -> value() );

  return h;
}

// druid_heal_t::player_buff ================================================

void druid_heal_t::player_buff()
{
  heal_t::player_buff();
  druid_t* p = player -> cast_druid();

  player_multiplier *= 1.0 + additive_factors;
  player_multiplier *= 1.0 + p -> talents.master_shapeshifter -> effect1().percent();
  player_multiplier *= 1.0 + p -> buffs_tree_of_life -> value();

  if ( p -> primary_tree() == TREE_RESTORATION && direct_dmg > 0 )
  {
    player_multiplier *= 1.0 + p -> spells.harmony -> effect1().coeff() * 0.01 * p -> composite_mastery();
  }

  if ( tick_dmg > 0 )
  {
    player_multiplier *= 1.0 + p -> buffs_harmony -> value();
  }

  if ( p -> buffs_natures_swiftness -> check() && base_execute_time > 0 )
  {
    player_multiplier *= 1.0 + p -> talents.natures_swiftness -> effect2().percent();
  }
}

// Healing Touch ============================================================

struct healing_touch_t : public druid_heal_t
{
  cooldown_t* ns_cd;

  healing_touch_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "healing_touch", p, 5185 ), ns_cd( 0 )
  {
    parse_options( NULL, options_str );

    base_dd_multiplier *= 1.0 + p -> talents.empowered_touch -> mod_additive( P_GENERIC );
    base_execute_time  += p -> talents.naturalist -> mod_additive( P_CAST_TIME );
    consume_ooc         = true;

    ns_cd = p -> get_cooldown( "natures_swiftness" );
  }

  virtual void execute()
  {
    druid_heal_t::execute();
    druid_t* p = player -> cast_druid();

    trigger_empowered_touch( this );

    if ( result == RESULT_CRIT )
      trigger_living_seed( this );

    if ( p -> glyphs.healing_touch -> enabled() )
    {
      ns_cd -> ready -= p -> glyphs.healing_touch -> effect1().base_value();
    }
  }
};

// Lifebloom ================================================================

struct lifebloom_bloom_t : public druid_heal_t
{
  lifebloom_bloom_t( druid_t* p ) :
    druid_heal_t( "lifebloom_bloom", p, 0 )
  {
    background = true;
    dual       = true;
    // stats      = player -> get_stats( "lifebloom", this );
    // The stats doesn't work as expected when merging

    // All the data exists in the original lifebloom spell
    const spell_data_t* bloom = player -> dbc.spell( 33763 );
    direct_power_mod   = bloom -> effect2().coeff();
    base_dd_min        = player -> dbc.effect_min( bloom -> effect2().id(), player -> level );
    base_dd_max        = player -> dbc.effect_max( bloom -> effect2().id(), player -> level );
    school             = spell_id_t::get_school_type( bloom -> school_mask() );

    base_crit          += p -> glyphs.lifebloom -> mod_additive( P_CRIT );
    base_dd_multiplier *= 1.0 + p -> talents.gift_of_the_earthmother -> effect1().percent();
  }

  virtual void execute()
  {
    druid_targetdata_t* td = targetdata() -> cast_druid();

    base_dd_multiplier = td -> buffs_lifebloom -> check();

    druid_heal_t::execute();
  }
};

struct lifebloom_t : public druid_heal_t
{
  lifebloom_bloom_t* bloom;

  lifebloom_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "lifebloom", p, 33763 ), bloom( 0 )
  {
    parse_options( NULL, options_str );

    base_crit += p -> glyphs.lifebloom -> mod_additive( P_CRIT );
    may_crit   = false;

    additive_factors += p -> talents.genesis -> mod_additive( P_TICK_DAMAGE );

    bloom = new lifebloom_bloom_t( p );

    // TODO: this can be only cast on one target, unless Tree of Life is up
  }

  virtual double calculate_tick_damage()
  {
    druid_targetdata_t* td = targetdata() -> cast_druid();

    return druid_heal_t::calculate_tick_damage() * td -> buffs_lifebloom -> check();
  }

  virtual void execute()
  {
    druid_heal_t::execute();
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata() -> cast_druid();

    td -> buffs_lifebloom -> trigger();

    p -> trigger_replenishment();
  }

  virtual void last_tick( dot_t* d )
  {
    bloom -> execute();

    druid_heal_t::last_tick( d );
    druid_targetdata_t* td = targetdata() -> cast_druid();

    td -> buffs_lifebloom -> expire();
  }

  virtual void tick( dot_t* d )
  {
    druid_heal_t::tick( d );
    druid_t* p = player -> cast_druid();

    if ( p -> talents.malfurions_gift -> rank() )
    {
      p -> buffs_omen_of_clarity -> trigger( 1, 1, p -> talents.malfurions_gift -> proc_chance() );
    }

    if ( p -> rng_heartfire -> roll( p -> sets -> set( SET_T12_2PC_HEAL ) -> proc_chance()  ) )
      player -> resource_gain( RESOURCE_MANA,
                               player -> resource_base[ RESOURCE_MANA ] *
                               p -> sets -> set( SET_T12_2PC_HEAL ) -> effect_base_value( 1 ) / 100.0,
                               p -> gains_heartfire );

    trigger_revitalize( this );
  }
};

// Nourish ==================================================================

struct nourish_t : public druid_heal_t
{
  nourish_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "nourish", p, 50464 )
  {
    parse_options( NULL, options_str );

    base_dd_multiplier *= 1.0 + p -> talents.empowered_touch -> mod_additive( P_GENERIC );
    base_execute_time  += p -> talents.naturalist -> mod_additive( P_CAST_TIME );
  }

  virtual void execute()
  {
    druid_heal_t::execute();

    trigger_empowered_touch( this );

    if ( result == RESULT_CRIT )
      trigger_living_seed( this );
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    druid_heal_t::target_debuff( t, dmg_type );
    druid_targetdata_t* td = targetdata() -> cast_druid();

    if ( td -> hot_ticking() )
      target_multiplier *= 1.20;
  }
};

// Regrowth =================================================================

struct regrowth_t : public druid_heal_t
{
  regrowth_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "regrowth", p, 8936 )
  {
    parse_options( NULL, options_str );

    additive_factors   += p -> talents.genesis -> mod_additive( P_TICK_DAMAGE );
    base_dd_multiplier *= 1.0 + p -> talents.empowered_touch -> mod_additive( P_GENERIC );
    base_crit          += p -> talents.natures_bounty -> mod_additive( P_CRIT );
    consume_ooc         = true;
  }

  virtual void execute()
  {
    druid_heal_t::execute();
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata() -> cast_druid();

    p -> buffs_natures_grace -> trigger( 1, p -> talents.natures_grace -> base_value() / 100.0 );
    trigger_empowered_touch( this );

    if ( result == RESULT_CRIT )
      trigger_living_seed( this );

    if ( p -> glyphs.regrowth -> enabled() )
    {
      if ( target -> health_percentage() <= p -> glyphs.regrowth ->effect1().percent() && td -> dots_regrowth -> ticking )
      {
        td -> dots_regrowth -> refresh_duration();
      }
    }
  }

  virtual double execute_time() const
  {
    druid_t* p = player -> cast_druid();

    if ( p -> buffs_tree_of_life -> check() )
      return 0;

    return druid_heal_t::execute_time();
  }

  virtual void player_buff()
  {
    druid_t* p = player -> cast_druid();

    // The direct heal portion doesn't benefit from Genesis, so remove it and then add it back in
    additive_factors -= p -> talents.genesis -> mod_additive( P_TICK_DAMAGE );
    druid_heal_t::player_buff();
    additive_factors += p -> talents.genesis -> mod_additive( P_TICK_DAMAGE );
  }
};

// Rejuvenation =============================================================

struct rejuvenation_t : public druid_heal_t
{
  rejuvenation_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "rejuvenation", p, 774 )
  {
    parse_options( NULL, options_str );

    may_crit     = p -> talents.gift_of_the_earthmother -> rank() ? true : false;
    trigger_gcd += p -> talents.swift_rejuvenation -> mod_additive( P_GCD );

    additive_factors += p -> talents.genesis -> mod_additive( P_TICK_DAMAGE ) +
                        p -> talents.blessing_of_the_grove -> mod_additive( P_TICK_DAMAGE ) +
                        p -> talents.improved_rejuvenation -> mod_additive( P_TICK_DAMAGE ) +
                        p -> glyphs.rejuvenation -> mod_additive( P_TICK_DAMAGE );
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();

    druid_heal_t::execute();

    if ( p -> talents.gift_of_the_earthmother -> rank() )
    {
      base_dd_min = base_dd_max = tick_dmg * num_ticks * p -> talents.gift_of_the_earthmother -> effect2().percent();
    }
  }

  virtual void tick( dot_t* d )
  {
    druid_heal_t::tick( d );

    trigger_revitalize( this );
  }
};

// Swiftmend ================================================================

// TODO: in game, you can swiftmend other druids' hots, which is not supported here
struct swiftmend_t : public druid_heal_t
{
  swiftmend_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "swiftmend", p, 18562 )
  {
    check_spec( TREE_RESTORATION );

    parse_options( NULL, options_str );

    additive_factors += p -> talents.improved_rejuvenation -> mod_additive( P_GENERIC );
    consume_ooc       = true;
  }

  virtual void execute()
  {
    druid_heal_t::execute();
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata() -> cast_druid();

    if ( ! p -> glyphs.swiftmend -> enabled() )
    {
      // Will consume the shortest of the two HoTs if both are up
      if ( td -> dots_regrowth -> ticking && td -> dots_rejuvenation -> ticking )
      {
        if ( td -> dots_regrowth -> remains() > td -> dots_rejuvenation -> remains() )
        {
          td -> dots_rejuvenation -> cancel();
        }
        else
        {
          td -> dots_regrowth -> cancel();
        }
      }
      else if ( td -> dots_regrowth -> ticking )
      {
        td -> dots_regrowth -> cancel();
      }
      else if ( td -> dots_rejuvenation -> ticking )
      {
        td -> dots_rejuvenation -> cancel();
      }
    }

    if ( result == RESULT_CRIT )
      trigger_living_seed( this );

    trigger_efflorescence( this );
  }

  virtual bool ready()
  {
    druid_targetdata_t* td = targetdata() -> cast_druid();

    // Note: with the glyph you can use other people's regrowth/rejuv
    if ( ! ( td -> dots_regrowth -> ticking || td -> dots_rejuvenation -> ticking ) )
      return false;

    return druid_heal_t::ready();
  }
};

// Tranquility ==============================================================

struct tranquility_t : public druid_heal_t
{
  tranquility_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "tranquility", p, 740 )
  {
    parse_options( NULL, options_str );

    aoe               = effect3().base_value(); // Heals 5 targets
    base_execute_time = duration();
    channeled         = true;

    // Healing is in spell effect 1
    parse_effect_data( this -> effect_trigger_spell( 1 ), 1 ); // Initial Hit
    parse_effect_data( this -> effect_trigger_spell( 1 ), 2 ); // HoT

    // FIXME: The hot should stack

    cooldown -> duration += p -> talents.malfurions_gift -> mod_additive( P_COOLDOWN );
  }
};

// Wild Growth ==============================================================

struct wild_growth_t : public druid_heal_t
{
  wild_growth_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "wild_growth", p, 48438 )
  {
    check_talent( p -> talents.wild_growth -> ok() );

    parse_options( NULL, options_str );

    aoe = effect3().base_value() + ( int ) p -> glyphs.wild_growth -> mod_additive( P_EFFECT_3 ); // Heals 5 targets

    additive_factors += p -> talents.genesis -> mod_additive( P_TICK_DAMAGE );
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();

    if ( p -> buffs_tree_of_life -> check() )
      aoe += 2;

    druid_heal_t::execute();

    // Reset AoE
    aoe = effect3().base_value() + ( int ) p -> glyphs.wild_growth -> mod_additive( P_EFFECT_3 );
  }
};

// ==========================================================================
// Druid Spell
// ==========================================================================

// druid_spell_t::cost_reduction ============================================

double druid_spell_t::cost_reduction() const
{
  druid_t* p = player -> cast_druid();

  double   cr = 0.0;

  cr += p -> talents.moonglow -> base_value();

  return cr;
}

// druid_spell_t::cost ======================================================

double druid_spell_t::cost() const
{
  druid_t* p = player -> cast_druid();

  if ( harmful && p -> buffs_omen_of_clarity -> check() && spell_t::execute_time() )
    return 0;

  double c = spell_t::cost();

  c *= 1.0 + cost_reduction();

  if ( c < 0 )
    c = 0.0;

  return c;
}

// druid_spell_t::haste =====================================================

double druid_spell_t::haste() const
{
  double h =  spell_t::haste();
  druid_t* p = player -> cast_druid();

  h *= 1.0 / ( 1.0 +  p -> buffs_natures_grace -> value() );

  return h;
}

// druid_spell_t::execute_time ==============================================

double druid_spell_t::execute_time() const
{
  druid_t* p = player -> cast_druid();

  if ( p -> buffs_natures_swiftness -> check() )
    return 0;

  return spell_t::execute_time();
}

// druid_spell_t::schedule_execute ==========================================

void druid_spell_t::schedule_execute()
{
  spell_t::schedule_execute();
  druid_t* p = player -> cast_druid();

  if ( base_execute_time > 0 )
    p -> buffs_natures_swiftness -> expire();
}

// druid_spell_t::execute ===================================================

void druid_spell_t::execute()
{
  spell_t::execute();
  druid_t* p = player -> cast_druid();

  if ( result == RESULT_CRIT )
    p -> buffs_t11_4pc_caster -> decrement();

  if ( ! aoe )
    trigger_omen_of_clarity( this );
}

// druid_spell_t::consume_resource ==========================================

void druid_spell_t::consume_resource()
{
  spell_t::consume_resource();
  druid_t* p = player -> cast_druid();

  if ( harmful && p -> buffs_omen_of_clarity -> up() && spell_t::execute_time() )
  {
    // Treat the savings like a mana gain.
    double amount = spell_t::cost();
    if ( amount > 0 )
    {
      p -> gains_omen_of_clarity -> add( amount );
      p -> gains_omen_of_clarity -> type = RESOURCE_MANA;
      p -> buffs_omen_of_clarity -> expire();
    }
  }
}

// druid_spell_t::player_tick ===============================================

void druid_spell_t::player_tick()
{
  druid_t* p = player -> cast_druid();

  player_crit = p -> composite_spell_crit();

  player_crit += 0.05 * p -> buffs_t11_4pc_caster -> stack();
}

// druid_spell_t::player_buff ===============================================

void druid_spell_t::player_buff()
{
  druid_t* p = player -> cast_druid();

  spell_t::player_buff();

  if ( school == SCHOOL_ARCANE || school == SCHOOL_NATURE || school == SCHOOL_SPELLSTORM )
  {
    // Moonfury is actually additive with other player_multipliers, like glyphs, etc.
    if ( p -> primary_tree() == TREE_BALANCE )
    {
      double m = p -> spells.moonfury -> effect1().percent();
      additive_multiplier += m;
    }
  }

  // Add in Additive Multipliers
  player_multiplier *= 1.0 + additive_multiplier;

  // Reset Additive_Multiplier
  additive_multiplier = 0.0;

  player_crit += 0.05 * p -> buffs_t11_4pc_caster -> stack();
}

// Auto Attack ==============================================================

struct auto_attack_t : public action_t
{
  auto_attack_t( player_t* player, const std::string& /* options_str */ ) :
    action_t( ACTION_OTHER, "auto_attack", player )
  {
    trigger_gcd = 0;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();

    p -> main_hand_attack -> weapon = &( p -> main_hand_weapon );
    p -> main_hand_attack -> base_execute_time = p -> main_hand_weapon.swing_time;
    p -> main_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();

    if ( p -> is_moving() )
      return false;

    return( p -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Barkskin =================================================================

struct barkskin_t : public druid_spell_t
{
  barkskin_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "barkskin", 22812, player )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buffs_barkskin -> trigger( 1, effect2().percent() );
  }
};

// Bear Form Spell ==========================================================

struct bear_form_t : public druid_spell_t
{
  bear_form_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "bear_form", 5487, p )
  {
    parse_options( NULL, options_str );

    // Override these as we can do it before combat
    trigger_gcd       = 0;
    base_execute_time = 0;
    harmful           = false;

    if ( ! p -> bear_melee_attack )
      p -> bear_melee_attack = new bear_melee_t( p );
  }

  virtual void execute()
  {
    spell_t::execute();
    druid_t* p = player -> cast_druid();

    if ( p -> primary_tree() == TREE_FERAL )
      p -> vengeance_enabled = true;

    weapon_t* w = &( p -> main_hand_weapon );

    if ( w -> type != WEAPON_BEAST )
    {
      w -> type = WEAPON_BEAST;
      w -> school = SCHOOL_PHYSICAL;
      w -> damage = 54.8 * 2.5;
      w -> swing_time = 2.5;
    }

    // Force melee swing to restart if necessary
    if ( p -> main_hand_attack ) p -> main_hand_attack -> cancel();

    p -> main_hand_attack = p -> bear_melee_attack;
    p -> main_hand_attack -> weapon = w;

    if ( p -> buffs_cat_form     -> check() ) p -> buffs_cat_form     -> expire();
    if ( p -> buffs_moonkin_form -> check() ) p -> buffs_moonkin_form -> expire();

    p -> buffs_bear_form -> start();
    p -> base_gcd = 1.0;
    p -> reset_gcd();

    p -> dodge += 0.02 * p -> talents.feral_swiftness -> rank() + p -> talents.natural_reaction -> effect1().percent();

    if ( p -> talents.leader_of_the_pack -> rank() )
    {
      sim -> auras.leader_of_the_pack -> trigger();
    }
  }

  virtual bool ready()
  {
    druid_t* d = player -> cast_druid();
    return ! d -> buffs_bear_form -> check();
  }
};

// Berserk ==================================================================

struct berserk_t : public druid_spell_t
{
  berserk_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "berserk", 50334, p )
  {
    check_talent( p -> talents.berserk -> rank() );

    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buffs_berserk -> trigger();

    if ( p -> buffs_cat_form -> check() )
    {
      if ( p -> talents.primal_madness -> rank() )
      {
        p -> buffs_primal_madness_cat -> buff_duration = p -> buffs_berserk -> buff_duration;
        p -> buffs_primal_madness_cat -> trigger();
      }
    }
    else if ( p -> buffs_bear_form -> check() )
    {
      if ( p -> talents.primal_madness -> rank() )
      {
        p -> resource_gain( RESOURCE_RAGE, p -> talents.primal_madness -> effect1().resource( RESOURCE_RAGE ), p -> gains_primal_madness );
        p -> buffs_primal_madness_bear -> buff_duration = p -> buffs_berserk -> buff_duration;
        p -> buffs_primal_madness_bear -> trigger();
      }

      p -> cooldowns_mangle_bear -> reset();
    }
  }
};

// Cat Form Spell ===========================================================

struct cat_form_t : public druid_spell_t
{
  cat_form_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "cat_form", 768, p )
  {
    parse_options( NULL, options_str );

    // Override for precombat casting
    trigger_gcd       = 0;
    base_execute_time = 0;
    harmful           = false;

    if ( ! p -> cat_melee_attack )
      p -> cat_melee_attack = new cat_melee_t( p );
  }

  virtual void execute()
  {
    spell_t::execute();
    druid_t* p = player -> cast_druid();

    weapon_t* w = &( p -> main_hand_weapon );

    if ( w -> type != WEAPON_BEAST )
    {
      // FIXME: If we really want to model switching between forms, the old values need to be saved somewhere
      w -> type = WEAPON_BEAST;
      w -> school = SCHOOL_PHYSICAL;
      w -> min_dmg /= w -> swing_time;
      w -> max_dmg /= w -> swing_time;
      w -> damage = ( w -> min_dmg + w -> max_dmg ) / 2;
      w -> swing_time = 1.0;
    }

    // Force melee swing to restart if necessary
    if ( p -> main_hand_attack ) p -> main_hand_attack -> cancel();

    p -> main_hand_attack = p -> cat_melee_attack;
    p -> main_hand_attack -> weapon = w;

    if ( p -> buffs_bear_form    -> check() ) p -> buffs_bear_form    -> expire();
    if ( p -> buffs_moonkin_form -> check() ) p -> buffs_moonkin_form -> expire();

    p -> dodge += 0.02 * p -> talents.feral_swiftness -> rank();

    p -> buffs_cat_form -> start();
    p -> base_gcd = 1.0;
    p -> reset_gcd();

    if ( p -> talents.leader_of_the_pack -> rank() )
    {
      sim -> auras.leader_of_the_pack -> trigger();
    }
  }

  virtual bool ready()
  {
    druid_t* d = player -> cast_druid();

    return ! d -> buffs_cat_form -> check();
  }
};

// Enrage ===================================================================

struct enrage_t : public druid_spell_t
{
  enrage_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "enrage", 5229, player )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buffs_enrage -> trigger();
    p -> resource_gain( RESOURCE_RAGE, effect2().resource( RESOURCE_RAGE ), p -> gains_enrage );

    if ( p -> talents.primal_madness -> rank() )
      p -> resource_gain( RESOURCE_RAGE, p -> talents.primal_madness -> effect1().resource( RESOURCE_RAGE ), p -> gains_primal_madness );
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();

    if ( ! p -> buffs_bear_form -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Faerie Fire (Feral) ======================================================

struct faerie_fire_feral_t : public druid_spell_t
{
  faerie_fire_feral_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "faerie_fire_feral", 60089, player )
  {
    parse_options( NULL, options_str );

    base_attack_power_multiplier = 1.0;
    base_spell_power_multiplier  = 0;
    direct_power_mod             = extra_coeff();
    cooldown -> duration         = player -> dbc.spell( 16857 ) -> cooldown(); // Cooldown is stored in another version of FF
    trigger_gcd                  = player -> dbc.spell( 16857 ) -> gcd();
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();

    if ( p -> buffs_bear_form -> check() )
    {
      // The damage component is only active in (Dire) Bear Form
      spell_t::execute();
    }
    else
    {
      if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
      update_ready();
    }

    target -> debuffs.faerie_fire -> trigger( 1 + p -> talents.feral_aggression -> rank(), 0.04 );
  }
};

// Faerie Fire Spell ========================================================

struct faerie_fire_t : public druid_spell_t
{
  faerie_fire_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "faerie_fire", 770, player )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t*  p = player -> cast_druid();

    if ( result_is_hit() )
    {
      target -> debuffs.faerie_fire -> trigger( 1, 0.04 );
      target -> debuffs.faerie_fire -> source = p;
    }
  }
};

// Innervate Buff ===========================================================

struct innervate_buff_t : public buff_t
{
  innervate_buff_t( player_t* p ) :
    buff_t ( p, 29166, "innervate" )
  {}

  virtual void start( int stacks, double value )
  {
    struct innervate_event_t : public event_t
    {
      innervate_event_t ( player_t* p ) :
        event_t( p -> sim, p, "innervate" )
      { sim -> add_event( this, 1.0 ); }

      virtual void execute()
      {
        if ( player -> buffs.innervate -> check() )
        {
          player -> resource_gain( RESOURCE_MANA, player -> buffs.innervate -> value(), player -> gains.innervate );
          new ( sim ) innervate_event_t( player );
        }
      }
    };

    new ( sim ) innervate_event_t( player );

    buff_t::start( stacks, value );
  }
};

// Innervate Spell ==========================================================

struct innervate_t : public druid_spell_t
{
  int trigger;
  player_t* innervate_target;

  innervate_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "innervate", 29166, p ), trigger( 0 )
  {
    std::string target_str;
    option_t options[] =
    {
      { "trigger", OPT_INT,    &trigger    },
      { "target",  OPT_STRING, &target_str },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    harmful = false;

    // If no target is set, assume we have innervate for ourself
    innervate_target = target_str.empty() ? p : sim -> find_player( target_str );
    assert ( innervate_target != 0 );
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();

    double gain;

    if ( innervate_target == player )
    {
      gain = 0.20;
      gain += p -> talents.dreamstate -> effect1().percent();
    }
    else
    {
      // Either Dreamstate increases innervate OR you get glyph of innervate
      gain = 0.05;
      p -> buffs_glyph_of_innervate -> trigger( 1, p -> resource_max[ RESOURCE_MANA ] * 0.1 / 10.0 );
    }
    innervate_target -> buffs.innervate -> trigger( 1, p -> resource_max[ RESOURCE_MANA ] * gain / 10.0 );
  }

  virtual bool ready()
  {
    if ( ! druid_spell_t::ready() )
      return false;

    if ( trigger < 0 )
      return ( innervate_target -> resource_current[ RESOURCE_MANA ] + trigger ) < 0;

    return ( innervate_target -> resource_max    [ RESOURCE_MANA ] -
             innervate_target -> resource_current[ RESOURCE_MANA ] ) > trigger;
  }
};

// Insect Swarm Spell =======================================================

struct insect_swarm_t : public druid_spell_t
{
  cooldown_t* starsurge_cd;

  insect_swarm_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "insect_swarm", 5570, p ),
    starsurge_cd( 0 )
  {
    parse_options( NULL, options_str );

    may_crit = false;

    // Genesis, additional time is given in ms. Current structure requires it to be converted into ticks
    num_ticks   += ( int ) ( p -> talents.genesis -> mod_additive( P_DURATION ) / 2.0 );
    dot_behavior = DOT_REFRESH;

    if ( p -> primary_tree() == TREE_BALANCE )
      crit_bonus_multiplier *= 1.0 + p -> spells.moonfury -> effect2().percent();

    if ( p -> set_bonus.tier11_2pc_caster() )
      base_crit += 0.05;

    starsurge_cd = p -> cooldowns_starsurge;
  }

  virtual void player_buff()
  {
    druid_t* p = player -> cast_druid();

    // Glyph of Insect Swarm is Additive with Moonfury
    additive_multiplier += p -> glyphs.insect_swarm -> mod_additive( P_TICK_DAMAGE );

    druid_spell_t::player_buff();
  }

  virtual void tick( dot_t* d )
  {
    druid_spell_t::tick( d );
    druid_t* p = player -> cast_druid();

    if ( p -> buffs_shooting_stars -> trigger() )
      starsurge_cd -> reset();
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();

    if ( result_is_hit() )
      p -> buffs_natures_grace -> trigger( 1, p -> talents.natures_grace -> base_value() / 100.0 );
  }
};

// Mark of the Wild Spell ===================================================

struct mark_of_the_wild_t : public druid_spell_t
{
  mark_of_the_wild_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "mark_of_the_wild", p, SCHOOL_NATURE, TREE_RESTORATION )
  {
    parse_options( NULL, options_str );

    trigger_gcd = 0;
    id          = 1126;
    base_cost  *= 1.0 + p -> glyphs.mark_of_the_wild -> mod_additive( P_RESOURCE_COST ) / 100.0;
    harmful     = false;

    background = ( sim -> overrides.mark_of_the_wild != 0 );
  }

  virtual void execute()
  {
    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    for ( player_t* p = sim -> player_list; p; p = p -> next )
    {
      if ( p -> ooc_buffs() )
      {
        p -> buffs.mark_of_the_wild -> trigger();
        // Force max mana recalculation here
        p -> recalculate_resource_max( RESOURCE_MANA );
        if ( ! p -> in_combat )
          p -> resource_gain( RESOURCE_MANA, p -> resource_max[ RESOURCE_MANA ] - p -> resource_current[ RESOURCE_MANA ], 0, this );
      }
    }
  }

  virtual bool ready()
  {
    return ! player -> buffs.mark_of_the_wild -> check();
  }
};

// Moonfire Spell ===========================================================

struct moonfire_t : public druid_spell_t
{
  cooldown_t* starsurge_cd;

  moonfire_t( druid_t* p, const std::string& options_str, bool dtr=false ) :
    druid_spell_t( "moonfire", 8921, p ),
    starsurge_cd( 0 )
  {
    parse_options( NULL, options_str );

    // Genesis, additional time is given in ms. Current structure requires it to be converted into ticks
    num_ticks   += ( int ) ( p -> talents.genesis -> mod_additive( P_DURATION ) / 2.0 );
    dot_behavior = DOT_REFRESH;

    if ( p -> primary_tree() == TREE_BALANCE )
      crit_bonus_multiplier *= 1.0 + p -> spells.moonfury -> effect2().percent();

    if ( p -> set_bonus.tier11_2pc_caster() )
      base_crit += 0.05;

    starsurge_cd = p -> cooldowns_starsurge;

    may_trigger_dtr = false; // Disable the dot ticks procing DTR

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new moonfire_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void player_buff()
  {
    druid_t* p = player -> cast_druid();

    // Lunar Shower and BoG are additive with Moonfury and only apply to DD
    additive_multiplier += p -> buffs_lunar_shower -> mod_additive( P_GENERIC ) * p -> buffs_lunar_shower -> stack()
                           + p -> talents.blessing_of_the_grove -> effect2().percent();

    druid_spell_t::player_buff();
  }

  void player_buff_tick()
  {
    druid_t* p = player -> cast_druid();

    // Lunar Shower and BoG are additive with Moonfury and only apply to DD
    // So after the DD redo the player_buff w/o Lunar Shower and BotG
    additive_multiplier += p -> glyphs.moonfire -> mod_additive( P_TICK_DAMAGE );

    druid_spell_t::player_buff();
  }

  virtual void tick( dot_t* d )
  {
    druid_spell_t::tick( d );
    druid_t* p = player -> cast_druid();

    if ( p -> buffs_shooting_stars -> trigger() )
      starsurge_cd -> reset();
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata() -> cast_druid();
    dot_t* sf = td ->dots_sunfire;

    // Recalculate all those multipliers w/o Lunar Shower/BotG
    player_buff_tick();

    if ( result_is_hit() )
    {
      if ( sf -> ticking )
        sf -> cancel();

      if ( p -> talents.lunar_shower -> rank() )
      {
        if ( p -> buffs_lunar_shower -> check() )
          trigger_eclipse_gain_delay( this, ( p -> eclipse_bar_direction > 0 ? 8 : -8 ) );

        p -> buffs_lunar_shower -> trigger();
      }
      p -> buffs_natures_grace -> trigger( 1, p -> talents.natures_grace -> base_value() / 100.0 );
    }
  }

  virtual double cost_reduction() const
  {
    double cr = druid_spell_t::cost_reduction();
    druid_t* p = player -> cast_druid();

    // Cost reduction from moonglow and lunar shower is additive
    cr += ( p -> buffs_lunar_shower -> mod_additive ( P_RESOURCE_COST ) * p -> buffs_lunar_shower -> stack() );

    return cr;
  }

  virtual bool ready()
  {
    if ( ! druid_spell_t::ready() )
      return false;

    druid_t* p = player -> cast_druid();

    if ( p -> talents.sunfire -> rank() && p -> buffs_eclipse_solar -> check() )
      return false;

    return true;
  }
};

// Moonkin Form Spell =======================================================

struct moonkin_form_t : public druid_spell_t
{
  moonkin_form_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "moonkin_form", 24858, p )
  {
    check_talent( p -> talents.moonkin_form -> rank() );

    parse_options( NULL, options_str );

    // Override these as we can precast before combat begins
    trigger_gcd       = 0;
    base_execute_time = 0;
    base_cost         = 0;
    harmful           = false;
  }

  virtual void execute()
  {
    spell_t::execute();
    druid_t* p = player -> cast_druid();

    if ( p -> buffs_bear_form -> check() ) p -> buffs_bear_form -> expire();
    if ( p -> buffs_cat_form  -> check() ) p -> buffs_cat_form  -> expire();

    p -> buffs_moonkin_form -> start();

    sim -> auras.moonkin -> trigger();
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();

    return ! p -> buffs_moonkin_form -> check();
  }
};

// Natures Swiftness Spell ==================================================

struct druids_swiftness_t : public druid_spell_t
{
  cooldown_t* sub_cooldown;
  dot_t*      sub_dot;

  druids_swiftness_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "natures_swiftness", 17116, p ),
    sub_cooldown( 0 ), sub_dot( 0 )
  {
    check_talent( p -> talents.natures_swiftness -> rank() );

    parse_options( NULL, options_str );

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
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();

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

    return druid_spell_t::ready();
  }
};

// Starfire Spell ===========================================================

struct starfire_t : public druid_spell_t
{
  int extend_moonfire;

  starfire_t( druid_t* p, const std::string& options_str, bool dtr=false ) :
    druid_spell_t( "starfire", 2912, p ),
    extend_moonfire( 0 )
  {
    option_t options[] =
    {
      { "extendmf", OPT_BOOL, &extend_moonfire },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    base_execute_time += p -> talents.starlight_wrath -> mod_additive( P_CAST_TIME );

    if ( p -> primary_tree() == TREE_BALANCE )
      crit_bonus_multiplier *= 1.0 + p -> spells.moonfury -> effect2().percent();

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new starfire_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata() -> cast_druid();
    dot_t* mf = td->dots_moonfire;
    dot_t* sf = td->dots_sunfire;

    trigger_burning_treant( this );

    if ( result_is_hit() )
    {
      trigger_earth_and_moon( this );

      if ( p -> glyphs.starfire -> enabled() )
      {
        if ( mf -> ticking )
        {
          if ( mf -> added_seconds < 9.0 )
            mf -> extend_duration_seconds( 3.0 );
          else
            mf -> extend_duration_seconds( 0.0 );
        }
        else if ( sf -> ticking )
        {
          if ( sf -> added_seconds < 9.0 )
            sf -> extend_duration_seconds( 3.0 );
          else
            sf -> extend_duration_seconds( 0.0 );
        }
      }

      if ( ! p -> buffs_eclipse_solar -> check() )
      {
        // BUG (FEATURE?) ON LIVE
        // #1 Euphoria does not proc, if you are more than 35 into the side the
        // Eclipse bar is moving towards, >35 for Starfire/towards Solar
        int gain = effect2().base_value();

        // Tier12 4 piece bonus is now affected by euphoria, see
        // http://themoonkinrepository.com/viewtopic.php?f=19&t=6251
        if ( p -> set_bonus.tier12_4pc_caster() )
        {
          gain += 5;
        }

        if ( ! p -> buffs_eclipse_lunar -> check() )
        {
          if ( p -> rng_euphoria -> roll( p -> talents.euphoria -> effect1().percent() ) )
          {
            if ( ! ( p -> bugs && p -> eclipse_bar_value > 35 ) )
            {
              gain *= 2;
            }
          }
        }

        trigger_eclipse_gain_delay( this, gain );
      }
      else
      {
        // Cast starfire, but solar eclipse was up?
        p -> procs_wrong_eclipse_starfire -> occur();
      }
    }
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    druid_spell_t::target_debuff( t, dmg_type );
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata() -> cast_druid();

    // Balance, 2P -- Insect Swarm increases all damage done by your Starfire,
    // Starsurge, and Wrath spells against that target by 3%.
    if ( td -> dots_insect_swarm -> ticking )
      target_multiplier *= 1.0 + p -> set_bonus.tier13_2pc_caster() * 0.03;
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata() -> cast_druid();

    if ( extend_moonfire )
    {
      if ( ! p -> glyphs.starfire -> enabled() )
        return false;

      if ( td -> dots_moonfire -> ticking )
      {
        if ( td -> dots_moonfire -> added_seconds > 8 )
          return false;
      }
      else if ( td -> dots_sunfire -> ticking )
      {
        if ( td -> dots_sunfire -> added_seconds > 8 )
          return false;
      }
      else
        return false;
    }

    return druid_spell_t::ready();
  }
};

// Starfall Spell ===========================================================

struct starfall_star_t : public druid_spell_t
{
  starfall_star_t( druid_t* p, bool dtr=false ) :
    druid_spell_t( "starfall_star", 50288, p )
  {
    background  = true;
    dual        = true;
    direct_tick = true;
    stats       = player -> get_stats( "starfall", this );

    if ( p -> primary_tree() == TREE_BALANCE )
      crit_bonus_multiplier *= 1.0 + p -> spells.moonfury -> effect2().percent();

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new starfall_star_t( p, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void player_buff()
  {
    druid_t* p = player -> cast_druid();

    // Glyph of Focus is Additive with Moonfury
    additive_multiplier += p -> glyphs.focus -> mod_additive( P_GENERIC );

    druid_spell_t::player_buff();
  }
};

struct starfall_t : public druid_spell_t
{
  spell_t* starfall_star;

  starfall_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "starfall", 48505, p ), starfall_star( 0 )
  {
    check_talent( p -> talents.starfall -> rank() );

    parse_options( NULL, options_str );

    num_ticks      = 10;
    base_tick_time = 1.0;
    hasted_ticks   = false;
    cooldown -> duration += p -> glyphs.starfall -> mod_additive( P_COOLDOWN );

    harmful = false;

    starfall_star = new starfall_star_t( p );
  }

  virtual void tick( dot_t* d )
  {
    if ( sim -> debug ) log_t::output( sim, "%s ticks (%d of %d)", name(), d -> current_tick, d -> num_ticks );
    starfall_star -> execute();

    // If there is at least one additional target around Starfall will
    // launch 2 Stars per tick, (10*2 = 20 max stars)
    /*target_t* t = target -> cast_target();
    if ( t -> adds_nearby > 0 )
      starfall_star -> execute();*/
    stats -> add_tick( d -> time_to_tick );
  }
};

// Starsurge Spell ==========================================================

struct starsurge_t : public druid_spell_t
{
  cooldown_t* starfall_cd;

  starsurge_t( druid_t* p, const std::string& options_str, bool dtr=false ) :
    druid_spell_t( "starsurge", 78674, p ),
    starfall_cd( 0 )
  {
    check_spec( TREE_BALANCE );

    parse_options( NULL, options_str );

    if ( p -> primary_tree() == TREE_BALANCE )
      crit_bonus_multiplier *= 1.0 + p -> spells.moonfury -> effect2().percent();

    if ( p -> set_bonus.tier13_4pc_caster() )
      cooldown -> duration -= 5.0;

    starfall_cd = p -> get_cooldown( "starfall" );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new starsurge_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact( player_t* t, int impact_result,
                       double travel_dmg )
  {
    druid_spell_t::impact( t, impact_result, travel_dmg );
    druid_t* p = player -> cast_druid();

    if ( result_is_hit( impact_result ) )
    {
      // gain is positive for p -> eclipse_bar_direction==0
      // else it is towards p -> eclipse_bar_direction
      int gain = effect2().base_value();
      if ( p -> eclipse_bar_direction < 0 ) gain = -gain;

      //trigger_eclipse_energy_gain( this, gain );
      trigger_eclipse_gain_delay( this, gain );
      if ( p -> glyphs.starsurge -> ok() )
      {
        starfall_cd -> ready -= p -> glyphs.starsurge -> base_value();
      }
    }
  }

  virtual void schedule_execute()
  {
    druid_spell_t::schedule_execute();
    druid_t* p = player -> cast_druid();

    p -> buffs_shooting_stars -> expire();
  }

  virtual double execute_time() const
  {
    druid_t* p = player -> cast_druid();

    if ( p -> buffs_shooting_stars -> up() )
      return 0;

    return druid_spell_t::execute_time();
  }

  virtual bool ready()
  {
    // Druids can only have 1 Starsurge in the air at a time
    if ( travel_event != NULL )
      return false;
    else
      return druid_spell_t::ready();
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    druid_spell_t::target_debuff( t, dmg_type );
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata() -> cast_druid();

    // Balance, 2P -- Insect Swarm increases all damage done by your Starfire,
    // Starsurge, and Wrath spells against that target by 3%.
    if ( td -> dots_insect_swarm -> ticking )
      target_multiplier *= 1.0 + p -> set_bonus.tier13_2pc_caster() * 0.03;
  }
};

// Stealth ==================================================================

struct stealth_t : public spell_t
{
  stealth_t( player_t* player, const std::string& options_str ) :
    spell_t( "stealth", player )
  {
    parse_options( NULL, options_str );

    trigger_gcd = 0;
    harmful     = false;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();

    if ( sim -> log )
      log_t::output( sim, "%s performs %s", p -> name(), name() );

    p -> buffs_stealthed -> trigger();
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();

    return ! p -> buffs_stealthed -> check();
  }
};

// Sunfire Spell ============================================================

struct sunfire_t : public druid_spell_t
{
  cooldown_t* starsurge_cd;

  // Identical to moonfire, except damage type and usability

  sunfire_t( druid_t* p, const std::string& options_str, bool dtr=false ) :
    druid_spell_t( "sunfire", 93402, p ),
    starsurge_cd( 0 )
  {
    check_talent( p -> talents.sunfire -> rank() );

    parse_options( NULL, options_str );

    // Genesis, additional time is given in ms. Current structure requires it to be converted into ticks
    num_ticks   += ( int ) ( p -> talents.genesis -> mod_additive( P_DURATION ) / 2.0 );
    dot_behavior = DOT_REFRESH;

    if ( p -> primary_tree() == TREE_BALANCE )
      crit_bonus_multiplier *= 1.0 + p -> spells.moonfury -> effect2().percent();

    if ( p -> set_bonus.tier11_2pc_caster() )
      base_crit += 0.05;

    starsurge_cd = p -> cooldowns_starsurge;

    may_trigger_dtr = false; // Disable the dot ticks procing DTR

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new sunfire_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void player_buff()
  {
    druid_t* p = player -> cast_druid();

    // Lunar Shower and BoG are additive with Moonfury and only apply to DD
    additive_multiplier += p -> buffs_lunar_shower -> mod_additive( P_GENERIC ) * p -> buffs_lunar_shower -> stack()
                           + p -> talents.blessing_of_the_grove -> effect2().percent();

    druid_spell_t::player_buff();
  }

  void player_buff_tick()
  {
    druid_t* p = player -> cast_druid();

    // Lunar Shower and BoG are additive with Moonfury and only apply to DD
    // So after the DD redo the player_buff w/o Lunar Shower and BotG
    additive_multiplier += p -> glyphs.moonfire -> mod_additive( P_TICK_DAMAGE );

    druid_spell_t::player_buff();
  }

  virtual void tick( dot_t* d )
  {
    druid_spell_t::tick( d );
    druid_t* p = player -> cast_druid();

    if ( p -> buffs_shooting_stars -> trigger() )
      starsurge_cd -> reset();
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata() -> cast_druid();
    dot_t* mf = td->dots_moonfire;

    // Recalculate all those multipliers w/o Lunar Shower/BotG
    player_buff_tick();

    if ( result_is_hit() )
    {
      if ( mf -> ticking )
        mf -> cancel();

      if ( p -> talents.lunar_shower -> rank() )
      {
        if ( p -> buffs_lunar_shower -> check() )
          trigger_eclipse_gain_delay( this, -8 );

        // If moving trigger all 3 stacks, because it will stack up immediately
        p -> buffs_lunar_shower -> trigger( 1 );
      }
      p -> buffs_natures_grace -> trigger( 1, p -> talents.natures_grace -> base_value() / 100.0 );

    }
  }

  virtual double cost_reduction() const
  {
    double cr = druid_spell_t::cost_reduction();
    druid_t* p = player -> cast_druid();

    // Costreduction from moonglow and lunar shower is additive
    cr += ( p -> buffs_lunar_shower -> mod_additive( P_RESOURCE_COST ) * p -> buffs_lunar_shower -> stack() );

    return cr;
  }

  virtual bool ready()
  {
    if ( ! druid_spell_t::ready() )
      return false;

    druid_t* p = player -> cast_druid();

    if ( ! p -> buffs_eclipse_solar -> check() )
      return false;

    return true;
  }
};

// Survival Instincts =======================================================

struct survival_instincts_t : public druid_spell_t
{
  survival_instincts_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "survival_instincts", 61336, p )
  {
    check_talent( p -> talents.survival_instincts -> ok() );

    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buffs_survival_instincts -> trigger( 1, -0.5 ); // DBC value is 60 for some reason
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();

    if ( ! ( p -> buffs_cat_form -> check() || p -> buffs_bear_form -> check() ) )
      return false;

    return druid_spell_t::ready();
  }
};

// Treants Spell ============================================================

struct treants_spell_t : public druid_spell_t
{
  treants_spell_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "treants", 33831, p )
  {
    check_talent( p -> talents.force_of_nature -> ok() );

    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();

    p -> pet_treants -> summon( 30.0 );
  }
};

// Tree of Life =============================================================

struct tree_of_life_t : public druid_spell_t
{
  tree_of_life_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "tree_of_life", 33891, p )
  {
    check_talent( p -> talents.tree_of_life -> ok() );

    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buffs_tree_of_life -> trigger( 1, p -> dbc.spell( 5420 ) -> effect1().percent() );
  }
};

// Typhoon ==================================================================

struct typhoon_t : public druid_spell_t
{
  typhoon_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "typhoon", 50516, p )
  {
    check_talent( p -> talents.typhoon -> ok() );

    parse_options( NULL, options_str );

    // Damage information is stored in effect 2's trigger spell
    const spell_data_t* damage_spell = p -> dbc.spell( effect_trigger_spell( 2 ) );

    aoe                   = -1;
    base_dd_min           = p -> dbc.effect_min( damage_spell -> effect_id( 2 ), p -> level );
    base_dd_max           = p -> dbc.effect_max( damage_spell -> effect_id( 2 ), p -> level );
    base_multiplier      *= 1.0 + p -> talents.gale_winds -> effect1().percent();
    direct_power_mod      = damage_spell -> effect2().coeff();
    cooldown -> duration += p -> glyphs.monsoon -> mod_additive( P_COOLDOWN );
    base_cost            *= 1.0 + p -> glyphs.typhoon -> mod_additive( P_RESOURCE_COST );
  }
};

// Wild Mushroom ============================================================

struct wild_mushroom_t : public druid_spell_t
{
  wild_mushroom_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "wild_mushroom", 88747, player )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buffs_wild_mushroom -> trigger();
  }
};

// Wild Mushroom: Detonate ==================================================

struct wild_mushroom_detonate_t : public druid_spell_t
{
  wild_mushroom_detonate_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "wild_mushroom_detonate", 88751, player )
  {
    parse_options( NULL, options_str );

    // Actual ability is 88751, all damage is in spell 78777
    const spell_data_t* damage_spell = player -> dbc.spell( 78777 );
    direct_power_mod   = damage_spell -> effect1().coeff();
    base_dd_min        = player -> dbc.effect_min( damage_spell -> effect1().id(), player -> level );
    base_dd_max        = player -> dbc.effect_max( damage_spell -> effect1().id(), player -> level );
    school             = spell_id_t::get_school_type( damage_spell -> school_mask() );
    stats -> school    = school;
    aoe                = -1;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buffs_wild_mushroom -> expire();

    if ( result_is_hit() )
      trigger_earth_and_moon( this );
  }

  virtual void player_buff()
  {
    druid_spell_t::player_buff();
    druid_t* p = player -> cast_druid();

    player_multiplier *= p -> buffs_wild_mushroom -> stack();
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();

    if ( ! p -> buffs_wild_mushroom -> stack() )
      return false;

    return druid_spell_t::ready();
  }
};

// Wrath Spell ==============================================================

struct wrath_t : public druid_spell_t
{
  wrath_t( druid_t* p, const std::string& options_str, bool dtr=false ) :
    druid_spell_t( "wrath", 5176, p )
  {
    parse_options( NULL, options_str );

    base_execute_time += p -> talents.starlight_wrath -> mod_additive( P_CAST_TIME );
    if ( p -> primary_tree() == TREE_BALANCE )
      crit_bonus_multiplier *= 1.0 + p -> spells.moonfury -> effect2().percent();

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new wrath_t( p, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual double execute_time() const
  {
    double t = druid_spell_t::execute_time();
    druid_t* p = player ->cast_druid();

    if ( p -> buffs_tree_of_life -> check() )
      t *= 0.50;

    return t;
  }

  virtual void player_buff()
  {
    druid_t* p = player -> cast_druid();

    // Glyph of Wrath is additive with Moonfury
    if ( p -> glyphs.wrath -> ok() )
      additive_multiplier += p -> glyphs.wrath -> effect1().percent();

    druid_spell_t::player_buff();

    if ( p -> buffs_tree_of_life -> check() )
      player_multiplier *= 1.30;
  }

  virtual void target_debuff( player_t* t, int dmg_type )
  {
    druid_spell_t::target_debuff( t, dmg_type );
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata() -> cast_druid();

    // Balance, 2P -- Insect Swarm increases all damage done by your Starfire,
    // Starsurge, and Wrath spells against that target by 3%.
    if ( td -> dots_insect_swarm -> ticking )
      target_multiplier *= 1.0 + p -> set_bonus.tier13_2pc_caster() * 0.03;
  }

  virtual void impact( player_t* t, int impact_result,
                       double travel_dmg )
  {
    druid_spell_t::impact( t, impact_result, travel_dmg );
    druid_t* p = player -> cast_druid();

    if ( result_is_hit( impact_result ) )
    {
      trigger_earth_and_moon( this );

      if ( p -> eclipse_bar_direction <= 0 )
      {
        // Wrath's Eclipse gain is a bit tricky, as it is NOT just 40/3 or
        // rather 40/3*2 in case of Euphoria procs. This is how it behaves:
        // There is an internal counter, a normal wrath increases it by 1,
        // Euphoria wrath by 2. If the counter reaches >=3 your wrath will give
        // you 1 additional eclipse energy and counter will be lowered. This
        // leads to predictable patterns where Ephoria is the only random thing
        // Example, ignoring eclipse procs / elipse bar value:
        // ++:      +1  +1  +1  +1  +1  +1  +2  +2  +2
        // Count: 0  1   2   3   1   2   3   2   4   3
        // Mod 3: 0  1   2   0   1   2   0   2   1   0
        // Gain:   -13 -13 -14 -13 -13 -14 -26 -27 -27

        // Wrath's second effect base value is positive in the DBC files
        int gain = effect2().base_value();

        // Every wrath increases the counter by 1
        p -> eclipse_wrath_count++;

        // (4) Set: While not in an Eclipse state, your Wrath generates 3
        // additional Lunar Energy and your Starfire generates 5 additional
        // Solar Energy.
        // With 4T12 the 13,13,14 sequence becomes a 17,17,16 sequence, which
        // means that the counter gets increased ad additional time per wrath
        // The extra energy is not doubled by Euphoria procs

        if ( p -> set_bonus.tier12_4pc_caster() )
        {
          gain += 3;
          // 4T12 also adds 1 to the counter
          p -> eclipse_wrath_count++;
        }

        // BUG (FEATURE?) ON LIVE
        // #1 Euphoria does not proc, if you are more than 35 into the side the
        // Eclipse bar is moving towards, <-35 for Wrath/towards Lunar
        if ( ! p -> buffs_eclipse_solar -> check() )
        {
          if ( p -> rng_euphoria -> roll( p -> talents.euphoria -> effect1().percent() ) )
          {
            if ( !( p -> bugs && p -> eclipse_bar_value < -35 ) )
            {
              gain *= 2;
              // Euphoria proc also adds 1 to the counter
              p -> eclipse_wrath_count++;
            }
          }
        }

        if ( p -> eclipse_wrath_count > 2 )
        {
          gain++;
          p -> eclipse_wrath_count -= 3;
        }
        // Wrath gain is negative!
        trigger_eclipse_gain_delay( this, -gain );
      }
    }
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();

    // Cast wrath, but lunar eclipse was up?
    if ( p -> buffs_eclipse_lunar -> check() )
      p -> procs_wrong_eclipse_wrath -> occur();

    trigger_burning_treant( this );
  }
};

} // ANONYMOUS NAMESPACE ====================================================

// ==========================================================================
// Druid Character Definition
// ==========================================================================

// druid_t::create_action  ==================================================

action_t* druid_t::create_action( const std::string& name,
                                  const std::string& options_str )
{
  if ( name == "auto_attack"            ) return new            auto_attack_t( this, options_str );
  if ( name == "barkskin"               ) return new               barkskin_t( this, options_str );
  if ( name == "berserk"                ) return new                berserk_t( this, options_str );
  if ( name == "bear_form"              ) return new              bear_form_t( this, options_str );
  if ( name == "cat_form"               ) return new               cat_form_t( this, options_str );
  if ( name == "claw"                   ) return new                   claw_t( this, options_str );
  if ( name == "demoralizing_roar"      ) return new      demoralizing_roar_t( this, options_str );
  if ( name == "enrage"                 ) return new                 enrage_t( this, options_str );
  if ( name == "faerie_fire"            ) return new            faerie_fire_t( this, options_str );
  if ( name == "faerie_fire_feral"      ) return new      faerie_fire_feral_t( this, options_str );
  if ( name == "feral_charge_bear"      ) return new      feral_charge_bear_t( this, options_str );
  if ( name == "feral_charge_cat"       ) return new       feral_charge_cat_t( this, options_str );
  if ( name == "ferocious_bite"         ) return new         ferocious_bite_t( this, options_str );
  if ( name == "frenzied_regeneration"  ) return new  frenzied_regeneration_t( this, options_str );
  if ( name == "healing_touch"          ) return new          healing_touch_t( this, options_str );
  if ( name == "insect_swarm"           ) return new           insect_swarm_t( this, options_str );
  if ( name == "innervate"              ) return new              innervate_t( this, options_str );
  if ( name == "lacerate"               ) return new               lacerate_t( this, options_str );
  if ( name == "lifebloom"              ) return new              lifebloom_t( this, options_str );
  if ( name == "maim"                   ) return new                   maim_t( this, options_str );
  if ( name == "mangle_bear"            ) return new            mangle_bear_t( this, options_str );
  if ( name == "mangle_cat"             ) return new             mangle_cat_t( this, options_str );
  if ( name == "mark_of_the_wild"       ) return new       mark_of_the_wild_t( this, options_str );
  if ( name == "maul"                   ) return new                   maul_t( this, options_str );
  if ( name == "moonfire"               ) return new               moonfire_t( this, options_str );
  if ( name == "moonkin_form"           ) return new           moonkin_form_t( this, options_str );
  if ( name == "natures_swiftness"      ) return new       druids_swiftness_t( this, options_str );
  if ( name == "nourish"                ) return new                nourish_t( this, options_str );
  if ( name == "pounce"                 ) return new                 pounce_t( this, options_str );
  if ( name == "pulverize"              ) return new              pulverize_t( this, options_str );
  if ( name == "rake"                   ) return new                   rake_t( this, options_str );
  if ( name == "ravage"                 ) return new                 ravage_t( this, options_str );
  if ( name == "regrowth"               ) return new               regrowth_t( this, options_str );
  if ( name == "rejuvenation"           ) return new           rejuvenation_t( this, options_str );
  if ( name == "rip"                    ) return new                    rip_t( this, options_str );
  if ( name == "savage_roar"            ) return new            savage_roar_t( this, options_str );
  if ( name == "shred"                  ) return new                  shred_t( this, options_str );
  if ( name == "skull_bash_bear"        ) return new        skull_bash_bear_t( this, options_str );
  if ( name == "skull_bash_cat"         ) return new         skull_bash_cat_t( this, options_str );
  if ( name == "starfire"               ) return new               starfire_t( this, options_str );
  if ( name == "starfall"               ) return new               starfall_t( this, options_str );
  if ( name == "starsurge"              ) return new              starsurge_t( this, options_str );
  if ( name == "stealth"                ) return new                stealth_t( this, options_str );
  if ( name == "sunfire"                ) return new                sunfire_t( this, options_str );
  if ( name == "survival_instincts"     ) return new     survival_instincts_t( this, options_str );
  if ( name == "swipe_bear"             ) return new             swipe_bear_t( this, options_str );
  if ( name == "swipe_cat"              ) return new              swipe_cat_t( this, options_str );
  if ( name == "swiftmend"              ) return new              swiftmend_t( this, options_str );
  if ( name == "tigers_fury"            ) return new            tigers_fury_t( this, options_str );
  if ( name == "thrash"                 ) return new                 thrash_t( this, options_str );
  if ( name == "treants"                ) return new          treants_spell_t( this, options_str );
  if ( name == "tree_of_life"           ) return new           tree_of_life_t( this, options_str );
  if ( name == "tranquility"            ) return new            tranquility_t( this, options_str );
  if ( name == "typhoon"                ) return new                typhoon_t( this, options_str );
  if ( name == "wild_growth"            ) return new            wild_growth_t( this, options_str );
  if ( name == "wild_mushroom"          ) return new          wild_mushroom_t( this, options_str );
  if ( name == "wild_mushroom_detonate" ) return new wild_mushroom_detonate_t( this, options_str );
  if ( name == "wrath"                  ) return new                  wrath_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// druid_t::create_pet ======================================================

pet_t* druid_t::create_pet( const std::string& pet_name,
                            const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  if ( pet_name == "treants"        ) return new        treants_pet_t( sim, this, pet_name );
  if ( pet_name == "burning_treant" ) return new burning_treant_pet_t( sim, this );

  return 0;
}

// druid_t::create_pets =====================================================

void druid_t::create_pets()
{
  pet_treants        = create_pet( "treants" );
  pet_burning_treant = create_pet( "burning_treant" );
}

// druid_t::init_talents ====================================================

void druid_t::init_talents()
{
  talents.balance_of_power        = find_talent( "Balance of Power" );
  talents.berserk                 = find_talent( "Berserk" );
  talents.blessing_of_the_grove   = find_talent( "Blessing of the Grove" );
  talents.blood_in_the_water      = find_talent( "Blood in the Water" );
  talents.brutal_impact           = find_talent( "Brutal Impact" );
  talents.earth_and_moon          = find_talent( "Earth and Moon" );
  talents.efflorescence           = find_talent( "Efflorescence" );
  talents.empowered_touch         = find_talent( "Empowered Touch" );
  talents.endless_carnage         = find_talent( "Endless Carnage" );
  talents.euphoria                = find_talent( "Euphoria" );
  talents.dreamstate              = find_talent( "Dreamstate" );
  talents.feral_aggression        = find_talent( "Feral Aggression" );
  talents.feral_charge            = find_talent( "Feral Charge" );
  talents.feral_swiftness         = find_talent( "Feral Swiftness" );
  talents.force_of_nature         = find_talent( "Force of Nature" );
  talents.fungal_growth           = find_talent( "Fungal Growth" );
  talents.furor                   = find_talent( "Furor" );
  talents.fury_of_stormrage       = find_talent( "Fury of Stormrage" );
  talents.fury_swipes             = find_talent( "Fury Swipes" );
  talents.gale_winds              = find_talent( "Gale Winds" );
  talents.genesis                 = find_talent( "Genesis" );
  talents.gift_of_the_earthmother = find_talent( "Gift of the Earthmother" );
  talents.heart_of_the_wild       = find_talent( "Heart of the Wild" );
  talents.improved_rejuvenation   = find_talent( "Improved Rejuvenation" );
  talents.infected_wounds         = find_talent( "Infected Wounds" );
  talents.king_of_the_jungle      = find_talent( "King of the Jungle" );
  talents.leader_of_the_pack      = find_talent( "Leader of the Pack" );
  talents.living_seed             = find_talent( "Living Seed" );
  talents.lunar_shower            = find_talent( "Lunar Shower" );
  talents.malfurions_gift         = find_talent( "Malfurion's Gift" );
  talents.master_shapeshifter     = find_talent( "Master Shapeshifter" );
  talents.moonglow                = find_talent( "Moonglow" );
  talents.moonkin_form            = find_talent( "Moonkin Form" );
  talents.naturalist              = find_talent( "Naturalist" );
  talents.natural_reaction        = find_talent( "Natural Reaction" );
  talents.natural_shapeshifter    = find_talent( "Natural Shapeshifter" );
  talents.natures_bounty          = find_talent( "Nature's Bounty" );
  talents.natures_cure            = find_talent( "Nature's Cure" );
  talents.natures_grace           = find_talent( "Nature's Grace" );
  talents.natures_majesty         = find_talent( "Nature's Majesty" );
  talents.natures_swiftness       = find_talent( "Nature's Swiftness" );
  talents.natures_ward            = find_talent( "Nature's Ward" );
  talents.nurturing_instict       = find_talent( "Nurturing Instinct" );
  talents.owlkin_frenzy           = find_talent( "Owlkin Frenzy" );
  talents.predatory_strikes       = find_talent( "Predatory Strikes" );
  talents.perseverance            = find_talent( "Perseverance" );
  talents.primal_fury             = find_talent( "Primal Fury" );
  talents.primal_madness          = find_talent( "Primal Madness" );
  talents.pulverize               = find_talent( "Pulverize" );
  talents.rend_and_tear           = find_talent( "Rend and Tear" );
  talents.revitalize              = find_talent( "Revitalize" );
  talents.shooting_stars          = find_talent( "Shooting Stars" );
  talents.solar_beam              = find_talent( "Solar Beam" );
  talents.stampede                = find_talent( "Stampede" );
  talents.starfall                = find_talent( "Starfall" );
  talents.starlight_wrath         = find_talent( "Starlight Wrath" );
  talents.sunfire                 = find_talent( "Sunfire" );
  talents.survival_instincts      = find_talent( "Survival Instincts" );
  talents.swift_rejuvenation      = find_talent( "Swift Rejuvenation" );
  talents.thick_hide              = find_talent( "Thick Hide" );
  talents.tree_of_life            = find_talent( "Tree of Life" );
  talents.typhoon                 = find_talent( "Typhoon" );
  talents.wild_growth             = find_talent( "Wild Growth" );

  player_t::init_talents();
}

// druid_t::init_spells =====================================================

void druid_t::init_spells()
{
  player_t::init_spells();

  // Specializations
  spells.aggression      = spell_data_t::find( 84735, "Aggression",      dbc.ptr );
  spells.gift_of_nature  = spell_data_t::find( 87305, "Gift of Nature",  dbc.ptr );
  spells.meditation      = spell_data_t::find( 85101, "Meditation",      dbc.ptr );
  spells.moonfury        = spell_data_t::find( 16913, "Moonfury",        dbc.ptr );
  spells.razor_claws     = spell_data_t::find( 77493, "Razor Claws",     dbc.ptr );
  spells.savage_defender = spell_data_t::find( 77494, "Savage Defender", dbc.ptr );
  spells.harmony         = spell_data_t::find( 77495, "Harmony",         dbc.ptr );
  spells.total_eclipse   = spell_data_t::find( 77492, "Total Eclipse",   dbc.ptr );
  spells.vengeance       = spell_data_t::find( 84840, "Vengeance",       dbc.ptr );

  if ( talents.primal_madness -> rank() )
  {
    spells.primal_madness_cat = spell_data_t::find( talents.primal_madness -> rank() > 1 ? 80886 : 80879,
                                                   "Primal Madness", dbc.ptr );
  }
  else
    spells.primal_madness_cat = spell_data_t::nil();

  // Glyphs
  glyphs.berserk               = find_glyph( "Glyph of Berserk" );
  glyphs.bloodletting          = find_glyph( "Glyph of Bloodletting" );
  glyphs.ferocious_bite        = find_glyph( "Glyph of Ferocious Bite" );
  glyphs.focus                 = find_glyph( "Glyph of Focus" );
  glyphs.frenzied_regeneration = find_glyph( "Glyph of Frenzied Regeneration" );
  glyphs.healing_touch         = find_glyph( "Glyph of Healing Touch" );
  glyphs.innervate             = find_glyph( "Glyph of Innervate" );
  glyphs.insect_swarm          = find_glyph( "Glyph of Insect Swarm" );
  glyphs.lacerate              = find_glyph( "Glyph of Lacerate" );
  glyphs.lifebloom             = find_glyph( "Glyph of Lifebloom" );
  glyphs.mangle                = find_glyph( "Glyph of Mangle" );
  glyphs.mark_of_the_wild      = find_glyph( "Glyph of Mark of the Wild" );
  glyphs.maul                  = find_glyph( "Glyph of Maul" );
  glyphs.monsoon               = find_glyph( "Glyph of Monsoon" );
  glyphs.moonfire              = find_glyph( "Glyph of Moonfire" );
  glyphs.regrowth              = find_glyph( "Glyph of Regrowth" );
  glyphs.rejuvenation          = find_glyph( "Glyph of Rejuvenation" );
  glyphs.rip                   = find_glyph( "Glyph of Rip" );
  glyphs.savage_roar           = find_glyph( "Glyph of Savage Roar" );
  glyphs.starfall              = find_glyph( "Glyph of Starfall" );
  glyphs.starfire              = find_glyph( "Glyph of Starfire" );
  glyphs.starsurge             = find_glyph( "Glyph of Starsurge" );
  glyphs.swiftmend             = find_glyph( "Glyph of Swiftmend" );
  glyphs.tigers_fury           = find_glyph( "Glyph of Tiger's Fury" );
  glyphs.typhoon               = find_glyph( "Glyph of Typhoon" );
  glyphs.wild_growth           = find_glyph( "Glyph of Wild Growth" );
  glyphs.wrath                 = find_glyph( "Glyph of Wrath" );

  // Tier Bonuses
  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //   C2P     C4P     M2P     M4P    T2P    T4P     H2P     H4P
    {  90160,  90163,  90162,  90165,     0,     0,      0,      0 }, // Tier11
    {  99019,  99049,  99001,  99009,     0,     0,  99013,  99015 }, // Tier12
    { 105722, 105717, 105725, 105735,     0,     0, 105715, 105770 }, // Tier13
    {      0,     0,      0,       0,     0,     0,      0,      0 },
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// druid_t::init_base =======================================================

void druid_t::init_base()
{
  player_t::init_base();

  base_attack_power = level * ( level > 80 ? 3.0 : 2.0 );

  attribute_multiplier_initial[ ATTR_INTELLECT ] *= 1.0 + talents.heart_of_the_wild -> effect1().percent();
  initial_attack_power_per_strength = 1.0;
  initial_spell_power_per_intellect = 1.0;

  // FIXME! Level-specific!  Should be form-specific!
  base_miss = 0.05;
  initial_armor_multiplier  = 1.0 + talents.thick_hide -> effect1().percent();

  diminished_kfactor    = 0.009720;
  diminished_dodge_capi = 0.008555;
  diminished_parry_capi = 0.008555;

  resource_base[ RESOURCE_ENERGY ] = 100;
  resource_base[ RESOURCE_RAGE   ] = 100;

  mana_per_intellect           = 15;
  base_energy_regen_per_second = 10;

  // Furor: +5/10/15% max mana
  resource_base[ RESOURCE_MANA ] *= 1.0 + talents.furor -> effect2().percent();
  mana_per_intellect             *= 1.0 + talents.furor -> effect2().percent();

  if ( primary_tree() == TREE_RESTORATION )
    mana_regen_while_casting = spells.meditation -> effect1().percent();

  base_gcd = 1.5;
}

// druid_t::init_buffs ======================================================

void druid_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( sim, player, name, max_stack, duration, cooldown, proc_chance, quiet )
  // These have either incorrect or no values in the DBC or don't really exist
  buffs_glyph_of_innervate = new buff_t( this, "glyph_of_innervate", 1,  10.0,     0, glyphs.innervate -> enabled() );
  buffs_natures_grace      = new buff_t( this, "natures_grace"     , 1,  15.0,  60.0, talents.natures_grace -> ok() );
  buffs_omen_of_clarity    = new buff_t( this, "omen_of_clarity"   , 1,  15.0,     0, 3.5 / 60.0 );
  buffs_pulverize          = new buff_t( this, "pulverize"         , 1,  10.0 + talents.endless_carnage -> effect2().seconds() );
  buffs_revitalize         = new buff_t( this, "revitalize"        , 1,   1.0, talents.revitalize -> spell( 1 ).effect2().base_value(), talents.revitalize -> ok() ? 0.20 : 0, true );
  buffs_stampede_bear      = new buff_t( this, "stampede_bear"     , 1,   8.0,     0, talents.stampede -> ok() );
  buffs_stampede_cat       = new buff_t( this, "stampede_cat"      , 1,  10.0,     0, talents.stampede -> ok() );
  buffs_t11_4pc_caster     = new buff_t( this, "t11_4pc_caster"    , 3,   8.0,     0, set_bonus.tier11_4pc_caster() );
  buffs_t11_4pc_melee      = new buff_t( this, "t11_4pc_melee"     , 3,  30.0,     0, set_bonus.tier11_4pc_melee()  );
  buffs_t13_4pc_melee      = new buff_t( this, "t13_4pc_melee"     , 1,  10.0,     0, ( set_bonus.tier13_4pc_melee() ) ? 1.0 : 0 );
  buffs_wild_mushroom      = new buff_t( this, "wild_mushroom"     , 3,     0,     0, 1.0, true );

  // buff_t ( sim, id, name, chance, cooldown, quiet, reverse, rng_type )
  buffs_barkskin              = new buff_t( this, 22812, "barkskin" );
  buffs_eclipse_lunar         = new buff_t( this, 48518, "lunar_eclipse" );
  buffs_eclipse_solar         = new buff_t( this, 48517, "solar_eclipse" );
  buffs_enrage                = new buff_t( this, dbc.class_ability_id( type, "Enrage" ), "enrage" );
  buffs_frenzied_regeneration = new frenzied_regeneration_buff_t( this );
  buffs_frenzied_regeneration -> cooldown -> duration = 0; //CD is handled by the ability
  buffs_harmony               = new buff_t( this, 100977, "harmony" );
  buffs_lacerate              = new buff_t( this, dbc.class_ability_id( type, "Lacerate" ), "lacerate" );
  buffs_lunar_shower          = new buff_t( this, talents.lunar_shower -> effect_trigger_spell( 1 ), "lunar_shower" );
  buffs_natures_swiftness     = new buff_t( this, talents.natures_swiftness ->spell_id(), "natures_swiftness" );
  buffs_natures_swiftness -> cooldown -> duration = 0;// CD is handled by the ability
  buffs_savage_defense        = new buff_t( this, 62606, "savage_defense", 0.5 ); // Correct chance is stored in the ability, 62600
  buffs_shooting_stars        = new buff_t( this, talents.shooting_stars -> effect_trigger_spell( 1 ), "shooting_stars", talents.shooting_stars -> proc_chance() );
  buffs_survival_instincts    = new buff_t( this, talents.survival_instincts -> spell_id(), "survival_instincts" );
  buffs_tree_of_life          = new buff_t( this, talents.tree_of_life, NULL );
  buffs_tree_of_life -> buff_duration += talents.natural_shapeshifter -> mod_additive( P_DURATION );

  buffs_primal_madness_cat  = new stat_buff_t( this, "primal_madness_cat", STAT_MAX_ENERGY, spells.primal_madness_cat -> effect1().base_value() );
  buffs_primal_madness_bear = new      buff_t( this, "primal_madness_bear" );
  buffs_berserk             = new      buff_t( this, "berserk", 1, 15.0 + glyphs.berserk -> mod_additive( P_DURATION ) );
  buffs_tigers_fury         = new      buff_t( this, "tigers_fury", 1, 6.0 );

  // simple
  buffs_bear_form    = new buff_t( this, 5487,  "bear_form" );
  buffs_cat_form     = new buff_t( this, 768,   "cat_form" );
  buffs_moonkin_form = new buff_t( this, 24858, "moonkin_form" );
  buffs_savage_roar  = new buff_t( this, 52610, "savage_roar" );
  buffs_stealthed    = new buff_t( this, 5215,  "stealthed" );
}

// druid_t::init_values ====================================================

void druid_t::init_values()
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

// druid_t::init_scaling ====================================================

void druid_t::init_scaling()
{
  player_t::init_scaling();

  equipped_weapon_dps = main_hand_weapon.damage / main_hand_weapon.swing_time;

  scales_with[ STAT_WEAPON_SPEED  ] = 0;

  if ( primary_tree() == TREE_FERAL )
    scales_with[ STAT_SPIRIT ] = 0;

  // Balance of Power treats Spirit like Spell Hit Rating
  if ( talents.balance_of_power -> rank() && sim -> scaling -> scale_stat == STAT_SPIRIT )
  {
    double v = sim -> scaling -> scale_value;
    if ( ! sim -> scaling -> positive_scale_delta )
    {
      invert_scaling = 1;
      attribute_initial[ ATTR_SPIRIT ] -= v * 2;
    }
  }
}

// druid_t::init_gains ======================================================

void druid_t::init_gains()
{
  player_t::init_gains();

  gains_bear_melee            = get_gain( "bear_melee"            );
  gains_energy_refund         = get_gain( "energy_refund"         );
  gains_enrage                = get_gain( "enrage"                );
  gains_euphoria              = get_gain( "euphoria"              );
  gains_frenzied_regeneration = get_gain( "frenzied_regeneration" );
  gains_glyph_ferocious_bite  = get_gain( "glyph_ferocious_bite"  );
  gains_glyph_of_innervate    = get_gain( "glyph_of_innervate"    );
  gains_incoming_damage       = get_gain( "incoming_damage"       );
  gains_lotp_health           = get_gain( "lotp_health"           );
  gains_lotp_mana             = get_gain( "lotp_mana"             );
  gains_moonkin_form          = get_gain( "moonkin_form"          );
  gains_natural_reaction      = get_gain( "natural_reaction"      );
  gains_omen_of_clarity       = get_gain( "omen_of_clarity"       );
  gains_primal_fury           = get_gain( "primal_fury"           );
  gains_primal_madness        = get_gain( "primal_madness"        );
  gains_revitalize            = get_gain( "revitalize"            );
  gains_tigers_fury           = get_gain( "tigers_fury"           );
  gains_heartfire             = get_gain( "heartfire"             );
}

// druid_t::init_procs ======================================================

void druid_t::init_procs()
{
  player_t::init_procs();

  procs_combo_points_wasted      = get_proc( "combo_points_wasted"    );
  procs_empowered_touch          = get_proc( "empowered_touch"        );
  procs_fury_swipes              = get_proc( "fury_swipes"            );
  procs_parry_haste              = get_proc( "parry_haste"            );
  procs_primal_fury              = get_proc( "primal_fury"            );
  procs_revitalize               = get_proc( "revitalize"             );
  procs_unaligned_eclipse_gain   = get_proc( "unaligned_eclipse_gain" );
  procs_wrong_eclipse_wrath      = get_proc( "wrong_eclipse_wrath"    );
  procs_wrong_eclipse_starfire   = get_proc( "wrong_eclipse_starfire" );
  procs_burning_treant           = get_proc( "burning_treant"         );
  procs_munched_tier12_2pc_melee = get_proc( "munched_fiery_claws"    );
  procs_rolled_tier12_2pc_melee  = get_proc( "rolled_fiery_claws"     );
}

// druid_t::init_uptimes ====================================================

void druid_t::init_benefits()
{
  player_t::init_benefits();

  uptimes_energy_cap   = get_benefit( "energy_cap" );
  uptimes_rage_cap     = get_benefit( "rage_cap"   );
}

// druid_t::init_rng ========================================================

void druid_t::init_rng()
{
  player_t::init_rng();

  rng_berserk             = get_rng( "berserk"             );
  rng_blood_in_the_water  = get_rng( "blood_in_the_water"  );
  rng_empowered_touch     = get_rng( "empowered_touch"     );
  rng_euphoria            = get_rng( "euphoria"            );
  rng_fury_swipes         = get_rng( "fury_swipes"         );
  rng_primal_fury         = get_rng( "primal_fury"         );
  rng_tier12_4pc_melee    = get_rng( "tier12_4pc_melee"    );
  rng_wrath_eclipsegain   = get_rng( "wrath_eclipsegain"   );
  rng_burning_treant      = get_rng( "burning_treant_proc" );
  rng_heartfire           = get_rng( "heartfire"           );
}

// druid_t::init_actions ====================================================

void druid_t::init_actions()
{
  if ( primary_role() == ROLE_ATTACK && main_hand_weapon.type == WEAPON_NONE )
  {
    if ( ! quiet )
      sim -> errorf( "Player %s has no weapon equipped at the Main-Hand slot.", name() );
    quiet = true;
    return;
  }

  if ( action_list_str.empty() )
  {
    std::string use_str = "";
    int num_items = ( int ) items.size();
    for ( int i=0; i < num_items; i++ )
    {
      if ( items[ i ].use.active() )
      {
        use_str += "/use_item,name=";
        use_str += items[ i ].name();
      }
    }

    if ( primary_tree() == TREE_FERAL )
    {
      if ( primary_role() == ROLE_TANK )
      {
        if ( level > 80 )
        {
          action_list_str += "flask,type=steelskin/food,type=seafood_magnifique_feast";
        }
        else
        {
          action_list_str += "flask,type=endless_rage/food,type=rhinolicious_wormsteak";
        }
        action_list_str += "/mark_of_the_wild";
        action_list_str += "/bear_form";
        action_list_str += "/auto_attack";
        action_list_str += "/snapshot_stats";
        action_list_str += init_use_racial_actions();
        action_list_str += "/skull_bash_bear";
        action_list_str += "/faerie_fire_feral,if=!debuff.faerie_fire.up";
        action_list_str += "/survival_instincts"; // For now use it on CD
        action_list_str += "/barkskin"; // For now use it on CD
        action_list_str += "/enrage";
        action_list_str += use_str;
        action_list_str += init_use_profession_actions();
        action_list_str += "/maul,if=rage>=75";
        action_list_str += "/mangle_bear";
        action_list_str += "/demoralizing_roar,if=!debuff.demoralizing_roar.up";
        action_list_str += "/lacerate,if=!ticking";
        action_list_str += "/thrash";
        action_list_str += "/pulverize,if=buff.lacerate.stack=3&buff.pulverize.remains<=2";
        action_list_str += "/lacerate,if=buff.lacerate.stack<3";
        if ( talents.berserk -> rank() ) action_list_str+="/berserk";
        action_list_str += "/faerie_fire_feral";
      }
      else
      {
        std::string bitw_hp = ( set_bonus.tier13_2pc_melee() ) ? "60" : "25";
        if ( level > 80 )
        {
          action_list_str += "flask,type=winds";
          action_list_str += "/food,type=seafood_magnifique_feast";
        }
        else
        {
          action_list_str += "flask,type=endless_rage";
          action_list_str += "/food,type=hearty_rhino";
        }
        action_list_str += "/mark_of_the_wild";
        action_list_str += "/cat_form";
        action_list_str += "/snapshot_stats";

        if ( level > 80 )
        {
          action_list_str += "/tolvir_potion,if=!in_combat";
        }
        else
        {
          action_list_str += "/speed_potion,if=!in_combat";
        }

        action_list_str += "/feral_charge_cat,if=!in_combat";
        action_list_str += "/auto_attack";
        action_list_str += "/skull_bash_cat";
        if ( set_bonus.tier13_4pc_melee() )
        {
          action_list_str += "/tigers_fury,if=energy<=45&(!buff.omen_of_clarity.react)";
        }
        else
        {
          action_list_str += "/tigers_fury,if=energy<=35&(!buff.omen_of_clarity.react)";
        }
        if ( talents.berserk -> rank() )
        {
          action_list_str += "/berserk,if=buff.tigers_fury.up|(target.time_to_die<";
          action_list_str += ( glyphs.berserk -> ok() ) ? "25" : "15";
          action_list_str += "&cooldown.tigers_fury.remains>6)";
        }
        if ( level > 80 )
        {
          action_list_str += "/tolvir_potion,if=buff.bloodlust.react|target.time_to_die<=40";
        }
        else
        {
          action_list_str += "/speed_potion,if=buff.bloodlust.react|target.time_to_die<=40";
        }
        action_list_str += init_use_racial_actions();
        if ( set_bonus.tier11_4pc_melee() )
          action_list_str += "/mangle_cat,if=set_bonus.tier11_4pc_melee&buff.t11_4pc_melee.remains<4";
        action_list_str += "/faerie_fire_feral,if=debuff.faerie_fire.stack<3|!(debuff.sunder_armor.up|debuff.expose_armor.up)";
        action_list_str += "/mangle_cat,if=debuff.mangle.remains<=2&(!debuff.mangle.up|debuff.mangle.remains>=0.0)";
        action_list_str += "/ravage,if=(buff.stampede_cat.up|buff.t13_4pc_melee.up)&(buff.stampede_cat.remains<=1|buff.t13_4pc_melee.remains<=1)";

        if ( talents.blood_in_the_water -> rank() )
        {
          action_list_str += "/ferocious_bite,if=buff.combo_points.stack>=1&dot.rip.ticking&dot.rip.remains<=2.1&target.health_pct<=" + bitw_hp;
          action_list_str += "/ferocious_bite,if=buff.combo_points.stack>=5&dot.rip.ticking&target.health_pct<=" + bitw_hp;
        }
        action_list_str += use_str;
        action_list_str += init_use_profession_actions();
        action_list_str += "/shred,extend_rip=1,if=dot.rip.ticking&dot.rip.remains<=4";
        if ( talents.blood_in_the_water -> rank() )
          action_list_str += "&target.health_pct>" + bitw_hp;
        action_list_str += "/rip,if=buff.combo_points.stack>=5&target.time_to_die>=6&dot.rip.remains<2.0&(buff.berserk.up|dot.rip.remains<=cooldown.tigers_fury.remains)";
        action_list_str += "/ferocious_bite,if=buff.combo_points.stack>=5&dot.rip.remains>5.0&buff.savage_roar.remains>=3.0&buff.berserk.up";
        action_list_str += "/rake,if=target.time_to_die>=8.5&buff.tigers_fury.up&dot.rake.remains<9.0&(!dot.rake.ticking|dot.rake.multiplier<multiplier)";
        action_list_str += "/rake,if=target.time_to_die>=dot.rake.remains&dot.rake.remains<3.0&(buff.berserk.up|energy>=71|(cooldown.tigers_fury.remains+0.8)>=dot.rake.remains)";
        action_list_str += "/shred,if=buff.omen_of_clarity.react";
        action_list_str += "/savage_roar,if=buff.combo_points.stack>=1&buff.savage_roar.remains<=1";
        action_list_str += "/ravage,if=(buff.stampede_cat.up|buff.t13_4pc_melee.up)&cooldown.tigers_fury.remains=0";
        action_list_str += "/ferocious_bite,if=(target.time_to_die<=4&buff.combo_points.stack>=5)|target.time_to_die<=1";
        if ( level <= 80 )
        {
          action_list_str += "/ferocious_bite,if=buff.combo_points.stack>=5&dot.rip.remains>=8.0&buff.savage_roar.remains>=4.0";
        }
        else
        {
          action_list_str += "/ferocious_bite,if=buff.combo_points.stack>=5&dot.rip.remains>=14.0&buff.savage_roar.remains>=10.0";
        }
        action_list_str += "/ravage,if=(buff.stampede_cat.up|buff.t13_4pc_melee.up)&!buff.omen_of_clarity.react&buff.tigers_fury.up&time_to_max_energy>1.0";
        if ( set_bonus.tier11_4pc_melee() )
          action_list_str += "/mangle_cat,if=set_bonus.tier11_4pc_melee&buff.t11_4pc_melee.stack<3";
        action_list_str += "/shred,if=buff.tigers_fury.up|buff.berserk.up";
        action_list_str += "/shred,if=(buff.combo_points.stack<5&dot.rip.remains<3.0)|(buff.combo_points.stack=0&buff.savage_roar.remains<2)";
        action_list_str += "/shred,if=cooldown.tigers_fury.remains<=3.0";
        action_list_str += "/shred,if=target.time_to_die<=8.5";
        action_list_str += "/shred,if=time_to_max_energy<=1.0";
      }
    }
    else if ( primary_role() == ROLE_SPELL )
    {
      if ( level > 80 )
      {
        action_list_str += "flask,type=draconic_mind/food,type=seafood_magnifique_feast";
      }
      else
      {
        action_list_str += "flask,type=frost_wyrm/food,type=fish_feast";
      }
      action_list_str += "/mark_of_the_wild";
      if ( talents.moonkin_form -> rank() )
        action_list_str += "/moonkin_form";
      action_list_str += "/snapshot_stats";
      action_list_str += "/volcanic_potion,if=!in_combat";
      action_list_str += "/volcanic_potion,if=buff.bloodlust.react|target.time_to_die<=40";
      action_list_str += "/faerie_fire,if=debuff.faerie_fire.stack<3&!(debuff.sunder_armor.up|debuff.expose_armor.up)";
      action_list_str += "/wild_mushroom_detonate,if=buff.wild_mushroom.stack=3";
      action_list_str += init_use_racial_actions();
      action_list_str += "/insect_swarm,if=(ticks_remain<2|(dot.insect_swarm.remains<10&buff.solar_eclipse.up&eclipse<15))&(buff.solar_eclipse.up|buff.lunar_eclipse.up|time<10)";

      action_list_str += "/wild_mushroom_detonate,moving=0,if=buff.wild_mushroom.stack>0&buff.solar_eclipse.up";
      if ( talents.typhoon -> rank() )
        action_list_str += "/typhoon,moving=1";
      if ( talents.starfall -> rank() )
      {
        action_list_str += "/starfall,if=buff.lunar_eclipse.up";
      }

      const std::string renew_after = util_t::to_string( set_bonus.tier12_4pc_caster() ? 7 : 10 );
      if ( talents.sunfire -> rank() )
      {
        action_list_str += "/sunfire,if=(ticks_remain<2&!dot.moonfire.remains>0)|(eclipse<15&dot.sunfire.remains<";
        action_list_str += renew_after;
        action_list_str += ')';
      }
      action_list_str += "/moonfire,if=buff.lunar_eclipse.up&((ticks_remain<2";
      if ( talents.sunfire -> rank() )
        action_list_str += "&!dot.sunfire.remains>0";
      action_list_str += ")|(eclipse>-20&dot.moonfire.remains<" + renew_after + "))";

      if ( primary_tree() == TREE_BALANCE )
      {
        action_list_str += "/starsurge,if=buff.solar_eclipse.up|buff.lunar_eclipse.up";
      }
      action_list_str += "/innervate,if=mana_pct<50";
      if ( talents.force_of_nature -> rank() )
        action_list_str += "/treants,time>=5";
      action_list_str += use_str;
      action_list_str += init_use_profession_actions();
      if ( set_bonus.tier12_4pc_caster() )
      {
        action_list_str += "/starfire,if=eclipse_dir=1&eclipse<75";
        action_list_str += "/starfire,if=prev.wrath=1&eclipse_dir=-1&eclipse<-84";
        action_list_str += "/wrath,if=eclipse_dir=-1&eclipse>=-84";
        action_list_str += "/wrath,if=prev.starfire=1&eclipse_dir=1&eclipse>=75";
      }
      else
      {
        action_list_str += "/starfire,if=eclipse_dir=1&eclipse<80";
        action_list_str += "/starfire,if=prev.wrath=1&eclipse_dir=-1&eclipse<-87";
        action_list_str += "/wrath,if=eclipse_dir=-1&eclipse>=-87";
        action_list_str += "/wrath,if=prev.starfire=1&eclipse_dir=1&eclipse>=80";
      }
      action_list_str += "/starfire,if=eclipse_dir=1";
      action_list_str += "/wrath,if=eclipse_dir=-1";
      action_list_str += "/starfire";
      action_list_str += "/wild_mushroom,moving=1,if=buff.wild_mushroom.stack<3";

      action_list_str += "/starsurge,moving=1,if=buff.shooting_stars.react";
      action_list_str += "/moonfire,moving=1";
      action_list_str += "/sunfire,moving=1";
    }
    else
    {
      if ( level > 80 )
      {
        action_list_str += "flask,type=draconic_mind/food,type=seafood_magnifique_feast";
      }
      else
      {
        action_list_str += "flask,type=frost_wyrm/food,type=fish_feast";
      }
      action_list_str += "/mark_of_the_wild";
      action_list_str += "/snapshot_stats";
      action_list_str += "/volcanic_potion,if=!in_combat";
      action_list_str += "/volcanic_potion,if=buff.bloodlust.react|target.time_to_die<=40";
      action_list_str += "/innervate";
      if ( talents.tree_of_life -> ok() ) action_list_str += "/tree_of_life";
      action_list_str += "/healing_touch,if=buff.omen_of_clarity.up";
      action_list_str += "/rejuvenation,if=!ticking|remains<tick_time";
      action_list_str += "/lifebloom,if=buff.lifebloom.stack<3";
      action_list_str += "/swiftmend";
      if ( talents.tree_of_life -> ok() )
      {
        action_list_str += "/healing_touch,if=buff.tree_of_life.up&mana_pct>=5";
        action_list_str += "/healing_touch,if=buff.tree_of_life.down&mana_pct>=30";
      }
      else
      {
        action_list_str += "/healing_touch,if=mana_pct>=30";
      }
      action_list_str += "/nourish";
    }
    action_list_default = 1;
  }

  player_t::init_actions();
}

// druid_t::reset ===========================================================

void druid_t::reset()
{
  player_t::reset();

  eclipse_bar_value     = 0;
  eclipse_wrath_count   = 0;
  eclipse_bar_direction = 0;
  base_gcd = 1.5;
}

// druid_t::regen ===========================================================

void druid_t::regen( double periodicity )
{
  int resource_type = primary_resource();

  if ( resource_type == RESOURCE_ENERGY )
  {
    uptimes_energy_cap -> update( resource_current[ RESOURCE_ENERGY ] ==
                                  resource_max    [ RESOURCE_ENERGY ] );
  }
  else if ( resource_type == RESOURCE_MANA )
  {
    if ( buffs_glyph_of_innervate -> check() )
      resource_gain( RESOURCE_MANA, buffs_glyph_of_innervate -> value() * periodicity, gains_glyph_of_innervate );
  }
  else if ( resource_type == RESOURCE_RAGE )
  {
    if ( buffs_enrage -> up() )
      resource_gain( RESOURCE_RAGE, 1.0 * periodicity, gains_enrage );

    uptimes_rage_cap -> update( resource_current[ RESOURCE_RAGE ] ==
                                resource_max    [ RESOURCE_RAGE ] );
  }

  player_t::regen( periodicity );
}

// druid_t::available =======================================================

double druid_t::available() const
{
  if ( primary_resource() != RESOURCE_ENERGY )
    return 0.1;

  double energy = resource_current[ RESOURCE_ENERGY ];

  if ( energy > 25 ) return 0.1;

  return std::max( ( 25 - energy ) / energy_regen_per_second(), 0.1 );
}

// druid_t::combat_begin ====================================================

void druid_t::combat_begin()
{
  player_t::combat_begin();

  // Start the fight with 0 rage
  resource_current[ RESOURCE_RAGE ] = 0;

  // Moonkins can precast 3 wild mushrooms without aggroing the boss
  buffs_wild_mushroom -> trigger( 3 );
}

// druid_t::composite_armor_multiplier ======================================

double druid_t::composite_armor_multiplier() const
{
  double a = player_t::composite_armor_multiplier();

  if ( buffs_bear_form -> check() )
    a += 3.7 * ( 1.0 + 0.11 * talents.thick_hide -> rank() );

  return a;
}

// druid_t::composite_attack_power ==========================================

double druid_t::composite_attack_power() const
{
  double ap = player_t::composite_attack_power();

  if ( buffs_bear_form -> check() || buffs_cat_form  -> check() )
    ap += 2.0 * ( agility() - 10.0 );

  if ( buffs_t11_4pc_melee -> check() )
    ap *= 1.0 + buffs_t11_4pc_melee -> stack() * 0.01;

  return floor( ap );
}

// druid_t::composite_attack_power_multiplier ===============================

double druid_t::composite_attack_power_multiplier() const
{
  double multiplier = player_t::composite_attack_power_multiplier();

  if ( buffs_cat_form -> check() )
    multiplier *= 1.0 + talents.heart_of_the_wild -> effect2().percent();

  if ( primary_tree() == TREE_FERAL )
    multiplier *= 1.0 + spells.aggression -> effect1().percent();

  return multiplier;
}

// druid_t::composite_attack_crit ===========================================

double druid_t::composite_attack_crit() const
{
  double c = player_t::composite_attack_crit();

  if ( buffs_cat_form -> check() )
    c += talents.master_shapeshifter -> base_value() / 100.0;

  return floor( c * 10000.0 ) / 10000.0;
}

// druid_t::composite_player_multiplier =====================================

double druid_t::composite_player_multiplier( const school_type school, action_t* a ) const
{
  double m = player_t::composite_player_multiplier( school, a );

  if ( ( school == SCHOOL_PHYSICAL ) || ( school == SCHOOL_BLEED ) )
    m *= 1.0 + buffs_tigers_fury -> value();

  if ( school == SCHOOL_BLEED && primary_tree() == TREE_FERAL )
    m *= 1.0 + spells.razor_claws -> effect1().coeff() * 0.01 * composite_mastery();

  if ( primary_tree() == TREE_BALANCE )
  {
    // Both eclipse buffs need their own checks
    if ( school == SCHOOL_ARCANE || school == SCHOOL_SPELLSTORM )
      if ( buffs_eclipse_lunar -> up() )
        m *= 1.0 + ( buffs_eclipse_lunar -> effect1().percent()
                 + composite_mastery() * spells.total_eclipse -> effect1().coeff() * 0.01 );

    if ( school == SCHOOL_NATURE || school == SCHOOL_SPELLSTORM )
      if ( buffs_eclipse_solar -> up() )
        m *= 1.0 + ( buffs_eclipse_solar -> effect1().percent()
                 + composite_mastery() * spells.total_eclipse -> effect1().coeff() * 0.01 );
  }

  if ( buffs_moonkin_form -> check() )
    m *= 1.0 + talents.master_shapeshifter -> base_value() * 0.01;

  if ( school == SCHOOL_ARCANE || school == SCHOOL_NATURE || school == SCHOOL_SPELLSTORM )
  {
    m *= 1.0 + talents.balance_of_power -> effect1().percent();

    if ( buffs_moonkin_form -> check() )
      m *= 1.10;
  }

  m *= 1.0 + talents.earth_and_moon -> effect2().percent();

  return m;
}

// druid_t::composite_spell_hit =============================================

double druid_t::composite_spell_hit() const
{
  double hit = player_t::composite_spell_hit();

  // BoP does not convert base spirit into hit!
  hit += ( spirit() - attribute_base[ ATTR_SPIRIT ] ) * ( talents.balance_of_power -> effect2().percent() ) / rating.spell_hit;

  return hit;
}

// druid_t::composite_spell_crit ============================================

double druid_t::composite_spell_crit() const
{
  double crit = player_t::composite_spell_crit();

  crit += talents.natures_majesty -> base_value() / 100.0;

  return crit;
}

// druid_t::composite_attribute_multiplier ==================================

double druid_t::composite_attribute_multiplier( int attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  // The matching_gear_multiplier is done statically for performance reasons,
  // unfortunately that's before we're in cat form or bear form, so let's compensate here
  if ( attr == ATTR_STAMINA && buffs_bear_form -> check() )
  {
    m *= 1.0 + talents.heart_of_the_wild -> effect1().percent();

    if ( primary_tree() == TREE_FERAL )
      m *= 1.05;
  }
  else if ( attr == ATTR_AGILITY && buffs_cat_form -> check() )
  {
    if ( primary_tree() == TREE_FERAL )
      m *= 1.05;
  }

  return m;
}

// Heart of the Wild does nothing for base int so we need to do completely silly
// tricks to match paper doll in game
double druid_t::intellect() const
{
  double a = attribute_base[ ATTR_INTELLECT ];
  a *= ( 1.0 + matching_gear_multiplier( ATTR_INTELLECT ) );

  double b = attribute[ ATTR_INTELLECT ] - attribute_base[ ATTR_INTELLECT ];
  b *= ( attribute_multiplier_initial[ ATTR_INTELLECT ] );
  b *= ( 1.0 + matching_gear_multiplier( ATTR_INTELLECT ) );

  double z = floor( a + b );
  if ( buffs.mark_of_the_wild -> check() || buffs.blessing_of_kings -> check() )
    z *= 1.05;

  return z;
}

// druid_t::matching_gear_multiplier ========================================

double druid_t::matching_gear_multiplier( const attribute_type attr ) const
{
  switch ( primary_tree() )
  {
  case TREE_BALANCE:
  case TREE_RESTORATION:
    if ( attr == ATTR_INTELLECT )
      return 0.05;
    break;
  default:
    break;
  }

  return 0.0;
}

// druid_t::composite_tank_crit =============================================

double druid_t::composite_tank_crit( const school_type school ) const
{
  double c = player_t::composite_tank_crit( school );

  if ( school == SCHOOL_PHYSICAL )
    c += talents.thick_hide -> effect3().percent();

  return c;
}

// druid_t::create_expression ===============================================

action_expr_t* druid_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "eclipse" )
  {
    struct eclipse_expr_t : public action_expr_t
    {
      eclipse_expr_t( action_t* a ) : action_expr_t( a, "eclipse", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> player -> cast_druid() -> eclipse_bar_value; return TOK_NUM; }
    };
    return new eclipse_expr_t( a );
  }

  else if ( name_str == "eclipse_dir" )
  {
    struct eclipse_dir_expr_t : public action_expr_t
    {
      eclipse_dir_expr_t( action_t* a ) : action_expr_t( a, "eclipse_dir", TOK_NUM ) {}
      virtual int evaluate() { result_num = action -> player -> cast_druid() -> eclipse_bar_direction; return TOK_NUM; }
    };
    return new eclipse_dir_expr_t( a );
  }

  return player_t::create_expression( a, name_str );
}

// druid_t::decode_set ======================================================

int druid_t::decode_set( item_t& item )
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

  if ( strstr( s, "stormriders" ) )
  {
    bool is_caster = ( strstr( s, "cover"         ) ||
                       strstr( s, "shoulderwraps" ) ||
                       strstr( s, "vestment"      ) ||
                       strstr( s, "leggings"      ) ||
                       strstr( s, "gloves"        ) );

    bool is_melee = ( strstr( s, "headpiece"    ) ||
                      strstr( s, "spaulders"    ) ||
                      strstr( s, "raiment"      ) ||
                      strstr( s, "legguards"    ) ||
                      strstr( s, "grips"        ) );
    if ( is_caster ) return SET_T11_CASTER;
    if ( is_melee  ) return SET_T11_MELEE;
  }

  if ( strstr( s, "obsidian_arborweave" ) )
  {
    bool is_caster = ( strstr( s, "cover"         ) ||
                       strstr( s, "shoulderwraps" ) ||
                       strstr( s, "vestment"      ) ||
                       strstr( s, "leggings"      ) ||
                       strstr( s, "gloves"        ) );

    bool is_melee = ( strstr( s, "headpiece"    ) ||
                      strstr( s, "spaulders"    ) ||
                      strstr( s, "raiment"      ) ||
                      strstr( s, "legguards"    ) ||
                      strstr( s, "grips"        ) );
    if ( is_caster ) return SET_T12_CASTER;
    if ( is_melee  ) return SET_T12_MELEE;
  }

  if ( strstr( s, "deep_earth" ) )
  {
    bool is_caster = ( strstr( s, "cover"         ) ||
                       strstr( s, "shoulderwraps" ) ||
                       strstr( s, "vestment"      ) ||
                       strstr( s, "leggings"      ) ||
                       strstr( s, "gloves"        ) );

    bool is_melee = ( strstr( s, "headpiece"      ) ||
                      strstr( s, "spaulders"      ) ||
                      strstr( s, "raiment"        ) ||
                      strstr( s, "legguards"      ) ||
                      strstr( s, "grips"          ) );

    bool is_healer = ( strstr( s, "helm"          ) ||
                       strstr( s, "mantle"        ) ||
                       strstr( s, "robes"         ) ||
                       strstr( s, "legwraps"      ) ||
                       strstr( s, "handwraps"     ) );
    if ( is_caster ) return SET_T13_CASTER;
    if ( is_melee  ) return SET_T13_MELEE;
    if ( is_healer ) return SET_T13_HEAL;
  }

  if ( strstr( s, "_gladiators_kodohide_"   ) )   return SET_PVP_HEAL;
  if ( strstr( s, "_gladiators_wyrmhide_"   ) )   return SET_PVP_CASTER;
  if ( strstr( s, "_gladiators_dragonhide_" ) )   return SET_PVP_MELEE;

  return SET_NONE;
}

// druid_t::primary_role ====================================================

int druid_t::primary_role() const
{

  if ( primary_tree() == TREE_BALANCE )
  {
    if ( player_t::primary_role() == ROLE_HEAL )
      return ROLE_HEAL;

    return ROLE_SPELL;
  }

  else if ( primary_tree() == TREE_FERAL )
  {
    if ( player_t::primary_role() == ROLE_TANK )
      return ROLE_TANK;

    // Implement Automatic Tank detection

    return ROLE_ATTACK;
  }

  else if ( primary_tree() == TREE_RESTORATION )
  {
    if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_SPELL )
      return ROLE_SPELL;

    return ROLE_HEAL;
  }

  return ROLE_NONE;
}

// druid_t::primary_resource ================================================

int druid_t::primary_resource() const
{
  if ( primary_role() == ROLE_SPELL || primary_role() == ROLE_HEAL )
    return RESOURCE_MANA;

  if ( primary_role() == ROLE_TANK )
    return RESOURCE_RAGE;

  return RESOURCE_ENERGY;
}

// druid_t::assess_damage ===================================================

double druid_t::assess_damage( double            amount,
                               const school_type school,
                               int               dmg_type,
                               int               result,
                               action_t*         action )
{
  if ( result == RESULT_DODGE && talents.natural_reaction -> rank() )
    resource_gain( RESOURCE_RAGE, talents.natural_reaction -> effect2().base_value(), gains_natural_reaction );

  // This needs to use unmitigated damage, which amount currently is
  // FIX ME: Rage gains need to trigger on every attempt to poke the bear
  double rage_gain = amount * 18.92 / resource_max[ RESOURCE_HEALTH ];
  resource_gain( RESOURCE_RAGE, rage_gain, gains_incoming_damage );

  if ( SCHOOL_SPELL_MASK & ( int64_t( 1 ) << school ) && talents.perseverance -> ok() )
    amount *= 1.0 + talents.perseverance -> effect1().percent();

  if ( talents.natural_reaction -> rank() )
    amount *= 1.0 + talents.natural_reaction -> effect3().percent();

  if ( buffs_barkskin -> up() )
    amount *= 1.0 + buffs_barkskin -> value();

  if ( buffs_survival_instincts -> up() )
    amount *= 1.0 + buffs_survival_instincts -> value();

  // Call here to benefit from -10% physical damage before SD is taken into account
  amount = player_t::assess_damage( amount, school, dmg_type, result, action );

  if ( school == SCHOOL_PHYSICAL && buffs_savage_defense -> up() )
  {
    amount -= buffs_savage_defense -> value();

    if ( amount < 0 )
      amount = 0;

    buffs_savage_defense -> expire();
  }

  return amount;
}

player_t::heal_info_t druid_t::assess_heal(  double            amount,
                                             const school_type school,
                                             int               dmg_type,
                                             int               result,
                                             action_t*         action )
{
  amount *= 1.0 + buffs_frenzied_regeneration -> check() * glyphs.frenzied_regeneration -> effect1().percent();

  return player_t::assess_heal( amount, school, dmg_type, result, action );
}


// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_druid  ==================================================

player_t* player_t::create_druid( sim_t*             sim,
                                  const std::string& name,
                                  race_type r )
{
  return new druid_t( sim, name, r );
}

// player_t::druid_init =====================================================

void player_t::druid_init( sim_t* sim )
{
  sim -> auras.leader_of_the_pack = new aura_t( sim, "leader_of_the_pack" );
  sim -> auras.moonkin            = new aura_t( sim, "moonkin" );

  for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
    p -> buffs.innervate              = new innervate_buff_t( p );
    p -> buffs.mark_of_the_wild       = new buff_t( p, "mark_of_the_wild", !p -> is_pet() );
    p -> debuffs.demoralizing_roar    = new debuff_t( p, 99, "demoralizing_roar" );
    p -> debuffs.earth_and_moon       = new debuff_t( p, 60433, "earth_and_moon" );
    p -> debuffs.faerie_fire          = new debuff_t( p, 91565, "faerie_fire" );
    p -> debuffs.infected_wounds      = new debuff_t( p, "infected_wounds",      1,  12.0 );
    p -> debuffs.mangle               = new debuff_t( p, "mangle",               1,  60.0 );
  }

}

// player_t::druid_combat_begin =============================================

void player_t::druid_combat_begin( sim_t* sim )
{
  if ( sim -> overrides.leader_of_the_pack     ) sim -> auras.leader_of_the_pack -> override();
  if ( sim -> overrides.moonkin_aura           ) sim -> auras.moonkin            -> override();

  for ( player_t* p = sim -> player_list; p; p = p -> next )
  {
    if ( p -> ooc_buffs() )
    {
      if ( sim -> overrides.mark_of_the_wild ) p -> buffs.mark_of_the_wild -> override();
    }
  }

  for ( player_t* t = sim -> target_list; t; t = t -> next )
  {
    if ( sim -> overrides.demoralizing_roar    ) t -> debuffs.demoralizing_roar    -> override();
    if ( sim -> overrides.earth_and_moon       ) t -> debuffs.earth_and_moon       -> override( 1, 8 );
    if ( sim -> overrides.faerie_fire          ) t -> debuffs.faerie_fire          -> override( 3, 0.04 );
    if ( sim -> overrides.infected_wounds      ) t -> debuffs.infected_wounds      -> override();
    if ( sim -> overrides.mangle               ) t -> debuffs.mangle               -> override();
  }
}

