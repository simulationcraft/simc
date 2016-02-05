// ==========================================================================
// Dedmonwakeen's DPS-DPM Simulator.
// Send questions to natehieter@gmail.com
// ==========================================================================

#include "simulationcraft.hpp"

namespace { // UNNAMED NAMESPACE
// ==========================================================================
// Druid
// ==========================================================================

 /* WoD 6.2 -- TODO:

    = Feral =

    = Balance =
    APL adjustments for set bonuses & trinket : Done 24/11/2015

    = Guardian =
    Max charges statistic
    Tooth & Claw in multi-tank sims

    = Restoration =
    Basically everything

    = To add to wiki = 
    New Options :
      target_self - bool - changes target of spell to caster
      active_rejuvenations - int - number of ticking rejuvenations on the raid
*/

// Forward declarations
struct druid_t;

// Active actions
struct cenarion_ward_hot_t;
struct gushing_wound_t;
struct leader_of_the_pack_t;
struct natures_vigil_proc_t;
struct stalwart_guardian_t;
struct ursocs_vigor_t;
struct yseras_gift_t;
namespace buffs {
struct heart_of_the_wild_buff_t;
}
namespace spells {
struct starshards_t;
}

struct druid_td_t : public actor_target_data_t
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

struct snapshot_counter_t
{
  const sim_t* sim;
  druid_t* p;
  std::vector<buff_t*> b;
  double exe_up;
  double exe_down;
  double tick_up;
  double tick_down;
  bool is_snapped;
  double wasted_buffs;

  snapshot_counter_t( druid_t* player , buff_t* buff );

  bool check_all()
  {
    double n_up = 0;
    for (auto & elem : b)
    {
      if ( elem -> check() )
        n_up++;
    }
    if ( n_up == 0 )
      return false;

    wasted_buffs += n_up - 1;
    return true;
  }

  void add_buff( buff_t* buff )
  {
    b.push_back( buff );
  }

  void count_execute()
  {
    // Skip iteration 0 for non-debug, non-log sims
    if ( sim -> current_iteration == 0 && sim -> iterations > sim -> threads && ! sim -> debug && ! sim -> log )
      return;

    check_all() ? ( exe_up++ , is_snapped = true ) : ( exe_down++ , is_snapped = false );
  }

  void count_tick()
  {
    // Skip iteration 0 for non-debug, non-log sims
    if ( sim -> current_iteration == 0 && sim -> iterations > sim -> threads && ! sim -> debug && ! sim -> log )
      return;

    is_snapped ? tick_up++ : tick_down++;
  }

  double divisor() const
  {
    if ( ! sim -> debug && ! sim -> log && sim -> iterations > sim -> threads )
      return sim -> iterations - sim -> threads;
    else
      return std::min( sim -> iterations, sim -> threads );
  }

  double mean_exe_up() const
  { return exe_up / divisor(); }

  double mean_exe_down() const
  { return exe_down / divisor(); }

  double mean_tick_up() const
  { return tick_up / divisor(); }

  double mean_tick_down() const
  { return tick_down / divisor(); }

  double mean_exe_total() const
  { return ( exe_up + exe_down ) / divisor(); }

  double mean_tick_total() const
  { return ( tick_up + tick_down ) / divisor(); }

  double mean_waste() const
  { return wasted_buffs / divisor(); }

  void merge( const snapshot_counter_t& other )
  {
    exe_up += other.exe_up;
    exe_down += other.exe_down;
    tick_up += other.tick_up;
    tick_down += other.tick_down;
    wasted_buffs += other.wasted_buffs;
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
  bool alternate_stellar_flare; // Player request.
  bool predatory_swiftness_bug; // Trigger a PS when combat begins.
  int initial_berserk_duration;
  bool stellar_flare_cast; //Hacky way of not consuming lunar/solar peak.
  int active_rejuvenations; // Number of rejuvenations on raid.  May be useful for Nature's Vigil timing or resto stuff.
  double max_fb_energy;
  player_t* last_target_dot_moonkin;

  // counters for snapshot tracking
  std::vector<snapshot_counter_t*> counters;

  // Active
  action_t* t16_2pc_starfall_bolt;
  action_t* t16_2pc_sun_bolt;

  // Absorb stats
  stats_t* primal_tenacity_stats;

  // RPPM objects
  real_ppm_t balance_t18_2pc;
  
  struct active_actions_t
  {
    cenarion_ward_hot_t*  cenarion_ward_hot;
    gushing_wound_t*      gushing_wound;
    leader_of_the_pack_t* leader_of_the_pack;
    natures_vigil_proc_t* natures_vigil;
    stalwart_guardian_t*  stalwart_guardian;
    spells::starshards_t* starshards;
    ursocs_vigor_t*       ursocs_vigor;
    yseras_gift_t*        yseras_gift;
  } active;

  // Pets
  std::array<pet_t*,4> pet_force_of_nature; // Add another pet, you can(maybe?) have 4 up with AoC
  std::array<pet_t*,11> pet_fey_moonwing; // 30 second duration, 3 second internal icd... create 11 to be safe.

  // Auto-attacks
  weapon_t caster_form_weapon;
  weapon_t cat_weapon;
  weapon_t bear_weapon;
  melee_attack_t* caster_melee_attack;
  melee_attack_t* cat_melee_attack;
  melee_attack_t* bear_melee_attack;

  double equipped_weapon_dps;

  // T18 (WoD 6.2) class specific trinket effects
  const special_effect_t* starshards;
  const special_effect_t* wildcat_celerity;
  const special_effect_t* stalwart_guardian;
  const special_effect_t* flourish;

  // Buffs
  struct buffs_t
  {
    // General
    buff_t* bear_form;
    buff_t* cat_form;
    buff_t* dash;
    buff_t* displacer_beast;
    buff_t* cenarion_ward;
    buff_t* dream_of_cenarius;
    buff_t* frenzied_regeneration;
    buff_t* incarnation;
    buff_t* natures_vigil;
    buff_t* omen_of_clarity;
    buff_t* prowl;
    buff_t* stampeding_roar;
    buff_t* wild_charge_movement;

    // Balance
    buff_t* astral_communion;
    buff_t* astral_insight;
    buff_t* astral_showers;
    buff_t* celestial_alignment;
    buff_t* empowered_moonkin;
    buff_t* hurricane;
    buff_t* lunar_empowerment;
    buff_t* lunar_peak;
    buff_t* moonkin_form;
    buff_t* solar_empowerment;
    buff_t* solar_peak;
    buff_t* shooting_stars;
    buff_t* starfall;
    buff_t* faerie_blessing; // T18 4P Balance

    // Feral
    buff_t* berserk;
    buff_t* bloodtalons;
    buff_t* claws_of_shirvallah;
    buff_t* predatory_swiftness;
    buff_t* savage_roar;
    buff_t* tigers_fury;
    buff_t* feral_tier15_4pc;
    buff_t* feral_tier16_2pc;
    buff_t* feral_tier16_4pc;
    buff_t* feral_tier17_4pc;

    // Guardian
    absorb_buff_t* primal_tenacity;
    absorb_buff_t* tooth_and_claw_absorb;
    buff_t* barkskin;
    buff_t* bladed_armor;
    buff_t* bristling_fur;
    buff_t* pulverize;
    buff_t* savage_defense;
    buff_t* survival_instincts;
    buff_t* tooth_and_claw;
    buff_t* ursa_major;
    buff_t* guardian_tier15_2pc;
    buff_t* guardian_tier17_4pc;

    // Restoration
    buff_t* natures_swiftness;
    buff_t* soul_of_the_forest;

    // NYI / Needs checking
    buff_t* harmony;
    buff_t* wild_mushroom;
    buffs::heart_of_the_wild_buff_t* heart_of_the_wild;
  } buff;

  // Cooldowns
  struct cooldowns_t
  {
    cooldown_t* berserk;
    cooldown_t* celestial_alignment;
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
    gain_t* glyph_ferocious_bite;
    gain_t* leader_of_the_pack;
    gain_t* moonfire;
    gain_t* rake;
    gain_t* shred;
    gain_t* swipe;
    gain_t* tigers_fury;
    gain_t* feral_tier15_2pc;
    gain_t* feral_tier16_4pc;
    gain_t* feral_tier17_2pc;
    gain_t* feral_tier18_4pc;

    // Guardian (Bear)
    gain_t* bear_form;
    gain_t* frenzied_regeneration;
    gain_t* primal_tenacity;
    gain_t* stalwart_guardian;
    gain_t* rage_refund;
    gain_t* tooth_and_claw;
    gain_t* guardian_tier17_2pc;
    gain_t* guardian_tier18_2pc;
  } gain;

  // Perks
  struct perk_t
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
    const spell_data_t* solstice;
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
    const spell_data_t* primal_tenacity_AP;
    const spell_data_t* razor_claws;
    const spell_data_t* total_eclipse;

    // NYI / TODO!
    const spell_data_t* harmony;
  } mastery;

  // Procs
  struct procs_t
  {
    proc_t* omen_of_clarity;
    proc_t* omen_of_clarity_wasted;
    proc_t* primal_fury;
    proc_t* shooting_stars;
    proc_t* shooting_stars_wasted;
    proc_t* starshards;
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
    const spell_data_t* swipe;
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
    const spell_data_t* guardian_passive; // Hidden guardian modifiers

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
    // Cat
    const spell_data_t* ferocious_bite; 
    const spell_data_t* berserk_cat; // Berserk cat resource cost reducer
    const spell_data_t* cat_form; // Cat form bonuses
    const spell_data_t* cat_form_speed;
    const spell_data_t* primal_fury; // Primal fury gain
    const spell_data_t* gushing_wound; // Feral t17 4pc driver
    const spell_data_t* survival_instincts; // Survival instincts aura

    // Bear
    const spell_data_t* bear_form_skill; // Bear form skill
    const spell_data_t* bear_form_passive; // Bear form passive buff
    const spell_data_t* berserk_bear; // Berserk bear mangler
    const spell_data_t* frenzied_regeneration;

    // Moonkin
    const spell_data_t* moonkin_form; // Moonkin form bonuses
    const spell_data_t* starfall_aura;

    // Resto
    const spell_data_t* regrowth; // Old GoRegrowth
  } spell;

  // Talents
  struct talents_t
  {
    const spell_data_t* feline_swiftness;
    const spell_data_t* displacer_beast;
    const spell_data_t* wild_charge;

    const spell_data_t* yseras_gift;
    const spell_data_t* renewal;
    const spell_data_t* cenarion_ward;

    const spell_data_t* faerie_swarm; //pvp
    const spell_data_t* mass_entanglement; //pvp
    const spell_data_t* typhoon; //pvp

    const spell_data_t* soul_of_the_forest;
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
    alternate_stellar_flare( 0 ),
    predatory_swiftness_bug( 0 ),
    initial_berserk_duration( 0 ),
    stellar_flare_cast( 0 ),
    active_rejuvenations( 0 ),
    max_fb_energy( 0 ),
    t16_2pc_starfall_bolt( nullptr ),
    t16_2pc_sun_bolt( nullptr ),
    balance_t18_2pc( *this ),
    active( active_actions_t() ),
    pet_force_of_nature(),
    pet_fey_moonwing(),
    caster_form_weapon(),
    starshards(),
    wildcat_celerity(),
    stalwart_guardian(),
    flourish(),
    buff( buffs_t() ),
    cooldown( cooldowns_t() ),
    gain( gains_t() ),
    perk( perk_t() ),
    glyph( glyphs_t() ),
    mastery( masteries_t() ),
    proc( procs_t() ),
    spec( specializations_t() ),
    spell( spells_t() ),
    talent( talents_t() ),
    inflight_starsurge( false )
  {
    t16_2pc_starfall_bolt = nullptr;
    t16_2pc_sun_bolt      = nullptr;
    double_dmg_triggered = false;
    last_target_dot_moonkin = nullptr;
    
    cooldown.berserk             = get_cooldown( "berserk"             );
    cooldown.celestial_alignment = get_cooldown( "celestial_alignment" );
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

    caster_melee_attack = nullptr;
    cat_melee_attack = nullptr;
    bear_melee_attack = nullptr;

    equipped_weapon_dps = 0;

    regen_type = REGEN_DYNAMIC;
    regen_caches[ CACHE_HASTE ] = true;
    regen_caches[ CACHE_ATTACK_HASTE ] = true;
  }

  virtual           ~druid_t();

  // Character Definition
  virtual void      arise() override;
  virtual void      init() override;
  virtual void      init_spells() override;
  virtual void      init_base_stats() override;
  virtual void      create_buffs() override;
  virtual void      init_scaling() override;
  virtual void      init_gains() override;
  virtual void      init_procs() override;
  virtual void      init_resources( bool ) override;
  virtual void      init_rng() override;
  virtual void      init_absorb_priority() override;
  virtual void      invalidate_cache( cache_e ) override;
  virtual void      combat_begin() override;
  virtual void      reset() override;
  virtual void      merge( player_t& other ) override;
  virtual void      regen( timespan_t periodicity ) override;
  virtual timespan_t available() const override;
  virtual double    composite_armor_multiplier() const override;
  virtual double    composite_melee_attack_power() const override;
  virtual double    composite_melee_crit() const override;
  virtual double    composite_attack_power_multiplier() const override;
  virtual double    temporary_movement_modifier() const override;
  virtual double    passive_movement_modifier() const override;
  virtual double    composite_player_multiplier( school_e school ) const override;
  virtual double    composite_player_heal_multiplier( const action_state_t* s ) const override;
  virtual double    composite_spell_crit() const override;
  virtual double    composite_spell_power( school_e school ) const override;
  virtual double    composite_attribute( attribute_e attr ) const override;
  virtual double    composite_attribute_multiplier( attribute_e attr ) const override;
  virtual double    matching_gear_multiplier( attribute_e attr ) const override;
  virtual double    composite_damage_versatility() const override;
  virtual double    composite_heal_versatility() const override;
  virtual double    composite_mitigation_versatility() const override;
  virtual double    composite_parry() const override { return 0; }
  virtual double    composite_block() const override { return 0; }
  virtual double    composite_crit_avoidance() const override;
  virtual double    composite_melee_expertise( const weapon_t* ) const override;
  virtual double    composite_dodge() const override;
  virtual double    composite_rating_multiplier( rating_e rating ) const override;
  virtual expr_t*   create_expression( action_t*, const std::string& name ) override;
  virtual action_t* create_action( const std::string& name, const std::string& options ) override;
  virtual pet_t*    create_pet   ( const std::string& name, const std::string& type = std::string() ) override;
  virtual void      create_pets() override;
  virtual resource_e primary_resource() const override;
  virtual role_e    primary_role() const override;
  virtual stat_e    convert_hybrid_stat( stat_e s ) const override;
  virtual double    mana_regen_per_second() const override;
  virtual void      assess_damage( school_e school, dmg_e, action_state_t* ) override;
  virtual void      assess_damage_imminent_pre_absorb( school_e, dmg_e, action_state_t* ) override;
  virtual void      assess_heal( school_e, dmg_e, action_state_t* ) override;
  virtual void      create_options() override;
  virtual action_t* create_proc_action( const std::string& name, const special_effect_t& ) override;
  virtual std::string      create_profile( save_e type = SAVE_ALL ) override;
  virtual void      recalculate_resource_max( resource_e r ) override;
  virtual void      copy_from( player_t* source ) override;

  void              apl_precombat();
  void              apl_default();
  void              apl_feral();
  void              apl_balance();
  void              apl_guardian();
  void              apl_restoration();
  virtual void      init_action_list() override;
  virtual bool      has_t18_class_trinket() const override;

  target_specific_t<druid_td_t> target_data;

  virtual druid_td_t* get_target_data( player_t* target ) const override
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

druid_t::~druid_t()
{
  range::dispose( counters );
}

snapshot_counter_t::snapshot_counter_t( druid_t* player , buff_t* buff ) :
  sim( player -> sim ), p( player ), b( 0 ), 
  exe_up( 0 ), exe_down( 0 ), tick_up( 0 ), tick_down( 0 ), is_snapped( false ), wasted_buffs( 0 )
{
  b.push_back( buff );
  p -> counters.push_back( this );
}

// Gushing Wound (tier17_4pc_melee) ========================================================

struct gushing_wound_t : public residual_action::residual_periodic_action_t< attack_t >
{
  bool trigger_t17_2p;
  gushing_wound_t( druid_t* p ) :
    residual_action::residual_periodic_action_t< attack_t >( "gushing_wound", p, p -> find_spell( 166638 ) ),
    trigger_t17_2p( false )
  {
    background = dual = proc = true;
    trigger_t17_2p = p -> sets.has_set_bonus( DRUID_FERAL, T17, B2 );
  }

  druid_t* p() const
  { return static_cast<druid_t*>( player ); }

  virtual void tick( dot_t* d ) override
  {
    residual_action::residual_periodic_action_t<attack_t>::tick( d );

    if ( trigger_t17_2p )
      p() -> resource_gain( RESOURCE_ENERGY,
                            p() -> sets.set( DRUID_FERAL, T17, B2 ) -> effectN( 1 ).base_value(),
                            p() -> gain.feral_tier17_2pc );
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

    virtual void init() override
    {
      heal_t::init();
      // Disable the snapshot_flags for all multipliers, but specifically allow factors in
      // action_state_t::composite_da_multiplier() to be called.
      snapshot_flags &= STATE_NO_MULTIPLIER;
      snapshot_flags |= STATE_MUL_DA;
    }

    virtual double action_multiplier() const override
    {
      double am = heal_t::action_multiplier();
      
      if ( fromDmg )
        am *= dmg_coeff;
      else
        am *= heal_coeff;

      return am;
    }

    virtual void execute() override
    {
      target = smart_target();

      heal_t::execute();
    }
  };

  heal_proc_t*   heal;

  natures_vigil_proc_t( druid_t* p ) :
    spell_t( "natures_vigil", p, spell_data_t::nil() )
  {
    heal    = new heal_proc_t( p );
  }

  void trigger( double amount, bool harmful = true )
  {
    heal -> base_dd_min = heal -> base_dd_max = amount;
    heal -> fromDmg = harmful;
    heal -> execute();
  }
};

// Stalwart Guardian ( 6.2 T18 Guardian Trinket) =========================

struct stalwart_guardian_t : public absorb_t
{
  struct stalwart_guardian_reflect_t : public attack_t
  {
    stalwart_guardian_reflect_t( druid_t* p ) :
      attack_t( "stalwart_guardian_reflect", p, p -> find_spell( 185321 ) )
    {
      may_block = may_dodge = may_parry = may_miss = true;
      may_crit = true;
    }
  };

  double incoming_damage;
  double absorb_limit;
  double absorb_size;
  player_t* triggering_enemy;
  stalwart_guardian_reflect_t* reflect;

  stalwart_guardian_t( druid_t* p ) :
    absorb_t( "stalwart_guardian_bg", p, p -> find_spell( 185321 ) ),
    incoming_damage( 0 ), absorb_limit( 0 ), absorb_size( 0 )
  {
    background = quiet = true;
    may_crit = false;
    may_multistrike = 0;
    target = p;
    harmful = false;

    reflect = new stalwart_guardian_reflect_t( p );
  }

  druid_t* p() const
  { return static_cast<druid_t*>( player ); }

  void init() override
  {
    if ( p() -> stalwart_guardian )
    {
      const spell_data_t* trinket = p() -> stalwart_guardian -> driver();
      attack_power_mod.direct     = trinket -> effectN( 1 ).average( p() -> stalwart_guardian -> item ) / 100.0;
      absorb_limit                = trinket -> effectN( 2 ).percent();
    }

    absorb_t::init();

    snapshot_flags &= ~STATE_VERSATILITY; // Is not affected by versatility.
  }

  void execute() override
  {
    assert( p() -> stalwart_guardian );

    absorb_t::execute();

    // Trigger damage reflect
    double resolve = 1.0;
    if ( p() -> resolve_manager.is_started() )
      resolve *= 1.0 + player -> buffs.resolve -> current_value / 100.0;

    // Base damage is equal to the size of the absorb pre-resolve.
    reflect -> base_dd_min = absorb_size / resolve;
    reflect -> target = triggering_enemy;
    reflect -> execute();
  }

  void impact( action_state_t* s ) override
  {
    s -> result_amount = std::min( s -> result_total, absorb_limit * incoming_damage );
    absorb_size = s -> result_amount;
  }
};

// Ursoc's Vigor ( tier16_4pc_tank ) =====================================

struct ursocs_vigor_t : public heal_t
{
  double ap_coefficient;
  int ticks_remain;

  ursocs_vigor_t( druid_t* p ) :
    heal_t( "ursocs_vigor", p, p -> find_spell( 144888 ) ),
    ticks_remain( 0 )
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

  virtual void tick( dot_t *d ) override
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
    hasted_ticks = false;
  }

  virtual void execute() override
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
    base_pct_heal = data().effectN( 1 ).percent();
  }
};

// Ysera's Gift ==============================================================

struct yseras_gift_t : public heal_t
{
  yseras_gift_t( druid_t* p ) :
    heal_t( "yseras_gift", p, p -> talent.yseras_gift )
  {
    base_tick_time = data().effectN( 1 ).period();
    dot_duration = sim -> expected_iteration_time > timespan_t::zero() ?
                   2 * sim -> expected_iteration_time :
                   2 * sim -> max_time * ( 1.0 + sim -> vary_combat_length ); // "infinite" duration
    harmful = tick_may_crit = hasted_ticks = false;
    may_multistrike = 0;
    background = proc = dual = true;
    target = p;

    tick_pct_heal = data().effectN( 1 ).percent();
    
    // 10/26/14: Heals for half for Guardians.
    if ( p -> specialization() == DRUID_GUARDIAN )
      tick_pct_heal *= 0.5;
  }

  virtual void init() override
  {
    heal_t::init();

    snapshot_flags &= ~STATE_RESOLVE; // Is not affected by resolve.
  }

  // Override calculate_tick_amount for unique mechanic (heals smart target for % of own health)
  // This might not be the best way to do this, but it works.
  virtual double calculate_tick_amount( action_state_t* state, double /* dmg_multiplier */ ) const override
  {
    double amount = state -> action -> player -> resources.max[ RESOURCE_HEALTH ] * tick_pct_heal;
    
    // Record initial amount to state
    state -> result_raw = amount;

    // Multipliers removed here to avoid unneccesary code.
    // Refer to heal_t::calculate_tick_amount if mechanics change.

    // replicate debug output of calculate_tick_amount
    if ( sim -> debug )
    {
      sim -> out_debug.printf( "%s amount for %s on %s: ta=%.0f pct=%.3f b_ta=%.0f bonus_ta=%.0f s_mod=%.2f s_power=%.0f a_mod=%.2f a_power=%.0f mult=%.2f",
        player -> name(), name(), target -> name(), amount,
        tick_pct_heal, base_ta( state ), bonus_ta( state ),
        spell_tick_power_coefficient( state ), state -> composite_spell_power(),
        attack_tick_power_coefficient( state ), state -> composite_attack_power(),
        state -> composite_ta_multiplier() );
    }

    // Record total amount to state
    state -> result_total = amount;

    return amount;
  }

  virtual void execute() override
  {
    if ( player -> health_percentage() < 100 )
      target = player;
    else
      target = smart_target();

    heal_t::execute();
  }
};

namespace pets {


// ==========================================================================
// Pets and Guardians
// ==========================================================================

// T18 2PC Balance Fairies ==================================================

  struct fey_moonwing_t: public pet_t
  {
    struct fey_missile_t: public spell_t
    {
      fey_missile_t( fey_moonwing_t* player ):
        spell_t( "fey_missile", player, player -> find_spell( 188046 ) )
      {
        if ( player -> o() -> pet_fey_moonwing[0] )
          stats = player -> o() -> pet_fey_moonwing[0] -> get_stats( "fey_missile" );
        may_crit = true;

        // Casts have a delay that decreases with haste. This is a very rough approximation.
        cooldown -> duration = timespan_t::from_millis( 600 );
      }

      double cooldown_reduction() const override
      {
        return spell_t::cooldown_reduction() * composite_haste();
      }
    };
    druid_t* o() { return static_cast<druid_t*>( owner ); }

    fey_moonwing_t( sim_t* sim, druid_t* owner ):
      pet_t( sim, owner, "fey_moonwing", true /*GUARDIAN*/, true )
    {
      owner_coeff.sp_from_sp = 0.75;
      regen_type = REGEN_DISABLED;
    }

    void init_base_stats() override
    {
      pet_t::init_base_stats();

      resources.base[RESOURCE_HEALTH] = owner -> resources.max[RESOURCE_HEALTH] * 0.4;
      resources.base[RESOURCE_MANA] = 0;

      initial.stats.attribute[ATTR_INTELLECT] = 0;
      initial.spell_power_per_intellect = 0;
      intellect_per_owner = 0;
      stamina_per_owner = 0;
      action_list_str = "fey_missile";
    }

    void summon( timespan_t duration ) override
    {
      pet_t::summon( duration );
      o() -> buff.faerie_blessing -> trigger();
    }

    action_t* create_action( const std::string& name,
      const std::string& options_str ) override
    {
      if ( name == "fey_missile"  ) return new fey_missile_t( this );
      return pet_t::create_action( name, options_str );
    }
  };

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

  force_of_nature_balance_t( sim_t* sim, druid_t* owner ):
    pet_t( sim, owner, "treant", true /*GUARDIAN*/, true )
  {
    owner_coeff.sp_from_sp = 0.45;
    owner_coeff.ap_from_ap = 0.60;
    owner_coeff.health = 1.35; // needs checking
    owner_coeff.armor = 1.80; // needs checking
    action_list_str = "wrath";
    regen_type = REGEN_DISABLED;
  }

  virtual void init_base_stats() override
  {
    pet_t::init_base_stats();

    resources.base[ RESOURCE_HEALTH ] = owner -> resources.max[ RESOURCE_HEALTH ] * 0.4;
    resources.base[ RESOURCE_MANA   ] = 0;

    initial.stats.attribute[ ATTR_INTELLECT ] = 0;
    initial.spell_power_per_intellect = 0;
    intellect_per_owner = 0;
    stamina_per_owner = 0;
  }

  virtual resource_e primary_resource() const override { return RESOURCE_MANA; }

  virtual action_t* create_action( const std::string& name,
                                   const std::string& options_str ) override
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
    bool first_swing;
    melee_t( force_of_nature_feral_t* p )
      : melee_attack_t( "melee", p, spell_data_t::nil() ), owner( nullptr ), first_swing( true )
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

    timespan_t execute_time() const override
    {
      timespan_t t = melee_attack_t::execute_time();
      if ( first_swing )
        t = timespan_t::from_seconds( rng().range( 1.0, 1.5 ) );
      return t;
    }

    void execute() override
    {
      melee_attack_t::execute();
      if ( first_swing )
        first_swing = false;
    }

    void reset() override
    {
      melee_attack_t::reset();
      first_swing = true;
    }

    void init() override
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
      melee_attack_t( "rake", p, p -> find_spell( 150017 ) ), owner( nullptr )
    {
      special = may_crit = tick_may_crit = true;
      owner            = p -> o();
    }

    force_of_nature_feral_t* p()
    { return static_cast<force_of_nature_feral_t*>( player ); }
    const force_of_nature_feral_t* p() const
    { return static_cast<force_of_nature_feral_t*>( player ); }

    void init() override
    {
      melee_attack_t::init();
      if ( ! player -> sim -> report_pets_separately && player != p() -> o() -> pet_force_of_nature[ 0 ] )
        stats = p() -> o() -> pet_force_of_nature[ 0 ] -> get_stats( name(), this );
    }

    virtual double composite_ta_multiplier( const action_state_t* state ) const override
    {
      double m = melee_attack_t::composite_ta_multiplier( state );

      if ( p() -> o() -> mastery.razor_claws -> ok() )
        m *= 1.0 + p() -> o() -> cache.mastery_value();

      return m;
    }

    // Treat direct damage as "bleed"
    // Must use direct damage because tick_zeroes cannot be blocked, and this attack is going to get blocked occasionally.
    double composite_da_multiplier( const action_state_t* state ) const override
    {
      double m = melee_attack_t::composite_da_multiplier( state );

      if ( p() -> o() -> mastery.razor_claws -> ok() )
        m *= 1.0 + p() -> o() -> cache.mastery_value();

      return m;
    }

    virtual double target_armor( player_t* ) const override
    { return 0.0; }

    virtual void execute() override
    {
      melee_attack_t::execute();
      p() -> change_position( POSITION_BACK ); // After casting it's "opener" ability, move behind the target.
    }
  };
  
  melee_t* melee;
  force_of_nature_feral_t( sim_t* sim, druid_t* p ):
    pet_t( sim, p, "treant", true, true ), melee( nullptr )
  {
    main_hand_weapon.type = WEAPON_BEAST;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    main_hand_weapon.min_dmg = owner -> find_spell( 102703 ) -> effectN( 1 ).min( owner );
    main_hand_weapon.max_dmg = owner -> find_spell( 102703 ) -> effectN( 1 ).max( owner );
    main_hand_weapon.damage = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    owner_coeff.sp_from_sp = 0.45;
    owner_coeff.ap_from_ap = 0.60;
    owner_coeff.health = 1.35; // needs checking
    owner_coeff.armor = 1.80; // needs checking
    regen_type = REGEN_DISABLED;
  }

  druid_t* o()
  { return static_cast< druid_t* >( owner ); }
  const druid_t* o() const
  { return static_cast<const druid_t* >( owner ); }

  virtual void init_base_stats() override
  {
    pet_t::init_base_stats();

    resources.base[ RESOURCE_HEALTH ] = owner -> resources.max[ RESOURCE_HEALTH ] * 0.4;
    resources.base[ RESOURCE_MANA   ] = 0;
    stamina_per_owner = 0.0;

    melee = new melee_t( this );
  }

  virtual void init_action_list() override
  {
    action_list_str = "rake";

    pet_t::init_action_list();
  }
  
  action_t* create_action( const std::string& name,
                           const std::string& options_str ) override
  {
    if ( name == "rake" ) return new rake_t( this );

    return pet_t::create_action( name, options_str );
  }

  virtual void summon( timespan_t duration = timespan_t::zero() ) override
  {
    pet_t::summon( duration );
    this -> change_position( POSITION_FRONT ); // Emulate Treant spawning in front of the target.
  }

  void schedule_ready( timespan_t delta_time = timespan_t::zero(), bool waiting = false ) override
  {
    if ( melee && !melee -> execute_event )
    {
      melee -> first_swing = true;
      melee -> schedule_execute();
    }
    
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
      : melee_attack_t( "melee", p, spell_data_t::nil() ), owner( nullptr )
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

  force_of_nature_guardian_t( sim_t* sim, druid_t* p ):
    pet_t( sim, p, "treant", true, true ), melee( nullptr )
  {
    main_hand_weapon.type = WEAPON_BEAST;
    main_hand_weapon.swing_time = timespan_t::from_seconds( 2.0 );
    main_hand_weapon.min_dmg = owner -> find_spell( 102706 ) -> effectN( 1 ).min( owner ) * 0.2;
    main_hand_weapon.max_dmg = owner -> find_spell( 102706 ) -> effectN( 1 ).max( owner ) * 0.2;
    main_hand_weapon.damage = ( main_hand_weapon.min_dmg + main_hand_weapon.max_dmg ) / 2;
    owner_coeff.ap_from_ap = 0.2 * 1.2;
    owner_coeff.ap_from_ap *= 1.8;
    owner_coeff.sp_from_sp = 0.45;
    owner_coeff.health = 1.35; // needs checking
    owner_coeff.armor = 1.80; // needs checking
    regen_type = REGEN_DISABLED;
  }

  druid_t* o()
  { return static_cast< druid_t* >( owner ); }

  virtual void init_base_stats() override
  {
    pet_t::init_base_stats();

    resources.base[ RESOURCE_HEALTH ] = owner -> resources.max[ RESOURCE_HEALTH ] * 0.4;
    resources.base[ RESOURCE_MANA   ] = 0;
    stamina_per_owner = 0.0;

    melee = new melee_t( this );
  }

  virtual void summon( timespan_t duration = timespan_t::zero() ) override
  {
    pet_t::summon( duration );
    schedule_ready();
  }

  virtual void schedule_ready( timespan_t delta_time = timespan_t::zero(), bool waiting = false ) override
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

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );

    druid.last_check = sim -> current_time() - druid.last_check;
    druid.last_check *= 1 + druid.buff.astral_communion -> data().effectN( 1 ).percent();
    druid.balance_time += druid.last_check;
    druid.last_check = sim -> current_time();
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

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );

    swap_melee( druid.caster_melee_attack, druid.caster_form_weapon );

    sim -> auras.critical_strike -> decrement();

    druid.recalculate_resource_max( RESOURCE_HEALTH );

    if ( druid.specialization() == DRUID_GUARDIAN )
      druid.resolve_manager.stop();
  }

  virtual void start( int stacks, double value, timespan_t duration ) override
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
    ) {}

  virtual bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    bool refresh = druid.buff.berserk -> check() != 0;

    if ( druid.perk.enhanced_berserk -> ok() )
      player -> resources.max[ RESOURCE_ENERGY ] += druid.perk.enhanced_berserk -> effectN( 1 ).base_value();
    
    /* If Druid Tier 18 (WoD 6.2) trinket effect is in use, adjust Berserk duration
       based on spell data of the special effect. */
    if ( druid.wildcat_celerity )
      duration *= 1.0 + druid.wildcat_celerity -> driver() -> effectN( 1 ).average( druid.wildcat_celerity -> item ) / 100.0;

    bool success = druid_buff_t<buff_t>::trigger( stacks, value, chance, duration );

    if ( ! refresh && success )
      druid.max_fb_energy *= 1.0 + druid.spell.berserk_cat -> effectN( 1 ).percent();

    return success;
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    druid.max_fb_energy /= 1.0 + druid.spell.berserk_cat -> effectN( 1 ).percent();

    if ( druid.perk.enhanced_berserk -> ok() )
    {
      player -> resources.max[ RESOURCE_ENERGY ] -= druid.perk.enhanced_berserk -> effectN( 1 ).base_value();
      // Force energy down to cap if it's higher.
      player -> resources.current[ RESOURCE_ENERGY ] = std::min( player -> resources.current[ RESOURCE_ENERGY ], player -> resources.max[ RESOURCE_ENERGY ]);
    }

    druid_buff_t<buff_t>::expire_override( expiration_stacks, remaining_duration );
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

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );
    
    swap_melee( druid.caster_melee_attack, druid.caster_form_weapon );

    druid.buff.claws_of_shirvallah -> expire();

    sim -> auras.critical_strike -> decrement();
  }

  virtual void start( int stacks, double value, timespan_t duration ) override
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
    tick_behavior = BUFF_TICK_NONE;
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );

    druid.last_check = sim -> current_time();
  }
};

// Moonkin Form =============================================================

struct moonkin_form_t : public druid_buff_t< buff_t >
{
  moonkin_form_t( druid_t& p ) :
    base_t( p, buff_creator_t( &p, "moonkin_form", p.spec.moonkin_form )
               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER ) )
  { }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );

    sim -> auras.mastery -> decrement();
  }

  virtual void start( int stacks, double value, timespan_t duration ) override
  {
    druid.buff.bear_form -> expire();
    druid.buff.cat_form  -> expire();

    base_t::start( stacks, value, duration );

    if ( ! sim -> overrides.mastery )
      sim -> auras.mastery -> trigger();
  }
};

// Omen of Clarity =========================================================

struct omen_of_clarity_buff_t : public druid_buff_t<buff_t>
{
  omen_of_clarity_buff_t( druid_t& p ) :
    druid_buff_t<buff_t>( p, buff_creator_t( &p, "omen_of_clarity", p.spec.omen_of_clarity -> effectN( 1 ).trigger() )
      .chance( p.specialization() == DRUID_RESTORATION ? p.find_spell( 113043 ) -> proc_chance()
                                                     : p.find_spell( 16864 ) -> proc_chance() )
      .cd( timespan_t::from_seconds( 0.0 ) ) // Cooldown handled by ability
    )
  {}

  virtual bool trigger( int stacks, double value, double chance, timespan_t duration ) override
  {
    bool refresh = druid.buff.omen_of_clarity -> check() != 0;

    bool success = druid_buff_t<buff_t>::trigger( stacks, value, chance, duration );

    if ( ! refresh && success )
      druid.max_fb_energy -= druid.spell.ferocious_bite -> powerN( 1 ).cost() * ( 1.0 + druid.buff.berserk -> check() * druid.spell.berserk_cat -> effectN( 1 ).percent() );

    return success;
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    druid.max_fb_energy += druid.spell.ferocious_bite -> powerN( 1 ).cost() * ( 1.0 + druid.buff.berserk -> check() * druid.spell.berserk_cat -> effectN( 1 ).percent() );

    druid_buff_t<buff_t>::expire_override( expiration_stacks, remaining_duration );
  }
};

// Tooth and Claw Absorb Buff ===============================================

static bool tooth_and_claw_can_absorb( const action_state_t* s )
{ return ! s -> action -> special; }

struct tooth_and_claw_absorb_t : public absorb_buff_t
{
  tooth_and_claw_absorb_t( druid_t& p ) :
    absorb_buff_t( absorb_buff_creator_t( &p, "tooth_and_claw_absorb", p.find_spell( 135597 ) )
      .school( SCHOOL_PHYSICAL )
      .source( p.get_stats( "tooth_and_claw_absorb" ) )
      .gain( p.get_gain( "tooth_and_claw_absorb" ) )
      .high_priority( true )
      .eligibility( &tooth_and_claw_can_absorb )
    )
  {}

  virtual bool trigger( int stacks, double value, double, timespan_adl_barrier::timespan_t ) override
  {
    // Tooth and Claw absorb stacks by value.
    return absorb_buff_t::trigger( stacks, value + current_value );
  }

  virtual void absorb_used( double /* amount */ ) override
  {
    // Tooth and Claw expires after ANY amount of the absorb is used.
    expire();
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

  virtual void start( int stacks, double value, timespan_t duration ) override
  {
    base_t::start( stacks, value, duration );

    recalculate_temporary_health( value );
  }

  virtual void refresh( int stacks, double value, timespan_t duration ) override
  {
    base_t::refresh( stacks, value, duration );
    
    recalculate_temporary_health( value );
  }

  virtual void expire_override( int expiration_stacks, timespan_t remaining_duration ) override
  {
    base_t::expire_override( expiration_stacks, remaining_duration );

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
      druid.stat_gain( STAT_MAX_HEALTH, diff, (gain_t*) nullptr, (action_t*) nullptr, true );
    else if ( diff < 0 )
      druid.stat_loss( STAT_MAX_HEALTH, -diff, (gain_t*) nullptr, (action_t*) nullptr, true );
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

  bool triggers_natures_vigil;

  druid_action_t( const std::string& n, druid_t* player,
                  const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ), triggers_natures_vigil( true )
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

    if ( p() -> buff.feral_tier17_4pc -> check() )
      trigger_gushing_wound( s -> target, s -> result_amount );

    if ( ab::aoe == 0 && s -> result_total > 0 && p() -> buff.natures_vigil -> up() && triggers_natures_vigil ) 
      p() -> active.natures_vigil -> trigger( ab::harmful ? s -> result_amount :  s -> result_total , ab::harmful ); // Natures Vigil procs from overhealing
  }

  virtual void tick( dot_t* d )
  {
    ab::tick( d );

    if ( p() -> buff.feral_tier17_4pc -> check() )
      trigger_gushing_wound( d -> target, d -> state -> result_amount );

    if ( ab::aoe == 0 && d -> state -> result_total > 0 && p() -> buff.natures_vigil -> up() && triggers_natures_vigil )
      p() -> active.natures_vigil -> trigger( ab::harmful ? d -> state -> result_amount : d -> state -> result_total, ab::harmful );
  }

  virtual void multistrike_tick( const action_state_t* src_state, action_state_t* ms_state, double multiplier )
  {
    ab::multistrike_tick( src_state, ms_state, multiplier );

    if ( p() -> buff.feral_tier17_4pc -> check() )
      trigger_gushing_wound( ms_state -> target, ms_state -> result_amount );

    if ( ab::aoe == 0 && ms_state -> result_total > 0 && p() -> buff.natures_vigil -> up() && triggers_natures_vigil )
      p() -> active.natures_vigil -> trigger( ab::harmful ? ms_state -> result_amount : ms_state -> result_total, ab::harmful );
  }

  void trigger_gushing_wound( player_t* t, double dmg )
  {
    if ( ! ( ab::special && ab::harmful && dmg > 0 ) )
      return;

    residual_action::trigger(
      p() -> active.gushing_wound, // ignite spell
      t, // target
      p() -> spell.gushing_wound -> effectN( 1 ).percent() * dmg );
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
      virtual double evaluate() override
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
protected:
  bool attackHit;
private:
  typedef druid_action_t< Base > ab;
public:
  typedef druid_attack_t base_t;
  
  bool consume_bloodtalons;
  snapshot_counter_t* bt_counter;
  snapshot_counter_t* tf_counter;

  druid_attack_t( const std::string& n, druid_t* player,
                  const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s ), attackHit( false ), consume_bloodtalons( false ),
    bt_counter( nullptr ), tf_counter( nullptr )
  {
    ab::may_glance    = false;
    ab::special       = true;
  }

  virtual void init()
  {
    ab::init();
    
    consume_bloodtalons = ab::harmful && ab::special && ab::trigger_gcd > timespan_t::zero();

    if ( consume_bloodtalons )
    {
      bt_counter = new snapshot_counter_t( ab::p() , ab::p() -> buff.bloodtalons );
      tf_counter = new snapshot_counter_t( ab::p() , ab::p() -> buff.tigers_fury );
    }
  }

  virtual void execute()
  {
    attackHit = false;

    ab::execute();

    if( consume_bloodtalons && attackHit )
    {
      bt_counter -> count_execute();
      tf_counter -> count_execute();

      ab::p() -> buff.bloodtalons -> decrement();
    }
  }

  virtual void impact( action_state_t* s )
  {
    ab::impact( s );

    if ( ab::result_is_hit( s -> result ) )
      attackHit = true;

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

    if( consume_bloodtalons )
    {
      bt_counter -> count_tick();
      tf_counter -> count_tick();
    }
  }

  virtual double composite_persistent_multiplier( const action_state_t* s ) const
  {
    double pm = ab::composite_persistent_multiplier( s );
    
    if ( ab::p() -> talent.bloodtalons -> ok() && consume_bloodtalons && ab::p() -> buff.bloodtalons -> check() )
      pm *= 1.0 + ab::p() -> buff.bloodtalons -> data().effectN( 1 ).percent();

    if ( ! ab::p() -> buff.bear_form -> check() && dbc::is_school( ab::school, SCHOOL_PHYSICAL ) )
      pm *= 1.0 + ab::p() -> buff.savage_roar -> check() * ab::p() -> buff.savage_roar -> default_value; // Avoid using value() to prevent skewing benefit_pct.

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

    double chance = ab::weapon -> proc_chance_on_swing( 3.5 );

    if ( ab::p() -> sets.has_set_bonus( DRUID_FERAL, T18, B2 ) )
      chance *= 1.0 + ab::p() -> sets.set( DRUID_FERAL, T18, B2 ) -> effectN( 1 ).percent();

    int active = ab::p() -> buff.omen_of_clarity -> check();

    // 3.5 PPM via https://twitter.com/Celestalon/status/482329896404799488
    if ( ab::p() -> buff.omen_of_clarity -> trigger( 1, buff_t::DEFAULT_VALUE(), chance, ab::p() -> buff.omen_of_clarity -> buff_duration ) ) {
      ab::p() -> proc.omen_of_clarity -> occur();
      
      if ( active )
        ab::p() -> proc.omen_of_clarity_wasted -> occur();

      if ( ab::p() -> sets.has_set_bonus( SET_MELEE, T16, B2 ) )
        ab::p() -> buff.feral_tier16_2pc -> trigger();
    }
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
    parse_options( options );
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

    triggers_natures_vigil = false;
  }

  virtual timespan_t execute_time() const override
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
  bool   requires_stealth;
  int    combo_point_gain;
  double base_dd_bonus;
  double base_td_bonus;
  bool   consume_ooc;
  bool   trigger_t17_2p;

  cat_attack_t( const std::string& token, druid_t* p,
                const spell_data_t* s = spell_data_t::nil(),
                const std::string& options = std::string() ) :
    base_t( token, p, s ),
    requires_stealth( false ), combo_point_gain( 0 ),
    base_dd_bonus( 0.0 ), base_td_bonus( 0.0 ), consume_ooc( true ),
    trigger_t17_2p( false )
  {
    parse_options( options );

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
  virtual double cost() const override
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

  virtual bool prowling() const // For effects that specifically trigger only when "prowling."
  {
    // Make sure we call all three methods for accurate benefit tracking.
    bool prowl = p() -> buff.prowl -> up(),
           inc = p() -> buff.incarnation -> up() && p() -> specialization() == DRUID_FERAL;

    return prowl || inc;
  }

  virtual bool stealthed() const // For effects that require any form of stealth.
  {
    // Make sure we call all three methods for accurate benefit tracking.
    bool shadowmeld = p() -> buffs.shadowmeld -> up(),
              prowl = prowling();

    return prowl || shadowmeld;
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

  virtual void execute() override
  {
    base_t::execute();

    if ( this -> base_costs[ RESOURCE_COMBO_POINT ] > 0 )
    {
      if ( player -> sets.has_set_bonus( SET_MELEE, T15, B2 ) &&
          rng().roll( cost() * 0.15 ) )
      {
        p() -> proc.tier15_2pc_melee -> occur();
        p() -> resource_gain( RESOURCE_COMBO_POINT, 1, p() -> gain.feral_tier15_2pc );
      }

      if ( p() -> buff.feral_tier16_4pc -> up() )
      {
        p() -> resource_gain( RESOURCE_COMBO_POINT, p() -> buff.feral_tier16_4pc -> data().effectN( 1 ).base_value(), p() -> gain.feral_tier16_4pc );
        p() -> buff.feral_tier16_4pc -> expire();
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

  virtual void impact( action_state_t* s ) override
  {
    base_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      if ( p() -> spec.predatory_swiftness -> ok() && base_costs[ RESOURCE_COMBO_POINT ] )
        p() -> buff.predatory_swiftness -> trigger( 1, 1, p() -> resources.current[ RESOURCE_COMBO_POINT ] * 0.20 );

      // Only manage for single target generators because AoE generators need special logic.
      if ( combo_point_gain && p() -> spell.primal_fury -> ok() &&
           s -> result == RESULT_CRIT && aoe == 0 )
      {
        p() -> proc.primal_fury -> occur();
        p() -> resource_gain( RESOURCE_COMBO_POINT, p() -> spell.primal_fury -> effectN( 1 ).base_value(), p() -> gain.primal_fury );
      }
    }
  }

  virtual void consume_resource() override
  {
    // Treat Omen of Clarity energy savings like an energy gain for tracking purposes.
    if ( base_t::cost() > 0 && consume_ooc && p() -> buff.omen_of_clarity -> up() )
    {
      // Base cost doesn't factor in Berserk, but Omen of Clarity does net us less energy during it, so account for that here.
      double eff_cost = base_t::cost() * ( 1.0 + p() -> buff.berserk -> check() * p() -> spell.berserk_cat -> effectN( 1 ).percent() );
      p() -> gain.omen_of_clarity -> add( RESOURCE_ENERGY, eff_cost );

      // Feral tier18 4pc occurs before the base cost is consumed.
      if ( p() -> sets.has_set_bonus( DRUID_FERAL, T18, B4 ) )
        p() -> resource_gain( RESOURCE_ENERGY, base_t::cost() * p() -> sets.set( DRUID_FERAL, T18, B4 ) -> effectN( 1 ).percent(), p() -> gain.feral_tier18_4pc );
    }

    base_t::consume_resource();

    if ( base_t::cost() > 0 && consume_ooc )
      p() -> buff.omen_of_clarity -> decrement();

    if ( base_costs[ RESOURCE_COMBO_POINT ] && result_is_hit( execute_state -> result ) )
    {
      int consumed = (int) p() -> resources.current[ RESOURCE_COMBO_POINT ];

      p() -> resource_loss( RESOURCE_COMBO_POINT, consumed, nullptr, this );

      if ( sim -> log )
        sim -> out_log.printf( "%s consumes %d %s for %s (%d)",
                               player -> name(),
                               consumed,
                               util::resource_type_string( RESOURCE_COMBO_POINT ),
                               name(),
                               (int) player -> resources.current[ RESOURCE_COMBO_POINT ] );

      if ( p() -> talent.soul_of_the_forest -> ok() && p() -> specialization() == DRUID_FERAL )
        p() -> resource_gain( RESOURCE_ENERGY,
                              consumed * p() -> talent.soul_of_the_forest -> effectN( 1 ).base_value(),
                              p() -> gain.soul_of_the_forest );
    }
  }

  virtual bool ready() override
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

  virtual double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double pm = base_t::composite_persistent_multiplier( s );

    if ( p() -> buff.cat_form -> check() && dbc::is_school( s -> action -> school, SCHOOL_PHYSICAL ) )
      pm *= 1.0 + p() -> buff.tigers_fury -> check() * p() -> buff.tigers_fury -> default_value; // Avoid using value() to prevent skewing benefit_pct.

    return pm;
  }

  virtual double composite_ta_multiplier( const action_state_t* s ) const override
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

  void tick( dot_t* d ) override
  {
    base_t::tick( d );

    if ( trigger_t17_2p )
      p() -> resource_gain( RESOURCE_ENERGY,
                            p() -> sets.set( DRUID_FERAL, T17, B2 ) -> effectN( 1 ).base_value(),
                            p() -> gain.feral_tier17_2pc );
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

    triggers_natures_vigil = false;
  }

  virtual timespan_t execute_time() const override
  {
    if ( ! player -> in_combat )
      return timespan_t::from_seconds( 0.01 );

    return cat_attack_t::execute_time();
  }

  virtual double action_multiplier() const override
  {
    double cm = cat_attack_t::action_multiplier();

    if ( p() -> buff.cat_form -> check() )
      cm *= 1.0 + p() -> buff.cat_form -> data().effectN( 3 ).percent();

    return cm;
  }
};

// Ferocious Bite ===========================================================

struct ferocious_bite_t : public cat_attack_t
{
  struct glyph_of_ferocious_bite_t : public heal_t
  {
    double energy_spent, energy_divisor;

    glyph_of_ferocious_bite_t( druid_t* p ) :
      heal_t( "glyph_of_ferocious_bite", p, spell_data_t::nil() ),
      energy_spent( 0.0 )
    {
      background = dual = proc = true;
      target = p;
      may_crit = false;
      may_multistrike = 0;
      harmful = false;

      const spell_data_t* glyph = p -> find_glyph_spell( "Glyph of Ferocious Bite" );
      base_pct_heal = glyph -> effectN( 1 ).percent() / 10.0;
      energy_divisor = glyph -> effectN( 2 ).resource( RESOURCE_ENERGY );
    }

    druid_t* p() const
    { return static_cast<druid_t*>( player ); }

    void init() override
    {
      heal_t::init();

      snapshot_flags |= STATE_MUL_DA;
    }

    double action_multiplier() const override
    {
      double am = heal_t::action_multiplier();

      am *= energy_spent / energy_divisor;

      return am;
    }

    void execute() override
    {
      if ( energy_spent <= 0 )
        return;

      heal_t::execute();
    }

    void impact( action_state_t* s ) override
    {
      heal_t::impact( s );

      // Explicitly trigger Nature's Vigil since this isn't in druid_heal_t
      if ( p() -> buff.natures_vigil -> check() )
        p() -> active.natures_vigil -> trigger( s -> result_amount, false );
    }
  };

  double excess_energy;
  double max_excess_energy;
  bool max_energy;
  glyph_of_ferocious_bite_t* glyph_effect;

  ferocious_bite_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "ferocious_bite", p, p -> find_class_spell( "Ferocious Bite" ), "" ),
    excess_energy( 0 ), max_excess_energy( 0 ), max_energy( false )
  {
    add_option( opt_bool( "max_energy" , max_energy ) );
    parse_options( options_str );
    base_costs[ RESOURCE_COMBO_POINT ] = 1;

    max_excess_energy      = -1 * data().effectN( 2 ).base_value();
    special                = true;
    spell_power_mod.direct = 0;

    if ( p -> glyph.ferocious_bite -> ok() )
      glyph_effect = new glyph_of_ferocious_bite_t( p );

  }

  bool ready() override
  {
    if ( max_energy && p() -> resources.current[ RESOURCE_ENERGY ] < p() -> max_fb_energy )
      return false;

    return cat_attack_t::ready();
  }

  void execute() override
  {
    // Berserk does affect the additional energy consumption.
    if ( p() -> buff.berserk -> check() )
      max_excess_energy *= 1.0 + p() -> spell.berserk_cat -> effectN( 1 ).percent();

    excess_energy = std::min( max_excess_energy,
                              ( p() -> resources.current[ RESOURCE_ENERGY ] - cat_attack_t::cost() ) );

    cat_attack_t::execute();

    if ( p() -> buff.feral_tier15_4pc -> up() )
      p() -> buff.feral_tier15_4pc -> decrement();

    max_excess_energy = -1 * data().effectN( 2 ).base_value();
  }

  void impact( action_state_t* state ) override
  {
    cat_attack_t::impact( state );

    if ( result_is_hit( state -> result ) )
    {
      if ( p() -> glyph.ferocious_bite -> ok() )
      {
        // Heal based on the energy spent before Berserk's reduction
        glyph_effect -> energy_spent = ( cost() + excess_energy ) / ( 1.0 + p() -> buff.berserk -> check() * p() -> spell.berserk_cat -> effectN( 1 ).percent() );
        glyph_effect -> execute();
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

  void consume_resource() override
  {
    // Extra energy consumption happens first.
    // In-game it happens before the skill even casts but let's not do that because its dumb.
    if ( result_is_hit( execute_state -> result ) )
    {
      player -> resource_loss( current_resource(), excess_energy );
      stats -> consume_resource( current_resource(), excess_energy );
    }

    cat_attack_t::consume_resource();
  }

  double action_multiplier() const override
  {
    double am = cat_attack_t::action_multiplier();

    am *= p() -> resources.current[ RESOURCE_COMBO_POINT ] / p() -> resources.max[ RESOURCE_COMBO_POINT ];

    am *= 1.0 + excess_energy / max_excess_energy;

    return am;
  }

  double composite_crit() const override
  {
    double c = cat_attack_t::composite_crit();

    if ( p() -> buff.feral_tier15_4pc -> check() )
      c += p() -> buff.feral_tier15_4pc -> data().effectN( 1 ).percent();

    return c;
  }

  double composite_crit_multiplier() const override
  {
    double cm = cat_attack_t::composite_crit_multiplier();

    if ( target -> debuffs.bleeding -> check() )
      cm *= 2.0;

    return cm;
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
  snapshot_counter_t* ir_counter; //Imp Rake counter

  rake_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "rake", p, p -> find_specialization_spell( "Rake" ), options_str ),
    ir_counter( nullptr )
  {
    special = true;
    attack_power_mod.direct = data().effectN( 1 ).ap_coeff();

    bleed_spell           = p -> find_spell( 155722 );
    base_td               = bleed_spell -> effectN( 1 ).base_value();
    attack_power_mod.tick = bleed_spell -> effectN( 1 ).ap_coeff();
    dot_duration          = bleed_spell -> duration();
    base_tick_time        = bleed_spell -> effectN( 1 ).period();

    ir_counter = new snapshot_counter_t( p, p -> buff.prowl );
    ir_counter -> add_buff( p -> buff.incarnation );
    ir_counter -> add_buff( p -> buffs.shadowmeld );

    trigger_t17_2p = p -> sets.has_set_bonus( DRUID_FERAL, T17, B2 );

    // 2015/09/01: Rake deals 20% less damage in PvP combat.
    if ( sim -> pvp_crit )
      base_multiplier *= 0.8;
  }

  virtual double composite_persistent_multiplier( const action_state_t* s ) const override
  {
    double pm = cat_attack_t::composite_persistent_multiplier( s );

    if ( stealthed() )
      pm *= 1.0 + p() -> perk.improved_rake -> effectN( 2 ).percent();

    return pm;
  }

  /* Treat direct damage as "bleed"
     Must use direct damage because tick_zeroes cannot be blocked, and
     this attack can be blocked if the druid is in front of the target. */
  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double dm = cat_attack_t::composite_da_multiplier( state );

    if ( p() -> mastery.razor_claws -> ok() )
      dm *= 1.0 + p() -> cache.mastery_value();

    return dm;
  }

  // Bleed damage penetrates armor.
  virtual double target_armor( player_t* ) const override
  { return 0.0; }

  virtual void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
      p() -> resource_gain( RESOURCE_COMBO_POINT, combo_point_gain, p() -> gain.rake );
  }

  virtual void execute() override
  {
    // 05/05/2015: Triggers regardless of action outcome
    if ( p() -> glyph.savage_roar -> ok() && prowling() )
      trigger_glyph_of_savage_roar();

    // Track Imp Rake
    ir_counter -> count_execute();

    cat_attack_t::execute();

    // Track buff benefits
    if ( p() -> specialization() == DRUID_FERAL )
      p() -> buff.incarnation -> up();
  }

  virtual void tick( dot_t* d ) override
  {
    cat_attack_t::tick( d );

    ir_counter -> count_tick();
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

    void initialize() override
    {
      action_state_t::initialize();

      combo_points = (int) druid -> resources.current[ RESOURCE_COMBO_POINT ];
    }

    void copy_state( const action_state_t* state ) override
    {
      action_state_t::copy_state( state );
      const rip_state_t* rip_state = debug_cast<const rip_state_t*>( state );
      combo_points = rip_state -> combo_points;
    }

    std::ostringstream& debug_str( std::ostringstream& s ) override
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
    dot_duration += player -> sets.set( SET_MELEE, T14, B4 ) -> effectN( 1 ).time_value();

    trigger_t17_2p = p -> sets.has_set_bonus( DRUID_FERAL, T17, B2 );

    // 2015/09/01: Rip deals 20% less damage in PvP combat.
    if ( sim -> pvp_crit )
      base_multiplier *= 0.8;
  }

  action_state_t* new_state() override
  { return new rip_state_t( p(), this, target ); }

  double attack_tick_power_coefficient( const action_state_t* s ) const override
  {
    rip_state_t* rip_state = debug_cast<rip_state_t*>( td( s -> target ) -> dots.rip -> state );
    if ( ! rip_state )
    {
      return 0;
    }

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
    may_multistrike = may_crit = may_miss = harmful = false;
    dot_duration  = timespan_t::zero();
    base_tick_time = timespan_t::zero();

    // Does not consume Omen of Clarity. http://us.battle.net/wow/en/blog/16549669/603-patch-notes-10-27-2014
    consume_ooc = false;
  }

  /* We need a custom implementation of Pandemic refresh mechanics since our ready()
     method relies on having the correct final value. */
  timespan_t duration( int combo_points = -1 )
  {
    if ( combo_points == -1 )
      combo_points = (int) p() -> resources.current[ RESOURCE_COMBO_POINT ];
  
    timespan_t d = data().duration() + timespan_t::from_seconds( 6.0 ) * combo_points;

    // Maximum duration is 130% of the raw duration of the new Savage Roar.
    if ( p() -> buff.savage_roar -> check() )
      d += std::min( p() -> buff.savage_roar -> remains(), d * 0.3 );

    return d;
  }

  virtual void execute() override
  {
    // Grab duration before we go and spend all of our combo points.
    timespan_t d = duration();

    cat_attack_t::execute();

    p() -> buff.savage_roar -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, d );
  }

  virtual bool ready() override
  {
    if ( p() -> glyph.savagery -> ok() )
      return false;
    // Savage Roar may not be cast if the new duration is less than that of the current
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

  virtual void execute() override
  {
    // 05/05/2015: Triggers regardless of action outcome
    if ( p() -> glyph.savage_roar -> ok() && stealthed() )
      trigger_glyph_of_savage_roar();

    cat_attack_t::execute();

    if ( p() -> buff.feral_tier15_4pc -> up() )
      p() -> buff.feral_tier15_4pc -> decrement();

    // Track buff benefits
    if ( p() -> specialization() == DRUID_FERAL )
      p() -> buff.incarnation -> up();
  }

  virtual void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      p() -> resource_gain( RESOURCE_COMBO_POINT, combo_point_gain, p() -> gain.shred );

      if ( s -> result == RESULT_CRIT && p() -> sets.has_set_bonus( DRUID_FERAL, PVP, B4 ) )
        td( s -> target ) -> buffs.bloodletting -> trigger(); // Druid module debuff
    }
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double tm = cat_attack_t::composite_target_multiplier( t );

    if ( t -> debuffs.bleeding -> up() )
      tm *= 1.0 + p() -> spec.swipe -> effectN( 2 ).percent();

    return tm;
  }

  double composite_crit() const override
  {
    double c = cat_attack_t::composite_crit();

    if ( p() -> buff.feral_tier15_4pc -> up() )
      c += p() -> buff.feral_tier15_4pc -> data().effectN( 1 ).percent();

    return c;
  }

  double composite_crit_multiplier() const override
  {
    double cm = cat_attack_t::composite_crit_multiplier();

    if ( stealthed() )
      cm *= 2.0;

    return cm;
  }

  double action_multiplier() const override
  {
    double m = cat_attack_t::action_multiplier();

    if ( p() -> buff.feral_tier16_2pc -> up() )
      m *= 1.0 + p() -> buff.feral_tier16_2pc -> data().effectN( 1 ).percent();

    if ( stealthed() )
    {
      m *= 1.0 + p() -> buff.prowl -> data().effectN( 4 ).percent();
    }

    return m;
  }
};

// Swipe ==============================================================

struct swipe_t : public cat_attack_t
{
private:
  bool attackCritical;
public:
  swipe_t( druid_t* player, const std::string& options_str ) :
    cat_attack_t( "swipe", player, player -> spec.swipe, options_str ),
    attackCritical( false )
  {
    aoe = -1;
    combo_point_gain = data().effectN( 1 ).base_value(); // Effect is not labelled correctly as CP gain
  }

  virtual void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );

    if ( s -> result == RESULT_CRIT )
      attackCritical = true;
  }

  virtual void execute() override
  {
    attackCritical = false;

    cat_attack_t::execute();
    
    if ( attackHit )
    {
      p() -> resource_gain( RESOURCE_COMBO_POINT, combo_point_gain, p() -> gain.swipe );
      if ( attackCritical && p() -> spell.primal_fury -> ok() )
      {
        p() -> proc.primal_fury -> occur();
        p() -> resource_gain( RESOURCE_COMBO_POINT, p() -> spell.primal_fury -> effectN( 1 ).base_value(), p() -> gain.primal_fury );
      }
    }

    p() -> buff.feral_tier16_2pc -> up();

    if ( p() -> buff.feral_tier15_4pc -> up() )
      p() -> buff.feral_tier15_4pc -> decrement();
  }

  double action_multiplier() const override
  {
    double m = cat_attack_t::action_multiplier();

    if ( p() -> buff.feral_tier16_2pc -> check() )
      m *= 1.0 + p() -> buff.feral_tier16_2pc -> data().effectN( 1 ).percent();

    return m;
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double tm = cat_attack_t::composite_target_multiplier( t );

    if ( t -> debuffs.bleeding -> up() )
      tm *= 1.0 + data().effectN( 2 ).percent();

    return tm;
  }

  double composite_crit() const override
  {
    double c = cat_attack_t::composite_crit();

    if ( p() -> buff.feral_tier15_4pc -> check() )
      c += p() -> buff.feral_tier15_4pc -> data().effectN( 1 ).percent();

    return c;
  }
};

// Thrash (Cat) =============================================================

struct thrash_cat_t : public cat_attack_t
{
  thrash_cat_t( druid_t* p, const std::string& options_str ) :
    cat_attack_t( "thrash_cat", p, p -> find_spell( 106830 ), options_str )
  {
    aoe                    = -1;
    spell_power_mod.direct = 0;

    trigger_t17_2p = p -> sets.has_set_bonus( DRUID_FERAL, T17, B2 );
  }

  // Treat direct damage as "bleed"
  double composite_da_multiplier( const action_state_t* state ) const override
  {
    double m = cat_attack_t::composite_da_multiplier( state );

    if ( p() -> mastery.razor_claws -> ok() )
      m *= 1.0 + p() -> cache.mastery_value();

    return m;
  }

  virtual void impact( action_state_t* s ) override
  {
    cat_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
      this -> td( s -> target ) -> dots.thrash_bear -> cancel();
  }

  // Treat direct damage as "bleed"
  virtual double target_armor( player_t* ) const override
  { return 0.0; }
};

// Flurry of Xuen (Fen-yu Legendary Cloak proc) =============================

struct flurry_of_xuen_t : public cat_attack_t
{
  flurry_of_xuen_t( druid_t* p ) :
    cat_attack_t( "flurry_of_xuen", p, p -> find_spell( 147891 ) )
  {
    special = may_miss = may_parry = may_block = may_dodge = may_crit = background = true;
    proc = false;
    aoe = 5;
  }
};

// Mark of the Shattered Hand ===============================================

struct shattered_bleed_t : public cat_attack_t
{
  shattered_bleed_t( druid_t* p ):
    cat_attack_t( "shattered_bleed", p, p -> find_spell( 159238 ) )
    {
      hasted_ticks = false; background = true; callbacks = false; special = true;
      may_miss = may_block = may_dodge = may_parry = false; may_crit = true;
      tick_may_crit = false;
    }

    void init() override
    {
      cat_attack_t::init();

      snapshot_flags |= STATE_MUL_TA;
    }

    double target_armor( player_t* ) const override
    { return 0.0; }

    timespan_t calculate_dot_refresh_duration( const dot_t* dot, timespan_t triggered_duration ) const override
    {
      timespan_t new_duration = std::min( triggered_duration * 0.3, dot -> remains() ) + triggered_duration;
      timespan_t period_mod = new_duration % base_tick_time;
      new_duration += ( base_tick_time - period_mod );

      return new_duration;
    }

    double action_multiplier() const override
    {
      double am = cat_attack_t::action_multiplier();

      // Benefits dynamically from Savage Roar and Tiger's Fury.
      am *= 1.0 + p() -> buff.savage_roar -> value();
      am *= 1.0 + p() -> buff.tigers_fury -> value();

      return am;
    }
    
    // Override to prevent benefitting from mastery.
    double composite_ta_multiplier( const action_state_t* /*s*/ ) const override
    { return action_multiplier(); }
    
    // Benefit from Savage Roar and Tiger's Fury is not persistent.
    double composite_persistent_multiplier( const action_state_t* /*s*/ ) const override
    { return 1.0; }
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
    base_t( n, p, s ), rage_amount( 0.0 ), 
    rage_tick_amount( 0.0 ),  rage_gain( p -> get_gain( name() ) )
  {}

  virtual timespan_t gcd() const override
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

  virtual double cooldown_reduction() const override
  {
    double cdr = base_t::cooldown_reduction();

    cdr *= p() -> cache.attack_haste();

    return cdr;
  }

  virtual void impact( action_state_t* s ) override
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

  virtual void tick( dot_t* d ) override
  {
    base_t::tick( d );

    p() -> resource_gain( RESOURCE_RAGE, rage_tick_amount, rage_gain );
  }

  virtual bool ready() override 
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
    triggers_natures_vigil = false;
  }

  virtual timespan_t execute_time() const override
  {
    if ( ! player -> in_combat )
      return timespan_t::from_seconds( 0.01 );

    return bear_attack_t::execute_time();
  }

  virtual void impact( action_state_t* state ) override
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
    parse_options( options_str );
    rage_amount = data().effectN( 3 ).resource( RESOURCE_RAGE );
  }

  virtual void impact( action_state_t* state ) override
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

  // Treat direct damage as "bleed"
  virtual double target_armor( player_t* ) const override
  { return 0.0; }

  virtual double composite_target_ta_multiplier( player_t* t ) const override
  {
    double tm = bear_attack_t::composite_target_ta_multiplier( t );

    tm *= td( t ) -> lacerate_stack;

    return tm;
  }

  virtual void multistrike_tick( const action_state_t* src_state, action_state_t* ms_state, double multiplier ) override
  {
    bear_attack_t::multistrike_tick( src_state, ms_state, multiplier );

    trigger_ursa_major();
  }

  virtual void last_tick( dot_t* d ) override
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
    parse_options( options_str );

    rage_amount = data().effectN( 3 ).resource( RESOURCE_RAGE );

    if ( p() -> specialization() == DRUID_GUARDIAN )
    {
      rage_amount += player -> talent.soul_of_the_forest -> effectN( 1 ).resource( RESOURCE_RAGE );
      base_multiplier *= 1.0 + player -> talent.soul_of_the_forest -> effectN( 2 ).percent();
      base_crit += p() -> talent.dream_of_cenarius -> effectN( 3 ).percent();
    }
  }

  void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.berserk -> check() || ( p() -> buff.incarnation -> check() && p() -> specialization() == DRUID_GUARDIAN ) )
      cd = timespan_t::zero();

    bear_attack_t::update_ready( cd );
  }

  virtual void execute() override
  {
    int base_aoe = aoe;
    if ( p() -> buff.berserk -> up() )
      aoe = p() -> spell.berserk_bear -> effectN( 1 ).base_value();

    bear_attack_t::execute();

    aoe = base_aoe;
  }

  virtual void impact( action_state_t* s ) override
  {
    bear_attack_t::impact( s );

    if ( result_is_multistrike( s -> result ) )
      trigger_ursa_major();
    if ( p() -> talent.dream_of_cenarius -> ok() && s -> result == RESULT_CRIT )
      p() -> buff.dream_of_cenarius -> trigger();
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
      stats = p -> get_stats( "tooth_and_claw_absorb" );

      // Coeff is in tooltip but not spell data, so hardcode the value here.
      attack_power_mod.direct = 2.40;
    }

    virtual void impact( action_state_t* s ) override
    {
      if ( p() -> sets.has_set_bonus( DRUID_GUARDIAN, T17, B4 ) )
        p() -> buff.guardian_tier17_4pc -> increment();

      p() -> buff.tooth_and_claw_absorb -> trigger( 1, s -> result_amount );
    }
  };

  tooth_and_claw_t* absorb;
  double cost_reduction;

  maul_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "maul", player, player -> find_specialization_spell( "Maul" ) ),
    cost_reduction( 0.0 )
  {
    parse_options( options_str );
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

    normalize_weapon_speed = false;
  }

  virtual double cost() const override
  {
    double c = bear_attack_t::cost();

    if ( p() -> buff.tooth_and_claw -> check() && p() -> sets.has_set_bonus( DRUID_GUARDIAN, T17, B2 ) )
      c -= cost_reduction;

    return c;
  }

  virtual double composite_target_multiplier( player_t* t ) const override
  {
    double tm = bear_attack_t::composite_target_multiplier( t );

    if ( t -> debuffs.bleeding -> up() )
      tm *= 1.0 + data().effectN( 3 ).percent();

    return tm;
  }

  virtual void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.incarnation -> check() && p() -> specialization() == DRUID_GUARDIAN )
      cd = timespan_t::zero();

    bear_attack_t::update_ready( cd );
  }

  virtual void impact( action_state_t* s ) override
  {
    bear_attack_t::impact( s );

    if ( result_is_hit( s -> result ) && p() -> buff.tooth_and_claw -> check() )
    {
      if ( p() -> sets.has_set_bonus( DRUID_GUARDIAN, T17, B2 ) )
      {
        // Treat the savings like a rage gain for tracking purposes.
        p() -> gain.guardian_tier17_2pc -> add( RESOURCE_RAGE, cost_reduction );
      }
      absorb -> execute();
    }
  }

  virtual void execute() override
  {
    // Benefit tracking
    p() -> buff.tooth_and_claw -> up();

    bear_attack_t::execute();

    p() -> buff.tooth_and_claw -> decrement();
  }
};

// Pulverize ================================================================

struct pulverize_t : public bear_attack_t
{
  pulverize_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "pulverize", player, player -> talent.pulverize )
  {
    parse_options( options_str );

    normalize_weapon_speed = false;
  }

  virtual void impact( action_state_t* s ) override
  {
    bear_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
    {
      // consumes 3 stacks of Lacerate on the target
      target -> get_dot( "lacerate", p() ) -> cancel();

      // and reduce damage taken by x% for y sec (pandemics)
      p() -> buff.pulverize -> trigger();
    }
  }

  virtual bool ready() override
  {
    // Call bear_attack_t::ready() first for proper targeting support.
    if ( bear_attack_t::ready() && td( target ) -> lacerate_stack >= 3 )
      return true;
    else
      return false;
  }
};

// Thrash (Bear) ============================================================

struct thrash_bear_t : public bear_attack_t
{
  thrash_bear_t( druid_t* player, const std::string& options_str ) :
    bear_attack_t( "thrash_bear", player, player -> find_spell( 77758 ) )
  {
    parse_options( options_str );
    aoe                    = -1;
    spell_power_mod.direct = 0;

    // Apply hidden passive damage multiplier
    base_dd_multiplier *= 1.0 + player -> spec.guardian_passive -> effectN( 6 ).percent();
    base_td_multiplier *= 1.0 + player -> spec.guardian_passive -> effectN( 7 ).percent();

    rage_amount = rage_tick_amount = p() -> find_spell( 158723 ) -> effectN( 1 ).resource( RESOURCE_RAGE );
  }

  virtual void impact( action_state_t* s ) override
  {
    bear_attack_t::impact( s );

    if ( result_is_hit( s -> result ) )
      this -> td( s -> target ) -> dots.thrash_cat -> cancel();
  }

  // Treat direct damage as "bleed"
  virtual double target_armor( player_t* ) const override
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

  druid_spell_base_t( const std::string& n, druid_t* player,
                      const spell_data_t* s = spell_data_t::nil() ) :
    ab( n, player, s )
  {}

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
  bool target_self;

  druid_heal_t( const std::string& token, druid_t* p,
                const spell_data_t* s = spell_data_t::nil(),
                const std::string& options_str = std::string() ) :
    base_t( token, p, s ),
    living_seed( nullptr ),
    target_self( 0 )
  {
    add_option( opt_bool( "target_self", target_self ) );
    parse_options( options_str );

    if( target_self )
      target = p;

    may_miss          = false;
    weapon_multiplier = 0;
    harmful           = false;
  }
    
protected:
  void init_living_seed();

public:
  virtual double cost() const override
  {
    if ( p() -> buff.heart_of_the_wild -> heals_are_free() && current_resource() == RESOURCE_MANA )
      return 0;

    return base_t::cost();
  }

  virtual void execute() override
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

    if ( p() -> mastery.harmony -> ok() && spell_power_mod.direct > 0 && ! background )
      p() -> buff.harmony -> trigger( 1, p() -> mastery.harmony -> ok() ? p() -> cache.mastery_value() : 0.0 );
  }

  virtual timespan_t execute_time() const override
  {
    if ( p() -> buff.natures_swiftness -> check() )
      return timespan_t::zero();

    return base_t::execute_time();
  }

  virtual double composite_haste() const override
  {
    double h = base_t::composite_haste();

    h *= 1.0 / ( 1.0 + p() -> buff.soul_of_the_forest -> value() );

    return h;
  }

  virtual double action_da_multiplier() const override
  {
    double adm = base_t::action_da_multiplier();

    if ( p() -> buff.incarnation -> up() && p() -> specialization() == DRUID_RESTORATION )
      adm *= 1.0 + p() -> buff.incarnation -> data().effectN( 1 ).percent();

    if ( p() -> buff.natures_swiftness -> check() && base_execute_time > timespan_t::zero() )
      adm *= 1.0 + p() -> buff.natures_swiftness -> data().effectN( 2 ).percent();

    if ( p() -> mastery.harmony -> ok() )
      adm *= 1.0 + p() -> cache.mastery_value();

    return adm;
  }

  virtual double action_ta_multiplier() const override
  {
    double adm = base_t::action_ta_multiplier();

    if ( p() -> buff.incarnation -> up() && p() -> specialization() == DRUID_RESTORATION )
      adm += p() -> buff.incarnation -> data().effectN( 2 ).percent();

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

  double composite_da_multiplier( const action_state_t* ) const override
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
private:
  double max_rage_cost() const
  {
    if ( p() -> buff.savage_defense -> check() )
      return base_max_rage_cost * ( 1.0 - p() -> sets.set( DRUID_GUARDIAN, T18, B4 ) -> effectN( 1 ).percent() );
    else
      return base_max_rage_cost;
  }
public:
  double base_max_rage_cost;

  frenzied_regeneration_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "frenzied_regeneration", p, p -> find_class_spell( "Frenzied Regeneration" ), options_str ),
    base_max_rage_cost( 0.0 )
  {
    parse_options( options_str );
    use_off_gcd = true;
    may_crit = false;
    may_multistrike = 0;
    target = p;

    base_max_rage_cost = data().effectN( 2 ).base_value();

    triggers_natures_vigil = false;

    if ( p -> sets.has_set_bonus( SET_TANK, T16, B2 ) )
      p -> active.ursocs_vigor = new ursocs_vigor_t( p );
  }

  virtual double cost() const override
  {
    const_cast<frenzied_regeneration_t*>(this) -> base_costs[ RESOURCE_RAGE ] =
      std::min( p() -> resources.current[ RESOURCE_RAGE ], max_rage_cost() );

    return druid_heal_t::cost();
  }

  virtual double action_multiplier() const override
  {
    double am = druid_heal_t::action_multiplier();

    am *= cost() / max_rage_cost();
    am *= 1.0 + p() -> buff.guardian_tier17_4pc -> default_value * p() -> buff.guardian_tier17_4pc -> current_stack;

    return am;
  }

  virtual void execute() override
  {
    // Benefit tracking
    p() -> buff.guardian_tier17_4pc -> up();

    druid_heal_t::execute();

    p() -> buff.guardian_tier15_2pc -> expire();
    p() -> buff.guardian_tier17_4pc -> expire();

    if ( p() -> sets.has_set_bonus( SET_TANK, T16, B4 ) )
      p() -> active.ursocs_vigor -> trigger_hot( resource_consumed );
  }

  virtual bool ready() override
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
    init_living_seed();
    ignore_false_positive = true; // Prevents cat/bear from failing a skill check and going into caster form.

    // redirect to self if not specified
    if ( target -> is_enemy() || ( target -> type == HEALING_ENEMY && p -> specialization() == DRUID_GUARDIAN ) )
      target = p;
    
    if ( p -> talent.dream_of_cenarius -> ok() && p -> specialization() == DRUID_GUARDIAN )
      attack_power_mod.direct = spell_power_mod.direct;
  }

  double spell_direct_power_coefficient( const action_state_t* /* state */ ) const override
  {
    return spell_power_mod.direct * ! p() -> buff.dream_of_cenarius -> check();
  }

  double attack_direct_power_coefficient( const action_state_t* /* state */ ) const override
  {
    return attack_power_mod.direct * p() -> buff.dream_of_cenarius -> check();
  }

  virtual double cost() const override
  {
    if ( p() -> buff.predatory_swiftness -> check() )
      return 0;

    if ( p() -> buff.natures_swiftness -> check() )
      return 0;

    if ( p() -> buff.dream_of_cenarius -> check() )
      return 0;

    return druid_heal_t::cost();
  }

  virtual void consume_resource() override
  {
    // Prevent from consuming Omen of Clarity unnecessarily
    if ( p() -> buff.predatory_swiftness -> check() )
      return;

    if ( p() -> buff.natures_swiftness -> check() )
      return;

    druid_heal_t::consume_resource();
  }

  virtual double action_da_multiplier() const override
  {
    double adm = base_t::action_da_multiplier();

    if ( p() -> talent.dream_of_cenarius -> ok() ) {
      if ( p() -> specialization() == DRUID_FERAL || p() -> specialization() == DRUID_BALANCE )
        adm *= 1.0 + p() -> talent.dream_of_cenarius -> effectN( 1 ).percent();
      else if ( p() -> specialization() == DRUID_GUARDIAN )
        adm *= 1.0 + p() -> talent.dream_of_cenarius -> effectN( 2 ).percent();
    }

    if ( p() -> spell.moonkin_form -> ok() && target == p() )
      adm *= 1.0 + 0.50; // Not in spell data

    return adm;
  }

  virtual timespan_t execute_time() const override
  {
    if ( p() -> buff.predatory_swiftness -> check() || p() -> buff.dream_of_cenarius -> up() )
      return timespan_t::zero();

    return druid_heal_t::execute_time();
  }

  virtual void impact( action_state_t* state ) override
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

  virtual void execute() override
  {
    druid_heal_t::execute();

    if ( p() -> talent.bloodtalons -> ok() )
      p() -> buff.bloodtalons -> trigger( 2 );

    if ( p() -> buff.dream_of_cenarius -> up() )
      p() -> cooldown.starfallsurge -> reset( false );

    p() -> buff.predatory_swiftness -> expire();
    p() -> buff.dream_of_cenarius -> expire();
  }

  virtual void schedule_execute( action_state_t* state = nullptr ) override
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

  virtual timespan_t gcd() const override
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
  }

  virtual double composite_target_multiplier( player_t* target ) const override
  {
    double ctm = druid_heal_t::composite_target_multiplier( target );

    ctm *= td( target ) -> buffs.lifebloom -> check();

    return ctm;
  }

  virtual double composite_da_multiplier( const action_state_t* state ) const override
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
    ignore_false_positive = true;
  }

  virtual void impact( action_state_t* state ) override
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

  virtual void last_tick( dot_t* d ) override
  {
    if ( ! d -> state -> target -> is_sleeping() ) // Prevent crash at end of simulation
      bloom -> execute();
    td( d -> state -> target ) -> buffs.lifebloom -> expire();

    druid_heal_t::last_tick( d );
  }

  virtual void tick( dot_t* d ) override
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
    base_crit += 0.60;

    if ( p -> glyph.regrowth -> ok() )
    {
      base_crit += p -> glyph.regrowth -> effectN( 1 ).percent();
      dot_duration  = timespan_t::zero();
    }

    ignore_false_positive = true;
    init_living_seed();
  }

  virtual double cost() const override
  {
    if ( p() -> buff.omen_of_clarity -> check() )
      return 0;

    return druid_heal_t::cost();
  }

  virtual void consume_resource() override
  {
    druid_heal_t::consume_resource();
    double c = druid_heal_t::cost();

    if ( c > 0 && p() -> buff.omen_of_clarity -> up() )
    {
      // Treat the savings like a mana gain for tracking purposes.
      p() -> gain.omen_of_clarity -> add( RESOURCE_MANA, c );
      p() -> buff.omen_of_clarity -> expire();
    }
  }

  virtual void impact( action_state_t* state ) override
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

  virtual timespan_t execute_time() const override
  {
    if ( p() -> buff.incarnation -> check() && p() -> specialization() == DRUID_RESTORATION )
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
    ignore_false_positive = true; // Prevents cat/bear from failing a skill check and going into caster form.
  }

  virtual void execute() override
  {
    druid_heal_t::execute(); 
    p() -> active_rejuvenations += 1 ;
  }

  virtual void schedule_execute( action_state_t* state = nullptr ) override
  {
    druid_heal_t::schedule_execute( state );

    if ( ! p() -> perk.enhanced_rejuvenation -> ok() )
      p() -> buff.cat_form  -> expire();
    if ( ! p() -> buff.heart_of_the_wild -> check() )
      p() -> buff.bear_form -> expire();
  }

  virtual double action_ta_multiplier() const override
  {
    double atm = base_t::action_ta_multiplier();

    if ( p() -> talent.dream_of_cenarius -> ok() && p() -> specialization() == DRUID_FERAL )
        atm *= 1.0 + p() -> talent.dream_of_cenarius -> effectN( 2 ).percent();

    return atm;
  }

  virtual void last_tick( dot_t* d ) override
  {
    p() -> active_rejuvenations -= 1;
    druid_heal_t::last_tick( d );
  }

};

// Renewal ============================================================

struct renewal_t : public druid_heal_t
{
  renewal_t( druid_t* p, const std::string& options_str ) :
    druid_heal_t( "renewal", p, p -> find_talent_spell( "Renewal" ), options_str )
  {
    may_crit = false;
    may_multistrike = 0;
  }

  virtual void init() override
  {
    druid_heal_t::init();

    snapshot_flags &= ~STATE_RESOLVE; // Is not affected by resolve.
  }

  virtual void execute() override
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
    init_living_seed();
  }

  virtual void impact( action_state_t* state ) override
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

  virtual bool ready() override
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
    ignore_false_positive = true;
  }

  virtual void execute() override
  {
    int save = aoe;
    if ( p() -> buff.incarnation -> check() && p() -> specialization() == DRUID_RESTORATION )
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
    parse_options( options );
  }

  virtual void execute() override
  {
    // Adjust buffs and cooldowns if we're in precombat.
    if ( ! p() -> in_combat )
    {
      if ( p() -> buff.incarnation -> check() )
      {
        timespan_t time = std::max( std::max( min_gcd, trigger_gcd * composite_haste() ), base_execute_time * composite_haste() );
        p() -> buff.incarnation -> extend_duration( p(), -time );
        p() -> get_cooldown( "incarnation" ) -> adjust( -time );
      }
    }

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

  virtual double action_multiplier() const override
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

  virtual double cost() const override
  {
    if ( harmful && p() -> buff.heart_of_the_wild -> damage_spells_are_free() )
      return 0;

    return base_t::cost();
  }

  virtual bool ready() override
  {
    if ( p() -> specialization() == DRUID_BALANCE )
      p() -> balance_tracker();

    return base_t::ready();
  }

  virtual void trigger_balance_t18_2pc()
  {
    if ( ! p() -> balance_t18_2pc.trigger() )
      return;

    for ( pet_t* pet : p() -> pet_fey_moonwing )
    {
      if ( pet -> is_sleeping() )
      {
        pet -> summon( timespan_t::from_seconds( 30 ) );
        return;
      }
    }
  }
}; // end druid_spell_t

// Auto Attack ==============================================================

struct auto_attack_t : public melee_attack_t
{
  auto_attack_t( druid_t* player, const std::string& options_str ) :
    melee_attack_t( "auto_attack", player, spell_data_t::nil() )
  {
    parse_options( options_str );

    trigger_gcd = timespan_t::zero();
    ignore_false_positive = true;
    use_off_gcd = true;
  }

  virtual void execute() override
  {
    player -> main_hand_attack -> weapon = &( player -> main_hand_weapon );
    player -> main_hand_attack -> base_execute_time = player -> main_hand_weapon.swing_time;
    player -> main_hand_attack -> schedule_execute();
  }

  virtual bool ready() override
  {
    if ( player -> is_moving() )
      return false;

    if ( ! player -> main_hand_attack )
      return false;

    return( player -> main_hand_attack -> execute_event == nullptr ); // not swinging
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

  double composite_haste() const override
  {
    return 1.0;
  }

  void execute() override
  {
    druid_spell_t::execute(); // Do not move the buff trigger in front of this.
    p() -> buff.astral_communion -> trigger();
  }

  void tick( dot_t* d ) override
  {
    druid_spell_t::tick( d );
    if ( p() -> specialization() == DRUID_BALANCE )
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

  void last_tick( dot_t* d ) override
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

  void execute() override
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
    ignore_false_positive = true;

    if ( ! player -> bear_melee_attack )
    {
      player -> init_beast_weapon( player -> bear_weapon, 2.5 );
      player -> bear_melee_attack = new bear_attacks::bear_melee_t( player );
    }

    base_costs[ RESOURCE_MANA ] *= 1.0 + p() -> glyph.master_shapeshifter -> effectN( 1 ).percent();
  }

  void execute() override
  {
    spell_t::execute();

    p() -> buff.bear_form -> start();
  }

  bool ready() override
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

  void execute() override
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

    if ( p() -> sets.has_set_bonus( DRUID_FERAL, T17, B4 ) )
      p() -> buff.feral_tier17_4pc -> trigger();
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

  void execute() override
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
    ignore_false_positive = true;

    if ( ! player -> cat_melee_attack )
    {
      player -> init_beast_weapon( player -> cat_weapon, 1.0 );
      player -> cat_melee_attack = new cat_attacks::cat_melee_t( player );
    }

    base_costs[ RESOURCE_MANA ] *= 1.0 + p() -> glyph.master_shapeshifter -> effectN( 1 ).percent();
  }

  void execute() override
  {
    spell_t::execute();

    p() -> buff.cat_form -> start();
  }

  bool ready() override
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
    parse_options( options_str );
    cooldown = player -> cooldown.celestial_alignment;
    harmful = false;
    dot_duration = timespan_t::zero();
  }

  void execute() override
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
    harmful = false;
  }

  void execute() override
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
    parse_options( options_str );

    harmful = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.dash -> trigger();
  }
};

// Displacer Beast ==============================================================

struct displacer_beast_t : public druid_spell_t
{
  displacer_beast_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "displacer_beast", p, p -> talent.displacer_beast )
  {
    parse_options( options_str );
    harmful = may_crit = may_miss = false;
    ignore_false_positive = true;
    base_teleport_distance = radius;
    movement_directionality = MOVEMENT_OMNI;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.cat_form -> trigger();
    p() -> buff.displacer_beast -> trigger();
  }
};

// Faerie Fire Spell ========================================================

struct faerie_fire_t : public druid_spell_t
{
  faerie_fire_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "faerie_fire", player, player -> find_class_spell( "Faerie Fire" ) )
  {
    parse_options( options_str );
    cooldown -> duration = timespan_t::from_seconds( 6.0 );
  }

  void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown -> duration;

    if ( ! ( ( p() -> buff.bear_form -> check() && ! p() -> perk.enhanced_faerie_fire -> ok() &&
               ! p() -> buff.incarnation -> check() && p() -> specialization() == DRUID_GUARDIAN )
            || p() -> buff.cat_form -> check() ) )
      cd = timespan_t::zero();

    druid_spell_t::update_ready( cd );
  }

  double action_multiplier() const override
  {
    double am = druid_spell_t::action_multiplier();

    if ( p() -> buff.bear_form -> check() )
      am *= 1.0 + p() -> perk.enhanced_faerie_fire -> effectN( 2 ).percent();
    else
      return 0.0;
    
    return am;
  }

  resource_e current_resource() const override
  {
    if ( p() -> buff.bear_form -> check() )
      return RESOURCE_RAGE;
    else if ( p() -> buff.cat_form -> check() )
      return RESOURCE_ENERGY;

    return RESOURCE_MANA;
  }
};

// Force of Nature Spell ====================================================

struct force_of_nature_spell_t : public druid_spell_t
{
  force_of_nature_spell_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "force_of_nature", player, player -> talent.force_of_nature )
  {
    parse_options( options_str );

    harmful = false;
    cooldown -> charges = 3;
    cooldown -> duration = timespan_t::from_seconds( 20.0 );
  }

  void execute() override
  {
    druid_spell_t::execute();
    if ( p() -> pet_force_of_nature[0] )
    {
      for ( pet_t* pet : p() -> pet_force_of_nature )
      {
        if ( pet -> is_sleeping() )
        {
          pet -> summon( p() -> talent.force_of_nature -> duration() );
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
    parse_options( options_str );
    ignore_false_positive = true;
    may_crit = false;
    use_off_gcd = true;
  }

  void update_ready( timespan_t ) override
  {
    timespan_t cd = cooldown -> duration;

    if ( p() -> buff.incarnation -> check() && p() -> specialization() == DRUID_GUARDIAN )
      cd = timespan_t::zero();

    druid_spell_t::update_ready( cd );
  }

  void impact( action_state_t* s ) override
  {
    if ( s -> target -> is_enemy() )
      target -> taunt( player );

    druid_spell_t::impact( s );
  }

  bool ready() override
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
    parse_options( options_str );
    harmful = may_hit = may_crit = false;
  }

  void execute() override
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
    ground_aoe = true;
  }
};

struct hurricane_t : public druid_spell_t
{
  hurricane_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "hurricane", player, player -> find_spell( 16914 ) )
  {
    parse_options( options_str );
    channeled = true;

    tick_action = new hurricane_tick_t( player, data().effectN( 3 ).trigger() );
    dynamic_tick_action = true;
    ignore_false_positive = true;
  }

  double action_multiplier() const override
  {
    double m = druid_spell_t::action_multiplier();

    m *= 1.0 + p() -> buff.heart_of_the_wild -> damage_spell_multiplier();

    return m;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.hurricane -> trigger();

  }

  void last_tick( dot_t * d ) override
  {
    druid_spell_t::last_tick( d );
    p() -> buff.hurricane -> expire();
  }

  void schedule_execute( action_state_t* state = nullptr ) override
  {
    druid_spell_t::schedule_execute( state );

    p() -> buff.cat_form  -> expire();
    p() -> buff.bear_form -> expire();
  }
};

// Incarnation ==============================================================

struct incarnation_t : public druid_spell_t
{
  incarnation_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "incarnation", p,
      p -> specialization() == DRUID_BALANCE     ? p -> talent.incarnation_chosen :
      p -> specialization() == DRUID_FERAL       ? p -> talent.incarnation_king   :
      p -> specialization() == DRUID_GUARDIAN    ? p -> talent.incarnation_son    :
      p -> specialization() == DRUID_RESTORATION ? p -> talent.incarnation_tree   :
      spell_data_t::nil()
    )
  {
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.incarnation -> trigger();

    if ( ! p() -> in_combat )
    {
      timespan_t time = std::max( min_gcd, trigger_gcd * composite_haste() );
      p() -> buff.incarnation -> extend_duration( p(), -time );
      cooldown -> adjust( -time );
    }

    if ( p() -> specialization() == DRUID_GUARDIAN )
    {
      p() -> cooldown.mangle -> reset( false );
      p() -> cooldown.growl  -> reset( false );
      p() -> cooldown.maul   -> reset( false );
      if ( ! p() -> perk.enhanced_faerie_fire -> ok() )
        p() -> cooldown.faerie_fire -> reset( false ); 
    }
  }
};

// Mark of the Wild Spell ===================================================

struct mark_of_the_wild_t : public druid_spell_t
{
  mark_of_the_wild_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "mark_of_the_wild", player, player -> find_class_spell( "Mark of the Wild" )  )
  {
    parse_options( options_str );

    trigger_gcd = timespan_t::zero();
    harmful     = false;
    ignore_false_positive = true;
    background  = ( sim -> overrides.str_agi_int != 0 && sim -> overrides.versatility != 0 );
  }

  void execute() override
  {
    druid_spell_t::execute();

    if ( sim -> log ) sim -> out_log.printf( "%s performs %s", player -> name(), name() );

    if ( ! sim -> overrides.str_agi_int )
      sim -> auras.str_agi_int -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, dbc::find_spell( player, 79060 ) -> duration() );

    if ( ! sim -> overrides.versatility )
      sim -> auras.versatility -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, dbc::find_spell( player, 79060 ) -> duration() );
  }
};

// Sunfire Base Spell ===========================================================

struct sunfire_base_t: public druid_spell_t
{
  sunfire_base_t( druid_t* player ):
    druid_spell_t( "sunfire", player, player -> find_spell( 93402 ) )
  {
    const spell_data_t* dmg_spell = player -> find_spell( 164815 );

    dot_duration           = dmg_spell -> duration();
    dot_duration          += player -> sets.set( SET_CASTER, T14, B4 ) -> effectN( 1 ).time_value();
    base_tick_time         = dmg_spell -> effectN( 2 ).period();
    spell_power_mod.direct = dmg_spell-> effectN( 1 ).sp_coeff();
    spell_power_mod.tick   = dmg_spell-> effectN( 2 ).sp_coeff();

    base_td_multiplier *= 1.0 + player -> talent.balance_of_power -> effectN( 3 ).percent();

    if ( player -> spec.astral_showers -> ok() )
      aoe = -1;
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  {
    if ( s -> target != s -> action -> target || p() -> stellar_flare_cast )
      return 0; // Sunfire will not deal direct damage to the targets that the dot is spread to.

    return druid_spell_t::spell_direct_power_coefficient( s );
  }

  double action_multiplier() const override
  {
    double am = druid_spell_t::action_multiplier();

    if ( p() -> buff.solar_peak -> up() )
      am *= 1.0 + p() -> buff.solar_peak -> data().effectN( 1 ).percent();

    return am;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> last_target_dot_moonkin = execute_state -> target;

    if ( ! p() -> stellar_flare_cast )
      p() -> buff.solar_peak -> expire();
  }

  void tick( dot_t* d ) override
  {
    druid_spell_t::tick( d );

    if ( result_is_hit( d -> state -> result ) )
      p() -> trigger_shooting_stars( d -> state );

    if ( p() -> sets.has_set_bonus( DRUID_BALANCE, T18, B2 ) )
      trigger_balance_t18_2pc();
  }

  bool ready() override
  {
    if ( p() -> eclipse_amount >= 0 )
      return false;

    return druid_spell_t::ready();
  }
};

// Moonfire Base Spell ===============================================================

struct moonfire_base_t : public druid_spell_t
{
  moonfire_base_t( druid_t* player ) :
    druid_spell_t( "moonfire", player, player -> find_spell( 8921 ) )
  {
    const spell_data_t* dmg_spell = player -> find_spell( 164812 );

    dot_duration                  = dmg_spell -> duration(); 
    dot_duration                 *= 1 + player -> spec.astral_showers -> effectN( 1 ).percent();
    dot_duration                 += player -> sets.set( SET_CASTER, T14, B4 ) -> effectN( 1 ).time_value();
    base_tick_time                = dmg_spell -> effectN( 2 ).period();
    spell_power_mod.tick          = dmg_spell -> effectN( 2 ).sp_coeff();
    spell_power_mod.direct        = dmg_spell -> effectN( 1 ).sp_coeff();

    base_td_multiplier           *= 1.0 + player -> talent.balance_of_power -> effectN( 3 ).percent();
  }

  double spell_direct_power_coefficient( const action_state_t* s ) const override
  {
    if ( p() -> stellar_flare_cast )
      return 0; // With this proposed stellar flare change, it does not deal direct damage.

    return druid_spell_t::spell_direct_power_coefficient( s );
  }

  void tick( dot_t* d ) override
  {
    druid_spell_t::tick( d );

    if ( result_is_hit( d -> state -> result ) )
      p() -> trigger_shooting_stars( d -> state );

    if ( p() -> sets.has_set_bonus( DRUID_BALANCE, T18, B2 ) )
      trigger_balance_t18_2pc();
  }

  double action_multiplier() const override
  {
    double am = druid_spell_t::action_multiplier();

    if ( p() -> buff.lunar_peak -> up() )
      am *= 1.0 + p() -> buff.lunar_peak -> data().effectN( 1 ).percent();

    return am;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> last_target_dot_moonkin = execute_state -> target;

    if ( ! p() -> stellar_flare_cast )
      p() -> buff.lunar_peak -> expire();
  }

  void schedule_execute( action_state_t* state = nullptr ) override
  {
    druid_spell_t::schedule_execute( state );

    p() -> buff.bear_form -> expire();
    p() -> buff.cat_form -> expire();
  }

  bool ready() override
  {
    if ( p() -> eclipse_amount < 0 )
      return false;

    return druid_spell_t::ready();
  }
};

// Moonfire & Sunfire "parent" spells ================================================

struct moonfire_t : public moonfire_base_t
{
  sunfire_base_t* sunfire;

  moonfire_t( druid_t* player, const std::string& options_str ) :
    moonfire_base_t( player )
  {
    parse_options( options_str );

    if ( player -> specialization() == DRUID_BALANCE )
    {
      sunfire = new sunfire_base_t( player );
      sunfire -> callbacks = false;
      sunfire -> dual = sunfire -> background = true;
    }
  }

  // Execute CA Sunfire in update_ready() to assure the correct timing.
  // It must occur after Moonfire has impacted but before callbacks occur.
  virtual void update_ready( timespan_adl_barrier::timespan_t cd_duration ) override
  {
    moonfire_base_t::update_ready( cd_duration );

    if ( p() -> buff.celestial_alignment -> up() )
    {
      sunfire -> target = execute_state -> target;
      sunfire -> execute();
    }
  }
};

struct sunfire_t : public sunfire_base_t
{
  moonfire_base_t* moonfire;

  sunfire_t( druid_t* player, const std::string& options_str ) :
    sunfire_base_t( player )
  {
    parse_options( options_str );

    if ( player -> specialization() == DRUID_BALANCE )
    {
      moonfire = new moonfire_base_t( player );
      moonfire -> callbacks = false;
      moonfire -> dual = moonfire -> background = true;
    }
  }
  
  // Execute CA Moonfire in update_ready() to assure the correct timing.
  // It must occur after Sunfire has impacted but before callbacks occur.
  virtual void update_ready( timespan_adl_barrier::timespan_t cd_duration ) override
  {
    sunfire_base_t::update_ready( cd_duration );

    if ( p() -> buff.celestial_alignment -> up() )
    {
      moonfire -> target = execute_state -> target;
      moonfire -> execute();
    }
  }
};

// Moonfire (Lunar Inspiration) Spell =======================================

struct moonfire_cat_t : public druid_spell_t
{
  moonfire_cat_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "moonfire_cat", player, player -> find_spell( 155625 ) )
  {
    parse_options( options_str );
  }

  virtual void impact( action_state_t* s ) override
  {
    druid_spell_t::impact( s );

    // Grant combo points
    if ( result_is_hit( s -> result ) )
    {
      p() -> resource_gain( RESOURCE_COMBO_POINT, 1, p() -> gain.moonfire );
      if ( p() -> spell.primal_fury -> ok() && s -> result == RESULT_CRIT )
      {
        p() -> proc.primal_fury -> occur();
        p() -> resource_gain( RESOURCE_COMBO_POINT, p() -> spell.primal_fury -> effectN( 1 ).base_value(), p() -> gain.primal_fury );
      }
    }
  }

  virtual bool ready() override
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
    parse_options( options_str );

    harmful           = false;
    ignore_false_positive = true;

    base_costs[ RESOURCE_MANA ] *= 1.0 + p() -> glyph.master_shapeshifter -> effectN( 1 ).percent();
  }

  void execute() override
  {
    spell_t::execute();

    p() -> buff.moonkin_form -> start();
  }

  bool ready() override
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
    parse_options( options_str );

    harmful = false;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.natures_swiftness -> trigger();
  }

  bool ready() override
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
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
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
    parse_options( options_str );

    trigger_gcd = timespan_t::zero();
    harmful     = false;
    ignore_false_positive = true;
  }

  void execute() override
  {
    if ( sim -> log )
      sim -> out_log.printf( "%s performs %s", player -> name(), name() );

    p() -> buff.prowl -> trigger();
  }

  bool ready() override
  {
    if ( p() -> buff.prowl -> check() )
      return false;

    if ( p() -> in_combat && ! ( p() -> buff.incarnation -> check() && p() -> specialization() == DRUID_FERAL ) )
      return false;

    return druid_spell_t::ready();
  }
};

// Savage Defense ===========================================================

struct savage_defense_t : public druid_spell_t
{
private:
  double dodge() const
  {
    // Get total dodge chance excluding SD.
    return p() -> cache.dodge() - p() -> buff.savage_defense -> check() * p() -> buff.savage_defense -> default_value;
  }
public:
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

  void execute() override
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

  double cooldown_reduction() const override
  {
    double cdr = druid_spell_t::cooldown_reduction();

    if ( p() -> talent.guardian_of_elune -> ok() )
      cdr /= 1 + dodge();

    return cdr;
  }

  double cost() const override
  {
    double c = druid_spell_t::cost();

    if ( p() -> talent.guardian_of_elune -> ok() )
    {
      c /= 1 + dodge();

      if ( sim -> debug )
        sim -> out_debug.printf( "druid_spell_t::cost: %s %.2f %s", name(), c, util::resource_type_string( current_resource() ) );
    }

    return c;
  }

  bool ready() override 
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
    parse_options( options_str );
    may_miss = may_glance = may_block = may_dodge = may_parry = may_crit = false;

    ignore_false_positive = true;

    cooldown -> duration += player -> glyph.skull_bash -> effectN( 1 ).time_value();
  }

  void execute() override
  {
    druid_spell_t::execute();

    if ( p() -> sets.has_set_bonus( DRUID_FERAL, PVP, B2 ) )
      p() -> cooldown.tigers_fury -> reset( false );
  }

  bool ready() override
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
    parse_options( options_str );
    harmful = false;
  }

  void execute() override
  {
    druid_spell_t::execute();

    for ( size_t i = 0; i < sim -> player_non_sleeping_list.size(); ++i )
    {
      player_t* p = sim -> player_non_sleeping_list[ i ];
      if( p -> type == PLAYER_GUARDIAN )
        continue;

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
    parse_options( options_str );
    base_execute_time *= 1 + player -> sets.set( DRUID_BALANCE, T17, B2 ) -> effectN( 1 ).percent();
  }

  double action_multiplier() const override
  {
    double m = druid_spell_t::action_multiplier();

    if ( p() -> buff.lunar_empowerment -> up() )
      m *= 1.0 + p() -> buff.lunar_empowerment -> data().effectN( 1 ).percent() +
                 p() -> talent.soul_of_the_forest -> effectN( 1 ).percent();

    return m;
  }

  timespan_t execute_time() const override
  {
    timespan_t casttime = druid_spell_t::execute_time();

    if ( p() -> buff.lunar_empowerment -> up() && p() -> talent.euphoria -> ok() )
      casttime *= 1 + p() -> talent.euphoria -> effectN( 2 ).percent();

    return casttime;
  }

  void execute() override
  {
    druid_spell_t::execute();

    if ( p() -> sets.has_set_bonus( DRUID_BALANCE, T17, B4 ) )
      p() -> cooldown.celestial_alignment -> adjust( -1 * p() -> sets.set( DRUID_BALANCE, T17, B4 ) -> effectN( 1 ).time_value() );

    if ( p() -> eclipse_amount < 0 && !p() -> buff.celestial_alignment -> up() )
      p() -> proc.wrong_eclipse_starfire -> occur();

    p() -> buff.lunar_empowerment -> decrement();
  }

  void impact( action_state_t* s ) override
  {
    druid_spell_t::impact( s );

    if ( p() -> talent.balance_of_power && result_is_hit( s -> result ) )
      td( s -> target ) -> dots.moonfire -> extend_duration( timespan_t::from_seconds( p() -> talent.balance_of_power -> effectN( 1 ).base_value() ), 0 );
  }
};

// Starfall Spell ===========================================================

struct starfall_t : public druid_spell_t
{
  struct starfall_pulse_t : public druid_spell_t
  {
    starfall_pulse_t( druid_t* player, const std::string& name ) :
      druid_spell_t( name, player, player -> find_spell( 50288 ) )
    {
      background = direct_tick = true;
      aoe = -1;
      range = 40;
      radius = 0;
      callbacks = false;
    }
  };

  spell_t* pulse;

  starfall_t( druid_t* player, const std::string& options_str ):
    druid_spell_t( "starfall", player, player -> find_specialization_spell( "Starfall" ) ),
    pulse( new starfall_pulse_t( player, "starfall_pulse" ) )
  {
    parse_options( options_str );

    const spell_data_t* aura_spell = player -> find_spell( data().effectN( 1 ).trigger_spell_id() );
    dot_duration = aura_spell -> duration();
    base_tick_time = aura_spell -> effectN( 1 ).period();
    hasted_ticks = may_crit = false;
    may_multistrike = 0;
    spell_power_mod.tick = spell_power_mod.direct = 0;
    cooldown = player -> cooldown.starfallsurge;
    base_multiplier *= 1.0 + player -> sets.set( SET_CASTER, T14, B2 ) -> effectN( 1 ).percent();
    add_child( pulse );
  }

  void tick( dot_t* d ) override
  {
    druid_spell_t::tick( d );

    // Only ticks while in moonkin form.
    if ( p() -> buff.moonkin_form -> check() )
    {
      pulse -> execute();
    }
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.starfall -> trigger();
  }

  bool ready() override
  {
    if ( p() -> buff.starfall -> check() )
      return false;

    return druid_spell_t::ready();
  }
};

struct starshards_t : public starfall_t
{
  starshards_t( druid_t* player ) :
    starfall_t( player, std::string( "" ) )
  {
    background = true;
    target = sim -> target;
    radius = 40;
    cooldown = player -> get_cooldown( "starshards" );
  }
  
  // Allow the action to execute even when Starfall is already active.
  bool ready() override
  { return druid_spell_t::ready(); }
};

// Starsurge Spell ==========================================================

struct starsurge_t : public druid_spell_t
{
  double starshards_chance;

  starsurge_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "starsurge", player, player -> spec.starsurge ),
    starshards_chance( 0.0 )
  {
    parse_options( options_str );

    base_multiplier *= 1.0 + player -> sets.set( SET_CASTER, T13, B4 ) -> effectN( 2 ).percent();
    base_multiplier *= 1.0 + p() -> sets.set( SET_CASTER, T13, B2 ) -> effectN( 1 ).percent();

    base_crit += p() -> sets.set( SET_CASTER, T15, B2 ) -> effectN( 1 ).percent();
    cooldown = player -> cooldown.starfallsurge;
    base_execute_time *= 1.0 + player -> perk.enhanced_starsurge -> effectN( 1 ).percent();

    if ( player -> starshards )
      starshards_chance = player -> starshards -> driver() -> effectN( 1 ).average( player -> starshards -> item ) / 100.0;
  }

  void execute() override
  {
    druid_spell_t::execute();

    if ( p() -> eclipse_amount < 0 )
      p() -> buff.solar_empowerment -> trigger( 3 );
    else
      p() -> buff.lunar_empowerment -> trigger( 2 );

    if ( p() -> starshards && rng().roll( starshards_chance ) )
    {
      p() -> proc.starshards -> occur();
      p() -> active.starshards -> execute();
    }
  }
};

// Stellar Flare ==========================================================

struct stellar_flare_t : public druid_spell_t
{
  moonfire_t* moonfire_;
  sunfire_t* sunfire_;

  stellar_flare_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "stellar_flare", player, player -> talent.stellar_flare ),
    moonfire_( nullptr ), sunfire_( nullptr )
  {
    parse_options( options_str );
    if ( player -> alternate_stellar_flare )
    {
      moonfire_ = new moonfire_t( player, options_str );
      sunfire_ = new sunfire_t( player, options_str );
      moonfire_ -> background = true;
      moonfire_ -> base_costs[RESOURCE_MANA] = 0;
      sunfire_ -> background = true;
      sunfire_ -> base_costs[RESOURCE_MANA] = 0;
    }
  }

  double action_multiplier() const override
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

  void execute() override
  {
    druid_spell_t::execute();
    if ( p() -> alternate_stellar_flare )
    {
      p() -> stellar_flare_cast = true;
      if ( p() -> eclipse_amount < 0 )
        sunfire_ -> execute();
      else
        moonfire_ -> execute();
      p() -> stellar_flare_cast = false;
    }
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

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.survival_instincts -> trigger(); // DBC value is 60 for some reason
  }
};

// Tiger's Fury =============================================================

struct tigers_fury_t : public druid_spell_t
{
  timespan_t duration;

  tigers_fury_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "tigers_fury", p, p -> find_specialization_spell( "Tiger's Fury" ), options_str ),
    duration( p -> buff.tigers_fury -> buff_duration )
  {
    harmful = false;
    
    /* If Druid Tier 18 (WoD 6.2) trinket effect is in use, adjust Tiger's Fury duration
       based on spell data of the special effect. */
    if ( p -> wildcat_celerity )
      duration *= 1.0 + p -> wildcat_celerity -> driver() -> effectN( 1 ).average( p -> wildcat_celerity -> item ) / 100.0;
  }

  void execute() override
  {
    druid_spell_t::execute();

    p() -> buff.tigers_fury -> trigger( 1, buff_t::DEFAULT_VALUE(), 1.0, duration );

    p() -> resource_gain( RESOURCE_ENERGY,
                          data().effectN( 2 ).resource( RESOURCE_ENERGY ),
                          p() -> gain.tigers_fury );

    if ( p() -> sets.has_set_bonus( SET_MELEE, T13, B4 ) )
      p() -> buff.omen_of_clarity -> trigger();

    if ( p() -> sets.has_set_bonus( SET_MELEE, T15, B4 ) )
      p() -> buff.feral_tier15_4pc -> trigger( 3 );

    if ( p() -> sets.has_set_bonus( SET_MELEE, T16, B4 ) )
      p() -> buff.feral_tier16_4pc -> trigger();
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
    parse_options( options_str );
    ignore_false_positive = true;
  }
};

// Wild Charge ==============================================================

struct wild_charge_t : public druid_spell_t
{
  double movement_speed_increase;

  wild_charge_t( druid_t* p, const std::string& options_str ) :
    druid_spell_t( "wild_charge", p, p -> talent.wild_charge ),
    movement_speed_increase( 5.0 )
  {
    parse_options( options_str );
    harmful = may_crit = may_miss = false;
    ignore_false_positive = true;
    range = data().max_range();
    movement_directionality = MOVEMENT_OMNI; 
    trigger_gcd = timespan_t::zero();
  }

  void schedule_execute( action_state_t* execute_state ) override
  {
    druid_spell_t::schedule_execute( execute_state );

    /* Since Cat/Bear charge is limited to moving towards a target,
       cancel form if the druid wants to move away.
       Other forms can already move in any direction they want so they're fine. */
    if ( p() -> current.movement_direction == MOVEMENT_AWAY )
    {
      p() -> buff.cat_form -> expire();
      p() -> buff.bear_form -> expire();
    }
  }

  void execute() override
  {
    if ( p() -> current.distance_to_move > data().min_range() )
    {
      p() -> buff.wild_charge_movement -> trigger( 1, movement_speed_increase, 1,
        timespan_t::from_seconds( p() -> current.distance_to_move / ( p() -> base_movement_speed * ( 1 + p() -> passive_movement_modifier() + movement_speed_increase ) ) ) );
    }

    druid_spell_t::execute();
  }

  bool ready() override
  {
    if ( p() -> current.distance_to_move < data().min_range() ) // Cannot charge if the target is too close.
      return false;

    return druid_spell_t::ready();
  }
};

// Wild Mushroom ============================================================

struct wild_mushroom_t : public druid_spell_t
{
  wild_mushroom_t( druid_t* player, const std::string& options_str ) :
    druid_spell_t( "wild_mushroom", player, player -> find_class_spell( "Wild Mushroom" ) )
  {
    parse_options( options_str );

    harmful = false;
    ignore_false_positive = true;
  }

  void execute() override
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
    parse_options( options_str );
    base_execute_time *= 1 + player -> sets.set( DRUID_BALANCE, T17, B2 ) -> effectN( 1 ).percent();
  }

  double action_multiplier() const override
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

  timespan_t execute_time() const override
  {
    timespan_t casttime = druid_spell_t::execute_time();

    if ( p() -> buff.solar_empowerment -> up() && p() -> talent.euphoria -> ok() )
      casttime *= 1 + p() -> talent.euphoria -> effectN( 2 ).percent();

    return casttime;
  }

  void schedule_execute( action_state_t* state = nullptr ) override
  {
    druid_spell_t::schedule_execute( state );

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

    p() -> buff.cat_form  -> expire();
    p() -> buff.bear_form -> expire();
  }

  void execute() override
  {
    druid_spell_t::execute();

    if ( p() -> sets.has_set_bonus( DRUID_BALANCE, T17, B4 ) )
      p() -> cooldown.celestial_alignment -> adjust( -1 * p() -> sets.set( DRUID_BALANCE, T17, B4 ) -> effectN( 1 ).time_value() );

    if ( p() -> eclipse_amount > 0 && !p() -> buff.celestial_alignment -> up() )
      p() -> proc.wrong_eclipse_wrath -> occur();

    p() -> buff.solar_empowerment -> decrement();
  }

  void impact( action_state_t* s ) override
  {
    druid_spell_t::impact( s );

    if ( p() -> talent.balance_of_power && result_is_hit( s -> result ) )
      td( s -> target ) -> dots.sunfire -> extend_duration( timespan_t::from_seconds( p() -> talent.balance_of_power -> effectN( 2 ).base_value() ), 0 );
  }
};

} // end namespace spells

// ==========================================================================
// Druid Character Definition
// ==========================================================================

void druid_t::trigger_shooting_stars( action_state_t* s )
{
  if ( s -> target != last_target_dot_moonkin )
    return;
  // Shooting stars will only proc on the most recent target of your moonfire/sunfire.
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
  if ( name == "displacer_beast"        ) return new        displacer_beast_t( this, options_str );
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
  if ( name == "moonfire"               ) return new               moonfire_t( this, options_str );
  if ( name == "moonfire_cat"           ) return new           moonfire_cat_t( this, options_str );
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
  if ( name == "wild_charge"            ) return new            wild_charge_t( this, options_str );
  if ( name == "wild_growth"            ) return new            wild_growth_t( this, options_str );
  if ( name == "wild_mushroom"          ) return new          wild_mushroom_t( this, options_str );
  if ( name == "wrath"                  ) return new                  wrath_t( this, options_str );
  if ( name == "incarnation"            ) return new            incarnation_t( this, options_str );

  return player_t::create_action( name, options_str );
}

// druid_t::create_pet ======================================================

pet_t* druid_t::create_pet( const std::string& pet_name,
                            const std::string& /* pet_type */ )
{
  pet_t* p = find_pet( pet_name );

  if ( p ) return p;

  using namespace pets;

  return nullptr;
}

// druid_t::create_pets =====================================================

void druid_t::create_pets()
{
  player_t::create_pets();

  if ( sets.has_set_bonus( DRUID_BALANCE, T18, B2 ) )
  {
    for ( pet_t*& pet : pet_fey_moonwing )
      pet = new pets::fey_moonwing_t( sim, this );
  }

  if ( talent.force_of_nature -> ok() && find_action( "force_of_nature" ) )
  {
    if ( specialization() == DRUID_BALANCE )
    {
      for ( pet_t*& pet : pet_force_of_nature )
        pet = new pets::force_of_nature_balance_t( sim, this );
    }
    else if ( specialization() == DRUID_FERAL )
    {
      for ( pet_t*& pet : pet_force_of_nature )
        pet = new pets::force_of_nature_feral_t( sim, this );
    }
    else if ( specialization() == DRUID_GUARDIAN )
    {
      for ( pet_t*& pet : pet_force_of_nature )
        pet = new pets::force_of_nature_guardian_t( sim, this );
    }
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
  spec.starfire                = find_specialization_spell( "Starfire" );
  spec.starsurge               = find_specialization_spell( "Starsurge" );
  spec.sunfire                 = find_specialization_spell( "Sunfire" );

  // Feral
  spec.predatory_swiftness     = find_specialization_spell( "Predatory Swiftness" );
  spec.nurturing_instinct      = find_specialization_spell( "Nurturing Instinct" );
  spec.predatory_swiftness     = find_specialization_spell( "Predatory Swiftness" );
  spec.rip                     = find_specialization_spell( "Rip" );
  spec.savage_roar             = find_specialization_spell( "Savage Roar" );
  spec.sharpened_claws         = find_specialization_spell( "Sharpened Claws" );
  spec.swipe                   = find_specialization_spell( "Swipe" );
  spec.tigers_fury             = find_specialization_spell( "Tiger's Fury" );

  // Guardian
  spec.bladed_armor            = find_specialization_spell( "Bladed Armor" );
  spec.resolve                 = find_specialization_spell( "Resolve" );
  spec.savage_defense          = find_specialization_spell( "Savage Defense" );
  spec.survival_of_the_fittest = find_specialization_spell( "Survival of the Fittest" );
  spec.thick_hide              = find_specialization_spell( "Thick Hide" );
  spec.tooth_and_claw          = find_specialization_spell( "Tooth and Claw" );
  spec.ursa_major              = find_specialization_spell( "Ursa Major" );
  spec.guardian_passive        = find_specialization_spell( "Guardian Overrides Passive" );

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
  mastery.primal_tenacity_AP = find_spell( 159195 );

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
  if ( specialization() == DRUID_BALANCE )
    active.starshards         = new spells::starshards_t( this );
  if ( specialization() == DRUID_GUARDIAN )
    active.stalwart_guardian  = new stalwart_guardian_t( this );

  // Spells
  spell.ferocious_bite                  = find_class_spell( "Ferocious Bite"              ) -> ok() ? find_spell( 22568  ) : spell_data_t::not_found(); // Get spell data for max_fb_energy calculation.
  spell.bear_form_passive               = find_class_spell( "Bear Form"                   ) -> ok() ? find_spell( 1178   ) : spell_data_t::not_found(); // This is the passive applied on shapeshift!
  spell.bear_form_skill                 = find_class_spell( "Bear Form"                   ) -> ok() ? find_spell( 5487   ) : spell_data_t::not_found(); // Bear form skill
  spell.berserk_bear                    = find_class_spell( "Berserk"                     ) -> ok() ? find_spell( 50334  ) : spell_data_t::not_found(); // Berserk bear mangler
  spell.berserk_cat                     = find_class_spell( "Berserk"                     ) -> ok() ? find_spell( 106951 ) : spell_data_t::not_found(); // Berserk cat resource cost reducer
  spell.cat_form                        = find_class_spell( "Cat Form"                    ) -> ok() ? find_spell( 3025   ) : spell_data_t::not_found();
  spell.cat_form_speed                  = find_class_spell( "Cat Form"                    ) -> ok() ? find_spell( 113636 ) : spell_data_t::not_found();
  spell.frenzied_regeneration           = find_class_spell( "Frenzied Regeneration"       ) -> ok() ? find_spell( 22842  ) : spell_data_t::not_found();
  spell.moonkin_form                    = find_class_spell( "Moonkin Form"                ) -> ok() ? find_spell( 24905  ) : spell_data_t::not_found(); // This is the passive applied on shapeshift!
  spell.regrowth                        = find_class_spell( "Regrowth"                    ) -> ok() ? find_spell( 93036  ) : spell_data_t::not_found(); // Regrowth refresh
  spell.starfall_aura                   = find_class_spell( "Starfall"                    ) -> ok() ? find_spell( 184989 ) : spell_data_t::not_found();

  if ( specialization() == DRUID_FERAL )
  {
    spell.primal_fury = find_spell( 16953 );
    spell.gushing_wound = find_spell( 165432 );
  }
  else if ( specialization() == DRUID_GUARDIAN )
  {
    spell.primal_fury = find_spell( 16959 );
  }

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

  // Set base distance based on spec
  base.distance = ( specialization() == DRUID_FERAL || specialization() == DRUID_GUARDIAN ) ? 3 : 30;

  // All specs get benefit from both agi and intellect.
  base.attack_power_per_agility  = 1.0;
  base.spell_power_per_intellect = 1.0;

  // Avoidance diminishing Returns constants/conversions now handled in player_t::init_base_stats().
  // Base miss, dodge, parry, and block are set in player_t::init_base_stats().
  // Just need to add class- or spec-based modifiers here (none for druids at the moment).

  if ( specialization() != DRUID_BALANCE )
  {
    resources.base[ RESOURCE_ENERGY      ] = 100 + sets.set( DRUID_FERAL, T18, B2 ) -> effectN( 2 ).resource( RESOURCE_ENERGY );
    resources.base[ RESOURCE_RAGE        ] = 100;
    resources.base[ RESOURCE_COMBO_POINT ] = 5;
  }
  else
    resources.base[ RESOURCE_ECLIPSE     ] = 105;

  base_energy_regen_per_second = 10;

  // Max Mana & Mana Regen modifiers
  resources.base_multiplier[ RESOURCE_MANA ] = 1.0 + spec.natural_insight -> effectN( 1 ).percent();
  base.mana_regen_per_second *= 1.0 + spec.natural_insight -> effectN( 1 ).percent();
  base.mana_regen_per_second *= 1.0 + spec.mana_attunement -> effectN( 1 ).percent();

  base_gcd = timespan_t::from_seconds( 1.5 );

  // initialize resolve for Guardians
  if ( specialization() == DRUID_GUARDIAN )
    resolve_manager.init();
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
                               .cd( timespan_t::zero() )
                               .default_value( find_class_spell( "Dash" ) -> effectN( 1 ).percent() );
  buff.frenzied_regeneration = buff_creator_t( this, "frenzied_regeneration", find_class_spell( "Frenzied Regeneration" ) );
  buff.moonkin_form          = new moonkin_form_t( *this );
  buff.omen_of_clarity       = new omen_of_clarity_buff_t( *this );
  buff.soul_of_the_forest    = buff_creator_t( this, "soul_of_the_forest", talent.soul_of_the_forest -> ok() ? find_spell( 114108 ) : spell_data_t::not_found() )
                               .default_value( find_spell( 114108 ) -> effectN( 1 ).percent() );
  buff.prowl                 = buff_creator_t( this, "prowl", find_class_spell( "Prowl" ) );
  buff.wild_mushroom         = buff_creator_t( this, "wild_mushroom", find_class_spell( "Wild Mushroom" ) )
                               .max_stack( ( specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION )
                                           ? find_class_spell( "Wild Mushroom" ) -> effectN( 2 ).base_value()
                                           : 1 )
                               .quiet( true );

  // Talent buffs

  buff.displacer_beast    = buff_creator_t( this, "displacer_beast", talent.displacer_beast -> effectN( 2 ).trigger() )
                            .default_value( talent.displacer_beast -> effectN( 2 ).trigger() -> effectN( 1 ).percent() );

  buff.wild_charge_movement = buff_creator_t( this, "wild_charge_movement" );

  buff.cenarion_ward = buff_creator_t( this, "cenarion_ward", find_talent_spell( "Cenarion Ward" ) );

  switch ( specialization() ) {
    case DRUID_BALANCE:     buff.incarnation = buff_creator_t( this, "incarnation", talent.incarnation_chosen )
                             .default_value( talent.incarnation_chosen -> effectN( 1 ).percent() )
                             .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER )
                             .cd( timespan_t::zero() );
                            break;
    case DRUID_FERAL:       buff.incarnation = buff_creator_t( this, "incarnation", talent.incarnation_king )
                             .cd( timespan_t::zero() );
                            break;
    case DRUID_GUARDIAN:    buff.incarnation = buff_creator_t( this, "incarnation", talent.incarnation_son )
                             .cd( timespan_t::zero() );
                            break;
    case DRUID_RESTORATION: buff.incarnation = buff_creator_t( this, "incarnation", talent.incarnation_tree )
                             .duration( timespan_t::from_seconds( 30 ) )
                             .cd( timespan_t::zero() );
                            break;
    default:
      break;
  }

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

  buff.starfall                  = buff_creator_t( this, "starfall", spell.starfall_aura )
                                   .refresh_behavior( BUFF_REFRESH_PANDEMIC );

  buff.faerie_blessing           = buff_creator_t( this, "faerie_blessing", find_spell( 188086 ) )
                                   .chance( sets.has_set_bonus( DRUID_BALANCE, T18, B4 ) )
                                   .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );

  // Feral
  buff.tigers_fury           = buff_creator_t( this, "tigers_fury", find_specialization_spell( "Tiger's Fury" ) )
                               .default_value( find_specialization_spell( "Tiger's Fury" ) -> effectN( 1 ).percent() )
                               .cd( timespan_t::zero() );
  buff.savage_roar           = buff_creator_t( this, "savage_roar", find_specialization_spell( "Savage Roar" ) )
                               .default_value( find_specialization_spell( "Savage Roar" ) -> effectN( 2 ).percent() - glyph.savagery -> effectN( 2 ).percent() )
                               .duration( find_spell( glyph.savagery -> effectN( 1 ).trigger_spell_id() ) -> duration() )
                               .refresh_behavior( BUFF_REFRESH_DURATION ) // Pandemic refresh is done by the action
                               .add_invalidate( CACHE_PLAYER_DAMAGE_MULTIPLIER );
  buff.predatory_swiftness   = buff_creator_t( this, "predatory_swiftness", spec.predatory_swiftness -> ok() ? find_spell( 69369 ) : spell_data_t::not_found() );
  buff.feral_tier15_4pc      = buff_creator_t( this, "feral_tier15_4pc", find_spell( 138358 ) );
  buff.feral_tier16_2pc      = buff_creator_t( this, "feral_tier16_2pc", find_spell( 144865 ) ); // tier16_2pc_melee
  buff.feral_tier16_4pc      = buff_creator_t( this, "feral_tier16_4pc", find_spell( 146874 ) ); // tier16_4pc_melee
  buff.feral_tier17_4pc      = buff_creator_t( this, "feral_tier17_4pc", find_spell( 166639 ) )
                               .quiet( true );

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
                               .gain( get_gain( "primal_tenacity" ) ) // gain.primal_tenacity isn't initialized yet
                               .high_priority( true );
  buff.pulverize             = buff_creator_t( this, "pulverize", find_spell( 158792 ) )
                               .default_value( find_spell( 158792 ) -> effectN( 1 ).percent() )
                               .refresh_behavior( BUFF_REFRESH_PANDEMIC );
  buff.savage_defense        = buff_creator_t( this, "savage_defense", find_class_spell( "Savage Defense" ) -> ok() ? find_spell( 132402 ) : spell_data_t::not_found() )
                               .add_invalidate( CACHE_DODGE )
                               .duration( find_spell( 132402 ) -> duration() + talent.guardian_of_elune -> effectN( 2 ).time_value() )
                               .default_value( find_spell( 132402 ) -> effectN( 1 ).percent() + talent.guardian_of_elune -> effectN( 1 ).percent() );
  buff.survival_instincts    = buff_creator_t( this, "survival_instincts", find_specialization_spell( "Survival Instincts" ) )
                               .cd( timespan_t::zero() )
                               .default_value( 0.0 - find_specialization_spell( "Survival Instincts" ) -> effectN( 1 ).percent() );
  buff.guardian_tier15_2pc   = buff_creator_t( this, "guardian_tier15_2pc", find_spell( 138217 ) );
  buff.guardian_tier17_4pc   = buff_creator_t( this, "guardian_tier17_4pc", find_spell( 177969 ) ) // FIXME: Remove fallback values after spell is in spell data.
                               .chance( find_spell( 177969 ) -> proc_chance() )
                               .duration( find_spell( 177969 ) -> duration() )
                               .max_stack( find_spell( 177969 ) -> max_stacks() )
                               .default_value( find_spell( 177969 ) -> effectN( 1 ).percent() );
  buff.tooth_and_claw        = buff_creator_t( this, "tooth_and_claw", find_spell( 135286 ) )
                               .max_stack( find_spell( 135286 ) -> _max_stack + perk.enhanced_tooth_and_claw -> effectN( 1 ).base_value() );
  buff.tooth_and_claw_absorb = new tooth_and_claw_absorb_t( *this );
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
  if ( sim -> allow_flasks && true_level >= 80 )
  {
    std::string flask = "flask,type=";
    std::string elixir1, elixir2;
    elixir1 = elixir2 = "elixir,type=";

    if ( primary_role() == ROLE_TANK ) // Guardian
    {
      if ( true_level > 90 )
        flask += "greater_draenic_agility_flask";
      else if ( true_level > 85 )
        flask += "winds";
      else
        flask += "steelskin";
    }
    else if ( primary_role() == ROLE_ATTACK ) // Feral
    {
      if ( true_level > 90 )
        flask += "greater_draenic_agility_flask";
      else
        flask += "winds";
    }
    else // Balance & Restoration
    {
      if ( true_level > 90 )
        flask += "greater_draenic_intellect_flask";
      else if ( true_level > 85 )
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
  if ( sim -> allow_food && level() > 80 )
  {
    std::string food = "food,type=";

    if ( level() > 90 )
    {
      if ( specialization() == DRUID_FERAL )
        food += "pickled_eel";
      else if ( specialization() == DRUID_BALANCE )
        food += "sleeper_sushi";
      else if ( specialization() == DRUID_GUARDIAN )
        food += "sleeper_sushi";
      else
        food += "buttered_sturgeon";
    }
    else if ( level() > 85 )
      food += "seafood_magnifique_feast";
    else
      food += "seafood_magnifique_feast";

    precombat -> add_action( food );
  }

  // Mark of the Wild
  precombat -> add_action( this, "Mark of the Wild", "if=!aura.str_agi_int.up" );

  // Feral: Bloodtalons
  if ( specialization() == DRUID_FERAL && true_level >= 100 )
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
  if ( sim -> allow_potions && true_level >= 80 )
  {
    std::string potion_action = "potion,name=";
    if ( specialization() == DRUID_FERAL && primary_role() == ROLE_ATTACK )
    {
      if ( true_level > 90 )
        potion_action += "draenic_agility";
      else if ( true_level > 85 )
        potion_action += "tolvir";
      else
        potion_action += "tolvir";
      precombat -> add_action( potion_action );
    }
    else if ( ( specialization() == DRUID_BALANCE || specialization() == DRUID_RESTORATION ) && ( primary_role() == ROLE_SPELL || primary_role() == ROLE_HEAL ) )
    {
      if ( true_level > 90 )
        potion_action += "draenic_intellect";
      else if ( true_level > 85 )
        potion_action += "jade_serpent";
      else
        potion_action += "volcanic";
      precombat -> add_action( potion_action );
    }
  }

  // Spec Specific Optimizations
  if ( specialization() == DRUID_BALANCE )
  {
    precombat -> add_action( "incarnation" );
    precombat -> add_action( this, "Starfire" );
  }
  else if ( specialization() == DRUID_GUARDIAN )
    precombat -> add_talent( this, "Cenarion Ward" );
  else if ( specialization() == DRUID_FERAL && ( find_item( "soul_capacitor") || find_item( "maalus_the_blood_drinker" ) ) )
    precombat -> add_action( "incarnation" );
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
  action_priority_list_t* def      = get_action_priority_list( "default"   );
  action_priority_list_t* finish   = get_action_priority_list( "finisher"  );
  action_priority_list_t* maintain = get_action_priority_list( "maintain"  );
  action_priority_list_t* generate = get_action_priority_list( "generator" );

  std::vector<std::string> racial_actions = get_racial_actions();
  std::string              potion_action  = "potion,name=";
  if ( true_level > 90 )
    potion_action += "draenic_agility";
  else if ( true_level > 85 )
    potion_action += "tolvir";
  else
    potion_action += "tolvir";

  // Main List =============================================================

  def -> add_action( this, "Cat Form" );
  if ( find_item( "maalus_the_blood_drinker") && find_item( "soul_capacitor" ) )
    def -> add_action( "cancel_buff,name=spirit_shift,if=buff.maalus.remains<1&buff.maalus.up&buff.spirit_shift.remains-buff.maalus.remains<5", "Explode Spirit Shift at the end of Maalus if it has significant damage stored up." );
  def -> add_talent( this, "Wild Charge" );
  def -> add_talent( this, "Displacer Beast", "if=movement.distance>10" );
  def -> add_action( this, "Dash", "if=movement.distance&buff.displacer_beast.down&buff.wild_charge_movement.down" );
  if ( race == RACE_NIGHT_ELF )
    def -> add_action( this, "Rake", "if=buff.prowl.up|buff.shadowmeld.up" );
  else
    def -> add_action( this, "Rake", "if=buff.prowl.up" );
  def -> add_action( "auto_attack" );
  def -> add_action( this, "Skull Bash" );
  def -> add_talent( this, "Force of Nature", "if=charges=3|trinket.proc.all.react|target.time_to_die<20");
  def -> add_action( this, "Berserk", "if=buff.tigers_fury.up&(buff.incarnation.up|!talent.incarnation_king_of_the_jungle.enabled)" );

  // On-Use Items
  for ( size_t i = 0; i < items.size(); i++ )
  {
    if ( items[ i ].has_use_special_effect() )
    {
      std::string line = std::string( "use_item,slot=" ) + items[ i ].slot_name();
      if ( items[ i ].name_str == "mirror_of_the_blademaster" )
        line += ",if=raid_event.adds.in>60|!raid_event.adds.exists|spell_targets.swipe>desired_targets";
      else if ( items[ i ].name_str != "maalus_the_blood_drinker" )
        line += ",if=(prev.tigers_fury&(target.time_to_die>trinket.stat.any.cooldown|target.time_to_die<45))|prev.berserk|(buff.incarnation.up&time<10)";

      def -> add_action( line );
    }
  }

  if ( sim -> allow_potions && true_level >= 80 )
    def -> add_action( potion_action + ",if=(buff.berserk.remains>10&(target.time_to_die<180|(trinket.proc.all.react&target.health.pct<25)))|target.time_to_die<=40" );

  // Racials
  for ( size_t i = 0; i < racial_actions.size(); i++ )
  {
    def -> add_action( racial_actions[ i ] + ",sync=tigers_fury" );
  }

  def -> add_action( this, "Tiger's Fury", "if=(!buff.omen_of_clarity.react&energy.deficit>=60)|energy.deficit>=80|(t18_class_trinket&buff.berserk.up&buff.tigers_fury.down)" );
  def -> add_action( "incarnation,if=cooldown.berserk.remains<10&energy.time_to_max>1" );
  def -> add_action( this, "Ferocious Bite", "cycle_targets=1,if=dot.rip.ticking&dot.rip.remains<3&target.health.pct<25",
                     "Keep Rip from falling off during execute range." );
  def -> add_action( this, "Healing Touch", "if=talent.bloodtalons.enabled&buff.predatory_swiftness.up&((combo_points>=4&!set_bonus.tier18_4pc)|combo_points=5|buff.predatory_swiftness.remains<1.5)" );
  def -> add_action( this, "Savage Roar", "if=buff.savage_roar.down" );
  def -> add_action( "thrash_cat,if=set_bonus.tier18_4pc&buff.omen_of_clarity.react&remains<4.5&combo_points+buff.bloodtalons.stack!=6" );
  def -> add_action( "pool_resource,for_next=1" );
  def -> add_action( "thrash_cat,cycle_targets=1,if=remains<4.5&(spell_targets.thrash_cat>=2&set_bonus.tier17_2pc|spell_targets.thrash_cat>=4)" );
  def -> add_action( "call_action_list,name=finisher,if=combo_points=5" );
  def -> add_action( this, "Savage Roar", "if=buff.savage_roar.remains<gcd" );
  def -> add_action( "call_action_list,name=maintain,if=combo_points<5" );
  def -> add_action( "pool_resource,for_next=1" );
  def -> add_action( "thrash_cat,cycle_targets=1,if=remains<4.5&spell_targets.thrash_cat>=2" );
  def -> add_action( "call_action_list,name=generator,if=combo_points<5" );

  // Finishers  
  finish -> add_action( this, "Rip", "cycle_targets=1,if=remains<2&target.time_to_die-remains>18&(target.health.pct>25|!dot.rip.ticking)" );
  finish -> add_action( this, "Ferocious Bite", "cycle_targets=1,max_energy=1,if=target.health.pct<25&dot.rip.ticking" );
  finish -> add_action( this, "Rip", "cycle_targets=1,if=remains<7.2&persistent_multiplier>dot.rip.pmultiplier&target.time_to_die-remains>18" );
  finish -> add_action( this, "Rip", "cycle_targets=1,if=remains<7.2&persistent_multiplier=dot.rip.pmultiplier&(energy.time_to_max<=1|(set_bonus.tier18_4pc&energy>50)|(set_bonus.tier18_2pc&buff.omen_of_clarity.react)|!talent.bloodtalons.enabled)&target.time_to_die-remains>18" );
  finish -> add_action( this, "Savage Roar", "if=((set_bonus.tier18_4pc&energy>50)|(set_bonus.tier18_2pc&buff.omen_of_clarity.react)|energy.time_to_max<=1|buff.berserk.up|cooldown.tigers_fury.remains<3)&buff.savage_roar.remains<12.6" );
  finish -> add_action( this, "Ferocious Bite", "max_energy=1,if=(set_bonus.tier18_4pc&energy>50)|(set_bonus.tier18_2pc&buff.omen_of_clarity.react)|energy.time_to_max<=1|buff.berserk.up|cooldown.tigers_fury.remains<3" );

  // DoT Maintenance
  if ( race == RACE_NIGHT_ELF )
  {
    maintain -> add_action( "shadowmeld,if=energy>=35&dot.rake.pmultiplier<2.1&buff.tigers_fury.up&(buff.bloodtalons.up|!talent.bloodtalons.enabled)&(!talent.incarnation.enabled|cooldown.incarnation.remains>18)&!buff.incarnation.up" );
  }
  maintain -> add_action( this, "Rake", "cycle_targets=1,if=remains<3&((target.time_to_die-remains>3&spell_targets.swipe<3)|target.time_to_die-remains>6)" );
  maintain -> add_action( this, "Rake", "cycle_targets=1,if=remains<4.5&(persistent_multiplier>=dot.rake.pmultiplier|(talent.bloodtalons.enabled&(buff.bloodtalons.up|!buff.predatory_swiftness.up)))&((target.time_to_die-remains>3&spell_targets.swipe<3)|target.time_to_die-remains>6)" );
  maintain -> add_action( "moonfire_cat,cycle_targets=1,if=remains<4.2&spell_targets.swipe<=5&target.time_to_die-remains>tick_time*5" );
  maintain -> add_action( this, "Rake", "cycle_targets=1,if=persistent_multiplier>dot.rake.pmultiplier&spell_targets.swipe=1&((target.time_to_die-remains>3&spell_targets.swipe<3)|target.time_to_die-remains>6)" );

  // Generators
  generate -> add_action( this, "Swipe", "if=spell_targets.swipe>=4|(spell_targets.swipe>=3&buff.incarnation.down)" );
  generate -> add_action( this, "Shred", "if=spell_targets.swipe<3|(spell_targets.swipe=3&buff.incarnation.up)" );
}

// Balance Combat Action Priority List ==============================

void druid_t::apl_balance()
{
  std::vector<std::string> racial_actions = get_racial_actions();
  std::vector<std::string> item_actions   = get_item_actions();
  std::string              potion_action  = "potion,name=";
  if ( true_level > 90 )
    potion_action += "draenic_intellect";
  else if ( true_level > 85 )
    potion_action += "jade_serpent";
  else
    potion_action += "volcanic";

  action_priority_list_t* default_list        = get_action_priority_list( "default" );
  action_priority_list_t* single_target       = get_action_priority_list( "single_target" );
  action_priority_list_t* aoe                 = get_action_priority_list( "aoe" );
  action_priority_list_t* ca                  = get_action_priority_list( "ca" );
  action_priority_list_t* ca_aoe              = get_action_priority_list( "ca_aoe" );
  action_priority_list_t* aoe_t18_trinket     = get_action_priority_list( "aoe_t18_trinket" );
  action_priority_list_t* cooldowns           = get_action_priority_list( "cooldowns" );
  
  // Main List
  default_list -> add_talent( this, "Force of Nature", "if=trinket.stat.intellect.up|charges=3|target.time_to_die<21" );
  default_list -> add_action( "call_action_list,name=cooldowns,if=cooldown.celestial_alignment.up&(eclipse_energy>=0|target.time_to_die<=30+gcd)" );
  for ( size_t i = 0; i < item_actions.size(); i++ )
    default_list -> add_action( item_actions[i] );
  default_list -> add_action( "call_action_list,name=ca_aoe,if=buff.celestial_alignment.up&spell_targets.starfall_pulse>1&!t18_class_trinket" );
  default_list -> add_action( "call_action_list,name=ca,if=buff.celestial_alignment.up&(spell_targets.starfall_pulse=1|t18_class_trinket)" );
  default_list -> add_action( "call_action_list,name=aoe_t18_trinket,if=buff.celestial_alignment.down&spell_targets.starfall.pulse>1&t18_class_trinket" );
  default_list -> add_action( "call_action_list,name=aoe,if=spell_targets.starfall_pulse>1&buff.celestial_alignment.down&!t18_class_trinket" );
  default_list -> add_action( "call_action_list,name=single_target,if=spell_targets.starfall_pulse=1&buff.celestial_alignment.down" );
  
  // Cooldowns
  cooldowns -> add_action( "incarnation" );
  if ( sim -> allow_potions && true_level >= 80 )
    cooldowns -> add_action( potion_action );
  if ( race == RACE_TROLL )
    cooldowns -> add_action( "Berserking" );
  cooldowns -> add_action( this, "Celestial Alignment" );
  
  // AOE with cooldowns still up, no t18 trinket
  ca_aoe -> add_action( this, "Starfall", "if=buff.starfall.remains<3" );
  ca_aoe -> add_action( this, "Moonfire", "cycle_targets=1,if=!dot.moonfire.ticking|!dot.sunfire.ticking" );
  ca_aoe -> add_action( this, "Sunfire", "cycle_targets=1,if=!dot.moonfire.ticking|!dot.sunfire.ticking" );
  ca_aoe -> add_action( this, "Starsurge", "if=buff.lunar_empowerment.down&eclipse_energy>=0&charges>1" );
  ca_aoe -> add_action( this, "Starsurge", "if=buff.solar_empowerment.down&eclipse_energy<0&charges>1" );
  ca_aoe -> add_action( this, "Starfire", "if=eclipse_energy>=0&buff.celestial_alignment.remains>cast_time" );
  ca_aoe -> add_action( this, "Wrath", "if=buff.celestial_alignment.remains>cast_time" );
  ca_aoe -> add_action( this, "Moonfire", "cycle_targets=1" );
  ca_aoe -> add_action( this, "Sunfire", "cycle_targets=1" );
  
  // Rotation with cooldowns still up, single target or t18 trinket
  ca -> add_action( this, "Starsurge", "if=(buff.lunar_empowerment.down&eclipse_energy>=0)|(buff.solar_empowerment.down&eclipse_energy<0)" );
  ca -> add_action( this, "Moonfire", "cycle_targets=1,if=!dot.moonfire.remains|!dot.sunfire.remains" );
  ca -> add_action( this, "Sunfire", "cycle_targets=1,if=!dot.moonfire.remains|!dot.sunfire.remains" );
  ca -> add_action( this, "Starfire", "if=eclipse_energy>=0&buff.celestial_alignment.remains>cast_time" );
  ca -> add_action( this, "Wrath", "if=buff.celestial_alignment.remains>cast_time" );
  ca -> add_action( this, "Moonfire", "cycle_targets=1" );
  ca -> add_action( this, "Sunfire", "cycle_targets=1" );
  
  // AOE with t18 trinket
  aoe_t18_trinket -> add_action( this, "Starsurge", "if=charges=3" );
  aoe_t18_trinket -> add_action( this, "Sunfire", "cycle_targets=1,if=remains<8" );
  aoe_t18_trinket -> add_action( this, "Moonfire", "cycle_targets=1,if=remains<12" );
  aoe_t18_trinket -> add_action( this, "Starsurge", "if=eclipse_energy>40&buff.lunar_empowerment.down" );
  aoe_t18_trinket -> add_action( this, "Starsurge", "if=eclipse_energy<-40&buff.solar_empowerment.down" );
  aoe_t18_trinket -> add_action( this, "Wrath", "if=(eclipse_energy<0&action.starfire.cast_time<eclipse_change)|(eclipse_energy>0&cast_time>eclipse_change)" );
  aoe_t18_trinket -> add_action( this, "Starfire" );
  
  // AOE without t18 trinket
  aoe -> add_action( this, "Sunfire", "cycle_targets=1,if=remains<8" );
  aoe -> add_action( this, "Starfall", "if=spell_targets.starfall_pulse>2&buff.starfall.remains<3" );
  aoe -> add_action( this, "Starfall", "if=@eclipse_energy<20&eclipse_dir.lunar&buff.starfall.remains<3&talent.euphoria.enabled" );
  aoe -> add_action( this, "Starfall", "if=@eclipse_energy<10&eclipse_dir.lunar&buff.starfall.remains<3&!talent.euphoria.enabled" );
  aoe -> add_action( this, "Moonfire", "cycle_targets=1,if=remains<12" );
  aoe -> add_talent( this, "Stellar Flare", "cycle_targets=1,if=remains<7" );
  aoe -> add_action( this, "Starsurge", "if=(buff.lunar_empowerment.down&eclipse_energy>40&charges>1)|charges=3" );
  aoe -> add_action( this, "Starsurge", "if=(buff.solar_empowerment.down&eclipse_energy<-40&charges>1)|charges=3" );
  aoe -> add_action( this, "Wrath", "if=(eclipse_energy<=0&eclipse_change>action.starfire.cast_time)|(eclipse_energy>0&cast_time>eclipse_change)" );
  aoe -> add_action( this, "Starfire" );
  
  // Single Target
  single_target -> add_action( this, "Starsurge", "if=charges=3" );
  single_target -> add_action( this, "Starsurge", "if=buff.lunar_empowerment.down&eclipse_energy>40" );
  single_target -> add_action( this, "Starsurge", "if=buff.solar_empowerment.down&eclipse_energy<-40" );
  single_target -> add_action( this, "Sunfire", "if=!talent.balance_of_power.enabled&((remains<solar_max&eclipse_dir.solar)|(buff.solar_peak.up&buff.solar_peak.remains<action.wrath.cast_time))" );
  single_target -> add_action( this, "Sunfire", "if=talent.balance_of_power.enabled&(remains<lunar_max+10|remains<action.wrath.cast_time)" );
  single_target -> add_talent( this, "Stellar Flare", "if=remains<7" );
  single_target -> add_action( this, "Moonfire", "if=!talent.euphoria.enabled&!talent.balance_of_power.enabled&((remains<lunar_max&eclipse_dir.lunar)|(buff.lunar_peak.up&buff.lunar_peak.remains<action.starfire.cast_time&remains<eclipse_change+20))" );
  single_target -> add_action( this, "Moonfire", "if=talent.euphoria.enabled&((remains<lunar_max&eclipse_dir.lunar)|(buff.lunar_peak.up&buff.lunar_peak.remains<action.starfire.cast_time&remains<eclipse_change+10))" );
  single_target -> add_action( this, "Moonfire", "if=talent.balance_of_power.enabled&(remains<solar_max+10|remains<action.starfire.cast_time)" );
  single_target -> add_action( this, "Wrath", "if=(eclipse_energy<0&eclipse_change>action.starfire.cast_time)|(eclipse_energy>0&cast_time>eclipse_change)" );
  single_target -> add_action( this, "Starfire" );
}

// Guardian Combat Action Priority List ==============================

void druid_t::apl_guardian()
{
  action_priority_list_t* default_list    = get_action_priority_list( "default" );

  std::vector<std::string> item_actions       = get_item_actions();
  std::vector<std::string> racial_actions     = get_racial_actions();

  default_list -> add_action( "auto_attack" );
  default_list -> add_action( this, "Skull Bash" );
  default_list -> add_action( this, "Savage Defense", "if=buff.barkskin.down" );

  for ( size_t i = 0; i < racial_actions.size(); i++ )
    default_list -> add_action( racial_actions[i] );
  for ( size_t i = 0; i < item_actions.size(); i++ )
    default_list -> add_action( item_actions[i] );

  default_list -> add_action( this, "Barkskin", "if=buff.bristling_fur.down" );
  default_list -> add_talent( this, "Bristling Fur", "if=buff.barkskin.down&buff.savage_defense.down" );
  default_list -> add_action( this, "Maul", "if=buff.tooth_and_claw.react&incoming_damage_1s" );
  default_list -> add_action( this, "Berserk", "if=(buff.pulverize.remains>10|!talent.pulverize.enabled)&buff.incarnation.down" );
  default_list -> add_action( this, "Frenzied Regeneration", "if=rage>=80" );
  default_list -> add_talent( this, "Cenarion Ward" );
  default_list -> add_talent( this, "Renewal", "if=health.pct<30" );
  default_list -> add_talent( this, "Heart of the Wild" );
  default_list -> add_action( this, "Rejuvenation", "if=buff.heart_of_the_wild.up&remains<=3.6" );
  default_list -> add_talent( this, "Nature's Vigil" );
  default_list -> add_action( this, "Healing Touch", "if=buff.dream_of_cenarius.react&health.pct<30" );
  default_list -> add_talent( this, "Pulverize", "if=buff.pulverize.remains<=3.6" );
  default_list -> add_action( this, "Lacerate", "if=talent.pulverize.enabled&buff.pulverize.remains<=(3-dot.lacerate.stack)*gcd&buff.berserk.down" );
  default_list -> add_action( "incarnation,if=buff.berserk.down" );
  default_list -> add_action( this, "Lacerate", "if=!ticking" );
  default_list -> add_action( "thrash_bear,if=!ticking" );
  default_list -> add_action( this, "Mangle" );
  default_list -> add_action( "thrash_bear,if=remains<=4.8" );
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
  default_list -> add_action( this, "Lifebloom", "if=debuff.lifebloom.down" );
  default_list -> add_action( this, "Swiftmend" );
  default_list -> add_action( this, "Healing Touch" );
}

// druid_t::init_scaling ====================================================

void druid_t::init_scaling()
{
  player_t::init_scaling();

  equipped_weapon_dps = main_hand_weapon.damage / main_hand_weapon.swing_time.total_seconds();

  if ( specialization() == DRUID_GUARDIAN )
  {
    scales_with[ STAT_WEAPON_DPS ] = false;
    scales_with[ STAT_PARRY_RATING ] = false;
    scales_with[ STAT_BONUS_ARMOR ] = true;
  }

  scales_with[ STAT_STRENGTH ] = false;

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

// druid_t::init ============================================================

void druid_t::init()
{
  player_t::init();

  if ( specialization() == DRUID_RESTORATION )
    sim -> errorf( "%s is using an unsupported spec.", name() );
}
// druid_t::init_gains ======================================================

void druid_t::init_gains()
{
  player_t::init_gains();

  gain.bear_form             = get_gain( "bear_form"             );
  gain.energy_refund         = get_gain( "energy_refund"         );
  gain.frenzied_regeneration = get_gain( "frenzied_regeneration" );
  gain.glyph_ferocious_bite  = get_gain( "glyph_ferocious_bite"  );
  gain.leader_of_the_pack    = get_gain( "leader_of_the_pack"    );
  gain.moonfire              = get_gain( "moonfire"              );
  gain.omen_of_clarity       = get_gain( "omen_of_clarity"       );
  gain.primal_fury           = get_gain( "primal_fury"           );
  gain.primal_tenacity       = get_gain( "primal_tenacity"       );
  gain.rage_refund           = get_gain( "rage_refund"           );
  gain.rake                  = get_gain( "rake"                  );
  gain.shred                 = get_gain( "shred"                 );
  gain.soul_of_the_forest    = get_gain( "soul_of_the_forest"    );
  gain.stalwart_guardian     = get_gain( "stalwart_guardian"     );
  gain.swipe                 = get_gain( "swipe"                 );
  gain.tigers_fury           = get_gain( "tigers_fury"           );
  gain.tooth_and_claw        = get_gain( "tooth_and_claw"        );
  
  gain.feral_tier15_2pc      = get_gain( "feral_tier15_2pc"      );
  gain.feral_tier16_4pc      = get_gain( "feral_tier16_4pc"      );
  gain.feral_tier17_2pc      = get_gain( "feral_tier17_2pc"      );
  gain.feral_tier18_4pc      = get_gain( "feral_tier18_4pc"      );
  gain.guardian_tier17_2pc   = get_gain( "guardian_tier17_2pc"   );
  gain.guardian_tier18_2pc   = get_gain( "guardian_tier18_2pc"   );

  primal_tenacity_stats      = get_stats( "primal_tenacity"      );
}

// druid_t::init_procs ======================================================

void druid_t::init_procs()
{
  player_t::init_procs();

  proc.omen_of_clarity          = get_proc( "omen_of_clarity"                           );
  proc.omen_of_clarity_wasted   = get_proc( "omen_of_clarity_wasted"                    );
  proc.primal_fury              = get_proc( "primal_fury"                               );
  proc.shooting_stars_wasted    = get_proc( "Shooting Stars overflow (buff already up)" );
  proc.shooting_stars           = get_proc( "Shooting Stars"                            );
  proc.starshards               = get_proc( "Starshards"                                );
  proc.tier15_2pc_melee         = get_proc( "tier15_2pc_melee"                          );
  proc.tier17_2pc_melee         = get_proc( "tier17_2pc_melee"                          );
  proc.tooth_and_claw           = get_proc( "tooth_and_claw"                            );
  proc.tooth_and_claw_wasted    = get_proc( "tooth_and_claw_wasted"                     );
  proc.ursa_major               = get_proc( "ursa_major"                                );
  proc.wrong_eclipse_wrath      = get_proc( "wrong_eclipse_wrath"                       );
  proc.wrong_eclipse_starfire   = get_proc( "wrong_eclipse_starfire"                    );
}

// druid_t::init_resources ===========================================

void druid_t::init_resources( bool force )
{
  player_t::init_resources( force );

  resources.current[ RESOURCE_ECLIPSE ] = 0;
}

// druid_t::init_rng =======================================================

void druid_t::init_rng()
{
  // RPPM objects
  balance_t18_2pc.set_frequency( sets.set( DRUID_BALANCE, T18, B2 ) -> real_ppm() );

  player_t::init_rng();
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

// druid_t::has_t18_class_trinket ===========================================

bool druid_t::has_t18_class_trinket() const
{
  switch( specialization() )
  {
    case DRUID_BALANCE:     return starshards != nullptr;
    case DRUID_FERAL:       return wildcat_celerity != nullptr;
    case DRUID_GUARDIAN:    return stalwart_guardian != nullptr;
    case DRUID_RESTORATION: return flourish != nullptr;
    default:                return false;
  }
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
  max_fb_energy = spell.ferocious_bite -> powerN( 1 ).cost() - spell.ferocious_bite -> effectN( 2 ).base_value();

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

// druid_t::merge ===========================================================

void druid_t::merge( player_t& other )
{
  player_t::merge( other );

  druid_t& od = static_cast<druid_t&>( other );

  for ( size_t i = 0, end = counters.size(); i < end; i++ )
    counters[ i ] -> merge( *od.counters[ i ] );
}

// druid_t::regen ===========================================================

void druid_t::regen( timespan_t periodicity )
{
  player_t::regen( periodicity );

  if ( specialization() == DRUID_BALANCE )
    balance_tracker(); // So much for trying to optimize, too many edge cases messing up. 

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

  if ( predatory_swiftness_bug && glyph.savage_roar -> ok() )
    buff.predatory_swiftness -> trigger();

  if ( spell.berserk_cat -> ok() && initial_berserk_duration > 0 )
  {
    buff.berserk -> trigger( 1, buff_t::DEFAULT_VALUE(), -1.0, timespan_t::from_seconds( initial_berserk_duration ) );
    resources.current[ RESOURCE_ENERGY ] = resources.max[ RESOURCE_ENERGY ];
  }
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
    case CACHE_BONUS_ARMOR:
      if ( spec.bladed_armor -> ok() )
        player_t::invalidate_cache( CACHE_ATTACK_POWER );
      break;
    default: break;
  }
}

// druid_t::composite_attack_power_multiplier ===============================

double druid_t::composite_attack_power_multiplier() const
{
  double ap = player_t::composite_attack_power_multiplier();

  if ( mastery.primal_tenacity -> ok() )
    ap *= 1.0 + cache.mastery() * mastery.primal_tenacity_AP -> effectN( 1 ).mastery_value();

  return ap;
}

// druid_t::composite_armor_multiplier ======================================

double druid_t::composite_armor_multiplier() const
{
  double a = player_t::composite_armor_multiplier();

  if ( buff.bear_form -> check() )
    a *= 1.0 + spell.bear_form_skill -> effectN( 3 ).percent() + glyph.ursols_defense -> effectN( 1 ).percent() + spec.survival_of_the_fittest -> effectN( 2 ).percent();

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
      m *= 1.0 + buff.celestial_alignment -> data().effectN( 2 ).percent();
    if ( dbc::is_school( school, SCHOOL_ARCANE ) || dbc::is_school( school, SCHOOL_NATURE ) )
    {
      if ( buff.moonkin_form -> check() )
        m *= 1.0 + spell.moonkin_form -> effectN( 2 ).percent();
      if ( buff.incarnation -> check() )
        m *= 1.0 + buff.incarnation -> default_value;
      if ( buff.faerie_blessing -> check() )
        m *= 1.0 + buff.faerie_blessing -> data().effectN( 1 ).percent();
    }
  }
  return m;
}

// druid_t::composite_player_heal_multiplier ================================

double druid_t::composite_player_heal_multiplier( const action_state_t* s ) const
{
  double m = player_t::composite_player_heal_multiplier( s );

  m *= 1.0 + buff.heart_of_the_wild -> heal_multiplier();

  return m;
}

// druid_t::composite_melee_expertise( weapon_t* ) ==========================

double druid_t::composite_melee_expertise( const weapon_t* ) const
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

  if ( buff.dash -> up() )
    active = std::max( active, buff.dash -> value() );

  if ( buff.wild_charge_movement -> up() )
    active = std::max( active, buff.wild_charge_movement -> value() );

  if ( buff.displacer_beast -> up() )
    active = std::max( active, buff.displacer_beast -> value() );

  return active;
}

// druid_t::passive_movement_modifier ========================================

double druid_t::passive_movement_modifier() const
{
  double ms = player_t::passive_movement_modifier();

  if ( buff.cat_form -> up() )
  {
    ms += spell.cat_form_speed -> effectN( 1 ).percent();
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
        p += spec.nurturing_instinct -> effectN( 1 ).percent() * cache.agility();
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
        a += spec.killer_instinct -> effectN( 1 ).percent() * cache.intellect();
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
        m *= 1.0 + spell.bear_form_passive -> effectN( 2 ).percent() + perk.empowered_bear_form -> effectN( 1 ).percent();
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

  d += talent.guardian_of_elune -> effectN( 5 ).percent();

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
    virtual double evaluate() override { return druid.eclipse_direction == rt; }
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
  else if ( util::str_compare_ci( name_str, "active_rejuvenations" ) )
  {
    return make_ref_expr( "active_rejuvenations", active_rejuvenations );
  }
  else if ( util::str_compare_ci( name_str, "eclipse_max" ) )
  {
    return make_ref_expr( "eclipse_max", eclipse_max );
  }
  else if ( util::str_compare_ci( name_str, "combo_points" ) )
  {
    return make_ref_expr( "combo_points", resources.current[ RESOURCE_COMBO_POINT ] );
  }
  else if ( util::str_compare_ci( name_str, "max_fb_energy" ) )
  {
    return make_ref_expr( "max_fb_energy", max_fb_energy );
  }
  return player_t::create_expression( a, name_str );
}

// druid_t::create_options ==================================================

void druid_t::create_options()
{
  player_t::create_options();

  add_option( opt_bool( "alternate_stellar_flare", alternate_stellar_flare ) );
  add_option( opt_bool( "predatory_swiftness_bug", predatory_swiftness_bug ) );
  add_option( opt_int( "initial_berserk_duration", initial_berserk_duration ) );
}

// druid_t::copy_from =======================================================

void druid_t::copy_from( player_t* source )
{
  player_t::copy_from( source );

  druid_t* p = debug_cast<druid_t*>( source );

  alternate_stellar_flare = p -> alternate_stellar_flare;
}

// druid_t::create_proc_action =============================================

action_t* druid_t::create_proc_action( const std::string& name, const special_effect_t& )
{
  if ( util::str_compare_ci( name, "shattered_bleed" ) && specialization() == DRUID_FERAL )
    return new cat_attacks::shattered_bleed_t( this );
  if ( util::str_compare_ci( name, "flurry_of_xuen" ) && specialization() == DRUID_FERAL )
    return new cat_attacks::flurry_of_xuen_t( this );

  return nullptr;
}

// druid_t::create_profile ==================================================

std::string druid_t::create_profile( save_e type )
{
  return player_t::create_profile( type );
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

// druid_t::init_absorb_priority ============================================

void druid_t::init_absorb_priority()
{
  absorb_priority.insert( absorb_priority.begin(), 155784 ); // Primal Tenacity

  player_t::init_absorb_priority();

  absorb_priority.push_back( 184878 ); // Stalwart Guardian
  absorb_priority.push_back( 135597 ); // Tooth & Claw
}

// druid_t::assess_damage ===================================================

void druid_t::assess_damage( school_e school,
                             dmg_e    dtype,
                             action_state_t* s )
{
  /* Store the amount primal_tenacity absorb remaining so we can use it 
    later to determine if we need to apply the absorb. */
  double pt_pre_amount = 0.0;
  if ( mastery.primal_tenacity -> ok() )
    if ( school == SCHOOL_PHYSICAL && // Check attack eligibility
         ! ( s -> result == RESULT_DODGE || s -> result == RESULT_MISS ) )
      pt_pre_amount = buff.primal_tenacity -> value();

  if ( sets.has_set_bonus( SET_TANK, T15, B2 ) && s -> result == RESULT_DODGE && buff.savage_defense -> check() )
    buff.guardian_tier15_2pc -> trigger();

  if ( buff.barkskin -> up() )
    s -> result_amount *= 1.0 + buff.barkskin -> default_value;

  s -> result_amount *= 1.0 + buff.survival_instincts -> value();

  s -> result_amount *= 1.0 + glyph.ninth_life -> effectN( 1 ).base_value();

  s -> result_amount *= 1.0 + buff.bristling_fur -> value();

  s -> result_amount *= 1.0 + buff.pulverize -> value();

  if ( specialization() == DRUID_GUARDIAN && buff.bear_form -> check() )
  {
    if ( buff.savage_defense -> up() && dbc::is_school( SCHOOL_PHYSICAL, school ) )
      s -> result_amount *= 1.0 + buff.savage_defense -> data().effectN( 4 ).percent();

    if ( dbc::get_school_mask( school ) & SCHOOL_MAGIC_MASK )
      s -> result_amount *= 1.0 + spec.thick_hide -> effectN( 1 ).percent();
  }

  player_t::assess_damage( school, dtype, s );

  // Trigger Primal Tenacity
  if ( mastery.primal_tenacity -> ok() )
  {
    if ( school == SCHOOL_PHYSICAL && // Check attack eligibility
         ! ( s -> result == RESULT_DODGE || s -> result == RESULT_MISS ) &&
         ! buff.primal_tenacity -> check() ) 
    {
      double pt_amount = s -> result_mitigated * cache.mastery_value();

      /* Primal Tenacity may only occur if the amount of damage PT absorbed from the triggering attack
          is less than 20% of the size of the new absorb. */
      if ( pt_pre_amount < pt_amount * 0.2 )
      {
        // Subtract the amount absorbed from the triggering attack from the new absorb.
        pt_amount -= pt_pre_amount;

        buff.primal_tenacity -> trigger( 1, pt_amount );
        primal_tenacity_stats -> add_execute( timespan_t::zero(), this );
      }
    }
  }
}

// druid_t::assess_damage_imminent_preabsorb ================================

// Trigger effects based on being hit or taking damage.

void druid_t::assess_damage_imminent_pre_absorb( school_e, dmg_e, action_state_t* s )
{
  if ( buff.cenarion_ward -> up() && s -> result_amount > 0 )
    active.cenarion_ward_hot -> execute();

  if ( buff.moonkin_form -> up() && s -> result_amount > 0 && s -> action -> aoe == 0 )
    buff.empowered_moonkin -> trigger();
}

// druid_t::assess_heal =====================================================

void druid_t::assess_heal( school_e school,
                           dmg_e    dmg_type,
                           action_state_t* s )
{
  s -> result_total *= 1.0 + buff.frenzied_regeneration -> check();

  s -> result_total *= 1.0 + buff.cat_form -> check() * glyph.cat_form -> effectN( 1 ).percent();

  if ( sets.has_set_bonus( DRUID_GUARDIAN, T18, B2 ) && buff.savage_defense -> check() )
  {
    double pct = sets.set( DRUID_GUARDIAN, T18, B2 ) -> effectN( 1 ).percent();

    // Trigger a gain so we can track how much the set bonus helped.
    // The gain is 100% overflow so it doesn't distort charts.
    gain.guardian_tier18_2pc -> add( RESOURCE_HEALTH, 0, s -> result_total * pct );

    s -> result_total *= 1.0 + pct;
  }

  player_t::assess_heal( school, dmg_type, s );
}

druid_td_t::druid_td_t( player_t& target, druid_t& source )
  : actor_target_data_t( &target, &source ),
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
  if ( last_check == sim -> current_time() ) // No need to re-check balance if the time hasn't changed.
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

  last_check = sim -> current_time() - last_check;
  // Subtract current time by the last time we checked to get the amount of time elapsed

  if ( talent.euphoria -> ok() ) // Euphoria speeds up the cycle to 20 seconds.
    last_check *= 2;  //To-do: Check if/how it stacks with astral communion/celestial.
  // Effectively, time moves twice as fast, so we'll just double the amount of time since the last check.

  if ( buff.astral_communion -> up() )
    last_check *= 1 + buff.astral_communion -> data().effectN( 1 ).percent();
  // Similarly, when astral communion is running, we will just multiply elapsed time by 3.

  balance_time += last_check; // Add the amount of elapsed time to balance_time
  last_check = sim -> current_time(); // Set current time for last check.

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

  // phi is the phase, which describes our position in the eclipse cycle, determined by the accumulator balance_time 
  // and frequency omega. Note that Euphoria and other effects are already baked into balance_time, so we don't need
  // to account for them (yet)
  double phi;
  phi = omega * balance_time.total_millis();

  // However.... Euphoria doubles the frequency of the cycle (reduces the period to 20 seconds). Since we're estimating 
  // how much time is left based on the phase difference between phi and our new target phase value, we need to account
  // for that in omega for our estimages. Note that this has to come AFTER the phi definition above!
  if ( talent.euphoria -> ok() )
    omega *= 2;

  // since sin is periodic modulo 2*pi (in other words, sin(x+2*pi)=sin(x) ), we can remove any multiple of 2*pi from phi.
  // This puts phi between 0 and 2*pi, which is convenient
  phi = fmod( phi, 2 * M_PI );
  if ( sim -> debug || sim -> log )
    sim -> out_debug.printf( "Phi: %f", phi );

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

// Copypasta for reporting
bool has_amount_results( const std::array<stats_t::stats_results_t,RESULT_MAX>& res )
{
  return (
      res[ RESULT_HIT ].actual_amount.mean() > 0 ||
      res[ RESULT_CRIT ].actual_amount.mean() > 0 ||
      res[ RESULT_MULTISTRIKE ].actual_amount.mean() > 0 ||
      res[ RESULT_MULTISTRIKE_CRIT ].actual_amount.mean() > 0
  );
}

/* Report Extension Class
 * Here you can define class specific report extensions/overrides
 */
class druid_report_t : public player_report_extension_t
{
public:
  druid_report_t( druid_t& player ) :
      p( player )
  { }

  void feral_snapshot_table( report::sc_html_stream& os )
  {
    // Write header
    os << "<table class=\"sc\">\n"
         << "<tr>\n"
           << "<th >Ability</th>\n"
           << "<th colspan=2>Tiger's Fury</th>\n";
    if ( p.talent.bloodtalons -> ok() )
    {
      os << "<th colspan=2>Bloodtalons</th>\n";
    }
    os << "</tr>\n";

    os << "<tr>\n"
         << "<th>Name</th>\n"
         << "<th>Execute %</th>\n"
         << "<th>Benefit %</th>\n";
    if ( p.talent.bloodtalons -> ok() )
    {
      os << "<th>Execute %</th>\n"
         << "<th>Benefit %</th>\n";
    }
    os << "</tr>\n";

// Compile and Write Contents 
    for ( size_t i = 0, end = p.stats_list.size(); i < end; i++ )
    {
      stats_t* stats = p.stats_list[ i ];
      double tf_exe_up = 0, tf_exe_total = 0;
      double tf_benefit_up = 0, tf_benefit_total = 0;
      double bt_exe_up = 0, bt_exe_total = 0;
      double bt_benefit_up = 0, bt_benefit_total = 0;
      int n = 0;

      for ( size_t j = 0, end2 = stats -> action_list.size(); j < end2; j++ )
      {
        cat_attacks::cat_attack_t* a = dynamic_cast<cat_attacks::cat_attack_t*>( stats -> action_list[ j ] );
        if ( ! a )
          continue;

        if ( ! a -> consume_bloodtalons )
          continue;

        tf_exe_up += a -> tf_counter -> mean_exe_up();
        tf_exe_total += a -> tf_counter -> mean_exe_total();
        tf_benefit_up += a -> tf_counter -> mean_tick_up();
        tf_benefit_total += a -> tf_counter -> mean_tick_total();
        if ( has_amount_results( stats -> direct_results ) )
        {
          tf_benefit_up += a -> tf_counter -> mean_exe_up();
          tf_benefit_total += a -> tf_counter -> mean_exe_total();
        }
        if ( p.talent.bloodtalons -> ok() )
        {
          bt_exe_up += a -> bt_counter -> mean_exe_up();
          bt_exe_total += a -> bt_counter -> mean_exe_total();
          bt_benefit_up += a -> bt_counter -> mean_tick_up();
          bt_benefit_total += a -> bt_counter -> mean_tick_total();
          if ( has_amount_results( stats -> direct_results ) )
          {
            bt_benefit_up += a -> bt_counter -> mean_exe_up();
            bt_benefit_total += a -> bt_counter -> mean_exe_total();
          }
        }
      }

      if ( tf_exe_total > 0 || bt_exe_total > 0 )
      {
        std::string name_str = report::decorated_action_name( stats -> action_list[ 0 ] );
        std::string row_class_str = "";
        if ( ++n & 1 )
          row_class_str = " class=\"odd\"";

        // Table Row : Name, TF up, TF total, TF up/total, TF up/sum(TF up)
        os.format("<tr%s><td class=\"left\">%s</td><td class=\"right\">%.2f %%</td><td class=\"right\">%.2f %%</td>\n",
            row_class_str.c_str(),
            name_str.c_str(),
            util::round( tf_exe_up / tf_exe_total * 100, 2 ),
            util::round( tf_benefit_up / tf_benefit_total * 100, 2 ) );

        if ( p.talent.bloodtalons -> ok() )
        {
          // Table Row : Name, TF up, TF total, TF up/total, TF up/sum(TF up)
          os.format("<td class=\"right\">%.2f %%</td><td class=\"right\">%.2f %%</td>\n",
              util::round( bt_exe_up / bt_exe_total * 100, 2 ),
              util::round( bt_benefit_up / bt_benefit_total * 100, 2 ) );
        }

        os << "</tr>";
      }
      
    }

    os << "</tr>";

    // Write footer
    os << "</table>\n";
  }

  void feral_imp_rake_table( report::sc_html_stream& os )
  {
    // Write header
    os << "<table class=\"sc\">\n"
         << "<tr>\n"
           << "<th colspan=2>Improved Rake</th>\n";
    os << "</tr>\n";

// Compile and Write Contents 
    for ( size_t i = 0, end = p.stats_list.size(); i < end; i++ )
    {
      stats_t* stats = p.stats_list[ i ];
      double ir_exe_up = 0, ir_exe_total = 0;
      double ir_benefit_up = 0, ir_benefit_total = 0;
      double ir_wasted_buffs = 0;
      int n = 0;

      for ( size_t j = 0, end2 = stats -> action_list.size(); j < end2; j++ )
      {
        cat_attacks::rake_t* a = dynamic_cast<cat_attacks::rake_t*>( stats -> action_list[ j ] );
        if ( ! a )
          continue;

        ir_exe_up += a -> ir_counter -> mean_exe_up();
        ir_exe_total += a -> ir_counter -> mean_exe_total();
        ir_benefit_up += a -> ir_counter -> mean_tick_up();
        ir_benefit_total += a -> ir_counter -> mean_tick_total();
        if ( has_amount_results( stats -> direct_results ) )
        {
          ir_benefit_up += a -> ir_counter -> mean_exe_up();
          ir_benefit_total += a -> ir_counter -> mean_exe_total();
        }
        ir_wasted_buffs += a -> ir_counter -> mean_waste();
      }

      if ( ir_exe_total > 0 )
      {
        std::string row_class_str = "";
        if ( ++n & 1 )
          row_class_str = " class=\"odd\"";

        // Table Row : Execute %
        os.format("<tr%s><td class=\"left\">Execute %%</td><td class=\"right\">%.2f %%</td>\n",
            row_class_str.c_str(),
            util::round( ir_exe_up / ir_exe_total * 100, 2 ) );

        // Table Row : Benefit %
        os.format("<tr%s><td class=\"left\">Benefit %%</td><td class=\"right\">%.2f %%</td>\n",
            row_class_str.c_str(),
            util::round( ir_benefit_up / ir_benefit_total * 100, 2 ) );

        // Table Row : Wasted Buffs
        os.format("<tr%s><td class=\"left\">Wasted Buffs</td><td class=\"right\">%.2f</td>\n",
            row_class_str.c_str(),
            util::round( ir_wasted_buffs , 2 ) );

        os << "</tr>";
      }
      
    }

    os << "</tr>";

    // Write footer
    os << "</table>\n";
  }

  virtual void html_customsection( report::sc_html_stream& os ) override
  {
    if ( p.specialization() == DRUID_FERAL )
    {
      os << "<div class=\"player-section custom_section\">\n"
          << "<h3 class=\"toggle open\">Snapshotting Details</h3>\n"
          << "<div class=\"toggle-content\">\n";

          feral_snapshot_table( os );

          if( p.perk.improved_rake -> ok() )
          {
            feral_imp_rake_table( os );
          }

      os << "<div class=\"clear\"></div>\n";
      os << "</div>\n" << "</div>\n";
    }
  }
private:
  druid_t& p;
};

// DRUID MODULE INTERFACE ===================================================

static void do_trinket_init( druid_t*                player,
                             specialization_e         spec,
                             const special_effect_t*& ptr,
                             const special_effect_t&  effect )
{
  // Ensure we have the spell data. This will prevent the trinket effect from working on live
  // Simulationcraft. Also ensure correct specialization.
  if ( ! player -> find_spell( effect.spell_id ) -> ok() ||
       player -> specialization() != spec )
  {
    return;
  }

  // Set pointer, module considers non-null pointer to mean the effect is "enabled"
  ptr = &( effect );
}

// Balance T18 (WoD 6.2) trinket effect
static void starshards( special_effect_t& effect )
{
  druid_t* s = debug_cast<druid_t*>( effect.player );
  do_trinket_init( s, DRUID_BALANCE, s -> starshards, effect );
}

// Feral T18 (WoD 6.2) trinket effect
static void wildcat_celerity( special_effect_t& effect )
{
  druid_t* s = debug_cast<druid_t*>( effect.player );
  do_trinket_init( s, DRUID_FERAL, s -> wildcat_celerity, effect );
}

// Guardian T18 (WoD 6.2) trinket effect
static double stalwart_guardian_handler( const action_state_t* s )
{
  druid_t* p = static_cast<druid_t*>( s -> target );
  assert( p -> active.stalwart_guardian );
  assert( s );

  // Pass incoming damage value so the absorb can be calculated.
  // TOCHECK: Does this use result_amount or result_mitigated?
  p -> active.stalwart_guardian -> incoming_damage = s -> result_mitigated;
  // Pass the triggering enemy so that the damage reflect has a target;
  p -> active.stalwart_guardian -> triggering_enemy = s -> action -> player;
  p -> active.stalwart_guardian -> execute();

  return p -> active.stalwart_guardian -> absorb_size;
}

static void stalwart_guardian( special_effect_t& effect )
{
  druid_t* s = debug_cast<druid_t*>( effect.player );
  do_trinket_init( s, DRUID_GUARDIAN, s -> stalwart_guardian, effect );

  if ( !s -> stalwart_guardian )
  {
    return;
  }

  effect.player -> instant_absorb_list[ 184878 ] =
    new instant_absorb_t( s, s -> find_spell( 184878 ), "stalwart_guardian", &stalwart_guardian_handler );
}

// Restoration T18 (WoD 6.2) trinket effect
static void flourish( special_effect_t& effect )
{
  druid_t* s = debug_cast<druid_t*>( effect.player );
  do_trinket_init( s, DRUID_RESTORATION, s -> flourish, effect );
}

struct druid_module_t : public module_t
{
  druid_module_t() : module_t( DRUID ) {}

  virtual player_t* create_player( sim_t* sim, const std::string& name, race_e r = RACE_NONE ) const override
  {
    auto  p = new druid_t( sim, name, r );
    p -> report_extension = std::unique_ptr<player_report_extension_t>( new druid_report_t( *p ) );
    return p;
  }
  virtual bool valid() const override { return true; }
  virtual void init( player_t* p ) const override
  {
    p -> buffs.stampeding_roar = buff_creator_t( p, "stampeding_roar", p -> find_spell( 77764 ) )
                                 .max_stack( 1 )
                                 .duration( timespan_t::from_seconds( 8.0 ) );
  }

  virtual void static_init() const override
  {
    unique_gear::register_special_effect( 184876, starshards );
    unique_gear::register_special_effect( 184877, wildcat_celerity );
    unique_gear::register_special_effect( 184878, stalwart_guardian );
    unique_gear::register_special_effect( 184879, flourish );
  } 

  virtual void register_hotfixes() const override
  {
  }

  virtual void combat_begin( sim_t* ) const override {}
  virtual void combat_end( sim_t* ) const override {}
};

} // UNNAMED NAMESPACE

const module_t* module_t::druid()
{
  static druid_module_t m;
  return &m;
}
