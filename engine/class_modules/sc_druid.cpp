// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE
// ==========================================================================
// Druid
// ==========================================================================

 /* WoD -- TODO:
    = Feral =
    Update LI implementation to work like in-game
    Damage check:
      Thrash (both forms) // Checked, is correct as of 9/14
      Swipe

    = Balance =
    PvP/Tier Bonuses

    = Guardian =
    Damage check
    PvP bonuses
    Add SD time @ max charges statistic
    Tooth & Claw in multi-tank sims

    = Restoration =
    Err'thing
*/

// Forward declarations
struct druid_t;

// Active actions
struct cenarion_ward_hot_t;
struct gushing_wound_t;
struct leader_of_the_pack_t;
struct natures_vigil_proc_t;
struct primal_tenacity_t;
struct ursocs_vigor_t;
struct yseras_gift_t;
namespace buffs {
struct heart_of_the_wild_buff_t;
}

struct druid_td_t : public actor_pair_t
{
  struct dots_t
  {
    dot_t* gushing_wound;
    dot_t* lacerate;
    dot_t* lifebloom;
    dot_t* moonfire;
    dot_t* rake;
    dot_t* regrowth;
    dot_t* rejuvenation;
    dot_t* rip;
    dot_t* stellar_flare;
    dot_t* sunfire;
    dot_t* thrash_bear;
    dot_t* thrash_cat;
    dot_t* wild_growth;
  } dots;

  struct buffs_t
  {
    buff_t* lifebloom;
    buff_t* bloodletting;
  } buffs;

  int lacerate_stack;

  druid_td_t( player_t& target, druid_t& source );

  bool hot_ticking()
  {
    return dots.regrowth      -> is_ticking() ||
           dots.rejuvenation  -> is_ticking() ||
           dots.lifebloom     -> is_ticking() ||
           dots.wild_growth   -> is_ticking();
  }

  void reset()
  {
    lacerate_stack = 0;
  }
};

struct druid_t : public player_t
{
public:
  timespan_t balance_time; // Balance power's current time, after accounting for celestial alignment/astral communion.
  timespan_t last_check; // Last time balance power was updated.
  bool double_dmg_triggered;
  double eclipse_amount; // Current balance power.
  double clamped_eclipse_amount;
  double eclipse_direction; // 1 = Going upwards, ie: Lunar ---> Solar
  // -1 = Downward, ie: Solar ---> Lunar
  double eclipse_max; // Amount of seconds until eclipse reaches maximum power.
  double eclipse_change; // Amount of seconds until eclipse changes.
  double time_to_next_lunar; // Amount of seconds until eclipse energy reaches 100 (Lunar Eclipse)
  double time_to_next_solar; // Amount of seconds until eclipse energy reaches -100 (Solar Eclipse)

  // Active
  action_t* t16_2pc_starfall_bolt;
  action_t* t16_2pc_sun_bolt;
  
  struct active_actions_t
  {
    cenarion_ward_hot_t*  cenarion_ward_hot;
    gushing_wound_t*      gushing_wound;
    leader_of_the_pack_t* leader_of_the_pack;
    natures_vigil_proc_t* natures_vigil;
    primal_tenacity_t*    primal_tenacity;
    ursocs_vigor_t*       ursocs_vigor;
    yseras_gift_t*        yseras_gift;
  } active;

  // Pets
  pet_t* pet_feral_spirit[ 2 ];
  pet_t* pet_mirror_images[ 3 ];
  pet_t* pet_force_of_nature[ 3 ];

  // Auto-attacks
  weapon_t caster_form_weapon;
  weapon_t cat_weapon;
  weapon_t bear_weapon;
  melee_attack_t* caster_melee_attack;
  melee_attack_t* cat_melee_attack;
  melee_attack_t* bear_melee_attack;

  double equipped_weapon_dps;

  // Buffs
  struct buffs_t
  {
    // General
    buff_t* bear_form;
    buff_t* cat_form;
    buff_t* dash;
    buff_t* cenarion_ward;
    buff_t* dream_of_cenarius;
    buff_t* frenzied_regeneration;
    buff_t* natures_vigil;
    buff_t* omen_of_clarity;
    buff_t* prowl;
    buff_t* stampeding_roar;

    // Balance
    buff_t* astral_communion;
    buff_t* astral_insight;
    buff_t* astral_showers;
    buff_t* celestial_alignment;
    buff_t* chosen_of_elune;
    buff_t* empowered_moonkin;
    buff_t* hurricane;
    buff_t* lunar_empowerment;
    buff_t* lunar_peak;
    buff_t* moonkin_form;
    buff_t* solar_empowerment;
    buff_t* solar_peak;
    buff_t* shooting_stars;
    buff_t* starfall;

    // Feral
    buff_t* berserk;
    buff_t* bloodtalons;
    buff_t* claws_of_shirvallah;
    buff_t* feral_fury;
    buff_t* feral_rage;
    buff_t* king_of_the_jungle;
    buff_t* predatory_swiftness;
    buff_t* savage_roar;
    buff_t* tigers_fury;
    buff_t* tier15_4pc_melee;

    // Guardian
    buff_t* barkskin;
    buff_t* bladed_armor;
    buff_t* bristling_fur;
    absorb_buff_t* primal_tenacity;
    buff_t* pulverize;
    buff_t* savage_defense;
    buff_t* son_of_ursoc;
    buff_t* survival_instincts;
    buff_t* tier15_2pc_tank;
    buff_t* tier17_4pc_tank;
    buff_t* tooth_and_claw;
    absorb_buff_t* tooth_and_claw_absorb;
    buff_t* ursa_major;

    // Restoration
    buff_t* natures_swiftness;
    buff_t* soul_of_the_forest;

    // NYI / Needs checking
    buff_t* harmony;
    buff_t* wild_mushroom;
    buff_t* tree_of_life;
    buffs::heart_of_the_wild_buff_t* heart_of_the_wild;
  } buff;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* berserk;
    cooldown_t* faerie_fire;
    cooldown_t* growl;
    cooldown_t* mangle;
    cooldown_t* maul;
    cooldown_t* natures_swiftness;
    cooldown_t* pvp_4pc_melee;
    cooldown_t* savage_defense_use;
    cooldown_t* starfallsurge;
    cooldown_t* swiftmend;
    cooldown_t* tigers_fury;
  } cooldown;

  // Gains
  struct gains_t
  {
    // Multiple Specs / Forms
    gain_t* omen_of_clarity;    // Feral & Restoration
    gain_t* primal_fury;        // Cat & Bear Forms
    gain_t* soul_of_the_forest; // Feral & Guardian

    // Feral (Cat)
    gain_t* energy_refund;
    gain_t* feral_rage; // tier16_4pc_melee
    gain_t* glyph_ferocious_bite;
    gain_t* leader_of_the_pack;
    gain_t* moonfire;
    gain_t* rake;
    gain_t* shred;
    gain_t* swipe;
    gain_t* tigers_fury;
    gain_t* tier15_2pc_melee;
    gain_t* tier17_2pc_melee;

    // Guardian (Bear)
    gain_t* bear_form;
    gain_t* frenzied_regeneration;
    gain_t* tier17_2pc_tank;
  } gain;

  // Perks
  struct
  {
    // Feral
    const spell_data_t* enhanced_berserk;
    const spell_data_t* enhanced_prowl;
    const spell_data_t* enhanced_rejuvenation;
    const spell_data_t* improved_rake;

    // Balance
    const spell_data_t* empowered_moonkin;
    const spell_data_t* enhanced_moonkin_form;
    const spell_data_t* enhanced_starsurge;

    // Guardian
    const spell_data_t* empowered_bear_form;
    const spell_data_t* empowered_berserk;
    const spell_data_t* enhanced_faerie_fire;
    const spell_data_t* enhanced_tooth_and_claw;

    // Restoration
    const spell_data_t* empowered_rejuvenation;
    const spell_data_t* enhanced_rebirth;
    const spell_data_t* empowered_ironbark;
    const spell_data_t* enhanced_lifebloom;

  } perk;

  // Glyphs
  struct glyphs_t
  {
    // DONE
    const spell_data_t* astral_communion;
    const spell_data_t* cat_form;
    const spell_data_t* celestial_alignment;
    const spell_data_t* dash;
    const spell_data_t* ferocious_bite;
    const spell_data_t* maim;
    const spell_data_t* maul;
    const spell_data_t* savage_roar;
    const spell_data_t* savagery;
    const spell_data_t* skull_bash;
    const spell_data_t* survival_instincts;
    const spell_data_t* stampeding_roar;

    // NYI / Needs checking
    const spell_data_t* blooming;
    const spell_data_t* healing_touch;
    const spell_data_t* moonwarding;
    const spell_data_t* lifebloom;
    const spell_data_t* master_shapeshifter;
    const spell_data_t* ninth_life;
    const spell_data_t* shapemender;
    const spell_data_t* regrowth;
    const spell_data_t* rejuvenation;
    const spell_data_t* ursols_defense;
    const spell_data_t* wild_growth;
  } glyph;

  // Masteries
  struct masteries_t
  {
    // Done
    const spell_data_t* primal_tenacity;
    const spell_data_t* razor_claws;
    const spell_data_t* total_eclipse;

    // NYI / TODO!
    const spell_data_t* harmony;
  } mastery;

  // Procs
  struct procs_t
  {
    proc_t* primal_fury;
    proc_t* shooting_stars;
    proc_t* shooting_stars_wasted;
    proc_t* tier15_2pc_melee;
    proc_t* tier17_2pc_melee;
    proc_t* tooth_and_claw;
    proc_t* tooth_and_claw_wasted;
    proc_t* ursa_major;
    proc_t* wrong_eclipse_wrath;
    proc_t* wrong_eclipse_starfire;
  } proc;

  // Class Specializations
  struct specializations_t
  {
    // Generic
    const spell_data_t* critical_strikes;       // Feral & Guardian
    const spell_data_t* killer_instinct;        // Feral & Guardian
    const spell_data_t* nurturing_instinct;     // Balance & Restoration
    const spell_data_t* leather_specialization; // All Specializations
    const spell_data_t* mana_attunement;        // Feral & Guardian
    const spell_data_t* omen_of_clarity;        // Feral & Restoration

    // Feral
    const spell_data_t* sharpened_claws;
    const spell_data_t* leader_of_the_pack;
    const spell_data_t* predatory_swiftness;
    const spell_data_t* rip;
    const spell_data_t* savage_roar;
    const spell_data_t* tigers_fury;

    // Balance
    const spell_data_t* astral_communion;
    const spell_data_t* astral_showers;
    const spell_data_t* celestial_alignment;
    const spell_data_t* celestial_focus;
    const spell_data_t* eclipse;
    const spell_data_t* lunar_guidance;
    const spell_data_t* moonkin_form;
    const spell_data_t* shooting_stars;
    const spell_data_t* starfall;
    const spell_data_t* starfire;
    const spell_data_t* starsurge;
    const spell_data_t* sunfire;

    // Guardian
    const spell_data_t* bladed_armor;
    const spell_data_t* resolve;
    const spell_data_t* savage_defense;
    const spell_data_t* survival_of_the_fittest;
    const spell_data_t* thick_hide; // Hidden passive for innate DR, expertise, and crit reduction
    const spell_data_t* tooth_and_claw;
    const spell_data_t* ursa_major;

    // Restoration
    // Done
    const spell_data_t* naturalist;
    // NYI / TODO or needs checking
    const spell_data_t* lifebloom;
    const spell_data_t* living_seed;
    const spell_data_t* genesis;
    const spell_data_t* ironbark;
    const spell_data_t* malfurions_gift;
    const spell_data_t* meditation;
    const spell_data_t* natural_insight;
    const spell_data_t* natures_focus;
    const spell_data_t* natures_swiftness;
    const spell_data_t* regrowth;
    const spell_data_t* swiftmend;
    const spell_data_t* tranquility;
    const spell_data_t* wild_growth;
  } spec;

  struct spells_t
  {
    const spell_data_t* bear_form; // Bear form bonuses
    const spell_data_t* berserk_bear; // Berserk bear mangler
    const spell_data_t* berserk_cat; // Berserk cat resource cost reducer
    const spell_data_t* cat_form; // Cat form bonuses
    const spell_data_t* frenzied_regeneration;
    const spell_data_t* mangle; // Mangle cooldown reset
    const spell_data_t* moonkin_form; // Moonkin form bonuses
    const spell_data_t* primal_fury; // Primal fury gain
    const spell_data_t* regrowth; // Old GoRegrowth
    const spell_data_t* survival_instincts; // Survival instincts aura
  } spell;

  // Talents
  struct talents_t
  {
    const spell_data_t* feline_swiftness;
    const spell_data_t* displacer_beast; //todo
    const spell_data_t* wild_charge; //todo

    const spell_data_t* yseras_gift;
    const spell_data_t* renewal;
    const spell_data_t* cenarion_ward;

    const spell_data_t* faerie_swarm; //pvp
    const spell_data_t* mass_entanglement; //pvp
    const spell_data_t* typhoon; //pvp

    const spell_data_t* soul_of_the_forest; //Re-do for balance when they announce changes for it.
    const spell_data_t* incarnation_tree;
    const spell_data_t* incarnation_son;
    const spell_data_t* incarnation_chosen;
    const spell_data_t* incarnation_king;
    const spell_data_t* force_of_nature; 

    const spell_data_t* incapacitating_roar; //pvp
    const spell_data_t* ursols_vortex; //pvp
    const spell_data_t* mighty_bash; //pvp

    const spell_data_t* heart_of_the_wild;
    const spell_data_t* dream_of_cenarius;
    const spell_data_t* natures_vigil;

    // Balance 100 Talents
    const spell_data_t* euphoria;
    const spell_data_t* stellar_flare;
    const spell_data_t* balance_of_power;

    // Feral 100 Talents
    const spell_data_t* lunar_inspiration;
    const spell_data_t* bloodtalons;
    const spell_data_t* claws_of_shirvallah;

    // Guardian 100 Talents
    const spell_data_t* guardian_of_elune;
    const spell_data_t* pulverize;
    const spell_data_t* bristling_fur;

    // Restoration 100 Talents
    const spell_data_t* moment_of_clarity;
    const spell_data_t* germination;
    const spell_data_t* rampant_growth;

  } talent;

  bool inflight_starsurge;

  druid_t( sim_t* sim, const std::string& name, race_e r = RACE_NIGHT_ELF ) :
    player_t( sim, DRUID, name, r ),
    balance_time( timespan_t::zero() ),
    last_check( timespan_t::zero() ),
    eclipse_amount( 0 ),
    clamped_eclipse_amount( 0 ),
    eclipse_direction( 1 ),
    eclipse_max( 10 ),
    eclipse_change( 20 ),
    time_to_next_lunar( 10 ),
    time_to_next_solar( 30 ),
    t16_2pc_starfall_bolt( nullptr ),
    t16_2pc_sun_bolt( nullptr ),
    active( active_actions_t() ),
    caster_form_weapon(),
    buff( buffs_t() ),
    cooldown( cooldowns_t() ),
    gain( gains_t() ),
    glyph( glyphs_t() ),
    mastery( masteries_t() ),
    proc( procs_t() ),
    spec( specializations_t() ),
    spell( spells_t() ),
    talent( talents_t() ),
    inflight_starsurge( false )
  {
    t16_2pc_starfall_bolt = 0;
    t16_2pc_sun_bolt      = 0;
    double_dmg_triggered = false;

    for (size_t i = 0; i < sizeof_array(pet_force_of_nature); i++)
    {
        pet_force_of_nature[i] = nullptr;
    }
    
    cooldown.berserk             = get_cooldown( "berserk"             );
    cooldown.faerie_fire         = get_cooldown( "faerie_fire"         );
    cooldown.growl               = get_cooldown( "growl"               );
    cooldown.mangle              = get_cooldown( "mangle"              );
    cooldown.maul                = get_cooldown( "maul"                );
    cooldown.natures_swiftness   = get_cooldown( "natures_swiftness"   );
    cooldown.pvp_4pc_melee       = get_cooldown( "pvp_4pc_melee"       );
    cooldown.savage_defense_use  = get_cooldown( "savage_defense_use"  );
    cooldown.starfallsurge       = get_cooldown( "starfallsurge"       );
    cooldown.swiftmend           = get_cooldown( "swiftmend"           );
    cooldown.tigers_fury         = get_cooldown( "tigers_fury"         );
    
    cooldown.pvp_4pc_melee -> duration = timespan_t::from_seconds( 30.0 );
    cooldown.starfallsurge -> charges = 3;
    cooldown.starfallsurge -> duration = timespan_t::from_seconds( 30.0 );

    cat_melee_attack = 0;
    bear_melee_attack = 0;

    equipped_weapon_dps = 0;

    base.distance = ( specialization() == DRUID_FERAL || specialization() == DRUID_GUARDIAN ) ? 3 : 30;
    regen_type = REGEN_DYNAMIC;
    regen_caches[ CACHE_HASTE ] = true;
    regen_caches[ CACHE_ATTACK_HASTE ] = true;
  }

  // Character Definition
  virtual void      arise();
  virtual void      init_spells();
  virtual void      init_base_stats();
  virtual void      create_buffs();
  virtual void      init_scaling();
  virtual void      init_gains();
  virtual void      init_procs();
  virtual void      init_benefits();
  virtual void      invalidate_cache( cache_e );
  virtual void      combat_begin();
  virtual void      reset();
  virtual void      regen( timespan_t periodicity );
  virtual timespan_t available() const;
  virtual double    composite_armor_multiplier() const;
  virtual double    composite_melee_attack_power() const;
  virtual double    composite_melee_crit() const;
  virtual double    composite_attack_power_multiplier() const;
  virtual double    temporary_movement_modifier() const;
  virtual double    passive_movement_modifier() const;
  virtual double    composite_player_multiplier( school_e school ) const;
  virtual double    composite_player_heal_multiplier( const action_state_t* s ) const;
  virtual double    composite_player_absorb_multiplier( const action_state_t* s ) const;
  virtual double    composite_spell_crit() const;
  virtual double    composite_spell_power( school_e school ) const;
  virtual double    composite_attribute( attribute_e attr ) const;
  virtual double    composite_attribute_multiplier( attribute_e attr ) const;
  virtual double    matching_gear_multiplier( attribute_e attr ) const;
  virtual double    composite_damage_versatility() const;
  virtual double    composite_heal_versatility() const;
  virtual double    composite_mitigation_versatility() const;
  virtual double    composite_parry() const { return 0; }
  virtual double    composite_block() const { return 0; }
  virtual double    composite_crit_avoidance() const;
  virtual double    composite_melee_expertise( weapon_t* ) const;
  virtual double    composite_dodge() const;
  virtual double    composite_rating_multiplier( rating_e rating ) const;
  virtual expr_t*   create_expression( action_t*, const std::string& name );
  virtual action_t* create_action( const std::string& name, const std::string& options );
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() );
  virtual void      create_pets();
  virtual resource_e primary_resource() const;
  virtual role_e    primary_role() const;
  virtual stat_e    convert_hybrid_stat( stat_e s ) const;
  virtual double    mana_regen_per_second() const;
  virtual void      assess_damage( school_e school, dmg_e, action_state_t* );
  virtual void      assess_heal( school_e, dmg_e, action_state_t* );
  virtual void      create_options();
  virtual bool      create_profile( std::string& profile_str, save_e type = SAVE_ALL, bool save_html = false );
  virtual void      recalculate_resource_max( resource_e r );

  void              apl_precombat();
  void              apl_default();
  void              apl_feral();
  void              apl_balance();
  void              apl_guardian();
  void              apl_restoration();
  virtual void      init_action_list();

  target_specific_t<druid_td_t*> target_data;

  virtual druid_td_t* get_target_data( player_t* target ) const
  {
    assert( target );
    druid_td_t*& td = target_data[ target ];
    if ( ! td )
    {
      td = new druid_td_t( *target, const_cast<druid_t&>(*this) );
    }
    return td;
  }

  void balance_tracker();
  void balance_expressions();
  void trigger_shooting_stars( action_state_t* state );
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

// Gushing Wound (tier17_4pc_melee) ========================================================

struct gushing_wound_t : public residual_action::residual_periodic_action_t< attack_t >
{
  typedef  residual_action::residual_periodic_action_t<attack_t> base_t;
  gushing_wound_t( druid_t* p ) :
    base_t( "gushing_wound", p, p -> find_spell( 166638 ) )
  {
    background = dual = true;
  }

  virtual void init()
  {
    attack_t::init();

    // Don't double dip!
    snapshot_flags &= STATE_NO_MULTIPLIER;
  }
};

// Nature's Vigil Proc ======================================================

struct natures_vigil_proc_t : public spell_t
{
  struct heal_proc_t : public heal_t
  {
    bool fromDmg;
    double heal_coeff, dmg_coeff;

    heal_proc_t( druid_t* p ) :
      heal_t( "natures_vigil_heal", p, p -> find_spell( 124988 ) ),
      fromDmg( true ), heal_coeff( 0.0 ), dmg_coeff( 0.0 )
    {
      background = proc = dual = true;
      may_crit = may_miss = false;
      may_multistrike = 0;
      trigger_gcd = timespan_t::zero();
      heal_coeff = p -> talent.natures_vigil -> effectN( 3 ).percent();
      dmg_coeff  = p -> talent.natures_vigil -> effectN( 4 ).percent();
    }

    virtual void init()
    {
      heal_t::init();
      // Disable the snapshot_flags for all multipliers, but specifically allow factors in
      // action_state_t::composite_da_multiplier() to be called. This works and I stole it 
      // from the paladin module but I don't understand why.
      snapshot_flags &= STATE_NO_MULTIPLIER;
      snapshot_flags |= STATE_MUL_DA;
    }

    virtual double action_multiplier() const
    {
      double am = heal_t::action_multiplier();
      
      if ( fromDmg )
        am *= dmg_coeff;
      else
        am *= heal_coeff;

      return am;
    }

    virtual void execute()
    {
      target = smart_target();

      heal_t::execute();
    }

  private:
    player_t* find_lowest_target()
    {
      // Ignoring range for the time being
      double lowest_health_pct_found = 100.1;
      player_t* lowest_player_found = nullptr;

      for ( size_t i = 0, size = sim -> player_no_pet_list.size(); i < size; i++ )
      {
        player_t* p = sim -> player_no_pet_list[ i ];

        // check their health against the current lowest
        if ( p -> health_percentage() < lowest_health_pct_found )
        {
          // if this player is lower, make them the current lowest
          lowest_health_pct_found = p -> health_percentage();
          lowest_player_found = p;
        }
      }
      return lowest_player_found;
    }
  };

  struct damage_proc_t : public spell_t
  {
    damage_proc_t( druid_t* p ) :
      spell_t( "natures_vigil_damage", p, p -> find_spell( 124991 ) )
    {
      background = proc = dual = true;
      may_crit = may_miss = false;
      may_multistrike = 0;
      trigger_gcd     = timespan_t::zero();
      base_multiplier = p -> talent.natures_vigil -> effectN( 3 ).percent();
    }

    virtual void init()
    {
      spell_t::init();
      // Disable the snapshot_flags for all multipliers, but specifically allow factors in
      // action_state_t::composite_da_multiplier() to be called. This works and I stole it 
      // from the paladin module.
      snapshot_flags &= STATE_NO_MULTIPLIER;
      snapshot_flags |= STATE_MUL_DA;
    }

    virtual void execute()
    {
      target = pick_random_target();

      spell_t::execute();
    }

  private:
    player_t* pick_random_target()
    {
      // Targeting is probably done by range, but since the sim doesn't really have
      // have a concept of that we'll just pick a target at random.

      unsigned t = static_cast<unsigned>( rng().range( 0, as<double>( sim -> target_list.size() ) ) );
      if ( t >= sim-> target_list.size() ) --t; // dsfmt range should not give a value actually equal to max, but be paranoid
      return sim-> target_list[ t ];
    }
  };

  damage_proc_t* damage;
  heal_proc_t*   heal;

  natures_vigil_proc_t( druid_t* p ) :
    spell_t( "natures_vigil", p, spell_data_t::nil() )
  {
    damage  = new damage_proc_t( p );
    heal    = new heal_proc_t( p );
  }

  void trigger( double amount, bool harmful = true )
  {
    if ( ! harmful )
    {
      damage -> base_dd_min = damage -> base_dd_max = amount;
      damage -> execute();
    }
    heal -> base_dd_min = heal -> base_dd_max = amount;
    heal -> fromDmg = harmful;
    heal -> execute();
  }
};

// Ursoc's Vigor ( tier16_4pc_tank ) =====================================

struct ursocs_vigor_t : public heal_t
{
  double ap_coefficient;
  int ticks_remain;

  ursocs_vigor_t( druid_t* p ) :
    heal_t( "ursocs_vigor", p, p -> find_spell( 144888 ) )
  {
    background = true;

    hasted_ticks = false;
    may_crit = tick_may_crit = false;
    may_multistrike = 0;
    base_td = 0;

    base_tick_time = timespan_t::from_seconds( 1.0 );
    dot_duration = 8 * base_tick_time;

    // store healing multiplier
    ap_coefficient = p -> find_spell( 144887 ) -> effectN( 1 ).percent();
  }

  druid_t* p() const
  { return static_cast<druid_t*>( player ); }

  void trigger_hot( double rage_consumed = 60.0 )
  {
    if ( dot_t* dot = get_dot() )
    {
      if ( dot -> is_ticking() )
      {
        // Adjust the current healing remaining to be spread over the total duration of the hot.
        base_td *= dot -> remains() / dot_duration;
      }
    }

    // Add the new amount of healing
    base_td += p() -> composite_melee_attack_power() * p() -> composite_attack_power_multiplier()
               * ap_coefficient
               * rage_consumed / 60
               * dot_duration / base_tick_time;

    execute();
  }

  virtual void tick( dot_t *d )
  {
    heal_t::tick( d );

    ticks_remain -= 1;
  }
};

// Cenarion Ward HoT ========================================================

struct cenarion_ward_hot_t : public heal_t
{
  cenarion_ward_hot_t( druid_t* p ) :
    heal_t( "cenarion_ward_hot", p, p -> find_spell( 102352 ) )
  {
    harmful = false;
    background = proc = true;
    target = p;
  }

  virtual void execute()
  {
    heal_t::execute();

    static_cast<druid_t*>( player ) -> buff.cenarion_ward -> expire();
  }
};

// Leader of the Pack =======================================================

struct leader_of_the_pack_t : public heal_t
{
  leader_of_the_pack_t( druid_t* p ) :
    heal_t( "leader_of_the_pack", p, p -> find_spell( 68285 ) )
  {
    harmful = may_crit = false;
    may_multistrike = 0;
    background = proc = true;
    target = p;

    cooldown -> duration = timespan_t::from_seconds( 6.0 );
    pct_heal = data().effectN( 1 ).percent();
  }
};

// Primal Tenacity ==========================================================

struct primal_tenacity_t : public absorb_t
{
  double absorb_remaining; // The absorb value the druid had when they took the triggering attack

  primal_tenacity_t( druid_t* p ) :
    absorb_t( "primal_tenacity", p, p -> mastery.primal_tenacity ),
    absorb_remaining( 0.0 )
  {
    harmful = may_crit = false;
    may_multistrike = 0;
    background = proc = dual = true;
    target = p;
  }

  druid_t* p() const
  { return static_cast<druid_t*>( player ); }

  virtual void init()
  {
    absorb_t::init();

    snapshot_flags &= ~STATE_RESOLVE; // Is not affected by resolve.
  }

  virtual double action_multiplier() const
  {
    double am = absorb_t::action_multiplier();

    am *= p() -> cache.mastery_value();

    return am;
  }

  virtual void impact( action_state_t* s )
  {
    /* Primal Tenacity may only occur if the amount of damage PT absorbed from the triggering attack
       is less than 20% of the size of the new absorb. */
    if ( absorb_remaining < s -> result_amount * 0.2 )
    {
      // Subtract the amount absorbed from the triggering attack from the new absorb.
      s -> result_amount -= absorb_remaining;

      p() -> buff.primal_tenacity -> trigger( 1, s -> result_amount );
    }
  }
};

// Ysera's Gift ==============================================================

struct yseras_gift_t : public heal_t
{
  yseras_gift_t( druid_t* p ) :
    heal_t( "yseras_gift", p, p -> talent.yseras_gift )
  {
    base_tick_time = data().effectN( 1 ).period();
    dot_duration   = base_tick_time * 2;
    harmful = tick_may_crit = hasted_ticks = false;
    may_multistrike = 0;
    background = proc = dual = true;
    target = p;

    tick_pct_heal = data().effectN( 1 ).percent();
  }

  virtual void init()
  {
    heal_t::init();

    snapshot_flags &= ~STATE_RESOLVE; // Is not affected by resolve.
  }

  virtual void tick( dot_t* d )
  {
    d -> refresh_duration(); // ticks indefinitely

    heal_t::tick( d );
  }

  // Override calculate_tick_amount for unique mechanic (heals smart target for % of own health)
  // This might not be the best way to do this, but it works.
  virtual double calculate_tick_amount( action_state_t* state, double /* dmg_multiplier */ ) // dmg_multiplier is unused, this removes compiler warning.
  {
    double amount = state -> action -> player -> resources.max[ RESOURCE_HEALTH ] * tick_pct_heal;
    return amount;
  }

  virtual void execute()
  {
    if( player -> health_percentage() < 100 )
    {
      target = player;
    }
    else
    { 
    target = smart_target();
    }

    heal_t::execute();
  }
};

namespace pets {


// ==========================================================================
// Pets and Guardians
// ==========================================================================

// Balance Force of Nature ==================================================

struct force_of_nature_balance_t : public pet_t
{
  struct wrath_t : public spell_t
  {
    wrath_t( force_of_nature_balance_t* player ) :
      spell_t( "wrath", player, player -> find_spell( 113769 ) )
    {
      if ( player -> o() -> pet_force_of_nature[ 0 ] )
        stats = player -> o() -> pet_force_of_nature[ 0 ] -> get_stats( "wrath" );
      may_crit = true;
    }
  };

  druid_t* o() { return static_cast< druid_t* >( owner ); }

  force_of_nature_balance_t( sim_t* sim, druid_t* owner ) :
    pet_t( sim, owner, "treant", true /*GUARDIAN*/, true )
  {
    owner_coeff.sp_from_sp = 1.0;
    action_list_str = "wrath";
    regen_type = REGEN_DISABLED;
  }

  virtual void init_base_stats()
  {
    pet_t::init_base_stats();

    resources.base[ RESOURCE_HEALTH ] = owner -> resources.max[ RESOURCE_HEALTH ] * 0.4;
    resources.base[ RESOURCE_MANA   ] = 0;

    initial.stats.attribute[ ATTR_INTELLECT ] = 0;
    initial.spell_power_per_intellect = 0;
    intellect_per_owner = 0;
    stamina_per_owner = 0;
  }

  virtual resource_e primary_resource() const { return RESOURCE_MANA; }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str )
  {
    if ( name == "wrath"  ) return new wrath_t( this );

    return pet_t::create_action( name, options_str );
  }
};

// Feral Force of Nature ====================================================

struct force_of_nature_feral_t : public pet_t
{
  struct melee_t : public melee_attack_t
  {
    druid_t* owner;

    melee_t( force_of_nature_feral_t* p )
      : melee_attack_t( "melee", p, spell_data_t::nil() ), owner( 0 )
    {
      school = SCHOOL_PHYSICAL;
      weapon = &( p -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      special     = false;
      background  = true;
      repeating   = true;
      may_crit    = true;
      may_glance  = true;
      trigger_gcd = timespan_t::zero();
      owner       = p -> o();
    }

    force_of_nature_feral_t* p()
    { return static_cast<force_of_nature_feral_t*>( player ); }

    void init()
    {
      melee_attack_t::init();
      if ( ! player -> sim -> report_pets_separately && player != p() -> o() -> pet_force_of_nature[ 0 ] )
        stats = p() -> o() -> pet_force_of_nature[ 0 ] -> get_stats( name(), this );
    }
  };

  struct rake_t : public melee_attack_t
  {
    druid_t* owner;

    rake_t( force_of_nature_feral_t* p ) :
      melee_attack_t( "rake", p, p -> find_spell( 150017 ) ), owner( 0 )
    {
      dot_behavior     = DOT_REFRESH;
      special = may_crit = tick_may_crit = true;
      owner            = p -> o();
    }

    force_of_nature_feral_t* p()
    { return static_cast<force_of_nature_feral_t*>( player ); }
    const force_of_nature_feral_t* p() const
    { return static_cast<force_of_nature_feral_t*>( player ); }

    void init()
    {
      melee_attack_t::init();
      if ( ! player -> sim -> report_pets_separately && player != p() -> o() -> pet_force_of_nature[ 0 ] )
        stats = p() -> o() -> pet_force_of_nature[ 0 ] -> get_stats( name(), this );
    }

    virtual double composite_ta_multiplier( const action_state_t* state ) const
    {
      double m = melee_attack_t::composite_ta_multiplier( state );

      if ( p() -> o() -> mastery.razor_claws -> ok() )
        m *= 1.0 + p() -> o() -> cache.mastery_value();

      return m;
    }

    // Treat direct damage as "bleed"
    // Must use direct damage because tick_zeroes cannot be blocked, and this attack is going to get blocked occasionally.
    double composite_da_multiplier( const action_state_t* state ) const
    {
      double m = melee_attack_t::composite_da_multiplier( state );

      if ( p() -> o() -> mastery.razor_claws -> ok() )
        m *= 1.0 + p() -> o() -> cache.mastery_value();

      return m;
    }

    virtual double target_armor( player_t* ) const
    { return 0.0; }

    virtual void execute()
    {
      melee_attack_t::execute();
      p() -> change_position( POSITION_BACK ); // After casting it's "opener" ability, move behind the target.
    }
  };
  
  melee_t* melee;

  force_of_nature_feral_t( sim_t* sim, druid_t* p ) :
    pet_t( sim, p, "treant", true, true ), melee( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    main_hand_weapon.min_dmg    = owner -> find_spell( 102703 ) -> effectN( 1 ).min( owner );
    main_hand_weapon.max_dmg    = owner -> find_spell( 102703 ) -> effectN( 1 ).max( owner );
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    owner_coeff.ap_from_ap      = 1.0;
    regen_type = REGEN_DISABLED;
  }

  druid_t* o()
  { return static_cast< druid_t* >( owner ); }
  const druid_t* o() const
  { return static_cast<const druid_t* >( owner ); }

  virtual void init_base_stats()
  {
    pet_t::init_base_stats();

    resources.base[ RESOURCE_HEALTH ] = owner -> resources.max[ RESOURCE_HEALTH ] * 0.4;
    resources.base[ RESOURCE_MANA   ] = 0;
    stamina_per_owner = 0.0;

    melee = new melee_t( this );
  }

  virtual void init_action_list()
  {
    action_list_str = "rake";

    pet_t::init_action_list();
  }
  
  action_t* create_action( const std::string& name,
                           const std::string& options_str )
  {
    if ( name == "rake" ) return new rake_t( this );

    return pet_t::create_action( name, options_str );
  }

  virtual void summon( timespan_t duration = timespan_t::zero() )
  {
    pet_t::summon( duration );
    this -> change_position( POSITION_FRONT ); // Emulate Treant spawning in front of the target.
  }

  void schedule_ready( timespan_t delta_time = timespan_t::zero(), bool waiting = false )
  {
    // FIXME: Currently first swing happens after swingtime # seconds, it should happen after 1-1.5 seconds (random).
    if ( melee && ! melee -> execute_event )
      melee -> schedule_execute();

    pet_t::schedule_ready( delta_time, waiting );
  }
};

// Guardian Force of Nature ====================================================

struct force_of_nature_guardian_t : public pet_t
{
  melee_attack_t* melee;

  struct melee_t : public melee_attack_t
  {
    druid_t* owner;

    melee_t( force_of_nature_guardian_t* p )
      : melee_attack_t( "melee", p, spell_data_t::nil() ), owner( 0 )
    {
      school = SCHOOL_PHYSICAL;
      weapon = &( p -> main_hand_weapon );
      base_execute_time = weapon -> swing_time;
      background = true;
      repeating  = true;
      may_crit   = true;
      trigger_gcd = timespan_t::zero();
      owner = p -> o();

      may_glance = true;
      special    = false;
    }
  };

  force_of_nature_guardian_t( sim_t* sim, druid_t* p ) :
    pet_t( sim, p, "treant", true, true ), melee( 0 )
  {
    main_hand_weapon.type       = WEAPON_BEAST;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    main_hand_weapon.min_dmg    = owner -> find_spell( 102706 ) -> effectN( 1 ).min( owner ) * 0.2;
    main_hand_weapon.max_dmg    = owner -> find_spell( 102706 ) -> effectN( 1 ).max( owner ) * 0.2;
    main_hand_weapon.damage     = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    owner_coeff.ap_from_ap      = 0.2 * 1.2;
    regen_type = REGEN_DISABLED;
  }

  druid_t* o()
  { return static_cast< druid_t* >( owner ); }

  virtual void init_base_stats()
  {
    pet_t::init_base_stats();

    resources.base[ RESOURCE_HEALTH ] = owner -> resources.max[ RESOURCE_HEALTH ] * 0.4;
    resources.base[ RESOURCE_MANA   ] = 0;
    stamina_per_owner = 0.0;

    melee = new melee_t( this );
  }

  virtual void summon( timespan_t duration = timespan_t::zero() )
  {
    pet_t::summon( duration );
    schedule_ready();
  }

  virtual void schedule_ready( timespan_t delta_time = timespan_t::zero(), bool waiting = false )
  {
    pet_t::schedule_ready( delta_time, waiting );
    if ( ! melee -> execute_event ) melee -> execute();
  }
};

} // end namespace pets

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

  druid_t& p() const { return druid; }
};

// Astral Communion Buff ====================================================

struct astral_communion_t : public druid_buff_t < buff_t >
{
  astral_communion_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "astral_communion", p.find_class_spell( "Astral Communion" ) ) )
  {
    cooldown -> duration = timespan_t::zero(); // CD is managed by the spell
  }

  virtual void expire_override()
  {
    base_t::expire_override();

    druid.last_check = sim -> current_time - druid.last_check;
    druid.last_check *= 1 + druid.buff.astral_communion -> data().effectN( 1 ).percent();
    druid.balance_time += druid.last_check;
    druid.last_check = sim -> current_time;
  }
};

// Bear Form ================================================================

struct bear_form_t : public druid_buff_t< buff_t >
{
public:
  bear_form_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "bear_form", p.find_class_spell( "Bear Form" ) ) ),
    rage_spell( p.find_spell( 17057 ) )
  {
    add_invalidate( CACHE_AGILITY );
    add_invalidate( CACHE_ATTACK_POWER );
    add_invalidate( CACHE_STAMINA );
    add_invalidate( CACHE_ARMOR );
  }

  virtual void expire_override()
  {
    base_t::expire_override();

    swap_melee( druid.caster_melee_attack, druid.caster_form_weapon );

    sim -> auras.critical_strike -> decrement();

    druid.recalculate_resource_max( RESOURCE_HEALTH );

    if ( druid.specialization() == DRUID_GUARDIAN )
      druid.resolve_manager.stop();
  }

  virtual void start( int stacks, double value, timespan_t duration )
  {
    druid.buff.moonkin_form -> expire();
    druid.buff.cat_form -> expire();

    druid.buff.tigers_fury -> expire(); // 6/29/2014: Tiger's Fury ends when you enter bear form

    if ( druid.specialization() == DRUID_GUARDIAN )
      druid.resolve_manager.start();

    swap_melee( druid.bear_melee_attack, druid.bear_weapon );

    // Set rage to 0 and then gain rage to 10
    druid.resource_loss( RESOURCE_RAGE, druid.resources.current[ RESOURCE_RAGE ] );
    druid.resource_gain( RESOURCE_RAGE, rage_spell -> effectN( 1 ).base_value() / 10.0, druid.gain.bear_form );
    // TODO: Clear rage on bear form exit instead of entry.

    base_t::start( stacks, value, duration );

    if ( ! sim -> overrides.critical_strike )
      sim -> auras.critical_strike -> trigger();

    druid.recalculate_resource_max( RESOURCE_HEALTH );
  }
private:
  const spell_data_t* rage_spell;
};

// Berserk Buff =============================================================

struct berserk_buff_t : public druid_buff_t<buff_t>
{
  berserk_buff_t( druid_t& p ) :
    druid_buff_t<buff_t>( p, buff_creator_t( &p, "berserk", p.find_specialization_spell( "Berserk" ) )
      .cd( timespan_t::from_seconds( 0.0 ) ) // Cooldown handled by ability
    )
  {}

  virtual bool trigger( int stacks, double value, double chance, timespan_t duration )
  {
    if ( druid.perk.enhanced_berserk -> ok() )
      player -> resources.max[ RESOURCE_ENERGY ] += druid.perk.enhanced_berserk -> effectN( 1 ).base_value();

    return druid_buff_t<buff_t>::trigger( stacks, value, chance, duration );
  }

  virtual void expire_override()
  {
    if ( druid.perk.enhanced_berserk -> ok() )
    {
      player -> resources.max[ RESOURCE_ENERGY ] -= druid.perk.enhanced_berserk -> effectN( 1 ).base_value();
      // Force energy down to cap if it's higher.
      player -> resources.current[ RESOURCE_ENERGY ] = std::min( player -> resources.current[ RESOURCE_ENERGY ], player -> resources.max[ RESOURCE_ENERGY ]);
    }

    druid_buff_t<buff_t>::expire_override();
  }
};

// Cat Form =================================================================

struct cat_form_t : public druid_buff_t< buff_t >
{
  cat_form_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "cat_form", p.find_class_spell( "Cat Form" ) ) )
  {
    add_invalidate( CACHE_AGILITY );
    add_invalidate( CACHE_ATTACK_POWER );
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

    if ( druid.talent.claws_of_shirvallah -> ok() )
      quiet = true;
  }

  virtual void expire_override()
  {
    base_t::expire_override();
    
    swap_melee( druid.caster_melee_attack, druid.caster_form_weapon );

    druid.buff.claws_of_shirvallah -> expire();

    sim -> auras.critical_strike -> decrement();
  }

  virtual void start( int stacks, double value, timespan_t duration )
  {
    druid.buff.bear_form -> expire();
    druid.buff.moonkin_form -> expire();

    swap_melee( druid.cat_melee_attack, druid.cat_weapon );

    base_t::start( stacks, value, duration );

    if ( druid.talent.claws_of_shirvallah -> ok() )
      druid.buff.claws_of_shirvallah -> start();

    if ( ! sim -> overrides.critical_strike )
      sim -> auras.critical_strike -> trigger();
  }
};

// Celestial Alignment Buff =================================================

struct celestial_alignment_t : public druid_buff_t < buff_t >
{
  celestial_alignment_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "celestial_alignment", p.find_class_spell( "Celestial Alignment" ) ) )
  {
    cooldown -> duration = timespan_t::zero(); // CD is managed by the spell
    add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  }

  virtual void expire_override()
  {
    base_t::expire_override();

    druid.last_check = sim -> current_time;
  }
};

// Moonkin Form =============================================================

struct moonkin_form_t : public druid_buff_t< buff_t >
{
  moonkin_form_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "moonkin_form", p.spec.moonkin_form )
               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER ) )
  { }

  virtual void expire_override()
  {
    base_t::expire_override();

    sim -> auras.haste -> decrement();
  }

  virtual void start( int stacks, double value, timespan_t duration )
  {
    druid.buff.bear_form -> expire();
    druid.buff.cat_form  -> expire();

    base_t::start( stacks, value, duration );

    if ( ! sim -> overrides.haste )
      sim -> auras.haste -> trigger();
  }
};

// Tooth and Claw Absorb Buff ===============================================

struct tooth_and_claw_absorb_t : public absorb_buff_t
{
  tooth_and_claw_absorb_t( druid_t* p ) :
    absorb_buff_t( absorb_buff_creator_t( p, "tooth_and_claw_absorb", p -> find_spell( 135597 ) )
                   .school( SCHOOL_PHYSICAL )
                   .source( p -> get_stats( "tooth_and_claw" ) )
                   .gain( p -> get_gain( "tooth_and_claw" ) )
    )
  {}

  druid_t* p() const
  { return static_cast<druid_t*>( player ); }

  virtual bool trigger( int stacks = 1, double value = 0.0, double chance = (-1.0), timespan_t duration = timespan_t::min() )
  {
    // Add the value of any current tooth and claw absorb (stacks)
    if ( p() -> buff.tooth_and_claw_absorb -> check() )
      value += p() -> buff.tooth_and_claw_absorb -> current_value;

    return absorb_buff_t::trigger( stacks, value, chance, duration );
  }

  virtual void absorb_used( double /* amount */ )
  {
    // Tooth and Claw expires after ANY amount of the absorb is used.
    p() -> buff.tooth_and_claw_absorb -> expire();
  }
};

// Ursa Major Buff ==========================================================

struct ursa_major_t : public druid_buff_t < buff_t >
{
  int health_gain;

  ursa_major_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "ursa_major", p.find_spell( 159233 ) )
                  .default_value( p.find_spell( 159233 ) -> effectN( 1 ).percent() )
    ), health_gain( 0 )
  {}

  virtual void start( int stacks, double value, timespan_t duration )
  {
    base_t::start( stacks, value, duration );

    recalculate_temporary_health( value );
  }

  virtual void refresh( int stacks, double value, timespan_t duration )
  {
    base_t::refresh( stacks, value, duration );
    
    recalculate_temporary_health( value );
  }

  virtual void expire_override()
  {
    base_t::expire_override();

    recalculate_temporary_health( 0.0 );
  }

private:
  void recalculate_temporary_health( double value )
  {
    // Calculate the benefit of the new buff
    int old_health_gain = health_gain;
    health_gain = (int) floor( ( druid.resources.max[ RESOURCE_HEALTH ] - old_health_gain ) * value );
    int diff = health_gain - old_health_gain;

    // Adjust the temporary HP gain
    if ( diff > 0 )
      druid.stat_gain( STAT_MAX_HEALTH, diff, (gain_t*) 0, (action_t*) 0, true );
    else if ( diff < 0 )
      druid.stat_loss( STAT_MAX_HEALTH, -diff, (gain_t*) 0, (action_t*) 0, true );
  }
};

// Heart of the Wild Buff ===================================================

struct heart_of_the_wild_buff_t : public druid_buff_t < buff_t >
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

public:
  heart_of_the_wild_buff_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "heart_of_the_wild" )
            .spell( select_spell( p ) ) )
  {
    add_invalidate( CACHE_PLAYER_HEAL_MULTIPLIER );
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

};
} // end namespace buffs

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

  druid_t* p()
  { return static_cast<druid_t*>( ab::player ); }
  const druid_t* p() const
  { return static_cast<druid_t*>( ab::player ); }

  druid_td_t* td( player_t* t ) const
  { return p() -> get_target_data( t ); }

  virtual void impact( action_state_t* s )
  {
    ab::impact( s );

    if ( p() -> sets.has_set_bonus( DRUID_FERAL, T17, B4 ) )
      trigger_gushing_wound( s -> target, s -> result_amount );

    if ( ab::aoe == 0 && s -> result_total > 0 && p() -> buff.natures_vigil -> up() ) 
      p() -> active.natures_vigil -> trigger( ab::harmful ? s -> result_amount :  s -> result_total , ab::harmful ); // Natures Vigil procs from overhealing
  }

  virtual void tick( dot_t* d )
  {
    ab::tick( d );

    if ( p() -> sets.has_set_bonus( DRUID_FERAL, T17, B4 ) )
      trigger_gushing_wound( d -> target, d -> state -> result_amount );

    if ( ab::aoe == 0 && d -> state -> result_total > 0 && p() -> buff.natures_vigil -> up() )
      p() -> active.natures_vigil -> trigger( ab::harmful ? d -> state -> result_amount : d -> state -> result_total, ab::harmful );
  }

  virtual void multistrike_tick( const action_state_t* src_state, action_state_t* ms_state, double multiplier )
  {
    ab::multistrike_tick( src_state, ms_state, multiplier );

    if ( p() -> sets.has_set_bonus( DRUID_FERAL, T17, B4 ) )
      trigger_gushing_wound( ms_state -> target, ms_state -> result_amount );

    if ( ab::aoe == 0 && ms_state -> result_total > 0 && p() -> buff.natures_vigil -> up() )
      p() -> active.natures_vigil -> trigger( ab::harmful ? ms_state -> result_amount : ms_state -> result_total, ab::harmful );
  }

  void trigger_gushing_wound( player_t* t, double dmg )
  {
    if ( ! ( p() -> buff.berserk -> check() && ab::special && ab::harmful ) )
      return;

    residual_action::trigger(
      p() -> active.gushing_wound, // ignite spell
      t, // target
      p() -> find_spell( 165432 ) -> effectN( 1 ).percent() * dmg );
  }

  virtual expr_t* create_expression( const std::string& name_str )
  {
    if ( ! util::str_compare_ci( name_str, "dot.lacerate.stack" ) )
      return ab::create_expression( name_str );

    struct lacerate_stack_expr_t : public expr_t
    {
      druid_t& druid;
      action_t& action;

      lacerate_stack_expr_t( action_t& a, druid_t& p ) :
        expr_t( "stack" ), druid( p ), action( a )
      {}
      virtual double evaluate()
      {
        return druid.get_target_data( action.target ) -> lacerate_stack;
      }
    };

    return new lacerate_stack_expr_t( *this, *p() );
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
  
  bool consume_bloodtalons;

  druid_attack_t( const std::string& n, druid_t* player,
                  const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ), consume_bloodtalons( false )
  {
    ab::may_glance    = false;
    ab::special       = true;
  }

  virtual void init()
  {
    ab::init();

    consume_bloodtalons = ab::harmful && ab::special;
  }

  virtual void execute()
  {
    ab::execute();

    if ( ab::p() -> talent.bloodtalons -> ok() && consume_bloodtalons & ab::p() -> buff.bloodtalons -> up() )
      ab::p() -> buff.bloodtalons -> decrement();
  }

  virtual void impact( action_state_t* s )
  {
    ab::impact( s );

    if ( ab::p() -> spec.leader_of_the_pack -> ok() && s -> result == RESULT_CRIT )
      trigger_lotp( s );

    if ( ! ab::special )
    {
      if ( ab::result_is_hit( s -> result ) )
        trigger_omen_of_clarity();
      else if ( ab::result_is_multistrike( s -> result ) )
        trigger_ursa_major();
    }
  }

  virtual void tick( dot_t* d )
  {
    ab::tick( d );

    if ( ab::p() -> sets.has_set_bonus( DRUID_FERAL, T17, B2 ) && dbc::is_school( ab::school, SCHOOL_PHYSICAL ) )
      ab::p() -> resource_gain( RESOURCE_ENERGY, 3.0, ab::p() -> gain.tier17_2pc_melee );
  }

  virtual double composite_persistent_multiplier( const action_state_t* s ) const
  {
    double pm = ab::composite_persistent_multiplier( s );
    
    if ( ab::p() -> talent.bloodtalons -> ok() && consume_bloodtalons && ab::p() -> buff.bloodtalons -> check() )
      pm *= 1.0 + ab::p() -> buff.bloodtalons -> data().effectN( 1 ).percent();

    return pm;
  }
  
  virtual double composite_target_multiplier( player_t* t ) const
  {
    double tm = ab::composite_target_multiplier( t );

    /* Assume that any action that deals physical and applies a dot deals all bleed damage, so
       that it scales direct "bleed" damage. This is a bad assumption if there is an action
       that applies a dot but does plain physical direct damage, but there are none of those. */
    if ( dbc::is_school( ab::school, SCHOOL_PHYSICAL ) && ab::dot_duration > timespan_t::zero() )
      tm *= 1.0 + ab::td( t ) -> buffs.bloodletting -> value();

    return tm;
  }

  void trigger_lotp( const action_state_t* s )
  {
    // Must be in cat or bear form
    if ( ! ( ab::p() -> buff.cat_form -> check() || ab::p() -> buff.bear_form -> check() ) )
      return;
    // Has to do damage and can't be a proc
    if ( ab::proc || s -> result_amount <= 0 )
      return;

    if ( ab::p() -> active.leader_of_the_pack -> ready() )
      ab::p() -> active.leader_of_the_pack -> execute();
  }

  void trigger_ursa_major()
  {
    if ( ! ab::p() -> spec.ursa_major -> ok() )
      return;
    
    ab::p() -> proc.ursa_major -> occur();

    if ( ab::p() -> buff.ursa_major -> check() )
    {
      double remaining_value = ab::p() -> buff.ursa_major -> value() * ( ab::p() -> buff.ursa_major -> remains() / ab::p() -> buff.ursa_major -> buff_duration );
      ab::p() -> buff.ursa_major -> trigger( 1, ab::p() -> buff.ursa_major -> default_value + remaining_value );
    } else
      ab::p() -> buff.ursa_major -> trigger();
  }

  void trigger_omen_of_clarity()
  {
    if ( ab::proc )
      return;
    if ( ! ( ab::p() -> specialization() == DRUID_FERAL && ab::p() -> spec.omen_of_clarity -> ok() ) )
      return;

    if ( ab::rng().roll( ab::weapon -> proc_chance_on_swing( 3.5 ) ) ) // 3.5 PPM via https://twitter.com/Celestalon/status/482329896404799488
      if ( ab::p() -> buff.omen_of_clarity -> trigger() && ab::p() -> sets.has_set_bonus( SET_MELEE, T16, B2 ) )
        ab::p() -> buff.feral_fury -> trigger();
  }

};

namespace caster_attacks {

// Caster Form Melee Attack ========================================================

struct caster_attack_t : public druid_attack_t < melee_attack_t >
{
  caster_attack_t( const std::string& token, druid_t* p,
                const spell_data_t* s = spell_data_t::nil(),
                const std::string& options = std::string() ) :
    base_t( token, p, s )
  {
    parse_options( 0, options );
  }
}; // end druid_caster_attack_t

struct druid_melee_t : public caster_attack_t
{
  druid_melee_t( druid_t* p ) :
    caster_attack_t( "melee", p )
  {
    school      = SCHOOL_PHYSICAL;
    may_glance  = background = repeating = true;
    trigger_gcd = timespan_t::zero();
    special     = false;
  }

  virtual timespan_t execute_time() const
  {
    if ( ! player -> in_combat )
      return timespan_t::from_seconds( 0.01 );

    return caster_attack_t::execute_time();
  }
};

}

namespace cat_attacks {

// ==========================================================================
// Druid Cat Attack
// ==========================================================================

struct cat_attack_t : public druid_attack_t < melee_attack_t >
{
  bool             requires_stealth;
  int              combo_point_gain;
  double           base_dd_bonus;
  double           base_td_bonus;
  bool             consume_ooc;

  cat_attack_t( const std::string& token, druid_t* p,
                const spell_data_t* s = spell_data_t::nil(),
                const std::string& options = std::string() ) :
    base_t( token, p, s ),
    requires_stealth( false ), combo_point_gain( 0 ),
    base_dd_bonus( 0.0 ), base_td_bonus( 0.0 ), consume_ooc( true )
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
        combo_point_gain = ed.base_value();
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
  virtual double cost() const
  {
    double c = base_t::cost();

    if ( c == 0 )
      return 0;

    if ( consume_ooc && p() -> buff.omen_of_clarity -> check() )
      return 0;

    if ( p() -> buff.berserk -> check() )
      c *= 1.0 + p() -> spell.berserk_cat -> effectN( 1 ).percent();

    return c;
  }

  virtual bool stealthed() const
  {
    return p() -> buff.prowl -> up() || p() -> buff.king_of_the_jungle -> up() || p() -> buffs.shadowmeld -> up();
  }

  void trigger_glyph_of_savage_roar()
  {
    // Bail out if we have Savagery
    if ( p() -> glyph.savagery -> ok() )
      return;

    timespan_t duration = p() -> spec.savage_roar -> duration() + timespan_t::from_seconds( 6.0 ) * 5;
    
    /* Savage Roar behaves like a DoT with DOT_REFRESH and has a maximum duration equal
       to 130% of the base duration.
     * Per Alpha testing (6/16/2014), the maximum duration is 130% of the raw duration
       of the NEW Savage Roar. */
    if ( p() -> buff.savage_roar -> check() )
      duration += std::min( p() -> buff.savage_roar -> remains(), duration * 0.3 );

    p() -> buff.savage_roar -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration );
  }

  virtual void execute()
  {
    base_t::execute();

    if ( this -> base_costs[ RESOURCE_COMBO_POINT ] > 0 )
    {
      if ( player -> sets.has_set_bonus( SET_MELEE, T15, B2 ) &&
          rng().roll( cost() * 0.15 ) )
      {
        p() -> proc.tier15_2pc_melee -> occur();
        p() -> resource_gain( RESOURCE_COMBO_POINT, 1, p() -> gain.tier15_2pc_melee );
      }

      if ( p() -> buff.feral_rage -> up() ) // tier16_4pc_melee
      {
        p() -> resource_gain( RESOURCE_COMBO_POINT, p() -> buff.feral_rage -> data().effectN( 1 ).base_value(), p() -> gain.feral_rage );
        p() -> buff.feral_rage -> expire();
      }
    }

    if ( ! result_is_hit( execute_state -> result ) )
      trigger_energy_refund();

    if ( harmful )
    {
      p() -> buff.prowl -> expire();
      p() -> buffs.shadowmeld -> expire();
      
      // Track benefit of damage buffs
      p() -> buff.tigers_fury -> up();
      p() -> buff.savage_roar -> up();
      if ( special )
        p() -> buff.berserk -> up();
    }
  }

  virtual void impact( action_state_t* s )
  {
    base_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> spec.predatory_swiftness -> ok() && base_costs[ RESOURCE_COMBO_POINT ] )
        p() -> buff.predatory_swiftness -> trigger( 1, 1, p() -> resources.current[ RESOURCE_COMBO_POINT ] * 0.20 );

      if ( combo_point_gain && p() -> spell.primal_fury -> ok() && 
           s -> target == target && s -> result == RESULT_CRIT ) // Only trigger from primary target
      {
        p() -> proc.primal_fury -> occur();
        p() -> resource_gain( RESOURCE_COMBO_POINT, p() -> spell.primal_fury -> effectN( 1 ).base_value(), p() -> gain.primal_fury );
      }
    }
  }

  virtual void consume_resource()
  {
    base_t::consume_resource();

    if ( base_costs[ RESOURCE_COMBO_POINT ] && result_is_hit( execute_state -> result ) )
    {
      int consumed = (int) p() -> resources.current[ RESOURCE_COMBO_POINT ];

      p() -> resource_loss( RESOURCE_COMBO_POINT, consumed, 0, this );

      if ( sim -> log )
        sim -> out_log.printf( "%s consumes %d %s for %s (%d)",
                               player -> name(),
                               consumed,
                               util::resource_type_string( RESOURCE_COMBO_POINT ),
                               name(),
                               player -> resources.current[ RESOURCE_COMBO_POINT ] );

      if ( p() -> talent.soul_of_the_forest -> ok() )
        p() -> resource_gain( RESOURCE_ENERGY,
                              consumed * p() -> talent.soul_of_the_forest -> effectN( 1 ).base_value(),
                              p() -> gain.soul_of_the_forest );
    }

    if ( consume_ooc && base_t::cost() > 0 && p() -> buff.omen_of_clarity -> up() )
    {
      // Treat the savings like a energy gain.
      p() -> gain.omen_of_clarity -> add( RESOURCE_ENERGY, base_t::cost() );
      p() -> buff.omen_of_clarity -> expire();
    }
  }

  virtual bool ready()
  {
    if ( ! base_t::ready() )
      return false;

    if ( ! p() -> buff.cat_form -> check() )
      return false;

    if ( requires_stealth && ! stealthed() )
      return false;

    if ( p() -> resources.current[ RESOURCE_COMBO_POINT ] < base_costs[ RESOURCE_COMBO_POINT ] )
      return false;

    return true;
  }

  virtual double composite_persistent_multiplier( const action_state_t* s ) const
  {
    double pm = base_t::composite_persistent_multiplier( s );

    if ( p() -> buff.cat_form -> check() && dbc::is_school( s -> action -> school, SCHOOL_PHYSICAL ) )
      pm *= 1.0 + p() -> buff.tigers_fury -> check() * p() -> buff.tigers_fury -> default_value; // Avoid using value() to prevent skewing benefit_pct.

    return pm;
  }

  virtual double composite_ta_multiplier( const action_state_t* s ) const
  {
    double tm = base_t::composite_ta_multiplier( s );

    if ( p() -> mastery.razor_claws -> ok() && dbc::is_school( s -> action -> school, SCHOOL_PHYSICAL ) )
      tm *= 1.0 + p() -> cache.mastery_value();

    return tm;
  }

  void trigger_energy_refund()
  {
    double energy_restored = resource_consumed * 0.80;

    player -> resource_gain( RESOURCE_ENERGY, energy_restored, p() -> gain.energy_refund );
  }
}; // end druid_cat_attack_t


// Cat Melee Attack =========================================================

struct cat_melee_t : public cat_attack_t
{
  cat_melee_t( druid_t* player ) :
    cat_attack_t( "cat_melee", player, spell_data_t::nil(), "" )
  {
    school = SCHOOL_PHYSICAL;
    may_glance = background = repeating = true;
    trigger_gcd = timespan_t::zero();
    special = false;
  }

  virtual void init()
  {
    cat_attack_t::init();

    consume_ooc = false;
  }

  virtual timespan_t execute_time() const
  {
    if ( ! player -> in_combat )
      return timespan_t::from_seconds( 0.01 );

    return cat_attack_t::execute_time();
  }

  virtual double action_multiplier() const
  {
    double cm = cat_attack_t::action_multiplier();

    if ( p() -> buff.cat_form -> check() )
      cm *= 1.0 + p() -> buff.cat_form -> data().effectN( 3 ).percent();

    return cm;
  }
};

// Feral Charge (Cat) =======================================================

struct feral_charge_cat_t : public cat_attack_t
{
  // TODO: Figure out Wild Charge
  feral_charge_cat_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "feral_charge_cat", p, p -> talent.wild_charge, options_str )
  {
    may_miss = may_dodge = may_parry = may_block = may_glance = false;
  }

  virtual void init()
  {
    cat_attack_t::init();

    consume_ooc         = false;
    consume_bloodtalons = false;
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
    cat_attack_t( "ferocious_bite", p, p -> find_class_spell( "Ferocious Bite" ), options_str ),
    excess_energy( 0 ), max_excess_energy( 0 )
  {
    parse_options( NULL, options_str );
    base_costs[ RESOURCE_COMBO_POINT ] = 1;

    max_excess_energy      = data().effectN( 2 ).base_value();
    special                = true;
    spell_power_mod.direct = 0;
  }

  virtual void execute()
  {
    // Berserk does affect the additional energy consumption.
    if ( p() -> buff.berserk -> check() )
      max_excess_energy *= 1.0 + p() -> spell.berserk_cat -> effectN( 1 ).percent();

    excess_energy = std::min( max_excess_energy,
                              ( p() -> resources.current[ RESOURCE_ENERGY ] - cat_attack_t::cost() ) );


    cat_attack_t::execute();

    if ( p() -> buff.tier15_4pc_melee -> up() )
      p() -> buff.tier15_4pc_melee -> decrement();

    max_excess_energy = data().effectN( 2 ).base_value();
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
      if ( p() -> sets.has_set_bonus( SET_MELEE, T13, B2 ) )
        health_percentage = p() -> sets.set( SET_MELEE, T13, B2 ) -> effectN( 2 ).base_value();

      if ( state -> target -> health_percentage() <= health_percentage )
      {
        if ( td( state -> target ) -> dots.rip -> is_ticking() )
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
        sim -> out_debug.printf( "%s consumes an additional %.1f %s for %s", player -> name(),
                       excess_energy, util::resource_type_string( current_resource() ), name() );

      player -> resource_loss( current_resource(), excess_energy );
      stats -> consume_resource( current_resource(), excess_energy );
    }
  }

  double action_multiplier() const
  {
    double am = cat_attack_t::action_multiplier();

    am *= p() -> resources.current[ RESOURCE_COMBO_POINT ] / p() -> resources.max[ RESOURCE_COMBO_POINT ];

    am *= 1.0 + excess_energy / max_excess_energy;

    return am;
  }

  double composite_crit() const
  {
    double c = cat_attack_t::composite_crit();

    // TODO: Determine which bonus is applied first.

    if ( p() -> buff.tier15_4pc_melee -> check() )
      c += p() -> buff.tier15_4pc_melee -> data().effectN( 1 ).percent();

    if ( target -> debuffs.bleeding -> check() )
      c *= 2.0;

    return c;
  }
};

// Maim =====================================================================

struct maim_t : public cat_attack_t
{
  maim_t( druid_t* player, const std::string& options_str ) :
    cat_attack_t( "maim", player, player -> find_specialization_spell( "Maim" ), options_str )
  {
    base_costs[ RESOURCE_COMBO_POINT ] = 1;

    special          = true;
    base_multiplier *= 1.0 + player -> glyph.maim -> effectN( 1 ).percent();
  }
};

// Rake =====================================================================

struct rake_t : public cat_attack_t
{
  const spell_data_t* bleed_spell;

  rake_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "rake", p, p -> find_specialization_spell( "Rake" ), options_str )
  {
    special = true;
    attack_power_mod.direct = data().effectN( 1 ).ap_coeff();

    bleed_spell = p -> find_spell( 155722 );
    dot_behavior          = DOT_REFRESH;
    attack_power_mod.tick = bleed_spell -> effectN( 1 ).ap_coeff();
    dot_duration          = bleed_spell -> duration();
    base_tick_time        = bleed_spell -> effectN( 1 ).period();
  }

  virtual double composite_persistent_multiplier( const action_state_t* s ) const
  {
    double pm = cat_attack_t::composite_persistent_multiplier( s );

    if ( stealthed() )
      pm *= 1.0 + p() -> perk.improved_rake -> effectN( 2 ).percent();

    return pm;
  }

  /* Treat direct damage as "bleed"
     Must use direct damage because tick_zeroes cannot be blocked, and
     this attack can be blocked if the druid is in front of the target. */
  double composite_da_multiplier( const action_state_t* state ) const
  {
    double dm = cat_attack_t::composite_da_multiplier( state );

    if ( p() -> mastery.razor_claws -> ok() )
      dm *= 1.0 + p() -> cache.mastery_value();

    return dm;
  }

  // Bleed damage penetrates armor.
  virtual double target_armor( player_t* ) const
  { return 0.0; }

  virtual void impact( action_state_t* s )
  {
    cat_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> resource_gain( RESOURCE_COMBO_POINT, combo_point_gain, p() -> gain.rake );

      if ( p() -> sets.has_set_bonus( DRUID_FERAL, T17, B2 ) )
        p() -> resource_gain( RESOURCE_ENERGY, 3.0, p() -> gain.tier17_2pc_melee );
    }
  }

  virtual void execute()
  {
    if ( p() -> glyph.savage_roar -> ok() && stealthed() )
      trigger_glyph_of_savage_roar();

    cat_attack_t::execute();

    // Track buff benefits
    p() -> buff.king_of_the_jungle -> up();
  }
};

// Rip ======================================================================

struct rip_t : public cat_attack_t
{
  struct rip_state_t : public action_state_t
  {
    int combo_points;
    druid_t* druid;

    rip_state_t( druid_t* p, action_t* a, player_t* target ) :
      action_state_t( a, target ), combo_points( 0 ), druid( p )
    { }

    void initialize()
    {
      action_state_t::initialize();

      combo_points = (int) druid -> resources.current[ RESOURCE_COMBO_POINT ];
    }

    void copy_state( const action_state_t* state )
    {
      action_state_t::copy_state( state );
      const rip_state_t* rip_state = debug_cast<const rip_state_t*>( state );
      combo_points = rip_state -> combo_points;
    }

    std::ostringstream& debug_str( std::ostringstream& s )
    {
      action_state_t::debug_str( s );

      s << " combo_points=" << combo_points;

      return s;
    }
  };

  double ap_per_point;

  rip_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "rip", p, p -> find_specialization_spell( "Rip" ), options_str ),
    ap_per_point( 0.0 )
  {
    base_costs[ RESOURCE_COMBO_POINT ] = 1;

    ap_per_point = data().effectN( 1 ).ap_coeff();
    special      = true;
    may_crit     = false;
    dot_behavior = DOT_REFRESH;

    dot_duration += player -> sets.set( SET_MELEE, T14, B4 ) -> effectN( 1 ).time_value();
  }

  action_state_t* new_state()
  { return new rip_state_t( p(), this, target ); }

  double attack_tick_power_coefficient( const action_state_t* s ) const
  {
    rip_state_t* rip_state = debug_cast<rip_state_t*>( td( s -> target ) -> dots.rip -> state );

    return ap_per_point * rip_state -> combo_points;
  }
};

// Savage Roar ==============================================================

struct savage_roar_t : public cat_attack_t
{
  savage_roar_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "savage_roar", p, p -> find_specialization_spell( "Savage Roar" ), options_str )
  {
    base_costs[ RESOURCE_COMBO_POINT ] = 1;

    may_miss = harmful = false;
    dot_duration  = timespan_t::zero();
    base_tick_time = timespan_t::zero();
  }

  timespan_t duration( int combo_points = -1 )
  {
    if ( combo_points == -1 )
      combo_points = (int) p() -> resources.current[ RESOURCE_COMBO_POINT ];
  
    timespan_t d = data().duration() + timespan_t::from_seconds( 6.0 ) * combo_points;

     /* Savage Roar behaves like a DoT with DOT_REFRESH and has a maximum duration equal
        to 130% of the base duration.
      * Per Alpha testing (6/16/2014), the maximum duration is 130% of the raw duration
        of the NEW Savage Roar. */
    if ( p() -> buff.savage_roar -> check() )
      d += std::min( p() -> buff.savage_roar -> remains(), d * 0.3 );

    return d;
  }

  virtual void impact( action_state_t* state )
  {
    cat_attack_t::impact( state );

    p() -> buff.savage_roar -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, duration() );
  }

  virtual bool ready()
  {
    if ( p() -> glyph.savagery -> ok() )
      return false;
    else if ( duration() < p() -> buff.savage_roar -> remains() )
      return false;
    
    return cat_attack_t::ready();
  }
};

// Shred ====================================================================

struct shred_t : public cat_attack_t
{
  shred_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "shred", p, p -> find_class_spell( "Shred" ), options_str )
  {
    base_multiplier *= 1.0 + player -> sets.set( SET_MELEE, T14, B2 ) -> effectN( 1 ).percent();
    special = true;
  }

  virtual void execute()
  {
    if ( p() -> glyph.savage_roar -> ok() && stealthed() )
      trigger_glyph_of_savage_roar();

    cat_attack_t::execute();

    p() -> buff.tier15_4pc_melee -> decrement();

    // Track buff benefits
    p() -> buff.king_of_the_jungle -> up();
  }

  virtual void impact( action_state_t* s )
  {
    cat_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> resource_gain( RESOURCE_COMBO_POINT, combo_point_gain, p() -> gain.shred );

      if ( s -> result == RESULT_CRIT && p() -> sets.has_set_bonus( DRUID_FERAL, PVP, B4 ) )
        td( s -> target ) -> buffs.bloodletting -> trigger(); // Druid module debuff
    }
  }

  virtual double composite_target_multiplier( player_t* t ) const
  {
    double tm = cat_attack_t::composite_target_multiplier( t );

    if ( t -> debuffs.bleeding -> up() )
      tm *= 1.0 + p() -> find_spell( 106785 ) -> effectN( 2 ).percent();

    return tm;
  }

  double composite_crit() const
  {
    double c = cat_attack_t::composite_crit();

    // TODO: Determine which bonus is applied first.

    if ( p() -> buff.tier15_4pc_melee -> up() )
      c += p() -> buff.tier15_4pc_melee -> data().effectN( 1 ).percent();
    if ( stealthed() )
      c *= 2.0;

    return c;
  }

  double action_multiplier() const
  {
    double m = cat_attack_t::action_multiplier();

    if ( p() -> buff.feral_fury -> up() )
      m *= 1.0 + p() -> buff.feral_fury -> data().effectN( 1 ).percent();
    if ( stealthed() )
      m *= 1.0 + p() -> buff.prowl -> data().effectN( 4 ).percent();

    return m;
  }
};

// Swipe ==============================================================

struct swipe_t : public cat_attack_t
{
  swipe_t( druid_t* player, const std::string& options_str ) :
    cat_attack_t( "swipe", player, player -> find_specialization_spell( "Swipe" ), options_str )
  {
    aoe = -1;
  }

  virtual void impact( action_state_t* s )
  {
    cat_attack_t::impact( s );

    if ( s -> target == target && result_is_hit( s -> result ) )
      p() -> resource_gain( RESOURCE_COMBO_POINT, combo_point_gain, p() -> gain.swipe );
  }

  virtual void execute()
  {
    cat_attack_t::execute();

    p() -> buff.feral_fury -> up();

    if ( p() -> buff.tier15_4pc_melee -> up() )
      p() -> buff.tier15_4pc_melee -> decrement();
  }

  double action_multiplier() const
  {
    double m = cat_attack_t::action_multiplier();

    if ( p() -> buff.feral_fury -> check() )
      m *= 1.0 + p() -> buff.feral_fury -> data().effectN( 1 ).percent();

    return m;
  }

  virtual double composite_target_multiplier( player_t* t ) const
  {
    double tm = cat_attack_t::composite_target_multiplier( t );

    if ( t -> debuffs.bleeding -> up() )
      tm *= 1.0 + data().effectN( 2 ).percent();

    return tm;
  }

  double composite_target_crit( player_t* t ) const
  {
    double tc = cat_attack_t::composite_target_crit( t );

    if ( p() -> buff.tier15_4pc_melee -> check() )
      tc += p() -> buff.tier15_4pc_melee -> data().effectN( 1 ).percent();

    return tc;
  }
};

// Thrash (Cat) =============================================================

struct thrash_cat_t : public cat_attack_t
{
  thrash_cat_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "thrash_cat", p, p -> find_spell( 106830 ), options_str )
  {
    aoe                    = -1;
    dot_behavior           = DOT_REFRESH;
    spell_power_mod.direct = 0;
  }

  // Treat direct damage as "bleed"
  double composite_da_multiplier( const action_state_t* state ) const
  {
    double m = cat_attack_t::composite_da_multiplier( state );

    if ( p() -> mastery.razor_claws -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  virtual void impact( action_state_t* s )
  {
    cat_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      this -> td( s -> target ) -> dots.thrash_bear -> cancel();

      if ( p() -> sets.has_set_bonus( DRUID_FERAL, T17, B2 ) )
        p() -> resource_gain( RESOURCE_ENERGY, 3.0, p() -> gain.tier17_2pc_melee );
    }
  }

  // Treat direct damage as "bleed"
  virtual double target_armor( player_t* ) const
  { return 0.0; }
};

} // end namespace cat_attacks

namespace bear_attacks {

// ==========================================================================
// Druid Bear Attack
// ==========================================================================

struct bear_attack_t : public druid_attack_t<melee_attack_t>
{
public:
  double rage_amount, rage_tick_amount;

  bear_attack_t( const std::string& n, druid_t* p,
                 const spell_data_t* s = spell_data_t::nil() ) :
    base_t( n, p, s ), rage_gain( p -> get_gain( name() ) ),
    rage_amount( 0.0 ), rage_tick_amount( 0.0 )
  {}

  virtual timespan_t gcd() const
  {
    if ( p() -> specialization() != DRUID_GUARDIAN )
      return base_t::gcd();

    timespan_t t = base_t::gcd();

    if ( t == timespan_t::zero() )
      return timespan_t::zero();

    t *= player -> cache.attack_haste();

    if ( t < min_gcd )
      t = min_gcd;

    return t;
  }

  virtual double cooldown_reduction() const
  {
    double cdr = base_t::cooldown_reduction();

    cdr *= p() -> cache.attack_haste();

    return cdr;
  }

  virtual void impact( action_state_t* s )
  {
    base_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> resource_gain( RESOURCE_RAGE, rage_amount, rage_gain );

      if ( p() -> spell.primal_fury -> ok() && s -> target == target && s -> result == RESULT_CRIT ) // Only trigger from primary target
      {
        p() -> resource_gain( RESOURCE_RAGE,
                              p() -> spell.primal_fury -> effectN( 1 ).resource( RESOURCE_RAGE ),
                              p() -> gain.primal_fury );
        p() -> proc.primal_fury -> occur();
      }
    }
  }

  virtual void tick( dot_t* d )
  {
    base_t::tick( d );

    p() -> resource_gain( RESOURCE_RAGE, rage_tick_amount, rage_gain );
  }

  virtual bool ready() 
  {
    if ( ! p() -> buff.bear_form -> check() )
      return false;
    
    return base_t::ready();
  }
private:
  gain_t* rage_gain;
}; // end druid_bear_attack_t

// Bear Melee Attack ========================================================

struct bear_melee_t : public bear_attack_t
{
  bear_melee_t( druid_t* player ) :
    bear_attack_t( "bear_melee", player )
  {
    school      = SCHOOL_PHYSICAL;
    may_glance  = background = repeating = true;
    trigger_gcd = timespan_t::zero();
    special     = false;

    rage_amount = 5.0;
  }

  virtual timespan_t execute_time() const
  {
    if ( ! player -> in_combat )
      return timespan_t::from_seconds( 0.01 );

    return bear_attack_t::execute_time();
  }

  virtual void impact( action_state_t* state )
  {
    bear_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      if ( rng().roll( p() -> spec.tooth_and_claw -> proc_chance() ) )
      {
        if ( p() -> buff.tooth_and_claw -> current_stack == p() -> buff.tooth_and_claw -> max_stack() )
          p() -> proc.tooth_and_claw_wasted -> occur();
        p() -> buff.tooth_and_claw -> trigger();
        p() -> proc.tooth_and_claw -> occur();
      }
    }
  }
};

// Lacerate =================================================================

struct lacerate_t : public bear_attack_t
{
  lacerate_t( druid_t* p, const std::string& options_str ) :
    bear_attack_t( "lacerate", p, p -> find_specialization_spell( "Lacerate" ) )
  {
    parse_options( NULL, options_str );
    dot_behavior = DOT_REFRESH;

    rage_amount = data().effectN( 3 ).resource( RESOURCE_RAGE );
  }

  virtual void impact( action_state_t* state )
  {
    if ( result_is_hit( state -> result ) )
    {
      if ( td( state -> target ) -> lacerate_stack < 3 )
        td( state -> target ) -> lacerate_stack++;

      if ( rng().roll( 0.25 ) ) // FIXME: Find in spell data.
        p() -> cooldown.mangle -> reset( true );
    }

    bear_attack_t::impact( state );
  }

  virtual double composite_target_ta_multiplier( player_t* t ) const
  {
    double tm = bear_attack_t::composite_target_ta_multiplier( t );

    tm *= td( t ) -> lacerate_stack;

    return tm;
  }

  virtual void multistrike_tick( const action_state_t* src_state, action_state_t* ms_state, double multiplier )
  {
    bear_attack_t::multistrike_tick( src_state, ms_state, multiplier );

    trigger_ursa_major();
  }

  virtual void last_tick( dot_t* d )
  {
    bear_attack_t::last_tick( d );

    td( target ) -> lacerate_stack = 0;
  }
};

// Mangle ============================================================

struct mangle_t : public bear_attack_t
{
  mangle_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "mangle", player, player -> find_class_spell( "Mangle" ) )
  {
    parse_options( NULL, options_str );

    rage_amount = data().effectN( 3 ).resource( RESOURCE_RAGE ) + player -> talent.soul_of_the_forest -> effectN( 1 ).resource( RESOURCE_RAGE );

    base_multiplier *= 1.0 + player -> talent.soul_of_the_forest -> effectN( 2 ).percent();
  }

  virtual double composite_crit() const
  {
    double c = bear_attack_t::composite_crit();

    if ( p() -> specialization() == DRUID_GUARDIAN )
      c += p() -> talent.dream_of_cenarius -> effectN( 3 ).percent();

    return c;
  }

  void update_ready( timespan_t )
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.berserk -> check() || p() -> buff.son_of_ursoc -> check() )
      cd = timespan_t::zero();

    bear_attack_t::update_ready( cd );
  }

  virtual void execute()
  {
    int base_aoe = aoe;
    if ( p() -> buff.berserk -> up() )
      aoe = p() -> spell.berserk_bear -> effectN( 1 ).base_value();

    bear_attack_t::execute();

    aoe = base_aoe;
  }

  virtual void impact( action_state_t* s )
  {
    bear_attack_t::impact( s );

    if ( result_is_multistrike( s -> result ) )
      trigger_ursa_major();
  }
};

// Maul =====================================================================

struct maul_t : public bear_attack_t
{ 
  struct tooth_and_claw_t : public druid_action_t<absorb_t>
  {
    /* FIXME: Functions as an absorb that only benefits the druid, to support multi-tank sims correctly
       it must cause the target's next attack to mitigated (applying Resolve appropriately).
       TODO: Determine if this uses the druid's resolve on application or on consumption. */
    tooth_and_claw_t( druid_t* p ) :
      druid_action_t<absorb_t>( "tooth_and_claw", p, p -> spec.tooth_and_claw )
    {
      harmful = special = may_crit = false;
      may_multistrike = 0;
      target = player;

      /* Coeff is hardcoded into tooltip and isn't in spell data, so hardcode coeff.
         Used to be able to use FR coeff * x but can't anymore because of recent changes. */
      attack_power_mod.direct = 2.40;
    }

    virtual void impact( action_state_t* s )
    {
      if ( p() -> sets.has_set_bonus( DRUID_GUARDIAN, T17, B4 ) )
        p() -> buff.tier17_4pc_tank -> increment();

      p() -> buff.tooth_and_claw_absorb -> trigger( 1, s -> result_amount );
    }
  };

  tooth_and_claw_t* absorb;
  double cost_reduction;

  maul_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "maul", player, player -> find_specialization_spell( "Maul" ) ),
    cost_reduction( 0.0 )
  {
    parse_options( NULL, options_str );
    weapon = &( player -> main_hand_weapon );

    aoe = player -> glyph.maul -> effectN( 1 ).base_value();
    base_add_multiplier = player -> glyph.maul -> effectN( 3 ).percent();
    use_off_gcd = true;

    if ( player -> spec.tooth_and_claw -> ok() )
    {
      absorb = new tooth_and_claw_t( player );
      if ( p() -> sets.has_set_bonus( DRUID_GUARDIAN, T17, B2 ) )
        cost_reduction = p() -> find_spell( 165410 ) -> effectN( 1 ).resource( RESOURCE_RAGE ) * -1.0;
    }
  }

  virtual double cost() const
  {
    double c = bear_attack_t::cost();

    if ( p() -> buff.tooth_and_claw -> check() && p() -> sets.has_set_bonus( DRUID_GUARDIAN, T17, B2 ) )
      c -= cost_reduction;

    return bear_attack_t::cost();
  }

  virtual double composite_target_multiplier( player_t* t ) const
  {
    double tm = bear_attack_t::composite_target_multiplier( t );

    if ( t -> debuffs.bleeding -> up() )
      tm *= 1.0 + data().effectN( 3 ).percent();

    return tm;
  }

  virtual void update_ready( timespan_t )
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.son_of_ursoc -> check() )
      cd = timespan_t::zero();

    bear_attack_t::update_ready( cd );
  }

  virtual void impact( action_state_t* s )
  {
    bear_attack_t::impact( s );

    if ( result_is_hit( s -> result ) && p() -> buff.tooth_and_claw -> up() )
    {
      if ( p() -> sets.has_set_bonus( DRUID_GUARDIAN, T17, B2 ) )
        p() -> gain.tier17_2pc_tank -> add( RESOURCE_RAGE, cost_reduction );

      absorb -> execute();
      p() -> buff.tooth_and_claw -> decrement(); // Only decrements on hit, tested 6/27/2014 by Zerrahki
    }
  }
};

// Pulverize ================================================================

struct pulverize_t : public bear_attack_t
{
  pulverize_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "pulverize", player, player -> talent.pulverize )
  {
    parse_options( NULL, options_str );
  }

  virtual void impact( action_state_t* s )
  {
    bear_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      // consumes 3 stacks of Lacerate on the target
      target -> get_dot( "lacerate", p() ) -> cancel();
      // and reduce damage taken by 20% for 10 sec
      p() -> buff.pulverize -> trigger();
    }
  }

  virtual bool ready()
  {
    if ( td( target ) -> lacerate_stack < 3 )
      return false;

    return bear_attack_t::ready();
  }
};

// Thrash (Bear) ============================================================

struct thrash_bear_t : public bear_attack_t
{
  thrash_bear_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "thrash_bear", player, player -> find_spell( 77758 ) )
  {
    parse_options( NULL, options_str );
    aoe                    = -1;
    dot_behavior           = DOT_REFRESH;
    spell_power_mod.direct = 0;

    rage_amount = rage_tick_amount = p() -> find_spell( 158723 ) -> effectN( 1 ).resource( RESOURCE_RAGE );
  }

  virtual void impact( action_state_t* s )
  {
    bear_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      this -> td( s -> target ) -> dots.thrash_cat -> cancel();

      if ( p() -> sets.has_set_bonus( DRUID_FERAL, T17, B2 ) )
        p() -> resource_gain( RESOURCE_ENERGY, 3.0, p() -> gain.tier17_2pc_melee );
    }
  }

  // Treat direct damage as "bleed"
  virtual double target_armor( player_t* ) const
  { return 0.0; }
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
    consume_ooc( true )
  {}

  virtual void consume_resource()
  {
    ab::consume_resource();
    druid_t& p = *this -> p();

    if ( consume_ooc && ab::cost() > 0 && ( this -> execute_time() != timespan_t::zero() || p.specialization() == DRUID_FERAL ) && p.buff.omen_of_clarity -> up() )
    {
      // Treat the savings like a mana gain.
      p.gain.omen_of_clarity -> add( RESOURCE_MANA, ab::cost() );
      p.buff.omen_of_clarity -> expire();
    }
  }

  virtual double cost() const
  {
    if ( consume_ooc && ( this -> execute_time() != timespan_t::zero() || ab::id == 155625 ) && this -> p() -> buff.omen_of_clarity -> check() )
      return 0;

    return std::max( 0.0, ab::cost() * ( 1.0 + cost_reduction() ) );
  }

  virtual double cost_reduction() const
  { return 0.0; }

  virtual double composite_haste() const
  {
    double h = ab::composite_haste();

    return h;
  }

  virtual timespan_t execute_time() const
  {
    if ( this -> p() -> buff.empowered_moonkin -> up() )
      return timespan_t::zero();

    return ab::execute_time();
  }

  virtual void execute()
  {
    ab::execute();

    if ( ab::base_execute_time > timespan_t::zero() )
      this -> p() -> buff.empowered_moonkin -> decrement();
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
    harmful           = false;
  }
    
protected:
  void init_living_seed();

public:
  virtual double cost() const
  {
    if ( p() -> buff.heart_of_the_wild -> heals_are_free() && current_resource() == RESOURCE_MANA )
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
      p() -> buff.harmony -> trigger( 1, p() -> mastery.harmony -> ok() ? p() -> cache.mastery_value() : 0.0 );
  }

  virtual timespan_t execute_time() const
  {
    if ( p() -> buff.natures_swiftness -> check() )
      return timespan_t::zero();

    return base_t::execute_time();
  }

  virtual double composite_haste() const
  {
    double h = base_t::composite_haste();

    h *= 1.0 / ( 1.0 + p() -> buff.soul_of_the_forest -> value() );

    return h;
  }

  virtual double action_da_multiplier() const
  {
    double adm = base_t::action_da_multiplier();

    if ( p() -> buff.tree_of_life -> up() )
      adm *= 1.0 + p() -> buff.tree_of_life -> data().effectN( 1 ).percent();

    if ( p() -> buff.natures_swiftness -> check() && base_execute_time > timespan_t::zero() )
      adm *= 1.0 + p() -> buff.natures_swiftness -> data().effectN( 2 ).percent();

    if ( p() -> mastery.harmony -> ok() )
      adm *= 1.0 + p() -> cache.mastery_value();

    return adm;
  }

  virtual double action_ta_multiplier() const
  {
    double adm = base_t::action_ta_multiplier();

    if ( p() -> buff.tree_of_life -> up() )
      adm += p() -> buff.tree_of_life -> data().effectN( 2 ).percent();

    if ( p() -> buff.natures_swiftness -> check() && base_execute_time > timespan_t::zero() )
      adm += p() -> buff.natures_swiftness -> data().effectN( 3 ).percent();

    adm += p() -> buff.harmony -> value();

    return adm;
  }

  void trigger_lifebloom_refresh( action_state_t* s )
  {
    druid_td_t& td = *this -> td( s -> target );

    if ( td.dots.lifebloom -> is_ticking() )
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

  void trigger_omen_of_clarity()
  {
    if ( ! proc && p() -> specialization() == DRUID_RESTORATION && p() -> spec.omen_of_clarity -> ok() )
      p() -> buff.omen_of_clarity -> trigger(); // Proc chance is handled by buff chance
  }
}; // end druid_heal_t

struct living_seed_t : public druid_heal_t
{
  living_seed_t( druid_t* player ) :
    druid_heal_t( "living_seed", player, player -> find_specialization_spell( "Living Seed" ) )
  {
    background = true;
    may_crit   = false;
    proc       = true;
    school     = SCHOOL_NATURE;
  }

  double composite_da_multiplier( const action_state_t* ) const
  {
    return data().effectN( 1 ).percent();
  }
};

void druid_heal_t::init_living_seed()
{
  if ( p() -> specialization() == DRUID_RESTORATION )
    living_seed = new living_seed_t( p() );
}

// Frenzied Regeneration ====================================================

struct frenzied_regeneration_t : public druid_heal_t
{
  double maximum_rage_cost;

  frenzied_regeneration_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "frenzied_regeneration", p, p -> find_class_spell( "Frenzied Regeneration" ), options_str ),
    maximum_rage_cost( 0.0 )
  {
    parse_options( NULL, options_str );
    use_off_gcd = true;
    may_crit = false;
    may_multistrike = 0;
    target = p;
    // Hardcode AP coeff because spelldata is incorrect. According to Arielle coeff is 2.4 AP as of 9/12/2014
    attack_power_mod.direct = 2.40;
    maximum_rage_cost = data().effectN( 2 ).base_value();

    if ( p -> sets.has_set_bonus( SET_TANK, T16, B2 ) )
      p -> active.ursocs_vigor = new ursocs_vigor_t( p );
  }

  virtual double cost() const
  {
    const_cast<frenzied_regeneration_t*>(this) -> base_costs[ RESOURCE_RAGE ] = std::min( p() -> resources.current[ RESOURCE_RAGE ],
                                                maximum_rage_cost );

    return druid_heal_t::cost();
  }

  virtual double action_multiplier() const
  {
    double am = druid_heal_t::action_multiplier();

    am *= cost() / 60.0;
    am *= 1.0 + p() -> buff.tier17_4pc_tank -> default_value * p() -> buff.tier17_4pc_tank -> current_stack;

    return am;
  }

  virtual void execute()
  {
    // Benefit tracking
    p() -> buff.tier17_4pc_tank -> up();

    p() -> buff.tier15_2pc_tank -> expire();
    p() -> buff.tier17_4pc_tank -> expire();

    druid_heal_t::execute();

    if ( p() -> sets.has_set_bonus( SET_TANK, T16, B4 ) )
      p() -> active.ursocs_vigor -> trigger_hot( resource_consumed );
  }

  virtual bool ready()
  {
    if ( ! p() -> buff.bear_form -> check() )
      return false;

    if ( p() -> resources.current[ RESOURCE_RAGE ] < 1 )
      return false;
    
    // This isn't how it actually works in-game, but lets us cut some bloat from the APL.
    if ( p() -> resources.current[ RESOURCE_HEALTH ] >= p() -> resources.max[ RESOURCE_HEALTH ] )
      return false;

    return druid_heal_t::ready();
  }
};

// Healing Touch ============================================================

struct healing_touch_t : public druid_heal_t
{
  healing_touch_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "healing_touch", p, p -> find_class_spell( "Healing Touch" ), options_str )
  {
    consume_ooc      = true;

    init_living_seed();
  }

  double spell_direct_power_coefficient( const action_state_t* /* state */ ) const
  {
    if ( p() -> specialization() == DRUID_GUARDIAN && p() -> buff.dream_of_cenarius -> check() )
      return 0.0;
    return data().effectN( 1 ).sp_coeff();
  }

  double attack_direct_power_coefficient( const action_state_t* /* state */ ) const
  {
    if ( p() -> specialization() == DRUID_GUARDIAN && p() -> buff.dream_of_cenarius -> check() )
      return data().effectN( 1 ).sp_coeff();
    return 0.0;
  }

  virtual double cost() const
  {
    if ( p() -> buff.predatory_swiftness -> check() )
      return 0;

    if ( p() -> buff.natures_swiftness -> check() )
      return 0;

    return druid_heal_t::cost();
  }

  virtual void consume_resource()
  {
    // Prevent from consuming Omen of Clarity unnecessarily
    if ( p() -> buff.predatory_swiftness -> check() )
      return;

    if ( p() -> buff.natures_swiftness -> check() )
      return;

    druid_heal_t::consume_resource();
  }

  virtual double action_da_multiplier() const
  {
    double adm = base_t::action_da_multiplier();

    if ( p() -> talent.dream_of_cenarius -> ok() ) {
      if ( p() -> specialization() == DRUID_FERAL || p() -> specialization() == DRUID_BALANCE )
        adm *= 1.0 + p() -> talent.dream_of_cenarius -> effectN( 1 ).percent();
      else if ( p() -> specialization() == DRUID_GUARDIAN )
        adm *= 1.0 + p() -> talent.dream_of_cenarius -> effectN( 2 ).percent();
    }

    if ( p() -> buff.predatory_swiftness -> check() )
      adm *= 1.0 + p() -> buff.predatory_swiftness -> data().effectN( 4 ).percent();

    if ( p() -> spell.moonkin_form -> ok() && target == p() )
      adm *= 1.0 + 0.50; // Not in spell data

    return adm;
  }

  virtual timespan_t execute_time() const
  {
    if ( p() -> buff.predatory_swiftness -> check() || p() -> buff.dream_of_cenarius -> up() )
      return timespan_t::zero();

    return druid_heal_t::execute_time();
  }

  virtual void impact( action_state_t* state )
  {
    druid_heal_t::impact( state );
    
    if ( result_is_hit( state -> result ) )
    {
      if ( ! p() -> glyph.blooming -> ok() )
        trigger_lifebloom_refresh( state );

      if ( state -> result == RESULT_CRIT )
        trigger_living_seed( state );

      p() -> cooldown.natures_swiftness -> adjust( timespan_t::from_seconds( - p() -> glyph.healing_touch -> effectN( 1 ).base_value() ) );
    }
  }

  virtual void execute()
  {
    druid_heal_t::execute();

    if ( p() -> talent.bloodtalons -> ok() )
      p() -> buff.bloodtalons -> trigger( 2 );

    if ( p() -> buff.dream_of_cenarius -> up() )
      p() -> cooldown.starfallsurge -> reset( false );

    p() -> buff.predatory_swiftness -> expire();
    p() -> buff.dream_of_cenarius -> expire();
  }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    druid_heal_t::schedule_execute( state );

    if ( ! p() -> buff.natures_swiftness -> up() &&
         ! p() -> buff.predatory_swiftness -> up() &&
         ! ( p() -> buff.dream_of_cenarius -> check() && p() -> specialization() == DRUID_GUARDIAN ) &&
         ! p() -> talent.claws_of_shirvallah -> ok() )
    {
      p() -> buff.cat_form         -> expire();
      p() -> buff.bear_form        -> expire();
    }
  }

  virtual timespan_t gcd() const
  {
    const druid_t& p = *this -> p();
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
    dot_duration        = timespan_t::zero();
    base_td          = 0;
    attack_power_mod.tick   = 0;
    base_dd_min      = data().effectN( 2 ).min( p );
    base_dd_max      = data().effectN( 2 ).max( p );
    attack_power_mod.direct = data().effectN( 2 ).sp_coeff();
  }

  virtual double composite_target_multiplier( player_t* target ) const
  {
    double ctm = druid_heal_t::composite_target_multiplier( target );

    ctm *= 1.0 + td( target ) -> buffs.lifebloom -> check();

    return ctm;
  }

  virtual double composite_da_multiplier( const action_state_t* state ) const
  {
    double cdm = druid_heal_t::composite_da_multiplier( state );

    cdm *= 1.0 + p() -> glyph.blooming -> effectN( 1 ).percent();

    return cdm;
  }
};

struct lifebloom_t : public druid_heal_t
{
  lifebloom_bloom_t* bloom;

  lifebloom_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "lifebloom", p, p -> find_class_spell( "Lifebloom" ), options_str ),
    bloom( new lifebloom_bloom_t( p ) )
  {
    may_crit   = false;

    // TODO: this can be only cast on one target, unless Tree of Life is up
  }

  virtual double composite_target_multiplier( player_t* target ) const
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

    if ( result_is_hit( state -> result ) )
      td( state -> target ) -> buffs.lifebloom -> trigger();
  }

  virtual void last_tick( dot_t* d )
  {
    if ( ! d -> state -> target -> is_sleeping() ) // Prevent crash at end of simulation
      bloom -> execute();
    td( d -> state -> target ) -> buffs.lifebloom -> expire();

    druid_heal_t::last_tick( d );
  }

  virtual void tick( dot_t* d )
  {
    druid_heal_t::tick( d );

    trigger_omen_of_clarity();
  }
};

// Regrowth =================================================================

struct regrowth_t : public druid_heal_t
{
  regrowth_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "regrowth", p, p -> find_class_spell( "Regrowth" ), options_str )
  {
    base_crit   += 0.6;
    consume_ooc  = true;

    if ( p -> glyph.regrowth -> ok() )
    {
      base_crit += p -> glyph.regrowth -> effectN( 1 ).percent();
      base_td    = 0;
      dot_duration  = timespan_t::zero();
    }

    init_living_seed();
  }

  virtual void impact( action_state_t* state )
  {
    druid_heal_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      if ( ! p() -> glyph.blooming -> ok() )
        trigger_lifebloom_refresh( state );

      if ( state -> result == RESULT_CRIT )
        trigger_living_seed( state );
    }
  }

  virtual void tick( dot_t* d )
  {
    druid_heal_t::tick( d );

    if ( result_is_hit( d -> state -> result ) && d -> state -> target -> health_percentage() <= p() -> spell.regrowth -> effectN( 1 ).percent() &&
         td( d -> state -> target ) -> dots.regrowth -> is_ticking() )
    {
      td( d -> state -> target )-> dots.regrowth -> refresh_duration();
    }
  }

  virtual timespan_t execute_time() const
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
    druid_heal_t( "rejuvenation", p, p -> find_class_spell( "Rejuvenation" ), options_str )
  {
    tick_zero = true;
  }

  virtual void schedule_execute( action_state_t* state = 0 )
  {
    druid_heal_t::schedule_execute( state );

    if ( ! p() -> perk.enhanced_rejuvenation -> ok() )
      p() -> buff.cat_form  -> expire();
    if ( ! p() -> buff.heart_of_the_wild -> check() )
      p() -> buff.bear_form -> expire();
  }

  virtual double action_ta_multiplier() const
  {
    double atm = base_t::action_ta_multiplier();

    if ( p() -> talent.dream_of_cenarius -> ok() && p() -> specialization() == DRUID_FERAL )
        atm *= 1.0 + p() -> talent.dream_of_cenarius -> effectN( 2 ).percent();

    return atm;
  }

};

// Renewal ============================================================

struct renewal_t : public druid_heal_t
{
  renewal_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "renewal", p, p -> find_talent_spell( "Renewal" ), options_str )
  {}

  virtual void init()
  {
    druid_heal_t::init();

    snapshot_flags &= ~STATE_RESOLVE; // Is not affected by resolve.
  }

  virtual void execute()
  {
    base_dd_min = base_dd_max = p() -> resources.max[ RESOURCE_HEALTH ] * data().effectN( 1 ).percent();

    druid_heal_t::execute();
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
      //dot_duration      = p -> spell.swiftmend -> duration(); Find effect ID
      proc           = true;
      tick_may_crit  = false;
    }
  };

  swiftmend_aoe_heal_t* aoe_heal;

  swiftmend_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "swiftmend", p, p -> find_class_spell( "Swiftmend" ), options_str ),
    aoe_heal( new swiftmend_aoe_heal_t( p, &data() ) )
  {
    consume_ooc = true;

    init_living_seed();
  }

  virtual void impact( action_state_t* state )
  {
    druid_heal_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      if ( state -> result == RESULT_CRIT )
        trigger_living_seed( state );

      if ( p() -> talent.soul_of_the_forest -> ok() )
        p() -> buff.soul_of_the_forest -> trigger();

      aoe_heal -> execute();
    }
  }

  virtual bool ready()
  {
    player_t* t = ( execute_state ) ? execute_state -> target : target;

    // Note: with the glyph you can use other people's regrowth/rejuv
    if ( ! ( td( t ) -> dots.regrowth -> is_ticking() ||
             td( t ) -> dots.rejuvenation -> is_ticking() ) )
      return false;

    return druid_heal_t::ready();
  }
};

// Tranquility ==============================================================

struct tranquility_t : public druid_heal_t
{
  tranquility_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "tranquility", p, p -> find_specialization_spell( "Tranquility" ), options_str )
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
    druid_heal_t( "wild_growth", p, p -> find_class_spell( "Wild Growth" ), options_str )
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

  virtual void execute()
  {
    if( p() -> specialization() == DRUID_BALANCE )
    {
      p() -> balance_tracker();
      if( sim -> log || sim -> debug )
      {
        sim -> out_debug.printf( "Eclipse Position: %f Eclipse Direction: %f Time till next Eclipse Change: %f Time to next lunar %f Time to next Solar %f Time Till Maximum Eclipse: %f",
          p() -> eclipse_amount,
          p() -> eclipse_direction,
          p() -> eclipse_change,
          p() -> time_to_next_lunar,
          p() -> time_to_next_solar,
          p() -> eclipse_max );
      }
    }
    spell_t::execute();
  }

  virtual double action_multiplier() const
  {
    double m = spell_t::action_multiplier();

    if ( p() -> specialization() == DRUID_BALANCE )
    {
      druid_t* p = debug_cast<druid_t*>( player );
      p -> balance_tracker();
      double balance;
      balance = p -> clamped_eclipse_amount;

      double mastery;
      mastery = p -> cache.mastery_value();
      mastery += p -> spec.eclipse -> effectN( 1 ).percent();

      if ( ( dbc::is_school( school, SCHOOL_ARCANE ) || dbc::is_school( school, SCHOOL_NATURE ) ) &&
           p -> buff.celestial_alignment -> up() )
        m *= 1.0 + mastery;
      else if ( dbc::is_school( school, SCHOOL_NATURE ) && balance < 0 )
        m *= 1.0 + mastery / 2 + mastery * std::abs( balance ) / 200;
      else if ( dbc::is_school( school, SCHOOL_ARCANE ) && balance >= 0 )
        m *= 1.0 + mastery / 2 + mastery * balance / 200;
      else if ( dbc::is_school( school, SCHOOL_ARCANE ) || dbc::is_school( school, SCHOOL_NATURE ) )
        m *= 1.0 + mastery / 2 - mastery * std::abs( balance ) / 200;

      if ( sim -> log || sim -> debug )
        sim -> out_debug.printf( "Action modifier %f", m );
    }
    return m;
  }

  virtual double cost() const
  {
    if ( harmful && p() -> buff.heart_of_the_wild -> damage_spells_are_free() )
      return 0;

    return base_t::cost();
  }

  virtual bool ready()
  {
    if ( p() -> specialization() == DRUID_BALANCE )
      p() -> balance_tracker();

    return base_t::ready();
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
  astral_communion_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "astral_communion", player, player -> spec.astral_communion, options_str )
  {
    harmful = proc = hasted_ticks = false;
    may_multistrike = 0;
    channeled = true;

    base_tick_time = timespan_t::from_millis( 100 );
  }

  double composite_haste() const
  {
    return 1.0;
  }

  void execute()
  {
    druid_spell_t::execute(); // Do not move the buff trigger in front of this.
    p() -> buff.astral_communion -> trigger();
  }

  void tick( dot_t* d )
  {
    druid_spell_t::tick( d );
    p() -> balance_tracker();
    if ( sim -> log || sim -> debug )
    {
      sim -> out_debug.printf( "Eclipse Position: %f Eclipse Direction: %f Time till next Eclipse Change: %f Time to next lunar %f Time to next Solar %f Time Till Maximum Eclipse: %f",
        p() -> eclipse_amount,
        p() -> eclipse_direction,
        p() -> eclipse_change,
        p() -> time_to_next_lunar,
        p() -> time_to_next_solar,
        p() -> eclipse_max );
    }
  }

  void last_tick( dot_t* d )
  {
    druid_spell_t::last_tick( d );
    p() -> buff.astral_communion -> expire();
  }
};

// Barkskin =================================================================

struct barkskin_t : public druid_spell_t
{
  barkskin_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "barkskin", player, player -> find_specialization_spell( "Barkskin" ), options_str )
  {
    harmful = false;
    use_off_gcd = true;
  }

  void execute()
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

    base_costs[ RESOURCE_MANA ] *= 1.0 + p() -> glyph.master_shapeshifter -> effectN( 1 ).percent();
  }

  void execute()
  {
    spell_t::execute();

    p() -> buff.bear_form -> start();
  }

  bool ready()
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

  void execute()
  {
    druid_spell_t::execute();

    if ( p() -> buff.bear_form -> check() )
    {
      p() -> buff.berserk -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, ( p() -> spell.berserk_bear -> duration() +
                                                                          p() -> perk.empowered_berserk -> effectN( 1 ).time_value() ) );
      p() -> cooldown.mangle -> reset( false );
    }
    else if ( p() -> buff.cat_form -> check() )
      p() -> buff.berserk -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, ( p() -> spell.berserk_cat -> duration()  +
                                                                        p() -> perk.empowered_berserk -> effectN( 1 ).time_value() ) );
  }
};

// Bristling Fur Spell ======================================================

struct bristling_fur_t : public druid_spell_t
{
  bristling_fur_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "bristling_fur", player, player -> talent.bristling_fur, options_str  )
  {
    harmful = false;
  }

  void execute()
  {
    druid_spell_t::execute();

    p() -> buff.bristling_fur -> trigger();
  }
};

// Cat Form Spell ===========================================================

struct cat_form_t : public druid_spell_t
{
  cat_form_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "cat_form", player, player -> talent.claws_of_shirvallah -> ok() ?
    player -> find_spell( player -> talent.claws_of_shirvallah -> effectN( 1 ).base_value() ) : 
    player -> find_class_spell( "Cat Form" ), 
    options_str )
  {
    harmful = false;
    min_gcd = timespan_t::from_seconds( 1.5 );

    if ( ! player -> cat_melee_attack )
    {
      player -> init_beast_weapon( player -> cat_weapon, 1.0 );
      player -> cat_melee_attack = new cat_attacks::cat_melee_t( player );
    }

    base_costs[ RESOURCE_MANA ] *= 1.0 + p() -> glyph.master_shapeshifter -> effectN( 1 ).percent();
  }

  void execute()
  {
    spell_t::execute();

    p() -> buff.cat_form -> start();
  }

  bool ready()
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
    druid_spell_t( "celestial_alignment", player, player -> spec.celestial_alignment , options_str )
  {
    parse_options( NULL, options_str );
    harmful = false;
    dot_duration = timespan_t::zero();
  }

  void execute()
  {
    druid_spell_t::execute(); // Do not change the order here. 
    p() -> buff.celestial_alignment -> trigger();
  }
};

// Cenarion Ward ============================================================

struct cenarion_ward_t : public druid_spell_t
{
  cenarion_ward_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "cenarion_ward", p, p -> talent.cenarion_ward,  options_str )
  {
    harmful    = false;
  }

  void execute()
  {
    druid_spell_t::execute();

    p() -> buff.cenarion_ward -> trigger();
  }
};

// Dash =====================================================================

struct dash_t : public druid_spell_t
{
  dash_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "dash", player, player -> find_class_spell( "Dash" ) )
  {
    parse_options( NULL, options_str );

    harmful = false;
    use_off_gcd = true;
  }

  void execute()
  {
    druid_spell_t::execute();

    p() -> buff.dash -> trigger();
  }
};

// Faerie Fire Spell ========================================================

struct faerie_fire_t : public druid_spell_t
{
  faerie_fire_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "faerie_fire", player, player -> find_class_spell( "Faerie Fire" ) )
  {
    parse_options( NULL, options_str );
    cooldown -> duration = timespan_t::from_seconds( 6.0 );
  }

  void update_ready( timespan_t )
  {
    timespan_t cd = cooldown -> duration;

    if ( ! ( ( p() -> buff.bear_form -> check() && ! p() -> perk.enhanced_faerie_fire -> ok() && ! p() -> buff.son_of_ursoc -> check() ) || p() -> buff.cat_form -> check() ) )
      cd = timespan_t::zero();

    druid_spell_t::update_ready( cd );
  }

  double action_multiplier() const
  {
    double am = druid_spell_t::action_multiplier();

    if ( p() -> buff.bear_form -> check() )
      am *= 1.0 + p() -> perk.enhanced_faerie_fire -> effectN( 2 ).percent();
    else
      return 0.0;
    
    return am;
  }

  resource_e current_resource() const
  {
    if ( p() -> buff.bear_form -> check() )
      return RESOURCE_RAGE;
    else if ( p() -> buff.cat_form -> check() )
      return RESOURCE_ENERGY;

    return RESOURCE_MANA;
  }
};

// Feral Charge (Bear) ======================================================

struct feral_charge_bear_t : public druid_spell_t
{
  feral_charge_bear_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "feral_charge", p, p -> talent.wild_charge )
  {
    parse_options( NULL, options_str );
    may_miss = may_dodge = may_parry = may_block = may_glance = false;
    base_teleport_distance = data().max_range();
    movement_directionality = MOVEMENT_OMNI;
  }

  bool ready()
  {
    if ( ! p() -> buff.bear_form -> check() )
      return false;

    if ( p() -> current.distance_to_move > base_teleport_distance ||
         p() -> current.distance_to_move < data().min_range() ) // Cannot charge unless target is in range.
      return false;

    return druid_spell_t::ready();
  }
};

// Force of Nature Spell ====================================================

struct force_of_nature_spell_t : public druid_spell_t
{
  force_of_nature_spell_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "force_of_nature", player, player -> talent.force_of_nature )
  {
    parse_options( NULL, options_str );

    harmful = false;
    cooldown -> charges = 3;
    cooldown -> duration = timespan_t::from_seconds( 20.0 );
    use_off_gcd = true;
  }

  void execute()
  {
    druid_spell_t::execute();

    if ( p() -> pet_force_of_nature[ 0 ] )
    {
      for ( int i = 0; i < 3; i++ )
      {
        if ( p() -> pet_force_of_nature[ i ] -> is_sleeping() )
        {
          p() -> pet_force_of_nature[ i ] -> summon( p() -> talent.force_of_nature -> duration() );
          return;
        }
      }

      p() -> sim -> errorf( "Player %s ran out of treants.\n", p() -> name() );
      assert( false ); // Will only get here if there are no available treants
    }
  }
};

// Growl  =======================================================================

struct growl_t: public druid_spell_t
{
  growl_t( druid_t* player, const std::string& options_str ):
    druid_spell_t( "growl", player, player -> find_class_spell( "Growl" ) )
  {
    parse_options( NULL, options_str );
    use_off_gcd = true;
  }

  void update_ready( timespan_t )
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.son_of_ursoc -> check() )
      cd = timespan_t::zero();

    druid_spell_t::update_ready( cd );
  }

  void impact( action_state_t* s )
  {
    if ( s -> target -> is_enemy() )
      target -> taunt( player );

    druid_spell_t::impact( s );
  }

  bool ready()
  {
    if ( ! p() -> buff.bear_form -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Heart of the Wild Spell ==================================================

struct heart_of_the_wild_t : public druid_spell_t
{
  heart_of_the_wild_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "heart_of_the_wild", player, player -> talent.heart_of_the_wild )
  {
    parse_options( NULL, options_str );
    harmful = may_hit = may_crit = false;
  }

  void execute()
  {
    druid_spell_t::execute();
    p() -> buff.heart_of_the_wild -> trigger();
  }
};

// Hurricane ================================================================

struct hurricane_tick_t : public druid_spell_t
{
  hurricane_tick_t( druid_t* player, const spell_data_t* s  ) :
    druid_spell_t( "hurricane", player, s )
  {
    aoe = -1;
  }
};

struct hurricane_t : public druid_spell_t
{
  hurricane_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "hurricane", player, player -> find_spell( 16914 ) )
  {
    parse_options( NULL, options_str );
    channeled = true;

    tick_action = new hurricane_tick_t( player, data().effectN( 3 ).trigger() );
    dynamic_tick_action = true;
  }

  double action_multiplier() const
  {
    double m = druid_spell_t::action_multiplier();

    m *= 1.0 + p() -> buff.heart_of_the_wild -> damage_spell_multiplier();

    return m;
  }

  void execute()
  {
    druid_spell_t::execute();

    p() -> buff.hurricane -> trigger();

  }

  void last_tick( dot_t * d )
  {
    druid_spell_t::last_tick( d );
    p() -> buff.hurricane -> expire();
  }

  void schedule_execute( action_state_t* state = 0 )
  {
    druid_spell_t::schedule_execute( state );

    p() -> buff.cat_form  -> expire();
    p() -> buff.bear_form -> expire();
  }
};

// Incarnation ==============================================================

struct incarnation_moonkin_t : public druid_spell_t
{
  incarnation_moonkin_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "incarnation", player, player -> talent.incarnation_chosen )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  void execute()
  {
    druid_spell_t::execute();

    p() -> buff.chosen_of_elune -> trigger();
  }
};

struct incarnation_cat_t : public druid_spell_t
{
  incarnation_cat_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "incarnation", player, player -> talent.incarnation_king )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  void execute()
  {
    druid_spell_t::execute();

    p() -> buff.king_of_the_jungle -> trigger(); 
  }
};

struct incarnation_resto_t : public druid_spell_t
{
  incarnation_resto_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "incarnation", player, player -> talent.incarnation_tree )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  void execute()
  {
    druid_spell_t::execute();

    p() -> buff.tree_of_life -> trigger();
  }
};

struct incarnation_bear_t : public druid_spell_t
{
  incarnation_bear_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "incarnation", player, player -> talent.incarnation_son )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  void execute()
  {
    druid_spell_t::execute();

    p() -> cooldown.mangle -> reset( false );
    p() -> cooldown.growl  -> reset( false );
    p() -> cooldown.maul   -> reset( false );
    if ( ! p() -> perk.enhanced_faerie_fire -> ok() )
      p() -> cooldown.faerie_fire -> reset( false ); 
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
    harmful     = false;
    background  = ( sim -> overrides.str_agi_int != 0 );
  }

  void execute()
  {
    druid_spell_t::execute();

    if ( sim -> log ) sim -> out_log.printf( "%s performs %s", player -> name(), name() );

    if ( ! sim -> overrides.str_agi_int )
      sim -> auras.str_agi_int -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, player -> dbc.spell( 79060 ) -> duration() );
  }
};

// Sunfire Spell ===========================================================

struct sunfire_t: public druid_spell_t
{
  // Sunfire also applies the Moonfire DoT during Celestial Alignment.
  struct moonfire_CA_t: public druid_spell_t
  {
    moonfire_CA_t(druid_t* player):
      druid_spell_t("moonfire", player, player -> find_spell(8921))
    {
      const spell_data_t* dmg_spell = player -> find_spell(164812);
      dot_duration = dmg_spell -> duration();
      dot_duration *= 1 + player -> spec.astral_showers -> effectN(1).percent();
      dot_duration += player -> sets.set( SET_CASTER, T14, B4 ) -> effectN(1).time_value();
      base_tick_time = dmg_spell -> effectN(2).period();
      spell_power_mod.tick = dmg_spell -> effectN(2).sp_coeff();

      base_td_multiplier *= 1.0 + player -> talent.balance_of_power -> effectN(3).percent();

      // Does no direct damage, costs no mana
      attack_power_mod.direct = 0;
      spell_power_mod.direct = 0;
      range::fill(base_costs, 0);
    }

    void tick(dot_t* d)
    {
      druid_spell_t::tick(d);

      if ( result_is_hit(d -> state -> result) )
        p() -> trigger_shooting_stars(d -> state);
    }
  };

  action_t* moonfire;
  sunfire_t(druid_t* player, const std::string& options_str):
    druid_spell_t("sunfire", player, player -> find_spell(93402))
  {
    parse_options(NULL, options_str);
    dot_behavior = DOT_REFRESH;
    const spell_data_t* dmg_spell = player -> find_spell(164815);
    dot_duration = dmg_spell -> duration();
    dot_duration += player -> sets.set( SET_CASTER, T14, B4 ) -> effectN(1).time_value();
    base_tick_time = dmg_spell -> effectN(2).period();
    spell_power_mod.direct = dmg_spell-> effectN(1).sp_coeff();
    spell_power_mod.tick = dmg_spell-> effectN(2).sp_coeff();

    base_td_multiplier *= 1.0 + player -> talent.balance_of_power -> effectN(3).percent();

    if ( p() -> spec.astral_showers -> ok() )
      aoe = -1;

    if ( player -> specialization() == DRUID_BALANCE )
      moonfire = new moonfire_CA_t(player);
  }

  double action_multiplier() const
  {
    double am = druid_spell_t::action_multiplier();

    if ( p() -> buff.solar_peak -> up() )
      am *= 2;

    return am;
  }

  void execute()
  {
    druid_spell_t::execute();

    p() -> buff.solar_peak -> expire();
  }

  void tick( dot_t* d )
  {
    druid_spell_t::tick( d );

    if ( result_is_hit( d -> state -> result ) )
      p() -> trigger_shooting_stars( d -> state );
  }

  void impact( action_state_t* s )
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
  }

  bool ready()
  {
    bool ready = druid_spell_t::ready();

    if ( p() -> buff.celestial_alignment -> up() )
      return ready;

    if ( p() -> eclipse_amount >= 0 )
      return false;

    return ready;
  }
};

// Moonfire spell ===============================================================

struct moonfire_t : public druid_spell_t
{
  // Moonfire also applies the Sunfire DoT during Celestial Alignment.
  struct sunfire_CA_t : public druid_spell_t
  {
    sunfire_CA_t( druid_t* player ) :
      druid_spell_t( "sunfire", player, player -> find_spell( 93402 ) )
    {
      const spell_data_t* dmg_spell = player -> find_spell( 164815 );
      dot_behavior = DOT_REFRESH;
      dot_duration                  = dmg_spell -> duration();
      dot_duration                 += player -> sets.set( SET_CASTER, T14, B4 ) -> effectN( 1 ).time_value();
      base_tick_time                = dmg_spell -> effectN( 2 ).period();
      spell_power_mod.tick          = dmg_spell -> effectN( 2 ).sp_coeff();

      base_td_multiplier           *= 1.0 + player -> talent.balance_of_power -> effectN( 3 ).percent();

      // Does no direct damage, costs no mana
      attack_power_mod.direct = 0;
      spell_power_mod.direct = 0;
      range::fill( base_costs, 0 );

      if ( p() -> spec.astral_showers -> ok() )
        aoe = -1;
    }

    void tick( dot_t* d )
    {
      druid_spell_t::tick( d );

      if ( result_is_hit( d -> state -> result ) )
        p() -> trigger_shooting_stars( d -> state );
    }
  };

  action_t* sunfire;
  moonfire_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "moonfire", player, player -> find_spell( 8921 ) )
  {
    parse_options( NULL, options_str );
    const spell_data_t* dmg_spell = player -> find_spell( 164812 );

    dot_behavior = DOT_REFRESH;
    dot_duration                  = dmg_spell -> duration(); 
    dot_duration                 *= 1 + player -> spec.astral_showers -> effectN( 1 ).percent();
    dot_duration                 += player -> sets.set( SET_CASTER, T14, B4 ) -> effectN( 1 ).time_value();
    base_tick_time                = dmg_spell -> effectN( 2 ).period();
    spell_power_mod.tick          = dmg_spell -> effectN( 2 ).sp_coeff();
    spell_power_mod.direct        = dmg_spell -> effectN( 1 ).sp_coeff();

    base_td_multiplier           *= 1.0 + player -> talent.balance_of_power -> effectN( 3 ).percent();

    if ( player -> specialization() == DRUID_BALANCE )
      sunfire = new sunfire_CA_t( player );
  }

  void tick( dot_t* d )
  {
    druid_spell_t::tick( d );

    if ( result_is_hit( d -> state -> result ) )
      p() -> trigger_shooting_stars( d -> state );
  }

  double action_multiplier() const
  {
    double am = druid_spell_t::action_multiplier();

    if ( p() -> buff.lunar_peak -> up() )
      am *= 2;

    return am;
  }

  void execute()
  {
    druid_spell_t::execute();

    p() -> buff.lunar_peak -> expire();
  }


  void schedule_execute( action_state_t* state = 0 )
  {
    druid_spell_t::schedule_execute( state );

    p() -> buff.bear_form -> expire();
    p() -> buff.cat_form -> expire();
  }

  void impact( action_state_t* s )
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
  }

  bool ready()
  {
    bool ready = druid_spell_t::ready();

    if ( p() -> buff.celestial_alignment -> up() )
      return ready;
    if ( p() -> eclipse_amount < 0 )
      return false;

    return ready;
  }
};

// Moonfire (Lunar Inspiration) Spell =======================================

struct moonfire_li_t : public druid_spell_t
{
  moonfire_li_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "moonfire", player, player -> find_spell( 155625 ) )
  {
    parse_options( NULL, options_str );
  }

  void impact( action_state_t* s )
  {
    druid_spell_t::impact( s );

    // Grant combo points
    if ( result_is_hit( s -> result ) )
    {
      p() -> resource_gain( RESOURCE_COMBO_POINT, 1, p() -> gain.moonfire );
      if ( p() -> spell.primal_fury -> ok() && s -> result == RESULT_CRIT )
      {
        p() -> proc.primal_fury -> occur();
        p() -> resource_gain( RESOURCE_COMBO_POINT, p() -> find_spell( 16953 ) -> effectN( 1 ).base_value(), p() -> gain.primal_fury );
      }
    }
  }

  bool ready()
  {
    if ( ! p() -> talent.lunar_inspiration -> ok() )
      return false;

    return druid_spell_t::ready();
  }
};

// Moonkin Form Spell =======================================================

struct moonkin_form_t : public druid_spell_t
{
  moonkin_form_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "moonkin_form", player, player -> spec.moonkin_form )
  {
    parse_options( NULL, options_str );

    harmful           = false;

    base_costs[ RESOURCE_MANA ] *= 1.0 + p() -> glyph.master_shapeshifter -> effectN( 1 ).percent();
  }

  void execute()
  {
    spell_t::execute();

    p() -> buff.moonkin_form -> start();
  }

  bool ready()
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
    druid_spell_t( "natures_swiftness", player, player -> spec.natures_swiftness )
  {
    parse_options( NULL, options_str );

    harmful = false;
  }

  void execute()
  {
    druid_spell_t::execute();

    p() -> buff.natures_swiftness -> trigger();
  }

  bool ready()
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

  void execute()
  {
    // Don't call druid_spell_t::execute() because the spell data has some weird stuff in it.
    if ( sim -> log ) sim -> out_log.printf( "%s performs %s", player -> name(), name() );
    update_ready();

    p() -> buff.natures_vigil -> trigger();
  }
};

// Prowl ==================================================================

struct prowl_t : public druid_spell_t
{
  prowl_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "prowl", player, player -> find_class_spell( "Prowl" ) )
  {
    parse_options( NULL, options_str );

    trigger_gcd = timespan_t::zero();
    harmful     = false;
  }

  void execute()
  {
    if ( sim -> log )
      sim -> out_log.printf( "%s performs %s", player -> name(), name() );

    p() -> buff.prowl -> trigger();
  }

  bool ready()
  {
    if ( p() -> buff.prowl -> check() )
      return false;

    if ( p() -> in_combat && ! p() -> buff.king_of_the_jungle -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

// Savage Defense ===========================================================

struct savage_defense_t : public druid_spell_t
{
  savage_defense_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "savage_defense", p, p -> find_specialization_spell( "Savage Defense" ), options_str )
  {
    // Savage Defense has 2 cooldowns, obey the 1.5s one via this:
    p -> cooldown.savage_defense_use -> duration = cooldown -> duration;

    // Hard code charge information since it isn't in the spell data.
    cooldown -> duration = timespan_t::from_seconds( 12.0 );
    cooldown -> charges = 2;

    use_off_gcd = true;
    harmful = false;

    if ( p -> sets.has_set_bonus( SET_TANK, T16, B2 ) )
      p -> active.ursocs_vigor = new ursocs_vigor_t( p );
  }

  void execute()
  {
    druid_spell_t::execute();

    p() -> cooldown.savage_defense_use -> start();

    if ( p() -> buff.savage_defense -> check() )
      p() -> buff.savage_defense -> extend_duration( p(), p() -> buff.savage_defense -> buff_duration );
    else
      p() -> buff.savage_defense -> trigger();

    if ( p() -> sets.has_set_bonus( SET_TANK, T16, B4 ) )
      p() -> active.ursocs_vigor -> trigger_hot();
  }

  double cooldown_reduction() const
  {
    double cdr = druid_spell_t::cooldown_reduction();

    if ( p() -> talent.guardian_of_elune -> ok() )
    {
      double dodge = p() -> cache.dodge() - p() -> buff.savage_defense -> check() * p() -> buff.savage_defense -> default_value;
      cdr /= 1 + dodge;
    }

    return cdr;
  }

  bool ready() 
  {
    if ( p() -> cooldown.savage_defense_use -> down() )
      return false;
    if ( ! p() -> buff.bear_form -> check() )
      return false;
    
    return druid_spell_t::ready();
  }
};

// Skull Bash (Bear) ========================================================

struct skull_bash_t : public druid_spell_t
{
  skull_bash_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "skull_bash", player, player -> find_specialization_spell( "Skull Bash" ) )
  {
    parse_options( NULL, options_str );
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;
    use_off_gcd = true;

    cooldown -> duration += player -> glyph.skull_bash -> effectN( 1 ).time_value();
  }

  void execute()
  {
    druid_spell_t::execute();

    if ( p() -> sets.has_set_bonus( SET_MELEE, PVP, B2 ) )
      p() -> cooldown.tigers_fury -> reset( false );
  }

  bool ready()
  {
    if ( ! target -> debuffs.casting -> check() )
      return false;
    if ( ! ( p() -> buff.bear_form -> check() || p() -> buff.cat_form -> check() ) )
      return false;

    return druid_spell_t::ready();
  }
};

// Stampeding Roar =========================================================

struct stampeding_roar_t : public druid_spell_t
{
  stampeding_roar_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "stampeding_roar", p, p -> find_class_spell( "Stampeding Roar" ) )
  {
    parse_options( NULL, options_str );
    harmful = false;
  }

  void execute()
  {
    druid_spell_t::execute();

    for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
    {
      player_t* p = sim -> player_non_sleeping_list[ i ];
      if( p -> is_enemy() || p -> type == PLAYER_GUARDIAN )
        break;

      p -> buffs.stampeding_roar -> trigger();
    }
  }
};

// Starfire Spell ===========================================================

struct starfire_t : public druid_spell_t
{
  starfire_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "starfire", player, player -> spec.starfire )
  {
    parse_options( NULL, options_str );
  }

  double action_multiplier() const
  {
    double m = druid_spell_t::action_multiplier();

    if ( p() -> buff.lunar_empowerment -> up() )
      m *= 1.0 + p() -> buff.lunar_empowerment -> data().effectN( 1 ).percent() +
                 p() -> talent.soul_of_the_forest -> effectN( 1 ).percent();

    return m;
  }

  timespan_t execute_time() const
  {
    timespan_t casttime = druid_spell_t::execute_time();

    if ( p() -> buff.lunar_empowerment -> up() && p() -> talent.euphoria -> ok() )
      casttime /= 1 + std::abs( p() -> talent.euphoria -> effectN( 2 ).percent() );

    return casttime;
  }

  void execute()
  {
    druid_spell_t::execute();

    if ( p() -> eclipse_amount < 0 && !p() -> buff.celestial_alignment -> up() )
      p() -> proc.wrong_eclipse_starfire -> occur();

    p() -> buff.lunar_empowerment -> decrement();
  }

  void impact( action_state_t* s )
  {
    druid_spell_t::impact( s );

    if ( p() -> talent.balance_of_power && result_is_hit( s -> result ) )
      td( s -> target ) -> dots.moonfire -> extend_duration( timespan_t::from_seconds( p() -> talent.balance_of_power -> effectN( 1 ).base_value() ), timespan_t::zero(), 0 );
  }
};

// Starfall Spell ===========================================================

struct starfall_pulse_t : public druid_spell_t
{
  starfall_pulse_t( druid_t* player, const std::string& name ) :
    druid_spell_t( name, player, player -> find_spell( 50288 ) )
  {
    direct_tick = true;
    aoe = -1;
  }
};

struct starfall_t : public druid_spell_t
{
  spell_t* starfall;
  starfall_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "starfall", player, player -> find_specialization_spell( "Starfall" ) ),
    starfall( new starfall_pulse_t( player, "starfall_pulse" ) )
  {
    parse_options( NULL, options_str );

    hasted_ticks = false;
    may_multistrike = 0;
    tick_zero = true;
    cooldown = player -> cooldown.starfallsurge;
    base_multiplier *= 1.0 + player -> sets.set( SET_CASTER, T14, B2 ) -> effectN( 1 ).percent();
    add_child( starfall );
  }

  void tick( dot_t* )
  {
    starfall -> execute();
  }

  void execute()
  {
    p() -> buff.starfall -> trigger();

    druid_spell_t::execute();
  }

  bool ready()
  {
    if ( p() -> buff.starfall -> up() )
      return false;

    return druid_spell_t::ready();
  }
};

// Starsurge Spell ==========================================================

struct starsurge_t : public druid_spell_t
{
  starsurge_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "starsurge", player, player -> spec.starsurge )
  {
    parse_options( NULL, options_str );

    base_multiplier *= 1.0 + player -> sets.set( SET_CASTER, T13, B4 ) -> effectN( 2 ).percent();

    base_multiplier *= 1.0 + p() -> sets.set( SET_CASTER, T13, B2 ) -> effectN( 1 ).percent();

    base_crit += p() -> sets.set( SET_CASTER, T15, B2 ) -> effectN( 1 ).percent();
    cooldown = player -> cooldown.starfallsurge;
    base_execute_time *= 1.0 + player -> perk.enhanced_starsurge -> effectN( 1 ).percent();
  }

  void execute()
  {
    druid_spell_t::execute();
    if ( p() -> eclipse_amount < 0 )
      p() -> buff.solar_empowerment -> trigger( 3 );
    else
      p() -> buff.lunar_empowerment -> trigger( 2 );
  }
};

// Stellar Flare ==========================================================

struct stellar_flare_t : public druid_spell_t
{
  stellar_flare_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "stellar_flare", player, player -> talent.stellar_flare )
  {
    parse_options( NULL, options_str );
  }

  double action_multiplier() const
  {
    double m = base_t::action_multiplier();

    double balance;
    balance = p() -> clamped_eclipse_amount;
    double mastery;
    mastery = p() -> cache.mastery_value();
    mastery += p() -> spec.eclipse -> effectN(1).percent();

    if ( p() -> buff.celestial_alignment -> up() )
      m *= ( 1.0 + mastery ) * ( 1.0 + mastery );
    else
      m *= ( 1.0 + ( mastery * ( 100 - std::abs(balance) ) / 200 ) ) * ( 1.0 + ( mastery / 2 + mastery * ( std::abs(balance) / 200 ) ) );

    if ( sim -> log || sim -> debug )
      sim -> out_debug.printf("Action modifier %f", m);
    return m;
  }
};

// Survival Instincts =======================================================

struct survival_instincts_t : public druid_spell_t
{
  survival_instincts_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "survival_instincts", player, player -> find_specialization_spell( "Survival Instincts" ), options_str )
  {
    harmful = false;
    use_off_gcd = true;
    cooldown -> duration = timespan_t::from_seconds( 120.0 ); // Spell data has wrong cooldown, as of 6/18/14

    cooldown -> charges = 2;
  }

  void execute()
  {
    druid_spell_t::execute();

    p() -> buff.survival_instincts -> trigger(); // DBC value is 60 for some reason
  }
};

// Tiger's Fury =============================================================

struct tigers_fury_t : public druid_spell_t
{
  tigers_fury_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "tigers_fury", p, p -> find_specialization_spell( "Tiger's Fury" ), options_str )
  {
    harmful = false;
  }

  void execute()
  {
    druid_spell_t::execute();

    p() -> buff.tigers_fury -> trigger();

    p() -> resource_gain( RESOURCE_ENERGY,
                          data().effectN( 2 ).resource( RESOURCE_ENERGY ),
                          p() -> gain.tigers_fury );

    if ( p() -> sets.has_set_bonus( SET_MELEE, T13, B4 ) )
      p() -> buff.omen_of_clarity -> trigger();

    if ( p() -> sets.has_set_bonus( SET_MELEE, T15, B4 ) )
      p() -> buff.tier15_4pc_melee -> trigger( 3 );

    if ( p() -> sets.has_set_bonus( SET_MELEE, T16, B4 ) )
      p() -> buff.feral_rage -> trigger();
  }
};

// T16 Balance 2P Bonus =====================================================

struct t16_2pc_starfall_bolt_t : public druid_spell_t
{
  t16_2pc_starfall_bolt_t( druid_t* player ) :
    druid_spell_t( "t16_2pc_starfall_bolt", player, player -> find_spell( 144770 ) )
  {
    background  = true;
  }
};

struct t16_2pc_sun_bolt_t : public druid_spell_t
{
  t16_2pc_sun_bolt_t( druid_t* player ) :
    druid_spell_t( "t16_2pc_sun_bolt", player, player -> find_spell( 144772 ) )
  {
    background  = true;
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

  void execute()
  {
    druid_spell_t::execute();

    p() -> buff.wild_mushroom -> trigger( !p() -> in_combat ? p() -> buff.wild_mushroom -> max_stack() : 1 );
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

  double action_multiplier() const
  {
    double m = druid_spell_t::action_multiplier();

    m *= 1.0 + p() -> sets.set( SET_CASTER, T13, B2 ) -> effectN( 1 ).percent();

    m *= 1.0 + p() -> buff.heart_of_the_wild -> damage_spell_multiplier();

    if ( p() -> talent.dream_of_cenarius && p() -> specialization() == DRUID_RESTORATION )
      m *= 1.0 + p() -> talent.dream_of_cenarius -> effectN( 1 ).percent();

    if ( p() -> buff.solar_empowerment -> up() )
      m *= 1.0 + p() -> buff.solar_empowerment -> data().effectN( 1 ).percent() +
                 p() -> talent.soul_of_the_forest -> effectN( 1 ).percent();

    return m;
  }

  timespan_t execute_time() const
  {
    timespan_t casttime = druid_spell_t::execute_time();

    if ( p() -> buff.solar_empowerment -> up() && p() -> talent.euphoria -> ok() )
      casttime /= 1 + std::abs( p() -> talent.euphoria -> effectN( 2 ).percent() );

    return casttime;
  }

  void schedule_execute( action_state_t* state = 0 )
  {
    druid_spell_t::schedule_execute( state );

    p() -> buff.cat_form  -> expire();
    p() -> buff.bear_form -> expire();
  }

  void execute()
  {
    druid_spell_t::execute();

    if ( p() -> eclipse_amount > 0 && !p() -> buff.celestial_alignment -> up() )
      p() -> proc.wrong_eclipse_wrath -> occur();

    p() -> buff.solar_empowerment -> decrement();
  }

  void impact( action_state_t* s )
  {
    druid_spell_t::impact( s );

    if ( p() -> talent.balance_of_power && result_is_hit( s -> result ) )
      td( s -> target ) -> dots.sunfire -> extend_duration( timespan_t::from_seconds( p() -> talent.balance_of_power -> effectN( 2 ).base_value() ), timespan_t::zero(), 0 );
  }
};

} // end namespace spells

// ==========================================================================
// Druid Character Definition
// ==========================================================================

void druid_t::trigger_shooting_stars( action_state_t* s )
{
  if ( s -> target != sim -> target )
    return;
  // Shooting stars will only proc on the most recent target of your moonfire/sunfire.
  // For now, assume that the main simulation "Fluffy Pillow" is the most recent target all the time.
  if ( s -> result == RESULT_CRIT )
  {
    if ( rng().roll( spec.shooting_stars -> effectN( 1 ).percent() * 2 ) )
    {
      if ( cooldown.starfallsurge -> current_charge == 3 )
        proc.shooting_stars_wasted -> occur();
      cooldown.starfallsurge -> reset( true );
      proc.shooting_stars -> occur();
    }
  }
  else if ( rng().roll( spec.shooting_stars -> effectN( 1 ).percent() ) )
  {
    if ( cooldown.starfallsurge -> current_charge == 3 )
      proc.shooting_stars_wasted -> occur();
    cooldown.starfallsurge -> reset( true );
    proc.shooting_stars -> occur();
  }
}

void druid_t::trigger_soul_of_the_forest()
{
  if ( ! talent.soul_of_the_forest -> ok() )
    return;
}

// druid_t::create_action  ==================================================

action_t* druid_t::create_action( const std::string& name,
                                  const std::string& options_str )
{
  using namespace cat_attacks;
  using namespace bear_attacks;
  using namespace heals;
  using namespace spells;

  if ( name == "astral_communion" || 
       name == "ac")                      return new       astral_communion_t( this, options_str );
  if ( name == "auto_attack"            ) return new            auto_attack_t( this, options_str );
  if ( name == "barkskin"               ) return new               barkskin_t( this, options_str );
  if ( name == "berserk"                ) return new                berserk_t( this, options_str );
  if ( name == "bear_form"              ) return new              bear_form_t( this, options_str );
  if ( name == "bristling_fur"          ) return new          bristling_fur_t( this, options_str );
  if ( name == "cat_form" ||
       name == "claws_of_shirvallah"    ) return new               cat_form_t( this, options_str );
  if ( name == "celestial_alignment" ||
       name == "ca"                     ) return new    celestial_alignment_t( this, options_str );
  if ( name == "cenarion_ward"          ) return new          cenarion_ward_t( this, options_str );
  if ( name == "dash"                   ) return new                   dash_t( this, options_str );
  if ( name == "faerie_fire"            ) return new            faerie_fire_t( this, options_str );
  if ( name == "ferocious_bite"         ) return new         ferocious_bite_t( this, options_str );
  if ( name == "frenzied_regeneration"  ) return new  frenzied_regeneration_t( this, options_str );
  if ( name == "growl"                  ) return new                  growl_t( this, options_str );
  if ( name == "healing_touch"          ) return new          healing_touch_t( this, options_str );
  if ( name == "hurricane"              ) return new              hurricane_t( this, options_str );
  if ( name == "heart_of_the_wild"  ||
       name == "hotw"                   ) return new      heart_of_the_wild_t( this, options_str );
  if ( name == "lacerate"               ) return new               lacerate_t( this, options_str );
  if ( name == "lifebloom"              ) return new              lifebloom_t( this, options_str );
  if ( name == "maim"                   ) return new                   maim_t( this, options_str );
  if ( name == "mangle"                 ) return new                 mangle_t( this, options_str );
  if ( name == "mark_of_the_wild"       ) return new       mark_of_the_wild_t( this, options_str );
  if ( name == "maul"                   ) return new                   maul_t( this, options_str );
  if ( name == "moonfire"               )
  {
    if ( specialization() == DRUID_FERAL )  return new            moonfire_li_t( this, options_str );
    else                                    return new               moonfire_t( this, options_str );
  }
  if ( name == "sunfire"                ) return new                sunfire_t( this, options_str ); // Moonfire and Sunfire are selected based on how much balance energy the player has.
  if ( name == "moonkin_form"           ) return new           moonkin_form_t( this, options_str );
  if ( name == "natures_swiftness"      ) return new       druids_swiftness_t( this, options_str );
  if ( name == "natures_vigil"          ) return new          natures_vigil_t( this, options_str );
  if ( name == "pulverize"              ) return new              pulverize_t( this, options_str );
  if ( name == "rake"                   ) return new                   rake_t( this, options_str );
  if ( name == "renewal"                ) return new                renewal_t( this, options_str );
  if ( name == "regrowth"               ) return new               regrowth_t( this, options_str );
  if ( name == "rejuvenation"           ) return new           rejuvenation_t( this, options_str );
  if ( name == "rip"                    ) return new                    rip_t( this, options_str );
  if ( name == "savage_roar"            ) return new            savage_roar_t( this, options_str );
  if ( name == "savage_defense"         ) return new         savage_defense_t( this, options_str );
  if ( name == "shred"                  ) return new                  shred_t( this, options_str );
  if ( name == "skull_bash"             ) return new             skull_bash_t( this, options_str );
  if ( name == "stampeding_roar"        ) return new        stampeding_roar_t( this, options_str );
  if ( name == "starfire"               ) return new               starfire_t( this, options_str );
  if ( name == "starfall"               ) return new               starfall_t( this, options_str );
  if ( name == "starsurge"              ) return new              starsurge_t( this, options_str );
  if ( name == "stellar_flare"          ) return new          stellar_flare_t( this, options_str );
  if ( name == "prowl"                  ) return new                  prowl_t( this, options_str );
  if ( name == "survival_instincts"     ) return new     survival_instincts_t( this, options_str );
  if ( name == "swipe"                  ) return new                  swipe_t( this, options_str );
  if ( name == "swiftmend"              ) return new              swiftmend_t( this, options_str );
  if ( name == "tigers_fury"            ) return new            tigers_fury_t( this, options_str );
  if ( name == "thrash_bear"            ) return new            thrash_bear_t( this, options_str );
  if ( name == "thrash_cat"             ) return new             thrash_cat_t( this, options_str );
  if ( name == "force_of_nature"        ) return new  force_of_nature_spell_t( this, options_str );
  if ( name == "tranquility"            ) return new            tranquility_t( this, options_str );
  if ( name == "typhoon"                ) return new                typhoon_t( this, options_str );
  if ( name == "wild_growth"            ) return new            wild_growth_t( this, options_str );
  if ( name == "wild_mushroom"          ) return new          wild_mushroom_t( this, options_str );
  if ( name == "wrath"                  ) return new                  wrath_t( this, options_str );
  if ( name == "incarnation"            )
    switch( specialization() )
  {
    case DRUID_FERAL:
      return new incarnation_cat_t( this, options_str ); break;
    case DRUID_GUARDIAN:
      return new incarnation_bear_t( this, options_str ); break;
    case DRUID_BALANCE:
      return new incarnation_moonkin_t( this, options_str ); break;
    case DRUID_RESTORATION:
      return new incarnation_resto_t( this, options_str ); break;
    default:
      break;
  }

  return player_t::create_action( name, options_str );
}

// druid_t::create_pet ======================================================

pet_t* druid_t::create_pet( const std::string& pet_name,
                            const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  using namespace pets;

  return 0;
}

// druid_t::create_pets =====================================================

void druid_t::create_pets()
{
  if ( specialization() == DRUID_BALANCE )
  {
    for ( int i = 0; i < 3; ++i )
      pet_force_of_nature[ i ] = new pets::force_of_nature_balance_t( sim, this );
  }
  else if ( specialization() == DRUID_FERAL )
  {
    for ( int i = 0; i < 3; ++i )
      pet_force_of_nature[ i ] = new pets::force_of_nature_feral_t( sim, this );
  }
  else if ( specialization() == DRUID_GUARDIAN )
  {
    for ( size_t i = 0; i < sizeof_array( pet_force_of_nature ); i++ )
      pet_force_of_nature[ i ] = new pets::force_of_nature_guardian_t( sim, this );
  }
}

// druid_t::init_spells =====================================================

void druid_t::init_spells()
{
  player_t::init_spells();

  // Specializations
  // Generic / Multiple specs
  spec.critical_strikes        = find_specialization_spell( "Critical Strikes" );
  spec.leader_of_the_pack      = find_specialization_spell( "Leader of the Pack" );
  spec.leather_specialization  = find_specialization_spell( "Leather Specialization" );
  spec.omen_of_clarity         = find_specialization_spell( "Omen of Clarity" );
  spec.killer_instinct         = find_specialization_spell( "Killer Instinct" );
  spec.mana_attunement         = find_specialization_spell( "Mana Attunement" );
  spec.natures_swiftness       = find_specialization_spell( "Nature's Swiftness" );
  spec.nurturing_instinct      = find_specialization_spell( "Nurturing Instinct" );

  // Boomkin
  spec.astral_communion        = find_specialization_spell( "Astral Communion" );
  spec.astral_showers          = find_specialization_spell( "Astral Showers" );
  spec.celestial_alignment     = find_specialization_spell( "Celestial Alignment" );
  spec.celestial_focus         = find_specialization_spell( "Celestial Focus" );
  spec.eclipse                 = find_specialization_spell( "Eclipse" );
  spec.lunar_guidance          = find_specialization_spell( "Lunar Guidance" );
  spec.moonkin_form            = find_specialization_spell( "Moonkin Form" );
  spec.shooting_stars          = find_specialization_spell( "Shooting Stars" );
  spec.starfall                = find_specialization_spell( "Starfall" );
  spec.starfire                = find_specialization_spell( "Starfire" );
  spec.starsurge               = find_specialization_spell( "Starsurge" );
  spec.sunfire                 = find_specialization_spell( "Sunfire" );

  // Feral
  spec.predatory_swiftness     = find_specialization_spell( "Predatory Swiftness" );
  spec.nurturing_instinct      = find_specialization_spell( "Nurturing Instinct" );
  spec.predatory_swiftness     = find_specialization_spell( "Predatory Swiftness" );
  spec.savage_roar             = find_specialization_spell( "Savage Roar" );
  spec.sharpened_claws         = find_specialization_spell( "Sharpened Claws" );
  spec.rip                     = find_specialization_spell( "Rip" );
  spec.tigers_fury             = find_specialization_spell( "Tiger's Fury" );

  // Guardian
  spec.bladed_armor            = find_specialization_spell( "Bladed Armor" );
  spec.resolve                 = find_specialization_spell( "Resolve" );
  spec.savage_defense          = find_specialization_spell( "Savage Defense" );
  spec.survival_of_the_fittest = find_specialization_spell( "Survival of the Fittest" );
  spec.thick_hide              = find_specialization_spell( "Thick Hide" );
  spec.tooth_and_claw          = find_specialization_spell( "Tooth and Claw" );
  spec.ursa_major              = find_specialization_spell( "Ursa Major" );

  // Restoration
  spec.lifebloom               = find_specialization_spell( "Lifebloom" );
  spec.living_seed             = find_specialization_spell( "Living Seed" );
  spec.genesis                 = find_specialization_spell( "Genesis" );
  spec.ironbark                = find_specialization_spell( "Ironbark" );
  spec.malfurions_gift         = find_specialization_spell( "Malfurion's Gift" );
  spec.meditation              = find_specialization_spell( "Meditation" );
  spec.naturalist              = find_specialization_spell( "Naturalist" );
  spec.natural_insight         = find_specialization_spell( "Natural Insight" );
  spec.natures_focus           = find_specialization_spell( "Nature's Focus" );
  spec.regrowth                = find_specialization_spell( "Regrowth" );
  spec.swiftmend               = find_specialization_spell( "Swiftmend" );
  spec.tranquility             = find_specialization_spell( "Tranquility" );
  spec.wild_growth             = find_specialization_spell( "Wild Growth" );

  // Talents
  talent.feline_swiftness    = find_talent_spell( "Feline Swiftness" );
  talent.displacer_beast     = find_talent_spell( "Displacer Beast" );
  talent.wild_charge         = find_talent_spell( "Wild Charge" );

  talent.yseras_gift         = find_talent_spell( "Ysera's Gift" );
  talent.renewal             = find_talent_spell( "Renewal" );
  talent.cenarion_ward       = find_talent_spell( "Cenarion Ward" );

  talent.faerie_swarm        = find_talent_spell( "Faerie Swarm" );
  talent.mass_entanglement   = find_talent_spell( "Mass Entanglement" );
  talent.typhoon             = find_talent_spell( "Typhoon" );

  talent.soul_of_the_forest  = find_talent_spell( "Soul of the Forest" );
  talent.incarnation_chosen  = find_talent_spell( "Incarnation: Chosen of Elune" );
  talent.incarnation_king    = find_talent_spell( "Incarnation: King of the Jungle" );
  talent.incarnation_son     = find_talent_spell( "Incarnation: Son of Ursoc" );
  talent.incarnation_tree    = find_talent_spell( "Incarnation: Tree of Life" );
  talent.force_of_nature     = find_talent_spell( "Force of Nature" );

  talent.incapacitating_roar = find_talent_spell( "Incapacitating Roar" );
  talent.ursols_vortex       = find_talent_spell( "Ursol's Vortex" );
  talent.mighty_bash         = find_talent_spell( "Mighty Bash" );

  talent.heart_of_the_wild   = find_talent_spell( "Heart of the Wild" );
  talent.dream_of_cenarius   = find_talent_spell( "Dream of Cenarius" );
  talent.natures_vigil       = find_talent_spell( "Nature's Vigil" );

  // Balance 100 Talents
  talent.euphoria            = find_talent_spell( "Euphoria" );
  talent.stellar_flare       = find_talent_spell( "Stellar Flare" );
  talent.balance_of_power    = find_talent_spell( "Balance of Power" );

  // Feral 100 Talents
  talent.lunar_inspiration   = find_talent_spell( "Lunar Inspiration" );
  talent.bloodtalons         = find_talent_spell( "Bloodtalons" );
  talent.claws_of_shirvallah = find_talent_spell( "Claws of Shirvallah" );
  
  // Guardian 100 Talents
  talent.guardian_of_elune   = find_talent_spell( "Guardian of Elune" );
  talent.pulverize           = find_talent_spell( "Pulverize" );
  talent.bristling_fur       = find_talent_spell( "Bristling Fur" );

  // Restoration 100 Talents
  talent.moment_of_clarity   = find_talent_spell( "Moment of Clarity" );
  talent.germination         = find_talent_spell( "Germination" );
  talent.rampant_growth      = find_talent_spell( "Rampant Growth" );

  // Masteries
  mastery.total_eclipse    = find_mastery_spell( DRUID_BALANCE );
  mastery.razor_claws      = find_mastery_spell( DRUID_FERAL );
  mastery.harmony          = find_mastery_spell( DRUID_RESTORATION );
  mastery.primal_tenacity  = find_mastery_spell( DRUID_GUARDIAN );

  // Active actions
  if ( spec.leader_of_the_pack -> ok() )
    active.leader_of_the_pack = new leader_of_the_pack_t( this );
  if ( talent.natures_vigil -> ok() )
    active.natures_vigil      = new natures_vigil_proc_t( this );
  if ( talent.cenarion_ward -> ok() )
    active.cenarion_ward_hot  = new cenarion_ward_hot_t( this );
  if ( talent.yseras_gift -> ok() )
    active.yseras_gift        = new yseras_gift_t( this );
  if ( sets.has_set_bonus( DRUID_FERAL, T17, B4 ) )
    active.gushing_wound      = new gushing_wound_t( this );
  if ( mastery.primal_tenacity -> ok() )
    active.primal_tenacity = new primal_tenacity_t( this );

  // Spells
  spell.bear_form                       = find_class_spell( "Bear Form"                   ) -> ok() ? find_spell( 1178   ) : spell_data_t::not_found(); // This is the passive applied on shapeshift!
  spell.berserk_bear                    = find_class_spell( "Berserk"                     ) -> ok() ? find_spell( 50334  ) : spell_data_t::not_found(); // Berserk bear mangler
  spell.berserk_cat                     = find_class_spell( "Berserk"                     ) -> ok() ? find_spell( 106951 ) : spell_data_t::not_found(); // Berserk cat resource cost reducer
  spell.frenzied_regeneration           = find_class_spell( "Frenzied Regeneration"       ) -> ok() ? find_spell( 22842  ) : spell_data_t::not_found();
  spell.moonkin_form                    = find_class_spell( "Moonkin Form"                ) -> ok() ? find_spell( 24905  ) : spell_data_t::not_found(); // This is the passive applied on shapeshift!
  spell.regrowth                        = find_class_spell( "Regrowth"                    ) -> ok() ? find_spell( 93036  ) : spell_data_t::not_found(); // Regrowth refresh

  if ( specialization() == DRUID_FERAL )
    spell.primal_fury = find_spell( 16953 );
  else if ( specialization() == DRUID_GUARDIAN )
    spell.primal_fury = find_spell( 16959 );

  // Perks

  // Feral
  perk.enhanced_berserk        = find_perk_spell( "Enhanced Berserk" );
  perk.enhanced_prowl          = find_perk_spell( "Enhanced Prowl" );
  perk.enhanced_rejuvenation   = find_perk_spell( "Enhanced Rejuvenation" );
  perk.improved_rake           = find_perk_spell( "Improved Rake" );

  // Balance
  perk.enhanced_moonkin_form   = find_perk_spell( "Enhanced Moonkin Form" );
  perk.empowered_moonkin       = find_perk_spell( "Empowered Moonkin" );
  perk.enhanced_starsurge      = find_perk_spell( "Enhanced Starsurge" );

  // Guardian
  perk.enhanced_faerie_fire           = find_perk_spell( "Enhanced Faerie Fire" );
  perk.enhanced_tooth_and_claw        = find_perk_spell( "Enhanced Tooth and Claw" );
  perk.empowered_bear_form            = find_perk_spell( "Empowered Bear Form" );
  perk.empowered_berserk              = find_perk_spell( "Empowered Berserk" );

  // Restoration
  perk.empowered_rejuvenation = find_perk_spell( "Empowered Rejuvenation" );
  perk.enhanced_rebirth       = find_perk_spell( "Enhanced Rebirth" );
  perk.empowered_ironbark     = find_perk_spell( "Empowered Ironbark" );
  perk.enhanced_lifebloom     = find_perk_spell( "Enhanced Lifebloom" );

  // Glyphs
  glyph.astral_communion      = find_glyph_spell( "Glyph of Astral Communion" );
  glyph.blooming              = find_glyph_spell( "Glyph of Blooming" );
  glyph.cat_form              = find_glyph_spell( "Glyph of Cat Form" );
  glyph.celestial_alignment   = find_glyph_spell( "Glyph of Celestial Alignment" );
  glyph.dash                  = find_glyph_spell( "Glyph of Dash" );
  glyph.ferocious_bite        = find_glyph_spell( "Glyph of Ferocious Bite" );
  glyph.healing_touch         = find_glyph_spell( "Glyph of Healing Touch" );
  glyph.lifebloom             = find_glyph_spell( "Glyph of Lifebloom" );
  glyph.maim                  = find_glyph_spell( "Glyph of Maim" );
  glyph.maul                  = find_glyph_spell( "Glyph of Maul" );
  glyph.master_shapeshifter   = find_glyph_spell( "Glyph of the Master Shapeshifter" );
  glyph.moonwarding           = find_glyph_spell( "Glyph of Moonwarding" );
  glyph.ninth_life            = find_glyph_spell( "Glyph of the Ninth Life" );
  glyph.regrowth              = find_glyph_spell( "Glyph of Regrowth" );
  glyph.rejuvenation          = find_glyph_spell( "Glyph of Rejuvenation" );
  glyph.savage_roar           = find_glyph_spell( "Glyph of Savage Roar" );
  glyph.savagery              = find_glyph_spell( "Glyph of Savagery" );
  glyph.skull_bash            = find_glyph_spell( "Glyph of Skull Bash" );
  glyph.shapemender           = find_glyph_spell( "Glyph of the Shapemender" );
  glyph.stampeding_roar       = find_glyph_spell( "Glyph of Stampeding Roar" );
  glyph.survival_instincts    = find_glyph_spell( "Glyph of Survival Instincts" );
  glyph.ursols_defense        = find_glyph_spell( "Glyph of Ursol's Defense" );
  glyph.wild_growth           = find_glyph_spell( "Glyph of Wild Growth" );

  if ( sets.has_set_bonus( SET_CASTER, T16, B2 ) )
  {
    t16_2pc_starfall_bolt = new spells::t16_2pc_starfall_bolt_t( this );
    t16_2pc_sun_bolt = new spells::t16_2pc_sun_bolt_t( this );
  }

  caster_melee_attack = new caster_attacks::druid_melee_t( this );
}

// druid_t::init_base =======================================================

void druid_t::init_base_stats()
{
  player_t::init_base_stats();

  if ( specialization () == DRUID_FERAL || specialization() == DRUID_GUARDIAN )
    base.attack_power_per_agility  = 1.0;
  if ( specialization () == DRUID_BALANCE || specialization() == DRUID_RESTORATION )
    base.spell_power_per_intellect = 1.0;

  // Avoidance diminishing Returns constants/conversions now handled in player_t::init_base_stats().
  // Base miss, dodge, parry, and block are set in player_t::init_base_stats().
  // Just need to add class- or spec-based modifiers here (none for druids at the moment).

  if ( specialization() != DRUID_BALANCE )
  {
    resources.base[RESOURCE_ENERGY] = 100;
    resources.base[RESOURCE_RAGE] = 100;
    resources.base[RESOURCE_COMBO_POINT] = 5;
  }
  else
    resources.base[RESOURCE_ECLIPSE] = 105;

  base_energy_regen_per_second = 10;

  // Max Mana & Mana Regen modifiers
  resources.base_multiplier[ RESOURCE_MANA ] = 1.0 + spec.natural_insight -> effectN( 1 ).percent();
  base.mana_regen_per_second *= 1.0 + spec.natural_insight -> effectN( 1 ).percent();
  base.mana_regen_per_second *= 1.0 + spec.mana_attunement -> effectN( 1 ).percent();

  base_gcd = timespan_t::from_seconds( 1.5 );
}

// druid_t::init_buffs ======================================================

void druid_t::create_buffs()
{
  player_t::create_buffs();

  using namespace buffs;

  // Generic / Multi-spec druid buffs
  buff.bear_form             = new bear_form_t( *this );
  buff.berserk               = new berserk_buff_t( *this );
  buff.cat_form              = new cat_form_t( *this );
  buff.claws_of_shirvallah   = buff_creator_t( this, "claws_of_shirvallah", find_spell( talent.claws_of_shirvallah -> effectN( 1 ).base_value() ) )
                               .default_value( find_spell( talent.claws_of_shirvallah -> effectN( 1 ).base_value() ) -> effectN( 5 ).percent() )
                               .add_invalidate( CACHE_VERSATILITY );
  buff.dash                  = buff_creator_t( this, "dash", find_class_spell( "Dash" ) )
                               .cd( timespan_t::zero() );
  buff.frenzied_regeneration = buff_creator_t( this, "frenzied_regeneration", find_class_spell( "Frenzied Regeneration" ) );
  buff.moonkin_form          = new moonkin_form_t( *this );
  buff.omen_of_clarity       = buff_creator_t( this, "omen_of_clarity", spec.omen_of_clarity -> effectN( 1 ).trigger() )
                               .chance( specialization() == DRUID_RESTORATION ? find_spell( 113043 ) -> proc_chance()
                                                                              : find_spell( 16864 ) -> proc_chance() );
  buff.soul_of_the_forest    = buff_creator_t( this, "soul_of_the_forest", talent.soul_of_the_forest -> ok() ? find_spell( 114108 ) : spell_data_t::not_found() )
                               .default_value( find_spell( 114108 ) -> effectN( 1 ).percent() );
  buff.prowl                 = buff_creator_t( this, "prowl", find_class_spell( "Prowl" ) );
  buff.wild_mushroom         = buff_creator_t( this, "wild_mushroom", find_class_spell( "Wild Mushroom" ) )
                               .max_stack( ( specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION )
                                           ? find_class_spell( "Wild Mushroom" ) -> effectN( 2 ).base_value()
                                           : 1 )
                               .quiet( true );

  // Talent buffs

  buff.cenarion_ward = buff_creator_t( this, "cenarion_ward", find_talent_spell( "Cenarion Ward" ) );

  buff.chosen_of_elune    = buff_creator_t( this, "chosen_of_elune" , talent.incarnation_chosen )
                            .default_value( talent.incarnation_chosen -> effectN( 1 ).percent() )
                            .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  buff.king_of_the_jungle = buff_creator_t( this, "king_of_the_jungle", talent.incarnation_king );

  buff.son_of_ursoc       = buff_creator_t( this, "son_of_ursoc"      , talent.incarnation_son );

  buff.tree_of_life       = buff_creator_t( this, "tree_of_life"      , talent.incarnation_tree )
                            .duration( timespan_t::from_seconds( 30 ) );

  if ( specialization() == DRUID_GUARDIAN )
    buff.dream_of_cenarius = buff_creator_t( this, "dream_of_cenarius", talent.dream_of_cenarius )
                            .chance( talent.dream_of_cenarius -> effectN( 1 ).percent() );
  else
    buff.dream_of_cenarius = buff_creator_t( this, "dream_of_cenarius", talent.dream_of_cenarius );


  buff.natures_vigil      = buff_creator_t( this, "natures_vigil", talent.natures_vigil -> ok() ? find_spell( 124974 ) : spell_data_t::not_found() );
  buff.heart_of_the_wild  = new heart_of_the_wild_buff_t( *this );

  buff.bloodtalons        = buff_creator_t( this, "bloodtalons", talent.bloodtalons -> ok() ? find_spell( 145152 ) : spell_data_t::not_found() )
                            .max_stack( 2 );

  // Balance

  buff.astral_communion          = new astral_communion_t( *this );

  buff.astral_insight            = buff_creator_t( this, "astral_insight", talent.soul_of_the_forest -> ok() ? find_spell( 145138 ) : spell_data_t::not_found() )
                                   .chance( 0.08 );

  buff.astral_showers              = buff_creator_t( this, "astral_showers",   spec.astral_showers );

  buff.celestial_alignment       = new celestial_alignment_t( *this );

  buff.empowered_moonkin         = buff_creator_t( this, "empowered_moonkin", find_spell( 157228 ) )
                                   .chance( perk.empowered_moonkin -> proc_chance() );

  buff.hurricane                 = buff_creator_t( this, "hurricane", find_class_spell( "Hurricane" ) );

  buff.lunar_empowerment         = buff_creator_t( this, "lunar_empowerment", find_spell( 164547 ) );

  buff.lunar_peak                = buff_creator_t( this, "lunar_peak", find_spell( 171743 ) );

  buff.solar_peak                = buff_creator_t( this, "solar_peak", find_spell( 171744 ) );

  buff.shooting_stars            = buff_creator_t( this, "shooting_stars", spec.shooting_stars -> effectN( 1 ).trigger() )
                                   .chance( spec.shooting_stars -> proc_chance() + sets.set( SET_CASTER, T16, B4 ) -> effectN( 1 ).percent() );

  buff.solar_empowerment         = buff_creator_t( this, "solar_empowerment", find_spell( 164545 ) );

  buff.starfall                  = buff_creator_t( this, "starfall", spec.starfall  );

  // Feral
  buff.tigers_fury           = buff_creator_t( this, "tigers_fury", find_specialization_spell( "Tiger's Fury" ) )
                               .default_value( find_specialization_spell( "Tiger's Fury" ) -> effectN( 1 ).percent() )
                               .cd( timespan_t::zero() )
                               .chance( 1.0 )
                               .duration( find_specialization_spell( "Tiger's Fury" ) -> duration() );
  buff.savage_roar           = buff_creator_t( this, "savage_roar", find_specialization_spell( "Savage Roar" ) )
                               .default_value( find_specialization_spell( "Savage Roar" ) -> effectN( 2 ).percent() - glyph.savagery -> effectN( 2 ).percent() )
                               .duration( find_spell( glyph.savagery -> effectN( 1 ).trigger_spell_id() ) -> duration() )
                               .refresh_behavior( BUFF_REFRESH_DURATION ) // Pandemic refresh is done by the action
                               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buff.predatory_swiftness   = buff_creator_t( this, "predatory_swiftness", spec.predatory_swiftness -> ok() ? find_spell( 69369 ) : spell_data_t::not_found() );
  buff.tier15_4pc_melee      = buff_creator_t( this, "tier15_4pc_melee", find_spell( 138358 ) );
  buff.feral_fury            = buff_creator_t( this, "feral_fury", find_spell( 144865 ) ); // tier16_2pc_melee
  buff.feral_rage            = buff_creator_t( this, "feral_rage", find_spell( 146874 ) ); // tier16_4pc_melee

  // Guardian
  buff.barkskin              = buff_creator_t( this, "barkskin", find_specialization_spell( "Barkskin" ) )
                               .cd( timespan_t::zero() )
                               .default_value( find_specialization_spell( "Barkskin" ) -> effectN( 2 ).percent() );
  buff.bladed_armor          = buff_creator_t( this, "bladed_armor", spec.bladed_armor )
                               .add_invalidate( CACHE_ATTACK_POWER );
  buff.bristling_fur         = buff_creator_t( this, "bristling_fur", talent.bristling_fur )
                               .default_value( talent.bristling_fur -> effectN( 1 ).percent() )\
                               .cd( timespan_t::zero() );
  buff.primal_tenacity       = absorb_buff_creator_t( this, "primal_tenacity", find_spell( 155784 ) )
                               .school( SCHOOL_PHYSICAL )
                               .source( get_stats( "primal_tenacity" ) )
                               .gain( get_gain( "primal_tenacity" ) );
  buff.pulverize             = buff_creator_t( this, "pulverize", find_spell( 158792 ) )
                               .default_value( find_spell( 158792 ) -> effectN( 1 ).percent() );
  buff.savage_defense        = buff_creator_t( this, "savage_defense", find_class_spell( "Savage Defense" ) -> ok() ? find_spell( 132402 ) : spell_data_t::not_found() )
                               .add_invalidate( CACHE_DODGE )
                               .duration( find_spell( 132402 ) -> duration() + talent.guardian_of_elune -> effectN( 2 ).time_value() )
                               .default_value( find_spell( 132402 ) -> effectN( 1 ).percent() + talent.guardian_of_elune -> effectN( 1 ).percent() );
  buff.survival_instincts    = buff_creator_t( this, "survival_instincts", find_specialization_spell( "Survival Instincts" ) )
                               .cd( timespan_t::zero() )
                               .default_value( 0.0 - find_specialization_spell( "Survival Instincts" ) -> effectN( 1 ).percent() );
  buff.tier15_2pc_tank       = buff_creator_t( this, "tier15_2pc_tank", find_spell( 138217 ) );
  buff.tier17_4pc_tank       = buff_creator_t( this, "tier17_4pc_tank", find_spell( 177969 ) ) // FIXME: Remove fallback values after spell is in spell data.
                               .chance( find_spell( 177969 ) ? find_spell( 177969 ) -> proc_chance() : 1.0 )
                               .duration( find_spell( 177969 ) ? find_spell( 177969 ) -> duration() : timespan_t::from_seconds( 10.0 ) )
                               .max_stack( find_spell( 177969 ) ? find_spell( 177969 ) -> max_stacks() : 3 )
                               .default_value( find_spell( 177969 ) ? find_spell( 177969 ) -> effectN( 1 ).percent() : 0.10 );
  buff.tooth_and_claw        = buff_creator_t( this, "tooth_and_claw", find_spell( 135286 ) )
                               .max_stack( find_spell( 135286 ) -> _max_stack + perk.enhanced_tooth_and_claw -> effectN( 1 ).base_value() );
  buff.tooth_and_claw_absorb = new tooth_and_claw_absorb_t( this );
  buff.ursa_major            = new ursa_major_t( *this );

  // Restoration
  buff.harmony               = buff_creator_t( this, "harmony", mastery.harmony -> ok() ? find_spell( 100977 ) : spell_data_t::not_found() );
  buff.natures_swiftness     = buff_creator_t( this, "natures_swiftness", find_specialization_spell( "Nature's Swiftness" ) )
                               .cd( timespan_t::zero() ); // Cooldown is handled in the spell
}

// ALL Spec Pre-Combat Action Priority List =================================

void druid_t::apl_precombat()
{
  action_priority_list_t* precombat = get_action_priority_list( "precombat" );

  // Flask or Elixir
  if ( sim -> allow_flasks && level >= 80 )
  {
    std::string flask = "flask,type=";
    std::string elixir1, elixir2;
    elixir1 = elixir2 = "elixir,type=";

    if ( primary_role() == ROLE_TANK ) // Guardian
    {
      if ( level > 90 )
        flask += "greater_draenic_stamina_flask";
      else if ( level > 85 )
      {
        elixir1 += "mad_hozen";
        elixir2 += "mantid";
      }
      else
        flask += "steelskin";
    }
    else if ( primary_role() == ROLE_ATTACK ) // Feral
    {
      if ( level > 90 )
        flask += "greater_draenic_agility_flask";
      else if ( level > 85 )
        flask += "spring_blossoms";
      else
        flask += "winds";
    }
    else // Balance & Restoration
    {
      if ( level > 90 )
        flask += "greater_draenic_intellect_flask";
      else if ( level > 85 )
        flask += "warm_sun";
      else
        flask += "draconic_mind";
    }

    if ( ! util::str_compare_ci( flask, "flask,type=" ) )
      precombat -> add_action( flask );
    else if ( ! util::str_compare_ci( elixir1, "elixir,type=" ) )
    {
      precombat -> add_action( elixir1 );
      precombat -> add_action( elixir2 );
    }
  }

  // Food
  if ( sim -> allow_food && level > 80 )
  {
    std::string food = "food,type=";

    if ( level > 90 )
    {
      if ( specialization() == DRUID_FERAL )
        food += "blackrock_barbecue";
      else if ( specialization() == DRUID_BALANCE )
        food += "sleeper_surprise"; // PH: Attuned stat
      else if ( specialization() == DRUID_GUARDIAN )
        food += "sleeper_surprise";
      else
        food += "frosty_stew"; // PH: Attuned stat
    }
    else if ( level > 85 )
      food += "pandaren_treasure_noodle_soup";
    else
      food += "seafood_magnifique_feast";

    precombat -> add_action( food );
  }

  // Mark of the Wild
  precombat -> add_action( this, "Mark of the Wild", "if=!aura.str_agi_int.up" );

  // Feral: Bloodtalons
  if ( specialization() == DRUID_FERAL && level >= 100 )
    precombat -> add_action( this, "Healing Touch", "if=talent.bloodtalons.enabled" );

  // Forms
  if ( ( specialization() == DRUID_FERAL && primary_role() == ROLE_ATTACK ) || primary_role() == ROLE_ATTACK )
  {
    precombat -> add_action( this, "Cat Form" );
    precombat -> add_action( this, "Prowl" );
  }
  else if ( primary_role() == ROLE_TANK )
  {
    precombat -> add_action( this, "Bear Form" );
  }
  else if ( specialization() == DRUID_BALANCE && ( primary_role() == ROLE_DPS || primary_role() == ROLE_SPELL ) )
  {
    precombat -> add_action( this, "Moonkin Form" );
  }

  // Snapshot stats
  precombat -> add_action( "snapshot_stats", "Snapshot raid buffed stats before combat begins and pre-potting is done." );

  // Pre-Potion
  if ( sim -> allow_potions && level >= 80 )
  {
    std::string potion_action = "potion,name=";
    if ( specialization() == DRUID_FERAL && primary_role() == ROLE_ATTACK )
    {
      if ( level > 90 )
        potion_action += "draenic_agility";
      else if ( level > 85 )
        potion_action += "virmens_bite";
      else
        potion_action += "tolvir";
      precombat -> add_action( potion_action );
    }
    else if ( ( specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION ) && ( primary_role() == ROLE_SPELL || primary_role() == ROLE_HEAL ) )
    {
      if ( level > 90 )
        potion_action += "draenic_intellect";
      else if ( level > 85 )
        potion_action += "jade_serpent";
      else
        potion_action += "volcanic";
      precombat -> add_action( potion_action );
    }
  }
  if ( specialization() == DRUID_BALANCE )
    precombat -> add_talent( this, "Stellar Flare" );
}

// NO Spec Combat Action Priority List ======================================

void druid_t::apl_default()
{
  action_priority_list_t* def = get_action_priority_list( "default" );

  // Assemble Racials / On-Use Items / Professions
  std::string extra_actions = "";

  std::vector<std::string> racial_actions = get_racial_actions();
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    extra_actions += add_action( racial_actions[ i ] );

  std::vector<std::string> item_actions = get_item_actions();
  for ( size_t i = 0; i < item_actions.size(); i++ )
    extra_actions += add_action( item_actions[ i ] );

  std::vector<std::string> profession_actions = get_profession_actions();
  for ( size_t i = 0; i < profession_actions.size(); i++ )
    extra_actions += add_action( profession_actions[ i ] );

  if ( primary_role() == ROLE_SPELL )
  {
    def -> add_action( extra_actions );
    def -> add_action( this, "Moonfire", "if=remains<=duration*0.3" );
    def -> add_action( "Wrath" );
  }
  // Specless (or speced non-main role) druid who has a primary role of a melee
  else if ( primary_role() == ROLE_ATTACK )
  {
    def -> add_action( this, "Faerie Fire", "if=debuff.weakened_armor.stack<3" );
    def -> add_action( extra_actions );
    def -> add_action( this, "Rake", "if=remains<=duration*0.3" );
    def -> add_action( this, "Shred" );
    def -> add_action( this, "Ferocious Bite", "if=combo_points>=5" );
  }
  // Specless (or speced non-main role) druid who has a primary role of a healer
  else if ( primary_role() == ROLE_HEAL )
  {
    def -> add_action( extra_actions );
    def -> add_action( this, "Rejuvenation", "if=remains<=duration*0.3" );
    def -> add_action( this, "Healing Touch", "if=mana.pct>=30" );
  }
}

// Feral Combat Action Priority List =======================================

void druid_t::apl_feral()
{
  action_priority_list_t* def      = get_action_priority_list( "default"  );

  std::vector<std::string> item_actions   = get_item_actions();
  std::vector<std::string> racial_actions = get_racial_actions();
  std::string              potion_action  = "potion,name=";
  if ( level > 90 )
    potion_action += "draenic_agility";
  else if ( level > 85 )
    potion_action += "virmens_bite";
  else
    potion_action += "tolvir";

  // Main List =============================================================

  if ( race == RACE_NIGHT_ELF )
    def -> add_action( this, "Rake", "if=buff.prowl.up|buff.shadowmeld.up" );
  else
    def -> add_action( this, "Rake", "if=buff.prowl.up" );
  def -> add_action( "auto_attack" );
  def -> add_action( this, "Skull Bash" );
  def -> add_action( "incarnation,sync=berserk" );
  def -> add_action( this, "Berserk", "if=cooldown.tigers_fury.remains<8" );
  if ( sim -> allow_potions && level >= 80 )
    def -> add_action( potion_action + ",if=target.time_to_die<=40" );
  // On-Use Items
  for ( size_t i = 0; i < item_actions.size(); i++ )
    def -> add_action( item_actions[ i ] + ",sync=tigers_fury" );
  // Racials
  for ( size_t i = 0; i < racial_actions.size(); i++ )
    def -> add_action( racial_actions[ i ] + ",sync=tigers_fury" );
  def -> add_action( this, "Tiger's Fury", "if=(!buff.omen_of_clarity.react&energy.max-energy>=60)|energy.max-energy>=80" );
  if ( race == RACE_NIGHT_ELF && glyph.savage_roar -> ok() )
    def -> add_action( "shadowmeld,if=buff.savage_roar.remains<3|(buff.bloodtalons.up&buff.savage_roar.remains<6)" );
  def -> add_action( this, "Ferocious Bite", "cycle_targets=1,if=dot.rip.ticking&dot.rip.remains<=3&target.health.pct<25",
                     "Keep Rip from falling off during execute range." );
  def -> add_action( this, "Healing Touch", "if=talent.bloodtalons.enabled&buff.predatory_swiftness.up&(combo_points>=4|buff.predatory_swiftness.remains<1.5)" );
  def -> add_action( this, "Savage Roar", "if=buff.savage_roar.remains<3" );
  if ( sim -> allow_potions && level >= 80 )
    def -> add_action( potion_action + ",sync=berserk,if=target.health.pct<25" );
  def -> add_action( "thrash_cat,if=buff.omen_of_clarity.react&remains<=duration*0.3&active_enemies>1" );

  // Finishers
  def -> add_action( this, "Rip", "cycle_targets=1,if=combo_points=5&dot.rip.ticking&persistent_multiplier>dot.rip.pmultiplier" );
  def -> add_action( this, "Ferocious Bite", "cycle_targets=1,if=combo_points=5&target.health.pct<25&dot.rip.ticking" );
  def -> add_action( this, "Rip", "cycle_targets=1,if=combo_points=5&remains<=duration*0.3" );
  def -> add_action( this, "Savage Roar", "if=combo_points=5&(energy.time_to_max<=1|buff.berserk.up|cooldown.tigers_fury.remains<3)&buff.savage_roar.remains<42*0.3" );
  def -> add_action( this, "Ferocious Bite", "if=combo_points=5&(energy.time_to_max<=1|buff.berserk.up|cooldown.tigers_fury.remains<3)" );

  def -> add_action( this, "Rake", "cycle_targets=1,if=remains<=duration*0.3&active_enemies<9&combo_points<5" );
  def -> add_action( "pool_resource,for_next=1" );
  def -> add_action( "thrash_cat,if=dot.thrash_cat.remains<4.5&active_enemies>1" );
  def -> add_action( this, "Rake", "cycle_targets=1,if=persistent_multiplier>dot.rake.pmultiplier&active_enemies<9&combo_points<5" );
  if ( level >= 100 )
    def -> add_action( "moonfire,cycle_targets=1,if=remains<=duration*0.3&active_enemies=1" );

  // Fillers
  def -> add_action( this, "Swipe", "if=combo_points<5&active_enemies>1" );
  // Disabled until Rake perk + Incarnation is fixed.
  /* def -> add_action( this, "Rake", "if=combo_points<5&hit_damage>=action.shred.hit_damage",
                        "Rake for CP if it hits harder than Shred." ); */
  def -> add_action( this, "Shred", "if=combo_points<5&active_enemies=1" );

  // Add in rejuv blanketing for nature's vigil -- not fully optimized
  def -> add_action( this, "rejuvenation", "cycle_targets=1,max_cycle_targets=3,if=talent.natures_vigil.enabled&!ticking&(buff.natures_vigil.up|cooldown.natures_vigil.remains<15)" );
  def -> add_talent( this, "natures_vigil" );
  def -> add_action( this, "rejuvenation", "cycle_targets=1,if=talent.natures_vigil.enabled&!ticking&(buff.natures_vigil.up|cooldown.natures_vigil.remains<15)" );  
  def -> add_action( this, "rejuvenation", "cycle_targets=1,if=talent.natures_vigil.enabled&buff.natures_vigil.up" );  

}

// Balance Combat Action Priority List ==============================

void druid_t::apl_balance()
{
  std::vector<std::string> racial_actions = get_racial_actions();
  std::vector<std::string> item_actions   = get_item_actions();
  std::string              potion_action  = "potion,name=";
  if ( level > 90 )
    potion_action += "draenic_intellect";
  else if ( level > 85 )
    potion_action += "jade_serpent";
  else
    potion_action += "volcanic";

  action_priority_list_t* default_list        = get_action_priority_list( "default" );
  action_priority_list_t* single_target       = get_action_priority_list( "single_target" );
  action_priority_list_t* aoe                 = get_action_priority_list( "aoe" );

  if ( sim -> allow_potions && level >= 80 )
    default_list -> add_action( potion_action + ",if=buff.celestial_alignment.up" );

  for ( size_t i = 0; i < racial_actions.size(); i++ )
    default_list -> add_action( racial_actions[i] + ",if=buff.celestial_alignment.up" );
  for ( size_t i = 0; i < item_actions.size(); i++ )
    default_list -> add_action( item_actions[i] );

  default_list -> add_talent( this, "Force of Nature", "if=trinket.stat.intellect.up|charges=3|target.time_to_die<21" );
  default_list -> add_action( "run_action_list,name=single_target,if=active_enemies=1" );
  default_list -> add_action( "run_action_list,name=aoe,if=active_enemies>1" );

  single_target -> add_action( this, "Starsurge", "if=buff.lunar_empowerment.down&eclipse_energy>20" );
  single_target -> add_action( this, "Starsurge", "if=buff.solar_empowerment.down&eclipse_energy<-20" );
  single_target -> add_action( this, "Starsurge", "if=(charges=2&recharge_time<15)|charges=3" );
  single_target -> add_action( this, "Celestial Alignment", "if=lunar_max<8|target.time_to_die<20" );
  single_target -> add_action( "incarnation,if=buff.celestial_alignment.up" );
  single_target -> add_action( this, "Sunfire", "if=remains<7|buff.solar_peak.up" );
  single_target -> add_talent( this, "Stellar Flare", "if=remains<7" );
  single_target -> add_action( this, "Moonfire" , "if=buff.lunar_peak.up&remains<eclipse_change+20|remains<4|(buff.celestial_alignment.up&buff.celestial_alignment.remains<=2&remains<eclipse_change+20)" );
  single_target -> add_action( this, "Wrath", "if=(eclipse_energy<=0&eclipse_change>cast_time)|(eclipse_energy>0&cast_time>eclipse_change)" );
  single_target -> add_action( this, "Starfire", "if=(eclipse_energy>=0&eclipse_change>cast_time)|(eclipse_energy<0&cast_time>eclipse_change)" );

  aoe -> add_action( this, "Celestial Alignment", "if=lunar_max<8|target.time_to_die<20" );
  aoe -> add_action( "incarnation,if=buff.celestial_alignment.up" );
  aoe -> add_action( this, "Sunfire", "if=remains<8" );
  aoe -> add_action( this, "Starfall" );
  aoe -> add_action( this, "Moonfire", "cycle_targets=1,if=remains<12" );
  aoe -> add_talent( this, "Stellar Flare", "cycle_targets=1,if=remains<7" );
  aoe -> add_action( this, "Wrath", "if=(eclipse_energy<=0&eclipse_change>cast_time)|(eclipse_energy>0&cast_time>eclipse_change)" );
  aoe -> add_action( this, "Starfire", "if=(eclipse_energy>=0&eclipse_change>cast_time)|(eclipse_energy<0&cast_time>eclipse_change)" );
}

// Guardian Combat Action Priority List ==============================

void druid_t::apl_guardian()
{
  action_priority_list_t* default_list    = get_action_priority_list( "default" );

  std::vector<std::string> item_actions       = get_item_actions();
  std::vector<std::string> racial_actions     = get_racial_actions();

  for ( size_t i = 0; i < racial_actions.size(); i++ )
    default_list -> add_action( racial_actions[i] );
  for ( size_t i = 0; i < item_actions.size(); i++ )
    default_list -> add_action( item_actions[i] );

  default_list -> add_action( "auto_attack" );
  default_list -> add_action( this, "Skull Bash" );
  default_list -> add_action( this, "Maul", "if=buff.tooth_and_claw.react&buff.tooth_and_claw_absorb.down&incoming_damage_1s" );
  default_list -> add_action( this, "Frenzied Regeneration", "if=incoming_damage_3>(1+action.savage_defense.charges_fractional)*0.04*health.max",
                              "Cast Frenzied Regeneration based on incoming damage, being more strict the closer we are to wasting SD charges." );
  default_list -> add_action( this, "Savage Defense" );
  default_list -> add_action( this, "Barkskin" );
  default_list -> add_talent( this, "Bristling Fur", "if=incoming_damage_4s>health.max*0.15" );
  default_list -> add_talent( this, "Renewal", "if=incoming_damage_5>0.8*health.max" );
  default_list -> add_talent( this, "Nature's Vigil", "if=!talent.incarnation.enabled|buff.son_of_ursoc.up|cooldown.incarnation.remains" );
  default_list -> add_action( this, "Lacerate", "if=((remains<=duration*0.3)|(dot.lacerate.stack<3&dot.thrash_bear.remains>dot.thrash_bear.duration*0.3))&(buff.son_of_ursoc.up|buff.berserk.up)" );
  default_list -> add_action( "thrash_bear,if=remains<=duration*0.3&(buff.son_of_ursoc.up|buff.berserk.up)" );
  default_list -> add_talent( this, "Pulverize", "if=buff.pulverize.remains<gcd" );
  default_list -> add_action( this, "Mangle" );
  default_list -> add_action( "incarnation" );
  default_list -> add_action( this, "Lacerate", "cycle_targets=1,if=remains<=duration*0.3|dot.lacerate.stack<3" );
  default_list -> add_action( "thrash_bear,if=remains<=duration*0.3" );
  default_list -> add_talent( this, "Cenarion Ward" );
  default_list -> add_action( "thrash_bear,if=active_enemies>=3" );
  default_list -> add_action( this, "Lacerate" );
}

// Restoration Combat Action Priority List ==============================

void druid_t::apl_restoration()
{
  action_priority_list_t* default_list    = get_action_priority_list( "default" );

  std::vector<std::string> item_actions       = get_item_actions();
  std::vector<std::string> racial_actions     = get_racial_actions();

  for ( size_t i = 0; i < racial_actions.size(); i++ )
    default_list -> add_action( racial_actions[i] );
  for ( size_t i = 0; i < item_actions.size(); i++ )
    default_list -> add_action( item_actions[i] );

  default_list -> add_action( this, "Natures Swiftness" );
  default_list -> add_talent( this, "Incarnation" );
  default_list -> add_action( this, "Healing Touch", "if=buff.natures_swiftness.up|buff.omen_of_clarity.up" );
  default_list -> add_action( this, "Rejuvenation", "if=remains<=duration*0.3" );
  default_list -> add_action( this, "Lifebloom", "if=debuff.lifebloom.stack<3" );
  default_list -> add_action( this, "Swiftmend" );
  default_list -> add_action( this, "Healing Touch" );
}

// druid_t::init_scaling ====================================================

void druid_t::init_scaling()
{
  player_t::init_scaling();

  equipped_weapon_dps = main_hand_weapon.damage / main_hand_weapon.swing_time.total_seconds();

  if ( specialization() == DRUID_GUARDIAN )
    scales_with[ STAT_WEAPON_DPS ] = false;

  scales_with[STAT_STRENGTH] = false;

  // Save a copy of the weapon
  caster_form_weapon = main_hand_weapon;
}


// druid_t::arise ============================================================

void druid_t::arise()
{
  player_t::arise();

  if ( glyph.savagery -> ok() )
    buff.savage_roar -> trigger();

}
// druid_t::init_gains ======================================================

void druid_t::init_gains()
{
  player_t::init_gains();

  gain.bear_form             = get_gain( "bear_form"             );
  gain.energy_refund         = get_gain( "energy_refund"         );
  gain.feral_rage            = get_gain( "feral_rage"            );
  gain.frenzied_regeneration = get_gain( "frenzied_regeneration" );
  gain.glyph_ferocious_bite  = get_gain( "glyph_ferocious_bite"  );
  gain.leader_of_the_pack    = get_gain( "leader_of_the_pack"    );
  gain.moonfire              = get_gain( "moonfire"              );
  gain.omen_of_clarity       = get_gain( "omen_of_clarity"       );
  gain.primal_fury           = get_gain( "primal_fury"           );
  gain.rake                  = get_gain( "rake"                  );
  gain.shred                 = get_gain( "shred"                 );
  gain.soul_of_the_forest    = get_gain( "soul_of_the_forest"    );
  gain.swipe                 = get_gain( "swipe"                 );
  gain.tier15_2pc_melee      = get_gain( "tier15_2pc_melee"      );
  gain.tier17_2pc_melee      = get_gain( "tier17_2pc_melee"      );
  gain.tier17_2pc_tank       = get_gain( "tier17_2pc_tank"       );
  gain.tigers_fury           = get_gain( "tigers_fury"           );
}

// druid_t::init_procs ======================================================

void druid_t::init_procs()
{
  player_t::init_procs();

  proc.primal_fury              = get_proc( "primal_fury"            );
  proc.shooting_stars_wasted    = get_proc( "Shooting Stars overflow (buff already up)" );
  proc.shooting_stars           = get_proc( "Shooting Stars"         );
  proc.tier15_2pc_melee         = get_proc( "tier15_2pc_melee"       );
  proc.tier17_2pc_melee         = get_proc( "tier17_2pc_melee"       );
  proc.tooth_and_claw           = get_proc( "tooth_and_claw"         );
  proc.tooth_and_claw_wasted    = get_proc( "tooth_and_claw_wasted"  );
  proc.ursa_major               = get_proc( "ursa_major"             );
  proc.wrong_eclipse_wrath      = get_proc( "wrong_eclipse_wrath"    );
  proc.wrong_eclipse_starfire   = get_proc( "wrong_eclipse_starfire" );
}

// druid_t::init_benefits ===================================================

void druid_t::init_benefits()
{
  player_t::init_benefits();
}

// druid_t::init_actions ====================================================

void druid_t::init_action_list()
{
  if ( ! action_list_str.empty() )
  {
    player_t::init_action_list();
    return;
  }
  clear_action_priority_lists();

  apl_precombat(); // PRE-COMBAT

  switch ( specialization() )
  {
    case DRUID_FERAL:
      apl_feral(); // FERAL
      break;
    case DRUID_BALANCE:
      apl_balance();  // BALANCE
      break;
    case DRUID_GUARDIAN:
      apl_guardian(); // GUARDIAN
      break;
    case DRUID_RESTORATION:
      apl_restoration(); // RESTORATION
      break;
    default:
      apl_default(); // DEFAULT
      break;
  }

  use_default_action_list = true;

  player_t::init_action_list();
}

// druid_t::reset ===========================================================

void druid_t::reset()
{
  player_t::reset();

  inflight_starsurge = false;
  double_dmg_triggered = false;
  eclipse_amount = 0;
  eclipse_direction = 1;
  eclipse_change = talent.euphoria -> ok() ? 10 : 20;
  eclipse_max = talent.euphoria -> ok() ? 5 : 10;
  time_to_next_lunar = eclipse_max;
  time_to_next_solar = eclipse_change + eclipse_max;
  clamped_eclipse_amount = 0;
  last_check = timespan_t::zero();
  balance_time = timespan_t::zero();

  base_gcd = timespan_t::from_seconds( 1.5 );

  // Restore main hand attack / weapon to normal state
  main_hand_attack = caster_melee_attack;
  main_hand_weapon = caster_form_weapon;

  for ( size_t i = 0; i < sim -> actor_list.size(); i++ )
  {
    druid_td_t* td = target_data[ sim -> actor_list[ i ] ];
    if ( td ) td -> reset();
  }
}

// druid_t::regen ===========================================================

void druid_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  // player_t::regen() only regens your primary resource, so we need to account for that here
  if ( primary_resource() != RESOURCE_MANA && mana_regen_per_second() )
    resource_gain( RESOURCE_MANA, mana_regen_per_second() * periodicity.total_seconds(), gains.mp5_regen );
  if ( primary_resource() != RESOURCE_ENERGY && energy_regen_per_second() )
    resource_gain( RESOURCE_ENERGY, energy_regen_per_second() * periodicity.total_seconds(), gains.energy_regen );

}

// druid_t::mana_regen_per_second ============================================================
double druid_t::mana_regen_per_second() const
{
  double mp5 = player_t::mana_regen_per_second();

  if ( buff.moonkin_form -> check() ) //Boomkins get 150% increased mana regeneration, scaling with haste.
    mp5 *= 1.0 + buff.moonkin_form -> data().effectN( 5 ).percent() + ( 1 / cache.spell_haste() );

  mp5 *= 1.0 + spec.mana_attunement -> effectN( 1 ).percent();

  return mp5;
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

  // Start the fight with 0 rage and 0 combo points
  resources.current[ RESOURCE_RAGE ] = 0;
  resources.current[ RESOURCE_COMBO_POINT ] = 0;
  resources.current[ RESOURCE_ECLIPSE ] = 0;

  // If Ysera's Gift is talented, apply it upon entering combat
  if ( talent.yseras_gift -> ok() )
    active.yseras_gift -> execute();
  
  // Apply Bladed Armor buff 
  if ( spec.bladed_armor -> ok() )
    buff.bladed_armor -> trigger();
}

// druid_t::invalidate_cache ================================================

void druid_t::invalidate_cache( cache_e c )
{
  player_t::invalidate_cache( c );

  switch ( c )
  {
    case CACHE_AGILITY:
      if ( spec.nurturing_instinct -> ok() )
        player_t::invalidate_cache( CACHE_SPELL_POWER );
      break;
    case CACHE_INTELLECT:
      if ( spec.killer_instinct -> ok() )
        player_t::invalidate_cache( CACHE_AGILITY );
      break;
    case CACHE_MASTERY:
      if ( mastery.primal_tenacity -> ok() )
        player_t::invalidate_cache( CACHE_ATTACK_POWER );
      if ( mastery.total_eclipse -> ok() )
        player_t::invalidate_cache( CACHE_PLAYER_DAMAGE_MULTIPLIER );
      break;
    default: break;
  }
}

// druid_t::composite_attack_power_multiplier ===============================

double druid_t::composite_attack_power_multiplier() const
{
  double ap = player_t::composite_attack_power_multiplier();

  if ( mastery.primal_tenacity -> ok() )
    ap *= 1.0 + cache.mastery_value() * ( find_spell( 159195 ) -> effectN( 1 ).sp_coeff() / mastery.primal_tenacity -> effectN( 1 ).sp_coeff() );

  return ap;
}

// druid_t::composite_armor_multiplier ======================================

double druid_t::composite_armor_multiplier() const
{
  double a = player_t::composite_armor_multiplier();

  if ( buff.bear_form -> check() )
    a *= 1.0 + buff.bear_form -> data().effectN( 3 ).percent() + glyph.ursols_defense -> effectN( 1 ).percent();

  if ( buff.moonkin_form -> check() )
    a *= 1.0 + buff.moonkin_form -> data().effectN( 3 ).percent() + perk.enhanced_moonkin_form -> effectN( 1 ).percent();

  return a;
}

// druid_t::composite_player_multiplier =====================================

double druid_t::composite_player_multiplier( school_e school ) const
{
  double m = player_t::composite_player_multiplier( school );

  if ( specialization() == DRUID_BALANCE )
  {
    if ( buff.celestial_alignment -> check() )
      m *= 1.0 + buff.celestial_alignment -> data().effectN(2).percent();
    if ( dbc::is_school( school, SCHOOL_ARCANE ) || dbc::is_school( school, SCHOOL_NATURE ) )
    {
      if ( buff.moonkin_form -> check() )
        m *= 1.0 + spell.moonkin_form -> effectN( 2 ).percent();
      if ( buff.chosen_of_elune -> check() )
        m *= 1.0 + buff.chosen_of_elune -> default_value;
    }
  }

  if ( ! buff.bear_form -> check() && dbc::is_school( school, SCHOOL_PHYSICAL ) )
    m *= 1.0 + buff.savage_roar -> check() * buff.savage_roar -> default_value; // Avoid using value() to prevent skewing benefit_pct.

  return m;
}

// druid_t::composite_player_heal_multiplier ================================

double druid_t::composite_player_heal_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_heal_multiplier( s );

  m *= 1.0 + buff.heart_of_the_wild -> heal_multiplier();

  // Resolve applies a blanket -60% healing for tanks
  if ( spec.resolve -> ok() )
    m *= 1.0 + spec.resolve -> effectN( 2 ).percent();

  return m;
}

// druid_t::composite_player_absorb_multiplier ================================

double druid_t::composite_player_absorb_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_absorb_multiplier( s );
  
  // Resolve applies a blanket -60% healing for tanks
  if ( spec.resolve -> ok() )
    m *= 1.0 + spec.resolve -> effectN( 3 ).percent();

  return m;
}

// druid_t::composite_melee_expertise( weapon_t* ) ==========================

double druid_t::composite_melee_expertise( weapon_t* ) const
{
  double exp = player_t::composite_melee_expertise();

  exp += spec.thick_hide -> effectN( 3 ).percent();

  return exp;
}

// druid_t::composite_melee_attack_power ==================================

double druid_t::composite_melee_attack_power() const
{
  double ap = player_t::composite_melee_attack_power();

  ap += buff.bladed_armor -> data().effectN( 1 ).percent() * current.stats.get_stat( STAT_BONUS_ARMOR );

  return ap;
}

// druid_t::composite_melee_crit ============================================

double druid_t::composite_melee_crit() const
{
  double crit = player_t::composite_melee_crit();

  crit += spec.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// druid_t::temporary_movement_modifier =========================================

double druid_t::temporary_movement_modifier() const
{
  double active = player_t::temporary_movement_modifier();

   if( buff.dash -> up() )
     active = std::max( active, buff.dash -> data().effectN( 1 ).percent() );

  return active;
}

// druid_t::passive_movement_modifier ========================================

double druid_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  if ( buff.cat_form -> up() )
  {
    ms += find_spell( 113636 ) -> effectN( 1 ).percent();
    if ( buff.prowl -> up() && ! perk.enhanced_prowl -> ok() )
      ms += buff.prowl -> data().effectN( 2 ).percent();
  }

  if ( talent.feline_swiftness -> ok() )
    ms += talent.feline_swiftness -> effectN( 1 ).percent();

  return ms;
}

// druid_t::composite_spell_crit ============================================

double druid_t::composite_spell_crit() const
{
  double crit = player_t::composite_spell_crit();

  crit += spec.critical_strikes -> effectN( 1 ).percent();

  return crit;
}

// druid_t::composite_spell_power ===========================================

double druid_t::composite_spell_power( school_e school ) const
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

// druid_t::composite_attribute =============================================

double druid_t::composite_attribute( attribute_e attr ) const
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

double druid_t::composite_attribute_multiplier( attribute_e attr ) const
{
  double m = player_t::composite_attribute_multiplier( attr );

  switch ( attr )
  {
    case ATTR_STAMINA:
      if( buff.bear_form -> check() )
        m *= 1.0 + spell.bear_form -> effectN( 2 ).percent() + perk.empowered_bear_form -> effectN( 1 ).percent();
      break;
    default:
      break;
  }

  return m;
}

// druid_t::matching_gear_multiplier ========================================

double druid_t::matching_gear_multiplier( attribute_e attr ) const
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

// druid_t::composite_damage_versatility =========================================

double druid_t::composite_damage_versatility() const
{
  double dv = player_t::composite_damage_versatility();

  if ( buff.claws_of_shirvallah -> check() )
    dv += buff.claws_of_shirvallah -> default_value;

  return dv;
}

// druid_t::composite_heal_versatility =========================================

double druid_t::composite_heal_versatility() const
{
  double hv = player_t::composite_heal_versatility();

  if ( buff.claws_of_shirvallah -> check() )
    hv += buff.claws_of_shirvallah -> default_value;

  return hv;
}

// druid_t::composite_mitigation_versatility =========================================

double druid_t::composite_mitigation_versatility() const
{
  double mv = player_t::composite_mitigation_versatility();

  if ( buff.claws_of_shirvallah -> check() )
    mv += buff.claws_of_shirvallah -> default_value / 2.0;

  return mv;
}

// druid_t::composite_crit_avoidance =============================================

double druid_t::composite_crit_avoidance() const
{
  double c = player_t::composite_crit_avoidance();

  c += spec.thick_hide -> effectN( 2 ).percent();

  return c;
}

// druid_t::composite_composite_dodge ============================================

double druid_t::composite_dodge() const
{
  double d = player_t::composite_dodge();

  if ( buff.savage_defense -> check() )
    d += buff.savage_defense -> default_value;

  return d;
}

// druid_t::composite_rating_multiplier =====================================

double druid_t::composite_rating_multiplier( rating_e rating ) const
{
  double m = player_t::composite_rating_multiplier( rating );

  switch ( rating )
  {
  case RATING_SPELL_HASTE:
    return m *= 1.0 + spec.naturalist -> effectN( 1 ).percent();
    break;
  case RATING_MELEE_HASTE:
    return m *= 1.0 + spec.naturalist -> effectN( 1 ).percent();
    break;
  case RATING_RANGED_HASTE:
    return m *= 1.0 + spec.naturalist -> effectN( 1 ).percent();
    break;
  case RATING_SPELL_CRIT:
    return m *= 1.0 + spec.sharpened_claws -> effectN( 1 ).percent();
    break;
  case RATING_MELEE_CRIT:
    return m *= 1.0 + spec.sharpened_claws -> effectN( 1 ).percent();
    break;
  case RATING_RANGED_CRIT:
    return m *= 1.0 + spec.sharpened_claws -> effectN( 1 ).percent();
    break;
  case RATING_MASTERY:
    return m *= 1.0 + spec.lunar_guidance -> effectN( 1 ).percent() + spec.survival_of_the_fittest -> effectN( 1 ).percent();
    break;
  default:
    break;
  }

  return m;
}

// druid_t::create_expression ===============================================

expr_t* druid_t::create_expression( action_t* a, const std::string& name_str )
{
  struct druid_expr_t : public expr_t
  {
    druid_t& druid;
    druid_expr_t( const std::string& n, druid_t& p ) :
      expr_t( n ), druid( p )
    {
    }
  };

  struct eclipse_expr_t : public druid_expr_t
  {
    int rt;
    eclipse_expr_t( const std::string& n, druid_t& p, int r ) :
      druid_expr_t( n, p ), rt( r )
    {
    }
    virtual double evaluate() { return druid.eclipse_direction == rt; }
  };

  std::vector<std::string> splits = util::string_split( name_str, "." );

  if ( ( splits.size() == 2 ) && ( splits[0] == "eclipse_dir" ) )
  {
    int e = 0;
    if ( splits[1] == "lunar" ) e = 1;
    else if ( splits[1] == "solar" ) e = -1;
    else
    {
      std::string error_str = "Invalid eclipse_direction value '" + splits[1] + "', valid values are 'lunar' or 'solar'";
      error_str.c_str();
      return player_t::create_expression( a, name_str );
    }
    return new eclipse_expr_t( name_str, *this, e );
  }
  else if ( util::str_compare_ci( name_str, "eclipse_energy" ) )
  {
    return make_ref_expr( name_str, clamped_eclipse_amount );
  }
  else if ( util::str_compare_ci( name_str, "eclipse_change" ) )
  {
    return make_ref_expr( "eclipse_change", eclipse_change );
  }
  else if ( util::str_compare_ci( name_str, "lunar_max" ) )
  {
    return make_ref_expr( "lunar_max", time_to_next_lunar );
  }
  else if ( util::str_compare_ci( name_str, "solar_max" ) )
  {
    return make_ref_expr( "solar_max", time_to_next_solar );
  }
  else if ( util::str_compare_ci( name_str, "eclipse_max" ) )
  {
    return make_ref_expr( "eclipse_max", eclipse_max );
  }
  else if ( util::str_compare_ci( name_str, "combo_points" ) )
  {
    return make_ref_expr( "combo_points", resources.current[ RESOURCE_COMBO_POINT ] );
  }
  return player_t::create_expression( a, name_str );
}

// druid_t::create_options ==================================================

void druid_t::create_options()
{
  player_t::create_options();

  option_t druid_options[] =
  {
    opt_null()
  };

  option_t::copy( options, druid_options );
}

// druid_t::create_profile ==================================================

bool druid_t::create_profile( std::string& profile_str, save_e type, bool save_html )
{
  player_t::create_profile( profile_str, type, save_html );

  return true;
}

// druid_t::recalculate_resource_max ========================================

void druid_t::recalculate_resource_max( resource_e r )
{
  player_t::recalculate_resource_max( r );
  
  // Update Ursa Major's value for the new health amount.
  if ( r == RESOURCE_HEALTH && buff.ursa_major -> check() )
    buff.ursa_major -> refresh( 1, buff.ursa_major -> value(), buff.ursa_major -> remains() );
}

// druid_t::primary_role ====================================================

role_e druid_t::primary_role() const
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

// druid_t::convert_hybrid_stat ==============================================

stat_e druid_t::convert_hybrid_stat( stat_e s ) const
{
  // this converts hybrid stats that either morph based on spec or only work
  // for certain specs into the appropriate "basic" stats
  switch ( s )
  {
  case STAT_STR_AGI_INT:
    switch ( specialization() )
    {
      case DRUID_BALANCE:
      case DRUID_RESTORATION:
        return STAT_INTELLECT;
      case DRUID_FERAL:
      case DRUID_GUARDIAN:
        return STAT_AGILITY;
      default:
        return STAT_NONE;
    }
  case STAT_AGI_INT: 
    if ( specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION )
      return STAT_INTELLECT;
    else
      return STAT_AGILITY; 
  // This is a guess at how AGI/STR gear will work for Balance/Resto, TODO: confirm  
  case STAT_STR_AGI:
    return STAT_AGILITY;
  // This is a guess at how STR/INT gear will work for Feral/Guardian, TODO: confirm  
  // This should probably never come up since druids can't equip plate, but....
  case STAT_STR_INT:
    return STAT_INTELLECT;
  case STAT_SPIRIT:
    if ( specialization() == DRUID_RESTORATION )
      return s;
    else
      return STAT_NONE;
  case STAT_BONUS_ARMOR:
    if ( specialization() == DRUID_GUARDIAN )
      return s;
    else
      return STAT_NONE;     
  default: return s; 
  }
}

// druid_t::primary_resource ================================================

resource_e druid_t::primary_resource() const
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
  /* Store the amount primal_tenacity absorb remaining so we can use it 
    later to determine if we need to apply the absorb. */
  if ( mastery.primal_tenacity -> ok() && school == SCHOOL_PHYSICAL &&
       ! ( s -> result == RESULT_DODGE || s -> result == RESULT_MISS ) )
    active.primal_tenacity -> absorb_remaining = buff.primal_tenacity -> value();

  if ( sets.has_set_bonus( SET_TANK, T15, B2 ) && s -> result == RESULT_DODGE && buff.savage_defense -> check() )
    buff.tier15_2pc_tank -> trigger();

  if ( buff.barkskin -> up() )
    s -> result_amount *= 1.0 + buff.barkskin -> default_value;

  s -> result_amount *= 1.0 + buff.survival_instincts -> value();

  s -> result_amount *= 1.0 + glyph.ninth_life -> effectN( 1 ).base_value();

  s -> result_amount *= 1.0 + buff.bristling_fur -> value();

  s -> result_amount *= 1.0 + buff.pulverize -> value();

  if ( specialization() == DRUID_GUARDIAN && buff.bear_form -> check() )
  {
    // Track buff benefit
    buff.savage_defense -> up();

    if ( dbc::get_school_mask( school ) & SCHOOL_MAGIC_MASK )
      s -> result_amount *= 1.0 + spec.thick_hide -> effectN( 1 ).percent();
  }

  if ( buff.cenarion_ward -> up() && s -> result_amount > 0 )
    active.cenarion_ward_hot -> execute();

  if ( buff.moonkin_form -> up() && s -> result_amount > 0 && s -> action -> aoe == 0 )
    buff.empowered_moonkin -> trigger();

  player_t::assess_damage( school, dtype, s );

  // Primal Tenacity
  if ( mastery.primal_tenacity -> ok() && school == SCHOOL_PHYSICAL &&
       ! ( s -> result == RESULT_DODGE || s -> result == RESULT_MISS ) &&
       ! buff.primal_tenacity -> check() ) // Check attack eligibility
  {
    // Set the absorb amount (mastery and resolve multipliers are managed by the action itself)
    active.primal_tenacity -> base_dd_min =
    active.primal_tenacity -> base_dd_max = s -> result_mitigated;
    /* Execute absorb, if too large of an absorb was present for the triggering attack then the
       absorb will not be reapplied. The outcome depends on the size of the resulting absorb
       so we must execute on every eligible attack; see primal_tenacity_t::impact() for details. */
    active.primal_tenacity -> execute();
  }
}

// druid_t::assess_heal =====================================================

void druid_t::assess_heal( school_e school,
                           dmg_e    dmg_type,
                           action_state_t* s )
{
  s -> result_amount *= 1.0 + buff.frenzied_regeneration -> check();
  s -> result_amount *= 1.0 + buff.cat_form -> check() * glyph.cat_form -> effectN( 1 ).percent();

  player_t::assess_heal( school, dmg_type, s );
}

druid_td_t::druid_td_t( player_t& target, druid_t& source )
  : actor_pair_t( &target, &source ),
    dots( dots_t() ),
    buffs( buffs_t() ),
    lacerate_stack( 0 )
{
  dots.gushing_wound = target.get_dot( "gushing_wound", &source );
  dots.lacerate      = target.get_dot( "lacerate",      &source );
  dots.lifebloom     = target.get_dot( "lifebloom",     &source );
  dots.moonfire      = target.get_dot( "moonfire",      &source );
  dots.stellar_flare = target.get_dot( "stellar_flare", &source );
  dots.rake          = target.get_dot( "rake",          &source );
  dots.regrowth      = target.get_dot( "regrowth",      &source );
  dots.rejuvenation  = target.get_dot( "rejuvenation",  &source );
  dots.rip           = target.get_dot( "rip",           &source );
  dots.sunfire       = target.get_dot( "sunfire",       &source );
  dots.thrash_bear   = target.get_dot( "thrash_bear",   &source );
  dots.thrash_cat    = target.get_dot( "thrash_cat",    &source );
  dots.wild_growth   = target.get_dot( "wild_growth",   &source );

  buffs.lifebloom = buff_creator_t( *this, "lifebloom", source.find_class_spell( "Lifebloom" ) );
  buffs.bloodletting = buff_creator_t( *this, "bloodletting", source.find_spell( 165699 ) )
                       .default_value( source.find_spell( 165699 ) -> ok() ? source.find_spell( 165699 ) -> effectN( 1 ).percent() : 0.10 )
                       .duration( source.find_spell( 165699 ) -> ok() ? source.find_spell( 165699 ) -> duration() : timespan_t::from_seconds( 6.0 ) )
                       .chance( 1.0 );
}

void druid_t::balance_tracker()
{
  if ( last_check == sim -> current_time ) // No need to re-check balance if the time hasn't changed.
    return;

  if ( buff.celestial_alignment -> up() ) // Balance power is locked while celestial alignment is active.
  { // We should still update the expressions to account for the length of time that celestial alignment is up.
    balance_expressions();

    double ca_remains;
    ca_remains = buff.celestial_alignment -> remains() / timespan_t::from_millis( 1000 );

    eclipse_change += ca_remains;
    eclipse_max += ca_remains;
    time_to_next_lunar += ca_remains;
    time_to_next_solar += ca_remains;
    return;
  }

  last_check = sim -> current_time - last_check;
  // Subtract current time by the last time we checked to get the amount of time elapsed

  if ( talent.euphoria -> ok() ) // Euphoria speeds up the cycle to 20 seconds.
    last_check *= 2;  //To-do: Check if/how it stacks with astral communion/celestial.
  // Effectively, time moves twice as fast, so we'll just double the amount of time since the last check.

  if ( buff.astral_communion -> up() )
    last_check *= 1 + buff.astral_communion -> data().effectN( 1 ).percent();
  // Similarly, when astral communion is running, we will just multiply elapsed time by 3.

  balance_time += last_check; // Add the amount of elapsed time to balance_time
  last_check = sim -> current_time; // Set current time for last check.

  eclipse_amount = 105 * sin( 2 * M_PI * balance_time / timespan_t::from_millis( 40000 ) ); // Re-calculate eclipse

  resources.current[ RESOURCE_ECLIPSE ] = eclipse_amount;

  if ( eclipse_amount >= 100 )
  {
    clamped_eclipse_amount = 100;
    if ( !double_dmg_triggered )
    {
      buff.lunar_peak -> trigger();
      double_dmg_triggered = true;
    }
  }
  else if ( eclipse_amount <= -100 )
  {
    clamped_eclipse_amount = -100;
    if ( !double_dmg_triggered )
    {
      buff.solar_peak -> trigger();
      double_dmg_triggered = true;
    }
  }
  else
  {
    clamped_eclipse_amount = eclipse_amount;
    double_dmg_triggered = false;
  }

  eclipse_direction = 105 * sin( 2 * M_PI * ( balance_time + timespan_t::from_millis( 1 ) ) / timespan_t::from_millis( 40000 ) );
  // Add 1 millisecond to eclipse in order to find the direction we are going.

  if ( eclipse_amount > eclipse_direction )  // Compare current eclipse with the last eclipse to find out what direction we are heading.
    eclipse_direction = -1;
  else
    eclipse_direction = 1;

  balance_expressions(); // Cue madness
}

void druid_t::balance_expressions()
{
  // Eclipse works off of sine waves, thus it is time for a quick trig lesson
  // The general form of eclipse energy is E = A * sin( phi ), where phi is the 
  // phase of the sin wave (corresponding to the phase of our lunar/solar cycle). 
  // Phi starts at zero (E=0), hits max lunar at phi=pi/2 (E=105), hits zero again at phi=pi (E=0)
  // hits max solar at phi=3*pi/2 (E=-105), and returns to E=0 at phi=2*pi completing the cycle.
  // We will exploit some trig properties to efficiently determine certain relevant expression values.

  // phi_lunar is the phase at which we hit the 100-energy lunar cap.
  static const double phi_lunar = asin( 100.0 / 105.0 );
  // easily determined by solving 100 = A * sin(phi) for phi

  // phi_solar is the phase at which we hit the 100-energy solar cap.
  static const double phi_solar = phi_lunar + M_PI;
  // note that this not simply asin(-100.0/A); that would give us the time we *leave* solar thanks
  // to the fact that asin returns values between -pi/2 and pi/2. We want a phase in the third quadrant 
  // (between phi=pi and phi=3*pi/2).  The easiest way to get it is to just add pi to phi_lunar, since what 
  // we're looking for is the exact complement at the other side of the cycle

  // omega is the frequency of our cycle, used to determine phi. We go through 2*pi phase every 40 seconds
  double omega = 2 * M_PI /  40000;
  // Euphoria doubles the frequency of the cycle (reduces the period to 20 seconds)
  if ( talent.euphoria -> ok() )
    omega *= 2;
  
  // phi is the phase, which describes our position in the eclipse cycle, determined by the accumulator balance_time and frequency omega
  double phi;
  phi = omega * balance_time.total_millis();

  // since sin is periodic modulo 2*pi (in other words, sin(x+2*pi)=sin(x) ), we can remove any multiple of 2*pi from phi.
  // This puts phi between 0 and 2*pi, which is convenient
  phi = fmod( phi, 2 * M_PI );

  // if we're already in the lunar max, just return zero
  if ( eclipse_amount > 100 )
    time_to_next_lunar = 0;
  else
    // otherwise, we want to know how long it will be until phi reaches phi_lunar. 
    // phi_lunar - phi gives us what we want when phi < phi_lunar, but gives a negative value if phi > phi_lunar (i.e. we just passed a lunar phase).
    // Again, since everything is modulo 2*pi, we can add 2*pi (to make everything positive) and then fmod().
    // This forces the result to be positive and accurately represent the amount of phase until the next lunar cycle.
    // Divide by omega and 1000 to convert from phase to seconds.
    time_to_next_lunar = fmod( phi_lunar - phi + 2 * M_PI, 2 * M_PI ) / omega / 1000;

  // Same tricks for solar; if we're already in solar max, return zero, otherwise pull the fmod( phi_solar - phi + 2*pi, 2*pi) trick
  if ( eclipse_amount < -100 )
    time_to_next_solar = 0;
  else
    time_to_next_solar = fmod( phi_solar - phi + 2 * M_PI, 2 * M_PI ) / omega / 1000;

  // eclipse_change is the time until we hit zero eclipse energy. This happens whenever the phase is 0, pi, 2*pi, 3*pi, etc.
  // The easiest way to get this is to just fmod() our phase to modulo pi and subtract from pi. 
  // That gives us the amount of phase until we reach the next multiple of pi (same trick as above, just using pi instead of phi_lunar/solar)
  eclipse_change = ( M_PI - fmod( phi, M_PI ) ) / omega / 1000;

  // Astral communion essentially speeds up omega by a factor of 4 for 4 seconds. 
  // In other words, it adds 4*4*omega phase to the cycle over 4 seconds.
  // Since we'd already be gaining 4*omega phase in this time, this represents an additional 3*4*omega phase.
  // Another way to view this is that we "fast-forward" the cycle 12 seconds ahead over our 4 second period,
  // in addition to the 4 seconds of time we'd already be traversing. 
  // This section handles how that additional phase accrual affects the different time estimates while AC is up.
  /*if ( buff.astral_communion -> up() )
  {
    // This is the amount of "fast-forward" time left on Astral Communion; e.g. if there's 3 seconds left, we'll
    // be adding 3*3=9 seconds worth of phase to the cycle. Done as a time value rather than phase since we want
    // to subtract off of existing time estimates.
    double ac;
    ac = buff.astral_communion -> remains() / timespan_t::from_millis( 1000 ) * 3;

    // If our estimate is > the fast-forward time, we can just subtract that time from the estimate.
    // Otherwise, we'll hit the estimate during AC; in that case, just divide by 4 to represent the speed-up.
    if ( eclipse_change > ac )
      eclipse_change -= ac; 
    else
      eclipse_change /= 4;

    if ( time_to_next_lunar > ac )
      time_to_next_lunar -= ac; 
    else
      time_to_next_lunar /= 4;

    if ( time_to_next_solar > ac )
      time_to_next_solar -= ac; 
    else
      time_to_next_solar /= 4;
  }*/

  // the time to next eclipse (either one) is just the minimum of the individual results
  eclipse_max = std::min( time_to_next_lunar, time_to_next_solar );
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class druid_report_t : public player_report_extension_t
{
public:
  druid_report_t( druid_t& player ) :
      p( player )
  {

  }

  virtual void html_customsection( report::sc_html_stream& /* os*/ ) override
  {
    (void) p;
    /*// Custom Class Section
    os << "\t\t\t\t<div class=\"player-section custom_section\">\n"
        << "\t\t\t\t\t<h3 class=\"toggle open\">Custom Section</h3>\n"
        << "\t\t\t\t\t<div class=\"toggle-content\">\n";

    os << p.name();

    os << "\t\t\t\t\t\t</div>\n" << "\t\t\t\t\t</div>\n";*/
  }
private:
  druid_t& p;
};

// DRUID MODULE INTERFACE ===================================================

struct druid_module_t : public module_t
{
  druid_module_t() : module_t( DRUID ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const
  {
    druid_t* p = new druid_t( sim, name, r );
    p -> report_extension = std::shared_ptr<player_report_extension_t>( new druid_report_t( *p ) );
    return p;
  }
  virtual bool valid() const { return true; }
  virtual void init( sim_t* sim ) const
  {
    for ( unsigned int i = 0; i < sim -> actor_list.size(); i++ )
    {
      player_t* p = sim -> actor_list[ i ];
      p -> buffs.stampeding_roar        = buff_creator_t( p, "stampeding_roar", p -> find_spell( 77764 ) )
                                          .max_stack( 1 )
                                          .duration( timespan_t::from_seconds( 8.0 ) );
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
