// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

// ==========================================================================
// Druid
// ==========================================================================

#if SC_DRUID == 1

#define COMBO_POINTS_MAX 5

static inline int clamp( int x, int low, int high )
{
  return x < low ? low : ( x > high ? high : x );
}

struct combo_points_t
{
  sim_t* sim;
  player_t* player;

  proc_t* proc;
  proc_t* wasted;

  int count;

  combo_points_t( player_t* p ) :
    sim( p -> sim ), player( p ), proc( 0 ), wasted( 0 ), count( 0 )
  {
    proc   = player -> get_proc( "combo_points" );
    wasted = player -> get_proc( "combo_points_wasted" );
  }

  void add( int num, const char* action = 0 )
  {
    int actual_num = clamp( num, 0, COMBO_POINTS_MAX - count );
    int overflow   = num - actual_num;

    // we count all combo points gained in the proc
    for ( int i = 0; i < num; i++ )
      proc -> occur();

    // add actual combo points
    if ( actual_num > 0 )
      count += actual_num;

    // count wasted combo points
    if ( overflow > 0 )
    {
      for ( int i = 0; i < overflow; i++ )
        wasted -> occur();
    }

    if ( sim -> log )
    {
      if ( action )
        log_t::output( sim, "%s gains %d (%d) combo_points from %s (%d)",
                       player -> name(), actual_num, num, action, count );
      else
        log_t::output( sim, "%s gains %d (%d) combo_points (%d)",
                       player -> name(), actual_num, num, count );
    }
  }

  void clear( const char* action = 0 )
  {
    if ( sim -> log )
    {
      if ( action )
        log_t::output( sim, "%s spends %d combo_points on %s",
                       player -> name(), count, action );
      else
        log_t::output( sim, "%s loses %d combo_points",
                       player -> name(), count );
    }

    count = 0;
  }

  double rank( double* cp_list ) const
  {
    assert( count > 0 );
    return cp_list[ count - 1 ];
  }

  double rank( double cp1, double cp2, double cp3, double cp4, double cp5 ) const
  {
    double cp_list[] = { cp1, cp2, cp3, cp4, cp5 };
    return rank( cp_list );
  }
};

struct druid_targetdata_t : public targetdata_t
{
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

  combo_points_t* combo_points;

  bool hot_ticking()
  {
    return dots_regrowth->ticking || dots_rejuvenation->ticking || dots_lifebloom->ticking || dots_wild_growth->ticking;
  }

  druid_targetdata_t( druid_t* source, player_t* target );

  virtual void reset()
  {
    targetdata_t::reset();

    combo_points->clear();
  }

  ~druid_targetdata_t()
  {
    delete combo_points;
  }
};

void register_druid_targetdata( sim_t* sim )
{
  player_type_e t = DRUID;
  typedef druid_targetdata_t type;

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

  //REGISTER_BUFF( combo_points );
  REGISTER_BUFF( lifebloom );
}

struct druid_t : public player_t
{
  // Active
  heal_t*   active_efflorescence;
  heal_t*   active_living_seed;

  // Pets
  pet_t* pet_treants;

  // Auto-attacks
  melee_attack_t* cat_melee_attack;
  melee_attack_t* bear_melee_attack;

  double equipped_weapon_dps;

  // Buffs
  struct buffs_t
  {
    buff_t* barkskin;
    buff_t* bear_form;
    buff_t* cat_form;
    buff_t* eclipse_lunar;
    buff_t* eclipse_solar;
    buff_t* enrage;
    buff_t* frenzied_regeneration;
    buff_t* glyph_of_innervate;
    buff_t* harmony;
    buff_t* lacerate;
    buff_t* lunar_shower;
    buff_t* moonkin_form;
    buff_t* natures_grace;
    buff_t* natures_swiftness;
    buff_t* omen_of_clarity;
    buff_t* pulverize;
    buff_t* revitalize;
    buff_t* savage_defense;
    buff_t* savage_roar;
    buff_t* shooting_stars;
    buff_t* stampede_bear;
    buff_t* stampede_cat;
    buff_t* stealthed;
    buff_t* survival_instincts;
    buff_t* t13_4pc_melee;
    buff_t* wild_mushroom;
    buff_t* berserk;
    buff_t* primal_madness_bear;
    buff_t* primal_madness_cat;
    buff_t* tigers_fury;
    buff_t* tree_of_life;
  } buff;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* lotp;
    cooldown_t* mangle_bear;
    cooldown_t* starsurge;
  } cooldown;

  // Gains
  struct gains_t
  {
    gain_t* bear_melee;
    gain_t* energy_refund;
    gain_t* enrage;
    gain_t* eclipse;
    gain_t* frenzied_regeneration;
    gain_t* glyph_of_innervate;
    gain_t* glyph_ferocious_bite;
    gain_t* incoming_damage;
    gain_t* lotp_health;
    gain_t* lotp_mana;
    gain_t* moonkin_form;
    gain_t* natural_reaction;
    gain_t* omen_of_clarity;
    gain_t* primal_fury;
    gain_t* primal_madness;
    gain_t* revitalize;
    gain_t* tigers_fury;
  } gain;

  // Glyphs
  struct glyphs_t
  {
    const spell_data_t* berserk;
    const spell_data_t* bloodletting;
    const spell_data_t* ferocious_bite;
    const spell_data_t* focus;
    const spell_data_t* frenzied_regeneration;
    const spell_data_t* healing_touch;
    const spell_data_t* innervate;
    const spell_data_t* insect_swarm;
    const spell_data_t* lacerate;
    const spell_data_t* lifebloom;
    const spell_data_t* mangle;
    const spell_data_t* mark_of_the_wild;
    const spell_data_t* maul;
    const spell_data_t* monsoon;
    const spell_data_t* moonfire;
    const spell_data_t* regrowth;
    const spell_data_t* rejuvenation;
    const spell_data_t* rip;
    const spell_data_t* savage_roar;
    const spell_data_t* starfall;
    const spell_data_t* starfire;
    const spell_data_t* starsurge;
    const spell_data_t* swiftmend;
    const spell_data_t* tigers_fury;
    const spell_data_t* typhoon;
    const spell_data_t* wild_growth;
    const spell_data_t* wrath;
  } glyph;

  // Procs
  struct procs_t
  {
    proc_t* empowered_touch;
    proc_t* parry_haste;
    proc_t* primal_fury;
    proc_t* revitalize;
    proc_t* wrong_eclipse_wrath;
    proc_t* wrong_eclipse_starfire;
    proc_t* unaligned_eclipse_gain;
  } proc;

  // Random Number Generation
  struct rngs_t
  {
    rng_t* berserk;
    rng_t* blood_in_the_water;
    rng_t* empowered_touch;
    rng_t* euphoria;
    rng_t* primal_fury;
  } rng;

  // Spell Data
  struct spells_t
  {
    // Masteries
    const spell_data_t* aggression;
    const spell_data_t* gift_of_nature;
    const spell_data_t* meditation;
    const spell_data_t* razor_claws;
    const spell_data_t* savage_defender; // NYI
    const spell_data_t* harmony;
    const spell_data_t* total_eclipse;
    const spell_data_t* vengeance;
  } spell;

  // Eclipse Management
  int eclipse_bar_value; // Tracking the current value of the eclipse bar
  int eclipse_bar_direction; // Tracking the current direction of the eclipse bar

  // Up-Times
  struct
  {
    benefit_t* energy_cap;
    benefit_t* rage_cap;
  } uptime;

  // Class Specializations
  struct
  {
    // Generic
    const spell_data_t* leahter_specialization;

    // By my shaggy bark (Restoration)
    const spell_data_t* omen_of_clarity;

    // Boom badoom boom boom badoom boomkin. (Balance)
    const spell_data_t* balance_of_power;
    const spell_data_t* celestial_focus;
    const spell_data_t* eclipse;
    const spell_data_t* eclipse_mana_gain; // http://mop.wowhead.com/spell=81070
    const spell_data_t* euphoria;
    const spell_data_t* lunar_shower;
    const spell_data_t* moonkin_form; // http://mop.wowhead.com/spell=24905
    const spell_data_t* moonkin_form_bonuses;
    const spell_data_t* owlkin_frenzy;
    const spell_data_t* shooting_stars;
    const spell_data_t* total_eclipse;
    
    // Itteh Bitteh Kitteh Speshulization Committeh. (Feral)
    // const spell_data_t* omen_of_clarity; TODO: Need to differentiate Feral/Resto version?
    const spell_data_t* razor_claws;

    // Tehm whos bare durids, can B 4 tank (Guardian)
    
  } specialization;

  // Talents
  struct talents_t
  {
    // MoP TODO: Fix/Implement
    const spell_data_t* feline_swiftness;
    const spell_data_t* displacer_beast;
    const spell_data_t* wild_charge;
    
    const spell_data_t* natures_swiftness;
    const spell_data_t* renewal;
    const spell_data_t* cenarion_ward;
    
    const spell_data_t* faerie_swarm;
    const spell_data_t* mass_entanglement;
    const spell_data_t* typhoon;
    
    const spell_data_t* soul_of_the_forest;
    const spell_data_t* incarnation;
    const spell_data_t* force_of_nature;
    
    const spell_data_t* disorienting_roar;
    const spell_data_t* ursols_vortex;
    const spell_data_t* mighty_bash;
    
    const spell_data_t* heart_of_the_wild;
    const spell_data_t* dream_of_cenarius;
    const spell_data_t* disentanglement;

    // MoP: Done
    
    // Balance
    const spell_data_t* natures_majesty;
    const spell_data_t* owlkin_frenzy;
    const spell_data_t* shooting_stars;
    const spell_data_t* solar_beam;
    const spell_data_t* starfall;
    const spell_data_t* starlight_wrath;
    const spell_data_t* sunfire;

    // Feral
    const spell_data_t* berserk;
    const spell_data_t* blood_in_the_water;
    const spell_data_t* brutal_impact;
    const spell_data_t* endless_carnage;
    const spell_data_t* feral_aggression;
    const spell_data_t* feral_charge;
    const spell_data_t* feral_swiftness;
    const spell_data_t* furor;
    const spell_data_t* infected_wounds;
    const spell_data_t* king_of_the_jungle;
    const spell_data_t* leader_of_the_pack;
    const spell_data_t* natural_reaction;
    const spell_data_t* nurturing_instict; // NYI
    const spell_data_t* predatory_strikes;
    const spell_data_t* primal_fury;
    const spell_data_t* primal_madness;
    const spell_data_t* pulverize;
    const spell_data_t* rend_and_tear;
    const spell_data_t* stampede;
    const spell_data_t* survival_instincts;
    const spell_data_t* thick_hide; // How does the +10% and +33%@bear stack etc

    // Resto
    const spell_data_t* blessing_of_the_grove;
    const spell_data_t* efflorescence;
    const spell_data_t* empowered_touch;
    const spell_data_t* fury_of_stormrage; // NYI
    const spell_data_t* gift_of_the_earthmother;
    const spell_data_t* improved_rejuvenation;
    const spell_data_t* living_seed;
    const spell_data_t* malfurions_gift;
    const spell_data_t* master_shapeshifter;
    const spell_data_t* natural_shapeshifter;
    const spell_data_t* natures_bounty;
    const spell_data_t* natures_cure; // NYI
    const spell_data_t* natures_ward; // NYI
    const spell_data_t* naturalist;
    const spell_data_t* perseverance;
    const spell_data_t* revitalize;
    const spell_data_t* swift_rejuvenation;
    const spell_data_t* tree_of_life;
    const spell_data_t* wild_growth;

  } talent;

  druid_t( sim_t* sim, const std::string& name, race_type_e r = RACE_NIGHT_ELF ) :
    player_t( sim, DRUID, name, r )
  {
    active_efflorescence      = 0;
    active_living_seed        = 0;

    pet_treants         = 0;

    eclipse_bar_value     = 0;
    eclipse_bar_direction = 0;

    cooldown.lotp           = get_cooldown( "lotp"           );
    cooldown.mangle_bear    = get_cooldown( "mangle_bear"    );
    cooldown.starsurge      = get_cooldown( "starsurge"      );

    cat_melee_attack = 0;
    bear_melee_attack = 0;

    equipped_weapon_dps = 0;

    distance = ( primary_tree() == DRUID_FERAL ) ? 3 : 30;
    default_distance = distance;

    create_options();
  }

  // Character Definition
  virtual druid_targetdata_t* new_targetdata( player_t* target )
  { return new druid_targetdata_t( this, target ); }
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
  virtual void      regen( timespan_t periodicity );
  virtual timespan_t available() const;
  virtual double    composite_armor_multiplier() const;
  virtual double    composite_attack_power() const;
  virtual double    composite_player_multiplier( school_type_e school, const action_t* a = NULL ) const;
  virtual double    composite_spell_hit() const;
  virtual double    composite_spell_crit() const;
  virtual double    composite_attribute_multiplier( attribute_type_e attr ) const;
  virtual double    matching_gear_multiplier( attribute_type_e attr ) const;
  virtual double    composite_block_value() const { return 0; }
  virtual double    composite_tank_parry() const { return 0; }
  virtual double    composite_tank_block() const { return 0; }
  virtual double    composite_tank_crit( school_type_e school ) const;
  virtual expr_t*   create_expression( action_t*, const std::string& name );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  buff_t*   create_buff( const spell_data_t* sd, const std::string& token );
  buff_t*   create_buff( int id, const std::string& token );
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual int       decode_set( const item_t& ) const;
  virtual resource_type_e primary_resource() const;
  virtual role_type_e primary_role() const;
  virtual double    assess_damage( double amount, school_type_e school, dmg_type_e, result_type_e, action_t* a );
  virtual heal_info_t assess_heal( double amount, school_type_e school, dmg_type_e, result_type_e, action_t* a );
  virtual double    composite_attribute( attribute_type_e attr ) const;


  void reset_gcd()
  {
    for ( size_t i = 0; i < action_list.size(); ++i )
    {
      action_t* a = action_list[ i ];
      if ( a -> trigger_gcd != timespan_t::zero() ) a -> trigger_gcd = base_gcd;
    }
  }
};

druid_targetdata_t::druid_targetdata_t( druid_t* source, player_t* target )
  : targetdata_t( source, target )
{
  combo_points = new combo_points_t( target );

  buffs_lifebloom = add_aura( buff_creator_t( this, "lifebloom", source -> find_class_spell( "Lifebloom" ) ) );
  buffs_lifebloom -> buff_duration = timespan_t::from_seconds( 11.0 ); // Override duration so the bloom works correctly
}

namespace { // ANONYMOUS NAMESPACE ==========================================

// ==========================================================================
// Druid Cat Attack
// ==========================================================================

struct druid_cat_melee_attack_t : public melee_attack_t
{
  int requires_stealth;
  int requires_position;
  bool requires_combo_points;
  int adds_combo_points;
  int combo_points_spent;

  druid_cat_melee_attack_t( const std::string& token, druid_t* p, const spell_data_t* s ) :
    melee_attack_t( token, p, s ),
    requires_stealth( 0 ),
    requires_position( POSITION_NONE ),
    requires_combo_points( false ),
    adds_combo_points( 0 ), combo_points_spent( 0 )
  {
    tick_may_crit = true;
  }

  druid_cat_melee_attack_t( druid_t* p, const spell_data_t* s ) :
    melee_attack_t( "", p, s ),
    adds_combo_points( 0 )
  {
    if ( p -> dbc.spell( id ) -> effect1().type() == E_ADD_COMBO_POINTS )
      adds_combo_points   = ( int ) player -> dbc.spell( id ) -> effect1().base_value();
    else if ( p -> dbc.spell( id ) -> effect2().type() == E_ADD_COMBO_POINTS )
      adds_combo_points   = ( int ) player -> dbc.spell( id ) -> effect2().base_value();
    else if ( p -> dbc.spell( id ) -> effect3().type() == E_ADD_COMBO_POINTS )
      adds_combo_points   = ( int ) player -> dbc.spell( id ) -> effect3().base_value();


    may_crit              = true;
    tick_may_crit         = true;
    requires_combo_points = false;
    requires_position     = POSITION_NONE;
    requires_stealth      = false;
  }

  druid_t* p() const
  { return static_cast<druid_t*>( player ); }

  virtual double cost() const;
  virtual void   execute();
  virtual void   consume_resource();
  virtual void   player_buff();
  virtual bool   ready();
};

// ==========================================================================
// Druid Bear Attack
// ==========================================================================

struct druid_bear_melee_attack_t : public melee_attack_t
{
  druid_bear_melee_attack_t( const std::string& token, druid_t* p, const spell_data_t* s ) :
    melee_attack_t( token, p, s )
  {}

  druid_bear_melee_attack_t( druid_t* p, const spell_data_t* s ) :
    melee_attack_t( "", p, s )
  {
    may_crit      = true;
    tick_may_crit = true;
  }

  druid_t* p() const
  { return static_cast<druid_t*>( player ); }

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

  druid_heal_t( const std::string& token, druid_t* p, const uint32_t id ) :
    heal_t( token, p, p -> find_spell( id ) ),
    additive_factors( 0 ), consume_ooc( false )
  {
    dot_behavior      = DOT_REFRESH;
    may_crit          = true;
    tick_may_crit     = true;
    weapon_multiplier = 0;

    if ( p -> primary_tree() == DRUID_RESTORATION )
    {
      additive_factors += p -> spell.gift_of_nature -> effect1().percent();
    }
  }

  druid_t* p() const
  { return static_cast<druid_t*>( player ); }

  virtual void   consume_resource();
  virtual double cost() const;
  virtual double cost_reduction() const;
  virtual void   execute();
  virtual timespan_t execute_time() const;
  virtual double haste() const;
  virtual void   player_buff();
};

// ==========================================================================
// Druid Spell
// ==========================================================================

struct druid_spell_t : public spell_t
{
  double additive_multiplier;

  druid_spell_t( const std::string& token, druid_t* p, 
                 const spell_data_t* s = spell_data_t::nil(), const std::string& options = std::string() ) :
    spell_t( token, p, s ), 
    additive_multiplier( 0.0 )
  {
    parse_options( 0, options );
  }

  druid_spell_t( druid_t* p, const spell_data_t* s = spell_data_t::nil(), const std::string& options = std::string() ) :
    spell_t( "", p, s ),
    additive_multiplier( 0.0 )
  {
    parse_options( 0, options );

    may_crit      = true;
    tick_may_crit = true;
  }

  druid_t* p() const
  { return static_cast<druid_t*>( player ); }

  virtual void   consume_resource();
  virtual double cost() const;
  virtual double cost_reduction() const;
  virtual void   execute();
  virtual timespan_t execute_time() const;
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
  struct melee_t : public melee_attack_t
  {
    melee_t( player_t* player ) :
      melee_attack_t( "treant_melee", player )
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

  treants_pet_t( sim_t* sim, druid_t* owner, const std::string& pet_name ) :
    pet_t( sim, owner, pet_name )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 580;
    main_hand_weapon.max_dmg    = 580;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.65 );
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
    
    stats_base.attack_crit  = .05;
    stats_base.attack_power = -20;
    stats_initial.attack_power_per_strength = 2.0;

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

  virtual double composite_attack_expertise( const weapon_t* ) const
  {
    // Hit scales that if they are hit capped, you're expertise capped.
    return owner -> composite_spell_hit() * 26.0 / 17.0;
  }

  virtual void schedule_ready( timespan_t delta_time=timespan_t::zero(),
                               bool   waiting=false )
  {
    pet_t::schedule_ready( delta_time, waiting );
    if ( ! main_hand_attack -> execute_event ) main_hand_attack -> execute();
  }
};

// trigger_eclipse_proc =====================================================

static void trigger_eclipse_proc( druid_t* p )
{
  // All extra procs when eclipse pops
  p -> resource_gain( RESOURCE_MANA, p -> resources.max[ RESOURCE_MANA ] * p -> specialization.eclipse_mana_gain -> effect1().percent(), p -> gain.eclipse );
  
  p -> buff.natures_grace -> cooldown -> reset();
  p -> buff.natures_grace -> trigger();
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
    p -> proc.unaligned_eclipse_gain -> occur();
    return;
  }
  else if ( p -> eclipse_bar_direction ==  1 && gain < 0 )
  {
    p -> proc.unaligned_eclipse_gain -> occur();
    return;
  }

  int old_eclipse_bar_value = p -> eclipse_bar_value;
  p -> eclipse_bar_value += gain;

  if ( p -> eclipse_bar_value <= 0 )
  {
    p -> buff.eclipse_solar -> expire();

    if ( p -> eclipse_bar_value < -100 )
      p -> eclipse_bar_value = -100;
  }

  if ( p -> eclipse_bar_value >= 0 )
  {
    p -> buff.eclipse_lunar -> expire();

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
      if ( p -> buff.eclipse_solar -> trigger() )
      {
        trigger_eclipse_proc( p );
        // Solar proc => bar direction changes to -1 (towards Lunar)
        p -> eclipse_bar_direction = -1;
      }
    }
    else if ( p -> eclipse_bar_value == -100 )
    {
      if ( p -> buff.eclipse_lunar -> trigger() )
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

// trigger_empowered_touch ==================================================

static void trigger_empowered_touch( heal_t* a )
{
  druid_t* p = a -> player -> cast_druid();
  druid_targetdata_t* td = a -> targetdata( a -> target ) -> cast_druid();

  if ( p -> rng.empowered_touch -> roll( p -> talent.empowered_touch -> effect2().percent() ) )
  {
    if ( td -> dots_lifebloom -> ticking )
    {
      td -> dots_lifebloom -> refresh_duration();
      if ( td -> buffs_lifebloom -> check() ) td -> buffs_lifebloom -> refresh();
      p -> proc.empowered_touch -> occur();
    }
  }
}

// trigger_energy_refund ====================================================

static void trigger_energy_refund( druid_cat_melee_attack_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  double energy_restored = a -> resource_consumed * 0.80;

  p -> resource_gain( RESOURCE_ENERGY, energy_restored, p -> gain.energy_refund );
}

// trigger_living_seed ======================================================

static void trigger_living_seed( heal_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( ! p -> talent.living_seed -> ok() )
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
  double heal = a -> direct_dmg * p -> talent.living_seed -> effect1().percent();
  p -> active_living_seed -> base_dd_min = heal;
  p -> active_living_seed -> base_dd_max = heal;
  p -> active_living_seed -> execute();
};

// trigger_lotp =============================================================

static void trigger_lotp( action_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( ! p -> talent.leader_of_the_pack -> ok() )
    return;

  if ( p -> cooldown.lotp -> remains() > timespan_t::zero() )
    return;

  // Has to do damage and can't be a proc
  if ( ( a -> direct_dmg <= 0 && a -> tick_dmg <= 0 ) || a -> proc )
    return;

  p -> resource_gain( RESOURCE_HEALTH,
                      p -> resources.max[ RESOURCE_HEALTH ] * p -> dbc.spell( 24932 ) -> effect2().percent(),
                      p -> gain.lotp_health );

  p -> resource_gain( RESOURCE_MANA,
                      p -> resources.max[ RESOURCE_MANA ] * p -> talent.leader_of_the_pack -> effect1().percent(),
                      p -> gain.lotp_mana );

  p -> cooldown.lotp -> start( timespan_t::from_seconds( 6.0 ) );
};

// trigger_omen_of_clarity ==================================================

static void trigger_omen_of_clarity( action_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( a -> proc ) return;

  p -> buff.omen_of_clarity -> trigger();
}

// trigger_primal_fury (Bear) ===============================================

static void trigger_primal_fury( druid_bear_melee_attack_t* a )
{
/*  druid_t* p = a -> player -> cast_druid();

  if ( ! p -> talent.primal_fury -> ok() )
    return;

  // Has to do damage and can't be a proc
  if ( ( a -> direct_dmg <= 0 && a -> tick_dmg <= 0 ) || a -> proc )
    return;

  const spell_data_t* primal_fury = p -> dbc.spell( p -> talent.primal_fury -> effect1().trigger_spell_id() );

  if ( p -> rng.primal_fury -> roll( primal_fury -> proc_chance() ) )
  {
    p -> resource_gain( RESOURCE_RAGE,
                        p -> dbc.spell( primal_fury -> effect1().trigger_spell_id() ) -> effect1().base_value() / 10,
                        p -> gain.primal_fury );
  }
*/
}

// trigger_primal_fury (Cat) ================================================

static void trigger_primal_fury( druid_cat_melee_attack_t* a )
{
  /*
  druid_t* p = a -> player -> cast_druid();
  druid_targetdata_t * td = a -> targetdata( a -> target ) -> cast_druid();

  if ( ! p -> talent.primal_fury -> ok() )
    return;

  if ( ! a -> adds_combo_points )
    return;

  const spell_data_t* blood_frenzy = p -> dbc.spell( p -> talent.primal_fury -> effect2().trigger_spell_id() );

  if ( p -> rng.primal_fury -> roll( blood_frenzy -> proc_chance() ) )
  {
    td -> combo_points -> add( adds_combo_points, name() );
    p -> proc.primal_fury -> occur();
  }
  */
}

// trigger_rage_gain ========================================================

static void trigger_rage_gain( druid_bear_melee_attack_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  double rage_gain = 16.0;

  p -> resource_gain( RESOURCE_RAGE, rage_gain, p -> gain.bear_melee );
}

// trigger_revitalize =======================================================

static void trigger_revitalize( druid_heal_t* a )
{
  druid_t* p = a -> player -> cast_druid();

  if ( p -> buff.revitalize -> trigger() )
  {
    p -> proc.revitalize -> occur();
    p -> resource_gain( RESOURCE_MANA,
                        p -> resources.max[ RESOURCE_MANA ] * p -> talent.revitalize -> effect1().percent(),
                        p -> gain.revitalize );
  }
}

// ==========================================================================
// Druid Cat Attack
// ==========================================================================

// druid_cat_melee_attack_t::cost =================================================

double druid_cat_melee_attack_t::cost() const
{
  double c = melee_attack_t::cost();
  druid_t* p = player -> cast_druid();

  if ( c == 0 )
    return 0;

  if ( harmful &&  p -> buff.omen_of_clarity -> check() )
    return 0;

  if ( p -> buff.berserk -> check() )
    c *= 1.0 + p -> talent.berserk -> effect1().percent();

  return c;
}

// druid_cat_melee_attack_t::consume_resource =====================================

void druid_cat_melee_attack_t::consume_resource()
{
  melee_attack_t::consume_resource();
  druid_t* p = player -> cast_druid();
  
  combo_points_spent = 0;
  if ( requires_combo_points && result_is_hit())
  {
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();
    combo_points_spent = td -> combo_points -> count;
    td -> combo_points -> clear( name() );
  }

  if ( harmful && p -> buff.omen_of_clarity -> up() )
  {
    // Treat the savings like a energy gain.
    double amount = melee_attack_t::cost();
    if ( amount > 0 )
    {
      p -> gain.omen_of_clarity -> add( RESOURCE_ENERGY, amount );
      p -> buff.omen_of_clarity -> expire();
    }
  }
}

// druid_cat_melee_attack_t::execute ==============================================

void druid_cat_melee_attack_t::execute()
{
  druid_t* p = player -> cast_druid();
  druid_targetdata_t* td = targetdata( target ) -> cast_druid();

  melee_attack_t::execute();

  if ( result_is_hit() )
  {
    if ( adds_combo_points ) td -> combo_points -> add( adds_combo_points, name() );

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

  if ( harmful ) p -> buff.stealthed -> expire();
}

// druid_cat_melee_attack_t::player_buff ==========================================

void druid_cat_melee_attack_t::player_buff()
{
  melee_attack_t::player_buff();
}

// druid_cat_melee_attack_t::ready ================================================

bool druid_cat_melee_attack_t::ready()
{
  if ( ! melee_attack_t::ready() )
    return false;

  druid_t*  p = player -> cast_druid();
  druid_targetdata_t* td = targetdata( target ) -> cast_druid();

  if ( ! p -> buff.cat_form -> check() )
    return false;

  if ( requires_position != POSITION_NONE )
    if ( p -> position != requires_position )
      return false;

  if ( requires_stealth )
    if ( ! p -> buff.stealthed -> check() )
      return false;

  if ( requires_combo_points && ! td -> combo_points -> count )
    return false;

  return true;
}

// Cat Melee Attack =========================================================

struct cat_melee_t : public druid_cat_melee_attack_t
{
  cat_melee_t( druid_t* player ) :
    druid_cat_melee_attack_t( "cat_melee", player, spell_data_t::nil() )
  {
    background  = true;
    repeating   = true;
    may_crit    = true;
    trigger_gcd = timespan_t::zero();
    stateless   = true;
    special     = false;
  }

  virtual void player_buff()
  {
    druid_cat_melee_attack_t::player_buff();
    druid_t* p = player -> cast_druid();

    player_multiplier *= 1.0 + p -> buff.savage_roar -> value();
  }

  virtual timespan_t execute_time() const
  {
    if ( ! player -> in_combat )
      return timespan_t::from_seconds( 0.01 );

    return druid_cat_melee_attack_t::execute_time();
  }

  virtual void execute()
  {
    druid_cat_melee_attack_t::execute();

    if ( result_is_hit() )
    {
      trigger_omen_of_clarity( this );
    }
  }
};

// Claw =====================================================================

struct claw_t : public druid_cat_melee_attack_t
{
  claw_t( druid_t* player, const std::string& options_str ) :
    druid_cat_melee_attack_t( "claw", player, player -> find_class_spell( "Claw" ) )
  {
    parse_options( NULL, options_str );
  }
};

// Feral Charge (Cat) =======================================================

struct feral_charge_cat_t : public druid_cat_melee_attack_t
{
  // TODO: Figure out Wile Charge 
  /*
  double stampede_cost_reduction;
  timespan_t stampede_duration;

  feral_charge_cat_t( druid_t* p, const std::string& options_str ) :
    druid_cat_melee_attack_t( "feral_charge_cat", 49376, p ),
    stampede_cost_reduction( 0.0 ), stampede_duration( timespan_t::zero() )
  {
    parse_options( NULL, options_str );

    if ( p -> talent.stampede -> ok() )
    {
      uint32_t sid = ( p -> talent.stampede -> ok() == 2 ) ? 81022 : 81021;
      spell_id_t stampede_spell( p, "stampede_cat_spell", sid );
      stampede_cost_reduction = stampede_spell.mod_additive( P_RESOURCE_COST );
      // 81022 is bugged and says FLAT_MODIFIER not PCT_MODIFIER so adjust it
      if ( sid == 81022 )
      {
        stampede_cost_reduction /= 100.0;
      }
      stampede_cost_reduction = -stampede_cost_reduction;
      stampede_duration = stampede_spell.duration();
    }

    p -> buff.stampede_cat -> buff_duration = stampede_duration;

    may_miss   = false;
    may_dodge  = false;
    may_parry  = false;
    may_block  = false;
    may_glance = false;
  }

  virtual void execute()
  {
    druid_cat_melee_attack_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buff.stampede_cat -> trigger( 1, stampede_cost_reduction );
  }

  virtual bool ready()
  {
    bool ranged = ( player -> position == POSITION_RANGED_FRONT ||
                    player -> position == POSITION_RANGED_BACK );

    if ( player -> in_combat && ! ranged )
    {
      return false;
    }

    return druid_cat_melee_attack_t::ready();
  }
  */
};

// Ferocious Bite ===========================================================

struct ferocious_bite_t : public druid_cat_melee_attack_t
{
  double base_dmg_per_point;
  double excess_energy;
  double max_excess_energy;

  ferocious_bite_t( druid_t* player, const std::string& options_str ) :
    druid_cat_melee_attack_t( "ferocious_bite", player, player -> find_class_spell( "Ferocious Bite" )  ),
    base_dmg_per_point( 0 ), excess_energy( 0 ), max_excess_energy( 0 )
  {
    parse_options( NULL, options_str );

    base_dmg_per_point    = data().effect1().bonus( player );
    //base_multiplier      *= 1.0 + player -> talent.feral_aggression -> mod_additive( P_GENERIC );

    max_excess_energy     = data().effect2().base_value();
    requires_combo_points = true;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();

    base_dd_adder    = base_dmg_per_point * td -> combo_points -> count;
    direct_power_mod = 0.109 * td -> combo_points -> count;

    // consumes up to 35 additional energy to increase damage by up to 100%.
    // Assume 100/35 = 2.857% per additional energy consumed

    excess_energy = ( p -> resources.current[ RESOURCE_ENERGY ] - druid_cat_melee_attack_t::cost() );

    if ( excess_energy > max_excess_energy )
    {
      excess_energy = max_excess_energy;
    }
    else if ( excess_energy < 0 )
    {
      excess_energy = 0;
    }

    druid_cat_melee_attack_t::execute();

    if ( result_is_hit() )
    {
      if ( p -> glyph.ferocious_bite -> ok() )
      {
        double amount = p -> resources.max[ RESOURCE_HEALTH ] *
                        p -> glyph.ferocious_bite -> effect1().percent() *
                        ( ( int ) ( excess_energy + druid_cat_melee_attack_t::cost() ) / 10 );
        p -> resource_gain( RESOURCE_HEALTH, amount, p -> gain.glyph_ferocious_bite );
      }
    }

    double health_percentage = ( p -> set_bonus.tier13_2pc_melee() ) ? 60.0 : p -> talent.blood_in_the_water -> effect1().base_value();
    if ( result_is_hit() && target -> health_percentage() <= health_percentage )
    {
      // Proc chance is not stored in the talent anymore
      if ( td -> dots_rip -> ticking && p -> rng.blood_in_the_water -> roll( p -> talent.blood_in_the_water -> ok() * 0.50 ) )
      {
        td -> dots_rip -> refresh_duration();
      }
    }
  }

  virtual void consume_resource()
  {
    // Ferocious Bite consumes 35+x energy, with 0 <= x <= 35.
    // Consumes the base_cost and handles Omen of Clarity
    druid_cat_melee_attack_t::consume_resource();

    if ( result_is_hit() )
    {
      // Let the additional energy consumption create it's own debug log entries.
      if ( sim -> debug )
        log_t::output( sim, "%s consumes an additional %.1f %s for %s", player -> name(),
                       excess_energy, util_t::resource_type_string( current_resource() ), name() );

      player -> resource_loss( current_resource(), excess_energy );
      stats -> consume_resource( current_resource(), excess_energy );
    }
  }

  virtual void player_buff()
  {
    druid_cat_melee_attack_t::player_buff();
    druid_t* p = player -> cast_druid();

    player_multiplier *= 1.0 + excess_energy / max_excess_energy;

    if ( target -> debuffs.bleeding -> check() )
      player_crit += p -> talent.rend_and_tear -> effect2().percent();
  }
};

// Frenzied Regeneration Buff ===============================================

struct frenzied_regeneration_buff_t : public buff_t
{
  int health_gain;

  frenzied_regeneration_buff_t( druid_t* p ) :
    buff_t( buff_creator_t( p, 22842, "frenzied_regeneration" ) ), health_gain( 0 )
  { }

  virtual void start( int stacks, double value, timespan_t duration )
  {
    druid_t* p = player -> cast_druid();

    // Kick off rage to health 'hot'
    if ( ! p -> glyph.frenzied_regeneration -> ok() )
    {
      struct frenzied_regeneration_event_t : public event_t
      {
        stats_t* rage_stats;

        frenzied_regeneration_event_t ( druid_t* p ) :
          event_t( p -> sim, p, "frenzied_regeneration_heal" ),
          rage_stats( 0 )
        {
          sim -> add_event( this, timespan_t::from_seconds( 1.0 ) );
          rage_stats = p -> get_stats( "frenzied_regeneration" );
        }

        virtual void execute()
        {
          druid_t* p = player -> cast_druid();

          if ( p -> buff.frenzied_regeneration -> check() )
          {
            int rage_consumed = ( int ) ( std::min( p -> resources.current[ RESOURCE_RAGE ], 10.0 ) );
            double health_pct = p -> dbc.spell( 22842 ) -> effect1().percent() / 100;
            double rage_health = rage_consumed * health_pct * p -> resources.max[ RESOURCE_HEALTH ];
            p -> resource_gain( RESOURCE_HEALTH, rage_health, p -> gain.frenzied_regeneration );
            p -> resource_loss( RESOURCE_RAGE, rage_consumed );
            rage_stats -> consume_resource( RESOURCE_RAGE, rage_consumed );
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

    double health_pct = data().effect2().percent();

    health_gain = ( int ) floor( player -> resources.max[ RESOURCE_HEALTH ] * health_pct );
    p -> stat_gain( STAT_MAX_HEALTH, health_gain );

    // Ability also heals to 30% if not at that amount
    if ( p -> resources.current[ RESOURCE_HEALTH ] < health_gain )
    {
      p -> resource_gain( RESOURCE_HEALTH, health_gain - p -> resources.current[ RESOURCE_HEALTH ], p -> gain.frenzied_regeneration );
    }

    buff_t::start( stacks, value, duration );
  }

  virtual void expire()
  {
    druid_t* p = player -> cast_druid();
    p -> stat_loss( STAT_MAX_HEALTH, health_gain );

    buff_t::expire();
  }
};

// Frenzied Regeneration ====================================================

struct frenzied_regeneration_t : public druid_bear_melee_attack_t
{
  frenzied_regeneration_t( druid_t* player, const std::string& options_str ) :
    druid_bear_melee_attack_t( "frenzied_regeneration", player, player -> find_class_spell( "Frenzied Regeneration" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;

    num_ticks = 0; // No need for this to tick, handled in the buff
  }

  virtual void execute()
  {
    druid_bear_melee_attack_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buff.frenzied_regeneration -> trigger();
  }
};

// Maim =====================================================================

struct maim_t : public druid_cat_melee_attack_t
{
  double base_dmg_per_point;

  maim_t( druid_t* player, const std::string& options_str ) :
    druid_cat_melee_attack_t( "maim", player, player -> find_class_spell( "Maim" ) ), base_dmg_per_point( 0 )
  {
    parse_options( NULL, options_str );

    base_dmg_per_point    = data().effect1().bonus( player );
    requires_combo_points = true;
  }

  virtual void execute()
  {
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();

    base_dd_adder = base_dmg_per_point * td -> combo_points -> count;

    druid_cat_melee_attack_t::execute();
  }
};

// Mangle (Cat) =============================================================

struct mangle_cat_t : public druid_cat_melee_attack_t
{
  int extend_rip;

  mangle_cat_t( druid_t* player, const std::string& options_str ) :
    druid_cat_melee_attack_t( "mangle_cat", player, player -> find_class_spell( "Mangle" ) ),
    extend_rip( 0 )
  {
    option_t options[] =
    {
      { "extend_rip", OPT_BOOL, &extend_rip },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    adds_combo_points = 1; // Not picked up from the DBC
    //base_multiplier  *= 1.0 + player -> glyph.mangle -> mod_additive( P_GENERIC );
  }

  virtual void execute()
  {
    druid_cat_melee_attack_t::execute();
    if ( result_is_hit() )
    {
      druid_t* p = player -> cast_druid();
      druid_targetdata_t* td = targetdata( target ) -> cast_druid();

      if ( p -> glyph.bloodletting -> ok() &&
           td -> dots_rip -> ticking  &&
           td -> dots_rip -> added_ticks < 4 )
      {
        // Glyph adds 1/1/2 ticks on execute
        int extra_ticks = ( td -> dots_rip -> added_ticks < 2 ) ? 1 : 2;
        td -> dots_rip -> extend_duration( extra_ticks );
      }
    }
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();

    if ( extend_rip )
      if ( ! p -> glyph.bloodletting -> ok() ||
           ! td -> dots_rip -> ticking ||
           ( td -> dots_rip -> added_ticks == 4 ) )
        return false;

    return druid_cat_melee_attack_t::ready();
  }
};

// Pounce ===================================================================

struct pounce_bleed_t : public druid_cat_melee_attack_t
{
  pounce_bleed_t( druid_t* player ) :
    druid_cat_melee_attack_t( "pounce_bleed", player, player -> dbc.spell( 9007 ) )
  {
    background     = true;
    tick_power_mod = 0.03;
  }
};

struct pounce_t : public druid_cat_melee_attack_t
{
  pounce_bleed_t* pounce_bleed;

  pounce_t( druid_t* player, const std::string& options_str ) :
    druid_cat_melee_attack_t( "pounce", player, player -> find_class_spell( "Pounce" ) ), pounce_bleed( 0 )
  {
    parse_options( NULL, options_str );

    requires_stealth = true;
    pounce_bleed     = new pounce_bleed_t( player );
  }

  virtual void init()
  {
    druid_cat_melee_attack_t::init();

    pounce_bleed -> stats = stats;
  }

  virtual void execute()
  {
    druid_cat_melee_attack_t::execute();

    if ( result_is_hit() )
      pounce_bleed -> execute();
  }
};

// Rake =====================================================================

struct rake_t : public druid_cat_melee_attack_t
{
  rake_t( druid_t* player, const std::string& options_str ) :
    druid_cat_melee_attack_t( "rake", player, player -> find_class_spell( "Rake" ) )
  {
    parse_options( NULL, options_str );

    dot_behavior        = DOT_REFRESH;
    direct_power_mod    = 0.147;
    tick_power_mod      = 0.147;
    num_ticks          += player -> talent.endless_carnage -> ok();
    base_dd_min = base_dd_max = base_td;
  }
};

// Ravage ===================================================================

struct ravage_t : public druid_cat_melee_attack_t
{
  ravage_t( druid_t* player, const std::string& options_str ) :
    druid_cat_melee_attack_t( "ravage", player, player -> find_specialization_spell( "Ravage" ) )
  {
    parse_options( NULL, options_str );

    requires_position = POSITION_BACK;
    requires_stealth  = true;
  }

  virtual void execute()
  {
    druid_cat_melee_attack_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buff.stampede_cat -> decrement();
    p -> buff.t13_4pc_melee -> decrement();
    requires_stealth = true;
    requires_position = POSITION_BACK;

    p -> buff.stampede_cat -> up();
  }

  virtual double cost() const
  {
    druid_t* p = player -> cast_druid();

    double c = druid_cat_melee_attack_t::cost();

    if ( p -> buff.t13_4pc_melee -> up() )
      return 0;

    c *= 1.0 - p -> buff.stampede_cat -> value();

    return c;
  }

  virtual void consume_resource()
  {
    melee_attack_t::consume_resource();
    druid_t* p = player -> cast_druid();

    if ( p -> buff.omen_of_clarity -> up() && ! p -> buff.stampede_cat -> check() && ! p -> buff.t13_4pc_melee -> check() )
    {
      // Treat the savings like a energy gain.
      double amount = melee_attack_t::cost();
      if ( amount > 0 )
      {
        p -> gain.omen_of_clarity -> add( RESOURCE_ENERGY, amount );
        p -> buff.omen_of_clarity -> expire();
      }
    }
  }

  virtual void player_buff()
  {
    druid_t* p = player -> cast_druid();

    if ( p -> talent.predatory_strikes -> ok() )
    {
      if ( target -> health_percentage() >= p -> talent.predatory_strikes -> effect2().base_value() )
        player_crit += p -> talent.predatory_strikes -> effect1().percent();
    }

    druid_cat_melee_attack_t::player_buff();
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();

    if ( p -> buff.stampede_cat -> check() )
    {
      requires_stealth = false;
    }

    if ( p -> buff.t13_4pc_melee -> check() )
    {
      requires_stealth = false;
      requires_position = POSITION_NONE;
    }

    return druid_cat_melee_attack_t::ready();
  }
};

// Rip ======================================================================

struct rip_t : public druid_cat_melee_attack_t
{
  double base_dmg_per_point;
  double ap_per_point;

  rip_t( druid_t* player, const std::string& options_str ) :
    druid_cat_melee_attack_t( "rip", player, player -> find_specialization_spell( "Rip" ) ),
    base_dmg_per_point( 0 ), ap_per_point( 0 )
  {
    parse_options( NULL, options_str );

    base_dmg_per_point    = data().effect1().bonus( player );//player -> dbc.effect_bonus( effect_id( 1 ), player -> level );
    ap_per_point          = 0.0207;
    requires_combo_points = true;
    may_crit              = false;
    //base_multiplier      *= 1.0 + player -> glyph.rip -> mod_additive( P_TICK_DAMAGE );

    dot_behavior          = DOT_REFRESH;
  }

  virtual void execute()
  {
    // We have to save these values for refreshes by Blood in the Water, so
    // we simply reset them to zeroes on each execute and check them in ::player_buff.
    base_td        = 0.0;
    tick_power_mod = 0.0;

    druid_cat_melee_attack_t::execute();
  }

  virtual void player_buff()
  {
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();

    if ( base_td == 0.0 && tick_power_mod == 0.0 )
    {
      base_td        = base_td_init + td -> combo_points -> count * base_dmg_per_point;
      tick_power_mod = td -> combo_points -> count * ap_per_point;
    }

    druid_cat_melee_attack_t::player_buff();
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();

    if ( td -> dots_rip -> ticking )
    {
      double b = base_td_init + td -> combo_points -> count * base_dmg_per_point;
      double t = td -> combo_points -> count * ap_per_point;
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
    return druid_cat_melee_attack_t::ready();
  }
};

// Savage Roar ==============================================================

struct savage_roar_t : public druid_cat_melee_attack_t
{
  double buff_value;

  savage_roar_t( druid_t* player, const std::string& options_str ) :
    druid_cat_melee_attack_t( "savage_roar", player, player -> find_specialization_spell( "Savage Roar" ) )
  {
    parse_options( NULL, options_str );

    buff_value            = data().effect2().percent();
    buff_value           += player -> glyph.savage_roar -> effect1().base_value() / 100.0;
    harmful               = false;
    requires_combo_points = true;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();

    timespan_t duration = this -> data().duration() + timespan_t::from_seconds( 5.0 ) * td -> combo_points -> count;
    duration += p -> talent.endless_carnage -> effect2().time_value();

    // execute clears CP, so has to be after calculation duration
    druid_cat_melee_attack_t::execute();

    p -> buff.savage_roar -> buff_duration = duration;
    p -> buff.savage_roar -> trigger( 1, buff_value );
  }
};

// Shred ====================================================================

struct shred_t : public druid_cat_melee_attack_t
{
  int extend_rip;

  shred_t( druid_t* player, const std::string& options_str ) :
    druid_cat_melee_attack_t( "shred", player, player -> find_specialization_spell( "Shred" ) ),
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
    druid_cat_melee_attack_t::execute();
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();

    if ( result_is_hit() )
    {
      if ( p -> glyph.bloodletting -> ok() &&
           td -> dots_rip -> ticking  &&
           td -> dots_rip -> added_ticks < 4 )
      {
        // Glyph adds 1/1/2 ticks on execute
        int extra_ticks = ( td -> dots_rip -> added_ticks < 2 ) ? 1 : 2;
        td -> dots_rip -> extend_duration( extra_ticks );
      }
    }
  }

  virtual void target_debuff( player_t* t, dmg_type_e dtype )
  {
    druid_cat_melee_attack_t::target_debuff( t, dtype );

    if ( t -> debuffs.bleeding -> up() )
      target_multiplier *= 1.20;
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();

    if ( extend_rip )
      if ( ! p -> glyph.bloodletting -> ok() ||
           ! td -> dots_rip -> ticking ||
           ( td -> dots_rip -> added_ticks == 4 ) )
        return false;

    return druid_cat_melee_attack_t::ready();
  }
};

// Skull Bash (Cat) =========================================================

struct skull_bash_cat_t : public druid_cat_melee_attack_t
{
  skull_bash_cat_t( druid_t* player, const std::string& options_str ) :
    druid_cat_melee_attack_t( "skull_bash_cat", player, player -> find_specialization_spell( "Skull Bash" ) )
  {
    parse_options( NULL, options_str );

    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;

    cooldown -> duration += player -> talent.brutal_impact -> effect3().time_value();
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return druid_cat_melee_attack_t::ready();
  }
};

// Swipe (Cat) ==============================================================

struct swipe_cat_t : public druid_cat_melee_attack_t
{
  swipe_cat_t( druid_t* player, const std::string& options_str ) :
    druid_cat_melee_attack_t( "swipe_cat", player, player -> find_class_spell( "Swipe" ) )
  {
    parse_options( NULL, options_str );

    aoe = -1;
  }
};

// Tigers Fury ==============================================================

struct tigers_fury_t : public druid_cat_melee_attack_t
{
  tigers_fury_t( druid_t* player, const std::string& options_str ) :
    druid_cat_melee_attack_t( "tigers_fury", player, player -> find_specialization_spell( "Tiger's Fury" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    druid_cat_melee_attack_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buff.tigers_fury -> trigger( 1, data().effect1().percent() );

    p -> resource_gain( RESOURCE_ENERGY, data().effect2().resource( RESOURCE_ENERGY ), p -> gain.tigers_fury );

    p -> buff.t13_4pc_melee -> trigger();
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();

    if ( p -> buff.berserk -> check() )
      return false;

    return druid_cat_melee_attack_t::ready();
  }
};

// ==========================================================================
// Druid Bear Attack
// ==========================================================================

// druid_bear_melee_attack_t::cost ================================================

double druid_bear_melee_attack_t::cost() const
{
  druid_t* p = player -> cast_druid();

  double c = melee_attack_t::cost();

  if ( harmful && p -> buff.omen_of_clarity -> check() )
    return 0;

  return c;
}

// druid_bear_melee_attack_t::consume_resource ====================================

void druid_bear_melee_attack_t::consume_resource()
{
  melee_attack_t::consume_resource();
  druid_t* p = player -> cast_druid();

  if ( harmful && p -> buff.omen_of_clarity -> up() )
  {
    // Treat the savings like a rage gain.
    double amount = melee_attack_t::cost();
    if ( amount > 0 )
    {
      p -> gain.omen_of_clarity -> add( RESOURCE_RAGE, amount );
      p -> buff.omen_of_clarity -> expire();
    }
  }
}

// druid_bear_melee_attack_t::execute =============================================

void druid_bear_melee_attack_t::execute()
{
  melee_attack_t::execute();

  if ( result_is_hit() )
  {
    if ( result == RESULT_CRIT )
    {
      trigger_lotp( this );
      trigger_primal_fury( this );
      druid_t* p = player -> cast_druid();
      p -> buff.savage_defense -> trigger( 1, p -> composite_attack_power() * 0.35 );
    }
  }
}

// druid_bear_melee_attack_t::haste ===============================================

double druid_bear_melee_attack_t::haste() const
{
  double h = melee_attack_t::haste();
  druid_t* p = player -> cast_druid();

  h *= p -> buff.stampede_bear -> value();

  return h;
}

// druid_bear_melee_attack_t::player_buff =========================================

void druid_bear_melee_attack_t::player_buff()
{
  melee_attack_t::player_buff();
  druid_t* p = player -> cast_druid();

  player_multiplier *= 1.0 + p -> talent.master_shapeshifter -> effect1().base_value() * 0.01;

  if ( p -> talent.king_of_the_jungle -> ok() && p -> buff.enrage -> up() )
  {
    player_multiplier *= 1.0 + p -> talent.king_of_the_jungle -> effect1().percent();
  }

  player_crit += p -> buff.pulverize -> value();
}

// Bear Melee Attack ========================================================

struct bear_melee_t : public druid_bear_melee_attack_t
{
  bear_melee_t( druid_t* player ) :
    druid_bear_melee_attack_t( "", player, spell_data_t::nil() )
  {
    background  = true;
    repeating   = true;
    may_crit    = true;
    trigger_gcd = timespan_t::zero();
    stateless   = true;
    special     = false;  }

  virtual timespan_t execute_time() const
  {
    if ( ! player -> in_combat )
      return timespan_t::from_seconds( 0.01 );

    return druid_bear_melee_attack_t::execute_time();
  }

  virtual void execute()
  {
    druid_bear_melee_attack_t::execute();

    if ( result != RESULT_MISS )
      trigger_rage_gain( this );

    if ( result_is_hit() )
    {
      trigger_omen_of_clarity( this );
    }
  }
};

// Feral Charge (Bear) ======================================================

struct feral_charge_bear_t : public druid_bear_melee_attack_t
{
/*
  // TODO: Get beta, figure it out
  double stampede_haste;
  timespan_t stampede_duration;

  feral_charge_bear_t( druid_t* p, const std::string& options_str ) :
    druid_bear_melee_attack_t( "feral_charge_bear", 16979, p ),
    stampede_haste( 0.0 ), stampede_duration( timespan_t::zero() )
  {
    parse_options( NULL, options_str );

    if ( p -> talent.stampede -> ok() )
    {
      uint32_t sid = ( p -> talent.stampede -> ok() == 2 ) ? 78893 : 78892;
      spell_id_t stampede_spell( p, "stampede_bear_spell", sid );
      stampede_haste = stampede_spell.base_value( E_APPLY_AURA, A_MOD_HASTE ) / 100.0;
      stampede_duration = stampede_spell.duration();
    }

    stampede_haste = 1.0 / ( 1.0 + stampede_haste );
    p -> buff.stampede_bear -> buff_duration = stampede_duration;

    may_miss   = false;
    may_dodge  = false;
    may_parry  = false;
    may_block  = false;
    may_glance = false;
  }

  virtual void execute()
  {
    druid_bear_melee_attack_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buff.stampede_bear -> trigger( 1, stampede_haste );
  }

  virtual bool ready()
  {
    bool ranged = ( player -> position == POSITION_RANGED_FRONT ||
                    player -> position == POSITION_RANGED_BACK );

    if ( player -> in_combat && ! ranged )
      return false;

    return druid_bear_melee_attack_t::ready();
  }*/
};
// Lacerate =================================================================

struct lacerate_t : public druid_bear_melee_attack_t
{
  cooldown_t* mangle_bear_cooldown;

  lacerate_t( druid_t* player, const std::string& options_str ) :
    druid_bear_melee_attack_t( "lacerate",  player, player -> find_class_spell( "Lacerate" ) ),
    mangle_bear_cooldown( 0 )
  {
    parse_options( NULL, options_str );

    direct_power_mod     = 0.0552;
    tick_power_mod       = 0.00369;
    dot_behavior         = DOT_REFRESH;
    //base_crit           += player -> glyph.lacerate -> mod_additive( P_CRIT );
    mangle_bear_cooldown = player -> cooldown.mangle_bear;
  }

  virtual void execute()
  {
    druid_bear_melee_attack_t::execute();
    druid_t* p = player -> cast_druid();

    if ( result_is_hit() )
    {
      p -> buff.lacerate -> trigger();
      base_td_multiplier  = p -> buff.lacerate -> current_stack;
    }
  }

  virtual void tick( dot_t* d )
  {
    druid_bear_melee_attack_t::tick( d );
    druid_t* p = player -> cast_druid();

    if ( p -> talent.berserk -> ok() )
    {
      if ( p -> rng.berserk -> roll( 0.30 ) )
        mangle_bear_cooldown -> reset();
    }
  }

  virtual void last_tick( dot_t* d )
  {
    druid_bear_melee_attack_t::last_tick( d );
    druid_t* p = player -> cast_druid();

    p -> buff.lacerate -> expire();
  }
};

// Mangle (Bear) ============================================================

struct mangle_bear_t : public druid_bear_melee_attack_t
{
  mangle_bear_t( druid_t* player, const std::string& options_str ) :
    druid_bear_melee_attack_t( "mangle_bear", player, player -> find_class_spell( "Mangle" ) )
  {
    parse_options( NULL, options_str );

    //base_multiplier *= 1.0 + player -> glyph.mangle -> mod_additive( P_GENERIC );
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();

    if ( p -> buff.berserk -> up() )
      aoe = 2;

    druid_bear_melee_attack_t::execute();

    aoe = 0;
    if ( p -> buff.berserk -> up() )
      cooldown -> reset();
  }
};

// Maul =====================================================================

struct maul_t : public druid_bear_melee_attack_t
{
  maul_t( druid_t* player, const std::string& options_str ) :
    druid_bear_melee_attack_t( "maul",  player, player -> find_class_spell( "Maul" ) )
  {
    parse_options( NULL, options_str );

    weapon = &( player -> main_hand_weapon );

    aoe = player -> glyph.maul -> effect1().base_value();
    base_add_multiplier = player -> glyph.maul -> effect3().percent();

    // DBC data points to base scaling, tooltip states AP scaling which is correct
    base_dd_min = base_dd_max = 35;
    direct_power_mod = 0.19;
  }

  virtual void target_debuff( player_t* t, dmg_type_e dtype )
  {
    druid_bear_melee_attack_t::target_debuff( t, dtype );

    if ( t -> debuffs.bleeding -> up() )
      target_multiplier *= 1.0 + 0.20;
  }
};

// Skull Bash (Bear) ========================================================

struct skull_bash_bear_t : public druid_bear_melee_attack_t
{
  skull_bash_bear_t( druid_t* player, const std::string& options_str ) :
    druid_bear_melee_attack_t( "skull_bash_bear", player, player -> find_specialization_spell( "Skull Bash" ) )
  {
    parse_options( NULL, options_str );

    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;

    cooldown -> duration  += player -> talent.brutal_impact -> effect2().time_value();
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return druid_bear_melee_attack_t::ready();
  }
};

// Swipe (Bear) =============================================================

struct swipe_bear_t : public druid_bear_melee_attack_t
{
  swipe_bear_t( druid_t* player, const std::string& options_str ) :
    druid_bear_melee_attack_t( "swipe_bear", player, player -> find_class_spell( "Swipe" ) )
  {
    parse_options( NULL, options_str );

    aoe               = -1;
    direct_power_mod  = data().extra_coeff();
    weapon            = &( player -> main_hand_weapon );
    weapon_multiplier = 0;
  }
};

// Thrash ===================================================================

struct thrash_t : public druid_bear_melee_attack_t
{
  thrash_t( druid_t* player, const std::string& options_str ) :
    druid_bear_melee_attack_t( "thrash", player, player -> find_specialization_spell( "Thread" ) )
  {
    parse_options( NULL, options_str );

    aoe               = -1;
    direct_power_mod  = 0.0982;
    tick_power_mod    = 0.0167;
    weapon            = &( player -> main_hand_weapon );
    weapon_multiplier = 0;
  }

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg )
  {
    druid_bear_melee_attack_t::impact( t, impact_result, travel_dmg );

    if ( result_is_hit( impact_result ) )
    {
      if ( ! sim -> overrides.weakened_blows )
        t -> debuffs.weakened_blows -> trigger();
    }
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

  if ( consume_ooc && p -> buff.omen_of_clarity -> up() )
  {
    // Treat the savings like a mana gain.
    double amount = heal_t::cost();
    if ( amount > 0 )
    {
      p -> gain.omen_of_clarity -> add( RESOURCE_MANA, amount );
      p -> buff.omen_of_clarity -> expire();
    }
  }
}

// druid_heal_t::cost =======================================================

double druid_heal_t::cost() const
{
  druid_t* p = player -> cast_druid();

  if ( consume_ooc && p -> buff.omen_of_clarity -> check() )
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
  //druid_t* p = player -> cast_druid();

  double cr = 0.0;

  return cr;
}

// druid_heal_t::execute ====================================================

void druid_heal_t::execute()
{
  druid_t* p = player -> cast_druid();

  heal_t::execute();

  if ( base_execute_time > timespan_t::zero() && p -> buff.natures_swiftness -> up() )
  {
    p -> buff.natures_swiftness -> expire();
  }

  if ( direct_dmg > 0 && ! background )
  {
    p -> buff.harmony -> trigger( 1, p -> spell.harmony -> effect1().coeff() * 0.01 * p -> composite_mastery() );
  }
}

// druid_heal_t::execute_time ===============================================

timespan_t druid_heal_t::execute_time() const
{
  druid_t* p = player -> cast_druid();

  if ( p -> buff.natures_swiftness -> check() )
    return timespan_t::zero();

  return heal_t::execute_time();
}

// druid_heal_t::haste ======================================================

double druid_heal_t::haste() const
{
  double h =  heal_t::haste();
  druid_t* p = player -> cast_druid();

  h *= 1.0 / ( 1.0 +  p -> buff.natures_grace -> data().effect1().percent() );

  return h;
}

// druid_heal_t::player_buff ================================================

void druid_heal_t::player_buff()
{
  heal_t::player_buff();
  druid_t* p = player -> cast_druid();

  player_multiplier *= 1.0 + additive_factors;
  player_multiplier *= 1.0 + p -> talent.master_shapeshifter -> effect1().percent();
  player_multiplier *= 1.0 + p -> buff.tree_of_life -> value();

  if ( p -> primary_tree() == DRUID_RESTORATION && direct_dmg > 0 )
  {
    player_multiplier *= 1.0 + p -> spell.harmony -> effect1().coeff() * 0.01 * p -> composite_mastery();
  }

  if ( tick_dmg > 0 )
  {
    player_multiplier *= 1.0 + p -> buff.harmony -> value();
  }

  if ( p -> buff.natures_swiftness -> check() && base_execute_time > timespan_t::zero() )
  {
    player_multiplier *= 1.0 + p -> talent.natures_swiftness -> effect2().percent();
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

    //base_dd_multiplier *= 1.0 + p -> talent.empowered_touch -> mod_additive( P_GENERIC );
    //base_execute_time  += p -> talent.naturalist -> mod_additive_time( P_CAST_TIME );
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

    if ( p -> glyph.healing_touch -> ok() )
    {
      ns_cd -> ready -= timespan_t::from_seconds( p -> glyph.healing_touch -> effect1().base_value() );
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
    school             = bloom -> get_school_type();

    //base_crit          += p -> glyph.lifebloom -> mod_additive( P_CRIT );
    base_dd_multiplier *= 1.0 + p -> talent.gift_of_the_earthmother -> effect1().percent();
  }

  virtual void execute()
  {
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();

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

    //base_crit += p -> glyph.lifebloom -> mod_additive( P_CRIT );
    may_crit   = false;

    bloom = new lifebloom_bloom_t( p );

    // TODO: this can be only cast on one target, unless Tree of Life is up
  }

  virtual double calculate_tick_damage( result_type_e r, double power, double multiplier )
  {
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();

    return druid_heal_t::calculate_tick_damage( r, power, multiplier ) * td -> buffs_lifebloom -> check();
  }

  virtual void execute()
  {
    druid_heal_t::execute();
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();

    td -> buffs_lifebloom -> trigger();
  }

  virtual void last_tick( dot_t* d )
  {
    bloom -> execute();

    druid_heal_t::last_tick( d );
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();

    td -> buffs_lifebloom -> expire();
  }

  virtual void tick( dot_t* d )
  {
    druid_heal_t::tick( d );
    druid_t* p = player -> cast_druid();

    if ( p -> talent.malfurions_gift -> ok() )
    {
      p -> buff.omen_of_clarity -> trigger( 1, 1, p -> talent.malfurions_gift -> proc_chance() );
    }

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

    //base_dd_multiplier *= 1.0 + p -> talent.empowered_touch -> mod_additive( P_GENERIC );
    //base_execute_time  += p -> talent.naturalist -> mod_additive_time( P_CAST_TIME );
  }

  virtual void execute()
  {
    druid_heal_t::execute();

    trigger_empowered_touch( this );

    if ( result == RESULT_CRIT )
      trigger_living_seed( this );
  }

  virtual void target_debuff( player_t* t, dmg_type_e dtype )
  {
    druid_heal_t::target_debuff( t, dtype );
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();

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

    //base_dd_multiplier *= 1.0 + p -> talent.empowered_touch -> mod_additive( P_GENERIC );
    //base_crit          += p -> talent.natures_bounty -> mod_additive( P_CRIT );
    consume_ooc         = true;
  }

  virtual void execute()
  {
    druid_heal_t::execute();
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();

    trigger_empowered_touch( this );

    if ( result == RESULT_CRIT )
      trigger_living_seed( this );

    if ( p -> glyph.regrowth -> ok() )
    {
      if ( target -> health_percentage() <= p -> glyph.regrowth ->effect1().percent() && td -> dots_regrowth -> ticking )
      {
        td -> dots_regrowth -> refresh_duration();
      }
    }
  }

  virtual timespan_t execute_time() const
  {
    druid_t* p = player -> cast_druid();

    if ( p -> buff.tree_of_life -> check() )
      return timespan_t::zero();

    return druid_heal_t::execute_time();
  }
};

// Rejuvenation =============================================================

struct rejuvenation_t : public druid_heal_t
{
  rejuvenation_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "rejuvenation", p, 774 )
  {
    parse_options( NULL, options_str );

    may_crit     = p -> talent.gift_of_the_earthmother -> ok() ? true : false;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();

    druid_heal_t::execute();

    if ( p -> talent.gift_of_the_earthmother -> ok() )
    {
      base_dd_min = base_dd_max = tick_dmg * num_ticks * p -> talent.gift_of_the_earthmother -> effect2().percent();
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
    check_spec( DRUID_RESTORATION );

    parse_options( NULL, options_str );

    //additive_factors += p -> talent.improved_rejuvenation -> mod_additive( P_GENERIC );
    consume_ooc       = true;
  }

  virtual void execute()
  {
    druid_heal_t::execute();
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();

    if ( ! p -> glyph.swiftmend -> ok() )
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

  }

  virtual bool ready()
  {
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();

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

    aoe               = data().effect3().base_value(); // Heals 5 targets
    base_execute_time = data().duration();
    channeled         = true;

    // Healing is in spell effect 1
    parse_spell_data( ( *player -> dbc.spell( data().effect1().trigger_spell_id() ) ) );

    // FIXME: The hot should stack

    //cooldown -> duration += p -> talent.malfurions_gift -> mod_additive_time( P_COOLDOWN );
  }
};

// Wild Growth ==============================================================

struct wild_growth_t : public druid_heal_t
{
  wild_growth_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "wild_growth", p, 48438 )
  {
    parse_options( NULL, options_str );

    aoe = data().effect3().base_value();// + ( int ) p -> glyph.wild_growth -> mod_additive( P_EFFECT_3 ); // Heals 5 targets
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();

    if ( p -> buff.tree_of_life -> check() )
      aoe += 2;

    druid_heal_t::execute();

    // Reset AoE
    aoe = data().effect3().base_value();// + ( int ) p -> glyph.wild_growth -> mod_additive( P_EFFECT_3 );
  }
};

// ==========================================================================
// Druid Spell
// ==========================================================================

// druid_spell_t::cost_reduction ============================================

double druid_spell_t::cost_reduction() const
{
  //druid_t* p = player -> cast_druid();

  double   cr = 0.0;

  return cr;
}

// druid_spell_t::cost ======================================================

double druid_spell_t::cost() const
{
  druid_t* p = player -> cast_druid();

  if ( harmful && p -> buff.omen_of_clarity -> check() && spell_t::execute_time() != timespan_t::zero() )
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

  h *= 1.0 / ( 1.0 +  p -> buff.natures_grace -> data().effect1().percent() );

  return h;
}

// druid_spell_t::execute_time ==============================================

timespan_t druid_spell_t::execute_time() const
{
  druid_t* p = player -> cast_druid();

  if ( p -> buff.natures_swiftness -> check() )
    return timespan_t::zero();

  return spell_t::execute_time();
}

// druid_spell_t::schedule_execute ==========================================

void druid_spell_t::schedule_execute()
{
  spell_t::schedule_execute();
  druid_t* p = player -> cast_druid();

  if ( base_execute_time > timespan_t::zero() )
    p -> buff.natures_swiftness -> expire();
}

// druid_spell_t::execute ===================================================

void druid_spell_t::execute()
{
  spell_t::execute();

  if ( ! aoe )
    trigger_omen_of_clarity( this );
}

// druid_spell_t::consume_resource ==========================================

void druid_spell_t::consume_resource()
{
  spell_t::consume_resource();
  druid_t* p = player -> cast_druid();

  if ( harmful && p -> buff.omen_of_clarity -> up() && spell_t::execute_time() != timespan_t::zero() )
  {
    // Treat the savings like a mana gain.
    double amount = spell_t::cost();
    if ( amount > 0 )
    {
      p -> gain.omen_of_clarity -> add( RESOURCE_MANA, amount );
      p -> buff.omen_of_clarity -> expire();
    }
  }
}

// druid_spell_t::player_tick ===============================================

void druid_spell_t::player_tick()
{
  druid_t* p = player -> cast_druid();

  player_crit = p -> composite_spell_crit();
}

// druid_spell_t::player_buff ===============================================

void druid_spell_t::player_buff()
{
  druid_t* p = player -> cast_druid();

  spell_t::player_buff();

  if ( school == SCHOOL_ARCANE || school == SCHOOL_NATURE || school == SCHOOL_SPELLSTORM )
  {
    // Moonkin is actually additive with other player_multipliers, like glyphs, etc.
    // TODO: BETA??
    if ( p -> primary_tree() == DRUID_BALANCE )
    {
      double m = p -> specialization.moonkin_form_bonuses -> effect3().percent();
      additive_multiplier += m;
      
    }
  }

  // Add in Additive Multipliers
  player_multiplier *= 1.0 + additive_multiplier;

  // Reset Additive_Multiplier
  additive_multiplier = 0.0;
}

// Auto Attack ==============================================================
/*
struct auto_melee_attack_t : public action_t
{
  auto_melee_attack_t( druid_t* player, const std::string& options_str ) :
    action_t( "auto_attack", player, spell_data_t::nil() )
  {
    trigger_gcd = timespan_t::zero();
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
*/
// Barkskin =================================================================

struct barkskin_t : public druid_spell_t
{
  barkskin_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "barkskin", player, player -> find_class_spell( "Barkskin" )  )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buff.barkskin -> trigger( 1,data().effect2().percent() );
  }
};

// Bear Form Spell ==========================================================

struct bear_form_t : public druid_spell_t
{
  bear_form_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "bear_form", player, player -> find_class_spell( "Bear Form" )  )
  {
    parse_options( NULL, options_str );

    // Override these as we can do it before combat
    trigger_gcd       = timespan_t::zero();
    base_execute_time = timespan_t::zero();
    harmful           = false;

    if ( ! player -> bear_melee_attack )
      player -> bear_melee_attack = new bear_melee_t( player );
  }

  virtual void execute()
  {
    spell_t::execute();
    druid_t* p = player -> cast_druid();

    if ( p -> primary_tree() == DRUID_FERAL )
      p -> vengeance_enabled = true;

    weapon_t* w = &( p -> main_hand_weapon );

    if ( w -> type != WEAPON_BEAST )
    {
      w -> type = WEAPON_BEAST;
      w -> school = SCHOOL_PHYSICAL;
      w -> damage = 54.8 * 2.5;
      w -> swing_time = timespan_t::from_seconds( 2.5 );
    }

    // Force melee swing to restart if necessary
    if ( p -> main_hand_attack ) p -> main_hand_attack -> cancel();

    p -> main_hand_attack = p -> bear_melee_attack;
    p -> main_hand_attack -> weapon = w;

    if ( p -> buff.cat_form -> check() )
    {
      sim -> auras.critical_strike -> decrement();
      p -> buff.cat_form     -> expire();
    }
    if ( p -> buff.moonkin_form -> check() )
    {
      sim -> auras.spell_haste -> decrement();
      p -> buff.moonkin_form -> expire();
    }

    p -> buff.bear_form -> start();
    p -> base_gcd = timespan_t::from_seconds( 1.0 );
    p -> reset_gcd();

    if ( ! sim -> overrides.critical_strike )
      sim -> auras.critical_strike -> trigger();
  }

  virtual bool ready()
  {
    druid_t* d = player -> cast_druid();
    return ! d -> buff.bear_form -> check();
  }
};

// Berserk ==================================================================

struct berserk_t : public druid_spell_t
{
  berserk_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "berserk", player, player -> find_specialization_spell( "Berserk" )  )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buff.berserk -> trigger();

    if ( p -> buff.cat_form -> check() )
    {
      if ( p -> talent.primal_madness -> ok() )
      {
        p -> buff.primal_madness_cat -> buff_duration = p -> buff.berserk -> buff_duration;
        p -> buff.primal_madness_cat -> trigger();
      }
    }
    else if ( p -> buff.bear_form -> check() )
    {
      if ( p -> talent.primal_madness -> ok() )
      {
        p -> resource_gain( RESOURCE_RAGE, p -> talent.primal_madness -> effect1().resource( RESOURCE_RAGE ), p -> gain.primal_madness );
        p -> buff.primal_madness_bear -> buff_duration = p -> buff.berserk -> buff_duration;
        p -> buff.primal_madness_bear -> trigger();
      }

      p -> cooldown.mangle_bear -> reset();
    }
  }
};

// Cat Form Spell ===========================================================

struct cat_form_t : public druid_spell_t
{
  cat_form_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "cat_form", player, player -> find_class_spell( "Cat Form" )  )
  {
    parse_options( NULL, options_str );

    // Override for precombat casting
    trigger_gcd       = timespan_t::zero();
    base_execute_time = timespan_t::zero();
    harmful           = false;

    if ( ! player -> cat_melee_attack )
      player -> cat_melee_attack = new cat_melee_t( player );
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
      w -> min_dmg /= w -> swing_time.total_seconds();
      w -> max_dmg /= w -> swing_time.total_seconds();
      w -> damage = ( w -> min_dmg + w -> max_dmg ) / 2;
      w -> swing_time = timespan_t::from_seconds( 1.0 );
    }

    // Force melee swing to restart if necessary
    if ( p -> main_hand_attack ) p -> main_hand_attack -> cancel();

    p -> main_hand_attack = p -> cat_melee_attack;
    p -> main_hand_attack -> weapon = w;

    if ( p -> buff.bear_form -> check() )
    {
      sim -> auras.critical_strike -> decrement();
      p -> buff.bear_form -> expire();
    }
    if ( p -> buff.moonkin_form -> check() )
    {
      sim -> auras.spell_haste -> decrement();
      p -> buff.moonkin_form -> expire();
    }

    p -> buff.cat_form -> start();
    p -> base_gcd = timespan_t::from_seconds( 1.0 );
    p -> reset_gcd();

    if ( ! sim -> overrides.critical_strike )
      sim -> auras.critical_strike -> trigger();
  }

  virtual bool ready()
  {
    druid_t* d = player -> cast_druid();

    return ! d -> buff.cat_form -> check();
  }
};

// Enrage ===================================================================

struct enrage_t : public druid_spell_t
{
  enrage_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "enrage", player, player -> find_specialization_spell( "Enrage" )  )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buff.enrage -> trigger();
    p -> resource_gain( RESOURCE_RAGE,data().effect2().resource( RESOURCE_RAGE ), p -> gain.enrage );

    if ( p -> talent.primal_madness -> ok() )
      p -> resource_gain( RESOURCE_RAGE, p -> talent.primal_madness -> effect1().resource( RESOURCE_RAGE ), p -> gain.primal_madness );
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();

    if ( ! p -> buff.bear_form -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Faerie Fire (Feral) ======================================================

struct faerie_fire_feral_t : public druid_spell_t
{
  // TODO: Fix this
  faerie_fire_feral_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "faerie_fire_feral", player, player -> find_class_spell( "Fearie Fire" ) )
  {
    parse_options( NULL, options_str );

    base_attack_power_multiplier = 1.0;
    base_spell_power_multiplier  = 0;
    direct_power_mod             = data().extra_coeff();
    cooldown -> duration         = player -> dbc.spell( 16857 ) -> cooldown(); // Cooldown is stored in another version of FF
    trigger_gcd                  = player -> dbc.spell( 16857 ) -> gcd();
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();

    if ( p -> buff.bear_form -> check() )
    {
      // The damage component is only active in (Dire) Bear Form
      spell_t::execute();
    }
    else
    {
      if ( sim -> log ) log_t::output( sim, "%s performs %s", p -> name(), name() );
      update_ready();
    }

    if ( ! sim -> overrides.weakened_armor )
      target -> debuffs.weakened_armor -> trigger( 3 );
  }
};

// Faerie Fire Spell ========================================================

struct faerie_fire_t : public druid_spell_t
{
  faerie_fire_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "faerie_fire", player, player -> find_class_spell( "Fearie Fire" ) )
  {
    parse_options( NULL, options_str );
    background = ( sim -> overrides.weakened_armor != 0 );
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    if ( result_is_hit() && ! sim -> overrides.weakened_armor )
      target -> debuffs.weakened_armor -> trigger( 3 );
  }
};

// Innervate Buff ===========================================================

struct innervate_buff_t : public buff_t
{
  innervate_buff_t( const buff_creator_t& params ) :
    buff_t ( params )
  {}

  virtual void start( int stacks, double value, timespan_t duration )
  {
    struct innervate_event_t : public event_t
    {
      innervate_event_t ( player_t* p ) :
        event_t( p -> sim, p, "innervate" )
      { sim -> add_event( this, timespan_t::from_seconds( 1.0 ) ); }

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

    buff_t::start( stacks, value, duration );
  }
};

// Innervate Spell ==========================================================

struct innervate_t : public druid_spell_t
{
  int trigger;
  player_t* innervate_target;

  innervate_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "innervate", player, player -> find_class_spell( "Innervate" )  ),
    trigger( 0 )
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
    innervate_target = target_str.empty() ? player : sim -> find_player( target_str );
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
    }
    else
    {
      gain = 0.10;
      p -> buff.glyph_of_innervate -> trigger( 1, p -> resources.max[ RESOURCE_MANA ] * 0.1 / 10.0 );
    }
    innervate_target -> buffs.innervate -> trigger( 1, p -> resources.max[ RESOURCE_MANA ] * gain / 10.0 );
  }

  virtual bool ready()
  {
    if ( ! druid_spell_t::ready() )
      return false;

    if ( trigger < 0 )
      return ( innervate_target -> resources.current[ RESOURCE_MANA ] + trigger ) < 0;

    return ( innervate_target -> resources.max    [ RESOURCE_MANA ] -
             innervate_target -> resources.current[ RESOURCE_MANA ] ) > trigger;
  }
};

// Insect Swarm Spell =======================================================

struct insect_swarm_t : public druid_spell_t
{
  cooldown_t* starsurge_cd;

  insect_swarm_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "insect_swarm", player, player -> find_class_spell( "Insect Swarm" )  ),
    starsurge_cd( 0 )
  {
    parse_options( NULL, options_str );

    may_crit = false;

    dot_behavior = DOT_REFRESH;

    starsurge_cd = player -> cooldown.starsurge;
  }

  virtual void tick( dot_t* d )
  {
    druid_spell_t::tick( d );
    druid_t* p = player -> cast_druid();

    if ( p -> buff.shooting_stars -> trigger() )
      starsurge_cd -> reset();
  }

};

// Mark of the Wild Spell ===================================================

struct mark_of_the_wild_t : public druid_spell_t
{
  mark_of_the_wild_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "mark_of_the_wild", player, player -> find_class_spell( "Mark of the Wild" )  )
  {
    parse_options( NULL, options_str );

    trigger_gcd = timespan_t::zero();
    id          = 1126;
    //base_costs[ current_resource() ]  *= 1.0 + p -> glyph.mark_of_the_wild -> mod_additive( P_RESOURCE_COST ) / 100.0;
    harmful     = false;
    background  = ( sim -> overrides.str_agi_int != 0 );
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    if ( sim -> log ) log_t::output( sim, "%s performs %s", player -> name(), name() );

    if ( ! sim -> overrides.str_agi_int )
      sim -> auras.str_agi_int -> trigger( 1, -1.0, -1.0, player -> dbc.spell( 79060 ) -> duration() );
  }
};

// Moonfire Spell ===========================================================

struct moonfire_t : public druid_spell_t
{
  cooldown_t* starsurge_cd;

  moonfire_t( druid_t* player, const std::string& options_str, bool dtr=false ) :
    druid_spell_t( "moonfire", player, player -> find_class_spell( "Moonfire" )  ),
    starsurge_cd( 0 )
  {
    parse_options( NULL, options_str );

    dot_behavior = DOT_REFRESH;


    starsurge_cd = player -> cooldown.starsurge;

    may_trigger_dtr = false; // Disable the dot ticks procing DTR

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new moonfire_t( player, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void player_buff()
  {
    druid_t* p = player -> cast_druid();

    // TODO: LS + 10% MK Form additiv in beta?
    additive_multiplier += ( p -> buff.lunar_shower -> data().effect1().percent() * p -> buff.lunar_shower -> stack() );

    druid_spell_t::player_buff();
  }

  void player_buff_tick()
  {
    //druid_t* p = player -> cast_druid();
    druid_spell_t::player_buff();
  }

  virtual void tick( dot_t* d )
  {
    druid_spell_t::tick( d );
    druid_t* p = player -> cast_druid();

    if ( p -> buff.shooting_stars -> trigger() )
      starsurge_cd -> reset();
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();
    dot_t* sf = td ->dots_sunfire;

    // Recalculate all those multipliers w/o Lunar Shower/BotG
    player_buff_tick();

    if ( result_is_hit() )
    {
      if ( sf -> ticking )
        sf -> cancel();
        

      if ( p -> specialization.lunar_shower -> ok() )
      {
        // TODO: BETA?
        //if ( p -> buff.lunar_shower -> check() )
        //  trigger_eclipse_gain_delay( this, ( p -> eclipse_bar_direction > 0 ? 8 : -8 ) );

        p -> buff.lunar_shower -> trigger();
      }
    }
  }

  virtual double cost_reduction() const
  {
    double cr = druid_spell_t::cost_reduction();
    druid_t* p = player -> cast_druid();

    cr += ( p -> buff.lunar_shower -> data().effect2().percent() * p -> buff.lunar_shower -> stack() );

    return cr;
  }

  virtual bool ready()
  {
    if ( ! druid_spell_t::ready() )
      return false;

    druid_t* p = player -> cast_druid();

    if ( p -> talent.sunfire -> ok() && p -> buff.eclipse_solar -> check() )
      return false;

    return true;
  }
};

// Moonkin Form Spell =======================================================

struct moonkin_form_t : public druid_spell_t
{
  moonkin_form_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "moonkin_form", player, player -> find_specialization_spell( "Moonkin Form" )  )
  {
    parse_options( NULL, options_str );

    // Override these as we can precast before combat begins
    trigger_gcd       = timespan_t::zero();
    base_execute_time = timespan_t::zero();
    harmful           = false;
  }

  virtual void execute()
  {
    spell_t::execute();
    druid_t* p = player -> cast_druid();

    if ( p -> buff.bear_form -> check() )
    {
      sim -> auras.critical_strike -> decrement();
      p -> buff.bear_form -> expire();
    }
    if ( p -> buff.cat_form  -> check() )
    {
      sim -> auras.critical_strike -> decrement();
      p -> buff.cat_form  -> expire();
    }

    p -> buff.moonkin_form -> start();

    if ( ! sim -> overrides.spell_haste )
      sim -> auras.spell_haste -> trigger();
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();

    return ! p -> buff.moonkin_form -> check();
  }
};

// Natures Swiftness Spell ==================================================

struct druids_swiftness_t : public druid_spell_t
{
  cooldown_t* sub_cooldown;
  dot_t*      sub_dot;

  druids_swiftness_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "natures_swiftness", player, player -> find_talent_spell( "Nature's Swiftness" ) ),
    sub_cooldown( 0 ), sub_dot( 0 )
  {
    parse_options( NULL, options_str );

    if ( ! options_str.empty() )
    {
      // This will prevent Natures Swiftness from being called before the desired "fast spell" is ready to be cast.
      sub_cooldown = player -> get_cooldown( options_str );
      sub_dot      = player -> get_dot     ( options_str );
    }

    harmful = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buff.natures_swiftness -> trigger();
  }

  virtual bool ready()
  {
    if ( sub_cooldown )
      if ( sub_cooldown -> remains() > timespan_t::zero() )
        return false;

    if ( sub_dot )
      if ( sub_dot -> remains() > timespan_t::zero() )
        return false;

    return druid_spell_t::ready();
  }
};

// Starfire Spell ===========================================================

struct starfire_t : public druid_spell_t
{
  int extend_moonfire;

  starfire_t( druid_t* player, const std::string& options_str, bool dtr=false ) :
    druid_spell_t( "starfire", player, player -> find_class_spell( "Starfire" ) ),
    extend_moonfire( 0 )
  {
    option_t options[] =
    {
      { "extendmf", OPT_BOOL, &extend_moonfire },
      { NULL, OPT_UNKNOWN, NULL }
    };
    parse_options( options, options_str );

    //base_execute_time += p -> talent.starlight_wrath -> mod_additive_time( P_CAST_TIME );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new starfire_t( player, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();
    dot_t* mf = td->dots_moonfire;
    dot_t* sf = td->dots_sunfire;

    if ( result_is_hit() )
    {
      if ( p -> glyph.starfire -> ok() )
      {
        if ( mf -> ticking )
        {
          if ( mf -> added_seconds < timespan_t::from_seconds( 9.0 ) )
            mf -> extend_duration_seconds( timespan_t::from_seconds( 3.0 ) );
          else
            mf -> extend_duration_seconds( timespan_t::zero() );
        }
        else if ( sf -> ticking )
        {
          if ( sf -> added_seconds < timespan_t::from_seconds( 9.0 ) )
            sf -> extend_duration_seconds( timespan_t::from_seconds( 3.0 ) );
          else
            sf -> extend_duration_seconds( timespan_t::zero() );
        }
      }

      if ( ! p -> buff.eclipse_solar -> check() )
      {
        // BUG (FEATURE?) ON LIVE, TODO: Beta too?
        // #1 Euphoria does not proc, if you are more than 35 into the side the
        // Eclipse bar is moving towards, >35 for Starfire/towards Solar
        int gain = data().effect2().base_value();

        if ( ! p -> buff.eclipse_lunar -> check() )
        {
          if ( p -> rng.euphoria -> roll( p -> specialization.euphoria -> effect1().percent() ) )
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
        p -> proc.wrong_eclipse_starfire -> occur();
      }
    }
  }

  virtual void target_debuff( player_t* t, dmg_type_e dtype )
  {
    druid_spell_t::target_debuff( t, dtype );
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();

    // Balance, 2P -- Insect Swarm increases all damage done by your Starfire,
    // Starsurge, and Wrath spells against that target by 3%.
    if ( td -> dots_insect_swarm -> ticking )
      target_multiplier *= 1.0 + p -> set_bonus.tier13_2pc_caster() * 0.03;
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();

    if ( extend_moonfire )
    {
      if ( ! p -> glyph.starfire -> ok() )
        return false;

      if ( td -> dots_moonfire -> ticking )
      {
        if ( td -> dots_moonfire -> added_seconds > timespan_t::from_seconds( 8 ) )
          return false;
      }
      else if ( td -> dots_sunfire -> ticking )
      {
        if ( td -> dots_sunfire -> added_seconds > timespan_t::from_seconds( 8 ) )
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
  starfall_star_t( druid_t* player, uint32_t spell_id, bool dtr=false ) :
    druid_spell_t( "starfall_star", player, player -> dbc.spell( spell_id ) )
  {
    background  = true;
    dual        = true;
    direct_tick = true;


    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new starfall_star_t( player, true );
      dtr_action -> is_dtr_action = true;
    }
  }

};

struct starfall_t : public druid_spell_t
{
  spell_t* starfall_star;

  starfall_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "starfall", player, player -> find_specialization_spell( "Starfall" ) ), 
    starfall_star( 0 )
  {
    parse_options( NULL, options_str );

    num_ticks      = 10;
    base_tick_time = timespan_t::from_seconds( 1.0 );
    hasted_ticks   = false;

    harmful = false;

    starfall_star = new starfall_star_t( player, data().effect1().trigger_spell_id() );
  }

  virtual void init()
  {
    druid_spell_t::init();

    starfall_star -> stats = stats;
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
  starsurge_t( druid_t* player, const std::string& options_str, bool dtr=false ) :
    druid_spell_t( "starsurge", player, player -> find_specialization_spell( "Starsurge" ) )
  {
    parse_options( NULL, options_str );

    if ( player -> set_bonus.tier13_4pc_caster() )
    {
      cooldown -> duration -= timespan_t::from_seconds( 5.0 );
      base_multiplier *= 1.10;
    }

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new starsurge_t( player, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg=0 )
  {
    druid_spell_t::impact( t, impact_result, travel_dmg );
    druid_t* p = player -> cast_druid();

    if ( result_is_hit( impact_result ) )
    {
      // gain is positive for p -> eclipse_bar_direction==0
      // else it is towards p -> eclipse_bar_direction
      int gain = data().effect2().base_value();
      if ( p -> eclipse_bar_direction < 0 ) gain = -gain;

      if ( ! p -> buff.eclipse_lunar -> check() && ! p -> buff.eclipse_solar -> check() )
      {
        if ( p -> rng.euphoria -> roll( p -> specialization.euphoria -> effect1().percent() ) )
        {
          if ( ! ( p -> bugs && p -> eclipse_bar_value > 35 ) )
          {
            gain *= 2;
          }
        }
      }
      trigger_eclipse_gain_delay( this, gain );
    }
  }

  virtual void schedule_execute()
  {
    druid_spell_t::schedule_execute();
    druid_t* p = player -> cast_druid();

    p -> buff.shooting_stars -> expire();
  }

  virtual timespan_t execute_time() const
  {
    druid_t* p = player -> cast_druid();

    if ( p -> buff.shooting_stars -> up() )
      return timespan_t::zero();

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

  virtual void target_debuff( player_t* t, dmg_type_e dtype )
  {
    druid_spell_t::target_debuff( t, dtype );
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();

    // Balance, 2P -- Insect Swarm increases all damage done by your Starfire,
    // Starsurge, and Wrath spells against that target by 3%.
    if ( td -> dots_insect_swarm -> ticking )
      target_multiplier *= 1.0 + p -> set_bonus.tier13_2pc_caster() * 0.03;
  }
};

// Stealth ==================================================================

struct stealth_t : public druid_spell_t
{
  stealth_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "stealth", player, player -> find_class_spell( "Prowl" )  )
  {
    parse_options( NULL, options_str );

    trigger_gcd = timespan_t::zero();
    harmful     = false;
  }

  virtual void execute()
  {
    druid_t* p = player -> cast_druid();

    if ( sim -> log )
      log_t::output( sim, "%s performs %s", p -> name(), name() );

    p -> buff.stealthed -> trigger();
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();

    return ! p -> buff.stealthed -> check();
  }
};

// Sunfire Spell ============================================================

struct sunfire_t : public druid_spell_t
{
  cooldown_t* starsurge_cd;

  // Identical to moonfire, except damage type and usability

  sunfire_t( druid_t* player, const std::string& options_str, bool dtr=false ) :
    druid_spell_t( "sunfire", player, player -> find_class_spell( "Sunfire" ) ),
    starsurge_cd( 0 )
  {
    parse_options( NULL, options_str );

    dot_behavior = DOT_REFRESH;

    starsurge_cd = player -> cooldown.starsurge;

    may_trigger_dtr = false; // Disable the dot ticks procing DTR

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new sunfire_t( player, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual void player_buff()
  {
    druid_t* p = player -> cast_druid();

    // TODO: LS + 10% MK Form additiv in beta?
    additive_multiplier += ( p -> buff.lunar_shower -> data().effect1().percent() * p -> buff.lunar_shower -> stack() );

    druid_spell_t::player_buff();
  }

  void player_buff_tick()
  {
    // druid_t* p = player -> cast_druid();
    druid_spell_t::player_buff();
  }

  virtual void tick( dot_t* d )
  {
    druid_spell_t::tick( d );
    druid_t* p = player -> cast_druid();

    if ( p -> buff.shooting_stars -> trigger() )
      starsurge_cd -> reset();
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();
    dot_t* mf = td->dots_moonfire;

    // Recalculate all those multipliers w/o Lunar Shower/BotG
    player_buff_tick();

    if ( result_is_hit() )
    {
      if ( mf -> ticking )
        mf -> cancel();

      if ( p -> specialization.lunar_shower -> ok() )
      {
        // TODO: BETA?
        //if ( p -> buff.lunar_shower -> check() )
        //  trigger_eclipse_gain_delay( this, -8 );

        // If moving trigger all 3 stacks, because it will stack up immediately
        p -> buff.lunar_shower -> trigger( 1 );
      }
    }
  }

  virtual double cost_reduction() const
  {
    double cr = druid_spell_t::cost_reduction();
    druid_t* p = player -> cast_druid();

    cr += ( p -> buff.lunar_shower -> data().effect2().percent() * p -> buff.lunar_shower -> stack() );

    return cr;
  }

  virtual bool ready()
  {
    if ( ! druid_spell_t::ready() )
      return false;

    druid_t* p = player -> cast_druid();

    if ( ! p -> buff.eclipse_solar -> check() )
      return false;

    return true;
  }
};

// Survival Instincts =======================================================

struct survival_instincts_t : public druid_spell_t
{
  survival_instincts_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "survival_instincts", player, player -> find_specialization_spell( "Survival Instincts" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buff.survival_instincts -> trigger( 1, -0.5 ); // DBC value is 60 for some reason
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();

    if ( ! ( p -> buff.cat_form -> check() || p -> buff.bear_form -> check() ) )
      return false;

    return druid_spell_t::ready();
  }
};

// Treants Spell ============================================================

struct treants_spell_t : public druid_spell_t
{
  /* TODO: Figure this out
  /  Multiple spells
  /  http://mop.wowhead.com/spell=106737 The talent itself?
  /  http://mop.wowhead.com/spell=33831  Summons 3 treants to attack and root enemy targets for 1 min. (Boomkin)
  /  http://mop.wowhead.com/spell=102693 Summons 3 treants that heal nearby friendly targets for 1 min. (By my shaggy bark, tree treants)
  /  http://mop.wowhead.com/spell=102703 Summons 3 treants to attack and stun enemy targets for 1 min. (The Itteh Bitteh Kitteh Woodmen Committeh)
  /  http://mop.wowhead.com/spell=102706 Summons 3 treants to protect the summoner and nearby allies for 1 min. (Bear is schtronk)
  */
  treants_spell_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "treants", player, player -> find_specialization_spell( "Force of Nature" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();

    p -> pet_treants -> summon( timespan_t::from_seconds( 30.0 ) );
  }
};

// Tree of Life =============================================================

struct tree_of_life_t : public druid_spell_t
{
  // TODO: Only available with the right specialization + talent combo
  tree_of_life_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "tree_of_life", player, player -> find_specialization_spell( "Tree of Life" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buff.tree_of_life -> trigger( 1, p -> dbc.spell( 5420 ) -> effect1().percent() );
  }
};

// Typhoon ==================================================================

struct typhoon_t : public druid_spell_t
{
  typhoon_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "typhoon", player, player -> find_specialization_spell( "Typhoon" ) )
  {
    parse_options( NULL, options_str );

    // Damage information is stored in effect 2's trigger spell
    const spell_data_t* damage_spell = player -> dbc.spell( data().effect2().trigger_spell_id() );

    aoe                   = -1;
    base_dd_min           = player -> dbc.effect_min( damage_spell -> effect2().id(), player -> level );
    base_dd_max           = player -> dbc.effect_max( damage_spell -> effect2().id(), player -> level );
    direct_power_mod      = damage_spell -> effect2().coeff();
    //cooldown -> duration += player -> glyph.monsoon -> mod_additive_time( P_COOLDOWN );
    //base_costs[ current_resource() ] *= 1.0 + player -> glyph.typhoon -> mod_additive( P_RESOURCE_COST );
  }
};

// Wild Mushroom ============================================================

struct wild_mushroom_t : public druid_spell_t
{
  wild_mushroom_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "wild_mushroom", player, player -> find_specialization_spell( "Wild Mushroom" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buff.wild_mushroom -> trigger();
  }
};

// Wild Mushroom: Detonate ==================================================

struct wild_mushroom_detonate_t : public druid_spell_t
{
  wild_mushroom_detonate_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "wild_mushroom_detonate", player, player -> find_specialization_spell( "Wild Mushroom: Detonate" ) )
  {
    parse_options( NULL, options_str );

    // Actual ability is 88751, all damage is in spell 78777
    const spell_data_t* damage_spell = player -> dbc.spell( 78777 );
    direct_power_mod   = damage_spell -> effect1().coeff();
    base_dd_min        = player -> dbc.effect_min( damage_spell -> effect1().id(), player -> level );
    base_dd_max        = player -> dbc.effect_max( damage_spell -> effect1().id(), player -> level );
    school             = damage_spell -> get_school_type();
    stats -> school    = school;
    aoe                = -1;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();

    p -> buff.wild_mushroom -> expire();
  }

  virtual void player_buff()
  {
    druid_spell_t::player_buff();
    druid_t* p = player -> cast_druid();

    player_multiplier *= p -> buff.wild_mushroom -> stack();
  }

  virtual bool ready()
  {
    druid_t* p = player -> cast_druid();

    if ( ! p -> buff.wild_mushroom -> stack() )
      return false;

    return druid_spell_t::ready();
  }
};

// Wrath Spell ==============================================================

struct wrath_t : public druid_spell_t
{
  wrath_t( druid_t* player, const std::string& options_str, bool dtr=false ) :
    druid_spell_t( "wrath", player, player -> find_class_spell( "Wrath" ) )
  {
    parse_options( NULL, options_str );

    if ( ! dtr && player -> has_dtr )
    {
      dtr_action = new wrath_t( player, options_str, true );
      dtr_action -> is_dtr_action = true;
    }
  }

  virtual timespan_t execute_time() const
  {
    timespan_t t = druid_spell_t::execute_time();
    druid_t* p = player ->cast_druid();

    if ( p -> buff.tree_of_life -> check() )
      t *= 0.50;

    return t;
  }

  virtual void target_debuff( player_t* t, dmg_type_e dtype )
  {
    druid_spell_t::target_debuff( t, dtype );
    druid_t* p = player -> cast_druid();
    druid_targetdata_t* td = targetdata( target ) -> cast_druid();

    // Balance, 2P -- Insect Swarm increases all damage done by your Starfire,
    // Starsurge, and Wrath spells against that target by 3%.
    if ( td -> dots_insect_swarm -> ticking )
      target_multiplier *= 1.0 + p -> set_bonus.tier13_2pc_caster() * 0.03;
  }

  virtual void impact( player_t* t, result_type_e impact_result, double travel_dmg=0 )
  {
    druid_spell_t::impact( t, impact_result, travel_dmg );
    druid_t* p = player -> cast_druid();

    if ( result_is_hit( impact_result ) )
    {
      if ( p -> eclipse_bar_direction <= 0 )
      {
        int gain = data().effect2().base_value();

        // BUG (FEATURE?) ON LIVE
        // #1 Euphoria does not proc, if you are more than 35 into the side the
        // Eclipse bar is moving towards, <-35 for Wrath/towards Lunar
        if ( ! p -> buff.eclipse_solar -> check() )
        {
          if ( p -> rng.euphoria -> roll( p -> specialization.euphoria -> effect1().percent() ) )
          {
            if ( !( p -> bugs && p -> eclipse_bar_value < -35 ) )
            {
              gain *= 2;
            }
          }
        }

        trigger_eclipse_gain_delay( this, -gain );
      }
    }
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    druid_t* p = player -> cast_druid();

    // Cast wrath, but lunar eclipse was up?
    if ( p -> buff.eclipse_lunar -> check() )
      p -> proc.wrong_eclipse_wrath -> occur();
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
  //if ( name == "auto_attack"            ) return new      auto_melee_attack_t( this, options_str );
  if ( name == "barkskin"               ) return new               barkskin_t( this, options_str );
  if ( name == "berserk"                ) return new                berserk_t( this, options_str );
  if ( name == "bear_form"              ) return new              bear_form_t( this, options_str );
  if ( name == "cat_form"               ) return new               cat_form_t( this, options_str );
  if ( name == "claw"                   ) return new                   claw_t( this, options_str );
  if ( name == "enrage"                 ) return new                 enrage_t( this, options_str );
  if ( name == "faerie_fire"            ) return new            faerie_fire_t( this, options_str );
  if ( name == "faerie_fire_feral"      ) return new      faerie_fire_feral_t( this, options_str );
  //if ( name == "feral_charge_bear"      ) return new      feral_charge_bear_t( this, options_str );
  //if ( name == "feral_charge_cat"       ) return new       feral_charge_cat_t( this, options_str );
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

  return 0;
}

// druid_t::create_pets =====================================================

void druid_t::create_pets()
{
  pet_treants        = create_pet( "treants" );
}

// druid_t::init_spells =====================================================

void druid_t::init_spells()
{
  player_t::init_spells();

  // Specializations
  specialization.leahter_specialization = find_specialization_spell( "Leather Specialization" );
  
  // Balance
  // Eclipse are 2 spells, the mana energize is not in the main spell!
  // http://mop.wowhead.com/spell=79577 => Specialization spell
  // http://mop.wowhead.com/spell=81070 => The mana gain energize spell
  specialization.eclipse_mana_gain      = find_spell( 81070 );
  specialization.eclipse                = find_specialization_spell( "Eclipse" );
  specialization.celestial_focus        = find_specialization_spell( "Celestial Focus" );
  specialization.shooting_stars         = find_specialization_spell( "Shooting Stars" );
  specialization.owlkin_frenzy          = find_specialization_spell( "Owlkin Frenzy" );
  specialization.balance_of_power       = find_specialization_spell( "Balance of Power" );
  specialization.lunar_shower           = find_specialization_spell( "Lunar Shower" );
  
  specialization.moonkin_form           = find_specialization_spell( "Moonkin Form" ); // Shapeshift spell
  specialization.moonkin_form_bonuses   = find_spell( 24905 ); // This is the passive applied on shapeshift!
  
  /*
  specialization.aggression      = spell_data_t::find( 84735, "Aggression",      dbc.ptr );
  specialization.gift_of_nature  = spell_data_t::find( 87305, "Gift of Nature",  dbc.ptr );
  specialization.meditation      = spell_data_t::find( 85101, "Meditation",      dbc.ptr );
  specialization.razor_claws     = spell_data_t::find( 77493, "Razor Claws",     dbc.ptr );
  specialization.savage_defender = spell_data_t::find( 77494, "Savage Defender", dbc.ptr );
  specialization.harmony         = spell_data_t::find( 77495, "Harmony",         dbc.ptr );
  specialization.total_eclipse   = spell_data_t::find( 77492, "Total Eclipse",   dbc.ptr );
  specialization.vengeance       = spell_data_t::find( 84840, "Vengeance",       dbc.ptr );
  */
    
  // Talents
  talent.feline_swiftness   = find_talent_spell( "Feline Swiftness" );
  talent.displacer_beast    = find_talent_spell( "Displacer Beast" );
  talent.wild_charge        = find_talent_spell( "Wild Charge" );
  
  talent.natures_swiftness  = find_talent_spell( "Nature's Swiftness" );
  talent.renewal            = find_talent_spell( "Renewal" );
  talent.cenarion_ward      = find_talent_spell( "Cenarion Ward" );
  
  talent.faerie_swarm       = find_talent_spell( "Fearie Swarm" );
  talent.mass_entanglement  = find_talent_spell( "Mass Entablement" );
  talent.typhoon            = find_talent_spell( "Typhoon" );
  
  talent.soul_of_the_forest = find_talent_spell( "Soul of the Forest" );
  talent.incarnation        = find_talent_spell( "Incarnation" );
  talent.force_of_nature    = find_talent_spell( "Force of Nature" );
  
  talent.disorienting_roar  = find_talent_spell( "Disorienting Roar" );
  talent.ursols_vortex      = find_talent_spell( "Ursol's Vortex" );
  talent.mighty_bash        = find_talent_spell( "Mighty Bash" );
  
  talent.heart_of_the_wild  = find_talent_spell( "Heart of the Wild" );
  talent.dream_of_cenarius  = find_talent_spell( "Dream of Cenarius" );
  talent.disentanglement    = find_talent_spell( "Disentanglement" );  

  // Glyphs
  glyph.berserk               = find_glyph( "Glyph of Berserk" );
  glyph.bloodletting          = find_glyph( "Glyph of Bloodletting" );
  glyph.ferocious_bite        = find_glyph( "Glyph of Ferocious Bite" );
  glyph.focus                 = find_glyph( "Glyph of Focus" );
  glyph.frenzied_regeneration = find_glyph( "Glyph of Frenzied Regeneration" );
  glyph.healing_touch         = find_glyph( "Glyph of Healing Touch" );
  glyph.innervate             = find_glyph( "Glyph of Innervate" );
  glyph.insect_swarm          = find_glyph( "Glyph of Insect Swarm" );
  glyph.lacerate              = find_glyph( "Glyph of Lacerate" );
  glyph.lifebloom             = find_glyph( "Glyph of Lifebloom" );
  glyph.mangle                = find_glyph( "Glyph of Mangle" );
  glyph.mark_of_the_wild      = find_glyph( "Glyph of Mark of the Wild" );
  glyph.maul                  = find_glyph( "Glyph of Maul" );
  glyph.monsoon               = find_glyph( "Glyph of Monsoon" );
  glyph.moonfire              = find_glyph( "Glyph of Moonfire" );
  glyph.regrowth              = find_glyph( "Glyph of Regrowth" );
  glyph.rejuvenation          = find_glyph( "Glyph of Rejuvenation" );
  glyph.rip                   = find_glyph( "Glyph of Rip" );
  glyph.savage_roar           = find_glyph( "Glyph of Savage Roar" );
  glyph.starfall              = find_glyph( "Glyph of Starfall" );
  glyph.starfire              = find_glyph( "Glyph of Starfire" );
  glyph.starsurge             = find_glyph( "Glyph of Starsurge" );
  glyph.swiftmend             = find_glyph( "Glyph of Swiftmend" );
  glyph.typhoon               = find_glyph( "Glyph of Typhoon" );
  glyph.wild_growth           = find_glyph( "Glyph of Wild Growth" );
  glyph.wrath                 = find_glyph( "Glyph of Wrath" );

  // Tier Bonuses
  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //   C2P     C4P     M2P     M4P    T2P    T4P     H2P     H4P
    { 105722, 105717, 105725, 105735,     0,     0, 105715, 105770 }, // Tier13
    {      0,     0,      0,       0,     0,     0,      0,      0 },
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// druid_t::init_base =======================================================

void druid_t::init_base()
{
  player_t::init_base();

  stats_base.attack_power = level * ( level > 80 ? 3.0 : 2.0 );

  stats_initial.attack_power_per_strength = 1.0;
  stats_initial.spell_power_per_intellect = 1.0;

  // FIXME! Level-specific!  Should be form-specific!
  initial_armor_multiplier  = 1.0 + talent.thick_hide -> effect1().percent();

  diminished_kfactor    = 0.009720;
  diminished_dodge_capi = 0.008555;
  diminished_parry_capi = 0.008555;

  resources.base[ RESOURCE_ENERGY ] = 100;
  resources.base[ RESOURCE_RAGE   ] = 100;

  mana_per_intellect           = 15;
  base_energy_regen_per_second = 10;

  // Natural Insight: +400% mana
  // resources.base[ RESOURCE_MANA ] *= 1.0 + specialization.natural_insight -> effect1().percent();
  // mana_per_intellect              *= 1.0 + specialization.natural_insight -> effect1().percent();

  base_gcd = timespan_t::from_seconds( 1.5 );
}

// druid_t::init_buffs ======================================================

// helpers

buff_t* druid_t::create_buff( const spell_data_t* sd, const std::string& token )
{
  return buff_creator_t( this, token, sd );
}

buff_t* druid_t::create_buff( int id, const std::string& token )
{
  return buff_creator_t( this, token, find_spell( id, token ) );
}

void druid_t::init_buffs()
{
  player_t::init_buffs();

  // buff_t( sim, player, name, max_stack, duration, cooldown, proc_chance, quiet )
  // These have either incorrect or no values in the DBC or don't really exist
  
  /*
  buff.glyph_of_innervate = new buff_t( this, "glyph_of_innervate", 1, timespan_t::from_seconds( 10.0 ), timespan_t::zero(), glyph.innervate -> ok() );
  buff.pulverize          = new buff_t( this, "pulverize"         , 1, timespan_t::from_seconds( 10.0 ) + talent.endless_carnage -> effect2().time_value() );
  buff.revitalize         = new buff_t( this, "revitalize"        , 1, timespan_t::from_seconds( 1.0 ), timespan_t::from_seconds( talent.revitalize -> spell( 1 ).effect2().base_value() ), talent.revitalize -> ok() ? 0.20 : 0, true );
  buff.stampede_bear      = new buff_t( this, "stampede_bear"     , 1, timespan_t::from_seconds( 8.0 ), timespan_t::zero(), talent.stampede -> ok() );
  buff.stampede_cat       = new buff_t( this, "stampede_cat"      , 1, timespan_t::from_seconds( 10.0 ), timespan_t::zero(), talent.stampede -> ok() );
  buff.t13_4pc_melee      = new buff_t( this, "t13_4pc_melee"     , 1, timespan_t::from_seconds( 10.0 ), timespan_t::zero(), ( set_bonus.tier13_4pc_melee() ) ? 1.0 : 0 );
  buff.wild_mushroom      = new buff_t( this, "wild_mushroom"     , 3, timespan_t::from_seconds(   0 ), timespan_t::zero(), 1.0, true );
  */

  buff.barkskin              = create_buff( 22812, "barkskin" );
  buff.eclipse_lunar         = create_buff( 48518, "lunar_eclipse" );
  buff.eclipse_solar         = create_buff( 48517, "solar_eclipse" );
  //buff.enrage                = create_buff( dbc.class_ability_id( type, "Enrage" ), "enrage" );
  buff.frenzied_regeneration = new frenzied_regeneration_buff_t( this );
  buff.frenzied_regeneration -> cooldown -> duration = timespan_t::zero(); //CD is handled by the ability
  buff.harmony               = create_buff( 100977, "harmony" );
  //buff.lacerate              = create_buff( dbc.class_ability_id( type, "Lacerate" ), "lacerate" );
  buff.lunar_shower          = create_buff( specialization.lunar_shower -> effect1().trigger_spell_id(), "lunar_shower" );
  buff.natures_grace         = create_buff( 16886, "natures_grace" );
  buff.natures_swiftness     = create_buff( talent.natures_swiftness, "natures_swiftness" );
  buff.natures_swiftness -> cooldown -> duration = timespan_t::zero();// CD is handled by the ability
  buff.omen_of_clarity       = create_buff( specialization.omen_of_clarity, "omen_of_clarity" );


  /*
  buff.savage_defense        = new buff_t( this, 62606, "savage_defense", 0.5 ); // Correct chance is stored in the ability, 62600
  buff.shooting_stars        = new buff_t( this, talent.shooting_stars -> effect_trigger_spell( 1 ), "shooting_stars", talent.shooting_stars -> proc_chance() );
  buff.survival_instincts    = new buff_t( this, talent.survival_instincts -> spell_id(), "survival_instincts" );
  buff.tree_of_life          = new buff_t( this, talent.tree_of_life, NULL );
  buff.tree_of_life -> buff_duration += talent.natural_shapeshifter -> mod_additive_time( P_DURATION );
  */
  /*
  buff.primal_madness_cat  = new stat_buff_t( this, "primal_madness_cat", STAT_MAX_ENERGY, spells.primal_madness_cat -> effect1().base_value() );
  buff.primal_madness_bear = new      buff_t( this, "primal_madness_bear" );
  buff.berserk             = new      buff_t( this, "berserk", 1, timespan_t::from_seconds( 15.0 ) );
  */
  buff.tigers_fury         = create_buff( 5217, "tigers_fury" );

  // simple
  buff.bear_form    = create_buff( 5487,  "bear_form" );
  buff.cat_form     = create_buff( 768,   "cat_form" );
  buff.moonkin_form = create_buff( 24858, "moonkin_form" );
  buff.savage_roar  = create_buff( 52610, "savage_roar" );
  buff.stealthed    = create_buff( 5215,  "stealthed" );
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

  equipped_weapon_dps = main_hand_weapon.damage / main_hand_weapon.swing_time.total_seconds();

  scales_with[ STAT_WEAPON_SPEED  ] = false;

  if ( primary_tree() == DRUID_FERAL )
    scales_with[ STAT_SPIRIT ] = false;

  // Balance of Power treats Spirit like Spell Hit Rating
  if ( specialization.balance_of_power -> ok() && sim -> scaling -> scale_stat == STAT_SPIRIT )
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

  gain.bear_melee            = get_gain( "bear_melee"            );
  gain.energy_refund         = get_gain( "energy_refund"         );
  gain.eclipse               = get_gain( "eclipse"               );
  gain.enrage                = get_gain( "enrage"                );
  gain.frenzied_regeneration = get_gain( "frenzied_regeneration" );
  gain.glyph_ferocious_bite  = get_gain( "glyph_ferocious_bite"  );
  gain.glyph_of_innervate    = get_gain( "glyph_of_innervate"    );
  gain.incoming_damage       = get_gain( "incoming_damage"       );
  gain.lotp_health           = get_gain( "lotp_health"           );
  gain.lotp_mana             = get_gain( "lotp_mana"             );
  gain.natural_reaction      = get_gain( "natural_reaction"      );
  gain.omen_of_clarity       = get_gain( "omen_of_clarity"       );
  gain.primal_fury           = get_gain( "primal_fury"           );
  gain.primal_madness        = get_gain( "primal_madness"        );
  gain.revitalize            = get_gain( "revitalize"            );
  gain.tigers_fury           = get_gain( "tigers_fury"           );
}

// druid_t::init_procs ======================================================

void druid_t::init_procs()
{
  player_t::init_procs();

  proc.empowered_touch          = get_proc( "empowered_touch"        );
  proc.parry_haste              = get_proc( "parry_haste"            );
  proc.primal_fury              = get_proc( "primal_fury"            );
  proc.revitalize               = get_proc( "revitalize"             );
  proc.unaligned_eclipse_gain   = get_proc( "unaligned_eclipse_gain" );
  proc.wrong_eclipse_wrath      = get_proc( "wrong_eclipse_wrath"    );
  proc.wrong_eclipse_starfire   = get_proc( "wrong_eclipse_starfire" );
}

// druid_t::init_uptimes ====================================================

void druid_t::init_benefits()
{
  player_t::init_benefits();

  uptime.energy_cap   = get_benefit( "energy_cap" );
  uptime.rage_cap     = get_benefit( "rage_cap"   );
}

// druid_t::init_rng ========================================================

void druid_t::init_rng()
{
  player_t::init_rng();

  rng.berserk             = get_rng( "berserk"             );
  rng.blood_in_the_water  = get_rng( "blood_in_the_water"  );
  rng.empowered_touch     = get_rng( "empowered_touch"     );
  rng.euphoria            = get_rng( "euphoria"            );
  rng.primal_fury         = get_rng( "primal_fury"         );
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

    if ( primary_tree() == DRUID_FERAL )
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
        action_list_str += "/mark_of_the_wild,if=!aura.str_agi_int.up";
        action_list_str += "/bear_form";
        action_list_str += "/auto_attack";
        action_list_str += "/snapshot_stats";
        action_list_str += init_use_racial_actions();
        action_list_str += "/skull_bash_bear";
        action_list_str += "/faerie_fire_feral,if=debuff.weakened_armor.stack<3";
        action_list_str += "/survival_instincts"; // For now use it on CD
        action_list_str += "/barkskin"; // For now use it on CD
        action_list_str += "/enrage";
        action_list_str += use_str;
        action_list_str += init_use_profession_actions();
        action_list_str += "/maul,if=rage>=75";
        action_list_str += "/mangle_bear";
        action_list_str += "/lacerate,if=!ticking";
        action_list_str += "/thrash";
        action_list_str += "/pulverize,if=buff.lacerate.stack=3&buff.pulverize.remains<=2";
        action_list_str += "/lacerate,if=buff.lacerate.stack<3";
        if ( talent.berserk -> ok() ) action_list_str+="/berserk";
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
        action_list_str += "/mark_of_the_wild,if=!aura.str_agi_int.up";
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
        if ( talent.berserk -> ok() )
        {
          action_list_str += "/berserk,if=buff.tigers_fury.up|(target.time_to_die<";
          action_list_str += ( glyph.berserk -> ok() ) ? "25" : "15";
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
        action_list_str += "/faerie_fire_feral,if=debuff.faerie_fire.stack<3|!(debuff.sunder_armor.up|debuff.expose_armor.up)";
        action_list_str += "/mangle_cat,if=debuff.mangle.remains<=2&(!debuff.mangle.up|debuff.mangle.remains>=0.0)";
        action_list_str += "/ravage,if=(buff.stampede_cat.up|buff.t13_4pc_melee.up)&(buff.stampede_cat.remains<=1|buff.t13_4pc_melee.remains<=1)";

        if ( talent.blood_in_the_water -> ok() )
        {
          action_list_str += "/ferocious_bite,if=buff.combo_points.stack>=1&dot.rip.ticking&dot.rip.remains<=2.1&target.health_pct<=" + bitw_hp;
          action_list_str += "/ferocious_bite,if=buff.combo_points.stack>=5&dot.rip.ticking&target.health_pct<=" + bitw_hp;
        }
        action_list_str += use_str;
        action_list_str += init_use_profession_actions();
        action_list_str += "/shred,extend_rip=1,if=position_back&dot.rip.ticking&dot.rip.remains<=4";
        action_list_str += "/mangle_cat,extend_rip=1,if=position_front&dot.rip.ticking&dot.rip.remains<=4";
        if ( talent.blood_in_the_water -> ok() )
          action_list_str += "&target.health_pct>" + bitw_hp;
        action_list_str += "/rip,if=buff.combo_points.stack>=5&target.time_to_die>=6&dot.rip.remains<2.0&(buff.berserk.up|dot.rip.remains<=cooldown.tigers_fury.remains)";
        action_list_str += "/ferocious_bite,if=buff.combo_points.stack>=5&dot.rip.remains>5.0&buff.savage_roar.remains>=3.0&buff.berserk.up";
        action_list_str += "/rake,if=target.time_to_die>=8.5&buff.tigers_fury.up&dot.rake.remains<9.0&(!dot.rake.ticking|dot.rake.multiplier<multiplier)";
        action_list_str += "/rake,if=target.time_to_die>=dot.rake.remains&dot.rake.remains<3.0&(buff.berserk.up|energy>=71|(cooldown.tigers_fury.remains+0.8)>=dot.rake.remains)";
        action_list_str += "/shred,if=position_back&buff.omen_of_clarity.react";
        action_list_str += "/mangle_cat,if=position_front&buff.omen_of_clarity.react";
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
        action_list_str += "/shred,if=position_back&(buff.tigers_fury.up|buff.berserk.up)";
        action_list_str += "/shred,if=position_back&((buff.combo_points.stack<5&dot.rip.remains<3.0)|(buff.combo_points.stack=0&buff.savage_roar.remains<2))";
        action_list_str += "/shred,if=position_back&cooldown.tigers_fury.remains<=3.0";
        action_list_str += "/shred,if=position_back&target.time_to_die<=8.5";
        action_list_str += "/shred,if=position_back&time_to_max_energy<=1.0";
        action_list_str += "/mangle_cat,if=position_front&(buff.tigers_fury.up|buff.berserk.up)";
        action_list_str += "/mangle_cat,if=position_front&((buff.combo_points.stack<5&dot.rip.remains<3.0)|(buff.combo_points.stack=0&buff.savage_roar.remains<2))";
        action_list_str += "/mangle_cat,if=position_front&cooldown.tigers_fury.remains<=3.0";
        action_list_str += "/mangle_cat,if=position_front&target.time_to_die<=8.5";
        action_list_str += "/mangle_cat,if=position_front&time_to_max_energy<=1.0";
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
      action_list_str += "/mark_of_the_wild,if=!aura.str_agi_int.up";
      if ( specialization.moonkin_form -> ok() )
        action_list_str += "/moonkin_form";
      action_list_str += "/snapshot_stats";
      action_list_str += "/volcanic_potion,if=!in_combat";
      action_list_str += "/volcanic_potion,if=buff.bloodlust.react|target.time_to_die<=40";
      action_list_str += "/faerie_fire,if=debuff.weakened_armor.stack<3";
      action_list_str += "/wild_mushroom_detonate,if=buff.wild_mushroom.stack=3";
      action_list_str += init_use_racial_actions();
      action_list_str += "/insect_swarm,if=(ticks_remain<2|(dot.insect_swarm.remains<10&buff.solar_eclipse.up&eclipse<15))&(buff.solar_eclipse.up|buff.lunar_eclipse.up|time<10)";

      action_list_str += "/wild_mushroom_detonate,moving=0,if=buff.wild_mushroom.stack>0&buff.solar_eclipse.up";
      if ( talent.typhoon -> ok() )
        action_list_str += "/typhoon,moving=1";
      if ( talent.starfall -> ok() )
      {
        action_list_str += "/starfall,if=eclipse<-80";
      }

      if ( talent.sunfire -> ok() )
      {
        action_list_str += "/sunfire,if=(ticks_remain<2&!dot.moonfire.remains>0)|(eclipse<15&dot.sunfire.remains<10)";
      }
      action_list_str += "/moonfire,if=buff.lunar_eclipse.up&((ticks_remain<2";
      if ( talent.sunfire -> ok() )
        action_list_str += "&!dot.sunfire.remains>0";
      action_list_str += ")|(eclipse>-20&dot.moonfire.remains<10))";

      if ( primary_tree() == DRUID_BALANCE )
      {
        action_list_str += "/starsurge,if=buff.solar_eclipse.up|buff.lunar_eclipse.up";
      }
      action_list_str += "/innervate,if=mana_pct<50";
      if ( talent.force_of_nature -> ok() )
        action_list_str += "/treants,time>=5";
      action_list_str += use_str;
      action_list_str += init_use_profession_actions();
      action_list_str += "/starfire,if=eclipse_dir=1&eclipse<80";
      action_list_str += "/starfire,if=prev.wrath=1&eclipse_dir=-1&eclipse<-87";
      action_list_str += "/wrath,if=eclipse_dir=-1&eclipse>=-87";
      action_list_str += "/wrath,if=prev.starfire=1&eclipse_dir=1&eclipse>=80";
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
      action_list_str += "/mark_of_the_wild,if=!aura.str_agi_int.up";
      action_list_str += "/snapshot_stats";
      action_list_str += "/volcanic_potion,if=!in_combat";
      action_list_str += "/volcanic_potion,if=buff.bloodlust.react|target.time_to_die<=40";
      action_list_str += "/innervate";
      if ( talent.tree_of_life -> ok() ) action_list_str += "/tree_of_life";
      action_list_str += "/healing_touch,if=buff.omen_of_clarity.up";
      action_list_str += "/rejuvenation,if=!ticking|remains<tick_time";
      action_list_str += "/lifebloom,if=buffs_lifebloom.stack<3";
      action_list_str += "/swiftmend";
      if ( talent.tree_of_life -> ok() )
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
  eclipse_bar_direction = 0;
  base_gcd = timespan_t::from_seconds( 1.5 );
}

// druid_t::regen ===========================================================

void druid_t::regen( timespan_t periodicity )
{
  resource_type_e resource_type = primary_resource();

  if ( resource_type == RESOURCE_ENERGY )
  {
    uptime.energy_cap -> update( resources.current[ RESOURCE_ENERGY ] ==
                                  resources.max    [ RESOURCE_ENERGY ] );
  }
  else if ( resource_type == RESOURCE_MANA )
  {
    if ( buff.glyph_of_innervate -> check() )
      resource_gain( RESOURCE_MANA, buff.glyph_of_innervate -> value() * periodicity.total_seconds(), gain.glyph_of_innervate );
  }
  else if ( resource_type == RESOURCE_RAGE )
  {
    if ( buff.enrage -> up() )
      resource_gain( RESOURCE_RAGE, 1.0 * periodicity.total_seconds(), gain.enrage );

    uptime.rage_cap -> update( resources.current[ RESOURCE_RAGE ] ==
                                resources.max    [ RESOURCE_RAGE ] );
  }

  player_t::regen( periodicity );
}

// druid_t::available =======================================================

timespan_t druid_t::available() const
{
  if ( primary_resource() != RESOURCE_ENERGY )
    return timespan_t::from_seconds( 0.1 );

  double energy = resources.current[ RESOURCE_ENERGY ];

  if ( energy > 25 ) return timespan_t::from_seconds( 0.1 );

  return std::max(
           timespan_t::from_seconds( ( 25 - energy ) / energy_regen_per_second() ),
           timespan_t::from_seconds( 0.1 )
         );
}

// druid_t::combat_begin ====================================================

void druid_t::combat_begin()
{
  player_t::combat_begin();

  // Start the fight with 0 rage
  resources.current[ RESOURCE_RAGE ] = 0;

  // Moonkins can precast 3 wild mushrooms without aggroing the boss
  buff.wild_mushroom -> trigger( 3 );
}

// druid_t::composite_armor_multiplier ======================================

double druid_t::composite_armor_multiplier() const
{
  double a = player_t::composite_armor_multiplier();

  if ( buff.bear_form -> check() )
    a += 3.7 * ( 1.0 + 0.11 * talent.thick_hide -> ok() );

  return a;
}

// druid_t::composite_attack_power ==========================================

double druid_t::composite_attack_power() const
{
  double ap = player_t::composite_attack_power();

  if ( buff.bear_form -> check() || buff.cat_form  -> check() )
    ap += 2.0 * ( agility() - 10.0 );

  return floor( ap );
}

// druid_t::composite_player_multiplier =====================================

double druid_t::composite_player_multiplier( school_type_e school, const action_t* a ) const
{
  double m = player_t::composite_player_multiplier( school, a );

  if ( ( school == SCHOOL_PHYSICAL ) || ( school == SCHOOL_BLEED ) )
    m *= 1.0 + buff.tigers_fury -> value();

  if ( school == SCHOOL_BLEED && primary_tree() == DRUID_FERAL )
    m *= 1.0 + specialization.razor_claws -> effect1().coeff() * 0.01 * composite_mastery();

  if ( primary_tree() == DRUID_BALANCE )
  {
    // Both eclipse buffs need their own checks
    if ( school == SCHOOL_ARCANE || school == SCHOOL_SPELLSTORM )
      if ( buff.eclipse_lunar -> up() )
        m *= 1.0 + ( buff.eclipse_lunar -> data().effect1().percent()
                 + composite_mastery() * specialization.total_eclipse -> effect1().coeff() * 0.01 );

    if ( school == SCHOOL_NATURE || school == SCHOOL_SPELLSTORM )
      if ( buff.eclipse_solar -> up() )
        m *= 1.0 + ( buff.eclipse_solar -> data().effect1().percent()
                 + composite_mastery() * specialization.total_eclipse -> effect1().coeff() * 0.01 );
  }
  
  if ( school == SCHOOL_ARCANE || school == SCHOOL_NATURE || school == SCHOOL_SPELLSTORM )
  {
    if ( buff.moonkin_form -> check() )
      m *= 1.0 + specialization.moonkin_form_bonuses -> effect3().percent();
  }

  return m;
}

// druid_t::composite_spell_hit =============================================

double druid_t::composite_spell_hit() const
{
  double hit = player_t::composite_spell_hit();

  // BoP does not convert base spirit into hit!
  hit += ( spirit() - attribute_base[ ATTR_SPIRIT ] ) * ( specialization.balance_of_power -> effect1().percent() ) / rating.spell_hit;

  return hit;
}

// druid_t::composite_spell_crit ============================================

double druid_t::composite_spell_crit() const
{
  double crit = player_t::composite_spell_crit();

  crit += talent.natures_majesty -> effect1().base_value() / 100.0;

  return crit;
}

// druid_t::composite_attribute_multiplier ==================================

double druid_t::composite_attribute_multiplier( attribute_type_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  // The matching_gear_multiplier is done statically for performance reasons,
  // unfortunately that's before we're in cat form or bear form, so let's compensate here

  switch ( attr )
  {
  case ATTR_AGILITY:
    if ( buff.cat_form -> check() )
    {
      if ( primary_tree() == DRUID_FERAL )
        m *= 1.05;
    }
    break;
  case ATTR_STAMINA:
    if ( buff.bear_form -> check() )
    {
      m *= 1.0 + talent.heart_of_the_wild -> effect1().percent();

      if ( primary_tree() == DRUID_FERAL )
        m *= 1.05;
    }
    break;
  case ATTR_INTELLECT:
    if ( sim -> auras.str_agi_int -> check() )
      m *= 1.05;
    break;
  default:
    break;
  }

  return m;
}

// Heart of the Wild does nothing for base int so we need to do completely silly
// tricks to match paper doll in game
double druid_t::composite_attribute( attribute_type_e attr ) const
{
  double a = player_t::composite_attribute( attr );

  if ( attr == ATTR_INTELLECT )
    a += ( a - attribute_base[ attr ] )* talent.heart_of_the_wild -> effect1().percent();

  return a;
}

// druid_t::matching_gear_multiplier ========================================

double druid_t::matching_gear_multiplier( attribute_type_e attr ) const
{
  switch ( primary_tree() )
  {
  case DRUID_BALANCE:
  case DRUID_RESTORATION:
    if ( attr == ATTR_INTELLECT )
      return 0.05;
    break;
  default:
    break;
  }

  return 0.0;
}

// druid_t::composite_tank_crit =============================================

double druid_t::composite_tank_crit( school_type_e school ) const
{
  double c = player_t::composite_tank_crit( school );

  if ( school == SCHOOL_PHYSICAL )
    c += talent.thick_hide -> effect3().percent();

  return c;
}

// druid_t::create_expression ===============================================

expr_t* druid_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "eclipse" )
  {
    return make_ref_expr( "eclipse", eclipse_bar_value);
  }
  else if ( name_str == "eclipse_dir" )
  {
    return make_ref_expr( "eclipse_dir", eclipse_bar_direction);
  }

  return player_t::create_expression( a, name_str );
}

// druid_t::decode_set ======================================================

int druid_t::decode_set( const item_t& item ) const
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

role_type_e druid_t::primary_role() const
{

  if ( primary_tree() == DRUID_BALANCE )
  {
    if ( player_t::primary_role() == ROLE_HEAL )
      return ROLE_HEAL;

    return ROLE_SPELL;
  }

  else if ( primary_tree() == DRUID_FERAL )
  {
    if ( player_t::primary_role() == ROLE_TANK )
      return ROLE_TANK;

    // Implement Automatic Tank detection

    return ROLE_ATTACK;
  }

  else if ( primary_tree() == DRUID_RESTORATION )
  {
    if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_SPELL )
      return ROLE_SPELL;

    return ROLE_HEAL;
  }

  return ROLE_NONE;
}

// druid_t::primary_resource ================================================

resource_type_e druid_t::primary_resource() const
{
  if ( primary_role() == ROLE_SPELL || primary_role() == ROLE_HEAL )
    return RESOURCE_MANA;

  if ( primary_role() == ROLE_TANK )
    return RESOURCE_RAGE;

  return RESOURCE_ENERGY;
}

// druid_t::assess_damage ===================================================

double druid_t::assess_damage( double        amount,
                               school_type_e school,
                               dmg_type_e    dtype,
                               result_type_e result,
                               action_t*     action )
{
  if ( result == RESULT_DODGE && talent.natural_reaction -> ok() )
    resource_gain( RESOURCE_RAGE, talent.natural_reaction -> effect2().base_value(), gain.natural_reaction );

  // This needs to use unmitigated damage, which amount currently is
  // FIX ME: Rage gains need to trigger on every attempt to poke the bear
  double rage_gain = amount * 18.92 / resources.max[ RESOURCE_HEALTH ];
  resource_gain( RESOURCE_RAGE, rage_gain, gain.incoming_damage );

  if ( SCHOOL_SPELL_MASK & ( int64_t( 1 ) << school ) && talent.perseverance -> ok() )
    amount *= 1.0 + talent.perseverance -> effect1().percent();

  if ( talent.natural_reaction -> ok() )
    amount *= 1.0 + talent.natural_reaction -> effect3().percent();

  if ( buff.barkskin -> up() )
    amount *= 1.0 + buff.barkskin -> value();

  if ( buff.survival_instincts -> up() )
    amount *= 1.0 + buff.survival_instincts -> value();

  // Call here to benefit from -10% physical damage before SD is taken into account
  amount = player_t::assess_damage( amount, school, dtype, result, action );

  if ( school == SCHOOL_PHYSICAL && buff.savage_defense -> up() )
  {
    amount -= buff.savage_defense -> value();

    if ( amount < 0 )
      amount = 0;

    buff.savage_defense -> expire();
  }

  return amount;
}

player_t::heal_info_t druid_t::assess_heal( double        amount,
                                            school_type_e school,
                                            dmg_type_e    dmg_type,
                                            result_type_e result,
                                            action_t*     action )
{
  amount *= 1.0 + buff.frenzied_regeneration -> check() * glyph.frenzied_regeneration -> effect1().percent();

  return player_t::assess_heal( amount, school, dmg_type, result, action );
}

#endif // SC_DRUID

// ==========================================================================
// PLAYER_T EXTENSIONS
// ==========================================================================

// player_t::create_druid  ==================================================

player_t* player_t::create_druid( sim_t*             sim,
                                  const std::string& name,
                                  race_type_e r )
{
  SC_CREATE_DRUID( sim, name, r );
}

// player_t::druid_init =====================================================

void player_t::druid_init( sim_t* sim )
{
  for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
  {
    player_t* p = sim -> actor_list[i];
#if SC_DRUID == 1
    // TODO: Figure this out
    //p -> buffs.innervate              = new innervate_buff_t( p );
#else
    p -> buffs.innervate = buff_creator_t( p, "innervate_dummy_buff" );
#endif // SC_DRUID
  }

}

// player_t::druid_combat_begin =============================================

void player_t::druid_combat_begin( sim_t* )
{
}

