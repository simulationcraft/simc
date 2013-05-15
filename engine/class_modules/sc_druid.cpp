// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE

// ==========================================================================
// Druid
// ==========================================================================

struct druid_t;

struct combo_points_t
{
private:
  druid_t& source;
  player_t& target;

  proc_t* proc;
  proc_t* wasted;

  int count;
public:
  static const int max_combo_points = 5;

  combo_points_t( druid_t& source, player_t& target );

  void add( int num, const std::string* source_name = nullptr );
  int consume( const std::string* source_name = nullptr );
  void clear() { count = 0; }
  int get() const { return count; }

  expr_t* count_expr() const
  { return make_ref_expr( "combo_points", count ); }
};

struct druid_td_t : public actor_pair_t
{
  struct dots_t
  {
    dot_t* lacerate;
    dot_t* lifebloom;
    dot_t* moonfire;
    dot_t* rake;
    dot_t* regrowth;
    dot_t* rejuvenation;
    dot_t* rip;
    dot_t* sunfire;
    dot_t* wild_growth;
  } dots;

  struct buffs_t
  {
    buff_t* lifebloom;
  } buffs;
  
  int lacerate_stack;
  combo_points_t combo_points;

  druid_td_t( player_t& target, druid_t& source );

  druid_t& p() const;

  bool hot_ticking()
  {
    return dots.regrowth      -> ticking ||
           dots.rejuvenation  -> ticking ||
           dots.lifebloom     -> ticking ||
           dots.wild_growth   -> ticking;
  }

  void reset()
  {
    lacerate_stack = 0;
    combo_points.clear();
  }
};

struct druid_t : public player_t
{
public:
  struct heart_of_the_wild_buff_t;

  // Pets
  pet_t* pet_feral_spirit[ 2 ];
  pet_t* pet_mirror_images[ 3 ];
  pet_t* pet_treants[ 3 ];

  // Auto-attacks
  weapon_t cat_weapon;
  weapon_t bear_weapon;
  melee_attack_t* cat_melee_attack;
  melee_attack_t* bear_melee_attack;

  weapon_t caster_form_weapon;
  double equipped_weapon_dps;

  // Buffs
  struct buffs_t
  {
    // General
    buff_t* cat_form;
    buff_t* dream_of_cenarius_damage;
    buff_t* dream_of_cenarius_heal;
    buff_t* frenzied_regeneration;
    buff_t* natures_swiftness;
    buff_t* natures_vigil;
    buff_t* omen_of_clarity;
    buff_t* stealthed;
    buff_t* symbiosis;

    // Balance
    buff_t* celestial_alignment;
    buff_t* chosen_of_elune;
    buff_t* eclipse_lunar;
    buff_t* eclipse_solar;
    buff_t* lunar_shower;
    buff_t* moonkin_form;
    buff_t* natures_grace;
    buff_t* shooting_stars;
    buff_t* starfall;

    // Feral
    buff_t* berserk;
    buff_t* king_of_the_jungle;
    buff_t* predatory_swiftness;
    buff_t* savage_roar;
    buff_t* tigers_fury;
    buff_t* tier15_4pc_melee;

    // Guardian
    buff_t* enrage;
    buff_t* lacerate;
    buff_t* survival_instincts;
    buff_t* tier15_2pc_tank;

    // Restoration
    buff_t* soul_of_the_forest;

    // NYI / Needs checking
    buff_t* barkskin;
    buff_t* bear_form;
    buff_t* glyph_of_innervate;
    buff_t* harmony;
    buff_t* revitalize;
    buff_t* savage_defense;
    buff_t* wild_mushroom;
    buff_t* son_of_ursoc;
    buff_t* tree_of_life;
    heart_of_the_wild_buff_t* heart_of_the_wild;
  } buff;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* lotp;
    cooldown_t* natures_swiftness;
    cooldown_t* mangle_bear;
    cooldown_t* pvp_4pc_melee;
    cooldown_t* revitalize;
    cooldown_t* starfall;
    cooldown_t* starsurge;
    cooldown_t* swiftmend;
  } cooldown;

  // Gains
  struct gains_t
  {
    // DONE
    gain_t* energy_refund;
    gain_t* enrage;
    gain_t* frenzied_regeneration;
    gain_t* lotp_health;
    gain_t* lotp_mana;
    gain_t* omen_of_clarity;
    gain_t* primal_fury;
    gain_t* soul_of_the_forest;
    gain_t* tigers_fury;
    gain_t* eclipse;

    // NYI / Needs checking
    gain_t* bear_melee;
    gain_t* bear_form;
    gain_t* glyph_of_innervate;
    gain_t* glyph_ferocious_bite;
    gain_t* mangle;
    gain_t* revitalize;
  } gain;

  // Glyphs
  struct glyphs_t
  {
    // DONE
    const spell_data_t* blooming;
    const spell_data_t* ferocious_bite;
    const spell_data_t* frenzied_regeneration;
    const spell_data_t* healing_touch;
    const spell_data_t* maul;
    const spell_data_t* moonbeast;
    const spell_data_t* skull_bash;
    const spell_data_t* wild_growth;

    // NYI / Needs checking
    const spell_data_t* innervate;
    const spell_data_t* lifebloom;
    const spell_data_t* regrowth;
    const spell_data_t* rejuvenation;
    const spell_data_t* savagery;
  } glyph;

  // Masteries
  struct masteries_t
  {
    // Done
    const spell_data_t* total_eclipse;
    const spell_data_t* razor_claws;

    // NYI / TODO!
    const spell_data_t* natures_guardian; // NYI
    const spell_data_t* harmony;
  } mastery;

  // Procs
  struct procs_t
  {
    proc_t* primal_fury;
    proc_t* revitalize;
    proc_t* wrong_eclipse_wrath;
    proc_t* wrong_eclipse_starfire;
    proc_t* unaligned_eclipse_gain;
    proc_t* combo_points;
    proc_t* combo_points_wasted;
    proc_t* shooting_stars_wasted;
    proc_t* tier15_2pc_melee;
  } proc;

  // Random Number Generation
  struct rngs_t
  {
    rng_t* euphoria;
    rng_t* mangle;
    rng_t* revitalize;
    rng_t* tier15_2pc_melee;
  } rng;

  // Class Specializations
  struct specializations_t
  {
    // DONE
    // Generic
    const spell_data_t* leather_specialization;
    const spell_data_t* omen_of_clarity; // Feral and Resto have this

    // Feral / Guardian
    const spell_data_t* leader_of_the_pack;
    const spell_data_t* predatory_swiftness;

    // NYI / Needs checking
    const spell_data_t* killer_instinct;
    const spell_data_t* nurturing_instinct;

    // Balance
    const spell_data_t* balance_of_power;
    const spell_data_t* celestial_focus;
    const spell_data_t* eclipse;
    const spell_data_t* euphoria;
    const spell_data_t* lunar_shower;
    const spell_data_t* owlkin_frenzy;
    const spell_data_t* shooting_stars;

    // Guardian
    const spell_data_t* thick_hide;

    // Restoration
    const spell_data_t* living_seed;
    const spell_data_t* meditation;
    const spell_data_t* natural_insight;
    const spell_data_t* natures_focus;
    const spell_data_t* revitalize;
  } spec;


  struct spells_t
  {
    const spell_data_t* berserk_bear; // Berserk bear mangler
    const spell_data_t* berserk_cat; // Berserk cat resource cost reducer
    const spell_data_t* bear_form; // Bear form bonuses
    const spell_data_t* combo_point; // Combo point spell
    const spell_data_t* eclipse; // Eclipse mana gain
    const spell_data_t* heart_of_the_wild; // HotW INT/AGI bonus
    const spell_data_t* leader_of_the_pack; // LotP aura
    const spell_data_t* mangle; // Lacerate mangle cooldown reset
    const spell_data_t* moonkin_form; // Moonkin form bonuses
    const spell_data_t* primal_fury; // Primal fury rage gain
    const spell_data_t* regrowth; // Old GoRegrowth
    const spell_data_t* survival_instincts; // Survival instincts aura
    const spell_data_t* swiftmend; // Swiftmend AOE heal trigger
    const spell_data_t* swipe; // Bleed damage multiplier for Shred etc
  } spell;

  // Eclipse Management
  int eclipse_bar_value; // Tracking the current value of the eclipse bar
  int eclipse_bar_direction; // Tracking the current direction of the eclipse bar

  int initial_eclipse;
  int default_initial_eclipse;
  int preplant_mushrooms;

  // Talents
  struct talents_t
  {
    // MoP: Done

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
    const spell_data_t* natures_vigil;
  } talent;

  bool inflight_starsurge;

  druid_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, DRUID, name, r ),
    caster_form_weapon(),
    buff( buffs_t() ),
    cooldown( cooldowns_t() ),
    gain( gains_t() ),
    glyph( glyphs_t() ),
    mastery( masteries_t() ),
    proc( procs_t() ),
    rng( rngs_t() ),
    spec( specializations_t() ),
    spell( spells_t() ),
    talent( talents_t() ),
    inflight_starsurge( false )
  {
    eclipse_bar_value     = 0;
    eclipse_bar_direction = 0;

    initial_eclipse = 65535;
    default_initial_eclipse = -75;
    preplant_mushrooms = true;

    cooldown.lotp              = get_cooldown( "lotp"              );
    cooldown.natures_swiftness = get_cooldown( "natures_swiftness" );
    cooldown.mangle_bear       = get_cooldown( "mangle_bear"       );
    cooldown.pvp_4pc_melee     = get_cooldown( "pvp_4pc_melee"     );
    cooldown.pvp_4pc_melee -> duration = timespan_t::from_seconds( 30.0 );
    cooldown.revitalize        = get_cooldown( "revitalize"        );
    cooldown.starfall          = get_cooldown( "starfall"          );
    cooldown.starsurge         = get_cooldown( "starsurge"         );
    cooldown.swiftmend         = get_cooldown( "swiftmend"         );


    cat_melee_attack = 0;
    bear_melee_attack = 0;

    equipped_weapon_dps = 0;

    base.distance = ( specialization() == DRUID_FERAL || specialization() == DRUID_GUARDIAN ) ? 3 : 30;
  }

  // Character Definition
  virtual void      init_spells();
  virtual void      init_base_stats();
  virtual void      create_buffs();
  virtual void      init_scaling();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_benefits();
  virtual void      init_rng();
  virtual void      init_actions();
  virtual void      invalidate_cache( cache_e );
  virtual void      combat_begin();
  virtual void      reset();
  virtual void      regen( timespan_t periodicity );
  virtual timespan_t available();
  virtual double    composite_armor_multiplier();
  virtual double    composite_melee_haste();
  virtual double    composite_melee_hit();
  virtual double    composite_melee_expertise( weapon_t* );
  virtual double    composite_melee_crit();
  virtual double    composite_player_multiplier( school_e school );
  virtual double    composite_player_td_multiplier( school_e, action_t* );
  virtual double    composite_player_heal_multiplier( school_e school );
  virtual double    composite_spell_haste();
  virtual double    composite_spell_hit();
  virtual double    composite_spell_crit();
  virtual double    composite_spell_power( school_e school );
  virtual double    composite_attribute( attribute_e attr );
  virtual double    composite_attribute_multiplier( attribute_e attr );
  virtual double    matching_gear_multiplier( attribute_e attr );
  virtual double    composite_tank_parry() { return 0; }
  virtual double    composite_tank_block() { return 0; }
  virtual double    composite_tank_crit( school_e school );
  virtual double    composite_tank_dodge();
  virtual expr_t*   create_expression( action_t*, const std::string& name );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual int       decode_set( item_t& );
  virtual resource_e primary_resource();
  virtual role_e primary_role();
  virtual void    assess_damage( school_e school, dmg_e, action_state_t* );
  virtual void assess_heal( school_e, dmg_e, heal_state_t* );
  virtual void      create_options();
  virtual bool      create_profile( std::string& profile_str, save_e type=SAVE_ALL, bool save_html=false );

  target_specific_t<druid_td_t*> target_data;

  virtual druid_td_t* get_target_data( player_t* target )
  {
    assert( target );
    druid_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new druid_td_t( *target, *this );
    }
    return td;
  }

  void trigger_shooting_stars( result_e );
  void trigger_soul_of_the_forest();

  void init_beast_weapon( weapon_t& w, double swing_time )
  {
    w = main_hand_weapon;
    double mod = swing_time /  w.swing_time.total_seconds();
    w.type = WEAPON_BEAST;
    w.school = SCHOOL_PHYSICAL;
    w.min_dmg *= mod;
    w.max_dmg *= mod;
    w.damage *= mod;
    w.swing_time = timespan_t::from_seconds( swing_time );
  }
};

namespace pets {


// ==========================================================================
// Pets and Guardians
// ==========================================================================

// Symbiosis Feral Spirits

struct symbiosis_feral_spirit_t : public pet_t
{
  struct melee_t : public melee_attack_t
  {
    melee_t( symbiosis_feral_spirit_t* player ) :
      melee_attack_t( "melee", player, spell_data_t::nil() )
    {
      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      background = true;
      repeating = true;
      may_crit = true;
      school = SCHOOL_PHYSICAL;
    }

    symbiosis_feral_spirit_t* p() { return static_cast<symbiosis_feral_spirit_t*>( player ); }

    void init()
    {
      melee_attack_t::init();

      pet_t* first_pet = p() -> o() -> find_pet( "symbiosis_wolf" );
      if ( first_pet != player )
        stats = first_pet -> find_stats( name() );
    }
  };

  struct spirit_bite_t : public melee_attack_t
  {
    spirit_bite_t( symbiosis_feral_spirit_t* player ) :
      melee_attack_t( "spirit_bite", player, player -> find_spell( 58859 ) )
    {
      may_crit  = true;
      special   = true;
      direct_power_mod = data().extra_coeff();
      cooldown -> duration = timespan_t::from_seconds( 7.0 );

    }

    symbiosis_feral_spirit_t* p() { return static_cast<symbiosis_feral_spirit_t*>( player ); }

    void init()
    {
      melee_attack_t::init();
      pet_t* first_pet = p() -> o() -> find_pet( "symbiosis_wolf" );
      if ( first_pet != player )
        stats = first_pet -> find_stats( name() );
    }
  };

  melee_t* melee;

  symbiosis_feral_spirit_t( sim_t* sim, druid_t* owner ) :
    pet_t( sim, owner, "symbiosis_wolf" ), melee( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = dbc.spell_scaling( o() -> type, level ) * 0.5;
    main_hand_weapon.max_dmg    = dbc.spell_scaling( o() -> type, level ) * 0.5;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.5 );

    owner_coeff.ap_from_ap = 0.31;
  }

  druid_t* o() { return static_cast<druid_t*>( owner ); }

  virtual void init_base_stats()
  {
    pet_t::init_base_stats();

    melee = new melee_t( this );
  }

  virtual void init_actions()
  {
    action_list_str = "spirit_bite";

    pet_t::init_actions();
  }

  action_t* create_action( const std::string& name,
                           const std::string& options_str )
  {
    if ( name == "spirit_bite" ) return new spirit_bite_t( this );

    return pet_t::create_action( name, options_str );
  }

  void schedule_ready( timespan_t delta_time = timespan_t::zero(), bool waiting = false )
  {
    if ( melee && ! melee -> execute_event )
      melee -> schedule_execute();

    pet_t::schedule_ready( delta_time, waiting );
  }
};

// Symbiosis Mirror Images

struct symbiosis_mirror_image_t : public pet_t
{
  struct wrath_t : public spell_t
  {
    wrath_t( symbiosis_mirror_image_t* player ) :
      spell_t( "wrath", player, player -> find_spell( 110691 ) )
    {
      if ( player -> o() -> pet_mirror_images[ 0 ] )
        stats = player -> o() -> pet_mirror_images[ 0 ] -> get_stats( "wrath" );
    }
  };

  druid_t* o() { return static_cast< druid_t* >( owner ); }

  symbiosis_mirror_image_t( sim_t* sim, druid_t* owner ) :
    pet_t( sim, owner, "mirror_image", true /*GUARDIAN*/ )
  {
    owner_coeff.sp_from_sp = 1.00;
    action_list_str = "wrath";
  }

  virtual void init_base_stats()
  {
    pet_t::init_base_stats();
  }

  virtual resource_e primary_resource() { return RESOURCE_MANA; }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "wrath"  ) return new wrath_t( this );

    return pet_t::create_action( name, options_str );
  }
};

// Balance Treants ==========================================================

struct treants_balance_t : public pet_t
{
  struct melee_t : public melee_attack_t
  {
    melee_t( treants_balance_t* player ) :
      melee_attack_t( "treant_melee", player )
    {
      if ( player -> o() -> pet_treants[ 0 ] )
        stats = player -> o() -> pet_treants[ 0 ] -> get_stats( "treant_melee" );

      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      base_dd_min = base_dd_max = 1;
      school = SCHOOL_PHYSICAL;

      trigger_gcd = timespan_t::zero();

      background = true;
      repeating  = false;
      special    = false;
      may_glance = true;
      may_crit   = true;
    }
  };

  struct wrath_t : public spell_t
  {
    wrath_t( treants_balance_t* player ) :
      spell_t( "wrath", player, player -> find_spell( 113769 ) )
    {
      if ( player -> o() -> pet_treants[ 0 ] )
        stats = player -> o() -> pet_treants[ 0 ] -> get_stats( "wrath" );
    }
  };

  druid_t* o() { return static_cast< druid_t* >( owner ); }

  treants_balance_t( sim_t* sim, druid_t* owner ) :
    pet_t( sim, owner, "treant", false /*GUARDIAN*/ )
  {
    owner_coeff.sp_from_sp = 1.0;
    action_list_str = "wrath";
  }

  virtual void init_base_stats()
  {
    pet_t::init_base_stats();

    resources.base[ RESOURCE_HEALTH ] = 9999; // Level 85 value
    resources.base[ RESOURCE_MANA   ] = 0;

    initial.stats.attribute[ ATTR_INTELLECT ] = 0;
    initial.spell_power_per_intellect = 0;
    intellect_per_owner = 0;
    stamina_per_owner = 0;

    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 580;
    main_hand_weapon.max_dmg    = 580;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.65 );

    main_hand_attack = new melee_t( this );
  }

  virtual resource_e primary_resource() { return RESOURCE_MANA; }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "wrath"  ) return new wrath_t( this );

    return pet_t::create_action( name, options_str );
  }
  virtual void summon( timespan_t duration=timespan_t::zero() )
  {
    pet_t::summon( duration );
  }
};

// Feral Treants ============================================================

struct treants_feral_t : public pet_t
{
  struct melee_t : public melee_attack_t
  {
    melee_t( treants_feral_t* player ) :
      melee_attack_t( "treant_melee", player )
    {
      if ( player -> o() -> pet_treants[ 0 ] )
        stats = player -> o() -> pet_treants[ 0 ] -> get_stats( "treant_melee" );

      weapon = &( player -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      base_dd_min = base_dd_max = 1;
      school = SCHOOL_PHYSICAL;

      trigger_gcd = timespan_t::zero();

      background = true;
      repeating  = true;
      special    = false;
      may_glance = true;
      may_crit   = true;
    }
  };

  druid_t* o() { return static_cast< druid_t* >( owner ); }

  treants_feral_t( sim_t* sim, druid_t* owner ) :
    pet_t( sim, owner, "treant", false /*GUARDIAN*/ )
  {
    // FIX ME
    owner_coeff.ap_from_ap = 1.0;
  }

  virtual void init_base_stats()
  {
    pet_t::init_base_stats();

    resources.base[ RESOURCE_HEALTH ] = 9999; // Level 85 value
    resources.base[ RESOURCE_MANA   ] = 0;

    stamina_per_owner = 0;

    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.min_dmg    = 580;
    main_hand_weapon.max_dmg    = 580;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 1.65 );

    main_hand_attack = new melee_t( this );
  }

  void init_actions()
  {
    action_list_str = "auto_attack";

    pet_t::init_actions();
  }

  virtual resource_e primary_resource() { return RESOURCE_MANA; }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "auto_attack"  ) return new melee_t( this );

    return pet_t::create_action( name, options_str );
  }
  virtual void summon( timespan_t duration=timespan_t::zero() )
  {
    pet_t::summon( duration );
    // Treants cast on the target will instantly perform a melee before
    // starting to cast wrath
    main_hand_attack -> execute();
  }
};

} // end namespace pets

struct druid_t::heart_of_the_wild_buff_t : public buff_t
{
private:
  const spell_data_t* select_spell( const druid_t& p )
  {
    unsigned id = 0;
    if ( p.talent.heart_of_the_wild -> ok() )
    {
      switch ( p.specialization() )
      {
      case DRUID_BALANCE:     id = 108291; break;
      case DRUID_FERAL:       id = 108292; break;
      case DRUID_GUARDIAN:    id = 108293; break;
      case DRUID_RESTORATION: id = 108294; break;
      default: break;
      }
    }
    return p.find_spell( id );
  }

  bool all_but( specialization_e spec )
  { return check() > 0 && player -> specialization() != spec; }

  druid_t& p() const { return *static_cast<druid_t*>( player ); }
public:
  heart_of_the_wild_buff_t( druid_t& p ) :
    buff_t( buff_creator_t( &p, "heart_of_the_wild" )
            .spell( select_spell( p ) ) )
  {
    invalidate_list.push_back( CACHE_AGILITY );
    invalidate_list.push_back( CACHE_HIT );
    invalidate_list.push_back( CACHE_EXP );
    invalidate_list.push_back( CACHE_PLAYER_HEAL_MULTIPLIER );
    requires_invalidation = true;
  }

  bool heals_are_free()
  { return all_but( DRUID_RESTORATION ); }

  bool damage_spells_are_free()
  { return all_but( DRUID_BALANCE ); }

  double damage_spell_multiplier()
  {
    if ( ! check() ) return 0.0;

    double m;
    switch ( player -> specialization() )
    {
    case DRUID_FERAL:
    case DRUID_RESTORATION:
      m = data().effectN( 4 ).percent();
      break;
    case DRUID_GUARDIAN:
      m = data().effectN( 5 ).percent();
      break;
    case DRUID_BALANCE:
    default:
      return 0.0;
    }

    return m;
  }

  double heal_multiplier()
  {
    if ( ! check() ) return 0.0;

    double m;
    switch ( player -> specialization() )
    {
    case DRUID_FERAL:
    case DRUID_GUARDIAN:
    case DRUID_BALANCE:
      m = data().effectN( 2 ).percent();
      break;
    case DRUID_RESTORATION:
    default:
      return 0.0;
    }

    return m;
  }

  double attack_hit_expertise()
  {
    if ( ! check() ) return 0.0;

    druid_t& p = this -> p();
    switch ( p.specialization() )
    {
    case DRUID_FERAL:
      if ( ! p.buff.bear_form -> check() ) return 0.0;
      break;
    case DRUID_GUARDIAN:
      if ( ! p.buff.cat_form  -> check() ) return 0.0;
      break;
    case DRUID_BALANCE:
    case DRUID_RESTORATION:
      if ( ! p.buff.bear_form -> check() && ! p.buff.cat_form -> check() ) return 0.0;
      break;
    default:
      return 0.0;
    }
    return 0.15;
  }

  double spell_hit()
  {
    if ( ! check() ) return 0.0;

    switch ( player -> specialization() )
    {
    case DRUID_FERAL:
    case DRUID_GUARDIAN:
    case DRUID_RESTORATION:
      return 0.15;
    default:
      return 0.0;
    }
  }

  double agility_multiplier()
  {
    if ( ! check() ) return 0.0;

    druid_t& p = this -> p();
    switch ( p.specialization() )
    {
    case DRUID_FERAL:
      if ( p.buff.bear_form -> check() )
        return 0.5;
      break;
    case DRUID_GUARDIAN:
      if ( p.buff.cat_form -> check() )
        return 1.1;
      break;
    case DRUID_BALANCE:
    case DRUID_RESTORATION:
      if ( p.buff.cat_form -> check() || p.buff.cat_form -> check() )
        return 1.1;
      break;
    default:
      break;
    }

    return 0.0;
  }
};

// Template for common druid action code. See priest_action_t.
template <class Base>
struct druid_action_t : public Base
{
private:
  typedef Base ab; // action base, eg. spell_t
public:
  typedef druid_action_t base_t;

  druid_action_t( const std::string& n, druid_t* player,
                  const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s )
  {
    ab::may_crit      = true;
    ab::tick_may_crit = true;
  }

  druid_t* p() const { return static_cast<druid_t*>( ab::player ); }

  druid_td_t* td( player_t* t = 0 ) { return p() -> get_target_data( t ? t : ab::target ); }

  void trigger_omen_of_clarity()
  {
    if ( ab::proc ) return;

    p() -> buff.omen_of_clarity -> trigger();
  }
};

// Druid melee attack base for cat_attack_t and bear_attack_t
template <class Base>
struct druid_attack_t : public druid_action_t< Base >
{
private:
  typedef druid_action_t< Base > ab;
public:
  typedef druid_attack_t base_t;

  druid_attack_t( const std::string& n, druid_t* player,
                  const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s )
  {
    ab::may_glance    = false;
    ab::special       = true;
  }


  virtual double action_da_multiplier()
  {
    double m = ab::action_da_multiplier();

    if ( this -> special && this -> harmful )
    {
      if ( this -> p() -> buff.dream_of_cenarius_damage -> check() )
      {
        m *= 1.0 + this -> p() -> buff.dream_of_cenarius_damage -> data().effectN( 1 ).percent();
      }
    }

    return m;
  }

  virtual double action_ta_multiplier()
  {
    double m = ab::action_ta_multiplier();

    if ( this -> special && this -> harmful )
    {
      if ( this -> p() -> buff.dream_of_cenarius_damage -> check() )
      {
        m *= 1.0 + this -> p() -> buff.dream_of_cenarius_damage -> data().effectN( 2 ).percent();
      }
    }

    return m;
  }

  virtual void execute()
  {
    ab::execute();

    if ( this -> special && this -> harmful )
    {
      if ( this -> p() -> buff.dream_of_cenarius_damage -> up() )
      {
        this -> p() -> buff.dream_of_cenarius_damage -> decrement();
      }
    }
  }

  void trigger_lotp( const action_state_t* s )
  {
    druid_t& p = *this -> p();

    if ( p.cooldown.lotp -> down() )
      return;

    // Has to do damage and can't be a proc
    if ( s -> result_amount == 0 || ab::proc )
      return;

    p.resource_gain( RESOURCE_HEALTH,
                     p.resources.max[ RESOURCE_HEALTH ] *
                     p.spell.leader_of_the_pack -> effectN( 2 ).percent(),
                     p.gain.lotp_health );

    p.resource_gain( RESOURCE_MANA,
                     p.resources.max[ RESOURCE_MANA ] *
                     p.spec.leader_of_the_pack -> effectN( 1 ).percent(),
                     p.gain.lotp_mana );

    p.cooldown.lotp -> start( timespan_t::from_seconds( 6.0 ) );
  }

};
namespace cat_attacks {

// ==========================================================================
// Druid Cat Attack
// ==========================================================================

struct cat_attack_state_t : public action_state_t
{
  int cp;

  cat_attack_state_t( action_t* action, player_t* target ) :
    action_state_t( action, target ), cp( 0 )
  { }

  void initialize()
  { action_state_t::initialize(); cp = 0; }

  std::ostringstream& debug_str( std::ostringstream& s )
  { action_state_t::debug_str( s ) << " cp=" << cp; return s; }

  void copy_state( const action_state_t* o )
  {
    action_state_t::copy_state( o );
    const cat_attack_state_t* st = debug_cast<const cat_attack_state_t*>( o );
    cp = st -> cp;
  }
};

struct cat_attack_t : public druid_attack_t<melee_attack_t>
{
  bool             requires_stealth_;
  position_e  requires_position_;
  bool             requires_combo_points;
  int              adds_combo_points;
  double           base_dd_bonus;
  double           base_td_bonus;

  cat_attack_t( const std::string& token, druid_t* p,
                const spell_data_t* s = spell_data_t::nil(),
                const std::string& options = std::string() ) :
    base_t( token, p, s ),
    requires_stealth_( false ), requires_position_( POSITION_NONE ),
    requires_combo_points( false ), adds_combo_points( 0 ),
    base_dd_bonus( 0.0 ), base_td_bonus( 0.0 )
  {
    parse_options( 0, options );

    parse_special_effect_data();
  }

  cat_attack_t( druid_t* p, const spell_data_t* s = spell_data_t::nil(),
                const std::string& options = std::string() ) :
    base_t( "", p, s ),
    requires_stealth_( false ), requires_position_( POSITION_NONE ),
    requires_combo_points( false ), adds_combo_points( 0 ),
    base_dd_bonus( 0 ), base_td_bonus( 0 )
  {
    parse_options( 0, options );

    parse_special_effect_data();
  }

private:
  void parse_special_effect_data()
  {
    for ( size_t i = 1; i <= data().effect_count(); i++ )
    {
      const spelleffect_data_t& ed = data().effectN( i );
      effect_type_t type = ed.type();

      if ( type == E_ADD_COMBO_POINTS )
        adds_combo_points = ed.base_value();
      else if ( type == E_APPLY_AURA && ed.subtype() == A_PERIODIC_DAMAGE )
      {
        snapshot_flags |= STATE_AP;
        base_td_bonus = ed.bonus( player );
      }
      else if ( type == E_SCHOOL_DAMAGE )
      {
        snapshot_flags |= STATE_AP;
        base_dd_bonus = ed.bonus( player );
      }
    }
  }
public:
  virtual double cost()
  {
    double c = base_t::cost();

    if ( c == 0 )
      return 0;

    if ( harmful && p() -> buff.omen_of_clarity -> check() )
      return 0;

    if ( p() -> buff.berserk -> check() )
      c *= 1.0 + p() -> spell.berserk_cat -> effectN( 1 ).percent();

    return c;
  }

  const cat_attack_state_t* cat_state( const action_state_t* st )
  { return debug_cast< const cat_attack_state_t* >( st ); }

  cat_attack_state_t* cat_state( action_state_t* st )
  { return debug_cast< cat_attack_state_t* >( st ); }

  virtual double bonus_ta( const action_state_t* s )
  { return base_td_bonus * ( requires_combo_points ? cat_state( s ) -> cp : 1 ); }

  virtual double bonus_da( const action_state_t* s )
  { return base_dd_bonus * ( requires_combo_points ? cat_state( s ) -> cp : 1 ); }

  virtual action_state_t* new_state()
  { return new cat_attack_state_t( this, target ); }

  virtual void snapshot_state( action_state_t* state, dmg_e rt )
  {
    base_t::snapshot_state( state, rt );
    cat_state( state ) -> cp = td( state -> target ) -> combo_points.get();
  }

  virtual void execute()
  {

    base_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( adds_combo_points )
      {
        td( target ) -> combo_points.add( adds_combo_points, &name_str );

        if ( aoe == 0 && ( p() -> specialization() == DRUID_FERAL || p() -> specialization() == DRUID_GUARDIAN ) &&
             execute_state -> result == RESULT_CRIT )
        {
          p() -> proc.primal_fury -> occur();
          td( target ) -> combo_points.add( adds_combo_points, &name_str );
        }
      }

      if ( execute_state -> result == RESULT_CRIT )
        trigger_lotp( execute_state );

      if ( cat_state( execute_state ) -> cp > 0 && requires_combo_points )
      {
        if ( player -> set_bonus.tier15_2pc_melee() &&
             p() -> rng.tier15_2pc_melee -> roll( cat_state( execute_state ) -> cp * 0.15 ) )
        {
          p() -> proc.tier15_2pc_melee -> occur();
          td( target ) -> combo_points.add( 1, &name_str );
        }
      }
    }
    else
    {
      trigger_energy_refund();
    }

    if ( harmful ) p() -> buff.stealthed -> expire();
  }

  virtual void impact( action_state_t* s )
  {
    base_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      druid_td_t& td = *this -> td( s -> target );
      if ( td.combo_points.get() > 0 && requires_combo_points && p() -> spec.predatory_swiftness -> ok() )
      {
        p() -> buff.predatory_swiftness -> trigger( 1, 1, td.combo_points.get() * 0.20 );
      }
    }
  }

  virtual void consume_resource()
  {
    int combo_points_spent = 0;
    base_t::consume_resource();

    if ( requires_combo_points && result_is_hit( execute_state -> result ) )
    {
      combo_points_spent = td( execute_state -> target ) -> combo_points.consume( &name_str );
    }

    if ( harmful && p() -> buff.omen_of_clarity -> up() )
    {
      // Treat the savings like a energy gain.
      double amount = melee_attack_t::cost();
      if ( amount > 0 )
      {
        p() -> gain.omen_of_clarity -> add( RESOURCE_ENERGY, amount );
        p() -> buff.omen_of_clarity -> expire();
      }
    }

    if ( combo_points_spent > 0 && p() -> talent.soul_of_the_forest -> ok() )
    {
      p() -> resource_gain( RESOURCE_ENERGY,
                            combo_points_spent * p() -> talent.soul_of_the_forest -> effectN( 1 ).base_value(),
                            p() -> gain.soul_of_the_forest );
    }
  }

  virtual bool ready()
  {
    if ( ! base_t::ready() )
      return false;

    if ( ! p() -> buff.cat_form -> check() )
      return false;

    if ( requires_position() != POSITION_NONE )
      if ( p() -> position() != requires_position() )
        return false;

    if ( requires_stealth() )
      if ( ! p() -> buff.stealthed -> check() )
        return false;

    if ( requires_combo_points && ! td( target ) -> combo_points.get() )
      return false;

    return true;
  }

  virtual bool   requires_stealth()
  {
    if ( p() -> buff.king_of_the_jungle -> check() )
      return false;

    return requires_stealth_;
  }

  virtual position_e requires_position()
  { return requires_position_; }

  void trigger_energy_refund()
  {
    double energy_restored = resource_consumed * 0.80;

    player -> resource_gain( RESOURCE_ENERGY, energy_restored, p() -> gain.energy_refund );
  }

  void extend_rip( action_state_t& s )
  {
    if ( result_is_hit( s.result ) )
    {
      if ( td( s.target ) -> dots.rip -> ticking &&
           td( s.target ) -> dots.rip -> added_ticks < 4 )
      {
        // In-game adds 3 seconds per extend, to model we'll add 1/2/1 ticks.
        int extra_ticks = ( td( s.target ) -> dots.rip -> added_ticks % 3 ) ? 2 : 1;
        td( s.target ) -> dots.rip -> extend_duration( extra_ticks, false, 0 );
      }
    }
  }
}; // end druid_cat_attack_t


// Cat Melee Attack =========================================================

struct cat_melee_t : public cat_attack_t
{
  cat_melee_t( druid_t* player ) :
    cat_attack_t( "cat_melee", player, spell_data_t::nil(), "" )
  {
    school = SCHOOL_PHYSICAL;
    may_glance  = true;
    background  = true;
    repeating   = true;
    trigger_gcd = timespan_t::zero();
    special = false;
  }

  virtual timespan_t execute_time()
  {
    if ( ! player -> in_combat )
      return timespan_t::from_seconds( 0.01 );

    return cat_attack_t::execute_time();
  }

  virtual double action_multiplier()
  {
    double cm = cat_attack_t::action_multiplier();

    if ( p() -> buff.cat_form -> check() )
      cm *= 1.0 + p() -> buff.cat_form -> data().effectN( 3 ).percent();

    return cm;
  }

  virtual void impact( action_state_t* state )
  {
    cat_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
      trigger_omen_of_clarity();
  }
};

// Death Coil ===============================================================

struct death_coil_t : public cat_attack_t
{
  death_coil_t( druid_t* player, const std::string& options_str ) :
    cat_attack_t( "death_coil", player,
                  ( player -> specialization() == DRUID_FERAL ) ? player -> find_spell( 122282 ) : spell_data_t::not_found() )
  {
    parse_options( NULL, options_str );

    // 122282 has generic spell info
    // 122283 has the damage dealing info
    parse_spell_data( ( *player -> dbc.spell( 122283 ) ) );

    special = true;
  }

  virtual bool ready()
  {
    if ( p() -> buff.symbiosis -> value() != DEATH_KNIGHT )
      return false;

    return cat_attack_t::ready();
  }

};

// Feral Charge (Cat) =======================================================

struct feral_charge_cat_t : public cat_attack_t
{
  // TODO: Figure out Wild Charge
  feral_charge_cat_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "feral_charge_cat", p, p -> talent.wild_charge, options_str )
  {
    may_miss   = false;
    may_dodge  = false;
    may_parry  = false;
    may_block  = false;
    may_glance = false;
    special = false;
  }

  virtual bool ready()
  {
    bool ranged = ( player -> position() == POSITION_RANGED_FRONT ||
                    player -> position() == POSITION_RANGED_BACK );

    if ( player -> in_combat && ! ranged )
    {
      return false;
    }

    return cat_attack_t::ready();
  }
};

// Ferocious Bite ===========================================================

struct ferocious_bite_t : public cat_attack_t
{
  double excess_energy;
  double max_excess_energy;

  ferocious_bite_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( p, p -> find_class_spell( "Ferocious Bite" ), options_str ),
    excess_energy( 0 ), max_excess_energy( 0 )
  {
    max_excess_energy     = 25.0;
    requires_combo_points = true;
    special = true;
  }

  double direct_power_coefficient( const action_state_t* state )
  { return 0.196 * cat_state( state ) -> cp; }

  virtual void execute()
  {
    // Berserk does affect the additional energy consumption.
    if ( p() -> buff.berserk -> check() )
      max_excess_energy *= 1.0 + p() -> spell.berserk_cat -> effectN( 1 ).percent();

    excess_energy = std::min( max_excess_energy,
                              ( p() -> resources.current[ RESOURCE_ENERGY ] - cat_attack_t::cost() ) );


    cat_attack_t::execute();

    p() -> buff.tier15_4pc_melee -> decrement();

    max_excess_energy = 25.0;
  }

  void impact( action_state_t* state )
  {
    cat_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      if ( p() -> glyph.ferocious_bite -> ok() )
      {
        double heal_pct = p() -> glyph.ferocious_bite -> effectN( 1 ).percent() *
                          ( excess_energy + cost() ) /
                          p() -> glyph.ferocious_bite -> effectN( 2 ).base_value();
        double amount = p() -> resources.max[ RESOURCE_HEALTH ] * heal_pct;
        p() -> resource_gain( RESOURCE_HEALTH, amount, p() -> gain.glyph_ferocious_bite );
      }

      double health_percentage = 25.0;
      if ( p() -> set_bonus.tier13_2pc_melee() )
        health_percentage = p() -> sets -> set( SET_T13_2PC_MELEE ) -> effectN( 2 ).base_value();

      if ( state -> target -> health_percentage() <= health_percentage )
      {
        if ( td( state -> target ) -> dots.rip -> ticking )
          td( state -> target ) -> dots.rip -> refresh_duration( 0 );
      }
    }
  }

  virtual void consume_resource()
  {
    // Ferocious Bite consumes 25+x energy, with 0 <= x <= 25.
    // Consumes the base_cost and handles Omen of Clarity
    cat_attack_t::consume_resource();

    if ( result_is_hit( execute_state -> result ) )
    {
      // Let the additional energy consumption create it's own debug log entries.
      if ( sim -> debug )
        sim -> output( "%s consumes an additional %.1f %s for %s", player -> name(),
                       excess_energy, util::resource_type_string( current_resource() ), name() );

      player -> resource_loss( current_resource(), excess_energy );
      stats -> consume_resource( current_resource(), excess_energy );
    }
  }

  double action_multiplier()
  {
    double dm = cat_attack_t::action_multiplier();

    dm *= 1.0 + excess_energy / max_excess_energy;

    return dm;
  }

  double composite_target_crit( player_t* t )
  {
    double tc = cat_attack_t::composite_target_crit( t );

    if ( t -> debuffs.bleeding -> check() )
      tc += data().effectN( 2 ).percent();

    if ( p() -> buff.tier15_4pc_melee -> up() )
      tc += 0.40;

    return tc;
  }
};

// Maim =====================================================================

struct maim_t : public cat_attack_t
{
  maim_t( druid_t* player, const std::string& options_str ) :
    cat_attack_t( player, player -> find_class_spell( "Maim" ), options_str )
  {
    requires_combo_points = true;
    special = true;
  }
};

// Mangle (Cat) =============================================================

struct mangle_cat_t : public cat_attack_t
{
  int extends_rip;

  mangle_cat_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "mangle_cat", p, p -> find_spell( 33876 ) ),
    extends_rip( 0 )
  {
    option_t options[] =
    {
      opt_bool( "extend_rip", extends_rip ),
      opt_null()
    };
    parse_options( options, options_str );

    adds_combo_points = p -> spell.combo_point -> effectN( 1 ).base_value();
    special = true;

    base_multiplier += player -> sets -> set( SET_T14_2PC_MELEE ) -> effectN( 1 ).percent();
  }

  virtual void execute()
  {
    cat_attack_t::execute();

    p() -> buff.tier15_4pc_melee -> decrement();
  }

  virtual void impact( action_state_t* state )
  {
    cat_attack_t::impact( state );

    extend_rip( *state );
  }

  double composite_target_crit( player_t* t )
  {
    double tc = cat_attack_t::composite_target_crit( t );

    if ( p() -> buff.tier15_4pc_melee -> up() )
      tc += 0.40;

    return tc;
  }

  virtual bool ready()
  {
    if ( extends_rip )
      if ( ! td( target ) -> dots.rip -> ticking ||
           ( td( target ) -> dots.rip -> added_ticks == 4 ) )
        return false;

    return cat_attack_t::ready();
  }
};

// Pounce ===================================================================

struct pounce_bleed_t : public cat_attack_t
{
  pounce_bleed_t( druid_t* player ) :
    cat_attack_t( player, player -> find_spell( 9007 ) )
  {
    background     = true;
    tick_power_mod = data().extra_coeff();
    special        = true;
  }
};

struct pounce_t : public cat_attack_t
{
  pounce_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( p, p -> find_class_spell( "Pounce" ), options_str )
  {
    execute_action = new pounce_bleed_t( p );
  }
};

// Rake =====================================================================

struct rake_t : public cat_attack_t
{
  rake_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( p, p -> find_class_spell( "Rake" ), options_str )
  {
    dot_behavior        = DOT_REFRESH;
    direct_power_mod    = data().extra_coeff();
    tick_power_mod      = data().extra_coeff();
    special             = true;

    // Set initial damage as tick zero, not as direct damage
    base_dd_min = base_dd_max = direct_power_mod = 0.0;
    tick_zero   = true;
  }
};

// Ravage ===================================================================

struct ravage_t : public cat_attack_t
{
  int extends_rip;
  double extra_crit_amount;
  double extra_crit_threshold;

  ravage_t( druid_t* player, const std::string& options_str ) :
    cat_attack_t( player, player -> find_class_spell( "Ravage" ) ),
    extends_rip( 0 ), extra_crit_amount( 0.0 ), extra_crit_threshold( 0.0 )
  {
    option_t options[] =
    {
      opt_bool( "extend_rip", extends_rip ),
      opt_null()
    };
    parse_options( options, options_str );
    requires_position_ = POSITION_BACK;
    requires_stealth_  = true;
    special            = true;

    const spell_data_t* extra_crit = player -> find_spell( 16974 );

    extra_crit_amount    = extra_crit -> effectN( 1 ).percent();
    extra_crit_threshold = extra_crit -> effectN( 2 ).base_value();
  }

  virtual position_e requires_position()
  {
    if ( p() -> buff.king_of_the_jungle -> check() )
      return POSITION_NONE;

    if ( p() -> set_bonus.pvp_4pc_melee() )
      if ( p() -> cooldown.pvp_4pc_melee -> up() )
        return POSITION_NONE;

    return cat_attack_t::requires_position();
  }

  virtual bool   requires_stealth()
  {
    if ( p() -> set_bonus.pvp_4pc_melee() )
      if ( p() -> cooldown.pvp_4pc_melee -> up() )
        return false;

    return cat_attack_t::requires_stealth();
  }

  virtual double cost()
  {
    if ( p() -> set_bonus.pvp_4pc_melee() )
      if ( p() -> cooldown.pvp_4pc_melee -> up() )
        return 0;

    return cat_attack_t::cost();
  }

  double composite_target_crit( player_t* t )
  {
    double tc = cat_attack_t::composite_target_crit( t );

    if ( target && ( target -> is_enemy() || target -> is_add() ) && ( target -> health_percentage() > extra_crit_threshold ) )
      tc += extra_crit_amount;

    if ( p() -> buff.tier15_4pc_melee -> up() )
      tc += 0.40;

    return tc;
  }

  virtual void impact( action_state_t* state )
  {
    cat_attack_t::impact( state );

    if ( p() -> set_bonus.pvp_4pc_melee() )
      if ( p() -> cooldown.pvp_4pc_melee -> up() )
        p() -> cooldown.pvp_4pc_melee -> start();

    extend_rip( *state );
  }

  virtual bool ready()
  {
    if ( extends_rip )
      if ( ! td( target ) -> dots.rip -> ticking ||
           ( td( target ) -> dots.rip -> added_ticks == 4 ) )
        return false;

    return cat_attack_t::ready();
  }

  virtual void execute()
  {
    cat_attack_t::execute();

    p() -> buff.tier15_4pc_melee -> decrement();
  }
};

// Rip ======================================================================

struct rip_t : public cat_attack_t
{
  double ap_per_point;

  rip_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( p, p -> find_class_spell( "Rip" ), options_str ),
    ap_per_point( 0.0 )
  {
    ap_per_point          = 0.0484;
    requires_combo_points = true;
    may_crit              = false;
    dot_behavior          = DOT_REFRESH;
    special               = true;

    if ( player -> set_bonus.tier14_4pc_melee() )
      num_ticks += ( int ) (  player -> sets -> set( SET_T14_4PC_MELEE ) -> effectN( 1 ).time_value() / base_tick_time );
  }

  double tick_power_coefficient( const action_state_t* state )
  { return cat_state( state ) -> cp * ap_per_point; }
};

// Savage Roar ==============================================================

struct savage_roar_t : public cat_attack_t
{
  timespan_t base_buff_duration;

  savage_roar_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( p, p -> find_class_spell( "Savage Roar" ), options_str ),
    base_buff_duration( data().duration() ) // Base duration is 12, glyphed or not, it just adds 6s per cp used.
  {
    may_miss              = false;
    harmful               = false;
    requires_combo_points = true;
    num_ticks             = 0;
    special               = false;
  }

  virtual void impact( action_state_t* state )
  {
    cat_attack_t::impact( state );

    timespan_t duration = ( player -> in_combat ? base_buff_duration : ( base_buff_duration - timespan_t::from_seconds( 3 ) ) );

    if ( p() -> buff.savage_roar -> check() )
    {
      // Savage Roar behaves like a dot with DOT_REFRESH.
      // You will not lose your current 'tick' when refreshing.
      int result = static_cast<int>( p() -> buff.savage_roar -> remains() / base_tick_time );
      timespan_t carryover = p() -> buff.savage_roar -> remains();
      carryover -= base_tick_time * result;
      duration += carryover;
    }
    duration += timespan_t::from_seconds( 6.0 ) * td( state -> target ) -> combo_points.get();


    p() -> buff.savage_roar -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
  }

  virtual bool ready()
  {
    if ( ! p() -> glyph.savagery -> ok() )
    {
      return cat_attack_t::ready();
    }
    else
    {
      requires_combo_points = false;
      bool glyphed_ready = cat_attack_t::ready();
      requires_combo_points = true;
      return glyphed_ready;
    }
  }
};

// Shattering Blow ==========================================================

struct shattering_blow_t : public cat_attack_t
{
  shattering_blow_t( druid_t* player, const std::string& options_str ) :
    cat_attack_t( "shattering_blow", player,
                  ( player -> specialization() == DRUID_FERAL ) ? player -> find_spell( 112997 ) : spell_data_t::not_found() )
  {
    parse_options( NULL, options_str );
    special = true; // FIXME: don't know if this actually consumes DoC
  }

  virtual void impact( action_state_t* s )
  {
    cat_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
      s -> target -> debuffs.shattering_throw -> trigger();
  }

  virtual bool ready()
  {
    if ( p() -> buff.symbiosis -> value() != WARRIOR )
      return false;

    if ( target -> debuffs.shattering_throw -> check() )
      return false;

    return cat_attack_t::ready();
  }
};

// Shred ====================================================================

struct shred_t : public cat_attack_t
{
  int extends_rip;

  shred_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( p, p -> find_class_spell( "Shred" ) ),
    extends_rip( 0 )
  {
    option_t options[] =
    {
      opt_bool( "extend_rip", extends_rip ),
      opt_null()
    };
    parse_options( options, options_str );

    base_multiplier += player -> sets -> set( SET_T14_2PC_MELEE ) -> effectN( 1 ).percent();

    requires_position_ = POSITION_BACK;
    special            = true;
  }

  virtual void execute()
  {
    cat_attack_t::execute();

    p() -> buff.tier15_4pc_melee -> decrement();
  }

  virtual void impact( action_state_t* state )
  {
    cat_attack_t::impact( state );

    extend_rip( *state );
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double tm = cat_attack_t::composite_target_multiplier( t );

    if ( t -> debuffs.bleeding -> up() )
      tm *= 1.0 + p() -> spell.swipe -> effectN( 2 ).percent();

    return tm;
  }

  double composite_target_crit( player_t* t )
  {
    double tc = cat_attack_t::composite_target_crit( t );

    if ( p() -> buff.tier15_4pc_melee -> up() )
      tc += 0.40;

    return tc;
  }

  virtual bool ready()
  {
    if ( extends_rip )
      if ( ! td( target ) -> dots.rip -> ticking ||
           ( td( target ) -> dots.rip -> added_ticks == 4 ) )
        return false;

    return cat_attack_t::ready();
  }
};

// Skull Bash (Cat) =========================================================

struct skull_bash_cat_t : public cat_attack_t
{
  skull_bash_cat_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( p, p -> find_specialization_spell( "Skull Bash" ), options_str )
  {
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;

    cooldown -> duration += p -> glyph.skull_bash -> effectN( 1 ).time_value();
    special = false;
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return cat_attack_t::ready();
  }
};

// Swipe (Cat) ==============================================================

struct swipe_cat_t : public cat_attack_t
{
  swipe_cat_t( druid_t* player, const std::string& options_str ) :
    cat_attack_t( "swipe_cat", player, player -> find_class_spell( "Swipe" ) -> ok() ? player -> find_spell( 62078 ) : spell_data_t::not_found(), options_str )
  {
    aoe     = -1;
    special = true;
  }
  
  virtual void execute()
  {
    cat_attack_t::execute();

    p() -> buff.tier15_4pc_melee -> decrement();
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double tm = cat_attack_t::composite_target_multiplier( t );

    if ( t -> debuffs.bleeding -> up() )
      tm *= 1.0 + data().effectN( 2 ).percent();

    return tm;
  }

  double composite_target_crit( player_t* t )
  {
    double tc = cat_attack_t::composite_target_crit( t );

    if ( p() -> buff.tier15_4pc_melee -> up() )
      tc += 0.40;

    return tc;
  }
};

// Thrash (Cat) ===================================================================

struct thrash_cat_t : public cat_attack_t
{
  thrash_cat_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "thrash_cat", p, p -> find_spell( 106830 ), options_str )
  {
    aoe               = -1;
    direct_power_mod  = data().effectN( 3 ).base_value() / 1000.0;
    tick_power_mod    = data().effectN( 4 ).base_value() / 1000.0;

    weapon            = &( player -> main_hand_weapon );
    weapon_multiplier = 0;
    dot_behavior      = DOT_REFRESH;
    special           = true;
  }

  virtual void impact( action_state_t* state )
  {
    cat_attack_t::impact( state );

    if ( result_is_hit( state -> result ) && ! sim -> overrides.weakened_blows )
      state -> target -> debuffs.weakened_blows -> trigger();
  }

  // Treat direct damage as "bleed"
  double composite_da_multiplier()
  {
    double m = cat_attack_t::composite_da_multiplier();

    if ( p() -> buff.cat_form -> up() && p() -> mastery.razor_claws -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  // Treat direct damage as "bleed"
  virtual double target_armor( player_t* )
  { return 0.0; }

  virtual bool ready()
  {
    if ( ! p() -> buff.cat_form -> check() )
      return false;

    return cat_attack_t::ready();
  }
};

// Tiger's Fury ==============================================================

struct tigers_fury_t : public cat_attack_t
{
  tigers_fury_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( p, p -> find_class_spell( "Tiger's Fury" ), options_str )
  {
    harmful = false;
    special = false;
  }

  virtual void execute()
  {
    cat_attack_t::execute();

    p() -> buff.tigers_fury -> trigger();

    p() -> resource_gain( RESOURCE_ENERGY,
                          data().effectN( 2 ).resource( RESOURCE_ENERGY ),
                          p() -> gain.tigers_fury );

    if ( p() -> set_bonus.tier13_4pc_melee() )
      p() -> buff.omen_of_clarity -> trigger( 1, buff_t::DEFAULT_VALUE(), 1 );
    if ( p() -> set_bonus.tier15_4pc_melee() )
      p() -> buff.tier15_4pc_melee -> trigger( 3 );
  }

  virtual bool ready()
  {
    if ( p() -> buff.berserk -> check() )
      return false;

    return cat_attack_t::ready();
  }
};

} // end namespace cat_attacks

namespace bear_attacks {

// ==========================================================================
// Druid Bear Attack
// ==========================================================================

struct bear_attack_t : public druid_attack_t<melee_attack_t>
{
  bear_attack_t( const std::string& token, druid_t* p,
                 const spell_data_t* s = spell_data_t::nil(),
                 const std::string& options = std::string() ) :
    base_t( token, p, s )
  {
    parse_options( 0, options );
  }

  bear_attack_t( druid_t* p, const spell_data_t* s = spell_data_t::nil(),
                 const std::string& options = std::string() ) :
    base_t( "", p, s )
  {
    parse_options( 0, options );
  }

  virtual void impact( action_state_t* s )
  {
    base_t::impact( s );

    if ( s -> result == RESULT_CRIT )
      trigger_lotp( s );
  }

  void trigger_rage_gain()
  {
    p() -> resource_gain( RESOURCE_RAGE, 16.0, p() -> gain.bear_melee );
  }
}; // end druid_bear_attack_t

// Bear Melee Attack ========================================================

struct bear_melee_t : public bear_attack_t
{
  bear_melee_t( druid_t* player ) :
    bear_attack_t( "bear_melee", player )
  {
    may_glance  = true;
    background  = true;
    repeating   = true;
    trigger_gcd = timespan_t::zero();
    special     = false;
  }

  virtual timespan_t execute_time()
  {
    if ( ! player -> in_combat )
      return timespan_t::from_seconds( 0.01 );

    return bear_attack_t::execute_time();
  }

  virtual void impact( action_state_t* state )
  {
    bear_attack_t::impact( state );

    if ( state -> result != RESULT_MISS )
      trigger_rage_gain();

    if ( ( p() -> specialization() == DRUID_FERAL || p() -> specialization() == DRUID_GUARDIAN ) &&
         state -> result == RESULT_CRIT )
    {
      p() -> resource_gain( RESOURCE_RAGE,
                            p() -> spell.primal_fury -> effectN( 1 ).resource( RESOURCE_RAGE ),
                            p() -> gain.primal_fury );
      p() -> proc.primal_fury -> occur();
    }
  }
};

// Feral Charge (Bear) ======================================================

struct feral_charge_bear_t : public bear_attack_t
{
  // TODO: Get beta, figure it out
  feral_charge_bear_t( druid_t* p, const std::string& options_str ) :
    bear_attack_t( p, p -> talent.wild_charge, options_str )
  {
    may_miss   = false;
    may_dodge  = false;
    may_parry  = false;
    may_block  = false;
    may_glance = false;
    special    = false;
  }

  virtual bool ready()
  {
    bool ranged = ( player -> position() == POSITION_RANGED_FRONT ||
                    player -> position() == POSITION_RANGED_BACK );

    if ( player -> in_combat && ! ranged )
      return false;

    return bear_attack_t::ready();
  }
};

// Lacerate =================================================================

struct lacerate_t : public bear_attack_t
{
  lacerate_t( druid_t* p, const std::string& options_str ) :
    bear_attack_t( p, p -> find_class_spell( "Lacerate" ), options_str )
  {
    direct_power_mod     = 0.616;
    tick_power_mod       = 0.0512;
    dot_behavior         = DOT_REFRESH;
    special              = true;
  }

  virtual void execute()
  {
    bear_attack_t::execute();

    if ( p() -> buff.son_of_ursoc -> check() || p() -> buff.berserk -> check() )
      cooldown -> reset( false );
  }

  virtual void impact( action_state_t* state )
  {
    if ( result_is_hit( state -> result ) )
    {
      if ( td( state -> target ) -> lacerate_stack < 3 )
        td( state -> target ) -> lacerate_stack++;
      p() -> buff.lacerate -> trigger();
    }

    bear_attack_t::impact( state );
  }

  virtual double action_ta_multiplier()
  {
    double tm = bear_attack_t::action_ta_multiplier();

    tm *= td( target ) -> lacerate_stack;

    return tm;
  }

  virtual void tick( dot_t* d )
  {
    bear_attack_t::tick( d );

    if ( p() -> rng.mangle -> roll( p() -> spell.mangle -> effectN( 1 ).percent() ) )
      p() -> cooldown.mangle_bear -> reset( true );
  }

  virtual void last_tick( dot_t* d )
  {
    bear_attack_t::last_tick( d );
    
    p() -> buff.lacerate -> expire();
    td( target ) -> lacerate_stack = 0;
  }

  virtual bool ready()
  {
    if ( ! p() -> buff.bear_form -> check() )
      return false;

    return bear_attack_t::ready();
  }
};

// Mangle (Bear) ============================================================

struct mangle_bear_t : public bear_attack_t
{
  mangle_bear_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "mangle_bear", player, player -> find_spell( 33878 ), options_str )
  {}

  virtual void execute()
  {
    if ( p() -> buff.berserk -> up() )
      aoe = p() -> spell.berserk_bear -> effectN( 1 ).base_value();

    bear_attack_t::execute();

    aoe     = 0;
    special = true;
    if ( p() -> buff.berserk -> check() || p() -> buff.son_of_ursoc -> check() )
      cooldown -> reset( false );

    p() -> resource_gain( RESOURCE_RAGE,
                          data().effectN( 3 ).resource( RESOURCE_RAGE ),
                          p() -> gain.mangle );
    p() -> resource_gain( RESOURCE_RAGE,
                          p() -> talent.soul_of_the_forest -> effectN( 3 ).base_value(),
                          p() -> gain.soul_of_the_forest );
  }

  virtual void impact( action_state_t* state )
  {
    bear_attack_t::impact( state );

    if ( state -> result == RESULT_CRIT )
    {
      p() -> resource_gain( RESOURCE_RAGE,
                            p() -> spell.primal_fury -> effectN( 1 ).resource( RESOURCE_RAGE ),
                            p() -> gain.primal_fury );
      p() -> proc.primal_fury -> occur();
    }
  }

  virtual bool ready()
  {
    if ( ! p() -> buff.bear_form -> check() )
      return false;

    return bear_attack_t::ready();
  }
};

// Maul =====================================================================

struct maul_t : public bear_attack_t
{
  maul_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( player, player -> find_class_spell( "Maul" ), options_str )
  {
    weapon = &( player -> main_hand_weapon );

    aoe = player -> glyph.maul -> effectN( 1 ).base_value();
    base_add_multiplier = player -> glyph.maul -> effectN( 3 ).percent();
    use_off_gcd = true;
    special     = true;
  }

  virtual void execute()
  {
    bear_attack_t::execute();

    if ( p() -> buff.son_of_ursoc -> check() )
      cooldown -> reset( false );
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double tm = bear_attack_t::composite_target_multiplier( t );

    if ( t -> debuffs.bleeding -> up() )
      tm *= 1.0 + p() -> spell.swipe -> effectN( 2 ).percent();

    return tm;
  }

  virtual bool ready()
  {
    if ( ! p() -> buff.bear_form -> check() )
      return false;

    return bear_attack_t::ready();
  }
};

// Skull Bash (Bear) ========================================================

struct skull_bash_bear_t : public bear_attack_t
{
  skull_bash_bear_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( player, player -> find_class_spell( "Skull Bash" ), options_str )
  {
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
    special = false;

    cooldown -> duration += player -> glyph.skull_bash -> effectN( 1 ).time_value();
  }

  virtual bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;

    return bear_attack_t::ready();
  }
};

// Swipe (Bear) =============================================================

struct swipe_bear_t : public bear_attack_t
{
  swipe_bear_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( player, player -> find_class_spell( "Swipe" ) -> ok() ? player -> find_spell( 779 ) : spell_data_t::not_found(), options_str )
  {
    aoe               = -1;
    direct_power_mod  = data().extra_coeff();
    weapon            = &( player -> main_hand_weapon );
    weapon_multiplier = 0;
    special           = true;
  }

  virtual void execute()
  {
    bear_attack_t::execute();

    if ( p() -> buff.son_of_ursoc -> check() )
      cooldown -> reset( false );
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double tm = bear_attack_t::composite_target_multiplier( t );

    if ( t -> debuffs.bleeding -> up() )
      tm *= 1.0 + data().effectN( 2 ).percent();

    return tm;
  }

  virtual bool ready()
  {
    if ( ! p() -> buff.bear_form -> check() )
      return false;

    return bear_attack_t::ready();
  }
};

// Thrash (Bear) ===================================================================

struct thrash_bear_t : public bear_attack_t
{
  thrash_bear_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "thrash_bear", player, player -> find_spell( 77758 ), options_str )
  {
    aoe               = -1;
    direct_power_mod  = data().effectN( 3 ).base_value() / 1000.0;
    tick_power_mod    = data().effectN( 4 ).base_value() / 1000.0;

    weapon            = &( player -> main_hand_weapon );
    weapon_multiplier = 0;
    dot_behavior      = DOT_REFRESH;
    special           = true;
  }

  virtual void execute()
  {
    bear_attack_t::execute();

    if ( p() -> buff.son_of_ursoc -> check() )
      cooldown -> reset( false );
  }

  virtual void impact( action_state_t* state )
  {
    bear_attack_t::impact( state );

    if ( result_is_hit( state -> result ) && ! sim -> overrides.weakened_blows )
      state -> target -> debuffs.weakened_blows -> trigger();
  }

  virtual void tick( dot_t* d )
  {
    bear_attack_t::tick( d );

    if ( p() -> rng.mangle -> roll( p() -> spell.mangle -> effectN( 1 ).percent() ) )
      p() -> cooldown.mangle_bear -> reset( true );
  }

  // Treat direct damage as "bleed"
  virtual double target_armor( player_t* )
  { return 0.0; }

  virtual bool ready()
  {
    if ( ! p() -> buff.bear_form -> check() )
      return false;

    return bear_attack_t::ready();
  }
};

// Savage Defense ==========================================================

struct savage_defense_t : public bear_attack_t
{
  savage_defense_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "savage_defense", player, player -> find_class_spell( "Savage Defense" ), options_str )
  {
    parse_options( NULL, options_str );
    harmful = false;
    special = false;
    cooldown -> duration = timespan_t::from_seconds( 9.0 );
    cooldown -> charges = 3;
  }

  virtual void execute()
  {
    bear_attack_t::execute();

    if ( p() -> buff.savage_defense -> up() )
    {
      p() -> buff.savage_defense -> extend_duration( p(), timespan_t::from_seconds( 6.0 ) );
    }
    else
    {
      p() -> buff.savage_defense -> trigger();
    }
  }
};

} // end namespace bear_attacks

// Druid "Spell" Base for druid_spell_t, druid_heal_t ( and potentially druid_absorb_t )
template <class Base>
struct druid_spell_base_t : public druid_action_t< Base >
{
private:
  typedef druid_action_t< Base > ab;
public:
  typedef druid_spell_base_t base_t;

  bool consume_ooc;

  druid_spell_base_t( const std::string& n, druid_t* player,
                      const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ),
    consume_ooc( false )
  {
  }

  virtual void consume_resource()
  {
    ab::consume_resource();
    druid_t& p = *this -> p();

    if ( consume_ooc && p.buff.omen_of_clarity -> up() && this -> execute_time() != timespan_t::zero() )
    {
      // Treat the savings like a mana gain.
      double amount = ab::cost();
      if ( amount > 0 )
      {
        p.gain.omen_of_clarity -> add( RESOURCE_MANA, amount );
        p.buff.omen_of_clarity -> expire();
      }
    }
  }

  virtual double cost()
  {
    druid_t& p = *this -> p();

    if ( consume_ooc && p.buff.omen_of_clarity -> check() && this -> execute_time() != timespan_t::zero() )
      return 0;

    return std::max( 0.0, ab::cost() * ( 1.0 + cost_reduction() ) );
  }

  virtual double cost_reduction()
  { return 0.0; }

  virtual double composite_haste()
  {
    double h = ab::composite_haste();
    druid_t& p = *this -> p();

    h *= 1.0 / ( 1.0 +  p.buff.natures_grace -> data().effectN( 1 ).percent() );

    return h;
  }
};

namespace heals {

// ==========================================================================
// Druid Heal
// ==========================================================================

struct druid_heal_t : public druid_spell_base_t<heal_t>
{
  action_t* living_seed;

  druid_heal_t( const std::string& token, druid_t* p,
                const spell_data_t* s = spell_data_t::nil(),
                const std::string& options = std::string() ) :
    base_t( token, p, s ),
    living_seed( nullptr )
  {
    parse_options( 0, options );

    dot_behavior      = DOT_REFRESH;
    may_miss          = false;
    weapon_multiplier = 0;
  }

  druid_heal_t( druid_t* p, const spell_data_t* s = spell_data_t::nil(),
                const std::string& options = std::string() ) :
    base_t( "", p, s ),
    living_seed( nullptr )
  {
    parse_options( 0, options );

    dot_behavior      = DOT_REFRESH;
    may_miss          = false;
    weapon_multiplier = 0;
  }

protected:
  void init_living_seed();

public:
  virtual double cost()
  {
    if ( p() -> buff.heart_of_the_wild -> heals_are_free() )
      return 0;

    return base_t::cost();
  }

  virtual void execute()
  {
    base_t::execute();

    if ( base_execute_time > timespan_t::zero() )
    {
      p() -> buff.soul_of_the_forest -> expire();

      if ( p() -> buff.natures_swiftness -> check() )
      {
        p() -> buff.natures_swiftness -> expire();
        // NS cd starts when the buff is consumed.
        p() -> cooldown.natures_swiftness -> start();
      }
    }

    if ( base_dd_min > 0 && ! background )
      p() -> buff.harmony -> trigger( 1, p() -> mastery.harmony -> ok() ? p() -> cache.mastery_value() : 0.0);
  }

  virtual timespan_t execute_time()
  {
    if ( p() -> buff.natures_swiftness -> check() )
      return timespan_t::zero();

    return base_t::execute_time();
  }

  virtual double composite_haste()
  {
    double h = base_t::composite_haste();

    h *= 1.0 / ( 1.0 + p() -> buff.soul_of_the_forest -> value() );

    return h;
  }

  virtual double action_da_multiplier()
  {
    double adm = base_t::action_da_multiplier();

    if ( p() -> buff.tree_of_life -> up() )
      adm += p() -> buff.tree_of_life -> data().effectN( 1 ).percent();

    if ( p() -> buff.natures_swiftness -> check() && base_execute_time > timespan_t::zero() )
      adm += p() -> talent.natures_swiftness -> effectN( 2 ).percent();

    if ( p() -> mastery.harmony -> ok() )
    adm += p() -> cache.mastery_value();

    return adm;
  }

  virtual double action_ta_multiplier()
  {
    double adm = base_t::action_ta_multiplier();

    if ( p() -> buff.tree_of_life -> up() )
      adm += p() -> buff.tree_of_life -> data().effectN( 2 ).percent();

    if ( p() -> buff.natures_swiftness -> check() && base_execute_time > timespan_t::zero() )
      adm += p() -> talent.natures_swiftness -> effectN( 3 ).percent();

    adm += p() -> buff.harmony -> value();

    return adm;
  }

  void trigger_revitalize()
  {
    if ( p() -> cooldown.revitalize -> down() ) return;

    if ( p() -> rng.revitalize -> roll( p() -> spec.revitalize -> proc_chance() ) )
    {
      p() -> proc.revitalize -> occur();
      p() -> resource_gain( RESOURCE_MANA,
                            p() -> resources.max[ RESOURCE_MANA ] * p() -> spec.revitalize -> effectN( 1 ).percent(),
                            p() -> gain.revitalize );

      p() -> cooldown.revitalize -> start( timespan_t::from_seconds( 12.0 ) );
    }
  }

  void trigger_lifebloom_refresh( action_state_t* s )
  {
    druid_td_t& td = *this -> td( s -> target );

    if ( td.dots.lifebloom -> ticking )
    {
      td.dots.lifebloom -> refresh_duration();

      if ( td.buffs.lifebloom -> check() )
        td.buffs.lifebloom -> refresh();
    }
  }

  void trigger_living_seed( action_state_t* s )
  {
    // Technically this should be a buff on the target, then bloom when they're attacked
    // For simplicity we're going to assume it always heals the target
    if ( living_seed )
    {
      living_seed -> base_dd_min = s -> result_amount;
      living_seed -> base_dd_max = s -> result_amount;
      living_seed -> execute();
    }
  }
}; // end druid_heal_t

struct living_seed_t : public druid_heal_t
{
  living_seed_t( druid_t* player ) :
    druid_heal_t( player, player -> find_specialization_spell( "Living Seed" ) )
  {
    background = true;
    may_crit   = false;
    proc       = true;
    school     = SCHOOL_NATURE;
  }

  double composite_da_multiplier()
  {
    return data().effectN( 1 ).percent();
  }
};

void druid_heal_t::init_living_seed()
{
  if ( p() -> specialization() == DRUID_RESTORATION )
    living_seed = new living_seed_t( p() );
}

// Healing Touch ============================================================

struct healing_touch_t : public druid_heal_t
{
  healing_touch_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( p, p -> find_class_spell( "Healing Touch" ), options_str )
  {
    consume_ooc = true;
    harmful = false;

    init_living_seed();
  }

  virtual double cost()
  {
    if ( p() -> buff.predatory_swiftness -> check() )
      return 0;

    if ( p() -> buff.natures_swiftness -> check() )
      return 0;

    return druid_heal_t::cost();
  }


  virtual timespan_t execute_time()
  {
    if ( p() -> buff.predatory_swiftness -> check() )
      return timespan_t::zero();

    return druid_heal_t::execute_time();
  }

  virtual void impact( action_state_t* state )
  {
    druid_heal_t::impact( state );

    if ( ! p() -> glyph.blooming -> ok() )
      trigger_lifebloom_refresh( state );

    if ( state -> result == RESULT_CRIT )
      trigger_living_seed( state );

    p() -> cooldown.swiftmend -> ready -= timespan_t::from_seconds( p() -> glyph.healing_touch -> effectN( 1 ).base_value() );
  }

  virtual void execute()
  {
    druid_heal_t::execute();

    p() -> buff.predatory_swiftness -> expire();
    p() -> buff.dream_of_cenarius_damage -> trigger( 2 );
  }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    druid_heal_t::schedule_execute( state );

    if ( ! p() -> buff.natures_swiftness -> up() &&
         ! p() -> buff.predatory_swiftness -> up() )
    {
      if ( ! p() -> glyph.moonbeast -> ok() )
        p() -> buff.moonkin_form -> expire();

      p() -> buff.cat_form         -> expire();
      p() -> buff.bear_form        -> expire();
    }
  }

  virtual timespan_t gcd()
  {
    druid_t& p = *this -> p();
    if ( p.buff.cat_form -> check() )
      if ( timespan_t::from_seconds( 1.0 ) < druid_heal_t::gcd() )
        return timespan_t::from_seconds( 1.0 );

    return druid_heal_t::gcd();
  }
};

// Lifebloom ================================================================

struct lifebloom_bloom_t : public druid_heal_t
{
  lifebloom_bloom_t( druid_t* p ) :
    druid_heal_t( "lifebloom_bloom", p, p -> find_class_spell( "Lifebloom" ) )
  {
    background       = true;
    dual             = true;
    num_ticks        = 0;
    base_td          = 0;
    tick_power_mod   = 0;
    base_dd_min      = data().effectN( 2 ).min( p );
    base_dd_max      = data().effectN( 2 ).max( p );
    direct_power_mod = data().effectN( 2 ).coeff();
  }

  virtual double composite_target_multiplier( player_t* target )
  {
    double ctm = druid_heal_t::composite_target_multiplier( target );

    ctm *= 1.0 + td( target ) -> buffs.lifebloom -> check();

    return ctm;
  }

  virtual double composite_da_multiplier()
  {
    double cdm = druid_heal_t::composite_da_multiplier();

    cdm *= 1.0 + p() -> glyph.blooming -> effectN( 1 ).percent();

    return cdm;
  }
};

struct lifebloom_t : public druid_heal_t
{
  lifebloom_bloom_t* bloom;

  lifebloom_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( p, p -> find_class_spell( "Lifebloom" ), options_str ),
    bloom( new lifebloom_bloom_t( p ) )
  {
    may_crit   = false;

    // TODO: this can be only cast on one target, unless Tree of Life is up
  }

  virtual double composite_target_multiplier( player_t* target )
  {
    double ctm = druid_heal_t::composite_target_multiplier( target );

    ctm *= 1.0 + td( target ) -> buffs.lifebloom -> check();

    return ctm;
  }

  virtual void impact( action_state_t* state )
  {
    // Cancel Dot/td-buff on all targets other than the one we impact on
    for ( size_t i = 0; i < sim -> actor_list.size(); ++i )
    {
      player_t* t = sim -> actor_list[ i ];
      if ( state -> target == t )
        continue;
      get_dot( t ) -> cancel();
      td( t ) -> buffs.lifebloom -> expire();
    }

    druid_heal_t::impact( state );

    td( state -> target ) -> buffs.lifebloom -> trigger();
  }

  virtual void last_tick( dot_t* d )
  {
    bloom -> execute();
    td( d -> state -> target ) -> buffs.lifebloom -> expire();

    druid_heal_t::last_tick( d );
  }

  virtual void tick( dot_t* d )
  {
    druid_heal_t::tick( d );

    p() -> buff.omen_of_clarity -> trigger();

    trigger_revitalize();
  }
};

// Nourish ==================================================================

struct nourish_t : public druid_heal_t
{
  nourish_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( p, p -> find_class_spell( "Nourish" ), options_str )
  {
    init_living_seed();
  }

  virtual void impact( action_state_t* state )
  {
    druid_heal_t::impact( state );

    if ( ! p() -> glyph.blooming -> ok() )
      trigger_lifebloom_refresh( state );

    if ( state -> result == RESULT_CRIT )
      trigger_living_seed( state );
  }

  virtual double composite_target_multiplier( player_t* t )
  {
    double ctm = druid_heal_t::composite_target_multiplier( t );

    if ( td( t ) -> hot_ticking() )
      ctm *= 1.20;

    return ctm;
  }

  virtual void execute()
  {
    druid_heal_t::execute();
    p() -> buff.dream_of_cenarius_damage -> trigger( 2 );
  }
};

// Regrowth =================================================================

struct regrowth_t : public druid_heal_t
{
  regrowth_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( p, p -> find_class_spell( "Regrowth" ), options_str )
  {
    base_crit   += 0.6;
    consume_ooc  = true;

    if ( p -> glyph.regrowth -> ok() )
    {
      base_crit += p -> glyph.regrowth -> effectN( 1 ).percent();
      base_td    = 0;
      num_ticks  = 0;
    }

    init_living_seed();
  }

  virtual void impact( action_state_t* state )
  {
    druid_heal_t::impact( state );

    if ( ! p() -> glyph.blooming -> ok() )
      trigger_lifebloom_refresh( state );

    if ( state -> result == RESULT_CRIT )
      trigger_living_seed( state );
  }

  virtual void tick( dot_t* d )
  {
    druid_heal_t::tick( d );

    if ( d -> state -> target -> health_percentage() <= p() -> spell.regrowth -> effectN( 1 ).percent() &&
         td( d -> state -> target ) -> dots.regrowth -> ticking )
    {
      td( d -> state -> target )-> dots.regrowth -> refresh_duration();
    }
  }

  virtual void execute()
  {
    druid_heal_t::execute();

    p() -> buff.dream_of_cenarius_damage -> trigger( 2 );
  }

  virtual timespan_t execute_time()
  {
    if ( p() -> buff.tree_of_life -> check() )
      return timespan_t::zero();

    return druid_heal_t::execute_time();
  }
};

// Rejuvenation =============================================================

struct rejuvenation_t : public druid_heal_t
{
  rejuvenation_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( p, p -> find_class_spell( "Rejuvenation" ), options_str )
  {
    tick_zero = true;
  }

  virtual void tick( dot_t* d )
  {
    druid_heal_t::tick( d );

    trigger_revitalize();
  }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    druid_heal_t::schedule_execute( state );

    p() -> buff.cat_form  -> expire();
    if ( ! p() -> buff.heart_of_the_wild -> check() )
      p() -> buff.bear_form -> expire();
  }
};

// Swiftmend ================================================================

// TODO: in game, you can swiftmend other druids' hots, which is not supported here
struct swiftmend_t : public druid_heal_t
{
  struct swiftmend_aoe_heal_t : public druid_heal_t
  {
    swiftmend_aoe_heal_t( druid_t* p, const spell_data_t* s ) :
      druid_heal_t( "swiftmend_aoe", p, s )
    {
      aoe            = 3;
      background     = true;
      base_tick_time = timespan_t::from_seconds( 1.0 );
      hasted_ticks   = true;
      may_crit       = false;
      num_ticks      = ( int ) p -> spell.swiftmend -> duration().total_seconds();
      proc           = true;
      tick_may_crit  = false;
    }
  };

  swiftmend_aoe_heal_t* aoe_heal;

  swiftmend_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( p, p -> find_class_spell( "Swiftmend" ), options_str ),
    aoe_heal( new swiftmend_aoe_heal_t( p, data().effectN( 2 ).trigger() ) )
  {
    consume_ooc = true;

    init_living_seed();
  }

  virtual void impact( action_state_t* state )
  {
    druid_heal_t::impact( state );

    if ( state -> result == RESULT_CRIT )
      trigger_living_seed( state );

    if ( p() -> talent.soul_of_the_forest -> ok() )
      p() -> buff.soul_of_the_forest -> trigger();

    aoe_heal -> execute();
  }

  virtual bool ready()
  {
    player_t* t = ( execute_state ) ? execute_state -> target : target;

    // Note: with the glyph you can use other people's regrowth/rejuv
    if ( ! ( td( t ) -> dots.regrowth -> ticking ||
             td( t ) -> dots.rejuvenation -> ticking ) )
      return false;

    return druid_heal_t::ready();
  }
};

// Tranquility ==============================================================

struct tranquility_t : public druid_heal_t
{
  tranquility_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( p, p -> find_class_spell( "Tranquility" ), options_str )
  {
    aoe               = data().effectN( 3 ).base_value(); // Heals 5 targets
    base_execute_time = data().duration();
    channeled         = true;

    // Healing is in spell effect 1
    parse_spell_data( ( *player -> dbc.spell( data().effectN( 1 ).trigger_spell_id() ) ) );

    // FIXME: The hot should stack
  }
};

// Wild Growth ==============================================================

struct wild_growth_t : public druid_heal_t
{
  wild_growth_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( p, p -> find_class_spell( "Wild Growth" ), options_str )
  {
    aoe = data().effectN( 3 ).base_value() + p -> glyph.wild_growth -> effectN( 1 ).base_value();
    cooldown -> duration = data().cooldown() + p -> glyph.wild_growth -> effectN( 2 ).time_value();
  }

  virtual void execute()
  {
    int save = aoe;
    if ( p() -> buff.tree_of_life -> check() )
      aoe += 2;

    druid_heal_t::execute();

    // Reset AoE
    aoe = save;
  }
};

// Frenzied Regeneration ====================================================

struct frenzied_regeneration_t : public druid_heal_t
{
  double maximum_rage_cost;

  frenzied_regeneration_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( p, p -> find_class_spell( "Frenzied Regeneration" ), options_str ),
    maximum_rage_cost( 0.0 )
  {
    base_dd_min = base_dd_max = direct_power_mod = 0.0;

    harmful = false;
    special = false;

    if ( p -> glyph.frenzied_regeneration -> ok() )
      base_costs[ RESOURCE_RAGE ] = p -> glyph.frenzied_regeneration -> effectN( 3 ).resource( RESOURCE_RAGE );
    else
      base_costs[ RESOURCE_RAGE ] = 0;

    maximum_rage_cost = data().effectN( 1 ).base_value();
  }

  virtual double cost()
  {
    if ( ! p() -> glyph.frenzied_regeneration -> ok() )
      base_costs[ RESOURCE_RAGE ] = std::min( p() -> resources.current[ RESOURCE_RAGE ],
                                              maximum_rage_cost );

    return druid_heal_t::cost();
  }

  virtual void execute()
  {
    druid_heal_t::execute();

    if ( p() -> glyph.frenzied_regeneration -> ok() )
      p() -> buff.frenzied_regeneration -> trigger();
    else
    {
      // max(2.2*(AP - 2*Agi), 2.5*Sta)
      double ap              = p() -> composite_melee_attack_power() * p() -> composite_attack_power_multiplier();
      double agility         = p() -> composite_attribute( ATTR_AGILITY ) * p() -> composite_attribute_multiplier( ATTR_AGILITY );
      double stamina         = p() -> composite_attribute( ATTR_STAMINA ) * p() -> composite_attribute_multiplier( ATTR_STAMINA );
      double health_restored = std::max( ( ap - 2 * agility ) * data().effectN( 2 ).percent(), stamina * data().effectN( 3 ).percent() )
                               * ( resource_consumed / maximum_rage_cost )
                               * ( 1.0 + p() -> buff.tier15_2pc_tank -> stack() * p() -> buff.tier15_2pc_tank -> data().effectN( 1 ).percent() );
      p() -> resource_gain( RESOURCE_HEALTH,
                            health_restored,
                            p() -> gain.frenzied_regeneration );

      p() -> buff.tier15_2pc_tank -> expire();
    }
  }
};

} // end namespace heals

namespace spells {

// ==========================================================================
// Druid Spells
// ==========================================================================

struct druid_spell_t : public druid_spell_base_t<spell_t>
{
  druid_spell_t( const std::string& token, druid_t* p,
                 const spell_data_t* s = spell_data_t::nil(),
                 const std::string& options = std::string() ) :
    base_t( token, p, s )
  {
    parse_options( 0, options );
  }

  druid_spell_t( druid_t* p, const spell_data_t* s = spell_data_t::nil(),
                 const std::string& options = std::string() ) :
    base_t( "", p, s )
  {
    parse_options( 0, options );
  }

  virtual void init()
  {
    base_t::init();
    consume_ooc = harmful;
  }

  virtual double cost()
  {
    if ( harmful && p() -> buff.heart_of_the_wild -> damage_spells_are_free() )
      return 0;

    return base_t::cost();
  }

  void trigger_eclipse_energy_gain( int gain )
  {
    if ( gain == 0 )
      return;

    if ( p() -> buff.celestial_alignment -> check() )
      return;

    // Gain will only happen if it is either aligned with the bar direction or
    // the bar direction has not been set yet.
    if ( p() -> eclipse_bar_direction == -1 && gain > 0 )
    {
      p() -> proc.unaligned_eclipse_gain -> occur();
      return;
    }
    else if ( p() -> eclipse_bar_direction ==  1 && gain < 0 )
    {
      p() -> proc.unaligned_eclipse_gain -> occur();
      return;
    }

    int old_eclipse_bar_value = p() -> eclipse_bar_value;
    p() -> eclipse_bar_value += gain;

    // When a Lunar Eclipse ends you gain 20 Solar Energy and when a Solar Eclipse
    // ends you gain 20 Lunar Energy.
    bool soul_of_the_forest = false;
    if ( p() -> eclipse_bar_value <= 0 )
    {
      if ( p() -> buff.eclipse_solar -> check() )
      {
        p() -> buff.eclipse_solar -> expire();
        soul_of_the_forest = true;
      }
      if ( p() -> eclipse_bar_value < p() -> spec.eclipse -> effectN( 2 ).base_value() )
        p() -> eclipse_bar_value = p() -> spec.eclipse -> effectN( 2 ).base_value();
    }

    if ( p() -> eclipse_bar_value >= 0 )
    {
      if ( p() -> buff.eclipse_lunar -> check() )
      {
        p() -> buff.eclipse_lunar -> expire();
        soul_of_the_forest = true;
      }
      if ( p() -> eclipse_bar_value > p() -> spec.eclipse -> effectN( 1 ).base_value() )
        p() -> eclipse_bar_value = p() -> spec.eclipse -> effectN( 1 ).base_value();
    }

    int actual_gain = p() -> eclipse_bar_value - old_eclipse_bar_value;
    if ( p() -> sim -> log )
    {
      p() -> sim -> output( "%s gains %d (%d) %s from %s (%d)",
                            p() -> name(), actual_gain, gain,
                            "Eclipse", name(),
                            p() -> eclipse_bar_value );
    }

    // Eclipse proc:
    // Procs when you reach 100 and only then, you have to have an
    // actual gain of eclipse energy (bar value)
    if ( actual_gain != 0 )
    {
      if ( p() -> eclipse_bar_value ==
           p() -> spec.eclipse -> effectN( 1 ).base_value() )
      {
        if ( p() -> buff.eclipse_solar -> trigger() )
        {
          trigger_eclipse_proc();
          // Solar proc => bar direction changes to -1 (towards Lunar)
          p() -> eclipse_bar_direction = -1;
        }
      }
      else if ( p() -> eclipse_bar_value ==
                p() -> spec.eclipse -> effectN( 2 ).base_value() )
      {
        if ( p() -> buff.eclipse_lunar -> trigger() )
        {
          trigger_eclipse_proc();
          p() -> cooldown.starfall -> reset( false );
          // Lunar proc => bar direction changes to +1 (towards Solar)
          p() -> eclipse_bar_direction = 1;
        }
      }
    }
    if ( soul_of_the_forest )
      p() -> trigger_soul_of_the_forest();
  }

  void trigger_eclipse_proc()
  {
    // All extra procs when eclipse pops
    p() -> resource_gain( RESOURCE_MANA,
                          p() -> resources.max[ RESOURCE_MANA ] * p() -> spell.eclipse -> effectN( 1 ).resource( RESOURCE_MANA ),
                          p() -> gain.eclipse );

    p() -> buff.natures_grace -> trigger();
  }
}; // end druid_spell_t

// Auto Attack ==============================================================

struct auto_attack_t : public melee_attack_t
{
  auto_attack_t( druid_t* player, const std::string& options_str ) :
    melee_attack_t( "auto_attack", player, spell_data_t::nil() )
  {
    parse_options( 0, options_str );

    trigger_gcd = timespan_t::zero();
  }

  virtual void execute()
  {
    player -> main_hand_attack -> weapon = &( player -> main_hand_weapon );
    player -> main_hand_attack -> base_execute_time = player -> main_hand_weapon.swing_time;
    player -> main_hand_attack -> schedule_execute();
  }

  virtual bool ready()
  {
    if ( player -> is_moving() )
      return false;

    if ( ! player -> main_hand_attack )
      return false;

    return( player -> main_hand_attack -> execute_event == 0 ); // not swinging
  }
};

// Astral Communion =========================================================

struct astral_communion_t : public druid_spell_t
{
  int starting_direction;

  astral_communion_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "astral_communion", player, player -> find_class_spell( "Astral Communion" ), options_str ),
    starting_direction( 0 )
  {
    harmful      = false;
    hasted_ticks = false;
    channeled    = true;
    may_miss     = false;
  }

  virtual double composite_haste()
  { return 1.0; }

  virtual void execute()
  {
    if ( p() -> eclipse_bar_direction == 0 )
      starting_direction = 1;
    else
      starting_direction = p() -> eclipse_bar_direction;

    druid_spell_t::execute();
  }

  virtual void tick( dot_t* /* d */ )
  {
    trigger_eclipse_energy_gain( data().effectN( 1 ).base_value() * starting_direction );
    // This should still call druid_spell_t::tick( d ); no?
  }
};

// Astral Storm =============================================================

struct astral_storm_tick_t : public druid_spell_t
{
  astral_storm_tick_t( druid_t* player, const spell_data_t* s  ) :
    druid_spell_t( "astral_storm", player, s )
  {
    background = true;
    aoe = -1;
  }
};

struct astral_storm_t : public druid_spell_t
{
  astral_storm_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "astral_storm", player, player -> find_class_spell( "Astral Storm" ) )
  {
    parse_options( NULL, options_str );
    channeled   = true;

    tick_action = new astral_storm_tick_t( player, data().effectN( 3 ).trigger() );
    dynamic_tick_action = true;
  }

  virtual bool ready()
  {
    if ( p() -> eclipse_bar_direction != 1 )
      return false;

    return druid_spell_t::ready();
  }
};

// Barkskin =================================================================

struct barkskin_t : public druid_spell_t
{
  barkskin_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "barkskin", player, player -> find_class_spell( "Barkskin" ), options_str )
  {
    harmful = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    p() -> buff.barkskin -> trigger();
  }
};

// Bear Form Spell ==========================================================

struct bear_form_t : public druid_spell_t
{
  bear_form_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "bear_form", player, player -> find_class_spell( "Bear Form" ), options_str )
  {
    harmful = false;
    min_gcd = timespan_t::from_seconds( 1.5 );

    if ( ! player -> bear_melee_attack )
    {
      player -> init_beast_weapon( player -> bear_weapon, 2.5 );
      player -> bear_melee_attack = new bear_attacks::bear_melee_t( player );
    }
  }

  virtual void execute()
  {
    spell_t::execute();

    p() -> buff.bear_form -> start();
  }

  virtual bool ready()
  {
    if ( p() -> buff.bear_form -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Berserk ==================================================================

struct berserk_t : public druid_spell_t
{
  berserk_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "berserk", player, player -> find_class_spell( "Berserk" ), options_str  )
  {
    harmful = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    if ( p() -> buff.bear_form -> check() )
    {
      p() -> buff.berserk -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, p() -> spell.berserk_bear -> duration() );
      p() -> cooldown.mangle_bear -> reset( false );
    }
    else if ( p() -> buff.cat_form -> check() )
      p() -> buff.berserk -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, p() -> spell.berserk_cat -> duration() );
  }
};

// Cat Form Spell ===========================================================

struct cat_form_t : public druid_spell_t
{
  cat_form_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "cat_form", player, player -> find_class_spell( "Cat Form" ), options_str )
  {
    harmful = false;
    min_gcd = timespan_t::from_seconds( 1.5 );

    if ( ! player -> cat_melee_attack )
    {
   	  player -> init_beast_weapon( player -> cat_weapon, 1.0 );
      player -> cat_melee_attack = new cat_attacks::cat_melee_t( player );
    }
  }

  virtual void execute()
  {
    spell_t::execute();

    p() -> buff.cat_form -> start();
  }

  virtual bool ready()
  {
    if ( p() -> buff.cat_form -> check() )
      return false;

    return druid_spell_t::ready();
  }
};


// Celestial Alignment ======================================================

struct celestial_alignment_t : public druid_spell_t
{
  celestial_alignment_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "celestial_alignment", player, player -> find_class_spell( "Celestial Alignment" ), options_str )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    if ( sim -> log ) sim -> output( "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();
    // http://elitistjerks.com/f73/t126893-mists_pandaria_all_specs/p11/#post2136096
    // I can confirm that CA works EXACTLY like combining the two eclipses
    // (starfall reset, dot refresh, SotF gives 20 energy afterwards, you
    // gain 35% mana back as you would from eclipse).
    p() -> buff.celestial_alignment -> trigger();

    // CA consumes ALL current eclipse energy, so just set the bar to 0
    p() -> eclipse_bar_value = 0;

    if ( ! p() -> buff.eclipse_lunar -> check() )
      p() -> buff.eclipse_lunar -> trigger();

    if ( ! p() -> buff.eclipse_solar -> check() )
      p() -> buff.eclipse_solar -> trigger();

    p() -> cooldown.starfall -> reset( false );

    trigger_eclipse_proc();
  }
};

// Enrage ===================================================================

struct enrage_t : public druid_spell_t
{
  enrage_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( player, player -> find_class_spell( "Enrage" ), options_str )
  {
    harmful = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    p() -> buff.enrage -> trigger();
    p() -> resource_gain( RESOURCE_RAGE, data().effectN( 2 ).resource( RESOURCE_RAGE ), p() -> gain.enrage );
  }

  virtual bool ready()
  {
    if ( ! p() -> buff.bear_form -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Faerie Fire Spell ========================================================

struct faerie_fire_t : public druid_spell_t
{
  faerie_fire_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( player, player -> find_class_spell( "Faerie Fire" ) )
  {
    parse_options( NULL, options_str );
    base_attack_power_multiplier = 1.0;
    base_spell_power_multiplier = 0.0;
    direct_power_mod = data().extra_coeff();
    cooldown -> duration = timespan_t::from_seconds( 6.0 );
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) && ! sim -> overrides.weakened_armor )
      target -> debuffs.weakened_armor -> trigger( 3 );

  }

  virtual void update_ready( timespan_t )
  {
    timespan_t cd = cooldown -> duration;

    if ( ! ( p() -> buff.bear_form -> check() || p() -> buff.cat_form -> check() ) )
      cd = timespan_t::zero();

    druid_spell_t::update_ready( cd );
  }

  virtual double action_multiplier()
  {
    if ( p() -> buff.bear_form -> check() )
      return druid_spell_t::action_multiplier();
    else
      return 0.0;
  }


  virtual void impact( action_state_t* state )
  {
    druid_spell_t::impact( state );

    if ( p() -> buff.bear_form -> check() )
    {
      // FIXME: check whether or not it is on hit only.
      if ( result_is_hit( state -> result ) )
        if ( p() -> rng.mangle -> roll( p() -> spell.mangle -> effectN( 1 ).percent() ) )
          p() -> cooldown.mangle_bear -> reset( true );
    }
  }

  virtual resource_e current_resource()
  {
    if ( p() -> buff.bear_form -> check() )
      return RESOURCE_RAGE;
    else if ( p() -> buff.cat_form -> check() )
      return RESOURCE_ENERGY;

    return RESOURCE_MANA;
  }
};

// Feral Spirit Spell =======================================================

struct feral_spirit_spell_t : public druid_spell_t
{
  feral_spirit_spell_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "feral_spirit", player,
                   ( player -> specialization() == DRUID_FERAL ) ? player -> find_spell( 110807 ) : spell_data_t::not_found() )
  {
    parse_options( NULL, options_str );
    harmful   = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    p() -> pet_feral_spirit[ 0 ] -> summon( data().duration() );
    p() -> pet_feral_spirit[ 1 ] -> summon( data().duration() );
  }

  virtual bool ready()
  {
    if ( p() -> buff.symbiosis -> value() != SHAMAN )
      return false;

    return druid_spell_t::ready();
  }
};

// Heart of the Wild Spell ====================================================

struct heart_of_the_wild_t : public druid_spell_t
{
  heart_of_the_wild_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "heart_of_the_wild", player, player -> talent.heart_of_the_wild )
  {
    parse_options( NULL, options_str );
    harmful = may_hit = may_crit = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    p() -> buff.heart_of_the_wild -> trigger();
  }
};

// Hurricane =================================================================

struct hurricane_tick_t : public druid_spell_t
{
  hurricane_tick_t( druid_t* player, const spell_data_t* s  ) :
    druid_spell_t( "hurricane", player, s )
  {
    background = true;
    aoe = -1;
  }
};

struct hurricane_t : public druid_spell_t
{
  hurricane_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "hurricane", player, player -> find_class_spell( "Hurricane" ) )
  {
    parse_options( NULL, options_str );
    channeled   = true;

    tick_action = new hurricane_tick_t( player, data().effectN( 3 ).trigger() );
    dynamic_tick_action = true;
  }

  virtual bool ready()
  {
    if ( p() -> eclipse_bar_direction == 1 )
      return false;

    return druid_spell_t::ready();
  }

  virtual double action_multiplier()
  {
    return druid_spell_t::action_multiplier()
           * ( 1.0 + p() -> buff.heart_of_the_wild -> damage_spell_multiplier() );
  }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    druid_spell_t::schedule_execute( state );

    p() -> buff.cat_form  -> expire();
    p() -> buff.bear_form -> expire();
  }
};

// Incarnation ==============================================================

struct incarnation_t : public druid_spell_t
{
  incarnation_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "incarnation", player, player -> talent.incarnation, options_str )
  {
    parse_options( NULL, options_str );
    harmful = false;
    /*
    Buff Spell: http://mop.wowhead.com/spell=117679

    Balance Passive: http://mop.wowhead.com/spell=122114

    Bear Passive: http://mop.wowhead.com/spell=113711

    Cat Passive: http://mop.wowhead.com/spell=102548
    Seems to replaces 3 spells:
      New Prowl http://mop.wowhead.com/spell=102547
      New Pounce http://mop.wowhead.com/spell=102546
      New Ravage http://mop.wowhead.com/spell=102545

    Resto Passive: http://mop.wowhead.com/spell=5420
    */
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    switch ( p() -> specialization() )
    {
    case DRUID_BALANCE:
      p() -> buff.chosen_of_elune -> trigger();
      break;
    case DRUID_FERAL:
      p() -> buff.king_of_the_jungle -> trigger();
      break;
    case DRUID_GUARDIAN:
      p() -> buff.son_of_ursoc -> trigger();
      break;
    case DRUID_RESTORATION:
      p() -> buff.tree_of_life -> trigger();
      break;
    default: break;
    }

    if ( p() -> buff.bear_form -> check() )
      p() -> cooldown.mangle_bear -> reset( false );
  }
};

// Innervate Spell ==========================================================

struct innervate_t : public druid_spell_t
{
  int trigger;

  innervate_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "innervate", player, player -> find_class_spell( "Innervate" )  ),
    trigger( 0 )
  {
    option_t options[] =
    {
      opt_int( "trigger", trigger ),
      opt_null()
    };
    parse_options( options, options_str );

    harmful = false;

    // If no target is set, assume we have innervate for ourself
    if ( target -> is_enemy() )
      target = player;
  }

  virtual void execute()
  {
    if ( sim -> log ) sim -> output( "%s performs %s", player -> name(), name() );
    consume_resource();
    update_ready();

    double gain;

    if ( target == player )
    {
      gain = 0.20;
    }
    else
    {
      gain = 0.10;
      p() -> buff.glyph_of_innervate -> trigger( 1, p() -> resources.max[ RESOURCE_MANA ] * 0.1 / 10.0 );
    }
    target -> buffs.innervate -> trigger( 1, p() -> resources.max[ RESOURCE_MANA ] * gain / 10.0 );
  }

  virtual bool ready()
  {
    if ( trigger < 0 )
      return ( target -> resources.current[ RESOURCE_MANA ] + trigger ) < 0;

    if ( trigger > 0 )
      return ( target -> resources.max    [ RESOURCE_MANA ] -
               target -> resources.current[ RESOURCE_MANA ] ) > trigger;

    return druid_spell_t::ready();
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
    //base_costs[ current_resource() ]  *= 1.0 + p -> glyph.mark_of_the_wild -> mod_additive( P_RESOURCE_COST ) / 100.0;
    harmful     = false;
    background  = ( sim -> overrides.str_agi_int != 0 );
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    if ( sim -> log ) sim -> output( "%s performs %s", player -> name(), name() );

    if ( ! sim -> overrides.str_agi_int )
      sim -> auras.str_agi_int -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, player -> dbc.spell( 79060 ) -> duration() );
  }
};

// Mirror Images (Symbiosis) Spell ==========================================

struct mirror_images_spell_t : public druid_spell_t
{
  mirror_images_spell_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "mirror_images", player,
                   ( player -> specialization() == DRUID_BALANCE ) ? player -> find_spell( 110621 ) : spell_data_t::not_found() )
  {
    parse_options( NULL, options_str );

    harmful           = false;
    min_gcd = timespan_t::from_seconds( 1.5 );
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    if ( p() -> pet_mirror_images[ 0 ] )
    {
      for ( int i = 0; i < 3; i++ )
      {
        p() -> pet_mirror_images[ i ] -> summon( data().duration() );
      }
    }
  }

  virtual bool ready()
  {
    if ( p() -> buff.symbiosis -> value() != MAGE )
      return false;

    return druid_spell_t::ready();
  }

};

// Moonfire Spell ===========================================================

struct moonfire_t : public druid_spell_t
{
  // Moonfire also applies the Sunfire DoT during Celestial Alignment.
  struct sunfire_CA_t : public druid_spell_t
  {
    sunfire_CA_t( druid_t* player ) :
      druid_spell_t( "sunfire", player, player -> find_class_spell( "Sunfire" ) )
    {
      assert( player -> specialization() == DRUID_BALANCE );

      dot_behavior = DOT_REFRESH;

      background = true;
      may_crit = false;
      may_miss = true; // Bug?

      if ( player -> set_bonus.tier14_4pc_caster() )
        num_ticks += ( int ) (  player -> sets -> set( SET_T14_4PC_CASTER ) -> effectN( 1 ).time_value() / base_tick_time );

      // Does no direct damage, costs no mana
      direct_power_mod = 0;
      base_dd_min = base_dd_max = 0;
      range::fill( base_costs, 0 );
    }

    virtual void tick( dot_t* d )
    {
      druid_spell_t::tick( d );
      // Todo: Does this sunfire proc SS?
      p() -> trigger_shooting_stars( d -> state -> result );
    }

    virtual double action_ta_multiplier()
    {
      double m = druid_spell_t::action_ta_multiplier();

      if ( p() -> buff.dream_of_cenarius_damage -> check() )
        m *= 1.0 + p() -> buff.dream_of_cenarius_damage -> data().effectN( 4 ).percent();

      return m;
    }

    virtual void execute()
    {
      p() -> buff.dream_of_cenarius_damage -> up();
      druid_spell_t::execute();
    }

    virtual void impact( action_state_t* s )
    {
      druid_spell_t::impact( s );
      p() -> buff.dream_of_cenarius_damage -> decrement();
    }
  };

  action_t* sunfire;

  moonfire_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "moonfire", player, player -> find_class_spell( "Moonfire" ) ),
    sunfire( nullptr )
  {
    parse_options( NULL, options_str );

    dot_behavior = DOT_REFRESH;

    if ( player -> set_bonus.tier14_4pc_caster() )
      num_ticks += ( int ) (  player -> sets -> set( SET_T14_4PC_CASTER ) -> effectN( 1 ).time_value() / base_tick_time );

    if ( player -> specialization() == DRUID_BALANCE )
      sunfire = new sunfire_CA_t( player );
  }

  virtual double action_da_multiplier()
  {
    double m = druid_spell_t::action_da_multiplier();

    m *= 1.0 + ( p() -> buff.lunar_shower -> data().effectN( 1 ).percent() * p() -> buff.lunar_shower -> check() );

    if ( p() -> buff.dream_of_cenarius_damage -> check() )
    {
      m *= 1.0 + p() -> buff.dream_of_cenarius_damage -> data().effectN( 3 ).percent();
    }

    return m;
  }

  virtual double action_ta_multiplier()
  {
    double m = druid_spell_t::action_ta_multiplier();

    if ( p() -> buff.dream_of_cenarius_damage -> check() )
    {
      m *= 1.0 + p() -> buff.dream_of_cenarius_damage -> data().effectN( 4 ).percent();
    }

    return m;
  }

  virtual void tick( dot_t* d )
  {
    druid_spell_t::tick( d );
    p() -> trigger_shooting_stars( d -> state -> result );
  }

  virtual void execute()
  {
    p() -> buff.dream_of_cenarius_damage -> up();
    p() -> buff.lunar_shower -> up();

    druid_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> spec.lunar_shower -> ok() )
      {
        p() -> buff.lunar_shower -> trigger();
      }
    }
  }

  virtual void impact( action_state_t* s )
  {
    // The Sunfire hits BEFORE the moonfire!
    if ( sunfire && result_is_hit( s -> result ) )
    {
      if ( p() -> buff.celestial_alignment -> check() )
      {
        sunfire -> target = s -> target;
        sunfire -> execute();
      }
    }
    druid_spell_t::impact( s );

    p() -> buff.dream_of_cenarius_damage -> decrement();
  }

  virtual double cost_reduction()
  {
    double cr = druid_spell_t::cost_reduction();

    cr += ( p() -> buff.lunar_shower -> data().effectN( 2 ).percent() * p() -> buff.lunar_shower -> check() );

    return cr;
  }
};

// Moonkin Form Spell =======================================================

struct moonkin_form_t : public druid_spell_t
{
  moonkin_form_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "moonkin_form", player, player -> find_class_spell( "Moonkin Form" )  )
  {
    parse_options( NULL, options_str );

    harmful           = false;
  }

  virtual void execute()
  {
    spell_t::execute();

    p() -> buff.moonkin_form -> start();
  }

  virtual bool ready()
  {
    if ( p() -> buff.moonkin_form -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Natures Swiftness Spell ==================================================

struct druids_swiftness_t : public druid_spell_t
{
  druids_swiftness_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "natures_swiftness", player, player -> talent.natures_swiftness )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    p() -> buff.natures_swiftness -> trigger();
  }

  virtual bool ready()
  {
    if ( p() -> buff.natures_swiftness -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Nature's Vigil ===========================================================

struct natures_vigil_t : public druid_spell_t
{
  natures_vigil_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "natures_vigil", player, player -> talent.natures_vigil )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  virtual void execute()
  {
    if ( sim -> log ) sim -> output( "%s performs %s", player -> name(), name() );

    update_ready();
    p() -> buff.natures_vigil -> trigger();
  }
};

// Starfire Spell ===========================================================

struct starfire_t : public druid_spell_t
{
  starfire_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "starfire", player, player -> find_class_spell( "Starfire" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual void impact( action_state_t* s )
  {
    druid_spell_t::impact( s );

    if ( s -> result == RESULT_CRIT && p() -> spec.eclipse -> ok() )
    {
      if ( td( s -> target ) -> dots.moonfire -> ticking )
      {
        td( s -> target ) -> dots.moonfire -> extend_duration( 1, false, td( s -> target ) -> dots.moonfire -> current_action -> snapshot_flags & ~STATE_CRIT );
      }
    }
  }

  virtual double action_multiplier()
  {
    double m = druid_spell_t::action_multiplier();

    if (  p() -> set_bonus.tier13_2pc_caster() )
      m *= 1.0 + p() -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 1 ).percent();

    return m;
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    // Cast starfire, but solar eclipse was up?
    if ( p() -> buff.eclipse_solar -> check() && ! p() -> buff.celestial_alignment -> check() )
      p() -> proc.wrong_eclipse_starfire -> occur();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> spec.eclipse -> ok() )
      {
        if ( ! p() -> buff.eclipse_solar -> check() )
        {
          int gain = data().effectN( 2 ).base_value();

          if ( ! p() -> buff.eclipse_lunar -> check() )
          {
            gain *= 2;
          }
          trigger_eclipse_energy_gain( gain );
        }
      }
    }
  }
};

// Starfall Spell ===========================================================

struct starfall_star_t : public druid_spell_t
{
  starfall_star_t( druid_t* player, uint32_t spell_id ) :
    druid_spell_t( "starfall_star", player, player -> find_spell( spell_id ) )
  {
    background  = true;
    direct_tick = true;
    aoe         = 2;
  }
};

struct starfall_t : public druid_spell_t
{
  starfall_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "starfall", player, player -> find_class_spell( "Starfall" ) )
  {
    parse_options( NULL, options_str );

    num_ticks      = 10;
    base_tick_time = timespan_t::from_seconds( 1.0 );
    hasted_ticks   = false;

    harmful = false;

    if ( player -> set_bonus.tier14_2pc_caster() )
      base_multiplier *= 1.0 + player -> sets -> set( SET_T14_2PC_CASTER ) -> effectN( 1 ).percent();

    // Starfall triggers a spell each second, that triggers the damage spell.
    const spell_data_t* stars_trigger_spell = data().effectN( 1 ).trigger();
    if ( ! stars_trigger_spell -> ok() )
    {
      background = true;
    }
    dynamic_tick_action = true;
    tick_action = new starfall_star_t( player, stars_trigger_spell -> effectN( 1 ).base_value() );
  }

  virtual void execute()
  {
    druid_spell_t::execute();
    p() -> buff.starfall -> trigger();
  }
};

// Starsurge Spell ==========================================================

struct starsurge_t : public druid_spell_t
{
  starsurge_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "starsurge", player, player -> find_class_spell( "Starsurge" ) )
  {
    parse_options( NULL, options_str );

    if ( player -> set_bonus.tier13_4pc_caster() )
    {
      cooldown -> duration += player -> sets -> set( SET_T13_4PC_CASTER ) -> effectN( 1 ).time_value();
      base_multiplier *= 1.0 + player -> sets -> set( SET_T13_4PC_CASTER ) -> effectN( 2 ).percent();
    }

    if (  p() -> set_bonus.tier13_2pc_caster() )
      base_multiplier *= 1.0 + p() -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 1 ).percent();

    if (  p() -> set_bonus.tier15_2pc_caster() )
      base_crit += p() -> sets -> set( SET_T15_2PC_CASTER ) -> effectN( 1 ).percent();
  }

  virtual void impact( action_state_t* s )
  {
    p() -> inflight_starsurge = false;

    druid_spell_t::impact( s );

    if ( s -> result == RESULT_CRIT && p() -> spec.eclipse -> ok() )
    {
      if ( td( s -> target ) -> dots.moonfire -> ticking )
        td( s -> target ) -> dots.moonfire -> extend_duration( 1, false, td( s -> target ) -> dots.moonfire -> current_action -> snapshot_flags & ~STATE_CRIT );

      if ( td( s -> target ) -> dots.sunfire -> ticking )
        td( s -> target ) -> dots.sunfire -> extend_duration( 1, false, td( s -> target ) -> dots.sunfire -> current_action -> snapshot_flags & ~STATE_CRIT );

    }
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      p() -> inflight_starsurge = true;

      if ( p() -> spec.eclipse -> ok() )
      {
        // Direction not set and bar at 0: +gain
        // Direction not set and bar < 0: -gain
        // Direction not set and bar > 0: +gain
        // Else gain is aligned with direction
        int gain = data().effectN( 2 ).base_value();
        if ( p() -> eclipse_bar_direction == -1 )
        {
          gain = -gain;
        }
        else if ( p() -> eclipse_bar_direction == 0 && p() -> eclipse_bar_value < 0 )
        {
          gain = -gain;
        }

        if ( ! p() -> buff.eclipse_lunar -> check() && ! p() -> buff.eclipse_solar -> check() )
        {
          gain *= 2;
        }
        trigger_eclipse_energy_gain( gain );
      }
    }
  }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    p() -> buff.shooting_stars -> up();

    druid_spell_t::schedule_execute( state );

    p() -> buff.shooting_stars -> expire();
  }

  virtual timespan_t execute_time()
  {
    if ( p() -> buff.shooting_stars -> check() )
      return timespan_t::zero();

    return druid_spell_t::execute_time();
  }

  virtual bool ready()
  {
    // Druids can only have 1 Starsurge in the air at a time
    if ( p() -> inflight_starsurge )
      return false;

    return druid_spell_t::ready();
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
    if ( sim -> log )
      sim -> output( "%s performs %s", player -> name(), name() );

    p() -> buff.stealthed -> trigger();
  }

  virtual bool ready()
  {
    if ( p() -> buff.stealthed -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Sunfire Spell ============================================================

struct sunfire_t : public druid_spell_t
{
  // Sunfire also applies the Moonfire DoT during Celestial Alignment.
  struct moonfire_CA_t : public druid_spell_t
  {
    moonfire_CA_t( druid_t* player ) :
      druid_spell_t( "moonfire", player, player -> find_class_spell( "Moonfire" ) )
    {
      assert( player -> specialization() == DRUID_BALANCE );

      dot_behavior = DOT_REFRESH;

      background = true;
      may_crit = false;
      may_miss = true; // Bug?

      if ( player -> set_bonus.tier14_4pc_caster() )
        num_ticks += ( int ) (  player -> sets -> set( SET_T14_4PC_CASTER ) -> effectN( 1 ).time_value() / base_tick_time );

      // Does no direct damage, costs no mana
      direct_power_mod = 0;
      base_dd_min = base_dd_max = 0;
      range::fill( base_costs, 0 );
    }

    virtual void tick( dot_t* d )
    {
      druid_spell_t::tick( d );
      // Todo: Does this dot proc SS?
      p() -> trigger_shooting_stars( d -> state -> result );
    }

    virtual double action_ta_multiplier()
    {
      double m = druid_spell_t::action_ta_multiplier();

      if ( p() -> buff.dream_of_cenarius_damage -> check() )
        m *= 1.0 + p() -> buff.dream_of_cenarius_damage -> data().effectN( 4 ).percent();

      return m;
    }

    virtual void execute()
    {
      p() -> buff.dream_of_cenarius_damage -> up();
      druid_spell_t::execute();
    }

    virtual void impact( action_state_t* s )
    {
      druid_spell_t::impact( s );
      p() -> buff.dream_of_cenarius_damage -> decrement();
    }
  };
  action_t* moonfire;

  sunfire_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "sunfire", player, player -> find_class_spell( "Sunfire" ) ),
    moonfire( nullptr )
  {
    parse_options( NULL, options_str );

    dot_behavior = DOT_REFRESH;

    if ( player -> set_bonus.tier14_4pc_caster() )
      num_ticks += ( int ) (  player -> sets -> set( SET_T14_4PC_CASTER ) -> effectN( 1 ).time_value() / base_tick_time );

    if ( player -> specialization() == DRUID_BALANCE )
      moonfire = new moonfire_CA_t( player );
  }

  virtual double action_da_multiplier()
  {
    double m = druid_spell_t::action_da_multiplier();

    m *= 1.0 + ( p() -> buff.lunar_shower -> data().effectN( 1 ).percent() * p() -> buff.lunar_shower -> check() );

    if ( p() -> buff.dream_of_cenarius_damage -> check() )
    {
      m *= 1.0 + p() -> buff.dream_of_cenarius_damage -> data().effectN( 3 ).percent();
    }

    return m;
  }

  virtual double action_ta_multiplier()
  {
    double m = druid_spell_t::action_ta_multiplier();

    if ( p() -> buff.dream_of_cenarius_damage -> check() )
    {
      m *= 1.0 + p() -> buff.dream_of_cenarius_damage -> data().effectN( 4 ).percent();
    }

    return m;
  }

  virtual void tick( dot_t* d )
  {
    druid_spell_t::tick( d );

    p() -> trigger_shooting_stars( d -> state -> result );
  }

  virtual void execute()
  {
    p() -> buff.dream_of_cenarius_damage -> up();
    p() -> buff.lunar_shower -> up();

    druid_spell_t::execute();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> spec.lunar_shower -> ok() )
      {
        p() -> buff.lunar_shower -> trigger( 1 );
      }
    }
  }

  virtual void impact( action_state_t* s )
  {
    if ( moonfire && result_is_hit( s -> result ) )
    {
      if ( p() -> buff.celestial_alignment -> check() )
      {
        moonfire -> target = s -> target;
        moonfire -> execute();
      }
    }
    druid_spell_t::impact( s );

    p() -> buff.dream_of_cenarius_damage -> decrement();
  }

  virtual double cost_reduction()
  {
    double cr = druid_spell_t::cost_reduction();

    cr += ( p() -> buff.lunar_shower -> data().effectN( 2 ).percent() * p() -> buff.lunar_shower -> check() );

    return cr;
  }
};

// Survival Instincts =======================================================

struct survival_instincts_t : public druid_spell_t
{
  survival_instincts_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( player, player -> find_specialization_spell( "Survival Instincts" ), options_str )
  {
    harmful = false;
    use_off_gcd = true;
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    p() -> buff.survival_instincts -> trigger(); // DBC value is 60 for some reason
  }

  virtual bool ready()
  {

    if ( ! p() -> buff.bear_form -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Symbiosis Spell ==========================================================

struct symbiosis_t : public druid_spell_t
{
  player_e target_class;

  symbiosis_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "symbiosis", player, player -> find_class_spell( "Symbiosis" ) ),
    target_class( PLAYER_NONE )
  {
    std::string class_str;
    option_t options[] =
    {
      opt_string( "class", class_str ),
      opt_null()
    };
    parse_options( options, options_str );

    harmful = false;

    if ( ! class_str.empty() )
      target_class = util::parse_player_type( class_str );
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    p() -> buff.symbiosis -> trigger( 1, target_class, 1.0 );
  }

  virtual bool ready()
  {
    if ( p() -> in_combat )
      return false;

    if ( p() -> buff.symbiosis -> check() )
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
    druid_spell_t( "treants", player, player -> talent.force_of_nature )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    if ( p() -> pet_treants[ 0 ] )
    {
      for ( int i = 0; i < 3; i++ )
      {
        p() -> pet_treants[ i ] -> summon( p() -> talent.force_of_nature -> duration() );
      }
    }
  }
};

// Typhoon ==================================================================

struct typhoon_t : public druid_spell_t
{
  typhoon_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "typhoon", player, player -> talent.typhoon )
  {
    parse_options( NULL, options_str );
  }
};

// Wild Mushroom ============================================================

struct wild_mushroom_t : public druid_spell_t
{
  wild_mushroom_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "wild_mushroom", player, player -> find_class_spell( "Wild Mushroom" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    p() -> buff.wild_mushroom -> trigger();
  }
};

// Wild Mushroom: Detonate ==================================================

struct wild_mushroom_detonate_damage_t : public druid_spell_t
{
  wild_mushroom_detonate_damage_t( druid_t* player ) :
    druid_spell_t( "wild_mushroom_detonate", player, player -> find_spell( 78777 ) )
  {
    aoe        = -1;
    background = true;
    dual       = true;
  }
};

struct wild_mushroom_detonate_t : public druid_spell_t
{
  action_t* detonation;

  wild_mushroom_detonate_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "wild_mushroom_detonate", player, player -> find_class_spell( "Wild Mushroom: Detonate" ) ),
    detonation( new wild_mushroom_detonate_damage_t( player ) )
  {
    parse_options( NULL, options_str );

    // Actual ability is 88751, all damage is in spell 78777
    school             = detonation -> school;
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    while ( p() -> buff.wild_mushroom -> check() )
    {
      detonation -> execute();
      p() -> buff.wild_mushroom -> decrement();
    }
  }

  virtual bool ready()
  {

    if ( ! p() -> buff.wild_mushroom -> stack() )
      return false;

    return druid_spell_t::ready();
  }
};

// Wrath Spell ==============================================================

struct wrath_t : public druid_spell_t
{
  wrath_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "wrath", player, player -> find_class_spell( "Wrath" ) )
  {
    parse_options( NULL, options_str );
  }

  virtual void impact( action_state_t* s )
  {
    druid_spell_t::impact( s );

    if ( s -> result == RESULT_CRIT && p() -> spec.eclipse -> ok() )
    {
      if ( td( s -> target ) -> dots.sunfire -> ticking )
      {
        td( s -> target ) -> dots.sunfire -> extend_duration( 1, false, td( s -> target ) -> dots.sunfire -> current_action -> snapshot_flags & ~STATE_CRIT );
      }
    }
  }

  virtual double action_multiplier()
  {
    double m = druid_spell_t::action_multiplier();

    if ( p() -> set_bonus.tier13_2pc_caster() )
      m *= 1.0 + p() -> sets -> set( SET_T13_2PC_CASTER ) -> effectN( 1 ).percent();

    m *= 1.0 + p() -> buff.heart_of_the_wild -> damage_spell_multiplier();

    return m;
  }

  virtual void execute()
  {
    druid_spell_t::execute();

    // Cast wrath, but lunar eclipse was up?
    if ( p() -> buff.eclipse_lunar -> check() && ! p() -> buff.celestial_alignment -> check() )
      p() -> proc.wrong_eclipse_wrath -> occur();

    if ( result_is_hit( execute_state -> result ) )
    {
      if ( p() -> spec.eclipse -> ok() )
      {
        if ( p() -> eclipse_bar_direction <= 0 )
        {
          int gain = data().effectN( 2 ).base_value();

          if ( ! p() -> buff.eclipse_solar -> check() )
          {
            gain *= 2;
          }
          trigger_eclipse_energy_gain( -gain );
        }
      }
    }
  }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    druid_spell_t::schedule_execute( state );

    p() -> buff.cat_form  -> expire();
    p() -> buff.bear_form -> expire();
  }
};

} // end namespace spells

namespace buffs {

template <typename BuffBase>
struct druid_buff_t : public BuffBase
{
protected:
  typedef druid_buff_t base_t;
  druid_t& druid;

  // Used when shapeshifting to switch to a new attack & schedule it to occur
  // when the current swing timer would have ended.
  void swap_melee( attack_t* new_attack, weapon_t& new_weapon )
  {
    if ( druid.main_hand_attack && druid.main_hand_attack -> execute_event )
    {
      new_attack -> base_execute_time = new_weapon.swing_time;
      new_attack -> execute_event = new_attack -> start_action_execute_event(
   	    druid.main_hand_attack -> execute_event -> remains() );
      druid.main_hand_attack -> cancel();
    }
    new_attack -> weapon = &new_weapon;
    druid.main_hand_attack = new_attack;
    druid.main_hand_weapon = new_weapon;
  }

public:
  druid_buff_t( druid_t& p, const buff_creator_basics_t& params ) :
    BuffBase( params ),
    druid( p )
  { }
};

// Celestial Ailgnment Buff =================================================

struct celestial_alignment_t : public druid_buff_t < buff_t >
{
  celestial_alignment_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "celestial_alignment", p.find_class_spell( "Celestial Alignment" ) ) )
  {
    cooldown -> duration = timespan_t::zero(); // CD is managed by the spell
  }

  virtual void expire_override()
  {
    buff_t::expire_override();

    druid_t* p = static_cast<druid_t*>( player );
    p -> buff.eclipse_lunar -> expire();
    p -> buff.eclipse_solar -> expire();
    p -> trigger_soul_of_the_forest();
  }
};

// Bear Form

class bear_form_t : public druid_buff_t< buff_t >
{
  const spell_data_t* rage_spell;

public:
  bear_form_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "bear_form", p.find_class_spell( "Bear Form" ) ) ),
    rage_spell( p.find_spell( 17057 ) )
  {
    if ( druid.specialization() == DRUID_GUARDIAN )
      druid.vengeance_init();

    // HOTW will require invalidation of hit
    if ( druid.talent.heart_of_the_wild -> ok() )
    {
      invalidate_list.push_back( CACHE_HIT );
      invalidate_list.push_back( CACHE_EXP );
    }
    invalidate_list.push_back( CACHE_AGILITY );
    invalidate_list.push_back( CACHE_ATTACK_POWER );
    invalidate_list.push_back( CACHE_HASTE );
    invalidate_list.push_back( CACHE_CRIT );
    requires_invalidation = true;
  }

  virtual void expire_override()
  {
    base_t::expire_override();

    druid.main_hand_weapon = druid.caster_form_weapon;

    sim -> auras.critical_strike -> decrement();

    if ( druid.specialization() == DRUID_GUARDIAN )
      druid.vengeance_stop();

    druid.current.attack_power_per_agility -= 2.0;
  }

  virtual void start( int stacks, double value, timespan_t duration )
  {
    druid.buff.moonkin_form -> expire();
    druid.buff.cat_form -> expire();

    if ( druid.specialization() == DRUID_GUARDIAN )
      druid.vengeance_start();

    swap_melee( druid.bear_melee_attack, druid.bear_weapon );

    // Set rage to 0 and then gain rage to 10
    druid.resource_loss( RESOURCE_RAGE, druid.resources.current[ RESOURCE_RAGE ] );
    druid.resource_gain( RESOURCE_RAGE, rage_spell -> effectN( 1 ).base_value() / 10.0, druid.gain.bear_form );
    // TODO: Clear rage on bear form exit instead of entry.

    base_t::start( stacks, value, duration );

    if ( ! sim -> overrides.critical_strike )
      sim -> auras.critical_strike -> trigger();

    druid.current.attack_power_per_agility += 2.0;
  }
};

// Cat Form

struct cat_form_t : public druid_buff_t< buff_t >
{
  cat_form_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "cat_form", p.find_class_spell( "Cat Form" ) ) )
  {
    // HOTW will require invalidation of various things, as the actor may
    // change shapeshift forms during HoTW, that affects the stats
    if ( druid.talent.heart_of_the_wild -> ok() )
    {
      invalidate_list.push_back( CACHE_HIT );
      invalidate_list.push_back( CACHE_EXP );
    }
    invalidate_list.push_back( CACHE_AGILITY );
    invalidate_list.push_back( CACHE_ATTACK_POWER );
    invalidate_list.push_back( CACHE_PLAYER_DAMAGE_MULTIPLIER );
    requires_invalidation = true;
  }

  virtual void expire_override()
  {
    base_t::expire_override();

    druid.main_hand_weapon = druid.caster_form_weapon;

    sim -> auras.critical_strike -> decrement();

    druid.current.attack_power_per_agility -= 2.0;
  }

  virtual void start( int stacks, double value, timespan_t duration )
  {
    druid.buff.bear_form -> expire();
    druid.buff.moonkin_form -> expire();

    swap_melee( druid.cat_melee_attack, druid.cat_weapon );

    base_t::start( stacks, value, duration );

    if ( ! sim -> overrides.critical_strike )
      sim -> auras.critical_strike -> trigger();

    druid.current.attack_power_per_agility += 2.0;
  }
};

// Moonkin Form

struct moonkin_form_t : public druid_buff_t< buff_t >
{
  moonkin_form_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "moonkin_form", p.find_class_spell( "Moonkin Form" ) )
               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER ) )
  { }

  virtual void expire_override()
  {
    base_t::expire_override();

    sim -> auras.spell_haste -> decrement();
  }

  virtual void start( int stacks, double value, timespan_t duration )
  {
    druid.buff.bear_form -> expire();
    druid.buff.cat_form  -> expire();

    base_t::start( stacks, value, duration );

    if ( ! sim -> overrides.spell_haste )
      sim -> auras.spell_haste -> trigger();
  }
};

// Innervate Buff ===========================================================

struct innervate_t : public buff_t
{
  struct innervate_event_t : public event_t
  {
    innervate_event_t ( player_t* p ) :
      event_t( p, "innervate" )
    {
      sim.add_event( this, timespan_t::from_seconds( 1.0 ) );
    }

    virtual void execute()
    {
      if ( player -> buffs.innervate -> check() )
      {
        player -> resource_gain( RESOURCE_MANA,
                                 player -> buffs.innervate -> value(),
                                 player -> gains.innervate );

        new ( sim ) innervate_event_t( player );
      }
    }
  };

  innervate_t( player_t* player ) :
    buff_t ( buff_creator_t( player, "innervate", player -> find_spell( 29166 ) ) )
  {
    cooldown -> duration = timespan_t::zero(); // CD is managed by the spell
  }

  virtual void start( int stacks, double value, timespan_t duration )
  {
    new ( *sim ) innervate_event_t( player );

    buff_t::start( stacks, value, duration );
  }
};

} // end namespace buffs


// ==========================================================================
// Druid Character Definition
// ==========================================================================

void druid_t::trigger_shooting_stars( result_e result )
{
  if ( result == RESULT_CRIT )
  {
    int stack = buff.shooting_stars -> check();
    if ( buff.shooting_stars -> trigger() )
    {
      if ( stack == buff.shooting_stars -> check() )
        proc.shooting_stars_wasted -> occur();
      cooldown.starsurge -> reset( true );
    }
  }
}

void druid_t::trigger_soul_of_the_forest()
{
  if ( ! talent.soul_of_the_forest -> ok() )
    return;

  int gain = talent.soul_of_the_forest -> effectN( 2 ).base_value() * eclipse_bar_direction;
  eclipse_bar_value += gain;

  if ( sim -> log )
  {
    sim -> output( "%s gains %d (%d) %s from %s (%d)",
                   name(), gain, gain,
                   "Eclipse", talent.soul_of_the_forest -> name_cstr(),
                   eclipse_bar_value );
  }
}

// druid_t::create_action  ==================================================

action_t* druid_t::create_action( const std::string& name,
                                  const std::string& options_str )
{
  using namespace cat_attacks;
  using namespace bear_attacks;
  using namespace heals;
  using namespace spells;

  if ( name == "astral_communion"       ) return new       astral_communion_t( this, options_str );
  if ( name == "astral_storm"           ) return new           astral_storm_t( this, options_str );
  if ( name == "auto_attack"            ) return new            auto_attack_t( this, options_str );
  if ( name == "barkskin"               ) return new               barkskin_t( this, options_str );
  if ( name == "berserk"                ) return new                berserk_t( this, options_str );
  if ( name == "bear_form"              ) return new              bear_form_t( this, options_str );
  if ( name == "cat_form"               ) return new               cat_form_t( this, options_str );
  if ( name == "celestial_alignment"    ) return new    celestial_alignment_t( this, options_str );
  if ( name == "death_coil"             ) return new             death_coil_t( this, options_str );
  if ( name == "enrage"                 ) return new                 enrage_t( this, options_str );
  if ( name == "faerie_fire"            ) return new            faerie_fire_t( this, options_str );
  if ( name == "feral_spirit"           ) return new     feral_spirit_spell_t( this, options_str );
  if ( name == "ferocious_bite"         ) return new         ferocious_bite_t( this, options_str );
  if ( name == "frenzied_regeneration"  ) return new  frenzied_regeneration_t( this, options_str );
  if ( name == "healing_touch"          ) return new          healing_touch_t( this, options_str );
  if ( name == "hurricane"              ) return new              hurricane_t( this, options_str );
  if ( name == "heart_of_the_wild"      ) return new      heart_of_the_wild_t( this, options_str );
  if ( name == "incarnation"            ) return new            incarnation_t( this, options_str );
  if ( name == "innervate"              ) return new              innervate_t( this, options_str );
  if ( name == "lacerate"               ) return new               lacerate_t( this, options_str );
  if ( name == "lifebloom"              ) return new              lifebloom_t( this, options_str );
  if ( name == "maim"                   ) return new                   maim_t( this, options_str );
  if ( name == "mangle_bear"            ) return new            mangle_bear_t( this, options_str );
  if ( name == "mangle_cat"             ) return new             mangle_cat_t( this, options_str );
  if ( name == "mark_of_the_wild"       ) return new       mark_of_the_wild_t( this, options_str );
  if ( name == "maul"                   ) return new                   maul_t( this, options_str );
  if ( name == "mirror_images"          ) return new    mirror_images_spell_t( this, options_str );
  if ( name == "moonfire"               ) return new               moonfire_t( this, options_str );
  if ( name == "moonkin_form"           ) return new           moonkin_form_t( this, options_str );
  if ( name == "natures_swiftness"      ) return new       druids_swiftness_t( this, options_str );
  if ( name == "natures_vigil"          ) return new          natures_vigil_t( this, options_str );
  if ( name == "nourish"                ) return new                nourish_t( this, options_str );
  if ( name == "pounce"                 ) return new                 pounce_t( this, options_str );
  if ( name == "rake"                   ) return new                   rake_t( this, options_str );
  if ( name == "ravage"                 ) return new                 ravage_t( this, options_str );
  if ( name == "regrowth"               ) return new               regrowth_t( this, options_str );
  if ( name == "rejuvenation"           ) return new           rejuvenation_t( this, options_str );
  if ( name == "rip"                    ) return new                    rip_t( this, options_str );
  if ( name == "savage_roar"            ) return new            savage_roar_t( this, options_str );
  if ( name == "savage_defense"            ) return new            savage_defense_t( this, options_str );
  if ( name == "shattering_blow"        ) return new        shattering_blow_t( this, options_str );
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
  if ( name == "symbiosis"              ) return new              symbiosis_t( this, options_str );
  if ( name == "tigers_fury"            ) return new            tigers_fury_t( this, options_str );
  if ( name == "thrash_bear"            ) return new            thrash_bear_t( this, options_str );
  if ( name == "thrash_cat"             ) return new             thrash_cat_t( this, options_str );
  if ( name == "treants"                ) return new          treants_spell_t( this, options_str );
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

  using namespace pets;

  if ( pet_name == "treants" )
  {
    if ( specialization() == DRUID_BALANCE ) return new treants_balance_t( sim, this );
    if ( specialization() == DRUID_FERAL   ) return new   treants_feral_t( sim, this );
  }

  if ( pet_name == "symbiosis_mirror_image" ) return new symbiosis_mirror_image_t( sim, this );
  if ( pet_name == "symbiosis_feral_spirit" ) return new symbiosis_feral_spirit_t( sim, this );

  return 0;
}

// druid_t::create_pets =====================================================

void druid_t::create_pets()
{
  if ( specialization() == DRUID_BALANCE )
  {
    for ( int i = 0; i < 3; ++i )
      pet_treants[ i ] = create_pet( "treants" );
    for ( int i = 0; i < 3; ++i )
      pet_mirror_images[ i ] = create_pet( "symbiosis_mirror_image" );
  }
  else if ( specialization() == DRUID_FERAL )
  {
    for ( int i = 0; i < 3; ++i )
      pet_treants[ i ] = create_pet( "treants" );
    for ( int i = 0; i < 2; ++i )
      pet_feral_spirit[ i ] = create_pet( "symbiosis_feral_spirit" );
  }
}

// druid_t::init_spells =====================================================

void druid_t::init_spells()
{
  player_t::init_spells();

  // Specializations
  // Generic / Multiple specs
  spec.leather_specialization = find_specialization_spell( "Leather Specialization" );
  spec.omen_of_clarity        = find_specialization_spell( "Omen of Clarity" );
  spec.killer_instinct        = find_specialization_spell( "Killer Instinct" );
  spec.nurturing_instinct     = find_specialization_spell( "Nurturing Instinct" );

  // Balance
  // Eclipse are 2 spells, the mana energize is not in the main spell!
  // http://mop.wowhead.com/spell=79577 => Specialization spell
  // http://mop.wowhead.com/spell=81070 => The mana gain energize spell
  // Moonkin is also split up into two spells
  spec.balance_of_power       = find_specialization_spell( "Balance of Power" );
  spec.celestial_focus        = find_specialization_spell( "Celestial Focus" );
  spec.eclipse                = find_specialization_spell( "Eclipse" );
  spec.euphoria               = find_specialization_spell( "Euphoria" );
  spec.lunar_shower           = find_specialization_spell( "Lunar Shower" );
  spec.owlkin_frenzy          = find_specialization_spell( "Owlkin Frenzy" );
  spec.shooting_stars         = find_specialization_spell( "Shooting Stars" );

  // Feral
  spec.predatory_swiftness    = find_specialization_spell( "Predatory Swiftness" );

  // Guardian
  spec.leader_of_the_pack     = find_specialization_spell( "Leader of the Pack" );
  spec.thick_hide             = find_specialization_spell( "Thick Hide" );

  // Restoration
  spec.living_seed            = find_specialization_spell( "Living Seed" );
  spec.meditation             = find_specialization_spell( "Meditation" );
  spec.natural_insight        = find_specialization_spell( "Natural Insight" );
  spec.natures_focus          = find_specialization_spell( "Nature's Focus" );
  spec.revitalize             = find_specialization_spell( "Revitalize" );


  // Talents
  talent.feline_swiftness   = find_talent_spell( "Feline Swiftness" );
  talent.displacer_beast    = find_talent_spell( "Displacer Beast" );
  talent.wild_charge        = find_talent_spell( "Wild Charge" );

  talent.natures_swiftness  = find_talent_spell( "Nature's Swiftness" );
  talent.renewal            = find_talent_spell( "Renewal" );
  talent.cenarion_ward      = find_talent_spell( "Cenarion Ward" );

  talent.faerie_swarm       = find_talent_spell( "Faerie Swarm" );
  talent.mass_entanglement  = find_talent_spell( "Mass Entanglement" );
  talent.typhoon            = find_talent_spell( "Typhoon" );

  talent.soul_of_the_forest = find_talent_spell( "Soul of the Forest" );
  talent.incarnation        = find_talent_spell( "Incarnation" );
  talent.force_of_nature    = find_talent_spell( "Force of Nature" );

  talent.disorienting_roar  = find_talent_spell( "Disorienting Roar" );
  talent.ursols_vortex      = find_talent_spell( "Ursol's Vortex" );
  talent.mighty_bash        = find_talent_spell( "Mighty Bash" );

  talent.heart_of_the_wild  = find_talent_spell( "Heart of the Wild" );
  talent.dream_of_cenarius  = find_talent_spell( "Dream of Cenarius" );
  talent.natures_vigil      = find_talent_spell( "Nature's Vigil" );

  // TODO: Check if this is really the passive applied, the actual shapeshift
  // only has data of shift, polymorph immunity and the general armor bonus

  spell.bear_form                       = find_class_spell( "Bear Form"                   ) -> ok() ? find_spell( 1178   ) : spell_data_t::not_found(); // This is the passive applied on shapeshift!
  spell.berserk_bear                    = find_class_spell( "Berserk"                     ) -> ok() ? find_spell( 50334  ) : spell_data_t::not_found(); // Berserk bear mangler
  spell.berserk_cat                     = find_class_spell( "Berserk"                     ) -> ok() ? find_spell( 106951 ) : spell_data_t::not_found(); // Berserk cat resource cost reducer
  spell.combo_point                     = find_class_spell( "Cat Form"                    ) -> ok() ? find_spell( 34071  ) : spell_data_t::not_found(); // Combo point add "spell", weird
  spell.eclipse                         = spec.eclipse -> ok() ? find_spell( 81070  ) : spell_data_t::not_found(); // Eclipse mana gain trigger
  spell.heart_of_the_wild               = talent.heart_of_the_wild -> ok() ? find_spell( 17005  ) : spell_data_t::not_found(); // HotW INT/AGI bonus
  spell.leader_of_the_pack              = spec.leader_of_the_pack -> ok() ? find_spell( 24932  ) : spell_data_t::not_found(); // LotP aura
  spell.mangle                          = find_class_spell( "Lacerate"                    ) -> ok() ||
                                          find_specialization_spell( "Thrash"             ) -> ok() ? find_spell( 93622  ) : spell_data_t::not_found(); // Lacerate mangle cooldown reset
  spell.moonkin_form                    = find_class_spell( "Moonkin Form"                ) -> ok() ? find_spell( 24905  ) : spell_data_t::not_found(); // This is the passive applied on shapeshift!
  spell.primal_fury                     = ( specialization() == DRUID_FERAL || specialization() == DRUID_GUARDIAN ) ? find_spell( 16959  ) : spell_data_t::not_found(); // Primal fury rage gain trigger
  spell.regrowth                        = find_class_spell( "Regrowth"                    ) -> ok() ? find_spell( 93036  ) : spell_data_t::not_found(); // Regrowth refresh
  spell.survival_instincts              = find_class_spell( "Survival Instincts"          ) -> ok() ? find_spell( 50322  ) : spell_data_t::not_found(); // Survival Instincts aura
  spell.swiftmend                       = find_class_spell( "Swiftmend"                   ) -> effectN( 2 ).trigger();
  spell.swipe                           = find_class_spell( "Maul"                        ) -> ok() ||
                                          find_class_spell( "Shred"                       ) -> ok() ? find_spell( 62078  ) : spell_data_t::not_found(); // Bleed damage multiplier for Shred etc.

  // Masteries
  mastery.total_eclipse    = find_mastery_spell( DRUID_BALANCE );
  mastery.razor_claws      = find_mastery_spell( DRUID_FERAL );
  mastery.harmony          = find_mastery_spell( DRUID_RESTORATION );
  mastery.natures_guardian = find_mastery_spell( DRUID_GUARDIAN );

  // Glyphs
  glyph.blooming              = find_glyph_spell( "Glyph of Blooming" );
  glyph.ferocious_bite        = find_glyph_spell( "Glyph of Ferocious Bite" );
  glyph.frenzied_regeneration = find_glyph_spell( "Glyph of Frenzied Regeneration" );
  glyph.healing_touch         = find_glyph_spell( "Glyph of Healing Touch" );
  glyph.innervate             = find_glyph_spell( "Glyph of Innervate" );
  glyph.lifebloom             = find_glyph_spell( "Glyph of Lifebloom" );
  glyph.maul                  = find_glyph_spell( "Glyph of Maul" );
  glyph.moonbeast             = find_glyph_spell( "Glyph of the Moonbeast" );
  glyph.regrowth              = find_glyph_spell( "Glyph of Regrowth" );
  glyph.rejuvenation          = find_glyph_spell( "Glyph of Rejuvenation" );
  glyph.skull_bash            = find_glyph_spell( "Glyph of Skull Bash" );
  glyph.wild_growth           = find_glyph_spell( "Glyph of Wild Growth" );
  glyph.savagery              = find_glyph_spell( "Glyph of Savagery" );

  // Tier Bonuses
  static const uint32_t set_bonuses[N_TIER][N_TIER_BONUS] =
  {
    //   C2P     C4P     M2P     M4P    T2P    T4P     H2P     H4P
    { 105722, 105717, 105725, 105735,      0,      0, 105715, 105770 }, // Tier13
    { 123082, 123083, 123084, 123085, 123086, 123087, 123088, 123089 }, // Tier14
    { 138348, 138350, 138352, 138357, 138216, 138222, 138284, 138286 }, // Tier15
    {      0,      0,      0,      0,      0,      0,      0,      0 }, // Tier16
  };

  sets = new set_bonus_array_t( this, set_bonuses );
}

// druid_t::init_base =======================================================

void druid_t::init_base_stats()
{
  player_t::init_base_stats();

  base.stats.attack_power = level * ( level > 80 ? 3.0 : 2.0 );

  base.attack_power_per_strength = 1.0;
  base.spell_power_per_intellect = 1.0;

  diminished_kfactor    = 0.009720;
  diminished_dodge_cap = 0.008555;
  diminished_parry_cap = 0.008555;

  resources.base[ RESOURCE_ENERGY ] = 100;
  resources.base[ RESOURCE_RAGE   ] = 100;

  base_energy_regen_per_second = 10;

  // Natural Insight: +400% mana
  resources.base_multiplier[ RESOURCE_MANA ] = 1.0 + spec.natural_insight -> effectN( 1 ).percent();
  base.mana_regen_per_second *= 1.0 + spec.natural_insight -> effectN( 1 ).percent();

  base_gcd = timespan_t::from_seconds( 1.5 );
}

// druid_t::init_buffs ======================================================

void druid_t::create_buffs()
{
  player_t::create_buffs();

  using namespace buffs;

  // MoP checked

  // Generic / Multi-spec druid buffs
  buff.bear_form             = new bear_form_t( *this );
  buff.berserk               = buff_creator_t( this, "berserk", spell_data_t::nil() );
  buff.cat_form              = new cat_form_t( *this );
  buff.frenzied_regeneration = buff_creator_t( this, "frenzied_regeneration", find_class_spell( "Frenzied Regeneration" ) );
  buff.moonkin_form          = new moonkin_form_t( *this );
  buff.omen_of_clarity       = buff_creator_t( this, "omen_of_clarity", spec.omen_of_clarity -> effectN( 1 ).trigger() )
                               .chance( spec.omen_of_clarity -> ok() ? find_spell( 113043 ) -> proc_chance() : 0.0 );
  buff.soul_of_the_forest    = buff_creator_t( this, "soul_of_the_forest", talent.soul_of_the_forest -> ok() ? find_spell( 114108 ) : spell_data_t::not_found() )
                               .default_value( find_spell( 114108 ) -> effectN( 1 ).percent() );
  buff.stealthed             = buff_creator_t( this, "stealthed", find_class_spell( "Prowl" ) );
  buff.symbiosis             = buff_creator_t( this, "symbiosis", find_class_spell( "Symbiosis" ) );
  buff.wild_mushroom         = buff_creator_t( this, "wild_mushroom", find_class_spell( "Wild Mushroom" ) )
                               .max_stack( ( specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION )
                                           ? find_class_spell( "Wild Mushroom" ) -> effectN( 2 ).base_value()
                                           : 1 )
                               .quiet( true );

  // Talent buffs

  // http://mop.wowhead.com/spell=122114 Chosen of Elune
  buff.chosen_of_elune    = buff_creator_t( this, "chosen_of_elune"   , talent.incarnation -> ok() ? find_spell( 122114 ) : spell_data_t::not_found() )
                            .duration( talent.incarnation -> duration() )
                            .chance( talent.incarnation -> ok() ? ( specialization() == DRUID_BALANCE ) : 0.0 )
                            .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  // http://mop.wowhead.com/spell=102548 Incarnation: King of the Jungle
  buff.king_of_the_jungle = buff_creator_t( this, "king_of_the_jungle", talent.incarnation -> ok() ? find_spell( 102543 ) : spell_data_t::not_found() )
                            .duration( talent.incarnation -> duration() )
                            .chance( talent.incarnation -> ok() ? ( specialization() == DRUID_FERAL ) : 0.0 );

  // http://mop.wowhead.com/spell=113711 Incarnation: Son of Ursoc      Passive
  buff.son_of_ursoc       = buff_creator_t( this, "son_of_ursoc"      , talent.incarnation -> ok() ? find_spell( 102558 ) : spell_data_t::not_found() )
                            .duration( talent.incarnation -> duration() )
                            .chance( talent.incarnation -> ok() ?  ( specialization() == DRUID_GUARDIAN ) : 0.0 );

  // http://mop.wowhead.com/spell=5420 Incarnation: Tree of Life        Passive
  buff.tree_of_life       = buff_creator_t( this, "tree_of_life"      , talent.incarnation -> ok() ? find_spell( 5420 ) : spell_data_t::not_found() )
                            .duration( talent.incarnation -> duration() )
                            .chance( talent.incarnation -> ok() ?  ( specialization() == DRUID_RESTORATION ) : 0.0 );

  buff.dream_of_cenarius_damage = buff_creator_t( this, "dream_of_cenarius_damage", talent.dream_of_cenarius -> ok() ? find_spell( 108381 ) : spell_data_t::not_found() )
                                  .max_stack( 2 );
  buff.dream_of_cenarius_heal   = buff_creator_t( this, "dream_of_cenarius_heal",   talent.dream_of_cenarius -> ok() ? find_spell( 108382 ) : spell_data_t::not_found() )
                                  .max_stack( 2 );

  buff.natures_vigil      = buff_creator_t( this, "natures_vigil", talent.natures_vigil -> ok() ? find_spell( 124974 ) : spell_data_t::not_found() )
                            .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                            .add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );

  buff.heart_of_the_wild  = new heart_of_the_wild_buff_t( *this );

  // Balance

  buff.celestial_alignment   = new celestial_alignment_t( *this );
  buff.eclipse_lunar         = buff_creator_t( this, "lunar_eclipse",  find_specialization_spell( "Eclipse" ) -> ok() ? find_spell( 48518 ) : spell_data_t::not_found() )
                               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buff.eclipse_solar         = buff_creator_t( this, "solar_eclipse",  find_specialization_spell( "Eclipse" ) -> ok() ? find_spell( 48517 ) : spell_data_t::not_found() )
                               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buff.lunar_shower          = buff_creator_t( this, "lunar_shower",   spec.lunar_shower -> effectN( 1 ).trigger() );
  buff.shooting_stars        = buff_creator_t( this, "shooting_stars", spec.shooting_stars -> effectN( 1 ).trigger() )
                               .chance( spec.shooting_stars -> proc_chance() );
  buff.starfall              = buff_creator_t( this, "starfall",       find_specialization_spell( "Starfall" ) )
                               .cd( timespan_t::zero() );

  stat_buff_creator_t ng = stat_buff_creator_t( this, "natures_grace" )
                               .spell( find_specialization_spell( "Eclipse" ) -> ok() ? find_spell( 16886 ) : spell_data_t::not_found() );
  if ( set_bonus.tier15_4pc_caster() )
  {
    ng.add_stat( STAT_CRIT_RATING, sets -> set( SET_T15_4PC_CASTER ) -> effectN( 1 ).base_value() )
      .add_stat( STAT_MASTERY_RATING, sets -> set( SET_T15_4PC_CASTER ) -> effectN( 1 ).base_value() );
  }
  buff.natures_grace = ng;

  // Feral
  buff.tigers_fury           = buff_creator_t( this, "tigers_fury", find_specialization_spell( "Tiger's Fury" ) )
                               .default_value( find_specialization_spell( "Tiger's Fury" ) -> effectN( 1 ).percent() )
                               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buff.savage_roar           = buff_creator_t( this, "savage_roar", find_specialization_spell( "Savage Roar" ) )
                               .default_value( find_spell( 62071 ) -> effectN( 1 ).percent() )
                               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buff.predatory_swiftness   = buff_creator_t( this, "predatory_swiftness", spec.predatory_swiftness -> ok() ? find_spell( 69369 ) : spell_data_t::not_found() );
  buff.tier15_4pc_melee      = buff_creator_t( this, "tier15_4pc_melee", spell_data_t::nil() )
                               .max_stack( 3 );

  // Guardian
  buff.enrage                = buff_creator_t( this, "enrage" , find_specialization_spell( "Enrage" ) );
  buff.lacerate              = buff_creator_t( this, "lacerate" , find_class_spell( "Lacerate" ) );
  buff.savage_defense        = buff_creator_t( this, "savage_defense", find_class_spell( "Savage Defense" ) -> ok() ? find_spell( 132402 ) : spell_data_t::not_found() );
  buff.survival_instincts    = buff_creator_t( this, "survival_instincts", spell.survival_instincts );
  buff.tier15_2pc_tank       = buff_creator_t( this, "tier15_2pc_tank", find_spell( 138217 ) );

  // Restoration

  // Not checked for MoP

  buff.barkskin              = buff_creator_t( this, "barkskin", find_class_spell( "Barkskin" ) -> ok() ? find_spell( 22812 ) : spell_data_t::not_found() );
  buff.harmony               = buff_creator_t( this, "harmony", mastery.harmony -> ok() ? find_spell( 100977 ) : spell_data_t::not_found() );
  // Cooldown is handled in the spell
  buff.natures_swiftness     = buff_creator_t( this, "natures_swiftness", talent.natures_swiftness )
                               .cd( timespan_t::zero() );

  buff.glyph_of_innervate  = buff_creator_t( this, "glyph_of_innervate" , spell_data_t::nil() )
                             .chance( glyph.innervate -> ok() );
  buff.revitalize          = buff_creator_t( this, "revitalize"         , spell_data_t::nil() );
}

// druid_t::init_scaling ====================================================

void druid_t::init_scaling()
{
  player_t::init_scaling();

  equipped_weapon_dps = main_hand_weapon.damage / main_hand_weapon.swing_time.total_seconds();

  scales_with[ STAT_WEAPON_SPEED  ] = false;

  if ( specialization() == DRUID_FERAL )
    scales_with[ STAT_SPIRIT ] = false;

  // Balance of Power treats Spirit like Spell Hit Rating
  // if ( spec.balance_of_power -> ok() && sim -> scaling -> scale_stat == STAT_SPIRIT )
  // {
  //   double v = sim -> scaling -> scale_value;
  //   if ( ! sim -> scaling -> positive_scale_delta )
  //   {
  //     invert_scaling = 1;
  //     initial.attribute[ ATTR_SPIRIT ] -= v * 2;
  //   }
  // }

  if ( specialization() == DRUID_BALANCE )
    scales_with[ STAT_SPIRIT ] = false;

  // Save a copy of the weapon
  caster_form_weapon = main_hand_weapon;
}

// druid_t::init_gains ======================================================

void druid_t::init_gains()
{
  player_t::init_gains();

  gain.bear_melee            = get_gain( "bear_melee"            );
  gain.bear_form             = get_gain( "bear_form"             );
  gain.energy_refund         = get_gain( "energy_refund"         );
  gain.eclipse               = get_gain( "eclipse"               );
  gain.enrage                = get_gain( "enrage"                );
  gain.frenzied_regeneration = get_gain( "frenzied_regeneration" );
  gain.glyph_ferocious_bite  = get_gain( "glyph_ferocious_bite"  );
  gain.glyph_of_innervate    = get_gain( "glyph_of_innervate"    );
  gain.lotp_health           = get_gain( "lotp_health"           );
  gain.lotp_mana             = get_gain( "lotp_mana"             );
  gain.mangle                = get_gain( "mangle"                );
  gain.omen_of_clarity       = get_gain( "omen_of_clarity"       );
  gain.primal_fury           = get_gain( "primal_fury"           );
  gain.revitalize            = get_gain( "revitalize"            );
  gain.soul_of_the_forest    = get_gain( "soul_of_the_forest"    );
  gain.tigers_fury           = get_gain( "tigers_fury"           );
}

// druid_t::init_procs ======================================================

void druid_t::init_procs()
{
  player_t::init_procs();

  proc.primal_fury              = get_proc( "primal_fury"            );
  proc.revitalize               = get_proc( "revitalize"             );
  proc.unaligned_eclipse_gain   = get_proc( "unaligned_eclipse_gain" );
  proc.wrong_eclipse_wrath      = get_proc( "wrong_eclipse_wrath"    );
  proc.wrong_eclipse_starfire   = get_proc( "wrong_eclipse_starfire" );
  proc.combo_points             = get_proc( "combo_points" );
  proc.combo_points_wasted      = get_proc( "combo_points_wasted" );
  proc.shooting_stars_wasted    = get_proc( "Shooting Stars overflow (buff already up)" );
  proc.tier15_2pc_melee         = get_proc( "tier15_2pc_melee"       );
}

// druid_t::init_benefits ===================================================

void druid_t::init_benefits()
{
  player_t::init_benefits();
}

// druid_t::init_rng ========================================================

void druid_t::init_rng()
{
  player_t::init_rng();

  rng.euphoria         = get_rng( "euphoria"         );
  rng.mangle           = get_rng( "mangle"           );
  rng.revitalize       = get_rng( "revitalize"       );
  rng.tier15_2pc_melee = get_rng( "tier15_2pc_melee" );
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
    clear_action_priority_lists();

    std::string& precombat_list = get_action_priority_list( "precombat" ) -> action_list_str;

    if ( level >= 80 )
    {
      if ( sim -> allow_flasks )
      {
        // Flask
        precombat_list = "flask,type=";
        if ( ( specialization() == DRUID_FERAL && primary_role() == ROLE_ATTACK ) || primary_role() == ROLE_ATTACK )
          precombat_list += ( ( level > 85 ) ? "spring_blossoms" : "winds" );
        else if ( ( specialization() == DRUID_GUARDIAN && primary_role() == ROLE_TANK ) || primary_role() == ROLE_TANK )
          precombat_list += ( ( level > 85 ) ? "earth" : "steelskin" );
        else
          precombat_list += ( ( level > 85 ) ? "warm_sun" : "draconic_mind" );
      }

      if ( sim -> allow_food )
      {
        // Food
        precombat_list += "/food,type=";

        if ( specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION )
          precombat_list += ( level > 85 ) ? "mogu_fish_stew" : "seafood_magnifique_feast";
        else
          precombat_list += ( level > 85 ) ? "sea_mist_rice_noodles" : "seafood_magnifique_feast";
      }
    }

    // MotW
    precombat_list += "/mark_of_the_wild,if=!aura.str_agi_int.up";

    if ( level >= 90 )
    {
      precombat_list += "/healing_touch,if=!buff.dream_of_cenarius_damage.up&talent.dream_of_cenarius.enabled";
    }

    // Symbiosis
    if ( level >= 87 )
    {
      if ( util::str_compare_ci( sim -> fight_style, "raiddummy" ) == false )
      {
        if ( ( specialization() == DRUID_FERAL && primary_role() == ROLE_ATTACK ) || primary_role() == ROLE_ATTACK )
        {
        }
      }
    }

    // Forms
    if ( ( specialization() == DRUID_FERAL && primary_role() == ROLE_ATTACK ) || primary_role() == ROLE_ATTACK )
    {
      precombat_list += "/cat_form";
      precombat_list += "/savage_roar";
    }
    else if ( primary_role() == ROLE_TANK )
      precombat_list += "/bear_form";
    else if ( specialization() == DRUID_BALANCE && ( primary_role() == ROLE_DPS || primary_role() == ROLE_SPELL ) )
    {
      precombat_list += "/moonkin_form";
    }

    // Force of Nature
    if ( talent.force_of_nature -> ok() && ( primary_role() == ROLE_DPS || primary_role() == ROLE_ATTACK ) )
      precombat_list += "/treants";

    // Snapshot stats
    precombat_list += "/snapshot_stats";

    if ( primary_role() == ROLE_ATTACK || primary_role() == ROLE_TANK )
    {
      action_list_str += "/auto_attack";
    }

    if ( level >= 80 )
    {
      if ( sim -> allow_potions )
      {
        // Prepotion
        if ( ( specialization() == DRUID_FERAL || specialization() == DRUID_GUARDIAN ) && ( primary_role() == ROLE_ATTACK || primary_role() == ROLE_TANK ) )
          precombat_list += ( level > 85 ) ? "/virmens_bite_potion" : "/tolvir_potion";
        else
          precombat_list += ( level > 85 ) ? "/jade_serpent_potion" : "/volcanic_potion";

        // Combat Potion + usage
        if ( specialization() == DRUID_BALANCE && ( primary_role() == ROLE_DPS || primary_role() == ROLE_SPELL ) )
        {
          action_list_str += ( level > 85 ) ? "/jade_serpent_potion" : "/volcanic_potion";
          action_list_str += ",if=buff.bloodlust.react|target.time_to_die<=40|buff.celestial_alignment.up";
        }
        else if ( specialization() == DRUID_RESTORATION && primary_role() == ROLE_HEAL )
        {
          action_list_str += ( level > 85 ) ? "/jade_serpent_potion" : "/volcanic_potion";
          action_list_str += ",if=buff.bloodlust.react|target.time_to_die<=40";
        }
      }
    }


    if ( specialization() == DRUID_FERAL && primary_role() == ROLE_ATTACK )
    {
      bool hasRune;
      if ( util::str_compare_ci( items[ SLOT_TRINKET_1 ].name(), "rune_of_reorigination" ) || util::str_compare_ci( items[ SLOT_TRINKET_2 ].name(), "rune_of_reorigination" ) )
      {
        hasRune = true;
      }
      else
      {
        hasRune = false;
      }

      std::string& filler_list = get_action_priority_list( "filler" ) -> action_list_str;

      if ( talent.dream_of_cenarius -> ok() )
      {
        action_list_str += "/swap_action_list,name=aoe,if=active_enemies>=5";
        action_list_str += "/skull_bash_cat";
        action_list_str += init_use_racial_actions();
        action_list_str += init_use_profession_actions();
        action_list_str += "/healing_touch,if=buff.predatory_swiftness.up&buff.predatory_swiftness.remains<=1.5&buff.dream_of_cenarius_damage.down";
        action_list_str += "/savage_roar,if=buff.savage_roar.down";
        action_list_str += "/faerie_fire,cycle_targets=1,if=debuff.weakened_armor.stack<3";
        action_list_str += "/healing_touch,if=buff.predatory_swiftness.up&combo_points>=4&buff.dream_of_cenarius_damage.stack<2";
        if ( talent.natures_swiftness -> ok() )
          action_list_str += "/healing_touch,if=buff.natures_swiftness.up";
        if ( talent.incarnation -> ok() )
          action_list_str += "/incarnation,if=energy<=35&!buff.omen_of_clarity.react&cooldown.tigers_fury.remains=0&cooldown.berserk.remains=0";
        action_list_str += init_use_item_actions( ",sync=tigers_fury" );
        action_list_str += "/tigers_fury,if=(energy<=35&!buff.omen_of_clarity.react)";
        if ( talent.incarnation -> ok() )
          action_list_str += "|buff.king_of_the_jungle.up";
        action_list_str += "/berserk,if=buff.tigers_fury.up|(target.time_to_die<18&cooldown.tigers_fury.remains>6)";
        action_list_str += "/ferocious_bite,if=combo_points>=1&dot.rip.ticking&dot.rip.remains<=3&target.health.pct<=25";
        action_list_str += "/thrash_cat,if=target.time_to_die>=6&buff.omen_of_clarity.react&dot.thrash_cat.remains<3";
        action_list_str += "/ferocious_bite,if=(target.time_to_die<=4&combo_points>=5)|(target.time_to_die<=1&combo_points>=3)";
        action_list_str += "/savage_roar,if=buff.savage_roar.remains<=3&combo_points>0&target.health.pct<25";
        if ( hasRune )
        {
          action_list_str += "/virmens_bite_potion,if=(combo_points>=5&target.health.pct<=25&buff.rune_of_reorigination.up&buff.dream_of_cenarius_damage.up)|target.time_to_die<=40";
          action_list_str += "/natures_swiftness,if=enabled&combo_points>=5&target.health.pct<=25&buff.dream_of_cenarius_damage.down&buff.predatory_swiftness.down&buff.rune_of_reorigination.up&target.time_to_die>30";
          action_list_str += "/rip,line_cd=30,if=combo_points>=5&target.health.pct<=25&buff.virmens_bite_potion.up&buff.dream_of_cenarius_damage.up&buff.rune_of_reorigination.up&target.time_to_die>30";
        }
        else
        {
          action_list_str += "/virmens_bite_potion,if=(combo_points>=5&target.health.pct<=25&buff.dream_of_cenarius_damage.up)|target.time_to_die<=40";
          if ( talent.natures_swiftness -> ok() )
            action_list_str += "/natures_swiftness,if=buff.dream_of_cenarius_damage.down&buff.predatory_swiftness.down&combo_points>=5&target.health.pct<=25";
          action_list_str += "/rip,line_cd=30,if=combo_points>=5&buff.virmens_bite_potion.up&buff.dream_of_cenarius_damage.up&target.health.pct<=25&target.time_to_die>30";
        }
        if ( hasRune )
          action_list_str += "/natures_swiftness,if=enabled&buff.dream_of_cenarius_damage.down&buff.predatory_swiftness.down&combo_points>=5&target.health.pct<=25";
        action_list_str += "/pool_resource,wait=0.25,if=combo_points>=5&dot.rip.ticking&target.health.pct<=25&((energy<50&buff.berserk.down)|(energy<25&buff.berserk.remains>1))";
        action_list_str += "/ferocious_bite,if=combo_points>=5&dot.rip.ticking&target.health.pct<=25";
        if ( hasRune )
        {
          if ( talent.natures_swiftness -> ok() )
            action_list_str += "/natures_swiftness,if=combo_points>=5&buff.rune_of_reorigination.react&buff.rune_of_reorigination.remains>1";
          action_list_str += "/rip,if=combo_points>=5&buff.rune_of_reorigination.react";
        }
        action_list_str += "/rip,if=combo_points>=5&target.time_to_die>=6&dot.rip.remains<2&buff.dream_of_cenarius_damage.up";
        // action_list_str += "/rip,if=combo_points>=5&target.time_to_die>=6&dot.rip.remains<6.0&buff.dream_of_cenarius_damage.up&dot.rip.multiplier<=tick_multiplier";
        if ( talent.natures_swiftness -> ok() )
          action_list_str += "/natures_swiftness,if=buff.dream_of_cenarius_damage.down&buff.predatory_swiftness.down&combo_points>=5&dot.rip.remains<3&(buff.berserk.up|dot.rip.remains+1.9<=cooldown.tigers_fury.remains)";
        action_list_str += "/rip,if=combo_points>=5&target.time_to_die>=6&dot.rip.remains<2&(buff.berserk.up|dot.rip.remains+1.9<=cooldown.tigers_fury.remains)";
        action_list_str += "/savage_roar,if=buff.savage_roar.remains<=3&combo_points>0&buff.savage_roar.remains+2>dot.rip.remains";
        action_list_str += "/savage_roar,if=buff.savage_roar.remains<=6&combo_points>=5&buff.savage_roar.remains+2<=dot.rip.remains";
        action_list_str += "/pool_resource,wait=0.25,if=combo_points>=5&((energy<50&buff.berserk.down)|(energy<25&buff.berserk.remains>1))&dot.rip.remains>=6.5";
        action_list_str += "/ferocious_bite,if=combo_points>=5&dot.rip.remains>6";
        if ( hasRune )
        {
          action_list_str += "/rake,if=buff.rune_of_reorigination.react";//&$(rake_ratio)>=1";
          // action_list_str += "/rake,if=buff.rune_of_reorigination.react&dot.rake.remains<9&(buff.rune_of_reorigination.remains<=1.5)";
        }
        action_list_str += "/rake,if=dot.rake.remains<9&buff.dream_of_cenarius_damage.up"; // threshold is more aggressive (to 9 from 6) to account for lack of clipping otherwise
        // action_list_str += "/rake,if=dot.rake.remains<3&tick_multiplier%dot.rake.multiplier>1.12";
        action_list_str += "/rake,if=dot.rake.remains<3";
        action_list_str += "/pool_resource,wait=0.25,for_next=1";
        action_list_str += "/thrash_cat,if=dot.thrash_cat.remains<3&target.time_to_die>=6&(dot.rip.remains>=4|buff.berserk.up)";
        action_list_str += "/run_action_list,name=filler,if=buff.omen_of_clarity.react";
        action_list_str += "/run_action_list,name=filler,if=(combo_points<5&dot.rip.remains<3)|(combo_points=0&buff.savage_roar.remains<2)";
        action_list_str += "/run_action_list,name=filler,if=buff.predatory_swiftness.remains>1";
        action_list_str += "/run_action_list,name=filler,if=target.time_to_die<=8.5";
        action_list_str += "/run_action_list,name=filler,if=buff.tigers_fury.up|buff.berserk.up";
        action_list_str += "/run_action_list,name=filler,if=cooldown.tigers_fury.remains<=3";
        action_list_str += "/run_action_list,name=filler,if=energy.time_to_max<=1";
        if ( talent.force_of_nature -> ok() )
          action_list_str += "/treants";
      }
      else
      {
        action_list_str += "/swap_action_list,name=aoe,if=active_enemies>=5";
        action_list_str += "/skull_bash_cat";
        action_list_str += init_use_racial_actions();
        action_list_str += init_use_profession_actions();
        action_list_str += "/savage_roar,if=buff.savage_roar.down";
        action_list_str += "/faerie_fire,cycle_targets=1,if=debuff.weakened_armor.stack<3";
        if ( talent.incarnation -> ok() )
          action_list_str += "/incarnation,if=energy<=35&!buff.omen_of_clarity.react&cooldown.tigers_fury.remains=0&cooldown.berserk.remains=0";
        action_list_str += init_use_item_actions( ",sync=tigers_fury" );
        action_list_str += "/tigers_fury,if=(energy<=35&!buff.omen_of_clarity.react)";
        if ( talent.incarnation -> ok() )
          action_list_str += "|buff.king_of_the_jungle.up";
        action_list_str += "/berserk,if=buff.tigers_fury.up|(target.time_to_die<18&cooldown.tigers_fury.remains>6)";
        if ( talent.natures_vigil -> ok() )
          action_list_str += "/natures_vigil,if=enabled&(buff.tigers_fury.up|(target.time_to_die<35&cooldown.tigers_fury.remains>6))";
        action_list_str += "/ferocious_bite,if=combo_points>=1&dot.rip.ticking&dot.rip.remains<=3&target.health.pct<=25";
        action_list_str += "/thrash_cat,if=target.time_to_die>=6&buff.omen_of_clarity.react&dot.thrash_cat.remains<3";
        action_list_str += "/ferocious_bite,if=(target.time_to_die<=4&combo_points>=5)|(target.time_to_die<=1&combo_points>=3)";
        action_list_str += "/savage_roar,if=buff.savage_roar.remains<=3&combo_points>0&target.health.pct<25";
        if ( hasRune )
        {
          action_list_str += "/virmens_bite_potion,if=(combo_points>=5&target.health.pct<=25&buff.rune_of_reorigination.up)|target.time_to_die<=40";
          action_list_str += "/rip,line_cd=30,if=combo_points>=5&target.health.pct<=25&buff.rune_of_reorigination.up&buff.virmens_bite_potion.up";
        }
        else
        {
          action_list_str += "/virmens_bite_potion,if=(buff.berserk.up&target.health.pct<=25)|target.time_to_die<=40";
          // action_list_str += "/rip,if=combo_points>=5&tick_multiplier%dot.rip.multiplier>1.14&target.health.pct<=25&target.time_to_die>30";
        }
        action_list_str += "/ferocious_bite,if=combo_points>=5&dot.rip.ticking&target.health.pct<=25";
        action_list_str += "/rip,if=combo_points>=5&target.time_to_die>=6&dot.rip.remains<2&(buff.berserk.up|dot.rip.remains+1.9<=cooldown.tigers_fury.remains)";
        if ( hasRune )
          action_list_str += "/rip,if=combo_points>=5&buff.rune_of_reorigination.react";
        action_list_str += "/savage_roar,if=buff.savage_roar.remains<=3&combo_points>0&buff.savage_roar.remains+2>dot.rip.remains";
        action_list_str += "/savage_roar,if=buff.savage_roar.remains<=6&combo_points>=5&buff.savage_roar.remains+2<=dot.rip.remains";
        action_list_str += "/ferocious_bite,if=combo_points>=5&(dot.rip.remains>10|(dot.rip.remains>6&buff.berserk.up))&dot.rip.ticking";
        if ( hasRune )
        {
          action_list_str += "/rake,if=buff.rune_of_reorigination.react";//&$(rake_ratio)>=1";
          // action_list_str += "/rake,if=buff.rune_of_reorigination.react&dot.rake.remains<9&(buff.rune_of_reorigination.remains<=1.5)";
        }
        action_list_str += "/rake,if=dot.rake.remains<9&buff.tigers_fury.up";
        action_list_str += "/rake,if=dot.rake.remains<3&(buff.berserk.up|(cooldown.tigers_fury.remains+0.8)>=dot.rake.remains|energy>60)";
        action_list_str += "/pool_resource,wait=0.1,for_next=1";
        action_list_str += "/thrash_cat,if=dot.thrash_cat.remains<3&target.time_to_die>=6&(dot.rip.remains>=4|buff.berserk.up)";
        action_list_str += "/run_action_list,name=filler,if=buff.omen_of_clarity.react";
        action_list_str += "/run_action_list,name=filler,if=(combo_points<5&dot.rip.remains<3.0)|(combo_points=0&buff.savage_roar.remains<2)";
        action_list_str += "/run_action_list,name=filler,if=target.time_to_die<=8.5";
        action_list_str += "/run_action_list,name=filler,if=buff.tigers_fury.up|buff.berserk.up";
        action_list_str += "/run_action_list,name=filler,if=cooldown.tigers_fury.remains<=3.0";
        action_list_str += "/run_action_list,name=filler,if=energy.time_to_max<=1.0";
        if ( talent.soul_of_the_forest -> ok() )
          action_list_str += "/run_action_list,name=filler,if=combo_points<5";
        if ( talent.force_of_nature -> ok() )
          action_list_str += "/treants";
        /*
        if ( talent.natures_vigil -> ok() )
        {
          if ( talent.natures_swiftness -> ok() )
            action_list_str += "/natures_swiftness,if=buff.natures_vigil.up&!buff.berserk.up&!buff.predatory_swiftness.up";
          else if ( talent.renewal -> ok() )
            action_list_str += "/renewal,if=buff.natures_vigil.up";
          else if ( talent.cenarion_ward -> ok() )
            action_list_str += "/cenarion_ward,if=buff.natures_vigil.up";
          action_list_str += "/healing_touch,if=buff.natures_vigil.up&(buff.predatory_swiftness.up|buff.natures_swiftness.up)&!buff.berserk.up";
        }
        */
      }

      filler_list += "/thrash_cat,if=dot.thrash_cat.remains<3&target.time_to_die>=6&combo_points>=5";
      if ( talent.incarnation -> ok() )
      {
        filler_list += "/ravage";
        filler_list += "/shred,if=(buff.omen_of_clarity.react|buff.berserk.up|energy.regen>=15)&buff.king_of_the_jungle.down";
        filler_list += "/mangle_cat,if=buff.king_of_the_jungle.down";
      }
      else
      {
        filler_list += "/shred,if=buff.omen_of_clarity.react|buff.berserk.up|energy.regen>=15";
        filler_list += "/mangle_cat";
      }

      std::string& aoe_list = get_action_priority_list( "aoe" ) -> action_list_str;

      aoe_list += "/swap_action_list,name=default,if=active_enemies<5";
      aoe_list += "/auto_attack";
      if ( talent.dream_of_cenarius -> ok() )
      {
        aoe_list += "/healing_touch,if=buff.predatory_swiftness.up&buff.predatory_swiftness.remains<=1.5&buff.dream_of_cenarius_damage.down";
        if ( talent.natures_swiftness -> ok() )
          aoe_list += "/healing_touch,if=buff.natures_swiftness.up";
      }
      aoe_list += "/faerie_fire,cycle_targets=1,if=debuff.weakened_armor.stack<3";
      aoe_list += "/savage_roar,if=buff.savage_roar.down|(buff.savage_roar.remains<3&combo_points>0)";
      aoe_list += init_use_item_actions( ",sync=tigers_fury" );
      aoe_list += init_use_racial_actions( ",sync=tigers_fury" );
      aoe_list += init_use_profession_actions( ",sync=tigers_fury" );
      aoe_list += "/tigers_fury,if=energy<=35&!buff.omen_of_clarity.react";
      aoe_list += "/berserk,if=buff.tigers_fury.up";
      if ( hasRune )
      {
        if ( talent.dream_of_cenarius -> ok() )
        {
          aoe_list += "/healing_touch,if=buff.rune_of_reorigination.remains<3&buff.predatory_swiftness.up";
          if ( talent.natures_swiftness -> ok() )
            aoe_list += "/natures_swiftness,if=buff.rune_of_reorigination.remains<3&buff.predatory_swiftness.down&buff.dream_of_cenarius_damage.down";
          aoe_list += "/pool_resource,wait=0.1,for_next=1";
          if ( talent.natures_swiftness -> ok() )
            aoe_list += "/thrash_cat,if=buff.rune_of_reorigination.up&(dot.thrash_cat.multiplier<=tick_multiplier|buff.predatory_swiftness.up|cooldown.natures_swiftness.remains=0)";
          else
            aoe_list += "/thrash_cat,if=buff.rune_of_reorigination.up&(dot.thrash_cat.multiplier<=tick_multiplier|buff.predatory_swiftness.up)";
        }
        else
        {
          aoe_list += "/pool_resource,wait=0.1,for_next=1";
          aoe_list += "/thrash_cat,if=buff.rune_of_reorigination.up";
        }
      }
      if ( talent.dream_of_cenarius -> ok() )
      {
        aoe_list += "/healing_touch,if=buff.predatory_swiftness.up&(dot.thrash_cat.remains<5|(buff.tigers_fury.remains>1&dot.thrash_cat.remains<9&energy>=40))";
        if ( talent.natures_swiftness -> ok() )
          aoe_list += "/natures_swiftness,if=buff.predatory_swiftness.down&buff.dream_of_cenarius_damage.down&(dot.thrash_cat.remains<5|(buff.tigers_fury.remains>1&dot.thrash_cat.remains<9&energy>=40))";
      }
      aoe_list += "/thrash_cat,if=buff.tigers_fury.up&dot.thrash_cat.remains<9";
      aoe_list += "/pool_resource,wait=0.1,for_next=1";
      aoe_list += "/thrash_cat,if=dot.thrash_cat.remains<3";
      aoe_list += "/savage_roar,if=buff.savage_roar.remains<9&combo_points>=5";
      aoe_list += "/rip,if=combo_points>=5";
      if ( hasRune )
        aoe_list += "/rake,cycle_targets=1,if=(active_enemies<8|buff.rune_of_reorigination.up)&dot.rake.remains<3&target.time_to_die>=15";
      else
        aoe_list += "/rake,cycle_targets=1,if=active_enemies<8&dot.rake.remains<3&target.time_to_die>=15";
      aoe_list += "/swipe_cat,if=buff.savage_roar.remains<=5";
      aoe_list += "/swipe_cat,if=(buff.tigers_fury.up|buff.berserk.up)";
      aoe_list += "/swipe_cat,if=cooldown.tigers_fury.remains<3";
      aoe_list += "/swipe_cat,if=buff.omen_of_clarity.react";
      aoe_list += "/swipe_cat,if=energy.time_to_max<=1";
    }
    else if ( specialization() == DRUID_BALANCE && ( primary_role() == ROLE_SPELL || primary_role() == ROLE_DPS ) )
    {
      action_list_str += "/starfall,if=!buff.starfall.up";
      action_list_str += "/treants,if=talent.force_of_nature.enabled";
      if ( race == RACE_TROLL && level >= 68 )
        action_list_str += "/berserking,if=buff.celestial_alignment.up";
      else
        action_list_str += init_use_racial_actions();
      action_list_str += init_use_item_actions( ",if=buff.celestial_alignment.up|cooldown.celestial_alignment.remains>30" );
      action_list_str += init_use_profession_actions( ",if=buff.celestial_alignment.up|cooldown.celestial_alignment.remains>30" );
      action_list_str += "/wild_mushroom_detonate,moving=0,if=buff.wild_mushroom.stack>0&buff.solar_eclipse.up";
      action_list_str += "/natures_swiftness,if=talent.natures_swiftness.enabled&talent.dream_of_cenarius.enabled";
      action_list_str += "/healing_touch,if=talent.dream_of_cenarius.enabled&!buff.dream_of_cenarius_damage.up&mana.pct>25";
      action_list_str += "/incarnation,if=talent.incarnation.enabled&(buff.lunar_eclipse.up|buff.solar_eclipse.up)";
      action_list_str += "/celestial_alignment,if=(!buff.lunar_eclipse.up&!buff.solar_eclipse.up)&(buff.chosen_of_elune.up|!talent.incarnation.enabled|cooldown.incarnation.remains>10)";
      action_list_str += "/natures_vigil,if=talent.natures_vigil.enabled";
      add_action( "Starsurge", "if=buff.shooting_stars.react&(active_enemies<5|!buff.solar_eclipse.up)" );
      action_list_str += "/moonfire,cycle_targets=1,if=buff.lunar_eclipse.up&(remains<(buff.natures_grace.remains-2+2*set_bonus.tier14_4pc_caster))";
      action_list_str += "/sunfire,cycle_targets=1,if=buff.solar_eclipse.up&(remains<(buff.natures_grace.remains-2+2*set_bonus.tier14_4pc_caster))";
      action_list_str += "/hurricane,if=active_enemies>4&buff.solar_eclipse.up&buff.natures_grace.up";
      action_list_str += "/moonfire,cycle_targets=1,if=active_enemies<5&(remains<(buff.natures_grace.remains-2+2*set_bonus.tier14_4pc_caster))";
      action_list_str += "/sunfire,cycle_targets=1,if=active_enemies<5&(remains<(buff.natures_grace.remains-2+2*set_bonus.tier14_4pc_caster))";
      action_list_str += "/hurricane,if=active_enemies>5&buff.solar_eclipse.up&mana.pct>25";
      action_list_str += "/moonfire,cycle_targets=1,if=buff.lunar_eclipse.up&ticks_remain<2";
      action_list_str += "/sunfire,cycle_targets=1,if=buff.solar_eclipse.up&ticks_remain<2";
      action_list_str += "/hurricane,if=active_enemies>4&buff.solar_eclipse.up&mana.pct>25";
      action_list_str += "/starsurge,if=cooldown_react";
      action_list_str += "/starfire,if=buff.celestial_alignment.up&cast_time<buff.celestial_alignment.remains";
      action_list_str += "/wrath,if=buff.celestial_alignment.up&cast_time<buff.celestial_alignment.remains";
      action_list_str += "/starfire,if=eclipse_dir=1|(eclipse_dir=0&eclipse>0)";
      action_list_str += "/wrath,if=eclipse_dir=-1|(eclipse_dir=0&eclipse<=0)";
      action_list_str += "/moonfire,moving=1,cycle_targets=1,if=ticks_remain<2";
      action_list_str += "/sunfire,moving=1,cycle_targets=1,if=ticks_remain<2";
      action_list_str += "/wild_mushroom,moving=1,if=buff.wild_mushroom.stack<buff.wild_mushroom.max_stack";
      action_list_str += "/starsurge,moving=1,if=buff.shooting_stars.react";
      action_list_str += "/moonfire,moving=1,if=buff.lunar_eclipse.up";
      action_list_str += "/sunfire,moving=1";
    }
    else if ( specialization() == DRUID_GUARDIAN && primary_role() == ROLE_TANK )
    {
      action_list_str += init_use_racial_actions();
      action_list_str += "/skull_bash_bear";
      action_list_str += init_use_item_actions();
      action_list_str += init_use_profession_actions();
      action_list_str += "/healing_touch,if=buff.natures_swiftness.up";
      add_action("Renewal", "if=health.pct<50");
      action_list_str += "/survival_instincts"; // PH
      action_list_str += "/barkskin,if=buff.survival_instincts.down"; // PH
      action_list_str += "/savage_defense";
      action_list_str += "/natures_vigil,if=buff.son_of_ursoc.up|cooldown.incarnation.remains";
      action_list_str += "/thrash_bear,if=debuff.weakened_blows.remains<3";
      action_list_str += "/maul,if=rage>=90";
      action_list_str += "/lacerate,if=dot.lacerate.ticking&dot.lacerate.remains<3&(buff.son_of_ursoc.up|buff.berserk.up)";
      action_list_str += "/faerie_fire,if=debuff.weakened_armor.stack<3";
      action_list_str += "/thrash_bear,if=dot.thrash_bear.remains<3&(buff.son_of_ursoc.up|buff.berserk.up)";
      action_list_str += "/mangle_bear";
      action_list_str += "/wait,sec=cooldown.mangle_bear.remains,if=cooldown.mangle_bear.remains<=0.5";
      action_list_str += "/natures_swiftness,if=talent.natures_swiftness.enabled&health.pct<60";
      add_action("Cenarion Ward", "if=health.pct<70");
      action_list_str += "/berserk";
      action_list_str += "/incarnation,if=talent.incarnation.enabled";
      action_list_str += "/lacerate,if=dot.lacerate.remains<3|buff.lacerate.stack<3";
      action_list_str += "/thrash_bear,if=dot.thrash_bear.remains<2";
      action_list_str += "/lacerate";
      action_list_str += "/faerie_fire,if=dot.thrash_bear.remains>6";
      action_list_str += "/thrash_bear";
    }
    else if ( specialization() == DRUID_RESTORATION && primary_role() == ROLE_HEAL )
    {
      action_list_str += init_use_racial_actions();
      action_list_str += init_use_item_actions();
      action_list_str += init_use_profession_actions();
      action_list_str += "/innervate,if=mana.pct<90";
      if ( talent.incarnation -> ok() )
        action_list_str += "/incarnation";
      action_list_str += "/healing_touch,if=buff.omen_of_clarity.up";
      action_list_str += "/rejuvenation,if=!ticking|remains<tick_time";
      action_list_str += "/lifebloom,if=debuff.lifebloom.stack<3";
      action_list_str += "/swiftmend";
      action_list_str += "/healing_touch,if=buff.tree_of_life.up&mana.pct>=5";
      action_list_str += "/healing_touch,if=buff.tree_of_life.down&mana.pct>=30";
      action_list_str += "/nourish";
    }
    // Specless (or speced non-main role) druid who has a primary role of caster
    else if ( primary_role() == ROLE_SPELL )
    {
      action_list_str += init_use_racial_actions();
      action_list_str += init_use_item_actions();
      action_list_str += init_use_profession_actions();
      action_list_str += "/innervate,if=mana.pct<90";
      action_list_str += "/moonfire,if=!dot.moonfire.ticking";
      action_list_str += "/wrath";
    }
    // Specless (or speced non-main role) druid who has a primary role of a melee
    else if ( primary_role() == ROLE_ATTACK )
    {
      action_list_str += "/faerie_fire,if=debuff.weakened_armor.stack<3";
      action_list_str += init_use_racial_actions();
      action_list_str += init_use_item_actions();
      action_list_str += init_use_profession_actions();
      action_list_str += "/rake,if=!ticking|ticks_remain<2";
      action_list_str += "/mangle_cat";
      action_list_str += "/ferocious_bite,if=combo_points>=5";
    }
    // Specless (or speced non-main role) druid who has a primary role of a healer
    else if ( primary_role() == ROLE_HEAL )
    {
      action_list_str += init_use_racial_actions();
      action_list_str += init_use_item_actions();
      action_list_str += init_use_profession_actions();
      action_list_str += "/innervate,if=mana.pct<90";
      action_list_str += "/rejuvenation,if=!ticking|remains<tick_time";
      action_list_str += "/healing_touch,if=mana.pct>=30";
    }
    action_list_default = 1;
  }

  player_t::init_actions();
}

// druid_t::reset ===========================================================

void druid_t::reset()
{
  player_t::reset();

  inflight_starsurge = false;

  eclipse_bar_value     = 0;
  eclipse_bar_direction = 0;
  base_gcd = timespan_t::from_seconds( 1.5 );

  // Restore main hand attack / weapon to normal state
  main_hand_attack = 0;
  main_hand_weapon = caster_form_weapon;

  for ( size_t i=0; i < sim -> actor_list.size(); i++ )
  {
    druid_td_t* td = target_data[ sim -> actor_list[ i ] ];
    if ( td ) td -> reset();
  }
}

// druid_t::regen ===========================================================

void druid_t::regen( timespan_t periodicity )
{
  if ( buff.glyph_of_innervate -> check() )
    resource_gain( RESOURCE_MANA, buff.glyph_of_innervate -> value() * periodicity.total_seconds(), gain.glyph_of_innervate );
  if ( buff.enrage -> up() )
    resource_gain( RESOURCE_RAGE, 1.0 * periodicity.total_seconds(), gain.enrage );

  player_t::regen( periodicity );

  // player_t::regen() only regens your primary resource, so we need to account for that here
  if ( primary_resource() != RESOURCE_MANA && mana_regen_per_second() )
    resource_gain( RESOURCE_MANA, mana_regen_per_second() * periodicity.total_seconds(), gains.mp5_regen );
  if ( primary_resource() != RESOURCE_ENERGY && energy_regen_per_second() )
    resource_gain( RESOURCE_ENERGY, energy_regen_per_second() * periodicity.total_seconds(), gains.energy_regen );
}

// druid_t::available =======================================================

timespan_t druid_t::available()
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

  // Moonkins and Resto can precast wild mushrooms without aggroing the boss
  if ( preplant_mushrooms )
    buff.wild_mushroom -> trigger( buff.wild_mushroom -> max_stack() );

  if ( specialization() == DRUID_BALANCE )
  {
    int starting_eclipse = initial_eclipse;
    if ( starting_eclipse == 65535 )
      starting_eclipse = default_initial_eclipse;

    // If we start in the +, assume we want to to towards +100
    // Start at +100, trigger solar and put direction towards -
    // If we start in the -, assume we want to go towards -100
    // Start at -100, trigger lunar and put direction towards +
    // No Nature's Grace if we start the fight with eclipse

    if ( starting_eclipse >= 0 )
    {
      eclipse_bar_value = std::min( spec.eclipse -> effectN( 1 ).base_value(), starting_eclipse );
      if ( eclipse_bar_value == spec.eclipse -> effectN( 1 ).base_value() )
      {
        buff.eclipse_solar -> trigger();
        eclipse_bar_direction = -1;
      }
    }
    else
    {
      eclipse_bar_value = std::max( spec.eclipse -> effectN( 2 ).base_value(), starting_eclipse );
      if ( eclipse_bar_value == spec.eclipse -> effectN( 2 ).base_value() )
      {
        buff.eclipse_lunar -> trigger();
        eclipse_bar_direction = 1;
      }
    }
  }
}

// druid_t::invalidate_cache ================================================

void druid_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_AGILITY:
      player_t::invalidate_cache( CACHE_SPELL_POWER );
      break;
    case CACHE_INTELLECT:
      player_t::invalidate_cache( CACHE_AGILITY );
      break;
    case CACHE_MASTERY:
        player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      break;
    default: break;
  }
}

// druid_t::composite_armor_multiplier ======================================

double druid_t::composite_armor_multiplier()
{
  double a = player_t::composite_armor_multiplier();

  if ( buff.bear_form -> check() )
  {
    // Increases the armor bonus of Bear Form to 330%
    // TODO: http://mop.wowhead.com/spell=5487 spell tooltip => +120% armor
    // But the actual spell data suggests +65% armor
    if ( spec.thick_hide -> ok() )
      a *= 1.0 + spec.thick_hide -> effectN( 2 ).percent();
    else
      a *= 1.0 + buff.bear_form -> data().effectN( 3 ).percent();

    // Mastery: Nature's Guardian
    if ( mastery.natures_guardian -> ok() )
      a *= 1.0 + cache.mastery_value();
  }
  return a;
}

// druid_t::composite_attack_haste ==========================================

double druid_t::composite_melee_haste()
{
  double h = player_t::composite_melee_haste();

  if ( buff.bear_form -> up() )
  {
    h *= 1.0 + current.stats.haste_rating / current_rating().attack_haste;
    h /= 1.0 + current.stats.haste_rating * ( 1 + spell.bear_form -> effectN( 4 ).percent() ) / current_rating().attack_haste;
  }

  return h;
}

// druid_t::composite_attack_crit ===========================================

double druid_t::composite_melee_crit()
{
  double c = player_t::composite_melee_crit();

  if ( buff.bear_form -> up() )
    c += current.stats.get_stat( STAT_CRIT_RATING ) * spell.bear_form -> effectN( 4 ).percent() / current_rating().attack_crit;

  return c;
}

// druid_t::composite_player_multiplier =====================================

double druid_t::composite_player_multiplier( school_e school )
{
  double m = player_t::composite_player_multiplier( school );

  if ( buff.natures_vigil -> up() )
  {
    m *= 1.0 + buff.natures_vigil -> data().effectN( 1 ).percent();
  }

  if ( dbc::is_school( school, SCHOOL_PHYSICAL ) && buff.cat_form -> up() )
  {
    m *= 1.0 + buff.tigers_fury -> value();
    m *= 1.0 + buff.savage_roar -> value();
  }

  if ( specialization() == DRUID_BALANCE )
  {
    if ( dbc::is_school( school, SCHOOL_SPELLSTORM ) )
    {
      if ( buff.eclipse_lunar -> up() || buff.eclipse_solar -> up() )
      {
        m *= 1.0 + ( buff.eclipse_lunar -> data().effectN( 1 ).percent()
                     + ( mastery.total_eclipse -> ok() ? cache.mastery_value() : 0.0 ) );
      }
    }
    else if ( dbc::is_school( school, SCHOOL_ARCANE ) )
    {
      if ( buff.eclipse_lunar -> up() )
      {
        m *= 1.0 + ( buff.eclipse_lunar -> data().effectN( 1 ).percent()
                     + ( mastery.total_eclipse -> ok() ? cache.mastery_value() : 0.0 ) );
      }
    }
    else if ( dbc::is_school( school, SCHOOL_NATURE ) )
    {
      if ( buff.eclipse_solar -> up() )
      {
        m *= 1.0 + ( buff.eclipse_solar -> data().effectN( 1 ).percent()
                     + ( mastery.total_eclipse -> ok() ? cache.mastery_value() : 0.0 ) );
      }
    }

    if ( dbc::is_school( school, SCHOOL_ARCANE ) || dbc::is_school( school, SCHOOL_NATURE ) )
    {
      if ( buff.moonkin_form -> check() )
        m *= 1.0 + spell.moonkin_form -> effectN( 3 ).percent();

      // BUG? Incarnation won't apply during CA!
      if ( buff.chosen_of_elune -> up() &&
           ( buff.eclipse_lunar -> check() || buff.eclipse_solar -> check() ) )
      {
        m *= 1.0 + buff.chosen_of_elune -> data().effectN( 1 ).percent();
      }
    }
  }

  return m;
}

double druid_t::composite_player_td_multiplier( school_e school, action_t* a )
{
  double m = player_t::composite_player_td_multiplier( school, a );

  if ( school == SCHOOL_PHYSICAL && mastery.razor_claws -> ok() && buff.cat_form -> up() )
    m *= 1.0 + cache.mastery_value();

  return m;
}

// druid_t::composite_player_heal_multiplier ================================

double druid_t::composite_player_heal_multiplier( school_e school )
{
  double m = player_t::composite_player_heal_multiplier( school );

  if ( buff.natures_vigil -> up() )
  {
    m *= 1.0 + buff.natures_vigil -> data().effectN( 2 ).percent();
  }

  m *= 1.0 + buff.heart_of_the_wild -> heal_multiplier();

  return m;
}

// druid_t::composite_attack_hit ============================================

double druid_t::composite_melee_hit()
{
  double hit = player_t::composite_melee_hit();

  hit += ( cache.spirit() - base.stats.get_stat( STAT_SPIRIT ) ) * ( spec.balance_of_power -> effectN( 1 ).percent() ) / current_rating().spell_hit;

  hit += buff.heart_of_the_wild -> attack_hit_expertise();

  return hit;
}

// druid_t::composite_attack_expertise =====================================

double druid_t::composite_melee_expertise( weapon_t* w )
{
  double exp = player_t::composite_melee_expertise( w );

  exp += buff.heart_of_the_wild -> attack_hit_expertise();

  return exp;
}

// druid_t::composite_spell_hit =============================================

double druid_t::composite_spell_hit()
{
  double hit = player_t::composite_spell_hit();

  hit += ( cache.spirit() - base.stats.get_stat( STAT_SPIRIT ) ) * ( spec.balance_of_power -> effectN( 1 ).percent() ) / current_rating().spell_hit;

  hit += buff.heart_of_the_wild -> spell_hit();

  return hit;
}

// druid_t::composite_spell_haste ===========================================

double druid_t::composite_spell_haste()
{
  double h = player_t::composite_spell_haste();

  if ( buff.bear_form -> up() )
  {
    h *= 1.0 + current.stats.haste_rating / current_rating().spell_haste;
    h /= 1.0 + current.stats.haste_rating * ( 1 + spell.bear_form -> effectN( 4 ).percent() ) / current_rating().spell_haste;
  }

  return h;
}

// druid_t::composite_spell_crit ============================================

double druid_t::composite_spell_crit()
{
  double c = player_t::composite_spell_crit();

  if ( buff.bear_form -> up() )
    c += current.stats.crit_rating * spell.bear_form -> effectN( 4 ).percent() / current_rating().spell_crit;

  return c;
}

// druid_t::composite_spell_power ==========================================

double druid_t::composite_spell_power( school_e school )
{
  double p = player_t::composite_spell_power( school );

  switch ( school )
  {
  case SCHOOL_NATURE:
    if ( spec.nurturing_instinct -> ok() )
      p += spec.nurturing_instinct -> effectN( 1 ).percent() * player_t::composite_attribute( ATTR_AGILITY );
    break;
  default:
    break;
  }

  return p;
}

// druid_t::composite_attribute ============================================

double druid_t::composite_attribute( attribute_e attr )
{
  double a = player_t::composite_attribute( attr );

  switch ( attr )
  {
  case ATTR_AGILITY:
    if ( spec.killer_instinct -> ok() && ( buff.bear_form -> up() || buff.cat_form -> up() ) )
      a += spec.killer_instinct -> effectN( 1 ).percent() * player_t::composite_attribute( ATTR_INTELLECT );
    break;
  default:
    break;
  }

  return a;
}

// druid_t::composite_attribute_multiplier ==================================

double druid_t::composite_attribute_multiplier( attribute_e attr )
{
  double m = player_t::composite_attribute_multiplier( attr );

  // The matching_gear_multiplier is done statically for performance reasons,
  // unfortunately that's before we're in cat form or bear form, so let's compensate here  // Heart of the Wild: +6% INT/AGI
  // http://mop.wowhead.com/spell=17005 Did they just use this?


  switch ( attr )
  {
  case ATTR_STAMINA:
    if ( buff.bear_form -> check() )
      m *= 1.0 + spell.bear_form -> effectN( 2 ).percent();
    if ( talent.heart_of_the_wild -> ok() )
      m *= 1.0 + spell.heart_of_the_wild -> effectN( 1 ).percent();
    break;
  case ATTR_AGILITY:
    if ( talent.heart_of_the_wild -> ok() )
      m *= 1.0 + spell.heart_of_the_wild -> effectN( 1 ).percent();
    m *= 1.0 + buff.heart_of_the_wild -> agility_multiplier();
    break;
  case ATTR_INTELLECT:
    if ( talent.heart_of_the_wild -> ok() )
      m *= 1.0 + spell.heart_of_the_wild -> effectN( 1 ).percent();
    break;
  default:
    break;
  }

  return m;
}

// druid_t::matching_gear_multiplier ========================================

double druid_t::matching_gear_multiplier( attribute_e attr )
{
  unsigned idx;

  switch ( attr )
  {
  case ATTR_AGILITY:
    idx = 1;
    break;
  case ATTR_INTELLECT:
    idx = 2;
    break;
  case ATTR_STAMINA:
    idx = 3;
    break;
  default:
    return 0;
  }

  return spec.leather_specialization -> effectN( idx ).percent();
}

// druid_t::composite_tank_crit =============================================

double druid_t::composite_tank_crit( school_e school )
{
  double c = player_t::composite_tank_crit( school );

  if ( dbc::is_school( school, SCHOOL_PHYSICAL ) )
    c += spec.thick_hide -> effectN( 1 ).percent();

  return c;
}

// druid_t::composite_tank_dodge =============================================

double druid_t::composite_tank_dodge()
{
  double d = player_t::composite_tank_dodge();

  if ( buff.savage_defense -> up() )
  {
    d += buff.savage_defense -> data().effectN( 1 ).percent();
    // TODO: Add Savage Defense dodge bonus granted by 4pT14 Guardian bonus.
  }

  return d;
}

// druid_t::create_expression ===============================================

expr_t* druid_t::create_expression( action_t* a, const std::string& name_str )
{
  if ( name_str == "eclipse" )
  {
    return make_ref_expr( "eclipse", eclipse_bar_value );
  }
  else if ( name_str == "eclipse_dir" )
  {
    return make_ref_expr( "eclipse_dir", eclipse_bar_direction );
  }
  else if ( util::str_compare_ci( name_str, "combo_points" ) )
  {
    // If an action targets the druid, but checks for combo points, check
    // sim -> target instead. Quick fix so HT can use combo_points
    druid_td_t* td = get_target_data( ( a -> target == this ) ? sim -> target : a -> target  );
    return td -> combo_points.count_expr();
  }

  return player_t::create_expression( a, name_str );
}

// druid_t::create_options =================================================

void druid_t::create_options()
{
  player_t::create_options();

  option_t druid_options[] =
  {
    opt_int( "initial_eclipse", initial_eclipse ),
    opt_bool( "preplant_mushrooms", preplant_mushrooms ),
    opt_null()
  };

  option_t::copy( options, druid_options );
}

// druid_t::create_profile =================================================

bool druid_t::create_profile( std::string& profile_str, save_e type, bool save_html )
{
  player_t::create_profile( profile_str, type, save_html );

  if ( type == SAVE_ALL )
  {
    if ( specialization() == DRUID_BALANCE )
    {
      if ( initial_eclipse != 65535 )
      {
        profile_str += "initial_eclipse=";
        profile_str += util::to_string( initial_eclipse );
        profile_str += "\n";
      }
    }
    if ( preplant_mushrooms == 0 )
    {
      profile_str += "preplant_mushrooms=";
      profile_str += util::to_string( preplant_mushrooms );
      profile_str += "\n";
    }
  }

  return true;
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

  if ( strstr( s, "eternal_blossom" ) )
  {
    bool is_caster = ( strstr( s, "cover"          ) ||
                       strstr( s, "shoulderwraps"  ) ||
                       strstr( s, "vestment"       ) ||
                       strstr( s, "leggings"       ) ||
                       strstr( s, "gloves"         ) );

    bool is_melee = ( strstr( s, "headpiece"       ) ||
                      strstr( s, "spaulders"       ) ||
                      strstr( s, "raiment"         ) ||
                      strstr( s, "legguards"       ) ||
                      strstr( s, "grips"           ) );

    bool is_healer = ( strstr( s, "helm"           ) ||
                       strstr( s, "mantle"         ) ||
                       strstr( s, "robes"          ) ||
                       strstr( s, "legwraps"       ) ||
                       strstr( s, "handwraps"      ) );

    bool is_tank   = ( strstr( s, "headguard"      ) ||
                       strstr( s, "shoulderguards" ) ||
                       strstr( s, "tunic"          ) ||
                       strstr( s, "breeches"       ) ||
                       strstr( s, "handguards"     ) );
    if ( is_caster ) return SET_T14_CASTER;
    if ( is_melee  ) return SET_T14_MELEE;
    if ( is_healer ) return SET_T14_HEAL;
    if ( is_tank   ) return SET_T14_TANK;
  }

  if ( strstr( s, "_of_the_haunted_forest" ) )
  {
    bool is_caster = ( strstr( s, "cover"          ) ||
                       strstr( s, "shoulderwraps"  ) ||
                       strstr( s, "vestment"       ) ||
                       strstr( s, "leggings"       ) ||
                       strstr( s, "gloves"         ) );

    bool is_melee = ( strstr( s, "headpiece"       ) ||
                      strstr( s, "spaulders"       ) ||
                      strstr( s, "raiment"         ) ||
                      strstr( s, "legguards"       ) ||
                      strstr( s, "grips"           ) );

    bool is_healer = ( strstr( s, "helm"           ) ||
                       strstr( s, "mantle"         ) ||
                       strstr( s, "robes"          ) ||
                       strstr( s, "legwraps"       ) ||
                       strstr( s, "handwraps"      ) );

    bool is_tank   = ( strstr( s, "headguard"      ) ||
                       strstr( s, "shoulderguards" ) ||
                       strstr( s, "tunic"          ) ||
                       strstr( s, "breeches"       ) ||
                       strstr( s, "handguards"     ) );
    if ( is_caster ) return SET_T15_CASTER;
    if ( is_melee  ) return SET_T15_MELEE;
    if ( is_healer ) return SET_T15_HEAL;
    if ( is_tank   ) return SET_T15_TANK;
  }

  if ( strstr( s, "_gladiators_kodohide_"   ) )   return SET_PVP_HEAL;
  if ( strstr( s, "_gladiators_wyrmhide_"   ) )   return SET_PVP_CASTER;
  if ( strstr( s, "_gladiators_dragonhide_" ) )   return SET_PVP_MELEE;

  return SET_NONE;
}

// druid_t::primary_role ====================================================

role_e druid_t::primary_role()
{
  if ( specialization() == DRUID_BALANCE )
  {
    if ( player_t::primary_role() == ROLE_HEAL )
      return ROLE_HEAL;

    return ROLE_SPELL;
  }

  else if ( specialization() == DRUID_FERAL )
  {
    if ( player_t::primary_role() == ROLE_TANK )
      return ROLE_TANK;

    return ROLE_ATTACK;
  }

  else if ( specialization() == DRUID_GUARDIAN )
  {
    if ( player_t::primary_role() == ROLE_ATTACK )
      return ROLE_ATTACK;

    return ROLE_TANK;
  }

  else if ( specialization() == DRUID_RESTORATION )
  {
    if ( player_t::primary_role() == ROLE_DPS || player_t::primary_role() == ROLE_SPELL )
      return ROLE_SPELL;

    return ROLE_HEAL;
  }

  return player_t::primary_role();
}

// druid_t::primary_resource ================================================

resource_e druid_t::primary_resource()
{
  if ( primary_role() == ROLE_SPELL || primary_role() == ROLE_HEAL )
    return RESOURCE_MANA;

  if ( primary_role() == ROLE_TANK )
    return RESOURCE_RAGE;

  return RESOURCE_ENERGY;
}

// druid_t::assess_damage ===================================================

void druid_t::assess_damage( school_e school,
                             dmg_e    dtype,
                             action_state_t* s )
{
  if ( set_bonus.tier15_2pc_tank() && s -> result == RESULT_DODGE && buff.savage_defense -> check() )
    buff.tier15_2pc_tank -> trigger();

  if ( buff.barkskin -> up() )
    s -> result_amount *= 1.0 + buff.barkskin -> value();

  if ( buff.survival_instincts -> up() )
    s -> result_amount *= 1.0 + buff.survival_instincts -> value();

  if ( spec.thick_hide -> ok() )
  {
    if ( school == SCHOOL_PHYSICAL )
      s -> result_amount *= 1.0 + spec.thick_hide -> effectN( 5 ).percent();
    else if ( dbc::get_school_mask( school ) & SCHOOL_MAGIC_MASK )
      s -> result_amount *= 1.0 + spec.thick_hide -> effectN( 3 ).percent();
  }

  // Call here to benefit from -10% physical damage before SD is taken into account
  player_t::assess_damage( school, dtype, s );
}

void druid_t::assess_heal( school_e school,
                           dmg_e    dmg_type,
                           heal_state_t* s )
{
  s -> result_amount *= 1.0 + buff.frenzied_regeneration -> check() * glyph.frenzied_regeneration -> effectN( 1 ).percent();

  player_t::assess_heal( school, dmg_type, s );
}

druid_td_t::druid_td_t( player_t& target, druid_t& source )
  : actor_pair_t( &target, &source ),
    lacerate_stack( 1 ),
    dots( dots_t() ),
    buffs( buffs_t() ),
    combo_points( source, target )
{
  dots.lacerate     = target.get_dot( "lacerate",     &source );
  dots.lifebloom    = target.get_dot( "lifebloom",    &source );
  dots.moonfire     = target.get_dot( "moonfire",     &source );
  dots.rake         = target.get_dot( "rake",         &source );
  dots.regrowth     = target.get_dot( "regrowth",     &source );
  dots.rejuvenation = target.get_dot( "rejuvenation", &source );
  dots.rip          = target.get_dot( "rip",          &source );
  dots.sunfire      = target.get_dot( "sunfire",      &source );
  dots.wild_growth  = target.get_dot( "wild_growth",  &source );

  buffs.lifebloom = buff_creator_t( *this, "lifebloom", source.find_class_spell( "Lifebloom" ) );
}

// ==========================================================================
// Combo Point System Functions
// ==========================================================================

combo_points_t::combo_points_t( druid_t& source, player_t& target ) :
  source( source ),
  target( target ),
  proc( nullptr ),
  wasted( nullptr ),
  count( 0 )
{
  proc   = target.get_proc( source.name_str + ": combo_points" );
  wasted = target.get_proc( source.name_str + ": combo_points_wasted" );
}

void combo_points_t::add( int num, const std::string* source_name )
{
  int actual_num = clamp( num, 0, max_combo_points - count );
  int overflow   = num - actual_num;

  // we count all combo points gained in the proc
  for ( int i = 0; i < num; i++ )
    proc -> occur();

  // add actual combo points
  if ( actual_num > 0 )
  {
    count += actual_num;
    source.trigger_ready();
  }

  // count wasted combo points
  for ( int i = 0; i < overflow; i++ )
    wasted -> occur();

  sim_t& sim = *source.sim;
  if ( sim.log )
  {
    if ( source_name )
    {
      if ( actual_num > 0 )
        sim.output( "%s gains %d (%d) combo_points from %s (%d)",
                    target.name(), actual_num, num, source_name -> c_str(), count );
    }
    else
    {
      if ( actual_num > 0 )
        sim.output( "%s gains %d (%d) combo_points (%d)",
                    target.name(), actual_num, num, count );
    }
  }
}

int combo_points_t::consume( const std::string* source_name )
{
  if ( source.sim -> log )
  {
    if ( source_name )
      source.sim -> output( "%s spends %d combo_points on %s",
                            target.name(), count, source_name -> c_str() );
    else
      source.sim -> output( "%s loses %d combo_points",
                            target.name(), count );
  }

  int tmp_count = count;
  count = 0;
  return tmp_count;
}

druid_t& druid_td_t::p() const
{ return *static_cast<druid_t*>( source ); }

// DRUID MODULE INTERFACE ================================================

struct druid_module_t : public module_t
{
  druid_module_t() : module_t( DRUID ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const
  {
    return new druid_t( sim, name, r );
  }
  virtual bool valid() const { return true; }
  virtual void init( sim_t* sim ) const
  {
    for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
    {
      player_t* p = sim -> actor_list[ i ];
      p -> buffs.innervate = new buffs::innervate_t( p );
    }
  }
  virtual void combat_begin( sim_t* ) const {}
  virtual void combat_end( sim_t* ) const {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::druid()
{
  static druid_module_t m;
  return &m;
}
